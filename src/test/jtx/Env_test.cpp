
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

#include <ripple/basics/Log.h>
#include <test/jtx.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/unit_test.h>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <utility>

namespace ripple {
namespace test {

class Env_test : public beast::unit_test::suite
{
public:
    template <class T>
    static
    std::string
    to_string (T const& t)
    {
        return boost::lexical_cast<
            std::string>(t);
    }

//帐户H中的声明
    void
    testAccount()
    {
        using namespace jtx;
        {
            Account a;
            Account b(a);
            a = b;
            a = std::move(b);
            Account c(std::move(a));
        }
        Account("alice");
        Account("alice", KeyType::secp256k1);
        Account("alice", KeyType::ed25519);
        auto const gw = Account("gw");
        [](AccountID){}(gw);
        auto const USD = gw["USD"];
        void(Account("alice") < gw);
        std::set<Account>().emplace(gw);
        std::unordered_set<Account,
            beast::uhash<>>().emplace("alice");
    }

//金额申报.h
    void
    testAmount()
    {
        using namespace jtx;

        PrettyAmount(0);
        PrettyAmount(1);
        PrettyAmount(0u);
        PrettyAmount(1u);
        PrettyAmount(-1);
        static_assert(! std::is_constructible<
            PrettyAmount, char>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, unsigned char>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, short>::value, "");
        static_assert(! std::is_constructible<
            PrettyAmount, unsigned short>::value, "");

        try
        {
            XRP(0.0000001);
            fail("missing exception");
        }
        catch(std::domain_error const&)
        {
            pass();
        }
        XRP(-0.000001);
        try
        {
            XRP(-0.0000009);
            fail("missing exception");
        }
        catch(std::domain_error const&)
        {
            pass();
        }

        BEAST_EXPECT(to_string(XRP(5)) == "5 XRP");
        BEAST_EXPECT(to_string(XRP(.80)) == "0.8 XRP");
        BEAST_EXPECT(to_string(XRP(.005)) == "5000 drops");
        BEAST_EXPECT(to_string(XRP(0.1)) == "0.1 XRP");
        BEAST_EXPECT(to_string(XRP(10000)) == "10000 XRP");
        BEAST_EXPECT(to_string(drops(10)) == "10 drops");
        BEAST_EXPECT(to_string(drops(123400000)) == "123.4 XRP");
        BEAST_EXPECT(to_string(XRP(-5)) == "-5 XRP");
        BEAST_EXPECT(to_string(XRP(-.99)) == "-0.99 XRP");
        BEAST_EXPECT(to_string(XRP(-.005)) == "-5000 drops");
        BEAST_EXPECT(to_string(XRP(-0.1)) == "-0.1 XRP");
        BEAST_EXPECT(to_string(drops(-10)) == "-10 drops");
        BEAST_EXPECT(to_string(drops(-123400000)) == "-123.4 XRP");

        BEAST_EXPECT(XRP(1) == drops(1000000));
        BEAST_EXPECT(XRP(1) == STAmount(1000000));
        BEAST_EXPECT(STAmount(1000000) == XRP(1));

        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        BEAST_EXPECT(to_string(USD(0)) == "0/USD(gw)");
        BEAST_EXPECT(to_string(USD(10)) == "10/USD(gw)");
        BEAST_EXPECT(to_string(USD(-10)) == "-10/USD(gw)");
        BEAST_EXPECT(USD(0) == STAmount(USD, 0));
        BEAST_EXPECT(USD(1) == STAmount(USD, 1));
        BEAST_EXPECT(USD(-1) == STAmount(USD, -1));

        auto const get = [](AnyAmount a){ return a; };
        BEAST_EXPECT(! get(USD(10)).is_any);
        BEAST_EXPECT(get(any(USD(10))).is_any);
    }

//测试环境
    void
    testEnv()
    {
        using namespace jtx;
        auto const n = XRP(10000);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        auto const alice = Account("alice");

//无资金的
        {
            Env env(*this);
            env(pay("alice", "bob", XRP(1000)), seq(1), fee(10), sig("alice"), ter(terNO_ACCOUNT));
        }

//基金
        {
            Env env(*this);

//变差函数
            env.fund(n, "alice");
            env.fund(n, "bob", "carol");
            env.fund(n, "dave", noripple("eric"));
            env.fund(n, "fred", noripple("gary", "hank"));
            env.fund(n, noripple("irene"));
            env.fund(n, noripple("jim"), "karen");
            env.fund(n, noripple("lisa", "mary"));

//旗帜
            env.fund(n, noripple("xavier"));
            env.require(nflags("xavier", asfDefaultRipple));
            env.fund(n, "yana");
            env.require(flags("yana", asfDefaultRipple));
        }

//信任
        {
            Env env(*this);
            env.fund(n, "alice", "bob", gw);
            env(trust("alice", USD(100)), require(lines("alice", 1)));
        }

//平衡
        {
            Env env(*this);
            BEAST_EXPECT(env.balance(alice) == 0);
            BEAST_EXPECT(env.balance(alice, USD) != 0);
            BEAST_EXPECT(env.balance(alice, USD) == USD(0));
            env.fund(n, alice, gw);
            BEAST_EXPECT(env.balance(alice) == n);
            BEAST_EXPECT(env.balance(gw) == n);
            env.trust(USD(1000), alice);
            env(pay(gw, alice, USD(10)));
            BEAST_EXPECT(to_string(env.balance("alice", USD)) == "10/USD(gw)");
            BEAST_EXPECT(to_string(env.balance(gw, alice["USD"])) == "-10/USD(alice)");
        }

//SEQ
        {
            Env env(*this);
            env.fund(n, noripple("alice", gw));
            BEAST_EXPECT(env.seq("alice") == 1);
            BEAST_EXPECT(env.seq(gw) == 1);
        }

//自动填充
        {
            Env env(*this);
            env.fund(n, "alice");
            env.require(balance("alice", n));
            env(noop("alice"), fee(1),                  ter(telINSUF_FEE_P));
            env(noop("alice"), seq(none),               ter(temMALFORMED));
            env(noop("alice"), seq(none), fee(10),      ter(temMALFORMED));
            env(noop("alice"), fee(none),               ter(temMALFORMED));
            env(noop("alice"), sig(none),               ter(temMALFORMED));
            env(noop("alice"), fee(autofill));
            env(noop("alice"), fee(autofill), seq(autofill));
            env(noop("alice"), fee(autofill), seq(autofill), sig(autofill));
        }
    }

//要求：要求
    void
    testRequire()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        env.require(balance("alice", none));
        env.require(balance("alice", XRP(none)));
        env.fund(XRP(10000), "alice", gw);
        env.require(balance("alice", USD(none)));
        env.trust(USD(100), "alice");
env.require(balance("alice", XRP(10000))); //费用退还
        env.require(balance("alice", USD(0)));
        env(pay(gw, "alice", USD(10)), require(balance("alice", USD(10))));

