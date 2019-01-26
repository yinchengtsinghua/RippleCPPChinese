
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
    版权所有（c）2016 Ripple Labs Inc.

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

#ifndef RIPPLE_CONDITIONS_UTILS_H
#define RIPPLE_CONDITIONS_UTILS_H

#include <ripple/basics/strHex.h>
#include <ripple/conditions/impl/error.h>
#include <boost/dynamic_bitset.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <utility>

namespace ripple {
namespace cryptoconditions {

//用于解码二进制blob的函数集合
//使用X.690可分辨编码规则编码。
//
//这是一个非常普通的解码器，只实现
//支持preimagesha256所需的最小值。
namespace der {

//前导码封装了der标识符和
//长度八位字节：
struct Preamble
{
    explicit Preamble() = default;
    std::uint8_t type = 0;
    std::size_t tag = 0;
    std::size_t length = 0;
};

inline
bool
isPrimitive(Preamble const& p)
{
    return (p.type & 0x20) == 0;
}

inline
bool
isConstructed(Preamble const& p)
{
    return !isPrimitive(p);
}

inline
bool
isUniversal(Preamble const& p)
{
    return (p.type & 0xC0) == 0;
}

inline
bool
isApplication(Preamble const& p)
{
    return (p.type & 0xC0) == 0x40;
}

inline
bool
isContextSpecific(Preamble const& p)
{
    return (p.type & 0xC0) == 0x80;
}

inline
bool
isPrivate(Preamble const& p)
{
    return (p.type & 0xC0) == 0xC0;
}

inline
Preamble
parsePreamble(Slice& s, std::error_code& ec)
{
    Preamble p;

    if (s.size() < 2)
    {
        ec = error::short_preamble;
        return p;
    }

    p.type = s[0] & 0xE0;
    p.tag = s[0] & 0x1F;

    s += 1;

    if (p.tag == 0x1F)
{ //长标签形式，我们不支持：
        ec = error::long_tag;
        return p;
    }

    p.length = s[0];
    s += 1;

    if (p.length & 0x80)
{ //长形长度：
        std::size_t const cnt = p.length & 0x7F;

        if (cnt == 0)
        {
            ec = error::malformed_encoding;
            return p;
        }

        if (cnt > sizeof(std::size_t))
        {
            ec = error::large_size;
            return p;
        }

        if (cnt > s.size())
        {
            ec = error::short_preamble;
            return p;
        }

        p.length = 0;

        for (std::size_t i = 0; i != cnt; ++i)
            p.length = (p.length << 8) + s[i];

        s += cnt;

        if (p.length == 0)
        {
            ec = error::malformed_encoding;
            return p;
        }
    }

    return p;
}

inline
Buffer
parseOctetString(Slice& s, std::uint32_t count, std::error_code& ec)
{
    if (count > s.size())
    {
        ec = error::buffer_underfull;
        return {};
    }

    if (count > 65535)
    {
        ec = error::large_size;
        return {};
    }

    Buffer b(s.data(), count);
    s += count;
    return b;
}

template <class Integer>
Integer
parseInteger(Slice& s, std::size_t count, std::error_code& ec)
{
    Integer v{0};

    if (s.empty())
    {
//不能有大小为零的整数
        ec = error::malformed_encoding;
        return v;
    }

    if (count > s.size())
    {
        ec = error::buffer_underfull;
        return v;
    }

    const bool isSigned = std::numeric_limits<Integer>::is_signed;
//无符号类型可以有前导零八位字节
    const size_t maxLength = isSigned ? sizeof(Integer) : sizeof(Integer) + 1;
    if (count > maxLength)
    {
        ec = error::large_size;
        return v;
    }

    if (!isSigned && (s[0] & (1 << 7)))
    {
//尝试将负数解码为正值
        ec = error::malformed_encoding;
        return v;
    }

    if (!isSigned && count == sizeof(Integer) + 1 && s[0])
    {
//由于整数被编码为2的补码，所以第一个字节可以
//无符号重复为零
        ec = error::malformed_encoding;
        return v;
    }

    v = 0;
    for (size_t i = 0; i < count; ++i)
        v = (v << 8) | (s[i] & 0xff);

    if (isSigned && (s[0] & (1 << 7)))
    {
        for (int i = count; i < sizeof(Integer); ++i)
            v |= (Integer(0xff) << (8 * i));
    }
    s += count;
    return v;
}

} //德尔德
} //密码条件
} //涟漪

#endif
