
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
    版权所有（c）2015 Ripple Labs Inc.

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

#ifndef RIPPLE_PROTOCOL_RATE_H_INCLUDED
#define RIPPLE_PROTOCOL_RATE_H_INCLUDED

#include <ripple/protocol/STAmount.h>
#include <boost/operators.hpp>
#include <cassert>
#include <cstdint>
#include <ostream>

namespace ripple {

/*表示传输速率

    转移率规定为10亿分之一。
    例如，1%的传输率表示为
    1010000000。
**/

struct Rate
    : private boost::totally_ordered <Rate>
{
    std::uint32_t value;

    Rate () = delete;

    explicit
    Rate (std::uint32_t rate)
        : value (rate)
    {
    }
};

inline
bool
operator== (Rate const& lhs, Rate const& rhs) noexcept
{
    return lhs.value == rhs.value;
}

inline
bool
operator< (Rate const& lhs, Rate const& rhs) noexcept
{
    return lhs.value < rhs.value;
}

inline
std::ostream&
operator<< (std::ostream& os, Rate const& rate)
{
    os << rate.value;
    return os;
}

STAmount
multiply (
    STAmount const& amount,
    Rate const& rate);

STAmount
multiplyRound (
    STAmount const& amount,
    Rate const& rate,
    bool roundUp);

STAmount
multiplyRound (
    STAmount const& amount,
    Rate const& rate,
    Issue const& issue,
    bool roundUp);

STAmount
divide (
    STAmount const& amount,
    Rate const& rate);

STAmount
divideRound (
    STAmount const& amount,
    Rate const& rate,
    bool roundUp);

STAmount
divideRound (
    STAmount const& amount,
    Rate const& rate,
    Issue const& issue,
    bool roundUp);

/*表示1:1交换的转移率*/
extern Rate const parityRate;

}

#endif
