
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
  版权所有（c）2012-2017 Ripple Labs Inc.

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
#include <test/jtx/WSClient.h>
#include <test/jtx/PathSet.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace test {

class Offer_test : public beast::unit_test::suite
{
    XRPAmount reserve(jtx::Env& env, std::uint32_t count)
    {
        return env.current()->fees().accountReserve (count);
    }

    std::uint32_t lastClose (jtx::Env& env)
    {
        return env.current()->info().parentCloseTime.time_since_epoch().count();
    }

    static auto
    xrpMinusFee (jtx::Env const& env, std::int64_t xrpAmount)
        -> jtx::PrettyAmount
    {
        using namespace jtx;
        auto feeDrops = env.current()->fees().base;
        return drops (dropsPerXRP<std::int64_t>::value * xrpAmount - feeDrops);
    }

    static auto
    ledgerEntryState(jtx::Env & env, jtx::Account const& acct_a,
        jtx::Account const & acct_b, std::string const& currency)
    {
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::ripple_state][jss::currency] = currency;
        jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
        jvParams[jss::ripple_state][jss::accounts].append(acct_a.human());
        jvParams[jss::ripple_state][jss::accounts].append(acct_b.human());
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    ledgerEntryRoot (jtx::Env & env, jtx::Account const& acct)
    {
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::account_root] = acct.human();
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    ledgerEntryOffer (jtx::Env & env,
        jtx::Account const& acct, std::uint32_t offer_seq)
    {
        Json::Value jvParams;
        jvParams[jss::offer][jss::account] = acct.human();
        jvParams[jss::offer][jss::seq] = offer_seq;
        return env.rpc ("json", "ledger_entry",
            to_string(jvParams))[jss::result];
    }

    static auto
    getBookOffers(jtx::Env & env,
        Issue const& taker_pays, Issue const& taker_gets)
    {
        Json::Value jvbp;
        jvbp[jss::ledger_index] = "current";
        jvbp[jss::taker_pays][jss::currency] = to_string(taker_pays.currency);
        jvbp[jss::taker_pays][jss::issuer] = to_string(taker_pays.account);
        jvbp[jss::taker_gets][jss::currency] = to_string(taker_gets.currency);
        jvbp[jss::taker_gets][jss::issuer] = to_string(taker_gets.account);
        return env.rpc("json", "book_offers", to_string(jvbp)) [jss::result];
    }

public:
    void testRmFundedOffer (FeatureBitset features)
    {
        testcase ("Incorrect Removal of Funded Offers");

//我们至少需要两条路。一个质量好，一个质量差
//质量。质量差的路线需要连续两本书。
//每本报价书应有两个质量相同的报价，即
//报价应完全消费，付款应
//需要满足两个报价。第一个报价必须
//成为“接受者得到”xrp。旧的，坏的会去掉第一个
//“接受者得到”xrp优惠，即使该优惠仍然是资助和
//不用于付款。

        using namespace jtx;
        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob, carol);
        env.trust (BTC (1000), alice, bob, carol);

        env (pay (gw, alice, BTC (1000)));

        env (pay (gw, carol, USD (1000)));
        env (pay (gw, carol, BTC (1000)));

//必须是同一质量的两个报价
//“Taker Gets”必须是xrp
//（不同的金额，以便我区分报价）
        env (offer (carol, BTC (49), XRP (49)));
        env (offer (carol, BTC (51), XRP (51)));

//为劣质道路提供服务
//必须是同一质量的两个报价
        env (offer (carol, XRP (50), USD (50)));
        env (offer (carol, XRP (50), USD (50)));

//提供优质路线
        env (offer (carol, BTC (1), USD (100)));

        PathSet paths (Path (XRP, USD), Path (USD));

        env (pay (alice, bob, USD (100)), json (paths.json ()),
            sendmax (BTC (1000)), txflags (tfPartialPayment));

        env.require (balance (bob, USD (100)));
        BEAST_EXPECT(!isOffer (env, carol, BTC (1), USD (100)) &&
            isOffer (env, carol, BTC (49), XRP (49)));
    }

    void testCanceledOffer (FeatureBitset features)
    {
        testcase ("Removing Canceled Offers");

        using namespace jtx;
        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), alice, gw);
        env.close();
        env.trust (USD (100), alice);
        env.close();

        env (pay (gw, alice, USD (50)));
        env.close();

        auto const offer1Seq = env.seq (alice);

        env (offer (alice, XRP (500), USD (100)),
            require (offers (alice, 1)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (500), USD (100)));

//取消上述报价，并用新报价替换。
        auto const offer2Seq = env.seq (alice);

        env (offer (alice, XRP (300), USD (100)),
             json (jss::OfferSequence, offer1Seq),
             require (offers (alice, 1)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (300), USD (100)) &&
            !isOffer (env, alice, XRP (500), USD (100)));

//测试取消不存在的报价。
//auto const offer3seq=env.seq（alice）；

        env (offer (alice, XRP (400), USD (200)),
             json (jss::OfferSequence, offer1Seq),
             require (offers (alice, 2)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (300), USD (100)) &&
            isOffer (env, alice, XRP (400), USD (200)));

//现在用offerCancel tx测试取消
        auto const offer4Seq = env.seq (alice);
        env (offer (alice, XRP (222), USD (111)),
            require (offers (alice, 3)));
        env.close();

        BEAST_EXPECT(isOffer (env, alice, XRP (222), USD (111)));
        {
            Json::Value cancelOffer;
            cancelOffer[jss::Account] = alice.human();
            cancelOffer[jss::OfferSequence] = offer4Seq;
            cancelOffer[jss::TransactionType] = "OfferCancel";
            env (cancelOffer);
        }
        env.close();
        BEAST_EXPECT(env.seq(alice) == offer4Seq + 2);

        BEAST_EXPECT(!isOffer (env, alice, XRP (222), USD (111)));

//创建一个同时使用TecExpired代码失败并删除
//要约表明取消报价的尝试失败。
        env.require (offers (alice, 2));

//FeatureDepositPreauthus更改过期报价的返回代码。
//适应这一点。
        bool const featPreauth {features[featureDepositPreauth]};
        env (offer (alice, XRP (5), USD (2)),
            json (sfExpiration.fieldName, lastClose(env)),
            json (jss::OfferSequence, offer2Seq),
            ter (featPreauth ? TER {tecEXPIRED} : TER {tesSUCCESS}));
        env.close();

        env.require (offers (alice, 2));
BEAST_EXPECT( isOffer (env, alice, XRP (300), USD (100))); //报价2
BEAST_EXPECT(!isOffer (env, alice, XRP   (5), USD   (2))); //期满
    }

    void testTinyPayment (FeatureBitset features)
    {
        testcase ("Tiny payments");

//小额支付的回归检验
        using namespace jtx;
        using namespace std::chrono_literals;
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const gw = Account {"gw"};

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        Env env {*this, features};

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob, carol);
        env.trust (EUR (1000), alice, bob, carol);
        env (pay (gw, alice, USD (100)));
        env (pay (gw, carol, EUR (100)));

//在delivernodereverse中创建比循环最大计数更多的产品
        for (int i=0;i<101;++i)
            env (offer (carol, USD (1), EUR (2)));

        auto hasFeature = [](Env& env, uint256 const& f)
        {
            return (env.app().config().features.find(f) !=
                env.app().config().features.end());
        };

        for (auto d : {-1, 1})
        {
            auto const closeTime = STAmountSO::soTime +
                d * env.closed()->info().closeTimeResolution;
            env.close (closeTime);
            *stAmountCalcSwitchover = closeTime > STAmountSO::soTime ||
                !hasFeature(env, fix1513);
//如果没有下溢修正，将失败
            TER const expectedResult = *stAmountCalcSwitchover ?
                TER {tesSUCCESS} : TER {tecPATH_PARTIAL};
            env (pay (alice, bob, EUR (epsilon)), path (~EUR),
                sendmax (USD (100)), ter (expectedResult));
        }
    }

    void testXRPTinyPayment (FeatureBitset features)
    {
        testcase ("XRP Tiny payments");

//小额xrp支付的回归测试
//在某些情况下，当付款代码计算
//输入xrp->iou报价所需的xrp金额
//它会错误地将金额四舍五入为零（即使在
//四舍五入设置为真）。
//这个错误会导致资助的提议被错误地删除。
//因为代码认为他们没有资金。
//触发错误的条件是：
//1）当我们计算报价所需的输入xrp数量时
//从xrp->iou，金额小于1滴（四舍五入后
//向上浮动表示）。
//2）同一本书中有另一个质量的报价。
//在计算输入量时
//需要，金额未设置为零。

        using namespace jtx;
        using namespace std::chrono_literals;
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const dan = Account {"dan"};
        auto const erin = Account {"erin"};
        auto const gw = Account {"gw"};

        auto const USD = gw["USD"];

        for (auto withFix : {false, true})
        {
            if (!withFix)
                continue;

            Env env {*this, features};

            auto closeTime = [&]
            {
                auto const delta =
                    100 * env.closed ()->info ().closeTimeResolution;
                if (withFix)
                    return STAmountSO::soTime2 + delta;
                else
                    return STAmountSO::soTime2 - delta;
            }();

            env.fund (XRP (10000), alice, bob, carol, dan, erin, gw);
            env.trust (USD (1000), alice, bob, carol, dan, erin);
            env (pay (gw, carol, USD (0.99999)));
            env (pay (gw, dan, USD (1)));
            env (pay (gw, erin, USD (1)));

//卡罗尔没有足够的资金来支付这个提议
//此报价后剩余的金额将导致
//当下一个报价时，stam等于错误地四舍五入为零。
//（质量好）考虑在内。（当
//StamCountCalcSwitchOver2补丁处于非活动状态）
            env (offer (carol, drops (1), USD (1)));
//提供足够低的质量，因此当输入xrp
//在逆向过程中计算，金额不是零。
            env (offer (dan, XRP (100), USD (1)));

            env.close (closeTime);
//这是将被错误删除的已资助的报价。
//这是在卡罗尔的提议之后考虑的，卡罗尔
//还剩很少的钱。当计算xrp的数量时
//此报价需要，它将错误地计算两者的零
//向前和向后通过（当StamCountCalcSwitchOver2
//是不活动的。）
            env (offer (erin, drops (1), USD (1)));

            {
                env (pay (alice, bob, USD (1)), path (~USD),
                    sendmax (XRP (102)),
                    txflags (tfNoRippleDirect | tfPartialPayment));

                env.require (
                    offers (carol, 0),
                    offers (dan, 1));
                if (!withFix)
                {
//资助的提议被取消
                    env.require (
                        balance (erin, USD (1)),
                        offers (erin, 0));
                }
                else
                {
//报价被正确消费。还有一些
//剩余的流动性。
                    env.require (
                        balance (erin, USD (0.99999)),
                        offers (erin, 1));
                }
            }
        }
    }

    void testEnforceNoRipple (FeatureBitset features)
    {
        testcase ("Enforce No Ripple");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];
        auto const EUR = gw["EUR"];
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const dan {"dan"};

        {
//报价后没有暗示账户步骤的波动
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const gw1 = Account {"gw1"};
            auto const USD1 = gw1["USD"];
            auto const gw2 = Account {"gw2"};
            auto const USD2 = gw2["USD"];

            env.fund (XRP (10000), alice, noripple (bob), carol, dan, gw1, gw2);
            env.trust (USD1 (1000), alice, carol, dan);
            env(trust (bob, USD1 (1000), tfSetNoRipple));
            env.trust (USD2 (1000), alice, carol, dan);
            env(trust (bob, USD2 (1000), tfSetNoRipple));

            env (pay (gw1, dan, USD1 (50)));
            env (pay (gw1, bob, USD1 (50)));
            env (pay (gw2, bob, USD2 (50)));

            env (offer (dan, XRP (50), USD1 (50)));

            env (pay (alice, carol, USD2 (50)), path (~USD1, bob),
                sendmax (XRP (50)), txflags (tfNoRippleDirect),
                ter(tecPATH_DRY));
        }
        {
//确保付款使用默认标志
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const gw1 = Account {"gw1"};
            auto const USD1 = gw1["USD"];
            auto const gw2 = Account {"gw2"};
            auto const USD2 = gw2["USD"];

            env.fund (XRP (10000), alice, bob, carol, dan, gw1, gw2);
            env.trust (USD1 (1000), alice, bob, carol, dan);
            env.trust (USD2 (1000), alice, bob, carol, dan);

            env (pay (gw1, dan, USD1 (50)));
            env (pay (gw1, bob, USD1 (50)));
            env (pay (gw2, bob, USD2 (50)));

            env (offer (dan, XRP (50), USD1 (50)));

            env (pay (alice, carol, USD2 (50)), path (~USD1, bob),
                sendmax (XRP (50)), txflags (tfNoRippleDirect));

            env.require (balance (alice, xrpMinusFee (env, 10000 - 50)));
            env.require (balance (bob, USD1 (100)));
            env.require (balance (bob, USD2 (0)));
            env.require (balance (carol, USD2 (50)));
        }
    }

    void
    testInsufficientReserve (FeatureBitset features)
    {
        testcase ("Insufficient Reserve");

//如果一个帐户发出一个报价和它的余额
//*在*之前，事务开始的时间不够高
//要在*事务运行后满足保留*，
//那就不应该有报价了，但是如果
//报价部分或全部通过Tx成功。

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

//无交叉：
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            env.fund (r + f, alice);

            env (trust (alice, usdOffer),           ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),         ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),
                ter(tecINSUF_RESERVE_OFFER));

            env.require (
                balance (alice, r - f),
                owners (alice, 1));
        }

//部分交叉：
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            auto const usdOffer2 = USD (500);
            auto const xrpOffer2 = XRP (500);

            env.fund (r + f + xrpOffer, bob);
            env (offer (bob, usdOffer2, xrpOffer2),   ter(tesSUCCESS));
            env.fund (r + f, alice);
            env (trust (alice, usdOffer),             ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),           ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),   ter(tesSUCCESS));

            env.require (
                balance (alice, r - f + xrpOffer2),
                balance (alice, usdOffer2),
                owners (alice, 1),
                balance (bob, r + xrpOffer2),
                balance (bob, usdOffer2),
                owners (bob, 1));
        }

//账户有足够的准备金，但还不够。
//如果增加了报价。试图把借据卖给
//购买XRP。如果一切顺利，我们就会成功。
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (XRP (1000000), gw);

            auto const f = env.current ()->fees ().base;
            auto const r = reserve (env, 0);

            auto const usdOffer2 = USD (500);
            auto const xrpOffer2 = XRP (500);

            env.fund (r + f + xrpOffer, bob, carol);
            env (offer (bob, usdOffer2, xrpOffer2),    ter(tesSUCCESS));
            env (offer (carol, usdOffer, xrpOffer),    ter(tesSUCCESS));

            env.fund (r + f, alice);
            env (trust (alice, usdOffer),              ter(tesSUCCESS));
            env (pay (gw, alice, usdOffer),            ter(tesSUCCESS));
            env (offer (alice, xrpOffer, usdOffer),    ter(tesSUCCESS));

            env.require (
                balance (alice, r - f + xrpOffer),
                balance (alice, USD(0)),
                owners (alice, 1),
                balance (bob, r + xrpOffer2),
                balance (bob, usdOffer2),
                owners (bob, 1),
                balance (carol, r + xrpOffer2),
                balance (carol, usdOffer2),
                owners (carol, 2));
        }
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
    testFillModes (FeatureBitset features)
    {
        testcase ("Fill Modes");

        using namespace jtx;

        auto const startBalance = XRP (1000000);
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

//填充或杀死-除非我们完全交叉，只收取费用，不要
//把报价写在书上。同时清理过期的报价
//一路上发现的。
//
//FIX1578更改返回代码。验证预期行为
//不带和带FIX1578。
        for (auto const tweakedFeatures :
            {features - fix1578, features | fix1578})
        {
            Env env {*this, tweakedFeatures};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const f = env.current ()->fees ().base;

            env.fund (startBalance, gw, alice, bob);

//Bob创建的报价将在下一个分类帐结算之前过期。
            env (offer (bob, USD (500), XRP (500)),
                json (sfExpiration.fieldName, lastClose(env) + 1),
                ter(tesSUCCESS));

//报价到期（尚未删除）。
            env.close ();
            env.require (
                owners (bob, 1),
                offers (bob, 1));

//Bob创建了将被交叉的报价。
            env (offer (bob, USD (500), XRP (500)), ter(tesSUCCESS));
            env.close();
            env.require (
                owners (bob, 2),
                offers (bob, 2));

            env (trust (alice, USD (1000)),         ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),       ter(tesSUCCESS));

//无法填写但将删除Bob过期报价的订单：
            {
                TER const killedCode {tweakedFeatures[fix1578] ?
                    TER {tecKILLED} : TER {tesSUCCESS}};
                env (offer (alice, XRP (1000), USD (1000)),
                    txflags (tfFillOrKill),         ter(killedCode));
            }
            env.require (
                balance (alice, startBalance - (f * 2)),
                balance (alice, USD (1000)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - (f * 2)),
                balance (bob, USD (none)),
                owners (bob, 1),
                offers (bob, 1));

//可以填写的订单
            env (offer (alice, XRP (500), USD (500)),
                txflags (tfFillOrKill),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - (f * 3) + XRP (500)),
                balance (alice, USD (500)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - (f * 2) - XRP (500)),
                balance (bob, USD (500)),
                owners (bob, 1),
                offers (bob, 0));
        }

