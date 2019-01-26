
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#ifndef ROCKSDB_LITE

#include <atomic>
#include <condition_variable>
#include <limits>
#include <list>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db/db_iter.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/db.h"
#include "rocksdb/listener.h"
#include "rocksdb/options.h"
#include "rocksdb/wal_filter.h"
#include "util/mpsc.h"
#include "util/mutexlock.h"
#include "util/timer_queue.h"
#include "utilities/blob_db/blob_db.h"
#include "utilities/blob_db/blob_file.h"
#include "utilities/blob_db/blob_log_format.h"
#include "utilities/blob_db/blob_log_reader.h"
#include "utilities/blob_db/blob_log_writer.h"

namespace rocksdb {

class DBImpl;
class ColumnFamilyHandle;
class ColumnFamilyData;
struct FlushJobInfo;

namespace blob_db {

class BlobFile;
class BlobDBImpl;

class BlobDBFlushBeginListener : public EventListener {
 public:
  explicit BlobDBFlushBeginListener() : impl_(nullptr) {}

  void OnFlushBegin(DB* db, const FlushJobInfo& info) override;

  void SetImplPtr(BlobDBImpl* p) { impl_ = p; }

 protected:
  BlobDBImpl* impl_;
};

//这实现了从Wal的回调，从而确保
//Blob记录存在于Blob日志中。如果fsync/fdatasync不在
//每次写入时，都有可能在
//Blob日志可以延迟Blob中的键
class BlobReconcileWalFilter : public WalFilter {
 public:
  virtual WalFilter::WalProcessingOption LogRecordFound(
      unsigned long long log_number, const std::string& log_file_name,
      const WriteBatch& batch, WriteBatch* new_batch,
      bool* batch_changed) override;

  virtual const char* Name() const override { return "BlobDBWalReconciler"; }

  void SetImplPtr(BlobDBImpl* p) { impl_ = p; }

 protected:
  BlobDBImpl* impl_;
};

class EvictAllVersionsCompactionListener : public EventListener {
 public:
  class InternalListener : public CompactionEventListener {
    friend class BlobDBImpl;

   public:
    virtual void OnCompaction(int level, const Slice& key,
                              CompactionListenerValueType value_type,
                              const Slice& existing_value,
                              const SequenceNumber& sn, bool is_new) override;

    void SetImplPtr(BlobDBImpl* p) { impl_ = p; }

   private:
    BlobDBImpl* impl_;
  };

  explicit EvictAllVersionsCompactionListener()
      : internal_listener_(new InternalListener()) {}

  virtual CompactionEventListener* GetCompactionEventListener() override {
    return internal_listener_.get();
  }

  void SetImplPtr(BlobDBImpl* p) { internal_listener_->SetImplPtr(p); }

 private:
  std::unique_ptr<InternalListener> internal_listener_;
};

#if 0
class EvictAllVersionsFilterFactory : public CompactionFilterFactory {
 private:
  BlobDBImpl* impl_;

 public:
  EvictAllVersionsFilterFactory() : impl_(nullptr) {}

  void SetImplPtr(BlobDBImpl* p) { impl_ = p; }

  virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) override;

  virtual const char* Name() const override {
    return "EvictAllVersionsFilterFactory";
  }
};
#endif

//比较器根据“ttl”感知的blob文件的较低值对其进行排序
//TTL范围。
struct blobf_compare_ttl {
  bool operator()(const std::shared_ptr<BlobFile>& lhs,
                  const std::shared_ptr<BlobFile>& rhs) const;
};

struct GCStats {
  uint64_t blob_count = 0;
  uint64_t num_deletes = 0;
  uint64_t deleted_size = 0;
  uint64_t retry_delete = 0;
  uint64_t delete_succeeded = 0;
  uint64_t overwritten_while_delete = 0;
  uint64_t num_relocate = 0;
  uint64_t retry_relocate = 0;
  uint64_t relocate_succeeded = 0;
  uint64_t overwritten_while_relocate = 0;
  std::shared_ptr<BlobFile> newfile = nullptr;
};

/*
 *blobdb的实现类。这将管理价值
 *部分TTL感知顺序写入文件。这些文件是
 *垃圾收集。
 **/

class BlobDBImpl : public BlobDB {
  friend class BlobDBFlushBeginListener;
  friend class EvictAllVersionsCompactionListener;
  friend class BlobDB;
  friend class BlobFile;
  friend class BlobDBIterator;

