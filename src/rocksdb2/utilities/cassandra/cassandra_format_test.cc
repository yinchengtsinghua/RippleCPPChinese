
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

#include <cstring>
#include <memory>
#include "util/testharness.h"
#include "utilities/cassandra/format.h"
#include "utilities/cassandra/serialize.h"
#include "utilities/cassandra/test_utils.h"

using namespace rocksdb::cassandra;

namespace rocksdb {
namespace cassandra {

TEST(ColumnTest, Column) {
  char data[4] = {'d', 'a', 't', 'a'};
  int8_t mask = 0;
  int8_t index = 1;
  int64_t timestamp = 1494022807044;
  Column c = Column(mask, index, timestamp, sizeof(data), data);

  EXPECT_EQ(c.Index(), index);
  EXPECT_EQ(c.Timestamp(), timestamp);
  EXPECT_EQ(c.Size(), 14 + sizeof(data));

//验证序列化。
  std::string dest;
  dest.reserve(c.Size() * 2);
  c.Serialize(&dest);

  EXPECT_EQ(dest.size(), c.Size());
  std::size_t offset = 0;
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), mask);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), timestamp);
  offset += sizeof(int64_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), sizeof(data));
  offset += sizeof(int32_t);
  EXPECT_TRUE(std::memcmp(data, dest.c_str() + offset, sizeof(data)) == 0);

//验证反序列化。
  std::string saved_dest = dest;
  std::shared_ptr<Column> c1 = Column::Deserialize(saved_dest.c_str(), 0);
  EXPECT_EQ(c1->Index(), index);
  EXPECT_EQ(c1->Timestamp(), timestamp);
  EXPECT_EQ(c1->Size(), 14 + sizeof(data));

  c1->Serialize(&dest);
  EXPECT_EQ(dest.size(), 2 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str(), dest.c_str() + c.Size(), c.Size()) == 0);

//验证ColumnBase:：反序列化。
  saved_dest = dest;
  std::shared_ptr<ColumnBase> c2 =
      ColumnBase::Deserialize(saved_dest.c_str(), c.Size());
  c2->Serialize(&dest);
  EXPECT_EQ(dest.size(), 3 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str() + c.Size(), dest.c_str() + c.Size() * 2, c.Size())
      == 0);
}

TEST(ExpiringColumnTest, ExpiringColumn) {
  char data[4] = {'d', 'a', 't', 'a'};
  int8_t mask = ColumnTypeMask::EXPIRATION_MASK;
  int8_t index = 3;
  int64_t timestamp = 1494022807044;
  int32_t ttl = 3600;
  ExpiringColumn c = ExpiringColumn(mask, index, timestamp,
                                    sizeof(data), data, ttl);

  EXPECT_EQ(c.Index(), index);
  EXPECT_EQ(c.Timestamp(), timestamp);
  EXPECT_EQ(c.Size(), 18 + sizeof(data));

//验证序列化。
  std::string dest;
  dest.reserve(c.Size() * 2);
  c.Serialize(&dest);

  EXPECT_EQ(dest.size(), c.Size());
  std::size_t offset = 0;
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), mask);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), timestamp);
  offset += sizeof(int64_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), sizeof(data));
  offset += sizeof(int32_t);
  EXPECT_TRUE(std::memcmp(data, dest.c_str() + offset, sizeof(data)) == 0);
  offset += sizeof(data);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), ttl);

//验证反序列化。
  std::string saved_dest = dest;
  std::shared_ptr<ExpiringColumn> c1 =
      ExpiringColumn::Deserialize(saved_dest.c_str(), 0);
  EXPECT_EQ(c1->Index(), index);
  EXPECT_EQ(c1->Timestamp(), timestamp);
  EXPECT_EQ(c1->Size(), 18 + sizeof(data));

  c1->Serialize(&dest);
  EXPECT_EQ(dest.size(), 2 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str(), dest.c_str() + c.Size(), c.Size()) == 0);

//验证ColumnBase:：反序列化。
  saved_dest = dest;
  std::shared_ptr<ColumnBase> c2 =
      ColumnBase::Deserialize(saved_dest.c_str(), c.Size());
  c2->Serialize(&dest);
  EXPECT_EQ(dest.size(), 3 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str() + c.Size(), dest.c_str() + c.Size() * 2, c.Size())
      == 0);
}

