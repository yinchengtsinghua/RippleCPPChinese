
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
#include "table/format.h"
#include "table/persistent_cache_options.h"

namespace rocksdb {

struct BlockContents;

//PersistentCacheHelper
//
//封装一些辅助逻辑，以便从缓存中读写
class PersistentCacheHelper {
 public:
//将块插入原始页缓存
  static void InsertRawPage(const PersistentCacheOptions& cache_options,
                            const BlockHandle& handle, const char* data,
                            const size_t size);

//将块插入未压缩缓存
  static void InsertUncompressedPage(
      const PersistentCacheOptions& cache_options, const BlockHandle& handle,
      const BlockContents& contents);

//原始页CACGE的查找块
  static Status LookupRawPage(const PersistentCacheOptions& cache_options,
                              const BlockHandle& handle,
                              std::unique_ptr<char[]>* raw_data,
                              const size_t raw_data_size);

//未压缩缓存中的查找块
  static Status LookupUncompressedPage(
      const PersistentCacheOptions& cache_options, const BlockHandle& handle,
      BlockContents* contents);
};

}  //命名空间rocksdb
