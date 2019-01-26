
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

#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/JsonFields.h>     //JSS：：定义
#include <ripple/protocol/Feature.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

class MultiSign_test : public beast::unit_test::suite
{
//用于虚拟签名的无资金帐户。
    jtx::Account const bogie {"bogie", KeyType::secp256k1};
    jtx::Account const demon {"demon", KeyType::ed25519};
    jtx::Account const ghost {"ghost", KeyType::secp256k1};
    jtx::Account const haunt {"haunt", KeyType::ed25519};
    jtx::Account const jinni {"jinni", KeyType::secp256k1};
    jtx::Account const phase {"phase", KeyType::ed25519};
    jtx::Account const shade {"shade", KeyType::secp256k1};
    jtx::Account const spook {"spook", KeyType::ed25519};

public:
    void test_noReserve (FeatureBitset features)
    {
        testcase ("No Reserve");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::secp256k1};

//签名者列表所需的保留随文章的变化而变化。
//多信号储备的特征。进行必要的调整。
        bool const reserve1 {features[featureMultiSignReserve]};

//付给艾丽丝足够的钱以满足最初的储备，但不足以
//满足签名列表集的保留。
        auto const fee = env.current()->fees().base;
        auto const smallSignersReserve = reserve1 ? XRP(250) : XRP(350);
        env.fund(smallSignersReserve - drops (1), alice);
        env.close();
        env.require (owners (alice, 0));

        {
//将签名者列表附加到Alice。应该失败。
            Json::Value smallSigners = signers(alice, 1, { { bogie, 1 } });
            env(smallSigners, ter(tecINSUFFICIENT_RESERVE));
            env.close();
            env.require (owners (alice, 0));

//给Alice足够的资金来设置签名者列表，然后附加签名者。
            env(pay(env.master, alice, fee + drops (1)));
            env.close();
            env(smallSigners);
            env.close();
            env.require (owners (alice, reserve1 ? 1 : 3));
        }
        {
//付给爱丽丝足够的钱，几乎可以为最大的
//可能的列表。
            auto const addReserveBigSigners = reserve1 ? XRP(0) : XRP(350);
            env(pay(env.master, alice, addReserveBigSigners + fee - drops(1)));

//替换为最大的签名者列表。应该失败。
            Json::Value bigSigners = signers(alice, 1, {
                { bogie, 1 }, { demon, 1 }, { ghost, 1 }, { haunt, 1 },
                { jinni, 1 }, { phase, 1 }, { shade, 1 }, { spook, 1 }});
            env(bigSigners, ter(tecINSUFFICIENT_RESERVE));
            env.close();
            env.require (owners (alice, reserve1 ? 1 : 3));

//再给艾丽丝一滴钱（加上费用）就成功了。
            env(pay(env.master, alice, fee + drops(1)));
            env.close();
            env(bigSigners);
            env.close();
            env.require (owners (alice, reserve1 ? 1 : 10));
        }
//删除Alice的签名者列表并返回所有者计数。
        env(signers(alice, jtx::none));
        env.close();
        env.require (owners (alice, 0));
    }

    void test_signerListSet (FeatureBitset features)
    {
        testcase ("SignerListSet");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);

//加上爱丽丝作为自己的多信号。应该失败。
        env(signers(alice, 1, { { alice, 1} }), ter (temBAD_SIGNER));

//添加一个权重为零的签名者。应该失败。
        env(signers(alice, 1, { { bogie, 0} }), ter (temBAD_WEIGHT));

//在权重太大的地方添加签名者。应该失败，因为
//权重字段只有16位。JTX框架做不到
//这种测试，所以被评论掉了。
//env（签名者（Alice，1，转向架，0x1000），ter（Tembad_weight））；

//添加同一签名者两次。应该失败。
        env(signers(alice, 1, {
            { bogie, 1 }, { demon, 1 }, { ghost, 1 }, { haunt, 1 },
            { jinni, 1 }, { phase, 1 }, { demon, 1 }, { spook, 1 }}),
            ter(temBAD_SIGNER));

//将仲裁设置为零。应该失败。
        env(signers(alice, 0, { { bogie, 1} }), ter (temMALFORMED));

//在无法满足法定人数的情况下制作签名者列表。应该失败。
        env(signers(alice, 9, {
            { bogie, 1 }, { demon, 1 }, { ghost, 1 }, { haunt, 1 },
            { jinni, 1 }, { phase, 1 }, { shade, 1 }, { spook, 1 }}),
            ter(temBAD_QUORUM));

//制作一个太大的签名者列表。应该失败。
        Account const spare ("spare", KeyType::secp256k1);
        env(signers(alice, 1, {
            { bogie, 1 }, { demon, 1 }, { ghost, 1 }, { haunt, 1 },
            { jinni, 1 }, { phase, 1 }, { shade, 1 }, { spook, 1 },
            { spare, 1 }}),
            ter(temMALFORMED));

        env.close();
        env.require (owners (alice, 0));
    }

    void test_phantomSigners (FeatureBitset features)
    {
        testcase ("Phantom Signers");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);
        env.close();

//将幻影签名者附加到Alice并将其用于事务。
        env(signers(alice, 1, {{bogie, 1}, {demon, 1}}));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 1 : 4));

