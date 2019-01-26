
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
#include "monitoring/perf_level_imp.h"
#include "rocksdb/env.h"
#include "util/stop_watch.h"

namespace rocksdb {

class PerfStepTimer {
 public:
  explicit PerfStepTimer(uint64_t* metric, bool for_mutex = false)
      : enabled_(perf_level >= PerfLevel::kEnableTime ||
                 (!for_mutex && perf_level >= kEnableTimeExceptForMutex)),
        env_(enabled_ ? Env::Default() : nullptr),
        start_(0),
        metric_(metric) {}

  ~PerfStepTimer() {
    Stop();
  }

  void Start() {
    if (enabled_) {
      start_ = env_->NowNanos();
    }
  }

  void Measure() {
    if (start_) {
      uint64_t now = env_->NowNanos();
      *metric_ += now - start_;
      start_ = now;
    }
  }

  void Stop() {
    if (start_) {
      *metric_ += env_->NowNanos() - start_;
      start_ = 0;
    }
  }

 private:
  const bool enabled_;
  Env* const env_;
  uint64_t start_;
  uint64_t* metric_;
};

}  //命名空间rocksdb
