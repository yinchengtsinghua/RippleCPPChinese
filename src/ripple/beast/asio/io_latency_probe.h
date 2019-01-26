
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

#ifndef BEAST_ASIO_IO_LATENCY_PROBE_H_INCLUDED
#define BEAST_ASIO_IO_LATENCY_PROBE_H_INCLUDED

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/config.hpp>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <stdexcept>

namespace beast {

/*测量IO服务队列上的处理程序延迟。*/
template <class Clock>
class io_latency_probe
{
private:
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;

    std::recursive_mutex m_mutex;
    std::condition_variable_any m_cond;
    std::size_t m_count;
    duration const m_period;
    boost::asio::io_service& m_ios;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> m_timer;
    bool m_cancel;

public:
    io_latency_probe (duration const& period,
        boost::asio::io_service& ios)
        : m_count (1)
        , m_period (period)
        , m_ios (ios)
        , m_timer (m_ios)
        , m_cancel (false)
    {
    }

    ~io_latency_probe ()
    {
        std::unique_lock <decltype (m_mutex)> lock (m_mutex);
        cancel (lock, true);
    }

    /*返回与延迟探测器关联的IO_服务。*/
    /*@ {*/
    boost::asio::io_service& get_io_service ()
    {
        return m_ios;
    }

    boost::asio::io_service const& get_io_service () const
    {
        return m_ios;
    }
    /*@ }*/

    /*取消所有挂起的I/O。
        仍将调用已排队的任何处理程序。
    **/

    /*@ {*/
    void cancel ()
    {
        std::unique_lock <decltype(m_mutex)> lock (m_mutex);
        cancel (lock, true);
    }

    void cancel_async ()
    {
        std::unique_lock <decltype(m_mutex)> lock (m_mutex);
        cancel (lock, false);
    }
    /*@ }*/

    /*测量一个I/O延迟样本。
        将使用此签名调用处理程序：
            无效处理程序（持续时间d）；
    **/

    template <class Handler>
    void sample_one (Handler&& handler)
    {
        std::lock_guard <decltype(m_mutex)> lock (m_mutex);
        if (m_cancel)
            throw std::logic_error ("io_latency_probe is canceled");
        m_ios.post (sample_op <Handler> (
            std::forward <Handler> (handler),
                Clock::now(), false, this));
    }

    /*启动连续I/O延迟采样。
        将使用此签名调用处理程序：
            void处理程序（std:：chrono:：millises）；
    **/

    template <class Handler>
    void sample (Handler&& handler)
    {
        std::lock_guard <decltype(m_mutex)> lock (m_mutex);
        if (m_cancel)
            throw std::logic_error ("io_latency_probe is canceled");
        m_ios.post (sample_op <Handler> (
            std::forward <Handler> (handler),
                Clock::now(), true, this));
    }

private:
    void cancel (std::unique_lock <decltype (m_mutex)>& lock,
        bool wait)
    {
        if (! m_cancel)
        {
            --m_count;
            m_cancel = true;
        }

        if (wait)
#ifdef BOOST_NO_CXX11_LAMBDAS
            while (m_count != 0)
                m_cond.wait (lock);
#else
            m_cond.wait (lock, [this] {
                return this->m_count == 0; });
#endif
    }

    void addref ()
    {
        std::lock_guard <decltype(m_mutex)> lock (m_mutex);
        ++m_count;
    }

    void release ()
    {
        std::lock_guard <decltype(m_mutex)> lock (m_mutex);
        if (--m_count == 0)
            m_cond.notify_all ();
    }

    template <class Handler>
    struct sample_op
    {
        Handler m_handler;
        time_point m_start;
        bool m_repeat;
        io_latency_probe* m_probe;

        sample_op (Handler const& handler, time_point const& start,
            bool repeat, io_latency_probe* probe)
            : m_handler (handler)
            , m_start (start)
            , m_repeat (repeat)
            , m_probe (probe)
        {
            assert(m_probe);
            m_probe->addref();
        }

        sample_op (sample_op&& from) noexcept
            : m_handler (std::move(from.m_handler))
            , m_start (from.m_start)
            , m_repeat (from.m_repeat)
            , m_probe (from.m_probe)
        {
            assert(m_probe);
            from.m_probe = nullptr;
        }

        sample_op (sample_op const&) = delete;
        sample_op operator= (sample_op const&) = delete;
        sample_op& operator= (sample_op&&) = delete;

        ~sample_op ()
        {
            if(m_probe)
                m_probe->release();
        }

        void operator() () const
        {
            if (!m_probe)
                return;
            typename Clock::time_point const now (Clock::now());
            typename Clock::duration const elapsed (now - m_start);

            m_handler (elapsed);

            {
                std::lock_guard <decltype (m_probe->m_mutex)
                    > lock (m_probe->m_mutex);
                if (m_probe->m_cancel)
                    return;
            }

            if (m_repeat)
            {
//当我们想再次取样时计算，并且
//调整预期延迟。
//
                typename Clock::time_point const when (
                    now + m_probe->m_period - 2 * elapsed);

                if (when <= now)
                {
//延迟太高，无法保持所需的
//周期，所以不要用计时器。
//
                    m_probe->m_ios.post (sample_op <Handler> (
                        m_handler, now, m_repeat, m_probe));
                }
                else
                {
                    m_probe->m_timer.expires_from_now(when - now);
                    m_probe->m_timer.async_wait (sample_op <Handler> (
                        m_handler, now, m_repeat, m_probe));
                }
            }
        }

        void operator () (boost::system::error_code const& ec)
        {
            if (!m_probe)
                return;
            typename Clock::time_point const now (Clock::now());
            m_probe->m_ios.post (sample_op <Handler> (
                m_handler, now, m_repeat, m_probe));
        }
    };
};

}

#endif
