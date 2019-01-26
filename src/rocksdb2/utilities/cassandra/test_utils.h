
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

#pragma once
#include <memory>
#include "util/testharness.h"
#include "utilities/cassandra/format.h"
#include "utilities/cassandra/serialize.h"

namespace rocksdb {
namespace cassandra {
extern const char kData[];
extern const char kExpiringData[];
extern const int32_t kLocalDeletionTime;
extern const int32_t kTtl;
extern const int8_t kColumn;
extern const int8_t kTombstone;
extern const int8_t kExpiringColumn;


std::shared_ptr<ColumnBase> CreateTestColumn(int8_t mask,
                                             int8_t index,
                                             int64_t timestamp);

RowValue CreateTestRowValue(
    std::vector<std::tuple<int8_t, int8_t, int64_t>> column_specs);

RowValue CreateRowTombstone(int64_t timestamp);

void VerifyRowValueColumns(
  std::vector<std::shared_ptr<ColumnBase>> &columns,
  std::size_t index_of_vector,
  int8_t expected_mask,
  int8_t expected_index,
  int64_t expected_timestamp
);

int64_t ToMicroSeconds(int64_t seconds);

}
}
