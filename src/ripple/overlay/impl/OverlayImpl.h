
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

#ifndef RIPPLE_OVERLAY_OVERLAYIMPL_H_INCLUDED
#define RIPPLE_OVERLAY_OVERLAYIMPL_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/core/Job.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/impl/TrafficCount.h>
#include <ripple/server/Handoff.h>
#include <ripple/rpc/ServerHandler.h>
#include <ripple/basics/Resolver.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/overlay/impl/TMHello.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/resource/ResourceManager.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/container/flat_map.hpp>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace ripple {

class PeerImp;
class BasicConfig;

enum
{
    maxTTL = 2
};

class OverlayImpl : public Overlay
{
public:
    class Child
    {
    protected:
        OverlayImpl& overlay_;

        explicit
        Child (OverlayImpl& overlay);

        virtual ~Child();

    public:
        virtual void stop() = 0;
    };

private:
    using clock_type = std::chrono::steady_clock;
    using socket_type = boost::asio::ip::tcp::socket;
    using address_type = boost::asio::ip::address;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using error_code = boost::system::error_code;

    struct Timer
        : Child
        , std::enable_shared_from_this<Timer>
    {
        boost::asio::basic_waitable_timer <clock_type> timer_;

        explicit
        Timer (OverlayImpl& overlay);

        void
        stop() override;

        void
        run();

        void
        on_timer (error_code ec);
    };

    Application& app_;
    boost::asio::io_service& io_service_;
    boost::optional<boost::asio::io_service::work> work_;
    boost::asio::io_service::strand strand_;
std::recursive_mutex mutex_; //vvalco使用std:：mutex
    std::condition_variable_any cond_;
    std::weak_ptr<Timer> timer_;
    boost::container::flat_map<
        Child*, std::weak_ptr<Child>> list_;
    Setup setup_;
    beast::Journal journal_;
    ServerHandler& serverHandler_;
    Resource::Manager& m_resourceManager;
    std::unique_ptr <PeerFinder::Manager> m_peerFinder;
    TrafficCount m_traffic;
    hash_map <PeerFinder::Slot::ptr,
        std::weak_ptr <PeerImp>> m_peers;
    hash_map<Peer::id_t, std::weak_ptr<PeerImp>> ids_;
    Resolver& m_resolver;
    std::atomic <Peer::id_t> next_id_;
    int timer_count_;
    std::atomic <uint64_t> jqTransOverflow_ {0};
    std::atomic <uint64_t> peerDisconnects_ {0};
    std::atomic <uint64_t> peerDisconnectsCharges_ {0};

//上一次我们对同龄人进行搜索以获取碎片信息。”cs'=抓取碎片
    std::atomic<std::chrono::seconds> csLast_{std::chrono::seconds{0}};
    std::mutex csMutex_;
    std::condition_variable csCV_;
//对等ID应接收最后一个链接通知
    std::set<std::uint32_t> csIDs_;

//——————————————————————————————————————————————————————————————

public:
    OverlayImpl (Application& app, Setup const& setup, Stoppable& parent,
        ServerHandler& serverHandler, Resource::Manager& resourceManager,
        Resolver& resolver, boost::asio::io_service& io_service,
        BasicConfig const& config);

    ~OverlayImpl();

    OverlayImpl (OverlayImpl const&) = delete;
    OverlayImpl& operator= (OverlayImpl const&) = delete;

    PeerFinder::Manager&
    peerFinder()
    {
        return *m_peerFinder;
    }

    Resource::Manager&
    resourceManager()
    {
        return m_resourceManager;
    }

    ServerHandler&
    serverHandler()
    {
        return serverHandler_;
    }

    Setup const&
    setup() const
    {
        return setup_;
    }

    Handoff
    onHandoff (std::unique_ptr <beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
            endpoint_type remote_endpoint) override;

    void
    connect(beast::IP::Endpoint const& remote_endpoint) override;

    int
    limit() override;

    std::size_t
    size() override;

    Json::Value
    json() override;

    PeerSequence
    getActivePeers() override;

    void
    check () override;

    void
    checkSanity (std::uint32_t) override;

    std::shared_ptr<Peer>
    findPeerByShortID (Peer::id_t const& id) override;

    void
    send (protocol::TMProposeSet& m) override;

    void
    send (protocol::TMValidation& m) override;

    void
    relay (protocol::TMProposeSet& m,
        uint256 const& uid) override;

    void
    relay (protocol::TMValidation& m,
        uint256 const& uid) override;

//——————————————————————————————————————————————————————————————
//
//过度干涉
//

    void
    add_active (std::shared_ptr<PeerImp> const& peer);

    void
    remove (PeerFinder::Slot::ptr const& slot);

    /*当对等机成功连接时调用
        在完成对等握手之后和
        对等激活。此时，对等地址和公钥
        是众所周知的。
    **/

