
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

#pragma once

#ifndef ROCKSDB_LITE

#include <string>

#include "port/port.h"

#include "rocksdb/sst_file_manager.h"
#include "util/delete_scheduler.h"

namespace rocksdb {

class Env;
class Logger;

//sstfilemanager用于跟踪数据库中的sst文件并在其中进行控制。
//缺失率。
//所有sstfileManager公共函数都是线程安全的。
class SstFileManagerImpl : public SstFileManager {
 public:
  explicit SstFileManagerImpl(Env* env, std::shared_ptr<Logger> logger,
                              const std::string& trash_dir,
                              int64_t rate_bytes_per_sec);

  ~SstFileManagerImpl();

//每当添加新的sst文件时，db将调用onaddfile。
  Status OnAddFile(const std::string& file_path);

//每当删除sst文件时，db将调用OnDeleteFile。
  Status OnDeleteFile(const std::string& file_path);

//每当sst文件移动到新路径时，db将调用onmovefile。
  Status OnMoveFile(const std::string& old_path, const std::string& new_path,
                    uint64_t* file_size = nullptr);

//更新rocksdb应使用的最大允许空间，如果
//sst文件的总大小超过了允许的最大空间，写入到
//RockSDB将失败。
//
//将“允许的最大空间”设置为0将禁用此功能，允许的最大空间
//空间将是无限的（默认值）。
//
//线程安全。
  void SetMaxAllowedSpaceUsage(uint64_t max_allowed_space) override;

//如果sst文件的总大小超过允许的最大值，则返回true
//空间使用。
//
//线程安全。
  bool IsMaxAllowedSpaceReached() override;

//返回所有跟踪文件的总大小。
  uint64_t GetTotalSize() override;

//返回包含所有跟踪文件和相应大小的映射。
  std::unordered_map<std::string, uint64_t> GetTrackedFiles() override;

//返回删除速率限制（字节/秒）。
  virtual int64_t GetDeleteRateBytesPerSecond() override;

//更新删除速率限制（字节/秒）。
  virtual void SetDeleteRateBytesPerSecond(int64_t delete_rate) override;

//将文件移到垃圾目录并计划删除。
  virtual Status ScheduleFileDeletion(const std::string& file_path);

//等待后台删除的所有文件完成或
//要调用的析构函数。
  virtual void WaitForEmptyTrash();

  DeleteScheduler* delete_scheduler() { return &delete_scheduler_; }

 private:
//要求：互斥锁
  void OnAddFileImpl(const std::string& file_path, uint64_t file_size);
//要求：互斥锁
  void OnDeleteFileImpl(const std::string& file_path);

  Env* env_;
  std::shared_ptr<Logger> logger_;
//保护被跟踪的文件的互斥体，总文件大小
  port::Mutex mu_;
//跟踪文件映射中所有文件大小的总和
  uint64_t total_files_size_;
//包含所有跟踪文件和大小的地图
//文件路径=>文件大小
  std::unordered_map<std::string, uint64_t> tracked_files_;
//SST文件允许的最大空间（字节）。
  uint64_t max_allowed_space_;
//如果sstfilemanagerimpl是
//创建速率为“字节/秒”=0或“垃圾桶目录”=0，删除“调度程序”
//速率限制将被禁用，只会删除文件。
  DeleteScheduler delete_scheduler_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