//这应该有效。
        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq = env.seq (alice);
        env(noop(alice), msig(bogie, demon), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//任何一个签名者都应该单独工作。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(bogie), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(demon), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//重复签名者应该失败。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(demon, demon), fee(3 * baseFee), ter(temINVALID));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//非签名者应该失败。
        aliceSeq = env.seq (alice);
        env(noop(alice),
            msig(bogie, spook), fee(3 * baseFee), ter(tefBAD_SIGNATURE));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//不符合法定人数。应该失败。
        env(signers(alice, 2, {{bogie, 1}, {demon, 1}}));
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(bogie), fee(2 * baseFee), ter(tefBAD_QUORUM));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//达到法定人数。应该成功。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(bogie, demon), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
    }

    void
    test_enablement (FeatureBitset features)
    {
        testcase ("Enablement");

        using namespace jtx;
        Env env(*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return cfg;
            }), features - featureMultiSign);

        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);
        env.close();

//注意：如果启用多信号，这六个测试将失败。
        env(signers(alice, 1, {{bogie, 1}}), ter(temDISABLED));
        env.close();
        env.require (owners (alice, 0));

        std::uint32_t aliceSeq = env.seq (alice);
        auto const baseFee = env.current()->fees().base;
        env(noop(alice), msig(bogie), fee(2 * baseFee), ter(temINVALID));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

        env(signers(alice, 1, {{bogie, 1}, {demon,1}}), ter(temDISABLED));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

        {
            Json::Value jvParams;
            jvParams[jss::account] = alice.human();
            auto const jsmr = env.rpc("json", "submit_multisigned", to_string(jvParams))[jss::result];
            BEAST_EXPECT(jsmr[jss::error]         == "notEnabled");
            BEAST_EXPECT(jsmr[jss::status]        == "error");
            BEAST_EXPECT(jsmr[jss::error_message] == "Not enabled in configuration.");
        }

        {
            Json::Value jvParams;
            jvParams[jss::account] = alice.human();
            auto const jsmr = env.rpc("json", "sign_for", to_string(jvParams))[jss::result];
            BEAST_EXPECT(jsmr[jss::error]         == "notEnabled");
            BEAST_EXPECT(jsmr[jss::status]        == "error");
            BEAST_EXPECT(jsmr[jss::error_message] == "Not enabled in configuration.");
        }
    }

    void test_fee (FeatureBitset features)
    {
        testcase ("Fee");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);
        env.close();

//将最大可能的签名者数附加到Alice。
        env(signers(alice, 1, {{bogie, 1}, {demon, 1}, {ghost, 1}, {haunt, 1},
            {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}}));
        env.close();
        env.require (owners(alice, features[featureMultiSignReserve] ? 1 : 10));

//这应该有效。
        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq = env.seq (alice);
        env(noop(alice), msig(bogie), fee(2 * baseFee));
        env.close();

        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//这应该会失败，因为费用太小。
        aliceSeq = env.seq (alice);
        env(noop(alice),
            msig(bogie), fee((2 * baseFee) - 1), ter(telINSUF_FEE_P));
        env.close();

        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//这应该有效。
        aliceSeq = env.seq (alice);
        env(noop(alice),
            msig(bogie, demon, ghost, haunt, jinni, phase, shade, spook),
            fee(9 * baseFee));
        env.close();

        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//这应该会失败，因为费用太小。
        aliceSeq = env.seq (alice);
        env(noop(alice),
            msig(bogie, demon, ghost, haunt, jinni, phase, shade, spook),
            fee((9 * baseFee) - 1),
            ter(telINSUF_FEE_P));
        env.close();

        BEAST_EXPECT(env.seq(alice) == aliceSeq);
    }

    void test_misorderedSigners (FeatureBitset features)
    {
        testcase ("Misordered Signers");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);
        env.close();

//事务中的签名必须按排序顺序提交。
//如果没有，请确保事务失败。
        env(signers(alice, 1, {{bogie, 1}, {demon, 1}}));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 1 : 4));

        msig phantoms {bogie, demon};
        std::reverse (phantoms.signers.begin(), phantoms.signers.end());
        std::uint32_t const aliceSeq = env.seq (alice);
        env(noop(alice), phantoms, ter(temINVALID));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);
    }

    void test_masterSigners (FeatureBitset features)
    {
        testcase ("Master Signers");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        Account const becky {"becky", KeyType::secp256k1};
        Account const cheri {"cheri", KeyType::ed25519};
        env.fund(XRP(1000), alice, becky, cheri);
        env.close();

//对于不同的情况，给爱丽丝一把普通的钥匙，但不要使用它。
        Account const alie {"alie", KeyType::secp256k1};
        env(regkey (alice, alie));
        env.close();
        std::uint32_t aliceSeq = env.seq (alice);
        env(noop(alice), sig(alice));
        env(noop(alice), sig(alie));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 2);

//将签名者附加到Alice
        env(signers(alice, 4, {{becky, 3}, {cheri, 4}}), sig (alice));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 1 : 4));

//尝试满足仲裁的多签名事务。
        auto const baseFee = env.current()->fees().base;
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(cheri), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//如果我们没有达到法定人数，交易就会失败。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky), fee(2 * baseFee), ter(tefBAD_QUORUM));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//给贝基和切丽普通钥匙。
        Account const beck {"beck", KeyType::ed25519};
        env(regkey (becky, beck));
        Account const cher {"cher", KeyType::ed25519};
        env(regkey (cheri, cher));
        env.close();

//贝基和切丽的万能钥匙应该还能用。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky, cheri), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
    }

    void test_regularSigners (FeatureBitset features)
    {
        testcase ("Regular Signers");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};
        Account const cheri {"cheri", KeyType::secp256k1};
        env.fund(XRP(1000), alice, becky, cheri);
        env.close();

