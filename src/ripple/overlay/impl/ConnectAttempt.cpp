
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

#include <ripple/overlay/impl/ConnectAttempt.h>
#include <ripple/overlay/impl/PeerImp.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/json/json_reader.h>

namespace ripple {

ConnectAttempt::ConnectAttempt (Application& app, boost::asio::io_service& io_service,
    endpoint_type const& remote_endpoint, Resource::Consumer usage,
        beast::asio::ssl_bundle::shared_context const& context,
            std::uint32_t id, PeerFinder::Slot::ptr const& slot,
                beast::Journal journal, OverlayImpl& overlay)
    : Child (overlay)
    , app_ (app)
    , id_ (id)
    , sink_ (journal, OverlayImpl::makePrefix(id))
    , journal_ (sink_)
    , remote_endpoint_ (remote_endpoint)
    , usage_ (usage)
    , strand_ (io_service)
    , timer_ (io_service)
    , ssl_bundle_ (std::make_unique<beast::asio::ssl_bundle>(
        context, io_service))
    , socket_ (ssl_bundle_->socket)
    , stream_ (ssl_bundle_->stream)
    , slot_ (slot)
{
    JLOG(journal_.debug()) <<
        "Connect " << remote_endpoint;
}

ConnectAttempt::~ConnectAttempt()
{
    if (slot_ != nullptr)
        overlay_.peerFinder().on_closed(slot_);
    JLOG(journal_.trace()) <<
        "~ConnectAttempt";
}

void
ConnectAttempt::stop()
{
    if (! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &ConnectAttempt::stop, shared_from_this()));
    if (stream_.next_layer().is_open())
    {
        JLOG(journal_.debug()) <<
            "Stop";
    }
    close();
}

