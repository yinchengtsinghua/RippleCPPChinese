
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

//版权所有（c）2009-2010 Satoshi Nakamoto
//版权所有（c）2011比特币开发商
//在MIT/X11软件许可证下分发，请参见随附的
//文件license.txt或http://www.opensource.org/licenses/mit-license.php。

#ifndef RIPPLE_BASICS_BASE_UINT_H_INCLUDED
#define RIPPLE_BASICS_BASE_UINT_H_INCLUDED

#include <ripple/basics/Blob.h>
#include <ripple/basics/strHex.h>
#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/utility/Zero.h>
#include <boost/endian/conversion.hpp>
#include <boost/functional/hash.hpp>
#include <array>
#include <functional>
#include <type_traits>

namespace ripple {

//此类在内部以big endian格式存储其值

template <std::size_t Bits, class Tag = void>
class base_uint
{
    static_assert ((Bits % 32) == 0,
        "The length of a base_uint in bits must be a multiple of 32.");

    static_assert (Bits >= 64,
        "The length of a base_uint in bits must be at least 64.");

protected:
    static constexpr std::size_t WIDTH = Bits / 32;

//这是按字节顺序排列的大尾数。
//我们有时会使用std：：uint32_t来表示速度。

    using array_type = std::array<std::uint32_t, WIDTH>;
    array_type pn;

public:
//——————————————————————————————————————————————————————————————
//
//STL容器接口
//

    static std::size_t constexpr bytes = Bits / 8;
    static_assert(sizeof(array_type) == bytes, "");

    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using value_type             = unsigned char;
    using pointer                = value_type*;
    using reference              = value_type&;
    using const_pointer          = value_type const*;
    using const_reference        = value_type const&;
    using iterator               = pointer;
    using const_iterator         = const_pointer;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using tag_type               = Tag;

    pointer data() { return reinterpret_cast<pointer>(pn.data ()); }
    const_pointer data() const { return reinterpret_cast<const_pointer>(pn.data ()); }


    iterator begin() { return data(); }
    iterator end()   { return data()+bytes; }
    const_iterator begin()  const { return data(); }
    const_iterator end()    const { return data()+bytes; }
    const_iterator cbegin() const { return data(); }
    const_iterator cend()   const { return data()+bytes; }

    /*值散列函数。
        种子防止精心编制的输入导致父容器退化。
    **/

    using hasher = hardened_hash <>;

//——————————————————————————————————————————————————————————————

private:
    /*从原始指针构造。
        “data”指向的缓冲区必须至少为位/8字节。

        @注意，该结构用于从std：：uint64中消除此歧义
              构造器：类似基值uint（0）的内容不明确。
    **/

//nikb todo不再需要这个构造函数。
    struct VoidHelper
    {
        explicit VoidHelper() = default;
    };

    explicit base_uint (void const* data, VoidHelper)
    {
        memcpy (pn.data (), data, bytes);
    }

public:
    base_uint()
    {
        *this = beast::zero;
    }

    base_uint(beast::Zero)
    {
        *this = beast::zero;
    }

    explicit base_uint (Blob const& vch)
    {
        assert (vch.size () == size ());

        if (vch.size () == size ())
            memcpy (pn.data (), vch.data (), size ());
        else
            *this = beast::zero;
    }

    explicit base_uint (std::uint64_t b)
    {
        *this = b;
    }

    template <class OtherTag>
    void copyFrom (base_uint<Bits, OtherTag> const& other)
    {
        memcpy (pn.data (), other.data(), bytes);
    }

    /*从原始指针构造。
        “data”指向的缓冲区必须至少为位/8字节。
    **/

    static base_uint
    fromVoid (void const* data)
    {
        return base_uint (data, VoidHelper ());
    }

    int signum() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (pn[i] != 0)
                return 1;

        return 0;
    }

    bool operator! () const
    {
        return *this == beast::zero;
    }

    const base_uint operator~ () const
    {
        base_uint ret;

        for (int i = 0; i < WIDTH; i++)
            ret.pn[i] = ~pn[i];

        return ret;
    }

    base_uint& operator= (std::uint64_t uHost)
    {
        *this = beast::zero;
        union
        {
            unsigned u[2];
            std::uint64_t ul;
        };
//放入最低有效位。
        ul = boost::endian::native_to_big(uHost);
        pn[WIDTH-2] = u[0];
        pn[WIDTH-1] = u[1];
        return *this;
    }

