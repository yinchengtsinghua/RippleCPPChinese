
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

#ifndef ROCKSDB_LITE

#include "utilities/transactions/transaction_db_mutex_impl.h"

#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>

#include "rocksdb/utilities/transaction_db_mutex.h"

namespace rocksdb {

class TransactionDBMutexImpl : public TransactionDBMutex {
 public:
  TransactionDBMutexImpl() {}
  ~TransactionDBMutexImpl() {}

  Status Lock() override;

  Status TryLockFor(int64_t timeout_time) override;

  void UnLock() override { mutex_.unlock(); }

  friend class TransactionDBCondVarImpl;

 private:
  std::mutex mutex_;
};

class TransactionDBCondVarImpl : public TransactionDBCondVar {
 public:
  TransactionDBCondVarImpl() {}
  ~TransactionDBCondVarImpl() {}

  Status Wait(std::shared_ptr<TransactionDBMutex> mutex) override;

  Status WaitFor(std::shared_ptr<TransactionDBMutex> mutex,
                 int64_t timeout_time) override;

  void Notify() override { cv_.notify_one(); }

  void NotifyAll() override { cv_.notify_all(); }

 private:
  std::condition_variable cv_;
};

std::shared_ptr<TransactionDBMutex>
TransactionDBMutexFactoryImpl::AllocateMutex() {
  return std::shared_ptr<TransactionDBMutex>(new TransactionDBMutexImpl());
}

std::shared_ptr<TransactionDBCondVar>
TransactionDBMutexFactoryImpl::AllocateCondVar() {
  return std::shared_ptr<TransactionDBCondVar>(new TransactionDBCondVarImpl());
}

Status TransactionDBMutexImpl::Lock() {
  mutex_.lock();
  return Status::OK();
}

Status TransactionDBMutexImpl::TryLockFor(int64_t timeout_time) {
  bool locked = true;

  if (timeout_time == 0) {
    locked = mutex_.try_lock();
  } else {
//以前，此代码使用了std：：timed_mutex。但是，这已经改变了
//由于GCC版本<4.9中的已知错误。
//https://gcc.gnu.org/bugzilla/show_bug.cgi？ID＝54562
//
//因为这个互斥对象不会保持很长时间，而且只有一个互斥对象
//一次保持，可以忽略锁定超时时间
//只有在等待条件变量时才检查它。
    mutex_.lock();
  }

  if (!locked) {
//获取互斥锁超时
    return Status::TimedOut(Status::SubCode::kMutexTimeout);
  }

  return Status::OK();
}

Status TransactionDBCondVarImpl::Wait(
    std::shared_ptr<TransactionDBMutex> mutex) {
  auto mutex_impl = reinterpret_cast<TransactionDBMutexImpl*>(mutex.get());

  std::unique_lock<std::mutex> lock(mutex_impl->mutex_, std::adopt_lock);
  cv_.wait(lock);

//确保唯一锁不会在互斥体破坏时解锁互斥体
  lock.release();

  return Status::OK();
}

Status TransactionDBCondVarImpl::WaitFor(
    std::shared_ptr<TransactionDBMutex> mutex, int64_t timeout_time) {
  Status s;

  auto mutex_impl = reinterpret_cast<TransactionDBMutexImpl*>(mutex.get());
  std::unique_lock<std::mutex> lock(mutex_impl->mutex_, std::adopt_lock);

  if (timeout_time < 0) {
//如果超时为负，则不要使用超时
    cv_.wait(lock);
  } else {
    auto duration = std::chrono::microseconds(timeout_time);
    auto cv_status = cv_.wait_for(lock, duration);

//检查等待是否因超时而停止。
    if (cv_status == std::cv_status::timeout) {
      s = Status::TimedOut(Status::SubCode::kMutexTimeout);
    }
  }

//确保唯一锁不会在互斥体破坏时解锁互斥体
  lock.release();

//有人给简历打了信号，或者我们假装醒了（但没有超时）
  return s;
}

}  //命名空间rocksdb

#endif  //摇滚乐
