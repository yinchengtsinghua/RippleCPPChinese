
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

#ifndef BEAST_CONTAINER_DETAIL_AGED_UNORDERED_CONTAINER_H_INCLUDED
#define BEAST_CONTAINER_DETAIL_AGED_UNORDERED_CONTAINER_H_INCLUDED

#include <ripple/beast/container/detail/aged_container_iterator.h>
#include <ripple/beast/container/detail/aged_associative_container.h>
#include <ripple/beast/container/aged_container.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <boost/beast/core/detail/empty_base_optimization.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <algorithm>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

/*

托多

-添加采用存储桶计数的构造函数变体

-审查无例外和例外保证

-调用采用4个迭代器的IS排列的安全版本

**/


#ifndef BEAST_NO_CXX14_IS_PERMUTATION
#define BEAST_NO_CXX14_IS_PERMUTATION 1
#endif

namespace beast {
namespace detail {

/*关联容器，其中每个元素也按时间索引。

    此容器无序地镜像标准库的接口
    关联容器，添加每个元素
    用从时钟值中获得的“when”和“time”点
    “现在”。函数“touch”将元素的时间更新为当前
    时钟报告的时间。

    中提供了一组额外的迭代器类型和成员函数。
    允许在时间或反转中遍历的'chronological'成员空间
    时间顺序。此容器可用作缓存的构建基块
    其项目在一定时间后到期。按时间顺序
    迭代器允许完全可定制的过期策略。

    @看老旧无序集，老旧无序集
    @查看老年人无序地图，老年人无序地图
**/

template <
    bool IsMulti,
    bool IsMap,
    class Key,
    class T,
    class Clock = std::chrono::steady_clock,
    class Hash = std::hash <Key>,
    class KeyEqual = std::equal_to <Key>,
    class Allocator = std::allocator <
        typename std::conditional <IsMap,
            std::pair <Key const, T>,
                Key>::type>
>
class aged_unordered_container
{
public:
    using clock_type = abstract_clock<Clock>;
    using time_point = typename clock_type::time_point;
    using duration = typename clock_type::duration;
    using key_type = Key;
    using mapped_type = T;
    using value_type = typename std::conditional <IsMap,
        std::pair <Key const, T>, Key>::type;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

//自省（用于单元测试）
    using is_unordered = std::true_type;
    using is_multi = std::integral_constant <bool, IsMulti>;
    using is_map = std::integral_constant <bool, IsMap>;

private:
    static Key const& extract (value_type const& value)
    {
        return aged_associative_container_extract_t <IsMap> () (value);
    }

//vfalc todo提升以删除模板参数依赖项
    struct element
        : boost::intrusive::unordered_set_base_hook <
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

            using value_type = typename aged_unordered_container::value_type;
            using time_point = typename aged_unordered_container::time_point;
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

//vfalc todo提升以删除模板参数依赖项
    class ValueHash
        : private boost::beast::detail::empty_base_optimization <Hash>
#ifdef _LIBCPP_VERSION
        , public std::unary_function <element, std::size_t>
#endif
    {
    public:
#ifndef _LIBCPP_VERSION
        using argument_type = element;
        using result_type = size_t;
#endif

        ValueHash ()
        {
        }

        ValueHash (Hash const& hash)
            : boost::beast::detail::empty_base_optimization <Hash> (hash)
        {
        }

        std::size_t operator() (element const& e) const
        {
            return this->member() (extract (e.value));
        }

        Hash& hash_function()
        {
            return this->member();
        }

        Hash const& hash_function() const
        {
            return this->member();
        }
    };

//比较值类型与元素，用于查找/插入检查
//vfalc todo提升以删除模板参数依赖项
    class KeyValueEqual
        : private boost::beast::detail::empty_base_optimization <KeyEqual>
#ifdef _LIBCPP_VERSION
        , public std::binary_function <Key, element, bool>
#endif
    {
    public:
#ifndef _LIBCPP_VERSION
        using first_argument_type = Key;
        using second_argument_type = element;
        using result_type = bool;
#endif

        KeyValueEqual ()
        {
        }

        KeyValueEqual (KeyEqual const& keyEqual)
            : boost::beast::detail::empty_base_optimization <KeyEqual> (keyEqual)
        {
        }

//vfalc注意，我们可能只想启用这些重载
//如果keyEqual具有透明
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

        bool operator() (element const& lhs, element const& rhs) const
        {
            return this->member() (extract (lhs.value), extract (rhs.value));
        }

        KeyEqual& key_eq()
        {
            return this->member();
        }

        KeyEqual const& key_eq() const
        {
            return this->member();
        }
    };

    using list_type = typename boost::intrusive::make_list <element,
        boost::intrusive::constant_time_size <false>>::type;

    using cont_type = typename std::conditional <
        IsMulti,
        typename boost::intrusive::make_unordered_multiset <element,
            boost::intrusive::constant_time_size <true>,
            boost::intrusive::hash <ValueHash>,
            boost::intrusive::equal <KeyValueEqual>,
            boost::intrusive::cache_begin <true>
                    >::type,
        typename boost::intrusive::make_unordered_set <element,
            boost::intrusive::constant_time_size <true>,
            boost::intrusive::hash <ValueHash>,
            boost::intrusive::equal <KeyValueEqual>,
            boost::intrusive::cache_begin <true>
                    >::type
        >::type;

