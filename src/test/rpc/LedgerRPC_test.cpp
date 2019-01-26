
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

#include <ripple/app/misc/TxQ.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class LedgerRPC_test : public beast::unit_test::suite
{
    void
    checkErrorValue(
        Json::Value const& jv,
        std::string const& err,
        std::string const& msg)
    {
        if (BEAST_EXPECT(jv.isMember(jss::status)))
            BEAST_EXPECT(jv[jss::status] == "error");
        if (BEAST_EXPECT(jv.isMember(jss::error)))
            BEAST_EXPECT(jv[jss::error] == err);
        if (msg.empty())
        {
            BEAST_EXPECT(
                jv[jss::error_message] == Json::nullValue ||
                jv[jss::error_message] == "");
        }
        else if (BEAST_EXPECT(jv.isMember(jss::error_message)))
            BEAST_EXPECT(jv[jss::error_message] == msg);
    }

//用“！”替换第10个字符，从而损坏有效地址.
//“！”不是Ripple字母表的一部分。
    std::string
    makeBadAddress (std::string good)
    {
        std::string ret = std::move (good);
        ret.replace (10, 1, 1, '!');
        return ret;
    }

    void testLedgerRequest()
    {
        testcase("Basic Request");
        using namespace test::jtx;

        Env env {*this};

        env.close();
        BEAST_EXPECT(env.current()->info().seq == 4);

        {
//在这种情况下，数字字符串转换为数字
            auto const jrr = env.rpc("ledger", "1") [jss::result];
            BEAST_EXPECT(jrr[jss::ledger][jss::closed] == true);
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "1");
            BEAST_EXPECT(jrr[jss::ledger][jss::accepted] == true);
            BEAST_EXPECT(jrr[jss::ledger][jss::totalCoins] == env.balance(env.master).value().getText());
        }

        {
//使用当前标识符
            auto const jrr = env.rpc("ledger", "current") [jss::result];
            BEAST_EXPECT(jrr[jss::ledger][jss::closed] == false);
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == to_string(env.current()->info().seq));
            BEAST_EXPECT(jrr[jss::ledger_current_index] == env.current()->info().seq);
        }
    }

    void testBadInput()
    {
        testcase("Bad Input");
        using namespace test::jtx;
        Env env { *this };
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        Account const bob { "bob" };

        env.fund(XRP(10000), gw, bob);
        env.close();
        env.trust(USD(1000), bob);
        env.close();

        {
            Json::Value jvParams;
jvParams[jss::ledger_index] = "0"; //不是整数
            auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
            checkErrorValue(jrr, "invalidParams", "ledgerIndexMalformed");
        }

        {
//请求坏的分类帐索引
            Json::Value jvParams;
            jvParams[jss::ledger_index] = 10u;
            auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
            checkErrorValue(jrr, "lgrNotFound", "ledgerNotFound");
        }

        {
//无法识别的字符串参数--错误
            auto const jrr = env.rpc("ledger", "arbitrary_text") [jss::result];
            checkErrorValue(jrr, "lgrNotFound", "ledgerNotFound");
        }

        {
//已关闭分类帐的请求队列
            Json::Value jvParams;
            jvParams[jss::ledger_index] = "validated";
            jvParams[jss::queue] = true;
            auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
            checkErrorValue(jrr, "invalidParams", "Invalid parameters.");
        }

        {
//请求一个具有非常大（双）序列的分类帐。
            auto const ret = env.rpc (
                "json", "ledger", "{ \"ledger_index\" : 2e15 }");
            BEAST_EXPECT (RPC::contains_error(ret));
            BEAST_EXPECT (ret[jss::error_message] == "Invalid parameters.");
        }

        {
//请求具有非常大（整数）序列的分类帐。
            auto const ret = env.rpc (
                "json", "ledger", "{ \"ledger_index\" : 1000000000000000 }");
            checkErrorValue(ret, "invalidParams", "Invalid parameters.");
        }
    }

    void testLedgerCurrent()
    {
        testcase("ledger_current Request");
        using namespace test::jtx;

        Env env {*this};

        env.close();
        BEAST_EXPECT(env.current()->info().seq == 4);

        {
            auto const jrr = env.rpc("ledger_current") [jss::result];
            BEAST_EXPECT(jrr[jss::ledger_current_index] == env.current()->info().seq);
        }
    }

    void testMissingLedgerEntryLedgerHash()
    {
        testcase("Missing ledger_entry ledger_hash");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        env.fund(XRP(10000), alice);
        env.close();

        Json::Value jvParams;
        jvParams[jss::account_root] = alice.human();
        jvParams[jss::ledger_hash] =
            "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        auto const jrr =
            env.rpc ("json", "ledger_entry", to_string(jvParams))[jss::result];
        checkErrorValue (jrr, "lgrNotFound", "ledgerNotFound");
    }

    void testLedgerFull()
    {
        testcase("Ledger Request, Full Option");
        using namespace test::jtx;

        Env env {*this};

        env.close();

        Json::Value jvParams;
        jvParams[jss::ledger_index] = 3u;
        jvParams[jss::full] = true;
        auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
        BEAST_EXPECT(jrr[jss::ledger].isMember(jss::accountState));
        BEAST_EXPECT(jrr[jss::ledger][jss::accountState].isArray());
        BEAST_EXPECT(jrr[jss::ledger][jss::accountState].size() == 2u);
    }

    void testLedgerFullNonAdmin()
    {
        testcase("Ledger Request, Full Option Without Admin");
        using namespace test::jtx;

        Env env { *this, envconfig(no_admin) };

        env.close();

        Json::Value jvParams;
        jvParams[jss::ledger_index] = 3u;
        jvParams[jss::full] = true;
        auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
        checkErrorValue(jrr, "noPermission", "You don't have permission for this command."); }

    void testLedgerAccounts()
    {
        testcase("Ledger Request, Accounts Option");
        using namespace test::jtx;

        Env env {*this};

        env.close();

        Json::Value jvParams;
        jvParams[jss::ledger_index] = 3u;
        jvParams[jss::accounts] = true;
        auto const jrr = env.rpc ( "json", "ledger", to_string(jvParams) ) [jss::result];
        BEAST_EXPECT(jrr[jss::ledger].isMember(jss::accountState));
        BEAST_EXPECT(jrr[jss::ledger][jss::accountState].isArray());
        BEAST_EXPECT(jrr[jss::ledger][jss::accountState].size() == 2u);
    }

    void testLedgerEntryAccountRoot()
    {
        testcase ("ledger_entry Request AccountRoot");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        env.fund (XRP(10000), alice);
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        {
//练习分类账一路关闭。
            Json::Value const jrr = env.rpc ("ledger_closed")[jss::result];
            BEAST_EXPECT(jrr[jss::ledger_hash] == ledgerHash);
            BEAST_EXPECT(jrr[jss::ledger_index] == 3);
        }

        std::string accountRootIndex;
        {
//请求Alice的帐户根。
            Json::Value jvParams;
            jvParams[jss::account_root] = alice.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::node));
            BEAST_EXPECT(jrr[jss::node][jss::Account] == alice.human());
            BEAST_EXPECT(jrr[jss::node][sfBalance.jsonName] == "10000000000");
            accountRootIndex = jrr[jss::index].asString();
        }
        {
            constexpr char alicesAcctRootBinary[] {
                "1100612200800000240000000225000000032D00000000554294BEBE5B569"
                "A18C0A2702387C9B1E7146DC3A5850C1E87204951C6FDAA4C426240000002"
                "540BE4008114AE123A8556F3CF91154711376AFB0F894F832B3D"
            };

//请求Alice的帐户根目录，但使用binary==true；
            Json::Value jvParams;
            jvParams[jss::account_root] = alice.human();
            jvParams[jss::binary] = 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::node_binary));
            BEAST_EXPECT(jrr[jss::node_binary] == alicesAcctRootBinary);
        }
        {
//使用索引请求Alice的帐户根。
            Json::Value jvParams;
            jvParams[jss::index] = accountRootIndex;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(! jrr.isMember(jss::node_binary));
            BEAST_EXPECT(jrr.isMember(jss::node));
            BEAST_EXPECT(jrr[jss::node][jss::Account] == alice.human());
            BEAST_EXPECT(jrr[jss::node][sfBalance.jsonName] == "10000000000");
        }
        {
//按索引请求Alice的帐户根目录，但使用binary==false。
            Json::Value jvParams;
            jvParams[jss::index] = accountRootIndex;
            jvParams[jss::binary] = 0;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::node));
            BEAST_EXPECT(jrr[jss::node][jss::Account] == alice.human());
            BEAST_EXPECT(jrr[jss::node][sfBalance.jsonName] == "10000000000");
        }
        {
//使用损坏的accountID请求。
            Json::Value jvParams;
            jvParams[jss::account_root] = makeBadAddress (alice.human());
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAddress", "");
        }
        {
//申请不在分类帐中的帐户。
            Json::Value jvParams;
            jvParams[jss::account_root] = Account("bob").human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "entryNotFound", "");
        }
    }

    void testLedgerEntryCheck()
    {
        testcase ("ledger_entry Request Check");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        env.fund (XRP(10000), alice);
        env.close();

        uint256 const checkId {
            getCheckIndex (env.master, env.seq (env.master))};

//lambda创建检查。
        auto checkCreate = [] (test::jtx::Account const& account,
            test::jtx::Account const& dest, STAmount const& sendMax)
        {
            Json::Value jv;
            jv[sfAccount.jsonName] = account.human();
            jv[sfSendMax.jsonName] = sendMax.getJson(0);
            jv[sfDestination.jsonName] = dest.human();
            jv[sfTransactionType.jsonName] = "CheckCreate";
            jv[sfFlags.jsonName] = tfUniversal;
            return jv;
        };

        env (checkCreate (env.master, alice, XRP(100)));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        {
//申请支票。
            Json::Value jvParams;
            jvParams[jss::check] = to_string (checkId);
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::node][sfLedgerEntryType.jsonName] == "Check");
            BEAST_EXPECT(jrr[jss::node][sfSendMax.jsonName] == "100000000");
        }
        {
//请求一个非支票索引。我们用爱丽丝的
//帐户根索引。
            std::string accountRootIndex;
            {
                Json::Value jvParams;
                jvParams[jss::account_root] = alice.human();
                Json::Value const jrr = env.rpc (
                    "json", "ledger_entry", to_string (jvParams))[jss::result];
                accountRootIndex = jrr[jss::index].asString();
            }
            Json::Value jvParams;
            jvParams[jss::check] = accountRootIndex;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
    }

    void testLedgerEntryDepositPreauth()
    {
        testcase ("ledger_entry Request Directory");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        Account const becky {"becky"};

        env.fund (XRP(10000), alice, becky);
        env.close();

        env (deposit::auth (alice, becky));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        std::string depositPreauthIndex;
        {
//由业主和授权人请求存款授权。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] = alice.human();
            jvParams[jss::deposit_preauth][jss::authorized] = becky.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];

            BEAST_EXPECT(
                jrr[jss::node][sfLedgerEntryType.jsonName] == "DepositPreauth");
            BEAST_EXPECT(jrr[jss::node][sfAccount.jsonName] == alice.human());
            BEAST_EXPECT(jrr[jss::node][sfAuthorize.jsonName] == becky.human());
            depositPreauthIndex = jrr[jss::node][jss::index].asString();
        }
        {
//按索引请求DepositPrauth。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth] = depositPreauthIndex;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];

            BEAST_EXPECT(
                jrr[jss::node][sfLedgerEntryType.jsonName] == "DepositPreauth");
            BEAST_EXPECT(jrr[jss::node][sfAccount.jsonName] == alice.human());
            BEAST_EXPECT(jrr[jss::node][sfAuthorize.jsonName] == becky.human());
        }
        {
//格式错误的请求：deposit_preauth既不是对象也不是字符串。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth] = -5;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式错误的请求：deposit_preauth不是十六进制字符串。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth] = "0123456789ABCDEFG";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式错误的请求：缺少[JSS:：Deposit_Preauth][JSS:：Owner]
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::authorized] = becky.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式错误的请求：[JSS:：Deposit_Preauth][JSS:：Owner]不是字符串。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] = 7;
            jvParams[jss::deposit_preauth][jss::authorized] = becky.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式错误：缺少[JSS:：Deposit_Preauth][JSS:：authorized]
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] = alice.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式错误：【jss:：deposit_preauth】【jss:：authorized】不是字符串。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] = alice.human();
            jvParams[jss::deposit_preauth][jss::authorized] = 47;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//格式不正确：[JSS:：Deposit_Preauth][JSS:：Owner]格式不正确。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] =
                "rP6P9ypfAmc!pw8SZHNwM4nvZHFXDraQas";

            jvParams[jss::deposit_preauth][jss::authorized] = becky.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedOwner", "");
        }
        {
//格式错误：【jss:：deposit_preauth】【jss:：authorized】格式错误。
            Json::Value jvParams;
            jvParams[jss::deposit_preauth][jss::owner] = alice.human();
            jvParams[jss::deposit_preauth][jss::authorized] =
                "rP6P9ypfAmc!pw8SZHNwM4nvZHFXDraQas";

            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAuthorized", "");
        }
    }

    void testLedgerEntryDirectory()
    {
        testcase ("ledger_entry Request Directory");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund (XRP(10000), alice, gw);
        env.close();

        env.trust(USD(1000), alice);
        env.close();

//增加目录项的数量，使Alice有两个
//目录节点。
        for (int d = 1'000'032; d >= 1'000'000; --d)
        {
            env (offer (alice, USD (1), drops (d)));
        }
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        {
//练习分类账一路关闭。
            Json::Value const jrr = env.rpc ("ledger_closed")[jss::result];
            BEAST_EXPECT(jrr[jss::ledger_hash] == ledgerHash);
            BEAST_EXPECT(jrr[jss::ledger_index] == 5);
        }

        std::string const dirRootIndex =
            "A33EC6BB85FB5674074C4A3A43373BB17645308F3EAE1933E3E35252162B217D";
        {
//按索引定位目录。
            Json::Value jvParams;
            jvParams[jss::directory] = dirRootIndex;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::node][sfIndexes.jsonName].size() == 32);
        }
        {
//按目录根目录定位目录。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::dir_root] = dirRootIndex;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::index] == dirRootIndex);
        }
        {
//按所有者查找目录。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::owner] = alice.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::index] == dirRootIndex);
        }
        {
//按目录根目录和子目录索引定位目录。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::dir_root] = dirRootIndex;
            jvParams[jss::directory][jss::sub_index] = 1;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::index] != dirRootIndex);
            BEAST_EXPECT(jrr[jss::node][sfIndexes.jsonName].size() == 2);
        }
        {
//按所有者和子索引定位目录。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::owner] = alice.human();
            jvParams[jss::directory][jss::sub_index] = 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::index] != dirRootIndex);
            BEAST_EXPECT(jrr[jss::node][sfIndexes.jsonName].size() == 2);
        }
        {
//空目录参数。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::nullValue;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//非整数子索引。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::dir_root] = dirRootIndex;
            jvParams[jss::directory][jss::sub_index] = 1.5;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//所有者条目格式错误。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;

            std::string const badAddress = makeBadAddress (alice.human());
            jvParams[jss::directory][jss::owner] = badAddress;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAddress", "");
        }
        {
//目录对象格式不正确。同时指定dir_root和owner。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::owner] = alice.human();
            jvParams[jss::directory][jss::dir_root] = dirRootIndex;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//目录对象不完整。同时缺少dir_root和owner。
            Json::Value jvParams;
            jvParams[jss::directory] = Json::objectValue;
            jvParams[jss::directory][jss::sub_index] = 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
    }

    void testLedgerEntryEscrow()
    {
        testcase ("ledger_entry Request Escrow");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        env.fund (XRP(10000), alice);
        env.close();

//lambda创建托管。
        auto escrowCreate = [] (
            test::jtx::Account const& account, test::jtx::Account const& to,
            STAmount const& amount, NetClock::time_point const& cancelAfter)
        {
            Json::Value jv;
            jv[jss::TransactionType] = "EscrowCreate";
            jv[jss::Flags] = tfUniversal;
            jv[jss::Account] = account.human();
            jv[jss::Destination] = to.human();
            jv[jss::Amount] = amount.getJson(0);
            jv[sfFinishAfter.jsonName] =
                cancelAfter.time_since_epoch().count() + 2;
            return jv;
        };

        using namespace std::chrono_literals;
        env (escrowCreate (alice, alice, XRP(333), env.now() + 2s));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        std::string escrowIndex;
        {
//使用所有者和顺序请求托管。
            Json::Value jvParams;
            jvParams[jss::escrow] = Json::objectValue;
            jvParams[jss::escrow][jss::owner] = alice.human();
            jvParams[jss::escrow][jss::seq] = env.seq (alice) - 1;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(
                jrr[jss::node][jss::Amount] == XRP(333).value().getText());
            escrowIndex = jrr[jss::index].asString();
        }
        {
//按索引请求托管。
            Json::Value jvParams;
            jvParams[jss::escrow] = escrowIndex;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(
                jrr[jss::node][jss::Amount] == XRP(333).value().getText());

        }
        {
//所有者条目格式错误。
            Json::Value jvParams;
            jvParams[jss::escrow] = Json::objectValue;

            std::string const badAddress = makeBadAddress (alice.human());
            jvParams[jss::escrow][jss::owner] = badAddress;
            jvParams[jss::escrow][jss::seq] = env.seq (alice) - 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedOwner", "");
        }
        {
//失踪的所有者。
            Json::Value jvParams;
            jvParams[jss::escrow] = Json::objectValue;
            jvParams[jss::escrow][jss::seq] = env.seq (alice) - 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//缺少序列。
            Json::Value jvParams;
            jvParams[jss::escrow] = Json::objectValue;
            jvParams[jss::escrow][jss::owner] = alice.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//非整数序列。
            Json::Value jvParams;
            jvParams[jss::escrow] = Json::objectValue;
            jvParams[jss::escrow][jss::owner] = alice.human();
            jvParams[jss::escrow][jss::seq] =
                std::to_string (env.seq (alice) - 1);
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
    }

    void testLedgerEntryGenerator()
    {
        testcase ("ledger_entry Request Generator");
        using namespace test::jtx;
        Env env {*this};

//所有生成器请求都已弃用。
        Json::Value jvParams;
        jvParams[jss::generator] = 5;
        jvParams[jss::ledger_hash] = to_string (env.closed()->info().hash);
        Json::Value const jrr = env.rpc (
            "json", "ledger_entry", to_string (jvParams))[jss::result];
        checkErrorValue (jrr, "deprecatedFeature", "");
    }

    void testLedgerEntryOffer()
    {
        testcase ("ledger_entry Request Offer");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund (XRP(10000), alice, gw);
        env.close();

        env (offer (alice, USD (321), XRP (322)));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        std::string offerIndex;
        {
//使用所有者和顺序请求报价。
            Json::Value jvParams;
            jvParams[jss::offer] = Json::objectValue;
            jvParams[jss::offer][jss::account] = alice.human();
            jvParams[jss::offer][jss::seq] = env.seq (alice) - 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::node][jss::TakerGets] == "322000000");
            offerIndex = jrr[jss::index].asString();
        }
        {
//使用其索引请求报价。
            Json::Value jvParams;
            jvParams[jss::offer] = offerIndex;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::node][jss::TakerGets] == "322000000");
        }
        {
//帐户条目格式不正确。
            Json::Value jvParams;
            jvParams[jss::offer] = Json::objectValue;

            std::string const badAddress = makeBadAddress (alice.human());
            jvParams[jss::offer][jss::account] = badAddress;
            jvParams[jss::offer][jss::seq] = env.seq (alice) - 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAddress", "");
        }
        {
//提供对象格式不正确。缺少帐户成员。
            Json::Value jvParams;
            jvParams[jss::offer] = Json::objectValue;
            jvParams[jss::offer][jss::seq] = env.seq (alice) - 1;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//提供对象格式不正确。缺少seq成员。
            Json::Value jvParams;
            jvParams[jss::offer] = Json::objectValue;
            jvParams[jss::offer][jss::account] = alice.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//提供对象格式不正确。非整数序列成员。
            Json::Value jvParams;
            jvParams[jss::offer] = Json::objectValue;
            jvParams[jss::offer][jss::account] = alice.human();
            jvParams[jss::offer][jss::seq] =
                std::to_string (env.seq (alice) - 1);
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
    }

    void testLedgerEntryPayChan()
    {
        testcase ("ledger_entry Request Pay Chan");
        using namespace test::jtx;
        using namespace std::literals::chrono_literals;
        Env env {*this};
        Account const alice {"alice"};

        env.fund (XRP(10000), alice);
        env.close();

//lambda创建paychan。
        auto payChanCreate = [] (
            test::jtx::Account const& account,
            test::jtx::Account const& to,
            STAmount const& amount,
            NetClock::duration const& settleDelay,
            PublicKey const& pk)
        {
            Json::Value jv;
            jv[jss::TransactionType] = "PaymentChannelCreate";
            jv[jss::Account] = account.human();
            jv[jss::Destination] = to.human();
            jv[jss::Amount] = amount.getJson (0);
            jv[sfSettleDelay.jsonName] = settleDelay.count();
            jv[sfPublicKey.jsonName] = strHex (pk.slice());
            return jv;
        };

        env (payChanCreate (alice, env.master, XRP(57), 18s, alice.pk()));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};

        uint256 const payChanIndex {
            keylet::payChan (alice, env.master, env.seq (alice) - 1).key};
        {
//使用其索引请求付款渠道。
            Json::Value jvParams;
            jvParams[jss::payment_channel] = to_string (payChanIndex);
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::node][sfAmount.jsonName] == "57000000");
            BEAST_EXPECT(jrr[jss::node][sfBalance.jsonName] == "0");
            BEAST_EXPECT(jrr[jss::node][sfSettleDelay.jsonName] == 18);
        }
        {
//请求一个不是付款渠道的索引。
            Json::Value jvParams;
            jvParams[jss::payment_channel] = ledgerHash;
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "entryNotFound", "");
        }
    }

    void testLedgerEntryRippleState()
    {
        testcase ("ledger_entry Request RippleState");
        using namespace test::jtx;
        Env env {*this};
        Account const alice {"alice"};
        Account const gw { "gateway" };
        auto const USD = gw["USD"];
        env.fund(XRP(10000), alice, gw);
        env.close();

        env.trust(USD(999), alice);
        env.close();

        env(pay (gw, alice, USD(97)));
        env.close();

        std::string const ledgerHash {to_string (env.closed()->info().hash)};
        {
//使用帐户和货币请求信任行。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            BEAST_EXPECT(
                jrr[jss::node][sfBalance.jsonName][jss::value] == "-97");
            BEAST_EXPECT(
                jrr[jss::node][sfHighLimit.jsonName][jss::value] == "999");
        }
        {
//纹波状态不是一个对象。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = "ripple_state";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State.currency丢失。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State帐户不是数组。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = 2;
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State其中一个账户不见了。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_状态超过2个帐户。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ripple_state][jss::accounts][2u] = alice.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State帐户[0]不是字符串。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = 44;
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State帐户[1]不是字符串。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = 21;
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State帐户[0]==帐户[1]。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = alice.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedRequest", "");
        }
        {
//Ripple_State格式错误的帐户[0]。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] =
                makeBadAddress (alice.human());
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAddress", "");
        }
        {
//Ripple_State格式错误的帐户[1]。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] =
                makeBadAddress (gw.human());
            jvParams[jss::ripple_state][jss::currency] = "USD";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedAddress", "");
        }
        {
//Ripple_State畸形货币。
            Json::Value jvParams;
            jvParams[jss::ripple_state] = Json::objectValue;
            jvParams[jss::ripple_state][jss::accounts] = Json::arrayValue;
            jvParams[jss::ripple_state][jss::accounts][0u] = alice.human();
            jvParams[jss::ripple_state][jss::accounts][1u] = gw.human();
            jvParams[jss::ripple_state][jss::currency] = "USDollars";
            jvParams[jss::ledger_hash] = ledgerHash;
            Json::Value const jrr = env.rpc (
                "json", "ledger_entry", to_string (jvParams))[jss::result];
            checkErrorValue (jrr, "malformedCurrency", "");
        }
    }

    void testLedgerEntryUnknownOption()
    {
        testcase ("ledger_entry Request Unknown Option");
        using namespace test::jtx;
        Env env {*this};

        std::string const ledgerHash {to_string (env.closed()->info().hash)};

//“功能”不是分类帐条目支持的选项。
        Json::Value jvParams;
        jvParams[jss::features] = ledgerHash;
        jvParams[jss::ledger_hash] = ledgerHash;
        Json::Value const jrr = env.rpc (
            "json", "ledger_entry", to_string (jvParams))[jss::result];
        checkErrorValue (jrr, "unknownOption", "");
    }

