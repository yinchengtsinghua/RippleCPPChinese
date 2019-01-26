
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

#ifndef RIPPLE_PEERFINDER_LOGIC_H_INCLUDED
#define RIPPLE_PEERFINDER_LOGIC_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/random.h>
#include <ripple/basics/contract.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <ripple/beast/net/IPAddressConversion.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Bootcache.h>
#include <ripple/peerfinder/impl/Counts.h>
#include <ripple/peerfinder/impl/Fixed.h>
#include <ripple/peerfinder/impl/Handouts.h>
#include <ripple/peerfinder/impl/Livecache.h>
#include <ripple/peerfinder/impl/Reporting.h>
#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/impl/Source.h>
#include <ripple/peerfinder/impl/Store.h>
#include <ripple/peerfinder/impl/iosformat.h>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace ripple {
namespace PeerFinder {

/*维护插槽地址列表的逻辑。
    我们将它保存在一个单独的类中，这样它就可以被实例化。
    用于单元测试。
**/

template <class Checker>
class Logic
{
public:
//将远程端点映射到插槽。因为吃角子老虎机有一个
//在构建远程端点时，这将保存所有计数。
//
    using Slots = std::map<beast::IP::Endpoint,
        std::shared_ptr<SlotImp>>;

    beast::Journal m_journal;
    clock_type& m_clock;
    Store& m_store;
    Checker& m_checker;

    std::recursive_mutex lock_;

//是的，如果我们停下来。
    bool stopping_ = false;

//我们当前正在获取的源。
//这用于在程序退出时取消I/O。
    std::shared_ptr<Source> fetchSource_;

//配置设置
    Config config_;

//槽计数和其他汇总统计。
    Counts counts_;

//应始终连接的插槽列表
    std::map<beast::IP::Endpoint, Fixed> fixed_;

//来自mtendpoints消息的实时livecache
    Livecache <> livecache_;

//适合获取初始连接的地址的LiveCache
    Bootcache bootcache_;

//保留所有计数
    Slots slots_;

//我们连接的地址（但不是端口）。这包括
//传出连接尝试。请注意，此集合可以包含
//重复（因为端口未设置）
    std::multiset<beast::IP::Address> connectedAddresses_;

//属于活动对等方的公钥集
    std::set<PublicKey> keys_;

//作为回退查询的动态源列表
    std::vector<std::shared_ptr<Source>> m_sources;

    clock_type::time_point m_whenBroadcast;

    ConnectHandouts::Squelches m_squelches;

//——————————————————————————————————————————————————————————————

    Logic (clock_type& clock, Store& store,
            Checker& checker, beast::Journal journal)
        : m_journal (journal)
        , m_clock (clock)
        , m_store (store)
        , m_checker (checker)
        , livecache_ (m_clock, journal)
        , bootcache_ (store, m_clock, journal)
        , m_whenBroadcast (m_clock.now())
        , m_squelches (m_clock)
    {
        config ({});
    }

//从存储中加载持久状态信息
//
    void load ()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        bootcache_.load ();
    }

    /*停止逻辑。
        这将取消当前提取并设置停止标志
        设置为“true”，以防止进一步提取。
        线程安全：
            从任何线程调用都是安全的。
    **/

    void stop ()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        stopping_ = true;
        if (fetchSource_ != nullptr)
            fetchSource_->cancel ();
    }

//——————————————————————————————————————————————————————————————
//
//经理
//
//——————————————————————————————————————————————————————————————

    void
    config (Config const& c)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        config_ = c;
        counts_.onConfig (config_);
    }

    Config
    config()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        return config_;
    }

    void
    addFixedPeer (std::string const& name,
        beast::IP::Endpoint const& ep)
    {
        std::vector<beast::IP::Endpoint> v;
        v.push_back(ep);
        addFixedPeer (name, v);
    }

    void
    addFixedPeer (std::string const& name,
        std::vector <beast::IP::Endpoint> const& addresses)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        if (addresses.empty ())
        {
            JLOG(m_journal.info()) <<
                "Could not resolve fixed slot '" << name << "'";
            return;
        }

        for (auto const& remote_address : addresses)
        {
            if (remote_address.port () == 0)
            {
                Throw<std::runtime_error> ("Port not specified for address:" +
                    remote_address.to_string ());
            }

            auto result (fixed_.emplace (std::piecewise_construct,
                std::forward_as_tuple (remote_address),
                    std::make_tuple (std::ref (m_clock))));

            if (result.second)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic add fixed '" << name <<
                    "' at " << remote_address;
                return;
            }
        }
    }

