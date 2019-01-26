
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

#include "util/log_buffer.h"

#include "port/sys_time.h"
#include "port/port.h"

namespace rocksdb {

LogBuffer::LogBuffer(const InfoLogLevel log_level,
                     Logger*info_log)
    : log_level_(log_level), info_log_(info_log) {}

void LogBuffer::AddLogToBuffer(size_t max_log_size, const char* format,
                               va_list ap) {
  if (!info_log_ || log_level_ < info_log_->GetInfoLogLevel()) {
//跳过级别，因为它是级别。
    return;
  }

  char* alloc_mem = arena_.AllocateAligned(max_log_size);
  BufferedLog* buffered_log = new (alloc_mem) BufferedLog();
  char* p = buffered_log->message;
  char* limit = alloc_mem + max_log_size - 1;

//储存时间
  gettimeofday(&(buffered_log->now_tv), nullptr);

//打印邮件
  if (p < limit) {
    va_list backup_ap;
    va_copy(backup_ap, ap);
    auto n = vsnprintf(p, limit - p, format, backup_ap);
#ifndef OS_WIN
//当缓冲区太短时MS报告-1
    assert(n >= 0);
#endif
    if (n > 0) {
      p += n;
    } else {
      p = limit;
    }
    va_end(backup_ap);
  }

  if (p > limit) {
    p = limit;
  }

//将'\0'添加到结尾
  *p = '\0';

  logs_.push_back(buffered_log);
}

void LogBuffer::FlushBufferToLog() {
  for (BufferedLog* log : logs_) {
    const time_t seconds = log->now_tv.tv_sec;
    struct tm t;
    if (localtime_r(&seconds, &t) != nullptr) {
      Log(log_level_, info_log_,
          "(Original Log Time %04d/%02d/%02d-%02d:%02d:%02d.%06d) %s",
          t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
          t.tm_sec, static_cast<int>(log->now_tv.tv_usec), log->message);
    }
  }
  logs_.clear();
}

void LogToBuffer(LogBuffer* log_buffer, size_t max_log_size, const char* format,
                 ...) {
  if (log_buffer != nullptr) {
    va_list ap;
    va_start(ap, format);
    log_buffer->AddLogToBuffer(max_log_size, format, ap);
    va_end(ap);
  }
}

void LogToBuffer(LogBuffer* log_buffer, const char* format, ...) {
  const size_t kDefaultMaxLogSize = 512;
  if (log_buffer != nullptr) {
    va_list ap;
    va_start(ap, format);
    log_buffer->AddLogToBuffer(kDefaultMaxLogSize, format, ap);
    va_end(ap);
  }
}

}  //命名空间rocksdb