///@brief ledger rpc请求作为一种驱动方式
///input选项到lookupledger。这次测试的重点是
///lookupledger的覆盖范围，而不是分类帐
///RPC请求。
    void testLookupLedger()
    {
        testcase ("Lookup ledger");
        using namespace test::jtx;
Env env {*this, FeatureBitset{}}; //下面请求的哈希假设
//无修正案
        env.fund(XRP(10000), "alice");
        env.close();
        env.fund(XRP(10000), "bob");
        env.close();
        env.fund(XRP(10000), "jim");
        env.close();
        env.fund(XRP(10000), "jill");

//已关闭的分类帐哈希为：
//1-AB868A6CFEEC779C2FF845CF00A6422259986AF40C01976A7F842B6918936C7
//2-8aedb96643962f1d40f01e25632abb3c56c9f04b0231ee4b18248b90173d189
//3-7c3eedb3124d92e49e75d81a8826a2e65a75fd71fc3f6feb803c5f1d812d
//4-9f9e6a4ecaa84a08ff94713fa41c3151177d622ea47dd2f0020ca49913ee2e6
//5-C516522DE274EB52CE69A3D22F66DD73A53E16597E06F7A86F66DF7DD4309173
//
        {
//通过旧分类账字段、关键字索引值访问
            Json::Value jvParams;
            jvParams[jss::ledger] = "closed";
            auto jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "5");

            jvParams[jss::ledger] = "validated";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "5");

            jvParams[jss::ledger] = "current";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "6");