    using bucket_type = typename cont_type::bucket_type;
    using bucket_traits = typename cont_type::bucket_traits;

    using ElementAllocator = typename std::allocator_traits <
        Allocator>::template rebind_alloc <element>;

    using ElementAllocatorTraits = std::allocator_traits <ElementAllocator>;

    using BucketAllocator = typename std::allocator_traits <
        Allocator>::template rebind_alloc <element>;

    using BucketAllocatorTraits = std::allocator_traits <BucketAllocator>;

    class config_t
        : private ValueHash
        , private KeyValueEqual
        , private boost::beast::detail::empty_base_optimization <ElementAllocator>
    {
    public:
        explicit config_t (
            clock_type& clock_)
            : clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Hash const& hash)
            : ValueHash (hash)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            KeyEqual const& keyEqual)
            : KeyValueEqual (keyEqual)
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
            Hash const& hash,
            KeyEqual const& keyEqual)
            : ValueHash (hash)
            , KeyValueEqual (keyEqual)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Hash const& hash,
            Allocator const& alloc_)
            : ValueHash (hash)
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc_)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            KeyEqual const& keyEqual,
            Allocator const& alloc_)
            : KeyValueEqual (keyEqual)
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc_)
            , clock (clock_)
        {
        }

        config_t (
            clock_type& clock_,
            Hash const& hash,
            KeyEqual const& keyEqual,
            Allocator const& alloc_)
            : ValueHash (hash)
            , KeyValueEqual (keyEqual)
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc_)
            , clock (clock_)
        {
        }

        config_t (config_t const& other)
            : ValueHash (other.hash_function())
            , KeyValueEqual (other.key_eq())
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (
                ElementAllocatorTraits::
                    select_on_container_copy_construction (
                        other.alloc()))
            , clock (other.clock)
        {
        }

        config_t (config_t const& other, Allocator const& alloc)
            : ValueHash (other.hash_function())
            , KeyValueEqual (other.key_eq())
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc)
            , clock (other.clock)
        {
        }

        config_t (config_t&& other)
            : ValueHash (std::move (other.hash_function()))
            , KeyValueEqual (std::move (other.key_eq()))
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (
                std::move (other.alloc()))
            , clock (other.clock)
        {
        }

        config_t (config_t&& other, Allocator const& alloc)
            : ValueHash (std::move (other.hash_function()))
            , KeyValueEqual (std::move (other.key_eq()))
            , boost::beast::detail::empty_base_optimization <ElementAllocator> (alloc)
            , clock (other.clock)
        {
        }

        config_t& operator= (config_t const& other)
        {
            hash_function() = other.hash_function();
            key_eq() = other.key_eq();
            alloc() = other.alloc();
            clock = other.clock;
            return *this;
        }

        config_t& operator= (config_t&& other)
        {
            hash_function() = std::move (other.hash_function());
            key_eq() = std::move (other.key_eq());
            alloc() = std::move (other.alloc());
            clock = other.clock;
            return *this;
        }

        ValueHash& value_hash()
        {
            return *this;
        }

        ValueHash const& value_hash() const
        {
            return *this;
        }

        Hash& hash_function()
        {
            return ValueHash::hash_function();
        }

        Hash const& hash_function() const
        {
            return ValueHash::hash_function();
        }

        KeyValueEqual& key_value_equal()
        {
            return *this;
        }

        KeyValueEqual const& key_value_equal() const
        {
            return *this;
        }

        KeyEqual& key_eq()
        {
            return key_value_equal().key_eq();
        }

        KeyEqual const& key_eq() const
        {
            return key_value_equal().key_eq();
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

    class Buckets
    {
    public:
        using vec_type = std::vector<
            bucket_type,
            typename std::allocator_traits <Allocator>::
                template rebind_alloc <bucket_type>>;

        Buckets ()
            : m_max_load_factor (1.f)
            , m_vec ()
        {
            m_vec.resize (
                cont_type::suggested_upper_bucket_count (0));
        }

        Buckets (Allocator const& alloc)
            : m_max_load_factor (1.f)
            , m_vec (alloc)
        {
            m_vec.resize (
                cont_type::suggested_upper_bucket_count (0));
        }

        operator bucket_traits()
        {
            return bucket_traits (&m_vec[0], m_vec.size());
        }

        void clear()
        {
            m_vec.clear();
        }

        size_type max_bucket_count() const
        {
            return m_vec.max_size();
        }

        float& max_load_factor()
        {
            return m_max_load_factor;
        }

        float const& max_load_factor() const
        {
            return m_max_load_factor;
        }

//Count是存储桶数
        template <class Container>
        void rehash (size_type count, Container& c)
        {
            size_type const size (m_vec.size());
            if (count == size)
                return;
            if (count > m_vec.capacity())
            {
//需要两个向量，否则我们
//将销毁非空桶。
                vec_type vec (m_vec.get_allocator());
                std::swap (m_vec, vec);
                m_vec.resize (count);
                c.rehash (bucket_traits (
                    &m_vec[0], m_vec.size()));
                return;
            }
//重新粉刷到位。
            if (count > size)
            {
//这不应该重新分配，因为
//我们之前检查过容量。
                m_vec.resize (count);
                c.rehash (bucket_traits (
                    &m_vec[0], count));
                return;
            }
//重新刷新后必须调整大小，否则
//我们可以销毁非空桶。
            c.rehash (bucket_traits (
                &m_vec[0], count));
            m_vec.resize (count);
        }

//调整存储桶的大小，以容纳至少n个项目。
        template <class Container>
        void resize (size_type n, Container& c)
        {
            size_type const suggested (
                cont_type::suggested_upper_bucket_count (n));
            rehash (suggested, c);
        }

    private:
        float m_max_load_factor;
        vec_type m_vec;
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
    using hasher = Hash;
    using key_equal = KeyEqual;
    using allocator_type = Allocator;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = typename std::allocator_traits <
        Allocator>::pointer;
    using const_pointer = typename std::allocator_traits <
        Allocator>::const_pointer;

//集合迭代器（ismap==false）始终是常量
//因为集合的元素是不可变的。
    using iterator= beast::detail::aged_container_iterator <!IsMap,
        typename cont_type::iterator>;
    using const_iterator = beast::detail::aged_container_iterator <true,
        typename cont_type::iterator>;

    using local_iterator = beast::detail::aged_container_iterator <!IsMap,
        typename cont_type::local_iterator>;
    using const_local_iterator = beast::detail::aged_container_iterator <true,
        typename cont_type::local_iterator>;

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
        using iterator = beast::detail::aged_container_iterator <
            ! IsMap, typename list_type::iterator>;
        using const_iterator = beast::detail::aged_container_iterator <
            true, typename list_type::iterator>;
        using reverse_iterator = beast::detail::aged_container_iterator <
            ! IsMap, typename list_type::reverse_iterator>;
        using const_reverse_iterator = beast::detail::aged_container_iterator <
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

        friend class aged_unordered_container;
        list_type mutable list;
    } chronological;

//——————————————————————————————————————————————————————————————
//
//施工
//
//——————————————————————————————————————————————————————————————

    aged_unordered_container() = delete;

    explicit aged_unordered_container (clock_type& clock);

    aged_unordered_container (clock_type& clock, Hash const& hash);

    aged_unordered_container (clock_type& clock,
        KeyEqual const& key_eq);

    aged_unordered_container (clock_type& clock,
        Allocator const& alloc);

    aged_unordered_container (clock_type& clock,
        Hash const& hash, KeyEqual const& key_eq);

    aged_unordered_container (clock_type& clock,
        Hash const& hash, Allocator const& alloc);

    aged_unordered_container (clock_type& clock,
        KeyEqual const& key_eq, Allocator const& alloc);

    aged_unordered_container (
        clock_type& clock, Hash const& hash, KeyEqual const& key_eq,
            Allocator const& alloc);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, Hash const& hash);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, KeyEqual const& key_eq);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, Allocator const& alloc);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, Hash const& hash, KeyEqual const& key_eq);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, Hash const& hash, Allocator const& alloc);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, KeyEqual const& key_eq,
            Allocator const& alloc);

    template <class InputIt>
    aged_unordered_container (InputIt first, InputIt last,
        clock_type& clock, Hash const& hash, KeyEqual const& key_eq,
            Allocator const& alloc);

    aged_unordered_container (aged_unordered_container const& other);

    aged_unordered_container (aged_unordered_container const& other,
        Allocator const& alloc);

    aged_unordered_container (aged_unordered_container&& other);

    aged_unordered_container (aged_unordered_container&& other,
        Allocator const& alloc);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Hash const& hash);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, KeyEqual const& key_eq);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Allocator const& alloc);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Hash const& hash, KeyEqual const& key_eq);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Hash const& hash, Allocator const& alloc);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, KeyEqual const& key_eq, Allocator const& alloc);

    aged_unordered_container (std::initializer_list <value_type> init,
        clock_type& clock, Hash const& hash, KeyEqual const& key_eq,
            Allocator const& alloc);

    ~aged_unordered_container();

    aged_unordered_container& operator= (aged_unordered_container const& other);

    aged_unordered_container& operator= (aged_unordered_container&& other);

    aged_unordered_container& operator= (std::initializer_list <value_type> init);

    allocator_type get_allocator() const
    {
        return m_config.alloc();
    }

    clock_type& clock()
    {
        return m_config.clock;
    }

    clock_type const& clock() const
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

    bool empty() const noexcept
    {
        return m_cont.empty();
    }

    size_type size() const noexcept
    {
        return m_cont.size();
    }

    size_type max_size() const noexcept
    {
        return m_config.max_size();
    }

