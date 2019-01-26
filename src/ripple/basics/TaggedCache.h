
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

#ifndef RIPPLE_BASICS_TAGGEDCACHE_H_INCLUDED
#define RIPPLE_BASICS_TAGGEDCACHE_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/insight/Insight.h>
#include <functional>
#include <mutex>
#include <vector>

namespace ripple {

//vvalco注释已弃用
struct TaggedCacheLog;

/*映射/缓存组合。
    这个类实现一个缓存和一个映射。缓存使对象保持活动状态
    在地图上。映射允许引用对象的多个代码路径
    使用相同的标记来获取相同的实际对象。

    只要数据在缓存中，它就会留在内存中。
    如果在从缓存中弹出后仍保留在内存中，
    地图会跟踪它。

    @note调用方不能修改缓存中存储的数据对象
          除非它们对所有缓存操作拥有自己的锁。
**/

//vfalc todo了解如何通过分配器
template <
    class Key,
    class T,
    class Hash = hardened_hash <>,
    class KeyEqual = std::equal_to <Key>,
//class allocator=std:：allocator<std:：pair<key const，entry>>，
    class Mutex = std::recursive_mutex
>
class TaggedCache
{
public:
    using mutex_type = Mutex;
//vfalc已弃用。调用方只能使用std:：unique_lock<type>
    using ScopedLockType = std::unique_lock <mutex_type>;
    using lock_guard = std::lock_guard <mutex_type>;
    using key_type = Key;
    using mapped_type = T;
//vfalc todo使用std:：shared_ptr，std:：weak_ptr
    using weak_mapped_ptr = std::weak_ptr <mapped_type>;
    using mapped_ptr = std::shared_ptr <mapped_type>;
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

public:
    TaggedCache (std::string const& name, int size,
        clock_type::duration expiration, clock_type& clock, beast::Journal journal,
            beast::insight::Collector::ptr const& collector = beast::insight::NullCollector::New ())
        : m_journal (journal)
        , m_clock (clock)
        , m_stats (name,
            std::bind (&TaggedCache::collect_metrics, this),
                collector)
        , m_name (name)
        , m_target_size (size)
        , m_target_age (expiration)
        , m_cache_count (0)
        , m_hits (0)
        , m_misses (0)
    {
    }

public:
    /*返回与缓存关联的时钟。*/
    clock_type& clock ()
    {
        return m_clock;
    }

    int getTargetSize () const
    {
        lock_guard lock (m_mutex);
        return m_target_size;
    }

    void setTargetSize (int s)
    {
        lock_guard lock (m_mutex);
        m_target_size = s;

        if (s > 0)
            m_cache.rehash (static_cast<std::size_t> ((s + (s >> 2)) / m_cache.max_load_factor () + 1));

        JLOG(m_journal.debug()) <<
            m_name << " target size set to " << s;
    }

    clock_type::duration getTargetAge () const
    {
        lock_guard lock (m_mutex);
        return m_target_age;
    }

    void setTargetAge (clock_type::duration s)
    {
        lock_guard lock (m_mutex);
        m_target_age = s;
        JLOG(m_journal.debug()) <<
            m_name << " target age set to " << m_target_age.count();
    }

    int getCacheSize () const
    {
        lock_guard lock (m_mutex);
        return m_cache_count;
    }

    int getTrackSize () const
    {
        lock_guard lock (m_mutex);
        return m_cache.size ();
    }

    float getHitRate ()
    {
        lock_guard lock (m_mutex);
        auto const total = static_cast<float> (m_hits + m_misses);
        return m_hits * (100.0f / std::max (1.0f, total));
    }

    void clear ()
    {
        lock_guard lock (m_mutex);
        m_cache.clear ();
        m_cache_count = 0;
    }

    void reset ()
    {
        lock_guard lock (m_mutex);
        m_cache.clear();
        m_cache_count = 0;
        m_hits = 0;
        m_misses = 0;
    }

