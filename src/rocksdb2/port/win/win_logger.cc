
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

#include "port/win/win_logger.h"
#include "port/win/io_win.h"

#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <atomic>

#include "rocksdb/env.h"

#include "monitoring/iostats_context_imp.h"
#include "port/sys_time.h"

namespace rocksdb {

namespace port {

WinLogger::WinLogger(uint64_t (*gettid)(), Env* env, HANDLE file,
                     const InfoLogLevel log_level)
    : Logger(log_level),
      file_(file),
      gettid_(gettid),
      log_size_(0),
      last_flush_micros_(0),
      env_(env),
      flush_pending_(false) {}

void WinLogger::DebugWriter(const char* str, int len) {
  DWORD bytesWritten = 0;
  BOOL ret = WriteFile(file_, str, len, &bytesWritten, NULL);
  if (ret == FALSE) {
    std::string errSz = GetWindowsErrSz(GetLastError());
    fprintf(stderr, errSz.c_str());
  }
}

WinLogger::~WinLogger() { close(); }

void WinLogger::close() { CloseHandle(file_); }

void WinLogger::Flush() {
  if (flush_pending_) {
    flush_pending_ = false;
//使用Windows API，写操作直接转到操作系统缓冲区，因此不需要进行快速刷新。
//与C运行时API不同。我们不会一路冲进磁盘
//出于性能原因。
  }

  last_flush_micros_ = env_->NowMicros();
}

void WinLogger::Logv(const char* format, va_list ap) {
  IOSTATS_TIMER_GUARD(logger_nanos);

  const uint64_t thread_id = (*gettid_)();

//我们尝试了两次：第一次使用固定大小的堆栈分配缓冲区，
//第二次使用了更大的动态分配缓冲区。
  char buffer[500];
  std::unique_ptr<char[]> largeBuffer;
  for (int iter = 0; iter < 2; ++iter) {
    char* base;
    int bufsize;
    if (iter == 0) {
      bufsize = sizeof(buffer);
      base = buffer;
    } else {
      bufsize = 30000;
      largeBuffer.reset(new char[bufsize]);
      base = largeBuffer.get();
    }

    char* p = base;
    char* limit = base + bufsize;

    struct timeval now_tv;
    gettimeofday(&now_tv, nullptr);
    const time_t seconds = now_tv.tv_sec;
    struct tm t;
    localtime_s(&t, &seconds);
    p += snprintf(p, limit - p, "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
                  t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour,
                  t.tm_min, t.tm_sec, static_cast<int>(now_tv.tv_usec),
                  static_cast<long long unsigned int>(thread_id));

//打印邮件
    if (p < limit) {
      va_list backup_ap;
      va_copy(backup_ap, ap);
      int done = vsnprintf(p, limit - p, format, backup_ap);
      if (done > 0) {
        p += done;
      } else {
        continue;
      }
      va_end(backup_ap);
    }

//必要时截断到可用空间
    if (p >= limit) {
      if (iter == 0) {
continue;  //使用较大的缓冲区重试
      } else {
        p = limit - 1;
      }
    }

//必要时添加换行符
    if (p == base || p[-1] != '\n') {
      *p++ = '\n';
    }

    assert(p <= limit);
    const size_t write_size = p - base;

    DWORD bytesWritten = 0;
    BOOL ret = WriteFile(file_, base, static_cast<DWORD>(write_size),
      &bytesWritten, NULL);
    if (ret == FALSE) {
      std::string errSz = GetWindowsErrSz(GetLastError());
      fprintf(stderr, errSz.c_str());
    }

    flush_pending_ = true;
    assert((bytesWritten == write_size) || (ret == FALSE));
    if (bytesWritten > 0) {
      log_size_ += write_size;
    }

    uint64_t now_micros =
        static_cast<uint64_t>(now_tv.tv_sec) * 1000000 + now_tv.tv_usec;
    if (now_micros - last_flush_micros_ >= flush_every_seconds_ * 1000000) {
      flush_pending_ = false;
//使用Windows API，写操作直接转到操作系统缓冲区，因此不需要进行快速刷新。
//与C运行时API不同。我们不会一路冲进磁盘
//出于性能原因。
      last_flush_micros_ = now_micros;
    }
    break;
  }
}

size_t WinLogger::GetLogFileSize() const { return log_size_; }

}

}  //命名空间rocksdb