//——————————————————————————————————————————————————————————————
//
//修饰语
//
//——————————————————————————————————————————————————————————————

    void clear();

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

//集合映射
    template <bool maybe_multi = IsMulti, bool maybe_map = IsMap>
    auto
    insert (value_type&& value) ->
        typename std::enable_if <! maybe_multi && ! maybe_map,
            std::pair <iterator, bool>>::type;

//多重映射，多重集
    template <bool maybe_multi = IsMulti, bool maybe_map = IsMap>
    auto
    insert (value_type&& value) ->
        typename std::enable_if <maybe_multi && ! maybe_map,
            iterator>::type;

//集合映射
    template <bool maybe_multi = IsMulti>
    typename std::enable_if <! maybe_multi,
        iterator>::type
    /*ert（const_迭代器/*提示*/，value_类型const&value）
    {
        //提示被忽略，但我们提供了接口，因此
        //调用方可以交替使用有序和无序。
        返回insert（value）.first；
    }

    //多重映射，多重集
    template<bool maybe_multi=ismulti>
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
    插入（const_iterator/*hin*/, value_type const& value)

    {
//vfalc todo提示可用于
//客户订单相等范围
        return insert (value);
    }

//集合映射
    template <bool maybe_multi = IsMulti>
    typename std::enable_if <! maybe_multi,
        iterator>::type
    /*ert（const_迭代器/*提示*/，value_类型和value）
    {
        //提示被忽略，但我们提供了接口，因此
        //调用方可以交替使用有序和无序。
        返回insert（std:：move（value））。首先；
    }

    //多重映射，多重集
    template<bool maybe_multi=ismulti>
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
    插入（const_iterator/*hin*/, value_type&& value)

    {
//vfalc todo提示可用于
//客户订单相等范围
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
            iterator, std::pair <iterator, bool>
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
            iterator, std::pair <iterator, bool>
        >::type
    >::type
    insert (const_iterator hint, P&& value)
    {
        return emplace_hint (hint, std::forward <P> (value));
    }

    template <class InputIt>
    void insert (InputIt first, InputIt last)
    {
        insert (first, last,
            typename std::iterator_traits <
                InputIt>::iterator_category());
    }

    void
    insert (std::initializer_list <value_type> init)
    {
        insert (init.begin(), init.end());
    }

//集合，映射
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

//集合，映射
    template <bool maybe_multi = IsMulti, class... Args>
    auto
    /*lace_提示（const_迭代器/*提示*/，参数&……ARGS）>
        typename std：：启用_if<！可能是多种多样的，
            std:：pair<iterator，bool>>：类型；

    //多集，多映射
    template<bool maybe_multi=ismulti，class…ARG>
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
    emplace_提示（const_迭代器/*hin*/, Args&&... args)

    {
//vfalc todo提示可用于multi，to let
//客户订单相等范围
        return emplace <maybe_multi> (
            std::forward <Args> (args)...);
    }

    template <bool is_const, class Iterator, class Base>
    beast::detail::aged_container_iterator <false, Iterator, Base>
    erase (beast::detail::aged_container_iterator <
        is_const, Iterator, Base> pos);

    template <bool is_const, class Iterator, class Base>
    beast::detail::aged_container_iterator <false, Iterator, Base>
    erase (beast::detail::aged_container_iterator <
        is_const, Iterator, Base> first,
            beast::detail::aged_container_iterator <
                is_const, Iterator, Base> last);

    template <class K>
    auto
    erase (K const& k) ->
        size_type;

    void
    swap (aged_unordered_container& other) noexcept;

    template <bool is_const, class Iterator, class Base>
    void
    touch (beast::detail::aged_container_iterator <
        is_const, Iterator, Base> pos)
    {
        touch (pos, clock().now());
    }

    template <class K>
    auto
    touch (K const& k) ->
        size_type;

