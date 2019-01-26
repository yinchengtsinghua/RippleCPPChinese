
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
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "cache/lru_cache.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "util/mutexlock.h"

namespace rocksdb {

LRUHandleTable::LRUHandleTable() : list_(nullptr), length_(0), elems_(0) {
  Resize();
}

LRUHandleTable::~LRUHandleTable() {
  ApplyToAllCacheEntries([](LRUHandle* h) {
    if (h->refs == 1) {
      h->Free();
    }
  });
  delete[] list_;
}

LRUHandle* LRUHandleTable::Lookup(const Slice& key, uint32_t hash) {
  return *FindPointer(key, hash);
}

LRUHandle* LRUHandleTable::Insert(LRUHandle* h) {
  LRUHandle** ptr = FindPointer(h->key(), h->hash);
  LRUHandle* old = *ptr;
  h->next_hash = (old == nullptr ? nullptr : old->next_hash);
  *ptr = h;
  if (old == nullptr) {
    ++elems_;
    if (elems_ > length_) {
//由于每个缓存条目都相当大，因此我们的目标是
//平均链接列表长度（<=1）。
      Resize();
    }
  }
  return old;
}

LRUHandle* LRUHandleTable::Remove(const Slice& key, uint32_t hash) {
  LRUHandle** ptr = FindPointer(key, hash);
  LRUHandle* result = *ptr;
  if (result != nullptr) {
    *ptr = result->next_hash;
    --elems_;
  }
  return result;
}

LRUHandle** LRUHandleTable::FindPointer(const Slice& key, uint32_t hash) {
  LRUHandle** ptr = &list_[hash & (length_ - 1)];
  while (*ptr != nullptr && ((*ptr)->hash != hash || key != (*ptr)->key())) {
    ptr = &(*ptr)->next_hash;
  }
  return ptr;
}

void LRUHandleTable::Resize() {
  uint32_t new_length = 16;
  while (new_length < elems_ * 1.5) {
    new_length *= 2;
  }
  LRUHandle** new_list = new LRUHandle*[new_length];
  memset(new_list, 0, sizeof(new_list[0]) * new_length);
  uint32_t count = 0;
  for (uint32_t i = 0; i < length_; i++) {
    LRUHandle* h = list_[i];
    while (h != nullptr) {
      LRUHandle* next = h->next_hash;
      uint32_t hash = h->hash;
      LRUHandle** ptr = &new_list[hash & (new_length - 1)];
      h->next_hash = *ptr;
      *ptr = h;
      h = next;
      count++;
    }
  }
  assert(elems_ == count);
  delete[] list_;
  list_ = new_list;
  length_ = new_length;
}

LRUCacheShard::LRUCacheShard()
    : high_pri_pool_usage_(0), usage_(0), lru_usage_(0) {
//生成空循环链接列表
  lru_.next = &lru_;
  lru_.prev = &lru_;
  lru_low_pri_ = &lru_;
}

LRUCacheShard::~LRUCacheShard() {}

bool LRUCacheShard::Unref(LRUHandle* e) {
  assert(e->refs > 0);
  e->refs--;
  return e->refs == 0;
}

//调用deleter和free

void LRUCacheShard::EraseUnRefEntries() {
  autovector<LRUHandle*> last_reference_list;
  {
    MutexLock l(&mutex_);
    while (lru_.next != &lru_) {
      LRUHandle* old = lru_.next;
      assert(old->InCache());
      assert(old->refs ==
1);  //LRU列表包含可能被收回的元素
      LRU_Remove(old);
      table_.Remove(old->key(), old->hash);
      old->SetInCache(false);
      Unref(old);
      usage_ -= old->charge;
      last_reference_list.push_back(old);
    }
  }

  for (auto entry : last_reference_list) {
    entry->Free();
  }
}

void LRUCacheShard::ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                           bool thread_safe) {
  if (thread_safe) {
    mutex_.Lock();
  }
  table_.ApplyToAllCacheEntries(
      [callback](LRUHandle* h) { callback(h->value, h->charge); });
  if (thread_safe) {
    mutex_.Unlock();
  }
}

