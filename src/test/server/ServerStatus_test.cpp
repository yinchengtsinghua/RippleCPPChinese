
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

#include <ripple/rpc/ServerHandler.h>
#include <ripple/json/json_reader.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <test/jtx.h>
#include <test/jtx/envconfig.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/JSONRPCClient.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/basics/base64.h>
#include <boost/beast/http.hpp>
#include <beast/test/yield_to.hpp>
#include <boost/beast/websocket/detail/mask.hpp>
#include <boost/beast/core/multi_buffer.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <regex>

namespace ripple {
namespace test {

class ServerStatus_test :
    public beast::unit_test::suite, public beast::test::enable_yield_to
{
    class myFields : public boost::beast::http::fields {};

    auto makeConfig(
        std::string const& proto,
        bool admin = true,
        bool credentials = false)
    {
        auto const section_name =
            boost::starts_with(proto, "h") ?  "port_rpc" : "port_ws";
        auto p = jtx::envconfig();

        p->overwrite(section_name, "protocol", proto);
        if(! admin)
            p->overwrite(section_name, "admin", "");

        if(credentials)
        {
            (*p)[section_name].set("admin_password", "p");
            (*p)[section_name].set("admin_user", "u");
        }

        p->overwrite(
            boost::starts_with(proto, "h") ?  "port_ws" : "port_rpc",
            "protocol",
            boost::starts_with(proto, "h") ?  "ws" : "http");

        if(proto == "https")
        {
//这个端口允许env创建其内部客户机，
//这需要一个HTTP端点进行通信。在连接中
//失败测试，不应使用此终结点
            (*p)["server"].append("port_alt");
            (*p)["port_alt"].set("ip", getEnvLocalhostAddr());
            (*p)["port_alt"].set("port", "8099");
            (*p)["port_alt"].set("protocol", "http");
            (*p)["port_alt"].set("admin", getEnvLocalhostAddr());
        }

        return p;
    }

    auto makeWSUpgrade(
        std::string const& host,
        uint16_t port)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        request<string_body> req;

        req.target("/");
        req.version(11);
        req.insert("Host", host + ":" + std::to_string(port));
        req.insert("User-Agent", "test");
        req.method(boost::beast::http::verb::get);
        req.insert("Upgrade", "websocket");
        boost::beast::websocket::detail::sec_ws_key_type key;
#if BOOST_VERSION >= 106800
        boost::beast::websocket::detail::make_sec_ws_key(key);
#else
        boost::beast::websocket::detail::maskgen maskgen;
        boost::beast::websocket::detail::make_sec_ws_key(key, maskgen);
#endif
        req.insert("Sec-WebSocket-Key", key);
        req.insert("Sec-WebSocket-Version", "13");
        req.insert(boost::beast::http::field::connection, "upgrade");
        return req;
    }

    auto makeHTTPRequest(
        std::string const& host,
        uint16_t port,
        std::string const& body,
        myFields const& fields)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        request<string_body> req;

        req.target("/");
        req.version(11);
        for(auto const& f : fields)
            req.insert(f.name(), f.value());
        req.insert("Host", host + ":" + std::to_string(port));
        req.insert("User-Agent", "test");
        if(body.empty())
        {
            req.method(boost::beast::http::verb::get);
        }
        else
        {
            req.method(boost::beast::http::verb::post);
            req.insert("Content-Type", "application/json; charset=UTF-8");
            req.body() = body;
        }
        req.prepare_payload();

        return req;
    }