//将签名者附加到Alice。
        env(signers(alice, 1, {{becky, 1}, {cheri, 1}}), sig (alice));

//给每个人常规钥匙。
        Account const alie {"alie", KeyType::ed25519};
        env(regkey (alice, alie));
        Account const beck {"beck", KeyType::secp256k1};
        env(regkey (becky, beck));
        Account const cher {"cher", KeyType::ed25519};
        env(regkey (cheri, cher));
        env.close();

//禁用切丽的万能钥匙来混淆事情。
        env(fset (cheri, asfDisableMaster), sig(cheri));
        env.close();

//尝试满足仲裁的多签名事务。
        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq = env.seq (alice);
        env(noop(alice), msig(msig::Reg{cheri, cher}), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//切丽不能用她的万能钥匙进行多重签名。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(cheri), fee(2 * baseFee), ter(tefMASTER_DISABLED));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//贝基应该能够使用她的任何一把钥匙进行多重签名。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(msig::Reg{becky, beck}), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//贝基和切丽都应该能用普通钥匙签名。
        aliceSeq = env.seq (alice);
        env(noop(alice), fee(3 * baseFee),
            msig(msig::Reg{becky, beck}, msig::Reg{cheri, cher}));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
    }

    void test_regularSignersUsingSubmitMulti (FeatureBitset features)
    {
        testcase ("Regular Signers Using submit_multisigned");

        using namespace jtx;
        Env env(*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return cfg;
            }), features);
        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};
        Account const cheri {"cheri", KeyType::secp256k1};
        env.fund(XRP(1000), alice, becky, cheri);
        env.close();

//将签名者附加到Alice。
        env(signers(alice, 2, {{becky, 1}, {cheri, 1}}), sig (alice));

//给每个人常规钥匙。
        Account const beck {"beck", KeyType::secp256k1};
        env(regkey (becky, beck));
        Account const cher {"cher", KeyType::ed25519};
        env(regkey (cheri, cher));
        env.close();

//禁用切丽的万能钥匙来混淆事情。
        env(fset (cheri, asfDisableMaster), sig(cheri));
        env.close();

        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq;

//这些表示下面输入json的经常重复设置
        auto setup_tx = [&]() -> Json::Value {
            Json::Value jv;
            jv[jss::tx_json][jss::Account]         = alice.human();
            jv[jss::tx_json][jss::TransactionType] = "AccountSet";
            jv[jss::tx_json][jss::Fee]             = static_cast<uint32_t>(8 * baseFee);
            jv[jss::tx_json][jss::Sequence]        = env.seq(alice);
            jv[jss::tx_json][jss::SigningPubKey]   = "";
            return jv;
        };
        auto cheri_sign = [&](Json::Value& jv) {
            jv[jss::account]       = cheri.human();
            jv[jss::key_type]      = "ed25519";
            jv[jss::passphrase]    = cher.name();
        };
        auto becky_sign = [&](Json::Value& jv) {
            jv[jss::account] = becky.human();
            jv[jss::secret]  = beck.name();
        };

        {
//尝试满足仲裁的多签名事务。
//使用sign_for和submit_multisigned
            aliceSeq = env.seq (alice);
            Json::Value jv_one = setup_tx();
            cheri_sign(jv_one);
            auto jrr = env.rpc("json", "sign_for", to_string(jv_one))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

//对于第二个符号“for”，使用返回的“tx”json
//第一签名人信息
            Json::Value jv_two;
            jv_two[jss::tx_json] = jrr[jss::tx_json];
            becky_sign(jv_two);
            jrr = env.rpc("json", "sign_for", to_string(jv_two))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

            Json::Value jv_submit;
            jv_submit[jss::tx_json] = jrr[jss::tx_json];
            jrr = env.rpc("json", "submit_multisigned", to_string(jv_submit))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");
            env.close();
            BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
        }

        {
//失败案例--签名PubKey不为空
            aliceSeq = env.seq (alice);
            Json::Value jv_one = setup_tx();
            jv_one[jss::tx_json][jss::SigningPubKey]   = strHex(alice.pk().slice());
            cheri_sign(jv_one);
            auto jrr = env.rpc("json", "sign_for", to_string(jv_one))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message]  == "When multi-signing 'tx_json.SigningPubKey' must be empty.");
        }

        {
//失败案例-不良费用
            aliceSeq = env.seq (alice);
            Json::Value jv_one = setup_tx();
            jv_one[jss::tx_json][jss::Fee] = -1;
            cheri_sign(jv_one);
            auto jrr = env.rpc("json", "sign_for", to_string(jv_one))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

//对于第二个符号“for”，使用返回的“tx”json
//第一签名人信息
            Json::Value jv_two;
            jv_two[jss::tx_json] = jrr[jss::tx_json];
            becky_sign(jv_two);
            jrr = env.rpc("json", "sign_for", to_string(jv_two))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

            Json::Value jv_submit;
            jv_submit[jss::tx_json] = jrr[jss::tx_json];
            jrr = env.rpc("json", "submit_multisigned", to_string(jv_submit))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message]  == "Invalid Fee field.  Fees must be greater than zero.");
        }

        {
//失败案例-不良费用v2
            aliceSeq = env.seq (alice);
            Json::Value jv_one = setup_tx();
            jv_one[jss::tx_json][jss::Fee]  = alice["USD"](10).value().getFullText();
            cheri_sign(jv_one);
            auto jrr = env.rpc("json", "sign_for", to_string(jv_one))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

//对于第二个符号“for”，使用返回的“tx”json
//第一签名人信息
            Json::Value jv_two;
            jv_two[jss::tx_json] = jrr[jss::tx_json];
            becky_sign(jv_two);
            jrr = env.rpc("json", "sign_for", to_string(jv_two))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

            Json::Value jv_submit;
            jv_submit[jss::tx_json] = jrr[jss::tx_json];
            jrr = env.rpc("json", "submit_multisigned", to_string(jv_submit))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "internal");
            BEAST_EXPECT(jrr[jss::error_message]  == "Internal error.");
        }

        {
//切丽不能用她的万能钥匙进行多重签名。
            aliceSeq = env.seq (alice);
            Json::Value jv = setup_tx();
            jv[jss::account]                       = cheri.human();
            jv[jss::secret]                        = cheri.name();
            auto jrr = env.rpc("json", "sign_for", to_string(jv))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "masterDisabled");
            env.close();
            BEAST_EXPECT(env.seq(alice) == aliceSeq);
        }

        {
//与奇瑞不同的是，贝基也应该能够用她的万能钥匙签名。
            aliceSeq = env.seq (alice);
            Json::Value jv_one = setup_tx();
            cheri_sign(jv_one);
            auto jrr = env.rpc("json", "sign_for", to_string(jv_one))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

//对于第二个符号“for”，使用返回的“tx”json
//第一签名人信息
            Json::Value jv_two;
            jv_two[jss::tx_json]    = jrr[jss::tx_json];
            jv_two[jss::account]    = becky.human();
            jv_two[jss::key_type]   = "ed25519";
            jv_two[jss::passphrase] = becky.name();
            jrr = env.rpc("json", "sign_for", to_string(jv_two))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");

            Json::Value jv_submit;
            jv_submit[jss::tx_json] = jrr[jss::tx_json];
            jrr = env.rpc("json", "submit_multisigned", to_string(jv_submit))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "success");
            env.close();
            BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
        }

        {
//检查Tx中是否存在错误或伪造的帐户
            Json::Value jv = setup_tx();
            jv[jss::tx_json][jss::Account]         = "DEADBEEF";
            cheri_sign(jv);
            auto jrr = env.rpc("json", "sign_for", to_string(jv))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "srcActMalformed");

            Account const jimmy {"jimmy"};
            jv[jss::tx_json][jss::Account]         = jimmy.human();
            jrr = env.rpc("json", "sign_for", to_string(jv))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "srcActNotFound");
        }

        {
            aliceSeq = env.seq (alice);
            Json::Value jv = setup_tx();
            jv[jss::tx_json][sfSigners.fieldName]  = Json::Value{Json::arrayValue};
            becky_sign(jv);
            auto jrr = env.rpc("json", "submit_multisigned", to_string(jv))[jss::result];
            BEAST_EXPECT(jrr[jss::status] == "error");
            BEAST_EXPECT(jrr[jss::error]  == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message]  == "tx_json.Signers array may not be empty.");
            env.close();
            BEAST_EXPECT(env.seq(alice) == aliceSeq);
        }
    }

    void test_heterogeneousSigners (FeatureBitset features)
    {
        testcase ("Heterogenious Signers");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};
        Account const cheri {"cheri", KeyType::secp256k1};
        Account const daria {"daria", KeyType::ed25519};
        env.fund(XRP(1000), alice, becky, cheri, daria);
        env.close();

