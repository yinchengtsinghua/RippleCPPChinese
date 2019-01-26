
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
  版权所有（c）2012-2016 Ripple Labs Inc.

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

#include <test/jtx.h>
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SField.h>
#include <test/jtx/WSClient.h>

namespace ripple {

class TrustAndBalance_test : public beast::unit_test::suite
{
    static auto
    ledgerEntryState(
        test::jtx::Env & env,
        test::jtx::Account const& acct_a,
        test::jtx::Account const & acct_b,
        std::string const& currency)
    {
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::ripple_state][jss::currency] = currency;
        jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
        jvParams[jss::ripple_state][jss::accounts].append(acct_a.human());
        jvParams[jss::ripple_state][jss::accounts].append(acct_b.human());
        return env.rpc ("json", "ledger_entry", to_string(jvParams))[jss::result];
    }

    void
    testPayNonexistent (FeatureBitset features)
    {
        testcase ("Payment to Nonexistent Account");
        using namespace test::jtx;

        Env env {*this, features};
        env (pay (env.master, "alice", XRP(1)), ter(tecNO_DST_INSUF_XRP));
        env.close();
    }

    void
    testTrustNonexistent ()
    {
        testcase ("Trust Nonexistent Account");
        using namespace test::jtx;

        Env env {*this};
        Account alice {"alice"};

        env (trust (env.master, alice["USD"](100)), ter(tecNO_DST));
    }

    void
    testCreditLimit ()
    {
        testcase ("Credit Limit");
        using namespace test::jtx;

        Env env {*this};
        Account gw {"gateway"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund (XRP(10000), gw, alice, bob);
        env.close();

//信用额度还不存在-验证分类帐条目
//反映了这个
        auto jrr = ledgerEntryState (env, gw, alice, "USD");
        BEAST_EXPECT(jrr[jss::error] == "entryNotFound");

//现在创建信用额度
        env (trust (alice, gw["USD"](800)));

        jrr = ledgerEntryState (env, gw, alice, "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::value] == "800");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::issuer] == alice.human());
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::currency] == "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::value] == "0");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::issuer] == gw.human());
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::currency] == "USD");

//修改信用额度
        env (trust (alice, gw["USD"](700)));

        jrr = ledgerEntryState (env, gw, alice, "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::value] == "700");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::issuer] == alice.human());
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::currency] == "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::value] == "0");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::issuer] == gw.human());
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::currency] == "USD");

//设置负限制-预期失败
        env (trust (alice, gw["USD"](-1)), ter(temBAD_LIMIT));

//设零极限
        env (trust (alice, gw["USD"](0)));

//确保删除行
        jrr = ledgerEntryState (env, gw, alice, "USD");
        BEAST_EXPECT(jrr[jss::error] == "entryNotFound");

//要登记两本车主手册。

//设置另一个信用额度
        env (trust (alice, bob["USD"](600)));

//在另一侧设置限制
        env (trust (bob, alice["USD"](500)));