    void sweep ()
    {
        int cacheRemovals = 0;
        int mapRemovals = 0;
        int cc = 0;

//保留对我们扫描的所有内容的引用
//这样我们就可以在锁外摧毁它们。
//
        std::vector <mapped_ptr> stuffToSweep;

        {
            clock_type::time_point const now (m_clock.now());
            clock_type::time_point when_expire;

            lock_guard lock (m_mutex);

            if (m_target_size == 0 ||
                (static_cast<int> (m_cache.size ()) <= m_target_size))
            {
                when_expire = now - m_target_age;
            }
            else
            {
                when_expire = now - m_target_age*m_target_size/m_cache.size();

                clock_type::duration const minimumAge (
                    std::chrono::seconds (1));
                if (when_expire > (now - minimumAge))
                    when_expire = now - minimumAge;

                JLOG(m_journal.trace()) <<
                    m_name << " is growing fast " << m_cache.size () << " of " << m_target_size <<
                        " aging at " << (now - when_expire).count() << " of " << m_target_age.count();
            }

            stuffToSweep.reserve (m_cache.size ());

            cache_iterator cit = m_cache.begin ();

            while (cit != m_cache.end ())
            {
                if (cit->second.isWeak ())
                {
//虚弱的
                    if (cit->second.isExpired ())
                    {
                        ++mapRemovals;
                        cit = m_cache.erase (cit);
                    }
                    else
                    {
                        ++cit;
                    }
                }
                else if (cit->second.last_access <= when_expire)
                {
//强，过期
                    --m_cache_count;
                    ++cacheRemovals;
                    if (cit->second.ptr.unique ())
                    {
                        stuffToSweep.push_back (cit->second.ptr);
                        ++mapRemovals;
                        cit = m_cache.erase (cit);
                    }
                    else
                    {
//仍然缓存不足
                        cit->second.ptr.reset ();
                        ++cit;
                    }
                }
                else
                {
//强，未过期
                    ++cc;
                    ++cit;
                }
            }
        }

        if (mapRemovals || cacheRemovals)
        {
            JLOG(m_journal.trace()) <<
                m_name << ": cache = " << m_cache.size () <<
                "-" << cacheRemovals << ", map-=" << mapRemovals;
        }

//此时，填充器排水将超出锁外的范围。
//并减少每个强指针上的引用计数。
    }

    bool del (const key_type& key, bool valid)
    {
//从缓存中删除，如果！有效，也从映射中删除。如果从缓存中删除，则返回true
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
            return false;

        Entry& entry = cit->second;

        bool ret = false;

        if (entry.isCached ())
        {
            --m_cache_count;
            entry.ptr.reset ();
            ret = true;
        }

        if (!valid || entry.isExpired ())
            m_cache.erase (cit);

        return ret;
    }

    /*将别名对象替换为原始对象。

        由于并发性，两个单独的对象
        同样的内容和指代同样独特的“事物”存在。
        此例程消除重复并执行替换
        如果需要，在调用方共享指针上。

        @param key与对象对应的键
        @param data指向对象对应数据的共享指针。
        @param replace`true` if`data`是对象的最新版本。

        @return`true`如果密钥已经存在。
    **/

    bool canonicalize (const key_type& key, std::shared_ptr<T>& data, bool replace = false)
    {
//返回规范值，必要时存储，在缓存中刷新
//返回值：真=我们已经有了数据
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
        {
            m_cache.emplace (std::piecewise_construct,
                std::forward_as_tuple(key),
                std::forward_as_tuple(m_clock.now(), data));
            ++m_cache_count;
            return false;
        }

        Entry& entry = cit->second;
        entry.touch (m_clock.now());

        if (entry.isCached ())
        {
            if (replace)
            {
                entry.ptr = data;
                entry.weak_ptr = data;
            }
            else
            {
                data = entry.ptr;
            }

            return true;
        }

        mapped_ptr cachedData = entry.lock ();

        if (cachedData)
        {
            if (replace)
            {
                entry.ptr = data;
                entry.weak_ptr = data;
            }
            else
            {
                entry.ptr = cachedData;
                data = cachedData;
            }

            ++m_cache_count;
            return true;
        }

        entry.ptr = data;
        entry.weak_ptr = data;
        ++m_cache_count;

        return false;
    }

    std::shared_ptr<T> fetch (const key_type& key)
    {
//获取存储数据对象的共享指针
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit == m_cache.end ())
        {
            ++m_misses;
            return mapped_ptr ();
        }

        Entry& entry = cit->second;
        entry.touch (m_clock.now());

        if (entry.isCached ())
        {
            ++m_hits;
            return entry.ptr;
        }

        entry.ptr = entry.lock ();

        if (entry.isCached ())
        {
//与缓存大小无关，因此不算作命中
            ++m_cache_count;
            return entry.ptr;
        }

