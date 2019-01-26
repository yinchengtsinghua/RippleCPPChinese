
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

#include <ripple/overlay/impl/PeerImp.h>
#include <ripple/overlay/impl/Tuning.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/InboundTransactions.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/tx/apply.h>
#include <ripple/basics/random.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/predicates.h>
#include <ripple/protocol/digest.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <memory>
#include <sstream>

using namespace std::chrono_literals;

namespace ripple {

PeerImp::PeerImp (Application& app, id_t id, endpoint_type remote_endpoint,
    PeerFinder::Slot::ptr const& slot, http_request_type&& request,
        protocol::TMHello const& hello, PublicKey const& publicKey,
            Resource::Consumer consumer,
                std::unique_ptr<beast::asio::ssl_bundle>&& ssl_bundle,
                    OverlayImpl& overlay)
    : Child (overlay)
    , app_ (app)
    , id_(id)
    , sink_(app_.journal("Peer"), makePrefix(id))
    , p_sink_(app_.journal("Protocol"), makePrefix(id))
    , journal_ (sink_)
    , p_journal_(p_sink_)
    , ssl_bundle_(std::move(ssl_bundle))
    , socket_ (ssl_bundle_->socket)
    , stream_ (ssl_bundle_->stream)
    , strand_ (socket_.get_io_service())
    , timer_ (socket_.get_io_service())
    , remote_address_ (
        beast::IPAddressConversion::from_asio(remote_endpoint))
    , overlay_ (overlay)
    , m_inbound (true)
    , state_ (State::active)
    , sanity_ (Sanity::unknown)
    , insaneTime_ (clock_type::now())
    , publicKey_(publicKey)
    , creationTime_ (clock_type::now())
    , hello_(hello)
    , usage_(consumer)
    , fee_ (Resource::feeLightPeer)
    , slot_ (slot)
    , request_(std::move(request))
    , headers_(request_)
{
}

PeerImp::~PeerImp ()
{
    if (cluster())
    {
        JLOG(journal_.warn()) << name_ << " left cluster";
    }
    if (state_ == State::active)
        overlay_.onPeerDeactivate(id_);
    overlay_.peerFinder().on_closed (slot_);
    overlay_.remove (slot_);
}

void
PeerImp::run()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind (
            &PeerImp::run, shared_from_this()));
    {
        auto s = getVersion();
        if (boost::starts_with(s, "rippled-"))
        {
            s.erase(s.begin(), s.begin() + 8);
            beast::SemanticVersion v;
            if (v.parse(s))
            {
                beast::SemanticVersion av;
                av.parse("0.28.1-b7");
                hopsAware_ = v >= av;
            }
        }
    }
    if (m_inbound)
    {
        doAccept();
    }
    else
    {
        assert (state_ == State::active);
//XXX设置计时器：连接处于宽限期以供使用。
//XXX设置计时器：连接空闲（空闲可能因连接类型而异。）
        if ((hello_.has_ledgerclosed ()) && (
            hello_.ledgerclosed ().size () == (256 / 8)))
        {
            memcpy (closedLedgerHash_.begin (),
                hello_.ledgerclosed ().data (), 256 / 8);
            if ((hello_.has_ledgerprevious ()) &&
                (hello_.ledgerprevious ().size () == (256 / 8)))
            {
                memcpy (previousLedgerHash_.begin (),
                    hello_.ledgerprevious ().data (), 256 / 8);
                addLedger (previousLedgerHash_);
            }
            else
            {
                previousLedgerHash_.zero();
            }
        }
        doProtocolStart();
    }

//从对等端请求碎片信息
    protocol::TMGetShardInfo tmGS;
    tmGS.set_hops(0);
    send(std::make_shared<Message>(tmGS, protocol::mtGET_SHARD_INFO));

    setTimer();
}

void
PeerImp::stop()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind (
            &PeerImp::stop, shared_from_this()));
    if (socket_.is_open())
    {
//使用不同严重程度的理由是
//出站连接在我们的控制之下，可能会被记录下来。
//在更高的层次上，但是入站连接更多，并且
//不受控制，因此，为了防止日志淹没，严重性降低。
//
        if(m_inbound)
        {
            JLOG(journal_.debug()) << "Stop";
        }
        else
        {
            JLOG(journal_.info()) << "Stop";
        }
    }
    close();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
PeerImp::send (Message::pointer const& m)
{
    if (! strand_.running_in_this_thread())
        return strand_.post(std::bind (
            &PeerImp::send, shared_from_this(), m));
    if(gracefulClose_)
        return;
    if(detaching_)
        return;

    overlay_.reportTraffic (
        safe_cast<TrafficCount::category>(m->getCategory()),
        false, static_cast<int>(m->getBuffer().size()));

    auto sendq_size = send_queue_.size();

    if (sendq_size < Tuning::targetSendQueue)
    {
//检测不从其
//在连接方面，我们希望对等机
//小森克周期性地
        large_sendq_ = 0;
    }
    else if ((sendq_size % Tuning::sendQueueLogFreq) == 0)
    {
        JLOG (journal_.debug()) <<
            (name_.empty() ? remote_address_.to_string() : name_) <<
                " sendq: " << sendq_size;
    }

    send_queue_.push(m);

    if(sendq_size != 0)
        return;

    boost::asio::async_write (stream_, boost::asio::buffer(
        send_queue_.front()->getBuffer()), strand_.wrap(std::bind(
            &PeerImp::onWriteMessage, shared_from_this(),
                std::placeholders::_1,
                    std::placeholders::_2)));
}

