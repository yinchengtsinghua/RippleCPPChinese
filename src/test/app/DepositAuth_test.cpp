
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
    版权所有（c）2017 Ripple Labs Inc.

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
#include <test/jtx.h>

namespace ripple {
namespace test {

//帮助程序函数，该函数基于
//传入的所有者数。
static XRPAmount reserve (jtx::Env& env, std::uint32_t count)
{
    return env.current()->fees().accountReserve (count);
}

//如果acct设置了lsfdeposauth标志，则返回true的helper函数。
static bool hasDepositAuth (jtx::Env const& env, jtx::Account const& acct)
{
    return ((*env.le(acct))[sfFlags] & lsfDepositAuth) == lsfDepositAuth;
}


struct DepositAuth_test : public beast::unit_test::suite
{
    void testEnable()
    {
        testcase ("Enable");

        using namespace jtx;
        Account const alice {"alice"};

        {
//已禁用FeatureDepositAuth。
            Env env (*this, supported_amendments() - featureDepositAuth);
            env.fund (XRP (10000), alice);

//请注意，为了支持旧的行为，将忽略无效的标志。
            env (fset (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));

            env (fclear (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));
        }
        {
//已启用FeatureDepositAuth。
            Env env (*this);
            env.fund (XRP (10000), alice);

            env (fset (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (hasDepositAuth (env, alice));

            env (fclear (alice, asfDepositAuth));
            env.close();
            BEAST_EXPECT (! hasDepositAuth (env, alice));
        }
    }

    void testPayIOU()
    {
//向账户执行IOU付款和非直接XRP付款
//设置了lsfdepositauth标志的。
        testcase ("Pay IOU");

        using namespace jtx;
        Account const alice {"alice"};
        Account const bob {"bob"};
        Account const carol {"carol"};
        Account const gw {"gw"};
        IOU const USD = gw["USD"];

        Env env (*this);

        env.fund (XRP (10000), alice, bob, carol, gw);
        env.trust (USD (1000), alice, bob);
        env.close();

        env (pay (gw, alice, USD (150)));
        env (offer (carol, USD(100), XRP(100)));
        env.close();

//确保Bob的信任线都设置好了，这样他就可以收到美元。
        env (pay (alice, bob, USD (50)));
        env.close();

//Bob设置lsfDepositauth标志。
        env (fset (bob, asfDepositAuth), require(flags (bob, asfDepositAuth)));
        env.close();

//以下付款均不应成功。
        auto failedIouPayments = [this, &env, &alice, &bob, &USD] ()
        {
            env.require (flags (bob, asfDepositAuth));

//在手前捕捉鲍勃的余额，以确认它们没有变化。
            PrettyAmount const bobXrpBalance {env.balance (bob, XRP)};
            PrettyAmount const bobUsdBalance {env.balance (bob, USD)};

            env (pay (alice, bob, USD (50)), ter (tecNO_PERMISSION));
            env.close();

//注意，即使爱丽丝用xrp支付Bob，支付
//仍然不允许，因为付款通过报价。
            env (pay (alice, bob, drops(1)),
                sendmax (USD (1)), ter (tecNO_PERMISSION));
            env.close();

            BEAST_EXPECT (bobXrpBalance == env.balance (bob, XRP));
            BEAST_EXPECT (bobUsdBalance == env.balance (bob, USD));
        };

//测试Bob何时具有xrp balance>base reserve。
        failedIouPayments();

//设置Bob的xrp balance==基本准备金。同时证明
//Bob可以在设置lsfdepositauth标志时付款。
        env (pay (bob, alice, USD(25)));
        env.close();

        {
            STAmount const bobPaysXRP {
                env.balance (bob, XRP) - reserve (env, 1)};
            XRPAmount const bobPaysFee {reserve (env, 1) - reserve (env, 0)};
            env (pay (bob, alice, bobPaysXRP), fee (bobPaysFee));
            env.close();
        }

//当Bob的xrp balance==基础储备时进行测试。
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));
        BEAST_EXPECT (env.balance (bob, USD) == USD(25));
        failedIouPayments();

//当Bob的xrp平衡==0时进行测试。
        env (noop (bob), fee (reserve (env, 0)));
        env.close ();

        BEAST_EXPECT (env.balance (bob, XRP) == XRP (0));
        failedIouPayments();

//给Bob足够的xrp来清除lsfdepositauth标志。
        env (pay (alice, bob, drops(env.current()->fees().base)));

//Bob清除lsfdepositauth，下一次付款成功。
        env (fclear (bob, asfDepositAuth));
        env.close();

        env (pay (alice, bob, USD (50)));
        env.close();

        env (pay (alice, bob, drops(1)), sendmax (USD (1)));
        env.close();
    }

