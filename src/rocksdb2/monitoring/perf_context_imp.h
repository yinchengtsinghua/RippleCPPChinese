
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
#include "rocksdb/perf_context.h"
#include "util/stop_watch.h"

namespace rocksdb {

#if defined(NPERF_CONTEXT)

#define PERF_TIMER_GUARD(metric)
#define PERF_CONDITIONAL_TIMER_FOR_MUTEX_GUARD(metric, condition)
#define PERF_TIMER_MEASURE(metric)
#define PERF_TIMER_STOP(metric)
#define PERF_TIMER_START(metric)
#define PERF_COUNTER_ADD(metric, value)

#else

//停止计时器并更新度量
#define PERF_TIMER_STOP(metric) perf_step_timer_##metric.Stop();

#define PERF_TIMER_START(metric) perf_step_timer_##metric.Start();

//声明并设置计时器的开始时间
#define PERF_TIMER_GUARD(metric)                                       \
  PerfStepTimer perf_step_timer_##metric(&(get_perf_context()->metric)); \
  perf_step_timer_##metric.Start();

#define PERF_CONDITIONAL_TIMER_FOR_MUTEX_GUARD(metric, condition)            \
  PerfStepTimer perf_step_timer_##metric(&(get_perf_context()->metric), true); \
  if ((condition)) {                                                         \
    perf_step_timer_##metric.Start();                                        \
  }

//使用自上次启动以来经过的时间更新度量。开始时间重置
//到当前时间戳。
#define PERF_TIMER_MEASURE(metric) perf_step_timer_##metric.Measure();

//增加度量值
#define PERF_COUNTER_ADD(metric, value)        \
  if (perf_level >= PerfLevel::kEnableCount) { \
    get_perf_context()->metric += value;       \
  }

#endif

}
