
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

#ifndef RIPPLE_BASICS_BUFFER_H_INCLUDED
#define RIPPLE_BASICS_BUFFER_H_INCLUDED

#include <ripple/basics/Slice.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>

namespace ripple {

/*像std:：vector<char>但更好。
    满足缓冲工厂的要求。
**/

class Buffer
{
private:
    std::unique_ptr<std::uint8_t[]> p_;
    std::size_t size_ = 0;

public:
    using const_iterator = std::uint8_t const*;

    Buffer() = default;

    /*创建具有给定大小的未初始化缓冲区。*/
    explicit
    Buffer (std::size_t size)
        : p_ (size ? new std::uint8_t[size] : nullptr)
        , size_ (size)
    {
    }

    /*创建一个缓冲区作为现有内存的副本。

        @param data指向现有内存的指针。如果
                    大小不是零，不能为空。
        现有内存块的@param size大小。
    **/

    Buffer (void const* data, std::size_t size)
        : Buffer (size)
    {
        if (size)
            std::memcpy(p_.get(), data, size);
    }

    /*复制结构*/
    Buffer (Buffer const& other)
        : Buffer (other.p_.get(), other.size_)
    {
    }

    /*拷贝赋值*/
    Buffer& operator= (Buffer const& other)
    {
        if (this != &other)
        {
            if (auto p = alloc (other.size_))
                std::memcpy (p, other.p_.get(), size_);
        }
        return *this;
    }

    /*移动构造。
        另一个缓冲区被重置。
    **/

    Buffer (Buffer&& other) noexcept
        : p_ (std::move(other.p_))
        , size_ (other.size_)
    {
        other.size_ = 0;
    }

    /*移动分配。
        另一个缓冲区被重置。
    **/

    Buffer& operator= (Buffer&& other) noexcept
    {
        if (this != &other)
        {
            p_ = std::move(other.p_);
            size_ = other.size_;
            other.size_ = 0;
        }
        return *this;
    }

    /*从切片构造*/
    explicit
    Buffer (Slice s)
        : Buffer (s.data(), s.size())
    {
    }

    /*从切片分配*/
    Buffer& operator= (Slice s)
    {
//确保切片不是缓冲区的子集。
        assert (s.size() == 0 || size_ == 0 ||
            s.data() < p_.get() ||
            s.data() >= p_.get() + size_);

        if (auto p = alloc (s.size()))
            std::memcpy (p, s.data(), s.size());
        return *this;
    }

    /*返回缓冲区中的字节数。*/
    std::size_t
    size() const noexcept
    {
        return size_;
    }

    bool
    empty () const noexcept
    {
        return 0 == size_;
    }

    operator Slice() const noexcept
    {
        if (! size_)
            return Slice{};
        return Slice{ p_.get(), size_ };
    }

    /*返回指向存储开头的指针。
        @注意，返回类型保证为指针
              为了便于指针运算，只需一个字节。
    **/

    /*@ {*/
    std::uint8_t const*
    data() const noexcept
    {
        return p_.get();
    }

    std::uint8_t*
    data() noexcept
    {
        return p_.get();
    }
    /*@ }*/

    /*重置缓冲区。
        所有内存都被释放。结果大小为0。
    **/

    void
    clear() noexcept
    {
        p_.reset();
        size_ = 0;
    }

    /*重新分配存储空间。
        现有数据（如果有）将被丢弃。
    **/

    std::uint8_t*
    alloc (std::size_t n)
    {
        if (n != size_)
        {
            p_.reset(n ? new std::uint8_t[n] : nullptr);
            size_ = n;
        }
        return p_.get();
    }

//满足缓冲工厂要求
    void*
    operator()(std::size_t n)
    {
        return alloc(n);
    }

    const_iterator
    begin() const noexcept
    {
        return p_.get();
    }

    const_iterator
    cbegin() const noexcept
    {
        return p_.get();
    }

    const_iterator
    end() const noexcept
    {
        return p_.get() + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return p_.get() + size_;
    }
};

inline bool operator==(Buffer const& lhs, Buffer const& rhs) noexcept
{
    if (lhs.size() != rhs.size())
        return false;

    if (lhs.size() == 0)
        return true;

    return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

inline bool operator!=(Buffer const& lhs, Buffer const& rhs) noexcept
{
    return !(lhs == rhs);
}

} //涟漪

#endif
