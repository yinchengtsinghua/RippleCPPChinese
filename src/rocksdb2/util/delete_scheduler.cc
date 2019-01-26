
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

#ifndef ROCKSDB_LITE

#include "util/delete_scheduler.h"

#include <thread>
#include <vector>

#include "port/port.h"
#include "rocksdb/env.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/sst_file_manager_impl.h"
#include "util/sync_point.h"

namespace rocksdb {

DeleteScheduler::DeleteScheduler(Env* env, const std::string& trash_dir,
                                 int64_t rate_bytes_per_sec, Logger* info_log,
                                 SstFileManagerImpl* sst_file_manager)
    : env_(env),
      trash_dir_(trash_dir),
      total_trash_size_(0),
      rate_bytes_per_sec_(rate_bytes_per_sec),
      pending_files_(0),
      closing_(false),
      cv_(&mu_),
      info_log_(info_log),
      sst_file_manager_(sst_file_manager) {
  assert(sst_file_manager != nullptr);
  bg_thread_.reset(
      new port::Thread(&DeleteScheduler::BackgroundEmptyTrash, this));
}

DeleteScheduler::~DeleteScheduler() {
  {
    InstrumentedMutexLock l(&mu_);
    closing_ = true;
    cv_.SignalAll();
  }
  if (bg_thread_) {
    bg_thread_->join();
  }
}

Status DeleteScheduler::DeleteFile(const std::string& file_path) {
  Status s;
  if (rate_bytes_per_sec_.load() <= 0 ||
      total_trash_size_.load() >
          sst_file_manager_->GetTotalSize() * max_trash_db_ratio_) {
//禁用速率限制或垃圾大小超过
//总数据库大小的最大垃圾箱比率（默认为25%）。
    TEST_SYNC_POINT("DeleteScheduler::DeleteFile");
    s = env_->DeleteFile(file_path);
    if (s.ok()) {
      sst_file_manager_->OnDeleteFile(file_path);
    }
    return s;
  }

//将文件移到垃圾桶
  std::string path_in_trash;
  s = MoveToTrash(file_path, &path_in_trash);
  if (!s.ok()) {
    ROCKS_LOG_ERROR(info_log_, "Failed to move %s to trash directory (%s)",
                    file_path.c_str(), trash_dir_.c_str());
    s = env_->DeleteFile(file_path);
    if (s.ok()) {
      sst_file_manager_->OnDeleteFile(file_path);
    }
    return s;
  }

//将文件添加到删除队列
  {
    InstrumentedMutexLock l(&mu_);
    queue_.push(path_in_trash);
    pending_files_++;
    if (pending_files_ == 1) {
      cv_.SignalAll();
    }
  }
  return s;
}

std::map<std::string, Status> DeleteScheduler::GetBackgroundErrors() {
  InstrumentedMutexLock l(&mu_);
  return bg_errors_;
}

Status DeleteScheduler::MoveToTrash(const std::string& file_path,
                                    std::string* path_in_trash) {
  Status s;
//找出垃圾文件夹中文件的名称
  size_t idx = file_path.rfind("/");
  if (idx == std::string::npos || idx == file_path.size() - 1) {
    return Status::InvalidArgument("file_path is corrupted");
  }
  *path_in_trash = trash_dir_ + file_path.substr(idx);
  std::string unique_suffix = "";

  if (*path_in_trash == file_path) {
//此文件已在垃圾桶中
    return s;
  }

//TODO（tec）：实现env:：renamefileifnotexist并移除
//文件移动mu mutex。
  InstrumentedMutexLock l(&file_move_mu_);
  while (true) {
    s = env_->FileExists(*path_in_trash + unique_suffix);
    if (s.IsNotFound()) {
//我们在垃圾桶里找到了文件的路径
      *path_in_trash += unique_suffix;
      s = env_->RenameFile(file_path, *path_in_trash);
      break;
    } else if (s.ok()) {
//名称冲突，生成新的随机后缀
      unique_suffix = env_->GenerateUniqueId();
    } else {
//fileexists调用过程中出错，无法继续
      break;
    }
  }
  if (s.ok()) {
    uint64_t trash_file_size = 0;
    sst_file_manager_->OnMoveFile(file_path, *path_in_trash, &trash_file_size);
    total_trash_size_.fetch_add(trash_file_size);
  }
  return s;
}

void DeleteScheduler::BackgroundEmptyTrash() {
  TEST_SYNC_POINT("DeleteScheduler::BackgroundEmptyTrash");

  while (true) {
    InstrumentedMutexLock l(&mu_);
    while (queue_.empty() && !closing_) {
      cv_.Wait();
    }

    if (closing_) {
      return;
    }

//删除队列中的所有文件
    uint64_t start_time = env_->NowMicros();
    uint64_t total_deleted_bytes = 0;
    int64_t current_delete_rate = rate_bytes_per_sec_.load();
    while (!queue_.empty() && !closing_) {
      if (current_delete_rate != rate_bytes_per_sec_.load()) {
//用户更改了删除率
        current_delete_rate = rate_bytes_per_sec_.load();
        start_time = env_->NowMicros();
        total_deleted_bytes = 0;
      }

//获取要删除的新文件
      std::string path_in_trash = queue_.front();
      queue_.pop();

//删除文件时不需要保持锁定
      mu_.Unlock();
      uint64_t deleted_bytes = 0;
//从垃圾桶中删除文件并更新总价值
      Status s = DeleteTrashFile(path_in_trash,  &deleted_bytes);
      total_deleted_bytes += deleted_bytes;
      mu_.Lock();

      if (!s.ok()) {
        bg_errors_[path_in_trash] = s;
      }

//必要时使用Penlty
      uint64_t total_penlty;
      if (current_delete_rate > 0) {
//速率限制已启用
        total_penlty =
            ((total_deleted_bytes * kMicrosInSecond) / current_delete_rate);
        while (!closing_ && !cv_.TimedWait(start_time + total_penlty)) {}
      } else {
//禁用速率限制
        total_penlty = 0;
      }
      TEST_SYNC_POINT_CALLBACK("DeleteScheduler::BackgroundEmptyTrash:Wait",
                               &total_penlty);

      pending_files_--;
      if (pending_files_ == 0) {
//取消阻止WaitForempyTrash，因为没有其他文件在等待
//被删除
        cv_.SignalAll();
      }
    }
  }
}

Status DeleteScheduler::DeleteTrashFile(const std::string& path_in_trash,
                                        uint64_t* deleted_bytes) {
  uint64_t file_size;
  Status s = env_->GetFileSize(path_in_trash, &file_size);
  if (s.ok()) {
    TEST_SYNC_POINT("DeleteScheduler::DeleteTrashFile:DeleteFile");
    s = env_->DeleteFile(path_in_trash);
  }

  if (!s.ok()) {
//获取文件大小或删除时出错
    ROCKS_LOG_ERROR(info_log_, "Failed to delete %s from trash -- %s",
                    path_in_trash.c_str(), s.ToString().c_str());
    *deleted_bytes = 0;
  } else {
    *deleted_bytes = file_size;
    total_trash_size_.fetch_sub(file_size);
    sst_file_manager_->OnDeleteFile(path_in_trash);
  }

  return s;
}

void DeleteScheduler::WaitForEmptyTrash() {
  InstrumentedMutexLock l(&mu_);
  while (pending_files_ > 0 && !closing_) {
    cv_.Wait();
  }
}

}  //命名空间rocksdb

#endif  //摇滚乐
