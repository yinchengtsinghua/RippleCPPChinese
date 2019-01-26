
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

#include "test_utils.h"

namespace rocksdb {
namespace cassandra {
const char kData[] = {'d', 'a', 't', 'a'};
const char kExpiringData[] = {'e', 'd', 'a', 't', 'a'};
const int32_t kLocalDeletionTime = 1;
const int32_t kTtl = 86400;
const int8_t kColumn = 0;
const int8_t kTombstone = 1;
const int8_t kExpiringColumn = 2;

std::shared_ptr<ColumnBase> CreateTestColumn(int8_t mask,
                                             int8_t index,
                                             int64_t timestamp) {
  if ((mask & ColumnTypeMask::DELETION_MASK) != 0) {
    return std::shared_ptr<Tombstone>(new Tombstone(
      mask, index, kLocalDeletionTime, timestamp));
  } else if ((mask & ColumnTypeMask::EXPIRATION_MASK) != 0) {
    return std::shared_ptr<ExpiringColumn>(new ExpiringColumn(
      mask, index, timestamp, sizeof(kExpiringData), kExpiringData, kTtl));
  } else {
    return std::shared_ptr<Column>(
      new Column(mask, index, timestamp, sizeof(kData), kData));
  }
}

RowValue CreateTestRowValue(
    std::vector<std::tuple<int8_t, int8_t, int64_t>> column_specs) {
  std::vector<std::shared_ptr<ColumnBase>> columns;
  int64_t last_modified_time = 0;
  for (auto spec: column_specs) {
    auto c = CreateTestColumn(std::get<0>(spec), std::get<1>(spec),
                              std::get<2>(spec));
    last_modified_time = std::max(last_modified_time, c -> Timestamp());
    columns.push_back(std::move(c));
  }
  return RowValue(std::move(columns), last_modified_time);
}

RowValue CreateRowTombstone(int64_t timestamp) {
  return RowValue(kLocalDeletionTime, timestamp);
}

void VerifyRowValueColumns(
  std::vector<std::shared_ptr<ColumnBase>> &columns,
  std::size_t index_of_vector,
  int8_t expected_mask,
  int8_t expected_index,
  int64_t expected_timestamp
) {
  EXPECT_EQ(expected_timestamp, columns[index_of_vector]->Timestamp());
  EXPECT_EQ(expected_mask, columns[index_of_vector]->Mask());
  EXPECT_EQ(expected_index, columns[index_of_vector]->Index());
}

int64_t ToMicroSeconds(int64_t seconds) {
  return seconds * (int64_t)1000000;
}

}
}
