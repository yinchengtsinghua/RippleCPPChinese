
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
#pragma once

#include <atomic>
#include <map>
#include <string>
#include <vector>
#include "rocksdb/env.h"
#include "rocksdb/status.h"
#include "port/port.h"
#include "util/mutexlock.h"

namespace rocksdb {

class MemFile;
class MockEnv : public EnvWrapper {
 public:
  explicit MockEnv(Env* base_env);

  virtual ~MockEnv();

//env接口的部分实现。
  virtual Status NewSequentialFile(const std::string& fname,
                                   unique_ptr<SequentialFile>* result,
                                   const EnvOptions& soptions) override;

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& soptions) override;

  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) override;

  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override;

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& env_options) override;

  virtual Status NewDirectory(const std::string& name,
                              unique_ptr<Directory>* result) override;

  virtual Status FileExists(const std::string& fname) override;

  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) override;

  void DeleteFileInternal(const std::string& fname);

  virtual Status DeleteFile(const std::string& fname) override;

  virtual Status CreateDir(const std::string& dirname) override;

  virtual Status CreateDirIfMissing(const std::string& dirname) override;

  virtual Status DeleteDir(const std::string& dirname) override;

  virtual Status GetFileSize(const std::string& fname,
                             uint64_t* file_size) override;

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* time) override;

  virtual Status RenameFile(const std::string& src,
                            const std::string& target) override;

  virtual Status LinkFile(const std::string& src,
                          const std::string& target) override;

  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) override;

  virtual Status LockFile(const std::string& fname, FileLock** flock) override;

  virtual Status UnlockFile(FileLock* flock) override;

  virtual Status GetTestDirectory(std::string* path) override;

//这些结果可能会受到fakesleepformicroseconds（）的影响。
  virtual Status GetCurrentTime(int64_t* unix_time) override;
  virtual uint64_t NowMicros() override;
  virtual uint64_t NowNanos() override;

//非虚拟函数，特定于MOCKENV
  Status Truncate(const std::string& fname, size_t size);

  Status CorruptBuffer(const std::string& fname);

//不会真正休眠，只影响getcurrentTime（）、nowmicros（）的输出
//和NoNANOSO（）
  void FakeSleepForMicroseconds(int64_t micros);

 private:
  std::string NormalizePath(const std::string path);

//从文件名映射到memfile对象，表示一个简单的文件系统。
  typedef std::map<std::string, MemFile*> FileSystem;
  port::Mutex mutex_;
FileSystem file_map_;  //由互斥保护。

  std::atomic<int64_t> fake_sleep_micros_;
};

}  //命名空间rocksdb