 public:
//删除检查期间
  static constexpr uint32_t kDeleteCheckPeriodMillisecs = 2 * 1000;

//每个检查周期的GC百分比
  static constexpr uint32_t kGCFilePercentage = 100;

//气相色谱周期
  static constexpr uint32_t kGCCheckPeriodMillisecs = 60 * 1000;

//健康检查任务
  static constexpr uint32_t kSanityCheckPeriodMillisecs = 20 * 60 * 1000;

//我们能容忍多少随机访问打开的文件
  static constexpr uint32_t kOpenFilesTrigger = 100;

//我们要保留多少个统计周期。
  static constexpr uint32_t kWriteAmplificationStatsPeriods = 24;

//任何周期的长度是多少
  static constexpr uint32_t kWriteAmplificationStatsPeriodMillisecs =
      3600 * 1000;

//我们将垃圾收集blob文件
//哪些文件已过期。但是如果
//TTL文件的范围非常大，比如说一天，我们
//我们得等一整天
//恢复大部分空间。
  static constexpr uint32_t kPartialExpirationGCRangeSecs = 4 * 3600;

//这应该基于允许的写放大
//如果BLOB文件的50%空间已被删除/过期，
  static constexpr uint32_t kPartialExpirationPercentage = 75;

//我们应该多长时间安排一个作业来同步打开的文件
  static constexpr uint32_t kFSyncFilesPeriodMillisecs = 10 * 1000;

//计划回收打开的文件的频率。
  static constexpr uint32_t kReclaimOpenFilesPeriodMillisecs = 1 * 1000;

//计划删除OBS文件周期的频率
  static constexpr uint32_t kDeleteObsoleteFilesPeriodMillisecs = 10 * 1000;

//计划检查seq文件周期的频率
  static constexpr uint32_t kCheckSeqFilesPeriodMillisecs = 10 * 1000;

//何时应收回最旧的文件：
//达到blob目录大小的90%
  static constexpr double kEvictOldestFileAtSize = 0.9;

  using BlobDB::Put;
  Status Put(const WriteOptions& options, const Slice& key,
             const Slice& value) override;

  using BlobDB::Delete;
  Status Delete(const WriteOptions& options, const Slice& key) override;

  using BlobDB::Get;
  Status Get(const ReadOptions& read_options, ColumnFamilyHandle* column_family,
             const Slice& key, PinnableSlice* value) override;

  using BlobDB::NewIterator;
  virtual Iterator* NewIterator(const ReadOptions& read_options) override;

  using BlobDB::NewIterators;
  virtual Status NewIterators(
      const ReadOptions& read_options,
      const std::vector<ColumnFamilyHandle*>& column_families,
      std::vector<Iterator*>* iterators) override {
    return Status::NotSupported("Not implemented");
  }

