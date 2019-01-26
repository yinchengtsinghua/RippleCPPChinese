
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

#include <stdint.h>
#include <memory>
#include <string>
#include "rocksdb/cache.h"
#include "rocksdb/env.h"
#include "rocksdb/slice.h"
#include "rocksdb/statistics.h"
#include "rocksdb/status.h"

namespace rocksdb {

class SimCache;

//出于检测目的，请使用newsimcache而不是newlrucache API
//newsimcache是返回simcache实例的包装函数，该实例可以
//在Simcache类中除了缓存接口之外还提供了其他接口
//在不实际分配内存的情况下预测块缓存命中率。它
//可以帮助用户调整当前块缓存大小，并确定如何
//他们正在使用内存。
//
//由于getSimCapacity（）返回模拟的容量，因此它与
//实际内存使用情况，可以估计为：
//SIM卡容量*入口尺寸/（入口尺寸+块尺寸）
//其中76<=入口尺寸<=104，
//blockBasedTableOptions.block_size=4096（默认值），但可以配置，
//因此，通常情况下，Simcache的实际内存开销小于
//SIM卡容量*2%
extern std::shared_ptr<SimCache> NewSimCache(std::shared_ptr<Cache> cache,
                                             size_t sim_capacity,
                                             int num_shard_bits);

class SimCache : public Cache {
 public:
  SimCache() {}

  ~SimCache() override {}

  const char* Name() const override { return "SimCache"; }

//返回模拟的Simcache的最大配置容量
  virtual size_t GetSimCapacity() const = 0;

//Simcache不向用户提供内部处理程序引用，因此始终
//pinnedusage=0，行为将不完全一致
//有真正的缓存。
//返回位于Simcache中的条目的内存大小。
  virtual size_t GetSimUsage() const = 0;

//设置Simcache的最大配置容量。当新
//容量小于旧容量，现有使用量为
//大于新容量，实现将清除旧条目
//以适应新的功能。
  virtual void SetSimCapacity(size_t capacity) = 0;

//返回Simcache的查找时间
  virtual uint64_t get_miss_counter() const = 0;
//返回Simcache的命中次数
  virtual uint64_t get_hit_counter() const = 0;
//重置查找和命中计数器
  virtual void reset_counter() = 0;
//simcache统计信息的字符串表示法
  virtual std::string ToString() const = 0;

//开始将缓存活动的日志（添加/查找）存储到
//位于活动日志文件，最大日志大小选项的文件可用于
//在达到特定大小后自动停止记录到文件
//字节，值为0禁用此功能
  virtual Status StartActivityLogging(const std::string& activity_log_file,
                                      Env* env, uint64_t max_logging_size = 0) = 0;

//停止缓存活动日志记录（如果有）
  virtual void StopActivityLogging() = 0;

//后台缓存日志记录的状态
  virtual Status GetActivityLoggingStatus() = 0;

 private:
  SimCache(const SimCache&);
  SimCache& operator=(const SimCache&);
};

}  //命名空间rocksdb
