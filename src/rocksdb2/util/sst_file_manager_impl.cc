
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

#include "util/sst_file_manager_impl.h"

#include <vector>

#include "port/port.h"
#include "rocksdb/env.h"
#include "rocksdb/sst_file_manager.h"
#include "util/mutexlock.h"
#include "util/sync_point.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE
SstFileManagerImpl::SstFileManagerImpl(Env* env, std::shared_ptr<Logger> logger,
                                       const std::string& trash_dir,
                                       int64_t rate_bytes_per_sec)
    : env_(env),
      logger_(logger),
      total_files_size_(0),
      max_allowed_space_(0),
      delete_scheduler_(env, trash_dir, rate_bytes_per_sec, logger.get(),
                        this) {}

SstFileManagerImpl::~SstFileManagerImpl() {}

Status SstFileManagerImpl::OnAddFile(const std::string& file_path) {
  uint64_t file_size;
  Status s = env_->GetFileSize(file_path, &file_size);
  if (s.ok()) {
    MutexLock l(&mu_);
    OnAddFileImpl(file_path, file_size);
  }
  TEST_SYNC_POINT("SstFileManagerImpl::OnAddFile");
  return s;
}

Status SstFileManagerImpl::OnDeleteFile(const std::string& file_path) {
  {
    MutexLock l(&mu_);
    OnDeleteFileImpl(file_path);
  }
  TEST_SYNC_POINT("SstFileManagerImpl::OnDeleteFile");
  return Status::OK();
}

Status SstFileManagerImpl::OnMoveFile(const std::string& old_path,
                                      const std::string& new_path,
                                      uint64_t* file_size) {
  {
    MutexLock l(&mu_);
    if (file_size != nullptr) {
      *file_size = tracked_files_[old_path];
    }
    OnAddFileImpl(new_path, tracked_files_[old_path]);
    OnDeleteFileImpl(old_path);
  }
  TEST_SYNC_POINT("SstFileManagerImpl::OnMoveFile");
  return Status::OK();
}

void SstFileManagerImpl::SetMaxAllowedSpaceUsage(uint64_t max_allowed_space) {
  MutexLock l(&mu_);
  max_allowed_space_ = max_allowed_space;
}

bool SstFileManagerImpl::IsMaxAllowedSpaceReached() {
  MutexLock l(&mu_);
  if (max_allowed_space_ <= 0) {
    return false;
  }
  return total_files_size_ >= max_allowed_space_;
}

uint64_t SstFileManagerImpl::GetTotalSize() {
  MutexLock l(&mu_);
  return total_files_size_;
}

std::unordered_map<std::string, uint64_t>
SstFileManagerImpl::GetTrackedFiles() {
  MutexLock l(&mu_);
  return tracked_files_;
}

int64_t SstFileManagerImpl::GetDeleteRateBytesPerSecond() {
  return delete_scheduler_.GetRateBytesPerSecond();
}

void SstFileManagerImpl::SetDeleteRateBytesPerSecond(int64_t delete_rate) {
  return delete_scheduler_.SetRateBytesPerSecond(delete_rate);
}

Status SstFileManagerImpl::ScheduleFileDeletion(const std::string& file_path) {
  return delete_scheduler_.DeleteFile(file_path);
}

void SstFileManagerImpl::WaitForEmptyTrash() {
  delete_scheduler_.WaitForEmptyTrash();
}

void SstFileManagerImpl::OnAddFileImpl(const std::string& file_path,
                                       uint64_t file_size) {
  auto tracked_file = tracked_files_.find(file_path);
  if (tracked_file != tracked_files_.end()) {
//以前添加过文件，我们只更新大小
    total_files_size_ -= tracked_file->second;
    total_files_size_ += file_size;
  } else {
    total_files_size_ += file_size;
  }
  tracked_files_[file_path] = file_size;
}

void SstFileManagerImpl::OnDeleteFileImpl(const std::string& file_path) {
  auto tracked_file = tracked_files_.find(file_path);
  if (tracked_file == tracked_files_.end()) {
//未跟踪文件
    return;
  }

  total_files_size_ -= tracked_file->second;
  tracked_files_.erase(tracked_file);
}

SstFileManager* NewSstFileManager(Env* env, std::shared_ptr<Logger> info_log,
                                  std::string trash_dir,
                                  int64_t rate_bytes_per_sec,
                                  bool delete_existing_trash, Status* status) {
  SstFileManagerImpl* res =
      new SstFileManagerImpl(env, info_log, trash_dir, rate_bytes_per_sec);

  Status s;
  if (trash_dir != "") {
    s = env->CreateDirIfMissing(trash_dir);
    if (s.ok() && delete_existing_trash) {
      std::vector<std::string> files_in_trash;
      s = env->GetChildren(trash_dir, &files_in_trash);
      if (s.ok()) {
        for (const std::string& trash_file : files_in_trash) {
          if (trash_file == "." || trash_file == "..") {
            continue;
          }

          std::string path_in_trash = trash_dir + "/" + trash_file;
          res->OnAddFile(path_in_trash);
          Status file_delete = res->ScheduleFileDeletion(path_in_trash);
          if (s.ok() && !file_delete.ok()) {
            s = file_delete;
          }
        }
      }
    }
  }

  if (status) {
    *status = s;
  }

  return res;
}

#else

SstFileManager* NewSstFileManager(Env* env, std::shared_ptr<Logger> info_log,
                                  std::string trash_dir,
                                  int64_t rate_bytes_per_sec,
                                  bool delete_existing_trash, Status* status) {
  if (status) {
    *status =
        Status::NotSupported("SstFileManager is not supported in ROCKSDB_LITE");
  }
  return nullptr;
}

#endif  //摇滚乐

}  //命名空间rocksdb