        env.require(nflags("alice", asfRequireDest));
        env(fset("alice", asfRequireDest), require(flags("alice", asfRequireDest)));
        env(fclear("alice", asfRequireDest), require(nflags("alice", asfRequireDest)));
    }

//使用secp256k1和ed25519密钥签名
    void
    testKeyType()
    {
        using namespace jtx;

        Env env(*this);
        Account const alice("alice", KeyType::ed25519);
        Account const bob("bob", KeyType::secp256k1);
        Account const carol("carol");
        env.fund(XRP(10000), alice, bob);

//仅主密钥
        env(noop(alice));
        env(noop(bob));
        env(noop(alice), sig("alice"),                          ter(tefBAD_AUTH_MASTER));
        env(noop(alice), sig(Account("alice",
            KeyType::secp256k1)),                               ter(tefBAD_AUTH_MASTER));
        env(noop(bob), sig(Account("bob",
            KeyType::ed25519)),                                 ter(tefBAD_AUTH_MASTER));
        env(noop(alice), sig(carol),                            ter(tefBAD_AUTH_MASTER));

//万能钥匙和普通钥匙
        env(regkey(alice, bob));
        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice));

//仅限普通钥匙
        env(fset(alice, asfDisableMaster), sig(alice));
        env(noop(alice));
        env(noop(alice), sig(bob));
        env(noop(alice), sig(alice),                            ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(alice),        ter(tefMASTER_DISABLED));
        env(fclear(alice, asfDisableMaster), sig(bob));
        env(noop(alice), sig(alice));
    }

