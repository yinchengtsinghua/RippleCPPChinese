
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

#include <ripple/app/misc/TxQ.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>
#include <test/jtx/envconfig.h>
#include <boost/algorithm/string/predicate.hpp>
#include <ripple/beast/utility/temp_dir.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/resource/impl/Entry.h>
#include <ripple/resource/impl/Tuning.h>
#include <ripple/rpc/impl/Tuning.h>

namespace ripple {

class NoRippleCheck_test : public beast::unit_test::suite
{
    void
    testBadInput ()
    {
        testcase ("Bad input to noripple_check");

        using namespace test::jtx;
        Env env {*this};

        auto const alice = Account {"alice"};
        env.fund (XRP(10000), alice);
        env.close ();

{ //缺少帐户字段
            auto const result = env.rpc ("json", "noripple_check", "{}")
                [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Missing field 'account'.");
        }

{ //缺少角色字段
            Json::Value params;
            params[jss::account] = alice.human();
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Missing field 'role'.");
        }

{ //角色字段无效
            Json::Value params;
            params[jss::account] = alice.human();
            params[jss::role] = "not_a_role";
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Invalid field 'role'.");
        }

{ //无效极限
            Json::Value params;
            params[jss::account] = alice.human();
            params[jss::role] = "user";
            params[jss::limit] = -1;
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "Invalid field 'limit', not unsigned integer.");
        }

{ //无效的分类帐（哈希）
            Json::Value params;
            params[jss::account] = alice.human();
            params[jss::role] = "user";
            params[jss::ledger_hash] = 1;
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "invalidParams");
            BEAST_EXPECT (result[jss::error_message] ==
                "ledgerHashNotString");
        }

{ //找不到帐户
            Json::Value params;
            params[jss::account] = Account{"nobody"}.human();
            params[jss::role] = "user";
            params[jss::ledger] = "current";
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "actNotFound");
            BEAST_EXPECT (result[jss::error_message] ==
                "Account not found.");
        }

{ //传递帐户私钥将导致
//作为失败的种子进行分析
            Json::Value params;
            params[jss::account] =
                toBase58 (TokenType::NodePrivate, alice.sk());
            params[jss::role] = "user";
            params[jss::ledger] = "current";
            auto const result = env.rpc ("json", "noripple_check",
                boost::lexical_cast<std::string>(params)) [jss::result];
            BEAST_EXPECT (result[jss::error] == "badSeed");
            BEAST_EXPECT (result[jss::error_message] ==
                "Disallowed seed.");
        }
    }

    void
    testBasic (bool user, bool problems)
    {
        testcase << "Request noripple_check for " <<
            (user ? "user" : "gateway") << " role, expect" <<
            (problems ? "" : " no") << " problems";

        using namespace test::jtx;
        Env env {*this};

        auto const gw = Account {"gw"};
        auto const alice = Account {"alice"};

        env.fund (XRP(10000), gw, alice);
        if ((user && problems) || (!user && !problems))
        {
            env (fset (alice, asfDefaultRipple));
            env (trust (alice, gw["USD"](100)));
        }
        else
        {
            env (fclear (alice, asfDefaultRipple));
            env (trust (alice, gw["USD"](100), gw, tfSetNoRipple));
        }
        env.close ();

        Json::Value params;
        params[jss::account] = alice.human();
        params[jss::role] = (user ? "user" : "gateway");
        params[jss::ledger] = "current";
        auto result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];

        auto const pa = result["problems"];
        if (! BEAST_EXPECT (pa.isArray ()))
            return;

        if (problems)
        {
            if (! BEAST_EXPECT (pa.size() == 2))
                return;

            if (user)
            {
                BEAST_EXPECT (
                    boost::starts_with(pa[0u].asString(),
                        "You appear to have set"));
                BEAST_EXPECT (
                    boost::starts_with(pa[1u].asString(),
                        "You should probably set"));
            }
            else
            {
                BEAST_EXPECT (
                    boost::starts_with(pa[0u].asString(),
                        "You should immediately set"));
                BEAST_EXPECT(
                    boost::starts_with(pa[1u].asString(),
                        "You should clear"));
            }
        }
        else
        {
            BEAST_EXPECT (pa.size() == 0);
        }

