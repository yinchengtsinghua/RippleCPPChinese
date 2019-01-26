
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

#include <functional>

#include "util/random.h"
#include "utilities/persistent_cache/hash_table.h"
#include "utilities/persistent_cache/lrulist.h"

namespace rocksdb {

//可收回哈希表
//
//哈希表索引，其中访问最少（或访问最少）的元素之一
//可以驱逐。
//
//请注意，只能为指针类型对象创建可收回的哈希表
template <class T, class Hash, class Equal>
class EvictableHashTable : private HashTable<T*, Hash, Equal> {
 public:
  typedef HashTable<T*, Hash, Equal> hash_table;

  explicit EvictableHashTable(const size_t capacity = 1024 * 1024,
                              const float load_factor = 2.0,
                              const uint32_t nlocks = 256)
      : HashTable<T*, Hash, Equal>(capacity, load_factor, nlocks),
        lru_lists_(new LRUList<T>[hash_table::nlocks_]) {
    assert(lru_lists_);
  }

  virtual ~EvictableHashTable() { AssertEmptyLRU(); }

//
//将给定记录插入哈希表（和LRU列表）
//
  bool Insert(T* t) {
    const uint64_t h = Hash()(t);
    typename hash_table::Bucket& bucket = GetBucket(h);
    LRUListType& lru = GetLRUList(h);
    port::RWMutex& lock = GetMutex(h);

    WriteLock _(&lock);
    if (hash_table::Insert(&bucket, t)) {
      lru.Push(t);
      return true;
    }
    return false;
  }

//
//查找哈希表
//
//请注意，读锁应该由调用方持有。这是因为
//调用者拥有数据，只要他
//对数据进行操作。
  bool Find(T* t, T** ret) {
    const uint64_t h = Hash()(t);
    typename hash_table::Bucket& bucket = GetBucket(h);
    LRUListType& lru = GetLRUList(h);
    port::RWMutex& lock = GetMutex(h);

    ReadLock _(&lock);
    if (hash_table::Find(&bucket, t, ret)) {
      ++(*ret)->refs_;
      lru.Touch(*ret);
      return true;
    }
    return false;
  }

//
//逐出最近使用最少的对象之一
//
  T* Evict(const std::function<void(T*)>& fn = nullptr) {
    uint32_t random = Random::GetTLSInstance()->Next();
    const size_t start_idx = random % hash_table::nlocks_;
    T* t = nullptr;

//从开始处迭代0…启动IDX
    for (size_t i = 0; !t && i < hash_table::nlocks_; ++i) {
      const size_t idx = (start_idx + i) % hash_table::nlocks_;

      WriteLock _(&hash_table::locks_[idx]);
      LRUListType& lru = lru_lists_[idx];
      if (!lru.IsEmpty() && (t = lru.Pop())) {
        assert(!t->refs_);
//我们有一个东西要清除，从桶里清除
        const uint64_t h = Hash()(t);
        typename hash_table::Bucket& bucket = GetBucket(h);
        T* tmp = nullptr;
        bool status = hash_table::Erase(&bucket, t, &tmp);
        assert(t == tmp);
        (void)status;
        assert(status);
        if (fn) {
          fn(t);
        }
        break;
      }
      assert(!t);
    }
    return t;
  }

  void Clear(void (*fn)(T*)) {
    for (uint32_t i = 0; i < hash_table::nbuckets_; ++i) {
      const uint32_t lock_idx = i % hash_table::nlocks_;
      WriteLock _(&hash_table::locks_[lock_idx]);
      auto& lru_list = lru_lists_[lock_idx];
      auto& bucket = hash_table::buckets_[i];
      for (auto* t : bucket.list_) {
        lru_list.Unlink(t);
        (*fn)(t);
      }
      bucket.list_.clear();
    }
//确保所有LRU列表都已清空
    AssertEmptyLRU();
  }

  void AssertEmptyLRU() {
#ifndef NDEBUG
    for (uint32_t i = 0; i < hash_table::nlocks_; ++i) {
      WriteLock _(&hash_table::locks_[i]);
      auto& lru_list = lru_lists_[i];
      assert(lru_list.IsEmpty());
    }
#endif
  }

//
//获取与键关联的互斥体
//此调用用于将给定数据的锁保持更长时间
//时间。
  port::RWMutex* GetMutex(T* t) { return hash_table::GetMutex(t); }

 private:
  typedef LRUList<T> LRUListType;

  typename hash_table::Bucket& GetBucket(const uint64_t h) {
    const uint32_t bucket_idx = h % hash_table::nbuckets_;
    return hash_table::buckets_[bucket_idx];
  }

  LRUListType& GetLRUList(const uint64_t h) {
    const uint32_t bucket_idx = h % hash_table::nbuckets_;
    const uint32_t lock_idx = bucket_idx % hash_table::nlocks_;
    return lru_lists_[lock_idx];
  }

  port::RWMutex& GetMutex(const uint64_t h) {
    const uint32_t bucket_idx = h % hash_table::nbuckets_;
    const uint32_t lock_idx = bucket_idx % hash_table::nlocks_;
    return hash_table::locks_[lock_idx];
  }

  std::unique_ptr<LRUListType[]> lru_lists_;
};

}  //命名空间rocksdb

#endif