    void testPayXRP()
    {
//向拥有
//lsfdepositauth标志集。
        testcase ("Pay XRP");

        using namespace jtx;
        Account const alice {"alice"};
        Account const bob {"bob"};

        Env env (*this);

        env.fund (XRP (10000), alice, bob);

//Bob设置lsfDepositauth标志。
        env (fset (bob, asfDepositAuth), fee (drops (10)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == XRP (10000) - drops(10));

//Bob的xrp比基本储备多。任何xrp付款都应失败。
        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == XRP (10000) - drops(10));

//将Bob的xrp余额更改为基本准备金。
        {
            STAmount const bobPaysXRP {
                env.balance (bob, XRP) - reserve (env, 1)};
            XRPAmount const bobPaysFee {reserve (env, 1) - reserve (env, 0)};
            env (pay (bob, alice, bobPaysXRP), fee (bobPaysFee));
            env.close();
        }

//鲍勃正好有基本储备。一个足够小的直接xrp
//付款应该成功。
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));
        env (pay (alice, bob, drops(1)));
        env.close();

//鲍勃正好有基础储备+1。付款不应成功。
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0) + drops(1));
        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();

//把鲍勃减到0 xrp的平衡。
        env (noop (bob), fee (reserve (env, 0) + drops(1)));
        env.close ();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));

//我们不应该支付给鲍勃超过基本准备金。
        env (pay (alice, bob, reserve (env, 0) + drops(1)),
            ter (tecNO_PERMISSION));
        env.close();

//然而，支付一笔准确的基本准备金应该是成功的。
        env (pay (alice, bob, reserve (env, 0) + drops(0)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0));

//我们应该能再付给鲍勃一次基本储备金。
        env (pay (alice, bob, reserve (env, 0) + drops(0)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) ==
            (reserve (env, 0) + reserve (env, 0)));

//鲍勃又超过了门槛。任何付款都应该失败。
        env (pay (alice, bob, drops(1)), ter (tecNO_PERMISSION));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) ==
            (reserve (env, 0) + reserve (env, 0)));

//把鲍勃带回到零xrp余额。
        env (noop (bob), fee (env.balance (bob, XRP)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));

//Bob不能清除lsfDepositauth。
        env (fclear (bob, asfDepositAuth), ter (terINSUF_FEE_B));
        env.close();