//——————————————————————————————————————————————————————————————

//当检查器完成连接测试时调用
    void checkComplete (beast::IP::Endpoint const& remoteAddress,
        beast::IP::Endpoint const& checkedAddress,
            boost::system::error_code ec)
    {
        if (ec == boost::asio::error::operation_aborted)
            return;

        std::lock_guard<std::recursive_mutex> _(lock_);
        auto const iter (slots_.find (remoteAddress));
        if (iter == slots_.end())
        {
//在我们检查完之前，插槽断开了。
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic tested " << checkedAddress <<
                " but the connection was closed";
            return;
        }

        SlotImp& slot (*iter->second);
        slot.checked = true;
        slot.connectivityCheckInProgress = false;

        if (ec)
        {
//vfalc todo是否应根据错误重试？
            slot.canAccept = false;
            JLOG(m_journal.error()) << beast::leftw (18) <<
                "Logic testing " << iter->first << " with error, " <<
                ec.message();
            bootcache_.on_failure (checkedAddress);
            return;
        }

        slot.canAccept = true;
        slot.set_listening_port (checkedAddress.port ());
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic testing " << checkedAddress << " succeeded";
    }

//——————————————————————————————————————————————————————————————

    SlotImp::ptr new_inbound_slot (beast::IP::Endpoint const& local_endpoint,
        beast::IP::Endpoint const& remote_endpoint)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic accept" << remote_endpoint <<
            " on local " << local_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

//检查重复连接
        if (is_public (remote_endpoint))
        {
            auto const count = connectedAddresses_.count (
                remote_endpoint.address());
            if (count > config_.ipLimit)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic dropping inbound " << remote_endpoint <<
                    " because of ip limits.";
                return SlotImp::ptr();
            }
        }

//创建插槽
        SlotImp::ptr const slot (std::make_shared <SlotImp> (local_endpoint,
            remote_endpoint, fixed (remote_endpoint.address ()),
                m_clock));
//向表中添加槽
        auto const result (slots_.emplace (
            slot->remote_endpoint (), slot));
//远程地址不能已经存在
        assert (result.second);
//添加到连接的地址列表
        connectedAddresses_.emplace (remote_endpoint.address());

//更新计数
        counts_.add (*slot);

        return result.first->second;
    }

//无法检查自我连接，因为我们不知道本地终结点
    SlotImp::ptr
    new_outbound_slot (beast::IP::Endpoint const& remote_endpoint)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic connect " << remote_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

//检查重复连接
        if (slots_.find (remote_endpoint) !=
            slots_.end ())
        {
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic dropping " << remote_endpoint <<
                " as duplicate connect";
            return SlotImp::ptr ();
        }

//创建插槽
        SlotImp::ptr const slot (std::make_shared <SlotImp> (
            remote_endpoint, fixed (remote_endpoint), m_clock));

//向表中添加槽
        auto const result =
            slots_.emplace (slot->remote_endpoint (),
                slot);
//远程地址不能已经存在
        assert (result.second);

//添加到连接的地址列表
        connectedAddresses_.emplace (remote_endpoint.address());

//更新计数
        counts_.add (*slot);

        return result.first->second;
    }

    bool
    onConnected (SlotImp::ptr const& slot,
        beast::IP::Endpoint const& local_endpoint)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Logic connected" << slot->remote_endpoint () <<
            " on local " << local_endpoint;

        std::lock_guard<std::recursive_mutex> _(lock_);

//对象必须存在于我们的表中
        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());
//现在已经知道本地端点
        slot->local_endpoint (local_endpoint);

//按地址检查自我连接
        {
            auto const iter (slots_.find (local_endpoint));
            if (iter != slots_.end ())
            {
                assert (iter->second->local_endpoint ()
                        == slot->remote_endpoint ());
                JLOG(m_journal.warn()) << beast::leftw (18) <<
                    "Logic dropping " << slot->remote_endpoint () <<
                    " as self connect";
                return false;
            }
        }

