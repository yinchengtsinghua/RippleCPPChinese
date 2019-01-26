
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
//所有环境都可以共享的记录器实现
//如果有足够的POSIX功能可用。

#pragma once
#include <list>
#include <string>

#include "port/port.h"
#include "port/util_logger.h"
#include "util/filename.h"
#include "util/mutexlock.h"
#include "util/sync_point.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE
//按大小和/或时间滚动日志文件
class AutoRollLogger : public Logger {
 public:
  AutoRollLogger(Env* env, const std::string& dbname,
                 const std::string& db_log_dir, size_t log_max_size,
                 size_t log_file_time_to_roll,
                 const InfoLogLevel log_level = InfoLogLevel::INFO_LEVEL)
      : Logger(log_level),
        dbname_(dbname),
        db_log_dir_(db_log_dir),
        env_(env),
        status_(Status::OK()),
        kMaxLogFileSize(log_max_size),
        kLogFileTimeToRoll(log_file_time_to_roll),
        cached_now(static_cast<uint64_t>(env_->NowMicros() * 1e-6)),
        ctime_(cached_now),
        cached_now_access_count(0),
        call_NowMicros_every_N_records_(100),
        mutex_() {
    env->GetAbsolutePath(dbname, &db_absolute_path_);
    log_fname_ = InfoLogFileName(dbname_, db_absolute_path_, db_log_dir_);
    RollLogFile();
    ResetLogger();
  }

  using Logger::Logv;
  void Logv(const char* format, va_list ap) override;

//将标题项写入日志。将写入所有标题信息
//每次原木翻滚时都会再次翻滚。
  virtual void LogHeader(const char* format, va_list ap) override;

//检查记录器是否遇到任何问题。
  Status GetStatus() {
    return status_;
  }

  size_t GetLogFileSize() const override {
    std::shared_ptr<Logger> logger;
    {
      MutexLock l(&mutex_);
//释放互斥体之前，请锁定当前记录器的实例。
      logger = logger_;
    }
    return logger->GetLogFileSize();
  }

  void Flush() override {
    std::shared_ptr<Logger> logger;
    {
      MutexLock l(&mutex_);
//释放互斥体之前，请锁定当前记录器的实例。
      logger = logger_;
    }
    TEST_SYNC_POINT("AutoRollLogger::Flush:PinnedLogger");
    if (logger) {
      logger->Flush();
    }
  }

  virtual ~AutoRollLogger() {
  }

  void SetCallNowMicrosEveryNRecords(uint64_t call_NowMicros_every_N_records) {
    call_NowMicros_every_N_records_ = call_NowMicros_every_N_records;
  }

//出于测试目的公开日志文件路径
  std::string TEST_log_fname() const {
    return log_fname_;
  }

  uint64_t TEST_ctime() const { return ctime_; }

 private:
  bool LogExpired();
  Status ResetLogger();
  void RollLogFile();
//不滚动地将消息记录到记录器
  void LogInternal(const char* format, ...);
//将va_列表序列化为字符串
  std::string ValistToString(const char* format, va_list args) const;
//将标记为头的日志写入新日志文件
  void WriteHeaderInfo();

std::string log_fname_; //当前活动信息日志的文件名。
  std::string dbname_;
  std::string db_log_dir_;
  std::string db_absolute_path_;
  Env* env_;
  std::shared_ptr<Logger> logger_;
//记录器的当前状态
  Status status_;
  const size_t kMaxLogFileSize;
  const size_t kLogFileTimeToRoll;
//标题信息
  std::list<std::string> headers_;
//为了避免频繁的env->nowmicros（）调用，我们缓存了当前时间
  uint64_t cached_now;
  uint64_t ctime_;
  uint64_t cached_now_access_count;
  uint64_t call_NowMicros_every_N_records_;
  mutable port::Mutex mutex_;
};
#endif  //！摇滚乐

//自动显示到Craete记录器
Status CreateLoggerFromOptions(const std::string& dbname,
                               const DBOptions& options,
                               std::shared_ptr<Logger>* logger);

}  //命名空间rocksdb
