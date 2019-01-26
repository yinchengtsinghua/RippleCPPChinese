
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

#include "util/rate_limiter.h"
#include "monitoring/statistics.h"
#include "port/port.h"
#include "rocksdb/env.h"
#include "util/aligned_buffer.h"
#include "util/sync_point.h"

namespace rocksdb {

size_t RateLimiter::RequestToken(size_t bytes, size_t alignment,
                                 Env::IOPriority io_priority, Statistics* stats,
                                 RateLimiter::OpType op_type) {
  if (io_priority < Env::IO_TOTAL && IsRateLimited(op_type)) {
    bytes = std::min(bytes, static_cast<size_t>(GetSingleBurstBytes()));

    if (alignment > 0) {
//在这里，我们可能需要的不仅仅是突发和阻塞
//但是我们不能一次在直接I/O上写少于一页
//因此，我们可能不想使用ratelimiter
      bytes = std::max(alignment, TruncateToPageBoundary(alignment, bytes));
    }
    Request(bytes, io_priority, stats, op_type);
  }
  return bytes;
}

//未决请求
struct GenericRateLimiter::Req {
  explicit Req(int64_t _bytes, port::Mutex* _mu)
      : request_bytes(_bytes), bytes(_bytes), cv(_mu), granted(false) {}
  int64_t request_bytes;
  int64_t bytes;
  port::CondVar cv;
  bool granted;
};

GenericRateLimiter::GenericRateLimiter(int64_t rate_bytes_per_sec,
                                       int64_t refill_period_us,
                                       int32_t fairness, RateLimiter::Mode mode)
    : RateLimiter(mode),
      refill_period_us_(refill_period_us),
      rate_bytes_per_sec_(rate_bytes_per_sec),
      refill_bytes_per_period_(
          CalculateRefillBytesPerPeriod(rate_bytes_per_sec)),
      env_(Env::Default()),
      stop_(false),
      exit_cv_(&request_mutex_),
      requests_to_wait_(0),
      available_bytes_(0),
      next_refill_us_(NowMicrosMonotonic(env_)),
      fairness_(fairness > 100 ? 100 : fairness),
      rnd_((uint32_t)time(nullptr)),
      leader_(nullptr) {
  total_requests_[0] = 0;
  total_requests_[1] = 0;
  total_bytes_through_[0] = 0;
  total_bytes_through_[1] = 0;
}

GenericRateLimiter::~GenericRateLimiter() {
  MutexLock g(&request_mutex_);
  stop_ = true;
  requests_to_wait_ = static_cast<int32_t>(queue_[Env::IO_LOW].size() +
                                           queue_[Env::IO_HIGH].size());
  for (auto& r : queue_[Env::IO_HIGH]) {
    r->cv.Signal();
  }
  for (auto& r : queue_[Env::IO_LOW]) {
    r->cv.Signal();
  }
  while (requests_to_wait_ > 0) {
    exit_cv_.Wait();
  }
}

//此API允许用户每秒动态更改速率限制器的字节数。
void GenericRateLimiter::SetBytesPerSecond(int64_t bytes_per_second) {
  assert(bytes_per_second > 0);
  rate_bytes_per_sec_ = bytes_per_second;
  refill_bytes_per_period_.store(
      CalculateRefillBytesPerPeriod(bytes_per_second),
      std::memory_order_relaxed);
}

void GenericRateLimiter::Request(int64_t bytes, const Env::IOPriority pri,
                                 Statistics* stats) {
  assert(bytes <= refill_bytes_per_period_.load(std::memory_order_relaxed));
  TEST_SYNC_POINT("GenericRateLimiter::Request");
  TEST_SYNC_POINT_CALLBACK("GenericRateLimiter::Request:1",
                           &rate_bytes_per_sec_);
  MutexLock g(&request_mutex_);
  if (stop_) {
    return;
  }

  ++total_requests_[pri];

  if (available_bytes_ >= bytes) {
//重新填充线程分配配额并通知等待的请求
//mutex下的队列。所以如果我们到了这里，就意味着没有人
//在等吗？
    available_bytes_ -= bytes;
    total_bytes_through_[pri] += bytes;
    return;
  }

//此时无法满足请求，排队
  Req r(bytes, &request_mutex_);
  queue_[pri].push_back(&r);

  do {
    bool timedout = false;
//领袖选举，候选人可以是：
//（1）新的传入请求，
//（二）上届领导，尚未确定名额的；
//降低优先级
//（3）队列前面的前一个服务员，接到通知
//前领导人
    if (leader_ == nullptr &&
        ((!queue_[Env::IO_HIGH].empty() &&
            &r == queue_[Env::IO_HIGH].front()) ||
         (!queue_[Env::IO_LOW].empty() &&
            &r == queue_[Env::IO_LOW].front()))) {
      leader_ = &r;
      int64_t delta = next_refill_us_ - NowMicrosMonotonic(env_);
      delta = delta > 0 ? delta : 0;
      if (delta == 0) {
        timedout = true;
      } else {
        int64_t wait_until = env_->NowMicros() + delta;
        RecordTick(stats, NUMBER_RATE_LIMITER_DRAINS);
        timedout = r.cv.TimedWait(wait_until);
      }
    } else {
//不是排在队伍前面，或者已经选出了一位领导
      r.cv.Wait();
    }

//请求静音从现在起保持
    if (stop_) {
      --requests_to_wait_;
      exit_cv_.Signal();
      return;
    }

//确保唤醒请求始终是其队列的头
    assert(r.granted ||
           (!queue_[Env::IO_HIGH].empty() &&
            &r == queue_[Env::IO_HIGH].front()) ||
           (!queue_[Env::IO_LOW].empty() &&
            &r == queue_[Env::IO_LOW].front()));
    assert(leader_ == nullptr ||
           (!queue_[Env::IO_HIGH].empty() &&
            leader_ == queue_[Env::IO_HIGH].front()) ||
           (!queue_[Env::IO_LOW].empty() &&
            leader_ == queue_[Env::IO_LOW].front()));

    if (leader_ == &r) {
//从TimedWait（）中唤醒
      if (timedout) {
//该加油了！
        Refill();

//不管怎样，重新选举一个新的领导人。这是为了简化
//选举处理。
        leader_ = nullptr;

//如果当前领队要离开，通知队列标题
        if (r.granted) {
//当前领导已获得配额。通知头
//排队参加下一轮选举。
          assert((queue_[Env::IO_HIGH].empty() ||
                    &r != queue_[Env::IO_HIGH].front()) &&
                 (queue_[Env::IO_LOW].empty() ||
                    &r != queue_[Env::IO_LOW].front()));
          if (!queue_[Env::IO_HIGH].empty()) {
            queue_[Env::IO_HIGH].front()->cv.Signal();
          } else if (!queue_[Env::IO_LOW].empty()) {
            queue_[Env::IO_LOW].front()->cv.Signal();
          }
//多恩
          break;
        }
      } else {
//自发醒来，需要继续等待
        assert(!r.granted);
        leader_ = nullptr;
      }
    } else {
//被前任领导叫醒：
//（1）如果授予了所请求的配额，就完成了。
//（2）如果未授予请求的配额，则表示当前线程
//被选为新的领导人候选人（前领导人获得了配额）。
//它需要参加领导人选举，因为新的要求可能
//在这条线被唤醒之前进来。所以可能需要
//再次执行wait（）。
      assert(!timedout);
    }
  } while (!r.granted);
}

void GenericRateLimiter::Refill() {
  TEST_SYNC_POINT("GenericRateLimiter::Refill");
  next_refill_us_ = NowMicrosMonotonic(env_) + refill_period_us_;
//结转上期剩余定额
  auto refill_bytes_per_period =
      refill_bytes_per_period_.load(std::memory_order_relaxed);
  if (available_bytes_ < refill_bytes_per_period) {
    available_bytes_ += refill_bytes_per_period;
  }

  int use_low_pri_first = rnd_.OneIn(fairness_) ? 0 : 1;
  for (int q = 0; q < 2; ++q) {
    auto use_pri = (use_low_pri_first == q) ? Env::IO_LOW : Env::IO_HIGH;
    auto* queue = &queue_[use_pri];
    while (!queue->empty()) {
      auto* next_req = queue->front();
      if (available_bytes_ < next_req->request_bytes) {
//避免饥饿
        next_req->request_bytes -= available_bytes_;
        available_bytes_ = 0;
        break;
      }
      available_bytes_ -= next_req->request_bytes;
      next_req->request_bytes = 0;
      total_bytes_through_[use_pri] += next_req->bytes;
      queue->pop_front();

      next_req->granted = true;
      if (next_req != leader_) {
//授予配额，向线程发出信号
        next_req->cv.Signal();
      }
    }
  }
}

int64_t GenericRateLimiter::CalculateRefillBytesPerPeriod(
    int64_t rate_bytes_per_sec) {
  if (port::kMaxInt64 / rate_bytes_per_sec < refill_period_us_) {
//避免在溢出情况下出现意外结果。现在的结果仍然是
//不准确，但数字足够大。
    return port::kMaxInt64 / 1000000;
  } else {
    return std::max(kMinRefillBytesPerPeriod,
                    rate_bytes_per_sec * refill_period_us_ / 1000000);
  }
}

RateLimiter* NewGenericRateLimiter(
    /*64_t速率_字节/秒，Int64_t重新填充周期_us/*=100*1000*/，
    Int32公平性/*=10*/,

    /*elimiter：：模式模式/*=ratelimiter：：模式：：kWriteOnly*/）
  断言（速率字节数/秒>0）；
  断言（重新填充期间大于0）；
  断言（公平性>0）；
  返回新的genericratelimiter（每秒\u字节数，重新填充周期\u，公平性，
                                模式）；
}

//命名空间rocksdb
