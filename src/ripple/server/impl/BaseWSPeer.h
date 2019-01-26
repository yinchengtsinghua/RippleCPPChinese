
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

#ifndef RIPPLE_SERVER_BASEWSPEER_H_INCLUDED
#define RIPPLE_SERVER_BASEWSPEER_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/server/impl/BasePeer.h>
#include <ripple/protocol/BuildInfo.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/crypto/csprng.h>
#include <boost/beast/websocket.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/message.hpp>
#include <cassert>
#include <functional>

namespace ripple {

/*表示活动的WebSocket连接。*/
template<class Handler, class Impl>
class BaseWSPeer
    : public BasePeer<Handler, Impl>
    , public WSSession
{
protected:
    using clock_type = std::chrono::system_clock;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using waitable_timer = boost::asio::basic_waitable_timer <clock_type>;
    using BasePeer<Handler, Impl>::strand_;

private:
    friend class BasePeer<Handler, Impl>;

    http_request_type request_;
    boost::beast::multi_buffer rb_;
    boost::beast::multi_buffer wb_;
    std::list<std::shared_ptr<WSMsg>> wq_;
    bool do_close_ = false;
    boost::beast::websocket::close_reason cr_;
    waitable_timer timer_;
    bool close_on_timer_ = false;
    bool ping_active_ = false;
    boost::beast::websocket::ping_data payload_;
    error_code ec_;
    std::function<void (boost::beast::websocket::frame_type , boost::beast::string_view)> control_callback_;
public:
    template<class Body, class Headers>
    BaseWSPeer(
        Port const& port,
        Handler& handler,
        endpoint_type remote_address,
        boost::beast::http::request<Body, Headers>&& request,
        boost::asio::io_service& io_service,
        beast::Journal journal);

    void
    run() override;

//
//WSSIsAccess
//

    Port const&
    port() const override
    {
        return this->port_;
    }

    http_request_type const&
    request() const override
    {
        return this->request_;
    }

    boost::asio::ip::tcp::endpoint const&
    remote_endpoint() const override
    {
        return this->remote_address_;
    }

    void
    send(std::shared_ptr<WSMsg> w) override;

    void
    close() override;

    void
    complete() override;

protected:
    Impl&
    impl()
    {
        return *static_cast<Impl*>(this);
    }

    void
    on_ws_handshake(error_code const& ec);

    void
    do_write();

    void
    on_write(error_code const& ec);

    void
    on_write_fin(error_code const& ec);

    void
    do_read();

    void
    on_read(error_code const& ec);

    void
    on_close(error_code const& ec);

    void
    start_timer();

    void
    cancel_timer();

    void
    on_ping(error_code const& ec);

    void
    on_ping_pong(boost::beast::websocket::frame_type kind,
        boost::beast::string_view payload);

    void
    on_timer(error_code ec);

    template<class String>
    void
    fail(error_code ec, String const& what);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler, class Impl>
template<class Body, class Headers>
BaseWSPeer<Handler, Impl>::
BaseWSPeer(
    Port const& port,
    Handler& handler,
    endpoint_type remote_address,
    boost::beast::http::request<Body, Headers>&& request,
    boost::asio::io_service& io_service,
    beast::Journal journal)
    : BasePeer<Handler, Impl>(port, handler, remote_address,
        io_service, journal)
    , request_(std::move(request))
    , timer_(io_service)
, payload_ ("12345678") //确保大小为8字节
{
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
run()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::run, impl().shared_from_this()));
    impl().ws_.set_option(port().pmd_options);
//必须在'control_callback'函数之外管理控件回调内存
    control_callback_ = std::bind(
        &BaseWSPeer::on_ping_pong,
        this, std::placeholders::_1, std::placeholders::_2);
    impl().ws_.control_callback(control_callback_);
    start_timer();
    close_on_timer_ = true;
    impl().ws_.async_accept_ex(request_,
        [](auto & res)
        {
            res.set(boost::beast::http::field::server,
                BuildInfo::getFullVersionString());
        },
        strand_.wrap(std::bind(&BaseWSPeer::on_ws_handshake,
            impl().shared_from_this(), std::placeholders::_1)));
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
send(std::shared_ptr<WSMsg> w)
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::send, impl().shared_from_this(),
                std::move(w)));
    if(do_close_)
        return;
    if(wq_.size() > port().ws_queue_limit)
    {
        JLOG(this->j_.info()) <<
            "closing slow client";
        cr_.code = safe_cast<decltype(cr_.code)>
                      (boost::beast::websocket::close_code::policy_error);
        cr_.reason = "Policy error: client is too slow.";
        wq_.erase(std::next(wq_.begin()), wq_.end());
        close();
        return;
    }
    wq_.emplace_back(std::move(w));
    if(wq_.size() == 1)
        on_write({});
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
close()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::close, impl().shared_from_this()));
    do_close_ = true;
    if(wq_.empty())
        impl().ws_.async_close({}, strand_.wrap(std::bind(
            &BaseWSPeer::on_close, impl().shared_from_this(),
                std::placeholders::_1)));
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
complete()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::complete, impl().shared_from_this()));
    do_read();
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_ws_handshake(error_code const& ec)
{
    if(ec)
        return fail(ec, "on_ws_handshake");
    close_on_timer_ = false;
    do_read();
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
do_write()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::do_write, impl().shared_from_this()));
    on_write({});
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_write(error_code const& ec)
{
    if(ec)
        return fail(ec, "write");
    auto& w = *wq_.front();
    auto const result = w.prepare(65536,
        std::bind(&BaseWSPeer::do_write,
            impl().shared_from_this()));
    if(boost::indeterminate(result.first))
        return;
    start_timer();
    if(! result.first)
        impl().ws_.async_write_some(
            result.first, result.second, strand_.wrap(std::bind(
                &BaseWSPeer::on_write, impl().shared_from_this(),
                    std::placeholders::_1)));
    else
        impl().ws_.async_write_some(
            result.first, result.second, strand_.wrap(std::bind(
                &BaseWSPeer::on_write_fin, impl().shared_from_this(),
                    std::placeholders::_1)));
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_write_fin(error_code const& ec)
{
    if(ec)
        return fail(ec, "write_fin");
    wq_.pop_front();
    if(do_close_)
        impl().ws_.async_close(cr_, strand_.wrap(std::bind(
            &BaseWSPeer::on_close, impl().shared_from_this(),
                std::placeholders::_1)));
    else if(! wq_.empty())
        on_write({});
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
do_read()
{
    if(! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BaseWSPeer::do_read, impl().shared_from_this()));
    impl().ws_.async_read(rb_, strand_.wrap(
        std::bind(&BaseWSPeer::on_read,
            impl().shared_from_this(), std::placeholders::_1)));
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_read(error_code const& ec)
{
    if(ec == boost::beast::websocket::error::closed)
        return on_close({});
    if(ec)
        return fail(ec, "read");
    auto const& data = rb_.data();
    std::vector<boost::asio::const_buffer> b;
    b.reserve(std::distance(data.begin(), data.end()));
    std::copy(data.begin(), data.end(),
        std::back_inserter(b));
    this->handler_.onWSMessage(impl().shared_from_this(), b);
    rb_.consume(rb_.size());
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_close(error_code const& ec)
{
    cancel_timer();
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
start_timer()
{
//不完成消息的最长秒数
    static constexpr std::chrono::seconds timeout{30};
    static constexpr std::chrono::seconds timeoutLocal{3};
    error_code ec;
    timer_.expires_from_now(
        remote_endpoint().address().is_loopback() ? timeoutLocal : timeout,
        ec);
    if(ec)
        return fail(ec, "start_timer");
    timer_.async_wait(strand_.wrap(std::bind(
        &BaseWSPeer<Handler, Impl>::on_timer, impl().shared_from_this(),
            std::placeholders::_1)));
}

//丢弃错误代码的便利性
template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
cancel_timer()
{
    error_code ec;
    timer_.cancel(ec);
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_ping(error_code const& ec)
{
    if(ec == boost::asio::error::operation_aborted)
        return;
    ping_active_ = false;
    if(! ec)
        return;
    fail(ec, "on_ping");
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_ping_pong(boost::beast::websocket::frame_type kind,
    boost::beast::string_view payload)
{
    if(kind == boost::beast::websocket::frame_type::pong)
    {
        boost::beast::string_view p(payload_.begin());
        if(payload == p)
        {
            close_on_timer_ = false;
            JLOG(this->j_.trace()) <<
                "got matching pong";
        }
        else
        {
            JLOG(this->j_.trace()) <<
                "got pong";
        }
    }
}

template<class Handler, class Impl>
void
BaseWSPeer<Handler, Impl>::
on_timer(error_code ec)
{
    if(ec == boost::asio::error::operation_aborted)
        return;
    if(! ec)
    {
        if(! close_on_timer_ || ! ping_active_)
        {
            start_timer();
            close_on_timer_ = true;
            ping_active_ = true;
//加密可能是杀戮过度。
            beast::rngfill(payload_.begin(),
                payload_.size(), crypto_prng());
            impl().ws_.async_ping(payload_,
                strand_.wrap(std::bind(
                    &BaseWSPeer::on_ping,
                        impl().shared_from_this(),
                            std::placeholders::_1)));
            JLOG(this->j_.trace()) <<
                "sent ping";
            return;
        }
        ec = boost::system::errc::make_error_code(
            boost::system::errc::timed_out);
    }
    fail(ec, "timer");
}

template<class Handler, class Impl>
template<class String>
void
BaseWSPeer<Handler, Impl>::
fail(error_code ec, String const& what)
{
    assert(strand_.running_in_this_thread());

    cancel_timer();
    if(! ec_ &&
        ec != boost::asio::error::operation_aborted)
    {
        ec_ = ec;
        JLOG(this->j_.trace()) <<
            what << ": " << ec.message();
        impl().ws_.lowest_layer().close(ec);
    }
}

} //涟漪

#endif
