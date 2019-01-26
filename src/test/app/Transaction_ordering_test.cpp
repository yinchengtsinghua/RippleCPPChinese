
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
#include <ripple/protocol/ErrorCodes.h>
#include <test/jtx.h>

namespace ripple {
namespace test {

struct Transaction_ordering_test : public beast::unit_test::suite
{
    void testCorrectOrder()
    {
        using namespace jtx;

        Env env(*this);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        auto const tx1 = env.jt(noop(alice), seq(aliceSequence));
        auto const tx2 = env.jt(noop(alice), seq(aliceSequence + 1),
            json(R"({"LastLedgerSequence":7})"));

        env(tx1);
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 1);
        env(tx2);
        env.close();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 2);

        env.close();

        {
            auto const result = env.rpc("tx", to_string(tx1.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
        {
            auto const result = env.rpc("tx", to_string(tx2.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void testIncorrectOrder()
    {
        using namespace jtx;

        Env env(*this);
        env.app().getJobQueue().setThreadCount(0, false);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        auto const tx1 = env.jt(noop(alice), seq(aliceSequence));
        auto const tx2 = env.jt(noop(alice), seq(aliceSequence + 1),
            json(R"({"LastLedgerSequence":7})"));

        env(tx2, ter(terPRE_SEQ));
        BEAST_EXPECT(env.seq(alice) == aliceSequence);
        env(tx1);
        env.app().getJobQueue().rendezvous();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 2);

        env.close();

        {
            auto const result = env.rpc("tx", to_string(tx1.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
        {
            auto const result = env.rpc("tx", to_string(tx2.stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void testIncorrectOrderMultipleIntermediaries()
    {
        using namespace jtx;

        Env env(*this);
        env.app().getJobQueue().setThreadCount(0, false);
        auto const alice = Account("alice");
        env.fund(XRP(1000), noripple(alice));

        auto const aliceSequence = env.seq(alice);

        std::vector<JTx> tx;
        for (auto i = 0; i < 5; ++i)
        {
            tx.emplace_back(
                env.jt(noop(alice), seq(aliceSequence + i),
                    json(R"({"LastLedgerSequence":7})"))
            );
        }

        for (auto i = 1; i < 5; ++i)
        {
            env(tx[i], ter(terPRE_SEQ));
            BEAST_EXPECT(env.seq(alice) == aliceSequence);
        }

        env(tx[0]);
        env.app().getJobQueue().rendezvous();
        BEAST_EXPECT(env.seq(alice) == aliceSequence + 5);

        env.close();

        for (auto i = 0; i < 5; ++i)
        {
            auto const result = env.rpc("tx", to_string(tx[i].stx->getTransactionID()));
            BEAST_EXPECT(result["result"]["meta"]["TransactionResult"] == "tesSUCCESS");
        }
    }

    void run() override
    {
        testCorrectOrder();
        testIncorrectOrder();
        testIncorrectOrderMultipleIntermediaries();
    }
};

BEAST_DEFINE_TESTSUITE(Transaction_ordering,app,ripple);

} //测试
} //涟漪
