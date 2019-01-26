
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

#ifndef RIPPLE_SHAMAP_FULLBELOWCACHE_H_INCLUDED
#define RIPPLE_SHAMAP_FULLBELOWCACHE_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/KeyCache.h>
#include <ripple/beast/insight/Collector.h>
#include <atomic>
#include <string>

namespace ripple {

namespace detail {

/*记住哪些树键驻留了所有子代。
    这优化了获取完整树的过程。
**/

template <class Key>
class BasicFullBelowCache
{
private:
    using CacheType = KeyCache <Key>;

public:
    enum
    {
         defaultCacheTargetSize = 0
    };

    using key_type   = Key;
    using size_type  = typename CacheType::size_type;
    using clock_type = typename CacheType::clock_type;

    /*构造缓存。

        @param name诊断和统计报告的标签。
        @param collector用于报告统计信息的收集器。
        @param target size缓存目标大小。
        @param targetexpirationseconds项目的过期时间。
    **/

    BasicFullBelowCache (std::string const& name, clock_type& clock,
        beast::insight::Collector::ptr const& collector =
            beast::insight::NullCollector::New (),
        std::size_t target_size = defaultCacheTargetSize,
        std::chrono::seconds expiration = std::chrono::minutes{2})
        : m_cache (name, clock, collector, target_size,
            expiration)
        , m_gen (1)
    {
    }

    /*返回与缓存关联的时钟。*/
    clock_type& clock()
    {
        return m_cache.clock ();
    }

    /*返回缓存中的元素数。
        线程安全：
            从任何线程调用都是安全的。
    **/

    size_type size () const
    {
        return m_cache.size ();
    }

    /*删除过期的缓存项。
        线程安全：
            从任何线程调用都是安全的。
    **/

    void sweep ()
    {
        m_cache.sweep ();
    }

    /*刷新项目的上次访问时间（如果存在）。
        线程安全：
            从任何线程调用都是安全的。
        @param key要刷新的键。
        @如果键存在，返回'true'。
    **/

    bool touch_if_exists (key_type const& key)
    {
        return m_cache.touch_if_exists (key);
    }

    /*将密钥插入缓存。
        如果密钥已存在，则最后一次访问时间仍将
        振作起来。
        线程安全：
            从任何线程调用都是安全的。
        @param key要插入的键。
    **/

    void insert (key_type const& key)
    {
        m_cache.insert (key);
    }

    /*生成确定缓存项是否有效*/
    std::uint32_t getGeneration (void) const
    {
        return m_gen;
    }

    void clear ()
    {
        m_cache.clear ();
        ++m_gen;
    }

    void reset ()
    {
        m_cache.clear();
        m_gen  = 1;
    }

private:
    KeyCache <Key> m_cache;
    std::atomic <std::uint32_t> m_gen;
};

} //细节

using FullBelowCache = detail::BasicFullBelowCache <uint256>;

}

#endif
