
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

#include <test/jtx.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/contract.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/Sandbox.h>
#include <test/jtx/PathSet.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {
namespace test {

bool getNoRippleFlag (jtx::Env const& env,
    jtx::Account const& src,
    jtx::Account const& dst,
    Currency const& cur)
{
    if (auto sle = env.le (keylet::line (src, dst, cur)))
    {
        auto const flag = (src.id() > dst.id()) ? lsfHighNoRipple : lsfLowNoRipple;
        return sle->isFlag (flag);
    }
    Throw<std::runtime_error> ("No line in getTrustFlag");
return false; //沉默警告
}

jtx::PrettyAmount
xrpMinusFee (jtx::Env const& env, std::int64_t xrpAmount)
{
    using namespace jtx;
    auto feeDrops = env.current ()->fees ().base;
    return drops (
        dropsPerXRP<std::int64_t>::value * xrpAmount - feeDrops);
};

struct Flow_test : public beast::unit_test::suite
{
    void testDirectStep (FeatureBitset features)
    {
        testcase ("Direct Step");

        using namespace jtx;
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        auto const dan = Account ("dan");
        auto const erin = Account ("erin");
        auto const USDA = alice["USD"];
        auto const USDB = bob["USD"];
        auto const USDC = carol["USD"];
        auto const USDD = dan["USD"];
        auto const gw = Account ("gw");
        auto const USD = gw["USD"];
        {
//支付美元，琐碎之路
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, gw);
            env.trust (USD (1000), alice, bob);
            env (pay (gw, alice, USD (100)));
            env (pay (alice, bob, USD (10)), paths (USD));
            env.require (balance (bob, USD (10)));
        }
        {
//XRP转移
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob);
            env (pay (alice, bob, XRP (100)));
            env.require (balance (bob, XRP (10000 + 100)));
            env.require (balance (alice, xrpMinusFee (env, 10000 - 100)));
        }
        {
//部分付款
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, gw);
            env.trust (USD (1000), alice, bob);
            env (pay (gw, alice, USD (100)));
            env (pay (alice, bob, USD (110)), paths (USD),
                ter (tecPATH_PARTIAL));
            env.require (balance (bob, USD (0)));
            env (pay (alice, bob, USD (110)), paths (USD),
                txflags (tfPartialPayment));
            env.require (balance (bob, USD (100)));
        }
        {
//通过在账户上翻动支付，使用路径查找器
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, dan);
            env.trust (USDA (10), bob);
            env.trust (USDB (10), carol);
            env.trust (USDC (10), dan);
            env (pay (alice, dan, USDC (10)), paths (USDA));
            env.require (
                balance (bob, USDA (10)),
                balance (carol, USDB (10)),
                balance (dan, USDC (10)));
        }
        {
//通过对帐户进行循环付款，指定路径
//并收取过户费
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, dan);
            env.trust (USDA (10), bob);
            env.trust (USDB (10), alice, carol);
            env.trust (USDC (10), dan);
            env (rate (bob, 1.1));

//艾丽斯将向鲍勃兑换，并收取过户费。
            env (pay (bob, alice, USDB(6)));
            env (pay (alice, dan, USDC (5)), path (bob, carol),
                sendmax (USDA (6)), txflags (tfNoRippleDirect));
            env.require (balance (dan, USDC (5)));
            env.require (balance (alice, USDB (0.5)));
        }
        {
//通过对账户进行涟漪式支付，指定路径和转账费用
//测试Alice发行时是否不收取转让费
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, dan);
            env.trust (USDA (10), bob);
            env.trust (USDB (10), alice, carol);
            env.trust (USDC (10), dan);
            env (rate (bob, 1.1));

            env (pay (alice, dan, USDC (5)), path (bob, carol),
                 sendmax (USDA (6)), txflags (tfNoRippleDirect));
            env.require (balance (dan, USDC (5)));
            env.require (balance (bob, USDA (5)));
        }
        {
//测试最佳质量路径
//路径：A->B->D->E；A->C->D->E
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, dan, erin);
            env.trust (USDA (10), bob, carol);
            env.trust (USDB (10), dan);
            env.trust (USDC (10), alice, dan);
            env.trust (USDD (20), erin);
            env (rate (bob, 1));
            env (rate (carol, 1.1));