    void
    activate (std::shared_ptr<PeerImp> const& peer);

//在销毁活动对等时调用。
    void
    onPeerDeactivate (Peer::id_t id);

//unaryfunc将被称为
//void（std:：shared_ptr<peerimp>&&）
//
    template <class UnaryFunc>
    void
    for_each (UnaryFunc&& f)
    {
        std::vector<std::weak_ptr<PeerImp>> wp;
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

//循环访问对等列表的副本，因为对等
//破坏会使迭代器失效。
            wp.reserve(ids_.size());

            for (auto& x : ids_)
                wp.push_back(x.second);
        }

        for (auto& w : wp)
        {
            if (auto p = w.lock())
                f(std::move(p));
        }
    }

    std::size_t
    selectPeers (PeerSet& set, std::size_t limit, std::function<
        bool(std::shared_ptr<Peer> const&)> score) override;

//当从对等端接收tmmanifest时调用
    void
    onManifests (
        std::shared_ptr<protocol::TMManifests> const& m,
            std::shared_ptr<PeerImp> const& from);

    static
    bool
    isPeerUpgrade (http_request_type const& request);

    template<class Body>
    static
    bool
    isPeerUpgrade (boost::beast::http::response<Body> const& response)
    {
        if (! is_upgrade(response))
            return false;
        if(response.result() != boost::beast::http::status::switching_protocols)
            return false;
        auto const versions = parse_ProtocolVersions(
            response["Upgrade"]);
        if (versions.size() == 0)
            return false;
        return true;
    }

    template<class Fields>
    static
    bool
    is_upgrade(boost::beast::http::header<true, Fields> const& req)
    {
        if(req.version() < 11)
            return false;
        if(req.method() != boost::beast::http::verb::get)
            return false;
        if(! boost::beast::http::token_list{req["Connection"]}.exists("upgrade"))
            return false;
        return true;
    }

    template<class Fields>
    static
    bool
    is_upgrade(boost::beast::http::header<false, Fields> const& req)
    {
        if(req.version() < 11)
            return false;
        if(! boost::beast::http::token_list{req["Connection"]}.exists("upgrade"))
            return false;
        return true;
    }

    static
    std::string
    makePrefix (std::uint32_t id);

    void
    reportTraffic (
        TrafficCount::category cat,
        bool isInbound,
        int bytes);

    void
    incJqTransOverflow() override
    {
        ++jqTransOverflow_;
    }

    std::uint64_t
    getJqTransOverflow() const override
    {
        return jqTransOverflow_;
    }

    void
    incPeerDisconnect() override
    {
        ++peerDisconnects_;
    }

    std::uint64_t
    getPeerDisconnect() const override
    {
        return peerDisconnects_;
    }

    void
    incPeerDisconnectCharges() override
    {
        ++peerDisconnectsCharges_;
    }

    std::uint64_t
    getPeerDisconnectCharges() const override
    {
        return peerDisconnectsCharges_;
    }

    Json::Value
    crawlShards(bool pubKey, std::uint32_t hops) override;


    /*当收到来自对等链的最后一个链接时调用。

        接收到碎片信息的@param id peer id。
    **/

    void
    lastLink(std::uint32_t id);

private:
    std::shared_ptr<Writer>
    makeRedirectResponse (PeerFinder::Slot::ptr const& slot,
        http_request_type const& request, address_type remote_address);

    std::shared_ptr<Writer>
    makeErrorResponse (PeerFinder::Slot::ptr const& slot,
        http_request_type const& request, address_type remote_address,
        std::string msg);

    bool
    processRequest (http_request_type const& req,
        Handoff& handoff);

    /*返回有关覆盖网络上对等点的信息。
        通过/crawl api报告
        通过配置部分控制[爬行]覆盖=[0 1]
    **/

    Json::Value
    getOverlayInfo();

    /*返回有关本地服务器的信息。
        通过/crawl api报告
        通过配置部分控制[爬行]服务器=[0 1]
    **/

    Json::Value
    getServerInfo();

    /*返回有关本地服务器性能计数器的信息。
        通过/crawl api报告
        通过配置节控制[爬行]计数=[0 1]
    **/

    Json::Value
    getServerCounts();

    /*返回有关本地服务器的UNL的信息。
        通过/crawl api报告
        通过配置节[Crawl]unl=[0 1]控制
    **/

    Json::Value
    getUnlInfo();

//——————————————————————————————————————————————————————————————

//
//可停止的
//

    void
    checkStopped();

    void
    onPrepare() override;

    void
    onStart() override;

    void
    onStop() override;

    void
    onChildrenStopped() override;

//
//属性流
//

    void
    onWrite (beast::PropertyStream::Map& stream) override;

//——————————————————————————————————————————————————————————————

    void
    remove (Child& child);

    void
    stop();

    void
    autoConnect();

    void
    sendEndpoints();
};

} //涟漪

#endif
