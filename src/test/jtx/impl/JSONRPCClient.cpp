
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
#include <test/jtx/JSONRPCClient.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/server/Port.h>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>
#include <boost/asio.hpp>
#include <string>

namespace ripple {
namespace test {

class JSONRPCClient : public AbstractClient
{
    static
    boost::asio::ip::tcp::endpoint
    getEndpoint(BasicConfig const& cfg)
    {
        auto& log = std::cerr;
        ParsedPort common;
        parse_Port (common, cfg["server"], log);
        for (auto const& name : cfg.section("server").values())
        {
            if (! cfg.exists(name))
                continue;
            ParsedPort pp;
            parse_Port(pp, cfg[name], log);
            if(pp.protocol.count("http") == 0)
                continue;
            using namespace boost::asio::ip;
            if(pp.ip && pp.ip->is_unspecified())
               *pp.ip = pp.ip->is_v6() ? address{address_v6::loopback()} : address{address_v4::loopback()};
            return { *pp.ip, *pp.port };
        }
        Throw<std::runtime_error>("Missing HTTP port");
return {}; //静默编译器控制路径返回值警告
    }

    template <class ConstBufferSequence>
    static
    std::string
    buffer_string (ConstBufferSequence const& b)
    {
        using namespace boost::asio;
        std::string s;
        s.resize(buffer_size(b));
        buffer_copy(buffer(&s[0], s.size()), b);
        return s;
    }

    boost::asio::ip::tcp::endpoint ep_;
    boost::asio::io_service ios_;
    boost::asio::ip::tcp::socket stream_;
    boost::beast::multi_buffer bin_;
    boost::beast::multi_buffer bout_;
    unsigned rpc_version_;

public:
    explicit
    JSONRPCClient(Config const& cfg, unsigned rpc_version)
        : ep_(getEndpoint(cfg))
        , stream_(ios_)
        , rpc_version_(rpc_version)
    {
        stream_.connect(ep_);
    }

    ~JSONRPCClient() override
    {
//流关闭（boost:：asio:：ip:：tcp:：socket:：shutdown）；
//流关闭（）；
    }

    /*
        返回值是一个对象类型，最多有三个键：
            地位
            错误
            结果
    **/

    Json::Value
    invoke(std::string const& cmd,
        Json::Value const& params) override
    {
        using namespace boost::beast::http;
        using namespace boost::asio;
        using namespace std::string_literals;

        request<string_body> req;
        req.method(boost::beast::http::verb::post);
        req.target("/");
        req.version(11);
        req.insert("Content-Type", "application/json; charset=UTF-8");
        req.insert("Host", ep_);
        {
            Json::Value jr;
            jr[jss::method] = cmd;
            if (rpc_version_ == 2)
            {
                jr[jss::jsonrpc] = "2.0";
                jr[jss::ripplerpc] = "2.0";
                jr[jss::id] = 5;
            }
            if(params)
            {
                Json::Value& ja = jr[jss::params] = Json::arrayValue;
                ja.append(params);
            }
            req.body() = to_string(jr);
        }
        req.prepare_payload();
        write(stream_, req);

        response<dynamic_body> res;
        read(stream_, bin_, res);

        Json::Reader jr;
        Json::Value jv;
        jr.parse(buffer_string(res.body().data()), jv);
        if(jv["result"].isMember("error"))
            jv["error"] = jv["result"]["error"];
        if(jv["result"].isMember("status"))
            jv["status"] = jv["result"]["status"];
        return jv;
    }

    unsigned version() const override
    {
        return rpc_version_;
    }
};

std::unique_ptr<AbstractClient>
makeJSONRPCClient(Config const& cfg, unsigned rpc_version)
{
    return std::make_unique<JSONRPCClient>(cfg, rpc_version);
}

} //测试
} //涟漪
