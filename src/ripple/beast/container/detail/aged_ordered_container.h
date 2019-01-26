
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

#ifndef BEAST_CONTAINER_DETAIL_AGED_ORDERED_CONTAINER_H_INCLUDED
#define BEAST_CONTAINER_DETAIL_AGED_ORDERED_CONTAINER_H_INCLUDED

#include <ripple/beast/container/detail/aged_container_iterator.h>
#include <ripple/beast/container/detail/aged_associative_container.h>
#include <ripple/beast/container/aged_container.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <boost/beast/core/detail/empty_base_optimization.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/version.hpp>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <ripple/beast/cxx17/type_traits.h> //<性状> >
#include <utility>

namespace beast {
namespace detail {

//用于识别反向_迭代器的特征模板，这是不允许的
//用于转换操作。
template <class It>
struct is_boost_reverse_iterator
    : std::false_type
{
    explicit is_boost_reverse_iterator() = default;
};

template <class It>
struct is_boost_reverse_iterator<boost::intrusive::reverse_iterator<It>>
    : std::true_type
{
    explicit is_boost_reverse_iterator() = default;
};

/*关联容器，其中每个元素也按时间索引。

    此容器镜像订购的标准库的接口
    关联容器，添加每个元素
    用从时钟值中获得的“when”和“time”点
    “现在”。函数“touch”将元素的时间更新为当前
    时钟报告的时间。

    中提供了一组额外的迭代器类型和成员函数。
    允许在时间或反转中遍历的'chronological'成员空间
    时间顺序。此容器可用作缓存的构建基块
    其项目在一定时间后到期。按时间顺序
    迭代器允许完全可定制的过期策略。

    @见老年人地图，老年人地图，老年人地图
**/

template <
    bool IsMulti,
    bool IsMap,
    class Key,
    class T,
    class Clock = std::chrono::steady_clock,
    class Compare = std::less <Key>,
    class Allocator = std::allocator <
        typename std::conditional <IsMap,
            std::pair <Key const, T>,
                Key>::type>
>
class aged_ordered_container
{
public:
    using clock_type = abstract_clock<Clock>;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename std::conditional <
        IsMap, std::pair <Key const, T>, Key>::type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

//自省（用于单元测试）
    using is_unordered = std::false_type;
    using is_multi = std::integral_constant <bool, IsMulti>;
    using is_map = std::integral_constant <bool, IsMap>;

private:
    static Key const& extract (value_type const& value)
    {
        return aged_associative_container_extract_t <IsMap> () (value);
    }

//vfalc todo提升以删除模板参数依赖项
    struct element
        : boost::intrusive::set_base_hook <
            boost::intrusive::link_mode <
                boost::intrusive::normal_link>
            >
        , boost::intrusive::list_base_hook <
            boost::intrusive::link_mode <
                boost::intrusive::normal_link>
            >
    {
//在这里存储类型，这样迭代器就不会
//需要查看容器声明。
        struct stashed
        {
            explicit stashed() = default;

            using value_type = typename aged_ordered_container::value_type;
            using time_point = typename aged_ordered_container::time_point;
        };

        element (
            time_point const& when_,
            value_type const& value_)
            : value (value_)
            , when (when_)
        {
        }

        element (
            time_point const& when_,
            value_type&& value_)
            : value (std::move (value_))
            , when (when_)
        {
        }

        template <
            class... Args,
            class = typename std::enable_if <
                std::is_constructible <value_type,
                    Args...>::value>::type
        >
        element (time_point const& when_, Args&&... args)
            : value (std::forward <Args> (args)...)
            , when (when_)
        {
        }

        value_type value;
        time_point when;
    };

//vfalc todo只能对地图启用此功能。
    class pair_value_compare
        : public boost::beast::detail::empty_base_optimization <Compare>
#ifdef _LIBCPP_VERSION
        , public std::binary_function <value_type, value_type, bool>
#endif
    {
    public:
#ifndef _LIBCPP_VERSION
        using first_argument = value_type;
        using second_argument = value_type;
        using result_type = bool;
#endif

        bool operator() (value_type const& lhs, value_type const& rhs) const
        {
            return this->member() (lhs.first, rhs.first);
        }

        pair_value_compare ()
        {
        }

        pair_value_compare (pair_value_compare const& other)
            : boost::beast::detail::empty_base_optimization <Compare> (other)
        {
        }

    private:
        friend aged_ordered_container;

        pair_value_compare (Compare const& compare)
            : boost::beast::detail::empty_base_optimization <Compare> (compare)
        {
        }
    };

//比较值类型与元素，用于插入检查
//vfalc todo提升以删除模板参数依赖项
    class KeyValueCompare
        : public boost::beast::detail::empty_base_optimization <Compare>
#ifdef _LIBCPP_VERSION
        , public std::binary_function <Key, element, bool>
#endif
    {
    public:
#ifndef _LIBCPP_VERSION
        using first_argument = Key;
        using second_argument = element;
        using result_type = bool;
#endif

        KeyValueCompare () = default;

        KeyValueCompare (Compare const& compare)
            : boost::beast::detail::empty_base_optimization <Compare> (compare)
        {
        }

//vfalc注意，我们可能只想启用这些重载
//如果比较具有透明性
#if 0
        template <class K>
        bool operator() (K const& k, element const& e) const
        {
            return this->member() (k, extract (e.value));
        }

        template <class K>
        bool operator() (element const& e, K const& k) const
        {
            return this->member() (extract (e.value), k);
        }
#endif

        bool operator() (Key const& k, element const& e) const
        {
            return this->member() (k, extract (e.value));
        }

        bool operator() (element const& e, Key const& k) const
        {
            return this->member() (extract (e.value), k);
        }

        bool operator() (element const& x, element const& y) const
        {
            return this->member() (extract (x.value), extract (y.value));
        }

        Compare& compare()
        {
            return boost::beast::detail::empty_base_optimization <Compare>::member();
        }

        Compare const& compare() const
        {
            return boost::beast::detail::empty_base_optimization <Compare>::member();
        }
    };

