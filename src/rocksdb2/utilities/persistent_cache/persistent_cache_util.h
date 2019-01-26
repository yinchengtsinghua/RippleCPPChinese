
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

#include <limits>
#include <list>

#include "util/mutexlock.h"

namespace rocksdb {

//
//具有以下选项的简单同步队列实现
//限定队列
//
//溢出时，元素将被丢弃
//
template <class T>
class BoundedQueue {
 public:
  explicit BoundedQueue(
      const size_t max_size = std::numeric_limits<size_t>::max())
      : cond_empty_(&lock_), max_size_(max_size) {}

  virtual ~BoundedQueue() {}

  void Push(T&& t) {
    MutexLock _(&lock_);
    if (max_size_ != std::numeric_limits<size_t>::max() &&
        size_ + t.Size() >= max_size_) {
//溢流
      return;
    }

    size_ += t.Size();
    q_.push_back(std::move(t));
    cond_empty_.SignalAll();
  }

  T Pop() {
    MutexLock _(&lock_);
    while (q_.empty()) {
      cond_empty_.Wait();
    }

    T t = std::move(q_.front());
    size_ -= t.Size();
    q_.pop_front();
    return std::move(t);
  }

  size_t Size() const {
    MutexLock _(&lock_);
    return size_;
  }

 private:
  mutable port::Mutex lock_;
  port::CondVar cond_empty_;
  std::list<T> q_;
  size_t size_ = 0;
  const size_t max_size_;
};

}  //命名空间rocksdb
