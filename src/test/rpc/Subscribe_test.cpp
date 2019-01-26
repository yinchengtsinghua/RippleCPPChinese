
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

#include <ripple/app/main/LoadManager.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx/WSClient.h>
#include <test/jtx/envconfig.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class Subscribe_test : public beast::unit_test::suite
{
public:
    void testServer()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        Json::Value stream;

        {
//RPC订阅服务器流
            stream[jss::streams] = Json::arrayValue;
            stream[jss::streams].append("server");
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

//在这里，我们强制停止负载管理器，因为它可以（很少但是
//在我们打电话给
//在我们取消订阅之前先查找，从而导致最终
//findmsg检查失败，因为创建了一个未处理的ws msg
//由LoadManager提供
        env.app().getLoadManager().onStop();
        {
//提高费用以进行更新
            auto& feeTrack = env.app().getFeeTrack();
            for(int i = 0; i < 5; ++i)
                feeTrack.raiseLocalFee();
            env.app().getOPs().reportFeeChange();

//检查流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::type] == "serverStatus";
                }));
        }

        {
//取消订阅
            auto jv = wsc->invoke("unsubscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
//提高费用以进行更新
            auto& feeTrack = env.app().getFeeTrack();
            for (int i = 0; i < 5; ++i)
                feeTrack.raiseLocalFee();
            env.app().getOPs().reportFeeChange();

//检查流更新
            auto jvo = wsc->getMsg(10ms);
            BEAST_EXPECTS(!jvo, "getMsg: " + to_string(jvo.get()) );
        }
    }

    void testLedger()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        Json::Value stream;

        {
//RPC订阅分类帐流
            stream[jss::streams] = Json::arrayValue;
            stream[jss::streams].append("ledger");
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::ledger_index] == 2);
        }

        {
//接受分类帐
            env.close();

//检查流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::ledger_index] == 3;
                }));
        }

        {
//接受另一个分类帐
            env.close();

//检查流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::ledger_index] == 4;
                }));
        }

//取消订阅
        auto jv = wsc->invoke("unsubscribe", stream);
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
        }
        BEAST_EXPECT(jv[jss::status] == "success");
    }

    void testTransactions()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        Json::Value stream;

        {
//RPC订阅事务流
            stream[jss::streams] = Json::arrayValue;
            stream[jss::streams].append("transactions");
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
            env.fund(XRP(10000), "alice");
            env.close();

//检查付款交易记录的流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][1u]
                        ["CreatedNode"]["NewFields"][jss::Account] ==
                            Account("alice").human();
                }));

//检查帐户集事务的流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][0u]
                        ["ModifiedNode"]["FinalFields"][jss::Account] ==
                            Account("alice").human();
                }));

            env.fund(XRP(10000), "bob");
            env.close();

//检查付款交易记录的流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][1u]
                        ["CreatedNode"]["NewFields"][jss::Account] ==
                            Account("bob").human();
                }));

//检查帐户集事务的流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][0u]
                        ["ModifiedNode"]["FinalFields"][jss::Account] ==
                            Account("bob").human();
                }));
        }

        {
//取消订阅
            auto jv = wsc->invoke("unsubscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
//RPC订阅帐户流
            stream = Json::objectValue;
            stream[jss::accounts] = Json::arrayValue;
            stream[jss::accounts].append(Account("alice").human());
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
//不影响流的事务
            env.fund(XRP(10000), "carol");
            env.close();
            BEAST_EXPECT(! wsc->getMsg(10ms));

//关于Alice的交易
            env.trust(Account("bob")["USD"](100), "alice");
            env.close();

//检查流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][1u]
                        ["ModifiedNode"]["FinalFields"][jss::Account] ==
                            Account("alice").human();
                }));

            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::meta]["AffectedNodes"][1u]
                        ["CreatedNode"]["NewFields"]["LowLimit"]
                            [jss::issuer] == Account("alice").human();
                }));
        }

