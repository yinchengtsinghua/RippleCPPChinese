
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
//
//env是rocksdb实现用于访问的接口
//操作系统功能，如文件系统等调用程序
//打开数据库时可能希望提供自定义env对象
//获得精细的增益控制；例如，速率限制文件系统操作。
//
//所有env实现对来自的并发访问都是安全的。
//多线程，没有任何外部同步。

#pragma once

#include "port/win/win_thread.h"
#include <rocksdb/env.h>
#include "util/threadpool_imp.h"

#include <stdint.h>
#include <windows.h>

#include <mutex>
#include <vector>
#include <string>


#undef GetCurrentTime
#undef DeleteFile
#undef GetTickCount

namespace rocksdb {
namespace port {

//目前不是为继承而设计的，而是为替换而设计的
class WinEnvThreads {
public:

  explicit WinEnvThreads(Env* hosted_env);

  ~WinEnvThreads();

  WinEnvThreads(const WinEnvThreads&) = delete;
  WinEnvThreads& operator=(const WinEnvThreads&) = delete;

  void Schedule(void(*function)(void*), void* arg, Env::Priority pri,
    void* tag,
    void(*unschedFunction)(void* arg));

  int UnSchedule(void* arg, Env::Priority pri);

  void StartThread(void(*function)(void* arg), void* arg);

  void WaitForJoin();

  unsigned int GetThreadPoolQueueLen(Env::Priority pri) const;

  static uint64_t gettid();

  uint64_t GetThreadID() const;

  void SleepForMicroseconds(int micros);

//允许增加工作线程数。
  void SetBackgroundThreads(int num, Env::Priority pri);
  int GetBackgroundThreads(Env::Priority pri);

  void IncBackgroundThreadsIfNeeded(int num, Env::Priority pri);

private:

  Env*                     hosted_env_;
  mutable std::mutex       mu_;
  std::vector<ThreadPoolImpl> thread_pools_;
  std::vector<WindowsThread> threads_to_join_;

};

//为继承而设计，因此可以重复使用
//但某些部件被更换了
class WinEnvIO {
public:
  explicit WinEnvIO(Env* hosted_env);

  virtual ~WinEnvIO();

  virtual Status DeleteFile(const std::string& fname);

  virtual Status GetCurrentTime(int64_t* unix_time);

  virtual Status NewSequentialFile(const std::string& fname,
    std::unique_ptr<SequentialFile>* result,
    const EnvOptions& options);

//newwritable和reopenwritablefile的帮助程序
  virtual Status OpenWritableFile(const std::string& fname,
    std::unique_ptr<WritableFile>* result,
    const EnvOptions& options,
    bool reopen);

  virtual Status NewRandomAccessFile(const std::string& fname,
    std::unique_ptr<RandomAccessFile>* result,
    const EnvOptions& options);

//返回的文件一次只能由一个线程访问。
  virtual Status NewRandomRWFile(const std::string& fname,
    unique_ptr<RandomRWFile>* result,
    const EnvOptions& options);

  virtual Status NewDirectory(const std::string& name,
    std::unique_ptr<Directory>* result);

  virtual Status FileExists(const std::string& fname);

  virtual Status GetChildren(const std::string& dir,
    std::vector<std::string>* result);

  virtual Status CreateDir(const std::string& name);

  virtual Status CreateDirIfMissing(const std::string& name);

  virtual Status DeleteDir(const std::string& name);

  virtual Status GetFileSize(const std::string& fname,
    uint64_t* size);

  static uint64_t FileTimeToUnixTime(const FILETIME& ftTime);

  virtual Status GetFileModificationTime(const std::string& fname,
    uint64_t* file_mtime);

  virtual Status RenameFile(const std::string& src,
    const std::string& target);

  virtual Status LinkFile(const std::string& src,
    const std::string& target);

  virtual Status LockFile(const std::string& lockFname,
    FileLock** lock);

  virtual Status UnlockFile(FileLock* lock);

  virtual Status GetTestDirectory(std::string* result);

  virtual Status NewLogger(const std::string& fname,
    std::shared_ptr<Logger>* result);

  virtual uint64_t NowMicros();

  virtual uint64_t NowNanos();

  virtual Status GetHostName(char* name, uint64_t len);

  virtual Status GetAbsolutePath(const std::string& db_path,
    std::string* output_path);

  virtual std::string TimeToString(uint64_t secondsSince1970);

  virtual EnvOptions OptimizeForLogWrite(const EnvOptions& env_options,
    const DBOptions& db_options) const;

