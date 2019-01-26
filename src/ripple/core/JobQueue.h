
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

#ifndef RIPPLE_CORE_JOBQUEUE_H_INCLUDED
#define RIPPLE_CORE_JOBQUEUE_H_INCLUDED

#include <ripple/basics/LocalValue.h>
#include <ripple/basics/win32_workaround.h>
#include <ripple/core/JobTypes.h>
#include <ripple/core/JobTypeData.h>
#include <ripple/core/Stoppable.h>
#include <ripple/core/impl/Workers.h>
#include <ripple/json/json_value.h>
#include <boost/coroutine/all.hpp>

namespace ripple {

namespace perf
{
    class PerfLog;
}

class Logs;
struct Coro_create_t
{
    explicit Coro_create_t() = default;
};

/*执行工作的线程池。

    发布的作业将一直运行到完成。

    必须恢复暂停的协同训练，
    然后跑完全程。

    当JobQueue停止时，它将等待所有作业
    完成协同训练。
**/

class JobQueue
    : public Stoppable
    , private Workers::Callback
{
public:
    /*协同程序必须运行到完成。*/
    class Coro : public std::enable_shared_from_this<Coro>
    {
    private:
        detail::LocalValues lvs_;
        JobQueue& jq_;
        JobType type_;
        std::string name_;
        bool running_;
        std::mutex mutex_;
        std::mutex mutex_run_;
        std::condition_variable cv_;
        boost::coroutines::asymmetric_coroutine<void>::pull_type coro_;
        boost::coroutines::asymmetric_coroutine<void>::push_type* yield_;
    #ifndef NDEBUG
        bool finished_ = false;
    #endif

    public:
//私有：在实现中使用
        template <class F>
        Coro(Coro_create_t, JobQueue&, JobType,
            std::string const&, F&&);

//不可复制构造或分配
        Coro(Coro const&) = delete;
        Coro& operator= (Coro const&) = delete;

        ~Coro();

        /*暂停协同程序执行。
            影响：
              协程的堆栈被保存。
              关联的作业线程被释放。
            注：
              关联的作业函数返回。
              如果连续调用而没有相应的post，则为未定义的行为。
        **/

        void yield() const;

        /*安排协同工作执行。
            影响：
              立即返回。
              一个新的作业被安排恢复协同程序的执行。
              当作业运行时，协程的堆栈将恢复并执行
                在协程函数或语句的开头继续
                在上一次要求屈服之后。
            如果在协同程序完成后调用，则为未定义的行为
              返回（与yield（）相反）。
            连续调用post（）或resume（）时的未定义行为
              没有相应的收益。

            @如果将coro的作业添加到jobqueue，则返回true。
        **/

        bool post();

        /*恢复协同执行。
            影响：
               协程从最后一次停止的地方继续执行。
                 使用相同的线程。
            如果在协同程序完成后调用，则为未定义的行为
              返回（与yield（）相反）。
            连续调用resume（）或post（）时的未定义行为
              没有相应的收益。
        **/

        void resume();

        /*如果coro仍然可以运行（尚未返回），则返回true。*/
        bool runnable() const;

        /*一旦调用，CORO就允许在没有断言的情况下提前退出。*/
        void expectEarlyExit();

        /*等待直到coroutine从用户函数返回。*/
        void join();
    };

    using JobFunction = std::function <void(Job&)>;

    JobQueue (beast::insight::Collector::ptr const& collector,
        Stoppable& parent, beast::Journal journal, Logs& logs,
        perf::PerfLog& perfLog);
    ~JobQueue ();

    /*将作业添加到作业队列。

        @param键入作业类型。
        @param name作业的名称。
        @param jobhandler lambda，签名为void（job&）。在执行作业时调用。

        如果jobhandler添加到队列中，@return true。
    **/

    template <typename JobHandler,
        typename = std::enable_if_t<std::is_same<decltype(
            std::declval<JobHandler&&>()(std::declval<Job&>())), void>::value>>
    bool addJob (JobType type,
        std::string const& name, JobHandler&& jobHandler)
    {
        if (auto optionalCountedJob =
            Stoppable::jobCounter().wrap (std::forward<JobHandler>(jobHandler)))
        {
            return addRefCountedJob (
                type, name, std::move (*optionalCountedJob));
        }
        return false;
    }

    /*创建协同工作并将作业添加到将运行它的队列中。

        @param t作业类型。
        @param name作业的名称。
        @param f的签名为void（std:：shared_ptr<coro>）。在作业执行时调用。

        @return shared-ptr到posted-coro。如果post不成功，则返回nullptr。
    **/

    template <class F>
    std::shared_ptr<Coro> postCoro (JobType t, std::string const& name, F&& f);

    /*按此优先级等待的作业。
    **/

    int getJobCount (JobType t) const;

    /*以此优先级等待并运行的作业。
    **/

    int getJobCountTotal (JobType t) const;

    /*所有等待作业的优先级都大于或等于此优先级。
    **/

    int getJobCountGE (JobType t) const;

    /*将为作业队列提供服务的线程数精确地设置为此数。
    **/

    void setThreadCount (int c, bool const standaloneMode);

    /*返回作用域LoadEvent。
    **/

    std::unique_ptr <LoadEvent>
    makeLoadEvent (JobType t, std::string const& name);

    /*添加多个加载事件。
    **/

    void addLoadEvents (JobType t, int count, std::chrono::milliseconds elapsed);

//无法为常量，因为LoadMonitor没有常量方法。
    bool isOverloaded ();

//无法为常量，因为LoadMonitor没有常量方法。
    Json::Value getJson (int c = 0);

    /*阻止，直到没有任务运行。*/
    void
    rendezvous();

private:
    friend class Coro;

    using JobDataMap = std::map <JobType, JobTypeData>;

    beast::Journal m_journal;
    mutable std::mutex m_mutex;
    std::uint64_t m_lastJob;
    std::set <Job> m_jobSet;
    JobDataMap m_jobData;
    JobTypeData m_invalidJobData;

//当前在processtask（）中的作业数
    int m_processCount;

//挂起的协程数
    int nSuspend_ = 0;

    Workers m_workers;
    Job::CancelCallback m_cancelCallback;

//统计数据跟踪
    perf::PerfLog& perfLog_;
    beast::insight::Collector::ptr m_collector;
    beast::insight::Gauge job_count;
    beast::insight::Hook hook;

    std::condition_variable cv_;

    void collect();
    JobTypeData& getJobTypeData (JobType type);

    void onStop() override;

//如果满足停止条件，则向服务发出停止的信号。
    void checkStopped (std::lock_guard <std::mutex> const& lock);

//将引用计数的作业添加到作业队列。
//
//param键入作业类型。
//参数名称作业的名称。
//param func std：：带有签名void的函数（job&）。在执行作业时调用。
//
//如果func添加到队列，则返回true。
    bool addRefCountedJob (
        JobType type, std::string const& name, JobFunction const& func);

//表示要处理的附加作业。
//
//先决条件：
//JobType必须有效。
//该工作必须存在于mjobut中。
//作业以前一定没有排队。
//
//岗位条件：
//该类型的等待作业计数将增加。
//如果JobQueue存在，并且至少有一个线程，则作业最终将运行。
//
//Invariants：
//调用线程拥有joblock
    void queueJob (Job const& job, std::lock_guard <std::mutex> const& lock);

//返回我们现在应该运行的下一个作业。
//
//RunnableJob：
//作业集中的作业，其类型的槽数大于零。
//
//先决条件：
//mjobut不能为空。
//mjobut至少拥有一个可运行的作业
//
//岗位条件：
//作业是有效的作业对象。
//作业从mjobqueue中删除。
//其类型的等待作业计数递减
//其类型的运行作业计数递增
//
//Invariants：
//调用线程拥有joblock
    void getNextJob (Job& job);

//指示正在运行的作业已完成其任务。
//
//先决条件：
//在mjobut中不能存在作业。
//JobType不能无效。
//
//岗位条件：
//该作业类型的运行计数将递减
//如果有超过限制的等待作业（如果有），则会发出新任务的信号。
//
//Invariants：
//<无>
    void finishJob (JobType type);

//运行下一个适当的等待作业。
//
//先决条件：
//作业集中必须存在可运行作业
//
//岗位条件：
//所选的runnablejob将调用job:：dojob（）。
//
//Invariants：
//<无>
    void processTask (int instance) override;

//返回给定作业类型运行作业的限制。
//对于没有限制的工作，我们返回最大的int。希望
//就够了。
    int getJobLimit (JobType type);

    void onChildrenStopped () override;
};

/*
    接收到一个rpc命令，并通过serverhandler（http）或
    处理程序（WebSocket），具体取决于连接类型。然后，处理程序调用
    jobqueue:：postcoro（）方法创建协同程序并在以后运行它
    点。这将释放处理程序线程并允许它继续处理
    当rpc命令异步完成其工作时的其他请求。

    postcoro（）创建一个coro对象。当调用coro ctor时，
    coro_u成员已初始化（boost:：coroutines:：pull_类型），执行
    自动传递到协程，我们现在不想这样做，
    因为我们仍然在处理程序线程上下文中。重要的是要注意这里
    Boost Pull_类型的构造自动将执行传递给
    协同程序。pull类型的对象自动生成push类型，即
    在函数的签名中作为参数（do_yield）传递
    Pull_类型是用创建的。此函数在coro\u期间立即调用
    结构和内部，coro:：yield_u被分配为push_类型
    参数（do-yield）address和called（yield（））以便返回执行
    回到调用者的堆栈。

    post coro（）然后调用coro:：post（），它在作业上调度作业。
    队列以继续执行作业队列工作线程中的协同程序
    过一会儿。当作业运行时，我们锁定coro:：mutex并调用
    在我们离开的地方继续前进。自从我们做了最后一件事
    在coro_uuwas call yield（）中，接下来我们继续使用的是调用
    函数参数f，它被传递到coro ctor。就在这里面
    函数体，调用方指定在
    在协同程序中运行并允许它们暂停和恢复执行。
    依赖其他事件完成的任务，如路径查找、调用
    coro:：yield（）在等待这些事件时暂停其执行
    通过coro:：post（）方法发出信号时完成并继续。

    这里存在一个潜在的争用条件，post（）可以从中获取
    在yield（）之前调用，在f之后调用。从技术上讲，问题只发生在
    如果在调用yield（）之前执行了post（）调度的作业。
    如果要在yield（）之前执行post（）作业，则未定义的行为
    会发生。锁确保在我们退出之前不会再次调用coro_u
    协同程序。此时，计划的resume（）作业正在等待锁定
    会得到进入，无害地打电话给科罗，然后立即返回
    已经完成协程。

    比赛条件如下：

        1-连体衣在跑。
        2-协同程序即将挂起，但在它挂起之前，必须
            安排一些活动来唤醒它。
        3-协同训练安排了一些活动来唤醒它。
        4-在协程暂停之前，该事件发生并恢复
            在作业队列中调度协程的。
        再次，在协程线暂停之前，协程线的恢复
            被派遣。
        6-同样，在协程可以挂起之前，恢复代码运行
            协同程序。
        协同程序现在运行在两个线程中。

        锁可以防止这种情况发生，因为步骤6将阻止直到
            锁被释放，这只在协程完成后发生。
**/


} //涟漪

#include <ripple/core/Coro.ipp>

namespace ripple {

template <class F>
std::shared_ptr<JobQueue::Coro>
JobQueue::postCoro (JobType t, std::string const& name, F&& f)
{
    /*第一个参数是使构造私有化的详细信息类型。
        最后一个参数是协同程序运行的函数。签名
        void（std:：shared_ptr<coro>）。
    **/

    auto coro = std::make_shared<Coro>(
        Coro_create_t{}, *this, t, name, std::forward<F>(f));
    if (! coro->post())
    {
//CORO未成功发布。禁用它，使其成为析构函数
//可以在没有负面影响的情况下运行。然后摧毁它。
        coro->expectEarlyExit();
        coro.reset();
    }
    return coro;
}

}

#endif