//取消订阅
        auto jv = wsc->invoke("unsubscribe", stream);
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
        }
        BEAST_EXPECT(jv[jss::status] == "success");
    }

    void testManifests()
    {
        using namespace jtx;
        Env env(*this);
        auto wsc = makeWSClient(env.app().config());
        Json::Value stream;

        {
//RPC订阅清单流
            stream[jss::streams] = Json::arrayValue;
            stream[jss::streams].append("manifests");
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

//取消订阅
        auto jv = wsc->invoke("unsubscribe", stream);
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
        }
        BEAST_EXPECT(jv[jss::status] == "success");
    }

    void testValidations()
    {
        using namespace jtx;

        Env env {*this, envconfig(validator, "")};
        auto& cfg = env.app().config();
        if(! BEAST_EXPECT(cfg.section(SECTION_VALIDATION_SEED).empty()))
            return;
        auto const parsedseed = parseBase58<Seed>(
            cfg.section(SECTION_VALIDATION_SEED).values()[0]);
        if(! BEAST_EXPECT(parsedseed))
            return;

        std::string const valPublicKey =
            toBase58 (TokenType::NodePublic,
                derivePublicKey (KeyType::secp256k1,
                    generateSecretKey (KeyType::secp256k1, *parsedseed)));

        auto wsc = makeWSClient(env.app().config());
        Json::Value stream;

        {
//RPC订阅验证流
            stream[jss::streams] = Json::arrayValue;
            stream[jss::streams].append("validations");
            auto jv = wsc->invoke("subscribe", stream);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
//接受分类帐
            env.close();

//检查流更新
            using namespace std::chrono_literals;
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::type] == "validationReceived" &&
                        jv[jss::validation_public_key].asString() ==
                            valPublicKey &&
                        jv[jss::ledger_hash] ==
                            to_string(env.closed()->info().hash) &&
                        jv[jss::ledger_index] ==
                            to_string(env.closed()->info().seq) &&
                        jv[jss::flags] ==
                            (vfFullyCanonicalSig | STValidation::kFullFlag) &&
                        jv[jss::full] == true &&
                        !jv.isMember(jss::load_fee) &&
                        jv[jss::signature] &&
                        jv[jss::signing_time];
                }));
        }