void LRUCacheShard::TEST_GetLRUList(LRUHandle** lru, LRUHandle** lru_low_pri) {
  *lru = &lru_;
  *lru_low_pri = lru_low_pri_;
}

size_t LRUCacheShard::TEST_GetLRUSize() {
  LRUHandle* lru_handle = lru_.next;
  size_t lru_size = 0;
  while (lru_handle != &lru_) {
    lru_size++;
    lru_handle = lru_handle->next;
  }
  return lru_size;
}

void LRUCacheShard::LRU_Remove(LRUHandle* e) {
  assert(e->next != nullptr);
  assert(e->prev != nullptr);
  if (lru_low_pri_ == e) {
    lru_low_pri_ = e->prev;
  }
  e->next->prev = e->prev;
  e->prev->next = e->next;
  e->prev = e->next = nullptr;
  lru_usage_ -= e->charge;
  if (e->InHighPriPool()) {
    assert(high_pri_pool_usage_ >= e->charge);
    high_pri_pool_usage_ -= e->charge;
  }
}

void LRUCacheShard::LRU_Insert(LRUHandle* e) {
  assert(e->next == nullptr);
  assert(e->prev == nullptr);
  if (high_pri_pool_ratio_ > 0 && e->IsHighPri()) {
//将“E”插入LRU列表的标题。
    e->next = &lru_;
    e->prev = lru_.prev;
    e->prev->next = e;
    e->next->prev = e;
    e->SetInHighPriPool(true);
    high_pri_pool_usage_ += e->charge;
    MaintainPoolSize();
  } else {
//将“E”插入低优先级池的池头。注意什么时候
//高优先级池比率为0，低优先级池的头也是LRU列表的头。
    e->next = lru_low_pri_->next;
    e->prev = lru_low_pri_;
    e->prev->next = e;
    e->next->prev = e;
    e->SetInHighPriPool(false);
    lru_low_pri_ = e;
  }
  lru_usage_ += e->charge;
}

void LRUCacheShard::MaintainPoolSize() {
  while (high_pri_pool_usage_ > high_pri_pool_capacity_) {
//将高优先级池中的最后一个条目溢出到低优先级池。
    lru_low_pri_ = lru_low_pri_->next;
    assert(lru_low_pri_ != &lru_);
    lru_low_pri_->SetInHighPriPool(false);
    high_pri_pool_usage_ -= lru_low_pri_->charge;
  }
}

void LRUCacheShard::EvictFromLRU(size_t charge,
                                 autovector<LRUHandle*>* deleted) {
  while (usage_ + charge > capacity_ && lru_.next != &lru_) {
    LRUHandle* old = lru_.next;
    assert(old->InCache());
assert(old->refs == 1);  //LRU列表包含可能被收回的元素
    LRU_Remove(old);
    table_.Remove(old->key(), old->hash);
    old->SetInCache(false);
    Unref(old);
    usage_ -= old->charge;
    deleted->push_back(old);
  }
}

void* LRUCacheShard::operator new(size_t size) {
  return port::cacheline_aligned_alloc(size);
}

void* LRUCacheShard::operator new[](size_t size) {
  return port::cacheline_aligned_alloc(size);
}

void LRUCacheShard::operator delete(void *memblock) {
  port::cacheline_aligned_free(memblock);
}

void LRUCacheShard::operator delete[](void* memblock) {
  port::cacheline_aligned_free(memblock);
}

void LRUCacheShard::SetCapacity(size_t capacity) {
  autovector<LRUHandle*> last_reference_list;
  {
    MutexLock l(&mutex_);
    capacity_ = capacity;
    high_pri_pool_capacity_ = capacity_ * high_pri_pool_ratio_;
    EvictFromLRU(0, &last_reference_list);
  }
//我们在mutex之外为
//性能原因
  for (auto entry : last_reference_list) {
    entry->Free();
  }
}

void LRUCacheShard::SetStrictCapacityLimit(bool strict_capacity_limit) {
  MutexLock l(&mutex_);
  strict_capacity_limit_ = strict_capacity_limit;
}

