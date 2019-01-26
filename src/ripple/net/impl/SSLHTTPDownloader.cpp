
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

#include <ripple/net/SSLHTTPDownloader.h>
#include <ripple/net/RegisterSSLCerts.h>

namespace ripple {

SSLHTTPDownloader::SSLHTTPDownloader(
    boost::asio::io_service& io_service,
    beast::Journal j)
    : ctx_(boost::asio::ssl::context::sslv23_client)
    , strand_(io_service)
    , stream_(io_service, ctx_)
    , j_(j)
{
}

bool
SSLHTTPDownloader::init(Config const& config)
{
    boost::system::error_code ec;
    if (config.SSL_VERIFY_FILE.empty())
    {
        registerSSLCerts(ctx_, ec, j_);
        if (ec && config.SSL_VERIFY_DIR.empty())
        {
            JLOG(j_.error()) <<
                "Failed to set_default_verify_paths: " <<
                ec.message();
            return false;
        }
    }
    else
        ctx_.load_verify_file(config.SSL_VERIFY_FILE);

    if (!config.SSL_VERIFY_DIR.empty())
    {
        ctx_.add_verify_path(config.SSL_VERIFY_DIR, ec);
        if (ec)
        {
            JLOG(j_.error()) <<
                "Failed to add verify path: " <<
                ec.message();
            return false;
        }
    }
    return true;
}

bool
SSLHTTPDownloader::download(
    std::string const& host,
    std::string const& port,
    std::string const& target,
    int version,
    boost::filesystem::path const& dstPath,
    std::function<void(boost::filesystem::path)> complete)
{
    try
    {
        if (exists(dstPath))
        {
            JLOG(j_.error()) <<
                "Destination file exists";
            return false;
        }
    }
    catch (std::exception const& e)
    {
        JLOG(j_.error()) <<
            "exception: " << e.what();
        return false;
    }

    if (!strand_.running_in_this_thread())
        strand_.post(
            std::bind(
                &SSLHTTPDownloader::download,
                this->shared_from_this(),
                host,
                port,
                target,
                version,
                dstPath,
                complete));
    else
        boost::asio::spawn(
            strand_,
            std::bind(
                &SSLHTTPDownloader::do_session,
                this->shared_from_this(),
                host,
                port,
                target,
                version,
                dstPath,
                complete,
                std::placeholders::_1));
    return true;
}

void
SSLHTTPDownloader::do_session(
    std::string const host,
    std::string const port,
    std::string const target,
    int version,
    boost::filesystem::path dstPath,
    std::function<void(boost::filesystem::path)> complete,
    boost::asio::yield_context yield)
{
    using namespace boost::asio;
    using namespace boost::beast;

    boost::system::error_code ec;
    auto fail = [&](std::string errMsg)
    {
        if (ec != boost::asio::error::operation_aborted)
        {
            JLOG(j_.error()) <<
                errMsg << ": " << ec.message();
        }
        try
        {
            remove(dstPath);
        }
        catch (std::exception const& e)
        {
            JLOG(j_.error()) <<
                "exception: " << e.what();
        }
        complete(std::move(dstPath));
    };

    if (!SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str()))
    {
        ec.assign(static_cast<int>(
            ::ERR_get_error()), boost::asio::error::get_ssl_category());
        return fail("SSL_set_tlsext_host_name");
    }

    ip::tcp::resolver resolver {stream_.get_io_context()};
    auto const results = resolver.async_resolve(host, port, yield[ec]);
    if (ec)
        return fail("async_resolve");

    boost::asio::async_connect(
        stream_.next_layer(), results.begin(), results.end(), yield[ec]);
    if (ec)
        return fail("async_connect");

    stream_.async_handshake(ssl::stream_base::client, yield[ec]);
    if (ec)
        return fail("async_handshake");

//设置HTTP头请求消息以查找文件大小
    http::request<http::empty_body> req {http::verb::head, target, version};
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::async_write(stream_, req, yield[ec]);
    if(ec)
        return fail("async_write");

    {
//检查文件大小的可用存储空间
        http::response_parser<http::empty_body> p;
        p.skip(true);
        http::async_read(stream_, read_buf_, p, yield[ec]);
        if(ec)
            return fail("async_read");
        if (auto len = p.content_length())
        {
            try
            {
                if (*len > space(dstPath.parent_path()).available)
                    return fail("Insufficient disk space for download");
            }
            catch (std::exception const& e)
            {
                JLOG(j_.error()) <<
                    "exception: " << e.what();
                return fail({});
            }
        }
    }

//设置HTTP GET请求消息以下载文件
    req.method(http::verb::get);
    http::async_write(stream_, req, yield[ec]);
    if(ec)
        return fail("async_write");

//下载文件
    http::response_parser<http::file_body> p;
    p.body_limit(std::numeric_limits<std::uint64_t>::max());
    p.get().body().open(
        dstPath.string().c_str(),
        boost::beast::file_mode::write,
        ec);
    if (ec)
    {
        p.get().body().close();
        return fail("open");
    }

    http::async_read(stream_, read_buf_, p, yield[ec]);
    if (ec)
    {
        p.get().body().close();
        return fail("async_read");
    }
    p.get().body().close();

//优雅地关闭小溪
    stream_.async_shutdown(yield[ec]);
    if (ec == boost::asio::error::eof)
        ec.assign(0, ec.category());
    if (ec)
    {
//大多数Web服务器不需要执行
//为了提高速度，SSL关闭握手。
        JLOG(j_.trace()) <<
            "async_shutdown: " << ec.message();
    }

    JLOG(j_.trace()) <<
        "download completed: " << dstPath.string();

//通知完成处理程序
    complete(std::move(dstPath));
}

}// ripple
