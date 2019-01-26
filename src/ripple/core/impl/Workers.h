
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

#ifndef RIPPLE_CORE_WORKERS_H_INCLUDED
#define RIPPLE_CORE_WORKERS_H_INCLUDED

#include <ripple/core/impl/semaphore.h>
#include <ripple/beast/core/LockFreeStack.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace ripple {

namespace perf
{
    class PerfLog;
}

/*处理任务的一组线程。
**/

class Workers
{
public:
    /*调用以根据需要执行任务。*/
    struct Callback
    {
        virtual ~Callback () = default;
        Callback() = default;
        Callback(Callback const&) = delete;
        Callback& operator=(Callback const&) = delete;

        /*执行一项任务。

            调用是在工人拥有的线程上进行的。这很重要
            只处理回调中的一个任务。各
            对addtask的调用只会导致对processtask的一次调用。

            @param instance工作线程实例。

            @参见workers:：addtask
        **/

        virtual void processTask (int instance) = 0;
    };

    /*创建对象。

        可以选择指定一些初始线程。这个
        默认情况下，每个CPU创建一个线程。

        @param thread name为每个创建的工作线程命名。
    **/

    explicit Workers (Callback& callback,
                      perf::PerfLog& perfLog,
                      std::string const& threadNames = "Worker",
                      int numberOfThreads =
                         static_cast<int>(std::thread::hardware_concurrency()));

    ~Workers ();

    /*检索所需的线程数。

        这只返回请求的活动线程数。如果
        最近有一个对setNumberOfThreads的调用，实际活动的次数
        线程可能暂时不同于上次请求的线程。

        @注意这个函数不是线程安全的。
    **/

    int getNumberOfThreads () const noexcept;

    /*设置所需的线程数。
        @注意这个函数不是线程安全的。
    **/

    void setNumberOfThreads (int numberOfThreads);

    /*暂停所有线程，直到它们暂停。

        如果一个线程正在处理一个任务，它将在任务一完成时立即暂停。
        完成。即使在所有线程之后，仍可能有任务发出信号
        停顿了一下。

        @注意这个函数不是线程安全的。
    **/

    void pauseAllThreadsAndWait ();

    /*添加要执行的任务。

        每次调用addtask最终都会导致调用
        callback:：processtask，除非Workers对象被破坏或
        线程数从不设置为大于零。

        @注意这个函数是线程安全的。
    **/

    void addTask ();

    /*获取当前正在执行的callback:：processtask调用数。
        虽然此函数是线程安全的，但该值可能不会保留
        长期精确。主要用于诊断。
    **/

    int numberOfCurrentlyRunningTasks () const noexcept;

//——————————————————————————————————————————————————————————————

private:
    struct PausedTag
    {
        explicit PausedTag() = default;
    };

    /*工作者在其提供的线程上执行任务。

        这些是国家：

        活动：运行任务处理循环。
        空闲：活动，但在等待任务时被阻止。
        暂停：已阻止等待退出或变为活动状态。
    **/

    class Worker
        : public beast::LockFreeStack <Worker>::Node
        , public beast::LockFreeStack <Worker, PausedTag>::Node
    {
    public:
        Worker (Workers& workers,
            std::string const& threadName,
            int const instance);

        ~Worker ();

        void notify ();

    private:
        void run ();

    private:
        Workers& m_workers;
        std::string const threadName_;
        int const instance_;

        std::thread thread_;
        std::mutex mutex_;
        std::condition_variable wakeup_;
int wakeCount_;               //多少次取消暂停
        bool shouldExit_;
    };

private:
    static void deleteWorkers (beast::LockFreeStack <Worker>& stack);

private:
    Callback& m_callback;
    perf::PerfLog& perfLog_;
std::string m_threadNames;                   //每个线程的名称
std::condition_variable m_cv;                //当所有线程暂停时发出信号
    std::mutex              m_mut;
    bool                    m_allPaused;
semaphore m_semaphore;                       //每个挂起的任务是1个资源
int m_numberOfThreads;                       //我们现在要激活多少个
std::atomic <int> m_activeCount;             //知道什么时候暂停
std::atomic <int> m_pauseCount;              //现在需要暂停多少线程
std::atomic <int> m_runningTaskCount;        //对processtask（）的激活调用数
beast::LockFreeStack <Worker> m_everyone;           //保留所有已创建的员工
beast::LockFreeStack <Worker, PausedTag> m_paused;  //持有刚刚暂停的员工
};

} //野兽

#endif
