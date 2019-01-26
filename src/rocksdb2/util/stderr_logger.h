
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once

#include <stdarg.h>
#include <stdio.h>

#include "rocksdb/env.h"

namespace rocksdb {

//将日志打印到stderr以加快调试速度
class StderrLogger : public Logger {
 public:
  explicit StderrLogger(const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : Logger(log_level) {}

//将重载的logv（）引入作用域，以便在重写时不隐藏它们
//它们的一个子集。
  using Logger::Logv;

  virtual void Logv(const char* format, va_list ap) override {
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
  }
};

}  //命名空间rocksdb