//——————————————————————————————————————————————————————————————
//
//查找
//
//——————————————————————————————————————————————————————————————

//VFACO-toDO对ISO透明（C++ 14）的尊重
    template <class K>
    size_type
    count (K const& k) const
    {
        return m_cont.count (k, std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()));
    }

//VFACO-toDO对ISO透明（C++ 14）的尊重
    template <class K>
    iterator
    find (K const& k)
    {
        return iterator (m_cont.find (k,
            std::cref (m_config.hash_function()),
                std::cref (m_config.key_value_equal())));
    }

//VFACO-toDO对ISO透明（C++ 14）的尊重
    template <class K>
    const_iterator
    find (K const& k) const
    {
        return const_iterator (m_cont.find (k,
            std::cref (m_config.hash_function()),
                std::cref (m_config.key_value_equal())));
    }

//VFACO-toDO对ISO透明（C++ 14）的尊重
    template <class K>
    std::pair <iterator, iterator>
    equal_range (K const& k)
    {
        auto const r (m_cont.equal_range (k,
            std::cref (m_config.hash_function()),
                std::cref (m_config.key_value_equal())));
        return std::make_pair (iterator (r.first),
            iterator (r.second));
    }

//VFACO-toDO对ISO透明（C++ 14）的尊重
    template <class K>
    std::pair <const_iterator, const_iterator>
    equal_range (K const& k) const
    {
        auto const r (m_cont.equal_range (k,
            std::cref (m_config.hash_function()),
                std::cref (m_config.key_value_equal())));
        return std::make_pair (const_iterator (r.first),
            const_iterator (r.second));
    }

