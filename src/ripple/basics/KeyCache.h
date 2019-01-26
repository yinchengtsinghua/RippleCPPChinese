
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

#ifndef RIPPLE_BASICS_KEYCACHE_H_INCLUDED
#define RIPPLE_BASICS_KEYCACHE_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/insight/Insight.h>
#include <mutex>

namespace ripple {

/*维护没有关联数据的密钥缓存。

    缓存具有目标大小和过期时间。当缓存项变为
    年龄超过他们在
    调用@ref sweep。
**/

template <
    class Key,
    class Hash = hardened_hash <>,
    class KeyEqual = std::equal_to <Key>,
//class allocator=std:：allocator<std:：pair<key const，entry>>，
    class Mutex = std::mutex
>
class KeyCache
{
public:
    using key_type = Key;
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

private:
    struct Stats
    {
        template <class Handler>
        Stats (std::string const& prefix, Handler const& handler,
            beast::insight::Collector::ptr const& collector)
            : hook (collector->make_hook (handler))
            , size (collector->make_gauge (prefix, "size"))
            , hit_rate (collector->make_gauge (prefix, "hit_rate"))
            , hits (0)
            , misses (0)
            { }

        beast::insight::Hook hook;
        beast::insight::Gauge size;
        beast::insight::Gauge hit_rate;

        std::size_t hits;
        std::size_t misses;
    };

    struct Entry
    {
        explicit Entry (clock_type::time_point const& last_access_)
            : last_access (last_access_)
        {
        }

        clock_type::time_point last_access;
    };

    using map_type = hardened_hash_map <key_type, Entry, Hash, KeyEqual>;
    using iterator = typename map_type::iterator;
    using lock_guard = std::lock_guard <Mutex>;

public:
    using size_type = typename map_type::size_type;

private:
    Mutex mutable m_mutex;
    map_type m_map;
    Stats mutable m_stats;
    clock_type& m_clock;
    std::string const m_name;
    size_type m_target_size;
    clock_type::duration m_target_age;

public:
    /*使用指定的名称构造。

        @param size初始目标大小。
        @param age初始到期时间。
    **/

    KeyCache (std::string const& name, clock_type& clock,
        beast::insight::Collector::ptr const& collector, size_type target_size = 0,
            std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_stats (name,
            std::bind (&KeyCache::collect_metrics, this),
                collector)
        , m_clock (clock)
        , m_name (name)
        , m_target_size (target_size)
        , m_target_age (expiration)
    {
    }

//vfalc todo在此处使用转发构造函数调用
    KeyCache (std::string const& name, clock_type& clock,
        size_type target_size = 0,
        std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_stats (name,
            std::bind (&KeyCache::collect_metrics, this),
                beast::insight::NullCollector::New ())
        , m_clock (clock)
        , m_name (name)
        , m_target_size (target_size)
        , m_target_age (expiration)
    {
    }

//——————————————————————————————————————————————————————————————

    /*检索此对象的名称。*/
    std::string const& name () const
    {
        return m_name;
    }

    /*返回与缓存关联的时钟。*/
    clock_type& clock ()
    {
        return m_clock;
    }

    /*返回容器中的项数。*/
    size_type size () const
    {
        lock_guard lock (m_mutex);
        return m_map.size ();
    }

    /*清空缓存*/
    void clear ()
    {
        lock_guard lock (m_mutex);
        m_map.clear ();
    }

    void reset ()
    {
        lock_guard lock(m_mutex);
        m_map.clear();
        m_stats.hits = 0;
        m_stats.misses = 0;
    }

    void setTargetSize (size_type s)
    {
        lock_guard lock (m_mutex);
        m_target_size = s;
    }

    void setTargetAge (std::chrono::seconds s)
    {
        lock_guard lock (m_mutex);
        m_target_age = s;
    }

    /*如果找到键，则返回“true”。
        不更新上次访问时间。
    **/

    template <class KeyComparable>
    bool exists (KeyComparable const& key) const
    {
        lock_guard lock (m_mutex);
        typename map_type::const_iterator const iter (m_map.find (key));
        if (iter != m_map.end ())
        {
            ++m_stats.hits;
            return true;
        }
        ++m_stats.misses;
        return false;
    }

    /*插入指定的键。
        最后一次访问时间在所有情况下都会刷新。
        @return`true`如果密钥是新插入的。
    **/

    bool insert (Key const& key)
    {
        lock_guard lock (m_mutex);
        clock_type::time_point const now (m_clock.now ());
        std::pair <iterator, bool> result (m_map.emplace (
            std::piecewise_construct, std::forward_as_tuple (key),
                std::forward_as_tuple (now)));
        if (! result.second)
        {
            result.first->second.last_access = now;
            return false;
        }
        return true;
    }

    /*刷新密钥上的上次访问时间（如果存在）。
        @如果找到了键，返回'true'。
    **/

    template <class KeyComparable>
    bool touch_if_exists (KeyComparable const& key)
    {
        lock_guard lock (m_mutex);
        iterator const iter (m_map.find (key));
        if (iter == m_map.end ())
        {
            ++m_stats.misses;
            return false;
        }
        iter->second.last_access = m_clock.now ();
        ++m_stats.hits;
        return true;
    }

    /*删除指定的缓存项。
        @param键要删除的键。
        @如果找不到键，返回'false'。
    **/

    bool erase (key_type const& key)
    {
        lock_guard lock (m_mutex);
        if (m_map.erase (key) > 0)
        {
            ++m_stats.hits;
            return true;
        }
        ++m_stats.misses;
        return false;
    }

    /*从缓存中删除过时的条目。*/
    void sweep ()
    {
        clock_type::time_point const now (m_clock.now ());
        clock_type::time_point when_expire;

        lock_guard lock (m_mutex);

        if (m_target_size == 0 ||
            (m_map.size () <= m_target_size))
        {
            when_expire = now - m_target_age;
        }
        else
        {
            when_expire = now -
                m_target_age * m_target_size / m_map.size();

            clock_type::duration const minimumAge (
                std::chrono::seconds (1));
            if (when_expire > (now - minimumAge))
                when_expire = now - minimumAge;
        }

        iterator it = m_map.begin ();

        while (it != m_map.end ())
        {
            if (it->second.last_access > now)
            {
                it->second.last_access = now;
                ++it;
            }
            else if (it->second.last_access <= when_expire)
            {
                it = m_map.erase (it);
            }
            else
            {
                ++it;
            }
        }
    }

private:
    void collect_metrics ()
    {
        m_stats.size.set (size ());

        {
            beast::insight::Gauge::value_type hit_rate (0);
            {
                lock_guard lock (m_mutex);
                auto const total (m_stats.hits + m_stats.misses);
                if (total != 0)
                    hit_rate = (m_stats.hits * 100) / total;
            }
            m_stats.hit_rate.set (hit_rate);
        }
    }
};

}

#endif
