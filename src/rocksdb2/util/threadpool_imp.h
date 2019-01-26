
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

#include "rocksdb/threadpool.h"
#include "rocksdb/env.h"

#include <memory>
#include <functional>

namespace rocksdb {


class ThreadPoolImpl : public ThreadPool {
 public:
  ThreadPoolImpl();
  ~ThreadPoolImpl();

  ThreadPoolImpl(ThreadPoolImpl&&) = delete;
  ThreadPoolImpl& operator=(ThreadPoolImpl&&) = delete;

//实现线程池接口

//等待所有线程完成。
//放弃所有未完成的工作
//开始执行并等待那些正在运行的
//完成
  void JoinAllThreads() override;

//设置将要执行的后台线程数
//计划作业。
  void SetBackgroundThreads(int num) override;
  int GetBackgroundThreads() override;

//获取在线程池队列中计划的作业数。
  unsigned int GetQueueLen() const override;

//等待所有作业完成这些作业
//已经开始运行而那些没有
//开始还
  void WaitForJobsAndJoinAllThreads() override;

//使线程以较低的内核优先级运行
//目前只对Linux有影响
  void LowerIOPriority();

//确保池中至少有num个线程
//但是如果有更多的线程，就不要终止线程
  void IncBackgroundThreadsIfNeeded(int num);

//放火忘工作
//这些工作不能不按计划进行

//这允许多次提交同一作业
  void SubmitJob(const std::function<void()>&) override;
//这就提高了功能的效率
  void SubmitJob(std::function<void()>&&) override;

//使用非计划标记和非计划功能计划作业
//可用于按标记筛选和非计划作业
//仍在队列中且未开始运行的
  void Schedule(void (*function)(void* arg1), void* arg, void* tag,
                void (*unschedFunction)(void* arg));

//筛选仍在队列中且匹配的作业
//给定的标签。从队列中删除它们（如果有）
//对每一个这样的工作执行一个非计划的功能
//如果在计划时间给出。
  int UnSchedule(void* tag);

  void SetHostEnv(Env* env);

  Env* GetHostEnv() const;

//返回线程优先级。
//这将允许其成员线程知道其优先级。
  Env::Priority GetThreadPriority() const;

//设置线程优先级。
  void SetThreadPriority(Env::Priority priority);

  static void PthreadCall(const char* label, int result);

  struct Impl;

 private:

//当前公共虚拟接口不提供可用的
//功能性，因此不能在内部用于
//外观不同的实现。
//
//我们提出了一个PIMPL习惯用法，以便轻松地替换线程池impl
//不接触头文件，但可能提供不同的.cc
//CMAKE选项驱动。
//
//另一个选项是引入env:：makethreadpool（）虚拟接口
//并覆盖环境。这需要重构线程池的使用。
//
//我们也可以将这两种方法结合起来
   std::unique_ptr<Impl>   impl_;
};

}  //命名空间rocksdb