//更新计数
        counts_.remove (*slot);
        slot->state (Slot::connected);
        counts_.add (*slot);
        return true;
    }

    Result
    activate (SlotImp::ptr const& slot,
        PublicKey const& key, bool cluster)
    {
        JLOG(m_journal.debug()) << beast::leftw (18) <<
            "Logic handshake " << slot->remote_endpoint () <<
            " with " << (cluster ? "clustered " : "") << "key " << key;

        std::lock_guard<std::recursive_mutex> _(lock_);

//对象必须存在于我们的表中
        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());
//必须接受或连接
        assert (slot->state() == Slot::accept ||
            slot->state() == Slot::connected);

//按键检查重复连接
        if (keys_.find (key) != keys_.end())
            return Result::duplicate;

//如果对等机属于集群，请更新插槽以反映这一点。
        counts_.remove (*slot);
        slot->cluster (cluster);
        counts_.add (*slot);

//看看我们有没有空位给这个槽
        if (! counts_.can_activate (*slot))
        {
            if (! slot->inbound())
                bootcache_.on_success (
                    slot->remote_endpoint());
            return Result::full;
        }

//在添加到地图之前设置关键点，否则我们可能
//删除密钥时稍后断言。
        slot->public_key (key);
        {
            auto const result = keys_.insert(key);
//公钥不能已经存在
            assert (result.second);
            (void)result.second;
        }

//更改状态和更新计数
        counts_.remove (*slot);
        slot->activate (m_clock.now ());
        counts_.add (*slot);

        if (! slot->inbound())
            bootcache_.on_success (
                slot->remote_endpoint());

//标记固定槽成功
        if (slot->fixed() && ! slot->inbound())
        {
            auto iter (fixed_.find (slot->remote_endpoint()));
            assert (iter != fixed_.end ());
            iter->second.success (m_clock.now ());
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic fixed " << slot->remote_endpoint () << " success";
        }

        return Result::success;
    }

    /*返回适合重定向的地址列表。
        这是一个遗留函数，应在
        HTTP握手，而不是通过tmendpoints。
    **/

    std::vector <Endpoint>
    redirect (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);
        RedirectHandouts h (slot);
        livecache_.hops.shuffle();
        handout (&h, (&h)+1,
            livecache_.hops.begin(),
                livecache_.hops.end());
        return std::move(h.list());
    }

    /*根据需要创建新的出站连接尝试。
        这实现了peerfinder的“出站连接策略”
    **/

//vfalc todo这应该将返回的地址添加到
//一旦建立了列表，就可以一次完成列表的压缩，
//而不是让每个模块都添加到静噪列表中。
    std::vector <beast::IP::Endpoint>
    autoconnect()
    {
        std::vector <beast::IP::Endpoint> const none;

        std::lock_guard<std::recursive_mutex> _(lock_);

//计算还要进行多少出站尝试
//
        auto needed (counts_.attempts_needed ());
        if (needed == 0)
            return none;

        ConnectHandouts h (needed, m_squelches);

//确保我们没有连接到已经连接的条目。
        for (auto const& s : slots_)
        {
            auto const result (m_squelches.insert (
                s.second->remote_endpoint().address()));
            if (! result.second)
                m_squelches.touch (result.first);
        }

//1。使用固定IF：
//固定活动计数低于固定计数，并且
//（有符合条件的固定地址可尝试或
//任何出站尝试正在进行中）
//
        if (counts_.fixed_active() < fixed_.size ())
        {
            get_fixed (needed, h.list(), m_squelches);

            if (! h.list().empty ())
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic connect " << h.list().size() << " fixed";
                return h.list();
            }

            if (counts_.attempts() > 0)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic waiting on " <<
                        counts_.attempts() << " attempts";
                return none;
            }
        }

//仅在启用自动连接并且我们
//少于所需的出站插槽数
//
        if (! config_.autoConnect ||
            counts_.out_active () >= counts_.out_max ())
            return none;