//询问错误的分类帐关键字
            jvParams[jss::ledger] = "invalid";
            jrr = env.rpc ( "json", "ledger",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerIndexMalformed");

//数值索引
            jvParams[jss::ledger] = 4;
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "4");

//数字索引-超出范围
            jvParams[jss::ledger] = 20;
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }

        {
//通过分类账哈希字段访问
            Json::Value jvParams;
            jvParams[jss::ledger_hash] =
                "7C3EEDB3124D92E49E75D81A8826A2E6"
                "5A75FD71FC3FD6F36FEB803C5F1D812D";
            auto jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "3");

//哈希中多余的前导十六进制字符将被忽略
            jvParams[jss::ledger_hash] =
                "DEADBEEF"
                "7C3EEDB3124D92E49E75D81A8826A2E6"
                "5A75FD71FC3FD6F36FEB803C5F1D812D";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "3");

//带非字符串分类帐哈希的请求
            jvParams[jss::ledger_hash] = 2;
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerHashNotString");

//格式错误（非十六进制）哈希
            jvParams[jss::ledger_hash] =
                "ZZZZZZZZZZZD92E49E75D81A8826A2E6"
                "5A75FD71FC3FD6F36FEB803C5F1D812D";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerHashMalformed");

//格式正确，但不存在
            jvParams[jss::ledger_hash] =
                "8C3EEDB3124D92E49E75D81A8826A2E6"
                "5A75FD71FC3FD6F36FEB803C5F1D812D";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }

        {
//通过分类帐索引字段、关键字索引值访问
            Json::Value jvParams;
            jvParams[jss::ledger_index] = "closed";
            auto jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "5");
            BEAST_EXPECT(jrr.isMember(jss::ledger_index));

            jvParams[jss::ledger_index] = "validated";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "5");

            jvParams[jss::ledger_index] = "current";
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr.isMember(jss::ledger));
            BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == "6");
            BEAST_EXPECT(jrr.isMember(jss::ledger_current_index));

