
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
#include <ripple/beast/unit_test.h>
#include <ripple/protocol/Feature.h>

namespace ripple {
namespace test {

class CrossingLimits_test : public beast::unit_test::suite
{
private:
    void
    n_offers (
        jtx::Env& env,
        std::size_t n,
        jtx::Account const& account,
        STAmount const& in,
        STAmount const& out)
    {
        using namespace jtx;
        auto const ownerCount = env.le(account)->getFieldU32(sfOwnerCount);
        for (std::size_t i = 0; i < n; i++)
        {
            env(offer(account, in, out));
            env.close();
        }
        env.require (owners (account, ownerCount + n));
    }

public:

    void
    testStepLimit(FeatureBitset features)
    {
        testcase ("Step Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan");
        env.trust(USD(1), "bob");
        env(pay(gw, "bob", USD(1)));
        env.trust(USD(1), "dan");
        env(pay(gw, "dan", USD(1)));
        n_offers (env, 2000, "bob", XRP(1), USD(1));
        n_offers (env, 1, "dan", XRP(1), USD(1));

//Alice提议以1000美元的价格购买1000瑞波币。她先吃鲍勃的
//提供，删除999个以上的无资金，然后达到步骤限制。
        env(offer("alice", USD(1000), XRP(1000)));
        env.require (balance("alice", USD(1)));
        env.require (owners("alice", 2));
        env.require (balance("bob", USD(0)));
        env.require (owners("bob", 1001));
        env.require (balance("dan", USD(1)));
        env.require (owners("dan", 2));

//卡罗尔提出以1000美元购买1000瑞波币。她删除了鲍勃的下一个
//1000个报价没有资金支持，达到了步骤限制。
        env(offer("carol", USD(1000), XRP(1000)));
        env.require (balance("carol", USD(none)));
        env.require (owners("carol", 1));
        env.require (balance("bob", USD(0)));
        env.require (owners("bob", 1));
        env.require (balance("dan", USD(1)));
        env.require (owners("dan", 2));
    }

