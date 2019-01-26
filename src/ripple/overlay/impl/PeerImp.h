
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

#ifndef RIPPLE_OVERLAY_PEERIMP_H_INCLUDED
#define RIPPLE_OVERLAY_PEERIMP_H_INCLUDED

#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/beast/utility/WrappedSink.h>
#include <ripple/overlay/impl/ProtocolMessage.h>
#include <ripple/overlay/impl/OverlayImpl.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/resource/Fees.h>

#include <boost/endian/conversion.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <deque>
#include <queue>

namespace ripple {

class PeerImp
    : public Peer
    , public std::enable_shared_from_this <PeerImp>
    , public OverlayImpl::Child
{
public:
    /*连接类型。
        影响消息的路由方式。
    **/

    enum class Type
    {
        legacy,
        leaf,
        peer
    };

    /*当前状态*/
    enum class State
    {
        /*正在建立连接（出站）*/
        connecting

        /*已成功建立连接*/
        ,connected

        /*已收到来自此对等方的握手*/
        ,handshaked

        /*主动运行Ripple协议*/
        ,active
    };

    enum class Sanity
    {
        insane
        ,unknown
        ,sane
    };

    struct ShardInfo
    {
        beast::IP::Endpoint endpoint;
        RangeSet<std::uint32_t> shardIndexes;
    };

    using ptr = std::shared_ptr <PeerImp>;

private:
    using clock_type    = std::chrono::steady_clock;
    using error_code    = boost::system::error_code;
    using socket_type   = boost::asio::ip::tcp::socket;
    using stream_type   = boost::asio::ssl::stream <socket_type&>;
    using address_type  = boost::asio::ip::address;
    using endpoint_type = boost::asio::ip::tcp::endpoint;

//最小有效完成消息的长度
    static const size_t sslMinimumFinishedLength = 12;

    Application& app_;
    id_t const id_;
    beast::WrappedSink sink_;
    beast::WrappedSink p_sink_;
    beast::Journal journal_;
    beast::Journal p_journal_;
    std::unique_ptr<beast::asio::ssl_bundle> ssl_bundle_;
    socket_type& socket_;
    stream_type& stream_;
    boost::asio::io_service::strand strand_;
    boost::asio::basic_waitable_timer<
        std::chrono::steady_clock> timer_;

//type type_u=type:：legacy；

//在连接过程的每个阶段更新以反映
//当前条件尽可能接近。
    beast::IP::Endpoint const remote_address_;

//这些是为了防止有关初始化顺序的警告。
//
    OverlayImpl& overlay_;
    bool m_inbound;
State state_;          //当前状态
    std::atomic<Sanity> sanity_;
    clock_type::time_point insaneTime_;
    bool detaching_ = false;
//对等机的节点公钥。
    PublicKey publicKey_;
    std::string name_;
    uint256 sharedValue_;

//该同行提供的最小和最大分类账的索引
//
    LedgerIndex minLedger_ = 0;
    LedgerIndex maxLedger_ = 0;
    uint256 closedLedgerHash_;
    uint256 previousLedgerHash_;
    std::deque<uint256> recentLedgers_;
    std::deque<uint256> recentTxSets_;

    std::chrono::milliseconds latency_ = std::chrono::milliseconds (-1);
    std::uint64_t lastPingSeq_ = 0;
    clock_type::time_point lastPingTime_;
    clock_type::time_point creationTime_;

    std::mutex mutable recentLock_;
    protocol::TMStatusChange last_status_;
    protocol::TMHello hello_;
    Resource::Consumer usage_;
    Resource::Charge fee_;
    PeerFinder::Slot::ptr slot_;
    boost::beast::multi_buffer read_buffer_;
    http_request_type request_;
    http_response_type response_;
    boost::beast::http::fields const& headers_;
    boost::beast::multi_buffer write_buffer_;
    std::queue<Message::pointer> send_queue_;
    bool gracefulClose_ = false;
    int large_sendq_ = 0;
    int no_ping_ = 0;
    std::unique_ptr <LoadEvent> load_event_;
    bool hopsAware_ = false;

    std::mutex mutable shardInfoMutex_;
    hash_map<PublicKey, ShardInfo> shardInfo_;

    friend class OverlayImpl;

public:
    PeerImp (PeerImp const&) = delete;
    PeerImp& operator= (PeerImp const&) = delete;

    /*从已建立的SSL连接创建活动的传入对等端。*/
    PeerImp (Application& app, id_t id, endpoint_type remote_endpoint,
        PeerFinder::Slot::ptr const& slot, http_request_type&& request,
            protocol::TMHello const& hello, PublicKey const& publicKey,
                Resource::Consumer consumer,
                    std::unique_ptr<beast::asio::ssl_bundle>&& ssl_bundle,
                        OverlayImpl& overlay);

    /*创建传出的握手对等。*/
//vfalc legacypublickey应该由slot暗示
    template <class Buffers>
    PeerImp (Application& app, std::unique_ptr<beast::asio::ssl_bundle>&& ssl_bundle,
        Buffers const& buffers, PeerFinder::Slot::ptr&& slot,
            http_response_type&& response, Resource::Consumer usage,
                protocol::TMHello const& hello,
                    PublicKey const& publicKey, id_t id,
                        OverlayImpl& overlay);

    virtual
    ~PeerImp();

    beast::Journal const&
    pjournal() const
    {
        return p_journal_;
    }

    PeerFinder::Slot::ptr const&
    slot()
    {
        return slot_;
    }

//在构造器中从这个调用共享的
    void
    run();

//当覆盖收到停止请求时调用。
    void
    stop() override;

//
//网络
//

    void
    send (Message::pointer const& m) override;

    /*作为协议消息发送一组peerfinder端点。*/
    template <class FwdIt, class = typename std::enable_if_t<std::is_same<
        typename std::iterator_traits<FwdIt>::value_type,
            PeerFinder::Endpoint>::value>>
    void
    sendEndpoints (FwdIt first, FwdIt last);

    beast::IP::Endpoint
    getRemoteAddress() const override
    {
        return remote_address_;
    }

    void
    charge (Resource::Charge const& fee) override;

//
//身份
//

    Peer::id_t
    id() const override
    {
        return id_;
    }

    /*如果此连接将公开共享其IP地址，则返回“true”。*/
    bool
    crawl() const;

    bool
    cluster() const override
    {
        return slot_->cluster();
    }

    bool
    hopsAware() const
    {
        return hopsAware_;
    }

    void
    check();

    /*检查对方是否神志正常
        @param validationseq最近验证的分类帐的分类帐序列
    **/

    void
    checkSanity (std::uint32_t validationSeq);

    void
    checkSanity (std::uint32_t seq1, std::uint32_t seq2);

    PublicKey const&
    getNodePublic () const override
    {
        return publicKey_;
    }

    /*如果报告，则返回对等机正在运行的Rippled版本。*/
    std::string
    getVersion() const;

//返回连接运行时间。
    clock_type::duration
    uptime() const
    {
        return clock_type::now() - creationTime_;
    }

    Json::Value
    json() override;

//
//分类帐
//

    uint256 const&
    getClosedLedgerHash () const override
    {
        return closedLedgerHash_;
    }

    bool
    hasLedger (uint256 const& hash, std::uint32_t seq) const override;

    void
    ledgerRange (std::uint32_t& minSeq, std::uint32_t& maxSeq) const override;

    bool
    hasShard (std::uint32_t shardIndex) const override;

    bool
    hasTxSet (uint256 const& hash) const override;

    void
    cycleStatus () override;

    bool
    supportsVersion (int version) override;

    bool
    hasRange (std::uint32_t uMin, std::uint32_t uMax) override;

//调用以确定查询的优先级
    int
    getScore (bool haveItem) const override;

    bool
    isHighLatency() const override;

    void
    fail(std::string const& reason);

    /*从该对等端返回一组已知的shard索引。*/
    boost::optional<RangeSet<std::uint32_t>>
    getShardIndexes() const;

    /*从该对等端及其子对等端返回任何已知的碎片信息。*/
    boost::optional<hash_map<PublicKey, ShardInfo>>
    getPeerShardInfo() const;

private:
    void
    close();

    void
    fail(std::string const& name, error_code ec);

    void
    gracefulClose();

    void
    setTimer();

    void
    cancelTimer();

    static
    std::string
    makePrefix(id_t id);

//当计时器等待完成时调用
    void
    onTimer (boost::system::error_code const& ec);

//当SSL关闭完成时调用
    void
    onShutdown (error_code ec);

    void
    doAccept();

    http_response_type
    makeResponse (bool crawl, http_request_type const& req,
        beast::IP::Endpoint remoteAddress,
        uint256 const& sharedValue);

    void
    onWriteResponse (error_code ec, std::size_t bytes_transferred);

//
//协议消息循环
//

//启动协议消息循环
    void
    doProtocolStart ();

//当接收到协议消息字节时调用
    void
    onReadMessage (error_code ec, std::size_t bytes_transferred);

//在发送协议消息字节时调用
    void
    onWriteMessage (error_code ec, std::size_t bytes_transferred);

public:
//——————————————————————————————————————————————————————————————
//
//协议流
//
//——————————————————————————————————————————————————————————————

    static
    error_code
    invalid_argument_error()
    {
        return boost::system::errc::make_error_code (
            boost::system::errc::invalid_argument);
    }

    error_code
    onMessageUnknown (std::uint16_t type);

    error_code
    onMessageBegin (std::uint16_t type,
        std::shared_ptr <::google::protobuf::Message> const& m,
        std::size_t size);

    void
    onMessageEnd (std::uint16_t type,
        std::shared_ptr <::google::protobuf::Message> const& m);

    void onMessage (std::shared_ptr <protocol::TMHello> const& m);
    void onMessage (std::shared_ptr <protocol::TMManifests> const& m);
    void onMessage (std::shared_ptr <protocol::TMPing> const& m);
    void onMessage (std::shared_ptr <protocol::TMCluster> const& m);
    void onMessage (std::shared_ptr <protocol::TMGetShardInfo> const& m);
    void onMessage (std::shared_ptr <protocol::TMShardInfo> const& m);
    void onMessage (std::shared_ptr <protocol::TMGetPeers> const& m);
    void onMessage (std::shared_ptr <protocol::TMPeers> const& m);
    void onMessage (std::shared_ptr <protocol::TMEndpoints> const& m);
    void onMessage (std::shared_ptr <protocol::TMTransaction> const& m);
    void onMessage (std::shared_ptr <protocol::TMGetLedger> const& m);
    void onMessage (std::shared_ptr <protocol::TMLedgerData> const& m);
    void onMessage (std::shared_ptr <protocol::TMProposeSet> const& m);
    void onMessage (std::shared_ptr <protocol::TMStatusChange> const& m);
    void onMessage (std::shared_ptr <protocol::TMHaveTransactionSet> const& m);
    void onMessage (std::shared_ptr <protocol::TMValidation> const& m);
    void onMessage (std::shared_ptr <protocol::TMGetObjectByHash> const& m);

private:
    State state() const
    {
        return state_;
    }

    void state (State new_state)
    {
        state_ = new_state;
    }

//——————————————————————————————————————————————————————————————

    void
    addLedger (uint256 const& hash);

    void
    doFetchPack (const std::shared_ptr<protocol::TMGetObjectByHash>& packet);

    void
    checkTransaction (int flags, bool checkSignature,
        std::shared_ptr<STTx const> const& stx);

    void
    checkPropose (Job& job,
        std::shared_ptr<protocol::TMProposeSet> const& packet,
            RCLCxPeerPos peerPos);

    void
    checkValidation (STValidation::pointer val,
        bool isTrusted, std::shared_ptr<protocol::TMValidation> const& packet);

    void
    getLedger (std::shared_ptr<protocol::TMGetLedger> const&packet);

//当我们接收到Tx集数据时调用。
    void
    peerTXData (uint256 const& hash,
        std::shared_ptr <protocol::TMLedgerData> const& pPacket,
            beast::Journal journal);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Buffers>
PeerImp::PeerImp (Application& app, std::unique_ptr<beast::asio::ssl_bundle>&& ssl_bundle,
    Buffers const& buffers, PeerFinder::Slot::ptr&& slot,
        http_response_type&& response, Resource::Consumer usage,
            protocol::TMHello const& hello,
                PublicKey const& publicKey, id_t id,
                    OverlayImpl& overlay)
    : Child (overlay)
    , app_ (app)
    , id_ (id)
    , sink_ (app_.journal("Peer"), makePrefix(id))
    , p_sink_ (app_.journal("Protocol"), makePrefix(id))
    , journal_ (sink_)
    , p_journal_ (p_sink_)
    , ssl_bundle_(std::move(ssl_bundle))
    , socket_ (ssl_bundle_->socket)
    , stream_ (ssl_bundle_->stream)
    , strand_ (socket_.get_io_service())
    , timer_ (socket_.get_io_service())
    , remote_address_ (slot->remote_endpoint())
    , overlay_ (overlay)
    , m_inbound (false)
    , state_ (State::active)
    , sanity_ (Sanity::unknown)
    , insaneTime_ (clock_type::now())
    , publicKey_ (publicKey)
    , creationTime_ (clock_type::now())
    , hello_ (hello)
    , usage_ (usage)
    , fee_ (Resource::feeLightPeer)
    , slot_ (std::move(slot))
    , response_(std::move(response))
    , headers_(response_)
{
    read_buffer_.commit (boost::asio::buffer_copy(read_buffer_.prepare(
        boost::asio::buffer_size(buffers)), buffers));
}

template <class FwdIt, class>
void
PeerImp::sendEndpoints (FwdIt first, FwdIt last)
{
    protocol::TMEndpoints tm;
    for (;first != last; ++first)
    {
        auto const& ep = *first;
//最终删除端点并保留端点
//（一旦我们确定整个网络了解端点v2）
        protocol::TMEndpoint& tme (*tm.add_endpoints());
        if (ep.address.is_v4())
            tme.mutable_ipv4()->set_ipv4(
                boost::endian::native_to_big(
                    static_cast<std::uint32_t>(ep.address.to_v4().to_ulong())));
        else
            tme.mutable_ipv4()->set_ipv4(0);
        tme.mutable_ipv4()->set_ipv4port (ep.address.port());
        tme.set_hops (ep.hops);

//添加v2终结点（字符串）
        auto& tme2 (*tm.add_endpoints_v2());
        tme2.set_endpoint(ep.address.to_string());
        tme2.set_hops (ep.hops);
    }
    tm.set_version (2);

    send (std::make_shared <Message> (tm, protocol::mtENDPOINTS));
}

}

#endif