//艾丽斯使用一把普通的钥匙，但禁用了主钥匙。
        Account const alie {"alie", KeyType::secp256k1};
        env(regkey (alice, alie));
        env(fset (alice, asfDisableMaster), sig(alice));

//贝基只有在没有普通钥匙的情况下才是大师。

//Cheri有一个常规的密钥，但仍会启用主密钥。
        Account const cher {"cher", KeyType::secp256k1};
        env(regkey (cheri, cher));

//达里亚有一把普通的钥匙，并禁用了她的万能钥匙。
        Account const dari {"dari", KeyType::ed25519};
        env(regkey (daria, dari));
        env(fset (daria, asfDisableMaster), sig(daria));
        env.close();

//将签名者附加到Alice。
        env(signers(alice, 1,
            {{becky, 1}, {cheri, 1}, {daria, 1}, {jinni, 1}}), sig (alie));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 1 : 6));

//每种类型的签名者都应该分别成功。
        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(cheri), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(msig::Reg{cheri, cher}), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(msig::Reg{daria, dari}), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

        aliceSeq = env.seq (alice);
        env(noop(alice), msig(jinni), fee(2 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//如果所有签名者都签名，也应该有效。
        aliceSeq = env.seq (alice);
        env(noop(alice), fee(5 * baseFee),
            msig(becky, msig::Reg{cheri, cher}, msig::Reg{daria, dari}, jinni));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//要求所有签名者签名。
        env(signers(alice, 0x3FFFC, {{becky, 0xFFFF},
            {cheri, 0xFFFF}, {daria, 0xFFFF}, {jinni, 0xFFFF}}), sig (alie));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 1 : 6));

        aliceSeq = env.seq (alice);
        env(noop(alice), fee(9 * baseFee),
            msig(becky, msig::Reg{cheri, cher}, msig::Reg{daria, dari}, jinni));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//尝试两种关键类型的Cheri。
        aliceSeq = env.seq (alice);
        env(noop(alice), fee(5 * baseFee),
            msig(becky, cheri, msig::Reg{daria, dari}, jinni));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//确保允许的最大签名者数量有效。
        env(signers(alice, 0x7FFF8, {{becky, 0xFFFF}, {cheri, 0xFFFF},
            {daria, 0xFFFF}, {haunt, 0xFFFF}, {jinni, 0xFFFF},
            {phase, 0xFFFF}, {shade, 0xFFFF}, {spook, 0xFFFF}}), sig (alie));
        env.close();
        env.require (owners(alice, features[featureMultiSignReserve] ? 1 : 10));

        aliceSeq = env.seq (alice);
        env(noop(alice), fee(9 * baseFee), msig(becky, msig::Reg{cheri, cher},
            msig::Reg{daria, dari}, haunt, jinni, phase, shade, spook));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//一个短签名者应该失败。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky, cheri, haunt, jinni, phase, shade, spook),
            fee(8 * baseFee), ter (tefBAD_QUORUM));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq);