//询问错误的分类帐关键字
            jvParams[jss::ledger_index] = "invalid";
            jrr = env.rpc ( "json", "ledger",
                boost::lexical_cast<std::string>(jvParams)) [jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "invalidParams");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerIndexMalformed");

//数值索引
            for (auto i : {1, 2, 3, 4 ,5, 6})
            {
                jvParams[jss::ledger_index] = i;
                jrr = env.rpc("json", "ledger",
                    boost::lexical_cast<std::string>(jvParams))[jss::result];
                BEAST_EXPECT(jrr.isMember(jss::ledger));
                if(i < 6)
                    BEAST_EXPECT(jrr.isMember(jss::ledger_hash));
                BEAST_EXPECT(jrr[jss::ledger][jss::ledger_index] == std::to_string(i));
            }

//数字索引-超出范围
            jvParams[jss::ledger_index] = 7;
            jrr = env.rpc("json", "ledger",
                boost::lexical_cast<std::string>(jvParams))[jss::result];
            BEAST_EXPECT(jrr[jss::error]         == "lgrNotFound");
            BEAST_EXPECT(jrr[jss::error_message] == "ledgerNotFound");
        }
    }

    void testNoQueue()
    {
        testcase("Ledger with queueing disabled");
        using namespace test::jtx;
        Env env{ *this };

        Json::Value jv;
        jv[jss::ledger_index] = "current";
        jv[jss::queue] = true;
        jv[jss::expand] = true;

        auto jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        BEAST_EXPECT(! jrr.isMember(jss::queue_data));
    }

    void testQueue()
    {
        testcase("Ledger with Queued Transactions");
        using namespace test::jtx;
        Env env { *this,
            envconfig([](std::unique_ptr<Config> cfg) {
                auto& section = cfg->section("transaction_queue");
                section.set("minimum_txn_in_ledger_standalone", "3");
                section.set("normal_consensus_increase_percent", "0");
                return cfg;
            })};

        Json::Value jv;
        jv[jss::ledger_index] = "current";
        jv[jss::queue] = true;
        jv[jss::expand] = true;

        Account const alice{ "alice" };
        Account const bob{ "bob" };
        Account const charlie{ "charlie" };
        Account const daria{ "daria" };
        env.fund(XRP(10000), alice);
        env.fund(XRP(10000), bob);
        env.close();
        env.fund(XRP(10000), charlie);
        env.fund(XRP(10000), daria);
        env.close();

        auto jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        BEAST_EXPECT(! jrr.isMember(jss::queue_data));

//填写未结分类帐
        for (;;)
        {
            auto metrics = env.app().getTxQ().getMetrics(*env.current());
            if (metrics.openLedgerFeeLevel > metrics.minProcessingFeeLevel)
                break;
            env(noop(alice));
        }

        BEAST_EXPECT(env.current()->info().seq == 5);
//在队列中加入一些TXS
//爱丽丝
        auto aliceSeq = env.seq(alice);
        env(pay(alice, "george", XRP(1000)), json(R"({"LastLedgerSequence":7})"),
            ter(terQUEUED));
        env(offer(alice, XRP(50000), alice["USD"](5000)), seq(aliceSeq + 1),
            ter(terQUEUED));
        env(noop(alice), seq(aliceSeq + 2), ter(terQUEUED));
//鲍勃
        auto batch = [&env](Account a)
        {
            auto aSeq = env.seq(a);
//足够的费用排在爱丽丝前面
            for (int i = 0; i < 10; ++i)
            {
                env(noop(a), fee(1000 + i), seq(aSeq + i), ter(terQUEUED));
            }
        };
        batch(bob);
//查理
        batch(charlie);
//达里亚
        batch(daria);

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        BEAST_EXPECT(jrr[jss::queue_data].size() == 33);

//关闭足够多的分类帐，以便爱丽丝的第一个Tx到期。
        env.close();
        env.close();
        env.close();
        BEAST_EXPECT(env.current()->info().seq == 8);

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        BEAST_EXPECT(jrr[jss::queue_data].size() == 11);

        env.close();

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        std::string txid1;
        std::string txid2;
        if (BEAST_EXPECT(jrr[jss::queue_data].size() == 2))
        {
            auto const& txj = jrr[jss::queue_data][0u];
            BEAST_EXPECT(txj[jss::account] == alice.human());
            BEAST_EXPECT(txj[jss::fee_level] == "256");
            BEAST_EXPECT(txj["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj["retries_remaining"] == 10);
            BEAST_EXPECT(txj.isMember(jss::tx));
            auto const& tx = txj[jss::tx];
            BEAST_EXPECT(tx[jss::Account] == alice.human());
            BEAST_EXPECT(tx[jss::TransactionType] == "OfferCreate");
            txid1 = tx[jss::hash].asString();
        }

        env.close();

        jv[jss::expand] = false;

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        if (BEAST_EXPECT(jrr[jss::queue_data].size() == 2))
        {
            auto const& txj = jrr[jss::queue_data][0u];
            BEAST_EXPECT(txj[jss::account] == alice.human());
            BEAST_EXPECT(txj[jss::fee_level] == "256");
            BEAST_EXPECT(txj["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj["retries_remaining"] == 9);
            BEAST_EXPECT(txj["last_result"] == "terPRE_SEQ");
            BEAST_EXPECT(txj.isMember(jss::tx));
            BEAST_EXPECT(txj[jss::tx] == txid1);
        }

        env.close();

        jv[jss::expand] = true;
        jv[jss::binary] = true;

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        if (BEAST_EXPECT(jrr[jss::queue_data].size() == 2))
        {
            auto const& txj = jrr[jss::queue_data][0u];
            BEAST_EXPECT(txj[jss::account] == alice.human());
            BEAST_EXPECT(txj[jss::fee_level] == "256");
            BEAST_EXPECT(txj["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj["retries_remaining"] == 8);
            BEAST_EXPECT(txj["last_result"] == "terPRE_SEQ");
            BEAST_EXPECT(txj.isMember(jss::tx));
            BEAST_EXPECT(txj[jss::tx].isMember(jss::tx_blob));

            auto const& txj2 = jrr[jss::queue_data][1u];
            BEAST_EXPECT(txj2[jss::account] == alice.human());
            BEAST_EXPECT(txj2[jss::fee_level] == "256");
            BEAST_EXPECT(txj2["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj2["retries_remaining"] == 10);
            BEAST_EXPECT(! txj2.isMember("last_result"));
            BEAST_EXPECT(txj2.isMember(jss::tx));
            BEAST_EXPECT(txj2[jss::tx].isMember(jss::tx_blob));
        }

        for (int i = 0; i != 9; ++i)
        {
            env.close();
        }

        jv[jss::expand] = false;
        jv[jss::binary] = false;

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        if (BEAST_EXPECT(jrr[jss::queue_data].size() == 1))
        {
            auto const& txj = jrr[jss::queue_data][0u];
            BEAST_EXPECT(txj[jss::account] == alice.human());
            BEAST_EXPECT(txj[jss::fee_level] == "256");
            BEAST_EXPECT(txj["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj["retries_remaining"] == 1);
            BEAST_EXPECT(txj["last_result"] == "terPRE_SEQ");
            BEAST_EXPECT(txj.isMember(jss::tx));
            BEAST_EXPECT(txj[jss::tx] != txid1);
            txid2 = txj[jss::tx].asString();
        }

        jv[jss::full] = true;

        jrr = env.rpc("json", "ledger", to_string(jv))[jss::result];
        if (BEAST_EXPECT(jrr[jss::queue_data].size() == 1))
        {
            auto const& txj = jrr[jss::queue_data][0u];
            BEAST_EXPECT(txj[jss::account] == alice.human());
            BEAST_EXPECT(txj[jss::fee_level] == "256");
            BEAST_EXPECT(txj["preflight_result"] == "tesSUCCESS");
            BEAST_EXPECT(txj["retries_remaining"] == 1);
            BEAST_EXPECT(txj["last_result"] == "terPRE_SEQ");
            BEAST_EXPECT(txj.isMember(jss::tx));
            auto const& tx = txj[jss::tx];
            BEAST_EXPECT(tx[jss::Account] == alice.human());
            BEAST_EXPECT(tx[jss::TransactionType] == "AccountSet");
            BEAST_EXPECT(tx[jss::hash] == txid2);
        }
    }

    void testLedgerAccountsOption()
    {
        testcase("Ledger Request, Accounts Option");
        using namespace test::jtx;

        Env env {*this};

        env.close();

        std::string index;
        {
            Json::Value jvParams;
            jvParams[jss::ledger_index] = 3u;
            jvParams[jss::accounts] = true;
            jvParams[jss::expand] = true;
            jvParams[jss::type] = "hashes";
            auto const jrr = env.rpc (
                "json", "ledger", to_string(jvParams) )[jss::result];
            BEAST_EXPECT(jrr[jss::ledger].isMember(jss::accountState));
            BEAST_EXPECT(jrr[jss::ledger][jss::accountState].isArray());
            BEAST_EXPECT(jrr[jss::ledger][jss::accountState].size() == 1u);
            BEAST_EXPECT(
                jrr[jss::ledger][jss::accountState][0u]["LedgerEntryType"]
                == "LedgerHashes");
            index = jrr[jss::ledger][jss::accountState][0u]["index"].asString();
        }
        {
            Json::Value jvParams;
            jvParams[jss::ledger_index] = 3u;
            jvParams[jss::accounts] = true;
            jvParams[jss::expand] = false;
            jvParams[jss::type] = "hashes";
            auto const jrr = env.rpc (
                "json", "ledger", to_string(jvParams) )[jss::result];
            BEAST_EXPECT(jrr[jss::ledger].isMember(jss::accountState));
            BEAST_EXPECT(jrr[jss::ledger][jss::accountState].isArray());
            BEAST_EXPECT(jrr[jss::ledger][jss::accountState].size() == 1u);
            BEAST_EXPECT(jrr[jss::ledger][jss::accountState][0u] == index);
        }
    }

public:
    void run () override
    {
        testLedgerRequest();
        testBadInput();
        testLedgerCurrent();
        testMissingLedgerEntryLedgerHash();
        testLedgerFull();
        testLedgerFullNonAdmin();
        testLedgerAccounts();
        testLedgerEntryAccountRoot();
        testLedgerEntryCheck();
        testLedgerEntryDepositPreauth();
        testLedgerEntryDirectory();
        testLedgerEntryEscrow();
        testLedgerEntryGenerator();
        testLedgerEntryOffer();
        testLedgerEntryPayChan();
        testLedgerEntryRippleState();
        testLedgerEntryUnknownOption();
        testLookupLedger();
        testNoQueue();
        testQueue();
        testLedgerAccountsOption();
    }
};

BEAST_DEFINE_TESTSUITE(LedgerRPC,app,ripple);

} //涟漪

