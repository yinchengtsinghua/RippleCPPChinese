
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

#include <ripple/basics/chrono.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PayChan.h>
#include <ripple/protocol/TxFlags.h>
#include <test/jtx.h>

#include <chrono>

namespace ripple
{
namespace test
{
struct PayChan_test : public beast::unit_test::suite
{
    static
    uint256
    channel (ReadView const& view,
        jtx::Account const& account,
        jtx::Account const& dst)
    {
        auto const sle = view.read (keylet::account (account));
        if (!sle)
            return beast::zero;
        auto const k = keylet::payChan (account, dst, (*sle)[sfSequence] - 1);
        return k.key;
    }

    static Buffer
    signClaimAuth (PublicKey const& pk,
        SecretKey const& sk,
        uint256 const& channel,
        STAmount const& authAmt)
    {
        Serializer msg;
        serializePayChanAuthorization (msg, channel, authAmt.xrp ());
        return sign (pk, sk, msg.slice ());
    }

    static
    STAmount
    channelBalance (ReadView const& view, uint256 const& chan)
    {
        auto const slep = view.read ({ltPAYCHAN, chan});
        if (!slep)
            return XRPAmount{-1};
        return (*slep)[sfBalance];
    }

    static
    bool
    channelExists (ReadView const& view, uint256 const& chan)
    {
        auto const slep = view.read ({ltPAYCHAN, chan});
        return bool(slep);
    }

    static
    STAmount
    channelAmount (ReadView const& view, uint256 const& chan)
    {
        auto const slep = view.read ({ltPAYCHAN, chan});
        if (!slep)
            return XRPAmount{-1};
        return (*slep)[sfAmount];
    }

    static
    boost::optional<std::int64_t>
    channelExpiration (ReadView const& view, uint256 const& chan)
    {
        auto const slep = view.read ({ltPAYCHAN, chan});
        if (!slep)
            return boost::none;
        if (auto const r = (*slep)[~sfExpiration])
            return r.value();
        return boost::none;

    }

    static Json::Value
    create (jtx::Account const& account,
        jtx::Account const& to,
        STAmount const& amount,
        NetClock::duration const& settleDelay,
        PublicKey const& pk,
        boost::optional<NetClock::time_point> const& cancelAfter = boost::none,
        boost::optional<std::uint32_t> const& dstTag = boost::none)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = "PaymentChannelCreate";
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human ();
        jv[jss::Destination] = to.human ();
        jv[jss::Amount] = amount.getJson (0);
        jv["SettleDelay"] = settleDelay.count ();
        jv["PublicKey"] = strHex (pk.slice ());
        if (cancelAfter)
            jv["CancelAfter"] = cancelAfter->time_since_epoch ().count ();
        if (dstTag)
            jv["DestinationTag"] = *dstTag;
        return jv;
    }

    static
    Json::Value
    fund (jtx::Account const& account,
        uint256 const& channel,
        STAmount const& amount,
        boost::optional<NetClock::time_point> const& expiration = boost::none)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = "PaymentChannelFund";
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human ();
        jv["Channel"] = to_string (channel);
        jv[jss::Amount] = amount.getJson (0);
        if (expiration)
            jv["Expiration"] = expiration->time_since_epoch ().count ();
        return jv;
    }

    static
    Json::Value
    claim (jtx::Account const& account,
        uint256 const& channel,
        boost::optional<STAmount> const& balance = boost::none,
        boost::optional<STAmount> const& amount = boost::none,
        boost::optional<Slice> const& signature = boost::none,
        boost::optional<PublicKey> const& pk = boost::none)
    {
        using namespace jtx;
        Json::Value jv;
        jv[jss::TransactionType] = "PaymentChannelClaim";
        jv[jss::Flags] = tfUniversal;
        jv[jss::Account] = account.human ();
        jv["Channel"] = to_string (channel);
        if (amount)
            jv[jss::Amount] = amount->getJson (0);
        if (balance)
            jv["Balance"] = balance->getJson (0);
        if (signature)
            jv["Signature"] = strHex (*signature);
        if (pk)
            jv["PublicKey"] = strHex (pk->slice ());
        return jv;
    }

    void
    testSimple ()
    {
        testcase ("simple");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto USDA = alice["USD"];
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 100s;
        env (create (alice, bob, XRP (1000), settleDelay, pk));
        auto const chan = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelBalance (*env.current (), chan) == XRP (0));
        BEAST_EXPECT (channelAmount (*env.current (), chan) == XRP (1000));