//——————————————————————————————————————————————————————————————
//
//Bucket接口
//
//——————————————————————————————————————————————————————————————

    local_iterator begin (size_type n)
    {
        return local_iterator (m_cont.begin (n));
    }

    const_local_iterator begin (size_type n) const
    {
        return const_local_iterator (m_cont.begin (n));
    }

    const_local_iterator cbegin (size_type n) const
    {
        return const_local_iterator (m_cont.begin (n));
    }

    local_iterator end (size_type n)
    {
        return local_iterator (m_cont.end (n));
    }

    const_local_iterator end (size_type n) const
    {
        return const_local_iterator (m_cont.end (n));
    }

    const_local_iterator cend (size_type n) const
    {
        return const_local_iterator (m_cont.end (n));
    }

    size_type bucket_count() const
    {
        return m_cont.bucket_count();
    }

    size_type max_bucket_count() const
    {
        return m_buck.max_bucket_count();
    }

    size_type bucket_size (size_type n) const
    {
        return m_cont.bucket_size (n);
    }

    size_type bucket (Key const& k) const
    {
        assert (bucket_count() != 0);
        return m_cont.bucket (k,
            std::cref (m_config.hash_function()));
    }

//——————————————————————————————————————————————————————————————
//
//散列策略
//
//——————————————————————————————————————————————————————————————

    float load_factor() const
    {
        return size() /
            static_cast <float> (m_cont.bucket_count());
    }

    float max_load_factor() const
    {
        return m_buck.max_load_factor();
    }

    void max_load_factor (float ml)
    {
        m_buck.max_load_factor () =
            std::max (ml, m_buck.max_load_factor());
    }

    void rehash (size_type count)
    {
        count = std::max (count,
            size_type (size() / max_load_factor()));
        m_buck.rehash (count, m_cont);
    }

    void reserve (size_type count)
    {
        rehash (std::ceil (count / max_load_factor()));
    }

//——————————————————————————————————————————————————————————————
//
//观察员
//
//——————————————————————————————————————————————————————————————

    hasher const& hash_function() const
    {
        return m_config.hash_function();
    }

    key_equal const& key_eq () const
    {
        return m_config.key_eq();
    }

//——————————————————————————————————————————————————————————————
//
//比较
//
//——————————————————————————————————————————————————————————————

//这与标准的不同之处在于
//只对值类型的键部分执行，忽略
//映射的类型。
//
    template <
        bool OtherIsMap,
        class OtherKey,
        class OtherT,
        class OtherDuration,
        class OtherHash,
        class OtherAllocator,
        bool maybe_multi = IsMulti
    >
    typename std::enable_if <! maybe_multi, bool>::type
    operator== (
        aged_unordered_container <false, OtherIsMap,
            OtherKey, OtherT, OtherDuration, OtherHash, KeyEqual,
                OtherAllocator> const& other) const;

    template <
        bool OtherIsMap,
        class OtherKey,
        class OtherT,
        class OtherDuration,
        class OtherHash,
        class OtherAllocator,
        bool maybe_multi = IsMulti
    >
    typename std::enable_if <maybe_multi, bool>::type
    operator== (
        aged_unordered_container <true, OtherIsMap,
            OtherKey, OtherT, OtherDuration, OtherHash, KeyEqual,
                OtherAllocator> const& other) const;

    template <
        bool OtherIsMulti,
        bool OtherIsMap,
        class OtherKey,
        class OtherT,
        class OtherDuration,
        class OtherHash,
        class OtherAllocator
    >
    bool operator!= (
        aged_unordered_container <OtherIsMulti, OtherIsMap,
            OtherKey, OtherT, OtherDuration, OtherHash, KeyEqual,
                OtherAllocator> const& other) const
    {
        return ! (this->operator== (other));
    }

private:
    bool
    would_exceed (size_type additional) const
    {
        return size() + additional >
            bucket_count() * max_load_factor();
    }

    void
    maybe_rehash (size_type additional)
    {
        if (would_exceed (additional))
            m_buck.resize (size() + additional, m_cont);
        assert (load_factor() <= max_load_factor());
    }

//集合映射
    template <bool maybe_multi = IsMulti>
    auto
    insert_unchecked (value_type const& value) ->
        typename std::enable_if <! maybe_multi,
            std::pair <iterator, bool>>::type;

//多重映射，多重集
    template <bool maybe_multi = IsMulti>
    auto
    insert_unchecked (value_type const& value) ->
        typename std::enable_if <maybe_multi,
            iterator>::type;

    template <class InputIt>
    void
    insert_unchecked (InputIt first, InputIt last)
    {
        for (; first != last; ++first)
            insert_unchecked (*first);
    }

    template <class InputIt>
    void
    insert (InputIt first, InputIt last,
        std::input_iterator_tag)
    {
        for (; first != last; ++first)
            insert (*first);
    }

    template <class InputIt>
    void
    insert (InputIt first, InputIt last,
        std::random_access_iterator_tag)
    {
        auto const n (std::distance (first, last));
        maybe_rehash (n);
        insert_unchecked (first, last);
    }

    template <bool is_const, class Iterator, class Base>
    void
    touch (beast::detail::aged_container_iterator <
        is_const, Iterator, Base> pos,
            typename clock_type::time_point const& now)
    {
        auto& e (*pos.iterator());
        e.when = now;
        chronological.list.erase (chronological.list.iterator_to (e));
        chronological.list.push_back (e);
    }

    template <bool maybe_propagate = std::allocator_traits <
        Allocator>::propagate_on_container_swap::value>
    typename std::enable_if <maybe_propagate>::type
    swap_data (aged_unordered_container& other) noexcept
    {
        std::swap (m_config.key_compare(), other.m_config.key_compare());
        std::swap (m_config.alloc(), other.m_config.alloc());
        std::swap (m_config.clock, other.m_config.clock);
    }

    template <bool maybe_propagate = std::allocator_traits <
        Allocator>::propagate_on_container_swap::value>
    typename std::enable_if <! maybe_propagate>::type
    swap_data (aged_unordered_container& other) noexcept
    {
        std::swap (m_config.key_compare(), other.m_config.key_compare());
        std::swap (m_config.clock, other.m_config.clock);
    }

