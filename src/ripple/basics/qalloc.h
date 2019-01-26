
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

#ifndef RIPPLE_BASICS_QALLOC_H_INCLUDED
#define RIPPLE_BASICS_QALLOC_H_INCLUDED

#include <ripple/basics/contract.h>
#include <ripple/basics/ByteUtilities.h>
#include <boost/intrusive/list.hpp>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <sstream>
#include <type_traits>

namespace ripple {

namespace detail {

template <class = void>
class qalloc_impl
{
private:
    class block
    {
    private:
        std::size_t count_ = 0;
        std::size_t bytes_;
        std::size_t remain_;
        std::uint8_t* free_;

    public:
        block* next;

        block (block const&) = delete;
        block& operator= (block const&) = delete;

        explicit
        block (std::size_t bytes);

        void*
        allocate (std::size_t bytes, std::size_t align);

        bool
        deallocate();
    };

    block* used_ = nullptr;
    block* free_ = nullptr;

public:
    static constexpr auto block_size = kilobytes(256);

    qalloc_impl() = default;
    qalloc_impl (qalloc_impl const&) = delete;
    qalloc_impl& operator= (qalloc_impl const&) = delete;

    ~qalloc_impl();

    void*
    allocate (std::size_t bytes, std::size_t align);

    void
    deallocate (void* p);
};

} //细节

template <class T, bool ShareOnCopy = true>
class qalloc_type
{
private:
    template <class, bool>
    friend class qalloc_type;

    std::shared_ptr<
        detail::qalloc_impl<>> impl_;

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = T const*;
    using reference = typename
        std::add_lvalue_reference<T>::type;
    using const_reference = typename
        std::add_lvalue_reference<T const>::type;
    using propagate_on_container_move_assignment =
        std::true_type;

    template <class U>
    struct rebind
    {
        explicit rebind() = default;

        using other = qalloc_type<U, ShareOnCopy>;
    };

    qalloc_type (qalloc_type const&) = default;
    qalloc_type (qalloc_type&& other) noexcept = default;
    qalloc_type& operator= (qalloc_type const&) = default;
    qalloc_type& operator= (qalloc_type&&) noexcept = default;

    qalloc_type();

    template <class U>
    qalloc_type (qalloc_type<U, ShareOnCopy> const& u);

    template <class U>
    U*
    alloc (std::size_t n);

    template <class U>
    void
    dealloc (U* p, std::size_t n);

    T*
    allocate (std::size_t n);

    void
    deallocate (T* p, std::size_t n);

    template <class U>
    bool
    operator== (qalloc_type<U, ShareOnCopy> const& u);

    template <class U>
    bool
    operator!= (qalloc_type<U, ShareOnCopy> const& u);

    qalloc_type
    select_on_container_copy_construction() const;

private:
    qalloc_type
    select_on_copy(std::true_type) const;

