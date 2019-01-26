
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
#ifndef ROCKSDB_LITE

#include "utilities/persistent_cache/block_cache_tier_metadata.h"

#include <functional>

namespace rocksdb {

bool BlockCacheTierMetadata::Insert(BlockCacheFile* file) {
  return cache_file_index_.Insert(file);
}

BlockCacheFile* BlockCacheTierMetadata::Lookup(const uint32_t cache_id) {
  BlockCacheFile* ret = nullptr;
  BlockCacheFile lookup_key(cache_id);
  bool ok = cache_file_index_.Find(&lookup_key, &ret);
  if (ok) {
    assert(ret->refs_);
    return ret;
  }
  return nullptr;
}

BlockCacheFile* BlockCacheTierMetadata::Evict() {
  using std::placeholders::_1;
  auto fn = std::bind(&BlockCacheTierMetadata::RemoveAllKeys, this, _1);
  return cache_file_index_.Evict(fn);
}

void BlockCacheTierMetadata::Clear() {
  cache_file_index_.Clear([](BlockCacheFile* arg){ delete arg; });
  block_index_.Clear([](BlockInfo* arg){ delete arg; });
}

BlockInfo* BlockCacheTierMetadata::Insert(const Slice& key, const LBA& lba) {
  std::unique_ptr<BlockInfo> binfo(new BlockInfo(key, lba));
  if (!block_index_.Insert(binfo.get())) {
    return nullptr;
  }
  return binfo.release();
}

bool BlockCacheTierMetadata::Lookup(const Slice& key, LBA* lba) {
  BlockInfo lookup_key(key);
  BlockInfo* block;
  port::RWMutex* rlock = nullptr;
  if (!block_index_.Find(&lookup_key, &block, &rlock)) {
    return false;
  }

  ReadUnlock _(rlock);
  assert(block->key_ == key.ToString());
  if (lba) {
    *lba = block->lba_;
  }
  return true;
}

BlockInfo* BlockCacheTierMetadata::Remove(const Slice& key) {
  BlockInfo lookup_key(key);
  BlockInfo* binfo = nullptr;
  bool ok __attribute__((__unused__)) = block_index_.Erase(&lookup_key, &binfo);
  assert(ok);
  return binfo;
}

void BlockCacheTierMetadata::RemoveAllKeys(BlockCacheFile* f) {
  for (BlockInfo* binfo : f->block_infos()) {
    BlockInfo* tmp = nullptr;
    bool status = block_index_.Erase(binfo, &tmp);
    (void)status;
    assert(status);
    assert(tmp == binfo);
    delete binfo;
  }
  f->block_infos().clear();
}

}  //命名空间rocksdb

#endif