        {
            auto const preAlice = env.balance (alice);
            env (fund (alice, chan, XRP (1000)));
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (alice) == preAlice - XRP (1000) - feeDrops);
        }

        auto chanBal = channelBalance (*env.current (), chan);
        auto chanAmt = channelAmount (*env.current (), chan);
        BEAST_EXPECT (chanBal == XRP (0));
        BEAST_EXPECT (chanAmt == XRP (2000));

        {
//不良金额（非XRP，负金额）
            env (create (alice, bob, USDA (1000), settleDelay, pk),
                ter (temBAD_AMOUNT));
            env (fund (alice, chan, USDA (1000)),
                ter (temBAD_AMOUNT));
            env (create (alice, bob, XRP (-1000), settleDelay, pk),
                ter (temBAD_AMOUNT));
            env (fund (alice, chan, XRP (-1000)),
                ter (temBAD_AMOUNT));
        }

//无效帐户
        env (create (alice, "noAccount", XRP (1000), settleDelay, pk),
            ter (tecNO_DST));
//无法创建同一帐户的频道
        env (create (alice, alice, XRP (1000), settleDelay, pk),
             ter (temDST_IS_SRC));
//无效通道
        env (fund (alice, channel (*env.current (), alice, "noAccount"), XRP (1000)),
            ter (tecNO_ENTRY));