//付艾丽丝钱，她就把钱还给卡罗尔，并收取过户费。
            env (pay (carol, alice, USDC(10)));
            env (pay (alice, erin, USDD (5)), path (carol, dan),
                path (bob, dan), txflags (tfNoRippleDirect));

            env.require (balance (erin, USDD (5)));
            env.require (balance (dan, USDB (5)));
            env.require (balance (dan, USDC (0)));
        }
        {
//极限质量
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol);
            env.trust (USDA (10), bob);
            env.trust (USDB (10), carol);

            env (pay (alice, carol, USDB (5)), sendmax (USDA (4)),
                txflags (tfLimitQuality | tfPartialPayment), ter (tecPATH_DRY));
            env.require (balance (carol, USDB (0)));

            env (pay (alice, carol, USDB (5)), sendmax (USDA (4)),
                txflags (tfPartialPayment));
            env.require (balance (carol, USDB (4)));
        }
    }

    void testLineQuality (FeatureBitset features)
    {
        testcase ("Line Quality");

        using namespace jtx;
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        auto const dan = Account ("dan");
        auto const USDA = alice["USD"];
        auto const USDB = bob["USD"];
        auto const USDC = carol["USD"];
        auto const USDD = dan["USD"];

//dan->bob->alice->carol；改变bobdanqin和bobalieqout
        for (auto bobDanQIn : {80, 100, 120})
            for (auto bobAliceQOut : {80, 100, 120})
            {
                if (!features[featureFlow] && bobDanQIn < 100 &&
                    bobAliceQOut < 100)
continue;  //流V1中的缺陷
                Env env(*this, features);
                env.fund(XRP(10000), alice, bob, carol, dan);
                env(trust(bob, USDD(100)), qualityInPercent(bobDanQIn));
                env(trust(bob, USDA(100)), qualityOutPercent(bobAliceQOut));
                env(trust(carol, USDA(100)));

                env(pay(alice, bob, USDA(100)));
                env.require(balance(bob, USDA(100)));
                env(pay(dan, carol, USDA(10)),
                    path(bob), sendmax(USDD(100)), txflags(tfNoRippleDirect));
                env.require(balance(bob, USDA(90)));
                if (bobAliceQOut > bobDanQIn)
                    env.require(balance(
                        bob,
                        USDD(10.0 * double(bobAliceQOut) / double(bobDanQIn))));
                else
                    env.require(balance(bob, USDD(10)));
                env.require(balance(carol, USDA(10)));
            }

//Bob->Alice->Carol；Vary Caroliceqin
        for (auto carolAliceQIn : {80, 100, 120})
        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, carol);
            env(trust(bob, USDA(10)));
            env(trust(carol, USDA(10)), qualityInPercent(carolAliceQIn));

            env(pay(alice, bob, USDA(10)));
            env.require(balance(bob, USDA(10)));
            env(pay(bob, carol, USDA(5)), sendmax(USDA(10)));
            auto const effectiveQ =
                carolAliceQIn > 100 ? 1.0 : carolAliceQIn / 100.0;
            env.require(balance(bob, USDA(10.0 - 5.0 / effectiveQ)));
        }

//bob->alice->carol；bobaliceqout变化。
        for (auto bobAliceQOut : {80, 100, 120})
        {
            Env env(*this, features);
            env.fund(XRP(10000), alice, bob, carol);
            env(trust(bob, USDA(10)), qualityOutPercent(bobAliceQOut));
            env(trust(carol, USDA(10)));

            env(pay(alice, bob, USDA(10)));
            env.require(balance(bob, USDA(10)));
            env(pay(bob, carol, USDA(5)), sendmax(USDA(5)));
            env.require(balance(carol, USDA(5)));
            env.require(balance(bob, USDA(10 - 5)));
        }
    }

    void testBookStep (FeatureBitset features)
    {
        testcase ("Book Step");

        using namespace jtx;

        auto const gw = Account ("gateway");
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        auto const EUR = gw["EUR"];
        Account const alice ("alice");
        Account const bob ("bob");
        Account const carol ("carol");

        {
//简单借据/借据报价
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);

            env (pay (gw, alice, BTC (50)));
            env (pay (gw, bob, USD (50)));

            env (offer (bob, BTC (50), USD (50)));

            env (pay (alice, carol, USD (50)), path (~USD), sendmax (BTC (50)));

            env.require (balance (alice, BTC (0)));
            env.require (balance (bob, BTC (50)));
            env.require (balance (bob, USD (0)));
            env.require (balance (carol, USD (50)));
            BEAST_EXPECT(!isOffer (env, bob, BTC (50), USD (50)));
        }
        {
//简单IOU/XRP XRP/IOU优惠
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);

            env (pay (gw, alice, BTC (50)));
            env (pay (gw, bob, USD (50)));

            env (offer (bob, BTC (50), XRP (50)));
            env (offer (bob, XRP (50), USD (50)));

            env (pay (alice, carol, USD (50)), path (~XRP, ~USD),
                sendmax (BTC (50)));

            env.require (balance (alice, BTC (0)));
            env.require (balance (bob, BTC (50)));
            env.require (balance (bob, USD (0)));
            env.require (balance (carol, USD (50)));
            BEAST_EXPECT(!isOffer (env, bob, XRP (50), USD (50)));
            BEAST_EXPECT(!isOffer (env, bob, BTC (50), XRP (50)));
        }
        {
//简单的xrp->usd到offer和sendmax
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);

            env (pay (gw, bob, USD (50)));

            env (offer (bob, XRP (50), USD (50)));

            env (pay (alice, carol, USD (50)), path (~USD),
                 sendmax (XRP (50)));

            env.require (balance (alice, xrpMinusFee (env, 10000 - 50)));
            env.require (balance (bob, xrpMinusFee (env, 10000 + 50)));
            env.require (balance (bob, USD (0)));
            env.require (balance (carol, USD (50)));
            BEAST_EXPECT(!isOffer (env, bob, XRP (50), USD (50)));
        }
        {
//简单美元->xrp到offer和sendmax
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);

            env (pay (gw, alice, USD (50)));

            env (offer (bob, USD (50), XRP (50)));

            env (pay (alice, carol, XRP (50)), path (~XRP),
                 sendmax (USD (50)));

            env.require (balance (alice, USD (0)));
            env.require (balance (bob, xrpMinusFee (env, 10000 - 50)));
            env.require (balance (bob, USD (50)));
            env.require (balance (carol, XRP (10000 + 50)));
            BEAST_EXPECT(!isOffer (env, bob, USD (50), XRP (50)));
        }
        {
//当支付成功时，测试未提供资金的报价将被删除。
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);
            env.trust (EUR (1000), alice, bob, carol);

            env (pay (gw, alice, BTC (60)));
            env (pay (gw, bob, USD (50)));
            env (pay (gw, bob, EUR (50)));

            env (offer (bob, BTC (50), USD (50)));
            env (offer (bob, BTC (60), EUR (50)));
            env (offer (bob, EUR (50), USD (50)));