TEST(TombstoneTest, Tombstone) {
  int8_t mask = ColumnTypeMask::DELETION_MASK;
  int8_t index = 2;
  int32_t local_deletion_time = 1494022807;
  int64_t marked_for_delete_at = 1494022807044;
  Tombstone c = Tombstone(mask, index, local_deletion_time,
                          marked_for_delete_at);

  EXPECT_EQ(c.Index(), index);
  EXPECT_EQ(c.Timestamp(), marked_for_delete_at);
  EXPECT_EQ(c.Size(), 14);

//验证序列化。
  std::string dest;
  dest.reserve(c.Size() * 2);
  c.Serialize(&dest);

  EXPECT_EQ(dest.size(), c.Size());
  std::size_t offset = 0;
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), mask);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), local_deletion_time);
  offset += sizeof(int32_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), marked_for_delete_at);

//验证反序列化。
  std::shared_ptr<Tombstone> c1 = Tombstone::Deserialize(dest.c_str(), 0);
  EXPECT_EQ(c1->Index(), index);
  EXPECT_EQ(c1->Timestamp(), marked_for_delete_at);
  EXPECT_EQ(c1->Size(), 14);

  c1->Serialize(&dest);
  EXPECT_EQ(dest.size(), 2 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str(), dest.c_str() + c.Size(), c.Size()) == 0);

//验证ColumnBase:：反序列化。
  std::shared_ptr<ColumnBase> c2 =
    ColumnBase::Deserialize(dest.c_str(), c.Size());
  c2->Serialize(&dest);
  EXPECT_EQ(dest.size(), 3 * c.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str() + c.Size(), dest.c_str() + c.Size() * 2, c.Size())
      == 0);
}

TEST(RowValueTest, RowTombstone) {
  int32_t local_deletion_time = 1494022807;
  int64_t marked_for_delete_at = 1494022807044;
  RowValue r = RowValue(local_deletion_time, marked_for_delete_at);

  EXPECT_EQ(r.Size(), 12);
  EXPECT_EQ(r.IsTombstone(), true);
  EXPECT_EQ(r.LastModifiedTime(), marked_for_delete_at);

//验证序列化。
  std::string dest;
  dest.reserve(r.Size() * 2);
  r.Serialize(&dest);

  EXPECT_EQ(dest.size(), r.Size());
  std::size_t offset = 0;
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), local_deletion_time);
  offset += sizeof(int32_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), marked_for_delete_at);

//验证反序列化。
  RowValue r1 = RowValue::Deserialize(dest.c_str(), r.Size());
  EXPECT_EQ(r1.Size(), 12);
  EXPECT_EQ(r1.IsTombstone(), true);
  EXPECT_EQ(r1.LastModifiedTime(), marked_for_delete_at);

  r1.Serialize(&dest);
  EXPECT_EQ(dest.size(), 2 * r.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str(), dest.c_str() + r.Size(), r.Size()) == 0);
}

TEST(RowValueTest, RowWithColumns) {
  std::vector<std::shared_ptr<ColumnBase>> columns;
  int64_t last_modified_time = 1494022807048;
  std::size_t columns_data_size = 0;

  char e_data[5] = {'e', 'd', 'a', 't', 'a'};
  int8_t e_index = 0;
  int64_t e_timestamp = 1494022807044;
  int32_t e_ttl = 3600;
  columns.push_back(std::shared_ptr<ExpiringColumn>(
    new ExpiringColumn(ColumnTypeMask::EXPIRATION_MASK, e_index,
      e_timestamp, sizeof(e_data), e_data, e_ttl)));
  columns_data_size += columns[0]->Size();

  char c_data[4] = {'d', 'a', 't', 'a'};
  int8_t c_index = 1;
  int64_t c_timestamp = 1494022807048;
  columns.push_back(std::shared_ptr<Column>(
    new Column(0, c_index, c_timestamp, sizeof(c_data), c_data)));
  columns_data_size += columns[1]->Size();

  int8_t t_index = 2;
  int32_t t_local_deletion_time = 1494022801;
  int64_t t_marked_for_delete_at = 1494022807043;
  columns.push_back(std::shared_ptr<Tombstone>(
    new Tombstone(ColumnTypeMask::DELETION_MASK,
      t_index, t_local_deletion_time, t_marked_for_delete_at)));
  columns_data_size += columns[2]->Size();

  RowValue r = RowValue(std::move(columns), last_modified_time);

  EXPECT_EQ(r.Size(), columns_data_size + 12);
  EXPECT_EQ(r.IsTombstone(), false);
  EXPECT_EQ(r.LastModifiedTime(), last_modified_time);

//验证序列化。
  std::string dest;
  dest.reserve(r.Size() * 2);
  r.Serialize(&dest);

  EXPECT_EQ(dest.size(), r.Size());
  std::size_t offset = 0;
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset),
    std::numeric_limits<int32_t>::max());
  offset += sizeof(int32_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset),
    std::numeric_limits<int64_t>::min());
  offset += sizeof(int64_t);

