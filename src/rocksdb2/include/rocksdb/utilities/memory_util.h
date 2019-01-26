
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

#ifndef ROCKSDB_LITE

#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "rocksdb/cache.h"
#include "rocksdb/db.h"

namespace rocksdb {

//返回指定数据库实例的当前内存使用情况。
class MemoryUtil {
 public:
  enum UsageType : int {
//所有MEM表的内存使用情况。
    kMemTableTotal = 0,
//那些未刷新的MEM表的内存使用情况。
    kMemTableUnFlushed = 1,
//所有表读取器的内存使用情况。
    kTableReadersTotal = 2,
//缓存的内存使用情况。
    kCacheTotal = 3,
    kNumUsageTypes = 4
  };

//返回输入中不同类型的大致内存使用情况
//DBS和缓存集列表。例如，在输出映射中
//用法\按类型，用法\按类型[kmemtabletotal]将存储内存
//使用来自所有输入rocksdb实例的所有mem表。
//
//注意，对于缓存类内的内存使用，我们将
//仅报告输入“缓存集”的使用情况
//包括输入列表“dbs”中的缓存使用
//星展。
  static Status GetApproximateMemoryUsageByType(
      const std::vector<DB*>& dbs,
      const std::unordered_set<const Cache*> cache_set,
      std::map<MemoryUtil::UsageType, uint64_t>* usage_by_type);
};
}  //命名空间rocksdb
#endif  //！摇滚乐
