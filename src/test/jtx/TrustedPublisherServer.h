
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
    版权所有2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_TRUSTED_PUBLISHER_SERVER_H_INCLUDED
#define RIPPLE_TEST_TRUSTED_PUBLISHER_SERVER_H_INCLUDED

#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Sign.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/strHex.h>
#include <test/jtx/envconfig.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/http.hpp>

namespace ripple {
namespace test {

class TrustedPublisherServer
{
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using address_type = boost::asio::ip::address;
    using socket_type = boost::asio::ip::tcp::socket;

    using req_type = boost::beast::http::request<boost::beast::http::string_body>;
    using resp_type = boost::beast::http::response<boost::beast::http::string_body>;
    using error_code = boost::system::error_code;

    socket_type sock_;
    boost::asio::ip::tcp::acceptor acceptor_;

    std::string list_;

public:

    struct Validator
    {
        PublicKey masterPublic;
        PublicKey signingPublic;
        std::string manifest;
    };

    TrustedPublisherServer(
        boost::asio::io_service& ios,
        std::pair<PublicKey, SecretKey> keys,
        std::string const& manifest,
        int sequence,
        NetClock::time_point expiration,
        int version,
        std::vector<Validator> const& validators)
        : sock_(ios), acceptor_(ios)
    {
        endpoint_type const& ep {
            beast::IP::Address::from_string (ripple::test::getEnvLocalhostAddr()),
0}; //0表示让操作系统根据可用的端口选择端口
        std::string data = "{\"sequence\":" + std::to_string(sequence) +
            ",\"expiration\":" +
            std::to_string(expiration.time_since_epoch().count()) +
            ",\"validators\":[";

        for (auto const& val : validators)
        {
            data += "{\"validation_public_key\":\"" + strHex(val.masterPublic) +
                "\",\"manifest\":\"" + val.manifest + "\"},";
        }
        data.pop_back();
        data += "]}";
        std::string blob = base64_encode(data);

        list_ = "{\"blob\":\"" + blob + "\"";

        auto const sig = sign(keys.first, keys.second, makeSlice(data));

        list_ += ",\"signature\":\"" + strHex(sig) + "\"";
        list_ += ",\"manifest\":\"" + manifest + "\"";
        list_ += ",\"version\":" + std::to_string(version) + '}';

        acceptor_.open(ep.protocol());
        error_code ec;
        acceptor_.set_option(
            boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
        acceptor_.bind(ep);
        acceptor_.listen(boost::asio::socket_base::max_connections);
        acceptor_.async_accept(
            sock_,
            std::bind(
                &TrustedPublisherServer::on_accept, this, std::placeholders::_1));
    }

    ~TrustedPublisherServer()
    {
        error_code ec;
        acceptor_.close(ec);
    }

    endpoint_type
    local_endpoint() const
    {
        return acceptor_.local_endpoint();
    }

private:
    struct lambda
    {
        int id;
        TrustedPublisherServer& self;
        socket_type sock;
        boost::asio::io_service::work work;

        lambda(int id_, TrustedPublisherServer& self_, socket_type&& sock_)
            : id(id_)
            , self(self_)
            , sock(std::move(sock_))
            , work(sock.get_io_service())
        {
        }

        void
        operator()()
        {
            self.do_peer(id, std::move(sock));
        }
    };

    void
    on_accept(error_code ec)
    {
//“acceptor”之前必须检查ec，否则成员变量可能
//在析构函数完成后访问
        if (ec || !acceptor_.is_open())
            return;

        static int id_ = 0;
        std::thread{lambda{++id_, *this, std::move(sock_)}}.detach();
        acceptor_.async_accept(
            sock_,
            std::bind(
                &TrustedPublisherServer::on_accept, this, std::placeholders::_1));
    }

    void
    do_peer(int id, socket_type&& sock0)
    {
        using namespace boost::beast;
        socket_type sock(std::move(sock0));
        multi_buffer sb;
        error_code ec;
        for (;;)
        {
            req_type req;
            http::read(sock, sb, req, ec);
            if (ec)
                break;
            auto path = req.target().to_string();
            resp_type res;
            res.insert("Server", "TrustedPublisherServer");
            res.version(req.version());

            if (boost::starts_with(path, "/validators"))
            {
                res.result(http::status::ok);
                res.insert("Content-Type", "application/json");
                if (path == "/validators/bad")
                    res.body() = "{ 'bad': \"1']" ;
                else if (path == "/validators/missing")
                    res.body() = "{\"version\": 1}";
                else
                    res.body() = list_;
            }
            else if (boost::starts_with(path, "/redirect"))
            {
                if (boost::ends_with(path, "/301"))
                    res.result(http::status::moved_permanently);
                else if (boost::ends_with(path, "/302"))
                    res.result(http::status::found);
                else if (boost::ends_with(path, "/307"))
                    res.result(http::status::temporary_redirect);
                else if (boost::ends_with(path, "/308"))
                    res.result(http::status::permanent_redirect);

                std::stringstream location;
                if (boost::starts_with(path, "/redirect_to/"))
                {
                    location << path.substr(13);
                }
                else if (! boost::starts_with(path, "/redirect_nolo"))
                {
location << "http://“<<local_endpoint（）<<
                        (boost::starts_with(path, "/redirect_forever/") ?
                            path : "/validators");
                }
                if (! location.str().empty())
                    res.insert("Location", location.str());
            }
            else
            {
//未知请求
                res.result(boost::beast::http::status::not_found);
                res.insert("Content-Type", "text/html");
                res.body() = "The file '" + path + "' was not found";
            }

            try
            {
                res.prepare_payload();
            }
            catch (std::exception const& e)
            {
                res = {};
                res.result(boost::beast::http::status::internal_server_error);
                res.version(req.version());
                res.insert("Server", "TrustedPublisherServer");
                res.insert("Content-Type", "text/html");
                res.body() = std::string{"An internal error occurred"} + e.what();
                res.prepare_payload();
            }
            write(sock, res, ec);
            if (ec)
                break;
        }
    }
};

}  //命名空间测试
}  //命名空间波纹
#endif
