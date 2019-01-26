
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

#ifndef RIPPLE_PEERFINDER_LIVECACHE_H_INCLUDED
#define RIPPLE_PEERFINDER_LIVECACHE_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/random.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/iosformat.h>
#include <ripple/peerfinder/impl/Tuning.h>
#include <ripple/beast/container/aged_map.h>
#include <ripple/beast/utility/maybe_const.h>
#include <boost/intrusive/list.hpp>
#include <boost/iterator/transform_iterator.hpp>

#include <algorithm>

namespace ripple {
namespace PeerFinder {

template <class>
class Livecache;

namespace detail {

class LivecacheBase
{
public:
    explicit LivecacheBase() = default;
protected:
    struct Element
        : boost::intrusive::list_base_hook <>
    {
        Element (Endpoint const& endpoint_)
            : endpoint (endpoint_)
        {
        }

        Endpoint endpoint;
    };

    using list_type = boost::intrusive::make_list <Element,
        boost::intrusive::constant_time_size <false>
            >::type;

public:
    /*位于相同跃点的端点列表
        这是一个围绕对底层的引用的轻量级包装器
        容器。
    **/

    template <bool IsConst>
    class Hop
    {
    public:
//从元素中提取端点的迭代器转换
        struct Transform
#ifdef _LIBCPP_VERSION
            : public std::unary_function<Element, Endpoint>
#endif
        {
#ifndef _LIBCPP_VERSION
            using first_argument = Element;
            using result_type = Endpoint;
#endif

            explicit Transform() = default;

            Endpoint const& operator() (Element const& e) const
            {
                return e.endpoint;
            }
        };

    public:
        using iterator = boost::transform_iterator <Transform,
            typename list_type::const_iterator>;

        using const_iterator = iterator;

        using reverse_iterator = boost::transform_iterator <Transform,
            typename list_type::const_reverse_iterator>;

        using const_reverse_iterator = reverse_iterator;

        iterator begin () const
        {
            return iterator (m_list.get().cbegin(),
                Transform());
        }

        iterator cbegin () const
        {
            return iterator (m_list.get().cbegin(),
                Transform());
        }

        iterator end () const
        {
            return iterator (m_list.get().cend(),
                Transform());
        }

        iterator cend () const
        {
            return iterator (m_list.get().cend(),
                Transform());
        }

        reverse_iterator rbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(),
                Transform());
        }

        reverse_iterator crbegin () const
        {
            return reverse_iterator (m_list.get().crbegin(),
                Transform());
        }

        reverse_iterator rend () const
        {
            return reverse_iterator (m_list.get().crend(),
                Transform());
        }

        reverse_iterator crend () const
        {
            return reverse_iterator (m_list.get().crend(),
                Transform());
        }

//将元素移动到容器的末尾
        void move_back (const_iterator pos)
        {
            auto& e (const_cast <Element&>(*pos.base()));
            m_list.get().erase (m_list.get().iterator_to (e));
            m_list.get().push_back (e);
        }

    private:
        explicit Hop (typename beast::maybe_const <
            IsConst, list_type>::type& list)
            : m_list (list)
        {
        }

        friend class LivecacheBase;

        std::reference_wrapper <typename beast::maybe_const <
            IsConst, list_type>::type> m_list;
    };

protected:
//利用LiveCache调用Hop的私有构造函数
    template <bool IsConst>
    static Hop <IsConst> make_hop (typename beast::maybe_const <
        IsConst, list_type>::type& list)
    {
        return Hop <IsConst> (list);
    }
};

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*LiveCache保存短暂的中继端点消息。

    因为同龄人只有在有空位的时候才会做广告，
    我们希望这些梅萨格在同龄人变为
    满的。

    添加到缓存的地址没有进行连接测试以查看
    它们是可连接的（有一个关于邻居的小例外）。
    因此，这些地址不适合在
    启动或用于引导，因为它们没有可验证的
    以及本地观察到的正常运行时间和连接信息。
**/

template <class Allocator = std::allocator <char>>
class Livecache : protected detail::LivecacheBase
{
private:
    using cache_type = beast::aged_map <beast::IP::Endpoint, Element,
        std::chrono::steady_clock, std::less <beast::IP::Endpoint>,
            Allocator>;

    beast::Journal m_journal;
    cache_type m_cache;

public:
    using allocator_type = Allocator;

