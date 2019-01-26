
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

#include "table/persistent_cache_helper.h"
#include "table/block_based_table_reader.h"
#include "table/format.h"

namespace rocksdb {

void PersistentCacheHelper::InsertRawPage(
    const PersistentCacheOptions& cache_options, const BlockHandle& handle,
    const char* data, const size_t size) {
  assert(cache_options.persistent_cache);
  assert(cache_options.persistent_cache->IsCompressed());

//构造页键
  char cache_key[BlockBasedTable::kMaxCacheKeyPrefixSize + kMaxVarint64Length];
  auto key = BlockBasedTable::GetCacheKey(cache_options.key_prefix.c_str(),
                                          cache_options.key_prefix.size(),
                                          handle, cache_key);
//将内容插入缓存
  cache_options.persistent_cache->Insert(key, data, size);
}

void PersistentCacheHelper::InsertUncompressedPage(
    const PersistentCacheOptions& cache_options, const BlockHandle& handle,
    const BlockContents& contents) {
  assert(cache_options.persistent_cache);
  assert(!cache_options.persistent_cache->IsCompressed());
  if (!contents.cachable || contents.compression_type != kNoCompression) {
//我们不应该缓存这个。要么
//（1）内容不可缓存
//（2）内容被压缩
    return;
  }

//构造页键
  char cache_key[BlockBasedTable::kMaxCacheKeyPrefixSize + kMaxVarint64Length];
  auto key = BlockBasedTable::GetCacheKey(cache_options.key_prefix.c_str(),
                                          cache_options.key_prefix.size(),
                                          handle, cache_key);
//将块内容插入页面缓存
  cache_options.persistent_cache->Insert(key, contents.data.data(),
                                         contents.data.size());
}

Status PersistentCacheHelper::LookupRawPage(
    const PersistentCacheOptions& cache_options, const BlockHandle& handle,
    std::unique_ptr<char[]>* raw_data, const size_t raw_data_size) {
  assert(cache_options.persistent_cache);
  assert(cache_options.persistent_cache->IsCompressed());

//构造页键
  char cache_key[BlockBasedTable::kMaxCacheKeyPrefixSize + kMaxVarint64Length];
  auto key = BlockBasedTable::GetCacheKey(cache_options.key_prefix.c_str(),
                                          cache_options.key_prefix.size(),
                                          handle, cache_key);
//查找页
  size_t size;
  Status s = cache_options.persistent_cache->Lookup(key, raw_data, &size);
  if (!s.ok()) {
//缓存缺失
    RecordTick(cache_options.statistics, PERSISTENT_CACHE_MISS);
    return s;
  }

//缓存命中
  assert(raw_data_size == handle.size() + kBlockTrailerSize);
  assert(size == raw_data_size);
  RecordTick(cache_options.statistics, PERSISTENT_CACHE_HIT);
  return Status::OK();
}

Status PersistentCacheHelper::LookupUncompressedPage(
    const PersistentCacheOptions& cache_options, const BlockHandle& handle,
    BlockContents* contents) {
  assert(cache_options.persistent_cache);
  assert(!cache_options.persistent_cache->IsCompressed());
  if (!contents) {
//我们不应该在缓存中查找。要么
//（1）无处存放
    return Status::NotFound();
  }

//构造页键
  char cache_key[BlockBasedTable::kMaxCacheKeyPrefixSize + kMaxVarint64Length];
  auto key = BlockBasedTable::GetCacheKey(cache_options.key_prefix.c_str(),
                                          cache_options.key_prefix.size(),
                                          handle, cache_key);
//查找页
  std::unique_ptr<char[]> data;
  size_t size;
  Status s = cache_options.persistent_cache->Lookup(key, &data, &size);
  if (!s.ok()) {
//缓存缺失
    RecordTick(cache_options.statistics, PERSISTENT_CACHE_MISS);
    return s;
  }

//请注意，我们可能正在将压缩数据大小与
//未压缩数据大小
  assert(handle.size() <= size);

//更新统计数据
  RecordTick(cache_options.statistics, PERSISTENT_CACHE_HIT);
//构造结果和返回
  *contents =
      /*ckcontents（std:：move（data），size，false/*可缓存*/，knocompression）；
  返回状态：：OK（）；
}

//命名空间rocksdb
