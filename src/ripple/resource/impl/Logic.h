
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

#ifndef RIPPLE_RESOURCE_LOGIC_H_INCLUDED
#define RIPPLE_RESOURCE_LOGIC_H_INCLUDED

#include <ripple/resource/Fees.h>
#include <ripple/resource/Gossip.h>
#include <ripple/resource/impl/Import.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/insight/Insight.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <cassert>
#include <mutex>

namespace ripple {
namespace Resource {

class Logic
{
private:
    using clock_type = Stopwatch;
    using Imports = hash_map <std::string, Import>;
    using Table = hash_map <Key, Entry, Key::hasher, Key::key_equal>;
    using EntryIntrusiveList = beast::List <Entry>;

    struct Stats
    {
        Stats (beast::insight::Collector::ptr const& collector)
        {
            warn = collector->make_meter ("warn");
            drop = collector->make_meter ("drop");
        }

        beast::insight::Meter warn;
        beast::insight::Meter drop;
    };

    Stats m_stats;
    Stopwatch& m_clock;
    beast::Journal m_journal;

    std::recursive_mutex lock_;

//所有条目表
    Table table_;

//由于以下是侵入性列表，因此给定条目可能位于
//至多在给定时刻列出。必须将条目从
//一个列表，然后将其放入另一个列表。

//所有活动入站条目的列表
    EntryIntrusiveList inbound_;

//所有活动出站条目的列表
    EntryIntrusiveList outbound_;

//所有活动管理条目的列表
    EntryIntrusiveList admin_;

//所有不符合项列表
    EntryIntrusiveList inactive_;

//所有导入的八卦数据
    Imports importTable_;

//——————————————————————————————————————————————————————————————
public:

    Logic (beast::insight::Collector::ptr const& collector,
        clock_type& clock, beast::Journal journal)
        : m_stats (collector)
        , m_clock (clock)
        , m_journal (journal)
    {
    }

    ~Logic ()
    {
//这些必须在逻辑被破坏之前被清除。
//因为它们的析构函数调用了类。
//这里也有订购事宜，导入表必须
//在消费表之前销毁。
//
        importTable_.clear();
        table_.clear();
    }

    Consumer newInboundEndpoint (beast::IP::Endpoint const& address)
    {
        Entry* entry (nullptr);

        {
            std::lock_guard<std::recursive_mutex> _(lock_);
            auto result =
                table_.emplace (std::piecewise_construct,
std::make_tuple (kindInbound, address.at_port (0)), //钥匙
std::make_tuple (m_clock.now()));                   //条目

            entry = &result.first->second;
            entry->key = &result.first->first;
            ++entry->refcount;
            if (entry->refcount == 1)
            {
                if (! result.second)
                {
                    inactive_.erase (
                        inactive_.iterator_to (*entry));
                }
                inbound_.push_back (*entry);
            }
        }

        JLOG(m_journal.debug()) <<
            "New inbound endpoint " << *entry;

        return Consumer (*this, *entry);
    }

    Consumer newOutboundEndpoint (beast::IP::Endpoint const& address)
    {
        Entry* entry (nullptr);

        {
            std::lock_guard<std::recursive_mutex> _(lock_);
            auto result =
                table_.emplace (std::piecewise_construct,
std::make_tuple (kindOutbound, address),            //钥匙
std::make_tuple (m_clock.now()));                   //条目

            entry = &result.first->second;
            entry->key = &result.first->first;
            ++entry->refcount;
            if (entry->refcount == 1)
            {
                if (! result.second)
                    inactive_.erase (
                        inactive_.iterator_to (*entry));
                outbound_.push_back (*entry);
            }
        }

        JLOG(m_journal.debug()) <<
            "New outbound endpoint " << *entry;

        return Consumer (*this, *entry);
    }

    /*
     *创建不应应用资源限制的终结点。其他
     *执行某些RPC调用的权限等限制可能是
     *启用。
     **/

    Consumer newUnlimitedEndpoint (std::string const& name)
    {
        Entry* entry (nullptr);

        {
            std::lock_guard<std::recursive_mutex> _(lock_);
            auto result =
                table_.emplace (std::piecewise_construct,
std::make_tuple (name),                             //钥匙
std::make_tuple (m_clock.now()));                   //条目

            entry = &result.first->second;
            entry->key = &result.first->first;
            ++entry->refcount;
            if (entry->refcount == 1)
            {
                if (! result.second)
                    inactive_.erase (
                        inactive_.iterator_to (*entry));
                admin_.push_back (*entry);
            }
        }

        JLOG(m_journal.debug()) <<
            "New unlimited endpoint " << *entry;

        return Consumer (*this, *entry);
    }

