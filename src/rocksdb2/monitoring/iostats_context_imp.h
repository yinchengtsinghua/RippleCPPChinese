
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
#pragma once
#include "monitoring/perf_step_timer.h"
#include "rocksdb/iostats_context.h"

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL

//按指定值递增特定计数器
#define IOSTATS_ADD(metric, value)     \
  (get_iostats_context()->metric += value)

//仅当公制值为正数时才增加公制值
#define IOSTATS_ADD_IF_POSITIVE(metric, value)   \
  if (value > 0) { IOSTATS_ADD(metric, value); }

//将特定计数器重置为零
#define IOSTATS_RESET(metric)          \
  (get_iostats_context()->metric = 0)

//将所有计数器重置为零
#define IOSTATS_RESET_ALL()                        \
  (get_iostats_context()->Reset())

#define IOSTATS_SET_THREAD_POOL_ID(value)      \
  (get_iostats_context()->thread_pool_id = value)

#define IOSTATS_THREAD_POOL_ID()               \
  (get_iostats_context()->thread_pool_id)

#define IOSTATS(metric)                        \
  (get_iostats_context()->metric)

//声明并设置计时器的开始时间
#define IOSTATS_TIMER_GUARD(metric)                                          \
  PerfStepTimer iostats_step_timer_##metric(&(get_iostats_context()->metric)); \
  iostats_step_timer_##metric.Start();

#else  //RocksDB_支持_线程_本地

#define IOSTATS_ADD(metric, value)
#define IOSTATS_ADD_IF_POSITIVE(metric, value)
#define IOSTATS_RESET(metric)
#define IOSTATS_RESET_ALL()
#define IOSTATS_SET_THREAD_POOL_ID(value)
#define IOSTATS_THREAD_POOL_ID()
#define IOSTATS(metric) 0

#define IOSTATS_TIMER_GUARD(metric)

#endif  //RocksDB_支持_线程_本地
