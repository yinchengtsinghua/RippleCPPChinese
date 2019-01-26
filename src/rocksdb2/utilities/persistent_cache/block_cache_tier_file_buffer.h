
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

#include <list>
#include <memory>
#include <string>

#include "include/rocksdb/comparator.h"
#include "util/arena.h"
#include "util/mutexlock.h"

namespace rocksdb {

//
//缓存写入缓冲区
//
//可以通过附加操作的缓冲区抽象
//（非线程安全）
class CacheWriteBuffer {
 public:
  explicit CacheWriteBuffer(const size_t size) : size_(size), pos_(0) {
    buf_.reset(new char[size_]);
    assert(!pos_);
    assert(size_);
  }

  virtual ~CacheWriteBuffer() {}

  void Append(const char* buf, const size_t size) {
    assert(pos_ + size <= size_);
    memcpy(buf_.get() + pos_, buf, size);
    pos_ += size;
    assert(pos_ <= size_);
  }

  void FillTrailingZeros() {
    assert(pos_ <= size_);
    memset(buf_.get() + pos_, '0', size_ - pos_);
    pos_ = size_;
  }

  void Reset() { pos_ = 0; }
  size_t Free() const { return size_ - pos_; }
  size_t Capacity() const { return size_; }
  size_t Used() const { return pos_; }
  char* Data() const { return buf_.get(); }

 private:
  std::unique_ptr<char[]> buf_;
  const size_t size_;
  size_t pos_;
};

//
//缓存写入缓冲分配程序
//
//缓冲池抽象（非线程安全）
//
class CacheWriteBufferAllocator {
 public:
  explicit CacheWriteBufferAllocator(const size_t buffer_size,
                                     const size_t buffer_count)
      : cond_empty_(&lock_), buffer_size_(buffer_size) {
    MutexLock _(&lock_);
    buffer_size_ = buffer_size;
    for (uint32_t i = 0; i < buffer_count; i++) {
      auto* buf = new CacheWriteBuffer(buffer_size_);
      assert(buf);
      if (buf) {
        bufs_.push_back(buf);
        cond_empty_.Signal();
      }
    }
  }

  virtual ~CacheWriteBufferAllocator() {
    MutexLock _(&lock_);
    assert(bufs_.size() * buffer_size_ == Capacity());
    for (auto* buf : bufs_) {
      delete buf;
    }
    bufs_.clear();
  }

  CacheWriteBuffer* Allocate() {
    MutexLock _(&lock_);
    if (bufs_.empty()) {
      return nullptr;
    }

    assert(!bufs_.empty());
    CacheWriteBuffer* const buf = bufs_.front();
    bufs_.pop_front();
    return buf;
  }

  void Deallocate(CacheWriteBuffer* const buf) {
    assert(buf);
    MutexLock _(&lock_);
    buf->Reset();
    bufs_.push_back(buf);
    cond_empty_.Signal();
  }

  void WaitUntilUsable() {
//我们被要求等到有缓冲器
    MutexLock _(&lock_);
    while (bufs_.empty()) {
      cond_empty_.Wait();
    }
  }

  size_t Capacity() const { return bufs_.size() * buffer_size_; }
  size_t Free() const { return bufs_.size() * buffer_size_; }
  size_t BufferSize() const { return buffer_size_; }

 private:
port::Mutex lock_;                   //同步锁
port::CondVar cond_empty_;           //空缓冲区的条件变量
size_t buffer_size_;                 //每个缓冲区的大小
std::list<CacheWriteBuffer*> bufs_;  //缓冲区
};

}  //命名空间rocksdb