//资金不足
        env (create (alice, bob, XRP (10000), settleDelay, pk),
            ter (tecUNFUNDED));

        {
//无不良金额的签名索赔（负数和非XRP）
            auto const iou = USDA (100).value ();
            auto const negXRP = XRP (-100).value ();
            auto const posXRP = XRP (100).value ();
            env (claim (alice, chan, iou, iou), ter (temBAD_AMOUNT));
            env (claim (alice, chan, posXRP, iou), ter (temBAD_AMOUNT));
            env (claim (alice, chan, iou, posXRP), ter (temBAD_AMOUNT));
            env (claim (alice, chan, negXRP, negXRP), ter (temBAD_AMOUNT));
            env (claim (alice, chan, posXRP, negXRP), ter (temBAD_AMOUNT));
            env (claim (alice, chan, negXRP, posXRP), ter (temBAD_AMOUNT));
        }
        {
//签名声明不得超过授权
            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            auto const authAmt = reqBal + XRP (-100);
            assert (reqBal <= chanAmt);
            env (claim (alice, chan, reqBal, authAmt), ter (temBAD_AMOUNT));
        }
        {
//无需签名，因为业主要求
            auto const preBob = env.balance (bob);
            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            auto const authAmt = reqBal + XRP (100);
            assert (reqBal <= chanAmt);
            env (claim (alice, chan, reqBal, authAmt));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == reqBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
            BEAST_EXPECT (env.balance (bob) == preBob + delta);
            chanBal = reqBal;
        }
        {
//带签名的索赔
            auto preBob = env.balance (bob);
            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            auto const authAmt = reqBal + XRP (100);
            assert (reqBal <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
            env (claim (bob, chan, reqBal, authAmt, Slice (sig), alice.pk ()));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == reqBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (bob) == preBob + delta - feeDrops);
            chanBal = reqBal;

//再次索赔
            preBob = env.balance (bob);
            env (claim (bob, chan, reqBal, authAmt, Slice (sig), alice.pk ()),
                ter (tecUNFUNDED_PAYMENT));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == chanBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
            BEAST_EXPECT (env.balance (bob) == preBob - feeDrops);
        }
        {
//尝试索赔超过授权
            auto const preBob = env.balance (bob);
            STAmount const authAmt = chanBal + XRP (500);
            STAmount const reqAmt = authAmt + 1;
            assert (reqAmt <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
            env (claim (bob, chan, reqAmt, authAmt, Slice (sig), alice.pk ()),
                ter (temBAD_AMOUNT));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == chanBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
            BEAST_EXPECT (env.balance (bob) == preBob);
        }

//DST试图为渠道提供资金
        env (fund (bob, chan, XRP (1000)), ter (tecNO_PERMISSION));
        BEAST_EXPECT (channelBalance (*env.current (), chan) == chanBal);
        BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);

        {
//签名密钥错误
            auto const sig =
                signClaimAuth (bob.pk (), bob.sk (), chan, XRP (1500));
            env (claim (bob, chan, XRP (1500).value (), XRP (1500).value (),
                     Slice (sig), bob.pk ()),
                ter (temBAD_SIGNER));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == chanBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
        }
        {
//不良签名
            auto const sig =
                signClaimAuth (bob.pk (), bob.sk (), chan, XRP (1500));
            env (claim (bob, chan, XRP (1500).value (), XRP (1500).value (),
                     Slice (sig), alice.pk ()),
                ter (temBAD_SIGNATURE));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == chanBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
        }
        {
//DST关闭通道
            auto const preAlice = env.balance (alice);
            auto const preBob = env.balance (bob);
            env (claim (bob, chan), txflags (tfClose));
            BEAST_EXPECT (!channelExists (*env.current (), chan));
            auto const feeDrops = env.current ()->fees ().base;
            auto const delta = chanAmt - chanBal;
            assert (delta > beast::zero);
            BEAST_EXPECT (env.balance (alice) == preAlice + delta);
            BEAST_EXPECT (env.balance (bob) == preBob - feeDrops);
        }
    }

    void
    testCancelAfter ()
    {
        testcase ("cancel after");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        {
//如果DST在取消后声明，则通道关闭
            Env env (*this);
            env.fund (XRP (10000), alice, bob);
            auto const pk = alice.pk ();
            auto const settleDelay = 100s;
            NetClock::time_point const cancelAfter =
                env.current ()->info ().parentCloseTime + 3600s;
            auto const channelFunds = XRP (1000);
            env (create (
                alice, bob, channelFunds, settleDelay, pk, cancelAfter));
            auto const chan = channel (*env.current (), alice, bob);
            if (!chan)
            {
                fail ();
                return;
            }
            BEAST_EXPECT (channelExists (*env.current (), chan));
            env.close (cancelAfter);
            {
//在取消之后DST不能声明
                auto const chanBal = channelBalance (*env.current (), chan);
                auto const chanAmt = channelAmount (*env.current (), chan);
                auto preAlice = env.balance (alice);
                auto preBob = env.balance (bob);
                auto const delta = XRP (500);
                auto const reqBal = chanBal + delta;
                auto const authAmt = reqBal + XRP (100);
                assert (reqBal <= chanAmt);
                auto const sig =
                    signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
                env (claim (
                    bob, chan, reqBal, authAmt, Slice (sig), alice.pk ()));
                auto const feeDrops = env.current ()->fees ().base;
                BEAST_EXPECT (!channelExists (*env.current (), chan));
                BEAST_EXPECT (env.balance (bob) == preBob - feeDrops);
                BEAST_EXPECT (env.balance (alice) == preAlice + channelFunds);
            }
        }
        {
//取消后第三方可以关闭
            Env env (*this);
            env.fund (XRP (10000), alice, bob, carol);
            auto const pk = alice.pk ();
            auto const settleDelay = 100s;
            NetClock::time_point const cancelAfter =
                env.current ()->info ().parentCloseTime + 3600s;
            auto const channelFunds = XRP (1000);
            env (create (
                alice, bob, channelFunds, settleDelay, pk, cancelAfter));
            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (channelExists (*env.current (), chan));
//取消前第三方关闭
            env (claim (carol, chan), txflags (tfClose),
                ter (tecNO_PERMISSION));
            BEAST_EXPECT (channelExists (*env.current (), chan));
            env.close (cancelAfter);
//取消后第三方关闭
            auto const preAlice = env.balance (alice);
            env (claim (carol, chan), txflags (tfClose));
            BEAST_EXPECT (!channelExists (*env.current (), chan));
            BEAST_EXPECT (env.balance (alice) == preAlice + channelFunds);
        }
    }

    void
    testExpiration ()
    {
        testcase ("expiration");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        env.fund (XRP (10000), alice, bob, carol);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const closeTime = env.current ()->info ().parentCloseTime;
        auto const minExpiration = closeTime + settleDelay;
        NetClock::time_point const cancelAfter = closeTime + 7200s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk, cancelAfter));
        auto const chan = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan));
        BEAST_EXPECT (!channelExpiration (*env.current (), chan));
//业主关闭，在结算延迟后关闭
        env (claim (alice, chan), txflags (tfClose));
        auto counts = [](
            auto const& t) { return t.time_since_epoch ().count (); };
        BEAST_EXPECT (*channelExpiration (*env.current (), chan) ==
            counts (minExpiration));
//增加到期时间
        env (fund (
            alice, chan, XRP (1), NetClock::time_point{minExpiration + 100s}));
        BEAST_EXPECT (*channelExpiration (*env.current (), chan) ==
            counts (minExpiration) + 100);
//减少到期时间，但仍高于我的到期时间
        env (fund (
            alice, chan, XRP (1), NetClock::time_point{minExpiration + 50s}));
        BEAST_EXPECT (*channelExpiration (*env.current (), chan) ==
            counts (minExpiration) + 50);
