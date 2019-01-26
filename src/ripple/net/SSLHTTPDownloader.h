
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
    版权所有（c）2012，2018 Ripple Labs Inc.

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

#ifndef RIPPLE_NET_SSLHTTPDOWNLOADER_H_INCLUDED
#define RIPPLE_NET_SSLHTTPDOWNLOADER_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/filesystem.hpp>

#include <memory>

namespace ripple {

/*提供异步HTTPS文件下载程序
**/

class SSLHTTPDownloader
    : public std::enable_shared_from_this <SSLHTTPDownloader>
{
public:
    using error_code = boost::system::error_code;

    SSLHTTPDownloader(
        boost::asio::io_service& io_service,
        beast::Journal j);

    bool
    init(Config const& config);

    bool
    download(
        std::string const& host,
        std::string const& port,
        std::string const& target,
        int version,
        boost::filesystem::path const& dstPath,
        std::function<void(boost::filesystem::path)> complete);

private:
    boost::asio::ssl::context ctx_;
    boost::asio::io_service::strand strand_;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
    boost::beast::flat_buffer read_buf_;
    beast::Journal j_;

    void
    do_session(
        std::string host,
        std::string port,
        std::string target,
        int version,
        boost::filesystem::path dstPath,
        std::function<void(boost::filesystem::path)> complete,
        boost::asio::yield_context yield);
};

} //涟漪

#endif