//不提供资金
            env (pay (bob, gw, EUR (50)));
            BEAST_EXPECT(isOffer (env, bob, BTC (50), USD (50)));
            BEAST_EXPECT(isOffer (env, bob, BTC (60), EUR (50)));
            BEAST_EXPECT(isOffer (env, bob, EUR (50), USD (50)));

            env (pay (alice, carol, USD (50)),
                path (~USD), path (~EUR, ~USD),
                sendmax (BTC (60)));

            env.require (balance (alice, BTC (10)));
            env.require (balance (bob, BTC (50)));
            env.require (balance (bob, USD (0)));
            env.require (balance (bob, EUR (0)));
            env.require (balance (carol, USD (50)));
//用于支付
            BEAST_EXPECT(!isOffer (env, bob, BTC (50), USD (50)));
//发现资金不足
            BEAST_EXPECT(!isOffer (env, bob, BTC (60), EUR (50)));
//没有资金，但还不应该被发现没有资金
            BEAST_EXPECT(isOffer (env, bob, EUR (50), USD (50)));
        }
        {
//当付款失败时，将返回测试未提供资金的报价。
//Bob提供了两个报价：一个为50 BTC提供资金50美元，另一个为50 BTC提供资金。
//60 BTC欧元。爱丽丝用61英磅的现金付给卡罗尔61美元。爱丽丝只
//有60个BTC，所以付款失败。付款方式有两种：
//一个是Bob提供的资金，另一个是他提供的资金
//报盘。当付款失败时，“流”应返回无资金
//报盘。此测试有意类似于删除
//当付款成功时，无资金提供。
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (BTC (1000), alice, bob, carol);
            env.trust (EUR (1000), alice, bob, carol);

            env (pay (gw, alice, BTC (60)));
            env (pay (gw, bob, USD (50)));
            env (pay (gw, bob, EUR (50)));

            env (offer (bob, BTC (50), USD (50)));
            env (offer (bob, BTC (60), EUR (50)));
            env (offer (bob, EUR (50), USD (50)));

//不提供资金
            env (pay (bob, gw, EUR (50)));
            BEAST_EXPECT(isOffer (env, bob, BTC (50), USD (50)));
            BEAST_EXPECT(isOffer (env, bob, BTC (60), EUR (50)));

            auto flowJournal = env.app ().logs ().journal ("Flow");
            auto const flowResult = [&]
            {
                STAmount deliver (USD (51));
                STAmount smax (BTC (61));
                PaymentSandbox sb (env.current ().get (), tapNONE);
                STPathSet paths;
                auto IPE = [](Issue const& iss) {
                    return STPathElement(
                        STPathElement::typeCurrency | STPathElement::typeIssuer,
                        xrpAccount(),
                        iss.currency,
                        iss.account);
                };
                {

//BTC >美元
                    STPath p1 ({IPE (USD.issue ())});
                    paths.push_back (p1);
//BTC->欧元->美元
                    STPath p2 ({IPE (EUR.issue ()), IPE (USD.issue ())});
                    paths.push_back (p2);
                }

                return flow (sb, deliver, alice, carol, paths, false, false,
                    true, false, boost::none, smax, flowJournal);
            }();

            BEAST_EXPECT(flowResult.removableOffers.size () == 1);
            env.app ().openLedger ().modify (
                [&](OpenView& view, beast::Journal j)
                {
                    if (flowResult.removableOffers.empty())
                        return false;
                    Sandbox sb (&view, tapNONE);
                    for (auto const& o : flowResult.removableOffers)
                        if (auto ok = sb.peek (keylet::offer (o)))
                            offerDelete (sb, ok, flowJournal);
                    sb.apply (view);
                    return true;
                });

//用于付款，但由于付款失败，应保持不变
            BEAST_EXPECT(isOffer (env, bob, BTC (50), USD (50)));
//发现资金不足
            BEAST_EXPECT(!isOffer (env, bob, BTC (60), EUR (50)));
        }
        {
//向前传球不比向后传球多
//此测试使用的路径的反向传递将计算
//1欧元产出需要0.5美元投入。它将sendmax设置为
//0.4美元，所以支付引擎需要做一个远期通行证。
//如果没有限制，0.4美元远期将产生1000欧元。
//通过。此测试检查付款是否产生1欧元，如预期。

            Env env (*this, features);

            auto const closeTime = STAmountSO::soTime2 +
                100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env.trust (USD (1000), alice, bob, carol);
            env.trust (EUR (1000), alice, bob, carol);

            env (pay (gw, alice, USD (1000)));
            env (pay (gw, bob, EUR (1000)));

            env (offer (bob, USD (1), drops (2)), txflags (tfPassive));
            env (offer (bob, drops (1), EUR (1000)), txflags (tfPassive));

            env (pay (alice, carol, EUR (1)), path (~XRP, ~EUR),
                sendmax (USD (0.4)), txflags (tfNoRippleDirect|tfPartialPayment));

            env.require (balance (carol, EUR (1)));
            env.require (balance (bob, USD (0.4)));
            env.require (balance (bob, EUR (999)));
        }
    }

    void testTransferRate (FeatureBitset features)
    {
        testcase ("Transfer Rate");

        using namespace jtx;

        auto const gw = Account ("gateway");
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        auto const EUR = gw["EUR"];
        Account const alice ("alice");
        Account const bob ("bob");
        Account const carol ("carol");


        {
//通过带有
//转移率
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol);
            env (pay (gw, alice, USD (50)));
            env.require (balance (alice, USD (50)));
            env (pay (alice, bob, USD (40)), sendmax (USD (50)));
            env.require (balance (bob, USD (40)), balance (alice, USD (0)));
        }
        {
//当发卡行为SRC或DST时，不收取传输率。
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol);
            env (pay (gw, alice, USD (50)));
            env.require (balance (alice, USD (50)));
            env (pay (alice, gw, USD (40)), sendmax (USD (40)));
            env.require (balance (alice, USD (10)));
        }
        {
//报价转让费
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol);
            env (pay (gw, bob, USD (65)));

            env (offer (bob, XRP (50), USD (50)));

            env (pay (alice, carol, USD (50)), path (~USD), sendmax (XRP (50)));
            env.require (
                balance (alice, xrpMinusFee (env, 10000 - 50)),
balance (bob, USD (2.5)), //业主支付转让费
                balance (carol, USD (50)));
        }

        {
//转让费两次连续报价
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, carol, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol);
            env.trust (EUR (1000), alice, bob, carol);
            env (pay (gw, bob, USD (50)));
            env (pay (gw, bob, EUR (50)));

            env (offer (bob, XRP (50), USD (50)));
            env (offer (bob, USD (50), EUR (50)));

            env (pay (alice, carol, EUR (40)), path (~USD, ~EUR), sendmax (XRP (40)));
            env.require (
                balance (alice,  xrpMinusFee (env, 10000 - 40)),
                balance (bob, USD (40)),
                balance (bob, EUR (0)),
                balance (carol, EUR (40)));
        }

        {
//第一次通过股赎回，第二次通过发行，无报价
//限制步骤不是终结点
            Env env (*this, features);
            auto const USDA = alice["USD"];
            auto const USDB = bob["USD"];

            env.fund (XRP (10000), alice, bob, carol, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol);
            env.trust (USDA (1000), bob);
            env.trust (USDB (1000), gw);
            env (pay (gw, bob, USD (50)));
//爱丽丝->鲍勃->吉布达·伟士->卡罗尔。50美元应该有转让费；10美元，不收费
            env (pay (alice, carol, USD(50)), path (bob), sendmax (USDA(60)));
            env.require (
                balance (bob, USD (-10)),
                balance (bob, USDA (60)),
                balance (carol, USD (50)));
        }
        {
//第一次通过股赎回，第二次通过发行，通过出价
//限制步骤不是终结点
            Env env (*this, features);
            auto const USDA = alice["USD"];
            auto const USDB = bob["USD"];
            Account const dan ("dan");

            env.fund (XRP (10000), alice, bob, carol, dan, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob, carol, dan);
            env.trust (EUR (1000), carol, dan);
            env.trust (USDA (1000), bob);
            env.trust (USDB (1000), gw);
            env (pay (gw, bob, USD (50)));
            env (pay (gw, dan, EUR (100)));
            env (offer (dan, USD (100), EUR (100)));
//爱丽丝->鲍勃->吉布达·伟士->卡罗尔。50美元应该有转让费；10美元，不收费
            env (pay (alice, carol, EUR (50)), path (bob, gw, ~EUR),
                sendmax (USDA (60)), txflags (tfNoRippleDirect));
            env.require (
                balance (bob, USD (-10)),
                balance (bob, USDA (60)),
                balance (dan, USD (50)),
                balance (dan, EUR (37.5)),
                balance (carol, EUR (50)));
        }

        {
//如果所有人也是发行人，所有人支付费用。
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob);
            env (offer (gw, XRP (100), USD (100)));
            env (pay (alice, bob, USD (100)),
                 sendmax (XRP (100)));
            env.require (
                balance (alice, xrpMinusFee(env, 10000-100)),
                balance (bob, USD (100)));
        }
        if (!features[featureOwnerPaysFee])
        {
//如果所有人也是发行人，发送人支付费用。
            Env env (*this, features);

            env.fund (XRP (10000), alice, bob, gw);
            env(rate(gw, 1.25));
            env.trust (USD (1000), alice, bob);
            env (offer (gw, XRP (125), USD (125)));
            env (pay (alice, bob, USD (100)),
                 sendmax (XRP (200)));
            env.require (
                balance (alice, xrpMinusFee(env, 10000-125)),
                balance (bob, USD (100)));
        }
    }

    void
    testFalseDry(FeatureBitset features)
    {
        testcase ("falseDryChanges");

        using namespace jtx;

        auto const gw = Account ("gateway");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];
        Account const alice ("alice");
        Account const bob ("bob");
        Account const carol ("carol");

        Env env (*this, features);

        auto const closeTime = fix1141Time() +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000), alice, carol, gw);
        env.fund (reserve(env, 5), bob);
        env.trust (USD (1000), alice, bob, carol);
        env.trust (EUR (1000), alice, bob, carol);


        env (pay (gw, alice, EUR (50)));
        env (pay (gw, bob, USD (50)));