//立即或取消-尽可能交叉
//在书上什么都不加：
        {
            Env env {*this, features};
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            auto const f = env.current ()->fees ().base;

            env.fund (startBalance, gw, alice, bob);

            env (trust (alice, USD (1000)),                 ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),               ter(tesSUCCESS));

//没有十字架：
            env (offer (alice, XRP (1000), USD (1000)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f),
                balance (alice, USD (1000)),
                owners (alice, 1),
                offers (alice, 0));

//部分交叉：
            env (offer (bob, USD (50), XRP (50)),           ter(tesSUCCESS));
            env (offer (alice, XRP (1000), USD (1000)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f - f + XRP (50)),
                balance (alice, USD (950)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - f - XRP (50)),
                balance (bob, USD (50)),
                owners (bob, 1),
                offers (bob, 0));

//完全交叉：
            env (offer (bob, USD (50), XRP (50)),           ter(tesSUCCESS));
            env (offer (alice, XRP (50), USD (50)),
                txflags (tfImmediateOrCancel),              ter(tesSUCCESS));

            env.require (
                balance (alice, startBalance - f - f - f - f + XRP (100)),
                balance (alice, USD (900)),
                owners (alice, 1),
                offers (alice, 0),
                balance (bob, startBalance - f - f - XRP (100)),
                balance (bob, USD (100)),
                owners (bob, 1),
                offers (bob, 0));
        }

//tf被动——在报价时不要越界。
        {
            Env env (*this, features);
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (startBalance, gw, alice, bob);
            env.close();

            env (trust (bob, USD(1000)));
            env.close();

            env (pay (gw, bob, USD(1000)));
            env.close();

            env (offer (alice, USD(1000), XRP(2000)));
            env.close();

            auto const aliceOffers = offersOnAccount (env, alice);
            BEAST_EXPECT (aliceOffers.size() == 1);
            for (auto offerPtr : aliceOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == XRP (2000));
                BEAST_EXPECT (offer[sfTakerPays] == USD (1000));
            }

//鲍勃创造了一个被动的提议，可以跨越爱丽丝的。
//鲍勃的提议应该留在账本上。
            env (offer (bob, XRP(2000), USD(1000), tfPassive));
            env.close();
            env.require (offers (alice, 1));

            auto const bobOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobOffers.size() == 1);
            for (auto offerPtr : bobOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == USD (1000));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (2000));
            }

//吉布达•伟士应该有可能通过这两个报价。
            env (offer (gw, XRP(2000), USD(1000)));
            env.close();
            env.require (offers (alice, 0));
            env.require (offers (gw, 0));
            env.require (offers (bob, 1));

            env (offer (gw, USD(1000), XRP(2000)));
            env.close();
            env.require (offers (bob, 0));
            env.require (offers (gw, 0));
        }

//tfpassive——Cross只提供更好的质量。
        {
            Env env (*this, features);
            auto const closeTime =
                fix1449Time () +
                    100 * env.closed ()->info ().closeTimeResolution;
            env.close (closeTime);

            env.fund (startBalance, gw, "alice", "bob");
            env.close();

            env (trust ("bob", USD(1000)));
            env.close();

            env (pay (gw, "bob", USD(1000)));
            env (offer ("alice", USD(500), XRP(1001)));
            env.close();

            env (offer ("alice", USD(500), XRP(1000)));
            env.close();

            auto const aliceOffers = offersOnAccount (env, "alice");
            BEAST_EXPECT (aliceOffers.size() == 2);

//鲍勃提出了一个被动的提议。这个提议应该与之交叉
//爱丽丝的（质量更好的）和离开爱丽丝的
//其他报价未受影响。
            env (offer ("bob", XRP(2000), USD(1000), tfPassive));
            env.close();
            env.require (offers ("alice", 1));

            auto const bobOffers = offersOnAccount (env, "bob");
            BEAST_EXPECT (bobOffers.size() == 1);
            for (auto offerPtr : bobOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfTakerGets] == USD (499.5));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (999));
            }
        }
    }

    void
    testMalformed(FeatureBitset features)
    {
        testcase ("Malformed Detection");

        using namespace jtx;

        auto const startBalance = XRP(1000000);
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (startBalance, gw, alice);

//具有无效标志的顺序
        env (offer (alice, USD (1000), XRP (1000)),
            txflags (tfImmediateOrCancel + 1),            ter(temINVALID_FLAG));
        env.require (
            balance (alice, startBalance),
            owners (alice, 0),
            offers (alice, 0));

//带有不兼容标志的顺序
        env (offer (alice, USD (1000), XRP (1000)),
            txflags (tfImmediateOrCancel | tfFillOrKill), ter(temINVALID_FLAG));
        env.require (
            balance (alice, startBalance),
            owners (alice, 0),
            offers (alice, 0));

//出售和购买同一资产
        {
//Alice尝试从xrp到xrp的顺序：
            env (offer (alice, XRP (1000), XRP (1000)),   ter(temBAD_OFFER));
            env.require (
                owners (alice, 0),
                offers (alice, 0));

//Alice尝试借据到借据顺序：
            env (trust (alice, USD (1000)),               ter(tesSUCCESS));
            env (pay (gw, alice, USD (1000)),             ter(tesSUCCESS));
            env (offer (alice, USD (1000), USD (1000)),   ter(temREDUNDANT));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

//报价金额为负数
        {
            env (offer (alice, -USD (1000), XRP (1000)),  ter(temBAD_OFFER));
            env.require (
                owners (alice, 1),
                offers (alice, 0));

            env (offer (alice, USD (1000), -XRP (1000)),  ter(temBAD_OFFER));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

//有效期不好的报价
        {
            env (offer (alice, USD (1000), XRP (1000)),
                json (sfExpiration.fieldName, std::uint32_t (0)),
                ter(temBAD_EXPIRATION));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

//报价顺序错误
        {
            env (offer (alice, USD (1000), XRP (1000)),
                json (jss::OfferSequence, std::uint32_t (0)),
                ter(temBAD_SEQUENCE));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }

//使用XRP作为货币代码
        {
            auto const BAD = IOU(gw, badCurrency());

            env (offer (alice, XRP (1000), BAD (1000)),   ter(temBAD_CURRENCY));
            env.require (
                owners (alice, 1),
                offers (alice, 0));
        }
    }

    void
    testExpiration(FeatureBitset features)
    {
        testcase ("Offer Expiration");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const startBalance = XRP (1000000);
        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (startBalance, gw, alice, bob);
        env.close();

        auto const f = env.current ()->fees ().base;

        env (trust (alice, usdOffer),             ter(tesSUCCESS));
        env (pay (gw, alice, usdOffer),           ter(tesSUCCESS));
        env.close();
        env.require (
            balance (alice, startBalance - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1));

//提出一个本应过期的报价。
//DepositPrauth修正案更改了返回代码；对此进行了调整。
        bool const featPreauth {features[featureDepositPreauth]};

        env (offer (alice, xrpOffer, usdOffer),
            json (sfExpiration.fieldName, lastClose(env)),
            ter (featPreauth ? TER {tecEXPIRED} : TER {tesSUCCESS}));

        env.require (
            balance (alice, startBalance - f - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1));
        env.close();

//添加在下一个分类帐关闭之前过期的报价
        env (offer (alice, xrpOffer, usdOffer),
            json (sfExpiration.fieldName, lastClose(env) + 1),
            ter(tesSUCCESS));
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 1),
            owners (alice, 2));

//报价到期（尚未删除）
        env.close ();
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 1),
            owners (alice, 2));

//添加优惠-已过期的优惠已删除
        env (offer (bob, usdOffer, xrpOffer),     ter(tesSUCCESS));
        env.require (
            balance (alice, startBalance - f - f - f),
            balance (alice, usdOffer),
            offers (alice, 0),
            owners (alice, 1),
            balance (bob, startBalance - f),
            balance (bob, USD (none)),
            offers (bob, 1),
            owners (bob, 1));
    }

    void
    testUnfundedCross(FeatureBitset features)
    {
        testcase ("Unfunded Crossing");

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];

        auto const usdOffer = USD (1000);
        auto const xrpOffer = XRP (1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw);

//为交易收取的费用
        auto const f = env.current ()->fees ().base;

//账户在准备金中，将下降一次。
//扣除费用。
        env.fund (reserve (env, 0), "alice");
        env (offer ("alice", usdOffer, xrpOffer),     ter(tecUNFUNDED_OFFER));
        env.require (
            balance ("alice", reserve (env, 0) - f),
            owners ("alice", 0));

//帐户只够支付储备金和
//费用。
        env.fund (reserve (env, 0) + f, "bob");
        env (offer ("bob", usdOffer, xrpOffer),       ter(tecUNFUNDED_OFFER));
        env.require (
            balance ("bob", reserve (env, 0)),
            owners ("bob", 0));

//账户有足够的储备、费用和
//报价，还有一点，但还不够
//报价后保留。
        env.fund (reserve (env, 0) + f + XRP(1), "carol");
        env (offer ("carol", usdOffer, xrpOffer), ter(tecINSUF_RESERVE_OFFER));
        env.require (
            balance ("carol", reserve (env, 0) + XRP(1)),
            owners ("carol", 0));

//帐户有足够的储备加上一个
//报价和费用。
        env.fund (reserve (env, 1) + f, "dan");
        env (offer ("dan", usdOffer, xrpOffer),       ter(tesSUCCESS));
        env.require (
            balance ("dan", reserve (env, 1)),
            owners ("dan", 1));

//帐户有足够的储备加上一个
//报价、费用和全部报价金额。
        env.fund (reserve (env, 1) + f + xrpOffer, "eve");
        env (offer ("eve", usdOffer, xrpOffer),       ter(tesSUCCESS));
        env.require (
            balance ("eve", reserve (env, 1) + xrpOffer),
            owners ("eve", 1));
    }

    void
    testSelfCross(bool use_partner, FeatureBitset features)
    {
        testcase (std::string("Self-crossing") +
            (use_partner ? ", with partner account" : ""));

        using namespace jtx;

        auto const gw = Account {"gateway"};
        auto const partner = Account {"partner"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP (10000), gw);
        if (use_partner)
        {
            env.fund (XRP (10000), partner);
            env (trust (partner, USD (100)));
            env (trust (partner, BTC (500)));
            env (pay (gw, partner, USD(100)));
            env (pay (gw, partner, BTC(500)));
        }
        auto const& account_to_test = use_partner ? partner : gw;

        env.close();
        env.require (offers (account_to_test, 0));

//第1部分：
//我们将提供两个报价，用于将BTC连接到美元。
//通过XRP
        env (offer (account_to_test, BTC (250), XRP (1000)),
                 offers (account_to_test, 1));

//验证书籍现在显示xrp优惠的BTC
        BEAST_EXPECT(isOffer(env, account_to_test, BTC(250), XRP(1000)));

        auto const secondLegSeq = env.seq(account_to_test);
        env (offer (account_to_test, XRP(1000), USD (50)),
                 offers (account_to_test, 2));

//验证该书是否还显示了美元报价的xrp
        BEAST_EXPECT(isOffer(env, account_to_test, XRP(1000), USD(50)));

//现在提出一个可以跨越汽车桥的报价，意思是
//未付的报价将被接受，我们将一无所获。
        env (offer (account_to_test, USD (50), BTC (250)));

        auto jrr = getBookOffers(env, USD, BTC);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        jrr = getBookOffers(env, BTC, XRP);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

//注：
//此时，所有报价都将被消耗掉。
//唉，他们不是-因为一个窃听器自动桥接的错误
//由FixTakerDryOfferRemoval解决的实现。
//Pre-FixTaker DryOfferRemove实现（不正确）离开
//在桥的第二段空报价。验证两个
//旧的和新的行为。
        {
            auto acctOffers = offersOnAccount (env, account_to_test);
            bool const noStaleOffers {
                features[featureFlowCross] ||
                features[fixTakerDryOfferRemoval]};

            BEAST_EXPECT(acctOffers.size() == (noStaleOffers ? 0 : 1));
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (offer[sfTakerGets] == USD (0));
                BEAST_EXPECT (offer[sfTakerPays] == XRP (0));
            }
        }

//取消延迟的第二次报价，这样它就不会干扰
//我们将测试下一组报价。不需要一次
//桥接错误已修复
        Json::Value cancelOffer;
        cancelOffer[jss::Account] = account_to_test.human();
        cancelOffer[jss::OfferSequence] = secondLegSeq;
        cancelOffer[jss::TransactionType] = "OfferCancel";
        env (cancelOffer);
        env.require (offers (account_to_test, 0));

//第2部分：
//简单的直接跨越BTC到美元，然后美元到BTC，这导致
//第一个要替换的报价
        env (offer (account_to_test, BTC (250), USD (50)),
                 offers (account_to_test, 1));

//验证书籍显示一个BTC（美元报价），没有美元（美元报价）
//BTC报价
        BEAST_EXPECT(isOffer(env, account_to_test, BTC(250), USD(50)));

        jrr = getBookOffers(env, USD, BTC);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

//第二个报价会直接自交叉，因此它会导致第一个报价
//由同一所有人/接受者提出的要移除的报价
        env (offer (account_to_test, USD (50), BTC (250)),
                 offers (account_to_test, 1));

//确认我们现在只有第二个报价…第一个
//被移除
        jrr = getBookOffers(env, BTC, USD);
        BEAST_EXPECT(jrr[jss::offers].isArray());
        BEAST_EXPECT(jrr[jss::offers].size() == 0);

        BEAST_EXPECT(isOffer(env, account_to_test, USD(50), BTC(250)));
    }

    void
    testNegativeBalance(FeatureBitset features)
    {
//此测试创建负余额的报价测试
//转账费和小额资金。
        testcase ("Negative Balance");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];
        auto const BTC = gw["BTC"];

//这些*有趣的*数量
//从这里移植的原始JS测试
        auto const gw_initial_balance = drops (1149999730);
        auto const alice_initial_balance = drops (499946999680);
        auto const bob_initial_balance = drops (10199999920);
        auto const small_amount =
            STAmount { bob["USD"].issue(), UINT64_C(2710505431213761), -33};

        env.fund (gw_initial_balance, gw);
        env.fund (alice_initial_balance, alice);
        env.fund (bob_initial_balance, bob);

        env (rate (gw, 1.005));

        env (trust (alice, USD (500)));
        env (trust (bob, USD (50)));
        env (trust (gw, alice["USD"] (100)));

        env (pay (gw, alice, alice["USD"] (50)));
        env (pay (gw, bob, small_amount));

        env (offer (alice, USD (50), XRP (150000)));

//取消出价
        env (pay (alice, gw, USD (100)));

//删除信任行（设置为0）
        env (trust (gw, alice["USD"] (0)));

//核实余额
        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "50");

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName][jss::value] ==
                "-2710505431213761e-33");

//创建交叉报价
        env (offer (bob, XRP (2000), USD (1)));

//再次核实余额。
//
//注：
//这是我们两个报价交叉点的舍入方式的不同之处
//算法变得显而易见。旧的出价交叉会消耗
//金额小，转账号为XRP。新报价交叉转让
//一滴，而不是一滴。
        auto const crossingDelta = features[featureFlowCross] ? drops (1) : drops (0);

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "50");
        BEAST_EXPECT(env.balance (alice, xrpIssue()) ==
            alice_initial_balance -
                env.current ()->fees ().base * 3 - crossingDelta
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT( jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        BEAST_EXPECT(env.balance (bob, xrpIssue()) ==
            bob_initial_balance -
                env.current ()->fees ().base * 2 + crossingDelta
        );
    }

    void
    testOfferCrossWithXRP(bool reverse_order, FeatureBitset features)
    {
        testcase (std::string("Offer Crossing with XRP, ") +
                (reverse_order ? "Reverse" : "Normal") +
                " order");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));

        env (pay (gw, alice, alice["USD"] (500)));

        if (reverse_order)
            env (offer (bob, USD (1), XRP (4000)));

        env (offer (alice, XRP (150000), USD (50)));

        if (! reverse_order)
            env (offer (bob, USD (1), XRP (4000)));