    /*创建缓存。*/
    Livecache (
        clock_type& clock,
        beast::Journal journal,
        Allocator alloc = Allocator());

//
//按跃点迭代
//
//范围[开始，结束]提供列表类型的序列
//其中每个列表包含给定跃点处的端点。
//

    class hops_t
    {
    private:
//hops=0的端点表示本地节点。
//到达maxhops的端点存储在maxhops+1，
//但没有给出（因为它们会超过最大跳数）。他们
//用于自动连接尝试。
//
        using Histogram = std::array <int, 1 + Tuning::maxHops + 1>;
        using lists_type = std::array <list_type, 1 + Tuning::maxHops + 1>;

        template <bool IsConst>
        struct Transform
#ifdef _LIBCPP_VERSION
            : public std::unary_function<typename lists_type::value_type, Hop<IsConst>>
#endif
        {
#ifndef _LIBCPP_VERSION
            using first_argument = typename lists_type::value_type;
            using result_type = Hop <IsConst>;
#endif

            explicit Transform() = default;

            Hop <IsConst> operator() (typename beast::maybe_const <
                IsConst, typename lists_type::value_type>::type& list) const
            {
                return make_hop <IsConst> (list);
            }
        };

    public:
        using iterator = boost::transform_iterator <Transform <false>,
            typename lists_type::iterator>;

        using const_iterator = boost::transform_iterator <Transform <true>,
            typename lists_type::const_iterator>;

        using reverse_iterator = boost::transform_iterator <Transform <false>,
            typename lists_type::reverse_iterator>;

        using const_reverse_iterator = boost::transform_iterator <Transform <true>,
            typename lists_type::const_reverse_iterator>;

        iterator begin ()
        {
            return iterator (m_lists.begin(),
                Transform <false>());
        }

        const_iterator begin () const
        {
            return const_iterator (m_lists.cbegin(),
                Transform <true>());
        }

        const_iterator cbegin () const
        {
            return const_iterator (m_lists.cbegin(),
                Transform <true>());
        }

        iterator end ()
        {
            return iterator (m_lists.end(),
                Transform <false>());
        }

        const_iterator end () const
        {
            return const_iterator (m_lists.cend(),
                Transform <true>());
        }

        const_iterator cend () const
        {
            return const_iterator (m_lists.cend(),
                Transform <true>());
        }

        reverse_iterator rbegin ()
        {
            return reverse_iterator (m_lists.rbegin(),
                Transform <false>());
        }

        const_reverse_iterator rbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(),
                Transform <true>());
        }

        const_reverse_iterator crbegin () const
        {
            return const_reverse_iterator (m_lists.crbegin(),
                Transform <true>());
        }

        reverse_iterator rend ()
        {
            return reverse_iterator (m_lists.rend(),
                Transform <false>());
        }

        const_reverse_iterator rend () const
        {
            return const_reverse_iterator (m_lists.crend(),
                Transform <true>());
        }

        const_reverse_iterator crend () const
        {
            return const_reverse_iterator (m_lists.crend(),
                Transform <true>());
        }

        /*随机播放每个跃点列表。*/
        void shuffle ();

        std::string histogram() const;

    private:
        explicit hops_t (Allocator const& alloc);

        void insert (Element& e);

//重新插入新的啤酒花
        void reinsert (Element& e, int hops);

        void remove (Element& e);

        friend class Livecache;
        lists_type m_lists;
        Histogram m_hist;
    } hops;

    /*如果缓存为空，则返回“true”。*/
    bool empty () const
    {
        return m_cache.empty ();
    }

    /*返回缓存中的条目数。*/
    typename cache_type::size_type size() const
    {
        return m_cache.size();
    }

    /*删除时间已过的条目。*/
    void expire ();

    /*基于新消息创建或更新现有元素。*/
    void insert (Endpoint const& ep);

    /*输出统计。*/
    void onWrite (beast::PropertyStream::Map& map);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Allocator>
Livecache <Allocator>::Livecache (
    clock_type& clock,
    beast::Journal journal,
    Allocator alloc)
    : m_journal (journal)
    , m_cache (clock, alloc)
    , hops (alloc)
{
}

