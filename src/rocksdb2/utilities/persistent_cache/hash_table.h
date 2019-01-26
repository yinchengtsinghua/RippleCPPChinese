
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

#include <assert.h>
#include <list>
#include <vector>

#ifdef OS_LINUX
#include <sys/mman.h>
#endif

#include "include/rocksdb/env.h"
#include "util/mutexlock.h"

namespace rocksdb {

//hashtable<t，hash，equal>
//
//基于同步的哈希表的传统实现
//在多核场景中表现不太好。这是一个实现
//设计用于具有高锁争用的多核场景。
//
//<-------阿尔法---------->
//桶碰撞列表
//---+---++---+---+---+----+-----------++--+
///----->
///+---++---+----+---+----.--------++--+
///*
//锁/+---+
//+--+/。.
//。.
//+——+。.
//。.
//+——+。.
//。.
//+——+。.
//\+---+
//\>
//++--+
//\>
//----+--+
//
//锁争用分布在一组锁上。这有助于改善
//并发访问。脊柱是为一定的承载力和负荷而设计的。
//因素。当容量规划正确完成时，我们可以预期
//o（加载系数=1）插入、访问和移除时间。
//
//调试版本上的微基准测试提供了大约50万/秒的插入速率，
//并行擦除和查找（总计约150万次操作/秒）。如果
//块为4K，哈希表可以支持
//6 Gb/s。
//
//t对象类型（包含键和值）
//从类型t返回哈希的哈希函数
//如果两个对象相等，则返回equal
//（指针类型需要显式相等）
//
template <class T, class Hash, class Equal>
class HashTable {
 public:
  explicit HashTable(const size_t capacity = 1024 * 1024,
                     const float load_factor = 2.0, const uint32_t nlocks = 256)
      : nbuckets_(
            static_cast<uint32_t>(load_factor ? capacity / load_factor : 0)),
        nlocks_(nlocks) {
//先决条件
    assert(capacity);
    assert(load_factor);
    assert(nbuckets_);
    assert(nlocks_);

    buckets_.reset(new Bucket[nbuckets_]);
#ifdef OS_LINUX
    mlock(buckets_.get(), nbuckets_ * sizeof(Bucket));
#endif

//初始化锁
    locks_.reset(new port::RWMutex[nlocks_]);
#ifdef OS_LINUX
    mlock(locks_.get(), nlocks_ * sizeof(port::RWMutex));
#endif

//后置条件
    assert(buckets_);
    assert(locks_);
  }

  virtual ~HashTable() { AssertEmptyBuckets(); }

//
//将给定记录插入哈希表
//
  bool Insert(const T& t) {
    const uint64_t h = Hash()(t);
    const uint32_t bucket_idx = h % nbuckets_;
    const uint32_t lock_idx = bucket_idx % nlocks_;

    WriteLock _(&locks_[lock_idx]);
    auto& bucket = buckets_[bucket_idx];
    return Insert(&bucket, t);
  }

//查找哈希表
//
//请注意，读锁应该由调用方持有。这是因为
//调用者拥有数据，只要他
//对数据进行操作。
  bool Find(const T& t, T* ret, port::RWMutex** ret_lock) {
    const uint64_t h = Hash()(t);
    const uint32_t bucket_idx = h % nbuckets_;
    const uint32_t lock_idx = bucket_idx % nlocks_;

    port::RWMutex& lock = locks_[lock_idx];
    lock.ReadLock();

    auto& bucket = buckets_[bucket_idx];
    if (Find(&bucket, t, ret)) {
      *ret_lock = &lock;
      return true;
    }

    lock.ReadUnlock();
    return false;
  }

//
//从哈希表中删除给定的键
//
  bool Erase(const T& t, T* ret) {
    const uint64_t h = Hash()(t);
    const uint32_t bucket_idx = h % nbuckets_;
    const uint32_t lock_idx = bucket_idx % nlocks_;

    WriteLock _(&locks_[lock_idx]);

    auto& bucket = buckets_[bucket_idx];
    return Erase(&bucket, t, ret);
  }

//获取与键关联的互斥体
//此调用用于将给定数据的锁保持更长时间
//时间。
  port::RWMutex* GetMutex(const T& t) {
    const uint64_t h = Hash()(t);
    const uint32_t bucket_idx = h % nbuckets_;
    const uint32_t lock_idx = bucket_idx % nlocks_;

    return &locks_[lock_idx];
  }

  void Clear(void (*fn)(T)) {
    for (uint32_t i = 0; i < nbuckets_; ++i) {
      const uint32_t lock_idx = i % nlocks_;
      WriteLock _(&locks_[lock_idx]);
      for (auto& t : buckets_[i].list_) {
        (*fn)(t);
      }
      buckets_[i].list_.clear();
    }
  }

 protected:
//将散列到相同存储桶编号的键的存储桶建模
  struct Bucket {
    std::list<T> list_;
  };

//用自定义比较器运算符替换std:：find
  typename std::list<T>::iterator Find(std::list<T>* list, const T& t) {
    for (auto it = list->begin(); it != list->end(); ++it) {
      if (Equal()(*it, t)) {
        return it;
      }
    }
    return list->end();
  }

  bool Insert(Bucket* bucket, const T& t) {
//检查密钥是否已存在
    auto it = Find(&bucket->list_, t);
    if (it != bucket->list_.end()) {
      return false;
    }

//插入到桶
    bucket->list_.push_back(t);
    return true;
  }

  bool Find(Bucket* bucket, const T& t, T* ret) {
    auto it = Find(&bucket->list_, t);
    if (it != bucket->list_.end()) {
      if (ret) {
        *ret = *it;
      }
      return true;
    }
    return false;
  }

  bool Erase(Bucket* bucket, const T& t, T* ret) {
    auto it = Find(&bucket->list_, t);
    if (it != bucket->list_.end()) {
      if (ret) {
        *ret = *it;
      }

      bucket->list_.erase(it);
      return true;
    }
    return false;
  }

//断言所有存储桶都为空
  void AssertEmptyBuckets() {
#ifndef NDEBUG
    for (size_t i = 0; i < nbuckets_; ++i) {
      WriteLock _(&locks_[i % nlocks_]);
      assert(buckets_[i].list_.empty());
    }
#endif
  }

const uint32_t nbuckets_;                 //脊椎中的桶数
std::unique_ptr<Bucket[]> buckets_;       //垃圾桶的侧面
const uint32_t nlocks_;                   //锁号
std::unique_ptr<port::RWMutex[]> locks_;  //粒状锁
};

}  //命名空间rocksdb

#endif