    void
    doRequest(
        boost::asio::yield_context& yield,
        boost::beast::http::request<boost::beast::http::string_body>&& req,
        std::string const& host,
        uint16_t port,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec)
    {
        using namespace boost::asio;
        using namespace boost::beast::http;
        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};
        boost::beast::multi_buffer sb;

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{host, std::to_string(port)}, yield[ec]);
        if(ec)
            return;

        if(secure)
        {
            ssl::context ctx{ssl::context::sslv23};
            ctx.set_verify_mode(ssl::verify_none);
            ssl::stream<ip::tcp::socket> ss{ios, ctx};
            async_connect(ss.next_layer(), it, yield[ec]);
            if(ec)
                return;
            ss.async_handshake(ssl::stream_base::client, yield[ec]);
            if(ec)
                return;
            boost::beast::http::async_write(ss, req, yield[ec]);
            if(ec)
                return;
            async_read(ss, sb, resp, yield[ec]);
            if(ec)
                return;
        }
        else
        {
            ip::tcp::socket sock{ios};
            async_connect(sock, it, yield[ec]);
            if(ec)
                return;
            boost::beast::http::async_write(sock, req, yield[ec]);
            if(ec)
                return;
            async_read(sock, sb, resp, yield[ec]);
            if(ec)
                return;
        }

        return;
    }

    void
    doWSRequest(
        test::jtx::Env& env,
        boost::asio::yield_context& yield,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec)
    {
        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto ip = env.app().config()["port_ws"].
            get<std::string>("ip");
        doRequest(
            yield,
            makeWSUpgrade(*ip, *port),
            *ip,
            *port,
            secure,
            resp,
            ec);
        return;
    }

    void
    doHTTPRequest(
        test::jtx::Env& env,
        boost::asio::yield_context& yield,
        bool secure,
        boost::beast::http::response<boost::beast::http::string_body>& resp,
        boost::system::error_code& ec,
        std::string const& body = "",
        myFields const& fields = {})
    {
        auto const port = env.app().config()["port_rpc"].
            get<std::uint16_t>("port");
        auto const ip = env.app().config()["port_rpc"].
            get<std::string>("ip");
        doRequest(
            yield,
            makeHTTPRequest(*ip, *port, body, fields),
            *ip,
            *port,
            secure,
            resp,
            ec);
        return;
    }

    auto makeAdminRequest(
        jtx::Env & env,
        std::string const& proto,
        std::string const& user,
        std::string const& password,
        bool subobject = false)
    {
        Json::Value jrr;

        Json::Value jp = Json::objectValue;
        if(! user.empty())
        {
            jp["admin_user"] = user;
            if(subobject)
            {
//密码错误的特殊情况..作为对象传递
                Json::Value jpi = Json::objectValue;
                jpi["admin_password"] = password;
                jp["admin_password"] = jpi;
            }
            else
            {
                jp["admin_password"] = password;
            }
        }

        if(boost::starts_with(proto, "h"))
        {
            auto jrc = makeJSONRPCClient(env.app().config());
            jrr = jrc->invoke("ledger_accept", jp);
        }
        else
        {
            auto wsc = makeWSClient(env.app().config(), proto == "ws2");
            jrr = wsc->invoke("ledger_accept", jp);
        }

        return jrr;
    }

//--------------
//测试用例
//--------------

    void
    testAdminRequest(std::string const& proto, bool admin, bool credentials)
    {
        testcase << "Admin request over " << proto <<
            ", config " << (admin ? "enabled" : "disabled") <<
            ", credentials " << (credentials ? "" : "not ") << "set";
        using namespace jtx;
        Env env {*this, makeConfig(proto, admin, credentials)};

        Json::Value jrr;
        auto const proto_ws = boost::starts_with(proto, "w");

//我们做的一套检查是不同的，取决于
//关于如何设置管理配置选项

        if(admin && credentials)
        {
            auto const user = env.app().config()
                [proto_ws ? "port_ws" : "port_rpc"].
                get<std::string>("admin_user");

            auto const password = env.app().config()
                [proto_ws ? "port_ws" : "port_rpc"].
                get<std::string>("admin_password");

//1-错误通过失败
            jrr = makeAdminRequest(env, proto, *user, *password + "_")
                [jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

//2-在对象中使用密码失败
            jrr = makeAdminRequest(env, proto, *user, *password, true)[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

//3-错误用户失败
            jrr = makeAdminRequest(env, proto, *user + "_", *password)[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

//4-没有凭据失败
            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");

//5-通过适当的证书获得成功
            jrr = makeAdminRequest(env, proto, *user, *password)[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");
        }
        else if(admin)
        {
//1-通过适当的凭证获得成功
            jrr = makeAdminRequest(env, proto, "u", "p")[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");

//2-没有适当的凭证成功
            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["status"] == "success");
        }
        else
        {
//1-失败-已禁用管理员
            jrr = makeAdminRequest(env, proto, "", "")[jss::result];
            BEAST_EXPECT(jrr["error"] == proto_ws ? "forbidden" : "noPermission");
            BEAST_EXPECT(jrr["error_message"] ==
                proto_ws ?
                "Bad credentials." :
                "You don't have permission for this command.");
        }
    }

    void
    testWSClientToHttpServer(boost::asio::yield_context& yield)
    {
        testcase("WS client to http server fails");
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_ws").set("protocol", "http,https");
                return cfg;
            })};

//非安全请求
        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doWSRequest(env, yield, false, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::unauthorized);
        }

//安全请求
        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doWSRequest(env, yield, true, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::unauthorized);
        }
    }

    void
    testStatusRequest(boost::asio::yield_context& yield)
    {
        testcase("Status request");
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_rpc").set("protocol", "ws2,wss2");
                cfg->section("port_ws").set("protocol", "http");
                return cfg;
            })};

