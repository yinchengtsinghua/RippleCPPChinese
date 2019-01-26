
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

#pragma once
#include <algorithm>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include "port/sys_time.h"
#include "rocksdb/env.h"
#include "rocksdb/status.h"

#ifdef USE_HDFS
#include <hdfs.h>

namespace rocksdb {

//在执行过程中，当提供的
//争论。
class HdfsUsageException : public std::exception { };

//一个简单的异常，它表明出了问题，而不是
//可恢复的。其目的是打印信息（与
//没有别的），进程终止。
class HdfsFatalException : public std::exception {
public:
  explicit HdfsFatalException(const std::string& s) : what_(s) { }
  virtual ~HdfsFatalException() throw() { }
  virtual const char* what() const throw() {
    return what_.c_str();
  }
private:
  const std::string what_;
};

//
//RockSDB的HDFS环境。此类重写所有
//文件/目录访问方法，并将线程管理方法委托给
//默认POSIX环境。
//
class HdfsEnv : public Env {

 public:
  explicit HdfsEnv(const std::string& fsname) : fsname_(fsname) {
    posixEnv = Env::Default();
    fileSys_ = connectToPath(fsname_);
  }

  virtual ~HdfsEnv() {
    fprintf(stderr, "Destroying HdfsEnv::Default()\n");
    hdfsDisconnect(fileSys_);
  }

  virtual Status NewSequentialFile(const std::string& fname,
                                   std::unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options);

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     std::unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options);

  virtual Status NewWritableFile(const std::string& fname,
                                 std::unique_ptr<WritableFile>* result,
                                 const EnvOptions& options);

  virtual Status NewDirectory(const std::string& name,
                              std::unique_ptr<Directory>* result);

  virtual Status FileExists(const std::string& fname);

  virtual Status GetChildren(const std::string& path,
                             std::vector<std::string>* result);

  virtual Status DeleteFile(const std::string& fname);

  virtual Status CreateDir(const std::string& name);

  virtual Status CreateDirIfMissing(const std::string& name);

  virtual Status DeleteDir(const std::string& name);

  virtual Status GetFileSize(const std::string& fname, uint64_t* size);

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime);

  virtual Status RenameFile(const std::string& src, const std::string& target);

  virtual Status LinkFile(const std::string& src, const std::string& target) {
return Status::NotSupported(); //不支持
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock);

  virtual Status UnlockFile(FileLock* lock);

  virtual Status NewLogger(const std::string& fname,
                           std::shared_ptr<Logger>* result);

  virtual void Schedule(void (*function)(void* arg), void* arg,
                        Priority pri = LOW, void* tag = nullptr, void (*unschedFunction)(void* arg) = 0) {
    posixEnv->Schedule(function, arg, pri, tag, unschedFunction);
  }

  virtual int UnSchedule(void* tag, Priority pri) {
    return posixEnv->UnSchedule(tag, pri);
  }

  virtual void StartThread(void (*function)(void* arg), void* arg) {
    posixEnv->StartThread(function, arg);
  }

  virtual void WaitForJoin() { posixEnv->WaitForJoin(); }

  virtual unsigned int GetThreadPoolQueueLen(Priority pri = LOW) const
      override {
    return posixEnv->GetThreadPoolQueueLen(pri);
  }

  virtual Status GetTestDirectory(std::string* path) {
    return posixEnv->GetTestDirectory(path);
  }

  virtual uint64_t NowMicros() {
    return posixEnv->NowMicros();
  }

  virtual void SleepForMicroseconds(int micros) {
    posixEnv->SleepForMicroseconds(micros);
  }

  virtual Status GetHostName(char* name, uint64_t len) {
    return posixEnv->GetHostName(name, len);
  }

  virtual Status GetCurrentTime(int64_t* unix_time) {
    return posixEnv->GetCurrentTime(unix_time);
  }

  virtual Status GetAbsolutePath(const std::string& db_path,
      std::string* output_path) {
    return posixEnv->GetAbsolutePath(db_path, output_path);
  }

  virtual void SetBackgroundThreads(int number, Priority pri = LOW) {
    posixEnv->SetBackgroundThreads(number, pri);
  }

  virtual int GetBackgroundThreads(Priority pri = LOW) {
    return posixEnv->GetBackgroundThreads(pri);
  }

  virtual void IncBackgroundThreadsIfNeeded(int number, Priority pri) override {
    posixEnv->IncBackgroundThreadsIfNeeded(number, pri);
  }

  virtual std::string TimeToString(uint64_t number) {
    return posixEnv->TimeToString(number);
  }

  static uint64_t gettid() {
    assert(sizeof(pthread_t) <= sizeof(uint64_t));
    return (uint64_t)pthread_self();
  }

  virtual uint64_t GetThreadID() const override {
    return HdfsEnv::gettid();
  }

 private:
std::string fsname_;  //格式为“hdfs://hostname:port/”的字符串
hdfsFS fileSys_;      //所有文件的单个文件系统对象
Env*  posixEnv;       //此对象是从env派生的，但不是从
//正点的我们把posixnv作为一个封装的
//对象，以便我们可以使用posix计时器，
//POSIX线程等。

  static const std::string kProto;
  static const std::string pathsep;

  /*
   *如果以hdfs://server:port/path的形式指定了URI，
   *然后连接到指定的群集
   *否则连接到默认值。
   **/

  hdfsFS connectToPath(const std::string& uri) {
    if (uri.empty()) {
      return nullptr;
    }
    if (uri.find(kProto) != 0) {
//uri不以hdfs://->开头，使用默认值：0，这是特殊的
//到LBHDFs。
      return hdfsConnectNewInstance("default", 0);
    }
    const std::string hostport = uri.substr(kProto.length());

    std::vector <std::string> parts;
    split(hostport, ':', parts);
    if (parts.size() != 2) {
      throw HdfsFatalException("Bad uri for hdfs " + uri);
    }
//零件[0]=主机，零件[1]=端口/XXX/YYY
    std::string host(parts[0]);
    std::string remaining(parts[1]);

    int rem = remaining.find(pathsep);
    std::string portStr = (rem == 0 ? remaining :
                           remaining.substr(0, rem));

    tPort port;
    port = atoi(portStr.c_str());
    if (port == 0) {
      throw HdfsFatalException("Bad host-port for hdfs " + uri);
    }
    hdfsFS fs = hdfsConnectNewInstance(host.c_str(), port);
    return fs;
  }

  void split(const std::string &s, char delim,
             std::vector<std::string> &elems) {
    elems.clear();
    size_t prev = 0;
    size_t pos = s.find(delim);
    while (pos != std::string::npos) {
      elems.push_back(s.substr(prev, pos));
      prev = pos + 1;
      pos = s.find(delim, prev);
    }
    elems.push_back(s.substr(prev, s.size()));
  }
};

}  //命名空间rocksdb

#else //使用HDFS


namespace rocksdb {

static const Status notsup;

class HdfsEnv : public Env {

 public:
  explicit HdfsEnv(const std::string& fsname) {
    fprintf(stderr, "You have not build rocksdb with HDFS support\n");
    fprintf(stderr, "Please see hdfs/README for details\n");
    abort();
  }

  virtual ~HdfsEnv() {
  }

  virtual Status NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) override;

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options) override {
    return notsup;
  }

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) override {
    return notsup;
  }

  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) override {
    return notsup;
  }

  virtual Status FileExists(const std::string& fname) override {
    return notsup;
  }

  virtual Status GetChildren(const std::string& path,
                             std::vector<std::string>* result) override {
    return notsup;
  }

  virtual Status DeleteFile(const std::string& fname) override {
    return notsup;
  }

  virtual Status CreateDir(const std::string& name) override { return notsup; }

  virtual Status CreateDirIfMissing(const std::string& name) override {
    return notsup;
  }

  virtual Status DeleteDir(const std::string& name) override { return notsup; }

  virtual Status GetFileSize(const std::string& fname,
                             uint64_t* size) override {
    return notsup;
  }

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* time) override {
    return notsup;
  }

  virtual Status RenameFile(const std::string& src,
                            const std::string& target) override {
    return notsup;
  }

  virtual Status LinkFile(const std::string& src,
                          const std::string& target) override {
    return notsup;
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock) override {
    return notsup;
  }

  virtual Status UnlockFile(FileLock* lock) override { return notsup; }

  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) override {
    return notsup;
  }

  virtual void Schedule(void (*function)(void* arg), void* arg,
                        Priority pri = LOW, void* tag = nullptr,
                        void (*unschedFunction)(void* arg) = 0) override {}

  virtual int UnSchedule(void* tag, Priority pri) override { return 0; }

  virtual void StartThread(void (*function)(void* arg), void* arg) override {}

  virtual void WaitForJoin() override {}

  virtual unsigned int GetThreadPoolQueueLen(
      Priority pri = LOW) const override {
    return 0;
  }

  virtual Status GetTestDirectory(std::string* path) override { return notsup; }

  virtual uint64_t NowMicros() override { return 0; }

  virtual void SleepForMicroseconds(int micros) override {}

  virtual Status GetHostName(char* name, uint64_t len) override {
    return notsup;
  }

  virtual Status GetCurrentTime(int64_t* unix_time) override { return notsup; }

  virtual Status GetAbsolutePath(const std::string& db_path,
                                 std::string* outputpath) override {
    return notsup;
  }

  virtual void SetBackgroundThreads(int number, Priority pri = LOW) override {}
  virtual int GetBackgroundThreads(Priority pri = LOW) override { return 0; }
  virtual void IncBackgroundThreadsIfNeeded(int number, Priority pri) override {
  }
  virtual std::string TimeToString(uint64_t number) override { return ""; }

  virtual uint64_t GetThreadID() const override {
    return 0;
  }
};
}

#endif //使用HDFS
