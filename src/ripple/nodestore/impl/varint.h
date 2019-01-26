
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
    版权所有2014，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_NUDB_VARINT_H_INCLUDED
#define BEAST_NUDB_VARINT_H_INCLUDED

#include <nudb/detail/stream.hpp>
#include <cstdint>
#include <type_traits>

namespace ripple {
namespace NodeStore {

//这是base128变量格式的变体，来自
//Google协议缓冲区：
//https://developers.google.com/protocol buffers/docs/encoding变量

//字段标记
struct varint;

//返回最大值的元函数
//T的可能大小表示为变量。
//t必须是无符号的
template <class T,
    bool = std::is_unsigned<T>::value>
struct varint_traits;

template <class T>
struct varint_traits<T, true>
{
    explicit varint_traits() = default;

    static std::size_t constexpr max =
        (8 * sizeof(T) + 6) / 7;
};

//返回：已消耗的字节数或出错时为0，
//如果缓冲区太小或T溢出。
//
template <class = void>
std::size_t
read_varint (void const* buf,
    std::size_t buflen, std::size_t& t)
{
    t = 0;
    std::uint8_t const* p =
        reinterpret_cast<
            std::uint8_t const*>(buf);
    std::size_t n = 0;
    while (p[n] & 0x80)
        if (++n >= buflen)
            return 0;
    if (++n > buflen)
        return 0;
//0的特殊情况
    if (n == 1 && *p == 0)
    {
        t = 0;
        return 1;
    }
    auto const used = n;
    while (n--)
    {
        auto const d = p[n];
        auto const t0 = t;
        t *= 127;
        t += d & 0x7f;
        if (t <= t0)
return 0; //溢流
    }
    return used;
}

template <class T,
    std::enable_if_t<std::is_unsigned<
        T>::value>* = nullptr>
std::size_t
size_varint (T v)
{
    std::size_t n = 0;
    do
    {
        v /= 127;
        ++n;
    }
    while (v != 0);
    return n;
}

template <class = void>
std::size_t
write_varint (void* p0, std::size_t v)
{
    std::uint8_t* p = reinterpret_cast<
        std::uint8_t*>(p0);
    do
    {
        std::uint8_t d =
            v % 127;
        v /= 127;
        if (v != 0)
            d |= 0x80;
        *p++ = d;
    }
    while (v != 0);
    return p - reinterpret_cast<
        std::uint8_t*>(p0);
}

//输入流

template <class T, std::enable_if_t<
    std::is_same<T, varint>::value>* = nullptr>
void
read (nudb::detail::istream& is, std::size_t& u)
{
    auto p0 = is(1);
    auto p1 = p0;
    while (*p1++ & 0x80)
        is(1);
    read_varint(p0, p1 - p0, u);
}

//输出流

template <class T, std::enable_if_t<
    std::is_same<T, varint>::value>* = nullptr>
void
write (nudb::detail::ostream& os, std::size_t t)
{
    write_varint(os.data(
        size_varint(t)), t);
}

} //节点存储
} //涟漪

#endif