    using list_type = typename boost::intrusive::make_list <element,
        boost::intrusive::constant_time_size <false>>::type;

    using cont_type = typename std::conditional <
        IsMulti,
        typename boost::intrusive::make_multiset <element,
            boost::intrusive::constant_time_size <true>,
            boost::intrusive::compare<KeyValueCompare>
                >::type,
        typename boost::intrusive::make_set <element,
            boost::intrusive::constant_time_size <true>,
            boost::intrusive::compare<KeyValueCompare>
                >::type
        >::type;

    using ElementAllocator = typename std::allocator_traits <
        Allocator>::template rebind_alloc <element>;

    using ElementAllocatorTraits = std::allocator_traits <ElementAllocator>;

    class config_t
        : private KeyValueCompare
        , public boost::beast::detail::empty_base_optimization <ElementAllocator>
    {
    public:
        explicit config_t (
            clock_type& clock_)
            : clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Compare const& comp)
            : KeyValueCompare (comp)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Allocator const& alloc_)
            : boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc_)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Compare const& comp,
            Allocator const& alloc_)
            : KeyValueCompare (comp)
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc_)
            , clock (clock_)
        {
        }

        config_t (config_t const& other)
            : KeyValueCompare (other.key_compare())
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (
                ElementAllocatorTraits::
                    select_on_container_copy_construction (
                        other.alloc()))
            , clock (other.clock)
        {
        }

        config_t (config_t const& other, Allocator const& alloc)
            : KeyValueCompare (other.key_compare())
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc)
            , clock (other.clock)
        {
        }

        config_t (config_t&& other)
            : KeyValueCompare (std::move (other.key_compare()))
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (
                std::move (other))
            , clock (other.clock)
        {
        }

        config_t (config_t&& other, Allocator const& alloc)
            : KeyValueCompare (std::move (other.key_compare()))
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc)
            , clock (other.clock)
        {
        }

        config_t& operator= (config_t const& other)
        {
            if (this != &other)
            {
                compare() = other.compare();
                alloc() = other.alloc();
                clock = other.clock;
            }
            return *this;
        }

        config_t& operator= (config_t&& other)
        {
            compare() = std::move (other.compare());
            alloc() = std::move (other.alloc());
            clock = other.clock;
            return *this;
        }

        Compare& compare ()
        {
            return KeyValueCompare::compare();
        }

        Compare const& compare () const
        {
            return KeyValueCompare::compare();
        }

        KeyValueCompare& key_compare()
        {
            return *this;
        }

        KeyValueCompare const& key_compare() const
        {
            return *this;
        }

        ElementAllocator& alloc()
        {
            return boost::beast::detail::empty_base_optimization <
                ElementAllocator>::member();
        }

        ElementAllocator const& alloc() const
        {
            return boost::beast::detail::empty_base_optimization <
                ElementAllocator>::member();
        }

        std::reference_wrapper <clock_type> clock;
    };

    template <class... Args>
    element* new_element (Args&&... args)
    {
        struct Deleter
        {
            std::reference_wrapper <ElementAllocator> a_;
            Deleter (ElementAllocator& a)
                : a_(a)
            {
            }

            void
            operator()(element* p)
            {
                ElementAllocatorTraits::deallocate (a_.get(), p, 1);
            }
        };

        std::unique_ptr <element, Deleter> p (ElementAllocatorTraits::allocate (
            m_config.alloc(), 1), Deleter(m_config.alloc()));
        ElementAllocatorTraits::construct (m_config.alloc(),
            p.get(), clock().now(), std::forward <Args> (args)...);
        return p.release();
    }

    void delete_element (element const* p)
    {
        ElementAllocatorTraits::destroy (m_config.alloc(), p);
        ElementAllocatorTraits::deallocate (
            m_config.alloc(), const_cast<element*>(p), 1);
    }

    void unlink_and_delete_element (element const* p)
    {
        chronological.list.erase (
            chronological.list.iterator_to (*p));
        m_cont.erase (m_cont.iterator_to (*p));
        delete_element (p);
    }

public:
    using key_compare = Compare;
    using value_compare = typename std::conditional <
        IsMap,
        pair_value_compare,
        Compare>::type;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = typename std::allocator_traits <
        Allocator>::pointer;
    using const_pointer = typename std::allocator_traits <
        Allocator>::const_pointer;

//集合迭代器（ismap==false）始终是常量
//因为集合的元素是不可变的。
    using iterator = beast::detail::aged_container_iterator<
        ! IsMap, typename cont_type::iterator>;
    using const_iterator = beast::detail::aged_container_iterator<
        true, typename cont_type::iterator>;
    using reverse_iterator = beast::detail::aged_container_iterator<
        ! IsMap, typename cont_type::reverse_iterator>;
    using const_reverse_iterator = beast::detail::aged_container_iterator<
        true, typename cont_type::reverse_iterator>;

