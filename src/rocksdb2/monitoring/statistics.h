
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
#include "rocksdb/statistics.h"

#include <vector>
#include <atomic>
#include <string>

#include "monitoring/histogram.h"
#include "port/likely.h"
#include "port/port.h"
#include "util/core_local.h"
#include "util/mutexlock.h"

#ifdef __clang__
#define ROCKSDB_FIELD_UNUSED __attribute__((__unused__))
#else
#define ROCKSDB_FIELD_UNUSED
#endif  //阿克克兰格

namespace rocksdb {

enum TickersInternal : uint32_t {
  INTERNAL_TICKER_ENUM_START = TICKER_ENUM_MAX,
  INTERNAL_TICKER_ENUM_MAX
};

enum HistogramsInternal : uint32_t {
  INTERNAL_HISTOGRAM_START = HISTOGRAM_ENUM_MAX,
  INTERNAL_HISTOGRAM_ENUM_MAX
};


class StatisticsImpl : public Statistics {
 public:
  StatisticsImpl(std::shared_ptr<Statistics> stats,
                 bool enable_internal_stats);
  virtual ~StatisticsImpl();

  virtual uint64_t getTickerCount(uint32_t ticker_type) const override;
  virtual void histogramData(uint32_t histogram_type,
                             HistogramData* const data) const override;
  std::string getHistogramString(uint32_t histogram_type) const override;

  virtual void setTickerCount(uint32_t ticker_type, uint64_t count) override;
  virtual uint64_t getAndResetTickerCount(uint32_t ticker_type) override;
  virtual void recordTick(uint32_t ticker_type, uint64_t count) override;
  virtual void measureTime(uint32_t histogram_type, uint64_t value) override;

  virtual Status Reset() override;
  virtual std::string ToString() const override;
  virtual bool HistEnabledForType(uint32_t type) const override;

 private:
//如果不是nullptr，则将更新转发到“stats”指向的对象。
  std::shared_ptr<Statistics> stats_;
//TODO（AJKR）：清除此项，因为不再有内部统计信息
  bool enable_internal_stats_;
//同步在其他核心的本地数据上运行的任何操作，
//这样，像reset（）这样的操作可以原子地执行。
  mutable port::Mutex aggregate_lock_;

//ticker/histogram数据存储在这个结构中，我们将存储它
//每个核心。它与缓存对齐，因此属于不同
//核心永远不能共享同一缓存线。
//
//对齐属性将根据平台扩展为“无”。
  struct StatisticsData {
    std::atomic_uint_fast64_t tickers_[INTERNAL_TICKER_ENUM_MAX] = {{0}};
    HistogramImpl histograms_[INTERNAL_HISTOGRAM_ENUM_MAX];
    char
        padding[(CACHE_LINE_SIZE -
                 (INTERNAL_TICKER_ENUM_MAX * sizeof(std::atomic_uint_fast64_t) +
                  INTERNAL_HISTOGRAM_ENUM_MAX * sizeof(HistogramImpl)) %
                     CACHE_LINE_SIZE) %
                CACHE_LINE_SIZE] ROCKSDB_FIELD_UNUSED;
  };

  static_assert(sizeof(StatisticsData) % 64 == 0, "Expected 64-byte aligned");

  CoreLocalArray<StatisticsData> per_core_stats_;

  uint64_t getTickerCountLocked(uint32_t ticker_type) const;
  std::unique_ptr<HistogramImpl> getHistogramImplLocked(
      uint32_t histogram_type) const;
  void setTickerCountLocked(uint32_t ticker_type, uint64_t count);
};

//效用函数
inline void MeasureTime(Statistics* statistics, uint32_t histogram_type,
                        uint64_t value) {
  if (statistics) {
    statistics->measureTime(histogram_type, value);
  }
}

inline void RecordTick(Statistics* statistics, uint32_t ticker_type,
                       uint64_t count = 1) {
  if (statistics) {
    statistics->recordTick(ticker_type, count);
  }
}

inline void SetTickerCount(Statistics* statistics, uint32_t ticker_type,
                           uint64_t count) {
  if (statistics) {
    statistics->setTickerCount(ticker_type, count);
  }
}

}
