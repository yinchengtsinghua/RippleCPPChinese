
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

#ifndef RIPPLE_BASICS_DECAYINGSAMPLE_H_INCLUDED
#define RIPPLE_BASICS_DECAYINGSAMPLE_H_INCLUDED

#include <chrono>
#include <cmath>

namespace ripple {

/*采样函数利用指数衰减提供连续值。
    @tparam衰减窗口中的秒数。
**/

template <int Window, typename Clock>
class DecayingSample
{
public:
    using value_type = typename Clock::duration::rep;
    using time_point = typename Clock::time_point;

    DecayingSample () = delete;

    /*
        @param现在开始腐烂样品的时间。
    **/

    explicit DecayingSample (time_point now)
        : m_value (value_type())
        , m_when (now)
    {
    }

    /*添加新示例。
        该值将根据指定的时间进行首次老化。
    **/

    value_type add (value_type value, time_point now)
    {
        decay (now);
        m_value += value;
        return m_value / Window;
    }

    /*以标准化单位检索当前值。
        样品按规定时间进行首次老化。
    **/

    value_type value (time_point now)
    {
        decay (now);
        return m_value / Window;
    }

private:
//根据指定的时间应用指数衰减。
    void decay (time_point now)
    {
        if (now == m_when)
            return;

        if (m_value != value_type())
        {
            std::size_t elapsed = std::chrono::duration_cast<
                std::chrono::seconds>(now - m_when).count();

//大于窗户四倍的跨度会使
//值不重要，所以只需重置它。
//
            if (elapsed > 4 * Window)
            {
                m_value = value_type();
            }
            else
            {
                while (elapsed--)
                    m_value -= (m_value + Window - 1) / Window;
            }
        }

        m_when = now;
    }

//以指数单位表示的当前值
    value_type m_value;

//上次应用老化功能时
    time_point m_when;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*采样函数利用指数衰减提供连续值。
    @t参数半衰期样本的半衰期，以秒为单位。
**/

template <int HalfLife, class Clock>
class DecayWindow
{
public:
    using time_point = typename Clock::time_point;

    explicit
    DecayWindow (time_point now)
        : value_(0)
        , when_(now)
    {
    }

    void
    add (double value, time_point now)
    {
        decay(now);
        value_ += value;
    }

    double
    value (time_point now)
    {
        decay(now);
        return value_ / HalfLife;
    }

private:
    static_assert(HalfLife > 0,
        "half life must be positive");

    void
    decay (time_point now)
    {
        if (now <= when_)
            return;
        using namespace std::chrono;
        auto const elapsed =
            duration<double>(now - when_).count();
        value_ *= std::pow(2.0, - elapsed / HalfLife);
        when_ = now;
    }

    double value_;
    time_point when_;
};

}

#endif
