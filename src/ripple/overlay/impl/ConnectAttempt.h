
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

#ifndef RIPPLE_OVERLAY_CONNECTATTEMPT_H_INCLUDED
#define RIPPLE_OVERLAY_CONNECTATTEMPT_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <ripple/overlay/impl/OverlayImpl.h>
#include <ripple/overlay/impl/Tuning.h>

namespace ripple {

/*管理出站连接尝试。*/
class ConnectAttempt
    : public OverlayImpl::Child
    , public std::enable_shared_from_this<ConnectAttempt>
{
private:
    using error_code = boost::system::error_code;

    using endpoint_type = boost::asio::ip::tcp::endpoint;

    using request_type =
        boost::beast::http::request<boost::beast::http::empty_body>;

    using response_type =
        boost::beast::http::response<boost::beast::http::dynamic_body>;

    Application& app_;
    std::uint32_t const id_;
    beast::WrappedSink sink_;
    beast::Journal journal_;
    endpoint_type remote_endpoint_;
    Resource::Consumer usage_;
    boost::asio::io_service::strand strand_;
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> timer_;
    std::unique_ptr<beast::asio::ssl_bundle> ssl_bundle_;
    beast::asio::ssl_bundle::socket_type& socket_;
    beast::asio::ssl_bundle::stream_type& stream_;
    boost::beast::multi_buffer read_buf_;
    response_type response_;
    PeerFinder::Slot::ptr slot_;
    request_type req_;

public:
    ConnectAttempt (Application& app, boost::asio::io_service& io_service,
        endpoint_type const& remote_endpoint, Resource::Consumer usage,
            beast::asio::ssl_bundle::shared_context const& context,
                std::uint32_t id, PeerFinder::Slot::ptr const& slot,
                    beast::Journal journal, OverlayImpl& overlay);

    ~ConnectAttempt();

    void
    stop() override;

    void
    run();

private:

    void close();
    void fail (std::string const& reason);
    void fail (std::string const& name, error_code ec);
    void setTimer();
    void cancelTimer();
    void onTimer (error_code ec);
    void onConnect (error_code ec);
    void onHandshake (error_code ec);
    void onWrite (error_code ec);
    void onRead (error_code ec);
    void onShutdown (error_code ec);

    static
    request_type
    makeRequest (bool crawl,
        boost::asio::ip::address const& remote_address);

    void processResponse();

    template <class = void>
    static
    boost::asio::ip::tcp::endpoint
    parse_endpoint (std::string const& s, boost::system::error_code& ec)
    {
        beast::IP::Endpoint bep;
        std::istringstream is(s);
        is >> bep;
        if (is.fail())
        {
            ec = boost::system::errc::make_error_code(
                boost::system::errc::invalid_argument);
            return boost::asio::ip::tcp::endpoint{};
        }

        return beast::IPAddressConversion::to_asio_endpoint(bep);
    }
};

}

#endif
