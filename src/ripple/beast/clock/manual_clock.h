
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

#ifndef BEAST_CHRONO_MANUAL_CLOCK_H_INCLUDED
#define BEAST_CHRONO_MANUAL_CLOCK_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <cassert>

namespace beast {

/*手动时钟执行。

    这个具体类实现了@ref abstract_clock接口和
    允许手动提前时间，主要是为了
    提供一个时钟输入单元测试。

    @tparam时钟A型符合这些要求：
        http://en.cppreference.com/w/cpp/concept/clock/时钟
**/

template <class Clock>
class manual_clock
    : public abstract_clock<Clock>
{
public:
    using typename abstract_clock<Clock>::rep;
    using typename abstract_clock<Clock>::duration;
    using typename abstract_clock<Clock>::time_point;

private:
    time_point now_;

public:
    explicit
    manual_clock (time_point const& now = time_point(duration(0)))
        : now_(now)
    {
    }

    time_point
    now() const override
    {
        return now_;
    }

    /*设置手动时钟的当前时间。*/
    void
    set (time_point const& when)
    {
        assert(! Clock::is_steady || when >= now_);
        now_ = when;
    }

    /*以秒为单位设置时间的便利性。*/
    template <class Integer>
    void
    set(Integer seconds_from_epoch)
    {
        set(time_point(duration(
            std::chrono::seconds(seconds_from_epoch))));
    }

    /*将时钟提前一段时间。*/
    template <class Rep, class Period>
    void
    advance(std::chrono::duration<Rep, Period> const& elapsed)
    {
        assert(! Clock::is_steady ||
            (now_ + elapsed) >= now_);
        now_ += elapsed;
    }

    /*使时钟提前一秒钟的便利性。*/
    manual_clock&
    operator++ ()
    {
        advance(std::chrono::seconds(1));
        return *this;
    }
};

}

#endif
