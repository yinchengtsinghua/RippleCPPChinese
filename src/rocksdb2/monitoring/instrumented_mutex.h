
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

#include "monitoring/statistics.h"
#include "port/port.h"
#include "rocksdb/env.h"
#include "rocksdb/statistics.h"
#include "rocksdb/thread_status.h"
#include "util/stop_watch.h"

namespace rocksdb {
class InstrumentedCondVar;

//为端口：：mutex提供附加层的包装类
//用于收集统计数据和仪器。
class InstrumentedMutex {
 public:
  explicit InstrumentedMutex(bool adaptive = false)
      : mutex_(adaptive), stats_(nullptr), env_(nullptr),
        stats_code_(0) {}

  InstrumentedMutex(
      Statistics* stats, Env* env,
      int stats_code, bool adaptive = false)
      : mutex_(adaptive), stats_(stats), env_(env),
        stats_code_(stats_code) {}

  void Lock();

  void Unlock() {
    mutex_.Unlock();
  }

  void AssertHeld() {
    mutex_.AssertHeld();
  }

 private:
  void LockInternal();
  friend class InstrumentedCondVar;
  port::Mutex mutex_;
  Statistics* stats_;
  Env* env_;
  int stats_code_;
};

//为端口：：mutex提供附加层的包装类
//用于收集统计数据和仪器。
class InstrumentedMutexLock {
 public:
  explicit InstrumentedMutexLock(InstrumentedMutex* mutex) : mutex_(mutex) {
    mutex_->Lock();
  }

  ~InstrumentedMutexLock() {
    mutex_->Unlock();
  }

 private:
  InstrumentedMutex* const mutex_;
  InstrumentedMutexLock(const InstrumentedMutexLock&) = delete;
  void operator=(const InstrumentedMutexLock&) = delete;
};

class InstrumentedCondVar {
 public:
  explicit InstrumentedCondVar(InstrumentedMutex* instrumented_mutex)
      : cond_(&(instrumented_mutex->mutex_)),
        stats_(instrumented_mutex->stats_),
        env_(instrumented_mutex->env_),
        stats_code_(instrumented_mutex->stats_code_) {}

  void Wait();

  bool TimedWait(uint64_t abs_time_us);

  void Signal() {
    cond_.Signal();
  }

  void SignalAll() {
    cond_.SignalAll();
  }

 private:
  void WaitInternal();
  bool TimedWaitInternal(uint64_t abs_time_us);
  port::CondVar cond_;
  Statistics* stats_;
  Env* env_;
  int stats_code_;
};

}  //命名空间rocksdb
