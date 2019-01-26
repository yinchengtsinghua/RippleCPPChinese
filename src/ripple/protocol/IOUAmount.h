
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

#ifndef RIPPLE_PROTOCOL_IOUAMOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_IOUAMOUNT_H_INCLUDED

#include <ripple/beast/utility/Zero.h>
#include <boost/operators.hpp>
#include <cstdint>
#include <string>
#include <utility>

namespace ripple {

/*高动态范围量的浮点表示法

    金额存储为标准化有符号尾数和指数。这个
    归一化指数的范围是[-96,80]和绝对值的范围
    标准化尾数的值为[100000000000000999999999]。

    算术运算在标准化过程中可能引发std:：overflow_错误
    如果金额超过最大可代表金额，但下溢
    将无声地转到零。
**/

class IOUAmount
    : private boost::totally_ordered<IOUAmount>
    , private boost::additive <IOUAmount>
{
private:
    std::int64_t mantissa_;
    int exponent_;

    /*将尾数和指数调整到适当的范围。

        如果数量不能正常化，或者大于
        可以表示为借据金额的最大值。数量
        太小，无法表示为标准化为0。
    **/

    void
    normalize ();

public:
    IOUAmount () = default;
    IOUAmount (IOUAmount const& other) = default;
    IOUAmount&operator= (IOUAmount const& other) = default;

    IOUAmount (beast::Zero)
    {
        *this = beast::zero;
    }

    IOUAmount (std::int64_t mantissa, int exponent)
        : mantissa_ (mantissa)
        , exponent_ (exponent)
    {
        normalize ();
    }

    IOUAmount&
    operator= (beast::Zero)
    {
//-100用于允许0对小于小正值的值进行排序
//会有很大的负指数。
        mantissa_ = 0;
        exponent_ = -100;
        return *this;
    }

    IOUAmount&
    operator+= (IOUAmount const& other);

    IOUAmount&
    operator-= (IOUAmount const& other)
    {
        *this += -other;
        return *this;
    }

    IOUAmount
    operator- () const
    {
        return { -mantissa_, exponent_ };
    }

    bool
    operator==(IOUAmount const& other) const
    {
        return exponent_ == other.exponent_ &&
            mantissa_ == other.mantissa_;
    }

    bool
    operator<(IOUAmount const& other) const;

    /*如果金额不是零，则返回“真”*/
    explicit
    operator bool() const noexcept
    {
        return mantissa_ != 0;
    }

    /*返回金额的符号*/
    int
    signum() const noexcept
    {
        return (mantissa_ < 0) ? -1 : (mantissa_ ? 1 : 0);
    }

    int
    exponent() const noexcept
    {
        return exponent_;
    }

    std::int64_t
    mantissa() const noexcept
    {
        return mantissa_;
    }
};

std::string
to_string (IOUAmount const& amount);

/*返回数字*金额/日期
   这个函数比计算更精确
   num*amt，将结果存储在iouamount中，然后
   除以den。
**/

IOUAmount
mulRatio (
    IOUAmount const& amt,
    std::uint32_t num,
    std::uint32_t den,
    bool roundUp);

}

#endif
