
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
#pragma once

#include <string>

#include "cache/sharded_cache.h"

#include "port/port.h"
#include "util/autovector.h"

namespace rocksdb {

//LRU缓存实现

//条目是一个可变长度的堆分配结构。
//缓存和/或任何外部实体引用条目。
//缓存将其所有条目保存在表中。一些元素
//也存储在LRU列表中。
//
//lruhandle可以处于以下状态：
//1。在哈希表中和外部引用。
//在这种情况下，条目*不是*在LRU中。（refs>1&&在缓存中==true）
//2。未在外部和哈希表中引用。在这种情况下，入口是
//在LRU中，可以释放。（refs==1&&in_cache==true）
//三。外部引用，但不在哈希表中。在这种情况下，入口是
//不在LRU上也不在表中。（refs>=1&&在缓存中==false）
//
//所有新创建的lruhandles都处于状态1。如果你打电话
//lrucacheshard：：释放
//进入状态1时，它将进入状态2。从状态1移动到
//状态3，可以使用调用lrucacheshard:：erase或lrucacheshard:：insert
//同样的钥匙。
//要从状态2移动到状态1，请使用lrucacheshard:：lookup。
//在销毁之前，请确保没有句柄处于状态1。这意味着
//任何成功的lrucacheshard:：lookup/lrucacheshard:：insert都有一个
//匹配
//rucache:：release（移动到状态2）或lrucacheshard:：erase（状态3）

struct LRUHandle {
  void* value;
  void (*deleter)(const Slice&, void* value);
  LRUHandle* next_hash;
  LRUHandle* next;
  LRUHandle* prev;
size_t charge;  //TODO（opt）：只允许uint32？
  size_t key_length;
uint32_t refs;     //此条目的引用数
//缓存本身被计为1

//包括以下标志：
//在缓存中：哈希表是否引用此条目。
//高优先级：此条目是否为高优先级条目。
//在高优先级池中：此条目是否在高优先级池中。
  char flags;

uint32_t hash;     //key（）的哈希；用于快速切分和比较

char key_data[1];  //键的开头

  Slice key() const {
//为了更便宜的查找，我们允许使用临时句柄对象
//将指向键的指针存储在“value”中。
    if (next == this) {
      return *(reinterpret_cast<Slice*>(value));
    } else {
      return Slice(key_data, key_length);
    }
  }

  bool InCache() { return flags & 1; }
  bool IsHighPri() { return flags & 2; }
  bool InHighPriPool() { return flags & 4; }

  void SetInCache(bool in_cache) {
    if (in_cache) {
      flags |= 1;
    } else {
      flags &= ~1;
    }
  }

  void SetPriority(Cache::Priority priority) {
    if (priority == Cache::Priority::HIGH) {
      flags |= 2;
    } else {
      flags &= ~2;
    }
  }

  void SetInHighPriPool(bool in_high_pri_pool) {
    if (in_high_pri_pool) {
      flags |= 4;
    } else {
      flags &= ~4;
    }
  }

  void Free() {
    assert((refs == 1 && InCache()) || (refs == 0 && !InCache()));
    if (deleter) {
      (*deleter)(key(), value);
    }
    delete[] reinterpret_cast<char*>(this);
  }
};

//我们提供了我们自己的简单哈希表，因为它删除了大量
//移植黑客，也比一些内置哈希更快
//一些编译器/运行时组合中的表实现
//我们已经测试过了。例如，ReadRandom的速度比G++快了约5%
//4.4.3的内置哈希表。
class LRUHandleTable {
 public:
  LRUHandleTable();
  ~LRUHandleTable();

  LRUHandle* Lookup(const Slice& key, uint32_t hash);
  LRUHandle* Insert(LRUHandle* h);
  LRUHandle* Remove(const Slice& key, uint32_t hash);

  template <typename T>
  void ApplyToAllCacheEntries(T func) {
    for (uint32_t i = 0; i < length_; i++) {
      LRUHandle* h = list_[i];
      while (h != nullptr) {
        auto n = h->next_hash;
        assert(h->InCache());
        func(h);
        h = n;
      }
    }
  }

 private:
//返回指向指向缓存项的插槽的指针，
//匹配键/哈希。如果没有此类缓存项，则返回
//指向相应链接列表中尾随插槽的指针。
  LRUHandle** FindPointer(const Slice& key, uint32_t hash);