//非安全请求
        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, false, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        }

//安全请求
        {
            boost::system::error_code ec;
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, true, resp, ec);
            if(! BEAST_EXPECTS(! ec, ec.message()))
                return;
            BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        }
    }

    void
    testTruncatedWSUpgrade(boost::asio::yield_context& yield)
    {
        testcase("Partial WS upgrade request");
        using namespace jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_ws").set("protocol", "ws2");
                return cfg;
            })};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip");

        boost::system::error_code ec;
        response<string_body> resp;
        auto req = makeWSUpgrade(*ip, *port);

//截断请求消息，使其接近版本头的值
        auto req_string = boost::lexical_cast<std::string>(req);
        req_string.erase(req_string.find_last_of("13"), std::string::npos);

        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};
        boost::beast::multi_buffer sb;

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{*ip, std::to_string(*port)}, yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;

        ip::tcp::socket sock{ios};
        async_connect(sock, it, yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        async_write(sock, boost::asio::buffer(req_string), yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
//由于我们发送了一个不完整的请求，服务器将
//一直读直到它放弃（超时）
        async_read(sock, sb, resp, yield[ec]);
        BEAST_EXPECT(ec);
    }

    void
    testCantConnect(
        std::string const& client_protocol,
        std::string const& server_protocol,
        boost::asio::yield_context& yield)
    {
//此测试的本质是配置客户机和服务器
//与SSL（安全客户端和不安全服务器）不同步
//反之亦然）
        testcase << "Connect fails: " << client_protocol << " client to " <<
            server_protocol << " server";
        using namespace jtx;
        Env env {*this, makeConfig(server_protocol)};

        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        if(boost::starts_with(client_protocol, "h"))
        {
                doHTTPRequest(
                    env,
                    yield,
                    client_protocol == "https",
                    resp,
                    ec);
                BEAST_EXPECT(ec);
        }
        else
        {
                doWSRequest(
                    env,
                    yield,
                    client_protocol == "wss" || client_protocol == "wss2",
                    resp,
                    ec);
                BEAST_EXPECT(ec);
        }
    }

    void
    testAuth(bool secure, boost::asio::yield_context& yield)
    {
        testcase << "Server with authorization, " <<
            (secure ? "secure" : "non-secure");

        using namespace test::jtx;
        Env env {*this, envconfig([secure](std::unique_ptr<Config> cfg) {
            (*cfg)["port_rpc"].set("user","me");
            (*cfg)["port_rpc"].set("password","secret");
            (*cfg)["port_rpc"].set("protocol", secure ? "https" : "http");
            if (secure)
                (*cfg)["port_ws"].set("protocol","http,ws");
            return cfg;
        })};

        Json::Value jr;
        jr[jss::method] = "server_info";
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr));
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        myFields auth;
        auth.insert("Authorization", "");
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auth.set("Authorization", "Basic NOT-VALID");
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auth.set("Authorization", "Basic " + base64_encode("me:badpass"));
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

        auto const user = env.app().config().section("port_rpc").
            get<std::string>("user").value();
        auto const pass = env.app().config().section("port_rpc").
            get<std::string>("password").value();