//Bob的xrp略低于50
//如果他的所有者计数发生变化，他将有更多的流动性。
//这是一个要测试的错误情况（使用流时）。
//计算输入xrp到xrp/usd报价需要两个
//对eur/xrp产品的递归调用。第二个电话会回来
//tecpath_干燥，但整个路径不应标记为干燥。这个
//是要测试的第二个错误情况（使用flowv1时）。
        env (offer (bob, EUR (50), XRP (50)));
        env (offer (bob, XRP (50), USD (50)));

        env (pay (alice, carol, USD (1000000)), path (~XRP, ~USD),
            sendmax (EUR (500)),
            txflags (tfNoRippleDirect | tfPartialPayment));

        auto const carolUSD = env.balance(carol, USD).value();
        BEAST_EXPECT(carolUSD > USD (0) && carolUSD < USD (50));
    }

    void
    testLimitQuality ()
    {
//有两个报价和限制质量的单程。质量限制是
//这样第一个报价应该被接受，第二个不应该。
//交付的总金额应为两个报价和
//sendmax应该超过第一个报价。
        testcase ("limitQuality");
        using namespace jtx;

        auto const gw = Account ("gateway");
        auto const USD = gw["USD"];
        Account const alice ("alice");
        Account const bob ("bob");
        Account const carol ("carol");

        auto const timeDelta = Env{*this}.closed ()->info ().closeTimeResolution;

        for (auto const& d : {-100 * timeDelta, +100 * timeDelta})
        {
            auto const closeTime = fix1141Time () + d ;
            Env env (*this, FeatureBitset{});
            env.close (closeTime);

            env.fund (XRP(10000), alice, bob, carol, gw);

            env.trust (USD(100), alice, bob, carol);
            env (pay (gw, bob, USD (100)));
            env (offer (bob, XRP (50), USD (50)));
            env (offer (bob, XRP (100), USD (50)));

            TER const expectedResult = closeTime < fix1141Time ()
                ? TER {tecPATH_DRY}
                : TER {tesSUCCESS};
            env (pay (alice, carol, USD (100)), path (~USD), sendmax (XRP (100)),
                txflags (tfNoRippleDirect | tfPartialPayment | tfLimitQuality),
                ter (expectedResult));

            if (expectedResult == tesSUCCESS)
                env.require (balance (carol, USD (50)));
        }
    }

