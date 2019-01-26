
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
#pragma once

#ifndef ROCKSDB_LITE

#include <functional>
#include <string>
#include <unordered_map>

#include "rocksdb/slice.h"

#include "utilities/persistent_cache/block_cache_tier_file.h"
#include "utilities/persistent_cache/hash_table.h"
#include "utilities/persistent_cache/hash_table_evictable.h"
#include "utilities/persistent_cache/lrulist.h"

namespace rocksdb {

//
//块缓存层元数据
//
//blockcachetiermetadata保存与块关联的所有元数据
//隐藏物。它
//基本上包含2个索引和一个LRU。
//
//块缓存索引
//
//这是一个将给定键映射到LBA（逻辑块）的正向索引
//地址）LBA是指向缓存中记录的磁盘指针。
//
//lba=缓存ID、偏移量、大小
//
//缓存文件索引
//
//这是将给定缓存ID映射到缓存文件对象的转发索引。
//通常，您将使用LBA进行查找，并使用对象进行读或写操作。
struct BlockInfo {
  explicit BlockInfo(const Slice& key, const LBA& lba = LBA())
      : key_(key.ToString()), lba_(lba) {}

  std::string key_;
  LBA lba_;
};

class BlockCacheTierMetadata {
 public:
  explicit BlockCacheTierMetadata(const uint32_t blocks_capacity = 1024 * 1024,
                                  const uint32_t cachefile_capacity = 10 * 1024)
      : cache_file_index_(cachefile_capacity), block_index_(blocks_capacity) {}

  virtual ~BlockCacheTierMetadata() {}

//插入给定的缓存文件
  bool Insert(BlockCacheFile* file);

//基于缓存ID的查找缓存文件
  BlockCacheFile* Lookup(const uint32_t cache_id);

//插入块信息到块索引
  BlockInfo* Insert(const Slice& key, const LBA& lba);
//布尔插入（blockinfo*binfo）；

//从块索引查找块信息
  bool Lookup(const Slice& key, LBA* lba);

//从块索引中删除给定的
  BlockInfo* Remove(const Slice& key);

//使用lru策略查找和逐出缓存文件
  BlockCacheFile* Evict();

//清除元数据内容
  virtual void Clear();

 protected:
//从给定文件中删除所有块信息
  virtual void RemoveAllKeys(BlockCacheFile* file);

 private:
//缓存文件索引定义
//
//缓存ID=>blockcachefile
  struct BlockCacheFileHash {
    uint64_t operator()(const BlockCacheFile* rec) {
      return std::hash<uint32_t>()(rec->cacheid());
    }
  };

  struct BlockCacheFileEqual {
    uint64_t operator()(const BlockCacheFile* lhs, const BlockCacheFile* rhs) {
      return lhs->cacheid() == rhs->cacheid();
    }
  };

  typedef EvictableHashTable<BlockCacheFile, BlockCacheFileHash,
                             BlockCacheFileEqual>
      CacheFileIndexType;

//块查找索引
//
//密钥＝LBA
  struct Hash {
    size_t operator()(BlockInfo* node) const {
      return std::hash<std::string>()(node->key_);
    }
  };

  struct Equal {
    size_t operator()(BlockInfo* lhs, BlockInfo* rhs) const {
      return lhs->key_ == rhs->key_;
    }
  };

  typedef HashTable<BlockInfo*, Hash, Equal> BlockIndexType;

  CacheFileIndexType cache_file_index_;
  BlockIndexType block_index_;
};

}  //命名空间rocksdb

#endif
