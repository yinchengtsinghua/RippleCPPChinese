
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

#ifndef RIPPLE_PROTOCOL_XRPAMOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_XRPAMOUNT_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstdint>
#include <string>
#include <type_traits>

namespace ripple {

class XRPAmount
    : private boost::totally_ordered <XRPAmount>
    , private boost::additive <XRPAmount>
{
private:
    std::int64_t drops_;

public:
    XRPAmount () = default;
    XRPAmount (XRPAmount const& other) = default;
    XRPAmount& operator= (XRPAmount const& other) = default;

    XRPAmount (beast::Zero)
        : drops_ (0)
    {
    }

    XRPAmount&
    operator= (beast::Zero)
    {
        drops_ = 0;
        return *this;
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    XRPAmount (Integer drops)
        : drops_ (static_cast<std::int64_t> (drops))
    {
    }

    template <class Integer,
        class = typename std::enable_if_t <
            std::is_integral<Integer>::value>>
    XRPAmount&
    operator= (Integer drops)
    {
        drops_ = static_cast<std::int64_t> (drops);
        return *this;
    }

    XRPAmount&
    operator+= (XRPAmount const& other)
    {
        drops_ += other.drops_;
        return *this;
    }

    XRPAmount&
    operator-= (XRPAmount const& other)
    {
        drops_ -= other.drops_;
        return *this;
    }

    XRPAmount
    operator- () const
    {
        return { -drops_ };
    }

    bool
    operator==(XRPAmount const& other) const
    {
        return drops_ == other.drops_;
    }

    bool
    operator<(XRPAmount const& other) const
    {
        return drops_ < other.drops_;
    }

    /*如果金额不是零，则返回“真”*/
    explicit
    operator bool() const noexcept
    {
        return drops_ != 0;
    }

    /*返回金额的符号*/
    int
    signum() const noexcept
    {
        return (drops_ < 0) ? -1 : (drops_ ? 1 : 0);
    }

    /*返回滴数*/
    std::int64_t
    drops () const
    {
        return drops_;
    }
};

inline
std::string
to_string (XRPAmount const& amount)
{
    return std::to_string (amount.drops ());
}

inline
XRPAmount
mulRatio (
    XRPAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

    int128_t const amt128 (amt.drops ());
    auto const neg = amt.drops () < 0;
    auto const m = amt128 * num;
    auto r = m / den;
    if (m % den)
    {
        if (!neg && roundUp)
            r += 1;
        if (neg && !roundUp)
            r -= 1;
    }
    if (r > std::numeric_limits<std::int64_t>::max ())
        Throw<std::overflow_error> ("XRP mulRatio overflow");
    return XRPAmount (r.convert_to<std::int64_t> ());
}

/*如果金额不超过现有的初始xrp，则返回true。*/
inline
bool isLegalAmount (XRPAmount const& amount)
{
    return amount.drops () <= SYSTEM_CURRENCY_START;
}

}

#endif
