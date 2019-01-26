
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
#include "monitoring/statistics.h"
#include "rocksdb/env.h"

namespace rocksdb {
//自动范围。
//将测量时间记录到相应的柱状图（如果统计）
//不是nullptr。如果指针不是nullptr，它也会保存到*elapsed中。
//如果overwrite为true，则会将其添加到*elapsed中，如果overwrite为false。
class StopWatch {
 public:
  StopWatch(Env* const env, Statistics* statistics, const uint32_t hist_type,
            uint64_t* elapsed = nullptr, bool overwrite = true)
      : env_(env),
        statistics_(statistics),
        hist_type_(hist_type),
        elapsed_(elapsed),
        overwrite_(overwrite),
        stats_enabled_(statistics && statistics->HistEnabledForType(hist_type)),
        start_time_((stats_enabled_ || elapsed != nullptr) ? env->NowMicros()
                                                           : 0) {}

  ~StopWatch() {
    if (elapsed_) {
      if (overwrite_) {
        *elapsed_ = env_->NowMicros() - start_time_;
      } else {
        *elapsed_ += env_->NowMicros() - start_time_;
      }
    }
    if (stats_enabled_) {
      statistics_->measureTime(hist_type_,
          (elapsed_ != nullptr) ? *elapsed_ :
                                  (env_->NowMicros() - start_time_));
    }
  }

  uint64_t start_time() const { return start_time_; }

 private:
  Env* const env_;
  Statistics* statistics_;
  const uint32_t hist_type_;
  uint64_t* elapsed_;
  bool overwrite_;
  bool stats_enabled_;
  const uint64_t start_time_;
};

//纳米二次精密秒表
class StopWatchNano {
 public:
  explicit StopWatchNano(Env* const env, bool auto_start = false)
      : env_(env), start_(0) {
    if (auto_start) {
      Start();
    }
  }

  void Start() { start_ = env_->NowNanos(); }

  uint64_t ElapsedNanos(bool reset = false) {
    auto now = env_->NowNanos();
    auto elapsed = now - start_;
    if (reset) {
      start_ = now;
    }
    return elapsed;
  }

  uint64_t ElapsedNanosSafe(bool reset = false) {
    return (env_ != nullptr) ? ElapsedNanos(reset) : 0U;
  }

 private:
  Env* const env_;
  uint64_t start_;
};

} //命名空间rocksdb
