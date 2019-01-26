
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

#include "rocksdb/utilities/memory_util.h"

#include "db/db_impl.h"

namespace rocksdb {

Status MemoryUtil::GetApproximateMemoryUsageByType(
    const std::vector<DB*>& dbs,
    const std::unordered_set<const Cache*> cache_set,
    std::map<MemoryUtil::UsageType, uint64_t>* usage_by_type) {
  usage_by_type->clear();

//可记忆的
  for (auto* db : dbs) {
    uint64_t usage = 0;
    if (db->GetAggregatedIntProperty(DB::Properties::kSizeAllMemTables,
                                     &usage)) {
      (*usage_by_type)[MemoryUtil::kMemTableTotal] += usage;
    }
    if (db->GetAggregatedIntProperty(DB::Properties::kCurSizeAllMemTables,
                                     &usage)) {
      (*usage_by_type)[MemoryUtil::kMemTableUnFlushed] += usage;
    }
  }

//表格阅读器
  for (auto* db : dbs) {
    uint64_t usage = 0;
    if (db->GetAggregatedIntProperty(DB::Properties::kEstimateTableReadersMem,
                                     &usage)) {
      (*usage_by_type)[MemoryUtil::kTableReadersTotal] += usage;
    }
  }

//隐藏物
  for (const auto* cache : cache_set) {
    if (cache != nullptr) {
      (*usage_by_type)[MemoryUtil::kCacheTotal] += cache->GetUsage();
    }
  }

  return Status::OK();
}
}  //命名空间rocksdb
#endif  //！摇滚乐
