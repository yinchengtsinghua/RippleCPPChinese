
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#ifndef RIPPLE_CORE_STOPPABLE_H_INCLUDED
#define RIPPLE_CORE_STOPPABLE_H_INCLUDED

#include <ripple/beast/core/LockFreeStack.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/core/Job.h>
#include <ripple/core/ClosureCounter.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace ripple {

//给工作台起个合理的名字
using JobCounter = ClosureCounter<void, Job&>;

class RootStoppable;

/*提供启动和停止的接口。

    构建服务器或对等代码的一种常见方法是隔离
    将功能的概念部分聚合到单个类中
    一些较大的“应用程序”或“核心”对象，其中包含所有部分。
    通常，这些组成部分不可避免地相互依赖
    复杂的方法。它们还经常使用线程并执行异步I/O
    涉及套接字或其他操作系统对象的操作。过程
    启动和停止这样一个系统可能是复杂的。此接口
    提供一组行为以确保
    复合应用程序样式对象定义良好。

    初始化复合对象后，将执行以下步骤：

    1。构建子组件。

        这些都是典型的从可停止衍生出来的。可能有一个很深的
        层次结构：可停止的对象本身可能有可停止的子级
        物体。这捕获依赖关系。

    2。准备（）

        由于某些组件可能依赖于其他组件，因此准备步骤需要
        所有的对象都是首先构造的。准备步骤调用所有
        树上可停止的物体从树叶开始向上运动
        根部。在此阶段，我们保证所有对象
        构造并处于定义良好的状态。

    三。准备（）

        对层次结构中的所有可停止对象调用此重写
        在准备阶段。保证所有儿童都能停止
        调用时已准备好对象。

        对象首先称为子对象。

    4。START（）

        在这一点上，所有的子组件都已经建造和准备好了，
        因此，启动它们应该是安全的。有些是可以阻止的
        对象的启动功能可能不起作用，其他对象将启动
        线程或调用异步I/O初始化函数，如计时器或
        插座。

    5。OnStand（）

        对层次结构中的所有可停止对象调用此重写
        在开始阶段。保证儿童不可停止
        对象已在调用时启动。

        对象首先称为父对象。

    这是涉及停止的事件序列：

    6。stopasync（）[可选]

        这将通知根stoppable及其所有子级stop是
        请求。

    7。停止（）

        这首先调用stopasync（），然后在
        从树的底部向上，直到停止指示它
        停止。这通常从执行的主线程调用
        当一些外部信号指示进程应该停止时。为了
        例如，rpc“stop”命令或sigint posix信号。

    8。on（）

        当发生以下情况时，将为根stoppable及其所有子级调用此重写：
        调用了StopAsync（）。派生类应取消挂起的I/O和
        计时器、线程应退出的信号、队列清理作业和执行
        为准备退出而采取的任何其他必要的最终行动。

        对象首先称为父对象。

    9。一个孩子被阻止了（））

        当所有子级都已停止时调用此重写。这个通知
        不应该再有任何家属打电话的可阻止性
        它的成员函数。没有孩子的可阻挡物
        调用此函数。

        对象首先称为子对象。

    10。停止（）

        派生类调用此函数以通知可停止的API
        它已经完成了停车。这将取消阻止stop（）的调用方。

        对于只有当所有的
        孩子们已经停止了，他们自己的内部逻辑指示停止，它
        需要在onchildrenstopped（）中执行特殊操作。这个
        函数areChildrenStopped（）可在子级停止后使用，
        但在可停止逻辑本身停止之前，要确定
        可停止的逻辑是真正的停止。

        此过程的伪代码如下：

        @代码

        //如果派生逻辑已停止，则返回“true”。
        / /
        //当逻辑停止时，不再调用logicprocessingstop（）。
        //如果孩子们仍然活跃，我们需要等到我们
        //通知子级已停止。
        / /
        bool logichassstopped（）；

        //儿童停止时调用
        未阻止时无效（）。
        {
            //当派生逻辑停止，子逻辑停止时，我们已经停止。
            if（逻辑已停止）
                停止（）；
        }

        //定期执行的派生特定逻辑
        void logicprocessingstep（）。
        {
            /工艺
            /…

            //现在看看我们是否已经停止
            if（logichassstopped（）&&arechildrenstopped（））
                停止（）；
        }

        @端码

        管理一个或多个线程的派生类通常应通知
        上面的线程应该退出。在thread函数中，
        当最后一个线程即将退出时，它将调用stopped（）。

    @注意，不能重新启动可停止。

    波纹应用程序中可停止树的形式演变为
    源代码会更改并对新的需求做出反应。截至2017年3月
    可停止树的形式如下：

    @代码

                                   应用
                                        γ
                   +----------------+-----------+
                   |                    |                    |
              LoadManager shamapstore节点目录调度程序
                                                             γ
                                                         作业队列
                                                             γ
        +------+------+------+------+------+------+------+
        __
        网络运营收件箱订单簿数据库
        |                       |                       |
     覆盖InboundTransactions LedgerMaster
        _
    Peerfinder壁架清洁器

    @端码
**/

/*@ {*/
class Stoppable
{
protected:
    Stoppable (std::string name, RootStoppable& root);

public:
    /*创建可停止的。*/
    Stoppable (std::string name, Stoppable& parent);

    /*摧毁停止装置。*/
    virtual ~Stoppable ();

    /*如果stoppable应停止，则返回“true”。*/
    bool isStopping () const;