//2。如果出现以下情况，请使用LiveCache：
//缓存中有任何条目或
//正在进行任何出站尝试
//
        {
            livecache_.hops.shuffle();
            handout (&h, (&h)+1,
                livecache_.hops.rbegin(),
                    livecache_.hops.rend());
            if (! h.list().empty ())
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic connect " << h.list().size () << " live " <<
                    ((h.list().size () > 1) ? "endpoints" : "endpoint");
                return h.list();
            }
            else if (counts_.attempts() > 0)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Logic waiting on " <<
                        counts_.attempts() << " attempts";
                return none;
            }
        }

        /*三。引导缓存重新填充
            如果bootcache为空，请尝试从当前
            一组源并将它们添加到引导缓存中。

            Pseudocode：
                if（域名.count（）>0和（
                           unusedbootstrapips.count（）=0
                        或activenamerelesolutions.count（）>0）
                    foroneormore（最近未解决的域名）
                        联系域名并向未使用的bootstrapips添加条目
                    返回；
        **/


//4。在以下情况下使用bootcache：
//有一些条目我们最近没有尝试过
//
        for (auto iter (bootcache_.begin());
            ! h.full() && iter != bootcache_.end(); ++iter)
            h.try_insert (*iter);

        if (! h.list().empty ())
        {
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic connect " << h.list().size () << " boot " <<
                ((h.list().size () > 1) ? "addresses" : "address");
            return h.list();
        }

//如果我们到了这里，就卡住了
        return none;
    }

    std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>>
    buildEndpointsForPeers()
    {
        std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>> result;

        std::lock_guard<std::recursive_mutex> _(lock_);

        clock_type::time_point const now = m_clock.now();
        if (m_whenBroadcast <= now)
        {
            std::vector <SlotHandouts> targets;

            {
//构建活动插槽列表
                std::vector <SlotImp::ptr> slots;
                slots.reserve (slots_.size());
                std::for_each (slots_.cbegin(), slots_.cend(),
                    [&slots](Slots::value_type const& value)
                    {
                        if (value.second->state() == Slot::active)
                            slots.emplace_back (value.second);
                    });
                std::shuffle (slots.begin(), slots.end(), default_prng());

//建立目标向量
                targets.reserve (slots.size());
                std::for_each (slots.cbegin(), slots.cend(),
                    [&targets](SlotImp::ptr const& slot)
                    {
                        targets.emplace_back (slot);
                    });
            }

            /*弗法科纸币
                这是一个临时措施。一旦我们知道自己的知识产权
                地址，正确的解决方案是将其放入LiveCache
                跳0，并通过常规的讲义路径。这种方式
                我们避免把地址发得太频繁，这段代码
                遭受痛苦。
            **/

//如果出现以下情况，请为我们自己添加一个条目：
//1。我们要进来
//2。我们有槽
//三。我们还没有通过防火墙测试
//
            if (config_.wantIncoming &&
                counts_.inboundSlots() > 0)
            {
                Endpoint ep;
                ep.hops = 0;
//我们在此处使用未指定的（0）地址，因为值为
//与接受者无关。当对等端接收到终结点时
//对于0个跃点，它们使用socket remote_addr而不是
//消息中的值。此外，由于地址值
//被忽略，类型/版本（IPv4与IPv6）无关紧要
//要么。ipv6有一个更紧凑的字符串
//表示0，因此将其用于自输入。
                ep.address = beast::IP::Endpoint (
                    beast::IP::AddressV6 ()).at_port (
                        config_.listeningPort);
                for (auto& t : targets)
                    t.insert (ep);
            }

//按跃点构建端点序列
            livecache_.hops.shuffle();
            handout (targets.begin(), targets.end(),
                livecache_.hops.begin(),
                    livecache_.hops.end());

//广播
            for (auto const& t : targets)
            {
                SlotImp::ptr const& slot = t.slot();
                auto const& list = t.list();
                JLOG(m_journal.trace()) << beast::leftw (18) <<
                    "Logic sending " << slot->remote_endpoint() <<
                    " with " << list.size() <<
                    ((list.size() == 1) ? " endpoint" : " endpoints");
                result.push_back (std::make_pair (slot, list));
            }

            m_whenBroadcast = now + Tuning::secondsPerMessage;
        }

        return result;
    }

    void once_per_second()
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

