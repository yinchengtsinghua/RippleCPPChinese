
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

#include <ripple/core/JobQueue.h>
#include <ripple/protocol/JsonFields.h>
#include <test/jtx.h>
#include <test/jtx/WSClient.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace test {

class RobustTransaction_test : public beast::unit_test::suite
{
public:
    void
    testSequenceRealignment()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
//RPC订阅事务流
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("subscribe", jv);
            BEAST_EXPECT(jv[jss::status] == "success");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
        }

        {
//提交过去的分类帐序列交易记录
            Json::Value payment;
            payment[jss::secret] = toBase58(generateSeed("alice"));
            payment[jss::tx_json] = pay("alice", "bob", XRP(1));
            payment[jss::tx_json][sfLastLedgerSequence.fieldName] = 1;
            auto jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tefMAX_LEDGER");

//提交过去的序列事务
            payment[jss::tx_json] = pay("alice", "bob", XRP(1));
            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice") - 1;
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tefPAST_SEQ");

//提交未来序列事务
            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice") + 1;
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "terPRE_SEQ");

//提交事务以弥合序列间隙
            payment[jss::tx_json][sfSequence.fieldName] =
                env.seq("alice");
            jv = wsc->invoke("submit", payment);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

//等待作业队列处理所有内容
            env.app().getJobQueue().rendezvous();

//完成交易
            jv = wsc->invoke("ledger_accept");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result].isMember(
                jss::ledger_current_index));
        }

        {
//支票余额
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    auto const& ff = jv[jss::meta]["AffectedNodes"]
                        [1u]["ModifiedNode"]["FinalFields"];
                    return ff[jss::Account] == Account("bob").human() &&
                        ff["Balance"] == "10001000000";
                }));

            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    auto const& ff = jv[jss::meta]["AffectedNodes"]
                        [1u]["ModifiedNode"]["FinalFields"];
                    return ff[jss::Account] == Account("bob").human() &&
                        ff["Balance"] == "10002000000";
                }));
        }

        {
//rpc取消订阅事务流
            Json::Value jv;
            jv[jss::streams] = Json::arrayValue;
            jv[jss::streams].append("transactions");
            jv = wsc->invoke("unsubscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }
    }

    /*
    提交正常付款。在建议的
    收到交易结果。

    客户端将在将来重新连接。在此期间，假定
    事务应该已成功。

    重新连接后，将加载最近的帐户交易历史记录。
    应检测提交的事务，并且该事务应
    最终成功。
    **/

    void
    testReconnect()
    {
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
//提交正常付款
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = pay("alice", "bob", XRP(1));
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

//断开
            wsc.reset();

//服务器完成事务
            env.close();
        }

        {
//RPC会计
            Json::Value jv;
            jv[jss::account] = Account("bob").human();
            jv[jss::ledger_index_min] = -1;
            jv[jss::ledger_index_max] = -1;
            wsc = makeWSClient(env.app().config());
            jv = wsc->invoke("account_tx", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }

//支票余额
            auto ff = jv[jss::result][jss::transactions][0u][jss::meta]
                ["AffectedNodes"][1u]["ModifiedNode"]["FinalFields"];
            BEAST_EXPECT(ff[jss::Account] ==
                Account("bob").human());
            BEAST_EXPECT(ff["Balance"] == "10001000000");
        }
    }

    void
    testReconnectAfterWait()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice", "bob");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
//提交正常付款
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = pay("alice", "bob", XRP(1));
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");

//完成事务
            jv = wsc->invoke("ledger_accept");
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result].isMember(
                jss::ledger_current_index));

//等待作业队列处理所有内容
            env.app().getJobQueue().rendezvous();
        }

        {
            {
//RPC订阅分类帐流
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("subscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }

//封闭式分类帐
            for(auto i = 0; i < 8; ++i)
            {
                auto jv = wsc->invoke("ledger_accept");
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::result].
                    isMember(jss::ledger_current_index));

//等待作业队列处理所有内容
                env.app().getJobQueue().rendezvous();

                BEAST_EXPECT(wsc->findMsg(5s,
                    [&](auto const& jv)
                    {
                        return jv[jss::type] == "ledgerClosed";
                    }));
            }

            {
//RPC取消订阅分类帐流
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("unsubscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }
        }

        {
//断开，重新连接
            wsc = makeWSClient(env.app().config());
            {
//RPC订阅分类帐流
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("subscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }

//封闭式分类帐
            for (auto i = 0; i < 2; ++i)
            {
                auto jv = wsc->invoke("ledger_accept");
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::result].
                    isMember(jss::ledger_current_index));

//等待作业队列处理所有内容
                env.app().getJobQueue().rendezvous();

                BEAST_EXPECT(wsc->findMsg(5s,
                    [&](auto const& jv)
                    {
                        return jv[jss::type] == "ledgerClosed";
                    }));
            }

            {
//RPC取消订阅分类帐流
                Json::Value jv;
                jv[jss::streams] = Json::arrayValue;
                jv[jss::streams].append("ledger");
                jv = wsc->invoke("unsubscribe", jv);
                if (wsc->version() == 2)
                {
                    BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                    BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
                }
                BEAST_EXPECT(jv[jss::status] == "success");
            }
        }

        {
//RPC会计
            Json::Value jv;
            jv[jss::account] = Account("bob").human();
            jv[jss::ledger_index_min] = -1;
            jv[jss::ledger_index_max] = -1;
            wsc = makeWSClient(env.app().config());
            jv = wsc->invoke("account_tx", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }

//支票余额
            auto ff = jv[jss::result][jss::transactions][0u][jss::meta]
                ["AffectedNodes"][1u]["ModifiedNode"]["FinalFields"];
            BEAST_EXPECT(ff[jss::Account] ==
                Account("bob").human());
            BEAST_EXPECT(ff["Balance"] == "10001000000");
        }
    }

    void
    testAccountsProposed()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        env.fund(XRP(10000), "alice");
        env.close();
        auto wsc = makeWSClient(env.app().config());

        {
//rpc订阅帐户\建议的流
            Json::Value jv;
            jv[jss::accounts_proposed] = Json::arrayValue;
            jv[jss::accounts_proposed].append(
                Account("alice").human());
            jv = wsc->invoke("subscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        {
//提交帐户\设置交易记录
            Json::Value jv;
            jv[jss::secret] = toBase58(generateSeed("alice"));
            jv[jss::tx_json] = fset("alice", 0);
            jv[jss::tx_json][jss::Fee] = 10;
            jv = wsc->invoke("submit", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::result][jss::engine_result] ==
                "tesSUCCESS");
        }

        {
//检查流更新
            BEAST_EXPECT(wsc->findMsg(5s,
                [&](auto const& jv)
                {
                    return jv[jss::transaction][jss::TransactionType] ==
                        "AccountSet";
                }));
        }

        {
//rpc取消订阅帐户\u建议的流
            Json::Value jv;
            jv[jss::accounts_proposed] = Json::arrayValue;
            jv[jss::accounts_proposed].append(
                Account("alice").human());
            jv = wsc->invoke("unsubscribe", jv);
            if (wsc->version() == 2)
            {
                BEAST_EXPECT(jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0");
                BEAST_EXPECT(jv.isMember(jss::id) && jv[jss::id] == 5);
            }
            BEAST_EXPECT(jv[jss::status] == "success");
        }
    }

    void
    run() override
    {
        testSequenceRealignment();
        testReconnect();
        testReconnectAfterWait();
        testAccountsProposed();
    }
};

BEAST_DEFINE_TESTSUITE(RobustTransaction,app,ripple);

} //测试
} //涟漪
