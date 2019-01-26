
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#include <algorithm>
#include <atomic>
#include <deque>
#include "port/port.h"
#include "util/mutexlock.h"
#include "util/random.h"
#include "rocksdb/env.h"
#include "rocksdb/rate_limiter.h"

namespace rocksdb {

class GenericRateLimiter : public RateLimiter {
 public:
  GenericRateLimiter(int64_t refill_bytes, int64_t refill_period_us,
                     int32_t fairness,
                     RateLimiter::Mode mode = RateLimiter::Mode::kWritesOnly);

  virtual ~GenericRateLimiter();

//此API允许用户每秒动态更改速率限制器的字节数。
  virtual void SetBytesPerSecond(int64_t bytes_per_second) override;

//请求令牌写入字节。如果不能满足这个要求，
//呼叫被阻止。呼叫方负责确保
//字节<=GetSingleBurstBytes（）
  using RateLimiter::Request;
  virtual void Request(const int64_t bytes, const Env::IOPriority pri,
                       Statistics* stats) override;

  virtual int64_t GetSingleBurstBytes() const override {
    return refill_bytes_per_period_.load(std::memory_order_relaxed);
  }

  virtual int64_t GetTotalBytesThrough(
      const Env::IOPriority pri = Env::IO_TOTAL) const override {
    MutexLock g(&request_mutex_);
    if (pri == Env::IO_TOTAL) {
      return total_bytes_through_[Env::IO_LOW] +
             total_bytes_through_[Env::IO_HIGH];
    }
    return total_bytes_through_[pri];
  }

  virtual int64_t GetTotalRequests(
      const Env::IOPriority pri = Env::IO_TOTAL) const override {
    MutexLock g(&request_mutex_);
    if (pri == Env::IO_TOTAL) {
      return total_requests_[Env::IO_LOW] + total_requests_[Env::IO_HIGH];
    }
    return total_requests_[pri];
  }

  virtual int64_t GetBytesPerSecond() const override {
    return rate_bytes_per_sec_;
  }

 private:
  void Refill();
  int64_t CalculateRefillBytesPerPeriod(int64_t rate_bytes_per_sec);
  uint64_t NowMicrosMonotonic(Env* env) {
    return env->NowNanos() / std::milli::den;
  }

//这个互斥保护所有内部状态
  mutable port::Mutex request_mutex_;

  const int64_t kMinRefillBytesPerPeriod = 100;

  const int64_t refill_period_us_;

  int64_t rate_bytes_per_sec_;
//此变量可以动态更改。
  std::atomic<int64_t> refill_bytes_per_period_;
  Env* const env_;

  bool stop_;
  port::CondVar exit_cv_;
  int32_t requests_to_wait_;

  int64_t total_requests_[Env::IO_TOTAL];
  int64_t total_bytes_through_[Env::IO_TOTAL];
  int64_t available_bytes_;
  int64_t next_refill_us_;

  int32_t fairness_;
  Random rnd_;

  struct Req;
  Req* leader_;
  std::deque<Req*> queue_[Env::IO_TOTAL];
};

}  //命名空间rocksdb
