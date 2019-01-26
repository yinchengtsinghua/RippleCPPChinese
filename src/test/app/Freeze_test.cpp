
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
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/AccountID.h>

namespace ripple {

class Freeze_test : public beast::unit_test::suite
{

    static Json::Value
    getAccountLines(test::jtx::Env& env, test::jtx::Account const& account)
    {
        Json::Value jq;
        jq[jss::account] = account.human();
        return env.rpc("json", "account_lines", to_string(jq))[jss::result];
    }

    static Json::Value
    getAccountOffers(
        test::jtx::Env& env,
        test::jtx::Account const& account,
        bool current = false)
    {
        Json::Value jq;
        jq[jss::account] = account.human();
        jq[jss::ledger_index] = current ? "current" : "validated";
        return env.rpc("json", "account_offers", to_string(jq))[jss::result];
    }

    static bool checkArraySize(Json::Value const& val, unsigned int size)
    {
        return val.isArray() && val.size() == size;
    }

    void testRippleState(FeatureBitset features)
    {
        testcase("RippleState Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account alice {"alice"};
        Account bob {"bob"};

        env.fund(XRP(1000), G1, alice, bob);
        env.close();

        env.trust(G1["USD"](100), bob);
        env.trust(G1["USD"](100), alice);
        env.close();

        env(pay(G1, bob, G1["USD"](10)));
        env(pay(G1, alice, G1["USD"](100)));
        env.close();

        env(offer(alice, XRP(500), G1["USD"](100)));
        env.close();

        {
            auto lines = getAccountLines(env, bob);
            if(! BEAST_EXPECT(checkArraySize(lines[jss::lines], 1u)))
                return;
            BEAST_EXPECT(lines[jss::lines][0u][jss::account] == G1.human());
            BEAST_EXPECT(lines[jss::lines][0u][jss::limit] == "100");
            BEAST_EXPECT(lines[jss::lines][0u][jss::balance] == "10");
        }

        {
            auto lines = getAccountLines(env, alice);
            if(! BEAST_EXPECT(checkArraySize(lines[jss::lines], 1u)))
                return;
            BEAST_EXPECT(lines[jss::lines][0u][jss::account] == G1.human());
            BEAST_EXPECT(lines[jss::lines][0u][jss::limit] == "100");
            BEAST_EXPECT(lines[jss::lines][0u][jss::balance] == "100");
        }

        {
//行解冻账户（证明操作正常）
//测试：可以在那条线上付款
            env(pay(alice, bob, G1["USD"](1)));

//测试：可以在该线路上收到付款
            env(pay(bob, alice, G1["USD"](1)));
            env.close();
        }

        {
//通过带有setfreeze标志的信任集创建
//测试：设置低冻结高冻结标志
            env(trust(G1, bob["USD"](0), tfSetFreeze));
            auto affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
                return;
            auto ff =
                affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfLowLimit.fieldName] == G1["USD"](0).value().getJson(0));
            BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfLowFreeze);
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
            env.close();
        }

