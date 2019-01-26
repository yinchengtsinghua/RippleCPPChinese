
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
#ifndef ROCKSDB_LITE

#include "utilities/persistent_cache/volatile_tier_impl.h"

#include <string>

namespace rocksdb {

void VolatileCacheTier::DeleteCacheData(VolatileCacheTier::CacheData* data) {
  assert(data);
  delete data;
}

VolatileCacheTier::~VolatileCacheTier() { index_.Clear(&DeleteCacheData); }

PersistentCache::StatsType VolatileCacheTier::Stats() {
  std::map<std::string, double> stat;
  stat.insert({"persistent_cache.volatile_cache.hits",
               static_cast<double>(stats_.cache_hits_)});
  stat.insert({"persistent_cache.volatile_cache.misses",
               static_cast<double>(stats_.cache_misses_)});
  stat.insert({"persistent_cache.volatile_cache.inserts",
               static_cast<double>(stats_.cache_inserts_)});
  stat.insert({"persistent_cache.volatile_cache.evicts",
               static_cast<double>(stats_.cache_evicts_)});
  stat.insert({"persistent_cache.volatile_cache.hit_pct",
               static_cast<double>(stats_.CacheHitPct())});
  stat.insert({"persistent_cache.volatile_cache.miss_pct",
               static_cast<double>(stats_.CacheMissPct())});

  auto out = PersistentCacheTier::Stats();
  out.push_back(stat);
  return out;
}

Status VolatileCacheTier::Insert(const Slice& page_key, const char* data,
                                 const size_t size) {
//先决条件
  assert(data);
  assert(size);

//增加尺寸
  size_ += size;

//检查是否超出限制，如果超出限制，则清除一些空间
  while (size_ > max_size_) {
    if (!Evict()) {
//无法收回数据，我们放弃了这样我们就不会尖峰读取
//等待时间
      assert(size_ >= size);
      size_ -= size;
      return Status::TryAgain("Unable to evict any data");
    }
  }

  assert(size_ >= size);

//插入顺序：LRU，后跟索引
  std::string key(page_key.data(), page_key.size());
  std::string value(data, size);
  std::unique_ptr<CacheData> cache_data(
      new CacheData(std::move(key), std::move(value)));
  bool ok = index_.Insert(cache_data.get());
  if (!ok) {
//减小我们提前增加的大小
    assert(size_ >= size);
    size_ -= size;
//未能插入到缓存，块已在缓存中
    return Status::TryAgain("key already exists in volatile cache");
  }

  cache_data.release();
  stats_.cache_inserts_++;
  return Status::OK();
}

Status VolatileCacheTier::Lookup(const Slice& page_key,
                                 std::unique_ptr<char[]>* result,
                                 size_t* size) {
  CacheData key(std::move(page_key.ToString()));
  CacheData* kv;
  bool ok = index_.Find(&key, &kv);
  if (ok) {
//设置返回数据
    result->reset(new char[kv->value.size()]);
    memcpy(result->get(), kv->value.c_str(), kv->value.size());
    *size = kv->value.size();
//删除对缓存数据的引用
    kv->refs_--;
//更新统计数据
    stats_.cache_hits_++;
    return Status::OK();
  }

  stats_.cache_misses_++;

  if (next_tier()) {
    return next_tier()->Lookup(page_key, result, size);
  }

  return Status::NotFound("key not found in volatile cache");
}

bool VolatileCacheTier::Erase(const Slice& key) {
  assert(!"not supported");
  return true;
}

bool VolatileCacheTier::Evict() {
  CacheData* edata = index_.Evict();
  if (!edata) {
//无法逐出任何对象
    return false;
  }

  stats_.cache_evicts_++;

//将收回的对象推到下一个级别
  if (next_tier()) {
    next_tier()->Insert(Slice(edata->key), edata->value.c_str(),
                        edata->value.size());
  }

//调整大小并销毁数据
  size_ -= edata->value.size();
  delete edata;

  return true;
}

}  //命名空间rocksdb

#endif
