
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
    版权所有2014，nikolaos d.bogalis<nikb@bogalis.net>

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

#ifndef BEAST_UTILITY_TAGGED_INTEGER_H_INCLUDED
#define BEAST_UTILITY_TAGGED_INTEGER_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <boost/operators.hpp>
#include <functional>
#include <iostream>
#include <type_traits>
#include <utility>

namespace ripple {

/*A型安全环绕标准整数型

    标记用于实现类型安全性，在处捕获不匹配的类型
    编译时间。包装同一基础积分的多个实例化
    类型是不同的类型（由标记区分），不会进行互操作。一
    标记整数支持所有常见的赋值、算术、比较和
    为基础类型定义的移位操作

    标签不是一个单元，这需要限制
    允许的算术运算。
**/

template <class Int, class Tag>
class tagged_integer
    : boost::totally_ordered<
          tagged_integer<Int, Tag>,
          boost::integer_arithmetic<
              tagged_integer<Int, Tag>,
              boost::bitwise<
                  tagged_integer<Int, Tag>,
                  boost::unit_steppable<
                      tagged_integer<Int, Tag>,
                      boost::shiftable<tagged_integer<Int, Tag>>>>>>
{

private:
    Int m_value;

public:
    using value_type = Int;
    using tag_type = Tag;

    tagged_integer() = default;

    template <
        class OtherInt,
        class = typename std::enable_if<
            std::is_integral<OtherInt>::value &&
            sizeof(OtherInt) <= sizeof(Int)>::type>
    explicit
        /*常量表达式*/
        tagged_integer(OtherInt value) noexcept
        : m_value(value)
    {
        static_assert(
            sizeof(tagged_integer) == sizeof(Int),
            "tagged_integer is adding padding");
    }

    bool
    operator<(const tagged_integer & rhs) const noexcept
    {
        return m_value < rhs.m_value;
    }

    bool
    operator==(const tagged_integer & rhs) const noexcept
    {
        return m_value == rhs.m_value;
    }

    tagged_integer&
    operator+=(tagged_integer const& rhs) noexcept
    {
        m_value += rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator-=(tagged_integer const& rhs) noexcept
    {
        m_value -= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator*=(tagged_integer const& rhs) noexcept
    {
        m_value *= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator/=(tagged_integer const& rhs) noexcept
    {
        m_value /= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator%=(tagged_integer const& rhs) noexcept
    {
        m_value %= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator|=(tagged_integer const& rhs) noexcept
    {
        m_value |= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator&=(tagged_integer const& rhs) noexcept
    {
        m_value &= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator^=(tagged_integer const& rhs) noexcept
    {
        m_value ^= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator<<=(const tagged_integer& rhs) noexcept
    {
        m_value <<= rhs.m_value;
        return *this;
    }

    tagged_integer&
    operator>>=(const tagged_integer& rhs) noexcept
    {
        m_value >>= rhs.m_value;
        return *this;
    }

    tagged_integer
    operator~() const noexcept
    {
        return tagged_integer{~m_value};
    }

    tagged_integer
    operator+() const noexcept
    {
        return *this;
    }

    tagged_integer
    operator-() const noexcept
    {
        return tagged_integer{-m_value};
    }

    tagged_integer&
    operator++ () noexcept
    {
        ++m_value;
        return *this;
    }

    tagged_integer&
    operator-- () noexcept
    {
        --m_value;
        return *this;
    }

    explicit
    operator Int() const noexcept
    {
        return m_value;
    }

    friend
    std::ostream&
    operator<< (std::ostream& s, tagged_integer const& t)
    {
        s << t.m_value;
        return s;
    }

    friend
    std::istream&
    operator>> (std::istream& s, tagged_integer& t)
    {
        s >> t.m_value;
        return s;
    }

    friend
    std::string
    to_string(tagged_integer const& t)
    {
        return std::to_string(t.m_value);
    }
};

} //涟漪

namespace beast {
template <class Int, class Tag, class HashAlgorithm>
struct is_contiguously_hashable<ripple::tagged_integer<Int, Tag>, HashAlgorithm>
    : public is_contiguously_hashable<Int, HashAlgorithm>
{
    explicit is_contiguously_hashable() = default;
};

} //野兽
#endif

