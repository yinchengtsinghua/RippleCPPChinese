
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

#ifndef RIPPLE_APP_MISC_DETAIL_WORKSSL_H_INCLUDED
#define RIPPLE_APP_MISC_DETAIL_WORKSSL_H_INCLUDED

#include <ripple/app/misc/detail/WorkBase.h>
#include <ripple/net/RegisterSSLCerts.h>
#include <ripple/basics/contract.h>
#include <boost/asio/ssl.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

namespace ripple {

namespace detail {

class SSLContext : public boost::asio::ssl::context
{
public:
    SSLContext(beast::Journal j)
    : boost::asio::ssl::context(boost::asio::ssl::context::sslv23)
    {
        boost::system::error_code ec;
        registerSSLCerts(*this, ec, j);
        if (ec)
        {
            Throw<std::runtime_error> (
                boost::str (boost::format (
                    "Failed to set_default_verify_paths: %s") %
                    ec.message ()));
        }
    }
};

//在SSL上工作
class WorkSSL : public WorkBase<WorkSSL>
    , public std::enable_shared_from_this<WorkSSL>
{
    friend class WorkBase<WorkSSL>;

private:
    using stream_type = boost::asio::ssl::stream<socket_type&>;

    SSLContext context_;
    stream_type stream_;

public:
    WorkSSL(
        std::string const& host,
        std::string const& path,
        std::string const& port,
        boost::asio::io_service& ios,
        beast::Journal j,
        callback_type cb);
    ~WorkSSL() = default;

private:
    stream_type&
    stream()
    {
        return stream_;
    }

    void
    onConnect(error_code const& ec);

    void
    onHandshake(error_code const& ec);

    static bool
    rfc2818_verify(
        std::string const& domain,
        bool preverified,
        boost::asio::ssl::verify_context& ctx)
    {
        return boost::asio::ssl::rfc2818_verification(domain)(preverified, ctx);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

WorkSSL::WorkSSL(
    std::string const& host,
    std::string const& path,
    std::string const& port,
    boost::asio::io_service& ios,
    beast::Journal j,
    callback_type cb)
    : WorkBase(host, path, port, ios, cb)
    , context_(j)
    , stream_(socket_, context_)
{
//设置SNI主机名
    SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str());
    stream_.set_verify_mode (boost::asio::ssl::verify_peer);
    stream_.set_verify_callback(    std::bind (
            &WorkSSL::rfc2818_verify, host_,
            std::placeholders::_1, std::placeholders::_2));
}

void
WorkSSL::onConnect(error_code const& ec)
{
    if (ec)
        return fail(ec);

    stream_.async_handshake(
        boost::asio::ssl::stream_base::client,
        strand_.wrap (boost::bind(&WorkSSL::onHandshake, shared_from_this(),
            boost::asio::placeholders::error)));
}

void
WorkSSL::onHandshake(error_code const& ec)
{
    if (ec)
        return fail(ec);

    onStart ();
}

} //细节

} //涟漪

#endif