//现有的报价比这更划算。
//充分利用现有报价。
//支付1美元，获得4000瑞波币。

        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-1");
        jrr = ledgerEntryRoot (env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () -
                XRP (reverse_order ? 4000 : 3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-499");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa( ) +
                XRP(reverse_order ? 4000 : 3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );
    }

    void
    testOfferCrossWithLimitOverride(FeatureBitset features)
    {
        testcase ("Offer Crossing with Limit Override");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (100000), gw, alice, bob);

        env (trust (alice, USD (1000)));

        env (pay (gw, alice, alice["USD"] (500)));

        env (offer (alice, XRP (150000), USD (50)));
        env (offer (bob, USD (1), XRP (3000)));

        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-1");
        jrr = ledgerEntryRoot(env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (100000).value ().mantissa () -
                XRP (3000).value ().mantissa () -
                env.current ()->fees ().base * 1)
        );

        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-499");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (100000).value ().mantissa () +
                XRP (3000).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );
    }

    void
    testOfferAcceptThenCancel(FeatureBitset features)
    {
        testcase ("Offer Accept then Cancel.");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const USD = env.master["USD"];

        auto const nextOfferSeq = env.seq (env.master);
        env (offer (env.master, XRP (500), USD (100)));
        env.close ();

        Json::Value cancelOffer;
        cancelOffer[jss::Account] = env.master.human();
        cancelOffer[jss::OfferSequence] = nextOfferSeq;
        cancelOffer[jss::TransactionType] = "OfferCancel";
        env (cancelOffer);
        BEAST_EXPECT(env.seq (env.master) == nextOfferSeq + 2);

//分类帐接受，打两次电话确认没有异常行为
        env.close();
        env.close();
        BEAST_EXPECT(env.seq (env.master) == nextOfferSeq + 2);
    }

    void
    testOfferCancelPastAndFuture(FeatureBitset features)
    {

        testcase ("Offer Cancel Past and Future Sequence.");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account {"alice"};

        auto const nextOfferSeq = env.seq (env.master);
        env.fund (XRP (10000), alice);

        Json::Value cancelOffer;
        cancelOffer[jss::Account] = env.master.human();
        cancelOffer[jss::OfferSequence] = nextOfferSeq;
        cancelOffer[jss::TransactionType] = "OfferCancel";
        env (cancelOffer);

        cancelOffer[jss::OfferSequence] = env.seq (env.master);
        env (cancelOffer, ter(temBAD_SEQUENCE));

        cancelOffer[jss::OfferSequence] = env.seq (env.master) + 1;
        env (cancelOffer, ter(temBAD_SEQUENCE));

        env.close();
        env.close();
    }

    void
    testCurrencyConversionEntire(FeatureBitset features)
    {
        testcase ("Currency Conversion: Entire Offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);
        env.require (owners (bob, 0));

        env (trust (alice, USD (100)));
        env (trust (bob, USD (1000)));

        env.require (
            owners (alice, 1),
            owners (bob, 1));

        env (pay (gw, alice, alice["USD"] (100)));
        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, USD (100), XRP (500)));

        env.require (
            owners (alice, 1),
            owners (bob, 2));
        auto jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (500).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == USD (100).value ().getJson (0));

        env (pay (alice, alice, XRP (500)), sendmax (USD (100)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "0");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (500).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");

        jro = ledgerEntryOffer(env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");

        env.require (
            owners (alice, 1),
            owners (bob, 1));
    }

    void
    testCurrencyConversionIntoDebt(FeatureBitset features)
    {
        testcase ("Currency Conversion: Offerer Into Debt");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};

        env.fund (XRP (10000), alice, bob, carol);

        env (trust (alice, carol["EUR"] (2000)));
        env (trust (bob, alice["USD"] (100)));
        env (trust (carol, bob["EUR"] (1000)));

        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, alice["USD"] (50), carol["EUR"] (200)),
            ter(tecUNFUNDED_OFFER));

        env (offer (alice, carol["EUR"] (200), alice["USD"] (50)));

        auto jro = ledgerEntryOffer(env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");
    }

    void
    testCurrencyConversionInParts(FeatureBitset features)
    {
        testcase ("Currency Conversion: In Parts");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (trust (alice, USD (200)));
        env (trust (bob, USD (1000)));

        env (pay (gw, alice, alice["USD"] (200)));

        auto const bobOfferSeq = env.seq (bob);
        env (offer (bob, USD (100), XRP (500)));

        env (pay (alice, alice, XRP (200)), sendmax (USD (100)));

//上一笔付款将剩余报价金额减少了200瑞波币。
        auto jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (300).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == USD (60).value ().getJson (0));

//Alice和GW之间的余额为160美元，减去40美元
//根据要约
        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-160");
//Alice现在有200多个XRP
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (200).value ().mantissa () -
                env.current ()->fees ().base * 2)
        );

//鲍勃从该报盘的部分消费中得到40美元。
        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-40");

//Alice将美元转换为应失败的XRP
//由于分期付款。
        env (pay (alice, alice, XRP (600)), sendmax (USD (100)),
            ter(tecPATH_PARTIAL));

//Alice将美元转换为XRP，应该会成功，因为
//我们允许部分付款
        env (pay (alice, alice, XRP (600)), sendmax (USD (100)),
            txflags (tfPartialPayment));

//
        jro = ledgerEntryOffer (env, bob, bobOfferSeq);
        BEAST_EXPECT(jro[jss::error] == "entryNotFound");

//核实余额在部分付款后立即查看
//只需支付300瑞波币就行了
//这仍然是鲍勃的提议。爱丽丝天平现在是
//100美元，因为第二天又有60美元转给了鲍勃。
//付款
        jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (200).value ().mantissa () +
                XRP (300).value ().mantissa () -
                env.current ()->fees ().base * 4)
        );

//Bob现在从第一笔付款中获得100美元-40美元，从
//第二次（部分）付款
        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
    }

    void
    testCrossCurrencyStartXRP(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: Start with XRP");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob, carol);

        env (trust (carol, USD (1000)));
        env (trust (bob, USD (2000)));

        env (pay (gw, carol, carol["USD"] (500)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, XRP (500), USD (50)));

        env (pay (alice, bob, USD (25)), sendmax (XRP (333)));

        auto jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-25");

        jrr = ledgerEntryState (env, carol, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-475");

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == USD (25).value ().getJson (0));
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == XRP (250).value ().getText ());
    }

    void
    testCrossCurrencyEndXRP(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: End with XRP");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob, carol);

        env (trust (alice, USD (1000)));
        env (trust (carol, USD (2000)));

        env (pay (gw, alice, alice["USD"] (500)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, USD (50), XRP (500)));

        env (pay (alice, bob, XRP (250)), sendmax (USD (333)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-475");

        jrr = ledgerEntryState (env, carol, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-25");

        jrr = ledgerEntryRoot (env, bob);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] ==
            std::to_string(
                XRP (10000).value ().mantissa () +
                XRP (250).value ().mantissa ())
        );

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (250).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == USD (25).value ().getJson (0));
    }

    void
    testCrossCurrencyBridged(FeatureBitset features)
    {
        testcase ("Cross Currency Payment: Bridged");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account {"gateway_1"};
        auto const gw2 = Account {"gateway_2"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const carol = Account {"carol"};
        auto const dan = Account {"dan"};
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        env.fund (XRP (10000), gw1, gw2, alice, bob, carol, dan);

        env (trust (alice, USD (1000)));
        env (trust (bob, EUR (1000)));
        env (trust (carol, USD (1000)));
        env (trust (dan, EUR (1000)));

        env (pay (gw1, alice, alice["USD"] (500)));
        env (pay (gw2, dan, dan["EUR"] (400)));

        auto const carolOfferSeq = env.seq (carol);
        env (offer (carol, USD (50), XRP (500)));

        auto const danOfferSeq = env.seq (dan);
        env (offer (dan, XRP (500), EUR (50)));

        Json::Value jtp {Json::arrayValue};
        jtp[0u][0u][jss::currency] = "XRP";
        env (pay (alice, bob, EUR (30)),
            json (jss::Paths, jtp),
            sendmax (USD (333)));

        auto jrr = ledgerEntryState (env, alice, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "470");

        jrr = ledgerEntryState (env, bob, gw2, "EUR");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-30");

        jrr = ledgerEntryState (env, carol, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-30");

        jrr = ledgerEntryState (env, dan, gw2, "EUR");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-370");

        auto jro = ledgerEntryOffer (env, carol, carolOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] == XRP (200).value ().getText ());
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == USD (20).value ().getJson (0));

        jro = ledgerEntryOffer (env, dan, danOfferSeq);
        BEAST_EXPECT(
            jro[jss::node][jss::TakerGets] ==
            gw2["EUR"] (20).value ().getJson (0));
        BEAST_EXPECT(
            jro[jss::node][jss::TakerPays] == XRP (200).value ().getText ());
    }

    void
    testBridgedSecondLegDry(FeatureBitset features)
    {
//至少在Taker桥接中，如果
//第二条腿比第一条腿干。这个测试练习
//案例。
        testcase ("Auto Bridged Second Leg Dry");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const gw {"gateway"};
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        env.fund(XRP(100000000), alice, bob, carol, gw);

        env.trust(USD(10), alice);
        env.close();
        env(pay(gw, alice, USD(10)));
        env.trust(USD(10), carol);
        env.close();
        env(pay(gw, carol, USD(3)));

        env (offer (alice, EUR(2), XRP(1)));
        env (offer (alice, EUR(2), XRP(1)));

        env (offer (alice, XRP(1), USD(4)));
        env (offer (carol, XRP(1), USD(3)));
        env.close();

//Bob提议以10欧元的价格购买10美元。
//1。他花了2欧元接受爱丽丝的汽车搭桥优惠
//得到4美元。
//2。他又花了2欧元购买了Alice的最后一个欧元->xrp优惠和
//卡罗尔的xrp-usd报价。他为此得到3美元。
//这个测试的关键是Alice的xrp->usd leg在
//爱丽丝的欧元->xrp。xrp->usd段是第二段，显示
//有些敏感。
        env.trust(EUR(10), bob);
        env.close();
        env(pay(gw, bob, EUR(10)));
        env.close();
        env(offer(bob, USD(10), EUR(10)));
        env.close();

        env.require (balance(bob, USD(7)));
        env.require (balance(bob, EUR(6)));
        env.require (offers (bob, 1));
        env.require (owners (bob, 3));

        env.require (balance(alice, USD(6)));
        env.require (balance(alice, EUR(4)));
        env.require (offers(alice, 0));
        env.require (owners(alice, 2));

        env.require (balance(carol, USD(0)));
        env.require (balance(carol, EUR(none)));
//如果未定义FeatureFlowCross和Fixaker DryOfferRemoval
//卡罗尔的提议将被记录在案，但价值为零。
        int const emptyOfferCount {
            features[featureFlowCross] ||
            features[fixTakerDryOfferRemoval] ? 0 : 1};

        env.require (offers(carol, 0 + emptyOfferCount));
        env.require (owners(carol, 1 + emptyOfferCount));
    }

    void
    testOfferFeesConsumeFunds(FeatureBitset features)
    {
        testcase ("Offer Fees Consume Funds");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account {"gateway_1"};
        auto const gw2 = Account {"gateway_2"};
        auto const gw3 = Account {"gateway_3"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD1 = gw1["USD"];
        auto const USD2 = gw2["USD"];
        auto const USD3 = gw3["USD"];

//提供小额金额以补偿费用，使结果更圆。
//很好。
//准备金：Alice通过信托行在分类账中有3个条目
//费用：
//每个信任限制为1==3（alice<mtgox/amazon/bitstamp）+
//付款1=4
        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (3) +
            env.current ()->fees ().base * 4;

        env.fund (starting_xrp, gw1, gw2, gw3, alice, bob);

        env (trust (alice, USD1 (1000)));
        env (trust (alice, USD2 (1000)));
        env (trust (alice, USD3 (1000)));
        env (trust (bob, USD1 (1000)));
        env (trust (bob, USD2 (1000)));

        env (pay (gw1, bob, bob["USD"] (500)));

        env (offer (bob, XRP (200), USD1 (200)));
//Alice有350笔费用-50=250的准备金=100可用。
//要求提供足够的证据证明储备工程。
        env (offer (alice, USD1 (200), XRP (200)));

        auto jrr = ledgerEntryState (env, alice, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (350).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw1, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-400");
    }

    void
    testOfferCreateThenCross(FeatureBitset features)
    {
        testcase ("Offer Create, then Cross");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        env.fund (XRP (10000), gw, alice, bob);

        env (rate (gw, 1.005));

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));
        env (trust (gw, alice["USD"] (50)));

        env (pay (gw, bob, bob["USD"] (1)));
        env (pay (alice, gw, USD (50)));

        env (trust (gw, alice["USD"] (0)));

        env (offer (alice, USD (50), XRP (150000)));
        env (offer (bob, XRP (100), USD (0.1)));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "49.96666666666667");
        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-0.966500000033334");
    }

    void
    testSellFlagBasic(FeatureBitset features)
    {
        testcase ("Offer tfSell: Basic Sell");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, USD (1000)));
        env (trust (bob, USD (1000)));

        env (pay (gw, bob, bob["USD"] (500)));

        env (offer (bob, XRP (200), USD (200)), json(jss::Flags, tfSell));
//Alice有350+笔费用-50=250的准备金=100可用。
//Alice有350+笔费用-50=250的准备金=100可用。
//要求提供足够的证据证明储备工程。
        env (offer (alice, USD (200), XRP (200)), json(jss::Flags, tfSell));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-100");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (250).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-400");
    }

    void
    testSellFlagExceedLimit(FeatureBitset features)
    {
        testcase ("Offer tfSell: 2x Sell Exceed Limit");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const USD = gw["USD"];

        auto const starting_xrp = XRP (100) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, USD (150)));
        env (trust (bob, USD (1000)));

        env (pay (gw, bob, bob["USD"] (500)));

        env (offer (bob, XRP (100), USD (200)));
//Alice有350笔费用-50=250的准备金=100可用。
//要求提供足够的证据证明储备工程。
//承购方支付100美元100瑞波币。
//销售XRP。
//将出售所有100瑞波币，获得超过要求的美元。
        env (offer (alice, USD (100), XRP (100)), json(jss::Flags, tfSell));

        auto jrr = ledgerEntryState (env, alice, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-200");
        jrr = ledgerEntryRoot (env, alice);
        BEAST_EXPECT(
            jrr[jss::node][sfBalance.fieldName] == XRP (250).value ().getText ()
        );

        jrr = ledgerEntryState (env, bob, gw, "USD");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-300");
    }

    void
    testGatewayCrossCurrency(FeatureBitset features)
    {
        testcase ("Client Issue #535: Gateway Cross Currency");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        auto const XTS = gw["XTS"];
        auto const XXX = gw["XXX"];

        auto const starting_xrp = XRP (100.1) +
            env.current ()->fees ().accountReserve (1) +
            env.current ()->fees ().base * 2;

        env.fund (starting_xrp, gw, alice, bob);

        env (trust (alice, XTS (1000)));
        env (trust (alice, XXX (1000)));
        env (trust (bob, XTS (1000)));
        env (trust (bob, XXX (1000)));

        env (pay (gw, alice, alice["XTS"] (100)));
        env (pay (gw, alice, alice["XXX"] (100)));
        env (pay (gw, bob, bob["XTS"] (100)));
        env (pay (gw, bob, bob["XXX"] (100)));

        env (offer (alice, XTS (100), XXX (100)));

//这里使用了WS-Client，因为RPC客户端无法
//确信通过构建路径论证
        auto wsc = makeWSClient(env.app().config());
        Json::Value payment;
        payment[jss::secret] = toBase58(generateSeed("bob"));
        payment[jss::id] = env.seq (bob);
        payment[jss::build_path] = true;
        payment[jss::tx_json] = pay (bob, bob, bob["XXX"] (1));
        payment[jss::tx_json][jss::Sequence] =
            env.current ()->read (
                keylet::account (bob.id ()))->getFieldU32 (sfSequence);
        payment[jss::tx_json][jss::Fee] =
            std::to_string( env.current ()->fees ().base);
        payment[jss::tx_json][jss::SendMax] =
            bob ["XTS"] (1.5).value ().getJson (0);
        auto jrr = wsc->invoke("submit", payment);
        BEAST_EXPECT(jrr[jss::status] == "success");
        BEAST_EXPECT(jrr[jss::result][jss::engine_result] == "tesSUCCESS");
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jrr.isMember(jss::jsonrpc) && jrr[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jrr.isMember(jss::ripplerpc) && jrr[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jrr.isMember(jss::id) && jrr[jss::id] == 5);
        }

        jrr = ledgerEntryState (env, alice, gw, "XTS");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-101");
        jrr = ledgerEntryState (env, alice, gw, "XXX");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-99");

        jrr = ledgerEntryState (env, bob, gw, "XTS");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-99");
        jrr = ledgerEntryState (env, bob, gw, "XXX");
        BEAST_EXPECT(jrr[jss::node][sfBalance.fieldName][jss::value] == "-101");
    }

