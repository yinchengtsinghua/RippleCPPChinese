
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

#include <map>
#include <string>
#include <vector>

#include "monitoring/instrumented_mutex.h"
#include "options/cf_options.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/date_tiered_db.h"

namespace rocksdb {

//执行日期数据库。
class DateTieredDBImpl : public DateTieredDB {
 public:
  DateTieredDBImpl(DB* db, Options options,
                   const std::vector<ColumnFamilyDescriptor>& descriptors,
                   const std::vector<ColumnFamilyHandle*>& handles, int64_t ttl,
                   int64_t column_family_interval);

  virtual ~DateTieredDBImpl();

  Status Put(const WriteOptions& options, const Slice& key,
             const Slice& val) override;

  Status Get(const ReadOptions& options, const Slice& key,
             std::string* value) override;

  Status Delete(const WriteOptions& options, const Slice& key) override;

  bool KeyMayExist(const ReadOptions& options, const Slice& key,
                   std::string* value, bool* value_found = nullptr) override;

  Status Merge(const WriteOptions& options, const Slice& key,
               const Slice& value) override;

  Iterator* NewIterator(const ReadOptions& opts) override;

  Status DropObsoleteColumnFamilies() override;

//从键中提取时间戳。
  static Status GetTimestamp(const Slice& key, int64_t* result);

 private:
//基本数据库对象
  DB* db_;

  const ColumnFamilyOptions cf_options_;

  const ImmutableCFOptions ioptions_;

//存储时间序列数据的所有列族句柄。
  std::vector<ColumnFamilyHandle*> handles_;

//管理从列族的最大时间戳到其句柄的映射。
  std::map<int64_t, ColumnFamilyHandle*> handle_map_;

//表示何时应删除数据的生存时间值。
  int64_t ttl_;

//用于指示列族的时间范围的变量。
  int64_t column_family_interval_;

//指示列族的最大时间戳。
  int64_t latest_timebound_;

//互斥保护处理图操作。
  InstrumentedMutex mutex_;

//批量执行Put和Merge的内部方法。
  Status Write(const WriteOptions& opts, WriteBatch* updates);

  Status CreateColumnFamily(ColumnFamilyHandle** column_family);

  Status FindColumnFamily(int64_t keytime, ColumnFamilyHandle** column_family,
                          bool create_if_missing);

  static bool IsStale(int64_t keytime, int64_t ttl, Env* env);
};

}  //命名空间rocksdb
#endif  //摇滚乐
