
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

#include <ripple/core/JobQueue.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/PerfLog.h>

namespace ripple {

JobQueue::JobQueue (beast::insight::Collector::ptr const& collector,
    Stoppable& parent, beast::Journal journal, Logs& logs,
    perf::PerfLog& perfLog)
    : Stoppable ("JobQueue", parent)
    , m_journal (journal)
    , m_lastJob (0)
    , m_invalidJobData (JobTypes::instance().getInvalid (), collector, logs)
    , m_processCount (0)
    , m_workers (*this, perfLog, "JobQueue", 0)
    , m_cancelCallback (std::bind (&Stoppable::isStopping, this))
    , perfLog_ (perfLog)
    , m_collector (collector)
{
    hook = m_collector->make_hook (std::bind (&JobQueue::collect, this));
    job_count = m_collector->make_gauge ("job_count");

    {
        std::lock_guard <std::mutex> lock (m_mutex);

        for (auto const& x : JobTypes::instance())
        {
            JobTypeInfo const& jt = x.second;

//为所有工作创建动态信息
            auto const result (m_jobData.emplace (std::piecewise_construct,
                std::forward_as_tuple (jt.type ()),
                std::forward_as_tuple (jt, m_collector, logs)));
            assert (result.second == true);
            (void) result.second;
        }
    }
}

JobQueue::~JobQueue ()
{
//必须在销毁前解开
    hook = beast::insight::Hook ();
}

void
JobQueue::collect ()
{
    std::lock_guard <std::mutex> lock (m_mutex);
    job_count = m_jobSet.size ();
}

bool
JobQueue::addRefCountedJob (JobType type, std::string const& name,
    JobFunction const& func)
{
    assert (type != jtINVALID);

    auto iter (m_jobData.find (type));
    assert (iter != m_jobData.end ());
    if (iter == m_jobData.end ())
        return false;

    JobTypeData& data (iter->second);

//修正：解决不正确的客户端关闭命令
//不要将作业添加到没有线程的队列中
    assert (type == jtCLIENT || m_workers.getNumberOfThreads () > 0);

    {
        std::lock_guard <std::mutex> lock (m_mutex);

//如果这个消失了，就意味着一个孩子没有跟上
//可停止API规则。只有在以下情况下才能添加作业：
//
//-作业队列未停止
//和
//*我们目前正在处理工作
//或
//*我们还有待处理的工作
//或
//*并非所有儿童都停止
//
        assert (! isStopped() && (
            m_processCount>0 ||
            ! m_jobSet.empty () ||
            ! areChildrenStopped()));

        std::pair <std::set <Job>::iterator, bool> result (
            m_jobSet.insert (Job (type, name, ++m_lastJob,
                data.load (), func, m_cancelCallback)));
        queueJob (*result.first, lock);
    }
    return true;
}

int
JobQueue::getJobCount (JobType t) const
{
    std::lock_guard <std::mutex> lock (m_mutex);

    JobDataMap::const_iterator c = m_jobData.find (t);

    return (c == m_jobData.end ())
        ? 0
        : c->second.waiting;
}

int
JobQueue::getJobCountTotal (JobType t) const
{
    std::lock_guard <std::mutex> lock (m_mutex);

    JobDataMap::const_iterator c = m_jobData.find (t);

    return (c == m_jobData.end ())
        ? 0
        : (c->second.waiting + c->second.running);
}

int
JobQueue::getJobCountGE (JobType t) const
{
//返回此优先级或更高级别的作业数
    int ret = 0;

    std::lock_guard <std::mutex> lock (m_mutex);

    for (auto const& x : m_jobData)
    {
        if (x.first >= t)
            ret += x.second.waiting;
    }

    return ret;
}

void
JobQueue::setThreadCount (int c, bool const standaloneMode)
{
    if (standaloneMode)
    {
        c = 1;
    }
    else if (c == 0)
    {
        c = static_cast<int>(std::thread::hardware_concurrency());
c = 2 + std::min(c, 4); //I/O将成为瓶颈
        JLOG (m_journal.info()) << "Auto-tuning to " << c <<
                            " validation/transaction/proposal threads.";
    }
    else
    {
        JLOG (m_journal.info()) << "Configured " << c <<
                            " validation/transaction/proposal threads.";
    }

    m_workers.setNumberOfThreads (c);
}

std::unique_ptr<LoadEvent>
JobQueue::makeLoadEvent (JobType t, std::string const& name)
{
    JobDataMap::iterator iter (m_jobData.find (t));
    assert (iter != m_jobData.end ());

    if (iter == m_jobData.end ())
        return {};

    return std::make_unique<LoadEvent> (iter-> second.load (), name, true);
}

void
JobQueue::addLoadEvents (JobType t, int count,
    std::chrono::milliseconds elapsed)
{
    if (isStopped())
        LogicError ("JobQueue::addLoadEvents() called after JobQueue stopped");

    JobDataMap::iterator iter (m_jobData.find (t));
    assert (iter != m_jobData.end ());
    iter->second.load().addSamples (count, elapsed);
}

bool
JobQueue::isOverloaded ()
{
    int count = 0;

    for (auto& x : m_jobData)
    {
        if (x.second.load ().isOver ())
            ++count;
    }

    return count > 0;
}

Json::Value
JobQueue::getJson (int c)
{
    using namespace std::chrono_literals;
    Json::Value ret (Json::objectValue);

    ret["threads"] = m_workers.getNumberOfThreads ();

    Json::Value priorities = Json::arrayValue;

    std::lock_guard <std::mutex> lock (m_mutex);

    for (auto& x : m_jobData)
    {
        assert (x.first != jtINVALID);

        if (x.first == jtGENERIC)
            continue;

        JobTypeData& data (x.second);

        LoadMonitor::Stats stats (data.stats ());

        int waiting (data.waiting);
        int running (data.running);

        if ((stats.count != 0) || (waiting != 0) ||
            (stats.latencyPeak != 0ms) || (running != 0))
        {
            Json::Value& pri = priorities.append (Json::objectValue);

            pri["job_type"] = data.name ();

            if (stats.isOverloaded)
                pri["over_target"] = true;

            if (waiting != 0)
                pri["waiting"] = waiting;

            if (stats.count != 0)
                pri["per_second"] = static_cast<int> (stats.count);

            if (stats.latencyPeak != 0ms)
                pri["peak_time"] = static_cast<int> (stats.latencyPeak.count());

            if (stats.latencyAvg != 0ms)
                pri["avg_time"] = static_cast<int> (stats.latencyAvg.count());

            if (running != 0)
                pri["in_progress"] = running;
        }
    }

    ret["job_types"] = priorities;

    return ret;
}

void
JobQueue::rendezvous()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    cv_.wait(lock, [&]
    {
        return m_processCount == 0 &&
            m_jobSet.empty();
    });
}