//验证*默认*信任线的helper函数。如果
//信任线不是默认的，那么测试将不会通过。
    void
    verifyDefaultTrustline (jtx::Env& env,
        jtx::Account const& account, jtx::PrettyAmount const& expectBalance)
    {
        auto const sleTrust =
            env.le (keylet::line(account.id(), expectBalance.value().issue()));
        BEAST_EXPECT (sleTrust);
        if (sleTrust)
        {
            Issue const issue = expectBalance.value().issue();
            bool const accountLow = account.id() < issue.account;

            STAmount low {issue};
            STAmount high {issue};

            low.setIssuer (accountLow ? account.id() : issue.account);
            high.setIssuer (accountLow ? issue.account : account.id());

            BEAST_EXPECT (sleTrust->getFieldAmount (sfLowLimit) == low);
            BEAST_EXPECT (sleTrust->getFieldAmount (sfHighLimit) == high);

            STAmount actualBalance {sleTrust->getFieldAmount (sfBalance)};
            if (! accountLow)
                actualBalance.negate();

            BEAST_EXPECT (actualBalance == expectBalance);
        }
    }

    void testPartialCross (FeatureBitset features)
    {
//测试有关添加
//可能是交叉报价。测试是表格
//驱动，所以应该很容易添加或删除测试。
        testcase ("Partial Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw);

//为交易收取的费用
        auto const f = env.current ()->fees ().base;

//为了简单起见，所有报价都是1:1的xrp:usd。
        enum preTrustType {noPreTrust, gwPreTrust, acctPreTrust};
        struct TestData
        {
std::string account;       //开户日期
STAmount fundXrp;          //帐户资金来源
int bookAmount;            //美元->账面XRP报价
preTrustType preTrust;     //如果为真，则预先建立信任关系
int offerAmount;           //账户提供这么多xrp->USD
TER tec;                   //返回的TEC代码
STAmount spentXrp;         //从Fundxrp中移除的金额
PrettyAmount balanceUsd;   //期末余额
int offers;                //分期付款
int owners;                //账户所有人
        };

        TestData const tests[]
        {
//acct fundxrp bookamt pretrust offermat tec spentxrp balance美元优惠业主
{"ann",             reserve (env, 0) + 0*f,       1,   noPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0}, //账户是在储备金中的，一旦扣除费用，账户就会下降。
{"bev",             reserve (env, 0) + 1*f,       1,   noPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0}, //账户刚好够支付准备金和费用。
{"cam",             reserve (env, 0) + 2*f,       0,   noPreTrust,     1000, tecINSUF_RESERVE_OFFER,             f, USD(      0),      0,     0}, //账户上有足够的储备、费用和报价，还有更多的，但在报价后还不够储备。
{"deb",             reserve (env, 0) + 2*f,       1,   noPreTrust,     1000,             tesSUCCESS,           2*f, USD(0.00001),      0,     1}, //帐户有足够的钱买一点美元，然后报价就干涸了。
{"eve",             reserve (env, 1) + 0*f,       0,   noPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1}, //无交叉报价
{"flo",             reserve (env, 1) + 0*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      0,     1},
{"gay",             reserve (env, 1) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(  50) + f, USD(     50),      0,     1},
{"hye", XRP(1000)                    + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP( 800) + f, USD(    800),      0,     1},
{"ivy", XRP(   1) + reserve (env, 1) + 1*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      0,     1},
{"joy", XRP(   1) + reserve (env, 2) + 1*f,       1,   noPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     2},
{"kim", XRP( 900) + reserve (env, 2) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"liz", XRP( 998) + reserve (env, 0) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 998) + f, USD(    998),      0,     1},
{"meg", XRP( 998) + reserve (env, 1) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"nia", XRP( 998) + reserve (env, 2) + 1*f,     999,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      1,     2},
{"ova", XRP( 999) + reserve (env, 0) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP( 999) + f, USD(    999),      0,     1},
{"pam", XRP( 999) + reserve (env, 1) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(1000) + f, USD(   1000),      0,     1},
{"rae", XRP( 999) + reserve (env, 2) + 1*f,    1000,   noPreTrust,     1000,             tesSUCCESS, XRP(1000) + f, USD(   1000),      0,     1},
{"sue", XRP(1000) + reserve (env, 2) + 1*f,       0,   noPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1},

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
{"abe",             reserve (env, 0) + 0*f,       1,   gwPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0},
{"bud",             reserve (env, 0) + 1*f,       1,   gwPreTrust,     1000,      tecUNFUNDED_OFFER,             f, USD(      0),      0,     0},
{"che",             reserve (env, 0) + 2*f,       0,   gwPreTrust,     1000, tecINSUF_RESERVE_OFFER,             f, USD(      0),      0,     0},
{"dan",             reserve (env, 0) + 2*f,       1,   gwPreTrust,     1000,             tesSUCCESS,           2*f, USD(0.00001),      0,     0},
{"eli", XRP(  20) + reserve (env, 0) + 1*f,    1000,   gwPreTrust,     1000,             tesSUCCESS, XRP(20) + 1*f, USD(     20),      0,     0},
{"fyn",             reserve (env, 1) + 0*f,       0,   gwPreTrust,     1000,             tesSUCCESS,             f, USD(      0),      1,     1},
{"gar",             reserve (env, 1) + 0*f,       1,   gwPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     1},
{"hal",             reserve (env, 1) + 1*f,       1,   gwPreTrust,     1000,             tesSUCCESS, XRP(   1) + f, USD(      1),      1,     1},

{"ned",             reserve (env, 1) + 0*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"ole",             reserve (env, 1) + 1*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"pat",             reserve (env, 1) + 2*f,       0, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"quy",             reserve (env, 1) + 2*f,       1, acctPreTrust,     1000,      tecUNFUNDED_OFFER,           2*f, USD(      0),      0,     1},
{"ron",             reserve (env, 1) + 3*f,       0, acctPreTrust,     1000, tecINSUF_RESERVE_OFFER,           2*f, USD(      0),      0,     1},
{"syd",             reserve (env, 1) + 3*f,       1, acctPreTrust,     1000,             tesSUCCESS,           3*f, USD(0.00001),      0,     1},
{"ted", XRP(  20) + reserve (env, 1) + 2*f,    1000, acctPreTrust,     1000,             tesSUCCESS, XRP(20) + 2*f, USD(     20),      0,     1},
{"uli",             reserve (env, 2) + 0*f,       0, acctPreTrust,     1000, tecINSUF_RESERVE_OFFER,           2*f, USD(      0),      0,     1},
{"vic",             reserve (env, 2) + 0*f,       1, acctPreTrust,     1000,             tesSUCCESS, XRP( 1) + 2*f, USD(      1),      0,     1},
{"wes",             reserve (env, 2) + 1*f,       0, acctPreTrust,     1000,             tesSUCCESS,           2*f, USD(      0),      1,     2},
{"xan",             reserve (env, 2) + 1*f,       1, acctPreTrust,     1000,             tesSUCCESS, XRP( 1) + 2*f, USD(      1),      1,     2},
};

        for (auto const& t : tests)
        {
            auto const acct = Account(t.account);
            env.fund (t.fundXrp, acct);
            env.close();

//确保网关没有当前的报价。
            env.require (offers (gw, 0));

//网关可以选择创建一个将被跨越的报价。
            auto const book = t.bookAmount;
            if (book)
                env (offer (gw, XRP (book), USD (book)));
            env.close();
            std::uint32_t const gwOfferSeq = env.seq (gw) - 1;

//可选择在吉布达•伟士和ACCT之间预先建立信任关系。
            if (t.preTrust == gwPreTrust)
                env (trust (gw, acct["USD"] (1)));

//可选择在ACCT和GW之间预先建立信任线。
//注意，这并不是测试的一部分，所以我们希望
//为acct预留足够的xrp来创建信任行。
            if (t.preTrust == acctPreTrust)
                env (trust (acct, USD (1)));

            env.close();

//ACCT创建报价。这是测试的核心。
            auto const acctOffer = t.offerAmount;
            env (offer (acct, USD (acctOffer), XRP (acctOffer)), ter (t.tec));
            env.close();
            std::uint32_t const acctOfferSeq = env.seq (acct) - 1;

            BEAST_EXPECT (env.balance (acct, USD.issue()) == t.balanceUsd);
            BEAST_EXPECT (
                env.balance (acct, xrpIssue()) == t.fundXrp - t.spentXrp);
            env.require (offers (acct, t.offers));
            env.require (owners (acct, t.owners));

            auto acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == t.offers);
            if (acctOffers.size() && t.offers)
            {
                auto const& acctOffer = *(acctOffers.front());

                auto const leftover = t.offerAmount - t.bookAmount;
                BEAST_EXPECT (acctOffer[sfTakerGets] == XRP (leftover));
                BEAST_EXPECT (acctOffer[sfTakerPays] == USD (leftover));
            }

            if (t.preTrust == noPreTrust)
            {
                if (t.balanceUsd.value().signum())
                {
//验证信任线的正确内容
                    verifyDefaultTrustline (env, acct, t.balanceUsd);
                }
                else
                {
//验证是否未创建信任线。
                    auto const sleTrust =
                        env.le (keylet::line(acct, USD.issue()));
                    BEAST_EXPECT (! sleTrust);
                }
            }

//取消所有剩余的动作，让下一个循环一笔勾销
//在报价中。
            env (offer_cancel (acct, acctOfferSeq));
            env (offer_cancel (gw, gwOfferSeq));
            env.close();
        }
    }

    void
    testXRPDirectCross (FeatureBitset features)
    {
        testcase ("XRP Direct Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw["USD"];

        auto const usdOffer = USD(1000);
        auto const xrpOffer = XRP(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw, bob);
        env.close();

//为交易收取的费用。
        auto const fee = env.current ()->fees ().base;

//艾丽斯的账户足以支付储备金，一条信托额度加上两条
//报价和两个费用。
        env.fund (reserve (env, 2) + (fee * 2), alice);
        env.close();

        env (trust(alice, usdOffer));

        env.close();

        env (pay(gw, alice, usdOffer));
        env.close();
        env.require (
            balance (alice, usdOffer),
            offers (alice, 0),
            offers (bob, 0));

//情景：
//o Alice有美元，但想要XRP。
//o Bob有xrp，但想要美元。
        auto const alicesXRP = env.balance (alice);
        auto const bobsXRP = env.balance (bob);

        env (offer (alice, xrpOffer, usdOffer));
        env.close();
        env (offer (bob, usdOffer, xrpOffer));

        env.close();
        env.require (
            balance (alice, USD(0)),
            balance (bob, usdOffer),
            balance (alice, alicesXRP + xrpOffer - fee),
            balance (bob,   bobsXRP   - xrpOffer - fee),
            offers (alice, 0),
            offers (bob, 0));

        verifyDefaultTrustline (env, bob, usdOffer);

//再提出两个报价，让其中一个报价不干。
        env (offer (alice, USD(999), XRP(999)));
        env (offer (bob, xrpOffer, usdOffer));

        env.close();
        env.require (balance (alice, USD(999)));
        env.require (balance (bob, USD(1)));
        env.require (offers (alice, 0));
        verifyDefaultTrustline (env, bob, USD(1));
        {
            auto const bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (bobsOffer[sfTakerGets] == USD (1));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == XRP (1));
        }
   }

    void
    testDirectCross (FeatureBitset features)
    {
        testcase ("Direct Crossing");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const usdOffer = USD(1000);
        auto const eurOffer = EUR(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw);
        env.close();

//为交易收取的费用。
        auto const fee = env.current ()->fees ().base;

//每个账户都有足够的存款准备金，两条信托额度，一条
//报价和两个费用。
        env.fund (reserve (env, 3) + (fee * 3), alice);
        env.fund (reserve (env, 3) + (fee * 2), bob);
        env.close();
        env (trust(alice, usdOffer));
        env (trust(bob, eurOffer));
        env.close();

        env (pay(gw, alice, usdOffer));
        env (pay(gw, bob, eurOffer));
        env.close();
        env.require (
            balance (alice, usdOffer),
            balance (bob, eurOffer));

//情景：
//o爱丽丝有美元，但要欧元。
//o Bob有欧元，但想要美元。
        env (offer (alice, eurOffer, usdOffer));
        env (offer (bob, usdOffer, eurOffer));

        env.close();
        env.require (
            balance (alice, eurOffer),
            balance (bob, usdOffer),
            offers (alice, 0),
            offers (bob, 0));
        verifyDefaultTrustline (env, alice, eurOffer);
        verifyDefaultTrustline (env, bob, usdOffer);

//再提出两个报价，让其中一个报价不干。
        env (offer (alice, USD(999), eurOffer));
        env (offer (bob, eurOffer, usdOffer));

        env.close();
        env.require (balance (alice, USD(999)));
        env.require (balance (alice, EUR(1)));
        env.require (balance (bob, USD(1)));
        env.require (balance (bob, EUR(999)));
        env.require (offers (alice, 0));
        verifyDefaultTrustline (env, alice, EUR(1));
        verifyDefaultTrustline (env, bob, USD(1));
        {
            auto bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfTakerGets] == USD (1));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == EUR (1));
        }

//艾丽斯又提出一个报价，以澄清鲍勃的报价。
        env (offer (alice, USD(1), EUR(1)));

        env.close();
        env.require (balance (alice, USD(1000)));
        env.require (balance (alice, EUR(none)));
        env.require (balance (bob, USD(none)));
        env.require (balance (bob, EUR(1000)));
        env.require (offers (alice, 0));
        env.require (offers (bob, 0));

//由报价产生的两条信任线应该消失了。
        BEAST_EXPECT (! env.le (keylet::line (alice.id(), EUR.issue())));
        BEAST_EXPECT (! env.le (keylet::line (bob.id(), USD.issue())));
    }

    void
    testBridgedCross (FeatureBitset features)
    {
        testcase ("Bridged Crossing");

        using namespace jtx;

        auto const gw    = Account("gateway");
        auto const alice = Account("alice");
        auto const bob   = Account("bob");
        auto const carol = Account("carol");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        auto const usdOffer = USD(1000);
        auto const eurOffer = EUR(1000);

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw, alice, bob, carol);
        env.close();

        env (trust(alice, usdOffer));
        env (trust(carol, eurOffer));
        env.close();
        env (pay(gw, alice, usdOffer));
        env (pay(gw, carol, eurOffer));
        env.close();

//情景：
//o Alice有美元，但想要xpr。
//o Bob有xrp，但想要欧元。
//卡罗尔有欧元，但要美元。
//注意，卡罗尔的提议必须排在最后。如果卡罗尔的提议被提出
//在鲍勃或爱丽丝之前，不会发生自动驾驶。
        env (offer (alice, XRP(1000), usdOffer));
        env (offer (bob, eurOffer, XRP(1000)));
        auto const bobXrpBalance = env.balance (bob);
        env.close();