Cache::Handle* LRUCacheShard::Lookup(const Slice& key, uint32_t hash) {
  MutexLock l(&mutex_);
  LRUHandle* e = table_.Lookup(key, hash);
  if (e != nullptr) {
    assert(e->InCache());
    if (e->refs == 1) {
      LRU_Remove(e);
    }
    e->refs++;
  }
  return reinterpret_cast<Cache::Handle*>(e);
}

bool LRUCacheShard::Ref(Cache::Handle* h) {
  LRUHandle* handle = reinterpret_cast<LRUHandle*>(h);
  MutexLock l(&mutex_);
  if (handle->InCache() && handle->refs == 1) {
    LRU_Remove(handle);
  }
  handle->refs++;
  return true;
}

void LRUCacheShard::SetHighPriorityPoolRatio(double high_pri_pool_ratio) {
  MutexLock l(&mutex_);
  high_pri_pool_ratio_ = high_pri_pool_ratio;
  high_pri_pool_capacity_ = capacity_ * high_pri_pool_ratio_;
  MaintainPoolSize();
}

bool LRUCacheShard::Release(Cache::Handle* handle, bool force_erase) {
  if (handle == nullptr) {
    return false;
  }
  LRUHandle* e = reinterpret_cast<LRUHandle*>(handle);
  bool last_reference = false;
  {
    MutexLock l(&mutex_);
    last_reference = Unref(e);
    if (last_reference) {
      usage_ -= e->charge;
    }
    if (e->refs == 1 && e->InCache()) {
//该项仍在缓存中，没有其他人持有对它的引用
      if (usage_ > capacity_ || force_erase) {
//缓存已满
//由于缓存已满，因此lru列表必须为空。
        assert(!(usage_ > capacity_) || lru_.next == &lru_);
//抓住这个机会并删除该项目
        table_.Remove(e->key(), e->hash);
        e->SetInCache(false);
        Unref(e);
        usage_ -= e->charge;
        last_reference = true;
      } else {
//将该项放在列表中，以便可能释放
        LRU_Insert(e);
      }
    }
  }

//互斥之外的空闲
  if (last_reference) {
    e->Free();
  }
  return last_reference;
}

Status LRUCacheShard::Insert(const Slice& key, uint32_t hash, void* value,
                             size_t charge,
                             void (*deleter)(const Slice& key, void* value),
                             Cache::Handle** handle, Cache::Priority priority) {
//在互斥体外部分配内存
//如果缓存已满，我们将不得不释放它
//但这种情况不应该经常发生。
  LRUHandle* e = reinterpret_cast<LRUHandle*>(
      new char[sizeof(LRUHandle) - 1 + key.size()]);
  Status s;
  autovector<LRUHandle*> last_reference_list;

  e->value = value;
  e->deleter = deleter;
  e->charge = charge;
  e->key_length = key.size();
  e->hash = hash;
  e->refs = (handle == nullptr
                 ? 1
: 2);  //一个来自lrucache，一个用于返回句柄
  e->next = e->prev = nullptr;
  e->SetInCache(true);
  e->SetPriority(priority);
  memcpy(e->key_data, key.data(), key.size());

  {
    MutexLock l(&mutex_);

//按照严格的LRU政策释放空间，直到有足够的空间
//释放或LRU列表为空
    EvictFromLRU(charge, &last_reference_list);

    if (usage_ - lru_usage_ + charge > capacity_ &&
        (strict_capacity_limit_ || handle == nullptr)) {
      if (handle == nullptr) {
//不插入条目但仍返回OK，就像插入条目一样
//进入缓存并立即被逐出。
        last_reference_list.push_back(e);
      } else {
        delete[] reinterpret_cast<char*>(e);
        *handle = nullptr;
        s = Status::Incomplete("Insert failed due to LRU cache being full.");
      }
    } else {
//插入缓存
//请注意，如果不足够，缓存可能会超过其容量。
//空间解放了
      LRUHandle* old = table_.Insert(e);
      usage_ += e->charge;
      if (old != nullptr) {
        old->SetInCache(false);
        if (Unref(old)) {
          usage_ -= old->charge;
//旧的在LRU上，因为它在缓存中并且引用计数
//仅为1（未返回0）
          LRU_Remove(old);
          last_reference_list.push_back(old);
        }
      }
      if (handle == nullptr) {
        LRU_Insert(e);
      } else {
        *handle = reinterpret_cast<Cache::Handle*>(e);
      }
      s = Status::OK();
    }
  }

//我们在mutex之外为
//性能原因
  for (auto entry : last_reference_list) {
    entry->Free();
  }

  return s;
}