    qalloc_type
    select_on_copy(std::false_type) const;
};

/*为按时间顺序删除而优化的分配器。

    这个分配器针对对象的情况进行了优化
    删除顺序与删除顺序大致相同
    是创造出来的。

    线程安全：

        不能同时调用。
**/

using qalloc = qalloc_type<int, true>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

template <class _>
qalloc_impl<_>::block::block (std::size_t bytes)
    : bytes_ (bytes - sizeof(*this))
    , remain_ (bytes_)
    , free_ (reinterpret_cast<
        std::uint8_t*>(this + 1))
{
}

template <class _>
void*
qalloc_impl<_>::block::allocate(
    std::size_t bytes, std::size_t align)
{
    align = std::max(align,
        std::alignment_of<block*>::value);
    auto pad = [](void const* p, std::size_t align)
    {
        auto const i = reinterpret_cast<
            std::uintptr_t>(p);
        return (align - (i % align)) % align;
    };

    auto const n0 =
        pad(free_ + sizeof(block*), align);
    auto const n1 =
        n0 + sizeof(block*) + bytes;
    if (remain_ < n1)
        return nullptr;
    auto p = reinterpret_cast<block**>(
        free_ + n0 + sizeof(block*));
    assert(pad(p - 1,
        std::alignment_of<block*>::value) == 0);
    p[-1] = this;
    ++count_;
    free_ += n1;
    remain_ -= n1;
    return p;
}

template <class _>
bool
qalloc_impl<_>::block::deallocate()
{
    --count_;
    if (count_ > 0)
        return false;
    remain_ = bytes_;
    free_ = reinterpret_cast<
        std::uint8_t*>(this + 1);
    return true;
}

template <class _>
qalloc_impl<_>::~qalloc_impl()
{
    if (used_)
    {
        used_->~block();
        std::free(used_);
    }
    while (free_)
    {
        auto const next = free_->next;
        free_->~block();
        std::free(free_);
        free_ = next;
    }
}

template <class _>
void*
qalloc_impl<_>::allocate(
    std::size_t bytes, std::size_t align)
{
    if (used_)
    {
        auto const p =
            used_->allocate(bytes, align);
        if (p)
            return p;
        used_ = nullptr;
    }
    if (free_)
    {
        auto const p =
            free_->allocate(bytes, align);
        if (p)
        {
            used_ = free_;
            free_ = free_->next;
            return p;
        }
    }
    std::size_t const adj_align =
        std::max(align, std::alignment_of<block*>::value);
std::size_t const min_alloc =  //上对齐
        ((sizeof (block) + sizeof (block*) + bytes) + (adj_align - 1)) &
        ~(adj_align - 1);
    auto const n = std::max<std::size_t>(block_size, min_alloc);
    block* const b =
        new(std::malloc(n)) block(n);
    if (! b)
        Throw<std::bad_alloc> ();
    used_ = b;
//这必须成功
    return used_->allocate(bytes, align);
}

template <class _>
void
qalloc_impl<_>::deallocate (void* p)
{
    auto const b =
        reinterpret_cast<block**>(p)[-1];
    if (b->deallocate())
    {
        if (used_ == b)
            used_ = nullptr;
        b->next = free_;
        free_ = b;
    }
}

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class T, bool ShareOnCopy>
qalloc_type<T, ShareOnCopy>::qalloc_type()
    : impl_ (std::make_shared<
        detail::qalloc_impl<>>())
{
}

template <class T, bool ShareOnCopy>
template <class U>
qalloc_type<T, ShareOnCopy>::qalloc_type(
        qalloc_type<U, ShareOnCopy> const& u)
    : impl_ (u.impl_)
{
}

template <class T, bool ShareOnCopy>
template <class U>
U*
qalloc_type<T, ShareOnCopy>::alloc (std::size_t n)
{
    if (n > std::numeric_limits<
            std::size_t>::max() / sizeof(U))
        Throw<std::bad_alloc> ();
    auto const bytes = n * sizeof(U);
    return static_cast<U*>(
        impl_->allocate(bytes,
            std::alignment_of<U>::value));
}

template <class T, bool ShareOnCopy>
template <class U>
inline
void
qalloc_type<T, ShareOnCopy>::dealloc(
    U* p, std::size_t n)
{
    impl_->deallocate(p);
}

template <class T, bool ShareOnCopy>
T*
qalloc_type<T, ShareOnCopy>::allocate (std::size_t n)
{
    return alloc<T>(n);
}

template <class T, bool ShareOnCopy>
inline
void
qalloc_type<T, ShareOnCopy>::deallocate(
    T* p, std::size_t n)
{
    dealloc(p, n);
}

template <class T, bool ShareOnCopy>
template <class U>
inline
bool
qalloc_type<T, ShareOnCopy>::operator==(
    qalloc_type<U, ShareOnCopy> const& u)
{
    return impl_.get() == u.impl_.get();
}

template <class T, bool ShareOnCopy>
template <class U>
inline
bool
qalloc_type<T, ShareOnCopy>::operator!=(
    qalloc_type<U, ShareOnCopy> const& u)
{
    return ! (*this == u);
}

template <class T, bool ShareOnCopy>
auto
qalloc_type<T, ShareOnCopy>::select_on_container_copy_construction() const ->
    qalloc_type
{
    return select_on_copy(
        std::integral_constant<bool, ShareOnCopy>{});
}

template <class T, bool ShareOnCopy>
auto
qalloc_type<T, ShareOnCopy>::select_on_copy(std::true_type) const ->
    qalloc_type
{
return *this; //共享场所
}

template <class T, bool ShareOnCopy>
auto
qalloc_type<T, ShareOnCopy>::select_on_copy(std::false_type) const ->
    qalloc_type
{
return {}; //新竞技场
}

} //涟漪

#endif