//卡罗尔提出的报价部分消耗了爱丽丝和鲍勃的报价。
        env (offer (carol, USD(400), EUR(400)));
        env.close();

        env.require (
            balance (alice, USD(600)),
            balance (bob,   EUR(400)),
            balance (carol, USD(400)),
            balance (bob, bobXrpBalance - XRP(400)),
            offers (carol, 0));
        verifyDefaultTrustline (env, bob, EUR(400));
        verifyDefaultTrustline (env, carol, USD(400));
        {
            auto const alicesOffers = offersOnAccount (env, alice);
            BEAST_EXPECT (alicesOffers.size() == 1);
            auto const& alicesOffer = *(alicesOffers.front());

            BEAST_EXPECT (alicesOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (alicesOffer[sfTakerGets] == USD (600));
            BEAST_EXPECT (alicesOffer[sfTakerPays] == XRP (600));
        }
        {
            auto const bobsOffers = offersOnAccount (env, bob);
            BEAST_EXPECT (bobsOffers.size() == 1);
            auto const& bobsOffer = *(bobsOffers.front());

            BEAST_EXPECT (bobsOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (bobsOffer[sfTakerGets] == XRP (600));
            BEAST_EXPECT (bobsOffer[sfTakerPays] == EUR (600));
        }

//卡罗尔提出的报价完全消耗了爱丽丝和鲍勃的报价。
        env (offer (carol, USD(600), EUR(600)));
        env.close();

        env.require (
            balance (alice, USD(0)),
            balance (bob, eurOffer),
            balance (carol, usdOffer),
            balance (bob, bobXrpBalance - XRP(1000)),
            offers (bob, 0),
            offers (carol, 0));
        verifyDefaultTrustline (env, bob, EUR(1000));
        verifyDefaultTrustline (env, carol, USD(1000));

//在预处理代码中，爱丽丝的报价在分类帐中是空的。
        auto const alicesOffers = offersOnAccount (env, alice);
        if (alicesOffers.size() != 0)
        {
            BEAST_EXPECT (alicesOffers.size() == 1);
            auto const& alicesOffer = *(alicesOffers.front());

            BEAST_EXPECT (alicesOffer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (alicesOffer[sfTakerGets] == USD (0));
            BEAST_EXPECT (alicesOffer[sfTakerPays] == XRP (0));
        }
    }

    void
    testSellOffer (FeatureBitset features)
    {
//测试许多与报价交叉有关的不同转角案例
//当设置Tfsell标志时。测试是桌上驱动的，因此
//应该易于添加或删除测试。
        testcase ("Sell Offer");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw);

//为交易收取的费用
        auto const f = env.current ()->fees ().base;

//为了简单起见，所有报价都是1:1的xrp:usd。
        enum preTrustType {noPreTrust, gwPreTrust, acctPreTrust};
        struct TestData
        {
std::string account;       //开户日期
STAmount fundXrp;          //XRP账户资金来源
STAmount fundUSD;          //美元账户
STAmount gwGets;           //通用汽车的报价
STAmount gwPays;           //
STAmount acctGets;         //ACCT的报价
STAmount acctPays;         //
TER tec;                   //返回的TEC代码
STAmount spentXrp;         //从Fundxrp中移除的金额
STAmount finalUsd;         //账户最终美元余额
int offers;                //Acct报价
int owners;                //业主协会
STAmount takerGets;        //账户报价的剩余部分
STAmount takerPays;        //

//承建人和承建人
            TestData (
std::string&& account_,         //开户日期
STAmount const& fundXrp_,       //XRP账户资金来源
STAmount const& fundUSD_,       //美元账户
STAmount const& gwGets_,        //通用汽车的报价
STAmount const& gwPays_,        //
STAmount const& acctGets_,      //ACCT的报价
STAmount const& acctPays_,      //
TER tec_,                       //返回的TEC代码
STAmount const& spentXrp_,      //从Fundxrp中移除的金额
STAmount const& finalUsd_,      //账户最终美元余额
int offers_,                    //Acct报价
int owners_,                    //业主协会
STAmount const& takerGets_,     //账户报价的剩余部分
STAmount const& takerPays_)     //
                : account (std::move(account_))
                , fundXrp (fundXrp_)
                , fundUSD (fundUSD_)
                , gwGets (gwGets_)
                , gwPays (gwPays_)
                , acctGets (acctGets_)
                , acctPays (acctPays_)
                , tec (tec_)
                , spentXrp (spentXrp_)
                , finalUsd (finalUsd_)
                , offers (offers_)
                , owners (owners_)
                , takerGets (takerGets_)
                , takerPays (takerPays_)
            { }

//无承购人承建人获得/承购人支付
            TestData (
std::string&& account_,         //开户日期
STAmount const& fundXrp_,       //XRP账户资金来源
STAmount const& fundUSD_,       //美元账户
STAmount const& gwGets_,        //通用汽车的报价
STAmount const& gwPays_,        //
STAmount const& acctGets_,      //ACCT的报价
STAmount const& acctPays_,      //
TER tec_,                       //返回的TEC代码
STAmount const& spentXrp_,      //从Fundxrp中移除的金额
STAmount const& finalUsd_,      //账户最终美元余额
int offers_,                    //Acct报价
int owners_)                    //业主协会
                : TestData (std::move(account_), fundXrp_, fundUSD_, gwGets_,
                  gwPays_, acctGets_, acctPays_, tec_, spentXrp_, finalUsd_,
                  offers_, owners_, STAmount {0}, STAmount {0})
            { }
        };

        TestData const tests[]
        {
//Acct支付XRP
//acct fundxrp fundusd gwgets gwpays acctgets acctpays tec spentxrp finalusd offer owners takergets takerpay
{"ann", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD( 5),  USD(10),  XRP(10), tecINSUF_RESERVE_OFFER, XRP( 0) + (1*f),  USD( 0),      0,     0},
{"bev", XRP(10) + reserve (env, 1) + 1*f, USD( 0), XRP(10), USD( 5),  USD(10),  XRP(10),             tesSUCCESS, XRP( 0) + (1*f),  USD( 0),      1,     1,     XRP(10),  USD(10)},
{"cam", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(10),  USD(10),  XRP(10),             tesSUCCESS, XRP(10) + (1*f),  USD(10),      0,     1},
{"deb", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD(10),  XRP(10),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"eve", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD( 5),  XRP( 5),             tesSUCCESS, XRP( 5) + (1*f),  USD(10),      0,     1},
{"flo", XRP(10) + reserve (env, 0) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"gay", XRP(20) + reserve (env, 1) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      0,     1},
{"hye", XRP(20) + reserve (env, 2) + 1*f, USD( 0), XRP(10), USD(20),  USD(20),  XRP(20),             tesSUCCESS, XRP(10) + (1*f),  USD(20),      1,     2,     XRP(10),  USD(10)},
//ACT支付美元
{"meg",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP( 5),  XRP(10),  USD(10), tecINSUF_RESERVE_OFFER, XRP(  0) + (2*f),  USD(10),      0,     1},
{"nia",           reserve (env, 2) + 2*f, USD(10), USD(10), XRP( 5),  XRP(10),  USD(10),             tesSUCCESS, XRP(  0) + (2*f),  USD(10),      1,     2,     USD(10),  XRP(10)},
{"ova",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP(10),  XRP(10),  USD(10),             tesSUCCESS, XRP(-10) + (2*f),  USD( 0),      0,     1},
{"pam",           reserve (env, 1) + 2*f, USD(10), USD(10), XRP(20),  XRP(10),  USD(10),             tesSUCCESS, XRP(-20) + (2*f),  USD( 0),      0,     1},
{"qui",           reserve (env, 1) + 2*f, USD(10), USD(20), XRP(40),  XRP(10),  USD(10),             tesSUCCESS, XRP(-20) + (2*f),  USD( 0),      0,     1},
{"rae",           reserve (env, 2) + 2*f, USD(10), USD( 5), XRP( 5),  XRP(10),  USD(10),             tesSUCCESS, XRP( -5) + (2*f),  USD( 5),      1,     2,     USD( 5),  XRP( 5)},
{"sue",           reserve (env, 2) + 2*f, USD(10), USD( 5), XRP(10),  XRP(10),  USD(10),             tesSUCCESS, XRP(-10) + (2*f),  USD( 5),      1,     2,     USD( 5),  XRP( 5)},
};
        auto const zeroUsd = USD(0);
        for (auto const& t : tests)
        {
//确保网关没有当前的报价。
            env.require (offers (gw, 0));

            auto const acct = Account(t.account);

            env.fund (t.fundXrp, acct);
            env.close();

//或者给ACCT一些美元。这不是测试的一部分，
//因此，我们假设账户有足够的美元来支付准备金。
//在信任线上。
            if (t.fundUSD != zeroUsd)
            {
                env (trust (acct, t.fundUSD));
                env.close();
                env (pay (gw, acct, t.fundUSD));
                env.close();
            }

            env (offer (gw, t.gwGets, t.gwPays));
            env.close();
            std::uint32_t const gwOfferSeq = env.seq (gw) - 1;

//acct创建tfsell提供。这是测试的核心。
            env (offer (acct, t.acctGets, t.acctPays, tfSell), ter (t.tec));
            env.close();
            std::uint32_t const acctOfferSeq = env.seq (acct) - 1;

//检查结果
            BEAST_EXPECT (env.balance (acct, USD.issue()) == t.finalUsd);
            BEAST_EXPECT (
                env.balance (acct, xrpIssue()) == t.fundXrp - t.spentXrp);
            env.require (offers (acct, t.offers));
            env.require (owners (acct, t.owners));

            if (t.offers)
            {
                auto const acctOffers = offersOnAccount (env, acct);
                if (acctOffers.size() > 0)
                {
                    BEAST_EXPECT (acctOffers.size() == 1);
                    auto const& acctOffer = *(acctOffers.front());

                    BEAST_EXPECT (acctOffer[sfLedgerEntryType] == ltOFFER);
                    BEAST_EXPECT (acctOffer[sfTakerGets] == t.takerGets);
                    BEAST_EXPECT (acctOffer[sfTakerPays] == t.takerPays);
                }
            }

//取消所有剩余的动作，让下一个循环一笔勾销
//在报价中。
            env (offer_cancel (acct, acctOfferSeq));
            env (offer_cancel (gw, gwOfferSeq));
            env.close();
        }
    }

    void
    testSellWithFillOrKill (FeatureBitset features)
    {
//测试许多与报价交叉有关的不同转角案例
//当同时设置tfsell标志和tffillWorkill标志时。
        testcase ("Combine tfSell with tfFillOrKill");

        using namespace jtx;

        auto const gw    = Account("gateway");
        auto const alice = Account("alice");
        auto const bob   = Account("bob");
        auto const USD   = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(10000000), gw, alice, bob);

//如果报价被取消，则返回代码。
        TER const killedCode {
            features[fix1578] ? TER {tecKILLED} : TER {tesSUCCESS}};

//Bob提供美元的XRP。
        env (trust(bob, USD(200)));
        env.close();
        env (pay(gw, bob, USD(100)));
        env.close();
        env (offer(bob, XRP(2000), USD(20)));
        env.close();
        {
//Alice提交了不交叉的Tfsell TfFillWorkill报价。
            env (offer(alice, USD(21), XRP(2100),
                tfSell | tfFillOrKill), ter (killedCode));
            env.close();
            env.require (balance (alice, USD(none)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(100)));
        }
        {
//Alice提交了一个交叉的Tfsell TfFillWorkill报价。
//即使Tfsell在场，这次也没关系。
            env (offer(alice, USD(20), XRP(2000), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(20)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(80)));
        }
        {
//Alice提交了一份跨越和
//返回超出要求的值（因为tfsell标志）。
            env (offer(bob, XRP(2000), USD(20)));
            env.close();
            env (offer(alice, USD(10), XRP(1500), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(35)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(65)));
        }
        {
//Alice提交了一份不交叉的Tfsell TfFillWorkill报价。
//如果有一个常规的tfsell，这会成功，但是
//Fillorkill防止事务交叉，因为
//所有的报价都被消耗掉了。

//我们使用的是Bob对XRP（500）的剩余报价，美元（5）
            env (offer(alice, USD(1), XRP(501),
                tfSell | tfFillOrKill), ter (killedCode));
            env.close();
            env.require (balance (alice, USD(35)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(65)));
        }
        {
//Alice提交完成的Tfsell TfFillWorkill报价
//从鲍勃剩余的报价中扣除。

//我们使用的是Bob对XRP（500）的剩余报价，美元（5）
            env (offer(alice, USD(1), XRP(500), tfSell | tfFillOrKill));
            env.close();
            env.require (balance (alice, USD(40)));
            env.require (offers (alice, 0));
            env.require (balance (bob, USD(60)));
        }
    }

    void
    testTransferRateOffer (FeatureBitset features)
    {
        testcase ("Transfer Rate Offer");

        using namespace jtx;

        auto const gw1 = Account("gateway1");
        auto const USD = gw1["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

//为交易收取的费用。
        auto const fee = env.current ()->fees ().base;

        env.fund (XRP(100000), gw1);
        env.close();

        env(rate(gw1, 1.25));
        {
            auto const ann = Account("ann");
            auto const bob = Account("bob");
            env.fund (XRP(100) + reserve(env, 2) + (fee*2), ann, bob);
            env.close();

            env (trust(ann, USD(200)));
            env (trust(bob, USD(200)));
            env.close();

            env (pay (gw1, bob, USD(125)));
            env.close();

//Bob出价100美元购买XRP。爱丽丝接受了鲍勃的提议。
//请注意，虽然Bob只提供100美元，但125美元是
//因网关费而从他的帐户中删除。
//
//类似的付款方式如下：
//env（支付（bob，alice，100美元），sendmax（125美元）））
            env (offer (bob, XRP(1), USD(100)));
            env.close();

            env (offer (ann, USD(100), XRP(1)));
            env.close();

            env.require (balance (ann, USD(100)));
            env.require (balance (ann, XRP( 99) + reserve(env, 2)));
            env.require (offers (ann, 0));

            env.require (balance (bob, USD(  0)));
            env.require (balance (bob, XRP(101) + reserve(env, 2)));
            env.require (offers (bob, 0));
        }
        {
//相反的顺序，所以书中的报价是出售xrp
//以美元作为回报。网关速率的应用应该相同。
            auto const che = Account("che");
            auto const deb = Account("deb");
            env.fund (XRP(100) + reserve(env, 2) + (fee*2), che, deb);
            env.close();

            env (trust(che, USD(200)));
            env (trust(deb, USD(200)));
            env.close();

            env (pay (gw1, deb, USD(125)));
            env.close();

            env (offer (che, USD(100), XRP(1)));
            env.close();

            env (offer (deb, XRP(1), USD(100)));
            env.close();

            env.require (balance (che, USD(100)));
            env.require (balance (che, XRP( 99) + reserve(env, 2)));
            env.require (offers (che, 0));

            env.require (balance (deb, USD(  0)));
            env.require (balance (deb, XRP(101) + reserve(env, 2)));
            env.require (offers (deb, 0));
        }
        {
            auto const eve = Account("eve");
            auto const fyn = Account("fyn");

            env.fund (XRP(20000) + fee*2, eve, fyn);
            env.close();

            env (trust (eve, USD(1000)));
            env (trust (fyn, USD(1000)));
            env.close();

            env (pay (gw1, eve, USD(100)));
            env (pay (gw1, fyn, USD(100)));
            env.close();

//此测试验证从报价中删除的金额
//从中删除的转移费的帐户
//但不能从剩余报价中扣除。
            env (offer (eve, USD(10), XRP(4000)));
            env.close();
            std::uint32_t const eveOfferSeq = env.seq (eve) - 1;

            env (offer (fyn, XRP(2000), USD(5)));
            env.close();

            env.require (balance (eve, USD(105)));
            env.require (balance (eve, XRP(18000)));
            auto const evesOffers = offersOnAccount (env, eve);
            BEAST_EXPECT (evesOffers.size() == 1);
            if (evesOffers.size() != 0)
            {
                auto const& evesOffer = *(evesOffers.front());
                BEAST_EXPECT (evesOffer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (evesOffer[sfTakerGets] == XRP (2000));
                BEAST_EXPECT (evesOffer[sfTakerPays] == USD (5));
            }
env (offer_cancel (eve, eveOfferSeq)); //用于以后的测试

            env.require (balance (fyn, USD(93.75)));
            env.require (balance (fyn, XRP(22000)));
            env.require (offers (fyn, 0));
        }
//从两种非本国货币开始。
        auto const gw2   = Account("gateway2");
        auto const EUR = gw2["EUR"];

        env.fund (XRP(100000), gw2);
        env.close();

        env(rate(gw2, 1.5));
        {
//从公式中删除xrp。给这两种货币两个
//不同的转移率，这样我们可以看到两种转移率
//在同一事务中应用。
            auto const gay = Account("gay");
            auto const hal = Account("hal");
            env.fund (reserve(env, 3) + (fee*3), gay, hal);
            env.close();

            env (trust(gay, USD(200)));
            env (trust(gay, EUR(200)));
            env (trust(hal, USD(200)));
            env (trust(hal, EUR(200)));
            env.close();

            env (pay (gw1, gay, USD(125)));
            env (pay (gw2, hal, EUR(150)));
            env.close();

            env (offer (gay, EUR(100), USD(100)));
            env.close();

            env (offer (hal, USD(100), EUR(100)));
            env.close();

            env.require (balance (gay, USD(  0)));
            env.require (balance (gay, EUR(100)));
            env.require (balance (gay, reserve(env, 3)));
            env.require (offers (gay, 0));

            env.require (balance (hal, USD(100)));
            env.require (balance (hal, EUR(  0)));
            env.require (balance (hal, reserve(env, 3)));
            env.require (offers (hal, 0));
        }
        {
//信托额度的质量不应影响报价交叉。
            auto const ivy = Account("ivy");
            auto const joe = Account("joe");
            env.fund (reserve(env, 3) + (fee*3), ivy, joe);
            env.close();

            env (trust(ivy, USD(400)), qualityInPercent (90));
            env (trust(ivy, EUR(400)), qualityInPercent (80));
            env (trust(joe, USD(400)), qualityInPercent (70));
            env (trust(joe, EUR(400)), qualityInPercent (60));
            env.close();

            env (pay (gw1, ivy, USD(270)), sendmax (USD(500)));
            env (pay (gw2, joe, EUR(150)), sendmax (EUR(300)));
            env.close();
            env.require (balance (ivy, USD(300)));
            env.require (balance (joe, EUR(250)));

            env (offer (ivy, EUR(100), USD(200)));
            env.close();

            env (offer (joe, USD(200), EUR(100)));
            env.close();

            env.require (balance (ivy, USD( 50)));
            env.require (balance (ivy, EUR(100)));
            env.require (balance (ivy, reserve(env, 3)));
            env.require (offers (ivy, 0));

            env.require (balance (joe, USD(200)));
            env.require (balance (joe, EUR(100)));
            env.require (balance (joe, reserve(env, 3)));
            env.require (offers (joe, 0));
        }
        {
//信托额度的质量不应影响报价交叉。
            auto const kim = Account("kim");
            auto const K_BUX = kim["BUX"];
            auto const lex = Account("lex");
            auto const meg = Account("meg");
            auto const ned = Account("ned");
            auto const N_BUX = ned["BUX"];

//验证信托额度质量是否影响付款。
            env.fund (reserve(env, 4) + (fee*4), kim, lex, meg, ned);
            env.close();

            env (trust (lex, K_BUX(400)));
            env (trust (lex, N_BUX(200)), qualityOutPercent (120));
            env (trust (meg, N_BUX(100)));
            env.close();
            env (pay (ned, lex, N_BUX(100)));
            env.close();
            env.require (balance (lex, N_BUX(100)));

            env (pay (kim, meg,
                N_BUX(60)), path (lex, ned), sendmax (K_BUX(200)));
            env.close();

            env.require (balance (kim, K_BUX(none)));
            env.require (balance (kim, N_BUX(none)));
            env.require (balance (lex, K_BUX(  72)));
            env.require (balance (lex, N_BUX(  40)));
            env.require (balance (meg, K_BUX(none)));
            env.require (balance (meg, N_BUX(  60)));
            env.require (balance (ned, K_BUX(none)));
            env.require (balance (ned, N_BUX(none)));

//现在，确认报价交叉不受QualityOut的影响。
            env (offer (lex, K_BUX(30), N_BUX(30)));
            env.close();

            env (offer (kim, N_BUX(30), K_BUX(30)));
            env.close();

            env.require (balance (kim, K_BUX(none)));
            env.require (balance (kim, N_BUX(  30)));
            env.require (balance (lex, K_BUX( 102)));
            env.require (balance (lex, N_BUX(  10)));
            env.require (balance (meg, K_BUX(none)));
            env.require (balance (meg, N_BUX(  60)));
            env.require (balance (ned, K_BUX( -30)));
            env.require (balance (ned, N_BUX(none)));
        }
        {
//当我们进行自动桥接时，确保一切正常。
            auto const ova = Account("ova");
            auto const pat = Account("pat");
            auto const qae = Account("qae");
            env.fund (XRP(2) + reserve(env, 3) + (fee*3), ova, pat, qae);
            env.close();

//奥瓦有美元，但想要XPR。
//o pat有xrp，但想要欧元。
//o Qae有欧元，但需要美元。
            env (trust(ova, USD(200)));
            env (trust(ova, EUR(200)));
            env (trust(pat, USD(200)));
            env (trust(pat, EUR(200)));
            env (trust(qae, USD(200)));
            env (trust(qae, EUR(200)));
            env.close();

            env (pay (gw1, ova, USD(125)));
            env (pay (gw2, qae, EUR(150)));
            env.close();

            env (offer (ova, XRP(2), USD(100)));
            env (offer (pat, EUR(100), XRP(2)));
            env.close();

            env (offer (qae, USD(100), EUR(100)));
            env.close();

            env.require (balance (ova, USD(  0)));
            env.require (balance (ova, EUR(  0)));
            env.require (balance (ova, XRP(4) + reserve(env, 3)));

//在预处理代码中，Ova的报价在分类帐中保留为空。
            auto const ovasOffers = offersOnAccount (env, ova);
            if (ovasOffers.size() != 0)
            {
                BEAST_EXPECT (ovasOffers.size() == 1);
                auto const& ovasOffer = *(ovasOffers.front());

                BEAST_EXPECT (ovasOffer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (ovasOffer[sfTakerGets] == USD (0));
                BEAST_EXPECT (ovasOffer[sfTakerPays] == XRP (0));
            }

            env.require (balance (pat, USD(  0)));
            env.require (balance (pat, EUR(100)));
            env.require (balance (pat, XRP(0) + reserve(env, 3)));
            env.require (offers (pat, 0));

            env.require (balance (qae, USD(100)));
            env.require (balance (qae, EUR(  0)));
            env.require (balance (qae, XRP(2) + reserve(env, 3)));
            env.require (offers (qae, 0));
        }
    }

    void
    testSelfCrossOffer1 (FeatureBitset features)
    {
//下面的测试验证了一些正确但有点令人惊讶的地方
//报价交叉行为。情景：
//
//o一个实体已创建一个或多个报价。
//o实体创建另一个可直接交叉的报价
//（不是自动桥接）由先前创建的报价。
//o删除旧的报价，而不是自行跨越报价。
//
//在注释中查看更完整的解释
//BookOfferCrossingStep:：LimitSelfCrossQuality（）。
//
//请注意，在这个特定的示例中，一个报价会导致
//交叉报价（比新报价更值钱）
//从书中删除。
        using namespace jtx;

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

//为交易收取的费用。
        auto const fee = env.current ()->fees ().base;
        auto const startBalance = XRP(1000000);

        env.fund (startBalance + (fee*4), gw);
        env.close();

        env (offer (gw, USD(60), XRP(600)));
        env.close();
        env (offer (gw, USD(60), XRP(600)));
        env.close();
        env (offer (gw, USD(60), XRP(600)));
        env.close();

        env.require (owners (gw, 3));
        env.require (balance (gw, startBalance + fee));

        auto gwOffers = offersOnAccount (env, gw);
        BEAST_EXPECT (gwOffers.size() == 3);
        for (auto const& offerPtr : gwOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (offer[sfTakerGets] == XRP (600));
            BEAST_EXPECT (offer[sfTakerPays] == USD ( 60));
        }

//由于此报价与第一个报价交叉，所以以前的报价
//将被删除，此报价将放在订单簿上。
        env (offer (gw, XRP(1000), USD(100)));
        env.close();
        env.require (owners (gw, 1));
        env.require (offers (gw, 1));
        env.require (balance (gw, startBalance));

        gwOffers = offersOnAccount (env, gw);
        BEAST_EXPECT (gwOffers.size() == 1);
        for (auto const& offerPtr : gwOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT (offer[sfTakerGets] == USD (100));
            BEAST_EXPECT (offer[sfTakerPays] == XRP (1000));
        }
    }

    void
    testSelfCrossOffer2 (FeatureBitset features)
    {
        using namespace jtx;

        auto const gw1 = Account("gateway1");
        auto const gw2 = Account("gateway2");
        auto const alice = Account("alice");
        auto const USD = gw1["USD"];
        auto const EUR = gw2["EUR"];

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        env.fund (XRP(1000000), gw1, gw2);
        env.close();

//为交易收取的费用。
        auto const f = env.current ()->fees ().base;

//测试用例
        struct TestData
        {
std::string acct;          //开户日期
STAmount fundXRP;          //XRP账户资金来源
STAmount fundUSD;          //美元账户
STAmount fundEUR;          //欧元账户
TER firstOfferTec;         //第一次报价时的技术规范
TER secondOfferTec;        //第二次报价的技术规范
        };

        TestData const tests[]
        {
//acct fundxrp fundusd fundeur firstoffertec secondoffertec
{"ann", reserve(env, 3) + f*4, USD(1000), EUR(1000),             tesSUCCESS,             tesSUCCESS},
{"bev", reserve(env, 3) + f*4, USD(   1), EUR(1000),             tesSUCCESS,             tesSUCCESS},
{"cam", reserve(env, 3) + f*4, USD(1000), EUR(   1),             tesSUCCESS,             tesSUCCESS},
{"deb", reserve(env, 3) + f*4, USD(   0), EUR(   1),             tesSUCCESS,      tecUNFUNDED_OFFER},
{"eve", reserve(env, 3) + f*4, USD(   1), EUR(   0),      tecUNFUNDED_OFFER,             tesSUCCESS},
{"flo", reserve(env, 3) +   0, USD(1000), EUR(1000), tecINSUF_RESERVE_OFFER, tecINSUF_RESERVE_OFFER},
        };

        for (auto const& t : tests)
        {
            auto const acct = Account {t.acct};
            env.fund (t.fundXRP, acct);
            env.close();

            env (trust (acct, USD(1000)));
            env (trust (acct, EUR(1000)));
            env.close();

            if (t.fundUSD > USD(0))
                env (pay (gw1, acct, t.fundUSD));
            if (t.fundEUR > EUR(0))
                env (pay (gw2, acct, t.fundEUR));
            env.close();

            env (offer (acct, USD(500), EUR(600)), ter (t.firstOfferTec));
            env.close();
            std::uint32_t const firstOfferSeq = env.seq (acct) - 1;

            int offerCount = t.firstOfferTec == tesSUCCESS ? 1 : 0;
            env.require (owners (acct, 2 + offerCount));
            env.require (balance (acct, t.fundUSD));
            env.require (balance (acct, t.fundEUR));

            auto acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == offerCount);
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                BEAST_EXPECT (offer[sfTakerGets] == EUR (600));
                BEAST_EXPECT (offer[sfTakerPays] == USD (500));
            }

            env (offer (acct, EUR(600), USD(500)), ter (t.secondOfferTec));
            env.close();
            std::uint32_t const secondOfferSeq = env.seq (acct) - 1;

            offerCount = t.secondOfferTec == tesSUCCESS ? 1 : offerCount;
            env.require (owners (acct, 2 + offerCount));
            env.require (balance (acct, t.fundUSD));
            env.require (balance (acct, t.fundEUR));

            acctOffers = offersOnAccount (env, acct);
            BEAST_EXPECT (acctOffers.size() == offerCount);
            for (auto const& offerPtr : acctOffers)
            {
                auto const& offer = *offerPtr;
                BEAST_EXPECT (offer[sfLedgerEntryType] == ltOFFER);
                if (offer[sfSequence] == firstOfferSeq)
                {
                    BEAST_EXPECT (offer[sfTakerGets] == EUR (600));
                    BEAST_EXPECT (offer[sfTakerPays] == USD (500));
                }
                else
                {
                    BEAST_EXPECT (offer[sfTakerGets] == USD (500));
                    BEAST_EXPECT (offer[sfTakerPays] == EUR (600));
                }
            }

//从帐户中删除下一个通行证的所有优惠。
            env (offer_cancel (acct, firstOfferSeq));
            env.close();
            env (offer_cancel (acct, secondOfferSeq));
            env.close();
        }
    }

    void
    testSelfCrossOffer (FeatureBitset features)
    {
        testcase ("Self Cross Offer");
        testSelfCrossOffer1 (features);
        testSelfCrossOffer2 (features);
    }

    void
    testSelfIssueOffer (FeatureBitset features)
    {
//实际上，发行自己货币的人和发行自己货币的人一样多
//他们所信任的基金。这个测试过去失败是因为
//未正确检查自我发布。验证它是否有效
//现在是正确的。
        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = bob["USD"];
        auto const f = env.current ()->fees ().base;

        env.fund(XRP(50000) + f, alice, bob);
        env.close();

        env(offer(alice, USD(5000), XRP(50000)));
        env.close();

//这个提议应该使爱丽丝的提议达到爱丽丝的保留价。
        env(offer(bob, XRP(50000), USD(5000)));
        env.close();

//艾丽斯的提议应该取消，因为她已经决定了。
//XRP储备
        env.require (balance (alice, XRP(250)));
        env.require (owners (alice, 1));
        env.require (lines (alice, 1));

//但是鲍勃的报价应该在分类帐上，因为它不是
//完全交叉。
        auto const bobOffers = offersOnAccount (env, bob);
        BEAST_EXPECT(bobOffers.size() == 1);
        for (auto const& offerPtr : bobOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == USD ( 25));
            BEAST_EXPECT(offer[sfTakerPays] == XRP (250));
        }
    }

    void
    testBadPathAssert (FeatureBitset features)
    {
//在过去的某个时刻，这个无效路径导致了断言。它
//用户提供的数据不可能导致断言。
//确保断言不存在。
        testcase ("Bad path assert");

        using namespace jtx;

//当启用FeatureOwnerPaysFee时发现问题，
//所以一定要包括在内。
        Env env {*this, features | featureOwnerPaysFee};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

//为交易收取的费用。
        auto const fee = env.current ()->fees ().base;
        {
//信托额度的质量不应影响报价交叉。
            auto const ann   = Account("ann");
            auto const A_BUX = ann["BUX"];
            auto const bob   = Account("bob");
            auto const cam   = Account("cam");
            auto const dan   = Account("dan");
            auto const D_BUX = dan["BUX"];

//验证信托额度质量是否影响付款。
            env.fund (reserve(env, 4) + (fee*4), ann, bob, cam, dan);
            env.close();

            env (trust (bob, A_BUX(400)));
            env (trust (bob, D_BUX(200)), qualityOutPercent (120));
            env (trust (cam, D_BUX(100)));
            env.close();
            env (pay (dan, bob, D_BUX(100)));
            env.close();
            env.require (balance (bob, D_BUX(100)));

            env (pay (ann, cam, D_BUX(60)),
                path (bob, dan), sendmax (A_BUX(200)));
            env.close();

            env.require (balance (ann, A_BUX(none)));
            env.require (balance (ann, D_BUX(none)));
            env.require (balance (bob, A_BUX(  72)));
            env.require (balance (bob, D_BUX(  40)));
            env.require (balance (cam, A_BUX(none)));
            env.require (balance (cam, D_BUX(  60)));
            env.require (balance (dan, A_BUX(none)));
            env.require (balance (dan, D_BUX(none)));

            env (offer (bob, A_BUX(30), D_BUX(30)));
            env.close();

            env (trust (ann, D_BUX(100)));
            env.close();

//确定我们期望的TEC代码。
            TER const tecExpect =
                features[featureFlow] ? TER {temBAD_PATH} : TER {tecPATH_DRY};

//此付款导致断言。
            env (pay (ann, ann, D_BUX(30)),
                path (A_BUX, D_BUX), sendmax (A_BUX(30)), ter (tecExpect));
            env.close();

            env.require (balance (ann, A_BUX(none)));
            env.require (balance (ann, D_BUX(   0)));
            env.require (balance (bob, A_BUX(  72)));
            env.require (balance (bob, D_BUX(  40)));
            env.require (balance (cam, A_BUX(none)));
            env.require (balance (cam, D_BUX(  60)));
            env.require (balance (dan, A_BUX(   0)));
            env.require (balance (dan, D_BUX(none)));
        }
    }

    void testDirectToDirectPath (FeatureBitset features)
    {
//报价交叉代码期望DirectStep始终
//前面有一个bookstep。在一个实例中，默认路径
//与该假设不匹配。在这里我们重新创建了那个案例
//所以我们可以证明这个错误是固定的。
        testcase ("Direct to Direct path");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const ann = Account("ann");
        auto const bob = Account("bob");
        auto const cam = Account("cam");
        auto const A_BUX = ann["BUX"];
        auto const B_BUX = bob["BUX"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 4) + (fee*5), ann, bob, cam);
        env.close();

        env (trust (ann, B_BUX(40)));
        env (trust (cam, A_BUX(40)));
        env (trust (cam, B_BUX(40)));
        env.close();

        env (pay (ann, cam, A_BUX(35)));
        env (pay (bob, cam, B_BUX(35)));

        env (offer (bob, A_BUX(30), B_BUX(30)));
        env.close();