    Json::Value getJson ()
    {
        return getJson (warningThreshold);
    }

    /*返回json:：objectValue。*/
    Json::Value getJson (int threshold)
    {
        clock_type::time_point const now (m_clock.now());

        Json::Value ret (Json::objectValue);
        std::lock_guard<std::recursive_mutex> _(lock_);

        for (auto& inboundEntry : inbound_)
        {
            int localBalance = inboundEntry.local_balance.value (now);
            if ((localBalance + inboundEntry.remote_balance) >= threshold)
            {
                Json::Value& entry = (ret[inboundEntry.to_string()] = Json::objectValue);
                entry[jss::local] = localBalance;
                entry[jss::remote] = inboundEntry.remote_balance;
                entry[jss::type] = "inbound";
            }

        }
        for (auto& outboundEntry : outbound_)
        {
            int localBalance = outboundEntry.local_balance.value (now);
            if ((localBalance + outboundEntry.remote_balance) >= threshold)
            {
                Json::Value& entry = (ret[outboundEntry.to_string()] = Json::objectValue);
                entry[jss::local] = localBalance;
                entry[jss::remote] = outboundEntry.remote_balance;
                entry[jss::type] = "outbound";
            }

        }
        for (auto& adminEntry : admin_)
        {
            int localBalance = adminEntry.local_balance.value (now);
            if ((localBalance + adminEntry.remote_balance) >= threshold)
            {
                Json::Value& entry = (ret[adminEntry.to_string()] = Json::objectValue);
                entry[jss::local] = localBalance;
                entry[jss::remote] = adminEntry.remote_balance;
                entry[jss::type] = "admin";
            }

        }

        return ret;
    }

    Gossip exportConsumers ()
    {
        clock_type::time_point const now (m_clock.now());

        Gossip gossip;
        std::lock_guard<std::recursive_mutex> _(lock_);

        gossip.items.reserve (inbound_.size());

        for (auto& inboundEntry : inbound_)
        {
            Gossip::Item item;
            item.balance = inboundEntry.local_balance.value (now);
            if (item.balance >= minimumGossipBalance)
            {
                item.address = inboundEntry.key->address;
                gossip.items.push_back (item);
            }
        }

        return gossip;
    }

//——————————————————————————————————————————————————————————————

    void importConsumers (std::string const& origin, Gossip const& gossip)
    {
        auto const elapsed = m_clock.now();
        {
            std::lock_guard<std::recursive_mutex> _(lock_);
            auto result =
                importTable_.emplace (std::piecewise_construct,
std::make_tuple(origin),                  //钥匙
std::make_tuple(m_clock.now().time_since_epoch().count()));     //进口

            if (result.second)
            {
//这是新进口
                Import& next (result.first->second);
                next.whenExpires = elapsed + gossipExpirationSeconds;
                next.items.reserve (gossip.items.size());

                for (auto const& gossipItem : gossip.items)
                {
                    Import::Item item;
                    item.balance = gossipItem.balance;
                    item.consumer = newInboundEndpoint (gossipItem.address);
                    item.consumer.entry().remote_balance += item.balance;
                    next.items.push_back (item);
                }
            }
            else
            {
//以前的导入存在，因此添加新的远程
//余额，然后扣除旧的远程余额。

                Import next;
                next.whenExpires = elapsed + gossipExpirationSeconds;
                next.items.reserve (gossip.items.size());
                for (auto const& gossipItem : gossip.items)
                {
                    Import::Item item;
                    item.balance = gossipItem.balance;
                    item.consumer = newInboundEndpoint (gossipItem.address);
                    item.consumer.entry().remote_balance += item.balance;
                    next.items.push_back (item);
                }

                Import& prev (result.first->second);
                for (auto& item : prev.items)
                {
                    item.consumer.entry().remote_balance -= item.balance;
                }

                std::swap (next, prev);
            }
        }
    }

//——————————————————————————————————————————————————————————————

//定期调用以使条目过期并整理表。
//
    void periodicActivity ()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        auto const elapsed = m_clock.now();

        for (auto iter (inactive_.begin()); iter != inactive_.end();)
        {
            if (iter->whenExpires <= elapsed)
            {
                JLOG(m_journal.debug()) << "Expired " << *iter;
                auto table_iter =
                    table_.find (*iter->key);
                ++iter;
                erase (table_iter);
            }
            else
            {
                break;
            }
        }

