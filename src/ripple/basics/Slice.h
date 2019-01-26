
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

#ifndef RIPPLE_BASICS_SLICE_H_INCLUDED
#define RIPPLE_BASICS_SLICE_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/basics/strHex.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <type_traits>

namespace ripple {

/*不可变的线性字节范围。

    完全构造的切片保证处于有效状态。
    切片是轻量级和可复制的，它不保留所有权。
    基本内存的。
**/

class Slice
{
private:
    std::uint8_t const* data_ = nullptr;
    std::size_t size_ = 0;

public:
    using const_iterator = std::uint8_t const*;

    /*默认构造切片的长度为0。*/
    Slice() noexcept = default;

    Slice (Slice const&) noexcept = default;
    Slice& operator= (Slice const&) noexcept = default;

    /*创建指向现有内存的切片。*/
    Slice (void const* data, std::size_t size) noexcept
        : data_ (reinterpret_cast<std::uint8_t const*>(data))
        , size_ (size)
    {
    }

    /*如果字节范围为空，则返回“true”。*/
    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    /*返回存储中的字节数。

        对于空范围，这可能为零。
    **/

    std::size_t
    size() const noexcept
    {
        return size_;
    }

    /*返回指向存储开头的指针。
        @注意，返回类型保证为指针
              为了便于指针运算，只需一个字节。
    **/

    std::uint8_t const*
    data() const noexcept
    {
        return data_;
    }

    /*访问原始字节。*/
    std::uint8_t
    operator[](std::size_t i) const noexcept
    {
        assert(i < size_);
        return data_[i];
    }

    /*推进缓冲器。*/
    /*@ {*/
    Slice&
    operator+= (std::size_t n)
    {
        if (n > size_)
            Throw<std::domain_error> ("too small");
        data_ += n;
        size_ -= n;
        return *this;
    }

    Slice
    operator+ (std::size_t n) const
    {
        Slice temp = *this;
        return temp += n;
    }
    /*@ }*/


    const_iterator
    begin() const noexcept
    {
        return data_;
    }

    const_iterator
    cbegin() const noexcept
    {
        return data_;
    }

    const_iterator
    end() const noexcept
    {
        return data_ + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return data_ + size_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Hasher>
inline
void
hash_append (Hasher& h, Slice const& v)
{
    h(v.data(), v.size());
}

inline
bool
operator== (Slice const& lhs, Slice const& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    if (lhs.size() == 0)
        return true;

    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline
bool
operator!= (Slice const& lhs, Slice const& rhs) noexcept
{
    return !(lhs == rhs);
}

inline
bool
operator< (Slice const& lhs, Slice const& rhs) noexcept
{
    return std::lexicographical_compare(
        lhs.data(), lhs.data() + lhs.size(),
            rhs.data(), rhs.data() + rhs.size());
}


template <class Stream>
Stream& operator<<(Stream& s, Slice const& v)
{
    s << strHex(v);
    return s;
}

template <class T, std::size_t N>
std::enable_if_t<
    std::is_same<T, char>::value ||
        std::is_same<T, unsigned char>::value,
    Slice
>
makeSlice (std::array<T, N> const& a)
{
    return Slice(a.data(), a.size());
}

template <class T, class Alloc>
std::enable_if_t<
    std::is_same<T, char>::value ||
        std::is_same<T, unsigned char>::value,
    Slice
>
makeSlice (std::vector<T, Alloc> const& v)
{
    return Slice(v.data(), v.size());
}

template <class Traits, class Alloc>
Slice
makeSlice (std::basic_string<char, Traits, Alloc> const& s)
{
    return Slice(s.data(), s.size());
}

} //涟漪

#endif