//——————————————————————————————————————————————————————————————
//
//按时间顺序的迭代器
//
//“成员空间”
//http://accu.org/index.php/journals/1527页
//
//——————————————————————————————————————————————————————————————

    class chronological_t
    {
    public:
//集合迭代器（ismap==false）始终是常量
//因为集合的元素是不可变的。
        using iterator = beast::detail::aged_container_iterator<
            ! IsMap, typename list_type::iterator>;
        using const_iterator = beast::detail::aged_container_iterator<
            true, typename list_type::iterator>;
        using reverse_iterator = beast::detail::aged_container_iterator<
            ! IsMap, typename list_type::reverse_iterator>;
        using const_reverse_iterator = beast::detail::aged_container_iterator<
            true, typename list_type::reverse_iterator>;

        iterator begin ()
         {
            return iterator (list.begin());
        }

        const_iterator begin () const
        {
            return const_iterator (list.begin ());
        }

        const_iterator cbegin() const
        {
            return const_iterator (list.begin ());
        }

        iterator end ()
        {
            return iterator (list.end ());
        }

        const_iterator end () const
        {
            return const_iterator (list.end ());
        }

        const_iterator cend () const
        {
            return const_iterator (list.end ());
        }

        reverse_iterator rbegin ()
        {
            return reverse_iterator (list.rbegin());
        }

        const_reverse_iterator rbegin () const
        {
            return const_reverse_iterator (list.rbegin ());
        }

        const_reverse_iterator crbegin() const
        {
            return const_reverse_iterator (list.rbegin ());
        }

        reverse_iterator rend ()
        {
            return reverse_iterator (list.rend ());
        }

        const_reverse_iterator rend () const
        {
            return const_reverse_iterator (list.rend ());
        }

        const_reverse_iterator crend () const
        {
            return const_reverse_iterator (list.rend ());
        }

        iterator iterator_to (value_type& value)
        {
            static_assert (std::is_standard_layout <element>::value,
                "must be standard layout");
            return list.iterator_to (*reinterpret_cast <element*>(
                 reinterpret_cast<uint8_t*>(&value)-((std::size_t)
                    std::addressof(((element*)0)->member))));
        }

        const_iterator iterator_to (value_type const& value) const
        {
            static_assert (std::is_standard_layout <element>::value,
                "must be standard layout");
            return list.iterator_to (*reinterpret_cast <element const*>(
                 reinterpret_cast<uint8_t const*>(&value)-((std::size_t)
                    std::addressof(((element*)0)->member))));
        }

    private:
        chronological_t ()
        {
        }

        chronological_t (chronological_t const&) = delete;
        chronological_t (chronological_t&&) = delete;

        friend class aged_ordered_container;
        list_type mutable list;
    } chronological;

//——————————————————————————————————————————————————————————————
//
//施工
//
//——————————————————————————————————————————————————————————————

    aged_ordered_container() = delete;

    explicit aged_ordered_container (clock_type& clock);

    aged_ordered_container (clock_type& clock,
        Compare const& comp);

    aged_ordered_container (clock_type& clock,
        Allocator const& alloc);

    aged_ordered_container (clock_type& clock,
        Compare const& comp, Allocator const& alloc);

    template <class InputIt>
    aged_ordered_container (InputIt first, InputIt last, clock_type& clock);

    template <class InputIt>
    aged_ordered_container (InputIt first, InputIt last, clock_type& clock,
        Compare const& comp);

    template <class InputIt>
    aged_ordered_container (InputIt first, InputIt last, clock_type& clock,
        Allocator const& alloc);

    template <class InputIt>
    aged_ordered_container (InputIt first, InputIt last, clock_type& clock,
        Compare const& comp, Allocator const& alloc);

    aged_ordered_container (aged_ordered_container const& other);

    aged_ordered_container (aged_ordered_container const& other,
        Allocator const& alloc);

    aged_ordered_container (aged_ordered_container&& other);

    aged_ordered_container (aged_ordered_container&& other,
        Allocator const& alloc);

    aged_ordered_container (std::initializer_list <value_type> init,
        clock_type& clock);

    aged_ordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Compare const& comp);

    aged_ordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Allocator const& alloc);

    aged_ordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Compare const& comp, Allocator const& alloc);

    ~aged_ordered_container();

    aged_ordered_container&
    operator= (aged_ordered_container const& other);

    aged_ordered_container&
    operator= (aged_ordered_container&& other);

    aged_ordered_container&
    operator= (std::initializer_list <value_type> init);

    allocator_type
    get_allocator() const
    {
        return m_config.alloc();
    }

    clock_type&
    clock()
    {
        return m_config.clock;
    }

    clock_type const&
    clock() const
    {
        return m_config.clock;
    }

