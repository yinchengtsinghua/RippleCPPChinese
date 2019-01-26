
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2013 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#include <ripple/app/main/NodeStoreScheduler.h>
#include <cassert>

namespace ripple {

NodeStoreScheduler::NodeStoreScheduler (Stoppable& parent)
    : Stoppable ("NodeStoreScheduler", parent)
{
}

void NodeStoreScheduler::setJobQueue (JobQueue& jobQueue)
{
    m_jobQueue = &jobQueue;
}

void NodeStoreScheduler::onStop ()
{
}

void NodeStoreScheduler::onChildrenStopped ()
{
    assert (m_taskCount == 0);
    stopped ();
}

void NodeStoreScheduler::scheduleTask (NodeStore::Task& task)
{
    ++m_taskCount;
    if (!m_jobQueue->addJob (
        jtWRITE,
        "NodeObject::store",
        [this, &task] (Job&) { doTask(task); }))
    {
//作业没有添加，大概是因为我们要关闭。
//通过同步执行任务进行恢复。
        doTask (task);
    }
}

void NodeStoreScheduler::doTask (NodeStore::Task& task)
{
    task.performScheduledTask ();

//注意：有两种不同的方法
//call stopped（）：onchildrenstopped（）和dotask（）。有一个
//怀疑，只要配置了可停止树
//正确地说，在dotask（）中对stopped（）的调用永远不会发生。
//
//但是，直到我们增强了对怀疑
//正确，我们将保留此代码。
    if ((--m_taskCount == 0) && isStopping() && areChildrenStopped())
        stopped();
}

void NodeStoreScheduler::onFetch (NodeStore::FetchReport const& report)
{
    if (report.wentToDisk)
        m_jobQueue->addLoadEvents (
            report.isAsync ? jtNS_ASYNC_READ : jtNS_SYNC_READ,
                1, report.elapsed);
}

void NodeStoreScheduler::onBatchWrite (NodeStore::BatchWriteReport const& report)
{
    m_jobQueue->addLoadEvents (jtNS_WRITE,
        report.writeCount, report.elapsed);
}

} //涟漪
