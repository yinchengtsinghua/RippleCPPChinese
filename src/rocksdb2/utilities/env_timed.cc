
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#include "monitoring/perf_context_imp.h"
#include "rocksdb/env.h"
#include "rocksdb/status.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE

//测量文件系统函数调用时间的环境
//操作，将结果报告给PerfContext中的变量。
class TimedEnv : public EnvWrapper {
 public:
  explicit TimedEnv(Env* base_env) : EnvWrapper(base_env) {}

  virtual Status NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) override {
    PERF_TIMER_GUARD(env_new_sequential_file_nanos);
    return EnvWrapper::NewSequentialFile(fname, result, options);
  }

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options) override {
    PERF_TIMER_GUARD(env_new_random_access_file_nanos);
    return EnvWrapper::NewRandomAccessFile(fname, result, options);
  }

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) override {
    PERF_TIMER_GUARD(env_new_writable_file_nanos);
    return EnvWrapper::NewWritableFile(fname, result, options);
  }

  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override {
    PERF_TIMER_GUARD(env_reuse_writable_file_nanos);
    return EnvWrapper::ReuseWritableFile(fname, old_fname, result, options);
  }

  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) override {
    PERF_TIMER_GUARD(env_new_random_rw_file_nanos);
    return EnvWrapper::NewRandomRWFile(fname, result, options);
  }

  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) override {
    PERF_TIMER_GUARD(env_new_directory_nanos);
    return EnvWrapper::NewDirectory(name, result);
  }

  virtual Status FileExists(const std::string& fname) override {
    PERF_TIMER_GUARD(env_file_exists_nanos);
    return EnvWrapper::FileExists(fname);
  }

  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) override {
    PERF_TIMER_GUARD(env_get_children_nanos);
    return EnvWrapper::GetChildren(dir, result);
  }

  virtual Status GetChildrenFileAttributes(
      const std::string& dir, std::vector<FileAttributes>* result) override {
    PERF_TIMER_GUARD(env_get_children_file_attributes_nanos);
    return EnvWrapper::GetChildrenFileAttributes(dir, result);
  }

  virtual Status DeleteFile(const std::string& fname) override {
    PERF_TIMER_GUARD(env_delete_file_nanos);
    return EnvWrapper::DeleteFile(fname);
  }

  virtual Status CreateDir(const std::string& dirname) override {
    PERF_TIMER_GUARD(env_create_dir_nanos);
    return EnvWrapper::CreateDir(dirname);
  }

  virtual Status CreateDirIfMissing(const std::string& dirname) override {
    PERF_TIMER_GUARD(env_create_dir_if_missing_nanos);
    return EnvWrapper::CreateDirIfMissing(dirname);
  }

  virtual Status DeleteDir(const std::string& dirname) override {
    PERF_TIMER_GUARD(env_delete_dir_nanos);
    return EnvWrapper::DeleteDir(dirname);
  }

  virtual Status GetFileSize(const std::string& fname,
                             uint64_t* file_size) override {
    PERF_TIMER_GUARD(env_get_file_size_nanos);
    return EnvWrapper::GetFileSize(fname, file_size);
  }

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime) override {
    PERF_TIMER_GUARD(env_get_file_modification_time_nanos);
    return EnvWrapper::GetFileModificationTime(fname, file_mtime);
  }

  virtual Status RenameFile(const std::string& src,
                            const std::string& dst) override {
    PERF_TIMER_GUARD(env_rename_file_nanos);
    return EnvWrapper::RenameFile(src, dst);
  }

  virtual Status LinkFile(const std::string& src,
                          const std::string& dst) override {
    PERF_TIMER_GUARD(env_link_file_nanos);
    return EnvWrapper::LinkFile(src, dst);
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock) override {
    PERF_TIMER_GUARD(env_lock_file_nanos);
    return EnvWrapper::LockFile(fname, lock);
  }

  virtual Status UnlockFile(FileLock* lock) override {
    PERF_TIMER_GUARD(env_unlock_file_nanos);
    return EnvWrapper::UnlockFile(lock);
  }

  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) override {
    PERF_TIMER_GUARD(env_new_logger_nanos);
    return EnvWrapper::NewLogger(fname, result);
  }
};

Env* NewTimedEnv(Env* base_env) { return new TimedEnv(base_env); }

#else  //摇滚乐

Env* NewTimedEnv(Env* base_env) { return nullptr; }

#endif  //！摇滚乐

}  //命名空间rocksdb
