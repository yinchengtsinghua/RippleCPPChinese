
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
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>

namespace ripple {

class LedgerData_test : public beast::unit_test::suite
{
public:
//测试助手
    static bool checkArraySize(Json::Value const& val, unsigned int size)
    {
        return val.isArray() &&
               val.size() == size;
    }

//测试助手
    static bool checkMarker(Json::Value const& val)
    {
        return val.isMember(jss::marker) &&
               val[jss::marker].isString() &&
               val[jss::marker].asString().size() > 0;
    }

    void testCurrentLedgerToLimits(bool asAdmin)
    {
        using namespace test::jtx;
        Env env {*this, asAdmin ? envconfig() : envconfig(no_admin)};
        Account const gw {"gateway"};
        auto const USD = gw["USD"];
        env.fund(XRP(100000), gw);

int const max_limit = 256; //对于二进制请求是2048，不需要在这里测试它

        for (auto i = 0; i < max_limit + 10; i++)
        {
            Account const bob {std::string("bob") + std::to_string(i)};
            env.fund(XRP(1000), bob);
        }
        env.close();

//如果没有指定限制，我们将得到最大限制，如果
//帐户大于max，它在这里
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::binary]       = false;
        auto const jrr = env.rpc ( "json", "ledger_data",
            boost::lexical_cast<std::string>(jvParams)) [jss::result];
        BEAST_EXPECT(
            jrr[jss::ledger_current_index].isIntegral() &&
            jrr[jss::ledger_current_index].asInt() > 0 );
        BEAST_EXPECT( checkMarker(jrr) );
        BEAST_EXPECT( checkArraySize(jrr[jss::state], max_limit) );

//检查最大限值（+/-1）周围的限值
        for (auto delta = -1; delta <= 1; delta++)
        {
            jvParams[jss::limit] = max_limit + delta;
            auto const jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(
                checkArraySize( jrr[jss::state],
                    (delta > 0 && !asAdmin) ? max_limit : max_limit + delta ));
        }
    }

    void testCurrentLedgerBinary()
    {
        using namespace test::jtx;
        Env env { *this, envconfig(no_admin) };
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund(XRP(100000), gw);

        int const num_accounts = 10;

        for (auto i = 0; i < num_accounts; i++)
        {
            Account const bob { std::string("bob") + std::to_string(i) };
            env.fund(XRP(1000), bob);
        }
        env.close();

//如果没有规定限额，我们将获得所有基金项目
//另外还有三个与网关设置相关的
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::binary]       = true;
        auto const jrr = env.rpc ( "json", "ledger_data",
            boost::lexical_cast<std::string>(jvParams)) [jss::result];
        BEAST_EXPECT(
            jrr[jss::ledger_current_index].isIntegral() &&
            jrr[jss::ledger_current_index].asInt() > 0);
        BEAST_EXPECT( ! jrr.isMember(jss::marker) );
        BEAST_EXPECT( checkArraySize(jrr[jss::state], num_accounts + 3) );
    }

    void testBadInput()
    {
        using namespace test::jtx;
        Env env { *this };
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        Account const bob { "bob" };

        env.fund(XRP(10000), gw, bob);
        env.trust(USD(1000), bob);

        {
//坏界限
            Json::Value jvParams;
jvParams[jss::limit] = "0"; //不是整数
            auto const jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'limit', not integer.");
        }

        {
//无效标记
            Json::Value jvParams;
            jvParams[jss::marker] = "NOT_A_MARKER";
            auto const jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'marker', not valid.");
        }

        {
//无效标记-不是字符串
            Json::Value jvParams;
            jvParams[jss::marker] = 1;
            auto const jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "Invalid field 'marker', not valid.");
        }

        {
//请求坏的分类帐索引
            Json::Value jvParams;
            jvParams[jss::ledger_index] = 10u;
            auto const jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::status]        == "error");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }
    }

    void testMarkerFollow()
    {
        using namespace test::jtx;
        Env env { *this, envconfig(no_admin) };
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund(XRP(100000), gw);

        int const num_accounts = 20;

        for (auto i = 0; i < num_accounts; i++)
        {
            Account const bob { std::string("bob") + std::to_string(i) };
            env.fund(XRP(1000), bob);
        }
        env.close();

//如果没有规定限额，我们将获得所有基金项目
//另外还有三个与网关设置相关的
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::binary]       = false;
        auto jrr = env.rpc ( "json", "ledger_data",
            boost::lexical_cast<std::string>(jvParams)) [jss::result];
        auto const total_count = jrr[jss::state].size();

