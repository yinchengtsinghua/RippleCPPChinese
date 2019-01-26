
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
    版权所有（c）2016 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_MISC_DETAIL_WORKBASE_H_INCLUDED
#define RIPPLE_APP_MISC_DETAIL_WORKBASE_H_INCLUDED

#include <ripple/app/misc/detail/Work.h>
#include <ripple/protocol/BuildInfo.h>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio.hpp>

namespace ripple {

namespace detail {

template <class Impl>
class WorkBase
    : public Work
{
protected:
    using error_code = boost::system::error_code;

public:
    using callback_type =
        std::function<void(error_code const&, response_type&&)>;
protected:
    using socket_type = boost::asio::ip::tcp::socket;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using resolver_type = boost::asio::ip::tcp::resolver;
    using query_type = resolver_type::query;
    using request_type =
        boost::beast::http::request<boost::beast::http::empty_body>;

    std::string host_;
    std::string path_;
    std::string port_;
    callback_type cb_;
    boost::asio::io_service& ios_;
    boost::asio::io_service::strand strand_;
    resolver_type resolver_;
    socket_type socket_;
    request_type req_;
    response_type res_;
    boost::beast::multi_buffer read_buf_;

public:
    WorkBase(
        std::string const& host, std::string const& path,
        std::string const& port,
        boost::asio::io_service& ios, callback_type cb);
    ~WorkBase();

    Impl&
    impl()
    {
        return *static_cast<Impl*>(this);
    }

    void run() override;

    void cancel() override;

    void
    fail(error_code const& ec);

    void
    onResolve(error_code const& ec, resolver_type::iterator it);

    void
    onStart();

    void
    onRequest(error_code const& ec);

    void
    onResponse(error_code const& ec);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Impl>
WorkBase<Impl>::WorkBase(std::string const& host,
    std::string const& path, std::string const& port,
    boost::asio::io_service& ios, callback_type cb)
    : host_(host)
    , path_(path)
    , port_(port)
    , cb_(std::move(cb))
    , ios_(ios)
    , strand_(ios)
    , resolver_(ios)
    , socket_(ios)
{
}

template<class Impl>
WorkBase<Impl>::~WorkBase()
{
    if (cb_)
        cb_ (make_error_code(boost::system::errc::not_a_socket),
            std::move(res_));
}

template<class Impl>
void
WorkBase<Impl>::run()
{
    if (! strand_.running_in_this_thread())
        return ios_.post(strand_.wrap (std::bind(
            &WorkBase::run, impl().shared_from_this())));

    resolver_.async_resolve(
        query_type{host_, port_},
        strand_.wrap (std::bind(&WorkBase::onResolve, impl().shared_from_this(),
            std::placeholders::_1,
                std::placeholders::_2)));
}

template<class Impl>
void
WorkBase<Impl>::cancel()
{
    if (! strand_.running_in_this_thread())
    {
        return ios_.post(strand_.wrap (std::bind(
            &WorkBase::cancel, impl().shared_from_this())));
    }

    error_code ec;
    resolver_.cancel();
    socket_.cancel (ec);
}

template<class Impl>
void
WorkBase<Impl>::fail(error_code const& ec)
{
    if (cb_)
    {
        cb_(ec, std::move(res_));
        cb_ = nullptr;
    }
}

template<class Impl>
void
WorkBase<Impl>::onResolve(error_code const& ec, resolver_type::iterator it)
{
    if (ec)
        return fail(ec);

    socket_.async_connect(*it,
        strand_.wrap (std::bind(&Impl::onConnect, impl().shared_from_this(),
            std::placeholders::_1)));
}

template<class Impl>
void
WorkBase<Impl>::onStart()
{
    req_.method(boost::beast::http::verb::get);
    req_.target(path_.empty() ? "/" : path_);
    req_.version(11);
    req_.set (
        "Host", host_ + ":" + port_);
    req_.set ("User-Agent", BuildInfo::getFullVersionString());
    req_.prepare_payload();
    boost::beast::http::async_write(impl().stream(), req_,
        strand_.wrap (std::bind (&WorkBase::onRequest,
            impl().shared_from_this(), std::placeholders::_1)));
}

template<class Impl>
void
WorkBase<Impl>::onRequest(error_code const& ec)
{
    if (ec)
        return fail(ec);

    boost::beast::http::async_read (impl().stream(), read_buf_, res_,
        strand_.wrap (std::bind (&WorkBase::onResponse,
            impl().shared_from_this(), std::placeholders::_1)));
}

template<class Impl>
void
WorkBase<Impl>::onResponse(error_code const& ec)
{
    if (ec)
        return fail(ec);

    assert(cb_);
    cb_(ec, std::move(res_));
    cb_ = nullptr;
}

} //细节

} //涟漪

#endif
