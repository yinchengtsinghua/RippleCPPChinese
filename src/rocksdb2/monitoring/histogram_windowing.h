
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

#pragma once

#include "monitoring/histogram.h"
#include "rocksdb/env.h"

namespace rocksdb {

class HistogramWindowingImpl : public Histogram
{
public:
  HistogramWindowingImpl();
  HistogramWindowingImpl(uint64_t num_windows,
                         uint64_t micros_per_window,
                         uint64_t min_num_per_window);

  HistogramWindowingImpl(const HistogramImpl&) = delete;
  HistogramWindowingImpl& operator=(const HistogramImpl&) = delete;

  ~HistogramWindowingImpl();

  virtual void Clear() override;
  virtual bool Empty() const override;
  virtual void Add(uint64_t value) override;
  virtual void Merge(const Histogram& other) override;
  void Merge(const HistogramWindowingImpl& other);

  virtual std::string ToString() const override;
  virtual const char* Name() const override { return "HistogramWindowingImpl"; }
  virtual uint64_t min() const override { return stats_.min(); }
  virtual uint64_t max() const override { return stats_.max(); }
  virtual uint64_t num() const override { return stats_.num(); }
  virtual double Median() const override;
  virtual double Percentile(double p) const override;
  virtual double Average() const override;
  virtual double StandardDeviation() const override;
  virtual void Data(HistogramData* const data) const override;

private:
  void TimerTick();
  void SwapHistoryBucket();
  inline uint64_t current_window() const {
    return current_window_.load(std::memory_order_relaxed);
  }
  inline uint64_t last_swap_time() const{
    return last_swap_time_.load(std::memory_order_relaxed);
  }

  Env* env_;
  std::mutex mutex_;

//在Windows上聚合统计，所有计算都完成了
//基于聚合值
  HistogramStat stats_;

//这是一个代表最近n次窗口的循环数组。
//每个条目都存储一个数据时间窗口。过期已完成
//基于窗口。
  std::unique_ptr<HistogramStat[]> window_stats_;

  std::atomic_uint_fast64_t current_window_;
  std::atomic_uint_fast64_t last_swap_time_;

//以下参数是可配置的
  uint64_t num_windows_ = 5;
  uint64_t micros_per_window_ = 60000000;
//默认情况下，不关心当前窗口中的值数量
//当决定是否交换窗口时。
  uint64_t min_num_per_window_ = 0;
};

}  //命名空间rocksdb