JobTypeData&
JobQueue::getJobTypeData (JobType type)
{
    JobDataMap::iterator c (m_jobData.find (type));
    assert (c != m_jobData.end ());

//尼克：这很难看，我讨厌它。我们必须彻底清除JTINVAL
//用理智的东西。
    if (c == m_jobData.end ())
        return m_invalidJobData;

    return c->second;
}

void
JobQueue::onStop()
{
//OnStop必须在此处定义为空，
//否则，基类将做错误的事情。
}

void
JobQueue::checkStopped (std::lock_guard <std::mutex> const& lock)
{
//当以下所有条件都成立时，我们将停止：
//
//1。收到停止通知
//2。所有停止的孩子都停止了
//三。没有对processtask的执行调用
//4。作业集中没有剩余作业
//5。没有悬空的协同训练
//
    if (isStopping() &&
        areChildrenStopped() &&
        (m_processCount == 0) &&
        m_jobSet.empty() &&
        nSuspend_ == 0)
    {
        stopped();
    }
}

void
JobQueue::queueJob (Job const& job, std::lock_guard <std::mutex> const& lock)
{
    JobType const type (job.getType ());
    assert (type != jtINVALID);
    assert (m_jobSet.find (job) != m_jobSet.end ());
    perfLog_.jobQueue(type);

    JobTypeData& data (getJobTypeData (type));

    if (data.waiting + data.running < getJobLimit (type))
    {
        m_workers.addTask ();
    }
    else
    {
//把任务推迟到低于限度为止
//
        ++data.deferred;
    }
    ++data.waiting;
}

void
JobQueue::getNextJob (Job& job)
{
    assert (! m_jobSet.empty ());

    std::set <Job>::const_iterator iter;
    for (iter = m_jobSet.begin (); iter != m_jobSet.end (); ++iter)
    {
        JobTypeData& data (getJobTypeData (iter->getType ()));

        assert (data.running <= getJobLimit (data.type ()));

//如果我们运行低于限制，请运行此作业。
        if (data.running < getJobLimit (data.type ()))
        {
            assert (data.waiting > 0);
            break;
        }
    }

    assert (iter != m_jobSet.end ());

    JobType const type = iter->getType ();
    JobTypeData& data (getJobTypeData (type));

    assert (type != jtINVALID);

    job = *iter;
    m_jobSet.erase (iter);

    --data.waiting;
    ++data.running;
}

void
JobQueue::finishJob (JobType type)
{
    assert(type != jtINVALID);

    JobTypeData& data = getJobTypeData (type);

//如果可能，将延迟的任务排队
    if (data.deferred > 0)
    {
        assert (data.running + data.waiting >= getJobLimit (type));

        --data.deferred;
        m_workers.addTask ();
    }

    --data.running;
}

void
JobQueue::processTask (int instance)
{
    JobType type;

    {
        using namespace std::chrono;
        Job::clock_type::time_point const start_time (
            Job::clock_type::now());
        {
            Job job;
            {
                std::lock_guard <std::mutex> lock (m_mutex);
                getNextJob (job);
                ++m_processCount;
            }
            type = job.getType();
            JobTypeData& data(getJobTypeData(type));
            JLOG(m_journal.trace()) << "Doing " << data.name () << " job";
            auto const us = date::ceil<microseconds>(
                start_time - job.queue_time());
            perfLog_.jobStart(type, us, start_time, instance);
            if (us >= 10ms)
                getJobTypeData(type).dequeue.notify(us);
            job.doJob ();
        }
        auto const us (
            date::ceil<microseconds>(Job::clock_type::now() - start_time));
        perfLog_.jobFinish(type, us, instance);
        if (us >= 10ms)
            getJobTypeData(type).execute.notify(us);
    }

    {
        std::lock_guard <std::mutex> lock (m_mutex);
//应在调用checkstopped之前销毁作业
//否则具有副作用的析构函数可以访问
//已销毁的父对象。
        finishJob (type);
        if(--m_processCount == 0 && m_jobSet.empty())
            cv_.notify_all();
        checkStopped (lock);
    }

//注意，当调用job:：~ job时，最后一个引用
//关联的loadEvent对象（在作业中）可能会被销毁。
}

int
JobQueue::getJobLimit (JobType type)
{
    JobTypeInfo const& j (JobTypes::instance().get (type));
    assert (j.type () != jtINVALID);

    return j.limit ();
}

void
JobQueue::onChildrenStopped ()
{
    std::lock_guard <std::mutex> lock (m_mutex);
    checkStopped (lock);
}

}
