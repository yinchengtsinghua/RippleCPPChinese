
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
//版权所有2014 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

//此测试使用自定义env跟踪文件系统的状态
//最后一个“同步”。然后，它通过故意删除来检查数据丢失错误
//文件数据（或整个文件）不受“同步”保护。

#ifndef UTIL_FAULT_INJECTION_TEST_ENV_H_
#define UTIL_FAULT_INJECTION_TEST_ENV_H_

#include <map>
#include <set>
#include <string>

#include "db/version_set.h"
#include "env/mock_env.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "util/filename.h"
#include "util/mutexlock.h"
#include "util/random.h"

namespace rocksdb {

class TestWritableFile;
class FaultInjectionTestEnv;

struct FileState {
  std::string filename_;
  ssize_t pos_;
  ssize_t pos_at_last_sync_;
  ssize_t pos_at_last_flush_;

  explicit FileState(const std::string& filename)
      : filename_(filename),
        pos_(-1),
        pos_at_last_sync_(-1),
        pos_at_last_flush_(-1) {}

  FileState() : pos_(-1), pos_at_last_sync_(-1), pos_at_last_flush_(-1) {}

  bool IsFullySynced() const { return pos_ <= 0 || pos_ == pos_at_last_sync_; }

  Status DropUnsyncedData(Env* env) const;

  Status DropRandomUnsyncedData(Env* env, Random* rand) const;
};

//writablefilewriter*文件的包装
//写入或同步。
class TestWritableFile : public WritableFile {
 public:
  explicit TestWritableFile(const std::string& fname,
                            unique_ptr<WritableFile>&& f,
                            FaultInjectionTestEnv* env);
  virtual ~TestWritableFile();
  virtual Status Append(const Slice& data) override;
  virtual Status Truncate(uint64_t size) override {
    return target_->Truncate(size);
  }
  virtual Status Close() override;
  virtual Status Flush() override;
  virtual Status Sync() override;
  virtual bool IsSyncThreadSafe() const override { return true; }

 private:
  FileState state_;
  unique_ptr<WritableFile> target_;
  bool writable_file_opened_;
  FaultInjectionTestEnv* env_;
};

class TestDirectory : public Directory {
 public:
  explicit TestDirectory(FaultInjectionTestEnv* env, std::string dirname,
                         Directory* dir)
      : env_(env), dirname_(dirname), dir_(dir) {}
  ~TestDirectory() {}

  virtual Status Fsync() override;

 private:
  FaultInjectionTestEnv* env_;
  std::string dirname_;
  unique_ptr<Directory> dir_;
};

class FaultInjectionTestEnv : public EnvWrapper {
 public:
  explicit FaultInjectionTestEnv(Env* base)
      : EnvWrapper(base), filesystem_active_(true) {}
  virtual ~FaultInjectionTestEnv() {}

  Status NewDirectory(const std::string& name,
                      unique_ptr<Directory>* result) override;

  Status NewWritableFile(const std::string& fname,
                         unique_ptr<WritableFile>* result,
                         const EnvOptions& soptions) override;

  virtual Status DeleteFile(const std::string& f) override;

  virtual Status RenameFile(const std::string& s,
                            const std::string& t) override;

  void WritableFileClosed(const FileState& state);

//对于未完全同步的每个文件，调用“func”
//作为参数的文件状态。
  Status DropFileData(std::function<Status(Env*, FileState)> func);

  Status DropUnsyncedFileData();

  Status DropRandomUnsyncedFileData(Random* rnd);

  Status DeleteFilesCreatedAfterLastDirSync();

  void ResetState();

  void UntrackFile(const std::string& f);

  void SyncDir(const std::string& dirname) {
    MutexLock l(&mutex_);
    dir_to_new_files_since_last_sync_.erase(dirname);
  }

//将文件系统设置为非活动的测试相当于模拟
//系统复位。设置为inactive将冻结保存的文件系统状态，因此
//它将停止被记录。然后可以将其重置回状态
//重置时间。
  bool IsFilesystemActive() {
    MutexLock l(&mutex_);
    return filesystem_active_;
  }
  void SetFilesystemActiveNoLock(bool active) { filesystem_active_ = active; }
  void SetFilesystemActive(bool active) {
    MutexLock l(&mutex_);
    SetFilesystemActiveNoLock(active);
  }
  void AssertNoOpenFile() { assert(open_files_.empty()); }

 private:
  port::Mutex mutex_;
  std::map<std::string, FileState> db_file_state_;
  std::set<std::string> open_files_;
  std::unordered_map<std::string, std::set<std::string>>
      dir_to_new_files_since_last_sync_;
bool filesystem_active_;  //记录刷新、同步、写入
};

}  //命名空间rocksdb

#endif  //使用故障注入测试
