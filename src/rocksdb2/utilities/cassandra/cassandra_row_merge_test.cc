
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

#include <memory>
#include "util/testharness.h"
#include "utilities/cassandra/format.h"
#include "utilities/cassandra/test_utils.h"

namespace rocksdb {
namespace cassandra {

TEST(RowValueMergeTest, Merge) {
  std::vector<RowValue> row_values;
  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kTombstone, 0, 5),
      std::make_tuple(kColumn, 1, 8),
      std::make_tuple(kExpiringColumn, 2, 5),
    })
  );

  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kColumn, 0, 2),
      std::make_tuple(kExpiringColumn, 1, 5),
      std::make_tuple(kTombstone, 2, 7),
      std::make_tuple(kExpiringColumn, 7, 17),
    })
  );

  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kExpiringColumn, 0, 6),
      std::make_tuple(kTombstone, 1, 5),
      std::make_tuple(kColumn, 2, 4),
      std::make_tuple(kTombstone, 11, 11),
    })
  );

  RowValue merged = RowValue::Merge(std::move(row_values));
  EXPECT_FALSE(merged.IsTombstone());
  EXPECT_EQ(merged.columns_.size(), 5);
  VerifyRowValueColumns(merged.columns_, 0, kExpiringColumn, 0, 6);
  VerifyRowValueColumns(merged.columns_, 1, kColumn, 1, 8);
  VerifyRowValueColumns(merged.columns_, 2, kTombstone, 2, 7);
  VerifyRowValueColumns(merged.columns_, 3, kExpiringColumn, 7, 17);
  VerifyRowValueColumns(merged.columns_, 4, kTombstone, 11, 11);
}

TEST(RowValueMergeTest, MergeWithRowTombstone) {
  std::vector<RowValue> row_values;

//一排墓碑。
  row_values.push_back(
    CreateRowTombstone(11)
  );

//此行的时间戳小于逻辑删除。
  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kColumn, 0, 5),
      std::make_tuple(kColumn, 1, 6),
    })
  );

//有些列的行较小，有些列较大。
  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kColumn, 2, 10),
      std::make_tuple(kColumn, 3, 12),
    })
  );

//列的所有行都大于墓碑。
  row_values.push_back(
    CreateTestRowValue({
      std::make_tuple(kColumn, 4, 13),
      std::make_tuple(kColumn, 5, 14),
    })
  );

  RowValue merged = RowValue::Merge(std::move(row_values));
  EXPECT_FALSE(merged.IsTombstone());
  EXPECT_EQ(merged.columns_.size(), 3);
  VerifyRowValueColumns(merged.columns_, 0, kColumn, 3, 12);
  VerifyRowValueColumns(merged.columns_, 1, kColumn, 4, 13);
  VerifyRowValueColumns(merged.columns_, 2, kColumn, 5, 14);

//如果墓碑的时间戳是最新的，则返回
//行墓碑。
  row_values.push_back(
    CreateRowTombstone(15)
  );

  row_values.push_back(
    CreateRowTombstone(17)
  );

  merged = RowValue::Merge(std::move(row_values));
  EXPECT_TRUE(merged.IsTombstone());
  EXPECT_EQ(merged.LastModifiedTime(), 17);
}

} //命名空间cassandra
} //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
