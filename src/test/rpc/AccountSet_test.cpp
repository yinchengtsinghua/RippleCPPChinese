
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

#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/AmountConversions.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/Rate.h>
#include <test/jtx.h>

namespace ripple {

class AccountSet_test : public beast::unit_test::suite
{
public:

    void testNullAccountSet()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), noripple(alice));
//要求输入分类帐条目-帐户根目录，以检查其标志
        auto const jrr = env.le(alice);
        BEAST_EXPECT((*env.le(alice))[ sfFlags ] == 0u);
    }

    void testMostFlags()
    {
        using namespace test::jtx;
        Account const alice ("alice");

//最初启用了不带DepositAuth的测试。
        Env env(*this, supported_amendments() - featureDepositAuth);
        env.fund(XRP(10000), noripple(alice));

//给爱丽丝一把普通钥匙，这样她就可以合法地设置和清除
//她的旗帜是南非国旗。
        Account const alie {"alie", KeyType::secp256k1};
        env(regkey (alice, alie));
        env.close();

        auto testFlags = [this, &alice, &alie, &env]
            (std::initializer_list<std::uint32_t> goodFlags)
        {
            std::uint32_t const orig_flags = (*env.le(alice))[ sfFlags ];
            for (std::uint32_t flag {1u};
                 flag < std::numeric_limits<std::uint32_t>::digits; ++flag)
            {
                if (flag == asfNoFreeze)
                {
//无法清除asfnofreeze标志。它被测试了
//别处。
                    continue;
                }
                else if (std::find (goodFlags.begin(), goodFlags.end(), flag) !=
                    goodFlags.end())
                {
//好旗
                    env.require(nflags(alice, flag));
                    env(fset(alice, flag), sig(alice));
                    env.close();
                    env.require(flags(alice, flag));
                    env(fclear(alice, flag), sig(alie));
                    env.close();
                    env.require(nflags(alice, flag));
                    std::uint32_t const now_flags = (*env.le(alice))[ sfFlags ];
                    BEAST_EXPECT(now_flags == orig_flags);
                }
                else
                {
//坏旗
                    BEAST_EXPECT((*env.le(alice))[ sfFlags ] == orig_flags);
                    env(fset(alice, flag), sig(alice));
                    env.close();
                    BEAST_EXPECT((*env.le(alice))[ sfFlags ] == orig_flags);
                    env(fclear(alice, flag), sig(alie));
                    env.close();
                    BEAST_EXPECT((*env.le(alice))[ sfFlags ] == orig_flags);
                }
            }
        };

//禁用FeatureDepositAuth进行测试。
        testFlags ({asfRequireDest, asfRequireAuth, asfDisallowXRP,
            asfGlobalFreeze, asfDisableMaster, asfDefaultRipple});

//启用FeatureDepositauth并重新测试。
        env.enableFeature (featureDepositAuth);
        env.close();
        testFlags ({asfRequireDest, asfRequireAuth, asfDisallowXRP,
            asfGlobalFreeze, asfDisableMaster, asfDefaultRipple,
            asfDepositAuth});
    }

    void testSetAndResetAccountTxnID()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), noripple(alice));

        std::uint32_t const orig_flags = (*env.le(alice))[ sfFlags ];

//asfaccounttxnid是特殊的，实际上并没有设置为标志，
//所以我们检查现场情况
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfAccountTxnID));
        env(fset(alice, asfAccountTxnID), sig(alice));
        BEAST_EXPECT(env.le(alice)->isFieldPresent(sfAccountTxnID));
        env(fclear(alice, asfAccountTxnID));
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfAccountTxnID));
        std::uint32_t const now_flags = (*env.le(alice))[ sfFlags ];
        BEAST_EXPECT(now_flags == orig_flags);
    }

    void testSetNoFreeze()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), noripple(alice));
        env.memoize("eric");
        env(regkey(alice, "eric"));

        env.require(nflags(alice, asfNoFreeze));
        env(fset(alice, asfNoFreeze), sig("eric"), ter(tecNEED_MASTER_KEY));
        env(fset(alice, asfNoFreeze), sig(alice));
        env.require(flags(alice, asfNoFreeze));
        env(fclear(alice, asfNoFreeze), sig(alice));
//验证标志仍然设置（清除在这种情况下不清除）
        env.require(flags(alice, asfNoFreeze));
    }

    void testDomain()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), alice);
        auto jt = noop(alice);
