
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

#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Checker.h>
#include <ripple/peerfinder/impl/Logic.h>
#include <ripple/peerfinder/impl/SourceStrings.h>
#include <ripple/peerfinder/impl/StoreSqdb.h>
#include <ripple/core/SociDB.h>
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <memory>
#include <thread>

namespace ripple {
namespace PeerFinder {

class ManagerImp
    : public Manager
{
public:
    boost::asio::io_service &io_service_;
    boost::optional <boost::asio::io_service::work> work_;
    clock_type& m_clock;
    beast::Journal m_journal;
    StoreSqdb m_store;
    Checker<boost::asio::ip::tcp> checker_;
    Logic <decltype(checker_)> m_logic;
    SociConfig m_sociConfig;

//——————————————————————————————————————————————————————————————

    ManagerImp (
        Stoppable& stoppable,
        boost::asio::io_service& io_service,
        clock_type& clock,
        beast::Journal journal,
        BasicConfig const& config)
        : Manager (stoppable)
        , io_service_(io_service)
        , work_(boost::in_place(std::ref(io_service_)))
        , m_clock (clock)
        , m_journal (journal)
        , m_store (journal)
        , checker_ (io_service_)
        , m_logic (clock, m_store, checker_, journal)
        , m_sociConfig (config, "peerfinder")
    {
    }

    ~ManagerImp() override
    {
        close();
    }

    void
    close()
    {
        if (work_)
        {
            work_ = boost::none;
            checker_.stop();
            m_logic.stop();
        }
    }

//——————————————————————————————————————————————————————————————
//
//窥探者
//
//——————————————————————————————————————————————————————————————

    void setConfig (Config const& config) override
    {
        m_logic.config (config);
    }

    Config
    config() override
    {
        return m_logic.config();
    }

    void addFixedPeer (std::string const& name,
        std::vector <beast::IP::Endpoint> const& addresses) override
    {
        m_logic.addFixedPeer (name, addresses);
    }

    void
    addFallbackStrings (std::string const& name,
        std::vector <std::string> const& strings) override
    {
        m_logic.addStaticSource (SourceStrings::New (name, strings));
    }

    void addFallbackURL (std::string const& name,
        std::string const& url)
    {
//vfalc todo需要实施
    }

//——————————————————————————————————————————————————————————————

    Slot::ptr
    new_inbound_slot (
        beast::IP::Endpoint const& local_endpoint,
            beast::IP::Endpoint const& remote_endpoint) override
    {
        return m_logic.new_inbound_slot (local_endpoint, remote_endpoint);
    }

    Slot::ptr
    new_outbound_slot (beast::IP::Endpoint const& remote_endpoint) override
    {
        return m_logic.new_outbound_slot (remote_endpoint);
    }

    void
    on_endpoints (Slot::ptr const& slot,
        Endpoints const& endpoints)  override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        m_logic.on_endpoints (impl, endpoints);
    }

    void
    on_closed (Slot::ptr const& slot)  override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        m_logic.on_closed (impl);
    }

    void
    on_failure (Slot::ptr const& slot)  override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        m_logic.on_failure (impl);
    }

    void
    onRedirects (boost::asio::ip::tcp::endpoint const& remote_address,
        std::vector<boost::asio::ip::tcp::endpoint> const& eps) override
    {
        m_logic.onRedirects(eps.begin(), eps.end(), remote_address);
    }

//——————————————————————————————————————————————————————————————

    bool
    onConnected (Slot::ptr const& slot,
        beast::IP::Endpoint const& local_endpoint) override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        return m_logic.onConnected (impl, local_endpoint);
    }

    Result
    activate (Slot::ptr const& slot,
        PublicKey const& key, bool cluster) override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        return m_logic.activate (impl, key, cluster);
    }

    std::vector <Endpoint>
    redirect (Slot::ptr const& slot) override
    {
        SlotImp::ptr impl (std::dynamic_pointer_cast <SlotImp> (slot));
        return m_logic.redirect (impl);
    }

    std::vector <beast::IP::Endpoint>
    autoconnect() override
    {
        return m_logic.autoconnect();
    }

    void
    once_per_second() override
    {
        m_logic.once_per_second();
    }

    std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>>
    buildEndpointsForPeers() override
    {
        return m_logic.buildEndpointsForPeers();
    }

//——————————————————————————————————————————————————————————————
//
//可停止的
//
//——————————————————————————————————————————————————————————————

    void
    onPrepare () override
    {
        m_store.open (m_sociConfig);
        m_logic.load ();
    }

    void
    onStart() override
    {
    }

    void onStop () override
    {
        close();
        stopped();
    }

//——————————————————————————————————————————————————————————————
//
//属性流
//
//——————————————————————————————————————————————————————————————

    void onWrite (beast::PropertyStream::Map& map) override
    {
        m_logic.onWrite (map);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Manager::Manager (Stoppable& parent)
    : Stoppable ("PeerFinder", parent)
    , beast::PropertyStream::Source ("peerfinder")
{
}

std::unique_ptr<Manager>
make_Manager (Stoppable& parent, boost::asio::io_service& io_service,
        clock_type& clock, beast::Journal journal, BasicConfig const& config)
{
    return std::make_unique<ManagerImp> (
        parent, io_service, clock, journal, config);
}

}
}
