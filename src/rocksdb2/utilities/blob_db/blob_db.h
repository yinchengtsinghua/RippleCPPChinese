
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

#include <functional>
#include <string>
#include <vector>
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/stackable_db.h"

namespace rocksdb {

namespace blob_db {

class TTLExtractor;

//一个封装的数据库，将kv对的值放在单独的日志中。
//并将位置存储到底层数据库中的日志中。
//它缺乏许多重要的功能，例如数据库重启，
//垃圾收集、迭代器等。
//
//工厂需要搬迁到包括/rocksdb/公用设施
//使用Blob数据库的用户。

struct BlobDBOptions {
//主数据库下存储blob的目录的名称。
//默认为“blob_dir”
  std::string blob_dir = "blob_dir";

//blob目录路径是相对路径还是绝对路径。
  bool path_relative = true;

//驱逐策略是否基于FIFO？
  bool is_fifo = false;

//blob dir的最大大小。一旦这个用完了，
//逐出最旧的blob文件（is_fifo）
//0表示无限制
  uint64_t blob_dir_size = 0;

//一个新的桶被打开，用于TTL范围。所以，如果TTL范围是600秒
//（10分钟），第一个桶从1471542000开始
//那么斑点桶将是
//第一个桶是147154200-1471542600
//第二个桶是1471542600-1471543200
//等等
  uint64_t ttl_range_secs = 3600;

//要存储在blob日志中的最小值。大于此阈值的值
//将与键一起在基数据库中内联。
  uint64_t min_blob_size = 0;

//blob文件将以什么字节同步到blob日志。
  uint64_t bytes_per_sync = 0;

//每个blob文件的目标大小。文件将变得不可变
//超过这个尺寸后
  uint64_t blob_file_size = 256 * 1024 * 1024;

//不是通过调用putWithTTL或putUntil显式设置ttl，
//应用程序可以设置一个可以从键值提取TTL的ttlextractor。
//对。
  std::shared_ptr<TTLExtractor> ttl_extractor = nullptr;

//用于blob的压缩
  CompressionType compression = kNoCompression;

//如果启用，blob db会通过重写剩余的数据定期清理过时的数据
//将blob文件中的实时数据转换为新文件。如果未启用垃圾收集，
//将基于TTL清理Blob文件。
  bool enable_garbage_collection = false;

//禁用所有后台作业。仅用于测试。
  bool disable_background_tasks = false;

  void Dump(Logger* log) const;
};

class BlobDB : public StackableDB {
 public:
  using rocksdb::StackableDB::Put;
  virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& value) override = 0;
  virtual Status Put(const WriteOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& value) override {
    if (column_family != DefaultColumnFamily()) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    return Put(options, key, value);
  }

  using rocksdb::StackableDB::Delete;
  virtual Status Delete(const WriteOptions& options,
                        const Slice& key) override = 0;
  virtual Status Delete(const WriteOptions& options,
                        ColumnFamilyHandle* column_family,
                        const Slice& key) override {
    if (column_family != DefaultColumnFamily()) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    return Delete(options, key);
  }

  virtual Status PutWithTTL(const WriteOptions& options, const Slice& key,
                            const Slice& value, uint64_t ttl) = 0;
  virtual Status PutWithTTL(const WriteOptions& options,
                            ColumnFamilyHandle* column_family, const Slice& key,
                            const Slice& value, uint64_t ttl) {
    if (column_family != DefaultColumnFamily()) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    return PutWithTTL(options, key, value, ttl);
  }

//放在到期日。过期时间等于的密钥
//std:：numeric_limits<uint64_t>：：max（）表示密钥不会过期。
  virtual Status PutUntil(const WriteOptions& options, const Slice& key,
                          const Slice& value, uint64_t expiration) = 0;
  virtual Status PutUntil(const WriteOptions& options,
                          ColumnFamilyHandle* column_family, const Slice& key,
                          const Slice& value, uint64_t expiration) {
    if (column_family != DefaultColumnFamily()) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    return PutUntil(options, key, value, expiration);
  }

  using rocksdb::StackableDB::Get;
  virtual Status Get(const ReadOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     PinnableSlice* value) override = 0;

