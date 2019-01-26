
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

#include "rocksdb/env.h"

#include <thread>
#include "options/db_options.h"
#include "port/port.h"
#include "port/sys_time.h"
#include "rocksdb/options.h"
#include "util/arena.h"
#include "util/autovector.h"

namespace rocksdb {

Env::~Env() {
}

uint64_t Env::GetThreadID() const {
  std::hash<std::thread::id> hasher;
  return hasher(std::this_thread::get_id());
}

Status Env::ReuseWritableFile(const std::string& fname,
                              const std::string& old_fname,
                              unique_ptr<WritableFile>* result,
                              const EnvOptions& options) {
  Status s = RenameFile(old_fname, fname);
  if (!s.ok()) {
    return s;
  }
  return NewWritableFile(fname, result, options);
}

Status Env::GetChildrenFileAttributes(const std::string& dir,
                                      std::vector<FileAttributes>* result) {
  assert(result != nullptr);
  std::vector<std::string> child_fnames;
  Status s = GetChildren(dir, &child_fnames);
  if (!s.ok()) {
    return s;
  }
  result->resize(child_fnames.size());
  size_t result_size = 0;
  for (size_t i = 0; i < child_fnames.size(); ++i) {
    const std::string path = dir + "/" + child_fnames[i];
    if (!(s = GetFileSize(path, &(*result)[result_size].size_bytes)).ok()) {
      if (FileExists(path).IsNotFound()) {
//自从我们列出目录后，该文件可能已被删除。
        continue;
      }
      return s;
    }
    (*result)[result_size].name = std::move(child_fnames[i]);
    result_size++;
  }
  result->resize(result_size);
  return Status::OK();
}

SequentialFile::~SequentialFile() {
}

RandomAccessFile::~RandomAccessFile() {
}

WritableFile::~WritableFile() {
}

Logger::~Logger() {
}

FileLock::~FileLock() {
}

void LogFlush(Logger *info_log) {
  if (info_log) {
    info_log->Flush();
  }
}

void Log(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::INFO_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::INFO_LEVEL, format, ap);
    va_end(ap);
  }
}

void Logger::Logv(const InfoLogLevel log_level, const char* format, va_list ap) {
  static const char* kInfoLogLevelNames[5] = { "DEBUG", "INFO", "WARN",
    "ERROR", "FATAL" };
  if (log_level < log_level_) {
    return;
  }

  if (log_level == InfoLogLevel::INFO_LEVEL) {
//如果日志级别为信息级别，则不打印日志级别。
//这是为了避免在添加
//日志级别的功能。添加功能之前的所有日志
//是信息级别。我们不想在现有的基础上增加额外的成本
//登录中。
    Logv(format, ap);
  } else {
    char new_format[500];
    snprintf(new_format, sizeof(new_format) - 1, "[%s] %s",
      kInfoLogLevelNames[log_level], format);
    Logv(new_format, ap);
  }
}


void Log(const InfoLogLevel log_level, Logger* info_log, const char* format,
         ...) {
  if (info_log && info_log->GetInfoLogLevel() <= log_level) {
    va_list ap;
    va_start(ap, format);

    if (log_level == InfoLogLevel::HEADER_LEVEL) {
      info_log->LogHeader(format, ap);
    } else {
      info_log->Logv(log_level, format, ap);
    }

    va_end(ap);
  }
}

void Header(Logger* info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->LogHeader(format, ap);
    va_end(ap);
  }
}

void Debug(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::DEBUG_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::DEBUG_LEVEL, format, ap);
    va_end(ap);
  }
}

void Info(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::INFO_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::INFO_LEVEL, format, ap);
    va_end(ap);
  }
}

void Warn(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::WARN_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::WARN_LEVEL, format, ap);
    va_end(ap);
  }
}
void Error(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::ERROR_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::ERROR_LEVEL, format, ap);
    va_end(ap);
  }
}
void Fatal(Logger* info_log, const char* format, ...) {
  if (info_log && info_log->GetInfoLogLevel() <= InfoLogLevel::FATAL_LEVEL) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::FATAL_LEVEL, format, ap);
    va_end(ap);
  }
}

void LogFlush(const shared_ptr<Logger>& info_log) {
  if (info_log) {
    info_log->Flush();
  }
}

void Log(const InfoLogLevel log_level, const shared_ptr<Logger>& info_log,
         const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(log_level, format, ap);
    va_end(ap);
  }
}

void Header(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->LogHeader(format, ap);
    va_end(ap);
  }
}