//我们现在应该可以付鲍勃一滴了。
        env (pay (alice, bob, drops(1)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == drops(1));

//给鲍勃足够的钱，这样他就可以支付清除lsfdepositauth的费用。
        env (pay (alice, bob, drops(9)));
        env.close();

//有趣的是，在这一点上，terinsuf费重试抓住了
//请求清除lsfdepositauth。所以余额应该是零
//应清除lsfdepositauth。
        BEAST_EXPECT (env.balance (bob, XRP) == drops(0));
        env.require (nflags (bob, asfDepositAuth));

//既然Bob不再设置lsfdepositauth，我们应该能够
//付给他比基本储备更多的钱。
        env (pay (alice, bob, reserve (env, 0) + drops(1)));
        env.close();
        BEAST_EXPECT (env.balance (bob, XRP) == reserve (env, 0) + drops(1));
    }

    void testNoRipple()
    {
//它的当前化身depositauth标志不变
//任何有关涟漪和noripple标志的行为。
//证明这一点。
        testcase ("No Ripple");

        using namespace jtx;
        Account const gw1 ("gw1");
        Account const gw2 ("gw2");
        Account const alice ("alice");
        Account const bob ("bob");

        IOU const USD1 (gw1["USD"]);
        IOU const USD2 (gw2["USD"]);

        auto testIssuer = [&] (FeatureBitset const& features,
            bool noRipplePrev,
            bool noRippleNext,
            bool withDepositAuth)
        {
            assert(!withDepositAuth || features[featureDepositAuth]);

            Env env(*this, features);

            env.fund(XRP(10000), gw1, alice, bob);
            env (trust (gw1, alice["USD"](10), noRipplePrev ? tfSetNoRipple : 0));
            env (trust (gw1, bob["USD"](10), noRippleNext ? tfSetNoRipple : 0));
            env.trust(USD1 (10), alice, bob);

            env(pay(gw1, alice, USD1(10)));

            if (withDepositAuth)
                env(fset(gw1, asfDepositAuth));

            TER const result = (noRippleNext && noRipplePrev)
                ? TER {tecPATH_DRY}
                : TER {tesSUCCESS};
            env (pay (alice, bob, USD1(10)), path (gw1), ter (result));
        };

        auto testNonIssuer = [&] (FeatureBitset const& features,
            bool noRipplePrev,
            bool noRippleNext,
            bool withDepositAuth)
        {
            assert(!withDepositAuth || features[featureDepositAuth]);

            Env env(*this, features);

            env.fund(XRP(10000), gw1, gw2, alice);
            env (trust (alice, USD1(10), noRipplePrev ? tfSetNoRipple : 0));
            env (trust (alice, USD2(10), noRippleNext ? tfSetNoRipple : 0));
            env(pay(gw2, alice, USD2(10)));

            if (withDepositAuth)
                env(fset(alice, asfDepositAuth));

            TER const result = (noRippleNext && noRipplePrev)
                ? TER {tecPATH_DRY}
                : TER {tesSUCCESS};
            env (pay (gw1, gw2, USD2 (10)),
                path (alice), sendmax (USD1 (10)), ter (result));
        };

//测试NorippleRev、NorippleNext和Depositauth的每个组合
        for (int i = 0; i < 8; ++i)
        {
            auto const noRipplePrev = i & 0x1;
            auto const noRippleNext = i & 0x2;
            auto const withDepositAuth = i & 0x4;
            testIssuer(
                supported_amendments() | featureDepositAuth,
                noRipplePrev,
                noRippleNext,
                withDepositAuth);

            if (!withDepositAuth)
                testIssuer(
                    supported_amendments() - featureDepositAuth,
                    noRipplePrev,
                    noRippleNext,
                    withDepositAuth);

            testNonIssuer(
                supported_amendments() | featureDepositAuth,
                noRipplePrev,
                noRippleNext,
                withDepositAuth);

            if (!withDepositAuth)
                testNonIssuer(
                    supported_amendments() - featureDepositAuth,
                    noRipplePrev,
                    noRippleNext,
                    withDepositAuth);
        }
    }

    void run() override
    {
        testEnable();
        testPayIOU();
        testPayXRP();
        testNoRipple();
    }
};

