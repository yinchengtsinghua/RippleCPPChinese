
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

#include <map>
#include <queue>
#include <string>
#include <thread>

#include "monitoring/instrumented_mutex.h"
#include "port/port.h"

#include "rocksdb/status.h"

namespace rocksdb {

class Env;
class Logger;
class SstFileManagerImpl;

//DeleteScheduler允许数据库强制执行文件删除的速率限制，
//不是立即删除文件，而是将文件移动到垃圾桶目录
//并在后台线程中删除，该线程在删除之间应用sleep penlty
//如果它们发生的速率比每秒速率字节数快，
//
//速率限制可以通过设置速率字节/秒=0来关闭，在这里
//case deletescheduler将立即删除文件。
class DeleteScheduler {
 public:
  DeleteScheduler(Env* env, const std::string& trash_dir,
                  int64_t rate_bytes_per_sec, Logger* info_log,
                  SstFileManagerImpl* sst_file_manager);

  ~DeleteScheduler();

//返回删除速率限制（字节/秒）
  int64_t GetRateBytesPerSecond() { return rate_bytes_per_sec_.load(); }

//设置删除速率限制（字节/秒）
  void SetRateBytesPerSecond(int64_t bytes_per_sec) {
    return rate_bytes_per_sec_.store(bytes_per_sec);
  }

//将文件移到垃圾目录并计划删除
  Status DeleteFile(const std::string& fname);

//等待后台删除的所有文件完成或
//要调用的析构函数。
  void WaitForEmptyTrash();

//返回包含在BackgroundEmptyTrash中发生的错误的映射
//文件路径=>错误状态
  std::map<std::string, Status> GetBackgroundErrors();

  uint64_t GetTotalTrashSize() { return total_trash_size_.load(); }

  void TEST_SetMaxTrashDBRatio(double r) {
    assert(r >= 0);
    max_trash_db_ratio_ = r;
  }

 private:
  Status MoveToTrash(const std::string& file_path, std::string* path_in_trash);

  Status DeleteTrashFile(const std::string& path_in_trash,
                         uint64_t* deleted_bytes);

  void BackgroundEmptyTrash();

  Env* env_;
//垃圾目录路径
  std::string trash_dir_;
//垃圾目录的总大小
  std::atomic<uint64_t> total_trash_size_;
//每秒应删除的最大字节数
  std::atomic<int64_t> rate_bytes_per_sec_;
//用于保护队列、挂起的文件、bg错误、关闭的互斥体
  InstrumentedMutex mu_;
//垃圾桶中需要删除的文件队列
  std::queue<std::string> queue_;
//垃圾桶中等待删除的文件数
  int32_t pending_files_;
//backgroundEmptyTrash中发生的错误（文件路径=>错误）
  std::map<std::string, Status> bg_errors_;
//在~deleteScheduler（）中设置为true，以强制backgroundEmptyTrash停止。
  bool closing_;
//在这些条件下发出信号的条件变量
//-挂起的“文件”值从0变为大于1
//-挂起的文件值从1变为大于0
//-关闭值设置为真
  InstrumentedCondVar cv_;
//后台线程运行backgroundEmptyTrash
  std::unique_ptr<port::Thread> bg_thread_;
//防止线程发生文件名冲突的互斥体
  InstrumentedMutex file_move_mu_;
  Logger* info_log_;
  SstFileManagerImpl* sst_file_manager_;
//如果垃圾大小占总数据库大小的25%以上
//我们将立即开始删除传递给DeleteScheduler的新文件
  double max_trash_db_ratio_ = 0.25;
  static const uint64_t kMicrosInSecond = 1000 * 1000LL;
};

}  //命名空间rocksdb

#endif  //摇滚乐
