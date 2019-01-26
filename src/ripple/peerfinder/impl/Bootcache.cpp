
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

#include <ripple/peerfinder/impl/Bootcache.h>
#include <ripple/peerfinder/impl/iosformat.h>
#include <ripple/peerfinder/impl/Tuning.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace PeerFinder {

Bootcache::Bootcache (
    Store& store,
    clock_type& clock,
    beast::Journal journal)
    : m_store (store)
    , m_clock (clock)
    , m_journal (journal)
    , m_whenUpdate (m_clock.now ())
    , m_needsUpdate (false)
{
}

Bootcache::~Bootcache ()
{
    update();
}

bool
Bootcache::empty() const
{
    return m_map.empty();
}

Bootcache::map_type::size_type
Bootcache::size() const
{
    return m_map.size();
}

Bootcache::const_iterator
Bootcache::begin() const
{
    return const_iterator (m_map.right.begin());
}

Bootcache::const_iterator
Bootcache::cbegin() const
{
    return const_iterator (m_map.right.begin());
}

Bootcache::const_iterator
Bootcache::end() const
{
    return const_iterator (m_map.right.end());
}

Bootcache::const_iterator
Bootcache::cend() const
{
    return const_iterator (m_map.right.end());
}

void
Bootcache::clear()
{
    m_map.clear();
    m_needsUpdate = true;
}

//——————————————————————————————————————————————————————————————

void
Bootcache::load ()
{
    clear();
    auto const n (m_store.load (
        [this](beast::IP::Endpoint const& endpoint, int valence)
        {
            auto const result (this->m_map.insert (
                value_type (endpoint, valence)));
            if (! result.second)
            {
                JLOG(this->m_journal.error())
                    << beast::leftw (18) <<
                    "Bootcache discard " << endpoint;
            }
        }));

    if (n > 0)
    {
        JLOG(m_journal.info()) << beast::leftw (18) <<
            "Bootcache loaded " << n <<
                ((n > 1) ? " addresses" : " address");
        prune ();
    }
}

bool
Bootcache::insert (beast::IP::Endpoint const& endpoint)
{
    auto const result (m_map.insert (
        value_type (endpoint, 0)));
    if (result.second)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Bootcache insert " << endpoint;
        prune ();
        flagForUpdate();
    }
    return result.second;
}

bool
Bootcache::insertStatic (beast::IP::Endpoint const& endpoint)
{
    auto result (m_map.insert (
        value_type (endpoint, staticValence)));

    if (! result.second && (result.first->right.valence() < staticValence))
    {
//现有条目的价太低，请替换它
        m_map.erase (result.first);
        result = m_map.insert (
            value_type (endpoint, staticValence));
    }

    if (result.second)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Bootcache insert " << endpoint;
        prune ();
        flagForUpdate();
    }
    return result.second;
}

void
Bootcache::on_success (beast::IP::Endpoint const& endpoint)
{
    auto result (m_map.insert (
        value_type (endpoint, 1)));
    if (result.second)
    {
        prune ();
    }
    else
    {
        Entry entry (result.first->right);
        if (entry.valence() < 0)
            entry.valence() = 0;
        ++entry.valence();
        m_map.erase (result.first);
        result = m_map.insert (
            value_type (endpoint, entry));
        assert (result.second);
    }
    Entry const& entry (result.first->right);
    JLOG(m_journal.info()) << beast::leftw (18) <<
        "Bootcache connect " << endpoint <<
        " with " << entry.valence() <<
        ((entry.valence() > 1) ? " successes" : " success");
    flagForUpdate();
}

void
Bootcache::on_failure (beast::IP::Endpoint const& endpoint)
{
    auto result (m_map.insert (
        value_type (endpoint, -1)));
    if (result.second)
    {
        prune();
    }
    else
    {
        Entry entry (result.first->right);
        if (entry.valence() > 0)
            entry.valence() = 0;
        --entry.valence();
        m_map.erase (result.first);
        result = m_map.insert (
            value_type (endpoint, entry));
        assert (result.second);
    }
    Entry const& entry (result.first->right);
    auto const n (std::abs (entry.valence()));
    JLOG(m_journal.debug()) << beast::leftw (18) <<
        "Bootcache failed " << endpoint <<
        " with " << n <<
        ((n > 1) ? " attempts" : " attempt");
    flagForUpdate();
}

void
Bootcache::periodicActivity ()
{
    checkUpdate();
}

//——————————————————————————————————————————————————————————————

void
Bootcache::onWrite (beast::PropertyStream::Map& map)
{
    beast::PropertyStream::Set entries ("entries", map);
    for (auto iter = m_map.right.begin(); iter != m_map.right.end(); ++iter)
    {
        beast::PropertyStream::Map entry (entries);
        entry["endpoint"] = iter->get_left().to_string();
        entry["valence"] = std::int32_t (iter->get_right().valence());
    }
}

//检查缓存大小，如果超出限制则进行修剪。
void
Bootcache::prune ()
{
    if (size() <= Tuning::bootcacheSize)
        return;

//计算要删除的数量
    auto count ((size() *
        Tuning::bootcachePrunePercent) / 100);
    decltype(count) pruned (0);

//向后工作，因为BIMAP无法处理
//非常好地使用反向迭代器擦除。
//
    for (auto iter (m_map.right.end());
        count-- > 0 && iter != m_map.right.begin(); ++pruned)
    {
        --iter;
        beast::IP::Endpoint const& endpoint (iter->get_left());
        Entry const& entry (iter->get_right());
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Bootcache pruned" << endpoint <<
            " at valence " << entry.valence();
        iter = m_map.right.erase (iter);
    }

    JLOG(m_journal.debug()) << beast::leftw (18) <<
        "Bootcache pruned " << pruned << " entries total";
}

//如果需要，使用当前条目集更新存储。
void
Bootcache::update ()
{
    if (! m_needsUpdate)
        return;
    std::vector <Store::Entry> list;
    list.reserve (m_map.size());
    for (auto const& e : m_map)
    {
        Store::Entry se;
        se.endpoint = e.get_left();
        se.valence = e.get_right().valence();
        list.push_back (se);
    }
    m_store.save (list);
//重置标志和冷却计时器
    m_needsUpdate = false;
    m_whenUpdate = m_clock.now() + Tuning::bootcacheCooldownTime;
}

//检查时钟，如果我们关闭冷却，则调用update。
void
Bootcache::checkUpdate ()
{
    if (m_needsUpdate && m_whenUpdate < m_clock.now())
        update ();
}

//当项的更改将影响存储时调用。
void
Bootcache::flagForUpdate ()
{
    m_needsUpdate = true;
    checkUpdate ();
}

}
}