        m_cache.erase (cit);
        ++m_misses;
        return mapped_ptr ();
    }

    /*将元件插入容器。
        如果密钥已经存在，则不会发生任何事情。
        @如果插入了元素，返回'true'
    **/

    bool insert (key_type const& key, T const& value)
    {
        mapped_ptr p (std::make_shared <T> (
            std::cref (value)));
        return canonicalize (key, p);
    }

//vfalc note似乎返回
//输出参数'data'。这可能很贵。
//也许它应该像标准容器那样工作
//只需返回一个迭代器。
//
    bool retrieve (const key_type& key, T& data)
    {
//检索存储数据的值
        mapped_ptr entry = fetch (key);

        if (!entry)
            return false;

        data = *entry;
        return true;
    }

    /*刷新密钥的过期时间。

        @param key要刷新的键。
        @return`true`如果找到了键，并且缓存了对象。
    **/

    bool refreshIfPresent (const key_type& key)
    {
        bool found = false;

//如果存在，则使缓存中成为当前状态
        lock_guard lock (m_mutex);

        cache_iterator cit = m_cache.find (key);

        if (cit != m_cache.end ())
        {
            Entry& entry = cit->second;

            if (! entry.isCached ())
            {
//由弱变强。
                entry.ptr = entry.lock ();

                if (entry.isCached ())
                {
//我们只是把对象放回缓存
                    ++m_cache_count;
                    entry.touch (m_clock.now());
                    found = true;
                }
                else
                {
//无法获得强指针，
//对象从缓存中掉出，请删除该项。
                    m_cache.erase (cit);
                }
            }
            else
            {
//它已缓存，因此请更新计时器
                entry.touch (m_clock.now());
                found = true;
            }
        }
        else
        {
//不在场
        }

        return found;
    }

    mutex_type& peekMutex ()
    {
        return m_mutex;
    }

    std::vector <key_type> getKeys () const
    {
        std::vector <key_type> v;

        {
            lock_guard lock (m_mutex);
            v.reserve (m_cache.size());
            for (auto const& _ : m_cache)
                v.push_back (_.first);
        }

        return v;
    }

private:
    void collect_metrics ()
    {
        m_stats.size.set (getCacheSize ());

        {
            beast::insight::Gauge::value_type hit_rate (0);
            {
                lock_guard lock (m_mutex);
                auto const total (m_hits + m_misses);
                if (total != 0)
                    hit_rate = (m_hits * 100) / total;
            }
            m_stats.hit_rate.set (hit_rate);
        }
    }

private:
    struct Stats
    {
        template <class Handler>
        Stats (std::string const& prefix, Handler const& handler,
            beast::insight::Collector::ptr const& collector)
            : hook (collector->make_hook (handler))
            , size (collector->make_gauge (prefix, "size"))
            , hit_rate (collector->make_gauge (prefix, "hit_rate"))
            { }

        beast::insight::Hook hook;
        beast::insight::Gauge size;
        beast::insight::Gauge hit_rate;
    };

    class Entry
    {
    public:
        mapped_ptr ptr;
        weak_mapped_ptr weak_ptr;
        clock_type::time_point last_access;

        Entry (clock_type::time_point const& last_access_,
            mapped_ptr const& ptr_)
            : ptr (ptr_)
            , weak_ptr (ptr_)
            , last_access (last_access_)
        {
        }

        bool isWeak () const { return ptr == nullptr; }
        bool isCached () const { return ptr != nullptr; }
        bool isExpired () const { return weak_ptr.expired (); }
        mapped_ptr lock () { return weak_ptr.lock (); }
        void touch (clock_type::time_point const& now) { last_access = now; }
    };

    using cache_type = hardened_hash_map <key_type, Entry, Hash, KeyEqual>;
    using cache_iterator = typename cache_type::iterator;

    beast::Journal m_journal;
    clock_type& m_clock;
    Stats m_stats;

    mutex_type mutable m_mutex;

//用于日志记录
    std::string m_name;

//所需的缓存条目数（0=忽略）
    int m_target_size;

//所需的最大缓存期限
    clock_type::duration m_target_age;

//缓存的项目数
    int m_cache_count;
cache_type m_cache;  //对最近的对象有很强的引用
    std::uint64_t m_hits;
    std::uint64_t m_misses;
};

}

#endif