void LRUCacheShard::Erase(const Slice& key, uint32_t hash) {
  LRUHandle* e;
  bool last_reference = false;
  {
    MutexLock l(&mutex_);
    e = table_.Remove(key, hash);
    if (e != nullptr) {
      last_reference = Unref(e);
      if (last_reference) {
        usage_ -= e->charge;
      }
      if (last_reference && e->InCache()) {
        LRU_Remove(e);
      }
      e->SetInCache(false);
    }
  }

//mutex不在此处保留
//最后一个参考只有在e的情况下才是真的！= Null PTR
  if (last_reference) {
    e->Free();
  }
}

size_t LRUCacheShard::GetUsage() const {
  MutexLock l(&mutex_);
  return usage_;
}

size_t LRUCacheShard::GetPinnedUsage() const {
  MutexLock l(&mutex_);
  assert(usage_ >= lru_usage_);
  return usage_ - lru_usage_;
}

std::string LRUCacheShard::GetPrintableOptions() const {
  const int kBufferSize = 200;
  char buffer[kBufferSize];
  {
    MutexLock l(&mutex_);
    snprintf(buffer, kBufferSize, "    high_pri_pool_ratio: %.3lf\n",
             high_pri_pool_ratio_);
  }
  return std::string(buffer);
}

LRUCache::LRUCache(size_t capacity, int num_shard_bits,
                   bool strict_capacity_limit, double high_pri_pool_ratio)
    : ShardedCache(capacity, num_shard_bits, strict_capacity_limit) {
  num_shards_ = 1 << num_shard_bits;
  shards_ = new LRUCacheShard[num_shards_];
  SetCapacity(capacity);
  SetStrictCapacityLimit(strict_capacity_limit);
  for (int i = 0; i < num_shards_; i++) {
    shards_[i].SetHighPriorityPoolRatio(high_pri_pool_ratio);
  }
}

LRUCache::~LRUCache() { delete[] shards_; }

CacheShard* LRUCache::GetShard(int shard) {
  return reinterpret_cast<CacheShard*>(&shards_[shard]);
}

const CacheShard* LRUCache::GetShard(int shard) const {
  return reinterpret_cast<CacheShard*>(&shards_[shard]);
}

void* LRUCache::Value(Handle* handle) {
  return reinterpret_cast<const LRUHandle*>(handle)->value;
}

size_t LRUCache::GetCharge(Handle* handle) const {
  return reinterpret_cast<const LRUHandle*>(handle)->charge;
}

uint32_t LRUCache::GetHash(Handle* handle) const {
  return reinterpret_cast<const LRUHandle*>(handle)->hash;
}

void LRUCache::DisownData() {
//如果使用asan编译以抑制泄漏警告，则不要删除数据。
#ifndef __SANITIZE_ADDRESS__
  shards_ = nullptr;
#endif  //！消毒地址
}

size_t LRUCache::TEST_GetLRUSize() {
  size_t lru_size_of_all_shards = 0;
  for (int i = 0; i < num_shards_; i++) {
    lru_size_of_all_shards += shards_[i].TEST_GetLRUSize();
  }
  return lru_size_of_all_shards;
}

std::shared_ptr<Cache> NewLRUCache(size_t capacity, int num_shard_bits,
                                   bool strict_capacity_limit,
                                   double high_pri_pool_ratio) {
  if (num_shard_bits >= 20) {
return nullptr;  //缓存不能分割成太多的碎片
  }
  if (high_pri_pool_ratio < 0.0 || high_pri_pool_ratio > 1.0) {
//无效的高优先级池比率
    return nullptr;
  }
  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<LRUCache>(capacity, num_shard_bits,
                                    strict_capacity_limit, high_pri_pool_ratio);
}

}  //命名空间rocksdb