//尝试使用正确的用户/密码，但不编码
        auth.set("Authorization", "Basic " + user + ":" + pass);
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);

//最后，如果我们使用正确的用户/密码编码，我们应该得到一个200
        auth.set("Authorization", "Basic " +
            base64_encode(user + ":" + pass));
        doHTTPRequest(env, yield, secure, resp, ec, to_string(jr), auth);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(! resp.body().empty());
    }

    void
    testLimit(boost::asio::yield_context& yield, int limit)
    {
        testcase << "Server with connection limit of " << limit;

        using namespace test::jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, envconfig([&](std::unique_ptr<Config> cfg) {
            (*cfg)["port_rpc"].set("limit", to_string(limit));
            return cfg;
        })};


        auto const port = env.app().config()["port_rpc"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_rpc"].
            get<std::string>("ip").value();

        boost::system::error_code ec;
        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};

        Json::Value jr;
        jr[jss::method] = "server_info";

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{ip, to_string(port)}, yield[ec]);
        BEAST_EXPECT(! ec);

        std::vector<std::pair<ip::tcp::socket, boost::beast::multi_buffer>> clients;
int connectionCount {1}; //从1开始，因为env已经有了一个
//对于jsonrpcclient

//对于非零值限制，尽管发生了故障，但要超过限制一次。
//在一定程度上，这确实会导致最后两个客户机失败。
//对于零限制，选择任意非零数量的客户机-所有客户机都应
//连接好。

        int testTo = (limit == 0) ? 50 : limit + 1;
        while (connectionCount < testTo)
        {
            clients.emplace_back(
                std::make_pair(ip::tcp::socket {ios}, boost::beast::multi_buffer{}));
            async_connect(clients.back().first, it, yield[ec]);
            BEAST_EXPECT(! ec);
            auto req = makeHTTPRequest(ip, port, to_string(jr), {});
            async_write(
                clients.back().first,
                req,
                yield[ec]);
            BEAST_EXPECT(! ec);
            ++connectionCount;
        }

        int readCount = 0;
        for (auto& c : clients)
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            async_read(c.first, c.second, resp, yield[ec]);
            ++readCount;
//预期在或连接的客户端的读取失败
//超出限制。如果限制为0，则所有读取都应成功
            BEAST_EXPECT((limit == 0 || readCount < limit-1) ? (! ec) : bool(ec));
        }
    }

    void
    testWSHandoff(boost::asio::yield_context& yield)
    {
        testcase ("Connection with WS handoff");

        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg) {
            (*cfg)["port_ws"].set("protocol","wss");
            return cfg;
        })};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doRequest(
            yield, makeWSUpgrade(ip, port), ip, port, true, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::switching_protocols);
        BEAST_EXPECT(resp.find("Upgrade") != resp.end() &&
               resp["Upgrade"] == "websocket");
        BEAST_EXPECT(resp.find("Connection") != resp.end() &&
               resp["Connection"] == "upgrade");
    }

    void
    testNoRPC(boost::asio::yield_context& yield)
    {
        testcase ("Connection to port with no RPC enabled");

        using namespace test::jtx;
        Env env {*this};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
//此处要求提供正文内容以避免
//检测为状态请求
        doRequest(yield,
            makeHTTPRequest(ip, port, "foo", {}), ip, port, false, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::forbidden);
        BEAST_EXPECT(resp.body() == "Forbidden\r\n");
    }

    void
    testWSRequests(boost::asio::yield_context& yield)
    {
        testcase ("WS client sends assorted input");

        using namespace test::jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this};

        auto const port = env.app().config()["port_ws"].
            get<std::uint16_t>("port").value();
        auto const ip = env.app().config()["port_ws"].
            get<std::string>("ip").value();
        boost::system::error_code ec;

        io_service& ios = get_io_service();
        ip::tcp::resolver r{ios};

        auto it =
            r.async_resolve(
                ip::tcp::resolver::query{ip, to_string(port)}, yield[ec]);
        if(! BEAST_EXPECT(! ec))
            return;

        ip::tcp::socket sock{ios};
        async_connect(sock, it, yield[ec]);
        if(! BEAST_EXPECT(! ec))
            return;

        boost::beast::websocket::stream<boost::asio::ip::tcp::socket&> ws{sock};
        ws.handshake(ip + ":" + to_string(port), "/");

