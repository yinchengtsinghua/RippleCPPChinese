
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

#include <ripple/protocol/Quality.h>
#include <cassert>
#include <limits>

namespace ripple {

Quality::Quality (std::uint64_t value)
    : m_value (value)
{
}

Quality::Quality (Amounts const& amount)
    : m_value (getRate (amount.out, amount.in))
{
}

Quality&
Quality::operator++()
{
    assert (m_value > 0);
    --m_value;
    return *this;
}

Quality
Quality::operator++ (int)
{
    Quality prev (*this);
    ++*this;
    return prev;
}

Quality&
Quality::operator--()
{
    assert (m_value < std::numeric_limits<value_type>::max());
    ++m_value;
    return *this;
}

Quality
Quality::operator-- (int)
{
    Quality prev (*this);
    --*this;
    return prev;
}

Amounts
Quality::ceil_in (Amounts const& amount, STAmount const& limit) const
{
    if (amount.in > limit)
    {
        Amounts result (limit, divRound (
            limit, rate(), amount.out.issue (), true));
//箝位
        if (result.out > amount.out)
            result.out = amount.out;
        assert (result.in == limit);
        return result;
    }
    assert (amount.in <= limit);
    return amount;
}

Amounts
Quality::ceil_out (Amounts const& amount, STAmount const& limit) const
{
    if (amount.out > limit)
    {
        Amounts result (mulRound (
            limit, rate(), amount.in.issue (), true), limit);
//箝位
        if (result.in > amount.in)
            result.in = amount.in;
        assert (result.out == limit);
        return result;
    }
    assert (amount.out <= limit);
    return amount;
}

Quality
composed_quality (Quality const& lhs, Quality const& rhs)
{
    STAmount const lhs_rate (lhs.rate ());
    assert (lhs_rate != beast::zero);

    STAmount const rhs_rate (rhs.rate ());
    assert (rhs_rate != beast::zero);

    STAmount const rate (mulRound (
        lhs_rate, rhs_rate, lhs_rate.issue (), true));

    std::uint64_t const stored_exponent (rate.exponent () + 100);
    std::uint64_t const stored_mantissa (rate.mantissa());

    assert ((stored_exponent > 0) && (stored_exponent <= 255));

    return Quality ((stored_exponent << (64 - 8)) | stored_mantissa);
}

Quality
Quality::round (int digits) const
{
//尾数模
    static const std::uint64_t mod[17] = {
        /*0*/100000000000000万，
        /* 1*/  1000000000000000,

        /*2*/1000000000000万，
        /* 3*/    10000000000000,

        /*4*/10000000000万，
        /* 5*/      100000000000,

        /*6*/1000000000个，
        /* 7*/        1000000000,

        /*8*/100000000个，
        /* 9*/          10000000,

        /*10*/1000000个，
        /* 11*/           100000,

        /*12*/万，
        /* 13*/             1000,

        /*14*/100，
        /* 15*/               10,

        /*16个/1，
    }；

    自动指数=m_值>>（64-8）；
    自动尾数=m_值&0x00fffffffffffffffull；
    尾数+=模[数字]-1；
    尾数-=（尾数%mod[数字]）；

    返回质量（指数<（64-8））尾数
}

}
