
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

#include <ripple/core/Stoppable.h>
#include <cassert>

namespace ripple {

Stoppable::Stoppable (std::string name, RootStoppable& root)
    : m_name (std::move (name))
    , m_root (root)
    , m_child (this)
{
}

Stoppable::Stoppable (std::string name, Stoppable& parent)
    : m_name (std::move (name))
    , m_root (parent.m_root)
    , m_child (this)
{
//不能有停止父级。
    assert (! parent.isStopping());

    parent.m_children.push_front (&m_child);
}

Stoppable::~Stoppable ()
{
//要么我们不能开始，要么孩子们必须停止。
    assert (!m_root.started() || m_childrenStopped);
}

bool Stoppable::isStopping() const
{
    return m_root.isStopping();
}

bool Stoppable::isStopped () const
{
    return m_stopped;
}

bool Stoppable::areChildrenStopped () const
{
    return m_childrenStopped;
}

void Stoppable::stopped ()
{
    std::lock_guard<std::mutex> lk{m_mut};
    m_is_stopping = true;
    m_cv.notify_all();
}

void Stoppable::onPrepare ()
{
}

void Stoppable::onStart ()
{
}

void Stoppable::onStop ()
{
    stopped();
}

void Stoppable::onChildrenStopped ()
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void Stoppable::prepareRecursive ()
{
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->prepareRecursive ();
    onPrepare ();
}

void Stoppable::startRecursive ()
{
    onStart ();
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->startRecursive ();
}

void Stoppable::stopAsyncRecursive (beast::Journal j)
{
    onStop ();

    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->stopAsyncRecursive(j);
}

void Stoppable::stopRecursive (beast::Journal j)
{
//从树的底部向上挡住每个孩子。
//
    for (Children::const_iterator iter (m_children.cbegin ());
        iter != m_children.cend(); ++iter)
        iter->stoppable->stopRecursive (j);

//如果我们到了这里，所有的孩子都停下来了
//
    m_childrenStopped = true;
    onChildrenStopped ();

//现在阻止这个可停止的，直到m_停止被stopped（）设置。
//
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lk{m_mut};
    if (!m_cv.wait_for(lk, 1s, [this]{return m_is_stopping;}))
    {
        if (auto stream = j.error())
            stream << "Waiting for '" << m_name << "' to stop";
        m_cv.wait(lk, [this]{return m_is_stopping;});
    }
    m_stopped = true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

RootStoppable::RootStoppable (std::string name)
    : Stoppable (std::move (name), *this)
{
}

RootStoppable::~RootStoppable ()
{
    using namespace std::chrono_literals;
    jobCounter_.join(m_name.c_str(), 1s, debugLog());
}

bool RootStoppable::isStopping() const
{
    return m_calledStop;
}

void RootStoppable::prepare ()
{
    if (m_prepared.exchange (true) == false)
        prepareRecursive ();
}

void RootStoppable::start ()
{
//礼貌地打电话准备。
    if (m_prepared.exchange (true) == false)
        prepareRecursive ();

    if (m_started.exchange (true) == false)
        startRecursive ();
}

void RootStoppable::stop (beast::Journal j)
{
//必须有一个预先调用才能开始（）。
    assert (m_started);

    if (stopAsync (j))
        stopRecursive (j);
}

bool RootStoppable::stopAsync (beast::Journal j)
{
    bool alreadyCalled;
    {
//尽管m_calledstop是原子的，但我们在
//锁。这将删除一个小的计时窗口，如果
//等待线程正在处理一个虚假的唤醒，而m_调用了stop
//改变状态。
        std::unique_lock<std::mutex> lock (m_);
        alreadyCalled = m_calledStop.exchange (true);
    }
    if (alreadyCalled)
    {
        if (auto stream = j.warn())
            stream << "Stoppable::stop called again";
        return false;
    }

//等待所有飞行中的作业队列作业完成。
    using namespace std::chrono_literals;
    jobCounter_.join (m_name.c_str(), 1s, j);

    c_.notify_all();
    stopAsyncRecursive(j);
    return true;
}

}
