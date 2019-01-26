
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

#include <ripple/core/impl/Workers.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <cassert>

namespace ripple {

Workers::Workers (
    Callback& callback,
    perf::PerfLog& perfLog,
    std::string const& threadNames,
    int numberOfThreads)
        : m_callback (callback)
        , perfLog_ (perfLog)
        , m_threadNames (threadNames)
        , m_allPaused (true)
        , m_semaphore (0)
        , m_numberOfThreads (0)
        , m_activeCount (0)
        , m_pauseCount (0)
        , m_runningTaskCount (0)
{
    setNumberOfThreads (numberOfThreads);
}

Workers::~Workers ()
{
    pauseAllThreadsAndWait ();

    deleteWorkers (m_everyone);
}

int Workers::getNumberOfThreads () const noexcept
{
    return m_numberOfThreads;
}

//vfalc注意：如果快速调用此函数以减少
//增加线程数，可能会导致
//创建的暂停线程比预期的多。
//
void Workers::setNumberOfThreads (int numberOfThreads)
{
    static int instance {0};
    if (m_numberOfThreads != numberOfThreads)
    {
        perfLog_.resizeJobs(numberOfThreads);

        if (numberOfThreads > m_numberOfThreads)
        {
//增加工作线程的数量
            int const amount = numberOfThreads - m_numberOfThreads;

            for (int i = 0; i < amount; ++i)
            {
//看看我们是否可以重用一个暂停的工作者
                Worker* worker = m_paused.pop_front ();

                if (worker != nullptr)
                {
//如果我们到了这里，工作线程就在[1]
//这将取消阻止他们的呼叫等待（）。
//
                    worker->notify ();
                }
                else
                {
                    worker = new Worker (*this, m_threadNames, instance++);
                    m_everyone.push_front (worker);
                }
            }
        }
        else
        {
//减少工作线程数
            int const amount = m_numberOfThreads - numberOfThreads;

            for (int i = 0; i < amount; ++i)
            {
                ++m_pauseCount;

//暂停线程算作一个“内部任务”
                m_semaphore.notify ();
            }
        }

        m_numberOfThreads = numberOfThreads;
    }
}

void Workers::pauseAllThreadsAndWait ()
{
    setNumberOfThreads (0);

    std::unique_lock<std::mutex> lk{m_mut};
    m_cv.wait(lk, [this]{return m_allPaused;});
    lk.unlock();

    assert (numberOfCurrentlyRunningTasks () == 0);
}

void Workers::addTask ()
{
    m_semaphore.notify ();
}

int Workers::numberOfCurrentlyRunningTasks () const noexcept
{
    return m_runningTaskCount.load ();
}

void Workers::deleteWorkers (beast::LockFreeStack <Worker>& stack)
{
    for (;;)
    {
        Worker* const worker = stack.pop_front ();

        if (worker != nullptr)
        {
//此调用将一直阻塞，直到线程有序退出
            delete worker;
        }
        else
        {
            break;
        }
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Workers::Worker::Worker (Workers& workers, std::string const& threadName,
    int const instance)
    : m_workers {workers}
    , threadName_ {threadName}
    , instance_ {instance}
    , wakeCount_ {0}
    , shouldExit_ {false}
{
    thread_ = std::thread {&Workers::Worker::run, this};
}

Workers::Worker::~Worker ()
{
    {
        std::lock_guard <std::mutex> lock {mutex_};
        ++wakeCount_;
        shouldExit_ = true;
    }

    wakeup_.notify_one();
    thread_.join();
}

void Workers::Worker::notify ()
{
    std::lock_guard <std::mutex> lock {mutex_};
    ++wakeCount_;
    wakeup_.notify_one();
}

void Workers::Worker::run ()
{
    bool shouldExit = true;
    do
    {
//增加活跃工人的数量，如果
//我们是第一个，然后重置“全部暂停”事件
//
        if (++m_workers.m_activeCount == 1)
        {
            std::lock_guard<std::mutex> lk{m_workers.m_mut};
            m_workers.m_allPaused = false;
        }

        for (;;)
        {
//如果回调更改了名称，请将其放回原处
            beast::setCurrentThreadName (threadName_);

//获取任务或“内部任务”。
//
            m_workers.m_semaphore.wait ();

//看看是否有暂停请求。这个
//算作“内部任务”。
//
            int pauseCount = m_workers.m_pauseCount.load ();

            if (pauseCount > 0)
            {
//尝试减量
                pauseCount = --m_workers.m_pauseCount;

                if (pauseCount >= 0)
                {
//我们停顿了一下
                    break;
                }
                else
                {
//撤销我们的法令
                    ++m_workers.m_pauseCount;
                }
            }

//我们不能停下来，所以我们必须
//为了处理任务而取消阻止。
//
            ++m_workers.m_runningTaskCount;
            m_workers.m_callback.processTask (instance_);
            --m_workers.m_runningTaskCount;
        }

//进入暂停列表的任何工作人员必须
//保证它最终会阻止
//事件对象。
//
        m_workers.m_paused.push_front (this);

//减少活跃工人的数量，如果我们
//是最后一个发出“全部暂停”信号的事件。
//
        if (--m_workers.m_activeCount == 0)
        {
            std::lock_guard<std::mutex> lk{m_workers.m_mut};
            m_workers.m_allPaused = true;
            m_workers.m_cv.notify_all();
        }

//设置非活动线程名称。
        beast::setCurrentThreadName ("(" + threadName_ + ")");

//[1]当弹出暂停列表时，我们将在这里
//
//我们根据自己的条件阻止变量唤醒
//放入暂停列表。
//
//唤醒将由worker:：notify（）或~worker发出信号。
        {
            std::unique_lock <std::mutex> lock {mutex_};
            wakeup_.wait (lock, [this] {return this->wakeCount_ > 0;});

            shouldExit = shouldExit_;
            --wakeCount_;
        }
    } while (! shouldExit);
}

} //涟漪