//现在用一个限制和循环提出请求，直到我们得到所有
        jvParams[jss::limit]        = 5;
        jrr = env.rpc ( "json", "ledger_data",
            boost::lexical_cast<std::string>(jvParams)) [jss::result];
        BEAST_EXPECT( checkMarker(jrr) );
        auto running_total = jrr[jss::state].size();
        while ( jrr.isMember(jss::marker) )
        {
            jvParams[jss::marker] = jrr[jss::marker];
            jrr = env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            running_total += jrr[jss::state].size();
        }
        BEAST_EXPECT( running_total == total_count );
    }

    void testLedgerHeader()
    {
        using namespace test::jtx;
        Env env { *this };
        env.fund(XRP(100000), "alice");
        env.close();

//第一个查询中应存在分类帐标题
        {
//非二进制形式的已关闭分类帐
            Json::Value jvParams;
            jvParams[jss::ledger_index] = "closed";
            auto jrr = env.rpc("json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            if (BEAST_EXPECT(jrr.isMember(jss::ledger)))
                BEAST_EXPECT(jrr[jss::ledger][jss::ledger_hash] ==
                    to_string(env.closed()->info().hash));
        }
        {
//以二进制形式关闭的分类帐
            Json::Value jvParams;
            jvParams[jss::ledger_index] = "closed";
            jvParams[jss::binary] = true;
            auto jrr = env.rpc("json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            if (BEAST_EXPECT(jrr.isMember(jss::ledger)))
            {
                auto data = strUnHex(
                    jrr[jss::ledger][jss::ledger_data].asString());
                if (BEAST_EXPECT(data.second))
                {
                    Serializer s(data.first.data(), data.first.size());
                    std::uint32_t seq = 0;
                    BEAST_EXPECT(s.getInteger<std::uint32_t>(seq, 0));
                    BEAST_EXPECT(seq == 3);
                }
            }
        }
        {
//二进制形式的当前分类帐
            Json::Value jvParams;
            jvParams[jss::binary] = true;
            auto jrr = env.rpc("json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(! jrr[jss::ledger].isMember(jss::ledger_data));
        }
    }

    void testLedgerType()
    {
//把一堆不同的分类帐类型放进分类帐
        using namespace test::jtx;
        using namespace std::chrono;
        Env env{*this,
                envconfig(validator, ""),
                supported_amendments().set(featureTickets)};

        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund(XRP(100000), gw);

        int const num_accounts = 10;

        for (auto i = 0; i < num_accounts; i++)
        {
            Account const bob { std::string("bob") + std::to_string(i) };
            env.fund(XRP(1000), bob);
        }
        env(offer(Account{"bob0"}, USD(100), XRP(100)));
        env.trust(Account{"bob2"}["USD"](100), Account{"bob3"});

        auto majorities = getMajorityAmendments(*env.closed());
        for (int i = 0; i <= 256; ++i)
        {
            env.close();
            majorities = getMajorityAmendments(*env.closed());
            if (!majorities.empty())
                break;
        }
        env(signers(Account{"bob0"}, 1,
                {{Account{"bob1"}, 1}, {Account{"bob2"}, 1}}));
        env(ticket::create(env.master));

        {
            Json::Value jv;
            jv[jss::TransactionType] = "EscrowCreate";
            jv[jss::Flags] = tfUniversal;
            jv[jss::Account] = Account{"bob5"}.human();
            jv[jss::Destination] = Account{"bob6"}.human();
            jv[jss::Amount] = XRP(50).value().getJson(0);
            jv[sfFinishAfter.fieldName] =
                NetClock::time_point{env.now() + 10s}
                    .time_since_epoch().count();
            env(jv);
        }

        {
            Json::Value jv;
            jv[jss::TransactionType] = "PaymentChannelCreate";
            jv[jss::Flags] = tfUniversal;
            jv[jss::Account] = Account{"bob6"}.human ();
            jv[jss::Destination] = Account{"bob7"}.human ();
            jv[jss::Amount] = XRP(100).value().getJson (0);
            jv[jss::SettleDelay] = NetClock::duration{10s}.count();
            jv[sfPublicKey.fieldName] = strHex (Account{"bob6"}.pk().slice ());
            jv[sfCancelAfter.fieldName] =
                NetClock::time_point{env.now() + 300s}
                    .time_since_epoch().count();
            env(jv);
        }

//BOB9授权BOB4和BOB8。
        env (deposit::auth (Account {"bob9"}, Account {"bob4"}));
        env (deposit::auth (Account {"bob9"}, Account {"bob8"}));
        env.close();

//现在获取每种类型
        auto makeRequest = [&env](Json::StaticString t)
        {
            Json::Value jvParams;
            jvParams[jss::ledger_index] = "current";
            jvParams[jss::type] = t;
            return env.rpc ( "json", "ledger_data",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
        };

{  //jvParams[jss:：type]=“账户”；
        auto const jrr = makeRequest(jss::account);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 12) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "AccountRoot" );
        }

{  //jvParams[jss:：type]=“修改”；
        auto const jrr = makeRequest(jss::amendments);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "Amendments" );
        }

{  //jvParams[jss:：type]=“目录”；
        auto const jrr = makeRequest(jss::directory);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 8) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "DirectoryNode" );
        }