void
PeerImp::charge (Resource::Charge const& fee)
{
    if ((usage_.charge(fee) == Resource::drop) &&
        usage_.disconnect() && strand_.running_in_this_thread())
    {
//切断连接
        overlay_.incPeerDisconnectCharges();
        fail("charge: Resources");
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

bool
PeerImp::crawl() const
{
    auto const iter = headers_.find("Crawl");
    if (iter == headers_.end())
        return false;
    return boost::beast::detail::iequals(iter->value(), "public");
}

std::string
PeerImp::getVersion() const
{
    if (hello_.has_fullversion ())
        return hello_.fullversion ();

    return std::string ();
}

Json::Value
PeerImp::json()
{
    Json::Value ret (Json::objectValue);

    ret[jss::public_key]   = toBase58 (
        TokenType::NodePublic, publicKey_);
    ret[jss::address]      = remote_address_.to_string();

    if (m_inbound)
        ret[jss::inbound] = true;

    if (cluster())
    {
        ret[jss::cluster] = true;

        if (!name_.empty ())
            ret[jss::name] = name_;
    }

    ret[jss::load] = usage_.balance ();

    if (hello_.has_fullversion ())
        ret[jss::version] = hello_.fullversion ();

    if (hello_.has_protoversion ())
    {
        auto protocol = BuildInfo::make_protocol (hello_.protoversion ());

        if (protocol != BuildInfo::getCurrentProtocol())
            ret[jss::protocol] = to_string (protocol);
    }

    {
        std::chrono::milliseconds latency;
        {
            std::lock_guard<std::mutex> sl (recentLock_);
            latency = latency_;
        }

        if (latency != std::chrono::milliseconds (-1))
            ret[jss::latency] = static_cast<Json::UInt> (latency.count());
    }

    ret[jss::uptime] = static_cast<Json::UInt>(
        std::chrono::duration_cast<std::chrono::seconds>(uptime()).count());

    std::uint32_t minSeq, maxSeq;
    ledgerRange(minSeq, maxSeq);

    if ((minSeq != 0) || (maxSeq != 0))
        ret[jss::complete_ledgers] = std::to_string(minSeq) +
            " - " + std::to_string(maxSeq);

    if (closedLedgerHash_ != beast::zero)
        ret[jss::ledger] = to_string (closedLedgerHash_);

    switch (sanity_.load ())
    {
        case Sanity::insane:
            ret[jss::sanity] = "insane";
            break;

        case Sanity::unknown:
            ret[jss::sanity] = "unknown";
            break;

        case Sanity::sane:
//这里没什么可做的
            break;
    }

    if (last_status_.has_newstatus ())
    {
        switch (last_status_.newstatus ())
        {
        case protocol::nsCONNECTING:
            ret[jss::status] = "connecting";
            break;

        case protocol::nsCONNECTED:
            ret[jss::status] = "connected";
            break;

        case protocol::nsMONITORING:
            ret[jss::status] = "monitoring";
            break;

        case protocol::nsVALIDATING:
            ret[jss::status] = "validating";
            break;

        case protocol::nsSHUTTING:
            ret[jss::status] = "shutting";
            break;

        default:
//我们真的想要这个吗？
            JLOG(p_journal_.warn()) <<
                "Unknown status: " << last_status_.newstatus ();
        }
    }

    return ret;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

bool
PeerImp::hasLedger (uint256 const& hash, std::uint32_t seq) const
{
    {
        std::lock_guard<std::mutex> sl(recentLock_);
        if ((seq != 0) && (seq >= minLedger_) && (seq <= maxLedger_) &&
                (sanity_.load() == Sanity::sane))
            return true;
        if (std::find(recentLedgers_.begin(),
                recentLedgers_.end(), hash) != recentLedgers_.end())
            return true;
    }

    if (seq >= app_.getNodeStore().earliestSeq())
        return hasShard(
            (seq - 1) / NodeStore::DatabaseShard::ledgersPerShardDefault);
    return false;
}

void
PeerImp::ledgerRange (std::uint32_t& minSeq,
    std::uint32_t& maxSeq) const
{
    std::lock_guard<std::mutex> sl(recentLock_);

    minSeq = minLedger_;
    maxSeq = maxLedger_;
}

bool
PeerImp::hasShard (std::uint32_t shardIndex) const
{
    std::lock_guard<std::mutex> l {shardInfoMutex_};
    auto const it {shardInfo_.find(publicKey_)};
    if (it != shardInfo_.end())
        return boost::icl::contains(it->second.shardIndexes, shardIndex);
    return false;
}

bool
PeerImp::hasTxSet (uint256 const& hash) const
{
    std::lock_guard<std::mutex> sl(recentLock_);
    return std::find (recentTxSets_.begin(),
        recentTxSets_.end(), hash) != recentTxSets_.end();
}

void
PeerImp::cycleStatus ()
{
    previousLedgerHash_ = closedLedgerHash_;
    closedLedgerHash_.zero ();
}

bool
PeerImp::supportsVersion (int version)
{
    return hello_.has_protoversion () && (hello_.protoversion () >= version);
}

bool
PeerImp::hasRange (std::uint32_t uMin, std::uint32_t uMax)
{
    return (sanity_ != Sanity::insane) && (uMin >= minLedger_) && (uMax <= maxLedger_);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
PeerImp::close()
{
    assert(strand_.running_in_this_thread());
    if (socket_.is_open())
    {
detaching_ = true; //贬低
        error_code ec;
        timer_.cancel(ec);
        socket_.close(ec);
        overlay_.incPeerDisconnect();
        if(m_inbound)
        {
            JLOG(journal_.debug()) << "Closed";
        }
        else
        {
            JLOG(journal_.info()) << "Closed";
        }
    }
}

void
PeerImp::fail(std::string const& reason)
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind (
            (void(Peer::*)(std::string const&))&PeerImp::fail,
                shared_from_this(), reason));
    if (socket_.is_open())
    {
        JLOG (journal_.warn()) <<
            (name_.empty() ? remote_address_.to_string() : name_) <<
                " failed: " << reason;
    }
    close();
}

void
PeerImp::fail(std::string const& name, error_code ec)
{
    assert(strand_.running_in_this_thread());
    if (socket_.is_open())
    {
        JLOG(journal_.warn()) << name << ": " << ec.message();
    }
    close();
}

boost::optional<RangeSet<std::uint32_t>>
PeerImp::getShardIndexes() const
{
    std::lock_guard<std::mutex> l {shardInfoMutex_};
    auto it{shardInfo_.find(publicKey_)};
    if (it != shardInfo_.end())
        return it->second.shardIndexes;
    return boost::none;
}

boost::optional<hash_map<PublicKey, PeerImp::ShardInfo>>
PeerImp::getPeerShardInfo() const
{
    std::lock_guard<std::mutex> l {shardInfoMutex_};
    if (!shardInfo_.empty())
        return shardInfo_;
    return boost::none;
}

void
PeerImp::gracefulClose()
{
    assert(strand_.running_in_this_thread());
    assert(socket_.is_open());
    assert(! gracefulClose_);
    gracefulClose_ = true;
#if 0
//刷新消息
    while(send_queue_.size() > 1)
        send_queue_.pop_back();
#endif
    if (send_queue_.size() > 0)
        return;
    setTimer();
    stream_.async_shutdown(strand_.wrap(std::bind(&PeerImp::onShutdown,
        shared_from_this(), std::placeholders::_1)));
}

void
PeerImp::setTimer()
{
    error_code ec;
    timer_.expires_from_now( std::chrono::seconds(
        Tuning::timerSeconds), ec);

    if (ec)
    {
        JLOG(journal_.error()) << "setTimer: " << ec.message();
        return;
    }
    timer_.async_wait(strand_.wrap(std::bind(&PeerImp::onTimer,
        shared_from_this(), std::placeholders::_1)));
}

//忽略错误代码的便利性
void
PeerImp::cancelTimer()
{
    error_code ec;
    timer_.cancel(ec);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::string
PeerImp::makePrefix(id_t id)
{
    std::stringstream ss;
    ss << "[" << std::setfill('0') << std::setw(3) << id << "] ";
    return ss.str();
}

void
PeerImp::onTimer (error_code const& ec)
{
    if (! socket_.is_open())
        return;

    if (ec == boost::asio::error::operation_aborted)
        return;

    if (ec)
    {
//这不应该发生
        JLOG(journal_.error()) << "onTimer: " << ec.message();
        return close();
    }

    if (large_sendq_++ >= Tuning::sendqIntervals)
    {
        fail ("Large send queue");
        return;
    }

    if (no_ping_++ >= Tuning::noPing)
    {
        fail ("No ping reply received");
        return;
    }

    if (lastPingSeq_ == 0)
    {
//使序列不可预测，足以使对等
//不能假装他们的潜伏期
        lastPingSeq_ = rand_int (65535);
        lastPingTime_ = clock_type::now();

        protocol::TMPing message;
        message.set_type (protocol::TMPing::ptPING);
        message.set_seq (lastPingSeq_);

        send (std::make_shared<Message> (
            message, protocol::mtPING));
    }
    else
    {
//我们有一个出色的ping，提高了延迟
        auto minLatency = std::chrono::duration_cast <std::chrono::milliseconds>
            (clock_type::now() - lastPingTime_);

        std::lock_guard<std::mutex> sl(recentLock_);

        if (latency_ < minLatency)
            latency_ = minLatency;
    }

    setTimer();
}

void
PeerImp::onShutdown(error_code ec)
{
    cancelTimer();
//如果我们没有得到EOF，那就出了问题。
    if (! ec)
    {
        JLOG(journal_.error()) << "onShutdown: expected error condition";
        return close();
    }
    if (ec != boost::asio::error::eof)
        return fail("onShutdown", ec);
    close();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void PeerImp::doAccept()
{
    assert(read_buffer_.size() == 0);
//断言（请求升级）；

    JLOG(journal_.debug()) << "doAccept: " << remote_address_;

    auto sharedValue = makeSharedValue(
        ssl_bundle_->stream.native_handle(), journal_);
//这不应该失败，因为我们已经计算了
//套印格式impl中的共享值成功
    if(! sharedValue)
        return fail("makeSharedValue: Unexpected failure");

//要将头应用于连接状态。

    boost::beast::ostream(write_buffer_) << makeResponse(
        ! overlay_.peerFinder().config().peerPrivate,
            request_, remote_address_, *sharedValue);

    auto const protocol = BuildInfo::make_protocol(hello_.protoversion());
    JLOG(journal_.info()) << "Protocol: " << to_string(protocol);
    JLOG(journal_.info()) <<
        "Public Key: " << toBase58 (
            TokenType::NodePublic,
            publicKey_);
    if (auto member = app_.cluster().member(publicKey_))
    {
        name_ = *member;
        JLOG(journal_.info()) << "Cluster name: " << name_;
    }

    overlay_.activate(shared_from_this());

//XXX设置计时器：连接处于宽限期以供使用。
//XXX设置计时器：连接空闲（空闲可能因连接类型而异。）
    if ((hello_.has_ledgerclosed ()) && (
        hello_.ledgerclosed ().size () == (256 / 8)))
    {
        memcpy (closedLedgerHash_.begin (),
            hello_.ledgerclosed ().data (), 256 / 8);
        if ((hello_.has_ledgerprevious ()) &&
            (hello_.ledgerprevious ().size () == (256 / 8)))
        {
            memcpy (previousLedgerHash_.begin (),
                hello_.ledgerprevious ().data (), 256 / 8);
            addLedger (previousLedgerHash_);
        }
        else
        {
            previousLedgerHash_.zero();
        }
    }

    onWriteResponse(error_code(), 0);
}

http_response_type
PeerImp::makeResponse (bool crawl,
    http_request_type const& req,
    beast::IP::Endpoint remote,
    uint256 const& sharedValue)
{
    http_response_type resp;
    resp.result(boost::beast::http::status::switching_protocols);
    resp.version(req.version());
    resp.insert("Connection", "Upgrade");
    resp.insert("Upgrade", "RTXP/1.2");
    resp.insert("Connect-As", "Peer");
    resp.insert("Server", BuildInfo::getFullVersionString());
    resp.insert("Crawl", crawl ? "public" : "private");
    protocol::TMHello hello = buildHello(sharedValue,
        overlay_.setup().public_ip, remote, app_);
    appendHello(resp, hello);
    return resp;
}

//重复调用以发送响应中的字节
void
PeerImp::onWriteResponse (error_code ec, std::size_t bytes_transferred)
{
    if(! socket_.is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(ec)
        return fail("onWriteResponse", ec);
    if(auto stream = journal_.trace())
    {
        if (bytes_transferred > 0)
            stream <<
                "onWriteResponse: " << bytes_transferred << " bytes";
        else
            stream << "onWriteResponse";
    }

    write_buffer_.consume (bytes_transferred);
    if (write_buffer_.size() == 0)
        return doProtocolStart();

    stream_.async_write_some (write_buffer_.data(),
        strand_.wrap (std::bind (&PeerImp::onWriteResponse,
            shared_from_this(), std::placeholders::_1,
                std::placeholders::_2)));
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//协议逻辑

void
PeerImp::doProtocolStart()
{
    onReadMessage(error_code(), 0);

    protocol::TMManifests tm;

    app_.validatorManifests ().for_each_manifest (
        [&tm](std::size_t s){tm.mutable_list()->Reserve(s);},
        [&tm, &hr = app_.getHashRouter()](Manifest const& manifest)
        {
            auto const& s = manifest.serialized;
            auto& tm_e = *tm.add_list();
            tm_e.set_stobject(s.data(), s.size());
            hr.addSuppression(manifest.hash());
        });

    if (tm.list_size() > 0)
    {
        auto m = std::make_shared<Message>(tm, protocol::mtMANIFESTS);
        send (m);
    }
}

//用协议消息数据重复调用
void
PeerImp::onReadMessage (error_code ec, std::size_t bytes_transferred)
{
    if(! socket_.is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(ec == boost::asio::error::eof)
    {
        JLOG(journal_.info()) << "EOF";
        return gracefulClose();
    }
    if(ec)
        return fail("onReadMessage", ec);
    if(auto stream = journal_.trace())
    {
        if (bytes_transferred > 0)
            stream <<
                "onReadMessage: " << bytes_transferred << " bytes";
        else
            stream << "onReadMessage";
    }

    read_buffer_.commit (bytes_transferred);

    while (read_buffer_.size() > 0)
    {
        std::size_t bytes_consumed;
        std::tie(bytes_consumed, ec) = invokeProtocolMessage(
            read_buffer_.data(), *this);
        if (ec)
            return fail("onReadMessage", ec);
        if (! stream_.next_layer().is_open())
            return;
        if(gracefulClose_)
            return;
        if (bytes_consumed == 0)
            break;
        read_buffer_.consume (bytes_consumed);
    }
//仅写入时超时
    stream_.async_read_some (read_buffer_.prepare (Tuning::readBufferBytes),
        strand_.wrap (std::bind (&PeerImp::onReadMessage,
            shared_from_this(), std::placeholders::_1,
                std::placeholders::_2)));
}

void
PeerImp::onWriteMessage (error_code ec, std::size_t bytes_transferred)
{
    if(! socket_.is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(ec)
        return fail("onWriteMessage", ec);
    if(auto stream = journal_.trace())
    {
        if (bytes_transferred > 0)
            stream <<
                "onWriteMessage: " << bytes_transferred << " bytes";
        else
            stream << "onWriteMessage";
    }

    assert(! send_queue_.empty());
    send_queue_.pop();
    if (! send_queue_.empty())
    {
//仅写入时超时
        return boost::asio::async_write (stream_, boost::asio::buffer(
            send_queue_.front()->getBuffer()), strand_.wrap(std::bind(
                &PeerImp::onWriteMessage, shared_from_this(),
                    std::placeholders::_1,
                        std::placeholders::_2)));
    }

    if (gracefulClose_)
    {
        return stream_.async_shutdown(strand_.wrap(std::bind(
            &PeerImp::onShutdown, shared_from_this(),
                std::placeholders::_1)));
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//协议处理器
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PeerImp::error_code
PeerImp::onMessageUnknown (std::uint16_t type)
{
    error_code ec;
//托多
    return ec;
}

PeerImp::error_code
PeerImp::onMessageBegin (std::uint16_t type,
    std::shared_ptr <::google::protobuf::Message> const& m,
    std::size_t size)
{
    load_event_ = app_.getJobQueue ().makeLoadEvent (
        jtPEER, protocolMessageName(type));
    fee_ = Resource::feeLightPeer;
    overlay_.reportTraffic (TrafficCount::categorize (*m, type, true),
        true, static_cast<int>(size));
    return error_code{};
}

void
PeerImp::onMessageEnd (std::uint16_t,
    std::shared_ptr <::google::protobuf::Message> const&)
{
    load_event_.reset();
    charge (fee_);
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMHello> const& m)
{
    fail("Deprecated TMHello");
}

void
PeerImp::onMessage (std::shared_ptr<protocol::TMManifests> const& m)
{
//vvalco正确的工作类型是什么？
    auto that = shared_from_this();
    app_.getJobQueue().addJob (
        jtVALIDATION_ut, "receiveManifests",
        [this, that, m] (Job&) { overlay_.onManifests(m, that); });
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMPing> const& m)
{
    if (m->type () == protocol::TMPing::ptPING)
    {
//我们收到了一个ping请求，用pong回复
        fee_ = Resource::feeMediumBurdenPeer;
        m->set_type (protocol::TMPing::ptPONG);
        send (std::make_shared<Message> (*m, protocol::mtPING));

        return;
    }

    if ((m->type () == protocol::TMPing::ptPONG) && m->has_seq ())
    {
//我们收到一个pong，更新我们的延迟估计
        auto unknownLatency = std::chrono::milliseconds (-1);

        std::lock_guard<std::mutex> sl(recentLock_);

        if ((lastPingSeq_ != 0) && (m->seq () == lastPingSeq_))
        {
            no_ping_ = 0;
            auto estimate = std::chrono::duration_cast <std::chrono::milliseconds>
                (clock_type::now() - lastPingTime_);
            if (latency_ == unknownLatency)
                latency_ = estimate;
            else
                latency_ = (latency_ * 7 + estimate) / 8;
        }
        else
            latency_ = unknownLatency;
        lastPingSeq_ = 0;

        return;
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMCluster> const& m)
{
//vfalco注：我认为我们应该立即放弃同行。
    if (! cluster())
    {
        fee_ = Resource::feeUnwantedData;
        return;
    }

    for (int i = 0; i < m->clusternodes().size(); ++i)
    {
        protocol::TMClusterNode const& node = m->clusternodes(i);

        std::string name;
        if (node.has_nodename())
            name = node.nodename();

        auto const publicKey = parseBase58<PublicKey>(
            TokenType::NodePublic, node.publickey());

//注意，如果
//他们给我们发送了一个无法解析的公钥
        if (publicKey)
        {
            auto const reportTime =
                NetClock::time_point{
                    NetClock::duration{node.reporttime()}};

            app_.cluster().update(
                *publicKey,
                name,
                node.nodeload(),
                reportTime);
        }
    }

    int loadSources = m->loadsources().size();
    if (loadSources != 0)
    {
        Resource::Gossip gossip;
        gossip.items.reserve (loadSources);
        for (int i = 0; i < m->loadsources().size(); ++i)
        {
            protocol::TMLoadSource const& node = m->loadsources (i);
            Resource::Gossip::Item item;
            item.address = beast::IP::Endpoint::from_string (node.name());
            item.balance = node.cost();
            if (item.address != beast::IP::Endpoint())
                gossip.items.push_back(item);
        }
        overlay_.resourceManager().importConsumers (name_, gossip);
    }

//计算集群费用：
    auto const thresh = app_.timeKeeper().now() - 90s;
    std::uint32_t clusterFee = 0;

    std::vector<std::uint32_t> fees;
    fees.reserve (app_.cluster().size());

    app_.cluster().for_each(
        [&fees,thresh](ClusterNode const& status)
        {
            if (status.getReportTime() >= thresh)
                fees.push_back (status.getLoadFee ());
        });

    if (!fees.empty())
    {
        auto const index = fees.size() / 2;
        std::nth_element (
            fees.begin(),
            fees.begin () + index,
            fees.end());
        clusterFee = fees[index];
    }

    app_.getFeeTrack().setClusterFee(clusterFee);
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMGetShardInfo> const& m)
{
    if (m->hops() > csHopLimit || m->peerchain_size() > csHopLimit)
    {
        fee_ = Resource::feeInvalidRequest;
        JLOG(p_journal_.warn()) <<
            (m->hops() > csHopLimit ?
            "Hops (" + std::to_string(m->hops()) + ") exceed limit" :
            "Invalid Peerchain");
        return;
    }

//回复我们可能有的碎片信息
    if (auto shardStore = app_.getShardStore())
    {
        fee_ = Resource::feeLightPeer;
        auto shards {shardStore->getCompleteShards()};
        if (!shards.empty())
        {
            protocol::TMShardInfo reply;
            reply.set_shardindexes(shards);

            if (m->has_lastlink())
                reply.set_lastlink(true);

            if (m->peerchain_size() > 0)
                *reply.mutable_peerchain() = m->peerchain();

            send(std::make_shared<Message>(reply, protocol::mtSHARD_INFO));

            JLOG(p_journal_.trace()) <<
                "Sent shard indexes " << shards;
        }
    }

//向对等方中继请求
    if (m->hops() > 0)
    {
        fee_ = Resource::feeMediumBurdenPeer;

        m->set_hops(m->hops() - 1);
        if (m->hops() == 0)
            m->set_lastlink(true);

        m->add_peerchain(id());
        overlay_.foreach(send_if_not(
            std::make_shared<Message>(*m, protocol::mtGET_SHARD_INFO),
            match_peer(this)));
    }
}

void
PeerImp::onMessage(std::shared_ptr <protocol::TMShardInfo> const& m)
{
    if (m->shardindexes().empty() || m->peerchain_size() > csHopLimit)
    {
        fee_ = Resource::feeBadData;
        JLOG(p_journal_.warn()) <<
            (m->shardindexes().empty() ?
            "Missing shard indexes" :
            "Invalid Peerchain");
        return;
    }

//检查是否应将消息转发给其他对等方
    if (m->peerchain_size() > 0)
    {
        auto const peerId {m->peerchain(m->peerchain_size() - 1)};
        if (auto peer = overlay_.findPeerByShortID(peerId))
        {
            if (!m->has_nodepubkey())
                m->set_nodepubkey(publicKey_.data(), publicKey_.size());

            if (!m->has_endpoint())
            {
//检查对等端是否公开共享IP
                if (crawl())
                    m->set_endpoint(remote_address_.address().to_string());
                else
                    m->set_endpoint("0");
            }

            m->mutable_peerchain()->RemoveLast();
            peer->send(std::make_shared<Message>(*m, protocol::mtSHARD_INFO));

            JLOG(p_journal_.trace()) <<
                "Relayed TMShardInfo to peer with IP " <<
                remote_address_.address().to_string();
        }
        else
        {
//对等机不再可用，因此中继结束
            fee_ = Resource::feeUnwantedData;
            JLOG(p_journal_.info()) <<
                "Unable to route shard info";
        }
        return;
    }

//分析在碎片信息中接收到的碎片索引
    RangeSet<std::uint32_t> shardIndexes;
    {
        std::uint32_t earliestShard;
        boost::optional<std::uint32_t> latestShard;
        {
            auto const curLedgerSeq {
                app_.getLedgerMaster().getCurrentLedgerIndex()};
            if (auto shardStore = app_.getShardStore())
            {
                earliestShard = shardStore->earliestShardIndex();
                if (curLedgerSeq >= shardStore->earliestSeq())
                    latestShard = shardStore->seqToShardIndex(curLedgerSeq);
            }
            else
            {
                auto const earliestSeq {app_.getNodeStore().earliestSeq()};
                earliestShard = NodeStore::seqToShardIndex(earliestSeq);
                if (curLedgerSeq >= earliestSeq)
                    latestShard = NodeStore::seqToShardIndex(curLedgerSeq);
            }
        }

        auto getIndex = [this, &earliestShard, &latestShard]
            (std::string const& s) -> boost::optional<std::uint32_t>
        {
            std::uint32_t shardIndex;
            if (!beast::lexicalCastChecked(shardIndex, s))
            {
                fee_ = Resource::feeBadData;
                return boost::none;
            }
            if (shardIndex < earliestShard ||
                (latestShard && shardIndex > latestShard))
            {
                fee_ = Resource::feeBadData;
                JLOG(p_journal_.error()) <<
                    "Invalid shard index " << shardIndex;
                return boost::none;
            }
            return shardIndex;
        };

        std::vector<std::string> tokens;
        boost::split(tokens, m->shardindexes(),
            boost::algorithm::is_any_of(","));
        std::vector<std::string> indexes;
        for (auto const& t : tokens)
        {
            indexes.clear();
            boost::split(indexes, t, boost::algorithm::is_any_of("-"));
            switch (indexes.size())
            {
            case 1:
            {
                auto const first {getIndex(indexes.front())};
                if (!first)
                    return;
                shardIndexes.insert(*first);
                break;
            }
            case 2:
            {
                auto const first {getIndex(indexes.front())};
                if (!first)
                    return;
                auto const second {getIndex(indexes.back())};
                if (!second)
                    return;
                shardIndexes.insert(range(*first, *second));
                break;
            }
            default:
                fee_ = Resource::feeBadData;
                return;
            }
        }
    }

//获取报告碎片信息的节点的公钥
    PublicKey publicKey;
    if (m->has_nodepubkey())
        publicKey = PublicKey(makeSlice(m->nodepubkey()));
    else
        publicKey = publicKey_;

//获取报告碎片信息的节点的IP
    beast::IP::Endpoint endpoint;
    if (m->has_endpoint())
    {
        if (m->endpoint() != "0")
        {
            auto result {
                beast::IP::Endpoint::from_string_checked(m->endpoint())};
            if (!result.second)
            {
                fee_ = Resource::feeBadData;
                JLOG(p_journal_.warn()) <<
                    "failed to parse incoming endpoint: {" <<
                    m->endpoint() << "}";
                return;
            }
            endpoint = std::move(result.first);
        }
    }
else if (crawl()) //检查对等端是否公开共享IP
        endpoint = remote_address_;

    {
        std::lock_guard<std::mutex> l {shardInfoMutex_};
        auto it {shardInfo_.find(publicKey)};
        if (it != shardInfo_.end())
        {
//更新节点的IP地址
            it->second.endpoint = std::move(endpoint);

//加入碎片索引范围集
            it->second.shardIndexes += shardIndexes;
        }
        else
        {
//添加新节点
            ShardInfo shardInfo;
            shardInfo.endpoint = std::move(endpoint);
            shardInfo.shardIndexes = std::move(shardIndexes);
            shardInfo_.emplace(publicKey, std::move(shardInfo));
        }
    }

    JLOG(p_journal_.trace()) <<
        "Consumed TMShardInfo originating from public key " <<
        toBase58(TokenType::NodePublic, publicKey) <<
        " shard indexes " << m->shardindexes();

    if (m->has_lastlink())
        overlay_.lastLink(id_);
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMGetPeers> const& m)
{
//由于peerfinder和
//我们不再对此作出回应。
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMPeers> const& m)
{
//由于peerfinder和
//我们不再处理它了。
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMEndpoints> const& m)
{
    if (sanity_.load() != Sanity::sane)
    {
//不允许来自未知对等端的终结点正常
        return;
    }

    std::vector <PeerFinder::Endpoint> endpoints;

    if (m->endpoints_v2().size())
    {
        endpoints.reserve (m->endpoints_v2().size());
        for (auto const& tm : m->endpoints_v2 ())
        {
//这些端点字符串支持IPv4和IPv6
            auto result = beast::IP::Endpoint::from_string_checked(tm.endpoint());
            if (! result.second)
            {
                JLOG(p_journal_.error()) <<
                    "failed to parse incoming endpoint: {" <<
                    tm.endpoint() << "}";
                continue;
            }

//如果hops==0，则此端点描述所连接的对等端
//在这种情况下，我们采用
//套接字并将其存储在IP:：Endpoint中。如果这是第一次
//时间，然后我们将验证他们的侦听器是否可以接收传入的
//通过执行连接测试。如果跳数大于0，那么我们只是
//记下我们的地址/端口

            endpoints.emplace_back(
                tm.hops() > 0 ?
                    result.first :
                    remote_address_.at_port(result.first.port()),
                tm.hops());
            JLOG(p_journal_.trace()) <<
                "got v2 EP: " << endpoints.back().address <<
                ", hops = " << endpoints.back().hops;
        }
    }
    else
    {
//一旦整个网络使用
//endpoint_v2（）项（字符串）
        endpoints.reserve (m->endpoints().size());
        for (int i = 0; i < m->endpoints ().size (); ++i)
        {
            PeerFinder::Endpoint endpoint;
            protocol::TMEndpoint const& tm (m->endpoints(i));

//酒花
            endpoint.hops = tm.hops();

//IPv4
            if (endpoint.hops > 0)
            {
                in_addr addr;
                addr.s_addr = tm.ipv4().ipv4();
                beast::IP::AddressV4 v4 (ntohl (addr.s_addr));
                endpoint.address = beast::IP::Endpoint (v4, tm.ipv4().ipv4port ());
            }
            else
            {
//此端点描述了我们所连接的对等端。
//我们将获取插座上的远程地址，
//将其存储在IP:：Endpoint中。如果这是第一次，
//然后我们将验证他们的侦听器是否可以接收传入的
//通过执行连接测试。
//
                endpoint.address = remote_address_.at_port (
                    tm.ipv4().ipv4port ());
            }
            endpoints.push_back (endpoint);
            JLOG(p_journal_.trace()) <<
                "got v1 EP: " << endpoints.back().address <<
                ", hops = " << endpoints.back().hops;
        }
    }

    if (! endpoints.empty())
        overlay_.peerFinder().on_endpoints (slot_, endpoints);
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMTransaction> const& m)
{

    if (sanity_.load() == Sanity::insane)
        return;

    if (app_.getOPs().isNeedNetworkLedger ())
    {
//如果我们从未同步过，我们就无能为力了
//有交易的
        JLOG(p_journal_.debug()) << "Ignoring incoming transaction: " <<
            "Need network ledger";
        return;
    }

    SerialIter sit (makeSlice(m->rawtransaction()));

    try
    {
        auto stx = std::make_shared<STTx const>(sit);
        uint256 txID = stx->getTransactionID ();

        int flags;
        constexpr std::chrono::seconds tx_interval = 10s;

        if (! app_.getHashRouter ().shouldProcess (txID, id_, flags,
            tx_interval))
        {
//我们最近看到了这笔交易
            if (flags & SF_BAD)
            {
                fee_ = Resource::feeInvalidSignature;
                JLOG(p_journal_.debug()) << "Ignoring known bad tx " <<
                    txID;
            }

            return;
        }

        JLOG(p_journal_.debug()) << "Got tx " << txID;

        bool checkSignature = true;
        if (cluster())
        {
            if (! m->has_deferred () || ! m->deferred ())
            {
//如果我们信任的服务器，则跳过本地检查
//将交易记录放入其未结分类帐
                flags |= SF_TRUSTED;
            }

            if (app_.getValidationPublicKey().empty())
            {
//现在，要有偏执狂和每个验证器
//检查每个事务，无论其来源如何
                checkSignature = false;
            }
        }

//作业队列中要具有的最大事务数。
        constexpr int max_transactions = 250;
        if (app_.getJobQueue().getJobCount(jtTRANSACTION) > max_transactions)
        {
            overlay_.incJqTransOverflow();
            JLOG(p_journal_.info()) << "Transaction queue is full";
        }
        else if (app_.getLedgerMaster().getValidatedLedgerAge() > 4min)
        {
            JLOG(p_journal_.trace()) << "No new transactions until synchronized";
        }
        else
        {
            app_.getJobQueue ().addJob (
                jtTRANSACTION, "recvTransaction->checkTransaction",
                [weak = std::weak_ptr<PeerImp>(shared_from_this()),
                flags, checkSignature, stx] (Job&) {
                    if (auto peer = weak.lock())
                        peer->checkTransaction(flags,
                            checkSignature, stx);
                });
        }
    }
    catch (std::exception const&)
    {
        JLOG(p_journal_.warn()) << "Transaction invalid: " <<
            strHex(m->rawtransaction ());
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMGetLedger> const& m)
{
    fee_ = Resource::feeMediumBurdenPeer;
    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue().addJob (
        jtLEDGER_REQ, "recvGetLedger",
        [weak, m] (Job&) {
            if (auto peer = weak.lock())
                peer->getLedger(m);
        });
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMLedgerData> const& m)
{
    protocol::TMLedgerData& packet = *m;

    if (m->nodes ().size () <= 0)
    {
        JLOG(p_journal_.warn()) << "Ledger/TXset data with no nodes";
        return;
    }

    if (m->has_requestcookie ())
    {
        std::shared_ptr<Peer> target = overlay_.findPeerByShortID (m->requestcookie ());
        if (target)
        {
            m->clear_requestcookie ();
            target->send (std::make_shared<Message> (
                packet, protocol::mtLEDGER_DATA));
        }
        else
        {
            JLOG(p_journal_.info()) << "Unable to route TX/ledger data reply";
            fee_ = Resource::feeUnwantedData;
        }
        return;
    }

    uint256 hash;

    if (m->ledgerhash ().size () != 32)
    {
        JLOG(p_journal_.warn()) << "TX candidate reply with invalid hash size";
        fee_ = Resource::feeInvalidRequest;
        return;
    }

    memcpy (hash.begin (), m->ledgerhash ().data (), 32);

    if (m->type () == protocol::liTS_CANDIDATE)
    {
//获取候选事务集的数据
        std::weak_ptr<PeerImp> weak = shared_from_this();
        auto& journal = p_journal_;
        app_.getJobQueue().addJob(
            jtTXN_DATA, "recvPeerData",
            [weak, hash, journal, m] (Job&) {
                if (auto peer = weak.lock())
                    peer->peerTXData(hash, m, journal);
            });
        return;
    }

    if (!app_.getInboundLedgers ().gotLedgerData (
        hash, shared_from_this(), m))
    {
        JLOG(p_journal_.trace()) << "Got data for unwanted ledger";
        fee_ = Resource::feeUnwantedData;
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMProposeSet> const& m)
{
    protocol::TMProposeSet& set = *m;

    if (set.has_hops() && ! slot_->cluster())
        set.set_hops(set.hops() + 1);

    auto const type = publicKeyType(
        makeSlice(set.nodepubkey()));

//vfalc幻数不好
//将其转入验证函数
    if ((! type) ||
        (set.currenttxhash ().size () != 32) ||
        (set.signature ().size () < 56) ||
        (set.signature ().size () > 128)
    )
    {
        JLOG(p_journal_.warn()) << "Proposal: malformed";
        fee_ = Resource::feeInvalidSignature;
        return;
    }

    if (set.previousledger ().size () != 32)
    {
        JLOG(p_journal_.warn()) << "Proposal: malformed";
        fee_ = Resource::feeInvalidRequest;
        return;
    }

    PublicKey const publicKey (makeSlice(set.nodepubkey()));
    NetClock::time_point const closeTime { NetClock::duration{set.closetime()} };
    Slice signature (set.signature().data(), set.signature ().size());

    uint256 proposeHash, prevLedger;
    memcpy (proposeHash.begin (), set.currenttxhash ().data (), 32);
    memcpy (prevLedger.begin (), set.previousledger ().data (), 32);

    uint256 suppression = proposalUniqueId (
        proposeHash, prevLedger, set.proposeseq(),
        closeTime, publicKey.slice(), signature);

    if (! app_.getHashRouter ().addSuppressionPeer (suppression, id_))
    {
        JLOG(p_journal_.trace()) << "Proposal: duplicate";
        return;
    }

    auto const isTrusted = app_.validators().trusted (publicKey);

    if (!isTrusted)
    {
        if (sanity_.load() == Sanity::insane)
        {
            JLOG(p_journal_.debug()) << "Proposal: Dropping UNTRUSTED (insane)";
            return;
        }

        if (! cluster() && app_.getFeeTrack ().isLoadedLocal())
        {
            JLOG(p_journal_.debug()) << "Proposal: Dropping UNTRUSTED (load)";
            return;
        }
    }

    JLOG(p_journal_.trace()) <<
        "Proposal: " << (isTrusted ? "trusted" : "UNTRUSTED");

    auto proposal = RCLCxPeerPos(
        publicKey,
        signature,
        suppression,
        RCLCxPeerPos::Proposal{
            prevLedger,
            set.proposeseq(),
            proposeHash,
            closeTime,
            app_.timeKeeper().closeTime(),
            calcNodeID(app_.validatorManifests().getMasterKey(publicKey))});

    std::weak_ptr<PeerImp> weak = shared_from_this();
    app_.getJobQueue ().addJob (
        isTrusted ? jtPROPOSAL_t : jtPROPOSAL_ut, "recvPropose->checkPropose",
        [weak, m, proposal] (Job& job) {
            if (auto peer = weak.lock())
                peer->checkPropose(job, m, proposal);
        });
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMStatusChange> const& m)
{
    JLOG(p_journal_.trace()) << "Status: Change";

    if (!m->has_networktime ())
        m->set_networktime (app_.timeKeeper().now().time_since_epoch().count());

    if (!last_status_.has_newstatus () || m->has_newstatus ())
        last_status_ = *m;
    else
    {
//保留旧状态
        protocol::NodeStatus status = last_status_.newstatus ();
        last_status_ = *m;
        m->set_newstatus (status);
    }

    if (m->newevent () == protocol::neLOST_SYNC)
    {
        if (!closedLedgerHash_.isZero ())
        {
            JLOG(p_journal_.debug()) << "Status: Out of sync";
            closedLedgerHash_.zero ();
        }

        previousLedgerHash_.zero ();
        return;
    }

    if (m->has_ledgerhash () && (m->ledgerhash ().size () == (256 / 8)))
    {
//同龄人已更改分类帐
        memcpy (closedLedgerHash_.begin (), m->ledgerhash ().data (), 256 / 8);
        addLedger (closedLedgerHash_);
        JLOG(p_journal_.debug()) << "LCL is " << closedLedgerHash_;
    }
    else
    {
        JLOG(p_journal_.debug()) << "Status: No ledger";
        closedLedgerHash_.zero ();
    }

    if (m->has_ledgerhashprevious () &&
        m->ledgerhashprevious ().size () == (256 / 8))
    {
        memcpy (previousLedgerHash_.begin (),
            m->ledgerhashprevious ().data (), 256 / 8);
        addLedger (previousLedgerHash_);
    }
    else previousLedgerHash_.zero ();

    if (m->has_firstseq () && m->has_lastseq())
    {
        minLedger_ = m->firstseq ();
        maxLedger_ = m->lastseq ();

//vfalc是否仍需要此解决方案？
//处理一些报告顺序错误的服务器
        if (minLedger_ == 0)
            maxLedger_ = 0;
        if (maxLedger_ == 0)
            minLedger_ = 0;
    }

    if (m->has_ledgerseq() &&
        app_.getLedgerMaster().getValidatedLedgerAge() < 2min)
    {
        checkSanity (m->ledgerseq(), app_.getLedgerMaster().getValidLedgerIndex());
    }

    app_.getOPs().pubPeerStatus (
        [=]() -> Json::Value
        {
            Json::Value j = Json::objectValue;

            if (m->has_newstatus ())
            {
               switch (m->newstatus ())
               {
                   case protocol::nsCONNECTING:
                       j[jss::status] = "CONNECTING";
                       break;
                   case protocol::nsCONNECTED:
                       j[jss::status] = "CONNECTED";
                       break;
                   case protocol::nsMONITORING:
                       j[jss::status] = "MONITORING";
                       break;
                   case protocol::nsVALIDATING:
                       j[jss::status] = "VALIDATING";
                       break;
                   case protocol::nsSHUTTING:
                       j[jss::status] = "SHUTTING";
                       break;
               }
            }

            if (m->has_newevent())
            {
                switch (m->newevent ())
                {
                    case protocol::neCLOSING_LEDGER:
                        j[jss::action] = "CLOSING_LEDGER";
                        break;
                    case protocol::neACCEPTED_LEDGER:
                        j[jss::action] = "ACCEPTED_LEDGER";
                        break;
                    case protocol::neSWITCHED_LEDGER:
                        j[jss::action] = "SWITCHED_LEDGER";
                        break;
                    case protocol::neLOST_SYNC:
                        j[jss::action] = "LOST_SYNC";
                        break;
                }
            }

            if (m->has_ledgerseq ())
            {
                j[jss::ledger_index] = m->ledgerseq();
            }

            if (m->has_ledgerhash ())
            {
                j[jss::ledger_hash] = to_string (closedLedgerHash_);
            }

            if (m->has_networktime ())
            {
                j[jss::date] = Json::UInt (m->networktime());
            }

            if (m->has_firstseq () && m->has_lastseq ())
            {
                j[jss::ledger_index_min] =
                    Json::UInt (m->firstseq ());
                j[jss::ledger_index_max] =
                    Json::UInt (m->lastseq ());
            }

            return j;
        });
}

void
PeerImp::checkSanity (std::uint32_t validationSeq)
{
    std::uint32_t serverSeq;
    {
//提取最高的序列号
//这个同伴的账本
        std::lock_guard<std::mutex> sl (recentLock_);

        serverSeq = maxLedger_;
    }
    if (serverSeq != 0)
    {
//将对等方的分类帐序列与
//最近验证的分类帐的序列
        checkSanity (serverSeq, validationSeq);
    }
}

void
PeerImp::checkSanity (std::uint32_t seq1, std::uint32_t seq2)
{
        int diff = std::max (seq1, seq2) - std::min (seq1, seq2);

        if (diff < Tuning::saneLedgerLimit)
        {
//对等方的分类帐序列接近验证的
            sanity_ = Sanity::sane;
        }

        if ((diff > Tuning::insaneLedgerLimit) && (sanity_.load() != Sanity::insane))
        {
//对等方的分类帐序列与验证序列相差甚远。
            std::lock_guard<std::mutex> sl(recentLock_);

            sanity_ = Sanity::insane;
            insaneTime_ = clock_type::now();
        }
}

//是否应拒绝此连接
//被认为是失败
void PeerImp::check ()
{
    if (m_inbound || (sanity_.load() == Sanity::sane))
        return;

    clock_type::time_point insaneTime;
    {
        std::lock_guard<std::mutex> sl(recentLock_);

        insaneTime = insaneTime_;
    }

    bool reject = false;

    if (sanity_.load() == Sanity::insane)
        reject = (insaneTime - clock_type::now())
            > std::chrono::seconds (Tuning::maxInsaneTime);

    if (sanity_.load() == Sanity::unknown)
        reject = (insaneTime - clock_type::now())
            > std::chrono::seconds (Tuning::maxUnknownTime);

    if (reject)
    {
        overlay_.peerFinder().on_failure (slot_);
        strand_.post (std::bind (
            (void (PeerImp::*)(std::string const&)) &PeerImp::fail,
                shared_from_this(), "Not useful"));
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMHaveTransactionSet> const& m)
{
    uint256 hashes;

    if (m->hash ().size () != (256 / 8))
    {
        fee_ = Resource::feeInvalidRequest;
        return;
    }

    uint256 hash;

//vvalco todo在整个程序中不应该使用memcpy（）。
//托多清理这个魔法数字
//
    memcpy (hash.begin (), m->hash ().data (), 32);

    if (m->status () == protocol::tsHAVE)
    {
        std::lock_guard<std::mutex> sl(recentLock_);

        if (std::find (recentTxSets_.begin (),
            recentTxSets_.end (), hash) != recentTxSets_.end ())
        {
            fee_ = Resource::feeUnwantedData;
            return;
        }

        if (recentTxSets_.size () == 128)
            recentTxSets_.pop_front ();

        recentTxSets_.push_back (hash);
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMValidation> const& m)
{
    auto const closeTime = app_.timeKeeper().closeTime();

    if (m->has_hops() && ! slot_->cluster())
        m->set_hops(m->hops() + 1);

    if (m->validation ().size () < 50)
    {
        JLOG(p_journal_.warn()) << "Validation: Too small";
        fee_ = Resource::feeInvalidRequest;
        return;
    }

    try
    {
        STValidation::pointer val;
        {
            SerialIter sit (makeSlice(m->validation()));
            val = std::make_shared<STValidation>(
                std::ref(sit),
                [this](PublicKey const& pk) {
                    return calcNodeID(
                        app_.validatorManifests().getMasterKey(pk));
                },
                false);
            val->setSeen (closeTime);
        }

        if (! isCurrent(app_.getValidations().parms(),
            app_.timeKeeper().closeTime(),
            val->getSignTime(),
            val->getSeenTime()))
        {
            JLOG(p_journal_.trace()) << "Validation: Not current";
            fee_ = Resource::feeUnwantedData;
            return;
        }

        if (! app_.getHashRouter ().addSuppressionPeer(
            sha512Half(makeSlice(m->validation())), id_))
        {
            JLOG(p_journal_.trace()) << "Validation: duplicate";
            return;
        }

        auto const isTrusted =
            app_.validators().trusted(val->getSignerPublic ());

        if (!isTrusted && (sanity_.load () == Sanity::insane))
        {
            JLOG(p_journal_.debug()) <<
                "Validation: dropping untrusted from insane peer";
        }
        if (isTrusted || cluster() ||
            ! app_.getFeeTrack ().isLoadedLocal ())
        {
            std::weak_ptr<PeerImp> weak = shared_from_this();
            app_.getJobQueue ().addJob (
                isTrusted ? jtVALIDATION_t : jtVALIDATION_ut,
                "recvValidation->checkValidation",
                [weak, val, isTrusted, m] (Job&)
                {
                    if (auto peer = weak.lock())
                        peer->checkValidation(
                            val,
                            isTrusted,
                            m);
                });
        }
        else
        {
            JLOG(p_journal_.debug()) <<
                "Validation: Dropping UNTRUSTED (load)";
        }
    }
    catch (std::exception const& e)
    {
        JLOG(p_journal_.warn()) <<
            "Validation: Exception, " << e.what();
        fee_ = Resource::feeInvalidRequest;
    }
}

void
PeerImp::onMessage (std::shared_ptr <protocol::TMGetObjectByHash> const& m)
{
    protocol::TMGetObjectByHash& packet = *m;

    if (packet.query ())
    {
//这是一个查询
        if (send_queue_.size() >= Tuning::dropSendQueue)
        {
            JLOG(p_journal_.debug()) << "GetObject: Large send queue";
            return;
        }

        if (packet.type () == protocol::TMGetObjectByHash::otFETCH_PACK)
        {
            doFetchPack (m);
            return;
        }

        fee_ = Resource::feeMediumBurdenPeer;

        protocol::TMGetObjectByHash reply;

        reply.set_query (false);

        if (packet.has_seq())
            reply.set_seq(packet.seq());

        reply.set_type (packet.type ());

        if (packet.has_ledgerhash ())
            reply.set_ledgerhash (packet.ledgerhash ());

//这是一个非常小的实现
        for (int i = 0; i < packet.objects_size (); ++i)
        {
            auto const& obj = packet.objects (i);
            if (obj.has_hash () && (obj.hash ().size () == (256 / 8)))
            {
                uint256 hash;
                memcpy (hash.begin (), obj.hash ().data (), 256 / 8);
//vfalc todo把这个搬到更明智的地方，这样我们就不会
//需要注入nodestore接口。
                std::uint32_t seq {obj.has_ledgerseq() ? obj.ledgerseq() : 0};
                auto hObj {app_.getNodeStore ().fetch (hash, seq)};
                if (!hObj)
                {
                    if (auto shardStore = app_.getShardStore())
                    {
                        if (seq >= shardStore->earliestSeq())
                            hObj = shardStore->fetch(hash, seq);
                    }
                }
                if (hObj)
                {
                    protocol::TMIndexedObject& newObj = *reply.add_objects ();
                    newObj.set_hash (hash.begin (), hash.size ());
                    newObj.set_data (&hObj->getData ().front (),
                        hObj->getData ().size ());

                    if (obj.has_nodeid ())
                        newObj.set_index (obj.nodeid ());
                    if (obj.has_ledgerseq())
                        newObj.set_ledgerseq(obj.ledgerseq());

//消息中的vfalc注释“seq”已过时
                }
            }
        }

        JLOG(p_journal_.trace()) <<
            "GetObj: " << reply.objects_size () <<
                " of " << packet.objects_size ();
        send (std::make_shared<Message> (reply, protocol::mtGET_OBJECTS));
    }
    else
    {
//这是一个答复
        std::uint32_t pLSeq = 0;
        bool pLDo = true;
        bool progress = false;

        for (int i = 0; i < packet.objects_size (); ++i)
        {
            const protocol::TMIndexedObject& obj = packet.objects (i);

            if (obj.has_hash () && (obj.hash ().size () == (256 / 8)))
            {
                if (obj.has_ledgerseq ())
                {
                    if (obj.ledgerseq () != pLSeq)
                    {
                        if (pLDo && (pLSeq != 0))
                        {
                            JLOG(p_journal_.debug()) <<
                                "GetObj: Full fetch pack for " << pLSeq;
                        }
                        pLSeq = obj.ledgerseq ();
                        pLDo = !app_.getLedgerMaster ().haveLedger (pLSeq);

                        if (!pLDo)
                        {
                            JLOG(p_journal_.debug()) <<
                                "GetObj: Late fetch pack for " << pLSeq;
                        }
                        else
                            progress = true;
                    }
                }

                if (pLDo)
                {
                    uint256 hash;
                    memcpy (hash.begin (), obj.hash ().data (), 256 / 8);

                    std::shared_ptr< Blob > data (
                        std::make_shared< Blob > (
                            obj.data ().begin (), obj.data ().end ()));

                    app_.getLedgerMaster ().addFetchPack (hash, data);
                }
            }
        }

        if (pLDo && (pLSeq != 0))
        {
            JLOG(p_journal_.debug()) <<
                "GetObj: Partial fetch pack for " << pLSeq;
        }
        if (packet.type () == protocol::TMGetObjectByHash::otFETCH_PACK)
            app_.getLedgerMaster ().gotFetchPack (progress, pLSeq);
    }
}

//——————————————————————————————————————————————————————————————

void
PeerImp::addLedger (uint256 const& hash)
{
    std::lock_guard<std::mutex> sl(recentLock_);

    if (std::find (recentLedgers_.begin(),
        recentLedgers_.end(), hash) != recentLedgers_.end())
        return;

//vfalco要查看排序向量是否更好。

    if (recentLedgers_.size () == 128)
        recentLedgers_.pop_front ();

    recentLedgers_.push_back (hash);
}

void
PeerImp::doFetchPack (const std::shared_ptr<protocol::TMGetObjectByHash>& packet)
{
//vfalc todo使用观察者和共享状态对象来反转此依赖关系。
//如果我们正在加载或已经加载，不要排队获取包作业
//有些排队。
    if (app_.getFeeTrack ().isLoadedLocal () ||
        (app_.getLedgerMaster().getValidatedLedgerAge() > 40s) ||
        (app_.getJobQueue().getJobCount(jtPACK) > 10))
    {
        JLOG(p_journal_.info()) << "Too busy to make fetch pack";
        return;
    }

    if (packet->ledgerhash ().size () != 32)
    {
        JLOG(p_journal_.warn()) << "FetchPack hash size malformed";
        fee_ = Resource::feeInvalidRequest;
        return;
    }

    fee_ = Resource::feeHighBurdenPeer;

    uint256 hash;
    memcpy (hash.begin (), packet->ledgerhash ().data (), 32);

    std::weak_ptr<PeerImp> weak = shared_from_this();
    auto elapsed = UptimeClock::now();
    auto const pap = &app_;
    app_.getJobQueue ().addJob (
        jtPACK, "MakeFetchPack",
        [pap, weak, packet, hash, elapsed] (Job&) {
            pap->getLedgerMaster().makeFetchPack(
                weak, packet, hash, elapsed);
        });
}

void
PeerImp::checkTransaction (int flags,
    bool checkSignature, std::shared_ptr<STTx const> const& stx)
{
//vfalc todo重写为不使用异常
    try
    {
//期满？
        if (stx->isFieldPresent(sfLastLedgerSequence) &&
            (stx->getFieldU32 (sfLastLedgerSequence) <
            app_.getLedgerMaster().getValidLedgerIndex()))
        {
            app_.getHashRouter().setFlags(stx->getTransactionID(), SF_BAD);
            charge (Resource::feeUnwantedData);
            return;
        }

        if (checkSignature)
        {
//在移交到作业队列之前检查签名。
            auto valid = checkValidity(app_.getHashRouter(), *stx,
                app_.getLedgerMaster().getValidatedRules(),
                    app_.config());
            if (valid.first != Validity::Valid)
            {
                if (!valid.second.empty())
                {
                    JLOG(p_journal_.trace()) <<
                        "Exception checking transaction: " <<
                            valid.second;
                }

//可能没必要把它弄坏，但不会伤害它。
                app_.getHashRouter().setFlags(stx->getTransactionID(), SF_BAD);
                charge(Resource::feeInvalidSignature);
                return;
            }
        }
        else
        {
            forceValidity(app_.getHashRouter(),
                stx->getTransactionID(), Validity::Valid);
        }

        std::string reason;
        auto tx = std::make_shared<Transaction> (
            stx, reason, app_);

        if (tx->getStatus () == INVALID)
        {
            if (! reason.empty ())
            {
                JLOG(p_journal_.trace()) <<
                    "Exception checking transaction: " << reason;
            }
            app_.getHashRouter ().setFlags (stx->getTransactionID (), SF_BAD);
            charge (Resource::feeInvalidSignature);
            return;
        }

        bool const trusted (flags & SF_TRUSTED);
        app_.getOPs ().processTransaction (
            tx, trusted, false, NetworkOPs::FailHard::no);
    }
    catch (std::exception const&)
    {
        app_.getHashRouter ().setFlags (stx->getTransactionID (), SF_BAD);
        charge (Resource::feeBadData);
    }
}

//从我们的工作队列调用
void
PeerImp::checkPropose (Job& job,
    std::shared_ptr <protocol::TMProposeSet> const& packet,
        RCLCxPeerPos peerPos)
{
    bool isTrusted = (job.getType () == jtPROPOSAL_t);

    JLOG(p_journal_.trace()) <<
        "Checking " << (isTrusted ? "trusted" : "UNTRUSTED") << " proposal";

    assert (packet);
    protocol::TMProposeSet& set = *packet;

    if (! cluster() && !peerPos.checkSign ())
    {
        JLOG(p_journal_.warn()) <<
            "Proposal fails sig check";
        charge (Resource::feeInvalidSignature);
        return;
    }

    if (isTrusted)
    {
        app_.getOPs ().processTrustedProposal (peerPos, packet);
    }
    else
    {
        if (cluster() ||
            (app_.getOPs().getConsensusLCL() == peerPos.proposal().prevLedger()))
        {
//中继不可信的建议
            JLOG(p_journal_.trace()) <<
                "relaying UNTRUSTED proposal";
            overlay_.relay(set, peerPos.suppressionID());
        }
        else
        {
            JLOG(p_journal_.debug()) <<
                "Not relaying UNTRUSTED proposal";
        }
    }
}

void
PeerImp::checkValidation (STValidation::pointer val,
    bool isTrusted, std::shared_ptr<protocol::TMValidation> const& packet)
{
    try
    {
//vfalc哪个函数抛出？
        uint256 signingHash = val->getSigningHash();
        if (! cluster() && !val->isValid (signingHash))
        {
            JLOG(p_journal_.warn()) <<
                "Validation is invalid";
            charge (Resource::feeInvalidRequest);
            return;
        }

        if (app_.getOPs ().recvValidation(val, std::to_string(id())) ||
            cluster())
        {
            auto const suppression = sha512Half(
                makeSlice(val->getSerialized()));
            overlay_.relay(*packet, suppression);
        }
    }
    catch (std::exception const&)
    {
        JLOG(p_journal_.trace()) <<
            "Exception processing validation";
        charge (Resource::feeInvalidRequest);
    }
}

//返回可以帮助我们获取
//具有指定根哈希的Tx树。
//
static
std::shared_ptr<PeerImp>
getPeerWithTree (OverlayImpl& ov,
    uint256 const& rootHash, PeerImp const* skip)
{
    std::shared_ptr<PeerImp> ret;
    int retScore = 0;

    ov.for_each([&](std::shared_ptr<PeerImp>&& p)
    {
        if (p->hasTxSet(rootHash) && p.get() != skip)
        {
            auto score = p->getScore (true);
            if (! ret || (score > retScore))
            {
                ret = std::move(p);
                retScore = score;
            }
        }
    });

    return ret;
}

//返回一个随机对等机，其权重取决于
//把分类帐拿出来，看看它有多灵敏。
//
static
std::shared_ptr<PeerImp>
getPeerWithLedger (OverlayImpl& ov,
    uint256 const& ledgerHash, LedgerIndex ledger,
        PeerImp const* skip)
{
    std::shared_ptr<PeerImp> ret;
    int retScore = 0;

    ov.for_each([&](std::shared_ptr<PeerImp>&& p)
    {
        if (p->hasLedger(ledgerHash, ledger) &&
                p.get() != skip)
        {
            auto score = p->getScore (true);
            if (! ret || (score > retScore))
            {
                ret = std::move(p);
                retScore = score;
            }
        }
    });

    return ret;
}

//vfalc注意到这个函数太大太麻烦了。
void
PeerImp::getLedger (std::shared_ptr<protocol::TMGetLedger> const& m)
{
    protocol::TMGetLedger& packet = *m;
    std::shared_ptr<SHAMap> shared;
    SHAMap const* map = nullptr;
    protocol::TMLedgerData reply;
    bool fatLeaves = true;
    std::shared_ptr<Ledger const> ledger;

    if (packet.has_requestcookie ())
        reply.set_requestcookie (packet.requestcookie ());

    std::string logMe;

    if (packet.itype () == protocol::liTS_CANDIDATE)
    {
//请求用于事务候选集
        JLOG(p_journal_.trace()) << "GetLedger: Tx candidate set";

        if ((!packet.has_ledgerhash () || packet.ledgerhash ().size () != 32))
        {
            charge (Resource::feeInvalidRequest);
            JLOG(p_journal_.warn()) << "GetLedger: Tx candidate set invalid";
            return;
        }

        uint256 txHash;
        memcpy (txHash.begin (), packet.ledgerhash ().data (), 32);

        shared = app_.getInboundTransactions().getSet (txHash, false);
        map = shared.get();

        if (! map)
        {
            if (packet.has_querytype () && !packet.has_requestcookie ())
            {
                JLOG(p_journal_.debug()) << "GetLedger: Routing Tx set request";

                auto const v = getPeerWithTree(
                    overlay_, txHash, this);
                if (! v)
                {
                    JLOG(p_journal_.info()) << "GetLedger: Route TX set failed";
                    return;
                }

                packet.set_requestcookie (id ());
                v->send (std::make_shared<Message> (
                    packet, protocol::mtGET_LEDGER));
                return;
            }

            JLOG(p_journal_.debug()) << "GetLedger: Can't provide map ";
            charge (Resource::feeInvalidRequest);
            return;
        }

        reply.set_ledgerseq (0);
        reply.set_ledgerhash (txHash.begin (), txHash.size ());
        reply.set_type (protocol::liTS_CANDIDATE);
fatLeaves = false; //我们已经有很多交易了
    }
    else
    {
        if (send_queue_.size() >= Tuning::dropSendQueue)
        {
            JLOG(p_journal_.debug()) << "GetLedger: Large send queue";
            return;
        }

        if (app_.getFeeTrack().isLoadedLocal() && ! cluster())
        {
            JLOG(p_journal_.debug()) << "GetLedger: Too busy";
            return;
        }

//弄清楚他们想要什么账本
        JLOG(p_journal_.trace()) << "GetLedger: Received";

        if (packet.has_ledgerhash ())
        {
            uint256 ledgerhash;

            if (packet.ledgerhash ().size () != 32)
            {
                charge (Resource::feeInvalidRequest);
                JLOG(p_journal_.warn()) << "GetLedger: Invalid request";
                return;
            }

            memcpy (ledgerhash.begin (), packet.ledgerhash ().data (), 32);
            logMe += "LedgerHash:";
            logMe += to_string (ledgerhash);
            ledger = app_.getLedgerMaster ().getLedgerByHash (ledgerhash);

            if (!ledger && packet.has_ledgerseq())
            {
                if (auto shardStore = app_.getShardStore())
                {
                    auto seq = packet.ledgerseq();
                    if (seq >= shardStore->earliestSeq())
                        ledger = shardStore->fetchLedger(ledgerhash, seq);
                }
            }

            if (!ledger)
            {
                JLOG(p_journal_.trace()) <<
                    "GetLedger: Don't have " << ledgerhash;
            }

            if (!ledger && (packet.has_querytype () &&
                !packet.has_requestcookie ()))
            {
//我们没有要求的分类帐
//搜索可能
                auto const v = getPeerWithLedger(overlay_, ledgerhash,
                    packet.has_ledgerseq() ? packet.ledgerseq() : 0, this);
                if (!v)
                {
                    JLOG(p_journal_.trace()) << "GetLedger: Cannot route";
                    return;
                }

                packet.set_requestcookie (id ());
                v->send (std::make_shared<Message>(
                    packet, protocol::mtGET_LEDGER));
                JLOG(p_journal_.debug()) << "GetLedger: Request routed";
                return;
            }
        }
        else if (packet.has_ledgerseq ())
        {
            if (packet.ledgerseq() <
                    app_.getLedgerMaster().getEarliestFetch())
            {
                JLOG(p_journal_.debug()) << "GetLedger: Early ledger request";
                return;
            }
            ledger = app_.getLedgerMaster ().getLedgerBySeq (
                packet.ledgerseq ());
            if (! ledger)
            {
                JLOG(p_journal_.debug()) <<
                    "GetLedger: Don't have " << packet.ledgerseq ();
            }
        }
        else if (packet.has_ltype () && (packet.ltype () == protocol::ltCLOSED) )
        {
            ledger = app_.getLedgerMaster ().getClosedLedger ();
            assert(! ledger->open());
//vvalco分类帐不应为空！
//vfalc如何打开已关闭的分类帐？
        #if 0
            if (ledger && ledger->info().open)
                ledger = app_.getLedgerMaster ().getLedgerBySeq (
                    ledger->info().seq - 1);
        #endif
        }
        else
        {
            charge (Resource::feeInvalidRequest);
            JLOG(p_journal_.warn()) << "GetLedger: Unknown request";
            return;
        }

        if ((!ledger) || (packet.has_ledgerseq () && (
            packet.ledgerseq () != ledger->info().seq)))
        {
            charge (Resource::feeInvalidRequest);

            if (ledger)
            {
                JLOG(p_journal_.warn()) << "GetLedger: Invalid sequence";
            }
            return;
        }

        if (!packet.has_ledgerseq() && (ledger->info().seq <
            app_.getLedgerMaster().getEarliestFetch()))
        {
            JLOG(p_journal_.debug()) << "GetLedger: Early ledger request";
            return;
        }

//填写答复
        auto const lHash = ledger->info().hash;
        reply.set_ledgerhash (lHash.begin (), lHash.size ());
        reply.set_ledgerseq (ledger->info().seq);
        reply.set_type (packet.itype ());

        if (packet.itype () == protocol::liBASE)
        {
//他们需要分类帐基础数据
            JLOG(p_journal_.trace()) << "GetLedger: Base data";
            Serializer nData (128);
            addRaw(ledger->info(), nData);
            reply.add_nodes ()->set_nodedata (
                nData.getDataPtr (), nData.getLength ());

            auto const& stateMap = ledger->stateMap ();
            if (stateMap.getHash() != beast::zero)
            {
//如果可能，返回帐户状态根节点
                Serializer rootNode (768);
                if (stateMap.getRootNode(rootNode, snfWIRE))
                {
                    reply.add_nodes ()->set_nodedata (
                        rootNode.getDataPtr (), rootNode.getLength ());

                    if (ledger->info().txHash != beast::zero)
                    {
                        auto const& txMap = ledger->txMap ();

                        if (txMap.getHash() != beast::zero)
                        {
                            rootNode.erase ();

                            if (txMap.getRootNode (rootNode, snfWIRE))
                                reply.add_nodes ()->set_nodedata (
                                    rootNode.getDataPtr (),
                                    rootNode.getLength ());
                        }
                    }
                }
            }

            Message::pointer oPacket = std::make_shared<Message> (
                reply, protocol::mtLEDGER_DATA);
            send (oPacket);
            return;
        }

        if (packet.itype () == protocol::liTX_NODE)
        {
            map = &ledger->txMap ();
            logMe += " TX:";
            logMe += to_string (map->getHash ());
        }
        else if (packet.itype () == protocol::liAS_NODE)
        {
            map = &ledger->stateMap ();
            logMe += " AS:";
            logMe += to_string (map->getHash ());
        }
    }

    if (!map || (packet.nodeids_size () == 0))
    {
        JLOG(p_journal_.warn()) <<
            "GetLedger: Can't find map or empty request";
        charge (Resource::feeInvalidRequest);
        return;
    }

    JLOG(p_journal_.trace()) << "GetLedger: " << logMe;

    auto const depth =
        packet.has_querydepth() ?
            (std::min(packet.querydepth(), 3u)) :
            (isHighLatency() ? 2 : 1);

    for (int i = 0;
            (i < packet.nodeids().size() &&
            (reply.nodes().size() < Tuning::maxReplyNodes)); ++i)
    {
        SHAMapNodeID mn (packet.nodeids (i).data (), packet.nodeids (i).size ());

        if (!mn.isValid ())
        {
            JLOG(p_journal_.warn()) << "GetLedger: Invalid node " << logMe;
            charge (Resource::feeInvalidRequest);
            return;
        }

        std::vector<SHAMapNodeID> nodeIDs;
        std::vector< Blob > rawNodes;

        try
        {
            if (map->getNodeFat(mn, nodeIDs, rawNodes, fatLeaves, depth))
            {
                assert (nodeIDs.size () == rawNodes.size ());
                JLOG(p_journal_.trace()) <<
                    "GetLedger: getNodeFat got " << rawNodes.size () << " nodes";
                std::vector<SHAMapNodeID>::iterator nodeIDIterator;
                std::vector< Blob >::iterator rawNodeIterator;

                for (nodeIDIterator = nodeIDs.begin (),
                        rawNodeIterator = rawNodes.begin ();
                            nodeIDIterator != nodeIDs.end ();
                                ++nodeIDIterator, ++rawNodeIterator)
                {
                    Serializer nID (33);
                    nodeIDIterator->addIDRaw (nID);
                    protocol::TMLedgerNode* node = reply.add_nodes ();
                    node->set_nodeid (nID.getDataPtr (), nID.getLength ());
                    node->set_nodedata (&rawNodeIterator->front (),
                        rawNodeIterator->size ());
                }
            }
            else
            {
                JLOG(p_journal_.warn()) <<
                    "GetLedger: getNodeFat returns false";
            }
        }
        catch (std::exception&)
        {
            std::string info;

            if (packet.itype () == protocol::liTS_CANDIDATE)
                info = "TS candidate";
            else if (packet.itype () == protocol::liBASE)
                info = "Ledger base";
            else if (packet.itype () == protocol::liTX_NODE)
                info = "TX node";
            else if (packet.itype () == protocol::liAS_NODE)
                info = "AS node";

            if (!packet.has_ledgerhash ())
                info += ", no hash specified";

            JLOG(p_journal_.warn()) <<
                "getNodeFat( " << mn << ") throws exception: " << info;
        }
    }

    JLOG(p_journal_.info()) <<
        "Got request for " << packet.nodeids().size() << " nodes at depth " <<
        depth << ", return " << reply.nodes().size() << " nodes";

    Message::pointer oPacket = std::make_shared<Message> (
        reply, protocol::mtLEDGER_DATA);
    send (oPacket);
}

void
PeerImp::peerTXData (uint256 const& hash,
    std::shared_ptr <protocol::TMLedgerData> const& pPacket,
        beast::Journal journal)
{
    app_.getInboundTransactions().gotData (hash, shared_from_this(), pPacket);
}

int
PeerImp::getScore (bool haveItem) const
{
//分数的随机组成部分，用于打破关系并避免
//重载“最佳”对等
   static const int spRandomMax = 9999;

//因为很可能拥有我们的东西而得分
//寻找；应该大致是斯兰多玛
   static const int spHaveItem = 10000;

//每毫秒潜伏期得分降低；应该
//大致用sprandommax除以最大合理值
//等待时间
   static const int spLatency = 30;

//对未知延迟的惩罚；应该大致是sprandommax
   static const int spNoLatency = 8000;

   int score = rand_int(spRandomMax);

   if (haveItem)
       score += spHaveItem;

   std::chrono::milliseconds latency;
   {
       std::lock_guard<std::mutex> sl (recentLock_);
       latency = latency_;
   }

   if (latency != std::chrono::milliseconds (-1))
       score -= latency.count() * spLatency;
   else
       score -= spNoLatency;

   return score;
}

bool
PeerImp::isHighLatency() const
{
    std::lock_guard<std::mutex> sl (recentLock_);
    return latency_.count() >= Tuning::peerHighLatency;
}

} //涟漪