//取消订阅
        auto jv = wsc->invoke("unsubscribe", stream);
        if (wsc->version() == 2)
        {
            BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
            BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
        }
        BEAST_EXPECT(jv[jss::status] == "success");
    }

    void
    testSubByUrl()
    {
        using namespace jtx;
        testcase("Subscribe by url");
        Env env {*this};

        Json::Value jv;
jv[jss::url] = "http://本地主机/事件“；
        jv[jss::url_username] = "admin";
        jv[jss::url_password] = "password";
        jv[jss::streams] = Json::arrayValue;
        jv[jss::streams][0u] = "validations";
        auto jr = env.rpc("json", "subscribe", to_string(jv)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");

        jv[jss::streams][0u] = "ledger";
        jr = env.rpc("json", "subscribe", to_string(jv)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");

        jr = env.rpc("json", "unsubscribe", to_string(jv)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");

        jv[jss::streams][0u] = "validations";
        jr = env.rpc("json", "unsubscribe", to_string(jv)) [jss::result];
        BEAST_EXPECT(jr[jss::status] == "success");
    }

    void
    testSubErrors(bool subscribe)
    {
        using namespace jtx;
        auto const method = subscribe ? "subscribe" : "unsubscribe";
        testcase << "Error cases for " << method;

        Env env {*this};
        auto wsc = makeWSClient(env.app().config());

        {
            auto jr = env.rpc("json", method, "{}") [jss::result];
            BEAST_EXPECT(jr[jss::error] == "invalidParams");
            BEAST_EXPECT(jr[jss::error_message] == "Invalid parameters.");
        }

        {
            Json::Value jv;
            jv[jss::url] = "not-a-url";
            jv[jss::username] = "admin";
            jv[jss::password] = "password";
            auto jr = env.rpc("json", method, to_string(jv)) [jss::result];
            if (subscribe)
            {
                BEAST_EXPECT(jr[jss::error] == "invalidParams");
                BEAST_EXPECT(jr[jss::error_message] == "Failed to parse url.");
            }
//否则TODO:为什么这不是取消订阅的错误？
//（findrpcsub返回空值）
        }

        {
            Json::Value jv;
jv[jss::url] = "ftp://scheme.not.supported.tld“；
            auto jr = env.rpc("json", method, to_string(jv)) [jss::result];
            if (subscribe)
            {
                BEAST_EXPECT(jr[jss::error] == "invalidParams");
                BEAST_EXPECT(jr[jss::error_message] == "Only http and https is supported.");
            }
        }

        {
            Env env_nonadmin {*this, no_admin(envconfig(port_increment, 3))};
            Json::Value jv;
            jv[jss::url] = "no-url";
            auto jr = env_nonadmin.rpc("json", method, to_string(jv)) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "noPermission");
            BEAST_EXPECT(jr[jss::error_message] == "You don't have permission for this command.");
        }

        std::initializer_list<Json::Value> const nonArrays {Json::nullValue,
            Json::intValue, Json::uintValue, Json::realValue, "",
            Json::booleanValue, Json::objectValue};

        for (auto const& f : {jss::accounts_proposed, jss::accounts})
        {
            for (auto const& nonArray : nonArrays)
            {
                Json::Value jv;
                jv[f] = nonArray;
                auto jr = wsc->invoke(method, jv) [jss::result];
                BEAST_EXPECT(jr[jss::error] == "invalidParams");
                BEAST_EXPECT(jr[jss::error_message] == "Invalid parameters.");
            }

            {
                Json::Value jv;
                jv[f] = Json::arrayValue;
                auto jr = wsc->invoke(method, jv) [jss::result];
                BEAST_EXPECT(jr[jss::error] == "actMalformed");
                BEAST_EXPECT(jr[jss::error_message] == "Account malformed.");
            }
        }

        for (auto const& nonArray : nonArrays)
        {
            Json::Value jv;
            jv[jss::books] = nonArray;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "invalidParams");
            BEAST_EXPECT(jr[jss::error_message] == "Invalid parameters.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = 1;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "invalidParams");
            BEAST_EXPECT(jr[jss::error_message] == "Invalid parameters.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_gets] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Json::objectValue;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "srcCurMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Source currency is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_gets] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays][jss::currency] = "ZZZZ";
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "srcCurMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Source currency is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_gets] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays][jss::currency] = "USD";
            jv[jss::books][0u][jss::taker_pays][jss::issuer] = 1;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "srcIsrMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Source issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_gets] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays][jss::currency] = "USD";
            jv[jss::books][0u][jss::taker_pays][jss::issuer] = Account{"gateway"}.human() + "DEAD";
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "srcIsrMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Source issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Account{"gateway"}["USD"](1).value().getJson(1);
            jv[jss::books][0u][jss::taker_gets] = Json::objectValue;
            auto jr = wsc->invoke(method, jv) [jss::result];
//注意：此错误与
//等值源货币错误
            BEAST_EXPECT(jr[jss::error] == "dstAmtMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Destination amount/currency/issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Account{"gateway"}["USD"](1).value().getJson(1);
            jv[jss::books][0u][jss::taker_gets][jss::currency] = "ZZZZ";
            auto jr = wsc->invoke(method, jv) [jss::result];
//注意：此错误与
//等值源货币错误
            BEAST_EXPECT(jr[jss::error] == "dstAmtMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Destination amount/currency/issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Account{"gateway"}["USD"](1).value().getJson(1);
            jv[jss::books][0u][jss::taker_gets][jss::currency] = "USD";
            jv[jss::books][0u][jss::taker_gets][jss::issuer] = 1;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "dstIsrMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Destination issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Account{"gateway"}["USD"](1).value().getJson(1);
            jv[jss::books][0u][jss::taker_gets][jss::currency] = "USD";
            jv[jss::books][0u][jss::taker_gets][jss::issuer] = Account{"gateway"}.human() + "DEAD";
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "dstIsrMalformed");
            BEAST_EXPECT(jr[jss::error_message] == "Destination issuer is malformed.");
        }

        {
            Json::Value jv;
            jv[jss::books] = Json::arrayValue;
            jv[jss::books][0u] = Json::objectValue;
            jv[jss::books][0u][jss::taker_pays] = Account{"gateway"}["USD"](1).value().getJson(1);
            jv[jss::books][0u][jss::taker_gets] = Account{"gateway"}["USD"](1).value().getJson(1);
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "badMarket");
            BEAST_EXPECT(jr[jss::error_message] == "No such market.");
        }

        for (auto const& nonArray : nonArrays)
        {
            Json::Value jv;
            jv[jss::streams] = nonArray;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "invalidParams");
            BEAST_EXPECT(jr[jss::error_message] == "Invalid parameters.");
        }

        {
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams][0u] = 1;
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "malformedStream");
            BEAST_EXPECT(jr[jss::error_message] == "Stream malformed.");
        }

        {
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams][0u] = "not_a_stream";
            auto jr = wsc->invoke(method, jv) [jss::result];
            BEAST_EXPECT(jr[jss::error] == "malformedStream");
            BEAST_EXPECT(jr[jss::error_message] == "Stream malformed.");
        }

    }

    void run() override
    {
        testServer();
        testLedger();
        testTransactions();
        testManifests();
        testValidations();
        testSubErrors(true);
        testSubErrors(false);
        testSubByUrl();
    }
};

BEAST_DEFINE_TESTSUITE(Subscribe,app,ripple);

} //测试
} //涟漪