//使LiveCache过期
        livecache_.expire ();

//使每个插槽中最近的缓存过期
        for (auto const& entry : slots_)
            entry.second->expire();

//使最近的尝试表过期
        beast::expire (m_squelches,
            Tuning::recentAttemptDuration);

        bootcache_.periodicActivity ();
    }

//——————————————————————————————————————————————————————————————

//验证并清除从槽中收到的列表。
    void preprocess (SlotImp::ptr const& slot, Endpoints& list)
    {
        bool neighbor (false);
        for (auto iter = list.begin(); iter != list.end();)
        {
            Endpoint& ep (*iter);

//强制跃点限制
            if (ep.hops > Tuning::maxHops)
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " for excess hops " << ep.hops;
                iter = list.erase (iter);
                continue;
            }

//看看我们是否有直接联系
            if (ep.hops == 0)
            {
                if (! neighbor)
                {
//填写邻居的远程地址
                    neighbor = true;
                    ep.address = slot->remote_endpoint().at_port (
                        ep.address.port ());
                }
                else
                {
                    JLOG(m_journal.debug()) << beast::leftw (18) <<
                        "Endpoints drop " << ep.address <<
                        " for extra self";
                    iter = list.erase (iter);
                    continue;
                }
            }

//放弃无效地址
            if (! is_valid_address (ep.address))
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " as invalid";
                iter = list.erase (iter);
                continue;
            }

//筛选重复项
            if (std::any_of (list.begin(), iter,
                [ep](Endpoints::value_type const& other)
                {
                    return ep.address == other.address;
                }))
            {
                JLOG(m_journal.debug()) << beast::leftw (18) <<
                    "Endpoints drop " << ep.address <<
                    " as duplicate";
                iter = list.erase (iter);
                continue;
            }

//增加传入消息的跃点计数，因此
//我们将它存储在我们将发送到的跃点计数处。
//
            ++ep.hops;

            ++iter;
        }
    }

    void on_endpoints (SlotImp::ptr const& slot, Endpoints list)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Endpoints from " << slot->remote_endpoint () <<
            " contained " << list.size () <<
            ((list.size() > 1) ? " entries" : " entry");

        std::lock_guard<std::recursive_mutex> _(lock_);

//对象必须存在于我们的表中
        assert (slots_.find (slot->remote_endpoint ()) !=
            slots_.end ());

//必须握手！
        assert (slot->state() == Slot::active);

        preprocess (slot, list);

        clock_type::time_point const now (m_clock.now());

        for (auto const& ep : list)
        {
            assert (ep.hops != 0);

            slot->recent.insert (ep.address, ep.hops);

//注意，跃点已增加，因此1
//表示直接连接的邻居。
//
            if (ep.hops == 1)
            {
                if (slot->connectivityCheckInProgress)
                {
                    JLOG(m_journal.debug()) << beast::leftw (18) <<
                        "Logic testing " << ep.address << " already in progress";
                    continue;
                }

                if (! slot->checked)
                {
//标记此插槽的支票正在进行中。
                    slot->connectivityCheckInProgress = true;

//测试插槽的监听端口之前
//首次将其添加到LiveCache。
//
                    m_checker.async_connect (ep.address, std::bind (
                        &Logic::checkComplete, this, slot->remote_endpoint(),
                            ep.address, std::placeholders::_1));

//注意，我们只是丢弃第一个端点
//当我们执行
//听力测试。他们会再给我们寄一份
//几秒钟后一个。

                    continue;
                }

//如果测试失败，则跳过地址
                if (! slot->canAccept)
                    continue;
            }

//只有当邻居通过
//听力测试，否则我们就悄悄放下他们的凌乱
//因为他们的监听端口配置错误。
//
            livecache_.insert (ep);
            bootcache_.insert (ep.address);
        }

        slot->whenAcceptEndpoints = now + Tuning::secondsPerMessage;
    }

