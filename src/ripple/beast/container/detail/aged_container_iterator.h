
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
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_CONTAINER_DETAIL_AGED_CONTAINER_ITERATOR_H_INCLUDED
#define BEAST_CONTAINER_DETAIL_AGED_CONTAINER_ITERATOR_H_INCLUDED

#include <iterator>
#include <type_traits>

namespace beast {

template <bool, bool, class, class, class, class, class>
class aged_ordered_container;

namespace detail {

//基本模板参数防止重复的想法
//基类声明来自于NexBez上的
//
//如果迭代器是可怕的，那么这个迭代器也是。
template <
    bool is_const,
    class Iterator,
    class Base =
        std::iterator <
            typename std::iterator_traits <Iterator>::iterator_category,
            typename std::conditional <is_const,
                typename Iterator::value_type::stashed::value_type const,
                typename Iterator::value_type::stashed::value_type>::type,
            typename std::iterator_traits <Iterator>::difference_type>
>
class aged_container_iterator
    : public Base
{
public:
    using time_point = typename Iterator::value_type::stashed::time_point;

    aged_container_iterator() = default;

//禁止从非常量迭代器构造常量迭代器。
//反向和非反向迭代器之间的转换应该是显式的。
    template <bool other_is_const, class OtherIterator, class OtherBase,
        class = typename std::enable_if <
            (other_is_const == false || is_const == true) &&
                std::is_same<Iterator, OtherIterator>::value == false>::type>
    explicit aged_container_iterator (aged_container_iterator <
        other_is_const, OtherIterator, OtherBase> const& other)
        : m_iter (other.m_iter)
    {
    }

//禁止从非常量迭代器构造常量迭代器。
    template <bool other_is_const, class OtherBase,
        class = typename std::enable_if <
            other_is_const == false || is_const == true>::type>
    aged_container_iterator (aged_container_iterator <
        other_is_const, Iterator, OtherBase> const& other)
        : m_iter (other.m_iter)
    {
    }

//禁用将常量迭代器分配给非常量迭代器
    template <bool other_is_const, class OtherIterator, class OtherBase>
    auto
    operator= (aged_container_iterator <
        other_is_const, OtherIterator, OtherBase> const& other) ->
            typename std::enable_if <
                other_is_const == false || is_const == true,
                    aged_container_iterator&>::type
    {
        m_iter = other.m_iter;
        return *this;
    }

    template <bool other_is_const, class OtherIterator, class OtherBase>
    bool operator== (aged_container_iterator <
        other_is_const, OtherIterator, OtherBase> const& other) const
    {
        return m_iter == other.m_iter;
    }

    template <bool other_is_const, class OtherIterator, class OtherBase>
    bool operator!= (aged_container_iterator <
        other_is_const, OtherIterator, OtherBase> const& other) const
    {
        return m_iter != other.m_iter;
    }

    aged_container_iterator& operator++ ()
    {
        ++m_iter;
        return *this;
    }

    aged_container_iterator operator++ (int)
    {
        aged_container_iterator const prev (*this);
        ++m_iter;
        return prev;
    }

    aged_container_iterator& operator-- ()
    {
        --m_iter;
        return *this;
    }

    aged_container_iterator operator-- (int)
    {
        aged_container_iterator const prev (*this);
        --m_iter;
        return prev;
    }

    typename Base::reference operator* () const
    {
        return m_iter->value;
    }

    typename Base::pointer operator-> () const
    {
        return &m_iter->value;
    }

    time_point const& when () const
    {
        return m_iter->when;
    }

private:
    template <bool, bool, class, class, class, class, class>
    friend class aged_ordered_container;

    template <bool, bool, class, class, class, class, class, class>
    friend class aged_unordered_container;

    template <bool, class, class>
    friend class aged_container_iterator;

    template <class OtherIterator>
    aged_container_iterator (OtherIterator const& iter)
        : m_iter (iter)
    {
    }

    Iterator const& iterator() const
    {
        return m_iter;
    }

    Iterator m_iter;
};

}

}

#endif