private:
    config_t m_config;
    Buckets m_buck;
    cont_type mutable m_cont;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock)
    : m_config (clock)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    Hash const& hash)
    : m_config (clock, hash)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    KeyEqual const& key_eq)
    : m_config (clock, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    Allocator const& alloc)
    : m_config (clock, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq)
    : m_config (clock, hash, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    Hash const& hash,
    Allocator const& alloc)
    : m_config (clock, hash, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, hash, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock)
    : m_config (clock)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    Hash const& hash)
    : m_config (clock, hash)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    KeyEqual const& key_eq)
    : m_config (clock, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    Allocator const& alloc)
    : m_config (clock, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq)
    : m_config (clock, hash, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    Hash const& hash,
    Allocator const& alloc)
    : m_config (clock, hash, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class InputIt>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (InputIt first, InputIt last,
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, hash, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (first, last);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (aged_unordered_container const& other)
    : m_config (other.m_config)
    , m_buck (m_config.alloc())
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (other.cbegin(), other.cend());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (aged_unordered_container const& other,
    Allocator const& alloc)
    : m_config (other.m_config, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (other.cbegin(), other.cend());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (aged_unordered_container&& other)
    : m_config (std::move (other.m_config))
    , m_buck (std::move (other.m_buck))
    , m_cont (std::move (other.m_cont))
{
    chronological.list = std::move (other.chronological.list);
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (aged_unordered_container&& other,
    Allocator const& alloc)
    : m_config (std::move (other.m_config), alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (other.cbegin(), other.cend());
    other.clear ();
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock)
    : m_config (clock)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    Hash const& hash)
    : m_config (clock, hash)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    KeyEqual const& key_eq)
    : m_config (clock, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    Allocator const& alloc)
    : m_config (clock, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq)
    : m_config (clock, hash, key_eq)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    Hash const& hash,
    Allocator const& alloc)
    : m_config (clock, hash, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
aged_unordered_container (std::initializer_list <value_type> init,
    clock_type& clock,
    Hash const& hash,
    KeyEqual const& key_eq,
    Allocator const& alloc)
    : m_config (clock, hash, key_eq, alloc)
    , m_buck (alloc)
    , m_cont (m_buck,
        std::cref (m_config.value_hash()),
            std::cref (m_config.key_value_equal()))
{
    insert (init.begin(), init.end());
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
~aged_unordered_container()
{
    clear();
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
operator= (aged_unordered_container const& other)
    -> aged_unordered_container&
{
    if (this != &other)
    {
        size_type const n (other.size());
        clear();
        m_config = other.m_config;
        m_buck = Buckets (m_config.alloc());
        maybe_rehash (n);
        insert_unchecked (other.begin(), other.end());
    }
    return *this;
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
operator= (aged_unordered_container&& other) ->
    aged_unordered_container&
{
    size_type const n (other.size());
    clear();
    m_config = std::move (other.m_config);
    m_buck = Buckets (m_config.alloc());
    maybe_rehash (n);
    insert_unchecked (other.begin(), other.end());
    other.clear();
    return *this;
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
operator= (std::initializer_list <value_type> init) ->
    aged_unordered_container&
{
    clear ();
    insert (init);
    return *this;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class K, bool maybe_multi, bool maybe_map, class>
typename std::conditional <IsMap, T, void*>::type&
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
at (K const& k)
{
    auto const iter (m_cont.find (k,
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal())));
    if (iter == m_cont.end())
        throw std::out_of_range ("key not found");
    return iter->value.second;
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <class K, bool maybe_multi, bool maybe_map, class>
typename std::conditional <IsMap, T, void*>::type const&
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
at (K const& k) const
{
    auto const iter (m_cont.find (k,
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal())));
    if (iter == m_cont.end())
        throw std::out_of_range ("key not found");
    return iter->value.second;
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, bool maybe_map, class>
typename std::conditional <IsMap, T, void*>::type&
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
operator[] (Key const& key)
{
    maybe_rehash (1);
    typename cont_type::insert_commit_data d;
    auto const result (m_cont.insert_check (key,
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()), d));
    if (result.second)
    {
        element* const p (new_element (
            std::piecewise_construct,
                std::forward_as_tuple (key),
                    std::forward_as_tuple ()));
        m_cont.insert_commit (*p, d);
        chronological.list.push_back (*p);
        return p->value.second;
    }
    return result.first->value.second;
}

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, bool maybe_map, class>
typename std::conditional <IsMap, T, void*>::type&
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
operator[] (Key&& key)
{
    maybe_rehash (1);
    typename cont_type::insert_commit_data d;
    auto const result (m_cont.insert_check (key,
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()), d));
    if (result.second)
    {
        element* const p (new_element (
            std::piecewise_construct,
                std::forward_as_tuple (std::move (key)),
                    std::forward_as_tuple ()));
        m_cont.insert_commit (*p, d);
        chronological.list.push_back (*p);
        return p->value.second;
    }
    return result.first->value.second;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
void
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
clear()
{
    for (auto iter (chronological.list.begin());
        iter != chronological.list.end();)
        unlink_and_delete_element (&*iter++);
    chronological.list.clear();
    m_cont.clear();
    m_buck.clear();
}

//集合映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
insert (value_type const& value) ->
    typename std::enable_if <! maybe_multi,
        std::pair <iterator, bool>>::type
{
    maybe_rehash (1);
    typename cont_type::insert_commit_data d;
    auto const result (m_cont.insert_check (extract (value),
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()), d));
    if (result.second)
    {
        element* const p (new_element (value));
        auto const iter (m_cont.insert_commit (*p, d));
        chronological.list.push_back (*p);
        return std::make_pair (iterator (iter), true);
    }
    return std::make_pair (iterator (result.first), false);
}

//多重映射，多重集
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
insert (value_type const& value) ->
    typename std::enable_if <maybe_multi,
        iterator>::type
{
    maybe_rehash (1);
    element* const p (new_element (value));
    chronological.list.push_back (*p);
    auto const iter (m_cont.insert (*p));
    return iterator (iter);
}

//集合映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, bool maybe_map>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
insert (value_type&& value) ->
    typename std::enable_if <! maybe_multi && ! maybe_map,
        std::pair <iterator, bool>>::type
{
    maybe_rehash (1);
    typename cont_type::insert_commit_data d;
    auto const result (m_cont.insert_check (extract (value),
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()), d));
    if (result.second)
    {
        element* const p (new_element (std::move (value)));
        auto const iter (m_cont.insert_commit (*p, d));
        chronological.list.push_back (*p);
        return std::make_pair (iterator (iter), true);
    }
    return std::make_pair (iterator (result.first), false);
}

//多重映射，多重集
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, bool maybe_map>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
insert (value_type&& value) ->
    typename std::enable_if <maybe_multi && ! maybe_map,
        iterator>::type
{
    maybe_rehash (1);
    element* const p (new_element (std::move (value)));
    chronological.list.push_back (*p);
    auto const iter (m_cont.insert (*p));
    return iterator (iter);
}

#if 1 //使用insert（）而不是insert_check（）insert_commit（）。
//集合，映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, class... Args>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
emplace (Args&&... args) ->
    typename std::enable_if <! maybe_multi,
        std::pair <iterator, bool>>::type
{
    maybe_rehash (1);
//vfalc注意到不幸的是我们需要
//在此构造元素
    element* const p (new_element (std::forward <Args> (args)...));
    auto const result (m_cont.insert (*p));
    if (result.second)
    {
        chronological.list.push_back (*p);
        return std::make_pair (iterator (result.first), true);
    }
    delete_element (p);
    return std::make_pair (iterator (result.first), false);
}
#else //作为原始文件，使用insert_check（）/insert_commit（）对。
//集合，映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, class... Args>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
emplace (Args&&... args) ->
    typename std::enable_if <! maybe_multi,
        std::pair <iterator, bool>>::type
{
    maybe_rehash (1);
//vfalc注意到不幸的是我们需要
//在此构造元素
    element* const p (new_element (
        std::forward <Args> (args)...));
    typename cont_type::insert_commit_data d;
    auto const result (m_cont.insert_check (extract (p->value),
        std::cref (m_config.hash_function()),
            std::cref (m_config.key_value_equal()), d));
    if (result.second)
    {
        auto const iter (m_cont.insert_commit (*p, d));
        chronological.list.push_back (*p);
        return std::make_pair (iterator (iter), true);
    }
    delete_element (p);
    return std::make_pair (iterator (result.first), false);
}
#endif //零

//多重集，多重映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, class... Args>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
emplace (Args&&... args) ->
    typename std::enable_if <maybe_multi,
        iterator>::type
{
    maybe_rehash (1);
    element* const p (new_element (
        std::forward <Args> (args)...));
    chronological.list.push_back (*p);
    auto const iter (m_cont.insert (*p));
    return iterator (iter);
}

//集合，映射
template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator>
template <bool maybe_multi, class... Args>
auto
aged_unordered_container <IsMulti, IsMap, Key, T, Clock,
    Hash, KeyEqual, Allocator>::
/*lace_提示（const_迭代器/*提示*/，参数&……ARGS）>
    typename std：：启用_if<！可能是多种多样的，
        std:：pair<iterator，bool>>：类型
{
    可能是ou rehash（1）；
    //vfalc注意到我们需要
    //在此处构造元素
    元素*const p（new_element（
        std:：forward<args>（args）…）；
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（extract（p->value）），
        std:：cref（m_config.hash_function（）），
            std:：cref（m_config.key_value_equal（）），d））；
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
    类时钟、类散列、类keyequal、类分配器>
template<bool is_const，class iterator，class base>
beast:：detail:：aged_container_iterator<false，iterator，base>
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
擦除（Beast:：Detail:：Aged_Container_Iterator<
    是常量，迭代器，base>pos）
{
    unlink_and_delete_element（&*（（pos++）.iterator（））；
    返回beast:：detail:：aged_container_iterator<
        false，迭代器，base>（pos.迭代器（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
template<bool is_const，class iterator，class base>
beast:：detail:：aged_container_iterator<false，iterator，base>
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
擦除（Beast:：Detail:：Aged_Container_Iterator<
    是常量，迭代器，base>第一个，
        beast:：detail:：aged_container_iterator<
            是常量、迭代器、基>最后一个）
{
    为了；（首先）=最后；
        unlink_and_delete_element（&*（（first++）.iterator（））；

    返回beast:：detail:：aged_container_iterator<
        false，迭代器，base>（first.迭代器（））；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
模板<class k>
汽车
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
删除（k const&k）->
    SiZe型
{
    auto iter（m_cont.find（k，std:：cref（m_config.hash_function（）），
        std:：cref（m_config.key_value_equal（））；
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
    类时钟、类散列、类keyequal、类分配器>
无效
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
交换（旧的无序容器和其他）无例外
{
    交换数据（其他）；
    标准：：交换（按时间顺序，其他。按时间顺序）；
    标准：：交换（m_-cont，其他m_-cont）；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
模板<class k>
汽车
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
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

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
模板<
    布尔热疗法，
    类OutKEY，
    班级其他人，
    课程其他持续时间，
    类OtherHash，
    类otherallocator，
    bool可能是\u multi
>
typename std：：启用_if<！可能是“multi，bool>：：type”
老化的无序容器
    ismuti、ismap、key、t、clock、hash、keyequal、allocator>：：
运算符==
    老化的无序容器<false，otherismap，
        Otherkey、Othert、OtherDuration、OtherHash、KeyEqual、
            其他分配器>常量&其他）常量
{
    如果（siz）（）！=其他。siz（））
        返回错误；
    对于（auto iter（cbegin（）），last（cend（）），olast（other.cend（））；
        伊特尔！=最后；+ITER）
    {
        自动oiter（other.find（extract（*iter））；
        如果（oiter==olast）
            返回错误；
    }
    回归真实；
}

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
模板<
    布尔热疗法，
    类OutKEY，
    班级其他人，
    课程其他持续时间，
    类OtherHash，
    类otherallocator，
    bool可能是\u multi
>
typename std：：启用_if<maybe_multi，bool>：type
老化的无序容器
    ismuti、ismap、key、t、clock、hash、keyequal、allocator>：：
运算符==
    老化无序容器<true，otherismap，
        Otherkey、Othert、OtherDuration、OtherHash、KeyEqual、
            其他分配器>常量&其他）常量
{
    如果（siz）（）！=其他。siz（））
        返回错误；
    使用eqrng=std:：pair<const_迭代器，const_迭代器>；
    对于（auto iter（cbegin（）），last（cend（））；iter！=最后；
    {
        auto const&k（提取（*iter））；
        自动常数eq（等_范围（k））；
        自动const oeq（其他.equal_range（k））；
如果beast_no_cx14_是_排列
        如果（std：：距离（eq.first，eq.second）！=
            标准：距离（oeq.first，oeq.second）
            ！std：：是“排列”（eq.first、eq.second、oeq.first）
            返回错误；
否则
        如果（！）std：：是排列（式1，
            式二、oeq.first、oeq.second）
            返回错误；
第二节
        Iter=等秒；
    }
    回归真实；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//映射
template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
模板<bool maybe_multi>
汽车
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
插入_unchecked（value_type const&value）->
    typename std：：启用_if<！可能是多种多样的，
        std:：pair<iterator，bool>>：类型
{
    typename cont_type：：插入_commit_data d；
    自动常量结果（m_cont.insert_check（extract（value）），
        std:：cref（m_config.hash_function（）），
            std:：cref（m_config.key_value_equal（）），d））；
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
    类时钟、类散列、类keyequal、类分配器>
模板<bool maybe_multi>
汽车
老化的无序容器<ismulti，ismap，key，t，clock，
    哈希，keyEqual，分配器>：：
插入_unchecked（value_type const&value）->
    typename std：：启用_if<maybe_multi，
        迭代器>：：类型
{
    element*const p（new_element（value））；
    按时间顺序。list.push_back（*p）；
    自动构造工具（m_cont.insert（*p））；
    返回迭代器（ITER）；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<bool ismuti，bool ismap，class key，class t，
    类时钟、类散列、类keyequal、类分配器>
结构是过时的容器
        ismuti、ismap、key、t、clock、hash、keyequal、allocator>>
    ：std:：true_类型
{
    explicit is_aged_container（）=默认值；
}；

//自由函数

template<bool ismuti，bool ismap，class key，class t，class clock，
    类哈希，类keyequal，类分配器>
无效交换
    beast:：detail:：aged_unordered_container<ismulti，ismap，
        key，t，clock，hash，keyequal，分配器>&lhs，
    beast:：detail:：aged_unordered_container<ismulti，ismap，
        key，t，clock，hash，keyequal，allocator>>&rhs）无异常
{
    LHS互换（RHS）；
}

/**过期的容器项目超过指定的期限。*/

template <bool IsMulti, bool IsMap, class Key, class T,
    class Clock, class Hash, class KeyEqual, class Allocator,
        class Rep, class Period>
std::size_t expire (beast::detail::aged_unordered_container <
    IsMulti, IsMap, Key, T, Clock, Hash, KeyEqual, Allocator>& c,
        std::chrono::duration <Rep, Period> const& age) noexcept
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