//——————————————————————————————————————————————————————————————
//
//元素访问（映射）
//
//——————————————————————————————————————————————————————————————

    template <
        class K,
        bool maybe_multi = IsMulti,
        bool maybe_map = IsMap,
        class = typename std::enable_if <maybe_map && ! maybe_multi>::type>
    typename std::conditional <IsMap, T, void*>::type&
    at (K const& k);

    template <
        class K,
        bool maybe_multi = IsMulti,
        bool maybe_map = IsMap,
        class = typename std::enable_if <maybe_map && ! maybe_multi>::type>
    typename std::conditional <IsMap, T, void*>::type const&
    at (K const& k) const;

    template <
        bool maybe_multi = IsMulti,
        bool maybe_map = IsMap,
        class = typename std::enable_if <maybe_map && ! maybe_multi>::type>
    typename std::conditional <IsMap, T, void*>::type&
    operator[] (Key const& key);

    template <
        bool maybe_multi = IsMulti,
        bool maybe_map = IsMap,
        class = typename std::enable_if <maybe_map && ! maybe_multi>::type>
    typename std::conditional <IsMap, T, void*>::type&
    operator[] (Key&& key);

//——————————————————————————————————————————————————————————————
//
//遍历器
//
//——————————————————————————————————————————————————————————————

    iterator
    begin ()
    {
        return iterator (m_cont.begin());
    }

    const_iterator
    begin () const
    {
        return const_iterator (m_cont.begin ());
    }

    const_iterator
    cbegin() const
    {
        return const_iterator (m_cont.begin ());
    }

    iterator
    end ()
    {
        return iterator (m_cont.end ());
    }

    const_iterator
    end () const
    {
        return const_iterator (m_cont.end ());
    }

    const_iterator
    cend () const
    {
        return const_iterator (m_cont.end ());
    }

    reverse_iterator
    rbegin ()
    {
        return reverse_iterator (m_cont.rbegin());
    }

    const_reverse_iterator
    rbegin () const
    {
        return const_reverse_iterator (m_cont.rbegin ());
    }

    const_reverse_iterator
    crbegin() const
    {
        return const_reverse_iterator (m_cont.rbegin ());
    }

    reverse_iterator
    rend ()
    {
        return reverse_iterator (m_cont.rend ());
    }

    const_reverse_iterator
    rend () const
    {
        return const_reverse_iterator (m_cont.rend ());
    }

    const_reverse_iterator
    crend () const
    {
        return const_reverse_iterator (m_cont.rend ());
    }

    iterator
    iterator_to (value_type& value)
    {
        static_assert (std::is_standard_layout <element>::value,
            "must be standard layout");
        return m_cont.iterator_to (*reinterpret_cast <element*>(
             reinterpret_cast<uint8_t*>(&value)-((std::size_t)
                std::addressof(((element*)0)->member))));
    }

    const_iterator
    iterator_to (value_type const& value) const
    {
        static_assert (std::is_standard_layout <element>::value,
            "must be standard layout");
        return m_cont.iterator_to (*reinterpret_cast <element const*>(
             reinterpret_cast<uint8_t const*>(&value)-((std::size_t)
                std::addressof(((element*)0)->member))));
    }

//——————————————————————————————————————————————————————————————
//
//容量
//
//——————————————————————————————————————————————————————————————

    bool
    empty() const noexcept
    {
        return m_cont.empty();
    }

    size_type
    size() const noexcept
    {
        return m_cont.size();
    }

    size_type
    max_size() const noexcept
    {
        return m_config.max_size();
    }

//——————————————————————————————————————————————————————————————
//
//修饰语
//
//——————————————————————————————————————————————————————————————

    void
    clear();

//集合映射
    template <bool maybe_multi = IsMulti>
    auto
    insert (value_type const& value) ->
        typename std::enable_if <! maybe_multi,
            std::pair <iterator, bool>>::type;

//多重映射，多重集
    template <bool maybe_multi = IsMulti>
    auto
    insert (value_type const& value) ->
        typename std::enable_if <maybe_multi,
            iterator>::type;

//设置
    template <bool maybe_multi = IsMulti, bool maybe_map = IsMap>
    auto
    insert (value_type&& value) ->
        typename std::enable_if <! maybe_multi && ! maybe_map,
            std::pair <iterator, bool>>::type;

//多集
    template <bool maybe_multi = IsMulti, bool maybe_map = IsMap>
    auto
    insert (value_type&& value) ->
        typename std::enable_if <maybe_multi && ! maybe_map,
            iterator>::type;

//---

//集合映射
    template <bool maybe_multi = IsMulti>
    auto
    insert (const_iterator hint, value_type const& value) ->
        typename std::enable_if <! maybe_multi,
            iterator>::type;

//多重映射，多重集
    template <bool maybe_multi = IsMulti>
    typename std::enable_if <maybe_multi,
        iterator>::type
    /*ert（const_迭代器/*提示*/，value_类型const&value）
    {
        //vfalc todo了解如何使用“hint”
        返回插入（值）；
    }

    //映射
    template<bool maybe_multi=ismulti>
    汽车
    insert（const_iterator提示，value_type&&value）->
        typename std：：启用_if<！可能是多种多样的，
            迭代器>：：类型；

    //多重映射，多重集
    template<bool maybe_multi=ismulti>
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
    插入（const_iterator/*hin*/, value_type&& value)

    {
//vfalc todo了解如何使用“提示”
        return insert (std::move (value));
    }

//多重地图
    template <
        class P,
        bool maybe_map = IsMap
    >
    typename std::enable_if <maybe_map &&
        std::is_constructible <value_type, P&&>::value,
            typename std::conditional <IsMulti,
                iterator,
                std::pair <iterator, bool>
            >::type
    >::type
    insert (P&& value)
    {
        return emplace (std::forward <P> (value));
    }