//在minexpiration以下减少过期时间
        env (fund (alice, chan, XRP (1),
                 NetClock::time_point{minExpiration - 50s}),
            ter (temBAD_EXPIRATION));
        BEAST_EXPECT (*channelExpiration (*env.current (), chan) ==
            counts (minExpiration) + 50);
        env (claim (bob, chan), txflags (tfRenew), ter (tecNO_PERMISSION));
        BEAST_EXPECT (*channelExpiration (*env.current (), chan) ==
            counts (minExpiration) + 50);
        env (claim (alice, chan), txflags (tfRenew));
        BEAST_EXPECT (!channelExpiration (*env.current (), chan));
//在minexpiration以下减少过期时间
        env (fund (alice, chan, XRP (1),
                 NetClock::time_point{minExpiration - 50s}),
            ter (temBAD_EXPIRATION));
        BEAST_EXPECT (!channelExpiration (*env.current (), chan));
        env (fund (alice, chan, XRP (1), NetClock::time_point{minExpiration}));
        env.close (minExpiration);
//在过期之后尝试延长过期时间
        env (fund (
            alice, chan, XRP (1), NetClock::time_point{minExpiration + 1000s}));
        BEAST_EXPECT (!channelExists (*env.current (), chan));
    }

    void
    testSettleDelay ()
    {
        testcase ("settle delay");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        NetClock::time_point const settleTimepoint =
            env.current ()->info ().parentCloseTime + settleDelay;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk));
        auto const chan = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan));
//业主关闭，在结算延迟后关闭
        env (claim (alice, chan), txflags (tfClose));
        BEAST_EXPECT (channelExists (*env.current (), chan));
        env.close (settleTimepoint-settleDelay/2);
        {
//接收者仍可以索赔
            auto const chanBal = channelBalance (*env.current (), chan);
            auto const chanAmt = channelAmount (*env.current (), chan);
            auto preBob = env.balance (bob);
            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            auto const authAmt = reqBal + XRP (100);
            assert (reqBal <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
            env (claim (bob, chan, reqBal, authAmt, Slice (sig), alice.pk ()));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == reqBal);
            BEAST_EXPECT (channelAmount (*env.current (), chan) == chanAmt);
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (bob) == preBob + delta - feeDrops);
        }
        env.close (settleTimepoint);
        {
//过去的沉淀时间，通道将关闭
            auto const chanBal = channelBalance (*env.current (), chan);
            auto const chanAmt = channelAmount (*env.current (), chan);
            auto const preAlice = env.balance (alice);
            auto preBob = env.balance (bob);
            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            auto const authAmt = reqBal + XRP (100);
            assert (reqBal <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
            env (claim (bob, chan, reqBal, authAmt, Slice (sig), alice.pk ()));
            BEAST_EXPECT (!channelExists (*env.current (), chan));
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (alice) == preAlice + chanAmt - chanBal);
            BEAST_EXPECT (env.balance (bob) == preBob - feeDrops);
        }
    }

    void
    testCloseDry ()
    {
        testcase ("close dry");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk));
        auto const chan = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan));
//所有者试图关闭频道，但它将保持打开（解决延迟）
        env (claim (alice, chan), txflags (tfClose));
        BEAST_EXPECT (channelExists (*env.current (), chan));
        {
//索要全部金额
            auto const preBob = env.balance (bob);
            env (claim (
                alice, chan, channelFunds.value (), channelFunds.value ()));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == channelFunds);
            BEAST_EXPECT (env.balance (bob) == preBob + channelFunds);
        }
        auto const preAlice = env.balance (alice);
//通道现在干燥，可以在到期日期前关闭
        env (claim (alice, chan), txflags (tfClose));
        BEAST_EXPECT (!channelExists (*env.current (), chan));
        auto const feeDrops = env.current ()->fees ().base;
        BEAST_EXPECT (env.balance (alice) == preAlice - feeDrops);
    }

    void
    testDefaultAmount ()
    {
//授权金额默认为余额（如果不存在）
        testcase ("default amount");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk));
        auto const chan = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan));