  using rocksdb::StackableDB::MultiGet;
  virtual std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override = 0;
  virtual std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_families,
      const std::vector<Slice>& keys,
      std::vector<std::string>* values) override {
    for (auto column_family : column_families) {
      if (column_family != DefaultColumnFamily()) {
        return std::vector<Status>(
            column_families.size(),
            Status::NotSupported(
                "Blob DB doesn't support non-default column family."));
      }
    }
    return MultiGet(options, keys, values);
  }

  using rocksdb::StackableDB::SingleDelete;
  /*实际状态单删除（const write选项&/*wopts*/，
                              column系列handle*/*column系列*/,

                              /*st slice&/*key*/）override
    返回状态：：NotSupported（“Blob数据库中不支持的操作”）；
  }

  使用rocksdb:：stackabledb:：merge；
  虚拟状态合并（const writeoptions&/*选项*/,

                       /*umnfamilyhandle*/*column_family*/，
                       常量切片&/*ke*/, const Slice& /*value*/) override {

    return Status::NotSupported("Not supported operation in blob db.");
  }

  virtual Status Write(const WriteOptions& opts,
                       WriteBatch* updates) override = 0;

  using rocksdb::StackableDB::NewIterator;
  virtual Iterator* NewIterator(const ReadOptions& options) override = 0;
  virtual Iterator* NewIterator(const ReadOptions& options,
                                ColumnFamilyHandle* column_family) override {
    if (column_family != DefaultColumnFamily()) {
//Blob数据库不支持非默认列族。
      return nullptr;
    }
    return NewIterator(options);
  }

//打开Blob数据库的起点。
//已更改的选项-关键。blob db加载和插入侦听器
//到恢复和原子性所必需的选项中
//如果需要控制步骤2，即
//basedb不仅是一个简单的rocksdb，而且是一个堆叠的db
//1。：打开和加载
//2。用更改的选项打开基本数据库
//三。：LinkToBaseDB
  static Status OpenAndLoad(const Options& options,
                            const BlobDBOptions& bdb_options,
                            const std::string& dbname, BlobDB** blob_db,
                            Options* changed_options);

//这是另一种打开blob数据库的方法，它没有其他的
//可堆叠DB正在播放
//步骤。
//1。：：打开
  static Status Open(const Options& options, const BlobDBOptions& bdb_options,
                     const std::string& dbname, BlobDB** blob_db);

  static Status Open(const DBOptions& db_options,
                     const BlobDBOptions& bdb_options,
                     const std::string& dbname,
                     const std::vector<ColumnFamilyDescriptor>& column_families,
                     std::vector<ColumnFamilyHandle*>* handles,
                     BlobDB** blob_db, bool no_base_db = false);

  virtual BlobDBOptions GetBlobDBOptions() const = 0;

  virtual ~BlobDB() {}

  virtual Status LinkToBaseDB(DB* db_base) = 0;

 protected:
  explicit BlobDB(DB* db);
};

//销毁数据库的内容。
Status DestroyBlobDB(const std::string& dbname, const Options& options,
                     const BlobDBOptions& bdb_options);

//ttlextractor允许应用程序从键值对中提取TTL。
//这对于使用PUT或WRITEBATCH写入密钥和
//不打算迁移到PutWithTTL或PutUntil。
//
//应用程序可以实现ExtractTTL或ExtractExpiration。如果两者
//如果实现，则以ExtractExpiration为准。
class TTLExtractor {
 public:
//从键值对中提取TTL。
//如果键具有TTL，则返回“真”，否则返回“假”。如果密钥有TTL，
//TTL是通过TTL返回的。方法可以选择修改值，
//将结果传递回新的_值，并将值_更改为true。
  virtual bool ExtractTTL(const Slice& key, const Slice& value, uint64_t* ttl,
                          std::string* new_value, bool* value_changed);

//从键值对中提取到期时间。
//如果密钥有过期时间，则返回true，否则返回false。如果KEY有
//过期时间，它是通过过期传递回来的。方法可以
//或者修改该值，通过新的_值返回结果，
//并将值设置为“真”。
  virtual bool ExtractExpiration(const Slice& key, const Slice& value,
                                 uint64_t now, uint64_t* expiration,
                                 std::string* new_value, bool* value_changed);

  virtual ~TTLExtractor() = default;
};

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