void
ConnectAttempt::run()
{
    stream_.next_layer().async_connect (remote_endpoint_,
        strand_.wrap (std::bind (&ConnectAttempt::onConnect,
            shared_from_this(), std::placeholders::_1)));
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
ConnectAttempt::close()
{
    assert(strand_.running_in_this_thread());
    if (stream_.next_layer().is_open())
    {
        error_code ec;
        timer_.cancel(ec);
        socket_.close(ec);
        JLOG(journal_.debug()) <<
            "Closed";
    }
}

void
ConnectAttempt::fail (std::string const& reason)
{
    assert(strand_.running_in_this_thread());
    if (stream_.next_layer().is_open())
    {
        JLOG(journal_.debug()) <<
            reason;
    }
    close();
}

void
ConnectAttempt::fail (std::string const& name, error_code ec)
{
    assert(strand_.running_in_this_thread());
    if (stream_.next_layer().is_open())
    {
        JLOG(journal_.debug()) <<
            name << ": " << ec.message();
    }
    close();
}

void
ConnectAttempt::setTimer()
{
    error_code ec;
    timer_.expires_from_now(std::chrono::seconds(15), ec);
    if (ec)
    {
        JLOG(journal_.error()) <<
            "setTimer: " << ec.message();
        return;
    }

    timer_.async_wait(strand_.wrap(std::bind(
        &ConnectAttempt::onTimer, shared_from_this(),
            std::placeholders::_1)));
}

void
ConnectAttempt::cancelTimer()
{
    error_code ec;
    timer_.cancel(ec);
}

void
ConnectAttempt::onTimer (error_code ec)
{
    if (! stream_.next_layer().is_open())
        return;
    if (ec == boost::asio::error::operation_aborted)
        return;
    if (ec)
    {
//这不应该发生
        JLOG(journal_.error()) <<
            "onTimer: " << ec.message();
        return close();
    }
    fail("Timeout");
}

void
ConnectAttempt::onConnect (error_code ec)
{
    cancelTimer();

    if(ec == boost::asio::error::operation_aborted)
        return;
    endpoint_type local_endpoint;
    if(! ec)
        local_endpoint = stream_.next_layer().local_endpoint(ec);
    if(ec)
        return fail("onConnect", ec);
    if(! stream_.next_layer().is_open())
        return;
    JLOG(journal_.trace()) <<
        "onConnect";

    setTimer();
    stream_.set_verify_mode (boost::asio::ssl::verify_none);
    stream_.async_handshake (boost::asio::ssl::stream_base::client,
        strand_.wrap (std::bind (&ConnectAttempt::onHandshake,
            shared_from_this(), std::placeholders::_1)));
}

void
ConnectAttempt::onHandshake (error_code ec)
{
    cancelTimer();
    if(! stream_.next_layer().is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    endpoint_type local_endpoint;
    if (! ec)
        local_endpoint = stream_.next_layer().local_endpoint(ec);
    if(ec)
        return fail("onHandshake", ec);
    JLOG(journal_.trace()) <<
        "onHandshake";

    if (! overlay_.peerFinder().onConnected (slot_,
            beast::IPAddressConversion::from_asio (local_endpoint)))
        return fail("Duplicate connection");

    auto sharedValue = makeSharedValue(
        stream_.native_handle(), journal_);
    if (! sharedValue)
return close(); //MakeSharedValue日志

    req_ = makeRequest(! overlay_.peerFinder().config().peerPrivate,
        remote_endpoint_.address());
    auto const hello = buildHello (
        *sharedValue,
        overlay_.setup().public_ip,
        beast::IPAddressConversion::from_asio(remote_endpoint_),
        app_);
    appendHello (req_, hello);

    setTimer();
    boost::beast::http::async_write(stream_, req_,
        strand_.wrap (std::bind (&ConnectAttempt::onWrite,
            shared_from_this(), std::placeholders::_1)));
}

void
ConnectAttempt::onWrite (error_code ec)
{
    cancelTimer();
    if(! stream_.next_layer().is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(ec)
        return fail("onWrite", ec);
    boost::beast::http::async_read(stream_, read_buf_, response_,
        strand_.wrap(std::bind(&ConnectAttempt::onRead,
            shared_from_this(), std::placeholders::_1)));
}

void
ConnectAttempt::onRead (error_code ec)
{
    cancelTimer();

    if(! stream_.next_layer().is_open())
        return;
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(ec == boost::asio::error::eof)
    {
        JLOG(journal_.info()) <<
            "EOF";
        setTimer();
        return stream_.async_shutdown(strand_.wrap(std::bind(
            &ConnectAttempt::onShutdown, shared_from_this(),
                std::placeholders::_1)));
    }
    if(ec)
        return fail("onRead", ec);
    processResponse();
}

void
ConnectAttempt::onShutdown (error_code ec)
{
    cancelTimer();
    if (! ec)
    {
        JLOG(journal_.error()) <<
            "onShutdown: expected error condition";
        return close();
    }
    if (ec != boost::asio::error::eof)
        return fail("onShutdown", ec);
    close();
}

//——————————————————————————————————————————————————————————————

auto
ConnectAttempt::makeRequest (bool crawl,
    boost::asio::ip::address const& remote_address) ->
        request_type
{
    request_type m;
    m.method(boost::beast::http::verb::get);
    m.target("/");
    m.version(11);
    m.insert ("User-Agent", BuildInfo::getFullVersionString());
    m.insert ("Upgrade", "RTXP/1.2");
//std:：string（“rtxp/”）+to_string（buildinfo:：getcurrentprotocol（））；
    m.insert ("Connection", "Upgrade");
    m.insert ("Connect-As", "Peer");
    m.insert ("Crawl", crawl ? "public" : "private");
    return m;
}

void
ConnectAttempt::processResponse()
{
    if (response_.result() == boost::beast::http::status::service_unavailable)
    {
        Json::Value json;
        Json::Reader r;
        std::string s;
        s.reserve(boost::asio::buffer_size(response_.body().data()));
        for(auto const& buffer : response_.body().data())
            s.append(
                boost::asio::buffer_cast<char const*>(buffer),
                boost::asio::buffer_size(buffer));
        auto const success = r.parse(s, json);
        if (success)
        {
            if (json.isObject() && json.isMember("peer-ips"))
            {
                Json::Value const& ips = json["peer-ips"];
                if (ips.isArray())
                {
                    std::vector<boost::asio::ip::tcp::endpoint> eps;
                    eps.reserve(ips.size());
                    for (auto const& v : ips)
                    {
                        if (v.isString())
                        {
                            error_code ec;
                            auto const ep = parse_endpoint(v.asString(), ec);
                            if (!ec)
                                eps.push_back(ep);
                        }
                    }
                    overlay_.peerFinder().onRedirects(
                        remote_endpoint_, eps);
                }
            }
        }
    }

    if (! OverlayImpl::isPeerUpgrade(response_))
    {
        JLOG(journal_.info()) <<
            "HTTP Response: " << response_.result() << " " << response_.reason();
        return close();
    }

    auto hello = parseHello (false, response_, journal_);
    if(! hello)
        return fail("processResponse: Bad TMHello");

    auto sharedValue = makeSharedValue(
        ssl_bundle_->stream.native_handle(), journal_);
    if(! sharedValue)
return close(); //MakeSharedValue日志

    auto publicKey = verifyHello (*hello,
        *sharedValue,
        overlay_.setup().public_ip,
        beast::IPAddressConversion::from_asio(remote_endpoint_),
        journal_, app_);
    if(! publicKey)
return close(); //验证Hello日志
    JLOG(journal_.info()) <<
        "Public Key: " << toBase58 (
            TokenType::NodePublic,
            *publicKey);

    auto const protocol =
        BuildInfo::make_protocol(hello->protoversion());
    JLOG(journal_.info()) <<
        "Protocol: " << to_string(protocol);

    auto member = app_.cluster().member(*publicKey);
    if (member)
    {
        JLOG(journal_.info()) <<
            "Cluster name: " << *member;
    }

    auto const result = overlay_.peerFinder().activate (slot_,
        *publicKey, static_cast<bool>(member));
    if (result != PeerFinder::Result::success)
        return fail("Outbound slots full");

    auto const peer = std::make_shared<PeerImp>(app_,
        std::move(ssl_bundle_), read_buf_.data(),
            std::move(slot_), std::move(response_),
                usage_, *hello, *publicKey, id_, overlay_);

    overlay_.add_active (peer);
}

} //涟漪