  using BlobDB::MultiGet;
  virtual std::vector<Status> MultiGet(
      const ReadOptions& read_options,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override;

  virtual Status Write(const WriteOptions& opts, WriteBatch* updates) override;

  virtual Status GetLiveFiles(std::vector<std::string>&,
                              uint64_t* manifest_file_size,
                              bool flush_memtable = true) override;
  virtual void GetLiveFilesMetaData(
      std::vector<LiveFileMetaData>* ) override;

  using BlobDB::PutWithTTL;
  Status PutWithTTL(const WriteOptions& options, const Slice& key,
                    const Slice& value, uint64_t ttl) override;

  using BlobDB::PutUntil;
  Status PutUntil(const WriteOptions& options, const Slice& key,
                  const Slice& value, uint64_t expiration) override;

  Status LinkToBaseDB(DB* db) override;

  BlobDBOptions GetBlobDBOptions() const override;

  BlobDBImpl(DB* db, const BlobDBOptions& bdb_options);

  BlobDBImpl(const std::string& dbname, const BlobDBOptions& bdb_options,
             const DBOptions& db_options);

  ~BlobDBImpl();

#ifndef NDEBUG
  Status TEST_GetBlobValue(const Slice& key, const Slice& index_entry,
                           PinnableSlice* value);

  std::vector<std::shared_ptr<BlobFile>> TEST_GetBlobFiles() const;

  std::vector<std::shared_ptr<BlobFile>> TEST_GetObsoleteFiles() const;

  Status TEST_CloseBlobFile(std::shared_ptr<BlobFile>& bfile);

  Status TEST_GCFileAndUpdateLSM(std::shared_ptr<BlobFile>& bfile,
                                 GCStats* gc_stats);

  void TEST_RunGC();

  void TEST_DeleteObsoleteFiles();
#endif  //！调试程序

 private:
  class GarbageCollectionWriteCallback;
  class BlobInserter;

  Status OpenPhase1();

//如果在读取选项中没有快照，则创建快照。
//如果创建了快照，则返回true。
  bool SetSnapshotIfNeeded(ReadOptions* read_options);

  Status GetBlobValue(const Slice& key, const Slice& index_entry,
                      PinnableSlice* value);

  Slice GetCompressedSlice(const Slice& raw,
                           std::string* compression_output) const;

//在flush开始对memtable文件执行操作之前，
//调用此处理程序。
  void OnFlushBeginHandler(DB* db, const FlushJobInfo& info);

//这个文件准备好垃圾收集了吗？如果文件的TTL
//已过期或文件的阈值已被逐出
//tt-当前时间
//Last_ID-要退出的非TTL文件的ID
  bool ShouldGCFile(std::shared_ptr<BlobFile> bfile, uint64_t now,
                    bool is_oldest_non_ttl_file, std::string* reason);

//从blob目录收集所有blob日志文件
  Status GetAllLogFiles(std::set<std::pair<uint64_t, std::string>>* file_nums);

//通过附加页脚关闭文件，并从“打开的文件”列表中删除文件。
  Status CloseBlobFile(std::shared_ptr<BlobFile> bfile);

//如果文件大小超过blob文件大小，则关闭该文件
  Status CloseBlobFileIfNeeded(std::shared_ptr<BlobFile>& bfile);

  uint64_t ExtractExpiration(const Slice& key, const Slice& value,
                             Slice* value_slice, std::string* new_value);

  Status PutBlobValue(const WriteOptions& options, const Slice& key,
                      const Slice& value, uint64_t expiration,
                      SequenceNumber sequence, WriteBatch* batch);

  Status AppendBlob(const std::shared_ptr<BlobFile>& bfile,
                    const std::string& headerbuf, const Slice& key,
                    const Slice& value, uint64_t expiration,
                    std::string* index_entry);

//根据过期的unix epoch查找现有的blob日志文件
//如果这样的文件不存在，则返回nullptr
  std::shared_ptr<BlobFile> SelectBlobFileTTL(uint64_t expiration);

//查找要将值附加到的现有blob日志文件
  std::shared_ptr<BlobFile> SelectBlobFile();

  std::shared_ptr<BlobFile> FindBlobFileLocked(uint64_t expiration) const;

  void Shutdown();

//定期进行健康检查。一串支票
  std::pair<bool, int64_t> SanityCheck(bool aborted);

//删除已被垃圾收集和标记的文件
//过时的。检查是否存在引用
//相同的
  std::pair<bool, int64_t> DeleteObsoleteFiles(bool aborted);

//垃圾收集过期和删除的Blob的主要任务
  std::pair<bool, int64_t> RunGC(bool aborted);

//异步任务以fsync/fdata同步打开的blob文件
  std::pair<bool, int64_t> FsyncFiles(bool aborted);

//定期检查打开的blob文件及其ttl是否过期
//如果过期，请关闭顺序写入程序并使文件不可变
  std::pair<bool, int64_t> CheckSeqFiles(bool aborted);

//如果打开的文件数接近ulimit，这是
//任务将关闭随机读卡器，这些读卡器保留在
//效率
  std::pair<bool, int64_t> ReclaimOpenFiles(bool aborted);

//定期打印写放大统计
  std::pair<bool, int64_t> WaStats(bool aborted);

//后台任务：对已删除的密钥进行簿记
  std::pair<bool, int64_t> EvictDeletions(bool aborted);

  std::pair<bool, int64_t> EvictCompacted(bool aborted);

  std::pair<bool, int64_t> RemoveTimerQ(TimerQueue* tq, bool aborted);

//将后台任务添加到计时器队列
  void StartBackgroundTasks();

//添加新的blob文件
  std::shared_ptr<BlobFile> NewBlobFile(const std::string& reason);

  Status OpenAllFiles();

//在文件上保持写互斥并调用
//为get调用创建随机访问读取器
  std::shared_ptr<RandomAccessFileReader> GetOrOpenRandomAccessReader(
      const std::shared_ptr<BlobFile>& bfile, Env* env,
      const EnvOptions& env_options);

//在文件上保持写互斥并调用。
//关闭上述随机访问读卡器
  void CloseRandomAccessLocked(const std::shared_ptr<BlobFile>& bfile);

//在文件上保持写互斥并调用
//为此blobfile创建顺序（追加）编写器
  Status CreateWriterLocked(const std::shared_ptr<BlobFile>& bfile);

//返回文件的编写器对象。如果作者不是
//已经存在，创建一个。需要保持写互斥
  std::shared_ptr<Writer> CheckOrCreateWriterLocked(
      const std::shared_ptr<BlobFile>& bfile);

//迭代blob上的键和值并写入
//将文件与剩余的blob分开并删除/更新指针
//原子级LSM
  Status GCFileAndUpdateLSM(const std::shared_ptr<BlobFile>& bfptr,
                            GCStats* gcstats);

//检查是否没有引用
//斑点
  bool VisibleToActiveSnapshot(const std::shared_ptr<BlobFile>& file);
  bool FileDeleteOk_SnapshotCheckLocked(const std::shared_ptr<BlobFile>& bfile);

  bool MarkBlobDeleted(const Slice& key, const Slice& lsmValue);

  bool FindFileAndEvictABlob(uint64_t file_number, uint64_t key_size,
                             uint64_t blob_offset, uint64_t blob_size);

  void CopyBlobFiles(
      std::vector<std::shared_ptr<BlobFile>>* bfiles_copy,
      std::function<bool(const std::shared_ptr<BlobFile>&)> predicate = {});

  void FilterSubsetOfFiles(
      const std::vector<std::shared_ptr<BlobFile>>& blob_files,
      std::vector<std::shared_ptr<BlobFile>>* to_process, uint64_t epoch,
      size_t files_to_collect);

  uint64_t EpochNow() { return env_->NowMicros() / 1000000; }

  Status CheckSize(size_t blob_size);

  std::shared_ptr<BlobFile> GetOldestBlobFile();

  bool EvictOldestBlobFile();

//基础数据库
  DBImpl* db_impl_;
  Env* env_;
  TTLExtractor* ttl_extractor_;

//控制Blob存储行为的选项
  BlobDBOptions bdb_options_;
  DBOptions db_options_;
  EnvOptions env_options_;

//数据库目录的名称
  std::string dbname_;

//默认情况下，这是dbname_下的“blob_dir”
//但可以配置
  std::string blob_dir_;

//指向目录的指针
  std::unique_ptr<Directory> dir_ent_;

  std::atomic<bool> dir_change_;

//读写互斥，保护所有数据结构
//大量贩运
  mutable port::RWMutex mutex_;

//写作者必须在写之前保持写互斥。
  mutable port::Mutex write_mutex_;

//Blob文件号计数器
  std::atomic<uint64_t> next_file_number_;

//所有blob文件内存的整个元数据
  std::map<uint64_t, std::shared_ptr<BlobFile>> blob_files_;

//打开文件的时代或版本。
  std::atomic<uint64_t> epoch_of_;

//打开了非TTL blob文件。
  std::shared_ptr<BlobFile> open_non_ttl_file_;

//当前附加到的所有blob文件基于
//关于输入TTL的变化
  std::multiset<std::shared_ptr<BlobFile>, blobf_compare_ttl> open_ttl_files_;

//要放入无锁删除队列的信息包
  struct delete_packet_t {
    ColumnFamilyHandle* cfh_;
    std::string key_;
    SequenceNumber dsn_;
  };

  struct override_packet_t {
    uint64_t file_number_;
    uint64_t key_size_;
    uint64_t blob_offset_;
    uint64_t blob_size_;
    SequenceNumber dsn_;
  };

//无锁多个生产者单个使用者队列以快速附加
//删除而不锁定。尺寸可以快速增长！！
//删除在LSM中发生，但需要在
//Blob端（用于触发逐出）
  mpsc_queue_t<delete_packet_t> delete_keys_q_;

//值的无锁多生产者单消费者队列
//正在被压缩
  mpsc_queue_t<override_packet_t> override_vals_q_;

//表示关闭的原子布尔
  std::atomic<bool> shutdown_;

//执行任务的基于计时器的队列
  TimerQueue tqueue_;

//只在GC线程中访问，因此不是原子的。世纪
//GC任务。每次执行都是一个时代。帮助我们分配
//文件到一次执行
  uint64_t current_epoch_;

//为随机访问/获取而打开的文件数
//计数器用于监视和关闭多余的RA文件。
  std::atomic<uint32_t> open_file_count_;

//应保持mutex以进行修改
//由于gc导致的blob文件的wa统计信息
//默认收取24小时
  std::list<uint64_t> all_periods_write_;
  std::list<uint64_t> all_periods_ampl_;

  std::atomic<uint64_t> last_period_write_;
  std::atomic<uint64_t> last_period_ampl_;

  uint64_t total_periods_write_;
  uint64_t total_periods_ampl_;

//给定时间内所有blob文件的总大小
  std::atomic<uint64_t> total_blob_space_;
  std::list<std::shared_ptr<BlobFile>> obsolete_files_;
  bool open_p1_done_;

  uint32_t debug_level_;

  std::atomic<bool> oldest_file_evicted_;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