    base_uint& operator^= (const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] ^= b.pn[i];

        return *this;
    }

    base_uint& operator&= (const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] &= b.pn[i];

        return *this;
    }

    base_uint& operator|= (const base_uint& b)
    {
        for (int i = 0; i < WIDTH; i++)
            pn[i] |= b.pn[i];

        return *this;
    }

    base_uint& operator++ ()
    {
//前缀算子
        for (int i = WIDTH - 1; i >= 0; --i)
        {
            pn[i] = boost::endian::native_to_big (boost::endian::big_to_native(pn[i]) + 1);

            if (pn[i] != 0)
                break;
        }

        return *this;
    }

    const base_uint operator++ (int)
    {
//后缀运算符
        const base_uint ret = *this;
        ++ (*this);

        return ret;
    }

    base_uint& operator-- ()
    {
        for (int i = WIDTH - 1; i >= 0; --i)
        {
            auto prev = pn[i];
            pn[i] = boost::endian::native_to_big (boost::endian::big_to_native(pn[i]) - 1);

            if (prev != 0)
                break;
        }

        return *this;
    }

    const base_uint operator-- (int)
    {
//后缀运算符
        const base_uint ret = *this;
        -- (*this);

        return ret;
    }

    base_uint& operator+= (const base_uint& b)
    {
        std::uint64_t carry = 0;

        for (int i = WIDTH; i--;)
        {
            std::uint64_t n = carry + boost::endian::big_to_native(pn[i]) +
                boost::endian::big_to_native(b.pn[i]);

            pn[i] = boost::endian::native_to_big (static_cast<std::uint32_t>(n));
            carry = n >> 32;
        }

        return *this;
    }

    template <class Hasher,
              class = std::enable_if_t<Hasher::endian != beast::endian::native>>
    friend void hash_append(
        Hasher& h, base_uint const& a) noexcept
    {
//不允许在此内存上进行任何endian转换
        h(a.pn.data (), sizeof(a.pn));
    }

    /*将十六进制字符串解析为基字符串
        字符串必须正好包含字节*2个十六进制字符，并且不能
        有任何前导或尾随空格。
    **/

    bool SetHexExact (const char* psz)
    {
        unsigned char* pOut  = begin ();

        for (int i = 0; i < sizeof (pn); ++i)
        {
            auto hi = charUnHex(*psz++);
            if (hi == -1)
                return false;

            auto lo = charUnHex (*psz++);
            if (lo == -1)
                return false;

            *pOut++ = (hi << 4) | lo;
        }

//我们现在消耗的字节正好和我们需要的一样多。
//所以我们应该在绳子的末端。
        return (*psz == 0);
    }

    /*将十六进制字符串解析为基字符串
        输入可以是：
            -短于完整十六进制表示，不包括前导
              零点。
            -比全十六进制表示长，在这种情况下，前导
              字节被丢弃。

        完成分析后，字符串必须完全使用
        剩余空终止符。

        当bstrict为false时，解析将在非严格模式下完成，如果
        当前，将跳过前导空格和0x前缀。
    **/

    bool SetHex (const char* psz, bool bStrict = false)
    {
//找到开始。
        auto pBegin = reinterpret_cast<const unsigned char*>(psz);
//跳过前导空格
        if (!bStrict)
            while (isspace(*pBegin))
                pBegin++;

//跳过0X
        if (!bStrict && pBegin[0] == '0' && tolower(pBegin[1]) == 'x')
            pBegin += 2;

//寻找结束。
        auto pEnd = pBegin;
        while (charUnHex(*pEnd) != -1)
            pEnd++;

//只取过长字符串的最后一位数字。
        if ((unsigned int) (pEnd - pBegin) > 2 * size ())
            pBegin = pEnd - 2 * size ();

        unsigned char* pOut = end () - ((pEnd - pBegin + 1) / 2);

        *this = beast::zero;

        if ((pEnd - pBegin) & 1)
            *pOut++ = charUnHex(*pBegin++);

        while (pBegin != pEnd)
        {
            auto cHigh = charUnHex(*pBegin++);
            auto cLow  = pBegin == pEnd
                            ? 0
                            : charUnHex(*pBegin++);

            if (cHigh == -1 || cLow == -1)
                return false;

            *pOut++ = (cHigh << 4) | cLow;
        }

        return !*pEnd;
    }

    bool SetHex (std::string const& str, bool bStrict = false)
    {
        return SetHex (str.c_str (), bStrict);
    }

    bool SetHexExact (std::string const& str)
    {
        return SetHexExact (str.c_str ());
    }

    constexpr static std::size_t size ()
    {
        return bytes;
    }

    base_uint<Bits, Tag>& operator=(beast::Zero)
    {
        pn.fill(0);
        return *this;
    }