//助手lambda，在下面使用
        auto sendAndParse = [&](std::string const& req) -> Json::Value
        {
            ws.async_write_some(true, buffer(req), yield[ec]);
            if(! BEAST_EXPECT(! ec))
                return Json::objectValue;

            boost::beast::multi_buffer sb;
            ws.async_read(sb, yield[ec]);
            if(! BEAST_EXPECT(! ec))
                return Json::objectValue;

            Json::Value resp;
            Json::Reader jr;
            if(! BEAST_EXPECT(jr.parse(
                boost::lexical_cast<std::string>(
                    boost::beast::buffers(sb.data())), resp)))
                return Json::objectValue;
            sb.consume(sb.size());
            return resp;
        };

{ //发送无效的JSON
            auto resp = sendAndParse("NOT JSON");
            BEAST_EXPECT(resp.isMember(jss::error) &&
                resp[jss::error] == "jsonInvalid");
            BEAST_EXPECT(! resp.isMember(jss::status));
        }

{ //发送错误的JSON（方法和命令字段不同）
            Json::Value jv;
            jv[jss::command] = "foo";
            jv[jss::method] = "bar";
            auto resp = sendAndParse(to_string(jv));
            BEAST_EXPECT(resp.isMember(jss::error) &&
                resp[jss::error] == "missingCommand");
            BEAST_EXPECT(resp.isMember(jss::status) &&
                resp[jss::status] == "error");
        }

{ //发送ping（不是错误）
            Json::Value jv;
            jv[jss::command] = "ping";
            auto resp = sendAndParse(to_string(jv));
            BEAST_EXPECT(resp.isMember(jss::status) &&
                resp[jss::status] == "success");
            BEAST_EXPECT(resp.isMember(jss::result) &&
                resp[jss::result].isMember(jss::role) &&
                resp[jss::result][jss::role] == "admin");
        }
    }

    void
    testAmendmentBlock(boost::asio::yield_context& yield)
    {
        testcase("Status request over WS and RPC with/without Amendment Block");
        using namespace jtx;
        using namespace boost::asio;
        using namespace boost::beast::http;
        Env env {*this, validator( envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("port_rpc").set("protocol", "http");
                return cfg;
            }), "")};

        env.close();

//推进分类帐以便服务器状态
//看到一个已发布的分类账——如果没有这个，我们会得到一个状态
//有关未发布分类帐的失败消息
        env.app().getLedgerMaster().tryAdvance();

//发出RPC服务器信息请求并查找
//修正案\封锁状态
        auto si = env.rpc("server_info") [jss::result];
        BEAST_EXPECT(! si[jss::info].isMember(jss::amendment_blocked));
        BEAST_EXPECT(
            env.app().getOPs().getConsensusInfo()["validating"] == true);

        auto const port_ws = env.app().config()["port_ws"].
            get<std::uint16_t>("port");
        auto const ip_ws = env.app().config()["port_ws"].
            get<std::string>("ip");


        boost::system::error_code ec;
        response<string_body> resp;

        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(
            resp.body().find("connectivity is working.") != std::string::npos);

//将网络标记为修正阻塞，但直到
//ELB已启用（下一步）
        env.app().getOPs().setAmendmentBlocked ();
        env.app().getOPs().beginConsensus(env.closed()->info().hash);

//共识现在看到验证被禁用
        BEAST_EXPECT(
            env.app().getOPs().getConsensusInfo()["validating"] == false);

//rpc请求服务器_信息，现在应该返回ab
        si = env.rpc("server_info") [jss::result];
        BEAST_EXPECT(
            si[jss::info].isMember(jss::amendment_blocked) &&
            si[jss::info][jss::amendment_blocked] == true);

