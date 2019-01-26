
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

#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx/WSClient.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class GatewayBalances_test : public beast::unit_test::suite
{
public:

    void
    testGWB(FeatureBitset features)
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this, features);

//网关帐户和资产
        Account const alice {"alice"};
        env.fund(XRP(10000), "alice");
        auto USD = alice["USD"];
        auto CNY = alice["CNY"];
        auto JPY = alice["JPY"];

//创建一个热钱包
        Account const hw {"hw"};
        env.fund(XRP(10000), "hw");
        env(trust(hw, USD(10000)));
        env(trust(hw, JPY(10000)));
        env(pay(alice, hw, USD(5000)));
        env(pay(alice, hw, JPY(5000)));

//创建一些客户端
        Account const bob {"bob"};
        env.fund(XRP(10000), "bob");
        env(trust(bob, USD(100)));
        env(trust(bob, CNY(100)));
        env(pay(alice, bob, USD(50)));

        Account const charley {"charley"};
        env.fund(XRP(10000), "charley");
        env(trust(charley, CNY(500)));
        env(trust(charley, JPY(500)));
        env(pay(alice, charley, CNY(250)));
        env(pay(alice, charley, JPY(250)));

        Account const dave {"dave"};
        env.fund(XRP(10000), "dave");
        env(trust(dave, CNY(100)));
        env(pay(alice, dave, CNY(30)));

//给网关一个资源
        env(trust(alice, charley["USD"](50)));
        env(pay(charley, alice, USD(10)));

//冻结戴夫
        env(trust(alice, dave["CNY"](0), dave, tfSetFreeze));

        env.close();

        auto wsc = makeWSClient(env.app().config());

        Json::Value qry;
        qry[jss::account] = alice.human();
        qry[jss::hotwallet] = hw.human();

        auto jv = wsc->invoke("gateway_balances", qry);
        expect(jv[jss::status] == "success");
        if (wsc->version() == 2)
        {
            expect(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
            expect(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
            expect(jv.isMember(jss::id) && jv[jss::id] == 5);
        }

        auto const& result = jv[jss::result];
        expect(result[jss::account] == alice.human());
        expect(result[jss::status] == "success");

        {
            auto const& balances = result[jss::balances];
            expect (balances.isObject(), "balances is not an object");
            expect (balances.size() == 1, "balances size is not 1");

            auto const& hwBalance = balances[hw.human()];
            expect (hwBalance.isArray(), "hwBalance is not an array");
            expect (hwBalance.size() == 2);
            auto c1 = hwBalance[0u][jss::currency];
            auto c2 = hwBalance[1u][jss::currency];
            expect (c1 == "USD" || c2 == "USD");
            expect (c1 == "JPY" || c2 == "JPY");
            expect (hwBalance[0u][jss::value] == "5000" &&
                hwBalance[1u][jss::value] == "5000");
        }

        {
            auto const& fBalances = result[jss::frozen_balances];
            expect (fBalances.isObject());
            expect (fBalances.size() == 1);

            auto const& fBal = fBalances[dave.human()];
            expect (fBal.isArray());
            expect (fBal.size() == 1);
            expect (fBal[0u].isObject());
            expect (fBal[0u][jss::currency] == "CNY");
            expect (fBal[0u][jss::value] == "30");
        }

        {
            auto const& assets = result[jss::assets];
            expect (assets.isObject(), "assets it not an object");
            expect (assets.size() == 1, "assets size is not 1");

            auto const& cAssets = assets[charley.human()];
            expect (cAssets.isArray());
            expect (cAssets.size() == 1);
            expect (cAssets[0u][jss::currency] == "USD");
            expect (cAssets[0u][jss::value] == "10");
        }

        {
            auto const& obligations = result[jss::obligations];
            expect (obligations.isObject(), "obligations is not an object");
            expect (obligations.size() == 3);
            expect (obligations["CNY"] == "250");
            expect (obligations["JPY"] == "250");
            expect (obligations["USD"] == "50");
        }

    }

    void
    run() override
    {
        using namespace jtx;
        auto const sa = supported_amendments();
        testGWB(sa - featureFlow - fix1373 - featureFlowCross);
        testGWB(sa                         - featureFlowCross);
        testGWB(sa);
    }
};

BEAST_DEFINE_TESTSUITE(GatewayBalances,app,ripple);

} //测试
} //涟漪
