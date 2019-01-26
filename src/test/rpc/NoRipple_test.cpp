
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

#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>

namespace ripple {

namespace test {

class NoRipple_test : public beast::unit_test::suite
{
public:
    void
    testSetAndClear()
    {
        testcase("Set and clear noripple");

        using namespace jtx;
        Env env(*this);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");

        env.fund(XRP(10000), gw, alice);

        auto const USD = gw["USD"];

        Json::Value account_gw;
        account_gw[jss::account] = gw.human();
        Json::Value account_alice;
        account_alice[jss::account] = alice.human();

        for (auto SetOrClear : {true,false})
        {
//创建不带Ripple标志设置的信任行
            env( trust(gw, USD(100), alice, SetOrClear ? tfSetNoRipple
                                                       : tfClearNoRipple));
            env.close();

//检查发送方“网关”上的无波纹标志
            Json::Value lines {env.rpc(
                "json", "account_lines", to_string(account_gw))};
            auto const& gline0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(gline0[jss::no_ripple].asBool() == SetOrClear);

//检查目的地“alice”上的“no ripple peer”标志
            lines = env.rpc("json", "account_lines", to_string(account_alice));
            auto const& aline0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(aline0[jss::no_ripple_peer].asBool() == SetOrClear);
        }
    }

    void testNegativeBalance(FeatureBitset features)
    {
        testcase("Set noripple on a line with negative balance");

        using namespace jtx;
        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

//FIX1578更改返回代码。验证预期行为
//不带和带FIX1578。
        for (auto const tweakedFeatures :
            {features - fix1578, features | fix1578})
        {
            Env env(*this, tweakedFeatures);

            env.fund(XRP(10000), gw, alice, bob, carol);

            env.trust(alice["USD"](100), bob);
            env.trust(bob["USD"](100), carol);
            env.close();

//付款后，Alice与Bob的余额为-50美元，并且
//Bob与Carol的余额为-50美元。所以爱丽丝和
//Bob应该能够清除Noripple标志。
            env(pay(alice, carol, carol["USD"](50)), path(bob));
            env.close();

            TER const terNeg {tweakedFeatures[fix1578] ?
                TER {tecNO_PERMISSION} : TER {tesSUCCESS}};

            env(trust(
                alice, bob["USD"](100), bob,   tfSetNoRipple), ter(terNeg));
            env(trust(
                bob, carol["USD"](100), carol, tfSetNoRipple), ter(terNeg));
            env.close();

            Json::Value params;
            params[jss::source_account] = alice.human();
            params[jss::destination_account] = carol.human();
            params[jss::destination_amount] = [] {
                Json::Value dest_amt;
                dest_amt[jss::currency] = "USD";
                dest_amt[jss::value] = "1";
                dest_amt[jss::issuer] = Account("carol").human();
                return dest_amt;
            }();

            auto const resp = env.rpc(
                "json", "ripple_path_find", to_string(params));
            BEAST_EXPECT(resp[jss::result][jss::alternatives].size()==1);

            auto getAccountLines = [&env] (Account const& acct)
            {
                Json::Value jv;
                jv[jss::account] = acct.human();
                auto const resp =
                    env.rpc("json", "account_lines", to_string(jv));
                return resp[jss::result][jss::lines];
            };
            {
                auto const aliceLines = getAccountLines (alice);
                BEAST_EXPECT(aliceLines.size() == 1);
                BEAST_EXPECT(!aliceLines[0u].isMember(jss::no_ripple));

                auto const bobLines = getAccountLines (bob);
                BEAST_EXPECT(bobLines.size() == 2);
                BEAST_EXPECT(!bobLines[0u].isMember(jss::no_ripple));
                BEAST_EXPECT(!bobLines[1u].isMember(jss::no_ripple));
            }

//现在卡罗尔把50美元还给爱丽丝。然后爱丽丝和
//Bob可以设置Noripple标志。
            env(pay(carol, alice, alice["USD"](50)), path(bob));
            env.close();

            env(trust(alice, bob["USD"](100), bob,   tfSetNoRipple));
            env(trust(bob, carol["USD"](100), carol, tfSetNoRipple));
            env.close();
            {
                auto const aliceLines = getAccountLines (alice);
                BEAST_EXPECT(aliceLines.size() == 1);
                BEAST_EXPECT(aliceLines[0u].isMember(jss::no_ripple));

                auto const bobLines = getAccountLines (bob);
                BEAST_EXPECT(bobLines.size() == 2);
                BEAST_EXPECT(bobLines[0u].isMember(jss::no_ripple_peer));
                BEAST_EXPECT(bobLines[1u].isMember(jss::no_ripple));
            }
        }
    }

    void testPairwise(FeatureBitset features)
    {
        testcase("pairwise NoRipple");

        using namespace jtx;
        Env env(*this, features);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

        env.fund(XRP(10000), alice, bob, carol);

        env(trust(bob, alice["USD"](100)));
        env(trust(carol, bob["USD"](100)));

        env(trust(bob, alice["USD"](100), alice, tfSetNoRipple));
        env(trust(bob, carol["USD"](100), carol, tfSetNoRipple));
        env.close();

        Json::Value params;
        params[jss::source_account] = alice.human();
        params[jss::destination_account] = carol.human();
        params[jss::destination_amount] = [] {
            Json::Value dest_amt;
            dest_amt[jss::currency] = "USD";
            dest_amt[jss::value] = "1";
            dest_amt[jss::issuer] = Account("carol").human();
            return dest_amt;
        }();

        Json::Value const resp {
            env.rpc("json", "ripple_path_find", to_string(params))};
        BEAST_EXPECT(resp[jss::result][jss::alternatives].size() == 0);

        env(pay(alice, carol, bob["USD"](50)), ter(tecPATH_DRY));
    }

    void testDefaultRipple(FeatureBitset features)
    {
        testcase("Set default ripple on an account and check new trustlines");

        using namespace jtx;
        Env env(*this, features);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob =   Account("bob");

        env.fund(XRP(10000), gw, noripple(alice, bob));

        env(fset(bob, asfDefaultRipple));

        auto const USD = gw["USD"];

        env(trust(gw, USD(100), alice, 0));
        env(trust(gw, USD(100), bob, 0));

        {
            Json::Value params;
            params[jss::account] = gw.human();
            params[jss::peer] = alice.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_ripple_peer].asBool() == true);
        }
        {
            Json::Value params;
            params[jss::account] = alice.human();
            params[jss::peer] = gw.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_ripple].asBool() == true);
        }
        {
            Json::Value params;
            params[jss::account] = gw.human();
            params[jss::peer] = bob.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_ripple].asBool() == false);
        }
        {
            Json::Value params;
            params[jss::account] = bob.human();
            params[jss::peer] = gw.human();

            auto lines = env.rpc("json", "account_lines", to_string(params));
            auto const& line0 = lines[jss::result][jss::lines][0u];
            BEAST_EXPECT(line0[jss::no_ripple_peer].asBool() == false);
        }
    }

    void run () override
    {
        testSetAndClear();

        auto withFeatsTests = [this](FeatureBitset features) {
            testNegativeBalance(features);
            testPairwise(features);
            testDefaultRipple(features);
        };
        using namespace jtx;
        auto const sa = supported_amendments();
        withFeatsTests(sa - featureFlow - fix1373 - featureFlowCross);
        withFeatsTests(sa               - fix1373 - featureFlowCross);
        withFeatsTests(sa                         - featureFlowCross);
        withFeatsTests(sa);
    }
};

BEAST_DEFINE_TESTSUITE(NoRipple,app,ripple);

} //RPC
} //涟漪

