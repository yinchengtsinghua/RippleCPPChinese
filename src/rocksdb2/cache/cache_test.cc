
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

#include "rocksdb/cache.h"

#include <forward_list>
#include <functional>
#include <iostream>
#include <string>
#include <vector>
#include "cache/clock_cache.h"
#include "cache/lru_cache.h"
#include "util/coding.h"
#include "util/string_util.h"
#include "util/testharness.h"

namespace rocksdb {

//数字键/值与缓存所需类型之间的转换。
static std::string EncodeKey(int k) {
  std::string result;
  PutFixed32(&result, k);
  return result;
}
static int DecodeKey(const Slice& k) {
  assert(k.size() == 4);
  return DecodeFixed32(k.data());
}
static void* EncodeValue(uintptr_t v) { return reinterpret_cast<void*>(v); }
static int DecodeValue(void* v) {
  return static_cast<int>(reinterpret_cast<uintptr_t>(v));
}

const std::string kLRU = "lru";
const std::string kClock = "clock";

void dumbDeleter(const Slice& key, void* value) {}

void eraseDeleter(const Slice& key, void* value) {
  Cache* cache = reinterpret_cast<Cache*>(value);
  cache->Erase("foo");
}

class CacheTest : public testing::TestWithParam<std::string> {
 public:
  static CacheTest* current_;

  static void Deleter(const Slice& key, void* v) {
    current_->deleted_keys_.push_back(DecodeKey(key));
    current_->deleted_values_.push_back(DecodeValue(v));
  }

  static const int kCacheSize = 1000;
  static const int kNumShardBits = 4;

  static const int kCacheSize2 = 100;
  static const int kNumShardBits2 = 2;

  std::vector<int> deleted_keys_;
  std::vector<int> deleted_values_;
  shared_ptr<Cache> cache_;
  shared_ptr<Cache> cache2_;

  CacheTest()
      : cache_(NewCache(kCacheSize, kNumShardBits, false)),
        cache2_(NewCache(kCacheSize2, kNumShardBits2, false)) {
    current_ = this;
  }

  ~CacheTest() {
  }

  std::shared_ptr<Cache> NewCache(size_t capacity) {
    auto type = GetParam();
    if (type == kLRU) {
      return NewLRUCache(capacity);
    }
    if (type == kClock) {
      return NewClockCache(capacity);
    }
    return nullptr;
  }

  std::shared_ptr<Cache> NewCache(size_t capacity, int num_shard_bits,
                                  bool strict_capacity_limit) {
    auto type = GetParam();
    if (type == kLRU) {
      return NewLRUCache(capacity, num_shard_bits, strict_capacity_limit);
    }
    if (type == kClock) {
      return NewClockCache(capacity, num_shard_bits, strict_capacity_limit);
    }
    return nullptr;
  }

  int Lookup(shared_ptr<Cache> cache, int key) {
    Cache::Handle* handle = cache->Lookup(EncodeKey(key));
    const int r = (handle == nullptr) ? -1 : DecodeValue(cache->Value(handle));
    if (handle != nullptr) {
      cache->Release(handle);
    }
    return r;
  }

  void Insert(shared_ptr<Cache> cache, int key, int value, int charge = 1) {
    cache->Insert(EncodeKey(key), EncodeValue(value), charge,
                  &CacheTest::Deleter);
  }

  void Erase(shared_ptr<Cache> cache, int key) {
    cache->Erase(EncodeKey(key));
  }


  int Lookup(int key) {
    return Lookup(cache_, key);
  }

  void Insert(int key, int value, int charge = 1) {
    Insert(cache_, key, value, charge);
  }

  void Erase(int key) {
    Erase(cache_, key);
  }

  int Lookup2(int key) {
    return Lookup(cache2_, key);
  }

  void Insert2(int key, int value, int charge = 1) {
    Insert(cache2_, key, value, charge);
  }