struct DepositPreauth_test : public beast::unit_test::suite
{
    void testEnable()
    {
        testcase ("Enable");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        {
//已禁用FeatureDepositPreauth。
            Env env (*this, supported_amendments() - featureDepositPreauth);
            env.fund (XRP (10000), alice, becky);
            env.close();

//不能向Alice添加DepositPrauth。
            env (deposit::auth (alice, becky), ter (temDISABLED));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));

//不能从Alice中删除DepositPrauth。
            env (deposit::unauth (alice, becky), ter (temDISABLED));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));
        }
        {
//已启用FeatureDepositPreauth。有效的例子是
//简单的：
//o我们应该能够添加和删除一个条目，以及
//o该条目应花费一个准备金。
//o删除条目时应返回保留。
            Env env (*this);
            env.fund (XRP (10000), alice, becky);
            env.close();

//在Alice中添加DepositPrauth。
            env (deposit::auth (alice, becky));
            env.close();
            env.require (owners (alice, 1));
            env.require (owners (becky, 0));

//从Alice中删除DepositPrauth。
            env (deposit::unauth (alice, becky));
            env.close();
            env.require (owners (alice, 0));
            env.require (owners (becky, 0));
        }
    }

    void testInvalid()
    {
        testcase ("Invalid");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const carol {"carol"};

        Env env (*this);

//告诉env关于爱丽丝、贝基和卡罗尔的事情，因为他们还没有得到资助。
        env.memoize (alice);
        env.memoize (becky);
        env.memoize (carol);

//将DepositPrauth添加到未提供资金的帐户。
        env (deposit::auth (alice, becky), seq (1), ter (terNO_ACCOUNT));

        env.fund (XRP (10000), alice, becky);
        env.close();

//不良费用。
        env (deposit::auth (alice, becky), fee (drops(-10)), ter (temBAD_FEE));
        env.close();

//坏旗
        env (deposit::auth (alice, becky),
            txflags (tfSell), ter (temINVALID_FLAG));
        env.close();

        {
//既非授权也非未授权。
            Json::Value tx {deposit::auth (alice, becky)};
            tx.removeMember (sfAuthorize.jsonName);
            env (tx, ter (temMALFORMED));
            env.close();
        }
        {
//授权和未授权。
            Json::Value tx {deposit::auth (alice, becky)};
            tx[sfUnauthorize.jsonName] = becky.human();
            env (tx, ter (temMALFORMED));
            env.close();
        }
        {
//艾丽斯批准一个零账户。
            Json::Value tx {deposit::auth (alice, becky)};
            tx[sfAuthorize.jsonName] = to_string (xrpAccount());
            env (tx, ter (temINVALID_ACCOUNT_ID));
            env.close();
        }

//爱丽丝授权自己。
        env (deposit::auth (alice, alice), ter (temCANNOT_PREAUTH_SELF));
        env.close();

//艾丽斯批准了一个没有资金的账户。
        env (deposit::auth (alice, carol), ter (tecNO_TARGET));
        env.close();

//爱丽丝成功地授权了贝基。
        env.require (owners (alice, 0));
        env.require (owners (becky, 0));
        env (deposit::auth (alice, becky));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

//Alice试图创建一个重复的授权。
        env (deposit::auth (alice, becky), ter (tecDUPLICATE));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

//卡罗尔试图预先授权，但没有足够的储备。
        env.fund (drops (249'999'999), carol);
        env.close();

        env (deposit::auth (carol, becky), ter (tecINSUFFICIENT_RESERVE));
        env.close();
        env.require (owners (carol, 0));
        env.require (owners (becky, 0));

//卡罗尔得到足够的xrp（勉强）满足储备。
        env (pay (alice, carol, drops (11)));
        env.close();
        env (deposit::auth (carol, becky));
        env.close();
        env.require (owners (carol, 1));
        env.require (owners (becky, 0));

//但是卡罗尔不能满足另一个预授权的保留。
        env (deposit::auth (carol, alice), ter (tecINSUFFICIENT_RESERVE));
        env.close();
        env.require (owners (carol, 1));
        env.require (owners (becky, 0));
        env.require (owners (alice, 1));

//爱丽丝试图删除她没有的授权。
        env (deposit::unauth (alice, carol), ter (tecNO_ENTRY));
        env.close();
        env.require (owners (alice, 1));
        env.require (owners (becky, 0));

//爱丽丝成功地取消了她对贝基的授权。
        env (deposit::unauth (alice, becky));
        env.close();
        env.require (owners (alice, 0));
        env.require (owners (becky, 0));

//爱丽丝又把贝基移走了，结果出错了。
        env (deposit::unauth (alice, becky), ter (tecNO_ENTRY));
        env.close();
        env.require (owners (alice, 0));
        env.require (owners (becky, 0));
    }

    void testPayment (FeatureBitset features)
    {
        testcase ("Payment");

        using namespace jtx;
        Account const alice {"alice"};
        Account const becky {"becky"};
        Account const gw {"gw"};
        IOU const USD (gw["USD"]);

        bool const supportsPreauth = {features[featureDepositPreauth]};

        {
//Depositauth的初始实现有一个bug，其中
//设置了depositauth标志的帐户无法付款
//对自己。那个错误在DepositPrauth修正案中被修正了。
            Env env (*this, features);
            env.fund (XRP (5000), alice, becky, gw);
            env.close();

            env.trust (USD (1000), alice);
            env.trust (USD (1000), becky);
            env.close();

            env (pay (gw, alice, USD (500)));
            env.close();

            env (offer (alice, XRP (100), USD (100), tfPassive),
                require (offers (alice, 1)));
            env.close();

//贝基通过消费艾丽斯的部分报价来支付自己10美元。
//如果不涉及paymentauth，请确保付款有效。
            env (pay (becky, becky, USD (10)),
                path (~USD), sendmax (XRP (10)));
            env.close();

//贝基决定要求授权存款。
            env(fset (becky, asfDepositAuth));
            env.close();

//贝基又付了自己的钱。是否成功取决于
//是否启用FeatureDepositPreauth。
            TER const expect {
                supportsPreauth ? TER {tesSUCCESS} : TER {tecNO_PERMISSION}};

            env (pay (becky, becky, USD (10)),
                path (~USD), sendmax (XRP (10)), ter (expect));
            env.close();
        }

        if (supportsPreauth)
        {
//确保存款预授权用于付款。

            Account const carol {"carol"};

            Env env (*this, features);
            env.fund (XRP (5000), alice, becky, carol, gw);
            env.close();

            env.trust (USD (1000), alice);
            env.trust (USD (1000), becky);
            env.trust (USD (1000), carol);
            env.close();

            env (pay (gw, alice, USD (1000)));
            env.close();

//从Alice支付XRP和IOU给Becky。应该是好的。
            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

//贝基决定要求授权存款。
            env(fset (becky, asfDepositAuth));
            env.close();

//爱丽丝不能再给贝基钱了。
            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

//Becky预先授权Carol存款，但没有提供
//授权爱丽丝。
            env (deposit::auth (becky, carol));
            env.close();

//爱丽丝还是付不起贝基的钱。
            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

//贝基预先授权爱丽丝存款。
            env (deposit::auth (becky, alice));
            env.close();

//爱丽丝现在可以付钱给贝基了。
            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

//爱丽丝决定要求授权存款。
            env(fset (alice, asfDepositAuth));
            env.close();

//尽管爱丽丝有权付钱给贝基，贝基却没有
//授权支付爱丽丝。
            env (pay (becky, alice, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (becky, alice, USD (100)), ter (tecNO_PERMISSION));
            env.close();

//贝基没有授权卡罗尔。对爱丽丝没有影响。
            env (deposit::unauth (becky, carol));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();

//贝基未授权爱丽丝。爱丽丝现在不能给贝基钱了。
            env (deposit::unauth (becky, alice));
            env.close();

            env (pay (alice, becky, XRP (100)), ter (tecNO_PERMISSION));
            env (pay (alice, becky, USD (100)), ter (tecNO_PERMISSION));
            env.close();

//贝基决定取消存款授权。现在
//爱丽丝可以再给贝基一次。
            env(fclear (becky, asfDepositAuth));
            env.close();

            env (pay (alice, becky, XRP (100)));
            env (pay (alice, becky, USD (100)));
            env.close();
        }
    }

    void run() override
    {
        testEnable();
        testInvalid();
        testPayment (jtx::supported_amendments() - featureDepositPreauth);
        testPayment (jtx::supported_amendments());
    }
};

BEAST_DEFINE_TESTSUITE(DepositAuth,app,ripple);
BEAST_DEFINE_TESTSUITE(DepositPreauth,app,ripple);

} //测试
} //涟漪
