
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
#include <memory>

#include "port/port.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "util/file_reader_writer.h"
#include "utilities/blob_db/blob_log_format.h"
#include "utilities/blob_db/blob_log_reader.h"
#include "utilities/blob_db/blob_log_writer.h"

namespace rocksdb {
namespace blob_db {

class BlobDBImpl;

class BlobFile {
  friend class BlobDBImpl;
  friend struct blobf_compare_ttl;

 private:
//访问父级
  const BlobDBImpl* parent_;

//Blob目录的路径
  std::string path_to_dir_;

//文件的ID。
//上面的2是在文件创建过程中创建的，并且从未更改过
//之后
  uint64_t file_number_;

//如果为真，则此文件中的键都具有TTL。否则所有钥匙都不能
//有TTL。
  bool has_ttl_;

//文件中blob的压缩类型
  CompressionType compression_;

//文件中的blob数
  std::atomic<uint64_t> blob_count_;

//该文件将在今后的时间段内选择用于GC。
  std::atomic<int64_t> gc_epoch_;

//文件大小
  std::atomic<uint64_t> file_size_;

//此特定文件中已被逐出的Blob数
  uint64_t deleted_count_;

//已删除blob的大小（启发式方法用于为gc选择文件）
  uint64_t deleted_size_;

  BlobLogHeader header_;

//closed_=true表示文件不再可变
//不会再追加更多的blob，并且页脚已被写出
  std::atomic<bool> closed_;

//已在此文件上成功完成垃圾收集传递
//过时的仍然需要进行迭代器/快照检查
  std::atomic<bool> obsolete_;

//文件标记为已过时时的最后一个序列号。
//此文件中的数据对序列之前拍摄的快照可见。
  SequenceNumber obsolete_sequence_;

//如果这个文件被GC处理一次以协调丢失的删除/压缩
  std::atomic<bool> gc_once_after_open_;

  ExpirationRange expiration_range_;

  SequenceRange sequence_range_;

//blob的顺序/附加编写器
  std::shared_ptr<Writer> log_writer_;

//用于GET调用的随机访问文件读取器
  std::shared_ptr<RandomAccessFileReader> ra_file_reader_;

//此读写互斥体是每个文件特定的，并保护
//所有数据结构
  mutable port::RWMutex mutex_;

//上次创建随机访问读取器的时间。
  std::atomic<std::int64_t> last_access_;

//上次文件是fsync'd/fdata同步
  std::atomic<uint64_t> last_fsync_;

  bool header_valid_;

  SequenceNumber garbage_collection_finish_sequence_;

 public:
  BlobFile();

  BlobFile(const BlobDBImpl* parent, const std::string& bdir, uint64_t fnum);

  ~BlobFile();

  uint32_t column_family_id() const;

//返回日志文件相对于主db dir的路径名
//例如，对于实时日志文件=blob_dir/000003.blob
  std::string PathName() const;

//Blob文件的主标识符。
//一旦文件被创建，这永远不会改变
  uint64_t BlobFileNumber() const { return file_number_; }

//以下函数是原子函数，不需要
//读锁
  uint64_t BlobCount() const {
    return blob_count_.load(std::memory_order_acquire);
  }

  std::string DumpState() const;

//如果文件已通过GC并且blob已重新定位
  bool Obsolete() const {
    assert(Immutable() || !obsolete_.load());
    return obsolete_.load();
  }

//将文件标记为垃圾回收已过时。文件对不可见
//序列大于或等于给定序列的快照。
  void MarkObsolete(SequenceNumber sequence);

  SequenceNumber GetObsoleteSequence() const {
    assert(Obsolete());
    return obsolete_sequence_;
  }

//如果文件不再带任何附件。
  bool Immutable() const { return closed_.load(); }

//我们假设这是原子的
  bool NeedsFsync(bool hard, uint64_t bytes_per_sync) const;

  void Fsync();

  uint64_t GetFileSize() const {
    return file_size_.load(std::memory_order_acquire);
  }

//所有非原子的get函数在互斥体上都需要readlock

  ExpirationRange GetExpirationRange() const { return expiration_range_; }

  void ExtendExpirationRange(uint64_t expiration) {
    expiration_range_.first = std::min(expiration_range_.first, expiration);
    expiration_range_.second = std::max(expiration_range_.second, expiration);
  }

  SequenceRange GetSequenceRange() const { return sequence_range_; }

  void SetSequenceRange(SequenceRange sequence_range) {
    sequence_range_ = sequence_range;
  }

  void ExtendSequenceRange(SequenceNumber sequence) {
    sequence_range_.first = std::min(sequence_range_.first, sequence);
    sequence_range_.second = std::max(sequence_range_.second, sequence);
  }

  bool HasTTL() const { return has_ttl_; }

  void SetHasTTL(bool has_ttl) { has_ttl_ = has_ttl; }

  CompressionType compression() const { return compression_; }

  void SetCompression(CompressionType c) {
    compression_ = c;
  }

  std::shared_ptr<Writer> GetWriter() const { return log_writer_; }

 private:
  std::shared_ptr<Reader> OpenSequentialReader(
      Env* env, const DBOptions& db_options,
      const EnvOptions& env_options) const;

  Status ReadFooter(BlobLogFooter* footer);

  Status WriteFooterAndCloseLocked();

  std::shared_ptr<RandomAccessFileReader> GetOrOpenRandomAccessReader(
      Env* env, const EnvOptions& env_options, bool* fresh_open);

  void CloseRandomAccessLocked();

//当您只阅读
//以前关闭的文件
  Status SetFromFooterLocked(const BlobLogFooter& footer);

  void set_expiration_range(const ExpirationRange& expiration_range) {
    expiration_range_ = expiration_range;
  }

//以下函数是原子函数，不需要锁
  void SetFileSize(uint64_t fs) { file_size_ = fs; }

  void SetBlobCount(uint64_t bc) { blob_count_ = bc; }
};
}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
