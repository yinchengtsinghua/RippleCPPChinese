
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

#ifndef RIPPLE_LEDGER_READVIEWFWDRANGE_H_INCLUDED
#define RIPPLE_LEDGER_READVIEWFWDRANGE_H_INCLUDED

#include <cstddef>
#include <iterator>
#include <memory>
#include <boost/optional.hpp>

namespace ripple {

class ReadView;

namespace detail {

//类型已擦除的ForwardIterator
//
template <class ValueType>
class ReadViewFwdIter
{
public:
    using base_type = ReadViewFwdIter;

    using value_type = ValueType;

    ReadViewFwdIter() = default;
    ReadViewFwdIter(ReadViewFwdIter const&) = default;
    ReadViewFwdIter& operator=(ReadViewFwdIter const&) = default;

    virtual
    ~ReadViewFwdIter() = default;

    virtual
    std::unique_ptr<ReadViewFwdIter>
    copy() const = 0;

    virtual
    bool
    equal (ReadViewFwdIter const& impl) const = 0;

    virtual
    void
    increment() = 0;

    virtual
    value_type
    dereference() const = 0;
};

//使用类型擦除的ForwardIterator的范围
//
template<class ValueType>
class ReadViewFwdRange
{
public:
    using iter_base =
        ReadViewFwdIter<ValueType>;

   static_assert(
        std::is_nothrow_move_constructible<ValueType>{},
        "ReadViewFwdRange move and move assign constructors should be "
        "noexcept");

    class iterator
    {
    public:
        using value_type = ValueType;

        using pointer = value_type const*;

        using reference = value_type const&;

        using difference_type =
            std::ptrdiff_t;

        using iterator_category =
            std::forward_iterator_tag;

        iterator() = default;

        iterator (iterator const& other);
        iterator (iterator&& other) noexcept;

//由实现使用
        explicit
        iterator (ReadView const* view,
            std::unique_ptr<iter_base> impl);

        iterator&
        operator= (iterator const& other);

        iterator&
        operator= (iterator&& other) noexcept;

        bool
        operator== (iterator const& other) const;

        bool
        operator!= (iterator const& other) const;

//可以扔
        reference
        operator*() const;

//可以扔
        pointer
        operator->() const;

        iterator&
        operator++();

        iterator
        operator++(int);

    private:
        ReadView const* view_ = nullptr;
        std::unique_ptr<iter_base> impl_;
        boost::optional<value_type> mutable cache_;
    };

    static_assert(std::is_nothrow_move_constructible<iterator>{}, "");
    static_assert(std::is_nothrow_move_assignable<iterator>{}, "");
 
    using const_iterator = iterator;

    using value_type = ValueType;

    ReadViewFwdRange() = delete;
    ReadViewFwdRange (ReadViewFwdRange const&) = default;
    ReadViewFwdRange& operator= (ReadViewFwdRange const&) = default;

//否则vfalc会导致clang错误
//私人：
//朋友类阅读视图；

    explicit
    ReadViewFwdRange (ReadView const& view)
        : view_ (&view)
    {
    }

protected:
    ReadView const* view_;
    boost::optional<iterator> mutable end_;
};

} //细节
} //涟漪

#endif