//多重地图
    template <
        class P,
        bool maybe_map = IsMap
    >
    typename std::enable_if <maybe_map &&
        std::is_constructible <value_type, P&&>::value,
            typename std::conditional <IsMulti,
                iterator,
                std::pair <iterator, bool>
            >::type
    >::type
    insert (const_iterator hint, P&& value)
    {
        return emplace_hint (hint, std::forward <P> (value));
    }

    template <class InputIt>
    void
    insert (InputIt first, InputIt last)
    {
        for (; first != last; ++first)
            insert (cend(), *first);
    }

    void
    insert (std::initializer_list <value_type> init)
    {
        insert (init.begin(), init.end());
    }

//集合映射
    template <bool maybe_multi = IsMulti, class... Args>
    auto
    emplace (Args&&... args) ->
        typename std::enable_if <! maybe_multi,
            std::pair <iterator, bool>>::type;

//多重集，多重映射
    template <bool maybe_multi = IsMulti, class... Args>
    auto
    emplace (Args&&... args) ->
        typename std::enable_if <maybe_multi,
            iterator>::type;

//集合映射
    template <bool maybe_multi = IsMulti, class... Args>
    auto
    emplace_hint (const_iterator hint, Args&&... args) ->
        typename std::enable_if <! maybe_multi,
            std::pair <iterator, bool>>::type;

