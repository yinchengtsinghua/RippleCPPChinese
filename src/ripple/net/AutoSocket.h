
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

#ifndef RIPPLE_WEBSOCKET_AUTOSOCKET_AUTOSOCKET_H_INCLUDED
#define RIPPLE_WEBSOCKET_AUTOSOCKET_AUTOSOCKET_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/beast/net/IPAddressConversion.h>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

//同时支持SSL和非SSL连接的套接字包装器。
//通常，像处理SSL连接一样处理它。
//要强制非SSL连接，只需不要调用异步握手。
//若要强制仅限SSL入站，请调用setssonly。

class AutoSocket
{
public:
    using ssl_socket   = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
    using endpoint_type     = boost::asio::ip::tcp::socket::endpoint_type;
    using socket_ptr        = std::unique_ptr<ssl_socket>;
    using plain_socket      = ssl_socket::next_layer_type;
    using lowest_layer_type = ssl_socket::lowest_layer_type;
    using handshake_type    = ssl_socket::handshake_type;
    using error_code        = boost::system::error_code;
    using callback          = std::function <void (error_code)>;

public:
    AutoSocket (
            boost::asio::io_service& s,
            boost::asio::ssl::context& c,
            bool secureOnly,
            bool plainOnly)
        : mSecure (secureOnly)
        , mBuffer ((plainOnly || secureOnly) ? 0 : 4)
        , j_ {beast::Journal::getNullSink()}
    {
        mSocket = std::make_unique<ssl_socket> (s, c);
    }

    AutoSocket (
            boost::asio::io_service& s,
            boost::asio::ssl::context& c)
        : AutoSocket (s, c, false, false)
    {
    }

    boost::asio::io_service& get_io_service () noexcept
    {
        return mSocket->get_io_service ();
    }

    bool            isSecure ()
    {
        return mSecure;
    }
    ssl_socket&     SSLSocket ()
    {
        return *mSocket;
    }
    plain_socket&   PlainSocket ()
    {
        return mSocket->next_layer ();
    }
    void setSSLOnly ()
    {
        mSecure = true;
    }
    void setPlainOnly ()
    {
        mBuffer.clear ();
    }

    beast::IP::Endpoint
    local_endpoint()
    {
        return beast::IP::from_asio(
            lowest_layer().local_endpoint());
    }

    beast::IP::Endpoint
    remote_endpoint()
    {
        return beast::IP::from_asio(
            lowest_layer().remote_endpoint());
    }

    lowest_layer_type& lowest_layer ()
    {
        return mSocket->lowest_layer ();
    }

    void swap (AutoSocket& s) noexcept
    {
        mBuffer.swap (s.mBuffer);
        mSocket.swap (s.mSocket);
        std::swap (mSecure, s.mSecure);
    }

    boost::system::error_code cancel (boost::system::error_code& ec)
    {
        return lowest_layer ().cancel (ec);
    }


    static bool rfc2818_verify (std::string const& domain, bool preverified,
                                boost::asio::ssl::verify_context& ctx, beast::Journal j)
    {
        using namespace ripple;

        if (boost::asio::ssl::rfc2818_verification (domain) (preverified, ctx))
            return true;

        JLOG (j.warn()) <<
            "Outbound SSL connection to " << domain <<
            " fails certificate verification";
        return false;
    }

    boost::system::error_code verify (std::string const& strDomain)
    {
        boost::system::error_code ec;

        mSocket->set_verify_mode (boost::asio::ssl::verify_peer);

//XXX验证RFC2818的语义是我们想要的。
        mSocket->set_verify_callback (
            std::bind (&rfc2818_verify, strDomain,
                       std::placeholders::_1, std::placeholders::_2, j_), ec);

        return ec;
    }

    void setTLSHostName(std::string const & host)
    {
        SSL_set_tlsext_host_name(mSocket->native_handle(), host.c_str());
    }

    void async_handshake (handshake_type type, callback cbFunc)
    {
        if ((type == ssl_socket::client) || (mSecure))
        {
//必须是SSL
            mSecure = true;
            mSocket->async_handshake (type, cbFunc);
        }
        else if (mBuffer.empty ())
        {
//必须是朴素的
            mSecure = false;
            mSocket->get_io_service ().post (
                boost::beast::bind_handler (cbFunc, error_code()));
        }
        else
        {
//自动检测
            mSocket->next_layer ().async_receive (
                boost::asio::buffer (mBuffer),
                boost::asio::socket_base::message_peek,
                std::bind (
                    &AutoSocket::handle_autodetect,
                    this, cbFunc,
                    std::placeholders::_1,
                    std::placeholders::_2));
        }
    }