//支付基础
    void
    testPayments()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gateway");
        auto const USD = gw["USD"];

        env.fund(XRP(10000), "alice", "bob", "carol", gw);
        env.require(balance("alice", XRP(10000)));
        env.require(balance("bob", XRP(10000)));
        env.require(balance("carol", XRP(10000)));
        env.require(balance(gw, XRP(10000)));

        env(pay(env.master, "alice", XRP(1000)), fee(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), fee(1),        ter(telINSUF_FEE_P));
        env(pay(env.master, "alice", XRP(1000)), seq(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), seq(20),       ter(terPRE_SEQ));
        env(pay(env.master, "alice", XRP(1000)), sig(none),     ter(temMALFORMED));
        env(pay(env.master, "alice", XRP(1000)), sig("bob"),    ter(tefBAD_AUTH_MASTER));

        env(pay(env.master, "dilbert", XRP(1000)), sig(env.master));

        env.trust(USD(100), "alice", "bob", "carol");
        env.require(owners("alice", 1), lines("alice", 1));
        env(rate(gw, 1.05));

        env(pay(gw, "carol", USD(50)));
        env.require(balance("carol", USD(50)));
        env.require(balance(gw, Account("carol")["USD"](-50)));

        env(offer("carol", XRP(50), USD(50)), require(owners("carol", 2)));
        env(pay("alice", "bob", any(USD(10))),                  ter(tecPATH_DRY));
        env(pay("alice", "bob", any(USD(10))),
            paths(XRP), sendmax(XRP(10)),                       ter(tecPATH_PARTIAL));
        env(pay("alice", "bob", any(USD(10))), paths(XRP),
            sendmax(XRP(20)));
        env.require(balance("bob", USD(10)));
        env.require(balance("carol", USD(39.5)));

        env.memoize("eric");
        env(regkey("alice", "eric"));
        env(noop("alice"));
        env(noop("alice"), sig("alice"));
        env(noop("alice"), sig("eric"));
        env(noop("alice"), sig("bob"),                          ter(tefBAD_AUTH));
        env(fset("alice", asfDisableMaster),                    ter(tecNEED_MASTER_KEY));
        env(fset("alice", asfDisableMaster), sig("eric"),       ter(tecNEED_MASTER_KEY));
        env.require(nflags("alice", asfDisableMaster));
        env(fset("alice", asfDisableMaster), sig("alice"));
        env.require(flags("alice", asfDisableMaster));
        env(regkey("alice", disabled),                          ter(tecNO_ALTERNATIVE_KEY));
        env(noop("alice"));
        env(noop("alice"), sig("alice"),                        ter(tefMASTER_DISABLED));
        env(noop("alice"), sig("eric"));
        env(noop("alice"), sig("bob"),                          ter(tefBAD_AUTH));
        env(fclear("alice", asfDisableMaster), sig("bob"),      ter(tefBAD_AUTH));
        env(fclear("alice", asfDisableMaster), sig("alice"),    ter(tefMASTER_DISABLED));
        env(fclear("alice", asfDisableMaster));
        env.require(nflags("alice", asfDisableMaster));
        env(regkey("alice", disabled));
        env(noop("alice"), sig("eric"),                         ter(tefBAD_AUTH_MASTER));
        env(noop("alice"));
    }

