
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

#include "cache/clock_cache.h"

#ifndef SUPPORT_CLOCK_CACHE

namespace rocksdb {

std::shared_ptr<Cache> NewClockCache(size_t capacity, int num_shard_bits,
                                     bool strict_capacity_limit) {
//不支持时钟缓存。
  return nullptr;
}

}  //命名空间rocksdb

#else

#include <assert.h>
#include <atomic>
#include <deque>

//如果启用了异常，“tbb/concurrent_hash_map.h”需要rtti。
//禁用它，这样用户可以选择禁用RTTI。
#ifndef ROCKSDB_USE_RTTI
#define TBB_USE_EXCEPTIONS 0
#endif
#include "tbb/concurrent_hash_map.h"

#include "cache/sharded_cache.h"
#include "port/port.h"
#include "util/autovector.h"
#include "util/mutexlock.h"

namespace rocksdb {

namespace {

//基于时钟算法的高速缓存接口的实现
//并发性能优于lrucache。时钟算法的思想
//在循环列表和迭代器中维护所有缓存项
//（以下简称“头部”）指的是最后一个检查过的条目。逐出开始于
//电流头。每一个条目在被驱逐之前都有第二次机会，如果它
//自上次检查以来已被访问。与LRU相比，没有修改
//到内部数据结构（翻转使用位除外）的需求
//在查找时完成。这给了我们实现缓存的机会
//具有更好的并发性。
//
//每个缓存条目由一个缓存句柄和所有句柄表示
//如上文所述，排列成循环列表。删除条目后，
//我们从不取下把手。相反，把手被放进回收站
//重新使用。这是为了避免内存释放，这是很难处理的
//在并发环境中。
//
//缓存还为查找维护一个并发哈希映射。任何并发
//哈希图实现应该完成这项工作。我们目前使用
//tbb:：concurrent_hash_映射，因为它支持并发擦除。
//
//每个缓存句柄都有以下标志和计数器，它们被挤压
//在原子集成电路中，确保手柄始终在
//状态：
//
//*缓存中位：条目是否被缓存本身引用。如果
//一个条目在缓存中，它的键也将在哈希图中可用。
//*使用位：用户自上次访问以来是否访问过该条目。
//检查是否驱逐。可以通过逐出重置。
//*引用计数：用户引用计数。
//
//一个条目只有在缓存中时才能被引用。可以收回条目
//只有当它在缓存中，自上次检查以来没有使用，并且引用
//计数是零。
//
//下图显示了缓存的可能布局。框表示
//缓存位、使用位和
//分别引用计数。
//
//散列图：
//+------+------+
//键手柄
//+------+------+
//“foo”5-------------------------------------+
//+------+------++
//“巴”2-+
//+------+------+
//_
//头部
//|  |                                  |
//循环列表：
//+——+++————++——++——++——++——++——++——++——++——++—————
//（0,0,0）---（1,1,0）---（0,0,0）---（0,1,3）---（1,0,0）---…
//+——+++————++——++——++——++——++——++——++——++——++—————
//_
//+------++------+
//γ
//+--+--+
//回收站：1 3_
//+--+--+
//
//假设我们在此时尝试将“baz”插入到缓存中，缓存是
//满的。缓存将首先查找要逐出的条目，从
//头指向（第二个入口）。它重置第二个条目的使用位，
//跳过第三个和第四个条目，因为它们不在缓存中，最后
//逐出第五个条目（“foo”）。它在回收站寻找可用的把手，
//抓住把手3，将钥匙插入把手。下图
//显示结果布局。
//
//散列图：
//+------+------+
//键手柄
//+------+------+
//“BAZ”3------------+
//+------+------++
//“巴”2-+
//+------+------+
//γ
//头部
//|          |                                   |
//循环列表：
//+——+++————++——++——++——++——++——++——++——++——++—————
//（0,0,0）---（1,0,0）---（1,0,0）---（0,1,3）---（0,0,0）---…
//+——+++————++——++——++——++——++——++——++——++——++—————
//_
//+------++-------------------------------+
//γ
//+--+--+
//回收站：1 5_
//+--+--+
//
//全局互斥保护循环列表、头部和回收站。
//我们还要求修改哈希映射需要保持互斥量。
//因此，修改缓存（例如insert（）和erase（））需要
//保持静音。lookup（）仅访问哈希映射和关联的标志
//对于每个句柄，不需要显式锁定。release（）必须
//只有当互斥体释放对该项的最后一个引用并且
//该条目已从缓存中显式删除。未来的改进可以
//完全移除互斥体。
//
//基准：
//我们在13GB大小的测试数据库上运行readrandom数据库工作台，每个数据库的大小为
//水平：
//
//级别文件大小（MB）
//---------------------
//10 1 0.01年
//l1 18 17.32级
//L2 230 182.94级
//L3 1186 1833.63标准
//L4 4602 8140.30型
//
//我们使用32和16个读线程进行测试，缓存大小为2GB（整个数据库
//不适合）和64GB缓存大小（整个数据库可以容纳在缓存中），以及
//是否在块缓存中放置索引块和筛选块。基准运行
//具有
//带RocksDB 4.10。我们得到了以下结果：
//
//线程缓存缓存时钟缓存lrucache
//大小索引/筛选器吞吐量（MB/s）命中吞吐量（MB/s）
//32 2GB是466.7 85.9%433.7 86.5%
//32 2GB编号529.9 72.7%532.7 73.9%
//32 64GB是649.9 99.9%507.9 99.9%
//32 64GB编号740.4 99.9%662.8 99.9%
//16 2GB是278.4 85.9%283.4 86.5%
//16 2GB编号318.6 72.7%335.8 73.9%
//16 64GB是391.9 99.9%353.3 99.9%
//16 64GB编号433.8 99.8%419.4 99.8%

//缓存条目元数据。
struct CacheHandle {
  Slice key;
  uint32_t hash;
  void* value;
  size_t charge;
  void (*deleter)(const Slice&, void* value);

//与缓存句柄关联的标志和计数器：
//最低位：n-缓存位
//第二最低位：使用位
//其余位：参考计数
//当标志等于0时，不使用句柄。线程减少了计数
//到0负责将句柄放回回收和清理内存。
  std::atomic<uint32_t> flags;