template <class Allocator>
void
Livecache <Allocator>::expire()
{
    std::size_t n (0);
    typename cache_type::time_point const expired (
        m_cache.clock().now() - Tuning::liveCacheSecondsToLive);
    for (auto iter (m_cache.chronological.begin());
        iter != m_cache.chronological.end() && iter.when() <= expired;)
    {
        Element& e (iter->second);
        hops.remove (e);
        iter = m_cache.erase (iter);
        ++n;
    }
    if (n > 0)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache expired " << n <<
            ((n > 1) ? " entries" : " entry");
    }
}

template <class Allocator>
void Livecache <Allocator>::insert (Endpoint const& ep)
{
//调用者已经增加了跃点，因此如果我们
//消息在maxhops，我们将它存储在maxhops+1。
//这意味着我们不会把地址透露给其他同行。
//但我们会用它来建立联系并分发出去
//重定向时。
//
    assert (ep.hops <= (Tuning::maxHops + 1));
    std::pair <typename cache_type::iterator, bool> result (
        m_cache.emplace (ep.address, ep));
    Element& e (result.first->second);
    if (result.second)
    {
        hops.insert (e);
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache insert " << ep.address <<
            " at hops " << ep.hops;
        return;
    }
    else if (! result.second && (ep.hops > e.endpoint.hops))
    {
//以较高的跃点删除重复项
        std::size_t const excess (
            ep.hops - e.endpoint.hops);
        JLOG(m_journal.trace()) << beast::leftw(18) <<
            "Livecache drop " << ep.address <<
            " at hops +" << excess;
        return;
    }

    m_cache.touch (result.first);

//地址已经在缓存中，因此更新元数据
    if (ep.hops < e.endpoint.hops)
    {
        hops.reinsert (e, ep.hops);
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Livecache update " << ep.address <<
            " at hops " << ep.hops;
    }
    else
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Livecache refresh " << ep.address <<
            " at hops " << ep.hops;
    }
}

template <class Allocator>
void
Livecache <Allocator>::onWrite (beast::PropertyStream::Map& map)
{
    typename cache_type::time_point const expired (
        m_cache.clock().now() - Tuning::liveCacheSecondsToLive);
    map ["size"] = size ();
    map ["hist"] = hops.histogram();
    beast::PropertyStream::Set set ("entries", map);
    for (auto iter (m_cache.cbegin()); iter != m_cache.cend(); ++iter)
    {
        auto const& e (iter->second);
        beast::PropertyStream::Map item (set);
        item ["hops"] = e.endpoint.hops;
        item ["address"] = e.endpoint.address.to_string ();
        std::stringstream ss;
        ss << (iter.when() - expired).count();
        item ["expires"] = ss.str();
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Allocator>
void
Livecache <Allocator>::hops_t::shuffle()
{
    for (auto& list : m_lists)
    {
        std::vector <std::reference_wrapper <Element>> v;
        v.reserve (list.size());
        std::copy (list.begin(), list.end(),
            std::back_inserter (v));
        std::shuffle (v.begin(), v.end(), default_prng());
        list.clear();
        for (auto& e : v)
            list.push_back (e);
    }
}

template <class Allocator>
std::string
Livecache <Allocator>::hops_t::histogram() const
{
    std::stringstream ss;
    for (typename decltype(m_hist)::size_type i (0);
        i < m_hist.size(); ++i)
    {
        ss << m_hist[i] <<
            ((i < Tuning::maxHops + 1) ? ", " : "");
    }
    return ss.str();
}

template <class Allocator>
Livecache <Allocator>::hops_t::hops_t (Allocator const& alloc)
{
    std::fill (m_hist.begin(), m_hist.end(), 0);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::insert (Element& e)
{
    assert (e.endpoint.hops >= 0 &&
        e.endpoint.hops <= Tuning::maxHops + 1);
//这对安全性有影响，不需要混乱
    m_lists [e.endpoint.hops].push_front (e);
    ++m_hist [e.endpoint.hops];
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::reinsert (Element& e, int hops)
{
    assert (hops >= 0 && hops <= Tuning::maxHops + 1);
    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
    --m_hist [e.endpoint.hops];

    e.endpoint.hops = hops;
    insert (e);
}

template <class Allocator>
void
Livecache <Allocator>::hops_t::remove (Element& e)
{
    --m_hist [e.endpoint.hops];
    list_type& list (m_lists [e.endpoint.hops]);
    list.erase (list.iterator_to (e));
}

}
}

#endif