        auto iter = importTable_.begin();
        while (iter != importTable_.end())
        {
            Import& import (iter->second);
            if (iter->second.whenExpires <= elapsed)
            {
                for (auto item_iter (import.items.begin());
                    item_iter != import.items.end(); ++item_iter)
                {
                    item_iter->consumer.entry().remote_balance -= item_iter->balance;
                }

                iter = importTable_.erase (iter);
            }
            else
                ++iter;
        }
    }

//——————————————————————————————————————————————————————————————

//基于余额和阈值返回处置
    static Disposition disposition (int balance)
    {
        if (balance >= dropThreshold)
            return Disposition::drop;

        if (balance >= warningThreshold)
            return Disposition::warn;

        return Disposition::ok;
    }

    void erase (Table::iterator iter)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        Entry& entry (iter->second);
        assert (entry.refcount == 0);
        inactive_.erase (
            inactive_.iterator_to (entry));
        table_.erase (iter);
    }

    void acquire (Entry& entry)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        ++entry.refcount;
    }

    void release (Entry& entry)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        if (--entry.refcount == 0)
        {
            JLOG(m_journal.debug()) <<
                "Inactive " << entry;

            switch (entry.key->kind)
            {
            case kindInbound:
                inbound_.erase (
                    inbound_.iterator_to (entry));
                break;
            case kindOutbound:
                outbound_.erase (
                    outbound_.iterator_to (entry));
                break;
            case kindUnlimited:
                admin_.erase (
                    admin_.iterator_to (entry));
                break;
            default:
                assert(false);
                break;
            }
            inactive_.push_back (entry);
            entry.whenExpires = m_clock.now() + secondsUntilExpiration;
        }
    }

    Disposition charge (Entry& entry, Charge const& fee)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        clock_type::time_point const now (m_clock.now());
        int const balance (entry.add (fee.cost(), now));
        JLOG(m_journal.trace()) <<
            "Charging " << entry << " for " << fee;
        return disposition (balance);
    }

    bool warn (Entry& entry)
    {
        if (entry.isUnlimited())
            return false;

        std::lock_guard<std::recursive_mutex> _(lock_);
        bool notify (false);
        auto const elapsed = m_clock.now();
        if (entry.balance (m_clock.now()) >= warningThreshold &&
            elapsed != entry.lastWarningTime)
        {
            charge (entry, feeWarning);
            notify = true;
            entry.lastWarningTime = elapsed;
        }
        if (notify)
        {
            JLOG(m_journal.info()) << "Load warning: " << entry;
            ++m_stats.warn;
        }
        return notify;
    }

    bool disconnect (Entry& entry)
    {
        if (entry.isUnlimited())
            return false;

        std::lock_guard<std::recursive_mutex> _(lock_);
        bool drop (false);
        clock_type::time_point const now (m_clock.now());
        int const balance (entry.balance (now));
        if (balance >= dropThreshold)
        {
            JLOG(m_journal.warn()) <<
                "Consumer entry " << entry <<
                " dropped with balance " << balance <<
                " at or above drop threshold " << dropThreshold;

//此时添加feedrop将保持断开的连接
//从重新连接至少一段时间后
//下降。
            charge (entry, feeDrop);
            ++m_stats.drop;
            drop = true;
        }
        return drop;
    }

    int balance (Entry& entry)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        return entry.balance (m_clock.now());
    }

//——————————————————————————————————————————————————————————————

    void writeList (
        clock_type::time_point const now,
            beast::PropertyStream::Set& items,
                EntryIntrusiveList& list)
    {
        for (auto& entry : list)
        {
            beast::PropertyStream::Map item (items);
            if (entry.refcount != 0)
                item ["count"] = entry.refcount;
            item ["name"] = entry.to_string();
            item ["balance"] = entry.balance(now);
            if (entry.remote_balance != 0)
                item ["remote_balance"] = entry.remote_balance;
        }
    }

    void onWrite (beast::PropertyStream::Map& map)
    {
        clock_type::time_point const now (m_clock.now());

        std::lock_guard<std::recursive_mutex> _(lock_);

        {
            beast::PropertyStream::Set s ("inbound", map);
            writeList (now, s, inbound_);
        }

        {
            beast::PropertyStream::Set s ("outbound", map);
            writeList (now, s, outbound_);
        }

        {
            beast::PropertyStream::Set s ("admin", map);
            writeList (now, s, admin_);
        }

        {
            beast::PropertyStream::Set s ("inactive", map);
            writeList (now, s, inactive_);
        }
    }
};

}
}

#endif