//帮助程序函数，该函数基于
//传入的所有者数。
    static XRPAmount reserve(jtx::Env& env, std::uint32_t count)
    {
        return env.current()->fees().accountReserve (count);
    }

//帮助程序函数，用于返回帐户上的报价。
    static std::vector<std::shared_ptr<SLE const>>
    offersOnAccount (jtx::Env& env, jtx::Account account)
    {
        std::vector<std::shared_ptr<SLE const>> result;
        forEachItem (*env.current (), account,
            [&result](std::shared_ptr<SLE const> const& sle)
            {
                if (sle->getType() == ltOFFER)
                     result.push_back (sle);
            });
        return result;
    }

    void
    testSelfPayment1(FeatureBitset features)
    {
        testcase ("Self-payment 1");

//在这个测试用例中，新的流代码错误地计算了
//要搬家的钱。幸运的是新代码重新执行了
//check捕获问题并抛出事务。
//
//旧付款代码正确处理付款。
        using namespace jtx;

        auto const gw1 = Account ("gw1");
        auto const gw2 = Account ("gw2");
        auto const alice = Account ("alice");
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        Env env (*this, features);

        auto const closeTime =
            fix1141Time () + 100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP (1000000), gw1, gw2);
        env.close ();

//为交易收取的费用。
        auto const f = env.current ()->fees ().base;

        env.fund (reserve (env, 3) + f * 4, alice);
        env.close ();

        env (trust (alice, USD (2000)));
        env (trust (alice, EUR (2000)));
        env.close ();

        env (pay (gw1, alice, USD (1)));
        env (pay (gw2, alice, EUR (1000)));
        env.close ();

        env (offer (alice, USD (500), EUR (600)));
        env.close ();

        env.require (owners (alice, 3));
        env.require (balance (alice, USD (1)));
        env.require (balance (alice, EUR (1000)));

        auto aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size () == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == EUR (600));
            BEAST_EXPECT(offer[sfTakerPays] == USD (500));
        }

        env (pay (alice, alice, EUR (600)), sendmax (USD (500)),
            txflags (tfPartialPayment));
        env.close ();

        env.require (owners (alice, 3));
        env.require (balance (alice, USD (1)));
        env.require (balance (alice, EUR (1000)));
        aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size () == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == EUR (598.8));
            BEAST_EXPECT(offer[sfTakerPays] == USD (499));
        }
    }

    void
    testSelfPayment2(FeatureBitset features)
    {
        testcase ("Self-payment 2");

//在这种情况下，旧支付代码和
//新的是报价中留下的价值观。也不说
//iOS铃声，只是不同而已。
        using namespace jtx;

        auto const gw1 = Account ("gw1");
        auto const gw2 = Account ("gw2");
        auto const alice = Account ("alice");
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        Env env (*this, features);

        auto const closeTime =
            fix1141Time () + 100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP (1000000), gw1, gw2);
        env.close ();