//删除Alice的签名者列表并返回所有者计数。
        env(signers(alice, jtx::none), sig(alie));
        env.close();
        env.require (owners (alice, 0));
    }

//我们希望始终保留一个可签名的帐户。确保我们
//不允许删除事务的最后签名方式。
    void test_keyDisable (FeatureBitset features)
    {
        testcase ("Key Disable");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        env.fund(XRP(1000), alice);

//我们需要做三个负面测试：
//M0。不能禁用单独的主密钥。
//R0。无法删除唯一的普通密钥。
//L0。无法删除单个签名者列表。
//
//此外，我们需要做6个阳性测试：
//M1。如果有普通钥匙，可以禁用主钥匙。
//M2。如果存在签名者列表，则可以禁用主密钥。
//
//R1如果存在签名者列表，则可以删除常规密钥。
//R2。如果启用了主密钥，则可以删除常规密钥。
//
//L1。如果启用了主密钥，则可以删除签名者列表。
//L2。如果有常规密钥，则可以删除签名者列表。

//主密钥测试。
//M0:不能禁用单独的主密钥。
        env(fset (alice, asfDisableMaster),
            sig(alice), ter(tecNO_ALTERNATIVE_KEY));

//添加常规键。
        Account const alie {"alie", KeyType::ed25519};
        env(regkey (alice, alie));

//M1：如果有普通钥匙，可以禁用万能钥匙。
        env(fset (alice, asfDisableMaster), sig(alice));

//r0:不能删除一个普通的密钥。
        env(regkey (alice, disabled), sig(alie), ter(tecNO_ALTERNATIVE_KEY));

//添加签名者列表。
        env(signers(alice, 1, {{bogie, 1}}), sig (alie));

//R1：如果有签名者列表，就可以删除常规密钥。
        env(regkey (alice, disabled), sig(alie));

//L0:无法删除单个签名者列表。
        auto const baseFee = env.current()->fees().base;
        env(signers(alice, jtx::none), msig(bogie),
            fee(2 * baseFee), ter(tecNO_ALTERNATIVE_KEY));

//启用主密钥。
        env(fclear (alice, asfDisableMaster), msig(bogie), fee(2 * baseFee));

//L1：如果启用了主密钥，则可以删除签名者列表。
        env(signers(alice, jtx::none), msig(bogie), fee(2 * baseFee));

//添加签名者列表。
        env(signers(alice, 1, {{bogie, 1}}), sig (alice));

//M2：如果有签名者列表，可以禁用主密钥。
        env(fset (alice, asfDisableMaster), sig(alice));

//添加常规键。
        env(regkey (alice, alie), msig(bogie), fee(2 * baseFee));

//L2：如果有一个普通的密钥，就可以删除签名者列表。
        env(signers(alice, jtx::none), sig(alie));

//启用主密钥。
        env(fclear (alice, asfDisableMaster), sig(alie));

//R2：如果主钥匙被启用，常规钥匙可以被移除。
        env(regkey (alice, disabled), sig(alie));
    }

//验证是否可以使用
//主钥匙，但不在多重点火时。
    void test_regKey (FeatureBitset features)
    {
        testcase ("Regular Key");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::secp256k1};
        env.fund(XRP(1000), alice);

//给爱丽丝一把免费的普通钥匙。应该成功。曾经。
        Account const alie {"alie", KeyType::ed25519};
        env(regkey (alice, alie), sig (alice), fee (0));

//再试一次，免费创建常规密钥会失败。
        Account const liss {"liss", KeyType::secp256k1};
        env(regkey (alice, liss), sig (alice), fee (0), ter(telINSUF_FEE_P));

//但是，为创建一个常规密钥而付费应该是成功的。
        env(regkey (alice, liss), sig (alice));

//与此相反，尝试对带有零的常规键进行多重签名
//费用总是不及格的。即使是第一次。
        Account const becky {"becky", KeyType::ed25519};
        env.fund(XRP(1000), becky);

        env(signers(becky, 1, {{alice, 1}}), sig (becky));
        env(regkey (becky, alie), msig (alice), fee (0), ter(telINSUF_FEE_P));

//使用主密钥免费签署常规密钥应该
//仍然工作。
        env(regkey (becky, alie), sig (becky), fee (0));
    }

//看看每种事务是否都可以成功地进行多重签名。
    void test_txTypes (FeatureBitset features)
    {
        testcase ("Transaction Types");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};
        Account const zelda {"zelda", KeyType::secp256k1};
        Account const gw {"gw"};
        auto const USD = gw["USD"];
        env.fund(XRP(1000), alice, becky, zelda, gw);
        env.close();