  CacheHandle() = default;

  CacheHandle(const CacheHandle& a) { *this = a; }

  CacheHandle(const Slice& k, void* v,
              void (*del)(const Slice& key, void* value))
      : key(k), value(v), deleter(del) {}

  CacheHandle& operator=(const CacheHandle& a) {
//仅复制删除所需的成员。
    key = a.key;
    value = a.value;
    deleter = a.deleter;
    return *this;
  }
};

//哈希图的键。为了方便起见，我们使用键存储哈希值。
struct CacheKey {
  Slice key;
  uint32_t hash_value;

  CacheKey() = default;

  CacheKey(const Slice& k, uint32_t h) {
    key = k;
    hash_value = h;
  }

  static bool equal(const CacheKey& a, const CacheKey& b) {
    return a.hash_value == b.hash_value && a.key == b.key;
  }

  static size_t hash(const CacheKey& a) {
    return static_cast<size_t>(a.hash_value);
  }
};

struct CleanupContext {
//要删除的值列表，以及键和删除器。
  autovector<CacheHandle> to_delete_value;

//要删除的密钥列表。
  autovector<const char*> to_delete_key;
};

//一种保持自身时钟缓存的缓存碎片。
class ClockCacheShard : public CacheShard {
 public:
//散列映射类型。
  typedef tbb::concurrent_hash_map<CacheKey, CacheHandle*, CacheKey> HashTable;

  ClockCacheShard();
  ~ClockCacheShard();

//界面
  virtual void SetCapacity(size_t capacity) override;
  virtual void SetStrictCapacityLimit(bool strict_capacity_limit) override;
  virtual Status Insert(const Slice& key, uint32_t hash, void* value,
                        size_t charge,
                        void (*deleter)(const Slice& key, void* value),
                        Cache::Handle** handle,
                        Cache::Priority priority) override;
  virtual Cache::Handle* Lookup(const Slice& key, uint32_t hash) override;
//如果缓存中的条目，请增加引用计数并返回true。
//否则返回false。
//
//在被调用之前不需要保持互斥体。
  virtual bool Ref(Cache::Handle* handle) override;
  virtual bool Release(Cache::Handle* handle,
                       bool force_erase = false) override;
  virtual void Erase(const Slice& key, uint32_t hash) override;
  bool EraseAndConfirm(const Slice& key, uint32_t hash,
                       CleanupContext* context);
  virtual size_t GetUsage() const override;
  virtual size_t GetPinnedUsage() const override;
  virtual void EraseUnRefEntries() override;
  virtual void ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                      bool thread_safe) override;

