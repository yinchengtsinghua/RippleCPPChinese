
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

#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/JSONRPCClient.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class RPCOverload_test : public beast::unit_test::suite
{
public:
    void testOverload(bool useWS)
    {
        testcase << "Overload " << (useWS ? "WS" : "HTTP") << " RPC client";
        using namespace jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return no_admin(std::move(cfg));
            })};

        Account const alice {"alice"};
        Account const bob {"bob"};
        env.fund (XRP (10000), alice, bob);

        std::unique_ptr<AbstractClient> client = useWS ?
              makeWSClient(env.app().config())
            : makeJSONRPCClient(env.app().config());

        Json::Value tx = Json::objectValue;
        tx[jss::tx_json] = pay(alice, bob, XRP(1));
        tx[jss::secret] = toBase58(generateSeed("alice"));

//要求服务器重复签署此事务
//签名是一个资源繁重的事务，因此我们需要服务器
//警告并最终引导我们。
        bool warned = false, booted = false;
        for(int i = 0 ; i < 500 && !booted; ++i)
        {
            auto jv = client->invoke("sign", tx);
            if(!useWS)
                jv = jv[jss::result];
//启动时，我们只得到一个空的JSON响应
            if(jv.isNull())
                booted = true;
            else if (!(jv.isMember(jss::status) &&
                       (jv[jss::status] == "success")))
            {
//不要在B/C以上使用Beast，它将被称为非确定性次数
//运行的测试数量应该是确定的
                fail("", __FILE__, __LINE__);
            }

            if(jv.isMember(jss::warning))
                warned = jv[jss::warning] == jss::load;
        }
        BEAST_EXPECT(warned && booted);
    }



    void run() override
    {
        /*toverload（错误/*http*/）；
        测试重载（真/*ws*/);

    }
};

BEAST_DEFINE_TESTSUITE(RPCOverload,app,ripple);

} //测试
} //涟漪
