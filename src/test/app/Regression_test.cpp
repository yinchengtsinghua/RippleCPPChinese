
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
#include <test/jtx/envconfig.h>
#include <ripple/app/tx/apply.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {
namespace test {

struct Regression_test : public beast::unit_test::suite
{
//offerCreate，然后用cancel创建offerCreate
    void testOffer1()
    {
        using namespace jtx;
        Env env(*this);
        auto const gw = Account("gw");
        auto const USD = gw["USD"];
        env.fund(XRP(10000), "alice", gw);
        env(offer("alice", USD(10), XRP(10)), require(owners("alice", 1)));
        env(offer("alice", USD(20), XRP(10)), json(R"raw(
                { "OfferSequence" : 2 }
            )raw"), require(owners("alice", 1)));
    }

    void testLowBalanceDestroy()
    {
        testcase("Account balance < fee destroys correct amount of XRP");
        using namespace jtx;
        Env env(*this);
        env.memoize("alice");

//低平衡方案不能确定地
//根据未结分类账进行复制。制作本地
//关闭分类帐并直接处理。
        auto closed = std::make_shared<Ledger>(
            create_genesis, env.app().config(),
            std::vector<uint256>{}, env.app().family());
        auto expectedDrops = SYSTEM_CURRENCY_START;
        BEAST_EXPECT(closed->info().drops == expectedDrops);

        auto const aliceXRP = 400;
        auto const aliceAmount = XRP(aliceXRP);

        auto next = std::make_shared<Ledger>(
            *closed,
            env.app().timeKeeper().closeTime());
        {
//爱丽丝基金
            auto const jt = env.jt(pay(env.master, "alice", aliceAmount));
            OpenView accum(&*next);

            auto const result = ripple::apply(env.app(),
                accum, *jt.stx, tapNONE, env.journal);
            BEAST_EXPECT(result.first == tesSUCCESS);
            BEAST_EXPECT(result.second);

            accum.apply(*next);
        }
        expectedDrops -= next->fees().base;
        BEAST_EXPECT(next->info().drops == expectedDrops);
        {
            auto const sle = next->read(
                keylet::account(Account("alice").id()));
            BEAST_EXPECT(sle);
            auto balance = sle->getFieldAmount(sfBalance);

            BEAST_EXPECT(balance == aliceAmount );
        }

        {
//从env的未结分类账开始手动指定seq
//不知道这个帐户。
            auto const jt = env.jt(noop("alice"), fee(expectedDrops),
                seq(1));

            OpenView accum(&*next);

            auto const result = ripple::apply(env.app(),
                accum, *jt.stx, tapNONE, env.journal);
            BEAST_EXPECT(result.first == tecINSUFF_FEE);
            BEAST_EXPECT(result.second);

            accum.apply(*next);
        }
        {
            auto const sle = next->read(
                keylet::account(Account("alice").id()));
            BEAST_EXPECT(sle);
            auto balance = sle->getFieldAmount(sfBalance);

            BEAST_EXPECT(balance == XRP(0));
        }
        expectedDrops -= aliceXRP * dropsPerXRP<int>::value;
        BEAST_EXPECT(next->info().drops == expectedDrops);
    }

    void testSecp256r1key ()
    {
        testcase("Signing with a secp256r1 key should fail gracefully");
        using namespace jtx;
        Env env(*this);

//我们将使用的测试用例。
        auto test256r1key = [&env] (Account const& acct)
        {
            auto const baseFee = env.current()->fees().base;
            std::uint32_t const acctSeq = env.seq (acct);
            Json::Value jsonNoop = env.json (
                noop (acct), fee(baseFee), seq(acctSeq), sig(acct));
            JTx jt = env.jt (jsonNoop);
            jt.fill_sig = false;

//随机secp256r1公钥生成者
//https://kjur.github.io/jrsasign/sample-ecdsa.html网站
            std::string const secp256r1PubKey =
                "045d02995ec24988d9a2ae06a3733aa35ba0741e87527"
                "ed12909b60bd458052c944b24cbf5893c3e5be321774e"
                "5082e11c034b765861d0effbde87423f8476bb2c";

//在JSON中设置键。
            jt.jv["SigningPubKey"] = secp256r1PubKey;

//在STTX中设置相同的键。
            auto secp256r1Sig = std::make_unique<STTx>(*(jt.stx));
            auto pubKeyBlob = strUnHex (secp256r1PubKey);
assert (pubKeyBlob.second); //公钥的十六进制必须有效
            secp256r1Sig->setFieldVL
                (sfSigningPubKey, std::move(pubKeyBlob.first));
            jt.stx.reset (secp256r1Sig.release());

            env (jt, ter (temINVALID));
        };

        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};

        env.fund(XRP(10000), alice, becky);

        test256r1key (alice);
        test256r1key (becky);
    }