//但状态并不表示，因为它仍然依赖ELB
//启用
        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::ok);
        BEAST_EXPECT(
            resp.body().find("connectivity is working.") != std::string::npos);

        env.app().config().ELB_SUPPORT = true;

        doRequest(
            yield,
            makeHTTPRequest(*ip_ws, *port_ws, "", {}),
            *ip_ws,
            *port_ws,
            false,
            resp,
            ec);

        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(resp.result() == boost::beast::http::status::internal_server_error);
        BEAST_EXPECT(
            resp.body().find("cannot accept clients:") != std::string::npos);
        BEAST_EXPECT(
            resp.body().find("Server version too old") != std::string::npos);
    }

    void
    testRPCRequests(boost::asio::yield_context& yield)
    {
        testcase ("RPC client sends assorted input");

        using namespace test::jtx;
        Env env {*this};

        boost::system::error_code ec;
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            doHTTPRequest(env, yield, false, resp, ec, "{}");
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv["invalid"] = 1;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Null method\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv(Json::arrayValue);
            jv.append("invalid");
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv(Json::arrayValue);
            Json::Value j;
            j["invalid"] = 1;
            jv.append(j);
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Unable to parse request: \r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv[jss::method] = "batch";
            jv[jss::params] = 2;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Malformed batch request\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            Json::Value jv;
            jv[jss::method] = "batch";
            jv[jss::params] = Json::objectValue;
            jv[jss::params]["invalid"] = 3;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Malformed batch request\r\n");
        }

        Json::Value jv;
        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = Json::nullValue;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "Null method\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = 1;
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "method is not string\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = "";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "method is empty\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::method] = "some_method";
            jv[jss::params] = "params";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "params unparseable\r\n");
        }

        {
            boost::beast::http::response<boost::beast::http::string_body> resp;
            jv[jss::params] = Json::arrayValue;
            jv[jss::params][0u] = "not an object";
            doHTTPRequest(env, yield, false, resp, ec, to_string(jv));
            BEAST_EXPECT(resp.result() == boost::beast::http::status::bad_request);
            BEAST_EXPECT(resp.body() == "params unparseable\r\n");
        }
    }

    void
    testStatusNotOkay(boost::asio::yield_context& yield)
    {
        testcase ("Server status not okay");

        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg) {
            cfg->ELB_SUPPORT = true;
            return cfg;
        })};

//提高费用，使服务器被认为超载
        env.app().getFeeTrack().raiseLocalFee();

        boost::beast::http::response<boost::beast::http::string_body> resp;
        boost::system::error_code ec;
        doHTTPRequest(env, yield, false, resp, ec);
        BEAST_EXPECT(resp.result() == boost::beast::http::status::internal_server_error);
        std::regex body {"Server cannot accept clients"};
        BEAST_EXPECT(std::regex_search(resp.body(), body));
    }

public:
    void
    run() override
    {
        for (auto it : {"http", "ws", "ws2"})
        {
            testAdminRequest (it, true, true);
            testAdminRequest (it, true, false);
            testAdminRequest (it, false, false);
        }

        yield_to([&](boost::asio::yield_context& yield)
        {
            testWSClientToHttpServer (yield);
            testStatusRequest (yield);
            testTruncatedWSUpgrade (yield);

//这些是安全/不安全的协议对，即
//每个项目，第二个值是安全或不安全的等效值
            testCantConnect ("ws", "wss", yield);
            testCantConnect ("ws2", "wss2", yield);
            testCantConnect ("http", "https", yield);
            testCantConnect ("wss", "ws", yield);
            testCantConnect ("wss2", "ws2", yield);
            testCantConnect ("https", "http", yield);

            testAmendmentBlock(yield);
            testAuth (false, yield);
            testAuth (true, yield);
            testLimit (yield, 5);
            testLimit (yield, 0);
            testWSHandoff (yield);
            testNoRPC (yield);
            testWSRequests (yield);
            testRPCRequests (yield);
            testStatusNotOkay (yield);
        });

    }
};

BEAST_DEFINE_TESTSUITE(ServerStatus, server, ripple);

} //测试
} //涟漪

