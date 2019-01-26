
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
#include <algorithm>
#include <stdio.h>
#include "port/sys_time.h"
#include <time.h>
#include <fcntl.h>

#ifdef OS_LINUX
#ifndef FALLOC_FL_KEEP_SIZE
#include <linux/falloc.h>
#endif
#endif

#include <atomic>
#include "monitoring/iostats_context_imp.h"
#include "rocksdb/env.h"
#include "util/sync_point.h"

namespace rocksdb {

class PosixLogger : public Logger {
 private:
  FILE* file_;
uint64_t (*gettid_)();  //返回当前线程的线程ID
  std::atomic_size_t log_size_;
  int fd_;
  const static uint64_t flush_every_seconds_ = 5;
  std::atomic_uint_fast64_t last_flush_micros_;
  Env* env_;
  std::atomic<bool> flush_pending_;
 public:
  PosixLogger(FILE* f, uint64_t (*gettid)(), Env* env,
              const InfoLogLevel log_level = InfoLogLevel::ERROR_LEVEL)
      : Logger(log_level),
        file_(f),
        gettid_(gettid),
        log_size_(0),
        fd_(fileno(f)),
        last_flush_micros_(0),
        env_(env),
        flush_pending_(false) {}
  virtual ~PosixLogger() {
    fclose(file_);
  }
  virtual void Flush() override {
    TEST_SYNC_POINT("PosixLogger::Flush:Begin1");
    TEST_SYNC_POINT("PosixLogger::Flush:Begin2");
    if (flush_pending_) {
      flush_pending_ = false;
      fflush(file_);
    }
    last_flush_micros_ = env_->NowMicros();
  }

  using Logger::Logv;
  virtual void Logv(const char* format, va_list ap) override {
    IOSTATS_TIMER_GUARD(logger_nanos);

    const uint64_t thread_id = (*gettid_)();

//我们尝试了两次：第一次使用固定大小的堆栈分配缓冲区，
//第二次使用了更大的动态分配缓冲区。
    char buffer[500];
    for (int iter = 0; iter < 2; iter++) {
      char* base;
      int bufsize;
      if (iter == 0) {
        bufsize = sizeof(buffer);
        base = buffer;
      } else {
        bufsize = 65536;
        base = new char[bufsize];
      }
      char* p = base;
      char* limit = base + bufsize;

      struct timeval now_tv;
      gettimeofday(&now_tv, nullptr);
      const time_t seconds = now_tv.tv_sec;
      struct tm t;
      localtime_r(&seconds, &t);
      p += snprintf(p, limit - p,
                    "%04d/%02d/%02d-%02d:%02d:%02d.%06d %llx ",
                    t.tm_year + 1900,
                    t.tm_mon + 1,
                    t.tm_mday,
                    t.tm_hour,
                    t.tm_min,
                    t.tm_sec,
                    static_cast<int>(now_tv.tv_usec),
                    static_cast<long long unsigned int>(thread_id));

//打印邮件
      if (p < limit) {
        va_list backup_ap;
        va_copy(backup_ap, ap);
        p += vsnprintf(p, limit - p, format, backup_ap);
        va_end(backup_ap);
      }

//必要时截断到可用空间
      if (p >= limit) {
        if (iter == 0) {
continue;       //使用较大的缓冲区重试
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

#ifdef ROCKSDB_FALLOCATE_PRESENT
      const int kDebugLogChunkSize = 128 * 1024;

//如果此写入将跨越kdebuglogchunksize的边界
//空间，预先分配更多空间以避免过大
//从文件系统allocsize选项分配。
      const size_t log_size = log_size_;
      const size_t last_allocation_chunk =
        ((kDebugLogChunkSize - 1 + log_size) / kDebugLogChunkSize);
      const size_t desired_allocation_chunk =
        ((kDebugLogChunkSize - 1 + log_size + write_size) /
           kDebugLogChunkSize);
      if (last_allocation_chunk != desired_allocation_chunk) {
        fallocate(
            fd_, FALLOC_FL_KEEP_SIZE, 0,
            static_cast<off_t>(desired_allocation_chunk * kDebugLogChunkSize));
      }
#endif

      size_t sz = fwrite(base, 1, write_size, file_);
      flush_pending_ = true;
      assert(sz == write_size);
      if (sz > 0) {
        log_size_ += write_size;
      }
      uint64_t now_micros = static_cast<uint64_t>(now_tv.tv_sec) * 1000000 +
        now_tv.tv_usec;
      if (now_micros - last_flush_micros_ >= flush_every_seconds_ * 1000000) {
        Flush();
      }
      if (base != buffer) {
        delete[] base;
      }
      break;
    }
  }
  size_t GetLogFileSize() const override { return log_size_; }
};

}  //命名空间rocksdb