    void testFeeEscalationAutofill()
    {
        testcase("Autofilled fee should use the escalated fee");
        using namespace jtx;
        Env env(*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->section("transaction_queue")
                    .set("minimum_txn_in_ledger_standalone", "3");
                return cfg;
            }));
        Env_ss envs(env);

        auto const alice = Account("alice");
        env.fund(XRP(100000), alice);

        auto params = Json::Value(Json::objectValue);
//最大费用=50K滴
        params[jss::fee_mult_max] = 5000;
        std::vector<int> const
            expectedFees({ 10, 10, 8889, 13889, 20000 });

//我们应该能够在
//我们的费用限制。
        for (int i = 0; i < 5; ++i)
        {
            envs(noop(alice), fee(none), seq(none))(params);

            auto tx = env.tx();
            if (BEAST_EXPECT(tx))
            {
                BEAST_EXPECT(tx->getAccountID(sfAccount) == alice.id());
                BEAST_EXPECT(tx->getTxnType() == ttACCOUNT_SET);
                auto const fee = tx->getFieldAmount(sfFee);
                BEAST_EXPECT(fee == drops(expectedFees[i]));
            }
        }
    }

    void testFeeEscalationExtremeConfig()
    {
        testcase("Fee escalation shouldn't allocate extreme memory");
        using clock_type = std::chrono::steady_clock;
        using namespace jtx;
        using namespace std::chrono_literals;

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg)
        {
            auto& s = cfg->section("transaction_queue");
            s.set("minimum_txn_in_ledger_standalone", "4294967295");
            s.set("minimum_txn_in_ledger", "4294967295");
            s.set("target_txn_in_ledger", "4294967295");
            s.set("normal_consensus_increase_percent", "4294967295");

            return cfg;
        }));

        env(noop(env.master));
//如果遇到任何断点，此测试可能会失败，
//但即使是最慢的机器也会被忽略。
        auto const start = clock_type::now();
        env.close();
        BEAST_EXPECT(clock_type::now() - start < 1s);
    }

    void testJsonInvalid()
    {
        using namespace jtx;
        using boost::asio::buffer;
        testcase("jsonInvalid");

        std::string const request = R"json({"command":"path_find","id":19,"subcommand":"create","source_account":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","destination_account":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","destination_amount":"1000000","source_currencies":[{"currency":"0000000000000000000000000000000000000000"},{"currency":"0000000000000000000000005553440000000000"},{"currency":"0000000000000000000000004254430000000000"},{"issuer":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","currency":"0000000000000000000000004254430000000000"},{"issuer":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","currency":"0000000000000000000000004254430000000000"},{"issuer":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","currency":"0000000000000000000000004555520000000000"},{"currency":"0000000000000000000000004554480000000000"},{"currency":"0000000000000000000000004A50590000000000"},{"issuer":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","currency":"000000000000000000000000434E590000000000"},{"currency":"0000000000000000000000004742490000000000"},{"issuer":"rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh","currency":"0000000000000000000000004341440000000000"}]})json";

        Json::Value jvRequest;
        Json::Reader jrReader;

        std::vector<boost::asio::const_buffer> buffers;
        buffers.emplace_back(buffer(request, 1024));
        buffers.emplace_back(buffer(request.data() + 1024, request.length() - 1024));
        BEAST_EXPECT(jrReader.parse(jvRequest, buffers) && jvRequest.isObject());
    }

    void run() override
    {
        testOffer1();
        testLowBalanceDestroy();
        testSecp256r1key();
        testFeeEscalationAutofill();
        testFeeEscalationExtremeConfig();
        testJsonInvalid();
    }
};

BEAST_DEFINE_TESTSUITE(Regression,app,ripple);

} //测试
} //涟漪