{  //jvParams[jss:：type]=“费用”；
        auto const jrr = makeRequest(jss::fee);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "FeeSettings" );
        }

{  //jvParams[jss:：type]=“哈希”；
        auto const jrr = makeRequest(jss::hashes);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 2) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "LedgerHashes" );
        }

{  //jvParams[jss:：type]=“报价”；
        auto const jrr = makeRequest(jss::offer);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "Offer" );
        }

{  //jvParams[jss:：type]=“签名者列表”；
        auto const jrr = makeRequest(jss::signer_list);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "SignerList" );
        }

{  //jvParams[jss:：type]=“状态”；
        auto const jrr = makeRequest(jss::state);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "RippleState" );
        }

{  //jvParams[jss:：type]=“票据”；
        auto const jrr = makeRequest(jss::ticket);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "Ticket" );
        }

{  //jvParams[jss:：type]=“托管”；
        auto const jrr = makeRequest(jss::escrow);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "Escrow" );
        }

{  //jvParams[jss:：type]=“支付渠道”；
        auto const jrr = makeRequest(jss::payment_channel);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 1) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "PayChannel" );
        }

{  //jvParams[jss:：type]=“存款预授权”；
        auto const jrr = makeRequest(jss::deposit_preauth);
        BEAST_EXPECT( checkArraySize(jrr[jss::state], 2) );
        for (auto const& j : jrr[jss::state])
            BEAST_EXPECT( j["LedgerEntryType"] == "DepositPreauth" );
        }

{  //jvParams[jss:：type]=“拼写错误”；
        Json::Value jvParams;
        jvParams[jss::ledger_index] = "current";
        jvParams[jss::type] = "misspelling";
        auto const jrr = env.rpc ( "json", "ledger_data",
            boost::lexical_cast<std::string>(jvParams)) [jss::result];
        BEAST_EXPECT( jrr.isMember("error") );
        BEAST_EXPECT( jrr["error"] == "invalidParams" );
        BEAST_EXPECT( jrr["error_message"] == "Invalid field 'type'." );
        }
    }

    void run() override
    {
        testCurrentLedgerToLimits(true);
        testCurrentLedgerToLimits(false);
        testCurrentLedgerBinary();
        testBadInput();
        testMarkerFollow();
        testLedgerHeader();
        testLedgerType();
    }
};

BEAST_DEFINE_TESTSUITE_PRIO(LedgerData,app,ripple,1);

}
