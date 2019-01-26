
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

#ifndef BEAST_CHRONO_ABSTRACT_CLOCK_H_INCLUDED
#define BEAST_CHRONO_ABSTRACT_CLOCK_H_INCLUDED

#include <chrono>
#include <string>

namespace beast {

/*时钟的抽象接口。

    这使得now（）成为成员函数而不是静态成员，因此
    类的一个实例可以被依赖注入，从而
    可以控制时间的单元测试。

    抽象时钟继承时钟的所有嵌套类型
    模板参数。

    例子：

    @代码

    结构实现
    {
        使用clock_type=abstract_clock<std:：chrono:：steady_clock>
        时钟类型和时钟
        显式实现（时钟类型和时钟）
            钟表（时钟）
        {
        }
    }；

    @端码

    @tparam时钟A型符合这些要求：
        http://en.cppreference.com/w/cpp/concept/clock/时钟
**/

template <class Clock>
class abstract_clock
{
public:
    using rep = typename Clock::rep;
    using period = typename Clock::period;
    using duration = typename Clock::duration;
    using time_point = typename Clock::time_point;
    using clock_type = Clock;

    static bool const is_steady = Clock::is_steady;

    virtual ~abstract_clock() = default;
    abstract_clock() = default;
    abstract_clock(abstract_clock const&) = default;

    /*返回当前时间。*/
    virtual time_point now() const = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

template <class Facade, class Clock>
struct abstract_clock_wrapper
    : public abstract_clock<Facade>
{
    explicit abstract_clock_wrapper() = default;

    using typename abstract_clock<Facade>::duration;
    using typename abstract_clock<Facade>::time_point;

    time_point
    now() const override
    {
        return Clock::now();
    }
};

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回抽象时钟的全局实例。
    @tparam facade a型符合以下要求：
        http://en.cppreference.com/w/cpp/concept/clock/时钟
    @tparam clock实际使用的混凝土时钟。
**/

template<class Facade, class Clock = Facade>
abstract_clock<Facade>&
get_abstract_clock()
{
    static detail::abstract_clock_wrapper<Facade, Clock> clock;
    return clock;
}

}

#endif
