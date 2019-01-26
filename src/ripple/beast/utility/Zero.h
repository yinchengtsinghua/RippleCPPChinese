
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
    版权所有2014，tom ritchford<tom@漩涡.com>

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

#ifndef BEAST_UTILITY_ZERO_H_INCLUDED
#define BEAST_UTILITY_ZERO_H_INCLUDED

namespace beast {

/*零允许类提供与零的有效比较。

    零是一个结构，允许类与零进行有效比较
    需要右值构造。

    通常情况下，我们的类是由一个数字和一个单位组成的。
    在这种情况下，比较如t>0或t！=0有意义，但比较
    比如t>1或t！= 1不。

    类0允许轻松进行此类比较。

    比较类T要么需要一个名为signum（）的方法，该方法
    返回正数、0或负数；或者需要有符号
    在命名空间中解析的函数，该命名空间采用T和
    返回一个正数、零或负数。
**/


struct Zero
{
    explicit Zero() = default;
};

namespace {
static constexpr Zero zero{};
}

/*signum的默认实现调用类上的方法。*/
template <typename T>
auto signum(T const& t)
{
    return t.signum();
}

namespace detail {
namespace zero_helper {

//要使依赖参数的查找正常工作，必须调用signum
//由不包含函数重载的命名空间生成。
template <class T>
auto call_signum(T const& t)
{
    return signum(t);
}

} //零辅助器
} //细节

//在T位于左侧的地方，使用signum处理操作员。

template <typename T>
bool operator==(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) == 0;
}

template <typename T>
bool operator!=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) != 0;
}

template <typename T>
bool operator<(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) < 0;
}

template <typename T>
bool operator>(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) > 0;
}

template <typename T>
bool operator>=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) >= 0;
}

template <typename T>
bool operator<=(T const& t, Zero)
{
    return detail::zero_helper::call_signum(t) <= 0;
}

//处理右边t旁边的运算符
//反转操作，使T位于左侧。

template <typename T>
bool operator==(Zero, T const& t)
{
    return t == zero;
}

template <typename T>
bool operator!=(Zero, T const& t)
{
    return t != zero;
}

template <typename T>
bool operator<(Zero, T const& t)
{
    return t > zero;
}

template <typename T>
bool operator>(Zero, T const& t)
{
    return t < zero;
}

template <typename T>
bool operator>=(Zero, T const& t)
{
    return t <= zero;
}

template <typename T>
bool operator<=(Zero, T const& t)
{
    return t >= zero;
}

} //野兽

#endif