  void Resize();

//该表由每个存储桶所在的存储桶数组组成。
//散列到存储桶中的缓存项的链接列表。
  LRUHandle** list_;
  uint32_t length_;
  uint32_t elems_;
};

//一个碎片缓存。
class ALIGN_AS(CACHE_LINE_SIZE) LRUCacheShard : public CacheShard {
 public:
  LRUCacheShard();
  virtual ~LRUCacheShard();

//与构造函数分离，以便调用方可以轻松地创建lrucache数组
//如果当前使用量大于新容量，函数将尝试
//释放所需空间
  virtual void SetCapacity(size_t capacity) override;

//如果缓存已满，请将该标志设置为拒绝插入。
  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) override;

//设置为高优先级缓存项保留的容量百分比。
  void SetHighPriorityPoolRatio(double high_pri_pool_ratio);

//类似于缓存方法，但带有额外的“hash”参数。
  virtual Status Insert(const Slice& key, uint32_t hash, void* value,
                        size_t charge,
                        void (*deleter)(const Slice& key, void* value),
                        Cache::Handle** handle,
                        Cache::Priority priority) override;
  virtual Cache::Handle* Lookup(const Slice& key, uint32_t hash) override;
  virtual bool Ref(Cache::Handle* handle) override;
  virtual bool Release(Cache::Handle* handle,
                       bool force_erase = false) override;
  virtual void Erase(const Slice& key, uint32_t hash) override;

//尽管在某些平台中，大小更新是原子的，以确保
//getusage（）和getpinedusage（）在任何平台下都能正常工作，我们将
//用mutex保护它们。

  virtual size_t GetUsage() const override;
  virtual size_t GetPinnedUsage() const override;

  virtual void ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                      bool thread_safe) override;

  virtual void EraseUnRefEntries() override;

  virtual std::string GetPrintableOptions() const override;

  void TEST_GetLRUList(LRUHandle** lru, LRUHandle** lru_low_pri);

//检索LRU中的元素数，仅用于单元测试
//不安全
  size_t TEST_GetLRUSize();

//重载以将其与缓存线大小对齐
  void* operator new(size_t);

  void* operator new[](size_t);

  void operator delete(void *);

  void operator delete[](void*);

 private:
  void LRU_Remove(LRUHandle* e);
  void LRU_Insert(LRUHandle* e);

//将高优先级池中的最后一个条目溢出到低优先级池，直到大小为
//高优先级池不大于高优先级池PCT指定的大小。
  void MaintainPoolSize();

//只需将引用计数减少1。
//如果最后一个引用返回true
  bool Unref(LRUHandle* e);

//按照严格的LRU政策释放一些空间，直到有足够的空间
//保留（使用费）已释放或LRU列表为空
//此函数不是线程安全的-需要在
//保持静音\u
  void EvictFromLRU(size_t charge, autovector<LRUHandle*>* deleted);

//使用前已初始化。
  size_t capacity_;

//高优先级池中条目的内存大小。
  size_t high_pri_pool_usage_;

//如果缓存达到最大容量，是否拒绝插入。
  bool strict_capacity_limit_;

//为高优先级缓存项保留的容量比率。
  double high_pri_pool_ratio_;

//高优先级池大小，等于容量*高优先级池比率。
//记住该值以避免每次重新计算。
  double high_pri_pool_capacity_;

//LRU列表的虚拟头。
//lru.prev是最新的条目，lru.next是最旧的条目。
//lru包含可收回的项，即仅由缓存引用
  LRUHandle lru_;

//指向LRU列表中低优先级池头的指针。
  LRUHandle* lru_low_pri_;

//------------
//不经常修改的数据成员
//—————————————————————————
//
//我们将频繁更新的数据成员与
//不经常更新以便它们不共享同一缓存线
//这将导致错误的缓存共享
//
//—————————————————————————
//经常修改的数据成员
//--------VVVVVVVVVV-------
  LRUHandleTable table_;

//缓存中条目的内存大小
  size_t usage_;

//仅驻留在LRU列表中的条目的内存大小
  size_t lru_usage_;

//互斥保护以下状态。
//我们不把mutex_u算作缓存的内部状态，所以在语义上我们
//不要介意mutex调用非常量操作。
  mutable port::Mutex mutex_;
};

class LRUCache : public ShardedCache {
 public:
  LRUCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit,
           double high_pri_pool_ratio);
  virtual ~LRUCache();
  virtual const char* Name() const override { return "LRUCache"; }
  virtual CacheShard* GetShard(int shard) override;
  virtual const CacheShard* GetShard(int shard) const override;
  virtual void* Value(Handle* handle) override;
  virtual size_t GetCharge(Handle* handle) const override;
  virtual uint32_t GetHash(Handle* handle) const override;
  virtual void DisownData() override;

//检索LRU中的元素数，仅用于单元测试
  size_t TEST_GetLRUSize();

 private:
  LRUCacheShard* shards_;
  int num_shards_ = 0;
};

}  //命名空间rocksdb
