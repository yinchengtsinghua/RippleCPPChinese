
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

#include <string>

#include "util/event_logger.h"
#include "util/testharness.h"

namespace rocksdb {

class EventLoggerTest : public testing::Test {};

class StringLogger : public Logger {
 public:
  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    vsnprintf(buffer_, sizeof(buffer_), format, ap);
  }
  char* buffer() { return buffer_; }

 private:
  char buffer_[1000];
};

TEST_F(EventLoggerTest, SimpleTest) {
  StringLogger logger;
  EventLogger event_logger(&logger);
  event_logger.Log() << "id" << 5 << "event"
                     << "just_testing";
  std::string output(logger.buffer());
  ASSERT_TRUE(output.find("\"event\": \"just_testing\"") != std::string::npos);
  ASSERT_TRUE(output.find("\"id\": 5") != std::string::npos);
  ASSERT_TRUE(output.find("\"time_micros\"") != std::string::npos);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