//Cam在书上提出了一个提议，她即将提出的提议可能会通过。
//但是这个提议应该被删除，而不是被她即将到来的
//报盘。
        env (offer (cam, A_BUX(29), B_BUX(30), tfPassive));
        env.close();
        env.require (balance (cam, A_BUX(35)));
        env.require (balance (cam, B_BUX(35)));
        env.require (offers (cam, 1));

//这个提议引起了断言。
        env (offer (cam, B_BUX(30), A_BUX(30)));
        env.close();

        env.require (balance (bob, A_BUX(30)));
        env.require (balance (cam, A_BUX( 5)));
        env.require (balance (cam, B_BUX(65)));
        env.require (offers (cam, 0));
    }

    void testSelfCrossLowQualityOffer (FeatureBitset features)
    {
//用于断言是否发出报价的流报价交叉代码
//比持有的发售账户更多的XRP。本单元测试
//复制那个失败的案例。
        testcase ("Self crossing low quality offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const ann = Account("ann");
        auto const gw = Account("gateway");
        auto const BTC = gw["BTC"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (9999640) + (fee), ann);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (rate(gw, 1.002));
        env (trust(ann, BTC(10)));
        env.close();

        env (pay(gw, ann, BTC(2.856)));
        env.close();

        env (offer(ann, drops(365611702030), BTC(5.713)));
        env.close();

//这个提议引起了断言。
        env (offer(ann,
            BTC(0.687), drops(20000000000)), ter (tecINSUF_RESERVE_OFFER));
    }

    void testOfferInScaling (FeatureBitset features)
    {
//流报价交叉代码有一个不舍入的情况
//报价单在部分交叉后正确交叉。这个
//在网络上找到失败的案例。在这里，我们将案例添加到
//单元测试。
        testcase ("Offer In Scaling");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const CNY = gw["CNY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (trust(bob, CNY(500)));
        env.close();

        env (pay(gw, bob, CNY(300)));
        env.close();

        env (offer(bob, drops(5400000000), CNY(216.054)));
        env.close();

//这个报价没有正确地圆整部分交叉的结果。
        env (offer(alice, CNY(13562.0001), drops(339000000000)));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] == drops (333599446582));
            BEAST_EXPECT(offer[sfTakerPays] == CNY (13345.9461));
        }
    }

    void testOfferInScalingWithXferRate (FeatureBitset features)
    {
//在添加上一个案例后，仍然存在舍入失败的情况。
//流动中的案例提供交叉。这是因为网关
//未正确处理传输速率。
        testcase ("Offer In Scaling With Xfer Rate");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const BTC = gw["BTC"];
        auto const JPY = gw["JPY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw);
        env.close();

        env (rate(gw, 1.002));
        env (trust(alice, JPY(4000)));
        env (trust(bob, BTC(2)));
        env.close();

        env (pay(gw, alice, JPY(3699.034802280317)));
        env (pay(gw, bob, BTC(1.156722559140311)));
        env.close();

        env (offer(bob, JPY(1241.913390770747), BTC(0.01969825690469254)));
        env.close();

//这个报价没有正确地圆整部分交叉的结果。
        env (offer(alice, BTC(0.05507568706427876), JPY(3472.696773391072)));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] ==
                STAmount (JPY.issue(), std::uint64_t(2230682446713524ul), -12));
            BEAST_EXPECT(offer[sfTakerPays] == BTC (0.035378));
        }
    }

    void testOfferThresholdWithReducedFunds (FeatureBitset features)
    {
//另一个例子是流量供给交叉口并不总是
//如果接受方的资金少于报价，就有权采取行动。
//是提供。这个测试的基础来自网络。
        testcase ("Offer Threshold With Reduced Funds");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time () +
                100 * env.closed ()->info ().closeTimeResolution;
        env.close (closeTime);

        auto const gw1 = Account("gw1");
        auto const gw2 = Account("gw2");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const USD = gw1["USD"];
        auto const JPY = gw2["JPY"];

        auto const fee = env.current ()->fees ().base;
        env.fund (reserve(env, 2) + drops (400000000000) + (fee), alice, bob);
        env.fund (reserve(env, 2) + (fee*4), gw1, gw2);
        env.close();

        env (rate(gw1, 1.002));
        env (trust(alice, USD(1000)));
        env (trust(bob, JPY(100000)));
        env.close();

        env (pay(gw1, alice,
            STAmount {USD.issue(), std::uint64_t(2185410179555600), -14}));
        env (pay(gw2, bob,
            STAmount {JPY.issue(), std::uint64_t(6351823459548956), -12}));
        env.close();

        env (offer(bob,
            STAmount {USD.issue(), std::uint64_t(4371257532306000), -17},
            STAmount {JPY.issue(), std::uint64_t(4573216636606000), -15}));
        env.close();

//这个提议没有部分正确地交叉。
        env (offer(alice,
            STAmount {JPY.issue(), std::uint64_t(2291181510070762), -12},
            STAmount {USD.issue(), std::uint64_t(2190218999914694), -14}));
        env.close();

        auto const aliceOffers = offersOnAccount (env, alice);
        BEAST_EXPECT(aliceOffers.size() == 1);
        for (auto const& offerPtr : aliceOffers)
        {
            auto const& offer = *offerPtr;
            BEAST_EXPECT(offer[sfLedgerEntryType] == ltOFFER);
            BEAST_EXPECT(offer[sfTakerGets] ==
                STAmount (USD.issue(), std::uint64_t(2185847305256635), -14));
            BEAST_EXPECT(offer[sfTakerPays] ==
                STAmount (JPY.issue(), std::uint64_t(2286608293434156), -12));
        }
    }

    void testTinyOffer (FeatureBitset features)
    {
        testcase ("Tiny Offer");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const CNY = gw["CNY"];
        auto const fee = env.current()->fees().base;
        auto const startXrpBalance = drops (400000000000) + (fee * 2);

        env.fund (startXrpBalance, gw, alice, bob);
        env.close();

        env (trust (bob, CNY(100000)));
        env.close();

//先把爱丽丝的小提议写在书上。让我们看看会发生什么
//当一个合理的报价通过时。
        STAmount const alicesCnyOffer {
            CNY.issue(), std::uint64_t(4926000000000000), -23 };

        env (offer (alice, alicesCnyOffer, drops (1), tfPassive));
        env.close();

//鲍勃提出了一个普通的报价
        STAmount const bobsCnyStartBalance {
            CNY.issue(), std::uint64_t(3767479960090235), -15};
        env (pay(gw, bob, bobsCnyStartBalance));
        env.close();

        env (offer (bob, drops (203),
            STAmount {CNY.issue(), std::uint64_t(1000000000000000), -20}));
        env.close();

        env.require (balance (alice, alicesCnyOffer));
        env.require (balance (alice, startXrpBalance - fee - drops(1)));
        env.require (balance (bob, bobsCnyStartBalance - alicesCnyOffer));
        env.require (balance (bob, startXrpBalance - (fee * 2) + drops(1)));
    }

    void testSelfPayXferFeeOffer (FeatureBitset features)
    {
        testcase ("Self Pay Xfer Fee");
//旧的报价交叉代码不收取过户费
//如果爱丽丝付钱给爱丽丝。这与付款方式不同。
//即使钱还在，付款也要收取过户费。
//在同一只手上。
//
//爱丽丝付钱给爱丽丝的例子是什么？有三个演员：
//吉布达•伟士、爱丽丝和鲍勃。
//
//1。GW发行BTC和美元。QW收取0.2%的转让费。
//
//2。Alice提出购买XRP并出售美元。
//三。Bob出价购买BTC并出售XRP。
//
//4。Alice现在提出出售BTC并购买美元。
//
//最后一个报价使用自动桥接交叉。
//o Alice最后一次出价是将BTC卖给……
//o Bob的报价，接受Alice的BTC并将XRP出售给……
//o Alice的第一个出价，它接受Bob的xrp并将美元出售给……
//哦，爱丽丝最后的提议。
//
//所以爱丽丝自己卖美元。
//
//我们需要测试六个案例：
//o爱丽丝在第一回合（BTC）中放弃了自己的提议。
//o爱丽丝在第二个回合中通过了自己的报价（美元）。
//o爱丽丝两腿交叉着自己的提议。
//这三种情况都需要测试：
//o反过来（爱丽丝有足够的BTC来支付她的报价）和
//o远期（爱丽丝拥有的BTC比她的最终报价少。
//
//结果发现两个前向案例的失败原因不同
//原因。因此，他们在这里被评论出来，但他们是
//在testselfPayUnlimitedFunds（）单元测试中重新访问。

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const BTC = gw["BTC"];
        auto const USD = gw["USD"];
        auto const startXrpBalance = XRP (4000000);

        env.fund (startXrpBalance, gw);
        env.close();

        env (rate (gw, 1.25));
        env.close();

//测试用例
        struct Actor
        {
            Account acct;
int offers;        //过路后分期付款
PrettyAmount xrp;  //穿越后的最终预期值
PrettyAmount btc;  //穿越后的最终预期值
PrettyAmount usd;  //穿越后的最终预期值
        };
        struct TestData
        {
//前三个整数给出了行动者中的*索引*。
//分配这三个角色中的每一个。通过使用索引
//艾丽丝很容易在第一回合，第二回合中获得报价
//腿，或两者兼而有之。
            std::size_t self;
            std::size_t leg0;
            std::size_t leg1;
            PrettyAmount btcStart;
            std::vector<Actor> actors;
        };

        TestData const tests[]
        {
//BtcStart----------------演员[0]------------------------------------------演员[1]------------
{ 0, 0, 1, BTC(20), { {"ann", 0, drops(3899999999960), BTC(20.0), USD(3000)}, {"abe", 0, drops(4099999999970), BTC( 0), USD(750)} } }, //没有BTC XFER费
{ 0, 1, 0, BTC(20), { {"bev", 0, drops(4099999999960), BTC( 7.5), USD(2000)}, {"bob", 0, drops(3899999999970), BTC(10), USD(  0)} } }, //无美元XFER费
{ 0, 0, 0, BTC(20), { {"cam", 0, drops(3999999999950), BTC(20.0), USD(2000)}                                                      } }, //不收费
//_0，0，1，BTC（5），_“DEB”，0，下降（38999999960），BTC（5.0），美元（3000），“DAN”，0，下降（409999999970），BTC（0），美元（750），//无BTC转换费
{ 0, 1, 0, BTC( 5), { {"eve", 1, drops(4039999999960), BTC( 0.0), USD(2000)}, {"eli", 1, drops(3959999999970), BTC( 4), USD(  0)} } }, //无美元XFER费
//_0，0，0，BTC（5），_“flo”，0，drops（39999999950），BTC（5.0），USD（2000）//无Xfer费用
        };

        for (auto const& t : tests)
        {
            Account const& self = t.actors[t.self].acct;
            Account const& leg0 = t.actors[t.leg0].acct;
            Account const& leg1 = t.actors[t.leg1].acct;

            for (auto const& actor : t.actors)
            {
                env.fund (XRP (4000000), actor.acct);
                env.close();

                env (trust (actor.acct, BTC(40)));
                env (trust (actor.acct, USD(8000)));
                env.close();
            }

            env (pay (gw, self, t.btcStart));
            env (pay (gw, self, USD(2000)));
            if (self.id() != leg1.id())
                env (pay (gw, leg1, USD(2000)));
            env.close();

//准备好最初的报价。记住他们的顺序
//所以我们可以稍后删除它们。
            env (offer (leg0, BTC(10),     XRP(100000), tfPassive));
            env.close();
            std::uint32_t const leg0OfferSeq = env.seq (leg0) - 1;

            env (offer (leg1, XRP(100000), USD(1000),   tfPassive));
            env.close();
            std::uint32_t const leg1OfferSeq = env.seq (leg1) - 1;

//这是最重要的提议。
            env (offer (self, USD(1000), BTC(10)));
            env.close();
            std::uint32_t const selfOfferSeq = env.seq (self) - 1;

//验证结果。
            for (auto const& actor : t.actors)
            {
//有时Taker Crossing会懒得删除报价。
//将空报价视为已删除。
                auto actorOffers = offersOnAccount (env, actor.acct);
                auto const offerCount = std::distance (actorOffers.begin(),
                    std::remove_if (actorOffers.begin(), actorOffers.end(),
                        [] (std::shared_ptr<SLE const>& offer)
                        {
                            return (*offer)[sfTakerGets].signum() == 0;
                        }));
                BEAST_EXPECT (offerCount == actor.offers);

                env.require (balance (actor.acct, actor.xrp));
                env.require (balance (actor.acct, actor.btc));
                env.require (balance (actor.acct, actor.usd));
            }
//删除可能留下的任何报价。他们
//可能会把以后的循环搞混。
            env (offer_cancel (leg0, leg0OfferSeq));
            env.close();
            env (offer_cancel (leg1, leg1OfferSeq));
            env.close();
            env (offer_cancel (self, selfOfferSeq));
            env.close();
        }
    }

    void testSelfPayUnlimitedFunds (FeatureBitset features)
    {
        testcase ("Self Pay Unlimited Funds");
//当爱丽丝付钱的时候，接受者提供的密码是公认的。
//爱丽丝是同一个名字。在这种情况下，只要爱丽丝
//有点像那个教派，它对待爱丽丝就像对待
//她那面额的资金是无限的。
//
//呵呵？这有什么意义？
//
//考虑它的一种方法是将单个付款分解为
//按顺序执行的一系列非常小的付款
//迅速地。爱丽丝需要自己付1美元，但她只有
//0.01美元。爱丽丝说：“嘿，爱丽丝，我给你一分钱。”
//爱丽丝这样做，从口袋里拿出一分钱，然后
//把它放回她的口袋里。然后她说，“嗨，爱丽丝，
//我又找到了一分钱。我可以再给你一分钱。”重复一遍。
//这些步骤100次，爱丽丝已经付了自己1美元，尽管
//她只有0.01美元。
//
//很好，但是付款代码不支持这个
//优化。部分原因是支付代码可以
//执行一整批报价。事实上，它可以
//连续两批发价。这需要一个伟大的
//对两批产品的报价进行分类
//拥有相同的所有者，并给予他们特殊的处理。而且，
//老实说，这是一个奇怪的小角落案件。
//
//因此，由于Flow Offer Crossing使用支付引擎，
//Offer Crossing不再支持此优化。
//
//下面的测试显示了
//承购人提供交叉口，流量提供交叉口。

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const BTC = gw["BTC"];
        auto const USD = gw["USD"];
        auto const startXrpBalance = XRP (4000000);

        env.fund (startXrpBalance, gw);
        env.close();

        env (rate (gw, 1.25));
        env.close();

//测试用例
        struct Actor
        {
            Account acct;
int offers;        //过路后分期付款
PrettyAmount xrp;  //穿越后的最终预期值
PrettyAmount btc;  //穿越后的最终预期值
PrettyAmount usd;  //穿越后的最终预期值
        };
        struct TestData
        {
//前三个整数给出了行动者中的*索引*。
//分配这三个角色中的每一个。通过使用索引
//艾丽丝很容易在第一回合，第二回合中获得报价
//腿，或两者兼而有之。
            std::size_t self;
            std::size_t leg0;
            std::size_t leg1;
            PrettyAmount btcStart;
            std::vector<Actor> actors;
        };

        TestData const takerTests[]
        {
//BtcStart----------------演员[0]-------------------------------------演员[1]-----------------
{ 0, 0, 1, BTC( 5), { {"deb", 0, drops(3899999999960), BTC(5), USD(3000)}, {"dan", 0, drops(4099999999970), BTC(0), USD( 750)} } }, //没有BTC XFER费
{ 0, 0, 0, BTC( 5), { {"flo", 0, drops(3999999999950), BTC(5), USD(2000)}                                                      } }  //不收费
        };

        TestData const flowTests[]
        {
//BtcStart----------------演员[0]-------------------------------------演员[1]-----------------
{ 0, 0, 1, BTC( 5), { {"gay", 1, drops(3949999999960), BTC(5), USD(2500)}, {"gar", 1, drops(4049999999970), BTC(0), USD(1375)} } }, //没有BTC XFER费
{ 0, 0, 0, BTC( 5), { {"hye", 2, drops(3999999999950), BTC(5), USD(2000)}                                                      } }  //不收费
        };

//选择正确的测试。
        auto const& tests = features[featureFlowCross] ? flowTests : takerTests;

        for (auto const& t : tests)
        {
            Account const& self = t.actors[t.self].acct;
            Account const& leg0 = t.actors[t.leg0].acct;
            Account const& leg1 = t.actors[t.leg1].acct;

            for (auto const& actor : t.actors)
            {
                env.fund (XRP (4000000), actor.acct);
                env.close();

                env (trust (actor.acct, BTC(40)));
                env (trust (actor.acct, USD(8000)));
                env.close();
            }

            env (pay (gw, self, t.btcStart));
            env (pay (gw, self, USD(2000)));
            if (self.id() != leg1.id())
                env (pay (gw, leg1, USD(2000)));
            env.close();

//准备好最初的报价。记住他们的顺序
//所以我们可以稍后删除它们。
            env (offer (leg0, BTC(10),     XRP(100000), tfPassive));
            env.close();
            std::uint32_t const leg0OfferSeq = env.seq (leg0) - 1;

            env (offer (leg1, XRP(100000), USD(1000),   tfPassive));
            env.close();
            std::uint32_t const leg1OfferSeq = env.seq (leg1) - 1;

//这是最重要的提议。
            env (offer (self, USD(1000), BTC(10)));
            env.close();
            std::uint32_t const selfOfferSeq = env.seq (self) - 1;

//验证结果。
            for (auto const& actor : t.actors)
            {
//有时taker provide crossing会懒得删除
//提供。将空报价视为已删除。
                auto actorOffers = offersOnAccount (env, actor.acct);
                auto const offerCount = std::distance (actorOffers.begin(),
                    std::remove_if (actorOffers.begin(), actorOffers.end(),
                        [] (std::shared_ptr<SLE const>& offer)
                        {
                            return (*offer)[sfTakerGets].signum() == 0;
                        }));
                BEAST_EXPECT (offerCount == actor.offers);

                env.require (balance (actor.acct, actor.xrp));
                env.require (balance (actor.acct, actor.btc));
                env.require (balance (actor.acct, actor.usd));
            }
//删除可能留下的任何报价。他们
//可能会把以后的循环搞混。
            env (offer_cancel (leg0, leg0OfferSeq));
            env.close();
            env (offer_cancel (leg1, leg1OfferSeq));
            env.close();
            env (offer_cancel (self, selfOfferSeq));
            env.close();
        }
    }

    void testRequireAuth (FeatureBitset features)
    {
        testcase ("lsfRequireAuth");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];
        auto const bobUSD = bob["USD"];

        env.fund (XRP(400000), gw, alice, bob);
        env.close();

