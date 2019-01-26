
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//所有环境都可以共享的记录器实现
//如果有足够的POSIX功能可用。

#pragma once

#include <atomic>

#include "rocksdb/env.h"

#include <stdint.h>
#include <windows.h>

namespace rocksdb {

class Env;

namespace port {

class WinLogger : public rocksdb::Logger {
 public:
  WinLogger(uint64_t (*gettid)(), Env* env, HANDLE file,
            const InfoLogLevel log_level = InfoLogLevel::ERROR_LEVEL);

  virtual ~WinLogger();

  WinLogger(const WinLogger&) = delete;

  WinLogger& operator=(const WinLogger&) = delete;

  void close();

  void Flush() override;

  using rocksdb::Logger::Logv;
  void Logv(const char* format, va_list ap) override;

  size_t GetLogFileSize() const override;

  void DebugWriter(const char* str, int len);

 private:
  HANDLE file_;
uint64_t (*gettid_)();  //返回当前线程的线程ID
  std::atomic_size_t log_size_;
  std::atomic_uint_fast64_t last_flush_micros_;
  Env* env_;
  bool flush_pending_;

  const static uint64_t flush_every_seconds_ = 5;
};

}

}  //命名空间rocksdb