//域字段表示为小写的十六进制字符串。
//域的ASCII。例如，域example.com
//表示为“6578616D706C652E636F6D”。
//
//要从帐户中删除域字段，请使用
//域设置为空字符串。
        std::string const domain = "example.com";
        jt[sfDomain.fieldName] = strHex(domain);
        env(jt);
        BEAST_EXPECT((*env.le(alice))[ sfDomain ] == makeSlice(domain));

        jt[sfDomain.fieldName] = "";
        env(jt);
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfDomain));

//长度上限为256字节
//（在setaccount中定义为域\u字节\u最大值）
//测试边缘情况：255、256、257。
        std::size_t const maxLength = 256;
        for (std::size_t len = maxLength - 1; len <= maxLength + 1; ++len)
        {
            std::string domain2 =
                std::string(len - domain.length() - 1, 'a') + "." + domain;

            BEAST_EXPECT (domain2.length() == len);

            jt[sfDomain.fieldName] = strHex(domain2);

            if (len <= maxLength)
            {
                env(jt);
                BEAST_EXPECT((*env.le(alice))[ sfDomain ] == makeSlice(domain2));
            }
            else
            {
                env(jt, ter(telBAD_DOMAIN));
            }
         }
    }

    void testMessageKey()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), alice);
        auto jt = noop(alice);

        auto const rkp = randomKeyPair(KeyType::ed25519);
        jt[sfMessageKey.fieldName] = strHex(rkp.first.slice());
        env(jt);
        BEAST_EXPECT(strHex((*env.le(alice))[ sfMessageKey ]) == strHex(rkp.first.slice()));

        jt[sfMessageKey.fieldName] = "";
        env(jt);
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfMessageKey));

        using namespace std::string_literals;
        jt[sfMessageKey.fieldName] = strHex("NOT_REALLY_A_PUBKEY"s);
        env(jt, ter(telBAD_PUBLIC_KEY));
    }

    void testWalletID()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), alice);
        auto jt = noop(alice);

        uint256 somehash = from_hex_text<uint256>("9633ec8af54f16b5286db1d7b519ef49eefc050c0c8ac4384f1d88acd1bfdf05");
        jt[sfWalletLocator.fieldName] = to_string(somehash);
        env(jt);
        BEAST_EXPECT((*env.le(alice))[ sfWalletLocator ] == somehash);

        jt[sfWalletLocator.fieldName] = "";
        env(jt);
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfWalletLocator));
    }

    void testEmailHash()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        env.fund(XRP(10000), alice);
        auto jt = noop(alice);

        uint128 somehash = from_hex_text<uint128>("fff680681c2f5e6095324e2e08838f221a72ab4f");
        jt[sfEmailHash.fieldName] = to_string(somehash);
        env(jt);
        BEAST_EXPECT((*env.le(alice))[ sfEmailHash ] == somehash);

        jt[sfEmailHash.fieldName] = "";
        env(jt);
        BEAST_EXPECT(! env.le(alice)->isFieldPresent(sfEmailHash));
    }

    void testTransferRate()
    {
        struct test_results
        {
            double set;
            TER code;
            double get;
        };

        using namespace test::jtx;
        auto doTests = [this] (FeatureBitset const& features,
            std::initializer_list<test_results> testData)
        {
            Env env (*this, features);

            Account const alice ("alice");
            env.fund(XRP(10000), alice);

            for (auto const& r : testData)
            {
                env(rate(alice, r.set), ter(r.code));

//如果字段不存在，则需要默认值
                if (!(*env.le(alice))[~sfTransferRate])
                    BEAST_EXPECT(r.get == 1.0);
                else
                    BEAST_EXPECT(*(*env.le(alice))[~sfTransferRate] ==
                        r.get * QUALITY_ONE);
            }
        };

        {
            testcase ("Setting transfer rate (without fix1201)");
            doTests (supported_amendments().reset(fix1201),
                {
                    { 1.0, tesSUCCESS,              1.0 },
                    { 1.1, tesSUCCESS,              1.1 },
                    { 2.0, tesSUCCESS,              2.0 },
                    { 2.1, tesSUCCESS,              2.1 },
                    { 0.0, tesSUCCESS,              1.0 },
                    { 2.0, tesSUCCESS,              2.0 },
                    { 0.9, temBAD_TRANSFER_RATE,    2.0 }});
        }

        {
            testcase ("Setting transfer rate (with fix1201)");
            doTests (supported_amendments(),
                {
                    { 1.0, tesSUCCESS,              1.0 },
                    { 1.1, tesSUCCESS,              1.1 },
                    { 2.0, tesSUCCESS,              2.0 },
                    { 2.1, temBAD_TRANSFER_RATE,    2.0 },
                    { 0.0, tesSUCCESS,              1.0 },
                    { 2.0, tesSUCCESS,              2.0 },
                    { 0.9, temBAD_TRANSFER_RATE,    2.0 }});
        }
    }

    void testGateway()
    {
        using namespace test::jtx;
        auto runTest = [](Env&& env, double tr)
        {
            Account const alice ("alice");
            Account const bob ("bob");
            Account const gw ("gateway");
            auto const USD = gw["USD"];

            env.fund(XRP(10000), gw, alice, bob);
            env.trust(USD(3), alice, bob);
            env(rate(gw, tr));
            env.close();

            auto const amount = USD(1);
            Rate const rate (tr * QUALITY_ONE);
            auto const amountWithRate =
                toAmount<STAmount> (multiply(amount.value(), rate));

            env(pay(gw, alice, USD(3)));
            env(pay(alice, bob, USD(1)), sendmax(USD(3)));

            env.require(balance(alice, USD(3) - amountWithRate));
            env.require(balance(bob, USD(1)));
        };

//使用允许的传输速率测试网关
        auto const noFix1201 = supported_amendments().reset(fix1201);
        runTest (Env{*this, noFix1201}, 1.02);
        runTest (Env{*this, noFix1201}, 1);
        runTest (Env{*this, noFix1201}, 2);
        runTest (Env{*this, noFix1201}, 2.1);
        runTest (Env{*this, supported_amendments()}, 1.02);
        runTest (Env{*this, supported_amendments()}, 2);

//传输速率后设置修正时测试网关
        {
            Env env (*this, noFix1201);
            Account const alice ("alice");
            Account const bob ("bob");
            Account const gw ("gateway");
            auto const USD = gw["USD"];
            double const tr = 2.75;

            env.fund(XRP(10000), gw, alice, bob);
            env.trust(USD(3), alice, bob);
            env(rate(gw, tr));
            env.close();
            env.enableFeature(fix1201);
            env.close();

            auto const amount = USD(1);
            Rate const rate (tr * QUALITY_ONE);
            auto const amountWithRate =
                toAmount<STAmount> (multiply(amount.value(), rate));

            env(pay(gw, alice, USD(3)));
            env(pay(alice, bob, amount), sendmax(USD(3)));

            env.require(balance(alice, USD(3) - amountWithRate));
            env.require(balance(bob, amount));
        }
    }

    void testBadInputs(bool withFeatures)
    {
        using namespace test::jtx;
        std::unique_ptr<Env> penv {
            withFeatures ?  new Env(*this) : new Env(*this, FeatureBitset{})};
        Env& env = *penv;
        Account const alice ("alice");
        env.fund(XRP(10000), alice);

        auto jt = fset(alice, asfDisallowXRP);
        jt[jss::ClearFlag] = asfDisallowXRP;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfRequireAuth);
        jt[jss::ClearFlag] = asfRequireAuth;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfRequireDest);
        jt[jss::ClearFlag] = asfRequireDest;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfDisallowXRP);
        jt[sfFlags.fieldName] = tfAllowXRP;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfRequireAuth);
        jt[sfFlags.fieldName] = tfOptionalAuth;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfRequireDest);
        jt[sfFlags.fieldName] = tfOptionalDestTag;
        env(jt, ter(temINVALID_FLAG));

        jt = fset(alice, asfRequireDest);
        jt[sfFlags.fieldName] = tfAccountSetMask;
        env(jt, ter(temINVALID_FLAG));

        env(fset (alice, asfDisableMaster),
            sig(alice),
            ter(withFeatures ? tecNO_ALTERNATIVE_KEY : tecNO_REGULAR_KEY));
    }

    void testRequireAuthWithDir()
    {
        using namespace test::jtx;
        Env env(*this);
        Account const alice ("alice");
        Account const bob ("bob");

        env.fund(XRP(10000), alice);
        env.close();

//爱丽丝应该有一个空目录。
        BEAST_EXPECT(dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

//给艾丽斯一个签名者名单，目录里就会有东西。
        env(signers(alice, 1, { { bob, 1} }));
        env.close();
        BEAST_EXPECT(! dirIsEmpty (*env.closed(), keylet::ownerDir(alice)));

        env(fset (alice, asfRequireAuth), ter(tecOWNERS));
    }

    void run() override
    {
        testNullAccountSet();
        testMostFlags();
        testSetAndResetAccountTxnID();
        testSetNoFreeze();
        testDomain();
        testGateway();
        testMessageKey();
        testWalletID();
        testEmailHash();
        testBadInputs(true);
        testBadInputs(false);
        testRequireAuthWithDir();
        testTransferRate();
    }


};

BEAST_DEFINE_TESTSUITE(AccountSet,app,ripple);

}