//艾丽斯使用一把普通的钥匙，但禁用了主钥匙。
        Account const alie {"alie", KeyType::secp256k1};
        env(regkey (alice, alie));
        env(fset (alice, asfDisableMaster), sig(alice));

//将签名者附加到Alice。
        env(signers(alice, 2, {{becky, 1}, {bogie, 1}}), sig (alie));
        env.close();
        int const signerListOwners {features[featureMultiSignReserve] ? 1 : 4};
        env.require (owners (alice, signerListOwners + 0));

//多签名TTPayment。
        auto const baseFee = env.current()->fees().base;
        std::uint32_t aliceSeq = env.seq (alice);
        env(pay(alice, env.master, XRP(1)),
            msig(becky, bogie), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//多重签名一个ttaccount_集。
        aliceSeq = env.seq (alice);
        env(noop(alice), msig(becky, bogie), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//多信号一组常规的钥匙。
        aliceSeq = env.seq (alice);
        Account const ace {"ace", KeyType::secp256k1};
        env(regkey (alice, ace), msig(becky, bogie), fee(3 * baseFee));
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);

//多信号tttrust_集
        env(trust("alice", USD(100)),
            msig(becky, bogie), fee(3 * baseFee), require (lines("alice", 1)));
        env.close();
        env.require (owners (alice, signerListOwners + 1));

//多重签名TToffer_创建交易。
        env(pay(gw, alice, USD(50)));
        env.close();
        env.require(balance(alice, USD(50)));
        env.require(balance(gw, alice["USD"](-50)));

        std::uint32_t const offerSeq = env.seq (alice);
        env(offer(alice, XRP(50), USD(50)),
            msig (becky, bogie), fee(3 * baseFee));
        env.close();
        env.require (owners (alice, signerListOwners + 2));

//现在多重签名一个TTOFFER取消取消我们刚刚创建的报价。
        {
            aliceSeq = env.seq (alice);
            Json::Value cancelOffer;
            cancelOffer[jss::Account] = alice.human();
            cancelOffer[jss::OfferSequence] = offerSeq;
            cancelOffer[jss::TransactionType] = "OfferCancel";
            env (cancelOffer, seq (aliceSeq),
                msig (becky, bogie), fee(3 * baseFee));
            env.close();
            BEAST_EXPECT(env.seq(alice) == aliceSeq + 1);
            env.require(owners(alice, signerListOwners + 1));
        }

//多重签名一个ttsigner_list_set。
        env(signers(alice, 3, {{becky, 1}, {bogie, 1}, {demon, 1}}),
            msig (becky, bogie), fee(3 * baseFee));
        env.close();
        env.require (owners (alice, features[featureMultiSignReserve] ? 2 : 6));
    }

    void test_badSignatureText (FeatureBitset features)
    {
        testcase ("Bad Signature Text");

//验证为签名失败返回的文本是否正确。
        using namespace jtx;

        Env env {*this, features};

//lambda，它提交一个sttx并返回结果JSON。
        auto submitSTTx = [&env] (STTx const& stx)
        {
            Json::Value jvResult;
            jvResult[jss::tx_blob] = strHex (stx.getSerializer().slice());
            return env.rpc ("json", "submit", to_string(jvResult));
        };

        Account const alice {"alice"};
        env.fund(XRP(1000), alice);
        env(signers(alice, 1, {{bogie, 1}, {demon, 1}}), sig (alice));

        auto const baseFee = env.current()->fees().base;
        {
//单个签名，但留下一个空的signingpubkey。
            JTx tx = env.jt (noop (alice), sig(alice));
            STTx local = *(tx.stx);
local.setFieldVL (sfSigningPubKey, Blob()); //空的signingpubkey
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Empty SigningPubKey.");
        }
        {
//单一签名，但签名无效。
            JTx tx = env.jt (noop (alice), sig(alice));
            STTx local = *(tx.stx);
//在签名中翻转一些位。
            auto badSig = local.getFieldVL (sfTxnSignature);
            badSig[20] ^= 0xAA;
            local.setFieldVL (sfTxnSignature, badSig);
//签名应该失败。
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                    "fails local checks: Invalid signature.");
        }
        {
//单一符号，但序列号无效。
            JTx tx = env.jt (noop (alice), sig(alice));
            STTx local = *(tx.stx);
//在签名中翻转一些位。
            auto seq = local.getFieldU32 (sfSequence);
            local.setFieldU32 (sfSequence, seq + 1);
//签名应该失败。
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                    "fails local checks: Invalid signature.");
        }
        {
//多重签名，但留下一个非空的sfsigningpubkey。
            JTx tx = env.jt (noop (alice), fee(2 * baseFee), msig(bogie));
            STTx local = *(tx.stx);
local[sfSigningPubKey] = alice.pk(); //插入sfsigningpubkey
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Cannot both single- and multi-sign.");
        }
        {
//带有空signingpubkey的多符号和单符号。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(bogie));
            STTx local = *(tx.stx);
            local.sign (alice.pk(), alice.sk());
local.setFieldVL (sfSigningPubKey, Blob()); //空的signingpubkey
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Cannot both single- and multi-sign.");
        }
        {
//多重签名，但其中一个签名无效。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(bogie));
            STTx local = *(tx.stx);
//在签名中翻转一些位。
            auto& signer = local.peekFieldArray (sfSigners).back();
            auto badSig = signer.getFieldVL (sfTxnSignature);
            badSig[20] ^= 0xAA;
            signer.setFieldVL (sfTxnSignature, badSig);
//签名应该失败。
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception].asString().
                find ("Invalid signature on account r") != std::string::npos);
        }
        {
//带有空签名者数组的多重签名应该失败。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(bogie));
            STTx local = *(tx.stx);
local.peekFieldArray (sfSigners).clear(); //空的签名者数组。
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                    "fails local checks: Invalid Signers array size.");
        }
        {
//多信号9次应失败。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee),
                msig(bogie, bogie, bogie,
                    bogie, bogie, bogie, bogie, bogie, bogie));
            STTx local = *(tx.stx);
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Invalid Signers array size.");
        }
        {
//帐户所有者不能为自己进行多重签名。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(alice));
            STTx local = *(tx.stx);
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Invalid multisigner.");
        }
        {
//不允许重复的多重签名。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(bogie, bogie));
            STTx local = *(tx.stx);
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Duplicate Signers not allowed.");
        }
        {
//多签名必须按排序顺序提交。
            JTx tx = env.jt (noop(alice), fee(2 * baseFee), msig(bogie, demon));
            STTx local = *(tx.stx);
//取消对签名者数组的排序。
            auto& signers = local.peekFieldArray (sfSigners);
            std::reverse (signers.begin(), signers.end());
//签名应该失败。
            auto const info = submitSTTx (local);
            BEAST_EXPECT(info[jss::result][jss::error_exception] ==
                "fails local checks: Unsorted Signers array.");
        }
    }

    void test_noMultiSigners (FeatureBitset features)
    {
        testcase ("No Multisigners");

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        Account const becky {"becky", KeyType::secp256k1};
        env.fund(XRP(1000), alice, becky);
        env.close();

        auto const baseFee = env.current()->fees().base;
        env(noop(alice), msig(becky, demon), fee(3 * baseFee), ter(tefNOT_MULTI_SIGNING));
    }

    void test_multisigningMultisigner (FeatureBitset features)
    {
        testcase ("Multisigning multisigner");

//设置签名者列表，其中一个签名者同时具有
//主控形状已禁用，没有常规密钥（因为签名者
//完全多点火）。那个签名者不应该再
//能够成功签署签名者列表。

        using namespace jtx;
        Env env {*this, features};
        Account const alice {"alice", KeyType::ed25519};
        Account const becky {"becky", KeyType::secp256k1};
        env.fund (XRP(1000), alice, becky);
        env.close();

//爱丽丝用贝基作为签名者建立了一个签名者列表。
        env (signers (alice, 1, {{becky, 1}}));
        env.close();

//贝基建立了她的签名人名单。
        env (signers (becky, 1, {{bogie, 1}, {demon, 1}}));
        env.close();

//因为贝基还没有禁用她的万能钥匙，她可以
//为Alice多签名交易。
        auto const baseFee = env.current()->fees().base;
        env (noop (alice), msig (becky), fee (2 * baseFee));
        env.close();

//现在贝基禁用了她的万能钥匙。
        env (fset (becky, asfDisableMaster));
        env.close();

//因为贝基的万能钥匙坏了，她再也不能了
//爱丽丝的多重签名。
        env (noop (alice), msig (becky), fee (2 * baseFee),
            ter (tefMASTER_DISABLED));
        env.close();

//贝基不能为爱丽丝二级多信号。2级多点点火
//不支持。
        env (noop (alice), msig (msig::Reg {becky, bogie}), fee (2 * baseFee),
            ter (tefBAD_SIGNATURE));
        env.close();

//验证Becky不能用她拥有的常规密钥签名
//尚未启用。
        Account const beck {"beck", KeyType::ed25519};
        env (noop (alice), msig (msig::Reg {becky, beck}), fee (2 * baseFee),
            ter (tefBAD_SIGNATURE));
        env.close();

//一旦贝基给了自己一把普通钥匙，她就可以为爱丽丝签名了。
//使用普通钥匙。
        env (regkey (becky, beck), msig (demon), fee (2 * baseFee));
        env.close();

        env (noop (alice), msig (msig::Reg {becky, beck}), fee (2 * baseFee));
        env.close();

//贝基的普通钥匙的存在并不影响她是否
//可以二级多信号；它仍然不能工作。
        env (noop (alice), msig (msig::Reg {becky, demon}), fee (2 * baseFee),
            ter (tefBAD_SIGNATURE));
        env.close();
    }

    void test_signForHash (FeatureBitset features)
    {
        testcase ("sign_for Hash");

//确保“sign_for”rpc返回的“hash”字段
//命令与发送该命令时返回的哈希匹配
//通过“提交多个签名”。确保哈希也位于
//分类帐中的交易记录。
        using namespace jtx;
        Account const alice {"alice", KeyType::ed25519};

        Env env(*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return cfg;
            }), features);
        env.fund (XRP(1000), alice);
        env.close();

        env (signers (alice, 2, {{bogie, 1}, {ghost, 1}}));
        env.close();

