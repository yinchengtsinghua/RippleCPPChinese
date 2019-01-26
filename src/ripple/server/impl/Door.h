
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

#ifndef RIPPLE_SERVER_DOOR_H_INCLUDED
#define RIPPLE_SERVER_DOOR_H_INCLUDED

#include <ripple/server/impl/io_list.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/server/impl/PlainHTTPPeer.h>
#include <ripple/server/impl/SSLHTTPPeer.h>
#include <ripple/beast/asio/ssl_bundle.h>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/container/flat_map.hpp>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace ripple {

/*一个监听插座。*/
template<class Handler>
class Door
    : public io_list::work
    , public std::enable_shared_from_this<Door<Handler>>
{
private:
    using clock_type = std::chrono::steady_clock;
    using timer_type = boost::asio::basic_waitable_timer<clock_type>;
    using error_code = boost::system::error_code;
    using yield_context = boost::asio::yield_context;
    using protocol_type = boost::asio::ip::tcp;
    using acceptor_type = protocol_type::acceptor;
    using endpoint_type = protocol_type::endpoint;
    using socket_type = protocol_type::socket;

//在套接字上检测SSL
    class Detector
        : public io_list::work
        , public std::enable_shared_from_this<Detector>
    {
    private:
        Port const& port_;
        Handler& handler_;
        socket_type socket_;
        timer_type timer_;
        endpoint_type remote_address_;
        boost::asio::io_service::strand strand_;
        beast::Journal j_;

    public:
        Detector (Port const& port, Handler& handler,
            socket_type&& socket, endpoint_type remote_address,
                beast::Journal j);
        void run();
        void close() override;

    private:
        void do_timer (yield_context yield);
        void do_detect (yield_context yield);
    };

    beast::Journal j_;
    Port const& port_;
    Handler& handler_;
    acceptor_type acceptor_;
    boost::asio::io_service::strand strand_;
    bool ssl_;
    bool plain_;

public:
    Door(Handler& handler, boost::asio::io_service& io_service,
        Port const& port, beast::Journal j);

//解决这个问题，因为我们不能在ctor中调用shared_
    void run();

    /*关闭车门监听插座和连接。
        监听插座关闭，所有连接打开
        属于门是关着的。
        线程安全：
            可以同时调用
    **/

    void close() override;

    endpoint_type get_endpoint() const
    {
        return acceptor_.local_endpoint();
    }

private:
    template <class ConstBufferSequence>
    void create (bool ssl, ConstBufferSequence const& buffers,
        socket_type&& socket, endpoint_type remote_address);

    void do_accept (yield_context yield);
};

/*检测SSL客户端握手。
    分析所提供缓冲区中的字节以检测SSL客户端
    握手。如果缓冲区包含的数据不足，将有更多的数据
    从流中读取，直到有足够的数据确定结果为止。
    BUF中没有丢弃任何字节。任何额外的字节读取都将被保留。
    buf必须提供与boost:：asio:：streambuf兼容的接口
    http://boost.org/doc/libs/1_56_0/doc/html/boost_asio/reference/streambuf.html
    见
        http://www.ietf.org/rfc/rfc2246.txt
        第7.4节。握手协议
    @param socket要读取的流
    @param buf保存接收数据的缓冲区
    @param do_生成do_yield上下文
    @如果发生错误，返回错误代码，否则返回'true'if
            读取的数据表示SSL客户机握手。
**/

template <class Socket, class StreamBuf, class Yield>
std::pair <boost::system::error_code, bool>
detect_ssl (Socket& socket, StreamBuf& buf, Yield do_yield)
{
    std::pair <boost::system::error_code, bool> result;
    result.second = false;
    for(;;)
    {
std::size_t const max = 4; //我们需要的最大字节数
        unsigned char data[max];
        auto const bytes = boost::asio::buffer_copy (
            boost::asio::buffer(data), buf.data());

        if (bytes > 0)
        {
if (data[0] != 0x16) //message type 0x16=“SSL握手”
                break;
        }

        if (bytes >= max)
        {
            result.second = true;
            break;
        }

        buf.commit(boost::asio::async_read (socket,
            buf.prepare(max - bytes), boost::asio::transfer_at_least(1),
                do_yield[result.first]));
        if (result.first)
            break;
    }
    return result;
}

template<class Handler>
Door<Handler>::Detector::
Detector(Port const& port,
    Handler& handler, socket_type&& socket,
        endpoint_type remote_address, beast::Journal j)
    : port_(port)
    , handler_(handler)
    , socket_(std::move(socket))
    , timer_(socket_.get_io_service())
    , remote_address_(remote_address)
    , strand_(socket_.get_io_service())
    , j_(j)
{
}

template<class Handler>
void
Door<Handler>::Detector::
run()
{
//Do-Detect必须在Do-Timer或Else之前调用
//定时器可以在设置前取消。
    boost::asio::spawn(strand_, std::bind (&Detector::do_detect,
        this->shared_from_this(), std::placeholders::_1));

    boost::asio::spawn(strand_, std::bind (&Detector::do_timer,
        this->shared_from_this(), std::placeholders::_1));
}