  void Erase2(int key) {
    Erase(cache2_, key);
  }
};
CacheTest* CacheTest::current_;

TEST_P(CacheTest, UsageTest) {
//缓存是共享的，将自动清除。
  const uint64_t kCapacity = 100000;
  auto cache = NewCache(kCapacity, 8, false);

  size_t usage = 0;
  char value[10] = "abcdef";
//确保所有内容都将被缓存
  for (int i = 1; i < 100; ++i) {
    std::string key(i, 'a');
    auto kv_size = key.size() + 5;
    cache->Insert(key, reinterpret_cast<void*>(value), kv_size, dumbDeleter);
    usage += kv_size;
    ASSERT_EQ(usage, cache->GetUsage());
  }

//确保缓存将过载
  for (uint64_t i = 1; i < kCapacity; ++i) {
    auto key = ToString(i);
    cache->Insert(key, reinterpret_cast<void*>(value), key.size() + 5,
                  dumbDeleter);
  }

//使用量应接近容量
  ASSERT_GT(kCapacity, cache->GetUsage());
  ASSERT_LT(kCapacity * 0.95, cache->GetUsage());
}

TEST_P(CacheTest, PinnedUsageTest) {
//缓存是共享的，将自动清除。
  const uint64_t kCapacity = 100000;
  auto cache = NewCache(kCapacity, 8, false);

  size_t pinned_usage = 0;
  char value[10] = "abcdef";

  std::forward_list<Cache::Handle*> unreleased_handles;

//添加条目。插入后将其中的一些解锁。然后，钉上一些
//再一次。选中GetPinedUsage（）。
  for (int i = 1; i < 100; ++i) {
    std::string key(i, 'a');
    auto kv_size = key.size() + 5;
    Cache::Handle* handle;
    cache->Insert(key, reinterpret_cast<void*>(value), kv_size, dumbDeleter,
                  &handle);
    pinned_usage += kv_size;
    ASSERT_EQ(pinned_usage, cache->GetPinnedUsage());
    if (i % 2 == 0) {
      cache->Release(handle);
      pinned_usage -= kv_size;
      ASSERT_EQ(pinned_usage, cache->GetPinnedUsage());
    } else {
      unreleased_handles.push_front(handle);
    }
    if (i % 3 == 0) {
      unreleased_handles.push_front(cache->Lookup(key));
//如果i%2==0，则在查找之前已取消固定该条目，因此已固定
//使用增加
      if (i % 2 == 0) {
        pinned_usage += kv_size;
      }
      ASSERT_EQ(pinned_usage, cache->GetPinnedUsage());
    }
  }

//检查是否重载缓存不会更改固定用法
  for (uint64_t i = 1; i < 2 * kCapacity; ++i) {
    auto key = ToString(i);
    cache->Insert(key, reinterpret_cast<void*>(value), key.size() + 5,
                  dumbDeleter);
  }
  ASSERT_EQ(pinned_usage, cache->GetPinnedUsage());

//释放固定条目的句柄以防止内存泄漏
  for (auto handle : unreleased_handles) {
    cache->Release(handle);
  }
}

TEST_P(CacheTest, HitAndMiss) {
  ASSERT_EQ(-1, Lookup(100));

  Insert(100, 101);
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(-1,  Lookup(200));
  ASSERT_EQ(-1,  Lookup(300));

  Insert(200, 201);
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(-1,  Lookup(300));

  Insert(100, 102);
  ASSERT_EQ(102, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(-1,  Lookup(300));

  ASSERT_EQ(1U, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);
}

TEST_P(CacheTest, InsertSameKey) {
  Insert(1, 1);
  Insert(1, 2);
  ASSERT_EQ(2, Lookup(1));
}

TEST_P(CacheTest, Erase) {
  Erase(200);
  ASSERT_EQ(0U, deleted_keys_.size());

  Insert(100, 101);
  Insert(200, 201);
  Erase(100);
  ASSERT_EQ(-1,  Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(1U, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);

  Erase(100);
  ASSERT_EQ(-1,  Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(1U, deleted_keys_.size());
}

TEST_P(CacheTest, EntriesArePinned) {
  Insert(100, 101);
  Cache::Handle* h1 = cache_->Lookup(EncodeKey(100));
  ASSERT_EQ(101, DecodeValue(cache_->Value(h1)));
  ASSERT_EQ(1U, cache_->GetUsage());

  Insert(100, 102);
  Cache::Handle* h2 = cache_->Lookup(EncodeKey(100));
  ASSERT_EQ(102, DecodeValue(cache_->Value(h2)));
  ASSERT_EQ(0U, deleted_keys_.size());
  ASSERT_EQ(2U, cache_->GetUsage());

  cache_->Release(h1);
  ASSERT_EQ(1U, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);
  ASSERT_EQ(1U, cache_->GetUsage());

  Erase(100);
  ASSERT_EQ(-1, Lookup(100));
  ASSERT_EQ(1U, deleted_keys_.size());
  ASSERT_EQ(1U, cache_->GetUsage());

  cache_->Release(h2);
  ASSERT_EQ(2U, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[1]);
  ASSERT_EQ(102, deleted_values_[1]);
  ASSERT_EQ(0U, cache_->GetUsage());
}

TEST_P(CacheTest, EvictionPolicy) {
  Insert(100, 101);
  Insert(200, 201);

//经常使用的入口必须保持在周围
  for (int i = 0; i < kCacheSize + 100; i++) {
    Insert(1000+i, 2000+i);
    ASSERT_EQ(101, Lookup(100));
  }
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(-1, Lookup(200));
}

TEST_P(CacheTest, ExternalRefPinsEntries) {
  Insert(100, 101);
  Cache::Handle* h = cache_->Lookup(EncodeKey(100));
  ASSERT_TRUE(cache_->Ref(h));
  ASSERT_EQ(101, DecodeValue(cache_->Value(h)));
  ASSERT_EQ(1U, cache_->GetUsage());

  for (int i = 0; i < 3; ++i) {
    if (i > 0) {
//第一个版本（i==1）对应于ref（），第二个版本（i==2）
//对应于lookup（）。然后，由于所有外部引用都被释放，
//下面的插入应该将缓存项推出。
      cache_->Release(h);
    }
//双倍缓存大小，因为块缓存中的使用位阻止100
//在第一个kcachesize迭代中被逐出
    for (int j = 0; j < 2 * kCacheSize + 100; j++) {
      Insert(1000 + j, 2000 + j);
    }
    if (i < 2) {
      ASSERT_EQ(101, Lookup(100));
    }
  }
  ASSERT_EQ(-1, Lookup(100));
}

TEST_P(CacheTest, EvictionPolicyRef) {
  Insert(100, 101);
  Insert(101, 102);
  Insert(102, 103);
  Insert(103, 104);
  Insert(200, 101);
  Insert(201, 102);
  Insert(202, 103);
  Insert(203, 104);
  Cache::Handle* h201 = cache_->Lookup(EncodeKey(200));
  Cache::Handle* h202 = cache_->Lookup(EncodeKey(201));
  Cache::Handle* h203 = cache_->Lookup(EncodeKey(202));
  Cache::Handle* h204 = cache_->Lookup(EncodeKey(203));
  Insert(300, 101);
  Insert(301, 102);
  Insert(302, 103);
  Insert(303, 104);

//插入比缓存容量大得多的条目
  for (int i = 0; i < kCacheSize + 100; i++) {
    Insert(1000 + i, 2000 + i);
  }

//检查是否在开头插入条目
//被驱逐出境。没有额外参考的被驱逐
//有没有。
  ASSERT_EQ(-1, Lookup(100));
  ASSERT_EQ(-1, Lookup(101));
  ASSERT_EQ(-1, Lookup(102));
  ASSERT_EQ(-1, Lookup(103));

  ASSERT_EQ(-1, Lookup(300));
  ASSERT_EQ(-1, Lookup(301));
  ASSERT_EQ(-1, Lookup(302));
  ASSERT_EQ(-1, Lookup(303));

  ASSERT_EQ(101, Lookup(200));
  ASSERT_EQ(102, Lookup(201));
  ASSERT_EQ(103, Lookup(202));
  ASSERT_EQ(104, Lookup(203));

//清理所有把手
  cache_->Release(h201);
  cache_->Release(h202);
  cache_->Release(h203);
  cache_->Release(h204);
}

TEST_P(CacheTest, EvictEmptyCache) {
//插入大于容量的项以在空缓存上触发逐出。
  auto cache = NewCache(1, 0, false);
  ASSERT_OK(cache->Insert("foo", nullptr, 10, dumbDeleter));
}

TEST_P(CacheTest, EraseFromDeleter) {
//具有将从缓存中删除项的删除程序，该删除程序将重新输入
//此时的缓存。
  std::shared_ptr<Cache> cache = NewCache(10, 0, false);
  ASSERT_OK(cache->Insert("foo", nullptr, 1, dumbDeleter));
  ASSERT_OK(cache->Insert("bar", cache.get(), 1, eraseDeleter));
  cache->Erase("bar");
  ASSERT_EQ(nullptr, cache->Lookup("foo"));
  ASSERT_EQ(nullptr, cache->Lookup("bar"));
}

TEST_P(CacheTest, ErasedHandleState) {
//插入一把钥匙并获得两个把手
  Insert(100, 1000);
  Cache::Handle* h1 = cache_->Lookup(EncodeKey(100));
  Cache::Handle* h2 = cache_->Lookup(EncodeKey(100));
  ASSERT_EQ(h1, h2);
  ASSERT_EQ(DecodeValue(cache_->Value(h1)), 1000);
  ASSERT_EQ(DecodeValue(cache_->Value(h2)), 1000);

//从缓存中删除密钥
  Erase(100);
//在缓存中找不到
  ASSERT_EQ(-1, Lookup(100));

//松开一个手柄
  cache_->Release(h1);
//在缓存中仍然找不到
  ASSERT_EQ(-1, Lookup(100));

  cache_->Release(h2);
}

TEST_P(CacheTest, HeavyEntries) {
//加上一堆轻重缓急的条目，然后计算
//仍在缓存中的项的大小，必须大约为
//与总容量相同。
  const int kLight = 1;
  const int kHeavy = 10;
  int added = 0;
  int index = 0;
  while (added < 2*kCacheSize) {
    const int weight = (index & 1) ? kLight : kHeavy;
    Insert(index, 1000+index, weight);
    added += weight;
    index++;
  }

  int cached_weight = 0;
  for (int i = 0; i < index; i++) {
    const int weight = (i & 1 ? kLight : kHeavy);
    int r = Lookup(i);
    if (r >= 0) {
      cached_weight += weight;
      ASSERT_EQ(1000+i, r);
    }
  }
  ASSERT_LE(cached_weight, kCacheSize + kCacheSize/10);
}

TEST_P(CacheTest, NewId) {
  uint64_t a = cache_->NewId();
  uint64_t b = cache_->NewId();
  ASSERT_NE(a, b);
}


class Value {
 public:
  explicit Value(size_t v) : v_(v) { }

  size_t v_;
};

namespace {
void deleter(const Slice& key, void* value) {
  delete static_cast<Value *>(value);
}
}  //命名空间

TEST_P(CacheTest, ReleaseAndErase) {
  std::shared_ptr<Cache> cache = NewCache(5, 0, false);
  Cache::Handle* handle;
  Status s = cache->Insert(EncodeKey(100), EncodeValue(100), 1,
                           &CacheTest::Deleter, &handle);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(5U, cache->GetCapacity());
  ASSERT_EQ(1U, cache->GetUsage());
  ASSERT_EQ(0U, deleted_keys_.size());
  auto erased = cache->Release(handle, true);
  ASSERT_TRUE(erased);
//此测试已调用Deleter
  ASSERT_EQ(1U, deleted_keys_.size());
}

TEST_P(CacheTest, ReleaseWithoutErase) {
  std::shared_ptr<Cache> cache = NewCache(5, 0, false);
  Cache::Handle* handle;
  Status s = cache->Insert(EncodeKey(100), EncodeValue(100), 1,
                           &CacheTest::Deleter, &handle);
  ASSERT_TRUE(s.ok());
  ASSERT_EQ(5U, cache->GetCapacity());
  ASSERT_EQ(1U, cache->GetUsage());
  ASSERT_EQ(0U, deleted_keys_.size());
  auto erased = cache->Release(handle);
  ASSERT_FALSE(erased);
//此测试不调用Deleter。当缓存有空闲容量时
//预计不会立即删除已发布的项目。
  ASSERT_EQ(0U, deleted_keys_.size());
}

TEST_P(CacheTest, SetCapacity) {
//测试1：增加容量
//让我们创建一个容量为5的缓存，
//然后，插入5个元素，然后增加容量
//到10，返回容量应为10，使用量=5
  std::shared_ptr<Cache> cache = NewCache(5, 0, false);
  std::vector<Cache::Handle*> handles(10);
//插入5个条目，但不释放。
  for (size_t i = 0; i < 5; i++) {
    std::string key = ToString(i+1);
    Status s = cache->Insert(key, new Value(i + 1), 1, &deleter, &handles[i]);
    ASSERT_TRUE(s.ok());
  }
  ASSERT_EQ(5U, cache->GetCapacity());
  ASSERT_EQ(5U, cache->GetUsage());
  cache->SetCapacity(10);
  ASSERT_EQ(10U, cache->GetCapacity());
  ASSERT_EQ(5U, cache->GetUsage());

//测试2：降低容量
//在缓存中再插入5个元素，然后释放5个，
//然后将通行能力降低到7，最终通行能力应为7
//用法应该是7
  for (size_t i = 5; i < 10; i++) {
    std::string key = ToString(i+1);
    Status s = cache->Insert(key, new Value(i + 1), 1, &deleter, &handles[i]);
    ASSERT_TRUE(s.ok());
  }
  ASSERT_EQ(10U, cache->GetCapacity());
  ASSERT_EQ(10U, cache->GetUsage());
  for (size_t i = 0; i < 5; i++) {
    cache->Release(handles[i]);
  }
  ASSERT_EQ(10U, cache->GetCapacity());
  ASSERT_EQ(10U, cache->GetUsage());
  cache->SetCapacity(7);
  ASSERT_EQ(7, cache->GetCapacity());
  ASSERT_EQ(7, cache->GetUsage());

//释放剩下的5个保持Valgrind快乐
  for (size_t i = 5; i < 10; i++) {
    cache->Release(handles[i]);
  }
}

TEST_P(CacheTest, SetStrictCapacityLimit) {
//测试1：将标志设置为假。插入的键多于容量。看看他们
//一切都过去了。
  std::shared_ptr<Cache> cache = NewLRUCache(5, 0, false);
  std::vector<Cache::Handle*> handles(10);
  Status s;
  for (size_t i = 0; i < 10; i++) {
    std::string key = ToString(i + 1);
    s = cache->Insert(key, new Value(i + 1), 1, &deleter, &handles[i]);
    ASSERT_OK(s);
    ASSERT_NE(nullptr, handles[i]);
  }

//测试2：将标志设置为真。插入并检查是否失败。
  std::string extra_key = "extra";
  Value* extra_value = new Value(0);
  cache->SetStrictCapacityLimit(true);
  Cache::Handle* handle;
  s = cache->Insert(extra_key, extra_value, 1, &deleter, &handle);
  ASSERT_TRUE(s.IsIncomplete());
  ASSERT_EQ(nullptr, handle);

  for (size_t i = 0; i < 10; i++) {
    cache->Release(handles[i]);
  }

//测试3:init，标志为true。
  std::shared_ptr<Cache> cache2 = NewLRUCache(5, 0, true);
  for (size_t i = 0; i < 5; i++) {
    std::string key = ToString(i + 1);
    s = cache2->Insert(key, new Value(i + 1), 1, &deleter, &handles[i]);
    ASSERT_OK(s);
    ASSERT_NE(nullptr, handles[i]);
  }
  s = cache2->Insert(extra_key, extra_value, 1, &deleter, &handle);
  ASSERT_TRUE(s.IsIncomplete());
  ASSERT_EQ(nullptr, handle);
//无手柄测试插件
  s = cache2->Insert(extra_key, extra_value, 1, &deleter);
//就好像密钥已经插入到缓存中，但被立即收回。
  ASSERT_OK(s);
  ASSERT_EQ(5, cache->GetUsage());
  ASSERT_EQ(nullptr, cache2->Lookup(extra_key));

  for (size_t i = 0; i < 5; i++) {
    cache2->Release(handles[i]);
  }
}

TEST_P(CacheTest, OverCapacity) {
  size_t n = 10;

//带有n个条目和一个碎片的lrucache
  std::shared_ptr<Cache> cache = NewCache(n, 0, false);

  std::vector<Cache::Handle*> handles(n+1);

//插入n+1项，但不释放。
  for (size_t i = 0; i < n + 1; i++) {
    std::string key = ToString(i+1);
    Status s = cache->Insert(key, new Value(i + 1), 1, &deleter, &handles[i]);
    ASSERT_TRUE(s.ok());
  }

//猜猜缓存里现在有什么？
  for (size_t i = 0; i < n + 1; i++) {
    std::string key = ToString(i+1);
    auto h = cache->Lookup(key);
    ASSERT_TRUE(h != nullptr);
    if (h) cache->Release(h);
  }

//缓存容量过大，因为无法收回任何内容
  ASSERT_EQ(n + 1U, cache->GetUsage());
  for (size_t i = 0; i < n + 1; i++) {
    cache->Release(handles[i]);
  }
//确保已触发逐出。
  cache->SetCapacity(n);

//自从元素被释放后，缓存现在容量不足
  ASSERT_EQ(n, cache->GetUsage());

//元素0被逐出，其余元素在那里
//这与LRU策略一致，因为元素0
//首先发布
  for (size_t i = 0; i < n + 1; i++) {
    std::string key = ToString(i+1);
    auto h = cache->Lookup(key);
    if (h) {
      ASSERT_NE(i, 0U);
      cache->Release(h);
    } else {
      ASSERT_EQ(i, 0U);
    }
  }
}

namespace {
std::vector<std::pair<int, int>> callback_state;
void callback(void* entry, size_t charge) {
  callback_state.push_back({DecodeValue(entry), static_cast<int>(charge)});
}
};

TEST_P(CacheTest, ApplyToAllCacheEntiresTest) {
  std::vector<std::pair<int, int>> inserted;
  callback_state.clear();

  for (int i = 0; i < 10; ++i) {
    Insert(i, i * 2, i + 1);
    inserted.push_back({i * 2, i + 1});
  }
  cache_->ApplyToAllCacheEntries(callback, true);

  std::sort(inserted.begin(), inserted.end());
  std::sort(callback_state.begin(), callback_state.end());
  ASSERT_TRUE(inserted == callback_state);
}

TEST_P(CacheTest, DefaultShardBits) {
//测试1：将标志设置为假。插入的键多于容量。看看他们
//一切都过去了。
  std::shared_ptr<Cache> cache = NewCache(16 * 1024L * 1024L);
  ShardedCache* sc = dynamic_cast<ShardedCache*>(cache.get());
  ASSERT_EQ(5, sc->GetNumShardBits());

  cache = NewLRUCache(511 * 1024L, -1, true);
  sc = dynamic_cast<ShardedCache*>(cache.get());
  ASSERT_EQ(0, sc->GetNumShardBits());

  cache = NewLRUCache(1024L * 1024L * 1024L, -1, true);
  sc = dynamic_cast<ShardedCache*>(cache.get());
  ASSERT_EQ(6, sc->GetNumShardBits());
}

#ifdef SUPPORT_CLOCK_CACHE
shared_ptr<Cache> (*new_clock_cache_func)(size_t, int, bool) = NewClockCache;
INSTANTIATE_TEST_CASE_P(CacheTestInstance, CacheTest,
                        testing::Values(kLRU, kClock));
#else
INSTANTIATE_TEST_CASE_P(CacheTestInstance, CacheTest, testing::Values(kLRU));
#endif  //支持时钟缓存

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
