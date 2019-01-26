
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

#include <string>

#include "monitoring/statistics.h"
#include "rocksdb/persistent_cache.h"

namespace rocksdb {

//持久缓存选项
//
//这描述了页面缓存的缓存行为
//这用于传递用于缓存的上下文和缓存句柄
struct PersistentCacheOptions {
  PersistentCacheOptions() {}
  explicit PersistentCacheOptions(
      const std::shared_ptr<PersistentCache>& _persistent_cache,
      const std::string _key_prefix, Statistics* const _statistics)
      : persistent_cache(_persistent_cache),
        key_prefix(_key_prefix),
        statistics(_statistics) {}

  virtual ~PersistentCacheOptions() {}

  std::shared_ptr<PersistentCache> persistent_cache;
  std::string key_prefix;
  Statistics* statistics = nullptr;
};

}  //命名空间rocksdb