//——————————————————————————————————————————————————————————————

    void remove (SlotImp::ptr const& slot)
    {
        auto const iter = slots_.find (
            slot->remote_endpoint ());
//插槽必须存在于表中
        assert (iter != slots_.end ());
//按IP表从插槽中删除
        slots_.erase (iter);
//拔出钥匙（如果有）
        if (slot->public_key () != boost::none)
        {
            auto const iter = keys_.find (*slot->public_key());
//密钥必须存在
            assert (iter != keys_.end ());
            keys_.erase (iter);
        }
//从连接的地址表中删除
        {
            auto const iter (connectedAddresses_.find (
                slot->remote_endpoint().address()));
//地址必须存在
            assert (iter != connectedAddresses_.end ());
            connectedAddresses_.erase (iter);
        }

//更新计数
        counts_.remove (*slot);
    }

    void on_closed (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        remove (slot);

//标记固定槽故障
        if (slot->fixed() && ! slot->inbound() && slot->state() != Slot::active)
        {
            auto iter (fixed_.find (slot->remote_endpoint()));
            assert (iter != fixed_.end ());
            iter->second.failure (m_clock.now ());
            JLOG(m_journal.debug()) << beast::leftw (18) <<
                "Logic fixed " << slot->remote_endpoint () << " failed";
        }

//按州记帐
        switch (slot->state())
        {
        case Slot::accept:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic accept " << slot->remote_endpoint () << " failed";
            break;

        case Slot::connect:
        case Slot::connected:
            bootcache_.on_failure (slot->remote_endpoint ());
//如果地址存在于临时/实时
//端点LiveCache，那么我们应该标记失败
//好像没有通过听力测试。我们也应该
//避免传播地址。
            break;

        case Slot::active:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic close " << slot->remote_endpoint();
            break;

        case Slot::closing:
            JLOG(m_journal.trace()) << beast::leftw (18) <<
                "Logic finished " << slot->remote_endpoint();
            break;

        default:
            assert (false);
            break;
        }
    }

    void on_failure (SlotImp::ptr const& slot)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

        bootcache_.on_failure (slot->remote_endpoint ());
    }

//将一组重定向IP地址插入bootcache
    template <class FwdIter>
    void
    onRedirects (FwdIter first, FwdIter last,
        boost::asio::ip::tcp::endpoint const& remote_address);

//——————————————————————————————————————————————————————————————

//如果地址与固定槽地址匹配，则返回“true”
//一定锁好了
    bool fixed (beast::IP::Endpoint const& endpoint) const
    {
        for (auto const& entry : fixed_)
            if (entry.first == endpoint)
                return true;
        return false;
    }

//如果地址与固定槽地址匹配，则返回“true”
//请注意，这不会使用IP:：Endpoint中的端口信息
//一定锁好了
    bool fixed (beast::IP::Address const& address) const
    {
        for (auto const& entry : fixed_)
            if (entry.first.address () == address)
                return true;
        return false;
    }

//——————————————————————————————————————————————————————————————
//
//连接策略
//
//——————————————————————————————————————————————————————————————

    /*为出站尝试添加合格的固定地址。*/
    template <class Container>
    void get_fixed (std::size_t needed, Container& c,
        typename ConnectHandouts::Squelches& squelches)
    {
        auto const now (m_clock.now());
        for (auto iter = fixed_.begin ();
            needed && iter != fixed_.end (); ++iter)
        {
            auto const& address (iter->first.address());
            if (iter->second.when() <= now && squelches.find(address) ==
                    squelches.end() && std::none_of (
                        slots_.cbegin(), slots_.cend(),
                    [address](Slots::value_type const& v)
                    {
                        return address == v.first.address();
                    }))
            {
                squelches.insert(iter->first.address());
                c.push_back (iter->first);
                --needed;
            }
        }
    }

//——————————————————————————————————————————————————————————————

    void
    addStaticSource (std::shared_ptr<Source> const& source)
    {
        fetch (source);
    }

    void
    addSource (std::shared_ptr<Source> const& source)
    {
        m_sources.push_back (source);
    }

//——————————————————————————————————————————————————————————————
//
//bootcache livecache源
//
//——————————————————————————————————————————————————————————————

//添加一组地址。
//返回添加的地址数。
//
    int addBootcacheAddresses (IPAddresses const& list)
    {
        int count (0);
        std::lock_guard<std::recursive_mutex> _(lock_);
        for (auto addr : list)
        {
            if (bootcache_.insertStatic (addr))
                ++count;
        }
        return count;
    }