void Debug(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::DEBUG_LEVEL, format, ap);
    va_end(ap);
  }
}

void Info(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::INFO_LEVEL, format, ap);
    va_end(ap);
  }
}

void Warn(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::WARN_LEVEL, format, ap);
    va_end(ap);
  }
}

void Error(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::ERROR_LEVEL, format, ap);
    va_end(ap);
  }
}

void Fatal(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::FATAL_LEVEL, format, ap);
    va_end(ap);
  }
}

void Log(const shared_ptr<Logger>& info_log, const char* format, ...) {
  if (info_log) {
    va_list ap;
    va_start(ap, format);
    info_log->Logv(InfoLogLevel::INFO_LEVEL, format, ap);
    va_end(ap);
  }
}

Status WriteStringToFile(Env* env, const Slice& data, const std::string& fname,
                         bool should_sync) {
  unique_ptr<WritableFile> file;
  EnvOptions soptions;
  Status s = env->NewWritableFile(fname, &file, soptions);
  if (!s.ok()) {
    return s;
  }
  s = file->Append(data);
  if (s.ok() && should_sync) {
    s = file->Sync();
  }
  if (!s.ok()) {
    env->DeleteFile(fname);
  }
  return s;
}

Status ReadFileToString(Env* env, const std::string& fname, std::string* data) {
  EnvOptions soptions;
  data->clear();
  unique_ptr<SequentialFile> file;
  Status s = env->NewSequentialFile(fname, &file, soptions);
  if (!s.ok()) {
    return s;
  }
  static const int kBufferSize = 8192;
  char* space = new char[kBufferSize];
  while (true) {
    Slice fragment;
    s = file->Read(kBufferSize, &fragment, space);
    if (!s.ok()) {
      break;
    }
    data->append(fragment.data(), fragment.size());
    if (fragment.empty()) {
      break;
    }
  }
  delete[] space;
  return s;
}

EnvWrapper::~EnvWrapper() {
}

namespace {  //匿名命名空间

void AssignEnvOptions(EnvOptions* env_options, const DBOptions& options) {
  env_options->use_mmap_reads = options.allow_mmap_reads;
  env_options->use_mmap_writes = options.allow_mmap_writes;
  env_options->use_direct_reads = options.use_direct_reads;
  env_options->set_fd_cloexec = options.is_fd_close_on_exec;
  env_options->bytes_per_sync = options.bytes_per_sync;
  env_options->compaction_readahead_size = options.compaction_readahead_size;
  env_options->random_access_max_buffer_size =
      options.random_access_max_buffer_size;
  env_options->rate_limiter = options.rate_limiter.get();
  env_options->writable_file_max_buffer_size =
      options.writable_file_max_buffer_size;
  env_options->allow_fallocate = options.allow_fallocate;
}

}

EnvOptions Env::OptimizeForLogWrite(const EnvOptions& env_options,
                                    const DBOptions& db_options) const {
  EnvOptions optimized_env_options(env_options);
  optimized_env_options.bytes_per_sync = db_options.wal_bytes_per_sync;
  return optimized_env_options;
}

EnvOptions Env::OptimizeForManifestWrite(const EnvOptions& env_options) const {
  return env_options;
}

EnvOptions Env::OptimizeForLogRead(const EnvOptions& env_options) const {
  EnvOptions optimized_env_options(env_options);
  optimized_env_options.use_direct_reads = false;
  return optimized_env_options;
}

EnvOptions Env::OptimizeForManifestRead(const EnvOptions& env_options) const {
  EnvOptions optimized_env_options(env_options);
  optimized_env_options.use_direct_reads = false;
  return optimized_env_options;
}

EnvOptions Env::OptimizeForCompactionTableWrite(
    const EnvOptions& env_options, const ImmutableDBOptions& db_options) const {
  EnvOptions optimized_env_options(env_options);
  optimized_env_options.use_direct_writes =
      db_options.use_direct_io_for_flush_and_compaction;
  return optimized_env_options;
}

EnvOptions Env::OptimizeForCompactionTableRead(
    const EnvOptions& env_options, const ImmutableDBOptions& db_options) const {
  EnvOptions optimized_env_options(env_options);
  optimized_env_options.use_direct_reads =
      db_options.use_direct_io_for_flush_and_compaction;
  return optimized_env_options;
}

EnvOptions::EnvOptions(const DBOptions& options) {
  AssignEnvOptions(this, options);
}

EnvOptions::EnvOptions() {
  DBOptions options;
  AssignEnvOptions(this, options);
}


}  //命名空间rocksdb