//为交易收取的费用。
        auto const f = env.current ()->fees ().base;

        env.fund (reserve (env, 3) + f * 4, alice);
        env.close ();

        env (trust (alice, USD (506)));
        env (trust (alice, EUR (606)));
        env.close ();

        env (pay (gw1, alice, USD (500)));
        env (pay (gw2, alice, EUR (600)));
        env.close ();

        env (offer (alice, USD (500), EUR (600)));
        env.close ();

        env.require (owners (alice, 3));
        env.require (balance (alice, USD (500)));
        env.require (balance (alice, EUR (600)));

        auto aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size () == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == EUR (600));
            BEAST_EXPECT(offer[sfTakerPays] == USD (500));
        }

        env (pay (alice, alice, EUR (60)), sendmax (USD (50)),
            txflags (tfPartialPayment));
        env.close ();

        env.require (owners (alice, 3));
        env.require (balance (alice, USD (500)));
        env.require (balance (alice, EUR (600)));
        aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size () == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == EUR (594));
            BEAST_EXPECT(offer[sfTakerPays] == USD (495));
        }
    }
    void testSelfFundedXRPEndpoint (bool consumeOffer, FeatureBitset features)
    {
//测试是否未绕过延期信用表
//xrppendpointsteps。如果第一步中的帐户正在发送xrp和
//该帐户还拥有一个接收xrp的报价，它不应该
//该步骤可以使用报价中收到的XRP作为部分
//付款方式。
        testcase("Self funded XRPEndpoint");

        using namespace jtx;

        Env env(*this, features);

//需要“accountholds”中的新行为
        auto const closeTime = fix1141Time() +
            env.closed()->info().closeTimeResolution;
        env.close(closeTime);

        auto const alice = Account("alice");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];

        env.fund(XRP(10000), alice, gw);
        env(trust(alice, USD(20)));
        env(pay(gw, alice, USD(10)));
        env(offer(alice, XRP(50000), USD(10)));

