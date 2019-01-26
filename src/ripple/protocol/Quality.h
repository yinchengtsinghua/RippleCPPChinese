
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

#ifndef RIPPLE_PROTOCOL_QUALITY_H_INCLUDED
#define RIPPLE_PROTOCOL_QUALITY_H_INCLUDED

#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <cstdint>
#include <ostream>

namespace ripple {

/*表示一对输入和输出货币。

    可以将输入货币转换为输出货币。
    货币乘以汇率，表示为
    质量。

    对于报价，“入”总是接受方付费，“出”是
    总是接球。
**/

template<class In, class Out>
struct TAmounts
{
    TAmounts() = default;

    TAmounts (beast::Zero, beast::Zero)
        : in (beast::zero)
        , out (beast::zero)
    {
    }

    TAmounts (In const& in_, Out const& out_)
        : in (in_)
        , out (out_)
    {
    }

    /*如果其中一个数量不是正数，则返回“true”。*/
    bool
    empty() const noexcept
    {
        return in <= beast::zero || out <= beast::zero;
    }

    TAmounts& operator+=(TAmounts const& rhs)
    {
        in += rhs.in;
        out += rhs.out;
        return *this;
    }

    TAmounts& operator-=(TAmounts const& rhs)
    {
        in -= rhs.in;
        out -= rhs.out;
        return *this;
    }

    In in;
    Out out;
};

template<class In, class Out>
TAmounts<In, Out> make_Amounts(In const& in, Out const& out)
{
    return TAmounts<In, Out>(in, out);
}

using Amounts = TAmounts<STAmount, STAmount>;

template<class In, class Out>
bool
operator== (
    TAmounts<In, Out> const& lhs,
    TAmounts<In, Out> const& rhs) noexcept
{
    return lhs.in == rhs.in && lhs.out == rhs.out;
}

template<class In, class Out>
bool
operator!= (
    TAmounts<In, Out> const& lhs,
    TAmounts<In, Out> const& rhs) noexcept
{
    return ! (lhs == rhs);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于分析质量和其他内容的Ripple特定常量
#define QUALITY_ONE 1000000000

/*表示输出货币与输入货币的逻辑比率。
    在内部，这是使用自定义浮点表示法存储的，
    作为比率的倒数，这样质量就会下降
    表示质量的实际值序列。
**/

class Quality
{
public:
//内部表示形式的类型。更高质量
//具有较低的无符号整数表示形式。
    using value_type = std::uint64_t;

    static const int minTickSize = 3;
    static const int maxTickSize = 16;

private:
    value_type m_value;

public:
    Quality() = default;

    /*从stamount的整数编码创建质量*/
    explicit
    Quality (std::uint64_t value);

    /*根据两个数量的比率创建质量。*/
    explicit
    Quality (Amounts const& amount);

    /*根据两个数量的比率创建质量。*/
    template<class In, class Out>
    Quality (Out const& out, In const& in)
        : Quality (Amounts (toSTAmount (in),
                            toSTAmount (out)))
    {}

    /*提升到下一个更高的质量水平。*/
    /*@ {*/
    Quality&
    operator++();

    Quality
    operator++ (int);
    /*@ }*/

    /*提升到下一个更低的质量水平。*/
    /*@ {*/
    Quality&
    operator--();

    Quality
    operator-- (int);
    /*@ }*/

    /*以stamount形式返回质量。*/
    STAmount
    rate () const
    {
        return amountFromQuality (m_value);
    }

    /*返回舍入到指定数字的质量
        小数位数。
    **/

    Quality
    round (int tickSize) const;

    /*返回带上限的缩放金额。
        如果结果准确，则避免数学运算。输出被夹紧
        防止货币创造。
    **/

    Amounts
    ceil_in (Amounts const& amount, STAmount const& limit) const;

    template<class In, class Out>
    TAmounts<In, Out>
    ceil_in (TAmounts<In, Out> const& amount, In const& limit) const
    {
        if (amount.in <= limit)
            return amount;

//现在使用现有的stamount实现，但是考虑
//替换为特定于iouamount和xrpamount的代码
        Amounts stAmt (toSTAmount (amount.in), toSTAmount (amount.out));
        STAmount stLim (toSTAmount (limit));
        auto const stRes = ceil_in (stAmt, stLim);
        return TAmounts<In, Out> (toAmount<In> (stRes.in), toAmount<Out> (stRes.out));
    }

    /*返回超出上限的缩放量。
        如果结果准确，则避免数学运算。输入被夹紧
        防止货币创造。
    **/

    Amounts
    ceil_out (Amounts const& amount, STAmount const& limit) const;

    template<class In, class Out>
    TAmounts<In, Out>
    ceil_out (TAmounts<In, Out> const& amount, Out const& limit) const
    {
        if (amount.out <= limit)
            return amount;

//现在使用现有的stamount实现，但是考虑
//替换为特定于iouamount和xrpamount的代码
        Amounts stAmt (toSTAmount (amount.in), toSTAmount (amount.out));
        STAmount stLim (toSTAmount (limit));
        auto const stRes = ceil_out (stAmt, stLim);
        return TAmounts<In, Out> (toAmount<In> (stRes.in), toAmount<Out> (stRes.out));
    }

    /*如果lhs的质量低于rhs，则返回“true”。
        低质量意味着接受者得到了更差的交易。
        更高的质量对接受者更有利。
    **/

    friend
    bool
    operator< (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value > rhs.m_value;
    }

    friend
    bool
    operator> (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value < rhs.m_value;
    }

    friend
    bool
    operator<= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return !(lhs > rhs);
    }

    friend
    bool
    operator>= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return !(lhs < rhs);
    }

    friend
    bool
    operator== (Quality const& lhs, Quality const& rhs) noexcept
    {
        return lhs.m_value == rhs.m_value;
    }

    friend
    bool
    operator!= (Quality const& lhs, Quality const& rhs) noexcept
    {
        return ! (lhs == rhs);
    }

    friend
    std::ostream&
    operator<< (std::ostream& os, Quality const& quality)
    {
        os << quality.m_value;
        return os;
    }
};

/*在给定两个跃点的情况下，计算两个跃点路径的质量。
    @param lhs路径的第一段：输入到中间。
    @param rhs路径的第二段：中间到输出。
**/

Quality
composed_quality (Quality const& lhs, Quality const& rhs);

}

#endif
