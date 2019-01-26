
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
//检查点是数据库在某个时间点的可打开快照。

#pragma once
#ifndef ROCKSDB_LITE

#include <string>
#include "rocksdb/status.h"

namespace rocksdb {

class DB;

class Checkpoint {
 public:
//创建用于创建可打开快照的检查点对象
  static Status Create(DB* db, Checkpoint** checkpoint_ptr);

//在同一个磁盘上构建rocksdb的可打开快照，其中
//接受同一磁盘上和目录下的输出目录
//（1）指向现有活动SST文件的硬链接SST文件
//如果输出目录在不同的文件系统上，则会复制sst文件。
//（2）复制的清单文件和其他文件
//目录不应该已经存在，将由该API创建。
//目录将是绝对路径
//日志文件大小：如果总日志文件大小等于或大于
//该值，然后对所有列族触发刷新。这个
//默认值为0，这意味着总是触发刷新。如果你移动
//远离默认值，检查点可能不包含最新数据
//如果不总是启用wal写入。
//如果是2pc，总是会触发flush。
  virtual Status CreateCheckpoint(const std::string& checkpoint_dir,
                                  uint64_t log_size_for_flush = 0);

  virtual ~Checkpoint() {}
};

}  //命名空间rocksdb
#endif  //！摇滚乐
