
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
#include "util/auto_roll_logger.h"
#include "util/mutexlock.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE
//--自动滚动记录器
Status AutoRollLogger::ResetLogger() {
  TEST_SYNC_POINT("AutoRollLogger::ResetLogger:BeforeNewLogger");
  status_ = env_->NewLogger(log_fname_, &logger_);
  TEST_SYNC_POINT("AutoRollLogger::ResetLogger:AfterNewLogger");

  if (!status_.ok()) {
    return status_;
  }

  if (logger_->GetLogFileSize() == Logger::kDoNotSupportGetLogFileSize) {
    status_ = Status::NotSupported(
        "The underlying logger doesn't support GetLogFileSize()");
  }
  if (status_.ok()) {
    cached_now = static_cast<uint64_t>(env_->NowMicros() * 1e-6);
    ctime_ = cached_now;
    cached_now_access_count = 0;
  }

  return status_;
}

void AutoRollLogger::RollLogFile() {
//此函数在日志旋转时调用。两个旋转
//可以很快发生（nowmicro返回相同的值）。不覆盖
//上一个日志文件将以微秒递增，然后重试。
  uint64_t now = env_->NowMicros();
  std::string old_fname;
  do {
    old_fname = OldInfoLogFileName(
      dbname_, now, db_absolute_path_, db_log_dir_);
    now++;
  } while (env_->FileExists(old_fname).ok());
  env_->RenameFile(log_fname_, old_fname);
}

std::string AutoRollLogger::ValistToString(const char* format,
                                           va_list args) const {
//任何超过1024的日志消息都将被截断。
//用户负责将较长的消息截断为多行日志。
  static const int MAXBUFFERSIZE = 1024;
  char buffer[MAXBUFFERSIZE];

  int count = vsnprintf(buffer, MAXBUFFERSIZE, format, args);
  (void) count;
  assert(count >= 0);

  return buffer;
}

void AutoRollLogger::LogInternal(const char* format, ...) {
  mutex_.AssertHeld();
  va_list args;
  va_start(args, format);
  logger_->Logv(format, args);
  va_end(args);
}

void AutoRollLogger::Logv(const char* format, va_list ap) {
  assert(GetStatus().ok());

  std::shared_ptr<Logger> logger;
  {
    MutexLock l(&mutex_);
    if ((kLogFileTimeToRoll > 0 && LogExpired()) ||
        (kMaxLogFileSize > 0 && logger_->GetLogFileSize() >= kMaxLogFileSize)) {
      RollLogFile();
      Status s = ResetLogger();
      if (!s.ok()) {
//如果创建新日志文件失败，则无法真正记录错误
        return;
      }

      WriteHeaderInfo();
    }

//释放互斥体之前，请锁定当前记录器的实例。
    logger = logger_;
  }

//另一个线程现在可能已经将一个新的记录器实例放入了logger_u中。
//但是，由于记录器仍挂在上一个实例上
//（参考计数不是零），我们不必担心
//在访问时删除。
//注意，logv本身不受互斥保护以允许最大并发性，
//因为线程安全应该由底层记录器处理。
  logger->Logv(format, ap);
}

void AutoRollLogger::WriteHeaderInfo() {
  mutex_.AssertHeld();
  for (auto& header : headers_) {
    LogInternal("%s", header.c_str());
  }
}

void AutoRollLogger::LogHeader(const char* format, va_list args) {
//头消息将保留在内存中。因为我们不能做任何
//关于VA_列表中包含的数据的假设，我们将保留它们作为
//串
  va_list tmp;
  va_copy(tmp, args);
  std::string data = ValistToString(format, tmp);
  va_end(tmp);

  MutexLock l(&mutex_);
  headers_.push_back(data);

//将原始消息记录到当前日志
  logger_->Logv(format, args);
}

bool AutoRollLogger::LogExpired() {
  if (cached_now_access_count >= call_NowMicros_every_N_records_) {
    cached_now = static_cast<uint64_t>(env_->NowMicros() * 1e-6);
    cached_now_access_count = 0;
  }

  ++cached_now_access_count;
  return cached_now >= ctime_ + kLogFileTimeToRoll;
}
#endif  //！摇滚乐

Status CreateLoggerFromOptions(const std::string& dbname,
                               const DBOptions& options,
                               std::shared_ptr<Logger>* logger) {
  if (options.info_log) {
    *logger = options.info_log;
    return Status::OK();
  }

  Env* env = options.env;
  std::string db_absolute_path;
  env->GetAbsolutePath(dbname, &db_absolute_path);
  std::string fname =
      InfoLogFileName(dbname, db_absolute_path, options.db_log_dir);

env->CreateDirIfMissing(dbname);  //如果它不存在
//目前我们只支持按时间滚动和日志大小
#ifndef ROCKSDB_LITE
  if (options.log_file_time_to_roll > 0 || options.max_log_file_size > 0) {
    AutoRollLogger* result = new AutoRollLogger(
        env, dbname, options.db_log_dir, options.max_log_file_size,
        options.log_file_time_to_roll, options.info_log_level);
    Status s = result->GetStatus();
    if (!s.ok()) {
      delete result;
    } else {
      logger->reset(result);
    }
    return s;
  }
#endif  //！摇滚乐
//在与数据库相同的目录中打开日志文件
  env->RenameFile(fname,
                  OldInfoLogFileName(dbname, env->NowMicros(), db_absolute_path,
                                     options.db_log_dir));
  auto s = env->NewLogger(fname, logger);
  if (logger->get() != nullptr) {
    (*logger)->SetInfoLogLevel(options.info_log_level);
  }
  return s;
}

}  //命名空间rocksdb