//现在再次请求相关交易
//时间。
        params[jss::transactions] = true;
        result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];
        if (! BEAST_EXPECT (result[jss::transactions].isArray ()))
            return;

        auto const txs = result[jss::transactions];
        if (problems)
        {
            if (! BEAST_EXPECT (txs.size () == (user ? 1 : 2)))
                return;

            if (! user)
            {
                BEAST_EXPECT (txs[0u][jss::Account] == alice.human());
                BEAST_EXPECT (txs[0u][jss::TransactionType] == "AccountSet");
            }

            BEAST_EXPECT (
                result[jss::transactions][txs.size()-1][jss::Account] ==
                alice.human());
            BEAST_EXPECT (
                result[jss::transactions][txs.size()-1][jss::TransactionType] ==
                "TrustSet");
            BEAST_EXPECT (
                result[jss::transactions][txs.size()-1][jss::LimitAmount] ==
                gw["USD"](100).value ().getJson (0));
        }
        else
        {
            BEAST_EXPECT (txs.size () == 0);
        }
    }

public:
    void run () override
    {
        testBadInput ();
        for (auto user : {true, false})
            for (auto problem : {true, false})
                testBasic (user, problem);
    }
};

class NoRippleCheckLimits_test : public beast::unit_test::suite
{
    void
    testLimits(bool admin)
    {
        testcase << "Check limits in returned data, " <<
            (admin ? "admin" : "non-admin");

        using namespace test::jtx;

        Env env {*this, admin ? envconfig () : envconfig(no_admin)};

        auto const alice = Account {"alice"};
        env.fund (XRP (100000), alice);
        env (fset (alice, asfDefaultRipple));
        env.close ();

        auto checkBalance = [&env]()
        {
//这是端点坠落预防。非管理端口将丢失
//如果请求的速度太快，那么我们将操纵
//这里的资源管理器用于重置端点平衡（用于
//本地主机）如果我们太接近下降限制。它会
//如果我们能以某种方式将这个功能添加到env中，那就更好了。
//或者对某些测试禁用端点充电
//病例。
            using namespace ripple::Resource;
            using namespace std::chrono;
            using namespace beast::IP;
            auto c = env.app ().getResourceManager ()
                .newInboundEndpoint (
                    Endpoint::from_string (test::getEnvLocalhostAddr()));

//如果我们超过警告阈值，重置
            if (c.balance() > warningThreshold)
            {
                using clock_type = beast::abstract_clock <steady_clock>;
                c.entry().local_balance =
                    DecayingSample <decayWindowSeconds, clock_type>
                        {steady_clock::now()};
            }
        };

        for (auto i = 0; i < ripple::RPC::Tuning::noRippleCheck.rmax + 5; ++i)
        {
            if (! admin)
                checkBalance();

            auto& txq = env.app().getTxQ();
            auto const gw = Account {"gw" + std::to_string(i)};
            env.memoize(gw);
            env (pay (env.master, gw, XRP(1000)),
                seq (autofill),
                fee (txq.getMetrics(*env.current()).openLedgerFeeLevel + 1),
                sig (autofill));
            env (fset (gw, asfDefaultRipple),
                seq (autofill),
                fee (txq.getMetrics(*env.current()).openLedgerFeeLevel + 1),
                sig (autofill));
            env (trust (alice, gw["USD"](10)),
                fee (txq.getMetrics(*env.current()).openLedgerFeeLevel + 1));
            env.close();
        }

//默认极限值
        Json::Value params;
        params[jss::account] = alice.human();
        params[jss::role] = "user";
        params[jss::ledger] = "current";
        auto result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];

        BEAST_EXPECT (result["problems"].size() == 301);

//低于最小值一个
        params[jss::limit] = 9;
        result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (result["problems"].size() == (admin ? 10 : 11));

//在最低限度
        params[jss::limit] = 10;
        result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (result["problems"].size() == 11);

//在最大
        params[jss::limit] = 400;
        result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (result["problems"].size() == 401);

//在max + 1
        params[jss::limit] = 401;
        result = env.rpc ("json", "noripple_check",
            boost::lexical_cast<std::string>(params)) [jss::result];
        BEAST_EXPECT (result["problems"].size() == (admin ? 402 : 401));
    }

public:
    void run () override
    {
        for (auto admin : {true, false})
            testLimits (admin);
    }
};

BEAST_DEFINE_TESTSUITE(NoRippleCheck, app, ripple);

//这些处理限额的测试很慢，因为
//提供/帐户设置，因此使它们成为手动的--提供的附加保险范围
//他们是最小的

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(NoRippleCheckLimits, app, ripple, 1);

} //涟漪

