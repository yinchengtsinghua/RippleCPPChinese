
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

#include <ripple/basics/contract.h>
#include <ripple/protocol/IOUAmount.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <stdexcept>

namespace ripple {

/*归一化后尾数的范围*/
static std::int64_t const minMantissa = 1000000000000000ull;
static std::int64_t const maxMantissa = 9999999999999999ull;
/*归一化后的指数范围*/
static int const minExponent = -96;
static int const maxExponent = 80;

void
IOUAmount::normalize ()
{
    if (mantissa_ == 0)
    {
        *this = beast::zero;
        return;
    }

    bool const negative = (mantissa_ < 0);

    if (negative)
        mantissa_ = -mantissa_;

    while ((mantissa_ < minMantissa) && (exponent_ > minExponent))
    {
        mantissa_ *= 10;
        --exponent_;
    }

    while (mantissa_ > maxMantissa)
    {
        if (exponent_ >= maxExponent)
            Throw<std::overflow_error> ("IOUAmount::normalize");

        mantissa_ /= 10;
        ++exponent_;
    }

    if ((exponent_ < minExponent) || (mantissa_ < minMantissa))
    {
        *this = beast::zero;
        return;
    }

    if (exponent_ > maxExponent)
        Throw<std::overflow_error> ("value overflow");

    if (negative)
        mantissa_ = -mantissa_;
}

IOUAmount&
IOUAmount::operator+= (IOUAmount const& other)
{
    if (other == beast::zero)
        return *this;

    if (*this == beast::zero)
    {
        *this = other;
        return *this;
    }

    auto m = other.mantissa_;
    auto e = other.exponent_;

    while (exponent_ < e)
    {
        mantissa_ /= 10;
        ++exponent_;
    }

    while (e < exponent_)
    {
        m /= 10;
        ++e;
    }

//此添加不能溢出std:：int64，但我们可能会从
//如果结果不可表示，则正常化。
    mantissa_ += m;

    if (mantissa_ >= -10 && mantissa_ <= 10)
    {
        *this = beast::zero;
        return *this;
    }

    normalize ();

    return *this;
}

bool
IOUAmount::operator<(IOUAmount const& other) const
{
//如果两个量有不同的符号（零被视为正）
//如果左边是负数，那么这个比较是真的。
    bool const lneg = mantissa_ < 0;
    bool const rneg = other.mantissa_ < 0;

    if (lneg != rneg)
        return lneg;

//两个都有相同的符号，左边是零：右边必须是
//大于0。
    if (mantissa_ == 0)
        return other.mantissa_ > 0;

//两者都有相同的符号，右边是零，左边是非零。
    if (other.mantissa_ == 0)
        return false;

//两者具有相同的符号，按指数比较：
    if (exponent_ > other.exponent_)
        return lneg;
    if (exponent_ < other.exponent_)
        return !lneg;

//如果指数相等，则比较尾数
    return mantissa_ < other.mantissa_;
}

std::string
to_string (IOUAmount const& amount)
{
//保持完整的内部准确度，但如果可能的话，使其更人性化。
    if (amount == beast::zero)
        return "0";

    int const exponent = amount.exponent ();
    auto mantissa = amount.mantissa ();

//对太小或太大的指数使用科学符号
    if (((exponent != 0) && ((exponent < -25) || (exponent > -5))))
    {
        std::string ret = std::to_string (mantissa);
        ret.append (1, 'e');
        ret.append (std::to_string (exponent));
        return ret;
    }

    bool negative = false;

    if (mantissa < 0)
    {
        mantissa = -mantissa;
        negative = true;
    }

    assert (exponent + 43 > 0);

    size_t const pad_prefix = 27;
    size_t const pad_suffix = 23;

    std::string const raw_value (std::to_string (mantissa));
    std::string val;

    val.reserve (raw_value.length () + pad_prefix + pad_suffix);
    val.append (pad_prefix, '0');
    val.append (raw_value);
    val.append (pad_suffix, '0');

    size_t const offset (exponent + 43);

    auto pre_from (val.begin ());
    auto const pre_to (val.begin () + offset);

    auto const post_from (val.begin () + offset);
    auto post_to (val.end ());

//裁剪前导零。利用这样一个事实
//固定数量的前导零并跳过它们。
    if (std::distance (pre_from, pre_to) > pad_prefix)
        pre_from += pad_prefix;

    assert (post_to >= post_from);

    pre_from = std::find_if (pre_from, pre_to,
        [](char c)
        {
            return c != '0';
        });

//裁剪尾随零。利用这样一个事实
//固定数量的尾随零并跳过它们。
    if (std::distance (post_from, post_to) > pad_suffix)
        post_to -= pad_suffix;

    assert (post_to >= post_from);

    post_to = std::find_if(
        std::make_reverse_iterator (post_to),
        std::make_reverse_iterator (post_from),
        [](char c)
        {
            return c != '0';
        }).base();

    std::string ret;

    if (negative)
        ret.append (1, '-');

//组装输出：
    if (pre_from == pre_to)
        ret.append (1, '0');
    else
        ret.append(pre_from, pre_to);

    if (post_to != post_from)
    {
        ret.append (1, '.');
        ret.append (post_from, post_to);
    }

    return ret;
}

IOUAmount
mulRatio (
    IOUAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp)
{
    using namespace boost::multiprecision;

    if (!den)
        Throw<std::runtime_error> ("division by zero");

//索引从0到29的索引值为10%的向量
//我们期望的最大中间值是2^96，即
//小于10^29
    static auto const powerTable = []
    {
        std::vector<uint128_t> result;
result.reserve (30);  //2^96是最大的中间结果大小
        uint128_t cur (1);
        for (int i = 0; i < 30; ++i)
        {
            result.push_back (cur);
            cur *= 10;
        };
        return result;
    }();

//返回楼层（log10（v））
//注：V==0时返回-1
    static auto log10Floor = [](uint128_t const& v)
    {
//查找第一个元素的索引>=请求的元素，索引
//是日志表中元素的日志。
        auto const l = std::lower_bound (powerTable.begin (), powerTable.end (), v);
        int index = std::distance (powerTable.begin (), l);
//如果我们不相等，减去得到地板。
        if (*l != v)
            --index;
        return index;
    };

//返回CEIL（log10（v））
    static auto log10Ceil = [](uint128_t const& v)
    {
//查找第一个元素的索引>=请求的元素，索引
//是日志表中元素的日志。
        auto const l = std::lower_bound (powerTable.begin (), powerTable.end (), v);
        return int(std::distance (powerTable.begin (), l));
    };

    static auto const fl64 =
        log10Floor (std::numeric_limits<std::int64_t>::max ());

    bool const neg = amt.mantissa () < 0;
    uint128_t const den128 (den);
//32值*64位值，存储在128位值中。这永远不会溢出
    uint128_t const mul =
        uint128_t (neg ? -amt.mantissa () : amt.mantissa ()) * uint128_t (num);

    auto low = mul / den128;
    uint128_t rem (mul - low * den128);

    int exponent = amt.exponent ();

    if (rem)
    {
//在数学上，结果是low+rem/den128。但是，既然这样
//使用整数除法REM/DEN128将为零。按比例缩放结果
//低并不溢出我们能在尾数中存储的最大数量
//并且（REM/DEN128）尽可能大。按低倍数缩放
//从指数中减去10。我们可以这样做
//使用循环，但使用对数更有效。
        auto const roomToGrow = fl64 - log10Ceil (low);
        if (roomToGrow > 0)
        {
            exponent -= roomToGrow;
            low *= powerTable[roomToGrow];
            rem *= powerTable[roomToGrow];
        }
        auto const addRem = rem / den128;
        low += addRem;
        rem = rem - addRem * den128;
    }

//我们能得到的最大结果是~2^95，它溢出64位
//结果我们可以在尾数中存储。将结果除以10缩小
//在指数上加一个直到低值适合64位
//尾数。使用对数以避免循环。
    bool hasRem = bool(rem);
    auto const mustShrink = log10Ceil (low) - fl64;
    if (mustShrink > 0)
    {
        uint128_t const sav (low);
        exponent += mustShrink;
        low /= powerTable[mustShrink];
        if (!hasRem)
            hasRem = bool(sav - low * powerTable[mustShrink]);
    }

    std::int64_t mantissa = low.convert_to<std::int64_t> ();

//舍入前标准化
    if (neg)
        mantissa *= -1;

    IOUAmount result (mantissa, exponent);

    if (hasRem)
    {
//句柄舍入
        if (roundUp && !neg)
        {
            if (!result)
            {
                return IOUAmount (minMantissa, minExponent);
            }
//这个加法不能溢出，因为尾数已经被规范化了
            return IOUAmount (result.mantissa () + 1, result.exponent ());
        }

        if (!roundUp && neg)
        {
            if (!result)
            {
                return IOUAmount (-minMantissa, minExponent);
            }
//此减法不能下溢，因为“result”不是零
            return IOUAmount (result.mantissa () - 1, result.exponent ());
        }
    }

    return result;
}


}