//不赞成的
    bool isZero () const { return *this == beast::zero; }
    bool isNonZero () const { return *this != beast::zero; }
    void zero () { *this = beast::zero; }
};

using uint128 = base_uint<128>;
using uint160 = base_uint<160>;
using uint256 = base_uint<256>;

template <std::size_t Bits, class Tag>
inline int compare (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    auto ret = std::mismatch (a.cbegin (), a.cend (), b.cbegin ());

    if (ret.first == a.cend ())
        return 0;

//A> B
    if (*ret.first > *ret.second)
        return 1;

//A＜B
    return -1;
}

template <std::size_t Bits, class Tag>
inline bool operator< (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) < 0;
}

template <std::size_t Bits, class Tag>
inline bool operator<= (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) <= 0;
}

template <std::size_t Bits, class Tag>
inline bool operator> (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) > 0;
}

template <std::size_t Bits, class Tag>
inline bool operator>= (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) >= 0;
}

template <std::size_t Bits, class Tag>
inline bool operator== (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) == 0;
}

template <std::size_t Bits, class Tag>
inline bool operator!= (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return compare (a, b) != 0;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
template <std::size_t Bits, class Tag>
inline bool operator== (base_uint<Bits, Tag> const& a, std::uint64_t b)
{
    return a == base_uint<Bits, Tag>(b);
}

template <std::size_t Bits, class Tag>
inline bool operator!= (base_uint<Bits, Tag> const& a, std::uint64_t b)
{
    return !(a == b);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
template <std::size_t Bits, class Tag>
inline const base_uint<Bits, Tag> operator^ (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return base_uint<Bits, Tag> (a) ^= b;
}

template <std::size_t Bits, class Tag>
inline const base_uint<Bits, Tag> operator& (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return base_uint<Bits, Tag> (a) &= b;
}

template <std::size_t Bits, class Tag>
inline const base_uint<Bits, Tag> operator| (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return base_uint<Bits, Tag> (a) |= b;
}

template <std::size_t Bits, class Tag>
inline const base_uint<Bits, Tag> operator+ (
    base_uint<Bits, Tag> const& a, base_uint<Bits, Tag> const& b)
{
    return base_uint<Bits, Tag> (a) += b;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
template <std::size_t Bits, class Tag>
inline std::string to_string (base_uint<Bits, Tag> const& a)
{
    return strHex (a.cbegin (), a.cend ());
}

//返回以十六进制表示的给定文本的基值的函数模板。
//像这样调用：
//auto i=from_hex_text<uint256>（“aaaaa”）；
template <typename T>
auto from_hex_text (char const* text) -> std::enable_if_t<
    std::is_same<T, base_uint<T::bytes*8, typename T::tag_type>>::value, T>
{
    T ret;
    ret.SetHex (text);
    return ret;
}

template <typename T>
auto from_hex_text (std::string const& text) -> std::enable_if_t<
    std::is_same<T, base_uint<T::bytes*8, typename T::tag_type>>::value, T>
{
    T ret;
    ret.SetHex (text);
    return ret;
}

template <std::size_t Bits, class Tag>
inline std::ostream& operator<< (
    std::ostream& out, base_uint<Bits, Tag> const& u)
{
    return out << to_string (u);
}

#ifndef __INTELLISENSE__
static_assert(sizeof(uint128) == 128/8, "There should be no padding bytes");
static_assert(sizeof(uint160) == 160/8, "There should be no padding bytes");
static_assert(sizeof(uint256) == 256/8, "There should be no padding bytes");
#endif

} //波纹的

namespace beast
{

template <std::size_t Bits, class Tag>
struct is_uniquely_represented<ripple::base_uint<Bits, Tag>>
    : public std::true_type
    {
        explicit is_uniquely_represented() = default;
    };

}  //野兽

#endif
