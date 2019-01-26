
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

#include "util/fault_injection_test_env.h"
#include <functional>
#include <utility>

namespace rocksdb {

//假定文件名，而不是目录名，如“/foo/bar/”
std::string GetDirName(const std::string filename) {
  size_t found = filename.find_last_of("/\\");
  if (found == std::string::npos) {
    return "";
  } else {
    return filename.substr(0, found);
  }
}

//适用于此测试的基本文件截断函数。
Status Truncate(Env* env, const std::string& filename, uint64_t length) {
  unique_ptr<SequentialFile> orig_file;
  const EnvOptions options;
  Status s = env->NewSequentialFile(filename, &orig_file, options);
  if (!s.ok()) {
    fprintf(stderr, "Cannot truncate file %s: %s\n", filename.c_str(),
            s.ToString().c_str());
    return s;
  }

  std::unique_ptr<char[]> scratch(new char[length]);
  rocksdb::Slice result;
  s = orig_file->Read(length, &result, scratch.get());
#ifdef OS_WIN
  orig_file.reset();
#endif
  if (s.ok()) {
    std::string tmp_name = GetDirName(filename) + "/truncate.tmp";
    unique_ptr<WritableFile> tmp_file;
    s = env->NewWritableFile(tmp_name, &tmp_file, options);
    if (s.ok()) {
      s = tmp_file->Append(result);
      if (s.ok()) {
        s = env->RenameFile(tmp_name, filename);
      } else {
        fprintf(stderr, "Cannot rename file %s to %s: %s\n", tmp_name.c_str(),
                filename.c_str(), s.ToString().c_str());
        env->DeleteFile(tmp_name);
      }
    }
  }
  if (!s.ok()) {
    fprintf(stderr, "Cannot truncate file %s: %s\n", filename.c_str(),
            s.ToString().c_str());
  }

  return s;
}

//修剪“str”末尾的“tailing”/“”
std::string TrimDirname(const std::string& str) {
  size_t found = str.find_last_not_of("/");
  if (found == std::string::npos) {
    return str;
  }
  return str.substr(0, found + 1);
}

//返回完整路径的对<父目录名，文件名>。
std::pair<std::string, std::string> GetDirAndName(const std::string& name) {
  std::string dirname = GetDirName(name);
  std::string fname = name.substr(dirname.size() + 1);
  return std::make_pair(dirname, fname);
}

Status FileState::DropUnsyncedData(Env* env) const {
  ssize_t sync_pos = pos_at_last_sync_ == -1 ? 0 : pos_at_last_sync_;
  return Truncate(env, filename_, sync_pos);
}

Status FileState::DropRandomUnsyncedData(Env* env, Random* rand) const {
  ssize_t sync_pos = pos_at_last_sync_ == -1 ? 0 : pos_at_last_sync_;
  assert(pos_ >= sync_pos);
  int range = static_cast<int>(pos_ - sync_pos);
  uint64_t truncated_size =
      static_cast<uint64_t>(sync_pos) + rand->Uniform(range);
  return Truncate(env, filename_, truncated_size);
}

Status TestDirectory::Fsync() {
  env_->SyncDir(dirname_);
  return dir_->Fsync();
}

TestWritableFile::TestWritableFile(const std::string& fname,
                                   unique_ptr<WritableFile>&& f,
                                   FaultInjectionTestEnv* env)
    : state_(fname),
      target_(std::move(f)),
      writable_file_opened_(true),
      env_(env) {
  assert(target_ != nullptr);
  state_.pos_ = 0;
}

TestWritableFile::~TestWritableFile() {
  if (writable_file_opened_) {
    Close();
  }
}

Status TestWritableFile::Append(const Slice& data) {
  if (!env_->IsFilesystemActive()) {
    return Status::Corruption("Not Active");
  }
  Status s = target_->Append(data);
  if (s.ok()) {
    state_.pos_ += data.size();
  }
  return s;
}

Status TestWritableFile::Close() {
  writable_file_opened_ = false;
  Status s = target_->Close();
  if (s.ok()) {
    env_->WritableFileClosed(state_);
  }
  return s;
}

Status TestWritableFile::Flush() {
  Status s = target_->Flush();
  if (s.ok() && env_->IsFilesystemActive()) {
    state_.pos_at_last_flush_ = state_.pos_;
  }
  return s;
}

Status TestWritableFile::Sync() {
  if (!env_->IsFilesystemActive()) {
    return Status::IOError("FaultInjectionTestEnv: not active");
  }
//不需要实际同步。
  state_.pos_at_last_sync_ = state_.pos_;
  return Status::OK();
}

Status FaultInjectionTestEnv::NewDirectory(const std::string& name,
                                           unique_ptr<Directory>* result) {
  unique_ptr<Directory> r;
  Status s = target()->NewDirectory(name, &r);
  assert(s.ok());
  if (!s.ok()) {
    return s;
  }
  result->reset(new TestDirectory(this, TrimDirname(name), r.release()));
  return Status::OK();
}

Status FaultInjectionTestEnv::NewWritableFile(const std::string& fname,
                                              unique_ptr<WritableFile>* result,
                                              const EnvOptions& soptions) {
  if (!IsFilesystemActive()) {
    return Status::Corruption("Not Active");
  }
//不允许覆盖文件
  Status s = target()->FileExists(fname);
  if (s.ok()) {
    return Status::Corruption("File already exists.");
  } else if (!s.IsNotFound()) {
    assert(s.IsIOError());
    return s;
  }
  s = target()->NewWritableFile(fname, result, soptions);
  if (s.ok()) {
    result->reset(new TestWritableFile(fname, std::move(*result), this));
//可写入文件写入程序*文件已打开
//再次，它将被截断-所以忘记我们保存的状态。
    UntrackFile(fname);
    MutexLock l(&mutex_);
    open_files_.insert(fname);
    auto dir_and_name = GetDirAndName(fname);
    auto& list = dir_to_new_files_since_last_sync_[dir_and_name.first];
    list.insert(dir_and_name.second);
  }
  return s;
}

Status FaultInjectionTestEnv::DeleteFile(const std::string& f) {
  if (!IsFilesystemActive()) {
    return Status::Corruption("Not Active");
  }
  Status s = EnvWrapper::DeleteFile(f);
  if (!s.ok()) {
    fprintf(stderr, "Cannot delete file %s: %s\n", f.c_str(),
            s.ToString().c_str());
  }
  assert(s.ok());
  if (s.ok()) {
    UntrackFile(f);
  }
  return s;
}

Status FaultInjectionTestEnv::RenameFile(const std::string& s,
                                         const std::string& t) {
  if (!IsFilesystemActive()) {
    return Status::Corruption("Not Active");
  }
  Status ret = EnvWrapper::RenameFile(s, t);

  if (ret.ok()) {
    MutexLock l(&mutex_);
    if (db_file_state_.find(s) != db_file_state_.end()) {
      db_file_state_[t] = db_file_state_[s];
      db_file_state_.erase(s);
    }

    auto sdn = GetDirAndName(s);
    auto tdn = GetDirAndName(t);
    if (dir_to_new_files_since_last_sync_[sdn.first].erase(sdn.second) != 0) {
      auto& tlist = dir_to_new_files_since_last_sync_[tdn.first];
      assert(tlist.find(tdn.second) == tlist.end());
      tlist.insert(tdn.second);
    }
  }

  return ret;
}

void FaultInjectionTestEnv::WritableFileClosed(const FileState& state) {
  MutexLock l(&mutex_);
  if (open_files_.find(state.filename_) != open_files_.end()) {
    db_file_state_[state.filename_] = state;
    open_files_.erase(state.filename_);
  }
}

//对于未完全同步的每个文件，调用“func”
//作为参数的文件状态。
Status FaultInjectionTestEnv::DropFileData(
    std::function<Status(Env*, FileState)> func) {
  Status s;
  MutexLock l(&mutex_);
  for (std::map<std::string, FileState>::const_iterator it =
           db_file_state_.begin();
       s.ok() && it != db_file_state_.end(); ++it) {
    const FileState& state = it->second;
    if (!state.IsFullySynced()) {
      s = func(target(), state);
    }
  }
  return s;
}

Status FaultInjectionTestEnv::DropUnsyncedFileData() {
  return DropFileData([&](Env* env, const FileState& state) {
    return state.DropUnsyncedData(env);
  });
}

Status FaultInjectionTestEnv::DropRandomUnsyncedFileData(Random* rnd) {
  return DropFileData([&](Env* env, const FileState& state) {
    return state.DropRandomUnsyncedData(env, rnd);
  });
}

Status FaultInjectionTestEnv::DeleteFilesCreatedAfterLastDirSync() {
//因为deletefile访问这个容器复制以避免死锁
  std::map<std::string, std::set<std::string>> map_copy;
  {
    MutexLock l(&mutex_);
    map_copy.insert(dir_to_new_files_since_last_sync_.begin(),
                    dir_to_new_files_since_last_sync_.end());
  }

  for (auto& pair : map_copy) {
    for (std::string name : pair.second) {
      Status s = DeleteFile(pair.first + "/" + name);
      if (!s.ok()) {
        return s;
      }
    }
  }
  return Status::OK();
}
void FaultInjectionTestEnv::ResetState() {
  MutexLock l(&mutex_);
  db_file_state_.clear();
  dir_to_new_files_since_last_sync_.clear();
  SetFilesystemActiveNoLock(true);
}

void FaultInjectionTestEnv::UntrackFile(const std::string& f) {
  MutexLock l(&mutex_);
  auto dir_and_name = GetDirAndName(f);
  dir_to_new_files_since_last_sync_[dir_and_name.first].erase(
      dir_and_name.second);
  db_file_state_.erase(f);
  open_files_.erase(f);
}
}  //命名空间rocksdb
