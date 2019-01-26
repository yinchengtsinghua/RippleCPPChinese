
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

#ifndef RIPPLE_PEERFINDER_BOOTCACHE_H_INCLUDED
#define RIPPLE_PEERFINDER_BOOTCACHE_H_INCLUDED

#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Store.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/bimap/unordered_set_of.hpp>
#include <boost/iterator/transform_iterator.hpp>

namespace ripple {
namespace PeerFinder {

/*存储用于获取初始连接的IP地址。

    这是附加传出时查询的缓存之一
    需要连接。连同地址，每个条目都有这个
    其他元数据：

    原子价
        表示成功数的有符号整数。
        连续连接尝试（如果为正数），以及
        连续连接尝试失败（如果为负）。

    从启动缓存中选择地址以
    建立传出连接时，地址按降序排列
    正常运行时间高的顺序，以价为纽带。
**/

class Bootcache
{
private:
    class Entry
    {
    public:
        Entry (int valence)
            : m_valence (valence)
        {
        }

        int& valence ()
        {
            return m_valence;
        }

        int valence () const
        {
            return m_valence;
        }

        friend bool operator< (Entry const& lhs, Entry const& rhs)
        {
            if (lhs.valence() > rhs.valence())
                return true;
            return false;
        }

    private:
        int m_valence;
    };

    using left_t = boost::bimaps::unordered_set_of <beast::IP::Endpoint>;
    using right_t = boost::bimaps::multiset_of <Entry>;
    using map_type = boost::bimap <left_t, right_t>;
    using value_type = map_type::value_type;

    struct Transform
#ifdef _LIBCPP_VERSION
        : std::unary_function<
              map_type::right_map::const_iterator::value_type const&,
              beast::IP::Endpoint const&>
#endif
    {
#ifndef _LIBCPP_VERSION
        using first_argument_type = map_type::right_map::const_iterator::value_type const&;
        using result_type = beast::IP::Endpoint const&;
#endif

        explicit Transform() = default;

        beast::IP::Endpoint const& operator() (
            map_type::right_map::
                const_iterator::value_type const& v) const
        {
            return v.get_left();
        }
    };

private:
    map_type m_map;

    Store& m_store;
    clock_type& m_clock;
    beast::Journal m_journal;

//之后我们可以再次更新数据库
    clock_type::time_point m_whenUpdate;

//需要数据库更新时设置为true
    bool m_needsUpdate;

public:
    static constexpr int staticValence = 32;

    using iterator = boost::transform_iterator <Transform,
        map_type::right_map::const_iterator>;

    using const_iterator = iterator;

    Bootcache (
        Store& store,
        clock_type& clock,
        beast::Journal journal);

    ~Bootcache ();

    /*如果缓存为空，则返回“true”。*/
    bool empty() const;

    /*返回缓存中的条目数。*/
    map_type::size_type size() const;

    /*以递减价遍历的端点迭代器。*/
    /*@ {*/
    const_iterator begin() const;
    const_iterator cbegin() const;
    const_iterator end() const;
    const_iterator cend() const;
    void clear();
    /*@ }*/

    /*将持久化数据从存储区加载到容器中。*/
    void load ();

    /*将新学习的地址添加到缓存中。*/
    bool insert (beast::IP::Endpoint const& endpoint);

    /*将静态配置的地址添加到缓存中。*/
    bool insertStatic (beast::IP::Endpoint const& endpoint);

    /*当出站连接握手完成时调用。*/
    void on_success (beast::IP::Endpoint const& endpoint);

    /*当出站连接尝试握手失败时调用。*/
    void on_failure (beast::IP::Endpoint const& endpoint);

    /*在计时器上将缓存存储在持久数据库中。*/
    void periodicActivity ();

    /*将缓存状态写入属性流。*/
    void onWrite (beast::PropertyStream::Map& map);

private:
    void prune ();
    void update ();
    void checkUpdate ();
    void flagForUpdate ();
};

}
}

#endif
