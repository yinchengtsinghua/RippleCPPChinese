
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

#pragma once
#ifndef ROCKSDB_LITE

#include <memory>

#include "rocksdb/status.h"

namespace rocksdb {

//TransactionDBmutex和TransactionDBcondvar API允许应用程序
//实现要由
//锁定密钥时事务数据库。
//
//要使用自定义TransactionDBmutexFactory打开TransactionDB，请设置
//transactiondboptions.custom_mutex_工厂。

class TransactionDBMutex {
 public:
  virtual ~TransactionDBMutex() {}

//尝试获取锁。成功时返回OK，失败时返回其他状态。
//如果返回状态为OK，TransactionDB最终将调用Unlock（）。
  virtual Status Lock() = 0;

//尝试获取锁。如果超时为非负，则操作可能是
//在这么多微秒后失败。
//成功后返回OK，
//如果超时，
//或其他故障状态。
//如果返回状态为OK，TransactionDB最终将调用Unlock（）。
  virtual Status TryLockFor(int64_t timeout_time) = 0;

//解锁被lock（）或trylockUntil（）成功锁定的互斥体
  virtual void UnLock() = 0;
};

class TransactionDBCondVar {
 public:
  virtual ~TransactionDBCondVar() {}

//阻止当前线程，直到调用通知条件变量
//notify（）或notifyAll（）。wait（）将在mutex锁定的情况下调用。
//通知后返回OK。
//如果TransactionDB应停止等待并使操作失败，则返回non-OK。
//即使没有通知，也可能错误地返回OK。
  virtual Status Wait(std::shared_ptr<TransactionDBMutex> mutex) = 0;

//阻止当前线程，直到调用通知条件变量
//notify（）或notifyall（），或者如果超时。
//wait（）将在mutex锁定的情况下调用。
//
//如果超时为非负，那么在这之后操作应该失败
//微秒。
//如果实现此类的自定义版本，则实现可以
//选择忽略超时。
//
//通知后返回OK。
//如果超时，则返回timedout。
//如果TransactionDB应其他人停止等待并返回其他状态
//操作失败。
//即使没有通知，也可能错误地返回OK。
  virtual Status WaitFor(std::shared_ptr<TransactionDBMutex> mutex,
                         int64_t timeout_time) = 0;

//如果有线程正在等待*，请至少取消阻止
//等待线程。
  virtual void Notify() = 0;

//取消阻止所有等待*的线程。
  virtual void NotifyAll() = 0;
};

//可以分配互斥体和条件变量的工厂类。
class TransactionDBMutexFactory {
 public:
//创建TransactionDBmutex对象。
  virtual std::shared_ptr<TransactionDBMutex> AllocateMutex() = 0;

//创建Transactiondbcondvar对象。
  virtual std::shared_ptr<TransactionDBCondVar> AllocateCondVar() = 0;

  virtual ~TransactionDBMutexFactory() {}
};

}  //命名空间rocksdb

#endif  //摇滚乐
