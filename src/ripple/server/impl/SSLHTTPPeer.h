
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

#ifndef RIPPLE_SERVER_SSLHTTPPEER_H_INCLUDED
#define RIPPLE_SERVER_SSLHTTPPEER_H_INCLUDED

#include <ripple/server/impl/BaseHTTPPeer.h>
#include <ripple/server/impl/SSLWSPeer.h>
#include <ripple/beast/asio/ssl_bundle.h>
#include <memory>

namespace ripple {

template<class Handler>
class SSLHTTPPeer
    : public BaseHTTPPeer<Handler, SSLHTTPPeer<Handler>>
    , public std::enable_shared_from_this<SSLHTTPPeer<Handler>>
{
private:
    friend class BaseHTTPPeer<Handler, SSLHTTPPeer>;
    using socket_type = boost::asio::ip::tcp::socket;
    using stream_type = boost::asio::ssl::stream <socket_type&>;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using yield_context = boost::asio::yield_context;
    using error_code = boost::system::error_code;

    std::unique_ptr<beast::asio::ssl_bundle> ssl_bundle_;
    stream_type& stream_;

public:
    template<class ConstBufferSequence>
    SSLHTTPPeer(Port const& port, Handler& handler,
        beast::Journal journal, endpoint_type remote_address,
            ConstBufferSequence const& buffers, socket_type&& socket);

    void
    run();

    std::shared_ptr<WSSession>
    websocketUpgrade() override;

private:
    void
    do_handshake(yield_context do_yield);

    void
    do_request() override;

    void
    do_close() override;

    void
    on_shutdown(error_code ec);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler>
template<class ConstBufferSequence>
SSLHTTPPeer<Handler>::
SSLHTTPPeer(Port const& port, Handler& handler,
    beast::Journal journal, endpoint_type remote_address,
        ConstBufferSequence const& buffers, socket_type&& socket)
    : BaseHTTPPeer<Handler, SSLHTTPPeer>(port, handler,
        socket.get_io_service(), journal, remote_address, buffers)
    , ssl_bundle_(std::make_unique<beast::asio::ssl_bundle>(
        port.context, std::move(socket)))
    , stream_(ssl_bundle_->stream)
{
}

//当接受者接受我们的套接字时调用。
template<class Handler>
void
SSLHTTPPeer<Handler>::
run()
{
    if(! this->handler_.onAccept(this->session(), this->remote_address_))
    {
        boost::asio::spawn(this->strand_,
            std::bind(&SSLHTTPPeer::do_close,
                this->shared_from_this()));
        return;
    }
    if (! stream_.lowest_layer().is_open())
        return;
    boost::asio::spawn(this->strand_, std::bind(
        &SSLHTTPPeer::do_handshake, this->shared_from_this(),
            std::placeholders::_1));
}

template<class Handler>
std::shared_ptr<WSSession>
SSLHTTPPeer<Handler>::
websocketUpgrade()
{
    auto ws = this->ios().template emplace<SSLWSPeer<Handler>>(
        this->port_, this->handler_, this->remote_address_,
            std::move(this->message_), std::move(this->ssl_bundle_),
                this->journal_);
    return ws;
}

template<class Handler>
void
SSLHTTPPeer<Handler>::
do_handshake(yield_context do_yield)
{
    boost::system::error_code ec;
    stream_.set_verify_mode(boost::asio::ssl::verify_none);
    this->start_timer();
    this->read_buf_.consume(stream_.async_handshake(
        stream_type::server, this->read_buf_.data(), do_yield[ec]));
    this->cancel_timer();
    if (ec)
        return this->fail(ec, "handshake");
    bool const http =
        this->port().protocol.count("peer") > 0 ||
        this->port().protocol.count("wss") > 0 ||
        this->port().protocol.count("wss2") > 0 ||
        this->port().protocol.count("https") > 0;
    if(http)
    {
        boost::asio::spawn(this->strand_,
            std::bind(&SSLHTTPPeer::do_read,
                this->shared_from_this(), std::placeholders::_1));
        return;
    }
//'此'将被销毁
}

template<class Handler>
void
SSLHTTPPeer<Handler>::
do_request()
{
    ++this->request_count_;
    auto const what = this->handler_.onHandoff(this->session(),
        std::move(ssl_bundle_), std::move(this->message_),
            this->remote_address_);
    if(what.moved)
        return;
    if(what.response)
        return this->write(what.response, what.keep_alive);
//遗产
    this->handler_.onRequest(this->session());
}

template<class Handler>
void
SSLHTTPPeer<Handler>::
do_close()
{
    this->start_timer();
    stream_.async_shutdown(this->strand_.wrap(std::bind (
        &SSLHTTPPeer::on_shutdown, this->shared_from_this(),
            std::placeholders::_1)));
}

template<class Handler>
void
SSLHTTPPeer<Handler>::
on_shutdown(error_code ec)
{
    this->cancel_timer();

    if (ec == boost::asio::error::operation_aborted)
        return;
    if (ec)
    {
        JLOG(this->journal_.debug()) <<
            "on_shutdown: " << ec.message();
    }

//如果这个->析构函数被延迟，立即关闭套接字
    stream_.lowest_layer().close(ec);
}

} //涟漪

#endif