//第0列：ExpiringColumn
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset),
    ColumnTypeMask::EXPIRATION_MASK);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), e_index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), e_timestamp);
  offset += sizeof(int64_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), sizeof(e_data));
  offset += sizeof(int32_t);
  EXPECT_TRUE(std::memcmp(e_data, dest.c_str() + offset, sizeof(e_data)) == 0);
  offset += sizeof(e_data);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), e_ttl);
  offset += sizeof(int32_t);

//哥伦布：专栏
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), 0);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), c_index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), c_timestamp);
  offset += sizeof(int64_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), sizeof(c_data));
  offset += sizeof(int32_t);
  EXPECT_TRUE(std::memcmp(c_data, dest.c_str() + offset, sizeof(c_data)) == 0);
  offset += sizeof(c_data);

//第2列：墓碑
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset),
    ColumnTypeMask::DELETION_MASK);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int8_t>(dest.c_str(), offset), t_index);
  offset += sizeof(int8_t);
  EXPECT_EQ(Deserialize<int32_t>(dest.c_str(), offset), t_local_deletion_time);
  offset += sizeof(int32_t);
  EXPECT_EQ(Deserialize<int64_t>(dest.c_str(), offset), t_marked_for_delete_at);

//验证反序列化。
  RowValue r1 = RowValue::Deserialize(dest.c_str(), r.Size());
  EXPECT_EQ(r1.Size(), columns_data_size + 12);
  EXPECT_EQ(r1.IsTombstone(), false);
  EXPECT_EQ(r1.LastModifiedTime(), last_modified_time);

  r1.Serialize(&dest);
  EXPECT_EQ(dest.size(), 2 * r.Size());
  EXPECT_TRUE(
    std::memcmp(dest.c_str(), dest.c_str() + r.Size(), r.Size()) == 0);
}

TEST(RowValueTest, PurgeTtlShouldRemvoeAllColumnsExpired) {
  int64_t now = time(nullptr);

  auto row_value = CreateTestRowValue({
    std::make_tuple(kColumn, 0, ToMicroSeconds(now)),
std::make_tuple(kExpiringColumn, 1, ToMicroSeconds(now - kTtl - 10)), //期满
std::make_tuple(kExpiringColumn, 2, ToMicroSeconds(now)), //未过期
    std::make_tuple(kTombstone, 3, ToMicroSeconds(now))
  });

  bool changed = false;
  auto purged = row_value.PurgeTtl(&changed);
  EXPECT_TRUE(changed);
  EXPECT_EQ(purged.columns_.size(), 3);
  VerifyRowValueColumns(purged.columns_, 0, kColumn, 0, ToMicroSeconds(now));
  VerifyRowValueColumns(purged.columns_, 1, kExpiringColumn, 2, ToMicroSeconds(now));
  VerifyRowValueColumns(purged.columns_, 2, kTombstone, 3, ToMicroSeconds(now));

  purged.PurgeTtl(&changed);
  EXPECT_FALSE(changed);
}

TEST(RowValueTest, ExpireTtlShouldConvertExpiredColumnsToTombstones) {
  int64_t now = time(nullptr);

  auto row_value = CreateTestRowValue({
    std::make_tuple(kColumn, 0, ToMicroSeconds(now)),
std::make_tuple(kExpiringColumn, 1, ToMicroSeconds(now - kTtl - 10)), //期满
std::make_tuple(kExpiringColumn, 2, ToMicroSeconds(now)), //未过期
    std::make_tuple(kTombstone, 3, ToMicroSeconds(now))
  });

  bool changed = false;
  auto compacted = row_value.ExpireTtl(&changed);
  EXPECT_TRUE(changed);
  EXPECT_EQ(compacted.columns_.size(), 4);
  VerifyRowValueColumns(compacted.columns_, 0, kColumn, 0, ToMicroSeconds(now));
  VerifyRowValueColumns(compacted.columns_, 1, kTombstone, 1, ToMicroSeconds(now - 10));
  VerifyRowValueColumns(compacted.columns_, 2, kExpiringColumn, 2, ToMicroSeconds(now));
  VerifyRowValueColumns(compacted.columns_, 3, kTombstone, 3, ToMicroSeconds(now));

  compacted.ExpireTtl(&changed);
  EXPECT_FALSE(changed);
}
} //命名空间cassandra
} //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