    /*如果请求的停止操作已完成，则返回“true”。*/
    bool isStopped () const;

    /*如果所有子级都已停止，则返回“true”。*/
    bool areChildrenStopped () const;

    /*JobQueue使用此方法进行作业计数。*/
    inline JobCounter& jobCounter ();

    /*停下来睡觉或醒来。

        @return`true`如果我们停止
    **/

    bool
    alertable_sleep_until(
        std::chrono::system_clock::time_point const& t);

protected:
    /*由派生类调用以指示可停止已停止。*/
    void stopped ();

private:
    /*准备期间调用了重写。
        因为树中所有其他可停止的对象
        构造，这提供了执行初始化的机会，
        取决于调用其他可停止对象。
        此调用是在调用Prepare（）的同一线程上进行的。
        默认实现什么也不做。
        保证只打一次电话。
    **/

    virtual void onPrepare ();

    /*在启动期间调用了重写。*/
    virtual void onStart ();

    /*发出停止通知时调用的重写。

        调用是在一个未指定的、特定于实现的线程上进行的。
        OnStop和OnChildrenstopped永远不会被同时调用，跨越
        所有可停止对象从同一根派生，包括
        根。

        打电话给IsStopping、IsStopping和Arechildren是安全的。
        在此函数中；返回的值将始终有效且从不
        在回调过程中更改。

        默认实现只调用stopped（）。这是适用的
        当可停止操作具有轻微停止操作（或无停止操作）时，
        我们只是使用可停止的API将其定位为依赖项
        一些父服务。

        线程安全：
            可能不会长时间阻塞。
            保证只打一次电话。
            任何时候从任何线程调用都必须是安全的。
    **/

    virtual void onStop ();

    /*当所有子级都停止时调用重写。

        调用是在一个未指定的、特定于实现的线程上进行的。
        OnStop和OnChildrenstopped永远不会被同时调用，跨越
        所有可停止对象从同一根派生，包括
        根。

        打电话给IsStopping、IsStopping和Arechildren是安全的。
        在此函数中；返回的值将始终有效且从不
        在回调过程中更改。

        默认实现什么也不做。

        线程安全：
            可能不会长时间阻塞。
            保证只打一次电话。
            任何时候从任何线程调用都必须是安全的。
    **/

    virtual void onChildrenStopped ();

    friend class RootStoppable;

    struct Child;
    using Children = beast::LockFreeStack <Child>;

    struct Child : Children::Node
    {
        Child (Stoppable* stoppable_) : stoppable (stoppable_)
        {
        }

        Stoppable* stoppable;
    };

    void prepareRecursive ();
    void startRecursive ();
    void stopAsyncRecursive (beast::Journal j);
    void stopRecursive (beast::Journal j);

    std::string m_name;
    RootStoppable& m_root;
    Child m_child;
    std::atomic<bool> m_stopped {false};
    std::atomic<bool> m_childrenStopped {false};
    Children m_children;
    std::condition_variable m_cv;
    std::mutex              m_mut;
    bool                    m_is_stopping = false;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class RootStoppable : public Stoppable
{
public:
    explicit RootStoppable (std::string name);

    ~RootStoppable ();

    bool isStopping() const;

    /*准备所有包含的可停止物体。
        这将调用onPrepare来处理树中所有可停止的对象。
        在第一次呼叫后进行的呼叫无效。
        线程安全：
            可以从任何线程调用。
    **/

    void prepare ();

    /*启动所有包含的可停止对象。
        默认实现什么也不做。
        在第一次呼叫后进行的呼叫无效。
        线程安全：
            可以从任何线程调用。
    **/

    void start ();

    /*通知根停止和子停止，并阻止直到停止。
        如果已通知可停止，则无效。
        直到停止及其所有子项停止为止。
        如果在没有先前调用的情况下调用stop（），将导致未定义的行为结果
        开始（）。
        线程安全：
            可以安全地从与可停止线程无关的任何线程调用。
    **/

    void stop (beast::Journal j);

    /*如果曾经调用Start（），则返回true。*/
    bool started () const
    {
        return m_started;
    }

    /*JobQueue使用此方法进行作业计数。*/
    JobCounter& rootJobCounter ()
    {
        return jobCounter_;
    }

    /*停下来睡觉或醒来。

        @return`true`如果我们停止
    **/

    bool
    alertable_sleep_until(
        std::chrono::system_clock::time_point const& t);

private:
    /*通知一个可停止的根目录和子目录停止，不要等待。
        如果已通知可停止，则无效。

        第一次调用此方法时返回true，否则返回false。

        线程安全：
            随时可以从任何线程安全调用。
    **/

    bool stopAsync(beast::Journal j);

    std::atomic<bool> m_prepared {false};
    std::atomic<bool> m_started {false};
    std::atomic<bool> m_calledStop {false};
    std::mutex m_;
    std::condition_variable c_;
    JobCounter jobCounter_;
};
/*@ }*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

JobCounter& Stoppable::jobCounter ()
{
    return m_root.rootJobCounter();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline
bool
RootStoppable::alertable_sleep_until(
    std::chrono::system_clock::time_point const& t)
{
    std::unique_lock<std::mutex> lock(m_);
    if (m_calledStop)
        return true;
    return c_.wait_until(lock, t, [this]{return m_calledStop.load();});
}

inline
bool
Stoppable::alertable_sleep_until(
    std::chrono::system_clock::time_point const& t)
{
    return m_root.alertable_sleep_until(t);
}

} //涟漪

#endif