        {
//发卡行冻结账户
//测试：可以在这条线上购买更多资产
            env(offer(bob, G1["USD"](5), XRP(25)));
            auto affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 5u)))
                return;
            auto ff =
                affected[3u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfHighLimit.fieldName] == bob["USD"](100).value().getJson(0));
            auto amt =
                STAmount{Issue{to_currency("USD"), noAccount()}, -15}
                    .value().getJson(0);
            BEAST_EXPECT(ff[sfBalance.fieldName] == amt);
            env.close();
        }

        {
//测试：不能从该行出售资产
            env(offer(bob, XRP(1), G1["USD"](5)),
                ter(tecUNFUNDED_OFFER));

//测试：可以在该线路上收到付款
            env(pay(alice, bob, G1["USD"](1)));

//测试：无法从该行付款
            env(pay(bob, alice, G1["USD"](1)), ter(tecPATH_DRY));
        }

        {
//检查G1账户行
//测试：显示冻结
            auto lines = getAccountLines(env, G1);
            Json::Value bobLine;
            for (auto const& it : lines[jss::lines])
            {
                if(it[jss::account] == bob.human())
                {
                    bobLine = it;
                    break;
                }
            }
            if(! BEAST_EXPECT(bobLine))
                return;
            BEAST_EXPECT(bobLine[jss::freeze] == true);
            BEAST_EXPECT(bobLine[jss::balance] == "-16");
        }

        {
//测试：显示冻结对等
            auto lines = getAccountLines(env, bob);
            Json::Value g1Line;
            for (auto const& it : lines[jss::lines])
            {
                if(it[jss::account] == G1.human())
                {
                    g1Line = it;
                    break;
                }
            }
            if(! BEAST_EXPECT(g1Line))
                return;
            BEAST_EXPECT(g1Line[jss::freeze_peer] == true);
            BEAST_EXPECT(g1Line[jss::balance] == "16");
        }

        {
//通过带有ClearFreeze标志的信任集清除
//测试：设置低冻结高冻结标志
            env(trust(G1, bob["USD"](0), tfClearFreeze));
            auto affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
            if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
                return;
            auto ff =
                affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
            BEAST_EXPECT(
                ff[sfLowLimit.fieldName] == G1["USD"](0).value().getJson(0));
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfLowFreeze));
            BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
            env.close();
        }
    }

    void
    testGlobalFreeze(FeatureBitset features)
    {
        testcase("Global Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A1 {"A1"};
        Account A2 {"A2"};
        Account A3 {"A3"};
        Account A4 {"A4"};

        env.fund(XRP(12000), G1);
        env.fund(XRP(1000), A1);
        env.fund(XRP(20000), A2, A3, A4);
        env.close();

        env.trust(G1["USD"](1200), A1);
        env.trust(G1["USD"](200), A2);
        env.trust(G1["BTC"](100), A3);
        env.trust(G1["BTC"](100), A4);
        env.close();

        env(pay(G1, A1, G1["USD"](1000)));
        env(pay(G1, A2, G1["USD"](100)));
        env(pay(G1, A3, G1["BTC"](100)));
        env(pay(G1, A4, G1["BTC"](100)));
        env.close();

        env(offer(G1, XRP(10000), G1["USD"](100)), txflags(tfPassive));
        env(offer(G1, G1["USD"](100), XRP(10000)), txflags(tfPassive));
        env(offer(A1, XRP(10000), G1["USD"](100)), txflags(tfPassive));
        env(offer(A2, G1["USD"](100), XRP(10000)), txflags(tfPassive));
        env.close();

        {
//通过使用setflag和clearfag的accountset切换
//测试：setflag globalfreeze
            env.require(nflags(G1, asfGlobalFreeze));
            env(fset(G1, asfGlobalFreeze));
            env.require(flags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));

//测试：ClearFlag GlobalFreeze
            env(fclear(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));
        }

        {
//不带globalfreeze的帐户（证明操作正常）
//测试：接受方支付未冻结发行人的可见报价
            auto offers =
                env.rpc("book_offers",
                    std::string("USD/")+G1.human(), "XRP")
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
            std::set<std::string> accounts;
            for (auto const& offer : offers)
            {
                accounts.insert(offer[jss::Account].asString());
            }
            BEAST_EXPECT(accounts.find(A2.human()) != std::end(accounts));
            BEAST_EXPECT(accounts.find(G1.human()) != std::end(accounts));

//测试：接受者获得的可见报价是未冻结发行人
            offers =
                env.rpc("book_offers",
                    "XRP", std::string("USD/")+G1.human())
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
            accounts.clear();
            for (auto const& offer : offers)
            {
                accounts.insert(offer[jss::Account].asString());
            }
            BEAST_EXPECT(accounts.find(A1.human()) != std::end(accounts));
            BEAST_EXPECT(accounts.find(G1.human()) != std::end(accounts));
        }

        {
//报价/付款
//测试：可以在市场上购买资产
            env(offer(A3, G1["BTC"](1), XRP(1)));

//测试：资产可以在市场上出售
            env(offer(A4, XRP(1), G1["BTC"](1)));

//测试：可以发送直接问题
            env(pay(G1, A2, G1["USD"](1)));

//测试：可以发送直接赎回
            env(pay(A2, G1, G1["USD"](1)));

//测试：可通过波纹发送
            env(pay(A2, A1, G1["USD"](1)));

//测试：可通过波纹发送回
            env(pay(A1, A2, G1["USD"](1)));
        }

        {
//在GlobalFreeze的帐户
//先设置globalfreeze
//测试：setflag globalfreeze将切换回冻结状态
            env.require(nflags(G1, asfGlobalFreeze));
            env(fset(G1, asfGlobalFreeze));
            env.require(flags(G1, asfGlobalFreeze));
            env.require(nflags(G1, asfNoFreeze));

//测试：无法在市场上购买资产
            env(offer(A3, G1["BTC"](1), XRP(1)), ter(tecFROZEN));

//测试：资产不能在市场上出售
            env(offer(A4, XRP(1), G1["BTC"](1)), ter(tecFROZEN));
        }

        {
//报价已筛选（似乎已被破坏？）
//测试：帐户优惠总是显示自己的优惠
            auto offers = getAccountOffers(env, G1)[jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;

//测试：图书报价显示报价
//（是否应该对其进行过滤？）
            offers =
                env.rpc("book_offers",
                    "XRP", std::string("USD/")+G1.human())
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;

            offers =
                env.rpc("book_offers",
                    std::string("USD/")+G1.human(), "XRP")
                [jss::result][jss::offers];
            if(! BEAST_EXPECT(checkArraySize(offers, 2u)))
                return;
        }

        {
//付款
//测试：可以发送直接问题
            env(pay(G1, A2, G1["USD"](1)));

//测试：可以发送直接赎回
            env(pay(A2, G1, G1["USD"](1)));

//测试：无法发送通过波纹
            env(pay(A2, A1, G1["USD"](1)), ter(tecPATH_DRY));
        }
    }

    void
    testNoFreeze(FeatureBitset features)
    {
        testcase("No Freeze");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A1 {"A1"};

        env.fund(XRP(12000), G1);
        env.fund(XRP(1000), A1);
        env.close();

        env.trust(G1["USD"](1000), A1);
        env.close();

        env(pay(G1, A1, G1["USD"](1000)));
        env.close();

//信任集不冻结
//测试：应在标志中设置nofreeze
        env.require(nflags(G1, asfNoFreeze));
        env(fset(G1, asfNoFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(nflags(G1, asfGlobalFreeze));

//测试：无法清除
        env(fclear(G1, asfNoFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(nflags(G1, asfGlobalFreeze));

//测试：可以设置globalfreeze
        env(fset(G1, asfGlobalFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(flags(G1, asfGlobalFreeze));

//测试：无法取消设置globalfreeze
        env(fclear(G1, asfGlobalFreeze));
        env.require(flags(G1, asfNoFreeze));
        env.require(flags(G1, asfGlobalFreeze));

//测试：信任线不能冻结
        env(trust(G1, A1["USD"](0), tfSetFreeze));
        auto affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 1u)))
            return;

        auto let =
            affected[0u][sfModifiedNode.fieldName][sfLedgerEntryType.fieldName];
        BEAST_EXPECT(let == "AccountRoot");
    }

    void
    testOffersWhenFrozen(FeatureBitset features)
    {
        testcase("Offers for Frozen Trust Lines");

        using namespace test::jtx;
        Env env(*this, features);

        Account G1 {"G1"};
        Account A2 {"A2"};
        Account A3 {"A3"};
        Account A4 {"A4"};

        env.fund(XRP(1000), G1, A3, A4);
        env.fund(XRP(2000), A2);
        env.close();

        env.trust(G1["USD"](1000), A2);
        env.trust(G1["USD"](2000), A3);
        env.trust(G1["USD"](2000), A4);
        env.close();

        env(pay(G1, A3, G1["USD"](2000)));
        env(pay(G1, A4, G1["USD"](2000)));
        env.close();

        env(offer(A3, XRP(1000), G1["USD"](1000)), txflags(tfPassive));
        env.close();

//成功付款后移除
//测试：以部分消费价付款
        env(pay(A2, G1, G1["USD"](1)), paths(G1["USD"]), sendmax(XRP(1)));
        env.close();

//测试：仅部分使用了报价
        auto offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 1u)))
            return;
        BEAST_EXPECT(
            offers[0u][jss::taker_gets] == G1["USD"](999).value().getJson(0));

//测试：其他人创建一个提供流动性的报价
        env(offer(A4, XRP(999), G1["USD"](999)));
        env.close();

//测试：部分消耗的报价行的所有者已冻结
        env(trust(G1, A3["USD"](0), tfSetFreeze));
        auto affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
            return;
        auto ff =
            affected[1u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
        BEAST_EXPECT(
            ff[sfHighLimit.fieldName] == G1["USD"](0).value().getJson(0));
        BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfLowFreeze));
        BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfHighFreeze);
        env.close();

