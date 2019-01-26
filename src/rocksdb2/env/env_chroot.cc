
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2016至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#if !defined(ROCKSDB_LITE) && !defined(OS_WIN)

#include "env/env_chroot.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <utility>
#include <vector>

#include "rocksdb/status.h"

namespace rocksdb {

class ChrootEnv : public EnvWrapper {
 public:
  ChrootEnv(Env* base_env, const std::string& chroot_dir)
      : EnvWrapper(base_env) {
#if defined(OS_AIX)
    char resolvedName[PATH_MAX];
    char* real_chroot_dir = realpath(chroot_dir.c_str(), resolvedName);
#else
    char* real_chroot_dir = realpath(chroot_dir.c_str(), nullptr);
#endif
//chroot_dir必须存在，因此realpath（）返回非nullptr。
    assert(real_chroot_dir != nullptr);
    chroot_dir_ = real_chroot_dir;
#if !defined(OS_AIX)
    free(real_chroot_dir);
#endif
  }

  virtual Status NewSequentialFile(const std::string& fname,
                                   std::unique_ptr<SequentialFile>* result,
                                   const EnvOptions& options) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewSequentialFile(status_and_enc_path.second, result,
                                         options);
  }

  virtual Status NewRandomAccessFile(const std::string& fname,
                                     unique_ptr<RandomAccessFile>* result,
                                     const EnvOptions& options) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewRandomAccessFile(status_and_enc_path.second, result,
                                           options);
  }

  virtual Status NewWritableFile(const std::string& fname,
                                 unique_ptr<WritableFile>* result,
                                 const EnvOptions& options) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewWritableFile(status_and_enc_path.second, result,
                                       options);
  }

  virtual Status ReuseWritableFile(const std::string& fname,
                                   const std::string& old_fname,
                                   unique_ptr<WritableFile>* result,
                                   const EnvOptions& options) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    auto status_and_old_enc_path = EncodePath(old_fname);
    if (!status_and_old_enc_path.first.ok()) {
      return status_and_old_enc_path.first;
    }
    return EnvWrapper::ReuseWritableFile(status_and_old_enc_path.second,
                                         status_and_old_enc_path.second, result,
                                         options);
  }

  virtual Status NewRandomRWFile(const std::string& fname,
                                 unique_ptr<RandomRWFile>* result,
                                 const EnvOptions& options) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewRandomRWFile(status_and_enc_path.second, result,
                                       options);
  }

  virtual Status NewDirectory(const std::string& dir,
                              unique_ptr<Directory>* result) override {
    auto status_and_enc_path = EncodePathWithNewBasename(dir);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewDirectory(status_and_enc_path.second, result);
  }

  virtual Status FileExists(const std::string& fname) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::FileExists(status_and_enc_path.second);
  }

  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) override {
    auto status_and_enc_path = EncodePath(dir);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::GetChildren(status_and_enc_path.second, result);
  }

  virtual Status GetChildrenFileAttributes(
      const std::string& dir, std::vector<FileAttributes>* result) override {
    auto status_and_enc_path = EncodePath(dir);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::GetChildrenFileAttributes(status_and_enc_path.second,
                                                 result);
  }

  virtual Status DeleteFile(const std::string& fname) override {
    auto status_and_enc_path = EncodePath(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::DeleteFile(status_and_enc_path.second);
  }

  virtual Status CreateDir(const std::string& dirname) override {
    auto status_and_enc_path = EncodePathWithNewBasename(dirname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::CreateDir(status_and_enc_path.second);
  }

  virtual Status CreateDirIfMissing(const std::string& dirname) override {
    auto status_and_enc_path = EncodePathWithNewBasename(dirname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::CreateDirIfMissing(status_and_enc_path.second);
  }

  virtual Status DeleteDir(const std::string& dirname) override {
    auto status_and_enc_path = EncodePath(dirname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::DeleteDir(status_and_enc_path.second);
  }

  virtual Status GetFileSize(const std::string& fname,
                             uint64_t* file_size) override {
    auto status_and_enc_path = EncodePath(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::GetFileSize(status_and_enc_path.second, file_size);
  }

  virtual Status GetFileModificationTime(const std::string& fname,
                                         uint64_t* file_mtime) override {
    auto status_and_enc_path = EncodePath(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::GetFileModificationTime(status_and_enc_path.second,
                                               file_mtime);
  }

  virtual Status RenameFile(const std::string& src,
                            const std::string& dest) override {
    auto status_and_src_enc_path = EncodePath(src);
    if (!status_and_src_enc_path.first.ok()) {
      return status_and_src_enc_path.first;
    }
    auto status_and_dest_enc_path = EncodePathWithNewBasename(dest);
    if (!status_and_dest_enc_path.first.ok()) {
      return status_and_dest_enc_path.first;
    }
    return EnvWrapper::RenameFile(status_and_src_enc_path.second,
                                  status_and_dest_enc_path.second);
  }

  virtual Status LinkFile(const std::string& src,
                          const std::string& dest) override {
    auto status_and_src_enc_path = EncodePath(src);
    if (!status_and_src_enc_path.first.ok()) {
      return status_and_src_enc_path.first;
    }
    auto status_and_dest_enc_path = EncodePathWithNewBasename(dest);
    if (!status_and_dest_enc_path.first.ok()) {
      return status_and_dest_enc_path.first;
    }
    return EnvWrapper::LinkFile(status_and_src_enc_path.second,
                                status_and_dest_enc_path.second);
  }

  virtual Status LockFile(const std::string& fname, FileLock** lock) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
//filelock子类可以存储路径（例如，posixfilelock存储路径）。我们
//无法跳过从此路径中剥离chroot目录，因为调用方
//不应该用它。
    return EnvWrapper::LockFile(status_and_enc_path.second, lock);
  }

  virtual Status GetTestDirectory(std::string* path) override {
//改编自posixenv的实现，因为它不提供
//在chroot中创建目录。
    char buf[256];
    snprintf(buf, sizeof(buf), "/rocksdbtest-%d", static_cast<int>(geteuid()));
    *path = buf;

//目录可能已经存在，因此忽略返回
    CreateDir(*path);
    return Status::OK();
  }

  virtual Status NewLogger(const std::string& fname,
                           shared_ptr<Logger>* result) override {
    auto status_and_enc_path = EncodePathWithNewBasename(fname);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::NewLogger(status_and_enc_path.second, result);
  }

  virtual Status GetAbsolutePath(const std::string& db_path,
                                 std::string* output_path) override {
    auto status_and_enc_path = EncodePath(db_path);
    if (!status_and_enc_path.first.ok()) {
      return status_and_enc_path.first;
    }
    return EnvWrapper::GetAbsolutePath(status_and_enc_path.second, output_path);
  }

 private:
//返回状态和扩展的绝对路径，包括chroot目录。
//检查提供的路径是否超出chroot。如果它回来
//非OK状态，不应使用返回的路径。
  std::pair<Status, std::string> EncodePath(const std::string& path) {
    if (path.empty() || path[0] != '/') {
      return {Status::InvalidArgument(path, "Not an absolute path"), ""};
    }
    std::pair<Status, std::string> res;
    res.second = chroot_dir_ + path;
#if defined(OS_AIX)
    char resolvedName[PATH_MAX];
    char* normalized_path = realpath(res.second.c_str(), resolvedName);
#else
    char* normalized_path = realpath(res.second.c_str(), nullptr);
#endif
    if (normalized_path == nullptr) {
      res.first = Status::NotFound(res.second, strerror(errno));
    } else if (strlen(normalized_path) < chroot_dir_.size() ||
               strncmp(normalized_path, chroot_dir_.c_str(),
                       chroot_dir_.size()) != 0) {
      res.first = Status::IOError(res.second,
                                  "Attempted to access path outside chroot");
    } else {
      res.first = Status::OK();
    }
#if !defined(OS_AIX)
    free(normalized_path);
#endif
    return res;
  }

//与encodePath（）类似，只是假定路径中的basename尚未
//创造了。
  std::pair<Status, std::string> EncodePathWithNewBasename(
      const std::string& path) {
    if (path.empty() || path[0] != '/') {
      return {Status::InvalidArgument(path, "Not an absolute path"), ""};
    }
//basename后面可以跟尾随斜杠
    size_t final_idx = path.find_last_not_of('/');
    if (final_idx == std::string::npos) {
//它只是斜杠，所以没有要提取的基名
      return EncodePath(path);
    }

//由于realname（3）（由
//encodePath（））需要存在的路径
    size_t base_sep = path.rfind('/', final_idx);
    auto status_and_enc_path = EncodePath(path.substr(0, base_sep + 1));
    status_and_enc_path.second.append(path.substr(base_sep + 1));
    return status_and_enc_path;
  }

  std::string chroot_dir_;
};

Env* NewChrootEnv(Env* base_env, const std::string& chroot_dir) {
  if (!base_env->FileExists(chroot_dir).ok()) {
    return nullptr;
  }
  return new ChrootEnv(base_env, chroot_dir);
}

}  //命名空间rocksdb

#endif  //！定义（RocksDB-Lite）&&！定义（OsWin）