//多符号基础
    void
    testMultiSign()
    {
        using namespace jtx;

        Env env(*this);
        env.fund(XRP(10000), "alice");
        env(signers("alice", 1,
            { { "alice", 1 }, { "bob", 2 } }),                  ter(temBAD_SIGNER));
        env(signers("alice", 1,
            { { "bob", 1 }, { "carol", 2 } }));
        env(noop("alice"));

        auto const baseFee = env.current()->fees().base;
        env(noop("alice"), msig("bob"), fee(2 * baseFee));
        env(noop("alice"), msig("carol"), fee(2 * baseFee));
        env(noop("alice"), msig("bob", "carol"), fee(3 * baseFee));
        env(noop("alice"), msig("bob", "carol", "dilbert"),
            fee(4 * baseFee),                                   ter(tefBAD_SIGNATURE));

        env(signers("alice", none));
    }

    void
    testTicket()
    {
        using namespace jtx;
//创建语法
        ticket::create("alice", "bob");
        ticket::create("alice", 60);
        ticket::create("alice", "bob", 60);
        ticket::create("alice", 60, "bob");

        {
            Env env(*this, supported_amendments().set(featureTickets));
            env.fund(XRP(10000), "alice");
            env(noop("alice"),                  require(owners("alice", 0), tickets("alice", 0)));
            env(ticket::create("alice"),        require(owners("alice", 1), tickets("alice", 1)));
            env(ticket::create("alice"),        require(owners("alice", 2), tickets("alice", 2)));
        }
    }

    struct UDT
    {
    };

    void testJTxProperties()
    {
        struct T { };
        using namespace jtx;
        JTx jt1;
//直接测试
//财产
        BEAST_EXPECT(!jt1.get<int>());
        jt1.set<int>(7);
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 7);
        BEAST_EXPECT(!jt1.get<UDT>());

//测试属性是否为
//如果存在，则更换。
        jt1.set<int>(17);
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 17);
        BEAST_EXPECT(!jt1.get<UDT>());

//测试修改
//返回的道具已保存
        *jt1.get<int>() = 42;
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 42);
        BEAST_EXPECT(!jt1.get<UDT>());

//test get（）常量
        auto const& jt2 = jt1;
        BEAST_EXPECT(jt2.get<int>());
        BEAST_EXPECT(*jt2.get<int>() == 42);
        BEAST_EXPECT(!jt2.get<UDT>());
    }

    void testProp()
    {
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(100000), "alice");
        auto jt1 = env.jt(noop("alice"));
        BEAST_EXPECT(!jt1.get<std::uint16_t>());
        auto jt2 = env.jt(noop("alice"),
            prop<std::uint16_t>(-1));
        BEAST_EXPECT(jt2.get<std::uint16_t>());
        BEAST_EXPECT(*jt2.get<std::uint16_t>() ==
            65535);
        auto jt3 = env.jt(noop("alice"),
            prop<std::string>(
                "Hello, world!"),
                    prop<bool>(false));
        BEAST_EXPECT(jt3.get<std::string>());
        BEAST_EXPECT(*jt3.get<std::string>() ==
            "Hello, world!");
        BEAST_EXPECT(jt3.get<bool>());
        BEAST_EXPECT(!*jt3.get<bool>());
    }

    void testJTxCopy()
    {
        struct T { };
        using namespace jtx;
        JTx jt1;
        jt1.set<int>(7);
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 7);
        BEAST_EXPECT(!jt1.get<UDT>());
        JTx jt2(jt1);
        BEAST_EXPECT(jt2.get<int>());
        BEAST_EXPECT(*jt2.get<int>() == 7);
        BEAST_EXPECT(!jt2.get<UDT>());
        JTx jt3;
        jt3 = jt1;
        BEAST_EXPECT(jt3.get<int>());
        BEAST_EXPECT(*jt3.get<int>() == 7);
        BEAST_EXPECT(!jt3.get<UDT>());
    }

    void testJTxMove()
    {
        struct T { };
        using namespace jtx;
        JTx jt1;
        jt1.set<int>(7);
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 7);
        BEAST_EXPECT(!jt1.get<UDT>());
        JTx jt2(std::move(jt1));
        BEAST_EXPECT(!jt1.get<int>());
        BEAST_EXPECT(!jt1.get<UDT>());
        BEAST_EXPECT(jt2.get<int>());
        BEAST_EXPECT(*jt2.get<int>() == 7);
        BEAST_EXPECT(!jt2.get<UDT>());
        jt1 = std::move(jt2);
        BEAST_EXPECT(!jt2.get<int>());
        BEAST_EXPECT(!jt2.get<UDT>());
        BEAST_EXPECT(jt1.get<int>());
        BEAST_EXPECT(*jt1.get<int>() == 7);
        BEAST_EXPECT(!jt1.get<UDT>());
    }

    void
    testMemo()
    {
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice");
        env(noop("alice"), memodata("data"));
        env(noop("alice"), memoformat("format"));
        env(noop("alice"), memotype("type"));
        env(noop("alice"), memondata("format", "type"));
        env(noop("alice"), memonformat("data", "type"));
        env(noop("alice"), memontype("data", "format"));
        env(noop("alice"), memo("data", "format", "type"));
        env(noop("alice"), memo("data1", "format1", "type1"), memo("data2", "format2", "type2"));
    }

    void
    testAdvance()
    {
        using namespace jtx;
        Env env(*this);
        auto seq = env.current()->seq();
        BEAST_EXPECT(seq == env.closed()->seq() + 1);
        env.close();
        BEAST_EXPECT(env.closed()->seq() == seq);
        BEAST_EXPECT(env.current()->seq() == seq + 1);
        env.close();
        BEAST_EXPECT(env.closed()->seq() == seq + 1);
        BEAST_EXPECT(env.current()->seq() == seq + 2);
    }

    void
    testClose()
    {
        using namespace jtx;
        Env env(*this);
        env.close();
        env.close();
        env.fund(XRP(100000), "alice", "bob");
        env.close();
        env(pay("alice", "bob", XRP(100)));
        env.close();
        env(noop("alice"));
        env.close();
        env(noop("bob"));
    }

    void
    testPath()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        env.fund(XRP(10000), "alice", "bob");
        env.json(
            pay("alice", "bob", USD(10)),
                path(Account("alice")),
                path("bob"),
                path(USD),
                path(~XRP),
                path(~USD),
                path("bob", USD, ~XRP, ~USD)
                );
    }