//检查信任行的分类帐状态
        jrr = ledgerEntryState (env, alice, bob, "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::value] == "500");
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::issuer] == bob.human());
        BEAST_EXPECT(
            jrr[jss::node][sfHighLimit.fieldName][jss::currency] == "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::value] == "600");
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::issuer] == alice.human());
        BEAST_EXPECT(
            jrr[jss::node][sfLowLimit.fieldName][jss::currency] == "USD");
    }

    void
    testDirectRipple (FeatureBitset features)
    {
        testcase ("Direct Payment, Ripple");
        using namespace test::jtx;

        Env env {*this, features};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund (XRP(10000), alice, bob);
        env.close();

        env (trust (alice, bob["USD"](600)));
        env (trust (bob, alice["USD"](700)));

//Alice将Bob部分发送给Alice作为发行人
        env (pay (alice, bob, alice["USD"](24)));
        env.require (balance (bob, alice["USD"](24)));

//Alice将Bob作为发行人与Bob一起发送更多信息
        env (pay (alice, bob, bob["USD"](33)));
        env.require (balance (bob, alice["USD"](57)));

//鲍勃发回的比发送的多
        env (pay (bob, alice, bob["USD"](90)));
        env.require (balance (bob, alice["USD"](-33)));

//艾丽丝发到极限
        env (pay (alice, bob, bob["USD"](733)));
        env.require (balance (bob, alice["USD"](700)));

//鲍勃发号施令
        env (pay (bob, alice, bob["USD"](1300)));
        env.require (balance (bob, alice["USD"](-600)));

//Bob发送超过限制
        env (pay (bob, alice, bob["USD"](1)), ter(tecPATH_DRY));
        env.require (balance (bob, alice["USD"](-600)));
    }

    void
    testWithTransferFee (bool subscribe, bool with_rate, FeatureBitset features)
    {
        testcase(std::string("Direct Payment: ") +
                (with_rate ? "With " : "Without ") + " Xfer Fee, " +
                (subscribe ? "With " : "Without ") + " Subscribe");
        using namespace test::jtx;

        Env env {*this, features};
        auto wsc = test::makeWSClient(env.app().config());
        Account gw {"gateway"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund (XRP(10000), gw, alice, bob);
        env.close();

        env (trust (alice, gw["AUD"](100)));
        env (trust (bob, gw["AUD"](100)));

        env (pay (gw, alice, alice["AUD"](1)));
        env.close();

        env.require (balance (alice, gw["AUD"](1)));

//爱丽丝寄给鲍勃1澳元
        env (pay (alice, bob, gw["AUD"](1)));
        env.close();

        env.require (balance (alice, gw["AUD"](0)));
        env.require (balance (bob, gw["AUD"](1)));
        env.require (balance (gw, bob["AUD"](-1)));

        if(with_rate)
        {
//设置传输速率
            env (rate (gw, 1.1));
            env.close();
//鲍勃给爱丽丝寄去了0.5澳元的最高消费额
            env (pay (bob, alice, gw["AUD"](0.5)), sendmax(gw["AUD"](0.55)));
        }
        else
        {
//鲍勃寄给爱丽丝0.5澳元
            env (pay (bob, alice, gw["AUD"](0.5)));
        }

        env.require (balance (alice, gw["AUD"](0.5)));
        env.require (balance (bob, gw["AUD"](with_rate ? 0.45 : 0.5)));
        env.require (balance (gw, bob["AUD"](with_rate ? -0.45 : -0.5)));

        if (subscribe)
        {
            Json::Value jvs;
            jvs[jss::accounts] = Json::arrayValue;
            jvs[jss::accounts].append(gw.human());
            jvs[jss::streams] = Json::arrayValue;
            jvs[jss::streams].append("transactions");
            jvs[jss::streams].append("ledger");
            auto jv = wsc->invoke("subscribe", jvs);
            BEAST_EXPECT(jv[jss::status] == "success");

            env.close();

            using namespace std::chrono_literals;
            BEAST_EXPECT(wsc->findMsg(5s,
                [](auto const& jv)
                {
                    auto const& t = jv[jss::transaction];
                    return t[jss::TransactionType] == "Payment";
                }));
            BEAST_EXPECT(wsc->findMsg(5s,
                [](auto const& jv)
                {
                    return jv[jss::type] == "ledgerClosed";
                }));

            BEAST_EXPECT(wsc->invoke("unsubscribe",jv)[jss::status] == "success");
        }
    }

    void
    testWithPath (FeatureBitset features)
    {
        testcase ("Payments With Paths and Fees");
        using namespace test::jtx;

        Env env {*this, features};
        Account gw {"gateway"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund (XRP(10000), gw, alice, bob);
        env.close();

//设置传输速率
        env (rate (gw, 1.1));

        env (trust (alice, gw["AUD"](100)));
        env (trust (bob, gw["AUD"](100)));

        env (pay (gw, alice, alice["AUD"](4.4)));
        env.require (balance (alice, gw["AUD"](4.4)));

//Alice以允许
//XFER率
        env (pay (alice, bob, gw["AUD"](1)), sendmax(gw["AUD"](1.1)));
        env.require (balance (alice, gw["AUD"](3.3)));
        env.require (balance (bob, gw["AUD"](1)));

//Alice将Bob问题发送给Bob，最大开销为
        env (pay (alice, bob, bob["AUD"](1)), sendmax(gw["AUD"](1.1)));
        env.require (balance (alice, gw["AUD"](2.2)));
        env.require (balance (bob, gw["AUD"](2)));

//Alice将gw问题发送给Bob，费用最高。
        env (pay (alice, bob, gw["AUD"](1)), sendmax(alice["AUD"](1.1)));
        env.require (balance (alice, gw["AUD"](1.1)));
        env.require (balance (bob, gw["AUD"](3)));

//Alice将Bob问题发送给Bob，在Alice问题上花费最多。
//由于不涉及GW，预计会失败
        env (pay (alice, bob, bob["AUD"](1)), sendmax(alice["AUD"](1.1)),
            ter(tecPATH_DRY));

        env.require (balance (alice, gw["AUD"](1.1)));
        env.require (balance (bob, gw["AUD"](3)));
    }

    void
    testIndirect (FeatureBitset features)
    {
        testcase ("Indirect Payment");
        using namespace test::jtx;

        Env env {*this, features};
        Account gw {"gateway"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund (XRP(10000), gw, alice, bob);
        env.close();

        env (trust (alice, gw["USD"](600)));
        env (trust (bob, gw["USD"](700)));

        env (pay (gw, alice, alice["USD"](70)));
        env (pay (gw, bob, bob["USD"](50)));

        env.require (balance (alice, gw["USD"](70)));
        env.require (balance (bob, gw["USD"](50)));

//Alice向发行人发送的信息超过了必须的：70个发行人中的100个
        env (pay (alice, gw, gw["USD"](100)), ter(tecPATH_PARTIAL));

//爱丽丝送给鲍勃的不只是这些：70分之100
        env (pay (alice, bob, gw["USD"](100)), ter(tecPATH_PARTIAL));

        env.close();

        env.require (balance (alice, gw["USD"](70)));
        env.require (balance (bob, gw["USD"](50)));

//使用帐户路径发送
        env (pay (alice, bob, gw["USD"](5)), test::jtx::path(gw));

        env.require (balance (alice, gw["USD"](65)));
        env.require (balance (bob, gw["USD"](55)));
    }

    void
    testIndirectMultiPath (bool with_rate, FeatureBitset features)
    {
        testcase (std::string("Indirect Payment, Multi Path, ") +
                (with_rate ? "With " : "Without ") + " Xfer Fee, ");
        using namespace test::jtx;

        Env env {*this, features};
        Account gw {"gateway"};
        Account amazon {"amazon"};
        Account alice {"alice"};
        Account bob {"bob"};
        Account carol {"carol"};

        env.fund (XRP(10000), gw, amazon, alice, bob, carol);
        env.close();

        env (trust (amazon, gw["USD"](2000)));
        env (trust (bob, alice["USD"](600)));
        env (trust (bob, gw["USD"](1000)));
        env (trust (carol, alice["USD"](700)));
        env (trust (carol, gw["USD"](1000)));

        if (with_rate)
            env (rate (gw, 1.1));

        env (pay (gw, bob, bob["USD"](100)));
        env (pay (gw, carol, carol["USD"](100)));
        env.close();

//爱丽丝通过多种途径向亚马逊支付费用
        if (with_rate)
            env (pay (alice, amazon, gw["USD"](150)),
                sendmax(alice["USD"](200)),
                test::jtx::path(bob),
                test::jtx::path(carol));
        else
            env (pay (alice, amazon, gw["USD"](150)),
                test::jtx::path(bob),
                test::jtx::path(carol));

        if (with_rate)
        {
//65.0000000000001正确。
//这是精度有限的结果。
            env.require (balance (
                alice,
                STAmount(
                    carol["USD"].issue(),
                    6500000000000001ull,
                    -14,
                    false,
                    true,
                    STAmount::unchecked{})));
            env.require (balance (carol, gw["USD"](35)));
        }
        else
        {
            env.require (balance (alice, carol["USD"](-50)));
            env.require (balance (carol, gw["USD"](50)));
        }
        env.require (balance (alice, bob["USD"](-100)));
        env.require (balance (amazon, gw["USD"](150)));
        env.require (balance (bob, gw["USD"](0)));
    }

    void
    testInvoiceID (FeatureBitset features)
    {
        testcase ("Set Invoice ID on Payment");
        using namespace test::jtx;

        Env env {*this, features};
        Account alice {"alice"};
        auto wsc = test::makeWSClient(env.app().config());

        env.fund (XRP(10000), alice);
        env.close();

        Json::Value jvs;
        jvs[jss::accounts] = Json::arrayValue;
        jvs[jss::accounts].append(env.master.human());
        jvs[jss::streams] = Json::arrayValue;
        jvs[jss::streams].append("transactions");
        BEAST_EXPECT(wsc->invoke("subscribe", jvs)[jss::status] == "success");

        Json::Value jv;
        auto tx = env.jt (
            pay (env.master, alice, XRP(10000)),
            json(sfInvoiceID.fieldName, "DEADBEEF"));
        jv[jss::tx_blob] = strHex (tx.stx->getSerializer().slice());
        auto jrr = wsc->invoke("submit", jv) [jss::result];
        BEAST_EXPECT(jrr[jss::status] == "success");
        BEAST_EXPECT(jrr[jss::tx_json][sfInvoiceID.fieldName] ==
            "0000000000000000"
            "0000000000000000"
            "0000000000000000"
            "00000000DEADBEEF");
        env.close();

        using namespace std::chrono_literals;
        BEAST_EXPECT(wsc->findMsg(2s,
            [](auto const& jv)
            {
                auto const& t = jv[jss::transaction];
                return
                    t[jss::TransactionType] == "Payment" &&
                    t[sfInvoiceID.fieldName] ==
                        "0000000000000000"
                        "0000000000000000"
                        "0000000000000000"
                        "00000000DEADBEEF";
            }));

        BEAST_EXPECT(wsc->invoke("unsubscribe",jv)[jss::status] == "success");
    }

public:
    void run () override
    {
        testTrustNonexistent ();
        testCreditLimit ();

        auto testWithFeatures = [this](FeatureBitset features) {
            testPayNonexistent(features);
            testDirectRipple(features);
            testWithTransferFee(false, false, features);
            testWithTransferFee(false, true, features);
            testWithTransferFee(true, false, features);
            testWithTransferFee(true, true, features);
            testWithPath(features);
            testIndirect(features);
            testIndirectMultiPath(true, features);
            testIndirectMultiPath(false, features);
            testInvoiceID(features);
        };

        using namespace test::jtx;
        auto const sa = supported_amendments();
        testWithFeatures(sa - featureFlow - fix1373 - featureFlowCross);
        testWithFeatures(sa               - fix1373 - featureFlowCross);
        testWithFeatures(sa                          -featureFlowCross);
        testWithFeatures(sa);
    }
};

BEAST_DEFINE_TESTSUITE_PRIO (TrustAndBalance, app, ripple, 1);

}  //涟漪


