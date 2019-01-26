
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
  版权所有（c）2012-2015 Ripple Labs Inc.

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

#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <test/jtx/PathSet.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

class PaymentSandbox_test : public beast::unit_test::suite
{
    /*
      创建路径，以便一条路径为另一条路径提供资金。

      两个帐户：发送者和接收者。
      两个网关：GW1和GW2。
      发送方和接收方都有到网关的信任线。
      发送方有2 GW1/美元和4 GW2/美元。
      发送方已向GW1提供GW2，GW2提供GW1 1-for-1。
      路径是：
      1）GW1->[OB GW1/USD->GW2/USD]->GW2
      2）GW2->[OB GW2/USD->GW1/USD]->GW1

      发送方支付接收方4美元。
      路径1：
      1）发送方以2 GW1/美元兑换2 GW2/美元
      2）旧代码：2 GW1/USD可供发送方使用。
         新代码：在
         交易结束。
      3）接收器获得2 GW2/美元
      路径2：
      1）旧代码：发送方以2 GW2/USD兑换2 GW1/USD
      2）旧代码：接收器获取2 GW1
      2）新代码：由于发送方没有任何
         GW1将一直花费到交易结束。
    **/

    void testSelfFunding (FeatureBitset features)
    {
        testcase ("selfFunding");

        using namespace jtx;
        Env env (*this, features);
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const snd ("snd");
        Account const rcv ("rcv");

        env.fund (XRP (10000), snd, rcv, gw1, gw2);

        auto const USD_gw1 = gw1["USD"];
        auto const USD_gw2 = gw2["USD"];

        env.trust (USD_gw1 (10), snd);
        env.trust (USD_gw2 (10), snd);
        env.trust (USD_gw1 (100), rcv);
        env.trust (USD_gw2 (100), rcv);

        env (pay (gw1, snd, USD_gw1 (2)));
        env (pay (gw2, snd, USD_gw2 (4)));

        env (offer (snd, USD_gw1 (2), USD_gw2 (2)),
             txflags (tfPassive));
        env (offer (snd, USD_gw2 (2), USD_gw1 (2)),
             txflags (tfPassive));

        PathSet paths (
            Path (gw1, USD_gw2, gw2),
            Path (gw2, USD_gw1, gw1));

        env (pay (snd, rcv, any (USD_gw1 (4))),
            json (paths.json ()),
            txflags (tfNoRippleDirect | tfPartialPayment));

        env.require (balance ("rcv", USD_gw1 (0)));
        env.require (balance ("rcv", USD_gw2 (2)));
    }

