
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once
#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/checkpoint.h"

#include <string>
#include "rocksdb/db.h"
#include "util/filename.h"

namespace rocksdb {

class CheckpointImpl : public Checkpoint {
 public:
//创建用于创建可打开快照的检查点对象
  explicit CheckpointImpl(DB* db) : db_(db) {}

//在同一个磁盘上构建rocksdb的可打开快照，其中
//接受同一磁盘上和目录下的输出目录
//（1）指向现有活动SST文件的硬链接SST文件
//如果输出目录在不同的文件系统上，则会复制sst文件。
//（2）复制的清单文件和其他文件
//目录不应该已经存在，将由该API创建。
//目录将是绝对路径
  using Checkpoint::CreateCheckpoint;
  virtual Status CreateCheckpoint(const std::string& checkpoint_dir,
                                  uint64_t log_size_for_flush) override;

//检查点逻辑可以通过为链接、复制和
//或创造。
  Status CreateCustomCheckpoint(
      const DBOptions& db_options,
      std::function<Status(const std::string& src_dirname,
                           const std::string& fname, FileType type)>
          link_file_cb,
      std::function<Status(const std::string& src_dirname,
                           const std::string& fname, uint64_t size_limit_bytes,
                           FileType type)>
          copy_file_cb,
      std::function<Status(const std::string& fname,
                           const std::string& contents, FileType type)>
          create_file_cb,
      uint64_t* sequence_number, uint64_t log_size_for_flush);

 private:
  DB* db_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