//吉布达•伟士要求获得其借据持有人的授权
        env(fset (gw, asfRequireAuth));
        env.close();

//适当设置信任，并让吉布达•伟士授权Bob和Alice
        env (trust (gw, bobUSD(100)), txflags (tfSetfAuth));
        env (trust (bob, gwUSD(100)));
        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env (trust (alice, gwUSD(100)));
//艾丽丝能够提出报价，因为华盛顿大学授权她
        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));
        env.require (balance (alice, gwUSD(0)));

        env (pay(gw, bob, gwUSD(50)));
        env.close();

        env.require (balance (bob, gwUSD(50)));

//鲍勃的提议应该与艾丽丝的相抵。
        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(40)));

        env.require (offers (bob, 0));
        env.require (balance (bob, gwUSD(10)));
    }

    void testMissingAuth (FeatureBitset features)
    {
        testcase ("Missing Auth");
//1。Alice提出收购美元/千兆瓦的要约，该资产
//她没有信托关系。在未来的某个时刻，
//gw增加了lsfrequeuireauth。后来，爱丽丝的提议被否决了。
//与接受者爱丽丝的未经授权的提议被消费。
//b.使用FlowCross Alice的报价将被删除，而不是消费，
//因为Alice无权持有美元/千兆瓦。
//
//2。Alice试图为美元/gw报价，现在gw已经
//lsfrequeuireauth设置。这次报价失败是因为
//Alice无权持有美元/千兆瓦。
//
//三。接下来，吉布达•伟士为Alice建立了一条信任线，但没有设置
//TfsetFauth在该信任行上。Alice试图创建一个
//一次又一次的失败。
//
//4。最后，吉布达•伟士将tsfsetauth设置为信任线授权
//Alice持有美元/千兆瓦。此时爱丽丝成功了
//创建并跨越美元/千兆瓦的报价。

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];
        auto const bobUSD = bob["USD"];

        env.fund (XRP(400000), gw, alice, bob);
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));
        env.require (balance (alice, gwUSD(none)));
        env(fset (gw, asfRequireAuth));
        env.close();

        env (trust (gw, bobUSD(100)), txflags (tfSetfAuth));
        env.close();
        env (trust (bob, gwUSD(100)));
        env.close();

        env (pay(gw, bob, gwUSD(50)));
        env.close();
        env.require (balance (bob, gwUSD(50)));

