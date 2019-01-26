
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2013，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "monitoring/histogram_windowing.h"
#include "monitoring/histogram.h"
#include "util/cast_util.h"

#include <algorithm>

namespace rocksdb {

HistogramWindowingImpl::HistogramWindowingImpl() {
  env_ = Env::Default();
  window_stats_.reset(new HistogramStat[num_windows_]);
  Clear();
}

HistogramWindowingImpl::HistogramWindowingImpl(
    uint64_t num_windows,
    uint64_t micros_per_window,
    uint64_t min_num_per_window) :
      num_windows_(num_windows),
      micros_per_window_(micros_per_window),
      min_num_per_window_(min_num_per_window) {
  env_ = Env::Default();
  window_stats_.reset(new HistogramStat[num_windows_]);
  Clear();
}

HistogramWindowingImpl::~HistogramWindowingImpl() {
}

void HistogramWindowingImpl::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);

  stats_.Clear();
  for (size_t i = 0; i < num_windows_; i++) {
    window_stats_[i].Clear();
  }
  current_window_.store(0, std::memory_order_relaxed);
  last_swap_time_.store(env_->NowMicros(), std::memory_order_relaxed);
}

bool HistogramWindowingImpl::Empty() const { return stats_.Empty(); }

//该功能设计为无锁，因为它处于关键路径中
//任何操作。
//每个单独的值都是原子的，只是一些样本可以
//在可以忍受的旧桶里。
void HistogramWindowingImpl::Add(uint64_t value){
  TimerTick();

//父（全局）成员更新
  stats_.Add(value);

//当前窗口更新
  window_stats_[current_window()].Add(value);
}

void HistogramWindowingImpl::Merge(const Histogram& other) {
  if (strcmp(Name(), other.Name()) == 0) {
    Merge(
        *static_cast_with_check<const HistogramWindowingImpl, const Histogram>(
            &other));
  }
}

void HistogramWindowingImpl::Merge(const HistogramWindowingImpl& other) {
  std::lock_guard<std::mutex> lock(mutex_);
  stats_.Merge(other.stats_);

  if (stats_.num_buckets_ != other.stats_.num_buckets_ ||
      micros_per_window_ != other.micros_per_window_) {
    return;
  }

  uint64_t cur_window = current_window();
  uint64_t other_cur_window = other.current_window();
//向后走以对齐
  for (unsigned int i = 0;
                    i < std::min(num_windows_, other.num_windows_); i++) {
    uint64_t window_index =
        (cur_window + num_windows_ - i) % num_windows_;
    uint64_t other_window_index =
        (other_cur_window + other.num_windows_ - i) % other.num_windows_;

    window_stats_[window_index].Merge(other.window_stats_[other_window_index]);
  }
}

std::string HistogramWindowingImpl::ToString() const {
  return stats_.ToString();
}

double HistogramWindowingImpl::Median() const {
  return Percentile(50.0);
}

double HistogramWindowingImpl::Percentile(double p) const {
//共重试3次
  for (int retry = 0; retry < 3; retry++) {
    uint64_t start_num = stats_.num();
    double result = stats_.Percentile(p);
//检测计算期间是否调用了交换存储桶或清除（）。
    if (stats_.num() >= start_num) {
      return result;
    }
  }
  return 0.0;
}

double HistogramWindowingImpl::Average() const {
  return stats_.Average();
}

double HistogramWindowingImpl::StandardDeviation() const {
  return stats_.StandardDeviation();
}

void HistogramWindowingImpl::Data(HistogramData * const data) const {
  stats_.Data(data);
}

void HistogramWindowingImpl::TimerTick() {
  uint64_t curr_time = env_->NowMicros();
  if (curr_time - last_swap_time() > micros_per_window_ &&
      window_stats_[current_window()].num() >= min_num_per_window_) {
    SwapHistoryBucket();
  }
}

void HistogramWindowingImpl::SwapHistoryBucket() {
//执行add（）的线程将与第一个互斥体竞争
//谁拿的计量器会处理桶交换和其他线程
//可以跳过这个。
//如果mutex由merge（）或clear（）保持，则下一个add（）将处理
//必要时交换。
  if (mutex_.try_lock()) {
    last_swap_time_.store(env_->NowMicros(), std::memory_order_relaxed);

    uint64_t curr_window = current_window();
    uint64_t next_window = (curr_window == num_windows_ - 1) ?
                                                    0 : curr_window + 1;

//从总计中减去下一个存储桶，然后交换到下一个存储桶
    HistogramStat& stats_to_drop = window_stats_[next_window];

    if (!stats_to_drop.Empty()) {
      for (size_t b = 0; b < stats_.num_buckets_; b++){
        stats_.buckets_[b].fetch_sub(
            stats_to_drop.bucket_at(b), std::memory_order_relaxed);
      }

      if (stats_.min() == stats_to_drop.min()) {
        uint64_t new_min = std::numeric_limits<uint64_t>::max();
        for (unsigned int i = 0; i < num_windows_; i++) {
          if (i != next_window) {
            uint64_t m = window_stats_[i].min();
            if (m < new_min) new_min = m;
          }
        }
        stats_.min_.store(new_min, std::memory_order_relaxed);
      }

      if (stats_.max() == stats_to_drop.max()) {
        uint64_t new_max = 0;
        for (unsigned int i = 0; i < num_windows_; i++) {
          if (i != next_window) {
            uint64_t m = window_stats_[i].max();
            if (m > new_max) new_max = m;
          }
        }
        stats_.max_.store(new_max, std::memory_order_relaxed);
      }

      stats_.num_.fetch_sub(stats_to_drop.num(), std::memory_order_relaxed);
      stats_.sum_.fetch_sub(stats_to_drop.sum(), std::memory_order_relaxed);
      stats_.sum_squares_.fetch_sub(
                  stats_to_drop.sum_squares(), std::memory_order_relaxed);

      stats_to_drop.Clear();
    }

//前进到下一个窗口桶
    current_window_.store(next_window, std::memory_order_relaxed);

    mutex_.unlock();
  }
}

}  //命名空间rocksdb
