
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "util/event_logger.h"

#include <inttypes.h>
#include <cassert>
#include <sstream>
#include <string>

#include "util/logging.h"
#include "util/string_util.h"

namespace rocksdb {


EventLoggerStream::EventLoggerStream(Logger* logger)
    : logger_(logger), log_buffer_(nullptr), json_writer_(nullptr) {}

EventLoggerStream::EventLoggerStream(LogBuffer* log_buffer)
    : logger_(nullptr), log_buffer_(log_buffer), json_writer_(nullptr) {}

EventLoggerStream::~EventLoggerStream() {
  if (json_writer_) {
    json_writer_->EndObject();
#ifdef ROCKSDB_PRINT_EVENTS_TO_STDOUT
    printf("%s\n", json_writer_->Get().c_str());
#else
    if (logger_) {
      EventLogger::Log(logger_, *json_writer_);
    } else if (log_buffer_) {
      EventLogger::LogToBuffer(log_buffer_, *json_writer_);
    }
#endif
    delete json_writer_;
  }
}

void EventLogger::Log(const JSONWriter& jwriter) {
  Log(logger_, jwriter);
}

void EventLogger::Log(Logger* logger, const JSONWriter& jwriter) {
#ifdef ROCKSDB_PRINT_EVENTS_TO_STDOUT
  printf("%s\n", jwriter.Get().c_str());
#else
  rocksdb::Log(logger, "%s %s", Prefix(), jwriter.Get().c_str());
#endif
}

void EventLogger::LogToBuffer(
    LogBuffer* log_buffer, const JSONWriter& jwriter) {
#ifdef ROCKSDB_PRINT_EVENTS_TO_STDOUT
  printf("%s\n", jwriter.Get().c_str());
#else
  assert(log_buffer);
  rocksdb::LogToBuffer(log_buffer, "%s %s", Prefix(), jwriter.Get().c_str());
#endif
}

}  //命名空间rocksdb