//核实书上的报价
        offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 1u)))
            return;

//测试：可以通过新报价付款
        env(pay(A2, G1, G1["USD"](1)), paths(G1["USD"]), sendmax(XRP(1)));
        env.close();

//测试：部分消费的报价已由TES*付款删除
        offers = getAccountOffers(env, A3)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 0u)))
            return;

//删除购买成功的产品创建
//测试：冻结新报价
        env(trust(G1, A4["USD"](0), tfSetFreeze));
        affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 2u)))
            return;
        ff =
            affected[0u][sfModifiedNode.fieldName][sfFinalFields.fieldName];
        BEAST_EXPECT(
            ff[sfLowLimit.fieldName] == G1["USD"](0).value().getJson(0));
        BEAST_EXPECT(ff[jss::Flags].asUInt() & lsfLowFreeze);
        BEAST_EXPECT(! (ff[jss::Flags].asUInt() & lsfHighFreeze));
        env.close();

//测试：无法再创建交叉报价
        env(offer(A2, G1["USD"](999), XRP(999)));
        affected = env.meta()->getJson(0)[sfAffectedNodes.fieldName];
        if(! BEAST_EXPECT(checkArraySize(affected, 8u)))
            return;
        auto created = affected[5u][sfCreatedNode.fieldName];
        BEAST_EXPECT(created[sfNewFields.fieldName][jss::Account] == A2.human());
        env.close();

//测试：报价已被报价删除\u创建
        offers = getAccountOffers(env, A4)[jss::offers];
        if(! BEAST_EXPECT(checkArraySize(offers, 0u)))
            return;
    }

public:

    void run() override
    {
        auto testAll = [this](FeatureBitset features)
        {
            testRippleState(features);
            testGlobalFreeze(features);
            testNoFreeze(features);
            testOffersWhenFrozen(features);
        };
        using namespace test::jtx;
        auto const sa = supported_amendments();
        testAll(sa - featureFlow - fix1373 - featureFlowCross);
        testAll(sa               - fix1373 - featureFlowCross);
        testAll(sa                         - featureFlowCross);
        testAll(sa);
    }
};

BEAST_DEFINE_TESTSUITE(Freeze, app, ripple);
} //涟漪