//使用Sign_for签署Alice向其支付10 XRP的交易
//主密码。
        std::uint32_t baseFee = env.current()->fees().base;
        Json::Value jvSig1;
        jvSig1[jss::account] = bogie.human();
        jvSig1[jss::secret]  = bogie.name();
        jvSig1[jss::tx_json][jss::Account]         = alice.human();
        jvSig1[jss::tx_json][jss::Amount]          = 10000000;
        jvSig1[jss::tx_json][jss::Destination]     = env.master.human();
        jvSig1[jss::tx_json][jss::Fee]             = 3 * baseFee;
        jvSig1[jss::tx_json][jss::Sequence]        = env.seq(alice);
        jvSig1[jss::tx_json][jss::TransactionType] = "Payment";

        Json::Value jvSig2 = env.rpc (
            "json", "sign_for", to_string (jvSig1));
        BEAST_EXPECT (
            jvSig2[jss::result][jss::status].asString() == "success");

//用一个签名保存哈希以供以后使用。
        std::string const hash1 =
            jvSig2[jss::result][jss::tx_json][jss::hash].asString();

//添加下一个签名并再次签名。
        jvSig2[jss::result][jss::account] = ghost.human();
        jvSig2[jss::result][jss::secret]  = ghost.name();
        Json::Value jvSubmit = env.rpc (
            "json", "sign_for", to_string (jvSig2[jss::result]));
        BEAST_EXPECT (
            jvSubmit[jss::result][jss::status].asString() == "success");

