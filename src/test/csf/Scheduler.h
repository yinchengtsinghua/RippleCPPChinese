
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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

#ifndef RIPPLE_TEST_CSF_SCHEDULER_H_INCLUDED
#define RIPPLE_TEST_CSF_SCHEDULER_H_INCLUDED

#include <ripple/basics/qalloc.h>
#include <ripple/beast/clock/manual_clock.h>
#include <boost/intrusive/set.hpp>
#include <type_traits>
#include <utility>

namespace ripple {
namespace test {
namespace csf {

/*模拟离散事件调度程序。

    使用单个公共时钟模拟事件的行为。

    事件是使用lambda函数建模的，并计划在
    具体时间。事件可以使用返回的令牌取消，当
    已安排事件。

    呼叫方使用步骤中的一个或多个，步骤\一，步骤\用于，步骤\直到和
    步骤\while函数处理计划的事件。
**/

class Scheduler
{
public:
    using clock_type = beast::manual_clock<std::chrono::steady_clock>;

    using duration = typename clock_type::duration;

    using time_point = typename clock_type::time_point;

private:
    using by_when_hook = boost::intrusive::set_base_hook<
        boost::intrusive::link_mode<boost::intrusive::normal_link>>;

    struct event : by_when_hook
    {
        time_point when;

        event(event const&) = delete;
        event&
        operator=(event const&) = delete;

        virtual ~event() = default;

//调用以执行事件
        virtual void
        operator()() const = 0;

        event(time_point when_) : when(when_)
        {
        }

        bool
        operator<(event const& other) const
        {
            return when < other.when;
        }
    };

    template <class Handler>
    class event_impl : public event
    {
        Handler const h_;

    public:
        event_impl(event_impl const&) = delete;

        event_impl&
        operator=(event_impl const&) = delete;

        template <class DeducedHandler>
        event_impl(time_point when_, DeducedHandler&& h)
            : event(when_)
            , h_(std::forward<DeducedHandler>(h))
        {
        }

        void
        operator()() const override
        {
            h_();
        }
    };

    class queue_type
    {
    private:
        using by_when_set = typename boost::intrusive::make_multiset<
            event,
            boost::intrusive::constant_time_size<false>>::type;

        qalloc alloc_;
        by_when_set by_when_;

    public:
        using iterator = typename by_when_set::iterator;

        queue_type(queue_type const&) = delete;
        queue_type&
        operator=(queue_type const&) = delete;

        explicit queue_type(qalloc const& alloc);

        ~queue_type();

        bool
        empty() const;

        iterator
        begin();

        iterator
        end();

        template <class Handler>
        typename by_when_set::iterator
        emplace(time_point when, Handler&& h);

        iterator
        erase(iterator iter);
    };

    qalloc alloc_;
    queue_type queue_;

//依赖此时钟的老化容器采用非常量引用=.
    mutable clock_type clock_;

public:
    Scheduler(Scheduler const&) = delete;
    Scheduler&
    operator=(Scheduler const&) = delete;

    Scheduler();

    /*返回分配器。*/
    qalloc const&
    alloc() const;

    /*把钟还回去。（老化的容器需要一个非常量ref=）*/
    clock_type &
    clock() const;

    /*返回当前网络时间。

        @注意，时代未定
    **/

    time_point
    now() const;

//用于取消计时器
    struct cancel_token;

    /*在特定时间安排事件

        影响：

            到达网络时间后，
            函数将用
            没有参数。
    **/

    template <class Function>
    cancel_token
    at(time_point const& when, Function&& f);

    /*在指定的持续时间过后安排事件

        影响：

            当规定的时间过去后，
            函数将用
            没有参数。
    **/

    template <class Function>
    cancel_token
    in(duration const& delay, Function&& f);

    /*取消计时器。

        先决条件：

            'token'是调用的返回值
            尚未调用的timer（）。
    **/

    void
    cancel(cancel_token const& token);

    /*为最多一个事件运行计划程序。

        影响：

            时钟提前到时间了
            上次传递的事件。

        @return`true`如果处理了一个事件。
    **/

    bool
    step_one();

    /*运行计划程序，直到没有事件保留。

        影响：

            时钟提前到时间了
            最后一个事件。

        @return`true`如果处理了一个事件。
    **/

    bool
    step();

