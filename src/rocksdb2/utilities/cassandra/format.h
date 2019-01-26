
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

/*
 *Cassandra行值的编码。
 *
 *cassandra行值可以是行墓碑，
 *或者包含多个列，它有以下字段：
 *
 *结构行值
 *Int32_t local_deletion_time；//删除行时的秒时间，
 *//仅用于Cassandra墓碑GC。
 *Int64_t marked_for_delete_at；//删除标记此行的ms。
 *struct column_base columns[]；//对于非逻辑删除行，所有列
 *//存储在这里。
 *}
 *
 *如果设置了本地删除时间和标记为删除时间，则为
 *一个墓碑，否则它包含多个列。
 *
 *有三种类型的列：普通列、过期列和列
 *墓碑，有以下字段：
 *
 *//标识列的类型。
 ＊枚举掩码{
 *删除\u mask=0x01，
 *过期\掩码=0x02，
 *}；
 *
 *结构柱
 *int8_t mask=0；
 *国际贸易指数；
 *Int64时间戳；
 *Int32_t值_长度；
 *char值[值_长度]；
 *}
 *
 *结构到期列
 *int8_t mask=mask.expiration_mask；
 *国际贸易指数；
 *Int64时间戳；
 *Int32_t值_长度；
 *char值[值_长度]；
 *Int32_t ttl；
 *}
 *
 *结构墓碑柱
 *int8_t mask=mask.deletion_mask；
 *国际贸易指数；
 *int32_t local_deletion_time；//类似于row_value的字段。
 *Int64_t标记为_删除_at；
 *}
 **/


#pragma once
#include <chrono>
#include <vector>
#include <memory>
#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"
#include "util/testharness.h"

namespace rocksdb {
namespace cassandra {

//标识列的类型。
enum ColumnTypeMask {
  DELETION_MASK = 0x01,
  EXPIRATION_MASK = 0x02,
};


class ColumnBase {
public:
  ColumnBase(int8_t mask, int8_t index);
  virtual ~ColumnBase() = default;

  virtual int64_t Timestamp() const = 0;
  virtual int8_t Mask() const;
  virtual int8_t Index() const;
  virtual std::size_t Size() const;
  virtual void Serialize(std::string* dest) const;
  static std::shared_ptr<ColumnBase> Deserialize(const char* src,
                                                 std::size_t offset);

private:
  int8_t mask_;
  int8_t index_;
};

class Column : public ColumnBase {
public:
  Column(int8_t mask, int8_t index, int64_t timestamp,
    int32_t value_size, const char* value);

  virtual int64_t Timestamp() const override;
  virtual std::size_t Size() const override;
  virtual void Serialize(std::string* dest) const override;
  static std::shared_ptr<Column> Deserialize(const char* src,
                                             std::size_t offset);

private:
  int64_t timestamp_;
  int32_t value_size_;
  const char* value_;
};

class Tombstone : public ColumnBase {
public:
  Tombstone(int8_t mask, int8_t index,
    int32_t local_deletion_time, int64_t marked_for_delete_at);

  virtual int64_t Timestamp() const override;
  virtual std::size_t Size() const override;
  virtual void Serialize(std::string* dest) const override;

  static std::shared_ptr<Tombstone> Deserialize(const char* src,
                                                std::size_t offset);

private:
  int32_t local_deletion_time_;
  int64_t marked_for_delete_at_;
};

class ExpiringColumn : public Column {
public:
  ExpiringColumn(int8_t mask, int8_t index, int64_t timestamp,
    int32_t value_size, const char* value, int32_t ttl);

  virtual std::size_t Size() const override;
  virtual void Serialize(std::string* dest) const override;
  bool Expired() const;
  std::shared_ptr<Tombstone> ToTombstone() const;

  static std::shared_ptr<ExpiringColumn> Deserialize(const char* src,
                                                     std::size_t offset);

private:
  int32_t ttl_;
  std::chrono::time_point<std::chrono::system_clock> TimePoint() const;
  std::chrono::seconds Ttl() const;
};

typedef std::vector<std::shared_ptr<ColumnBase>> Columns;

class RowValue {
public:
//创建行墓碑。
  RowValue(int32_t local_deletion_time, int64_t marked_for_delete_at);
//创建包含列的行。
  RowValue(Columns columns,
           int64_t last_modified_time);
  RowValue(const RowValue& that) = delete;
  RowValue(RowValue&& that) noexcept = default;
  RowValue& operator=(const RowValue& that) = delete;
  RowValue& operator=(RowValue&& that) = default;

  std::size_t Size() const;;
  bool IsTombstone() const;
//对于墓碑，返回标记为“删除”的“删除”。
//否则返回包含列的最大时间戳。
  int64_t LastModifiedTime() const;
  void Serialize(std::string* dest) const;
  RowValue PurgeTtl(bool* changed) const;
  RowValue ExpireTtl(bool* changed) const;
  bool Empty() const;

  static RowValue Deserialize(const char* src, std::size_t size);
//根据时间戳合并多行。
  static RowValue Merge(std::vector<RowValue>&& values);

private:
  int32_t local_deletion_time_;
  int64_t marked_for_delete_at_;
  Columns columns_;
  int64_t last_modified_time_;

  FRIEND_TEST(RowValueTest, PurgeTtlShouldRemvoeAllColumnsExpired);
  FRIEND_TEST(RowValueTest, ExpireTtlShouldConvertExpiredColumnsToTombstones);
  FRIEND_TEST(RowValueMergeTest, Merge);
  FRIEND_TEST(RowValueMergeTest, MergeWithRowTombstone);
  FRIEND_TEST(CassandraFunctionalTest, SimpleMergeTest);
  FRIEND_TEST(
    CassandraFunctionalTest, CompactionShouldConvertExpiredColumnsToTombstone);
  FRIEND_TEST(
    CassandraFunctionalTest, CompactionShouldPurgeExpiredColumnsIfPurgeTtlIsOn);
  FRIEND_TEST(
    CassandraFunctionalTest, CompactionShouldRemoveRowWhenAllColumnExpiredIfPurgeTtlIsOn);
};

} //名字卡桑德达
} //命名空间rocksdb
