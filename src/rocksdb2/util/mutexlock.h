
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
#include <assert.h>
#include <atomic>
#include <mutex>
#include <thread>
#include "port/port.h"

namespace rocksdb {

//helper类，在构造时锁定互斥体，并在
//将调用mutexlock对象的析构函数。
//
//典型用法：
//
//void myClass:：myMethod（）
//mutexlock l（&muou；//muou是一个实例变量
//…一些复杂的代码，可能有多个返回路径…
//}

class MutexLock {
 public:
  explicit MutexLock(port::Mutex *mu) : mu_(mu) {
    this->mu_->Lock();
  }
  ~MutexLock() { this->mu_->Unlock(); }

 private:
  port::Mutex *const mu_;
//不允许复制
  MutexLock(const MutexLock&);
  void operator=(const MutexLock&);
};

//
//在指定的rwmutex上获取readlock。
//锁将自动释放，然后
//对象超出范围。
//
class ReadLock {
 public:
  explicit ReadLock(port::RWMutex *mu) : mu_(mu) {
    this->mu_->ReadLock();
  }
  ~ReadLock() { this->mu_->ReadUnlock(); }

 private:
  port::RWMutex *const mu_;
//不允许复制
  ReadLock(const ReadLock&);
  void operator=(const ReadLock&);
};

//
//当对象被破坏时自动解锁锁定的互斥体
//
class ReadUnlock {
 public:
  explicit ReadUnlock(port::RWMutex *mu) : mu_(mu) { mu->AssertHeld(); }
  ~ReadUnlock() { mu_->ReadUnlock(); }

 private:
  port::RWMutex *const mu_;
//不允许复制
  ReadUnlock(const ReadUnlock &) = delete;
  ReadUnlock &operator=(const ReadUnlock &) = delete;
};

//
//在指定的rwmutex上获取写锁。
//锁将自动释放，然后
//对象超出范围。
//
class WriteLock {
 public:
  explicit WriteLock(port::RWMutex *mu) : mu_(mu) {
    this->mu_->WriteLock();
  }
  ~WriteLock() { this->mu_->WriteUnlock(); }

 private:
  port::RWMutex *const mu_;
//不允许复制
  WriteLock(const WriteLock&);
  void operator=(const WriteLock&);
};

//
//对于低争用情况，spinmutex的开销非常低。方法名称
//这样您就可以使用std：：unique_lock或std：：lock_guard。
//
class SpinMutex {
 public:
  SpinMutex() : locked_(false) {}

  bool try_lock() {
    auto currently_locked = locked_.load(std::memory_order_relaxed);
    return !currently_locked &&
           locked_.compare_exchange_weak(currently_locked, true,
                                         std::memory_order_acquire,
                                         std::memory_order_relaxed);
  }

  void lock() {
    for (size_t tries = 0;; ++tries) {
      if (try_lock()) {
//成功
        break;
      }
      port::AsmVolatilePause();
      if (tries > 100) {
        std::this_thread::yield();
      }
    }
  }

  void unlock() { locked_.store(false, std::memory_order_release); }

 private:
  std::atomic<bool> locked_;
};

}  //命名空间rocksdb