    template <typename ShutdownHandler>
    void async_shutdown (ShutdownHandler handler)
    {
        if (isSecure ())
            mSocket->async_shutdown (handler);
        else
        {
            error_code ec;
            try
            {
                lowest_layer ().shutdown (plain_socket::shutdown_both);
            }
            catch (boost::system::system_error& e)
            {
                ec = e.code();
            }
            mSocket->get_io_service ().post (
                boost::beast::bind_handler (handler, ec));
        }
    }

    template <typename Seq, typename Handler>
    void async_read_some (const Seq& buffers, Handler handler)
    {
        if (isSecure ())
            mSocket->async_read_some (buffers, handler);
        else
            PlainSocket ().async_read_some (buffers, handler);
    }

    template <typename Seq, typename Condition, typename Handler>
    void async_read_until(
        const Seq& buffers, Condition condition, Handler handler)
    {
        if (isSecure())
            boost::asio::async_read_until(
                *mSocket, buffers, condition, handler);
        else
            boost::asio::async_read_until(
                PlainSocket (), buffers, condition, handler);
    }

    template <typename Allocator, typename Handler>
    void async_read_until(boost::asio::basic_streambuf<Allocator>& buffers,
                          std::string const& delim, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_read_until(*mSocket, buffers, delim, handler);
        else
            boost::asio::async_read_until(
                PlainSocket(), buffers, delim, handler);
    }

    template <typename Allocator, typename MatchCondition, typename Handler>
    void async_read_until (boost::asio::basic_streambuf<Allocator>& buffers,
                           MatchCondition cond, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_read_until(*mSocket, buffers, cond, handler);
        else
            boost::asio::async_read_until(
                PlainSocket(), buffers, cond, handler);
    }

    template <typename Buf, typename Handler>
    void async_write (const Buf& buffers, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_write(*mSocket, buffers, handler);
        else
            boost::asio::async_write(PlainSocket (), buffers, handler);
    }

    template <typename Allocator, typename Handler>
    void async_write (boost::asio::basic_streambuf<Allocator>& buffers,
                      Handler handler)
    {
        if (isSecure ())
            boost::asio::async_write(*mSocket, buffers, handler);
        else
            boost::asio::async_write(PlainSocket(), buffers, handler);
    }

    template <typename Buf, typename Condition, typename Handler>
    void async_read (const Buf& buffers, Condition cond, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_read(*mSocket, buffers, cond, handler);
        else
            boost::asio::async_read(PlainSocket(), buffers, cond, handler);
    }

    template <typename Allocator, typename Condition, typename Handler>
    void async_read (boost::asio::basic_streambuf<Allocator>& buffers,
                     Condition cond, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_read (*mSocket, buffers, cond, handler);
        else
            boost::asio::async_read (PlainSocket (), buffers, cond, handler);
    }

    template <typename Buf, typename Handler>
    void async_read (const Buf& buffers, Handler handler)
    {
        if (isSecure ())
            boost::asio::async_read (*mSocket, buffers, handler);
        else
            boost::asio::async_read (PlainSocket (), buffers, handler);
    }

    template <typename Seq, typename Handler>
    void async_write_some (const Seq& buffers, Handler handler)
    {
        if (isSecure ())
            mSocket->async_write_some (buffers, handler);
        else
            PlainSocket ().async_write_some (buffers, handler);
    }

protected:
    void handle_autodetect (
        callback cbFunc, const error_code& ec, size_t bytesTransferred)
    {
        using namespace ripple;

        if (ec)
        {
            JLOG (j_.warn()) <<
                "Handle autodetect error: " << ec;
            cbFunc (ec);
        }
        else if ((mBuffer[0] < 127) && (mBuffer[0] > 31) &&
                 ((bytesTransferred < 2)
                  || ((mBuffer[1] < 127) && (mBuffer[1] > 31))) &&
                 ((bytesTransferred < 3)
                  || ((mBuffer[2] < 127) && (mBuffer[2] > 31))) &&
                 ((bytesTransferred < 4)
                  || ((mBuffer[3] < 127) && (mBuffer[3] > 31))))
        {
//非SSL
            JLOG (j_.trace()) << "non-SSL";
            mSecure = false;
            cbFunc (ec);
        }
        else
        {
//SSL
            JLOG (j_.trace()) << "SSL";
            mSecure = true;
            mSocket->async_handshake (ssl_socket::server, cbFunc);
        }
    }

private:
    socket_ptr          mSocket;
    bool                mSecure;
    std::vector<char>   mBuffer;
    beast::Journal      j_;
};

#endif