template<class Handler>
void
Door<Handler>::Detector::
close()
{
    error_code ec;
    socket_.close(ec);
    timer_.cancel(ec);
}

template<class Handler>
void
Door<Handler>::Detector::
do_timer(yield_context do_yield)
{
error_code ec; //忽略
    while (socket_.is_open())
    {
        timer_.async_wait (do_yield[ec]);
        if (timer_.expires_from_now() <= std::chrono::seconds(0))
            socket_.close(ec);
    }
}

template<class Handler>
void
Door<Handler>::Detector::
do_detect(boost::asio::yield_context do_yield)
{
    bool ssl;
    error_code ec;
    boost::beast::multi_buffer buf(16);
    timer_.expires_from_now(std::chrono::seconds(15));
    std::tie(ec, ssl) = detect_ssl(socket_, buf, do_yield);
    error_code unused;
    timer_.cancel(unused);
    if (! ec)
    {
        if (ssl)
        {
            if(auto sp = ios().template emplace<SSLHTTPPeer<Handler>>(
                port_, handler_, j_, remote_address_,
                    buf.data(), std::move(socket_)))
                sp->run();
            return;
        }
        if(auto sp = ios().template emplace<PlainHTTPPeer<Handler>>(
            port_, handler_, j_, remote_address_,
                buf.data(), std::move(socket_)))
            sp->run();
        return;
    }
    if (ec != boost::asio::error::operation_aborted)
    {
        JLOG(j_.trace()) <<
            "Error detecting ssl: " << ec.message() <<
                " from " << remote_address_;
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler>
Door<Handler>::
Door(Handler& handler, boost::asio::io_service& io_service,
        Port const& port, beast::Journal j)
    : j_(j)
    , port_(port)
    , handler_(handler)
    , acceptor_(io_service)
    , strand_(io_service)
    , ssl_(
        port_.protocol.count("https") > 0 ||
        port_.protocol.count("wss") > 0 ||
        port_.protocol.count("wss2")  > 0 ||
        port_.protocol.count("peer")  > 0)
    , plain_(
        port_.protocol.count("http") > 0 ||
        port_.protocol.count("ws") > 0 ||
        port_.protocol.count("ws2"))
{
    error_code ec;
    endpoint_type const local_address =
        endpoint_type(port.ip, port.port);

    acceptor_.open(local_address.protocol(), ec);
    if (ec)
    {
        JLOG(j_.error()) <<
            "Open port '" << port.name << "' failed:" << ec.message();
        Throw<std::exception> ();
    }

    acceptor_.set_option(
        boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec)
    {
        JLOG(j_.error()) <<
            "Option for port '" << port.name << "' failed:" << ec.message();
        Throw<std::exception> ();
    }

    acceptor_.bind(local_address, ec);
    if (ec)
    {
        JLOG(j_.error()) <<
            "Bind port '" << port.name << "' failed:" << ec.message();
        Throw<std::exception> ();
    }

    acceptor_.listen(boost::asio::socket_base::max_connections, ec);
    if (ec)
    {
        JLOG(j_.error()) <<
            "Listen on port '" << port.name << "' failed:" << ec.message();
        Throw<std::exception> ();
    }

    JLOG(j_.info()) <<
        "Opened " << port;
}

template<class Handler>
void
Door<Handler>::
run()
{
    boost::asio::spawn(strand_, std::bind(&Door<Handler>::do_accept,
        this->shared_from_this(), std::placeholders::_1));
}

template<class Handler>
void
Door<Handler>::
close()
{
    if (! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &Door<Handler>::close, this->shared_from_this()));
    error_code ec;
    acceptor_.close(ec);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler>
template<class ConstBufferSequence>
void
Door<Handler>::
create(bool ssl, ConstBufferSequence const& buffers,
    socket_type&& socket, endpoint_type remote_address)
{
    if (ssl)
    {
        if(auto sp = ios().template emplace<SSLHTTPPeer<Handler>>(
            port_, handler_, j_, remote_address,
                buffers, std::move(socket)))
            sp->run();
        return;
    }
    if(auto sp = ios().template emplace<PlainHTTPPeer<Handler>>(
        port_, handler_, j_, remote_address,
            buffers, std::move(socket)))
        sp->run();
}

template<class Handler>
void
Door<Handler>::
do_accept(boost::asio::yield_context do_yield)
{
    while (acceptor_.is_open())
    {
        error_code ec;
        endpoint_type remote_address;
        socket_type socket (acceptor_.get_io_service());
        acceptor_.async_accept (socket, remote_address, do_yield[ec]);
        if (ec && ec != boost::asio::error::operation_aborted)
        {
            JLOG(j_.error()) <<
                "accept: " << ec.message();
        }
        if (ec == boost::asio::error::operation_aborted)
            break;
        if (ec)
            continue;

        if (ssl_ && plain_)
        {
            if(auto sp = ios().template emplace<Detector>(
                port_, handler_, std::move(socket),
                    remote_address, j_))
                sp->run();
        }
        else if (ssl_ || plain_)
        {
            create(ssl_, boost::asio::null_buffers{},
                std::move(socket), remote_address);
        }
    }
}

} //涟漪

#endif