 private:
  static const uint32_t kInCacheBit = 1;
  static const uint32_t kUsageBit = 2;
  static const uint32_t kRefsOffset = 2;
  static const uint32_t kOneRef = 1 << kRefsOffset;

//用于提取缓存句柄标志和计数器的帮助程序函数。
  static bool InCache(uint32_t flags) { return flags & kInCacheBit; }
  static bool HasUsage(uint32_t flags) { return flags & kUsageBit; }
  static uint32_t CountRefs(uint32_t flags) { return flags >> kRefsOffset; }

//减少条目的引用计数。如果这将计数减少到0，
//回收条目。如果set_usage为真，也设置usage位。
//
//如果值被擦除，则返回true。
//
//在被调用之前不需要保持互斥体。
  bool Unref(CacheHandle* handle, bool set_usage, CleanupContext* context);

//取消设置项的缓存位。必要时回收手柄。
//
//如果值被擦除，则返回true。
//
//在被调用之前必须保持互斥。
  bool UnsetInCache(CacheHandle* handle, CleanupContext* context);

//将句柄放回回收列表，并将与
//进入待删除列表。它不会像清理钥匙那样清理钥匙
//由另一个手柄重复使用。
//
//在被调用之前必须保持互斥。
  void RecycleHandle(CacheHandle* handle, CleanupContext* context);

//删除待删除列表中的键和值。调用方法时不使用
//保持互斥，因为析构函数可能很昂贵。
  void Cleanup(const CleanupContext& context);

//检查手柄是否收回。如果句柄在缓存中，则使用位为
//未设置，引用计数为0，将其从缓存中逐出。否则未设置
//使用位。
//
//在被调用之前必须保持互斥。
  bool TryEvict(CacheHandle* value, CleanupContext* context);

//扫描循环列表，逐出条目，直到获得足够的容量
//用于特定大小的新缓存项。如果成功返回true，则返回false
//否则。
//
//在被调用之前必须保持互斥。
  bool EvictFromCache(size_t charge, CleanupContext* context);