//从指定的源获取bootcache地址。
    void
    fetch (std::shared_ptr<Source> const& source)
    {
        Source::Results results;

        {
            {
                std::lock_guard<std::recursive_mutex> _(lock_);
                if (stopping_)
                    return;
                fetchSource_ = source;
            }

//vfalc注意，获取是同步的，
//不确定这是不是好事。
//
            source->fetch (results, m_journal);

            {
                std::lock_guard<std::recursive_mutex> _(lock_);
                if (stopping_)
                    return;
                fetchSource_ = nullptr;
            }
        }

        if (! results.error)
        {
            int const count (addBootcacheAddresses (results.addresses));
            JLOG(m_journal.info()) << beast::leftw (18) <<
                "Logic added " << count <<
                " new " << ((count == 1) ? "address" : "addresses") <<
                " from " << source->name();
        }
        else
        {
            JLOG(m_journal.error()) << beast::leftw (18) <<
                "Logic failed " << "'" << source->name() << "' fetch, " <<
                results.error.message();
        }
    }

//——————————————————————————————————————————————————————————————
//
//终结点消息处理
//
//——————————————————————————————————————————————————————————————

//如果ip:：endpoint不包含无效数据，则返回true。
    bool is_valid_address (beast::IP::Endpoint const& address)
    {
        if (is_unspecified (address))
            return false;
        if (! is_public (address))
            return false;
        if (address.port() == 0)
            return false;
        return true;
    }

//——————————————————————————————————————————————————————————————
//
//属性流
//
//——————————————————————————————————————————————————————————————

    void writeSlots (beast::PropertyStream::Set& set, Slots const& slots)
    {
        for (auto const& entry : slots)
        {
            beast::PropertyStream::Map item (set);
            SlotImp const& slot (*entry.second);
            if (slot.local_endpoint () != boost::none)
                item ["local_address"] = to_string (*slot.local_endpoint ());
            item ["remote_address"]   = to_string (slot.remote_endpoint ());
            if (slot.inbound())
                item ["inbound"]    = "yes";
            if (slot.fixed())
                item ["fixed"]      = "yes";
            if (slot.cluster())
                item ["cluster"]    = "yes";

            item ["state"] = stateString (slot.state());
        }
    }

    void onWrite (beast::PropertyStream::Map& map)
    {
        std::lock_guard<std::recursive_mutex> _(lock_);

//vvalco注意到这些丑陋的石膏是需要的，因为
//关于在某些Linux上如何声明std:：size_t
//
        map ["bootcache"]   = std::uint32_t (bootcache_.size());
        map ["fixed"]       = std::uint32_t (fixed_.size());

        {
            beast::PropertyStream::Set child ("peers", map);
            writeSlots (child, slots_);
        }

        {
            beast::PropertyStream::Map child ("counts", map);
            counts_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("config", map);
            config_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("livecache", map);
            livecache_.onWrite (child);
        }

        {
            beast::PropertyStream::Map child ("bootcache", map);
            bootcache_.onWrite (child);
        }
    }

//——————————————————————————————————————————————————————————————
//
//诊断学
//
//——————————————————————————————————————————————————————————————

    Counts const& counts () const
    {
        return counts_;
    }

    static std::string stateString (Slot::State state)
    {
        switch (state)
        {
        case Slot::accept:     return "accept";
        case Slot::connect:    return "connect";
        case Slot::connected:  return "connected";
        case Slot::active:     return "active";
        case Slot::closing:    return "closing";
        default:
            break;
        };
        return "?";
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Checker>
template <class FwdIter>
void
Logic<Checker>::onRedirects (FwdIter first, FwdIter last,
    boost::asio::ip::tcp::endpoint const& remote_address)
{
    std::lock_guard<std::recursive_mutex> _(lock_);
    std::size_t n = 0;
    for(;first != last && n < Tuning::maxRedirects; ++first, ++n)
        bootcache_.insert(
            beast::IPAddressConversion::from_asio(*first));
    if (n > 0)
    {
        JLOG(m_journal.trace()) << beast::leftw (18) <<
            "Logic add " << n << " redirect IPs from " << remote_address;
    }
}

}
}

#endif
