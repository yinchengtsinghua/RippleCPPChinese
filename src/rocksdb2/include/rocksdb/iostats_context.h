
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

#include <stdint.h>
#include <string>

#include "rocksdb/perf_level.h"

//用于高效透明地收集IO状态的线程本地上下文。
//使用setperflevel（perflevel:：kenabletime）启用时间统计。

namespace rocksdb {

struct IOStatsContext {
//将所有IO统计计数器重置为零
  void Reset();

  std::string ToString(bool exclude_zero_counters = false) const;

//线程池ID
  uint64_t thread_pool_id;

//已写入的字节数。
  uint64_t bytes_written;
//已读取的字节数。
  uint64_t bytes_read;

//在open（）和fopen（）中花费的时间。
  uint64_t open_nanos;
//在fallocate（）中花费的时间。
  uint64_t allocate_nanos;
//在write（）和pwrite（）中花费的时间。
  uint64_t write_nanos;
//在read（）和pread（）中花费的时间
  uint64_t read_nanos;
//在同步文件范围（）中花费的时间。
  uint64_t range_sync_nanos;
//在fsync中花费的时间
  uint64_t fsync_nanos;
//准备写作（休息等）所花费的时间。
  uint64_t prepare_write_nanos;
//在logger:：logv（）中花费的时间。
  uint64_t logger_nanos;
};

//获取线程本地iostatsContext对象指针
IOStatsContext* get_iostats_context();

}  //命名空间rocksdb
