
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
//

#include "port/stack_trace.h"
#include "util/testharness.h"
#include "util/testutil.h"

#include "rocksdb/statistics.h"

namespace rocksdb {

class StatisticsTest : public testing::Test {};

//健全性检查，确保股票名称图的内容和顺序
//匹配tickers枚举
TEST_F(StatisticsTest, Sanity) {
  EXPECT_EQ(static_cast<size_t>(Tickers::TICKER_ENUM_MAX),
            TickersNameMap.size());

  for (uint32_t t = 0; t < Tickers::TICKER_ENUM_MAX; t++) {
    auto pair = TickersNameMap[static_cast<size_t>(t)];
    ASSERT_EQ(pair.first, t) << "Miss match at " << pair.second;
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  rocksdb::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
