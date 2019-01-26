
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

#include <test/jtx.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <test/jtx/WSClient.h>

namespace ripple {

class AmendmentBlocked_test : public beast::unit_test::suite
{
    void testBlockedMethods()
    {
        using namespace test::jtx;
        Env env {*this, envconfig([](std::unique_ptr<Config> cfg)
            {
                cfg->loadFromString ("[" SECTION_SIGNING_SUPPORT "]\ntrue");
                return cfg;
            })};
        auto const gw = Account {"gateway"};
        auto const USD = gw["USD"];
        auto const alice = Account {"alice"};
        auto const bob = Account {"bob"};
        env.fund (XRP(10000), alice, bob, gw);
        env.trust (USD(600), alice);
        env.trust (USD(700), bob);
        env(pay (gw, alice, USD(70)));
        env(pay (gw, bob, USD(50)));
        env.close();

        auto wsc = test::makeWSClient(env.app().config());

        auto current = env.current ();
//莱德格尔纳
        auto jr = env.rpc ("ledger_accept") [jss::result];
        BEAST_EXPECT (jr[jss::ledger_current_index] == current->seq ()+1);

//莱德格尔电流
        jr = env.rpc ("ledger_current") [jss::result];
        BEAST_EXPECT (jr[jss::ledger_current_index] == current->seq ()+1);

//所有者信息
        jr = env.rpc ("owner_info", alice.human()) [jss::result];
        BEAST_EXPECT (jr.isMember (jss::accepted) && jr.isMember (jss::current));

//路径查找
        Json::Value pf_req;
        pf_req[jss::subcommand] = "create";
        pf_req[jss::source_account] = alice.human();
        pf_req[jss::destination_account] = bob.human();
        pf_req[jss::destination_amount] = bob["USD"](20).value ().getJson (0);
        jr = wsc->invoke("path_find", pf_req) [jss::result];
        BEAST_EXPECT (jr.isMember (jss::alternatives) &&
            jr[jss::alternatives].isArray() &&
            jr[jss::alternatives].size () == 1);

//提交
        auto jt = env.jt (noop (alice));
        Serializer s;
        jt.stx->add (s);
        jr = env.rpc ("submit", strHex (s.slice ())) [jss::result];
        BEAST_EXPECT (jr.isMember (jss::engine_result) &&
            jr[jss::engine_result] == "tesSUCCESS");

//提交多签名
        env(signers(bob, 1, {{alice, 1}}), sig (bob));
        Account const ali {"ali", KeyType::secp256k1};
        env(regkey (alice, ali));
        env.close();

        Json::Value set_tx;
        set_tx[jss::Account] = bob.human();
        set_tx[jss::TransactionType] = "AccountSet";
        set_tx[jss::Fee] =
            static_cast<uint32_t>(8 * env.current()->fees().base);
        set_tx[jss::Sequence] = env.seq(bob);
        set_tx[jss::SigningPubKey] = "";

        Json::Value sign_for;
        sign_for[jss::tx_json] = set_tx;
        sign_for[jss::account] = alice.human();
        sign_for[jss::secret]  = ali.name();
        jr = env.rpc("json", "sign_for", to_string(sign_for)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");

        Json::Value ms_req;
        ms_req[jss::tx_json] = jr[jss::tx_json];
        jr = env.rpc("json", "submit_multisigned", to_string(ms_req))
            [jss::result];
        BEAST_EXPECT (jr.isMember (jss::engine_result) &&
            jr[jss::engine_result] == "tesSUCCESS");

//阻止网络修正…现在一切都一样
//请求应失败

        env.app ().getOPs ().setAmendmentBlocked ();

//莱德格尔纳
        jr = env.rpc ("ledger_accept") [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
        BEAST_EXPECT(jr[jss::status] == "error");

//莱德格尔电流
        jr = env.rpc ("ledger_current") [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
        BEAST_EXPECT(jr[jss::status] == "error");

//所有者信息
        jr = env.rpc ("owner_info", alice.human()) [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
        BEAST_EXPECT(jr[jss::status] == "error");

//路径查找
        jr = wsc->invoke("path_find", pf_req) [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
        BEAST_EXPECT(jr[jss::status] == "error");

//提交
        jr = env.rpc("submit", strHex(s.slice())) [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
        BEAST_EXPECT(jr[jss::status] == "error");

//提交多签名
        set_tx[jss::Sequence] = env.seq(bob);
        sign_for[jss::tx_json] = set_tx;
        jr = env.rpc("json", "sign_for", to_string(sign_for)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");
        ms_req[jss::tx_json] = jr[jss::tx_json];
        jr = env.rpc("json", "submit_multisigned", to_string(ms_req))
            [jss::result];
        BEAST_EXPECT(
            jr.isMember (jss::error) &&
            jr[jss::error] == "amendmentBlocked");
    }

public:
    void run() override
    {
        testBlockedMethods();
    }
};

BEAST_DEFINE_TESTSUITE(AmendmentBlocked,app,ripple);

}

