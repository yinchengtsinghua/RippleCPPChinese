
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
#pragma once

#ifndef ROCKSDB_LITE

#include <atomic>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include "rocksdb/cache.h"
#include "utilities/persistent_cache/hash_table.h"
#include "utilities/persistent_cache/hash_table_evictable.h"
#include "utilities/persistent_cache/persistent_cache_tier.h"

//挥发物
//
//此文件提供用于缓存的持久缓存层实现
//RAM中的键/值。
//
//键/值
//γ
//V
//+---------------+
//volatilecachetier存储在可收回哈希表中
//+---------------+
//γ
//V
//论驱逐
//推到下一层
//
//实现被设计为并发的。可收回哈希表
//不过，在这一点上实现不是并发的。
//
//驱逐算法是LRU
namespace rocksdb {

class VolatileCacheTier : public PersistentCacheTier {
 public:
  explicit VolatileCacheTier(
      const bool is_compressed = true,
      const size_t max_size = std::numeric_limits<size_t>::max())
      : is_compressed_(is_compressed), max_size_(max_size) {}

  virtual ~VolatileCacheTier();

//插入到缓存
  Status Insert(const Slice& page_key, const char* data,
                const size_t size) override;
//在缓存中查找密钥
  Status Lookup(const Slice& page_key, std::unique_ptr<char[]>* data,
                size_t* size) override;

//是压缩缓存吗？
  bool IsCompressed() override { return is_compressed_; }

//从缓存中清除密钥
  bool Erase(const Slice& key) override;

  std::string GetPrintableOptions() const override {
    return "VolatileCacheTier";
  }

//将状态显示为映射
  PersistentCache::StatsType Stats() override;

 private:
//
//缓存数据抽象
//
  struct CacheData : LRUElement<CacheData> {
    explicit CacheData(CacheData&& rhs) ROCKSDB_NOEXCEPT
        : key(std::move(rhs.key)),
          value(std::move(rhs.value)) {}

    explicit CacheData(const std::string& _key, const std::string& _value = "")
        : key(_key), value(_value) {}

    virtual ~CacheData() {}

    const std::string key;
    const std::string value;
  };

  static void DeleteCacheData(CacheData* data);

//
//索引和LRU定义
//
  struct CacheDataHash {
    uint64_t operator()(const CacheData* obj) const {
      assert(obj);
      return std::hash<std::string>()(obj->key);
    }
  };

  struct CacheDataEqual {
    bool operator()(const CacheData* lhs, const CacheData* rhs) const {
      assert(lhs);
      assert(rhs);
      return lhs->key == rhs->key;
    }
  };

  struct Statistics {
    std::atomic<uint64_t> cache_misses_{0};
    std::atomic<uint64_t> cache_hits_{0};
    std::atomic<uint64_t> cache_inserts_{0};
    std::atomic<uint64_t> cache_evicts_{0};

    double CacheHitPct() const {
      auto lookups = cache_hits_ + cache_misses_;
      return lookups ? 100 * cache_hits_ / static_cast<double>(lookups) : 0.0;
    }

    double CacheMissPct() const {
      auto lookups = cache_hits_ + cache_misses_;
      return lookups ? 100 * cache_misses_ / static_cast<double>(lookups) : 0.0;
    }
  };

  typedef EvictableHashTable<CacheData, CacheDataHash, CacheDataEqual>
      IndexType;

//驱逐鲁鲁尾巴
  bool Evict();

const bool is_compressed_ = true;    //它存储压缩数据吗
IndexType index_;                    //内存缓存
std::atomic<uint64_t> max_size_{0};  //缓存的最大大小
std::atomic<uint64_t> size_{0};      //缓存大小
  Statistics stats_;
};

}  //命名空间rocksdb

#endif