    void testSubtractCredits (FeatureBitset features)
    {
        testcase ("subtractCredits");

        using namespace jtx;
        Env env (*this, features);
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const alice ("alice");

        env.fund (XRP (10000), alice, gw1, gw2);

        auto j = env.app().journal ("View");

        auto const USD_gw1 = gw1["USD"];
        auto const USD_gw2 = gw2["USD"];

        env.trust (USD_gw1 (100), alice);
        env.trust (USD_gw2 (100), alice);

        env (pay (gw1, alice, USD_gw1 (50)));
        env (pay (gw2, alice, USD_gw2 (50)));

        STAmount const toCredit (USD_gw1 (30));
        STAmount const toDebit (USD_gw1 (20));
        {
//accountsend，无延期信用
            ApplyViewImpl av (&*env.current(), tapNONE);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                av, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (av, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit);

            accountSend (av, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit - toDebit);
        }

        {
//RippleCredit，无延期信贷
            ApplyViewImpl av (&*env.current(), tapNONE);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                av, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            rippleCredit (av, gw1, alice, toCredit, true, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit);

            rippleCredit (av, alice, gw1, toDebit, true, j);
            BEAST_EXPECT(accountHolds (av, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount + toCredit - toDebit);
        }

        {
//帐户发送，w/延期信用
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (pv, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);

            accountSend (pv, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }

        {
//RippleCredit，w/延期信贷
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            rippleCredit (pv, gw1, alice, toCredit, true, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);
        }

        {
//重新设计，w/延期信贷
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            redeemIOU (pv, alice, toDebit, iss, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }

        {
//发行有延期信用证
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            issueIOU (pv, alice, toCredit, iss, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);
        }

        {
//accountsend，w/deferredcredits和堆叠视图
            ApplyViewImpl av (&*env.current(), tapNONE);
            PaymentSandbox pv (&av);

            auto const iss = USD_gw1.issue ();
            auto const startingAmount = accountHolds (
                pv, alice, iss.currency, iss.account, fhIGNORE_FREEZE, j);

            accountSend (pv, gw1, alice, toCredit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount);

            {
                PaymentSandbox pv2(&pv);
                BEAST_EXPECT(accountHolds (pv2, alice, iss.currency, iss.account,
                            fhIGNORE_FREEZE, j) ==
                        startingAmount);
                accountSend (pv2, gw1, alice, toCredit, j);
                BEAST_EXPECT(accountHolds (pv2, alice, iss.currency, iss.account,
                            fhIGNORE_FREEZE, j) ==
                        startingAmount);
            }

            accountSend (pv, alice, gw1, toDebit, j);
            BEAST_EXPECT(accountHolds (pv, alice, iss.currency, iss.account,
                        fhIGNORE_FREEZE, j) ==
                    startingAmount - toDebit);
        }
    }

    void testTinyBalance (FeatureBitset features)
    {
        testcase ("Tiny balance");

//从微小的余额中加上和减去一个巨大的信用，期望微小的
//平衡回来。数值稳定性问题可能导致平衡
//为零。

        using namespace jtx;

        Env env (*this, features);

        Account const gw ("gw");
        Account const alice ("alice");
        auto const USD = gw["USD"];

        auto const issue = USD.issue ();
        STAmount tinyAmt (issue, STAmount::cMinValue, STAmount::cMinOffset + 1,
            false, false, STAmount::unchecked{});
        STAmount hugeAmt (issue, STAmount::cMaxValue, STAmount::cMaxOffset - 1,
            false, false, STAmount::unchecked{});

        for (auto d : {-1, 1})
        {
            auto const closeTime = fix1141Time () +
                d * env.closed()->info().closeTimeResolution;
            env.close (closeTime);
            ApplyViewImpl av (&*env.current (), tapNONE);
            PaymentSandbox pv (&av);
            pv.creditHook (gw, alice, hugeAmt, -tinyAmt);
            if (closeTime > fix1141Time ())
                BEAST_EXPECT(pv.balanceHook (alice, gw, hugeAmt) == tinyAmt);
            else
                BEAST_EXPECT(pv.balanceHook (alice, gw, hugeAmt) != tinyAmt);
        }
    }

    void testReserve(FeatureBitset features)
    {
        testcase ("Reserve");
        using namespace jtx;

        auto accountFundsXRP = [](ReadView const& view,
            AccountID const& id, beast::Journal j) -> XRPAmount
        {
            return toAmount<XRPAmount> (accountHolds (
                view, id, xrpCurrency (), xrpAccount (), fhZERO_IF_FROZEN, j));
        };

        auto reserve = [](jtx::Env& env, std::uint32_t count) -> XRPAmount
        {
            return env.current ()->fees ().accountReserve (count);
        };

        Env env (*this, features);

        Account const alice ("alice");
        env.fund (reserve(env, 1), alice);

        auto const closeTime = fix1141Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);
        ApplyViewImpl av (&*env.current (), tapNONE);
        PaymentSandbox sb (&av);
        {
//给艾丽丝寄一笔钱花掉。推迟的学分会使她的余额
//低于储备。确保她的资金为零（有一个错误
//使她的资金变成负值）。

            accountSend (sb, xrpAccount (), alice, XRP(100), env.journal);
            accountSend (sb, alice, xrpAccount (), XRP(100), env.journal);
            BEAST_EXPECT(
                accountFundsXRP (sb, alice, env.journal) == beast::zero);
        }
    }

    void testBalanceHook(FeatureBitset features)
    {
//确保PaymentSandbox:：BalanceHook返回的问题：：帐户
//是正确的。
        testcase ("balanceHook");

        using namespace jtx;
        Env env (*this, features);

        Account const gw ("gw");
        auto const USD = gw["USD"];
        Account const alice ("alice");

        auto const closeTime = fix1274Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        ApplyViewImpl av (&*env.current (), tapNONE);
        PaymentSandbox sb (&av);

//我们在最后一个论点中传递的货币模仿了
//通常传递给credithook，因为它来自信任线。
        Issue tlIssue = noIssue();
        tlIssue.currency = USD.issue().currency;

        sb.creditHook (gw.id(), alice.id(), {USD, 400}, {tlIssue, 600});
        sb.creditHook (gw.id(), alice.id(), {USD, 100}, {tlIssue, 600});

//预期BalanceHook（）返回的stamount issuer是正确的。
        STAmount const balance =
            sb.balanceHook (gw.id(), alice.id(), {USD, 600});
        BEAST_EXPECT (balance.getIssuer() == USD.issue().account);
    }

public:
    void run () override
    {
        auto testAll = [this](FeatureBitset features) {
            testSelfFunding(features);
            testSubtractCredits(features);
            testTinyBalance(features);
            testReserve(features);
            testBalanceHook(features);
        };
        using namespace jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa);
    }
};

BEAST_DEFINE_TESTSUITE (PaymentSandbox, ledger, ripple);

}  //测试
}  //涟漪
