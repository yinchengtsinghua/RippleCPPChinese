
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

#include "util/mutexlock.h"

namespace rocksdb {

//LRU元素定义
//
//任何需要成为LRU算法一部分的对象都应该扩展这个
//班
template <class T>
struct LRUElement {
  explicit LRUElement() : next_(nullptr), prev_(nullptr), refs_(0) {}

  virtual ~LRUElement() { assert(!refs_); }

  T* next_;
  T* prev_;
  std::atomic<size_t> refs_;
};

//LRU实施
//
//就地LRU实施。在以下情况下不涉及复制或分配：
//插入或移除元件。这使得数据结构变薄
template <class T>
class LRUList {
 public:
  virtual ~LRUList() {
    MutexLock _(&lock_);
    assert(!head_);
    assert(!tail_);
  }

//在冷端将元件推入LRU
  inline void Push(T* const t) {
    assert(t);
    assert(!t->next_);
    assert(!t->prev_);

    MutexLock _(&lock_);

    assert((!head_ && !tail_) || (head_ && tail_));
    assert(!head_ || !head_->prev_);
    assert(!tail_ || !tail_->next_);

    t->next_ = head_;
    if (head_) {
      head_->prev_ = t;
    }

    head_ = t;
    if (!tail_) {
      tail_ = t;
    }
  }

//取消元素与LRU的链接
  inline void Unlink(T* const t) {
    MutexLock _(&lock_);
    UnlinkImpl(t);
  }

//从LRU中逐出一个元素
  inline T* Pop() {
    MutexLock _(&lock_);

    assert(tail_ && head_);
    assert(!tail_->next_);
    assert(!head_->prev_);

    T* t = head_;
    while (t && t->refs_) {
      t = t->next_;
    }

    if (!t) {
//什么都不能驱逐
      return nullptr;
    }

    assert(!t->refs_);

//与元素不同
    UnlinkImpl(t);
    return t;
  }

//将元素从列表的前面移到列表的后面
  inline void Touch(T* const t) {
    MutexLock _(&lock_);
    UnlinkImpl(t);
    PushBackImpl(t);
  }

//检查LRU是否为空
  inline bool IsEmpty() const {
    MutexLock _(&lock_);
    return !head_ && !tail_;
  }

 private:
//取消元素与LRU的链接
  void UnlinkImpl(T* const t) {
    assert(t);

    lock_.AssertHeld();

    assert(head_ && tail_);
    assert(t->prev_ || head_ == t);
    assert(t->next_ || tail_ == t);

    if (t->prev_) {
      t->prev_->next_ = t->next_;
    }
    if (t->next_) {
      t->next_->prev_ = t->prev_;
    }

    if (tail_ == t) {
      tail_ = tail_->prev_;
    }
    if (head_ == t) {
      head_ = head_->next_;
    }

    t->next_ = t->prev_ = nullptr;
  }

//在热端插入元素
  inline void PushBack(T* const t) {
    MutexLock _(&lock_);
    PushBackImpl(t);
  }

  inline void PushBackImpl(T* const t) {
    assert(t);
    assert(!t->next_);
    assert(!t->prev_);

    lock_.AssertHeld();

    assert((!head_ && !tail_) || (head_ && tail_));
    assert(!head_ || !head_->prev_);
    assert(!tail_ || !tail_->next_);

    t->prev_ = tail_;
    if (tail_) {
      tail_->next_ = t;
    }

    tail_ = t;
    if (!head_) {
      head_ = tail_;
    }
  }

mutable port::Mutex lock_;  //同步原语
T* head_ = nullptr;         //锋（冷）
T* tail_ = nullptr;         //背部（热）
};

}  //命名空间rocksdb

#endif