//多重集，多重映射
    template <bool maybe_multi = IsMulti, class... Args>
    typename std::enable_if <maybe_multi,
        iterator>::type
    /*lace_提示（const_迭代器/*提示*/，参数&……ARGS）
    {
        //vfalc todo了解如何使用“hint”
        返回模板
            std:：forward<args>（args…）；
    }

    //如果阻止Erase（Reverse_Iterator pos）编译，则启用_
    template<bool is_const，class iterator，class base，
         class=std：：启用_如果_t<！是“boost”反转“iterator”<iterator>：：value>>
    beast:：detail:：aged_container_iterator<false，iterator，base>
    擦除（beast:：detail:：aged_container_iterator<is_const，iterator，base>pos）；

    //如果阻止擦除，则启用_（reverse_iterator first，reverse_iterator last）
    //来自编译
    template<bool is_const，class iterator，class base，
        class=std：：启用_如果_t<！是“boost”反转“iterator”<iterator>：：value>>
    beast:：detail:：aged_container_iterator<false，iterator，base>
    擦除（beast:：detail:：aged_container_iterator<is_const，iterator，base>first，
           beast:：detail:：aged_container_iterator<is_const，iterator，base>last）；

    模板<class k>
    汽车
    删除（k const&k）->
        SIZE型；

    无效
    交换（老化的订购容器和其他）无例外；

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    //如果阻止touch（reverse_iterator pos）编译，则启用_
    template<bool is_const，class iterator，class base，
        class=std：：启用_如果_t<！是“boost”反转“iterator”<iterator>：：value>>
    无效
    触摸（beast:：detail:：aged_container_iterator<is_const，iterator，base>pos）
    {
        触摸（pos，clock（）.now（））；
    }

    模板<class k>
    SiZe型
    触摸（k const&k）；

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————
    / /
    /查找
    / /
    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    SiZe型
    计数（k const&k）const
    {
        返回连续计数m（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    迭代器
    查找（k const&k）
    {
        返回迭代器（m_cont.find（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    常数迭代器
    查找（k const&k）const
    {
        返回常量迭代器（m_cont.find（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    std:：pair<iterator，iterator>
    等范围（k常量和k）
    {
        自动常数r（m_cont.equal_range（k，
            std:：cref（m_config.key_compare（））；
        返回std:：make_pair（迭代器（r.first），
            迭代器（r.second））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    std:：pair<const_iterator，const_iterator>
    等量范围（k常量和k常量）
    {
        自动常数r（m_cont.equal_range（k，
            std:：cref（m_config.key_compare（））；
        返回std:：make_pair（const_迭代器（r.first），
            常量迭代器（r.second））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    迭代器
    下限（k const&k）
    {
        返回迭代器（m_cont.lower_bound（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    常数迭代器
    下限（k const&k）const
    {
        返回const_迭代器（m_cont.lower_bound（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    迭代器
    上限（k const&k）
    {
        返回迭代器（m_cont.upper_bound（k，
            std:：cref（m_config.key_compare（））；
    }

    /VFARCO toto尊重IS-透明（C++ 14）
    模板<class k>
    常数迭代器
    上限（k const&k）const
    {
        返回const_迭代器（m_cont.upper_bound（k，
            std:：cref（m_config.key_compare（））；
    }

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————
    / /
    /观察员
    / /
    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    密钥比较
    键“comp（）const”
    {
        返回m_config.compare（）；
    }

    //vfalc todo此操作是否应返回set的const reference？
    价值比较
    value_comp（）常量
    {
        返回值_compare（m_config.compare（））；
    }

    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————
    / /
    比较
    / /
    //————————————————————————————————————————————————————————————————————————————————————————————————————————————————

    //这与标准的不同之处在于
    //仅对值类型的键部分执行，忽略
    //映射的类型。
    / /
    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    运算符==
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                otherallocator>const&other）const；

    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    接线员！=（
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                其他分配器>常量&其他）常量
    {
        回来！（此->运算符==（其他））；
    }

    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    操作员<
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                其他分配器>常量&其他）常量
    {
        value_compare const comp（value_comp（））；
        返回std：：词典编纂_compare（
            cbegin（）、cend（）、other.cbegin（）、other.cend（）、comp）；
    }

    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    运算符<
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                其他分配器>常量&其他）常量
    {
        回来！（其他<*此项）；
    }

    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    运算符>
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                其他分配器>常量&其他）常量
    {
        返回其他<*this；
    }

    模板<
        bool otherismulti公司，
        布尔热疗法，
        班级其他人，
        课程其他持续时间，
        类OtherAllocator
    >
    布尔
    运算符>
        陈年订箱<otherismulti，otherismap，
            key，othert，otherduration，比较，
                其他分配器>常量&其他）常量
    {
        回来！（*此项<其他项）；
    }

私人：
    //如果阻止Erase（Reverse_Iterator pos，now）编译，则启用_
    template<bool is_const，class iterator，class base，
        class=std：：启用_如果_t<！是“boost”反转“iterator”<iterator>：：value>>
    无效
    触摸（Beast:：Detail:：Aged_Container_Iterator<
        是常量，迭代器，base>pos，
            typename clock_type:：time_point const&now）；

    template<bool maybe_propagate=std:：allocator_traits<
        分配器>：：在容器上传播_swap:：value>
    typename std:：enable_if<maybe_propagate>：类型
    交换数据（老化的订购容器和其他）无例外；

    template<bool maybe_propagate=std:：allocator_traits<
        分配器>：：在容器上传播_swap:：value>
    typename std：：启用_if<！可能传播>：：类型
    交换数据（老化的订购容器和其他）无例外；

私人：
    配置
    连续型可变连续型；
}；

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（
    时钟类型和时钟）
    ：m_配置（时钟）
{
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（
    时钟类型和时钟，
    比较const&comp）
    ：m_配置（时钟、补偿）
    ，MyCONT（COMP）
{
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（
    时钟类型和时钟，
    分配器常数和分配）
    ：m_配置（时钟，分配）
{
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（
    时钟类型和时钟，
    比较const&comp，
    分配器常数和分配）
    ：m_配置（时钟、补偿、分配）
    ，MyCONT（COMP）
{
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<类输入>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老旧的集装箱（先装，后装，
    时钟类型和时钟）
    ：m_配置（时钟）
{
    插入（第一个、最后一个）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<类输入>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老旧的集装箱（先装，后装，
    时钟类型和时钟，
    比较const&comp）
    ：m_配置（时钟、补偿）
    ，MyCONT（COMP）
{
    插入（第一个、最后一个）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<类输入>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老旧的集装箱（先装，后装，
    时钟类型和时钟，
    分配器常数和分配）
    ：m_配置（时钟，分配）
{
    插入（第一个、最后一个）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<类输入>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老旧的集装箱（先装，后装，
    时钟类型和时钟，
    比较const&comp，
    分配器常数和分配）
    ：m_配置（时钟、补偿、分配）
    ，MyCONT（COMP）
{
    插入（第一个、最后一个）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（陈年订箱及其他）
    ：m_-config（其他.m_-config）
    ，M_cont（其他M_cont.comp（））
{
    插入（other.cbegin（），other.cend（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（陈年订箱及其他，
    分配器常数和分配）
    ：m_-config（其他.m_-config，alloc）
    ，M_cont（其他M_cont.comp（））
{
    插入（other.cbegin（），other.cend（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（陈年订箱及其他）
    ：m_-config（std:：move（other.m_-config））。
    ，M_cont（标准：：移动（其他.M_cont））
{
    chronological.list=std:：move（其他.chronological.list）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
陈年订箱（陈年订箱及其他，
    分配器常数和分配）
    ：m_-config（std：：move（other.m_-config），alloc）
    ，m_cont（std:：move（other.m_cont.comp（））
{
    插入（other.cbegin（），other.cend（））；
    明确的（）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老化的_有序_容器（std:：initializer_list<value_type>init，
    时钟类型和时钟）
    ：m_配置（时钟）
{
    insert（init.begin（），init.end（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老化的_有序_容器（std:：initializer_list<value_type>init，
    时钟类型和时钟，
    比较const&comp）
    ：m_配置（时钟、补偿）
    ，MyCONT（COMP）
{
    insert（init.begin（），init.end（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老化的_有序_容器（std:：initializer_list<value_type>init，
    时钟类型和时钟，
    分配器常数和分配）
    ：m_配置（时钟，分配）
{
    insert（init.begin（），init.end（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
老化的_有序_容器（std:：initializer_list<value_type>init，
    时钟类型和时钟，
    比较const&comp，
    分配器常数和分配）
    ：m_配置（时钟、补偿、分配）
    ，MyCONT（COMP）
{
    insert（init.begin（），init.end（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
~老旧的订购容器（）
{
    清除（）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
operator=（老化的订购容器构造和其他）->
    陈年订箱
{
    如果（这个）！=和其他）
    {
        清除（）；
        这个->m_config=other.m_config；
        插入（other.begin（），other.end（））；
    }
    返回这个；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
运算符=（老化的“订购的”容器和其他）->
    陈年订箱
{
    清除（）；
    此->m_config=std:：move（其他.m_config）；
    插入（other.begin（），other.end（））；
    其他。
    返回这个；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
operator=（std:：initializer_list<value_type>init）->
    陈年订箱
{
    清除（）；
    插入（init）；
    返回这个；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<class k，bool may_multi，bool may_map，class>
typename std:：conditional<ismap，t，void*>：：type&
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
AT（k const & k）
{
    自动const iter（m_cont.find（k，
        std:：cref（m_config.key_compare（））；
    if（iter==m_cont.end（））
        抛出std:：out_of_range（“key not found”）；
    返回iter->value.second；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<class k，bool may_multi，bool may_map，class>
typename std:：conditional<ismap，t，void*>：：type const&
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
AT（K常量和K）常量
{
    自动const iter（m_cont.find（k，
        std:：cref（m_config.key_compare（））；
    if（iter==m_cont.end（））
        抛出std:：out_of_range（“key not found”）；
    返回iter->value.second；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool maybe_multi，bool maybe_map，class>
typename std:：conditional<ismap，t，void*>：：type&
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
运算符[]（键常量和键）
{
    typename cont_type：：插入_commit_data d；
    自动构造结果（m_cont.insert_check（key，
        std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        元素*const p（new_element（
            std:：separewise_construct，std:：forward_as_tuple（key），
                std:：forward_as_tuple（））；
        m_cont.insert_commit（*p，d）；
        按时间顺序。list.push_back（*p）；
        返回p->value.second；
    }
    返回result.first->value.second；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool maybe_multi，bool maybe_map，class>
typename std:：conditional<ismap，t，void*>：：type&
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
运算符[]（键和键）
{
    typename cont_type：：插入_commit_data d；
    自动构造结果（m_cont.insert_check（key，
        std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        元素*const p（new_element（
            std：：分段构造，
                std:：forward_as_tuple（std:：move（key）），
                    std:：forward_as_tuple（））；
        m_cont.insert_commit（*p，d）；
        按时间顺序。list.push_back（*p）；
        返回p->value.second；
    }
    返回result.first->value.second；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
无效
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
清除（）
{
    for（auto iter（chronological.list.begin（））；
        伊特尔！=chronological.list.end（）；）
        删除_元素（&*iter++）；
    按时间顺序.list.clear（）；
    MyCONT.CULL（）；
}

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
插入（value_type const&value）->
    typename std：：启用_if<！可能是多种多样的，
        std:：pair<iterator，bool>>：类型
{
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（extract（value）），
        std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        element*const p（new_element（value））；
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回std:：make_pair（迭代器（iter），true）；
    }
    返回std：：make_pair（迭代器（result.first），false）；
}

//多重映射，多重集
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
插入（value_type const&value）->
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
{
    auto const before（m_cont.upper_bound（
        提取（值），std:：cref（m_config.key_compare（））；
    element*const p（new_element（value））；
    按时间顺序。list.push_back（*p）；
    auto const iter（m_cont.insert_before（before，*p））；
    返回迭代器（ITER）；
}

//集
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool maybe_multi，bool maybe_map>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
插入（值类型和值）->
    typename std：：启用_if<！可能是多&！梅贝地图
        std:：pair<iterator，bool>>：类型
{
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（extract（value）），
        std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        element*const p（new_element（std:：move（value））；
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回std:：make_pair（迭代器（iter），true）；
    }
    返回std：：make_pair（迭代器（result.first），false）；
}

/多集
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool maybe_multi，bool maybe_map>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
插入（值类型和值）->
    typename std：：启用_if<maybe_multi&！梅贝地图
        迭代器>：：类型
{
    auto const before（m_cont.upper_bound（
        提取（值），std:：cref（m_config.key_compare（））；
    element*const p（new_element（std:：move（value））；
    按时间顺序。list.push_back（*p）；
    auto const iter（m_cont.insert_before（before，*p））；
    返回迭代器（ITER）；
}

//

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
insert（const_迭代器提示，value_type const&value）->
    typename std：：启用_if<！可能是多种多样的，
        迭代器>：：类型
{
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（hint.iterator（），
        提取（值），std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        element*const p（new_element（value））；
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回迭代器（ITER）；
    }
    返回迭代器（result.first）；
}

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
insert（const_iterator提示，value_type&&value）->
    typename std：：启用_if<！可能是多种多样的，
        迭代器>：：类型
{
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（hint.iterator（），
        提取（值），std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        element*const p（new_element（std:：move（value））；
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回迭代器（ITER）；
    }
    返回迭代器（result.first）；
}

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi，class…ARG>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
安置（args和&…ARGS）>
    typename std：：启用_if<！可能是多种多样的，
        std:：pair<iterator，bool>>：类型
{
    //vfalc注意到我们需要
    //在此处构造元素
    元素*const p（new_element（
        std:：forward<args>（args）…）；
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（extract（p->value）），
        std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回std:：make_pair（迭代器（iter），true）；
    }
    删除元素（p）；
    返回std：：make_pair（迭代器（result.first），false）；
}

//多集，多映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi，class…ARG>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
安置（args和&…ARGS）>
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
{
    元素*const p（new_element（
        std:：forward<args>（args）…）；
    auto const before（m_cont.upper_bound（extract（p->value）），
        std:：cref（m_config.key_compare（））；
    按时间顺序。list.push_back（*p）；
    auto const iter（m_cont.insert_before（before，*p））；
    返回迭代器（ITER）；
}

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_multi，class…ARG>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
emplace_提示（const_迭代器提示，args&……ARGS）>
    typename std：：启用_if<！可能是多种多样的，
        std:：pair<iterator，bool>>：类型
{
    //vfalc注意到我们需要
    //在此处构造元素
    元素*const p（new_element（
        std:：forward<args>（args）…）；
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（hint.iterator（），
        提取（p->value），std:：cref（m_config.key_compare（）），d））；
    if（结果秒）
    {
        auto const iter（m_cont.insert_commit（*p，d））；
        按时间顺序。list.push_back（*p）；
        返回std:：make_pair（迭代器（iter），true）；
    }
    删除元素（p）；
    返回std：：make_pair（迭代器（result.first），false）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool is_const，class iterator，class base，class>
beast:：detail:：aged_container_iterator<false，iterator，base>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
擦除（beast:：detail:：aged_container_iterator<is_const，iterator，base>pos）
{
    unlink_and_delete_element（&*（（pos++）.iterator（））；
    返回beast:：detail:：aged_container_iterator<
        false，迭代器，base>（pos.迭代器（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool is_const，class iterator，class base，class>
beast:：detail:：aged_container_iterator<false，iterator，base>
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
擦除（beast:：detail:：aged_container_iterator<is_const，iterator，base>first，
       beast:：detail:：aged_container_iterator<is_const，iterator，base>last）
{
    为了；（首先）=最后；
        unlink_and_delete_element（&*（（first++）.iterator（））；

    返回beast:：detail:：aged_container_iterator<
        false，迭代器，base>（first.迭代器（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<class k>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
删除（k const&k）->
    SiZe型
{
    自动ITER（m_cont.find（k，
        std:：cref（m_config.key_compare（））；
    if（iter==m_cont.end（））
        返回0；
    尺寸 n型（0）；
    （；）
    {
        自动P（&*ITER++）；
        bool const完成（
            m_config（*p，extract（iter->value））；
        取消链接和删除元素（P）；
        ++N；
        如果（完成）
            断裂；
    }
    返回n；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
无效
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
交换（老化的订购容器和其他）无例外
{
    交换数据（其他）；
    标准：：交换（按时间顺序，其他。按时间顺序）；
    标准：：交换（m_-cont，其他m_-cont）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<class k>
汽车
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
触摸（k const&k）->
    SiZe型
{
    auto const now（clock（）.now（））；
    尺寸 n型（0）；
    自动常数范围（等于范围（k））；
    用于（auto-iter:range）
    {
        触摸（ITER，现在）；
        ++N；
    }
    返回n；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool otherismuti，bool otherismap，
    class othert，class otherduration，class otherallocator>
布尔
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
运算符==
    陈年订箱<otherismulti，otherismap，
        key，othert，otherduration，比较，
            其他分配器>常量&其他）常量
{
    使用other=aged_ordered_container<otherismulti，otherismap，
        key，othert，otherduration，比较，
            其他分配器>；
    如果（siz）（）！=其他。siz（））
        返回错误；
    std：：等于<void>eq；
    返回std:：equal（cbegin（），cend（），other.cbegin（），other.cend（），
        [&eq，&other]（值类型常量和lhs，
            typename other：：value_type const&rhs）
        {
            返回eq（提取（lhs），其他提取（rhs））；
        （}）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
template<bool is_const，class iterator，class base，class>
无效
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
触摸（Beast:：Detail:：Aged_Container_Iterator<
    是常量，迭代器，base>pos，
        typename clock_type:：time_point const&now）
{
    auto&e（*pos.iterator（））；
    E.何时=现在；
    chronological.list.erase（chronological.list.iterator_to（e））；
    按时间排列。列表。向后推（e）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_propagate>
typename std:：enable_if<maybe_propagate>：类型
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
交换数据（老化的订购容器和其他）无例外
{
    std:：swap（m_config.key_compare（），other.m_config.key_compare（））；
    std：：swap（m_config.alloc（），other.m_config.alloc（））；
    std:：swap（m_config.clock，其他.m_config.clock）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
模板<bool maybe_propagate>
typename std：：启用_if<！可能传播>：：类型
旧的_ordered_container<ismuti，ismap，key，t，clock，compare，allocator>：：
交换数据（老化的订购容器和其他）无例外
{
    std:：swap（m_config.key_compare（），other.m_config.key_compare（））；
    std:：swap（m_config.clock，其他.m_config.clock）；
}

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
结构是过时的容器
        ismuti、ismap、key、t、clock、compare、allocator>>
    ：std:：true_类型
{
    explicit is_aged_container（）=默认值；
}；

//自由函数

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类比较、类分配器>
无效交换
    beast:：detail:：aged_ordered_container<ismulti，ismap，
        key，t，clock，compare，allocator>和lhs，
    beast:：detail:：aged_ordered_container<ismulti，ismap，
        key，t，clock，compare，allocator>&rhs）无异常
{
    LHS互换（RHS）；
}

/**过期的容器项目超过指定的期限。*/

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Compare, class Allocator,
        class Rep, class Period>
std::size_t expire (detail::aged_ordered_container <
    IsMulti, IsMap, Key, T, Clock, Compare, Allocator>& c,
        std::chrono::duration <Rep, Period> const& age)
{
    std::size_t n (0);
    auto const expired (c.clock().now() - age);
    for (auto iter (c.chronological.cbegin());
        iter != c.chronological.cend() &&
            iter.when() <= expired;)
    {
        iter = c.erase (iter);
        ++n;
    }
    return n;
}

}

#endif
