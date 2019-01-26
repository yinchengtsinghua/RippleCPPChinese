
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//具有为所有键生成的二进制可搜索索引的WriteBatchWithIndex
//插入的。
#pragma once

#ifndef ROCKSDB_LITE

#include <memory>
#include <string>

#include "rocksdb/comparator.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_batch_base.h"

namespace rocksdb {

class ColumnFamilyHandle;
class Comparator;
class DB;
struct ReadOptions;
struct DBOptions;

enum WriteType {
  kPutRecord,
  kMergeRecord,
  kDeleteRecord,
  kSingleDeleteRecord,
  kDeleteRangeRecord,
  kLogDataRecord,
  kXIDRecord,
};

//用于写入批处理的Put、Merge、Delete或SingleDelete项。
//在wbwiiterator中使用。
struct WriteEntry {
  WriteType type;
  Slice key;
  Slice value;
};

//WriteBatchWithIndex中一个列族的迭代器。
class WBWIIterator {
 public:
  virtual ~WBWIIterator() {}

  virtual bool Valid() const = 0;

  virtual void SeekToFirst() = 0;

  virtual void SeekToLast() = 0;

  virtual void Seek(const Slice& key) = 0;

  virtual void SeekForPrev(const Slice& key) = 0;

  virtual void Next() = 0;

  virtual void Prev() = 0;

//返回的writeEntry仅在下一个
//带索引的WriteBatchWithIndex
  virtual WriteEntry Entry() const = 0;

  virtual Status status() const = 0;
};

//具有为所有键生成的二进制可搜索索引的WriteBatchWithIndex
//插入的。
//在put（）、merge（）delete（）或singledelete（）中，与
//将调用Wrapped。同时，将建立索引。
//通过调用get writebatch（），用户将获得数据的writebatch
//它们插入，可用于db:：write（）。
//用户可以调用newIterator（）来创建迭代器。
class WriteBatchWithIndex : public WriteBatchBase {
 public:
//备份比较器：用于比较键的备份比较器。
//在同一个列族中，如果在
//接口，或者在列族句柄中找不到列族
//传入后，列族将使用backup_index_comparator。
//保留字节：基础writebatch中的保留字节
//max_bytes:底层writebatch的最大大小（字节）
//覆盖键：如果为真，则在插入时覆盖索引中的键
//与以前相同的键，因此迭代器永远不会
//显示具有相同键的两个条目。
  explicit WriteBatchWithIndex(
      const Comparator* backup_index_comparator = BytewiseComparator(),
      size_t reserved_bytes = 0, bool overwrite_key = false,
      size_t max_bytes = 0);

  ~WriteBatchWithIndex() override;

  using WriteBatchBase::Put;
  Status Put(ColumnFamilyHandle* column_family, const Slice& key,
             const Slice& value) override;

  Status Put(const Slice& key, const Slice& value) override;

  using WriteBatchBase::Merge;
  Status Merge(ColumnFamilyHandle* column_family, const Slice& key,
               const Slice& value) override;

  Status Merge(const Slice& key, const Slice& value) override;

  using WriteBatchBase::Delete;
  Status Delete(ColumnFamilyHandle* column_family, const Slice& key) override;
  Status Delete(const Slice& key) override;

  using WriteBatchBase::SingleDelete;
  Status SingleDelete(ColumnFamilyHandle* column_family,
                      const Slice& key) override;
  Status SingleDelete(const Slice& key) override;

  using WriteBatchBase::DeleteRange;
  Status DeleteRange(ColumnFamilyHandle* column_family, const Slice& begin_key,
                     const Slice& end_key) override;
  Status DeleteRange(const Slice& begin_key, const Slice& end_key) override;

  using WriteBatchBase::PutLogData;
  Status PutLogData(const Slice& blob) override;

  using WriteBatchBase::Clear;
  void Clear() override;

  using WriteBatchBase::GetWriteBatch;
  WriteBatch* GetWriteBatch() override;

//创建列族的迭代器。用户可以调用迭代器.seek（）到
//搜索到键的下一个条目或键后的条目。密钥将在
//指数比较器给出的顺序。对于同一密钥上的多个更新，
//每个更新将作为一个单独的条目按更新顺序返回
//时间。
//
//调用方应删除返回的迭代器。
  WBWIIterator* NewIterator(ColumnFamilyHandle* column_family);
//创建默认列族的迭代器。
  WBWIIterator* NewIterator();

//将创建一个新的迭代器，该迭代器将使用wbwi迭代器作为增量，并且
//基_迭代器作为基。
//
//仅当writebatchwithindex为
//用overwrite_key=true构造。
//
//调用方应删除返回的迭代器。
//基_迭代器现在由返回的迭代器“拥有”。删除
//返回的迭代器还将删除基迭代器。
  Iterator* NewIteratorWithBase(ColumnFamilyHandle* column_family,
                                Iterator* base_iterator);
//默认列族
  Iterator* NewIteratorWithBase(Iterator* base_iterator);

//类似于db:：get（），但只从该批中读取密钥。
//如果批处理没有足够的数据来解析合并操作，
//可以返回MergeInProgress状态。
  Status GetFromBatch(ColumnFamilyHandle* column_family,
                      const DBOptions& options, const Slice& key,
                      std::string* value);

//类似于上一个函数，但不需要列\族。
//注意：如果存在任何合并，将返回InvalidArgument状态
//此键的运算符。改为使用以前的方法。
  Status GetFromBatch(const DBOptions& options, const Slice& key,
                      std::string* value) {
    return GetFromBatch(nullptr, options, key, value);
  }

//类似于db:：get（），但也将从该批中读取写操作。
//
//此函数将查询此批处理和数据库，然后合并
//使用db的merge运算符（如果批处理包含
//合并请求）。
//
//设置read_options.snapshot将影响从数据库中读取的内容
//但不会更改从批处理中读取的键
//此批处理尚不属于任何快照，将被提取
//无论如何）。
  Status GetFromBatchAndDB(DB* db, const ReadOptions& read_options,
                           const Slice& key, std::string* value);

//接收pinnableslice的上述方法的重载
  Status GetFromBatchAndDB(DB* db, const ReadOptions& read_options,
                           const Slice& key, PinnableSlice* value);

  Status GetFromBatchAndDB(DB* db, const ReadOptions& read_options,
                           ColumnFamilyHandle* column_family, const Slice& key,
                           std::string* value);

//接收pinnableslice的上述方法的重载
  Status GetFromBatchAndDB(DB* db, const ReadOptions& read_options,
                           ColumnFamilyHandle* column_family, const Slice& key,
                           PinnableSlice* value);

//记录批处理的状态，以便将来调用RollbackToSavePoint（）。
//可以多次调用以设置多个保存点。
  void SetSavePoint() override;

//删除此批中的所有条目（Put、Merge、Delete、SingleDelete、
//putlogdata）自最近调用setsavepoint（）以来，并移除
//最近的保存点。
//如果以前没有调用setsavepoint（），则其行为与
//清除（）
//
//调用RollbackToSavePoint将使此批处理中打开的所有迭代器失效。
//
//成功时返回状态：：OK（），
//如果以前没有调用setsavepoint（），则状态：：NotFound（），
//或其他腐败状况。
  Status RollbackToSavePoint() override;

//弹出最近的保存点。
//如果以前没有调用setsavepoint（），则状态：：NotFound（））
//将被退回。
//否则返回状态：：OK（）。
  Status PopSavePoint() override;

  void SetMaxBytes(size_t max_bytes) override;

 private:
  struct Rep;
  std::unique_ptr<Rep> rep;
};

}  //命名空间rocksdb

#endif  //！摇滚乐