//使用报价会更改所有者计数，这也可能导致
//远期流动性减少
        auto const toSend = consumeOffer ? USD(10) : USD(9);
        env(pay(alice, alice, toSend), path(~USD), sendmax(XRP(20000)),
            txflags(tfPartialPayment | tfNoRippleDirect));
    }

    void testUnfundedOffer (bool withFix, FeatureBitset features)
    {
        testcase(std::string("Unfunded Offer ") +
            (withFix ? "with fix" : "without fix"));

        using namespace jtx;
        {
//测试反向
            Env env(*this, features);
            auto closeTime = fix1298Time();
            if (withFix)
                closeTime += env.closed()->info().closeTimeResolution;
            else
                closeTime -= env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            auto const alice = Account("alice");
            auto const bob = Account("bob");
            auto const gw = Account("gw");
            auto const USD = gw["USD"];

            env.fund(XRP(100000), alice, bob, gw);
            env(trust(bob, USD(20)));

            STAmount tinyAmt1{USD.issue(), 9000000000000000ll, -17, false,
                false, STAmount::unchecked{}};
            STAmount tinyAmt3{USD.issue(), 9000000000000003ll, -17, false,
                false, STAmount::unchecked{}};

            env(offer(gw, drops(9000000000), tinyAmt3));
            env(pay(alice, bob, tinyAmt1), path(~USD),
                sendmax(drops(9000000000)), txflags(tfNoRippleDirect));

            if (withFix)
                BEAST_EXPECT(!isOffer(env, gw, XRP(0), USD(0)));
            else
                BEAST_EXPECT(isOffer(env, gw, XRP(0), USD(0)));
        }
        {
//正向测试
            Env env(*this, features);
            auto closeTime = fix1298Time();
            if (withFix)
                closeTime += env.closed()->info().closeTimeResolution;
            else
                closeTime -= env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            auto const alice = Account("alice");
            auto const bob = Account("bob");
            auto const gw = Account("gw");
            auto const USD = gw["USD"];

            env.fund(XRP(100000), alice, bob, gw);
            env(trust(alice, USD(20)));

            STAmount tinyAmt1{USD.issue(), 9000000000000000ll, -17, false,
                false, STAmount::unchecked{}};
            STAmount tinyAmt3{USD.issue(), 9000000000000003ll, -17, false,
                false, STAmount::unchecked{}};

            env(pay(gw, alice, tinyAmt1));

            env(offer(gw, tinyAmt3, drops(9000000000)));
            env(pay(alice, bob, drops(9000000000)), path(~XRP),
                sendmax(USD(1)), txflags(tfNoRippleDirect));

            if (withFix)
                BEAST_EXPECT(!isOffer(env, gw, USD(0), XRP(0)));
            else
                BEAST_EXPECT(isOffer(env, gw, USD(0), XRP(0)));
        }
    }

    void
    testReexecuteDirectStep(FeatureBitset features)
    {
        testcase("ReexecuteDirectStep");

        using namespace jtx;
        Env env(*this, features);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const usdC = USD.currency;

        env.fund(XRP(10000), alice, bob, gw);
//需要经过这段时间才能看到错误
        env.close(fix1274Time() +
            100 * env.closed()->info().closeTimeResolution);
        env(trust(alice, USD(100)));
        env.close();

        BEAST_EXPECT(!getNoRippleFlag(env, gw, alice, usdC));

        env(pay(
            gw, alice,
//12.55…
            STAmount{USD.issue(), std::uint64_t(1255555555555555ull), -14, false}));

        env(offer(gw,
//5…
            STAmount{
                USD.issue(), std::uint64_t(5000000000000000ull), -15, false},
            XRP(1000)));

        env(offer(gw,
//555…
            STAmount{
                USD.issue(), std::uint64_t(5555555555555555ull), -16, false},
            XRP(10)));

        env(offer(gw,
//4.44…
            STAmount{
                USD.issue(), std::uint64_t(4444444444444444ull), -15, false},
            XRP(.1)));

        env(offer(alice,
//十七
            STAmount{
                USD.issue(), std::uint64_t(1700000000000000ull), -14, false},
            XRP(.001)));

        env(pay(alice, bob, XRP(10000)), path(~XRP), sendmax(USD(100)),
            txflags(tfPartialPayment | tfNoRippleDirect));
    }

    void
    testRIPD1443(bool withFix)
    {
        testcase("ripd1443");

        using namespace jtx;
        Env env(*this);
        auto const timeDelta = env.closed ()->info ().closeTimeResolution;
        auto const d = withFix ? 100*timeDelta : -100*timeDelta;
        auto closeTime = fix1443Time() + d;
        env.close(closeTime);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");

        env.fund(XRP(100000000), alice, noripple(bob), carol, gw);
        env.trust(gw["USD"](10000), alice, carol);
        env(trust(bob, gw["USD"](10000), tfSetNoRipple));
        env.trust(gw["USD"](10000), bob);
        env.close();

//在Bob和网关之间不设置涟漪

        env(pay(gw, alice, gw["USD"](1000)));
        env.close();

        env(offer(alice, bob["USD"](1000), XRP(1)));
        env.close();

        env(pay(alice, alice, XRP(1)), path(gw, bob, ~XRP),
            sendmax(gw["USD"](1000)), txflags(tfNoRippleDirect),
            ter(withFix ? TER {tecPATH_DRY} : TER {tesSUCCESS}));
        env.close();

        if (withFix)
        {
            env.trust(bob["USD"](10000), alice);
            env(pay(bob, alice, bob["USD"](1000)));
        }

        env(offer(alice, XRP(1000), bob["USD"](1000)));
        env.close();

        env(pay (carol, carol, gw["USD"](1000)), path(~bob["USD"], gw),
            sendmax(XRP(100000)), txflags(tfNoRippleDirect),
            ter(withFix ? TER {tecPATH_DRY} : TER {tesSUCCESS}));
        env.close();

        pass();
    }

    void
    testRIPD1449(bool withFix)
    {
        testcase("ripd1449");

        using namespace jtx;
        Env env(*this);
        auto const timeDelta = env.closed ()->info ().closeTimeResolution;
        auto const d = withFix ? 100*timeDelta : -100*timeDelta;
        auto closeTime = fix1449Time() + d;
        env.close(closeTime);

//支付alice->xrp->usd/bob->bob->gw->alice
//在Bob/GW信托线路的Bob方面不设任何涟漪
//Carol有Bob/USD并出价，Bob有美元/GW

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");
        auto const gw = Account("gw");
        auto const USD = gw["USD"];

        env.fund(XRP(100000000), alice, bob, carol, gw);
        env.trust(USD(10000), alice, carol);
        env(trust(bob, USD(10000), tfSetNoRipple));
        env.trust(USD(10000), bob);
        env.trust(bob["USD"](10000), carol);
        env.close();

        env(pay(bob, carol, bob["USD"](1000)));
        env(pay(gw, bob, USD(1000)));
        env.close();

        env(offer(carol, XRP(1), bob["USD"](1000)));
        env.close();

        env(pay(alice, alice, USD(1000)), path(~bob["USD"], bob, gw),
            sendmax(XRP(1)), txflags(tfNoRippleDirect),
            ter(withFix ? TER {tecPATH_DRY} : TER {tesSUCCESS}));
        env.close();
    }

    void
    testSelfPayLowQualityOffer (FeatureBitset features)
    {
//新的付款代码，用于声明是否为更多
//xrp比持有的发行账户。此单元测试复制
//那个失败的案例。
        testcase ("Self crossing low quality offer");

        using namespace jtx;

        Env env(*this, features);

        auto const ann = Account("ann");
        auto const gw = Account("gateway");
        auto const CTB = gw["CTB"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (9999640) + (fee), ann);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (rate(gw, 1.002));
        env (trust(ann, CTB(10)));
        env.close();

        env (pay(gw, ann, CTB(2.856)));
        env.close();

        env (offer(ann, drops(365611702030), CTB(5.713)));
        env.close();

//此付款导致断言。
        env (pay(ann, ann, CTB(0.687)),
             sendmax (drops(20000000000)), txflags (tfPartialPayment));
    }

    void
    testEmptyStrand(FeatureBitset features)
    {
        testcase("Empty Strand");
        using namespace jtx;

        auto const alice = Account("alice");

        Env env(*this, features);

        env.fund(XRP(10000), alice);

        env(pay(alice, alice,
                alice["USD"](100)),
            path(~alice["USD"]),
            ter(temBAD_PATH));
    }

    void testWithFeats(FeatureBitset features)
    {
        using namespace jtx;
        FeatureBitset const ownerPaysFee{featureOwnerPaysFee};

        testLineQuality(features);
        testFalseDry(features);
//仅在启用FeatureFlow时执行其余测试。
        if (!features[featureFlow])
            return;
        testDirectStep(features);
        testBookStep(features);
        testDirectStep(features | ownerPaysFee);
        testBookStep(features | ownerPaysFee);
        testTransferRate(features | ownerPaysFee);
        testSelfPayment1(features);
        testSelfPayment2(features);
        testSelfFundedXRPEndpoint(false, features);
        testSelfFundedXRPEndpoint(true, features);
        testUnfundedOffer(true, features);
        testUnfundedOffer(false, features);
        testReexecuteDirectStep(features | fix1368);
        testSelfPayLowQualityOffer(features);
    }

    void run() override
    {
        testLimitQuality();
        testRIPD1443(true);
        testRIPD1443(false);
        testRIPD1449(true);
        testRIPD1449(false);

        using namespace jtx;
        auto const sa = supported_amendments();
        testWithFeats(sa - featureFlow - fix1373 - featureFlowCross);
        testWithFeats(sa               - fix1373 - featureFlowCross);
        testWithFeats(sa                         - featureFlowCross);
        testWithFeats(sa);
        testEmptyStrand(sa);
    }
};

struct Flow_manual_test : public Flow_test
{
    void run() override
    {
        using namespace jtx;
        auto const all = supported_amendments();
        FeatureBitset const flow{featureFlow};
        FeatureBitset const f1373{fix1373};
        FeatureBitset const flowCross{featureFlowCross};
        FeatureBitset const f1513{fix1513};

        testWithFeats(all                 - flow - f1373 - flowCross - f1513);
        testWithFeats(all                 - flow - f1373 - flowCross        );
        testWithFeats(all                        - f1373 - flowCross - f1513);
        testWithFeats(all                        - f1373 - flowCross        );
        testWithFeats(all                                - flowCross - f1513);
        testWithFeats(all                                - flowCross        );
        testWithFeats(all                                            - f1513);
        testWithFeats(all                                                      );

        testEmptyStrand(all                 - f1513);
        testEmptyStrand(all                        );
    }
};

BEAST_DEFINE_TESTSUITE_PRIO(Flow,app,ripple,2);
BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Flow_manual,app,ripple,4);

} //测试
} //涟漪
