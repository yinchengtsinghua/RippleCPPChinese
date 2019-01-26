
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

#ifndef RIPPLE_SERVER_PLAINHTTPPEER_H_INCLUDED
#define RIPPLE_SERVER_PLAINHTTPPEER_H_INCLUDED

#include <ripple/beast/rfc2616.h>
#include <ripple/server/impl/BaseHTTPPeer.h>
#include <ripple/server/impl/PlainWSPeer.h>
#include <memory>

namespace ripple {

template<class Handler>
class PlainHTTPPeer
    : public BaseHTTPPeer<Handler, PlainHTTPPeer<Handler>>
    , public std::enable_shared_from_this<PlainHTTPPeer<Handler>>
{
private:
    friend class BaseHTTPPeer<Handler, PlainHTTPPeer>;
    using socket_type = boost::asio::ip::tcp::socket;
    using endpoint_type = boost::asio::ip::tcp::endpoint;

    socket_type stream_;

public:
    template<class ConstBufferSequence>
    PlainHTTPPeer(Port const& port, Handler& handler,
        beast::Journal journal, endpoint_type remote_address,
            ConstBufferSequence const& buffers, socket_type&& socket);

    void
    run();

    std::shared_ptr<WSSession>
    websocketUpgrade() override;

private:
    void
    do_request() override;

    void
    do_close() override;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler>
template<class ConstBufferSequence>
PlainHTTPPeer<Handler>::
PlainHTTPPeer(Port const& port, Handler& handler,
    beast::Journal journal, endpoint_type remote_endpoint,
        ConstBufferSequence const& buffers, socket_type&& socket)
    : BaseHTTPPeer<Handler, PlainHTTPPeer>(port, handler,
        socket.get_io_service(), journal, remote_endpoint, buffers)
    , stream_(std::move(socket))
{
//在环回接口上设置tcp节点，
//否则，Nagle的算法使env
//在Linux系统上测试运行速度较慢。
//
    if(remote_endpoint.address().is_loopback())
        stream_.set_option(boost::asio::ip::tcp::no_delay{true});
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
run()
{
    if (! this->handler_.onAccept(this->session(), this->remote_address_))
    {
        boost::asio::spawn(this->strand_,
            std::bind (&PlainHTTPPeer::do_close,
                this->shared_from_this()));
        return;
    }

    if (! stream_.is_open())
        return;

    boost::asio::spawn(this->strand_, std::bind(&PlainHTTPPeer::do_read,
        this->shared_from_this(), std::placeholders::_1));
}

template<class Handler>
std::shared_ptr<WSSession>
PlainHTTPPeer<Handler>::
websocketUpgrade()
{
    auto ws = this->ios().template emplace<PlainWSPeer<Handler>>(
        this->port_, this->handler_, this->remote_address_,
            std::move(this->message_), std::move(stream_),
                this->journal_);
    return ws;
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
do_request()
{
    ++this->request_count_;
    auto const what = this->handler_.onHandoff(this->session(),
        std::move(this->message_), this->remote_address_);
    if (what.moved)
        return;
    boost::system::error_code ec;
    if (what.response)
    {
//半闭合连接：闭合
        if (! what.keep_alive)
            stream_.shutdown(socket_type::shutdown_receive, ec);
        if (ec)
            return this->fail(ec, "request");
        return this->write(what.response, what.keep_alive);
    }

//连接时执行半关闭：关闭而不是SSL
    if (! beast::rfc2616::is_keep_alive(this->message_))
        stream_.shutdown(socket_type::shutdown_receive, ec);
    if (ec)
        return this->fail(ec, "request");
//遗产
    this->handler_.onRequest(this->session());
}

template<class Handler>
void
PlainHTTPPeer<Handler>::
do_close()
{
    boost::system::error_code ec;
    stream_.shutdown(socket_type::shutdown_send, ec);
}

} //涟漪

#endif