//所有者试图关闭频道，但它将保持打开（解决延迟）
        env (claim (alice, chan), txflags (tfClose));
        BEAST_EXPECT (channelExists (*env.current (), chan));
        {
            auto chanBal = channelBalance (*env.current (), chan);
            auto chanAmt = channelAmount (*env.current (), chan);
            auto const preBob = env.balance (bob);

            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            assert (reqBal <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, reqBal);
            env (claim (
                bob, chan, reqBal, boost::none, Slice (sig), alice.pk ()));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == reqBal);
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (bob) == preBob + delta - feeDrops);
            chanBal = reqBal;
        }
        {
//再次索赔
            auto chanBal = channelBalance (*env.current (), chan);
            auto chanAmt = channelAmount (*env.current (), chan);
            auto const preBob = env.balance (bob);

            auto const delta = XRP (500);
            auto const reqBal = chanBal + delta;
            assert (reqBal <= chanAmt);
            auto const sig =
                signClaimAuth (alice.pk (), alice.sk (), chan, reqBal);
            env (claim (
                bob, chan, reqBal, boost::none, Slice (sig), alice.pk ()));
            BEAST_EXPECT (channelBalance (*env.current (), chan) == reqBal);
            auto const feeDrops = env.current ()->fees ().base;
            BEAST_EXPECT (env.balance (bob) == preBob + delta - feeDrops);
            chanBal = reqBal;
        }
    }

    void
    testDisallowXRP ()
    {
//授权金额默认为余额（如果不存在）
        testcase ("Disallow XRP");
        using namespace jtx;
        using namespace std::literals::chrono_literals;

        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        {
//创建一个DST不允许xrp的通道
            Env env (*this, supported_amendments() - featureDepositAuth);
            env.fund (XRP (10000), alice, bob);
            env (fset (bob, asfDisallowXRP));
            env (create (alice, bob, XRP (1000), 3600s, alice.pk()),
                ter (tecNO_TARGET));
            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (!channelExists (*env.current (), chan));
        }
        {
//创建一个DST不允许XRP的通道。忽略那个标志，
//因为这只是个建议。
            Env env (*this,  supported_amendments());
            env.fund (XRP (10000), alice, bob);
            env (fset (bob, asfDisallowXRP));
            env (create (alice, bob, XRP (1000), 3600s, alice.pk()));
            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (channelExists (*env.current (), chan));
        }

        {
//对DST不允许xrp的通道的声明
//（在设置不允许xrp之前创建通道）
            Env env (*this, supported_amendments() - featureDepositAuth);
            env.fund (XRP (10000), alice, bob);
            env (create (alice, bob, XRP (1000), 3600s, alice.pk()));
            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (channelExists (*env.current (), chan));

            env (fset (bob, asfDisallowXRP));
            auto const reqBal = XRP (500).value();
            env (claim (alice, chan, reqBal, reqBal), ter(tecNO_TARGET));
        }
        {
//对DST不允许xrp（通道为
//在设置不允许xrp之前创建）。忽略那个标志
//因为这只是个建议。
            Env env (*this, supported_amendments());
            env.fund (XRP (10000), alice, bob);
            env (create (alice, bob, XRP (1000), 3600s, alice.pk()));
            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (channelExists (*env.current (), chan));

            env (fset (bob, asfDisallowXRP));
            auto const reqBal = XRP (500).value();
            env (claim (alice, chan, reqBal, reqBal));
        }
    }

    void
    testDstTag ()
    {
//授权金额默认为余额（如果不存在）
        testcase ("Dst Tag");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
//创建一个DST不允许xrp的通道
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        env (fset (bob, asfRequireDest));
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk),
            ter (tecDST_TAG_NEEDED));
        BEAST_EXPECT (!channelExists (
            *env.current (), channel (*env.current (), alice, bob)));
        env (
            create (alice, bob, channelFunds, settleDelay, pk, boost::none, 1));
        BEAST_EXPECT (channelExists (
            *env.current (), channel (*env.current (), alice, bob)));
    }

    void
    testDepositAuth ()
    {
        testcase ("Deposit Authorization");
        using namespace jtx;
        using namespace std::literals::chrono_literals;

        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        auto USDA = alice["USD"];
        {
            Env env (*this);
            env.fund (XRP (10000), alice, bob, carol);

            env (fset (bob, asfDepositAuth));
            env.close();

            auto const pk = alice.pk ();
            auto const settleDelay = 100s;
            env (create (alice, bob, XRP (1000), settleDelay, pk));
            env.close();

            auto const chan = channel (*env.current (), alice, bob);
            BEAST_EXPECT (channelBalance (*env.current (), chan) == XRP (0));
            BEAST_EXPECT (channelAmount (*env.current (), chan) == XRP (1000));

//爱丽丝可以给频道增加更多的资金，即使鲍勃
//ASFDepositauth集。
            env (fund (alice, chan, XRP (1000)));
            env.close();

//爱丽丝声称。失败，因为已设置Bob的lsfDepositAuth标志。
            env (claim (alice, chan,
                XRP (500).value(), XRP (500).value()), ter (tecNO_PERMISSION));
            env.close();

//带签名的索赔
            auto const baseFee = env.current()->fees().base;
            auto const preBob = env.balance (bob);
            {
                auto const delta = XRP (500).value();
                auto const sig = signClaimAuth (pk, alice.sk (), chan, delta);

//艾丽斯要求签名。因为鲍勃失败了
//lsfdepositauth标志集。
                env (claim (alice, chan,
                    delta, delta, Slice (sig), pk), ter (tecNO_PERMISSION));
                env.close();
                BEAST_EXPECT (env.balance (bob) == preBob);

//鲍勃声称，但省略了签名。失败是因为
//艾丽斯可以不经签字就提出索赔。
                env (claim (bob, chan, delta, delta), ter (temBAD_SIGNATURE));
                env.close();

//鲍勃签名认领。即使鲍勃的
//lsfdepositauth标志自Bob提交
//交易。
                env (claim (bob, chan, delta, delta, Slice (sig), pk));
                env.close();
                BEAST_EXPECT (env.balance (bob) == preBob + delta - baseFee);
            }
            {
//探索存款预授权的范围。
                auto const delta = XRP (600).value();
                auto const sig = signClaimAuth (pk, alice.sk (), chan, delta);

//卡罗尔声称失败了。只有渠道参与者（Bob或
//爱丽丝）可以要求。
                env (claim (carol, chan,
                    delta, delta, Slice (sig), pk), ter (tecNO_PERMISSION));
                env.close();

//鲍勃预先授权卡罗尔存款。但在那之后，卡罗尔
//由于只有渠道参与者可以声明，所以仍然无法声明。
                env(deposit::auth (bob, carol));
                env.close();

                env (claim (carol, chan,
                    delta, delta, Slice (sig), pk), ter (tecNO_PERMISSION));

//既然爱丽丝没有被预先授权，她也不能声称
//为了鲍伯。
                env (claim (alice, chan, delta, delta,
                    Slice (sig), pk), ter (tecNO_PERMISSION));
                env.close();

//但是，如果鲍勃预先授权爱丽丝存钱，那么她可以
//成功提交索赔。
                env(deposit::auth (bob, alice));
                env.close();

                env (claim (alice, chan, delta, delta, Slice (sig), pk));
                env.close();

                BEAST_EXPECT (
                    env.balance (bob) == preBob + delta - (3 * baseFee));
            }
            {
//鲍勃取消了爱丽丝的预授权。她再一次
//无法提交索赔。
                auto const delta = XRP (800).value();

                env(deposit::unauth (bob, alice));
                env.close();

//艾丽斯声称，但由于她不再被预先授权而失败了。
                env (claim (alice, chan, delta, delta), ter (tecNO_PERMISSION));
                env.close();

//Bob清除lsfdepositauth。现在爱丽丝可以认领了。
                env (fclear (bob, asfDepositAuth));
                env.close();

//爱丽丝声称成功。
                env (claim (alice, chan, delta, delta));
                env.close();
                BEAST_EXPECT (
                    env.balance (bob) == preBob + XRP (800) - (5 * baseFee));
            }
        }
    }

    void
    testMultiple ()
    {
//授权金额默认为余额（如果不存在）
        testcase ("Multiple channels to the same account");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk));
        auto const chan1 = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan1));
        env (create (alice, bob, channelFunds, settleDelay, pk));
        auto const chan2 = channel (*env.current (), alice, bob);
        BEAST_EXPECT (channelExists (*env.current (), chan2));
        BEAST_EXPECT (chan1 != chan2);
    }

    void
    testRPC ()
    {
        testcase ("RPC");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);
        env (create (alice, bob, channelFunds, settleDelay, pk));
        env.close();
        auto const chan1Str = to_string (channel (*env.current (), alice, bob));
        std::string chan1PkStr;
        {
            auto const r =
                env.rpc ("account_channels", alice.human (), bob.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 1);
            BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::channel_id] == chan1Str);
            BEAST_EXPECT (r[jss::result][jss::validated]);
            chan1PkStr = r[jss::result][jss::channels][0u][jss::public_key].asString();
        }
        {
            auto const r =
                env.rpc ("account_channels", alice.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 1);
            BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::channel_id] == chan1Str);
            BEAST_EXPECT (r[jss::result][jss::validated]);
            chan1PkStr = r[jss::result][jss::channels][0u][jss::public_key].asString();
        }
        {
            auto const r =
                env.rpc ("account_channels", bob.human (), alice.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 0);
            BEAST_EXPECT (r[jss::result][jss::validated]);
        }
        env (create (alice, bob, channelFunds, settleDelay, pk));
        env.close();
        auto const chan2Str = to_string (channel (*env.current (), alice, bob));
        {
            auto const r =
                    env.rpc ("account_channels", alice.human (), bob.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 2);
            BEAST_EXPECT (r[jss::result][jss::validated]);
            BEAST_EXPECT (chan1Str != chan2Str);
            for (auto const& c : {chan1Str, chan2Str})
                BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::channel_id] == c ||
                        r[jss::result][jss::channels][1u][jss::channel_id] == c);
        }

        auto sliceToHex = [](Slice const& slice) {
            std::string s;
            s.reserve(2 * slice.size());
            for (int i = 0; i < slice.size(); ++i)
            {
                s += "0123456789ABCDEF"[((slice[i] & 0xf0) >> 4)];
                s += "0123456789ABCDEF"[((slice[i] & 0x0f) >> 0)];
            }
            return s;
        };

        {
//验证通道1身份验证
            auto const rs =
                env.rpc ("channel_authorize", "alice", chan1Str, "1000");
            auto const sig = rs[jss::result][jss::signature].asString ();
            BEAST_EXPECT (!sig.empty ());
            {
                auto const rv = env.rpc(
                    "channel_verify", chan1PkStr, chan1Str, "1000", sig);
                BEAST_EXPECT(rv[jss::result][jss::signature_verified].asBool());
            }

            {
//使用pk hex验证
                auto const pkAsHex = sliceToHex(pk.slice());
                auto const rv = env.rpc (
                    "channel_verify", pkAsHex, chan1Str, "1000", sig);
                BEAST_EXPECT (rv[jss::result][jss::signature_verified].asBool ());
            }
            {
//金额不正确
                auto const pkAsHex = sliceToHex(pk.slice());
                auto rv =
                    env.rpc("channel_verify", pkAsHex, chan1Str, "1000x", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "1000 ", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "x1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "x", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, " ", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "1000 1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "1,000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, " 1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
                rv = env.rpc("channel_verify", pkAsHex, chan1Str, "", sig);
                BEAST_EXPECT(rv[jss::error] == "channelAmtMalformed");
            }
            {
//畸形通道
                auto const pkAsHex = sliceToHex(pk.slice());
                auto chan1StrBad = chan1Str;
                chan1StrBad.pop_back();
                auto rv = env.rpc("channel_verify", pkAsHex, chan1StrBad, "1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");
                rv = env.rpc ("channel_authorize", "alice", chan1StrBad, "1000");
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");

                chan1StrBad = chan1Str;
                chan1StrBad.push_back('0');
                rv = env.rpc("channel_verify", pkAsHex, chan1StrBad, "1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");
                rv = env.rpc ("channel_authorize", "alice", chan1StrBad, "1000");
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");

                chan1StrBad = chan1Str;
                chan1StrBad.back() = 'x';
                rv = env.rpc("channel_verify", pkAsHex, chan1StrBad, "1000", sig);
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");
                rv = env.rpc ("channel_authorize", "alice", chan1StrBad, "1000");
                BEAST_EXPECT(rv[jss::error] == "channelMalformed");
            }
            {
//给出一个格式错误的基58公钥
                auto illFormedPk = chan1PkStr.substr(0, chan1PkStr.size() - 1);
                auto const rv = env.rpc(
                    "channel_verify", illFormedPk, chan1Str, "1000", sig);
                BEAST_EXPECT(!rv[jss::result][jss::signature_verified].asBool());
            }
            {
//给出格式错误的十六进制公钥
                auto const pkAsHex = sliceToHex(pk.slice());
                auto illFormedPk = pkAsHex.substr(0, chan1PkStr.size() - 1);
                auto const rv = env.rpc(
                    "channel_verify", illFormedPk, chan1Str, "1000", sig);
                BEAST_EXPECT(!rv[jss::result][jss::signature_verified].asBool());
            }
        }
        {
//尝试用chan1键验证chan2 auth
            auto const rs =
                env.rpc ("channel_authorize", "alice", chan2Str, "1000");
            auto const sig = rs[jss::result][jss::signature].asString ();
            BEAST_EXPECT (!sig.empty ());
            {
                auto const rv = env.rpc(
                    "channel_verify", chan1PkStr, chan1Str, "1000", sig);
                BEAST_EXPECT(
                    !rv[jss::result][jss::signature_verified].asBool());
            }
            {
//使用pk hex验证
                auto const pkAsHex = sliceToHex(pk.slice());
                auto const rv = env.rpc(
                    "channel_verify", pkAsHex, chan1Str, "1000", sig);
                BEAST_EXPECT(
                    !rv[jss::result][jss::signature_verified].asBool());
            }
        }
        {
//发送错误数量的RPC请求
            auto rs = env.rpc("channel_authorize", "alice", chan1Str, "1000x");
            BEAST_EXPECT(rs[jss::error] == "channelAmtMalformed");
            rs = env.rpc("channel_authorize", "alice", chan1Str, "x1000");
            BEAST_EXPECT(rs[jss::error] == "channelAmtMalformed");
            rs = env.rpc("channel_authorize", "alice", chan1Str, "x");
            BEAST_EXPECT(rs[jss::error] == "channelAmtMalformed");
        }
    }

    void
    testOptionalFields ()
    {
        testcase ("Optional Fields");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto const carol = Account ("carol");
        auto const dan = Account ("dan");
        env.fund (XRP (10000), alice, bob, carol, dan);
        auto const pk = alice.pk ();
        auto const settleDelay = 3600s;
        auto const channelFunds = XRP (1000);

        boost::optional<NetClock::time_point> cancelAfter;

        {
            env (create (alice, bob, channelFunds, settleDelay, pk));
            auto const chan = to_string (channel (*env.current (), alice, bob));
            auto const r =
                env.rpc ("account_channels", alice.human (), bob.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 1);
            BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::channel_id] == chan);
            BEAST_EXPECT (!r[jss::result][jss::channels][0u].isMember(jss::destination_tag));
        }
        {
            std::uint32_t dstTag=42;
            env (create (
                alice, carol, channelFunds, settleDelay, pk, cancelAfter, dstTag));
            auto const chan = to_string (channel (*env.current (), alice, carol));
            auto const r =
                env.rpc ("account_channels", alice.human (), carol.human ());
            BEAST_EXPECT (r[jss::result][jss::channels].size () == 1);
            BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::channel_id] == chan);
            BEAST_EXPECT (r[jss::result][jss::channels][0u][jss::destination_tag] == dstTag);
        }
    }

    void
    testMalformedPK ()
    {
        testcase ("malformed pk");
        using namespace jtx;
        using namespace std::literals::chrono_literals;
        Env env (*this);
        auto const alice = Account ("alice");
        auto const bob = Account ("bob");
        auto USDA = alice["USD"];
        env.fund (XRP (10000), alice, bob);
        auto const pk = alice.pk ();
        auto const settleDelay = 100s;

        auto jv = create (alice, bob, XRP (1000), settleDelay, pk);
        auto const pkHex = strHex (pk.slice ());
        jv["PublicKey"] = pkHex.substr(2, pkHex.size()-2);
        env (jv, ter(temMALFORMED));
        jv["PublicKey"] = pkHex.substr(0, pkHex.size()-2);
        env (jv, ter(temMALFORMED));
        auto badPrefix = pkHex;
        badPrefix[0]='f';
        badPrefix[1]='f';
        jv["PublicKey"] = badPrefix;
        env (jv, ter(temMALFORMED));

        jv["PublicKey"] = pkHex;
        env (jv);
        auto const chan = channel (*env.current (), alice, bob);

        auto const authAmt = XRP (100);
        auto const sig = signClaimAuth (alice.pk (), alice.sk (), chan, authAmt);
        jv = claim(bob, chan, authAmt.value(), authAmt.value(), Slice(sig), alice.pk());
        jv["PublicKey"] = pkHex.substr(2, pkHex.size()-2);
        env (jv, ter(temMALFORMED));
        jv["PublicKey"] = pkHex.substr(0, pkHex.size()-2);
        env (jv, ter(temMALFORMED));
        badPrefix = pkHex;
        badPrefix[0]='f';
        badPrefix[1]='f';
        jv["PublicKey"] = badPrefix;
        env (jv, ter(temMALFORMED));

//缺少公钥
        jv.removeMember("PublicKey");
        env (jv, ter(temMALFORMED));

        {
            auto const txn = R"*(
        {

        "channel_id":"5DB01B7FFED6B67E6B0414DED11E051D2EE2B7619CE0EAA6286D67A3A4D5BDB3",
                "signature":
        "304402204EF0AFB78AC23ED1C472E74F4299C0C21F1B21D07EFC0A3838A420F76D783A400220154FB11B6F54320666E4C36CA7F686C16A3A0456800BBC43746F34AF50290064",
                "public_key":
        "aKijDDiC2q2gXjMpM7i4BUS6cmixgsEe18e7CjsUxwihKfuoFgS5",
                "amount": "1000000"
            }
        )*";
            auto const r = env.rpc("json", "channel_verify", txn);
            BEAST_EXPECT(r["result"]["error"] == "publicMalformed");
        }
    }

    void
    run () override
    {
        testSimple ();
        testCancelAfter ();
        testSettleDelay ();
        testExpiration ();
        testCloseDry ();
        testDefaultAmount ();
        testDisallowXRP ();
        testDstTag ();
        testDepositAuth ();
        testMultiple ();
        testRPC ();
        testOptionalFields ();
        testMalformedPK ();
    }
};

BEAST_DEFINE_TESTSUITE (PayChan, app, ripple);
}  //测试
}  //涟漪