  virtual EnvOptions OptimizeForManifestWrite(
    const EnvOptions& env_options) const;

  size_t GetPageSize() const { return page_size_; }

  size_t GetAllocationGranularity() const { return allocation_granularity_; }

  uint64_t GetPerfCounterFrequency() const { return perf_counter_frequency_; }

private:
//如果指定的目录存在并且是目录，则返回true。
  virtual bool DirExists(const std::string& dname);

  typedef VOID(WINAPI * FnGetSystemTimePreciseAsFileTime)(LPFILETIME);

  Env*            hosted_env_;
  size_t          page_size_;
  size_t          allocation_granularity_;
  uint64_t        perf_counter_frequency_;
  FnGetSystemTimePreciseAsFileTime GetSystemTimePreciseAsFileTime_;
};

class WinEnv : public Env {
public:
  WinEnv();

  ~WinEnv();

  Status DeleteFile(const std::string& fname) override;

  Status GetCurrentTime(int64_t* unix_time) override;

  Status NewSequentialFile(const std::string& fname,
    std::unique_ptr<SequentialFile>* result,
    const EnvOptions& options) override;

  Status NewRandomAccessFile(const std::string& fname,
    std::unique_ptr<RandomAccessFile>* result,
    const EnvOptions& options) override;

  Status NewWritableFile(const std::string& fname,
                         std::unique_ptr<WritableFile>* result,
                         const EnvOptions& options) override;

//创建一个对象，该对象使用指定的
//姓名。删除具有相同名称的任何现有文件并创建
//新文件。成功后，将指向新文件的指针存储在
//*结果并返回OK。失败时将nullptr存储在*result和
//返回非OK。
//
//返回的文件一次只能由一个线程访问。
  Status ReopenWritableFile(const std::string& fname,
    std::unique_ptr<WritableFile>* result,
    const EnvOptions& options) override;

//返回的文件一次只能由一个线程访问。
  Status NewRandomRWFile(const std::string& fname,
    unique_ptr<RandomRWFile>* result,
    const EnvOptions& options) override;

  Status NewDirectory(const std::string& name,
    std::unique_ptr<Directory>* result) override;

  Status FileExists(const std::string& fname) override;

  Status GetChildren(const std::string& dir,
    std::vector<std::string>* result) override;

  Status CreateDir(const std::string& name) override;

  Status CreateDirIfMissing(const std::string& name) override;

  Status DeleteDir(const std::string& name) override;

  Status GetFileSize(const std::string& fname,
    uint64_t* size) override;

  Status GetFileModificationTime(const std::string& fname,
    uint64_t* file_mtime) override;

  Status RenameFile(const std::string& src,
    const std::string& target) override;

  Status LinkFile(const std::string& src,
    const std::string& target) override;

  Status LockFile(const std::string& lockFname,
    FileLock** lock) override;

  Status UnlockFile(FileLock* lock) override;

  Status GetTestDirectory(std::string* result) override;

  Status NewLogger(const std::string& fname,
    std::shared_ptr<Logger>* result) override;

  uint64_t NowMicros() override;

  uint64_t NowNanos() override;

  Status GetHostName(char* name, uint64_t len) override;

  Status GetAbsolutePath(const std::string& db_path,
    std::string* output_path) override;

  std::string TimeToString(uint64_t secondsSince1970) override;

  Status GetThreadList(
    std::vector<ThreadStatus>* thread_list) override;

  void Schedule(void(*function)(void*), void* arg, Env::Priority pri,
    void* tag,
    void(*unschedFunction)(void* arg)) override;

  int UnSchedule(void* arg, Env::Priority pri) override;

  void StartThread(void(*function)(void* arg), void* arg) override;

  void WaitForJoin();

  unsigned int GetThreadPoolQueueLen(Env::Priority pri) const override;

  uint64_t GetThreadID() const override;

  void SleepForMicroseconds(int micros) override;

//允许增加工作线程数。
  void SetBackgroundThreads(int num, Env::Priority pri) override;
  int GetBackgroundThreads(Env::Priority pri) override;

  void IncBackgroundThreadsIfNeeded(int num, Env::Priority pri) override;

  EnvOptions OptimizeForLogWrite(const EnvOptions& env_options,
    const DBOptions& db_options) const override;

  EnvOptions OptimizeForManifestWrite(
    const EnvOptions& env_options) const override;

private:

  WinEnvIO      winenv_io_;
  WinEnvThreads winenv_threads_;
};

} //命名空间端口
} //命名空间rocksdb
