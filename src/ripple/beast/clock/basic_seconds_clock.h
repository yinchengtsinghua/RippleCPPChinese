
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_CHRONO_BASIC_SECONDS_CLOCK_H_INCLUDED
#define BEAST_CHRONO_BASIC_SECONDS_CLOCK_H_INCLUDED

#include <ripple/basics/date.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace beast {

namespace detail {

class seconds_clock_worker
{
public:
    virtual void sample () = 0;
    virtual ~seconds_clock_worker() = default;
    seconds_clock_worker() = default;
    seconds_clock_worker(seconds_clock_worker const&) = delete;
    seconds_clock_worker& operator=(seconds_clock_worker const&) = delete;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//更新时钟
class seconds_clock_thread
{
public:
    using mutex = std::mutex;
    using cond_var = std::condition_variable;
    using lock_guard = std::lock_guard <mutex>;
    using unique_lock = std::unique_lock <mutex>;
    using clock_type = std::chrono::steady_clock;
    using seconds = std::chrono::seconds;
    using thread = std::thread;
    using workers = std::vector <seconds_clock_worker*>;

    bool stop_;
    mutex mutex_;
    cond_var cond_;
    workers workers_;
    thread thread_;

    seconds_clock_thread ()
        : stop_ (false)
    {
        thread_ = thread (&seconds_clock_thread::run, this);
    }

    ~seconds_clock_thread ()
    {
        stop();
    }

    void add (seconds_clock_worker& w)
    {
        lock_guard lock (mutex_);
        workers_.push_back (&w);
    }

    void remove (seconds_clock_worker& w)
    {
        lock_guard lock (mutex_);
        workers_.erase (std::find (
            workers_.begin (), workers_.end(), &w));
    }

    void stop()
    {
        if (thread_.joinable())
        {
            {
                lock_guard lock (mutex_);
                stop_ = true;
            }
            cond_.notify_all();
            thread_.join();
        }
    }

    void run()
    {
        unique_lock lock (mutex_);;

        for (;;)
        {
            for (auto iter : workers_)
                iter->sample();

            using namespace std::chrono;
            clock_type::time_point const when (
                date::floor <seconds> (
                    clock_type::now().time_since_epoch()) +
                        seconds (1));

            if (cond_.wait_until (lock, when, [this]{ return stop_; }))
                return;
        }
    }

    static seconds_clock_thread& instance ()
    {
        static seconds_clock_thread singleton;
        return singleton;
    }
};

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*在主出口之前调用以终止实用程序线程。
    这是Visual Studio 2013的解决方案：
    http://connect.microsoft.com/VisualStudio/feedback/details/786016/creating-a-global-c-object-that-used-thread-join-in-its-destructor-causes-a-lockup
    http://stackoverflow.com/questions/10915233/stdthreajoin-hangs-if-called-after-main-exits-when-using-vs2012-rc
**/

inline
void
basic_seconds_clock_main_hook()
{
#ifdef _MSC_VER
    detail::seconds_clock_thread::instance().stop();
#endif
}

/*最小分辨率为一秒钟的钟。

    此类的目的是优化now（）的性能
    成员函数调用。它使用一个专用线程，至少唤醒
    每秒采样一次请求的普通时钟。

    @tparam时钟A型符合这些要求：
        http://en.cppreference.com/w/cpp/concept/clock/时钟
**/

template <class Clock>
class basic_seconds_clock
{
public:
    explicit basic_seconds_clock() = default;

    using rep = typename Clock::rep;
    using period = typename Clock::period;
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;

    static bool const is_steady = Clock::is_steady;

    static time_point now()
    {
//确保螺纹在
//工人，否则我们会在破坏过程中崩溃。
//具有静态存储持续时间的对象。
        struct initializer
        {
            initializer ()
            {
                detail::seconds_clock_thread::instance();
            }
        };
        static initializer init;

        struct worker : detail::seconds_clock_worker
        {
            time_point m_now;
            std::mutex mutex_;

            worker()
                : m_now(Clock::now())
            {
                detail::seconds_clock_thread::instance().add(*this);
            }

            ~worker()
            {
                detail::seconds_clock_thread::instance().remove(*this);
            }

            time_point now()
            {
                std::lock_guard<std::mutex> lock (mutex_);
                return m_now;
            }

            void sample() override
            {
                std::lock_guard<std::mutex> lock (mutex_);
                m_now = Clock::now();
            }
        };

        static worker w;

        return w.now();
    }
};

}

#endif
