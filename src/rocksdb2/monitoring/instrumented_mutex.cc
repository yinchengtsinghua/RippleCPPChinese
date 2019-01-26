
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

#include "monitoring/instrumented_mutex.h"
#include "monitoring/perf_context_imp.h"
#include "monitoring/thread_status_util.h"
#include "util/sync_point.h"

namespace rocksdb {
namespace {
bool ShouldReportToStats(Env* env, Statistics* stats) {
  return env != nullptr && stats != nullptr &&
          stats->stats_level_ > kExceptTimeForMutex;
}
}  //命名空间

void InstrumentedMutex::Lock() {
  PERF_CONDITIONAL_TIMER_FOR_MUTEX_GUARD(db_mutex_lock_nanos,
                                         stats_code_ == DB_MUTEX_WAIT_MICROS);
  uint64_t wait_time_micros = 0;
  if (ShouldReportToStats(env_, stats_)) {
    {
      StopWatch sw(env_, nullptr, 0, &wait_time_micros);
      LockInternal();
    }
    RecordTick(stats_, stats_code_, wait_time_micros);
  } else {
    LockInternal();
  }
}

void InstrumentedMutex::LockInternal() {
#ifndef NDEBUG
  ThreadStatusUtil::TEST_StateDelay(ThreadStatus::STATE_MUTEX_WAIT);
#endif
  mutex_.Lock();
}

void InstrumentedCondVar::Wait() {
  PERF_CONDITIONAL_TIMER_FOR_MUTEX_GUARD(db_condition_wait_nanos,
                                         stats_code_ == DB_MUTEX_WAIT_MICROS);
  uint64_t wait_time_micros = 0;
  if (ShouldReportToStats(env_, stats_)) {
    {
      StopWatch sw(env_, nullptr, 0, &wait_time_micros);
      WaitInternal();
    }
    RecordTick(stats_, stats_code_, wait_time_micros);
  } else {
    WaitInternal();
  }
}

void InstrumentedCondVar::WaitInternal() {
#ifndef NDEBUG
  ThreadStatusUtil::TEST_StateDelay(ThreadStatus::STATE_MUTEX_WAIT);
#endif
  cond_.Wait();
}

bool InstrumentedCondVar::TimedWait(uint64_t abs_time_us) {
  PERF_CONDITIONAL_TIMER_FOR_MUTEX_GUARD(db_condition_wait_nanos,
                                         stats_code_ == DB_MUTEX_WAIT_MICROS);
  uint64_t wait_time_micros = 0;
  bool result = false;
  if (ShouldReportToStats(env_, stats_)) {
    {
      StopWatch sw(env_, nullptr, 0, &wait_time_micros);
      result = TimedWaitInternal(abs_time_us);
    }
    RecordTick(stats_, stats_code_, wait_time_micros);
  } else {
    result = TimedWaitInternal(abs_time_us);
  }
  return result;
}

bool InstrumentedCondVar::TimedWaitInternal(uint64_t abs_time_us) {
#ifndef NDEBUG
  ThreadStatusUtil::TEST_StateDelay(ThreadStatus::STATE_MUTEX_WAIT);
#endif

  TEST_SYNC_POINT_CALLBACK("InstrumentedCondVar::TimedWaitInternal",
                           &abs_time_us);

  return cond_.TimedWait(abs_time_us);
}

}  //命名空间rocksdb
