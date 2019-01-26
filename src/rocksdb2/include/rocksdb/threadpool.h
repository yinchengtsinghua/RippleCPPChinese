
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

#include <functional>

namespace rocksdb {

/*
 *threadpool是一个组件，它将生成n个后台线程，这些线程将
 *用于执行计划的工作，后台线程的数量可以
 *通过调用BackgroundThreads（）进行修改。
 **/

class ThreadPool {
 public:
  virtual ~ThreadPool() {}

//等待所有线程完成。
//丢弃那些没有启动的线程
//执行
  virtual void JoinAllThreads() = 0;

//设置将要执行的后台线程数
//计划作业。
  virtual void SetBackgroundThreads(int num) = 0;
  virtual int GetBackgroundThreads() = 0;

//获取在线程池队列中计划的作业数。
  virtual unsigned int GetQueueLen() const = 0;

//等待所有作业完成这些作业
//已经开始运行而那些没有
//开始吧。这确保了所有被抛出的
//在TP上运行，即使
//我们可能没有为这个数量指定足够的线程
//乔布斯
  virtual void WaitForJobsAndJoinAllThreads() = 0;

//放火忘工作
//这允许多次提交同一作业
  virtual void SubmitJob(const std::function<void()>&) = 0;
//这就提高了功能的效率
  virtual void SubmitJob(std::function<void()>&&) = 0;

};

//newThreadPool（）是一个可用于创建ThreadPool的函数
//使用'num_threads'后台线程。
extern ThreadPool* NewThreadPool(int num_threads);

}  //命名空间rocksdb
