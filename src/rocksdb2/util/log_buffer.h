
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

#pragma once

#include "rocksdb/env.h"
#include "util/arena.h"
#include "util/autovector.h"
#include "port/sys_time.h"
#include <ctime>

namespace rocksdb {

class Logger;

//一个类，用于缓冲信息日志条目并最终刷新它们。
class LogBuffer {
 public:
//日志级别：所有日志的日志级别
//信息日志：要将日志写入的日志记录器
  LogBuffer(const InfoLogLevel log_level, Logger* info_log);

//向缓冲区添加日志条目。使用默认的最大日志大小。
//最大日志大小表示最大日志大小，包括一些元数据。
  void AddLogToBuffer(size_t max_log_size, const char* format, va_list ap);

  size_t IsEmpty() const { return logs_.empty(); }

//将所有缓冲日志刷新到信息日志。
  void FlushBufferToLog();

 private:
//一个日志条目及其时间戳
  struct BufferedLog {
struct timeval now_tv;  //日志的时间戳
char message[1];        //日志消息开始
  };

  const InfoLogLevel log_level_;
  Logger* info_log_;
  Arena arena_;
  autovector<BufferedLog*> logs_;
};

//将日志添加到日志缓冲区以进行延迟的信息日志记录。可以在以下情况下使用
//我们想在互斥体中添加一些日志。
//最大日志大小表示最大日志大小，包括一些元数据。
extern void LogToBuffer(LogBuffer* log_buffer, size_t max_log_size,
                        const char* format, ...);
//与上一个函数相同，但具有默认的最大日志大小。
extern void LogToBuffer(LogBuffer* log_buffer, const char* format, ...);

}  //命名空间rocksdb
