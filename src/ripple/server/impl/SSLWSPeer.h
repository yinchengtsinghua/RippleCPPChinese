
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

#ifndef RIPPLE_SERVER_SSLWSPEER_H_INCLUDED
#define RIPPLE_SERVER_SSLWSPEER_H_INCLUDED

#include <ripple/server/impl/BaseHTTPPeer.h>
#include <ripple/server/WSSession.h>
#include <ripple/beast/asio/ssl_bundle.h>
#include <boost/beast/websocket/ssl.hpp>
#include <memory>

namespace ripple {

template<class Handler>
class SSLWSPeer
    : public BaseWSPeer<Handler, SSLWSPeer<Handler>>
    , public std::enable_shared_from_this<SSLWSPeer<Handler>>
{
    friend class BasePeer<Handler, SSLWSPeer>;
    friend class BaseWSPeer<Handler, SSLWSPeer>;

    using clock_type = std::chrono::system_clock;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using waitable_timer =
        boost::asio::basic_waitable_timer <clock_type>;

    std::unique_ptr<beast::asio::ssl_bundle> ssl_bundle_;
    boost::beast::websocket::stream<
        beast::asio::ssl_bundle::stream_type&> ws_;

public:
    template<class Body, class Headers>
    SSLWSPeer(
        Port const& port,
        Handler& handler,
        endpoint_type remote_endpoint,
        boost::beast::http::request<Body, Headers>&& request,
        std::unique_ptr<
            beast::asio::ssl_bundle>&& ssl_bundle,
        beast::Journal journal);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler>
template<class Body, class Headers>
SSLWSPeer<Handler>::
SSLWSPeer(
    Port const& port,
    Handler& handler,
    endpoint_type remote_endpoint,
    boost::beast::http::request<Body, Headers>&& request,
    std::unique_ptr<
        beast::asio::ssl_bundle>&& ssl_bundle,
    beast::Journal journal)
    : BaseWSPeer<Handler, SSLWSPeer>(port, handler,
        remote_endpoint, std::move(request),
            ssl_bundle->socket.get_io_service(), journal)
    , ssl_bundle_(std::move(ssl_bundle))
    , ws_(ssl_bundle_->stream)
{
}

} //涟漪

#endif