    void
    testCrossingLimit(FeatureBitset features)
    {
        testcase ("Crossing Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

//允许交叉的报价数量在
//接线员和流动交叉。Taker允许850，FlowCross允许1000。
//在测试中考虑到这种差异。
        int const maxConsumed = features[featureFlowCross] ? 1000 : 850;

        env.fund(XRP(100000000), gw, "alice", "bob", "carol");
        int const bobsOfferCount = maxConsumed + 150;
        env.trust(USD(bobsOfferCount), "bob");
        env(pay(gw, "bob", USD(bobsOfferCount)));
        env.close();
        n_offers (env, bobsOfferCount, "bob", XRP(1), USD(1));

//爱丽丝提出要买鲍勃的提议。不管她怎么做
//越过限制，所以她不能一次全部买下。
        env(offer("alice", USD(bobsOfferCount), XRP(bobsOfferCount)));
        env.close();
        env.require (balance("alice", USD(maxConsumed)));
        env.require (balance("bob", USD(150)));
        env.require (owners ("bob", 150 + 1));

//卡罗尔提出以1000美元购买1000瑞波币。她接受鲍伯的
//剩余150个报价未达到上限。
        env(offer("carol", USD(1000), XRP(1000)));
        env.close();
        env.require (balance("carol", USD(150)));
        env.require (balance("bob", USD(0)));
        env.require (owners ("bob", 1));
    }

    void
    testStepAndCrossingLimit(FeatureBitset features)
    {
        testcase ("Step And Crossing Limit");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan", "evita");

//允许交叉的报价数量在
//接线员和流动交叉。Taker允许850，FlowCross允许1000。
//在测试中考虑到这种差异。
        bool const isFlowCross {features[featureFlowCross]};
        int const maxConsumed = isFlowCross ? 1000 : 850;

        int const evitasOfferCount {maxConsumed + 49};
        env.trust(USD(1000), "alice");
        env(pay(gw, "alice", USD(1000)));
        env.trust(USD(1000), "carol");
        env(pay(gw, "carol", USD(1)));
        env.trust(USD(evitasOfferCount + 1), "evita");
        env(pay(gw, "evita", USD(evitasOfferCount + 1)));

//Taker和Flowcross还有另一个我们必须适应的区别。
//接受方允许总共使用1000个无资金支持的报价。
//超过850个报价。FlowCross没有这样的绘制
//区别；其限额为1000个资金或无资金。
//
//当我们使用Taker时，给Carol额外的150（无资金）优惠
//以适应这种差异。
        int const carolsOfferCount {isFlowCross ? 700 : 850};
        n_offers (env, 400, "alice", XRP(1), USD(1));
        n_offers (env, carolsOfferCount, "carol", XRP(1), USD(1));
        n_offers (env, evitasOfferCount, "evita", XRP(1), USD(1));

//Bob提议以1000美元的价格购买1000瑞波币。他拿走了所有400美元
//爱丽丝的出价，从卡罗尔的1美元，然后删除了卡罗尔的599美元。
//在达到步数限制之前，报价没有资金支持。
        env(offer("bob", USD(1000), XRP(1000)));
        env.require (balance("bob", USD(401)));
        env.require (balance("alice", USD(600)));
        env.require (owners("alice", 1));
        env.require (balance("carol", USD(0)));
        env.require (owners("carol", carolsOfferCount - 599));
        env.require (balance("evita", USD(evitasOfferCount + 1)));
        env.require (owners("evita", evitasOfferCount + 1));

//丹提出购买MaxConsumed+50瑞郎美元。他把所有的
//卡罗尔剩下的提议没有资金支持，然后接受
//（最大消费量-100）美元，达到交叉限额。
        env(offer("dan", USD(maxConsumed + 50), XRP(maxConsumed + 50)));
        env.require (balance("dan", USD(maxConsumed - 100)));
        env.require (owners("dan", 2));
        env.require (balance("alice", USD(600)));
        env.require (owners("alice", 1));
        env.require (balance("carol", USD(0)));
        env.require (owners("carol", 1));
        env.require (balance("evita", USD(150)));
        env.require (owners("evita", 150));
    }

    void testAutoBridgedLimitsTaker (FeatureBitset features)
    {
        testcase ("Auto Bridged Limits Taker");

        using namespace jtx;
        Env env(*this, features);
        auto const closeTime =
            fix1449Time() +
                100 * env.closed()->info().closeTimeResolution;
        env.close (closeTime);

        auto const gw = Account("gateway");
        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

        env.fund(XRP(100000000), gw, "alice", "bob", "carol", "dan", "evita");

        env.trust(USD(2000), "alice");
        env(pay(gw, "alice", USD(2000)));
        env.trust(USD(1000), "carol");
        env(pay(gw, "carol", USD(3)));
        env.trust(USD(1000), "evita");
        env(pay(gw, "evita", USD(1000)));

        n_offers (env,  302, "alice", EUR(2), XRP(1));
        n_offers (env,  300, "alice", XRP(1), USD(4));
        n_offers (env,  497, "carol", XRP(1), USD(3));
        n_offers (env, 1001, "evita", EUR(1), USD(1));

//Bob提出以2000欧元的价格购买2000美元，尽管他只有
//1000欧元。
//1。他花了600欧元接受爱丽丝的汽车搭桥优惠
//得到1200美元。
//2。他又花了2欧元买了爱丽丝的一个欧元->xrp和
//卡罗尔的xrp-usd优惠之一。他为此得到3美元。
//三。卡罗尔其余的提议现在没有资金支持。我们已经
//到目前为止，消费了602个产品。我们现在再仔细研究398个
//在我们达到1000个出价上限之前，卡罗尔没有提供资金。
//这个集合有错误的桥接-我们将不再处理
//桥接要约
//4。然而，有没有直接仍然是正确的。所以我们再来一次
//花点时间接受Evita的提议。
//5。在接受了Evita的提议后，我们再次注意到
//超过了提供计数。所以我们完全停止服用
//Evita的提议之一。
        env.trust(EUR(10000), "bob");
        env.close();
        env(pay(gw, "bob", EUR(1000)));
        env.close();
        env(offer("bob", USD(2000), EUR(2000)));
        env.require (balance("bob", USD(1204)));
        env.require (balance("bob", EUR( 397)));

        env.require (balance("alice", USD(800)));
        env.require (balance("alice", EUR(602)));
        env.require (offers("alice", 1));
        env.require (owners("alice", 3));

        env.require (balance("carol", USD(0)));
        env.require (balance("carol", EUR(none)));
        env.require (offers("carol", 100));
        env.require (owners("carol", 101));

        env.require (balance("evita", USD(999)));
        env.require (balance("evita", EUR(1)));
        env.require (offers("evita", 1000));
        env.require (owners("evita", 1002));

//丹提出以900美元购买900欧元。
//1。他取消了卡罗尔剩下的100个没有资金支持的提议。
//2。然后从Evita的报价中扣除850美元。
//三。消费了evita的850个资金支持的方案，就达到了十字路口。
//极限。所以丹的提议即使他会
//愿意接受Evita的另外50个报价。
        env.trust(EUR(10000), "dan");
        env.close();
        env(pay(gw, "dan", EUR(1000)));
        env.close();

        env(offer("dan", USD(900), EUR(900)));
        env.require (balance("dan", USD(850)));
        env.require (balance("dan", EUR(150)));

        env.require (balance("alice", USD(800)));
        env.require (balance("alice", EUR(602)));
        env.require (offers("alice", 1));
        env.require (owners("alice", 3));

        env.require (balance("carol", USD(0)));
        env.require (balance("carol", EUR(none)));
        env.require (offers("carol", 0));
        env.require (owners("carol", 1));

        env.require (balance("evita", USD(149)));
        env.require (balance("evita", EUR(851)));
        env.require (offers("evita", 150));
        env.require (owners("evita", 152));
    }

    void
    testAutoBridgedLimitsFlowCross(FeatureBitset features)
    {
        testcase("Auto Bridged Limits FlowCross");

//如果支付链中的任何预订步骤消耗1000个报价，则
//使用报价的流动性，但该股将标记为
//在交易的剩余时间内保持干燥。

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");
        auto const carol = Account("carol");

        auto const USD = gw["USD"];
        auto const EUR = gw["EUR"];

//有两个几乎相同的测试。有一股大股
//将导致钢绞线标记为干燥的无资金报价数量
//尽管该股仍有流动性。
//在第一次试验中，钢绞线具有最佳的初始质量。在
//第二次试验，钢绞线质量不好
//实现必须正确处理此情况，而不是标记
//在实际使用流动性之前保持干燥）
        {
            Env env(*this, features);
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            env.fund(XRP(100000000), gw, alice, bob, carol);

            env.trust(USD(4000), alice);
            env(pay(gw, alice, USD(4000)));
            env.trust(USD(1000), carol);
            env(pay(gw, carol, USD(3)));

//注意，有800个无资金支持的出价的股票有首字母
//最佳质量
            n_offers(env, 2000, alice, EUR(2), XRP(1));
            n_offers(env, 300, alice, XRP(1), USD(4));
            n_offers(
env, 801, carol, XRP(1), USD(3));  //只有一个报价是供资的
            n_offers(env, 1000, alice, XRP(1), USD(3));

            n_offers(env, 1, alice, EUR(500), USD(500));

//Bob提出以2000欧元的价格购买2000美元；他从2000欧元开始
//1。最好的质量是2欧元的自动报价。
//并给予4美元。
//Bob花费600欧元，收入1200美元。
//
//2。最好的质量是2欧元的自动报价。
//并给予3美元。
//卡罗尔的一个提议被接受了。这就剩下她另一个了
//提供无资金支持。
//卡罗尔剩下的800份报价被视为无资金支付。
//约199爱丽丝的xrp（1）到美元（3）的报价被消费。
//预订步骤最多可消费1000个优惠
//以一个给定的质量，现在达到了这个极限。
//D.尽管仍有资金支持，但现在该股已经干涸。
//提供从XRP（1）到美元（3）的优惠。鲍勃花了400欧元
//在此步骤中收到600美元。（消费了200个资助方案
//800个无资金支持的报价）
//三。最好的是非自动保险的报价，需要500欧元和
//给予500美元。
//Bob有2000欧元，花费600+400=1000欧元。他有1000个
//左边。鲍勃花了500欧元，得到500美元。
//总计：Bob支出欧元（600+400+500）=1500欧元。他开始
//2000年，剩下500人
//Bob收到美元（1200+600+500）=2300美元。
//爱丽丝花了300*4+199*3+500=2297美元。她开始
//4000美元，剩下1703美元。爱丽丝收到
//600+400+500=1500欧元
            env.trust(EUR(10000), bob);
            env.close();
            env(pay(gw, bob, EUR(2000)));
            env.close();
            env(offer(bob, USD(4000), EUR(4000)));
            env.close();

            env.require(balance(bob, USD(2300)));
            env.require(balance(bob, EUR(500)));
            env.require(offers(bob, 1));
            env.require(owners(bob, 3));

            env.require(balance(alice, USD(1703)));
            env.require(balance(alice, EUR(1500)));
            auto const numAOffers =
                2000 + 300 + 1000 + 1 - (2 * 300 + 2 * 199 + 1 + 1);
            env.require(offers(alice, numAOffers));
            env.require(owners(alice, numAOffers + 2));

            env.require(offers(carol, 0));
        }
        {
            Env env(*this, features);
            auto const closeTime =
                fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
            env.close(closeTime);

            env.fund(XRP(100000000), gw, alice, bob, carol);

            env.trust(USD(4000), alice);
            env(pay(gw, alice, USD(4000)));
            env.trust(USD(1000), carol);
            env(pay(gw, carol, USD(3)));

//注意，有800个未提供资金的报价的Strand没有
//初始最佳质量
            n_offers(env, 1, alice, EUR(1), USD(10));
            n_offers(env, 2000, alice, EUR(2), XRP(1));
            n_offers(env, 300, alice, XRP(1), USD(4));
            n_offers(
env, 801, carol, XRP(1), USD(3));  //只有一个报价是供资的
            n_offers(env, 1000, alice, XRP(1), USD(3));

            n_offers(env, 1, alice, EUR(499), USD(499));

//Bob提出以2000欧元的价格购买2000美元；他从2000欧元开始
//1。最好的质量是报价1欧元10美元。
//Bob花费1欧元，收到10美元。
//
//2。最好的质量是2欧元的自动报价。
//给4美元。
//Bob花费600欧元，收入1200美元。
//
//三。最好的质量是2欧元的自动报价。
//给3美元。
//卡罗尔的一个提议被接受了。这就剩下她另一个了
//提供无资金支持。
//卡罗尔剩下的800份报价被视为无资金支付。
//约199爱丽丝的xrp（1）到美元（3）的报价被消费。
//预订步骤最多可消费1000个优惠
//以一个给定的质量，现在达到了这个极限。
//D.尽管仍有资金支持，但现在该股已经干涸。
//提供从XRP（1）到美元（3）的优惠。鲍勃花了400欧元
//在此步骤中收到600美元。（消费了200个资助方案
//800个无资金支持的报价）
//4。最好的是非自动保险的报价，需要499欧元，而且
//给予499美元。
//Bob有2000欧元，花费1+600+400=1001欧元。他有
//左边999个。Bob花费499欧元，收到499美元。
//总计：Bob支出欧元（1+600+400+499）=1500欧元。他
//从2000年开始，剩下500人
//Bob收到美元（10+1200+600+499）=2309美元。
//爱丽丝花了10+300*4+199*3+499=2306美元。她
//从4000美元开始，剩下1704美元。爱丽丝
//收到600+400+500=1500欧元
            env.trust(EUR(10000), bob);
            env.close();
            env(pay(gw, bob, EUR(2000)));
            env.close();
            env(offer(bob, USD(4000), EUR(4000)));
            env.close();

            env.require(balance(bob, USD(2309)));
            env.require(balance(bob, EUR(500)));
            env.require(offers(bob, 1));
            env.require(owners(bob, 3));

            env.require(balance(alice, USD(1694)));
            env.require(balance(alice, EUR(1500)));
            auto const numAOffers =
                1 + 2000 + 300 + 1000 + 1 - (1 + 2 * 300 + 2 * 199 + 1 + 1);
            env.require(offers(alice, numAOffers));
            env.require(owners(alice, numAOffers + 2));

            env.require(offers(carol, 0));
        }
    }

    void testAutoBridgedLimits (FeatureBitset features)
    {
//Taker和Flowcross在处理方式上太不一样了
//使一个测试适合两种方法的自动桥接。
//
//o接受者轮流看书，完成一个完整的增量。
//在返回进行另一个通行证之前。
//
//o FlowCross在一本书中以一种质量尽可能多地提取
//在继续读另一本书之前。这减少了
//我们换书的时候。
//
//因此，这两种形式的自桥试验是分开的。
        if (features[featureFlowCross])
            testAutoBridgedLimitsFlowCross (features);
        else
            testAutoBridgedLimitsTaker (features);
    }

    void
    testOfferOverflow (FeatureBitset features)
    {
        testcase("Offer Overflow");

        using namespace jtx;

        auto const gw = Account("gateway");
        auto const alice = Account("alice");
        auto const bob = Account("bob");

        auto const USD = gw["USD"];

        Env env(*this, features);
        auto const closeTime =
            fix1449Time() + 100 * env.closed()->info().closeTimeResolution;
        env.close(closeTime);

        env.fund(XRP(100000000), gw, alice, bob);

        env.trust(USD(8000), alice);
        env.trust(USD(8000), bob);
        env.close();

        env(pay(gw, alice, USD(8000)));
        env.close();

//新的流量交叉处理消耗过多的报价不同于旧的
//提供交叉代码。在旧代码中，跟踪已使用的报价总数，以及
//在达到该限制后，交叉口将停止。在新的代码中，报价的数量是
//按报价单和质量跟踪。这个测试显示了它们之间的差异。建立一本书
//有很多优惠。在每种质量上，报价的数量都要低于限额。然而，如果
//所有的报价都被消耗掉了，这会造成一个技术上的错误。
        n_offers(env, 998, alice, XRP(1.00), USD(1));
        n_offers(env, 998, alice, XRP(0.99), USD(1));
        n_offers(env, 998, alice, XRP(0.98), USD(1));
        n_offers(env, 998, alice, XRP(0.97), USD(1));
        n_offers(env, 998, alice, XRP(0.96), USD(1));
        n_offers(env, 998, alice, XRP(0.95), USD(1));

        bool const withFlowCross = features[featureFlowCross];
        env(offer(bob, USD(8000), XRP(8000)), ter(withFlowCross ? TER{tecOVERSIZE} : tesSUCCESS));
        env.close();

        env.require(balance(bob, USD(withFlowCross ? 0 : 850)));
    }

    void
    run() override
    {
        auto testAll = [this](FeatureBitset features) {
            testStepLimit(features);
            testCrossingLimit(features);
            testStepAndCrossingLimit(features);
            testAutoBridgedLimits(features);
            testOfferOverflow(features);
        };
        using namespace jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa               - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa                                           );
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(CrossingLimits,tx,ripple,10);

} //测试
} //涟漪