//吉布达•伟士现在需要授权，Bob拥有GWUSD（50）。让我们看看是否
//鲍勃可以拒绝爱丽丝的提议。
//
//o接受者鲍勃的提议应该超过爱丽丝的。
//o对于Flowcross，Bob的报价不应与Alice的报价交叉。
//应删除未经授权的报价。
        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();
        std::uint32_t const bobOfferSeq = env.seq (bob) - 1;

        bool const flowCross = features[featureFlowCross];

        env.require (offers (alice, 0));
        if (flowCross)
        {
//Alice的未经授权的报价被删除，Bob的报价没有交叉。
            env.require (balance (alice, gwUSD(none)));
            env.require (offers (bob, 1));
            env.require (balance (bob, gwUSD(50)));
        }
        else
        {
//爱丽丝的提议与鲍勃的不一致。
            env.require (balance (alice, gwUSD(40)));
            env.require (offers (bob, 0));
            env.require (balance (bob, gwUSD(10)));

//测试的其余部分将验证FlowCross行为。
            return;
        }

//看看艾丽斯是否可以在没有授权的情况下创建一个报价。爱丽丝
//不能创建报价，而Bob的报价应该是
//未触及的
        env (offer (alice, gwUSD(40), XRP(4000)), ter(tecNO_LINE));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(none)));

        env.require (offers (bob, 1));
        env.require (balance (bob, gwUSD(50)));

//为Alice设置信任线，但不要授权它。爱丽丝
//仍不能提供美元/千兆瓦的报价。
        env (trust (gw, aliceUSD(100)));
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)), ter(tecNO_AUTH));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(0)));

        env.require (offers (bob, 1));
        env.require (balance (bob, gwUSD(50)));

//删除Bob的报价，以便Alice可以创建不交叉的报价。
        env (offer_cancel (bob, bobOfferSeq));
        env.close();
        env.require (offers (bob, 0));

//最后，为Alice建立一个授权的信任线。现在爱丽丝
//提议应该成功。注意，因为这是一个提议
//除了支付，Alice不需要设置信托额度限制。
        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env.close();

        env (offer (alice, gwUSD(40), XRP(4000)));
        env.close();

        env.require (offers (alice, 1));

//现在鲍勃又提出了他的建议。艾丽斯的提议应该是交叉的。
        env (offer (bob, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(40)));

        env.require (offers (bob, 0));
        env.require (balance (bob, gwUSD(10)));
    }

    void testRCSmoketest(FeatureBitset features)
    {
        testcase("RippleConnect Smoketest payment flow");
        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

//此测试模拟Ripple Connect中使用的支付流
//烟雾测试。球员：
//带冷热钱包的美元网关
//带冷热肠的欧洲门户
//提供美元->欧元和欧元->美元报价的MM网关
//找到一条从“热美国”到“冷欧元”的路径，然后用于发送
//通过做市商的欧元兑美元

        auto const hotUS = Account("hotUS");
        auto const coldUS = Account("coldUS");
        auto const hotEU = Account("hotEU");
        auto const coldEU = Account("coldEU");
        auto const mm = Account("mm");

        auto const USD = coldUS["USD"];
        auto const EUR = coldEU["EUR"];

        env.fund (XRP(100000), hotUS, coldUS, hotEU, coldEU, mm);
        env.close();

//冷钱包需要信任，但默认情况下会产生涟漪。
        for (auto const& cold : {coldUS, coldEU})
        {
            env(fset (cold, asfRequireAuth));
            env(fset (cold, asfDefaultRipple));
        }
        env.close();

//每个热钱包大量信任相关的冷钱包
        env (trust(hotUS, USD(10000000)), txflags (tfSetNoRipple));
        env (trust(hotEU, EUR(10000000)), txflags (tfSetNoRipple));
//做市商大量信任这两个冷钱包。
        env (trust(mm, USD(10000000)), txflags (tfSetNoRipple));
        env (trust(mm, EUR(10000000)), txflags (tfSetNoRipple));
        env.close();

//网关授权Hot和Market Maker的信任线
        env (trust (coldUS, USD(0), hotUS, tfSetfAuth));
        env (trust (coldEU, EUR(0), hotEU, tfSetfAuth));
        env (trust (coldUS, USD(0), mm, tfSetfAuth));
        env (trust (coldEU, EUR(0), mm, tfSetfAuth));
        env.close();

//从冷钱包向热和做市商发行货币
        env (pay(coldUS, hotUS, USD(5000000)));
        env (pay(coldEU, hotEU, EUR(5000000)));
        env (pay(coldUS, mm, USD(5000000)));
        env (pay(coldEU, mm, EUR(5000000)));
        env.close();

//mm位置提供
float const rate = 0.9f; //0.9美元= 1欧元
        env (offer(mm,  EUR(4000000 * rate), USD(4000000)),
            json(jss::Flags, tfSell));

        float const reverseRate = 1.0f/rate * 1.00101f;
        env (offer(mm,  USD(4000000 * reverseRate), EUR(4000000)),
            json(jss::Flags, tfSell));
        env.close();

//应该有一条从美国热销到欧洲冷的道路
        {
            Json::Value jvParams;
            jvParams[jss::destination_account] = coldEU.human();
            jvParams[jss::destination_amount][jss::issuer] = coldEU.human();
            jvParams[jss::destination_amount][jss::currency] = "EUR";
            jvParams[jss::destination_amount][jss::value] = 10;
            jvParams[jss::source_account] = hotUS.human();

            Json::Value const jrr {env.rpc(
                "json", "ripple_path_find", to_string(jvParams))[jss::result]};

            BEAST_EXPECT(jrr[jss::status] == "success");
            BEAST_EXPECT(
                jrr[jss::alternatives].isArray() &&
                jrr[jss::alternatives].size() > 0);
        }
//使用找到的路径发送付款。
        env (pay (hotUS, coldEU, EUR(10)), sendmax (USD(11.1223326)));
    }

    void testSelfAuth (FeatureBitset features)
    {
        testcase ("Self Auth");

        using namespace jtx;

        Env env {*this, features};
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gw");
        auto const alice = Account("alice");
        auto const gwUSD = gw["USD"];
        auto const aliceUSD = alice["USD"];

        env.fund (XRP(400000), gw, alice);
        env.close();

//测试一下吉布达•伟士是否能提出购买吉布达•伟士货币的报价。
        env (offer (gw, gwUSD(40), XRP(4000)));
        env.close();
        std::uint32_t const gwOfferSeq = env.seq (gw) - 1;
        env.require (offers (gw, 1));

//由于吉布达•伟士有报价，吉布达•伟士不应设置需求。
        env(fset (gw, asfRequireAuth), ter (tecOWNERS));
        env.close();

//取消吉布达•伟士的报价，这样我们就可以设定要求。
        env (offer_cancel (gw, gwOfferSeq));
        env.close();
        env.require (offers (gw, 0));

//吉布达•伟士现在需要授权其借据持有人
        env(fset (gw, asfRequireAuth));
        env.close();

//测试在有或没有depositprauth的情况下表现不同。
        bool const preauth = features[featureDepositPreauth];

//DepositPreauth之前，设置了lsfrequeuireauth的帐户无法
//创建购买自己货币的报价。存款后
//他们可以。
        env (offer (gw, gwUSD(40), XRP(4000)),
            ter (preauth ? TER {tesSUCCESS} : TER {tecNO_LINE}));
        env.close();

        env.require (offers (gw, preauth ? 1 : 0));

        if (!preauth)
//测试的其余部分验证DepositPrauth行为。
            return;

//设立授权信托额度，支付Alice GWUSD 50。
        env (trust (gw, aliceUSD(100)), txflags (tfSetfAuth));
        env (trust (alice, gwUSD(100)));
        env.close();

        env (pay(gw, alice, gwUSD(50)));
        env.close();

        env.require (balance (alice, gwUSD(50)));

//艾丽斯的报价应该超过吉布达·伟士的
        env (offer (alice, XRP(4000), gwUSD(40)));
        env.close();

        env.require (offers (alice, 0));
        env.require (balance (alice, gwUSD(10)));

        env.require (offers (gw, 0));
    }

    void testTickSize (FeatureBitset features)
    {
        testcase ("Tick Size");

        using namespace jtx;

//应在启用TickSize的情况下调用。
        BEAST_EXPECT(features[featureTickSize]);

//尝试在不启用功能的情况下设置刻度大小
        {
            Env env{*this, features - featureTickSize};
            auto const gw = Account {"gateway"};
            env.fund (XRP(10000), gw);

            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = 0;
            env(txn, ter(temDISABLED));
        }

//尝试将刻度线大小设置为超出范围
        {
            Env env {*this, features};
            auto const gw = Account {"gateway"};
            env.fund (XRP(10000), gw);

            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = Quality::minTickSize - 1;
            env(txn, ter (temBAD_TICK_SIZE));

            txn[sfTickSize.fieldName] = Quality::minTickSize;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize]
                == Quality::minTickSize);

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize;
            env(txn);
            BEAST_EXPECT (! env.le(gw)->isFieldPresent (sfTickSize));

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize - 1;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize]
                == Quality::maxTickSize - 1);

            txn = noop (gw);
            txn[sfTickSize.fieldName] = Quality::maxTickSize + 1;
            env(txn, ter (temBAD_TICK_SIZE));

            txn[sfTickSize.fieldName] = 0;
            env(txn, tesSUCCESS);
            BEAST_EXPECT (! env.le(gw)->isFieldPresent (sfTickSize));
        }

        Env env {*this, features};
        auto const gw = Account {"gateway"};
        auto const alice = Account {"alice"};
        auto const XTS = gw["XTS"];
        auto const XXX = gw["XXX"];

        env.fund (XRP (10000), gw, alice);

        {
//网关将其刻度大小设置为5
            auto txn = noop(gw);
            txn[sfTickSize.fieldName] = 5;
            env(txn);
            BEAST_EXPECT ((*env.le(gw))[sfTickSize] == 5);
        }

        env (trust (alice, XTS (1000)));
        env (trust (alice, XXX (1000)));

        env (pay (gw, alice, alice["XTS"] (100)));
        env (pay (gw, alice, alice["XXX"] (100)));

        env (offer (alice, XTS (10), XXX (30)));
        env (offer (alice, XTS (30), XXX (10)));
        env (offer (alice, XTS (10), XXX (30)),
            json(jss::Flags, tfSell));
        env (offer (alice, XTS (30), XXX (10)),
            json(jss::Flags, tfSell));

        std::map <std::uint32_t, std::pair<STAmount, STAmount>> offers;
        forEachItem (*env.current(), alice,
            [&](std::shared_ptr<SLE const> const& sle)
        {
            if (sle->getType() == ltOFFER)
                offers.emplace((*sle)[sfSequence],
                    std::make_pair((*sle)[sfTakerPays],
                        (*sle)[sfTakerGets]));
        });

//第一要约
        auto it = offers.begin();
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(10) &&
            it->second.second < XXX(30) &&
            it->second.second > XXX(29.9994));

//第二要约
        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(30) &&
            it->second.second == XXX(10));

//第三要约
        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(10.0002) &&
            it->second.second == XXX(30));

//第四要约
//确切的接受者支付XTS（1/.033333）
        ++it;
        BEAST_EXPECT (it != offers.end());
        BEAST_EXPECT (it->second.first == XTS(30) &&
            it->second.second == XXX(10));

        BEAST_EXPECT (++it == offers.end());
    }

    void testAll(FeatureBitset features)
    {
        testCanceledOffer(features);
        testRmFundedOffer(features);
        testTinyPayment(features);
        testXRPTinyPayment(features);
        testEnforceNoRipple(features);
        testInsufficientReserve(features);
        testFillModes(features);
        testMalformed(features);
        testExpiration(features);
        testUnfundedCross(features);
        testSelfCross(false, features);
        testSelfCross(true, features);
        testNegativeBalance(features);
        testOfferCrossWithXRP(true, features);
        testOfferCrossWithXRP(false, features);
        testOfferCrossWithLimitOverride(features);
        testOfferAcceptThenCancel(features);
        testOfferCancelPastAndFuture(features);
        testCurrencyConversionEntire(features);
        testCurrencyConversionIntoDebt(features);
        testCurrencyConversionInParts(features);
        testCrossCurrencyStartXRP(features);
        testCrossCurrencyEndXRP(features);
        testCrossCurrencyBridged(features);
        testBridgedSecondLegDry(features);
        testOfferFeesConsumeFunds(features);
        testOfferCreateThenCross(features);
        testSellFlagBasic(features);
        testSellFlagExceedLimit(features);
        testGatewayCrossCurrency(features);
        testPartialCross (features);
        testXRPDirectCross (features);
        testDirectCross (features);
        testBridgedCross (features);
        testSellOffer (features);
        testSellWithFillOrKill (features);
        testTransferRateOffer(features);
        testSelfCrossOffer (features);
        testSelfIssueOffer (features);
        testBadPathAssert (features);
        testDirectToDirectPath (features);
        testSelfCrossLowQualityOffer (features);
        testOfferInScaling (features);
        testOfferInScalingWithXferRate (features);
        testOfferThresholdWithReducedFunds (features);
        testTinyOffer (features);
        testSelfPayXferFeeOffer (features);
        testSelfPayUnlimitedFunds (features);
        testRequireAuth (features);
        testMissingAuth (features);
        testRCSmoketest (features);
        testSelfAuth (features);
        testTickSize (features | featureTickSize);
    }

    void run () override
    {
        using namespace jtx;
        FeatureBitset const all{supported_amendments()};
        FeatureBitset const f1373{fix1373};
        FeatureBitset const flowCross{featureFlowCross};
        FeatureBitset const takerDryOffer{fixTakerDryOfferRemoval};

        testAll(all - f1373             - takerDryOffer);
        testAll(all         - flowCross - takerDryOffer);
        testAll(all         - flowCross                );
        testAll(all                                    );
    }
};

class Offer_manual_test : public Offer_test
{
    void run() override
    {
        using namespace jtx;
        FeatureBitset const all{supported_amendments()};
        FeatureBitset const flow{featureFlow};
        FeatureBitset const f1373{fix1373};
        FeatureBitset const flowCross{featureFlowCross};
        FeatureBitset const f1513{fix1513};
        FeatureBitset const takerDryOffer{fixTakerDryOfferRemoval};

        testAll(all                - flow - f1373 - flowCross - f1513);
        testAll(all                - flow - f1373 - flowCross        );
        testAll(all                - flow - f1373             - f1513);
        testAll(all                - flow - f1373                    );
        testAll(all                       - f1373 - flowCross - f1513);
        testAll(all                       - f1373 - flowCross        );
        testAll(all                       - f1373             - f1513);
        testAll(all                       - f1373                    );
        testAll(all                               - flowCross - f1513);
        testAll(all                               - flowCross        );
        testAll(all                                           - f1513);
        testAll(all                                                  );

        testAll(all                - flow - f1373 - flowCross - takerDryOffer);
        testAll(all                - flow - f1373             - takerDryOffer);
        testAll(all                       - f1373 - flowCross - takerDryOffer);
    }
};

BEAST_DEFINE_TESTSUITE_PRIO (Offer, tx, ripple, 4);
BEAST_DEFINE_TESTSUITE_MANUAL_PRIO (Offer_manual, tx, ripple, 20);

}  //测试
}  //涟漪