  CacheHandle* Insert(const Slice& key, uint32_t hash, void* value,
                      size_t change,
                      void (*deleter)(const Slice& key, void* value),
                      bool hold_reference, CleanupContext* context);

//警卫名单，头颅和回收。此外，更新表也有
//保存互斥体，以避免缓存处于不一致状态。
  mutable port::Mutex mutex_;

//缓存句柄的循环列表。最初列表是空的。曾经一次
//插入时需要句柄，中没有其他句柄可用
//回收站，还有一个手柄附加在末尾。
//
//我们使用std:：deque作为循环列表，因为我们要确保
//指向句柄的指针在缓存的整个生命周期内都有效
//（与std:：vector相反），并且能够增加列表（与之相反
//静态分配的数组）。
  std::deque<CacheHandle> list_;

//指向循环列表中要检查的下一个句柄的指针
//驱逐。
  size_t head_;

//缓存句柄的回收站。
  autovector<CacheHandle*> recycle_;

//最大缓存大小。
  std::atomic<size_t> capacity_;

//缓存的当前总大小。
  std::atomic<size_t> usage_;

//未释放的缓存总大小。
  std::atomic<size_t> pinned_usage_;

//如果缓存已满，是否允许插入缓存。
  std::atomic<bool> strict_capacity_limit_;

//用于查找的哈希表（tbb:：concurrent_hash_map）。
  HashTable table_;
};

ClockCacheShard::ClockCacheShard()
    : head_(0), usage_(0), pinned_usage_(0), strict_capacity_limit_(false) {}

ClockCacheShard::~ClockCacheShard() {
  for (auto& handle : list_) {
    uint32_t flags = handle.flags.load(std::memory_order_relaxed);
    if (InCache(flags) || CountRefs(flags) > 0) {
      (*handle.deleter)(handle.key, handle.value);
      delete[] handle.key.data();
    }
  }
}

size_t ClockCacheShard::GetUsage() const {
  return usage_.load(std::memory_order_relaxed);
}

size_t ClockCacheShard::GetPinnedUsage() const {
  return pinned_usage_.load(std::memory_order_relaxed);
}

void ClockCacheShard::ApplyToAllCacheEntries(void (*callback)(void*, size_t),
                                             bool thread_safe) {
  if (thread_safe) {
    mutex_.Lock();
  }
  for (auto& handle : list_) {
//使用宽松的语义而不是获取语义，因为我们
//保持互斥，或者没有线程安全要求。
    uint32_t flags = handle.flags.load(std::memory_order_relaxed);
    if (InCache(flags)) {
      callback(handle.value, handle.charge);
    }
  }
  if (thread_safe) {
    mutex_.Unlock();
  }
}

void ClockCacheShard::RecycleHandle(CacheHandle* handle,
                                    CleanupContext* context) {
  mutex_.AssertHeld();
  assert(!InCache(handle->flags) && CountRefs(handle->flags) == 0);
  context->to_delete_key.push_back(handle->key.data());
  context->to_delete_value.emplace_back(*handle);
  handle->key.clear();
  handle->value = nullptr;
  handle->deleter = nullptr;
  recycle_.push_back(handle);
  usage_.fetch_sub(handle->charge, std::memory_order_relaxed);
}

void ClockCacheShard::Cleanup(const CleanupContext& context) {
  for (const CacheHandle& handle : context.to_delete_value) {
    if (handle.deleter) {
      (*handle.deleter)(handle.key, handle.value);
    }
  }
  for (const char* key : context.to_delete_key) {
    delete[] key;
  }
}

bool ClockCacheShard::Ref(Cache::Handle* h) {
  auto handle = reinterpret_cast<CacheHandle*>(h);
//CAS循环以增加引用计数。
  uint32_t flags = handle->flags.load(std::memory_order_relaxed);
  while (InCache(flags)) {
//成功时使用获取语义，作为缓存上的进一步操作
//在引用计数增加后，必须对条目进行排序。
    if (handle->flags.compare_exchange_weak(flags, flags + kOneRef,
                                            std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
      if (CountRefs(flags) == 0) {
//操作前没有引用计数。
        pinned_usage_.fetch_add(handle->charge, std::memory_order_relaxed);
      }
      return true;
    }
  }
  return false;
}

bool ClockCacheShard::Unref(CacheHandle* handle, bool set_usage,
                            CleanupContext* context) {
  if (set_usage) {
    handle->flags.fetch_or(kUsageBit, std::memory_order_relaxed);
  }
//将获取发布语义用作缓存项上的前一个操作
//在减少引用计数和可能的清除之前必须排序
//必须在之后订购。
  uint32_t flags = handle->flags.fetch_sub(kOneRef, std::memory_order_acq_rel);
  assert(CountRefs(flags) > 0);
  if (CountRefs(flags) == 1) {
//这是最后一次引用。
    pinned_usage_.fetch_sub(handle->charge, std::memory_order_relaxed);
//如果是最后一个引用，则清除。
    if (!InCache(flags)) {
      MutexLock l(&mutex_);
      RecycleHandle(handle, context);
    }
  }
  return context->to_delete_value.size();
}

bool ClockCacheShard::UnsetInCache(CacheHandle* handle,
                                   CleanupContext* context) {
  mutex_.AssertHeld();
//将获取发布语义用作缓存项上的前一个操作
//在减少引用计数和可能的清除之前必须排序
//必须在之后订购。
  uint32_t flags =
      handle->flags.fetch_and(~kInCacheBit, std::memory_order_acq_rel);
//如果是最后一个引用，则清除。
  if (InCache(flags) && CountRefs(flags) == 0) {
    RecycleHandle(handle, context);
  }
  return context->to_delete_value.size();
}

bool ClockCacheShard::TryEvict(CacheHandle* handle, CleanupContext* context) {
  mutex_.AssertHeld();
  uint32_t flags = kInCacheBit;
  if (handle->flags.compare_exchange_strong(flags, 0, std::memory_order_acquire,
                                            std::memory_order_relaxed)) {
    bool erased __attribute__((__unused__)) =
        table_.erase(CacheKey(handle->key, handle->hash));
    assert(erased);
    RecycleHandle(handle, context);
    return true;
  }
  handle->flags.fetch_and(~kUsageBit, std::memory_order_relaxed);
  return false;
}

bool ClockCacheShard::EvictFromCache(size_t charge, CleanupContext* context) {
  size_t usage = usage_.load(std::memory_order_relaxed);
  size_t capacity = capacity_.load(std::memory_order_relaxed);
  if (usage == 0) {
    return charge <= capacity;
  }
  size_t new_head = head_;
  bool second_iteration = false;
  while (usage + charge > capacity) {
    assert(new_head < list_.size());
    if (TryEvict(&list_[new_head], context)) {
      usage = usage_.load(std::memory_order_relaxed);
    }
    new_head = (new_head + 1 >= list_.size()) ? 0 : new_head + 1;
    if (new_head == head_) {
      if (second_iteration) {
        return false;
      } else {
        second_iteration = true;
      }
    }
  }
  head_ = new_head;
  return true;
}

void ClockCacheShard::SetCapacity(size_t capacity) {
  CleanupContext context;
  {
    MutexLock l(&mutex_);
    capacity_.store(capacity, std::memory_order_relaxed);
    EvictFromCache(0, &context);
  }
  Cleanup(context);
}

void ClockCacheShard::SetStrictCapacityLimit(bool strict_capacity_limit) {
  strict_capacity_limit_.store(strict_capacity_limit,
                               std::memory_order_relaxed);
}

CacheHandle* ClockCacheShard::Insert(
    const Slice& key, uint32_t hash, void* value, size_t charge,
    void (*deleter)(const Slice& key, void* value), bool hold_reference,
    CleanupContext* context) {
  MutexLock l(&mutex_);
  bool success = EvictFromCache(charge, context);
  bool strict = strict_capacity_limit_.load(std::memory_order_relaxed);
  if (!success && (strict || !hold_reference)) {
    context->to_delete_key.push_back(key.data());
    if (!hold_reference) {
      context->to_delete_value.emplace_back(key, value, deleter);
    }
    return nullptr;
  }
//从回收站抓取可用手柄。如果回收站为空，请创建
//并将新的句柄附加到循环列表的末尾。
  CacheHandle* handle = nullptr;
  if (!recycle_.empty()) {
    handle = recycle_.back();
    recycle_.pop_back();
  } else {
    list_.emplace_back();
    handle = &list_.back();
  }
//填充手柄。
  handle->key = key;
  handle->hash = hash;
  handle->value = value;
  handle->charge = charge;
  handle->deleter = deleter;
  uint32_t flags = hold_reference ? kInCacheBit + kOneRef : kInCacheBit;
  handle->flags.store(flags, std::memory_order_relaxed);
  HashTable::accessor accessor;
  if (table_.find(accessor, CacheKey(key, hash))) {
    CacheHandle* existing_handle = accessor->second;
    table_.erase(accessor);
    UnsetInCache(existing_handle, context);
  }
  table_.insert(HashTable::value_type(CacheKey(key, hash), handle));
  if (hold_reference) {
    pinned_usage_.fetch_add(charge, std::memory_order_relaxed);
  }
  usage_.fetch_add(charge, std::memory_order_relaxed);
  return handle;
}

Status ClockCacheShard::Insert(const Slice& key, uint32_t hash, void* value,
                               size_t charge,
                               void (*deleter)(const Slice& key, void* value),
                               Cache::Handle** out_handle,
                               Cache::Priority priority) {
  CleanupContext context;
  HashTable::accessor accessor;
  char* key_data = new char[key.size()];
  memcpy(key_data, key.data(), key.size());
  Slice key_copy(key_data, key.size());
  CacheHandle* handle = Insert(key_copy, hash, value, charge, deleter,
                               out_handle != nullptr, &context);
  Status s;
  if (out_handle != nullptr) {
    if (handle == nullptr) {
      s = Status::Incomplete("Insert failed due to LRU cache being full.");
    } else {
      *out_handle = reinterpret_cast<Cache::Handle*>(handle);
    }
  }
  Cleanup(context);
  return s;
}

Cache::Handle* ClockCacheShard::Lookup(const Slice& key, uint32_t hash) {
  HashTable::const_accessor accessor;
  if (!table_.find(accessor, CacheKey(key, hash))) {
    return nullptr;
  }
  CacheHandle* handle = accessor->second;
  accessor.release();
//如果另一个线程潜入并逐出/清除缓存，则ref（）可能会失败。
//在我们能够保存引用之前输入。
  if (!Ref(reinterpret_cast<Cache::Handle*>(handle))) {
    return nullptr;
  }
//再次检查该键，因为该句柄现在可能表示另一个键
//如果其他线程潜入，则逐出/删除条目并重新使用句柄
//另一个缓存项。
  if (hash != handle->hash || key != handle->key) {
    CleanupContext context;
    Unref(handle, false, &context);
//unref（）可能会删除条目，因此我们需要清理。
    Cleanup(context);
    return nullptr;
  }
  return reinterpret_cast<Cache::Handle*>(handle);
}

bool ClockCacheShard::Release(Cache::Handle* h, bool force_erase) {
  CleanupContext context;
  CacheHandle* handle = reinterpret_cast<CacheHandle*>(h);
  bool erased = Unref(handle, true, &context);
  if (force_erase && !erased) {
    erased = EraseAndConfirm(handle->key, handle->hash, &context);
  }
  Cleanup(context);
  return erased;
}

void ClockCacheShard::Erase(const Slice& key, uint32_t hash) {
  CleanupContext context;
  EraseAndConfirm(key, hash, &context);
  Cleanup(context);
}

bool ClockCacheShard::EraseAndConfirm(const Slice& key, uint32_t hash,
                                      CleanupContext* context) {
  MutexLock l(&mutex_);
  HashTable::accessor accessor;
  bool erased = false;
  if (table_.find(accessor, CacheKey(key, hash))) {
    CacheHandle* handle = accessor->second;
    table_.erase(accessor);
    erased = UnsetInCache(handle, context);
  }
  return erased;
}

void ClockCacheShard::EraseUnRefEntries() {
  CleanupContext context;
  {
    MutexLock l(&mutex_);
    table_.clear();
    for (auto& handle : list_) {
      UnsetInCache(&handle, &context);
    }
  }
  Cleanup(context);
}

class ClockCache : public ShardedCache {
 public:
  ClockCache(size_t capacity, int num_shard_bits, bool strict_capacity_limit)
      : ShardedCache(capacity, num_shard_bits, strict_capacity_limit) {
    int num_shards = 1 << num_shard_bits;
    shards_ = new ClockCacheShard[num_shards];
    SetCapacity(capacity);
    SetStrictCapacityLimit(strict_capacity_limit);
  }

  virtual ~ClockCache() { delete[] shards_; }

  virtual const char* Name() const override { return "ClockCache"; }

  virtual CacheShard* GetShard(int shard) override {
    return reinterpret_cast<CacheShard*>(&shards_[shard]);
  }

  virtual const CacheShard* GetShard(int shard) const override {
    return reinterpret_cast<CacheShard*>(&shards_[shard]);
  }

  virtual void* Value(Handle* handle) override {
    return reinterpret_cast<const CacheHandle*>(handle)->value;
  }

  virtual size_t GetCharge(Handle* handle) const override {
    return reinterpret_cast<const CacheHandle*>(handle)->charge;
  }

  virtual uint32_t GetHash(Handle* handle) const override {
    return reinterpret_cast<const CacheHandle*>(handle)->hash;
  }

  virtual void DisownData() override { shards_ = nullptr; }

 private:
  ClockCacheShard* shards_;
};

}  //结束匿名命名空间

std::shared_ptr<Cache> NewClockCache(size_t capacity, int num_shard_bits,
                                     bool strict_capacity_limit) {
  if (num_shard_bits < 0) {
    num_shard_bits = GetDefaultCacheShardBits(capacity);
  }
  return std::make_shared<ClockCache>(capacity, num_shard_bits,
                                      strict_capacity_limit);
}

}  //命名空间rocksdb

#endif  //支持时钟缓存