//用两个签名保存哈希，以便以后使用。
        std::string const hash2 =
            jvSubmit[jss::result][jss::tx_json][jss::hash].asString();
        BEAST_EXPECT (hash1 != hash2);

//提交两个签名的结果。
        Json::Value jvResult = env.rpc (
            "json", "submit_multisigned", to_string (jvSubmit[jss::result]));
        BEAST_EXPECT (
            jvResult[jss::result][jss::status].asString() == "success");
        BEAST_EXPECT (jvResult[jss::result]
            [jss::engine_result].asString() == "tesSUCCESS");

//提交的哈希值应与
//第二次签字。
        BEAST_EXPECT (
            hash2 == jvResult[jss::result][jss::tx_json][jss::hash].asString());
        env.close();

//我们刚刚提交的交易现在应该可以使用，并且
//验证。
        Json::Value jvTx = env.rpc ("tx", hash2);
        BEAST_EXPECT (jvTx[jss::result][jss::status].asString() == "success");
        BEAST_EXPECT (jvTx[jss::result][jss::validated].asString() == "true");
        BEAST_EXPECT (jvTx[jss::result][jss::meta]
            [sfTransactionResult.jsonName].asString() == "tesSUCCESS");
    }

    void test_amendmentTransition ()
    {
        testcase ("Amendment Transition");

//与签名者列表关联的所有者计数一旦
//功能多信号保留修正生效。创造一对夫妇
//修正案生效前后签署人名单
//验证是否为所有所有者正确管理了所有者计数。
        using namespace jtx;
        Account const alice {"alice", KeyType::secp256k1};
        Account const becky {"becky", KeyType::ed25519};
        Account const cheri {"cheri", KeyType::secp256k1};
        Account const daria {"daria", KeyType::ed25519};

        Env env {*this, supported_amendments() - featureMultiSignReserve};
        env.fund (XRP(1000), alice, becky, cheri, daria);
        env.close();

//在修正案生效前给爱丽丝和贝基签名人名单。
        env (signers (alice, 1, {{bogie, 1}}));
        env (signers (becky, 1, {{bogie, 1}, {demon, 1}, {ghost, 1},
            {haunt, 1}, {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}}));
        env.close();

        env.require (owners (alice, 3));
        env.require (owners (becky, 10));

//启用修正。
        env.enableFeature (featureMultiSignReserve);
        env.close();

//修正案生效后，给切丽和达里亚签名人名单。
        env (signers (cheri, 1, {{bogie, 1}}));
        env (signers (daria, 1, {{bogie, 1}, {demon, 1}, {ghost, 1},
            {haunt, 1}, {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}}));
        env.close();

        env.require (owners (alice, 3));
        env.require (owners (becky, 10));
        env.require (owners (cheri, 1));
        env.require (owners (daria, 1));

//删除贝基的签名者列表；她的所有者计数应降至零。
//替换Alice的签名者列表；她的所有者计数应降至1。
        env (signers (becky, jtx::none));
        env (signers (alice, 1, {{bogie, 1}, {demon, 1}, {ghost, 1},
            {haunt, 1}, {jinni, 1}, {phase, 1}, {shade, 1}, {spook, 1}}));
        env.close();

        env.require (owners (alice, 1));
        env.require (owners (becky, 0));
        env.require (owners (cheri, 1));
        env.require (owners (daria, 1));

//删除其余三个签名者列表。每个人的所有者计数
//现在应该是零。
        env (signers (alice, jtx::none));
        env (signers (cheri, jtx::none));
        env (signers (daria, jtx::none));
        env.close();

        env.require (owners (alice, 0));
        env.require (owners (becky, 0));
        env.require (owners (cheri, 0));
        env.require (owners (daria, 0));
    }

    void testAll(FeatureBitset features)
    {
        test_noReserve (features);
        test_signerListSet (features);
        test_phantomSigners (features);
        test_enablement (features);
        test_fee (features);
        test_misorderedSigners (features);
        test_masterSigners (features);
        test_regularSigners (features);
        test_regularSignersUsingSubmitMulti (features);
        test_heterogeneousSigners (features);
        test_keyDisable (features);
        test_regKey (features);
        test_txTypes (features);
        test_badSignatureText (features);
        test_noMultiSigners (features);
        test_multisigningMultisigner (features);
        test_signForHash (features);
    }

    void run() override
    {
        using namespace jtx;
        auto const all = supported_amendments();

//签名者列表所需的保留基于更改。
//特征多信号保留。有无测试。
        testAll (all - featureMultiSignReserve);
        testAll (all | featureMultiSignReserve);
        test_amendmentTransition();
    }
};

BEAST_DEFINE_TESTSUITE(MultiSign, app, ripple);

} //测试
} //涟漪