    /*条件为真时运行计划程序。

        函数不接受参数，将被调用
        在每个事件被处理到
        决定是否继续。

        影响：

            时钟提前到时间了
            上次传递的事件。

        @return`true`如果处理了任何事件。
    **/

    template <class Function>
    bool
    step_while(Function&& func);

    /*运行计划程序直到指定的时间。

        影响：

            时钟提前到
            规定时间。

        @return`true`如果还有任何事件。
    **/

    bool
    step_until(time_point const& until);

    /*运行计划程序直到时间过去。

        影响：

            时钟提前
            指定的持续时间。

        @return`true`如果还有任何事件。
    **/

    template <class Period, class Rep>
    bool
    step_for(std::chrono::duration<Period, Rep> const& amount);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline Scheduler::queue_type::queue_type(qalloc const& alloc) : alloc_(alloc)
{
}

inline Scheduler::queue_type::~queue_type()
{
    for (auto iter = by_when_.begin(); iter != by_when_.end();)
    {
        auto e = &*iter;
        ++iter;
        e->~event();
        alloc_.dealloc(e, 1);
    }
}

inline bool
Scheduler::queue_type::empty() const
{
    return by_when_.empty();
}

inline auto
Scheduler::queue_type::begin() -> iterator
{
    return by_when_.begin();
}

inline auto
Scheduler::queue_type::end() -> iterator
{
    return by_when_.end();
}


template <class Handler>
inline auto
Scheduler::queue_type::emplace(time_point when, Handler&& h) ->
    typename by_when_set::iterator
{
    using event_type = event_impl<std::decay_t<Handler>>;
    auto const p = alloc_.alloc<event_type>(1);
    auto& e = *new (p) event_type(
        when, std::forward<Handler>(h));
    return by_when_.insert(e);
}

inline auto
Scheduler::queue_type::erase(iterator iter) -> typename by_when_set::iterator
{
    auto& e = *iter;
    auto next = by_when_.erase(iter);
    e.~event();
    alloc_.dealloc(&e, 1);
    return next;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
struct Scheduler::cancel_token
{
private:
    typename queue_type::iterator iter_;

public:
    cancel_token() = delete;
    cancel_token(cancel_token const&) = default;
    cancel_token&
    operator=(cancel_token const&) = default;

private:
    friend class Scheduler;
    cancel_token(typename queue_type::iterator iter) : iter_(iter)
    {
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
inline Scheduler::Scheduler() : queue_(alloc_)
{
}

inline qalloc const&
Scheduler::alloc() const
{
    return alloc_;
}

inline auto
Scheduler::clock() const -> clock_type &
{
    return clock_;
}

inline auto
Scheduler::now() const -> time_point
{
    return clock_.now();
}

template <class Function>
inline auto
Scheduler::at(time_point const& when, Function&& f) -> cancel_token
{
    return queue_.emplace(when, std::forward<Function>(f));
}

template <class Function>
inline auto
Scheduler::in(duration const& delay, Function&& f) -> cancel_token
{
    return at(clock_.now() + delay, std::forward<Function>(f));
}

inline void
Scheduler::cancel(cancel_token const& token)
{
    queue_.erase(token.iter_);
}

inline bool
Scheduler::step_one()
{
    if (queue_.empty())
        return false;
    auto const iter = queue_.begin();
    clock_.set(iter->when);
    (*iter)();
    queue_.erase(iter);
    return true;
}

inline bool
Scheduler::step()
{
    if (!step_one())
        return false;
    for (;;)
        if (!step_one())
            break;
    return true;
}

template <class Function>
inline bool
Scheduler::step_while(Function&& f)
{
    bool ran = false;
    while (f() && step_one())
        ran = true;
    return ran;
}

inline bool
Scheduler::step_until(time_point const& until)
{
//vfalc这个程序需要优化
    if (queue_.empty())
    {
        clock_.set(until);
        return false;
    }
    auto iter = queue_.begin();
    if (iter->when > until)
    {
        clock_.set(until);
        return true;
    }
    do
    {
        step_one();
        iter = queue_.begin();
    } while (iter != queue_.end() && iter->when <= until);
    clock_.set(until);
    return iter != queue_.end();
}

template <class Period, class Rep>
inline bool
Scheduler::step_for(std::chrono::duration<Period, Rep> const& amount)
{
    return step_until(now() + amount);
}

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