//测试JTX是否可以重新签署已经签署的事务。
    void testResignSigned()
    {
        using namespace jtx;
        Env env(*this);

        env.fund(XRP(10000), "alice");
        auto const baseFee = env.current()->fees().base;
        std::uint32_t const aliceSeq = env.seq ("alice");

//签署JSONNOP。
        Json::Value jsonNoop = env.json (
            noop ("alice"), fee(baseFee), seq(aliceSeq), sig("alice"));
//重新签署jsonnoop。
        JTx jt = env.jt (jsonNoop);
        env (jt);
    }

    void testSignAndSubmit()
    {
        using namespace jtx;
        Env env(*this);
        Env_ss envs(env);

        auto const alice = Account("alice");
        env.fund(XRP(10000), alice);

        {
            envs(noop(alice), fee(none), seq(none))();

//确保我们取回正确的帐户。
            auto tx = env.tx();
            if (BEAST_EXPECT(tx))
            {
                BEAST_EXPECT(tx->getAccountID(sfAccount) == alice.id());
                BEAST_EXPECT(tx->getTxnType() == ttACCOUNT_SET);
            }
        }

        {
            auto params = Json::Value(Json::nullValue);
            envs(noop(alice), fee(none), seq(none))(params);

//确保我们取回正确的帐户。
            auto tx = env.tx();
            if (BEAST_EXPECT(tx))
            {
                BEAST_EXPECT(tx->getAccountID(sfAccount) == alice.id());
                BEAST_EXPECT(tx->getTxnType() == ttACCOUNT_SET);
            }
        }

        {
            auto params = Json::Value(Json::objectValue);
//将系数降低到足以使其失效。
            params[jss::fee_mult_max] = 1;
            params[jss::fee_div_max] = 2;
//RPC错误导致temInvalid
            envs(noop(alice), fee(none),
                seq(none), ter(temINVALID))(params);

            auto tx = env.tx();
            BEAST_EXPECT(!tx);
        }
    }

    void testFeatures()
    {
        testcase("Env features");
        using namespace jtx;
        auto const supported = supported_amendments();

//这将找到一个不在
//支持的修订列表和测试
//显式启用

        auto const neverSupportedFeat = [&]() -> boost::optional<uint256>
        {
            auto const n = supported.size();
            for(size_t i = 0; i < n; ++i)
                if (!supported[i])
                    return bitsetIndexToFeature(i);

            return boost::none;
        }();

        if (!neverSupportedFeat)
        {
            log << "No unsupported features found - skipping test." << std::endl;
            pass();
            return;
        }

        auto hasFeature = [](Env& env, uint256 const& f)
        {
            return (env.app().config().features.find (f) !=
                    env.app().config().features.end());
        };

        {
//默认env具有所有支持的功能
            Env env{*this};
            BEAST_EXPECT(
                supported.count() == env.app().config().features.size());
            foreachFeature(supported, [&](uint256 const& f) {
                this->BEAST_EXPECT(hasFeature(env, f));
            });
        }

        {
//env featurebitset只有*这些功能
            Env env{*this, FeatureBitset(featureEscrow, featureFlow)};
            BEAST_EXPECT(env.app().config().features.size() == 2);
            foreachFeature(supported, [&](uint256 const& f) {
                bool const has = (f == featureEscrow || f == featureFlow);
                this->BEAST_EXPECT(has == hasFeature(env, f));
            });
        }

        auto const noFlowOrEscrow =
            supported_amendments() - featureEscrow - featureFlow;
        {
//一个env支持的\u功能\除了缺少*仅*那些功能
            Env env{*this, noFlowOrEscrow};
            BEAST_EXPECT(
                env.app().config().features.size() == (supported.count() - 2));
            foreachFeature(supported, [&](uint256 const& f) {
                bool hasnot = (f == featureEscrow || f == featureFlow);
                this->BEAST_EXPECT(hasnot != hasFeature(env, f));
            });
        }

        {
//添加不在支持的修订列表中的功能
//以及一份明确修订的清单
//不支持的功能应与
//两个支持的
            Env env{
                *this,
                FeatureBitset(featureEscrow, featureFlow, *neverSupportedFeat)};

//这个应用只有2个支持的修正和
//另一个从不支持的功能标志
            BEAST_EXPECT(env.app().config().features.size() == (2 + 1));
            BEAST_EXPECT(hasFeature(env, *neverSupportedFeat));

            foreachFeature(supported, [&](uint256 const& f) {
                bool has = (f == featureEscrow || f == featureFlow);
                this->BEAST_EXPECT(has == hasFeature(env, f));
            });
        }

        {
//添加不在支持的修订列表中的功能
//省略了一些标准修正案
//应启用不支持的功能
            Env env{*this,
                    noFlowOrEscrow | FeatureBitset{*neverSupportedFeat}};

//此应用程序将包含所有支持的修订减去2，然后
//另一个从不支持的功能标志
            BEAST_EXPECT(
                env.app().config().features.size() ==
                (supported.count() - 2 + 1));
            BEAST_EXPECT(hasFeature(env, *neverSupportedFeat));
            foreachFeature(supported, [&](uint256 const& f) {
                bool hasnot = (f == featureEscrow || f == featureFlow);
                this->BEAST_EXPECT(hasnot != hasFeature(env, f));
            });
        }

        {
//添加不在支持的修订列表中的功能
//以及所有支持的修正案
//应启用不支持的功能
            Env env{*this, supported_amendments().set(*neverSupportedFeat)};

//此应用程序将包含所有支持的修订，然后
//另一个从不支持的功能标志
            BEAST_EXPECT(
                env.app().config().features.size() == (supported.count() + 1));
            BEAST_EXPECT(hasFeature(env, *neverSupportedFeat));
            foreachFeature(supported, [&](uint256 const& f) {
                this->BEAST_EXPECT(hasFeature(env, f));
            });
        }
    }

    void
    run() override
    {
        testAccount();
        testAmount();
        testEnv();
        testRequire();
        testKeyType();
        testPayments();
        testMultiSign();
        testTicket();
        testJTxProperties();
        testProp();
        testJTxCopy();
        testJTxMove();
        testMemo();
        testAdvance();
        testClose();
        testPath();
        testResignSigned();
        testSignAndSubmit();
        testFeatures();
    }
};

BEAST_DEFINE_TESTSUITE(Env,app,ripple);

} //测试
} //涟漪
