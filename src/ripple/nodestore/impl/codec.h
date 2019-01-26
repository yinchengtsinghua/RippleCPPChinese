
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

#ifndef RIPPLE_NODESTORE_CODEC_H_INCLUDED
#define RIPPLE_NODESTORE_CODEC_H_INCLUDED

//由于与clang属性不兼容，禁用lz4拒绝警告
#define LZ4_DISABLE_DEPRECATE_WARNINGS

#include <ripple/basics/contract.h>
#include <nudb/detail/field.hpp>
#include <ripple/nodestore/impl/varint.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/protocol/HashPrefix.h>
#include <lz4.h>
#include <cstddef>
#include <cstring>
#include <string>
#include <utility>

namespace ripple {
namespace NodeStore {

template <class BufferFactory>
std::pair<void const*, std::size_t>
lz4_decompress (void const* in,
    std::size_t in_size, BufferFactory&& bf)
{
    using std::runtime_error;
    using namespace nudb::detail;
    std::pair<void const*, std::size_t> result;
    std::uint8_t const* p = reinterpret_cast<
        std::uint8_t const*>(in);
    auto const n = read_varint(
        p, in_size, result.second);
    if (n == 0)
        Throw<std::runtime_error> (
            "lz4 decompress: n == 0");
    void* const out = bf(result.second);
    result.first = out;
    if (LZ4_decompress_fast(
        reinterpret_cast<char const*>(in) + n,
            reinterpret_cast<char*>(out),
                result.second) + n != in_size)
        Throw<std::runtime_error> (
            "lz4 decompress: LZ4_decompress_fast");
    return result;
}

template <class BufferFactory>
std::pair<void const*, std::size_t>
lz4_compress (void const* in,
    std::size_t in_size, BufferFactory&& bf)
{
    using std::runtime_error;
    using namespace nudb::detail;
    std::pair<void const*, std::size_t> result;
    std::array<std::uint8_t, varint_traits<
        std::size_t>::max> vi;
    auto const n = write_varint(
        vi.data(), in_size);
    auto const out_max =
        LZ4_compressBound(in_size);
    std::uint8_t* out = reinterpret_cast<
        std::uint8_t*>(bf(n + out_max));
    result.first = out;
    std::memcpy(out, vi.data(), n);
    auto const out_size = LZ4_compress_default(
        reinterpret_cast<char const*>(in),
            reinterpret_cast<char*>(out + n),
                in_size, out_max);
    if (out_size == 0)
        Throw<std::runtime_error> (
            "lz4 compress");
    result.second = n + out_size;
    return result;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*
    对象类型：

    0=未压缩
    1=LZ4压缩
    2=内部节点压缩
    3=完整内部节点
**/


template <class BufferFactory>
std::pair<void const*, std::size_t>
nodeobject_decompress (void const* in,
    std::size_t in_size, BufferFactory&& bf)
{
    using namespace nudb::detail;

    std::uint8_t const* p = reinterpret_cast<
        std::uint8_t const*>(in);
    std::size_t type;
    auto const vn = read_varint(
        p, in_size, type);
    if (vn == 0)
        Throw<std::runtime_error> (
            "nodeobject decompress");
    p += vn;
    in_size -= vn;

    std::pair<void const*, std::size_t> result;
    switch(type)
    {
case 0: //未压缩的
    {
        result.first = p;
        result.second = in_size;
        break;
    }
case 1: //LZ4
    {
        result = lz4_decompress(
            p, in_size, bf);
        break;
    }
case 2: //压缩v1内部节点
    {
        auto const hs =
field<std::uint16_t>::size; //面具
        if (in_size < hs + 32)
            Throw<std::runtime_error> (
                "nodeobject codec v1: short inner node size: "
                + std::string("in_size = ") + std::to_string(in_size)
                + " hs = " + std::to_string(hs));
        istream is(p, in_size);
        std::uint16_t mask;
read<std::uint16_t>(is, mask);  //面具
        in_size -= hs;
        result.second = 525;
        void* const out = bf(result.second);
        result.first = out;
        ostream os(out, result.second);
        write<std::uint32_t>(os, 0);
        write<std::uint32_t>(os, 0);
        write<std::uint8_t> (os, hotUNKNOWN);
        write<std::uint32_t>(os,
            static_cast<std::uint32_t>(HashPrefix::innerNode));
        if (mask == 0)
            Throw<std::runtime_error> (
                "nodeobject codec v1: empty inner node");
        std::uint16_t bit = 0x8000;
        for (int i = 16; i--; bit >>= 1)
        {
            if (mask & bit)
            {
                if (in_size < 32)
                    Throw<std::runtime_error> (
                        "nodeobject codec v1: short inner node subsize: "
                        + std::string("in_size = ") + std::to_string(in_size)
                        + " i = " + std::to_string(i));
                std::memcpy(os.data(32), is(32), 32);
                in_size -= 32;
            }
            else
            {
                std::memset(os.data(32), 0, 32);
            }
        }
        if (in_size > 0)
            Throw<std::runtime_error> (
                "nodeobject codec v1: long inner node, in_size = "
                + std::to_string(in_size));
        break;
    }
case 3: //完整v1内部节点
    {
if (in_size != 16 * 32) //散列
            Throw<std::runtime_error> (
                "nodeobject codec v1: short full inner node, in_size = "
                + std::to_string(in_size));
        istream is(p, in_size);
        result.second = 525;
        void* const out = bf(result.second);
        result.first = out;
        ostream os(out, result.second);
        write<std::uint32_t>(os, 0);
        write<std::uint32_t>(os, 0);
        write<std::uint8_t> (os, hotUNKNOWN);
        write<std::uint32_t>(os,
            static_cast<std::uint32_t>(HashPrefix::innerNode));
        write(os, is(512), 512);
        break;
    }
case 5: //压缩的v2内部节点
    {
        auto const hs =
field<std::uint16_t>::size; //掩模尺寸
        if (in_size < hs + 65)
            Throw<std::runtime_error> (
                "nodeobject codec v2: short inner node size: "
                + std::string("size = ") + std::to_string(in_size)
                + " hs = " + std::to_string(hs));
        istream is(p, in_size);
        std::uint16_t mask;
read<std::uint16_t>(is, mask);  //面具
        in_size -= hs;
        std::uint8_t depth;
        read<std::uint8_t>(is, depth);
        in_size -= 1;
        result.second = 525 + 1 + (depth+1)/2;
        void* const out = bf(result.second);
        result.first = out;
        ostream os(out, result.second);
        write<std::uint32_t>(os, 0);
        write<std::uint32_t>(os, 0);
        write<std::uint8_t> (os, hotUNKNOWN);
        write<std::uint32_t>(os,
            static_cast<std::uint32_t>(HashPrefix::innerNodeV2));
        if (mask == 0)
            Throw<std::runtime_error> (
                "nodeobject codec v2: empty inner node");
        std::uint16_t bit = 0x8000;
        for (int i = 16; i--; bit >>= 1)
        {
            if (mask & bit)
            {
                if (in_size < 32)
                    Throw<std::runtime_error> (
                        "nodeobject codec v2: short inner node subsize: "
                        + std::string("in_size = ") + std::to_string(in_size)
                        + " i = " + std::to_string(i));
                std::memcpy(os.data(32), is(32), 32);
                in_size -= 32;
            }
            else
            {
                std::memset(os.data(32), 0, 32);
            }
        }
        write<std::uint8_t>(os, depth);
        if (in_size < (depth+1)/2)
            Throw<std::runtime_error> (
                "nodeobject codec v2: short inner node: "
                + std::string("size = ") + std::to_string(in_size)
                + " depth = " + std::to_string(depth));
        std::memcpy(os.data((depth+1)/2), is((depth+1)/2), (depth+1)/2);
        in_size -= (depth+1)/2;
        if (in_size > 0)
            Throw<std::runtime_error> (
                "nodeobject codec v2: long inner node, in_size = "
                + std::to_string(in_size));
        break;
    }
case 6: //完整的v2内部节点
    {
        istream is(p, in_size);
        std::uint8_t depth;
        read<std::uint8_t>(is, depth);
        in_size -= 1;
        result.second = 525 + 1 + (depth+1)/2;
if (in_size != 16 * 32 + (depth+1)/2) //哈希和公共
            Throw<std::runtime_error> (
                "nodeobject codec v2: short full inner node: "
                + std::string("size = ") + std::to_string(in_size)
                + " depth = " + std::to_string(depth));
        void* const out = bf(result.second);
        result.first = out;
        ostream os(out, result.second);
        write<std::uint32_t>(os, 0);
        write<std::uint32_t>(os, 0);
        write<std::uint8_t> (os, hotUNKNOWN);
        write<std::uint32_t>(os,
            static_cast<std::uint32_t>(HashPrefix::innerNodeV2));
        write(os, is(512), 512);
        write<std::uint8_t>(os, depth);
        write(os, is((depth+1)/2), (depth+1)/2);
        break;
    }
    default:
        Throw<std::runtime_error> (
            "nodeobject codec: bad type=" +
                std::to_string(type));
    };
    return result;
}

template <class = void>
void const*
zero32()
{
    static std::array<char, 32> v =
        []
        {
            std::array<char, 32> v;
            v.fill(0);
            return v;
        }();
    return v.data();
}

template <class BufferFactory>
std::pair<void const*, std::size_t>
nodeobject_compress (void const* in,
    std::size_t in_size, BufferFactory&& bf)
{
    using std::runtime_error;
    using namespace nudb::detail;

    std::size_t type = 1;
//检查内部节点v1
    if (in_size == 525)
    {
        istream is(in, in_size);
        std::uint32_t index;
        std::uint32_t unused;
        std::uint8_t  kind;
        std::uint32_t prefix;
        read<std::uint32_t>(is, index);
        read<std::uint32_t>(is, unused);
        read<std::uint8_t> (is, kind);
        read<std::uint32_t>(is, prefix);
        if (prefix == HashPrefix::innerNode)
        {
            std::size_t n = 0;
            std::uint16_t mask = 0;
            std::array<
                std::uint8_t, 512> vh;
            for (unsigned bit = 0x8000;
                bit; bit >>= 1)
            {
                void const* const h = is(32);
                if (std::memcmp(
                        h, zero32(), 32) == 0)
                    continue;
                std::memcpy(
                    vh.data() + 32 * n, h, 32);
                mask |= bit;
                ++n;
            }
            std::pair<void const*,
                std::size_t> result;
            if (n < 16)
            {
//2=v1内部节点压缩
                auto const type = 2U;
                auto const vs = size_varint(type);
                result.second =
                    vs +
field<std::uint16_t>::size +    //面具
n * 32;                         //散列
                std::uint8_t* out = reinterpret_cast<
                    std::uint8_t*>(bf(result.second));
                result.first = out;
                ostream os(out, result.second);
                write<varint>(os, type);
                write<std::uint16_t>(os, mask);
                write(os, vh.data(), n * 32);
                return result;
            }
//3=完整v1内部节点
            auto const type = 3U;
            auto const vs = size_varint(type);
            result.second =
                vs +
n * 32;                         //散列
            std::uint8_t* out = reinterpret_cast<
                std::uint8_t*>(bf(result.second));
            result.first = out;
            ostream os(out, result.second);
            write<varint>(os, type);
            write(os, vh.data(), n * 32);
            return result;
        }
    }

//检查内部节点v2
    if (526 <= in_size && in_size <= 556)
    {
        istream is(in, in_size);
        std::uint32_t index;
        std::uint32_t unused;
        std::uint8_t  kind;
        std::uint32_t prefix;
        read<std::uint32_t>(is, index);
        read<std::uint32_t>(is, unused);
        read<std::uint8_t> (is, kind);
        read<std::uint32_t>(is, prefix);
        if (prefix == HashPrefix::innerNodeV2)
        {
            std::size_t n = 0;
            std::uint16_t mask = 0;
            std::array<
                std::uint8_t, 512> vh;
            for (unsigned bit = 0x8000;
                bit; bit >>= 1)
            {
                void const* const h = is(32);
                if (std::memcmp(
                        h, zero32(), 32) == 0)
                    continue;
                std::memcpy(
                    vh.data() + 32 * n, h, 32);
                mask |= bit;
                ++n;
            }
            std::uint8_t depth;
            read<std::uint8_t>(is, depth);
            std::array<std::uint8_t, 32> common{};
            for (unsigned d = 0; d < (depth+1)/2; ++d)
                read<std::uint8_t>(is, common[d]);
            std::pair<void const*,
                std::size_t> result;
            if (n < 16)
            {
//5=V2内部节点压缩
                auto const type = 5U;
                auto const vs = size_varint(type);
                result.second =
                    vs +
field<std::uint16_t>::size +    //面具
n * 32 +                        //散列
1 +                             //深度
(depth+1)/2;                    //公共前缀
                std::uint8_t* out = reinterpret_cast<
                    std::uint8_t*>(bf(result.second));
                result.first = out;
                ostream os(out, result.second);
                write<varint>(os, type);
                write<std::uint16_t>(os, mask);
                write<std::uint8_t>(os, depth);
                write(os, vh.data(), n * 32);
                for (unsigned d = 0; d < (depth+1)/2; ++d)
                    write<std::uint8_t>(os, common[d]);
                return result;
            }
//6=完整的V2内部节点
            auto const type = 6U;
            auto const vs = size_varint(type);
            result.second =
                vs +
n * 32 +                        //散列
1 +                             //深度
(depth+1)/2;                    //公共前缀
            std::uint8_t* out = reinterpret_cast<
                std::uint8_t*>(bf(result.second));
            result.first = out;
            ostream os(out, result.second);
            write<varint>(os, type);
            write<std::uint8_t>(os, depth);
            write(os, vh.data(), n * 32);
            for (unsigned d = 0; d < (depth+1)/2; ++d)
                write<std::uint8_t>(os, common[d]);
            return result;
        }
    }

    std::array<std::uint8_t, varint_traits<
        std::size_t>::max> vi;
    auto const vn = write_varint(
        vi.data(), type);
    std::pair<void const*, std::size_t> result;
    switch(type)
    {
//案例0是未压缩的数据；我们现在总是进行压缩。
case 1: //LZ4
    {
        std::uint8_t* p;
        auto const lzr = NodeStore::lz4_compress(
                in, in_size, [&p, &vn, &bf]
            (std::size_t n)
            {
                p = reinterpret_cast<
                    std::uint8_t*>(
                        bf(vn + n));
                return p + vn;
            });
        std::memcpy(p, vi.data(), vn);
        result.first = p;
        result.second = vn + lzr.second;
        break;
    }
    default:
        Throw<std::logic_error> (
            "nodeobject codec: unknown=" +
                std::to_string(type));
    };
    return result;
}

//修改内部节点以删除分类帐
//序列和类型信息，因此编解码器
//验证可以通过。
//
template <class = void>
void
filter_inner (void* in, std::size_t in_size)
{
    using namespace nudb::detail;

//检查内部节点
    if (in_size == 525)
    {
        istream is(in, in_size);
        std::uint32_t index;
        std::uint32_t unused;
        std::uint8_t  kind;
        std::uint32_t prefix;
        read<std::uint32_t>(is, index);
        read<std::uint32_t>(is, unused);
        read<std::uint8_t> (is, kind);
        read<std::uint32_t>(is, prefix);
        if (prefix == HashPrefix::innerNode)
        {
            ostream os(in, 9);
            write<std::uint32_t>(os, 0);
            write<std::uint32_t>(os, 0);
            write<std::uint8_t> (os, hotUNKNOWN);
        }
    }
}

} //节点存储
} //涟漪

#endif
