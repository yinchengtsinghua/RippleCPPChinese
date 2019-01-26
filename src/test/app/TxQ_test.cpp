
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

#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/tx/apply.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/mulDiv.h>
#include <test/jtx/TestSuite.h>
#include <test/jtx/envconfig.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/st.h>
#include <test/jtx.h>
#include <test/jtx/ticket.h>
#include <boost/optional.hpp>
#include <test/jtx/WSClient.h>

namespace ripple {

namespace test {

class TxQ_test : public beast::unit_test::suite
{
    void
    checkMetrics(
        jtx::Env& env,
        std::size_t expectedCount,
        boost::optional<std::size_t> expectedMaxCount,
        std::size_t expectedInLedger,
        std::size_t expectedPerLedger,
        std::uint64_t expectedMinFeeLevel,
        std::uint64_t expectedMedFeeLevel = 256 * 500)
    {
        auto const metrics = env.app().getTxQ().getMetrics(*env.current());
        BEAST_EXPECT(metrics.referenceFeeLevel == 256);
        BEAST_EXPECT(metrics.txCount == expectedCount);
        BEAST_EXPECT(metrics.txQMaxSize == expectedMaxCount);
        BEAST_EXPECT(metrics.txInLedger == expectedInLedger);
        BEAST_EXPECT(metrics.txPerLedger == expectedPerLedger);
        BEAST_EXPECT(metrics.minProcessingFeeLevel == expectedMinFeeLevel);
        BEAST_EXPECT(metrics.medFeeLevel == expectedMedFeeLevel);
        auto expectedCurFeeLevel = expectedInLedger > expectedPerLedger ?
            expectedMedFeeLevel * expectedInLedger * expectedInLedger /
                (expectedPerLedger * expectedPerLedger) :
                    metrics.referenceFeeLevel;
        BEAST_EXPECT(metrics.openLedgerFeeLevel == expectedCurFeeLevel);
    }

    void
    fillQueue(
        jtx::Env& env,
        jtx::Account const& account)
    {
        auto metrics = env.app().getTxQ().getMetrics(*env.current());
        for (int i = metrics.txInLedger; i <= metrics.txPerLedger; ++i)
            env(noop(account));
    }

    auto
    openLedgerFee(jtx::Env& env)
    {
        using namespace jtx;

        auto const& view = *env.current();
        auto metrics = env.app().getTxQ().getMetrics(view);

//不关心溢出标志
        return fee(mulDiv(metrics.openLedgerFeeLevel,
            view.fees().base, metrics.referenceFeeLevel).second + 1);
    }

    static
    std::unique_ptr<Config>
    makeConfig(std::map<std::string, std::string> extraTxQ = {},
        std::map<std::string, std::string> extraVoting = {})
    {
        auto p = test::jtx::envconfig();
        auto& section = p->section("transaction_queue");
        section.set("ledgers_in_queue", "2");
        section.set("minimum_queue_size", "2");
        section.set("min_ledgers_to_compute_size_limit", "3");
        section.set("max_ledger_counts_to_store", "100");
        section.set("retry_sequence_percent", "25");
        section.set("zero_basefee_transaction_feelevel", "100000000000");
        section.set("normal_consensus_increase_percent", "0");

        for (auto const& value : extraTxQ)
            section.set(value.first, value.second);

//某些测试指定由启用的不同费用设置
//拳头
        if (!extraVoting.empty())
        {

            auto& votingSection = p->section("voting");
            for (auto const & value : extraVoting)
            {
                votingSection.set(value.first, value.second);
            }

//为了进行投票，我们必须作为验证器运行
            p->section("validation_seed").legacy("shUwVw52ofnCUX5m7kPTKzJdr4HEH");
        }
        return p;
    }

    std::size_t
    initFee(jtx::Env& env, std::size_t expectedPerLedger,
        std::size_t ledgersInQueue, std::uint32_t base,
        std::uint32_t units, std::uint32_t reserve, std::uint32_t increment)
    {
//通过标志分类账，以便进行费用变更投票，以及
//降低预备费。（它还激活所有支持的
//修正案。）这将允许创建低于
//准备金和余额。
        for(auto i = env.current()->seq(); i <= 257; ++i)
            env.close();
//标志分类帐之后的分类帐创建所有
//费用（1）和修订（Supportedamendments（）.Size（））
//伪交易。他们都有0个费用，也就是说
//被队列视为高收费级别，因此
//中层为1000000000。
        auto const flagPerLedger = 1 +
            ripple::detail::supportedAmendments().size();
        auto const flagMaxQueue = ledgersInQueue * flagPerLedger;
        checkMetrics(env, 0, flagMaxQueue, 0, flagPerLedger, 256,
            100000000000);

//用正常的费用加上几个TXS，这样中位数就来了
//恢复正常
        env(noop(env.master));
        env(noop(env.master));

//延迟关闭分类帐，导致所有TxQ
//要重置为默认值的度量，但MaxQueue大小除外。
        using namespace std::chrono_literals;
        env.close(env.now() + 5s, 10000ms);
        checkMetrics(env, 0, flagMaxQueue, 0, expectedPerLedger, 256);
        auto const fees = env.current()->fees();
        BEAST_EXPECT(fees.base == base);
        BEAST_EXPECT(fees.units == units);
        BEAST_EXPECT(fees.reserve == reserve);
        BEAST_EXPECT(fees.increment == increment);

        return flagMaxQueue;
    }

public:
    void testQueue()
    {
        using namespace jtx;
        using namespace std::chrono;

        Env env(*this,
            makeConfig({ {"minimum_txn_in_ledger_standalone", "3"} }));
        auto& txq = env.app().getTxQ();

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");
        auto daria = Account("daria");
        auto elmo = Account("elmo");
        auto fred = Account("fred");
        auto gwen = Account("gwen");
        auto hank = Account("hank");

        auto queued = ter(terQUEUED);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 3, 256);

//在费用低廉的情况下创建多个帐户，因此它们都适用。
        env.fund(XRP(50000), noripple(alice, bob, charlie, daria));
        checkMetrics(env, 0, boost::none, 4, 3, 256);

//爱丽丝-价格开始暴涨：持仓
        env(noop(alice), queued);
        checkMetrics(env, 1, boost::none, 4, 3, 256);

//鲍勃收费很高-适用
        env(noop(bob), openLedgerFee(env));
        checkMetrics(env, 1, boost::none, 5, 3, 256);

//低收费达里亚：保持
        env(noop(daria), fee(1000), queued);
        checkMetrics(env, 2, boost::none, 5, 3, 256);

        env.close();
//验证已应用保留的事务
        checkMetrics(env, 0, 10, 2, 5, 256);

//////////////////////////////////////////

//多做些账目。我们以后需要他们来滥用队列。
        env.fund(XRP(50000), noripple(elmo, fred, gwen, hank));
        checkMetrics(env, 0, 10, 6, 5, 256);

//现在有一堆交易在进行。
        env(noop(alice), fee(12), queued);
        checkMetrics(env, 1, 10, 6, 5, 256);

env(noop(bob), fee(10), queued); //无法清除队列
        env(noop(charlie), fee(20), queued);
        env(noop(daria), fee(15), queued);
        env(noop(elmo), fee(11), queued);
        env(noop(fred), fee(19), queued);
        env(noop(gwen), fee(16), queued);
        env(noop(hank), fee(18), queued);
        checkMetrics(env, 8, 10, 6, 5, 256);

        env.close();
//验证已应用保留的事务
        checkMetrics(env, 1, 12, 7, 6, 256);

//Bob的事务仍然排在队列中。

//////////////////////////////////////////

//汉克又发了一个txn
        env(noop(hank), fee(10), queued);
//但他不会把它留在队伍里
        checkMetrics(env, 2, 12, 7, 6, 256);

//汉克看到他的txn被扣住了，并提高了费用，
//但都没有足够的撞击来重奏
        env(noop(hank), fee(11), ter(telCAN_NOT_QUEUE_FEE));
        checkMetrics(env, 2, 12, 7, 6, 256);

//汉克看到他的txn被扣住了，并提高了费用，
//足以重奏，但不足以重击
//应用于分类帐
        env(noop(hank), fee(6000), queued);
//但他不会把它留在队伍里
        checkMetrics(env, 2, 12, 7, 6, 256);

//汉克看到他的txn被扣住了，并提高了费用，
//高到可以进入未结分类账，因为
//他不想等。
        env(noop(hank), openLedgerFee(env));
        checkMetrics(env, 1, 12, 8, 6, 256);

//然后汉克发送另一个不太重要的txn
//（除了度量之外，这将验证
//原来的txn被移除。）
        env(noop(hank), fee(6000), queued);
        checkMetrics(env, 2, 12, 8, 6, 256);

        env.close();

//验证是否应用了Bob和Hank的TXN
        checkMetrics(env, 0, 16, 2, 8, 256);

//以模拟时间跳跃再次关闭
//将升级限制重置为最小值
        env.close(env.now() + 5s, 10000ms);
        checkMetrics(env, 0, 16, 0, 3, 256);
//然后再关闭一次，没有时间跳跃
//将队列最大值重置为最小值
        env.close();
        checkMetrics(env, 0, 6, 0, 3, 256);

//////////////////////////////////////////

//把分类帐和队列填起来，这样我们就可以核实
//东西被踢出去了。
        env(noop(hank), fee(7000));
        env(noop(gwen), fee(7000));
        env(noop(fred), fee(7000));
        env(noop(elmo), fee(7000));
        checkMetrics(env, 0, 6, 4, 3, 256);

//使用明确的费用，这样我们就可以控制哪个TXN
//会掉下来的
//这一个进入队列，但当
//以后再加一笔更高的费用。
        env(noop(daria), fee(15), queued);
//这些都在排队。
        env(noop(elmo), fee(16), queued);
        env(noop(fred), fee(17), queued);
        env(noop(gwen), fee(18), queued);
        env(noop(hank), fee(19), queued);
        env(noop(alice), fee(20), queued);

//队列已满。
        checkMetrics(env, 6, 6, 4, 3, 385);

//尝试添加另一个具有默认（低）费用的交易，
//它应该失败，因为队列已满。
        env(noop(charlie), ter(telCAN_NOT_QUEUE_FULL));

//增加另一笔交易，费用更高，
//不高到可以进入分类帐，但是高
//足以进入队列（并将某人踢出）
        env(noop(charlie), fee(100), queued);

//当然，排队的人还是满的，但是最低收费已经上涨了
        checkMetrics(env, 6, 6, 4, 3, 410);

//关闭分类帐，接受交易，以及
//队列被清除，然后重试localtx。在这
//点，从队列中删除的Daria的事务
//放回原处。整洁的
        env.close();
        checkMetrics(env, 2, 8, 5, 4, 256, 256 * 700);

        env.close();
        checkMetrics(env, 0, 10, 2, 5, 256);

//////////////////////////////////////////
//Cleanup：

//再创建一些事务，以便
//我们可以确定队列中有一个
//测试结束，txq被破坏。

        auto metrics = txq.getMetrics(*env.current());
        BEAST_EXPECT(metrics.txCount == 0);

//把账本塞进。
        for (int i = metrics.txInLedger; i <= metrics.txPerLedger; ++i)
        {
            env(noop(env.master));
        }

//将一个直接事务排队
        env(noop(env.master), fee(20), queued);
        ++metrics.txCount;

        checkMetrics(env, metrics.txCount,
            metrics.txQMaxSize, metrics.txPerLedger + 1,
            metrics.txPerLedger,
            256);
    }

    void testTecResult()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "2" } }));

        auto alice = Account("alice");
        auto gw = Account("gw");
        auto USD = gw["USD"];

        checkMetrics(env, 0, boost::none, 0, 2, 256);

//创建帐户
        env.fund(XRP(50000), noripple(alice, gw));
        checkMetrics(env, 0, boost::none, 2, 2, 256);
        env.close();
        checkMetrics(env, 0, 4, 0, 2, 256);

//艾丽斯在分类帐未满时创建一个无资金支持的报价
        env(offer(alice, XRP(1000), USD(1000)), ter(tecUNFUNDED_OFFER));
        checkMetrics(env, 0, 4, 1, 2, 256);

        fillQueue(env, alice);
        checkMetrics(env, 0, 4, 3, 2, 256);

//Alice创建了一个无资金支持的报价
        env(offer(alice, XRP(1000), USD(1000)), ter(terQUEUED));
        checkMetrics(env, 1, 4, 3, 2, 256);

//报价排在队伍之外。
        env.close();
        checkMetrics(env, 0, 6, 1, 3, 256);
    }

    void testLocalTxRetry()
    {
        using namespace jtx;
        using namespace std::chrono;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "2" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");

        auto queued = ter(terQUEUED);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 2, 256);

//在费用低廉的情况下创建多个帐户，因此它们都适用。
        env.fund(XRP(50000), noripple(alice, bob, charlie));
        checkMetrics(env, 0, boost::none, 3, 2, 256);

//Alice的未来交易-失败
        env(noop(alice), openLedgerFee(env),
            seq(env.seq(alice) + 1), ter(terPRE_SEQ));
        checkMetrics(env, 0, boost::none, 3, 2, 256);

//Alice的当前交易：持有
        env(noop(alice), queued);
        checkMetrics(env, 1, boost::none, 3, 2, 256);

//艾丽丝-序列太超前了，所以不会排队。
        env(noop(alice), seq(env.seq(alice) + 2),
            ter(terPRE_SEQ));
        checkMetrics(env, 1, boost::none, 3, 2, 256);

//鲍勃收费很高-适用
        env(noop(bob), openLedgerFee(env));
        checkMetrics(env, 1, boost::none, 4, 2, 256);

//低收费达里亚：保持
        env(noop(charlie), fee(1000), queued);
        checkMetrics(env, 2, boost::none, 4, 2, 256);

//爱丽丝正常收费：保留
        env(noop(alice), seq(env.seq(alice) + 1),
            queued);
        checkMetrics(env, 3, boost::none, 4, 2, 256);

        env.close();
//验证已应用保留的事务
//Alice的不良交易来自
//本地TXS。
        checkMetrics(env, 0, 8, 4, 4, 256);
    }

    void testLastLedgerSeq()
    {
        using namespace jtx;
        using namespace std::chrono;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "2" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");
        auto daria = Account("daria");
        auto edgar = Account("edgar");
        auto felicia = Account("felicia");

        auto queued = ter(terQUEUED);

        checkMetrics(env, 0, boost::none, 0, 2, 256);

//通过多个分类账进行资金筹措，以限制txq指标。
        env.fund(XRP(1000), noripple(alice, bob));
        env.close(env.now() + 5s, 10000ms);
        env.fund(XRP(1000), noripple(charlie, daria));
        env.close(env.now() + 5s, 10000ms);
        env.fund(XRP(1000), noripple(edgar, felicia));
        env.close(env.now() + 5s, 10000ms);

        checkMetrics(env, 0, boost::none, 0, 2, 256);
        env(noop(bob));
        env(noop(charlie));
        env(noop(daria));
        checkMetrics(env, 0, boost::none, 3, 2, 256);

        BEAST_EXPECT(env.current()->info().seq == 6);
//无法将具有低lastledgerseq的项排队
        env(noop(alice), json(R"({"LastLedgerSequence":7})"),
            ter(telCAN_NOT_QUEUE));
//使用足够的lastledgerseq对项目进行排队。
        env(noop(alice), json(R"({"LastLedgerSequence":8})"),
            queued);
//对收费较高的项目进行排队，以强制以前的项目
//TXN等待。
        env(noop(bob), fee(7000), queued);
        env(noop(charlie), fee(7000), queued);
        env(noop(daria), fee(7000), queued);
        env(noop(edgar), fee(7000), queued);
        checkMetrics(env, 5, boost::none, 3, 2, 256);
        {
            auto& txQ = env.app().getTxQ();
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            BEAST_EXPECT(aliceStat.size() == 1);
            BEAST_EXPECT(aliceStat.begin()->second.feeLevel == 256);
            BEAST_EXPECT(aliceStat.begin()->second.lastValid &&
                *aliceStat.begin()->second.lastValid == 8);
            BEAST_EXPECT(!aliceStat.begin()->second.consequences);

            auto bobStat = txQ.getAccountTxs(bob.id(), *env.current());
            BEAST_EXPECT(bobStat.size() == 1);
            BEAST_EXPECT(bobStat.begin()->second.feeLevel == 7000 * 256 / 10);
            BEAST_EXPECT(!bobStat.begin()->second.lastValid);
            BEAST_EXPECT(!bobStat.begin()->second.consequences);

            auto noStat = txQ.getAccountTxs(Account::master.id(),
                *env.current());
            BEAST_EXPECT(noStat.empty());
        }

        env.close();
        checkMetrics(env, 1, 6, 4, 3, 256);

//保持Alice的事务等待。
        env(noop(bob), fee(7000), queued);
        env(noop(charlie), fee(7000), queued);
        env(noop(daria), fee(7000), queued);
        env(noop(edgar), fee(7000), queued);
        env(noop(felicia), fee(7000), queued);
        checkMetrics(env, 6, 6, 4, 3, 257);

        env.close();
//艾丽斯的交易仍在继续。
        checkMetrics(env, 1, 8, 5, 4, 256, 700 * 256);
        BEAST_EXPECT(env.seq(alice) == 1);

//保持Alice的事务等待。
        env(noop(bob), fee(8000), queued);
        env(noop(charlie), fee(8000), queued);
        env(noop(daria), fee(8000), queued);
        env(noop(daria), fee(8000), seq(env.seq(daria) + 1),
            queued);
        env(noop(edgar), fee(8000), queued);
        env(noop(felicia), fee(8000), queued);
        env(noop(felicia), fee(8000), seq(env.seq(felicia) + 1),
            queued);
        checkMetrics(env, 8, 8, 5, 4, 257, 700 * 256);

        env.close();
//艾丽斯的交易到期了
//进入分类帐，她的交易就不复存在了，
//尽管费利西亚的一个仍在排队。
        checkMetrics(env, 1, 10, 6, 5, 256, 700 * 256);
        BEAST_EXPECT(env.seq(alice) == 1);

        env.close();
//现在队列是空的
        checkMetrics(env, 0, 12, 1, 6, 256, 800 * 256);
        BEAST_EXPECT(env.seq(alice) == 1);
    }

    void testZeroFeeTxn()
    {
        using namespace jtx;
        using namespace std::chrono;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "2" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto carol = Account("carol");

        auto queued = ter(terQUEUED);

        checkMetrics(env, 0, boost::none, 0, 2, 256);

//通过多个分类账进行资金筹措，以限制txq指标。
        env.fund(XRP(1000), noripple(alice, bob));
        env.close(env.now() + 5s, 10000ms);
        env.fund(XRP(1000), noripple(carol));
        env.close(env.now() + 5s, 10000ms);

//填入分类帐
        env(noop(alice));
        env(noop(alice));
        env(noop(alice));
        checkMetrics(env, 0, boost::none, 3, 2, 256);

        env(noop(bob), queued);
        checkMetrics(env, 1, boost::none, 3, 2, 256);

//即使这笔交易的费用是0，
//setRegularKey:：CalculateBaseFee表示
//“免费”交易，因此它有“无限”的费用
//水平并进入未结分类帐。
        env(regkey(alice, bob), fee(0));
        checkMetrics(env, 1, boost::none, 4, 2, 256);

//把这个账本关掉，这样我们可以得到一个最大的
        env.close();
        checkMetrics(env, 0, 8, 1, 4, 256);

        fillQueue(env, bob);
        checkMetrics(env, 0, 8, 5, 4, 256);

        auto feeBob = 30;
        auto seqBob = env.seq(bob);
        for (int i = 0; i < 4; ++i)
        {
            env(noop(bob), fee(feeBob),
                seq(seqBob), queued);
            feeBob = (feeBob + 1) * 125 / 100;
            ++seqBob;
        }
        checkMetrics(env, 4, 8, 5, 4, 256);

//该交易还具有“无限”的费用水平，
//但由于Bob在队列中有txn，所以它会排队。
        env(regkey(bob, alice), fee(0),
            seq(seqBob), queued);
        ++seqBob;
        checkMetrics(env, 5, 8, 5, 4, 256);

//不幸的是，鲍勃再也找不到TXN了。
//队列，因为存在多个百分比。
//坦斯塔夫
        env(noop(bob), fee(XRP(100)),
            seq(seqBob), ter(telINSUF_FEE_P));

//卡罗尔排满了队，但一个也踢不出来。
//交易。
        auto feeCarol = feeBob;
        auto seqCarol = env.seq(carol);
        for (int i = 0; i < 3; ++i)
        {
            env(noop(carol), fee(feeCarol),
                seq(seqCarol), queued);
            feeCarol = (feeCarol + 1) * 125 / 100;
            ++seqCarol;
        }
        checkMetrics(env, 8, 8, 5, 4, 3 * 256 + 1);

//卡罗尔没有足够高的屈服力来打败鲍勃。
//平均费用。（即~144115188075855907
//因为零费用txn。）
        env(noop(carol), fee(feeCarol),
            seq(seqCarol), ter(telCAN_NOT_QUEUE_FULL));

        env.close();
//Bob的一些事务仍在队列中，
//卡罗尔的低收费Tx从
//本地TXS。
        checkMetrics(env, 3, 10, 6, 5, 256);
        BEAST_EXPECT(env.seq(bob) == seqBob - 2);
        BEAST_EXPECT(env.seq(carol) == seqCarol);


        env.close();
        checkMetrics(env, 0, 12, 4, 6, 256);
        BEAST_EXPECT(env.seq(bob) == seqBob + 1);
        BEAST_EXPECT(env.seq(carol) == seqCarol + 1);

        env.close();
        checkMetrics(env, 0, 12, 0, 6, 256);
        BEAST_EXPECT(env.seq(bob) == seqBob + 1);
        BEAST_EXPECT(env.seq(carol) == seqCarol + 1);
    }

    void testPreclaimFailures()
    {
        using namespace jtx;

        Env env(*this, makeConfig());

        auto alice = Account("alice");
        auto bob = Account("bob");

        env.fund(XRP(1000), noripple(alice));

//这些类型的检查是在别处测试的，但是
//这将验证txq将故障处理为
//预期。

//飞行前失败
        env(pay(alice, bob, XRP(-1000)),
            ter(temBAD_AMOUNT));

//预索赔失败
        env(noop(alice), fee(XRP(100000)),
            ter(terINSUF_FEE_B));
    }

    void testQueuedFailure()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "2" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");

        auto queued = ter(terQUEUED);

        checkMetrics(env, 0, boost::none, 0, 2, 256);

        env.fund(XRP(1000), noripple(alice, bob));

        checkMetrics(env, 0, boost::none, 2, 2, 256);

//填入分类帐
        env(noop(alice));
        checkMetrics(env, 0, boost::none, 3, 2, 256);

//将事务放入队列
        env(noop(alice), queued);
        checkMetrics(env, 1, boost::none, 3, 2, 256);

//现在作弊，绕过队列。
        {
            auto const& jt = env.jt(noop(alice));
            BEAST_EXPECT(jt.stx);

            bool didApply;
            TER ter;

            env.app().openLedger().modify(
                [&](OpenView& view, beast::Journal j)
                {
                    std::tie(ter, didApply) =
                        ripple::apply(env.app(),
                            view, *jt.stx, tapNONE,
                                env.journal);
                    return didApply;
                }
                );
            env.postconditions(jt, ter, didApply);
        }
        checkMetrics(env, 1, boost::none, 4, 2, 256);

        env.close();
//Alice的排队事务在txq:：accept中失败
//带tefpast_seq
        checkMetrics(env, 0, 8, 0, 4, 256);
    }

    void testMultiTxnPerAccount()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig(
                {{"minimum_txn_in_ledger_standalone", "3"}},
                {{"account_reserve", "200"}, {"owner_reserve", "50"}}));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");
        auto daria = Account("daria");

        auto queued = ter(terQUEUED);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 3, 256);

//由于MakeConfig，队列中的分类帐为2
        auto const initQueueMax = initFee(env, 3, 2, 10, 10, 200, 50);

//在费用低廉的情况下创建多个帐户，因此它们都适用。
        env.fund(drops(2000), noripple(alice));
        env.fund(XRP(500000), noripple(bob, charlie, daria));
        checkMetrics(env, 0, initQueueMax, 4, 3, 256);

//爱丽丝-价格开始暴涨：持仓
        env(noop(alice), queued);
        checkMetrics(env, 1, initQueueMax, 4, 3, 256);

        auto aliceSeq = env.seq(alice);
        auto bobSeq = env.seq(bob);
        auto charlieSeq = env.seq(charlie);

//Alice-尝试将第二个事务排队，但留下一个空白
        env(noop(alice), seq(aliceSeq + 2), fee(100),
            ter(terPRE_SEQ));
        checkMetrics(env, 1, initQueueMax, 4, 3, 256);

//Alice-将第二个事务排队。是的。
        env(noop(alice), seq(aliceSeq + 1), fee(13),
            queued);
        checkMetrics(env, 2, initQueueMax, 4, 3, 256);

//Alice-将第三个事务排队。是的。
        env(noop(alice), seq(aliceSeq + 2), fee(17),
            queued);
        checkMetrics(env, 3, initQueueMax, 4, 3, 256);

//Bob-排队处理事务
        env(noop(bob), queued);
        checkMetrics(env, 4, initQueueMax, 4, 3, 256);

//Bob-排队处理第二个事务
        env(noop(bob), seq(bobSeq + 1), fee(50),
            queued);
        checkMetrics(env, 5, initQueueMax, 4, 3, 256);

//查理-以更高的费用排队交易
//比违约
        env(noop(charlie), fee(15), queued);
        checkMetrics(env, 6, initQueueMax, 4, 3, 256);

        BEAST_EXPECT(env.seq(alice) == aliceSeq);
        BEAST_EXPECT(env.seq(bob) == bobSeq);
        BEAST_EXPECT(env.seq(charlie) == charlieSeq);

        env.close();
//验证除了一个排队事务之外的所有事务
//得到了应用。
        checkMetrics(env, 1, 8, 5, 4, 256);

//验证阻塞的事务是否是Bob的第二个事务。
//尽管收费比爱丽丝的高
//查理的，直到费用上升才有人尝试。
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 3);
        BEAST_EXPECT(env.seq(bob) == bobSeq + 1);
        BEAST_EXPECT(env.seq(charlie) == charlieSeq + 1);

//爱丽丝-排队
        std::int64_t aliceFee = 20;
        aliceSeq = env.seq(alice);
        auto lastLedgerSeq = env.current()->info().seq + 2;
        for (auto i = 0; i < 7; i++)
        {
            env(noop(alice), seq(aliceSeq),
                json(jss::LastLedgerSequence, lastLedgerSeq + i),
                    fee(aliceFee), queued);
            ++aliceSeq;
        }
        checkMetrics(env, 8, 8, 5, 4, 513);
        {
            auto& txQ = env.app().getTxQ();
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            std::int64_t fee = 20;
            auto seq = env.seq(alice);
            BEAST_EXPECT(aliceStat.size() == 7);
            for (auto const& tx : aliceStat)
            {
                BEAST_EXPECT(tx.first == seq);
                BEAST_EXPECT(tx.second.feeLevel == mulDiv(fee, 256, 10).second);
                BEAST_EXPECT(tx.second.lastValid);
                BEAST_EXPECT((tx.second.consequences &&
                    tx.second.consequences->fee == drops(fee) &&
                    tx.second.consequences->potentialSpend == drops(0) &&
                    tx.second.consequences->category == TxConsequences::normal) ||
                    tx.first == env.seq(alice) + 6);
                ++seq;
            }
        }

//Alice试图将另一个项目添加到队列中，
//但你不能强迫自己早点离开
//排队。
        env(noop(alice), seq(aliceSeq),
            json(jss::LastLedgerSequence, lastLedgerSeq + 7),
                fee(aliceFee), ter(telCAN_NOT_QUEUE_FULL));
        checkMetrics(env, 8, 8, 5, 4, 513);

//查理-尝试将另一个项目添加到队列中，
//因为费用比爱丽丝的低，所以失败了
//排队平均数。
        env(noop(charlie), fee(19), ter(telCAN_NOT_QUEUE_FULL));
        checkMetrics(env, 8, 8, 5, 4, 513);

//查理-在队列中添加另一个项目，
//使爱丽丝的最后一个txn下降
        env(noop(charlie), fee(30), queued);
        checkMetrics(env, 8, 8, 5, 4, 513);

//Alice-现在尝试再向队列中添加一个，
//最后一次发送失败，所以
//没有完整的链条。
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), ter(terPRE_SEQ));
        checkMetrics(env, 8, 8, 5, 4, 513);

//艾丽斯想要这个TX而不是掉下来的TX，
//所以重新提交的费用更高，但是队列
//是满的，她的账户是最便宜的。
        env(noop(alice), seq(aliceSeq - 1),
            fee(aliceFee), ter(telCAN_NOT_QUEUE_FULL));
        checkMetrics(env, 8, 8, 5, 4, 513);

//尝试替换队列中的中间项
//没有足够的费用。
        aliceSeq = env.seq(alice) + 2;
        aliceFee = 25;
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), ter(telCAN_NOT_QUEUE_FEE));
        checkMetrics(env, 8, 8, 5, 4, 513);

//成功替换队列中的中间项
        ++aliceFee;
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), queued);
        checkMetrics(env, 8, 8, 5, 4, 513);

        env.close();
//爱丽丝的交易处理，连同
//查理的，丢失的一个被重播
//添加回队列。
        checkMetrics(env, 4, 10, 6, 5, 256);

        aliceSeq = env.seq(alice) + 1;

//尝试用将
//破产的爱丽丝。失败，因为帐户不能
//超过最低飞行储备
//上次排队的事务
        aliceFee = env.le(alice)->getFieldAmount(sfBalance).xrp().drops()
            - (59);
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), ter(telCAN_NOT_QUEUE_BALANCE));
        checkMetrics(env, 4, 10, 6, 5, 256);

//尽量多花钱，爱丽丝负担不起与所有其他TXS。
        aliceSeq += 2;
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), ter(terINSUF_FEE_B));
        checkMetrics(env, 4, 10, 6, 5, 256);

//用将
//破产的爱丽丝
        --aliceFee;
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), queued);
        checkMetrics(env, 4, 10, 6, 5, 256);

//Alice-尝试对最后一个事务进行排队，但它
//因为飞行费用太高而失败
//费用与余额核对
        aliceFee /= 5;
        ++aliceSeq;
        env(noop(alice), seq(aliceSeq),
            fee(aliceFee), ter(telCAN_NOT_QUEUE_BALANCE));
        checkMetrics(env, 4, 10, 6, 5, 256);

        env.close();
//所有爱丽丝的交易都适用。
        checkMetrics(env, 0, 12, 4, 6, 256);

        env.close();
        checkMetrics(env, 0, 12, 0, 6, 256);

//爱丽丝破产了
        env.require(balance(alice, XRP(0)));
        env(noop(alice), ter(terINSUF_FEE_B));

//鲍勃想排队比单曲多
//账户限额（10）TXS。
        fillQueue(env, bob);
        bobSeq = env.seq(bob);
        checkMetrics(env, 0, 12, 7, 6, 256);
        for (int i = 0; i < 10; ++i)
            env(noop(bob), seq(bobSeq + i), queued);
        checkMetrics(env, 10, 12, 7, 6, 256);
//鲍勃达到了单账户限额
        env(noop(bob), seq(bobSeq + 10), ter(terPRE_SEQ));
        checkMetrics(env, 10, 12, 7, 6, 256);
//Bob可以取代早期的TXS
//极限的
        env(noop(bob), seq(bobSeq + 5), fee(20), queued);
        checkMetrics(env, 10, 12, 7, 6, 256);
    }

    void testTieBreaking()
    {
        using namespace jtx;
        using namespace std::chrono;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "4" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");
        auto daria = Account("daria");
        auto elmo = Account("elmo");
        auto fred = Account("fred");
        auto gwen = Account("gwen");
        auto hank = Account("hank");

        auto queued = ter(terQUEUED);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 4, 256);

//在费用低廉的情况下创建多个帐户，因此它们都适用。
        env.fund(XRP(50000), noripple(alice, bob, charlie, daria));
        checkMetrics(env, 0, boost::none, 4, 4, 256);

        env.close();
        checkMetrics(env, 0, 8, 0, 4, 256);

        env.fund(XRP(50000), noripple(elmo, fred, gwen, hank));
        checkMetrics(env, 0, 8, 4, 4, 256);

        env.close();
        checkMetrics(env, 0, 8, 0, 4, 256);

//////////////////////////////////////////

//把分类帐和队列填起来，这样我们就可以核实
//东西被踢出去了。
        env(noop(gwen));
        env(noop(hank));
        env(noop(gwen));
        env(noop(fred));
        env(noop(elmo));
        checkMetrics(env, 0, 8, 5, 4, 256);

        auto aliceSeq = env.seq(alice);
        auto bobSeq = env.seq(bob);
        auto charlieSeq = env.seq(charlie);
        auto dariaSeq = env.seq(daria);
        auto elmoSeq = env.seq(elmo);
        auto fredSeq = env.seq(fred);
        auto gwenSeq = env.seq(gwen);
        auto hankSeq = env.seq(hank);

//这次，使用相同的费用。
        env(noop(alice), fee(15), queued);
        env(noop(bob), fee(15), queued);
        env(noop(charlie), fee(15), queued);
        env(noop(daria), fee(15), queued);
        env(noop(elmo), fee(15), queued);
        env(noop(fred), fee(15), queued);
        env(noop(gwen), fee(15), queued);
//这一个进入队列，但当
//以后再加一笔更高的费用。
        env(noop(hank), fee(15), queued);

//队列已满。最低费用现在反映了
//队列中的最低费用。
        checkMetrics(env, 8, 8, 5, 4, 385);

//尝试添加另一个具有默认（低）费用的交易，
//它应该会失败，因为它已经不能取代它了
//那里。
        env(noop(charlie), ter(telCAN_NOT_QUEUE_FEE));

//增加另一笔交易，费用更高，
//不高到可以进入分类帐，但是高
//足以进入队列（并将某人踢出）
        env(noop(charlie), fee(100), seq(charlieSeq + 1), queued);

//队列仍然满。
        checkMetrics(env, 8, 8, 5, 4, 385);

//爱丽丝、鲍勃、查理、达里亚和埃尔莫的TXS
//从队列中处理到分类帐中，
//离开弗雷德和格温的TXS。汉克的TX是
//从localtx重试，并将其放回
//排队。
        env.close();
        checkMetrics(env, 3, 10, 6, 5, 256);

        BEAST_EXPECT(aliceSeq + 1 == env.seq(alice));
        BEAST_EXPECT(bobSeq + 1 == env.seq(bob));
        BEAST_EXPECT(charlieSeq + 2 == env.seq(charlie));
        BEAST_EXPECT(dariaSeq + 1 == env.seq(daria));
        BEAST_EXPECT(elmoSeq + 1 == env.seq(elmo));
        BEAST_EXPECT(fredSeq == env.seq(fred));
        BEAST_EXPECT(gwenSeq == env.seq(gwen));
        BEAST_EXPECT(hankSeq == env.seq(hank));

        aliceSeq = env.seq(alice);
        bobSeq = env.seq(bob);
        charlieSeq = env.seq(charlie);
        dariaSeq = env.seq(daria);
        elmoSeq = env.seq(elmo);

//再次填满队列
        env(noop(alice), fee(15), queued);
        env(noop(alice), seq(aliceSeq + 1), fee(15), queued);
        env(noop(alice), seq(aliceSeq + 2), fee(15), queued);
        env(noop(bob), fee(15), queued);
        env(noop(charlie), fee(15), queued);
        env(noop(daria), fee(15), queued);
//这一个进入队列，但当
//以后再加一笔更高的费用。
        env(noop(elmo), fee(15), queued);
        checkMetrics(env, 10, 10, 6, 5, 385);

//增加另一笔交易，费用更高，
//不高到可以进入分类帐，但是高
//足以进入队列（并将某人踢出）
        env(noop(alice), fee(100), seq(aliceSeq + 3), queued);

        env.close();
        checkMetrics(env, 4, 12, 7, 6, 256);

        BEAST_EXPECT(fredSeq + 1 == env.seq(fred));
        BEAST_EXPECT(gwenSeq + 1 == env.seq(gwen));
        BEAST_EXPECT(hankSeq + 1 == env.seq(hank));
        BEAST_EXPECT(aliceSeq + 4 == env.seq(alice));
        BEAST_EXPECT(bobSeq == env.seq(bob));
        BEAST_EXPECT(charlieSeq == env.seq(charlie));
        BEAST_EXPECT(dariaSeq == env.seq(daria));
        BEAST_EXPECT(elmoSeq == env.seq(elmo));
    }

    void testAcctTxnID()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "1" } }));

        auto alice = Account("alice");

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 1, 256);

        env.fund(XRP(50000), noripple(alice));
        checkMetrics(env, 0, boost::none, 1, 1, 256);

        env(fset(alice, asfAccountTxnID));
        checkMetrics(env, 0, boost::none, 2, 1, 256);

//在fset之后，sfaccounttxnid字段
//仍然没有初始化，所以飞行前在这里成功，
//这个txn失败，因为它不能存储在队列中。
        env(noop(alice), json(R"({"AccountTxnID": "0"})"),
            ter(telCAN_NOT_QUEUE));

        checkMetrics(env, 0, boost::none, 2, 1, 256);
        env.close();
//从localtx重试失败的事务
//成功了。
        checkMetrics(env, 0, 4, 1, 2, 256);

        env(noop(alice));
        checkMetrics(env, 0, 4, 2, 2, 256);

        env(noop(alice), json(R"({"AccountTxnID": "0"})"),
            ter(tefWRONG_PRIOR));
    }

    void testMaximum()
    {
        using namespace jtx;

        Env env(*this, makeConfig(
            { {"minimum_txn_in_ledger_standalone", "2"},
                {"target_txn_in_ledger", "4"},
                    {"maximum_txn_in_ledger", "5"} }));

        auto alice = Account("alice");

        checkMetrics(env, 0, boost::none, 0, 2, 256);

        env.fund(XRP(50000), noripple(alice));
        checkMetrics(env, 0, boost::none, 1, 2, 256);

        for (int i = 0; i < 10; ++i)
            env(noop(alice), openLedgerFee(env));

        checkMetrics(env, 0, boost::none, 11, 2, 256);

        env.close();
//如果不是最大值，每个分类账将是11。
        checkMetrics(env, 0, 10, 0, 5, 256, 800025);

    }

    void testUnexpectedBalanceChange()
    {
        using namespace jtx;

        Env env(
            *this,
            makeConfig(
                {{"minimum_txn_in_ledger_standalone", "3"}},
                {{"account_reserve", "200"}, {"owner_reserve", "50"}}));

        auto alice = Account("alice");
        auto bob = Account("bob");

        auto queued = ter(terQUEUED);

//由于MakeConfig，队列中的分类帐为2
        auto const initQueueMax = initFee(env, 3, 2, 10, 10, 200, 50);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, initQueueMax, 0, 3, 256);

        env.fund(drops(5000), noripple(alice));
        env.fund(XRP(50000), noripple(bob));
        checkMetrics(env, 0, initQueueMax, 2, 3, 256);
        auto USD = bob["USD"];

        env(offer(alice, USD(5000), drops(5000)), require(owners(alice, 1)));
        checkMetrics(env, 0, initQueueMax, 3, 3, 256);

        env.close();
        checkMetrics(env, 0, 6, 0, 3, 256);

//填写分类帐
        fillQueue(env, alice);
        checkMetrics(env, 0, 6, 4, 3, 256);

//排队处理几个事务，再加上一个事务
//更贵的一个。
        auto aliceSeq = env.seq(alice);
        env(noop(alice), seq(aliceSeq++), queued);
        env(noop(alice), seq(aliceSeq++), queued);
        env(noop(alice), seq(aliceSeq++), queued);
        env(noop(alice), fee(drops(1000)),
            seq(aliceSeq), queued);
        checkMetrics(env, 4, 6, 4, 3, 256);

//这个提议应该接受爱丽丝的提议
//到爱丽丝的储备为止。
        env(offer(bob, drops(5000), USD(5000)),
            openLedgerFee(env), require(balance(alice, drops(250)),
                owners(alice, 1), lines(alice, 1)));
        checkMetrics(env, 4, 6, 5, 3, 256);

//尝试添加新事务。
//飞行费用太多。
        env(noop(alice), fee(drops(200)), seq(aliceSeq+1),
            ter(telCAN_NOT_QUEUE_BALANCE));
        checkMetrics(env, 4, 6, 5, 3, 256);

//关闭分类帐。爱丽丝所有的交易
//收取一笔费用，最后一笔除外。
        env.close();
        checkMetrics(env, 1, 10, 3, 5, 256);
        env.require(balance(alice, drops(250 - 30)));

//仍然无法为Alice添加新事务，
//不管费用是多少。
        env(noop(alice), fee(drops(200)), seq(aliceSeq + 1),
            ter(telCAN_NOT_QUEUE_BALANCE));
        checkMetrics(env, 1, 10, 3, 5, 256);

        /*此时，Alice的交易是无限期的
            卡在队列中。最终它也会
            过期，被更值钱的东西逼到最后
            事务，由Alice或Alice替换
            将得到更多的xrp，它将处理。
        **/


        for (int i = 0; i < 9; ++i)
        {
            env.close();
            checkMetrics(env, 1, 10, 0, 5, 256);
        }

//Alice的事务将过期（通过重试限制，
//不是lastledgersequence）。
        env.close();
        checkMetrics(env, 0, 10, 0, 5, 256);
    }

    void testBlockers()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } }));

        auto alice = Account("alice");
        auto bob = Account("bob");
        auto charlie = Account("charlie");
        auto daria = Account("daria");

        auto queued = ter(terQUEUED);

        BEAST_EXPECT(env.current()->fees().base == 10);

        checkMetrics(env, 0, boost::none, 0, 3, 256);

        env.fund(XRP(50000), noripple(alice, bob));
        env.memoize(charlie);
        env.memoize(daria);
        checkMetrics(env, 0, boost::none, 2, 3, 256);

//填写未结分类帐
        env(noop(alice));
//设置一个常规的密钥，以清除密码已用标志
        env(regkey(alice, charlie));
        checkMetrics(env, 0, boost::none, 4, 3, 256);

//在队列中放置一些“正常”tx
        auto aliceSeq = env.seq(alice);
        env(noop(alice), queued);
        env(noop(alice), seq(aliceSeq + 1), queued);
        env(noop(alice), seq(aliceSeq + 2), queued);

//无法用阻止程序替换第一个Tx
        env(fset(alice, asfAccountTxnID), fee(20), ter(telCAN_NOT_QUEUE_BLOCKS));
//无法用阻止程序替换第二/中间Tx
        env(regkey(alice, bob), seq(aliceSeq + 1), fee(20),
            ter(telCAN_NOT_QUEUE_BLOCKS));
        env(signers(alice, 2, { {bob}, {charlie}, {daria} }), fee(20),
            seq(aliceSeq + 1), ter(telCAN_NOT_QUEUE_BLOCKS));
//可以用拦截器替换上一个Tx
        env(signers(alice, 2, { { bob },{ charlie },{ daria } }), fee(20),
            seq(aliceSeq + 2), queued);
        env(regkey(alice, bob), seq(aliceSeq + 2), fee(30),
            queued);

//阻止程序之后不能再排队处理任何事务
        env(noop(alice), seq(aliceSeq + 3), ter(telCAN_NOT_QUEUE_BLOCKED));

//其他账户不受影响
        env(noop(bob), queued);

//可以在拦截器之前更换TXS
        env(noop(alice), fee(14), queued);

//可自行更换阻滞剂
        env(noop(alice), seq(aliceSeq + 2), fee(40), queued);

//现在没有街区了。
        env(noop(alice), seq(aliceSeq + 3), queued);
    }

    void testInFlightBalance()
    {
        using namespace jtx;
        testcase("In-flight balance checks");

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } },
            {{"account_reserve", "200"}, {"owner_reserve", "50"}}));

        auto alice = Account("alice");
        auto charlie = Account("charlie");
        auto gw = Account("gw");

        auto queued = ter(terQUEUED);

//将费用准备金设为“非常低”，以便进行收费交易。
//在球场上，预备队可以排队。默认情况下
//储备，几百笔交易必须
//在未结分类帐费用接近储备金之前排队，
//这会不必要地减慢测试速度。
//由于MakeConfig，队列中的分类帐为2
        auto const initQueueMax = initFee(env, 3, 2, 10, 10, 200, 50);

        auto limit = 3;

        checkMetrics(env, 0, initQueueMax, 0, limit, 256);

        env.fund(XRP(50000), noripple(alice, charlie), gw);
        checkMetrics(env, 0, initQueueMax, limit + 1, limit, 256);

        auto USD = gw["USD"];
        auto BUX = gw["BUX"];

///////////////////////////////////
//提供高xrp输出和低费用不会阻止
        auto aliceSeq = env.seq(alice);
        auto aliceBal = env.balance(alice);

        env.require(balance(alice, XRP(50000)),
            owners(alice, 0));

//如果这个提议通过，所有爱丽丝的
//将采用XRP（保留除外）。
        env(offer(alice, BUX(5000), XRP(50000)),
            queued);
        checkMetrics(env, 1, initQueueMax, limit + 1, limit, 256);

//但由于保护区受到保护，另一个
//将允许事务排队
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, initQueueMax, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit*2, 2, limit, 256);

//但是一旦我们关闭分类帐，我们就会发现爱丽丝
//有大量的xrp，因为报价没有
//交叉（当然）。
        env.require(balance(alice, aliceBal - drops(20)),
            owners(alice, 1));
//取消报价
        env(offer_cancel(alice, aliceSeq));

///////////////////////////////////
//提供高xrp输出和高总费用块稍后txs
        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);
        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);

        env.require(owners(alice, 0));

//艾丽斯提出了一个底价的一半
        env(offer(alice, BUX(5000), XRP(50000)), fee(drops(100)),
            queued);
        checkMetrics(env, 1, limit * 2, limit + 1, limit, 256);

//艾丽斯提出了另一个收费的方案
//这使总数略低于储备金。
        env(noop(alice), fee(drops(99)), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

//所以即使是一个noop也会像alice
//没有余额支付费用
        env(noop(alice), fee(drops(51)), seq(aliceSeq + 2),
            ter(terINSUF_FEE_B));
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 3, limit, 256);

//但是一旦我们关闭分类帐，我们就会发现爱丽丝
//有大量的xrp，因为报价没有
//交叉（当然）。
        env.require(balance(alice, aliceBal - drops(250)),
            owners(alice, 1));
//取消报价
        env(offer_cancel(alice, aliceSeq));

///////////////////////////////////
//提供高xrp输出和超高费用块后TXS
        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);
        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);

        env.require(owners(alice, 0));

//艾丽斯提出了一个比底价更高的报价
//这个可以排队，因为它是爱丽丝排队的第一个
        env(offer(alice, BUX(5000), XRP(50000)), fee(drops(300)),
            queued);
        checkMetrics(env, 1, limit * 2, limit + 1, limit, 256);

//所以即使是一个noop也会像alice
//没有余额支付费用
        env(noop(alice), fee(drops(51)), seq(aliceSeq + 1),
            ter(telCAN_NOT_QUEUE_BALANCE));
        checkMetrics(env, 1, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//但是一旦我们关闭分类帐，我们就会发现爱丽丝
//有大量的xrp，因为报价没有
//交叉（当然）。
        env.require(balance(alice, aliceBal - drops(351)),
            owners(alice, 1));
//取消报价
        env(offer_cancel(alice, aliceSeq));

///////////////////////////////////
//提供低xrp输出允许以后的txs
        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);
        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);

//如果这个提议通过，只要一点点
//爱丽丝的xrp将被采用。
        env(offer(alice, BUX(50), XRP(500)),
            queued);

//以后的交易还可以
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//但是一旦我们关闭分类帐，我们就会发现爱丽丝
//有大量的xrp，因为报价没有
//交叉（当然）。
        env.require(balance(alice, aliceBal - drops(20)),
            owners(alice, 1));
//取消报价
        env(offer_cancel(alice, aliceSeq));

///////////////////////////////////
//大额xrp付款不会阻止以后的txs
        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);

//如果付款成功，爱丽丝会
//把她的全部余额寄给查理
//（减去准备金）。
        env(pay(alice, charlie, XRP(50000)),
            queued);

//但由于保护区受到保护，另一个
//将允许事务排队
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//但是一旦我们关闭分类帐，我们就会发现爱丽丝
//她的平衡仍然很大，因为
//付款没有资金！
        env.require(balance(alice, aliceBal - drops(20)),
            owners(alice, 0));

///////////////////////////////////
//小额xrp付款允许以后的txs
        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);

//如果付款成功，爱丽丝会
//把一点余额寄给查理
        env(pay(alice, charlie, XRP(500)),
            queued);

//以后的交易还可以
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//付款成功
        env.require(balance(alice, aliceBal - XRP(500) - drops(20)),
            owners(alice, 0));

///////////////////////////////////
//大额借据支付允许以后的TXS
        auto const amount = USD(500000);
        env(trust(alice, USD(50000000)));
        env(trust(charlie, USD(50000000)));
        checkMetrics(env, 0, limit * 2, 4, limit, 256);
//关闭，这样我们就不必处理
//与Tx协商一致。
        env.close();

        env(pay(gw, alice, amount));
        checkMetrics(env, 0, limit * 2, 1, limit, 256);
//关闭，这样我们就不必处理
//与Tx协商一致。
        env.close();

        fillQueue(env, alice);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);
        auto aliceUSD = env.balance(alice, USD);

//如果付款成功，爱丽丝会
//把她所有的美元余额寄给查理。
        env(pay(alice, charlie, amount),
            queued);

//但那没关系，因为它不影响
//爱丽丝的xrp余额（当然不是费用）。
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//所以一旦我们关了账本，爱丽丝就把她带走了
//XRP余额，但她的美元余额归查理所有。
        env.require(balance(alice, aliceBal - drops(20)),
            balance(alice, USD(0)),
            balance(charlie, aliceUSD),
            owners(alice, 1),
            owners(charlie, 1));

///////////////////////////////////
//大额xrp-to-iou付款不会阻止以后的txs。

        env(offer(gw, XRP(500000), USD(50000)));
//关闭，这样我们就不必处理
//与Tx协商一致。
        env.close();

        fillQueue(env, charlie);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);
        auto charlieUSD = env.balance(charlie, USD);

//如果此付款成功，并使用
//整个sendmax，爱丽丝会把她
//整个xrp余额到charlie
//美元形式。
        BEAST_EXPECT(XRP(60000) > aliceBal);
        env(pay(alice, charlie, USD(1000)),
            sendmax(XRP(60000)), queued);

//但由于保护区受到保护，另一个
//将允许事务排队
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//所以一旦我们关闭了分类帐，爱丽丝就发了一笔款项
//对查理来说，她只使用了部分xrp余额
        env.require(balance(alice, aliceBal - XRP(10000) - drops(20)),
            balance(alice, USD(0)),
            balance(charlie, charlieUSD + USD(1000)),
            owners(alice, 1),
            owners(charlie, 1));

///////////////////////////////////
//小额xrp到iou付款允许以后的txs。

        fillQueue(env, charlie);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);
        charlieUSD = env.balance(charlie, USD);

//如果此付款成功，并使用
//整个sendmax，alice只发送
//她给查理的部分xrp余额
//美元。
        BEAST_EXPECT(aliceBal > XRP(6001));
        env(pay(alice, charlie, USD(500)),
            sendmax(XRP(6000)), queued);

//以后的交易还可以
        env(noop(alice), seq(aliceSeq + 1), queued);
        checkMetrics(env, 2, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 2, limit, 256);

//所以一旦我们关闭了分类帐，爱丽丝就发了一笔款项
//对查理来说，她只使用了部分xrp余额
        env.require(balance(alice, aliceBal - XRP(5000) - drops(20)),
            balance(alice, USD(0)),
            balance(charlie, charlieUSD + USD(500)),
            owners(alice, 1),
            owners(charlie, 1));

///////////////////////////////////
//边缘案例：如果余额低于准备金，会发生什么？
        env(noop(alice), fee(env.balance(alice) - drops(30)));
        env.close();

        fillQueue(env, charlie);
        checkMetrics(env, 0, limit * 2, limit + 1, limit, 256);

        aliceSeq = env.seq(alice);
        aliceBal = env.balance(alice);
        BEAST_EXPECT(aliceBal == drops(30));

        env(noop(alice), fee(drops(25)), queued);
        env(noop(alice), seq(aliceSeq + 1), ter(terINSUF_FEE_B));
        BEAST_EXPECT(env.balance(alice) == drops(30));

        checkMetrics(env, 1, limit * 2, limit + 1, limit, 256);

        env.close();
        ++limit;
        checkMetrics(env, 0, limit * 2, 1, limit, 256);
        BEAST_EXPECT(env.balance(alice) == drops(5));

    }

    void testConsequences()
    {
        using namespace jtx;
        using namespace std::chrono;
        Env env(*this, supported_amendments().set(featureTickets));
        auto const alice = Account("alice");
        env.memoize(alice);
        env.memoize("bob");
        env.memoize("carol");
        {
            Json::Value cancelOffer;
            cancelOffer[jss::Account] = alice.human();
            cancelOffer[jss::OfferSequence] = 3;
            cancelOffer[jss::TransactionType] = "OfferCancel";
            auto const jtx = env.jt(cancelOffer,
                seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }

        {
            auto USD = alice["USD"];

            auto const jtx = env.jt(trust("carol", USD(50000000)),
                seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }

        {
            auto const jtx = env.jt(ticket::create(alice, "bob", 60),
                seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }

        {
            Json::Value cancelTicket;
            cancelTicket[jss::Account] = alice.human();
            cancelTicket["TicketID"] = to_string(uint256());
            cancelTicket[jss::TransactionType] = "TicketCancel";
            auto const jtx = env.jt(cancelTicket,
                seq(1), fee(10));
            auto const pf = preflight(env.app(), env.current()->rules(),
                *jtx.stx, tapNONE, env.journal);
            BEAST_EXPECT(pf.ter == tesSUCCESS);
            auto const conseq = calculateConsequences(pf);
            BEAST_EXPECT(conseq.category == TxConsequences::normal);
            BEAST_EXPECT(conseq.fee == drops(10));
            BEAST_EXPECT(conseq.potentialSpend == XRP(0));
        }
    }

    void testRPC()
    {
        using namespace jtx;
        Env env(*this);

        auto fee = env.rpc("fee");

        if (BEAST_EXPECT(fee.isMember(jss::result)) &&
            BEAST_EXPECT(!RPC::contains_error(fee[jss::result])))
        {
            auto const& result = fee[jss::result];
            BEAST_EXPECT(result.isMember(jss::ledger_current_index)
                && result[jss::ledger_current_index] == 3);
            BEAST_EXPECT(result.isMember(jss::current_ledger_size));
            BEAST_EXPECT(result.isMember(jss::current_queue_size));
            BEAST_EXPECT(result.isMember(jss::expected_ledger_size));
            BEAST_EXPECT(!result.isMember(jss::max_queue_size));
            BEAST_EXPECT(result.isMember(jss::drops));
            auto const& drops = result[jss::drops];
            BEAST_EXPECT(drops.isMember(jss::base_fee));
            BEAST_EXPECT(drops.isMember(jss::median_fee));
            BEAST_EXPECT(drops.isMember(jss::minimum_fee));
            BEAST_EXPECT(drops.isMember(jss::open_ledger_fee));
            BEAST_EXPECT(result.isMember(jss::levels));
            auto const& levels = result[jss::levels];
            BEAST_EXPECT(levels.isMember(jss::median_level));
            BEAST_EXPECT(levels.isMember(jss::minimum_level));
            BEAST_EXPECT(levels.isMember(jss::open_ledger_level));
            BEAST_EXPECT(levels.isMember(jss::reference_level));
        }

        env.close();

        fee = env.rpc("fee");

        if (BEAST_EXPECT(fee.isMember(jss::result)) &&
            BEAST_EXPECT(!RPC::contains_error(fee[jss::result])))
        {
            auto const& result = fee[jss::result];
            BEAST_EXPECT(result.isMember(jss::ledger_current_index)
                && result[jss::ledger_current_index] == 4);
            BEAST_EXPECT(result.isMember(jss::current_ledger_size));
            BEAST_EXPECT(result.isMember(jss::current_queue_size));
            BEAST_EXPECT(result.isMember(jss::expected_ledger_size));
            BEAST_EXPECT(result.isMember(jss::max_queue_size));
            auto const& drops = result[jss::drops];
            BEAST_EXPECT(drops.isMember(jss::base_fee));
            BEAST_EXPECT(drops.isMember(jss::median_fee));
            BEAST_EXPECT(drops.isMember(jss::minimum_fee));
            BEAST_EXPECT(drops.isMember(jss::open_ledger_fee));
            BEAST_EXPECT(result.isMember(jss::levels));
            auto const& levels = result[jss::levels];
            BEAST_EXPECT(levels.isMember(jss::median_level));
            BEAST_EXPECT(levels.isMember(jss::minimum_level));
            BEAST_EXPECT(levels.isMember(jss::open_ledger_level));
            BEAST_EXPECT(levels.isMember(jss::reference_level));
        }
    }

    void testExpirationReplacement()
    {
        /*此测试基于报告的回归，其中
            替换候选事务找到它尝试的Tx
            替换没有设置“results”

            假设：队列从22到25。在某些时候，
            原来的'22'和'23都已过期，并已从
            排队。提交了第二个'22，以及多Tx逻辑
            没有启动，因为它符合帐户的顺序
            数字（a_seq==t_seq）。第三个'22提交并发现
            队列中的'22没有结果。
        **/

        using namespace jtx;

        Env env(*this, makeConfig({ { "minimum_txn_in_ledger_standalone", "1" },
            {"ledgers_in_queue", "10"}, {"maximum_txn_per_account", "20"} }));

//爱丽丝将重新创造这个场景。鲍伯将阻止。
        auto const alice = Account("alice");
        auto const bob = Account("bob");

        env.fund(XRP(500000), noripple(alice, bob));
        checkMetrics(env, 0, boost::none, 2, 1, 256);

        auto const aliceSeq = env.seq(alice);
        BEAST_EXPECT(env.current()->info().seq == 3);
        env(noop(alice), seq(aliceSeq), json(R"({"LastLedgerSequence":5})"), ter(terQUEUED));
        env(noop(alice), seq(aliceSeq + 1), json(R"({"LastLedgerSequence":5})"), ter(terQUEUED));
        env(noop(alice), seq(aliceSeq + 2), json(R"({"LastLedgerSequence":10})"), ter(terQUEUED));
        env(noop(alice), seq(aliceSeq + 3), json(R"({"LastLedgerSequence":11})"), ter(terQUEUED));
        checkMetrics(env, 4, boost::none, 2, 1, 256);
        auto const bobSeq = env.seq(bob);
//分类帐4得到3，
//分类帐5得到4，
//分类帐6得到5。
        for (int i = 0; i < 3 + 4 + 5; ++i)
        {
            env(noop(bob), seq(bobSeq + i), fee(200), ter(terQUEUED));
        }
        checkMetrics(env, 4 + 3 + 4 + 5, boost::none, 2, 1, 256);
//关闭分类帐3
        env.close();
        checkMetrics(env, 4 + 4 + 5, 20, 3, 2, 256);
//关闭分类帐4
        env.close();
        checkMetrics(env, 4 + 5, 30, 4, 3, 256);
//关闭分类帐5
        env.close();
//爱丽丝的头两个TXS过期了。
        checkMetrics(env, 2, 40, 5, 4, 256);

//由于缺少AliceSeq，AliceSeq+1失败
        env(noop(alice), seq(aliceSeq + 1), ter(terPRE_SEQ));

//排队等待新的AliceSeq TX。
//这只会对
//提高孤立的TXS
//恢复。因为转播后期TXS的成本
//已经支付，此tx可能是
//阻断剂
        env(fset(alice, asfAccountTxnID), seq(aliceSeq), ter(terQUEUED));
        checkMetrics(env, 3, 40, 5, 4, 256);

//即使没有计算后果，我们也可以替换它。
        env(noop(alice), seq(aliceSeq), fee(20), ter(terQUEUED));
        checkMetrics(env, 3, 40, 5, 4, 256);

//排队等待新的AliceSeq+1Tx。
//这个TX也只做一些多任务验证。
        env(fset(alice, asfAccountTxnID), seq(aliceSeq + 1), ter(terQUEUED));
        checkMetrics(env, 4, 40, 5, 4, 256);

//即使没有计算后果，我们也可以替换它，
//也是。
        env(noop(alice), seq(aliceSeq +1), fee(20), ter(terQUEUED));
        checkMetrics(env, 4, 40, 5, 4, 256);

//关闭分类帐6
        env.close();
//我们希望所有爱丽丝排队的德克萨斯州都能进入
//未结分类帐。
        checkMetrics(env, 0, 50, 4, 5, 256);
        BEAST_EXPECT(env.seq(alice) == aliceSeq + 4);
    }

    void testSignAndSubmitSequence()
    {
        testcase("Autofilled sequence should account for TxQ");
        using namespace jtx;
        Env env(*this,
            makeConfig({ {"minimum_txn_in_ledger_standalone", "6"} }));
        Env_ss envs(env);
        auto const& txQ = env.app().getTxQ();

        auto const alice = Account("alice");
        auto const bob = Account("bob");
        env.fund(XRP(100000), alice, bob);

        fillQueue(env, alice);
        checkMetrics(env, 0, boost::none, 7, 6, 256);

//为Alice签名和提交排队几个事务
        auto const aliceSeq = env.seq(alice);
        auto const lastLedgerSeq = env.current()->info().seq + 2;

        auto submitParams = Json::Value(Json::objectValue);
        for (int i = 0; i < 5; ++i)
        {
            if (i == 2)
                envs(noop(alice), fee(1000), seq(none),
                    json(jss::LastLedgerSequence, lastLedgerSeq),
                        ter(terQUEUED))(submitParams);
            else
                envs(noop(alice), fee(1000), seq(none),
                    ter(terQUEUED))(submitParams);
        }
        checkMetrics(env, 5, boost::none, 7, 6, 256);
        {
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            auto seq = aliceSeq;
            BEAST_EXPECT(aliceStat.size() == 5);
            for (auto const& tx : aliceStat)
            {
                BEAST_EXPECT(tx.first == seq);
                BEAST_EXPECT(tx.second.feeLevel == 25600);
                if(seq == aliceSeq + 2)
                {
                    BEAST_EXPECT(tx.second.lastValid &&
                        *tx.second.lastValid == lastLedgerSeq);
                }
                else
                {
                    BEAST_EXPECT(!tx.second.lastValid);
                }
                ++seq;
            }
        }
//给鲍勃排队买些热膨胀剂。
//给他们更高的费用，这样他们就能打败爱丽丝。
        for (int i = 0; i < 8; ++i)
            envs(noop(bob), fee(2000), seq(none), ter(terQUEUED))();
        checkMetrics(env, 13, boost::none, 7, 6, 256);

        env.close();
        checkMetrics(env, 5, 14, 8, 7, 256);
//再给鲍勃排队买些热膨胀剂。
//给他们更高的费用，这样他们就能打败爱丽丝。
        fillQueue(env, bob);
        for(int i = 0; i < 9; ++i)
            envs(noop(bob), fee(2000), seq(none), ter(terQUEUED))();
        checkMetrics(env, 14, 14, 8, 7, 25601);
        env.close();
//再给鲍勃排队买些热膨胀剂。
//给他们更高的费用，这样他们就能打败爱丽丝。
        fillQueue(env, bob);
        for (int i = 0; i < 10; ++i)
            envs(noop(bob), fee(2000), seq(none), ter(terQUEUED))();
        checkMetrics(env, 15, 16, 9, 8, 256);
        env.close();
        checkMetrics(env, 4, 18, 10, 9, 256);
        {
//鲍勃一无所有。
            auto bobStat = txQ.getAccountTxs(bob.id(), *env.current());
            BEAST_EXPECT(bobStat.empty());
        }
//验证Alice的Tx是否如我们所期望的那样被丢弃，并且
//她排队的txs有点空白。
        {
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            auto seq = aliceSeq;
            BEAST_EXPECT(aliceStat.size() == 4);
            for (auto const& tx : aliceStat)
            {
//跳过丢失的那个。
                if (seq == aliceSeq + 2)
                    ++seq;

                BEAST_EXPECT(tx.first == seq);
                BEAST_EXPECT(tx.second.feeLevel == 25600);
                BEAST_EXPECT(!tx.second.lastValid);
                ++seq;
            }
        }
//现在，填补这个空白。
        envs(noop(alice), fee(1000), seq(none), ter(terQUEUED))(submitParams);
        checkMetrics(env, 5, 18, 10, 9, 256);
        {
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            auto seq = aliceSeq;
            BEAST_EXPECT(aliceStat.size() == 5);
            for (auto const& tx : aliceStat)
            {
                BEAST_EXPECT(tx.first == seq);
                BEAST_EXPECT(tx.second.feeLevel == 25600);
                BEAST_EXPECT(!tx.second.lastValid);
                ++seq;
            }
        }

        env.close();
        checkMetrics(env, 0, 20, 5, 10, 256);
        {
//鲍勃的资料已经整理好了。
            auto bobStat = txQ.getAccountTxs(bob.id(), *env.current());
            BEAST_EXPECT(bobStat.empty());
        }
        {
            auto aliceStat = txQ.getAccountTxs(alice.id(), *env.current());
            BEAST_EXPECT(aliceStat.empty());
        }
    }

    void testAccountInfo()
    {
        using namespace jtx;
        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } }));
        Env_ss envs(env);

        Account const alice{ "alice" };
        env.fund(XRP(1000000), alice);
        env.close();

        auto const withQueue =
            R"({ "account": ")" + alice.human() +
            R"(", "queue": true })";
        auto const withoutQueue =
            R"({ "account": ")" + alice.human() +
            R"("})";
        auto const prevLedgerWithQueue =
            R"({ "account": ")" + alice.human() +
            R"(", "queue": true, "ledger_index": 3 })";
        BEAST_EXPECT(env.current()->info().seq > 3);

        {
//没有“queue”参数的帐户信息。
            auto const info = env.rpc("json", "account_info", withoutQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            BEAST_EXPECT(!info[jss::result].isMember(jss::queue_data));
        }
        {
//带有“queue”参数的帐户信息。
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 0);
            BEAST_EXPECT(!queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(!queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(!queue_data.isMember(jss::transactions));
        }
        checkMetrics(env, 0, 6, 0, 3, 256);

        fillQueue(env, alice);
        checkMetrics(env, 0, 6, 4, 3, 256);

        {
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 0);
            BEAST_EXPECT(!queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(!queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(!queue_data.isMember(jss::transactions));
        }

        auto submitParams = Json::Value(Json::objectValue);
        envs(noop(alice), fee(100), seq(none), ter(terQUEUED))(submitParams);
        envs(noop(alice), fee(100), seq(none), ter(terQUEUED))(submitParams);
        envs(noop(alice), fee(100), seq(none), ter(terQUEUED))(submitParams);
        envs(noop(alice), fee(100), seq(none), ter(terQUEUED))(submitParams);
        checkMetrics(env, 4, 6, 4, 3, 256);

        {
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            auto const& data = result[jss::account_data];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 4);
            BEAST_EXPECT(queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(queue_data[jss::lowest_sequence] == data[jss::Sequence]);
            BEAST_EXPECT(queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(queue_data[jss::highest_sequence] ==
                data[jss::Sequence].asUInt() +
                    queue_data[jss::txn_count].asUInt() - 1);
            BEAST_EXPECT(!queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(!queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(queue_data.isMember(jss::transactions));
            auto const& queued = queue_data[jss::transactions];
            BEAST_EXPECT(queued.size() == queue_data[jss::txn_count]);
            for (unsigned i = 0; i < queued.size(); ++i)
            {
                auto const& item = queued[i];
                BEAST_EXPECT(item[jss::seq] == data[jss::Sequence].asInt() + i);
                BEAST_EXPECT(item[jss::fee_level] == "2560");
                BEAST_EXPECT(!item.isMember(jss::LastLedgerSequence));

                if (i == queued.size() - 1)
                {
                    BEAST_EXPECT(!item.isMember(jss::fee));
                    BEAST_EXPECT(!item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(!item.isMember(jss::auth_change));
                }
                else
                {
                    BEAST_EXPECT(item.isMember(jss::fee));
                    BEAST_EXPECT(item[jss::fee] == "100");
                    BEAST_EXPECT(item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(item[jss::max_spend_drops] == "100");
                    BEAST_EXPECT(item.isMember(jss::auth_change));
                    BEAST_EXPECT(!item[jss::auth_change].asBool());
                }

            }
        }

//排队拦阻
        envs(fset(alice, asfAccountTxnID), fee(100), seq(none),
            json(jss::LastLedgerSequence, 10),
                ter(terQUEUED))(submitParams);
        checkMetrics(env, 5, 6, 4, 3, 256);

        {
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            auto const& data = result[jss::account_data];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 5);
            BEAST_EXPECT(queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(queue_data[jss::lowest_sequence] == data[jss::Sequence]);
            BEAST_EXPECT(queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(queue_data[jss::highest_sequence] ==
                data[jss::Sequence].asUInt() +
                    queue_data[jss::txn_count].asUInt() - 1);
            BEAST_EXPECT(!queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(!queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(queue_data.isMember(jss::transactions));
            auto const& queued = queue_data[jss::transactions];
            BEAST_EXPECT(queued.size() == queue_data[jss::txn_count]);
            for (unsigned i = 0; i < queued.size(); ++i)
            {
                auto const& item = queued[i];
                BEAST_EXPECT(item[jss::seq] == data[jss::Sequence].asInt() + i);
                BEAST_EXPECT(item[jss::fee_level] == "2560");

                if (i == queued.size() - 1)
                {
                    BEAST_EXPECT(!item.isMember(jss::fee));
                    BEAST_EXPECT(!item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(!item.isMember(jss::auth_change));
                    BEAST_EXPECT(item.isMember(jss::LastLedgerSequence));
                    BEAST_EXPECT(item[jss::LastLedgerSequence] == 10);
                }
                else
                {
                    BEAST_EXPECT(item.isMember(jss::fee));
                    BEAST_EXPECT(item[jss::fee] == "100");
                    BEAST_EXPECT(item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(item[jss::max_spend_drops] == "100");
                    BEAST_EXPECT(item.isMember(jss::auth_change));
                    BEAST_EXPECT(!item[jss::auth_change].asBool());
                    BEAST_EXPECT(!item.isMember(jss::LastLedgerSequence));
                }

            }
        }

        envs(noop(alice), fee(100), seq(none), ter(telCAN_NOT_QUEUE_BLOCKED))(submitParams);
        checkMetrics(env, 5, 6, 4, 3, 256);

        {
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            auto const& data = result[jss::account_data];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 5);
            BEAST_EXPECT(queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(queue_data[jss::lowest_sequence] == data[jss::Sequence]);
            BEAST_EXPECT(queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(queue_data[jss::highest_sequence] ==
                data[jss::Sequence].asUInt() +
                    queue_data[jss::txn_count].asUInt() - 1);
            BEAST_EXPECT(queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(queue_data[jss::auth_change_queued].asBool());
            BEAST_EXPECT(queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(queue_data[jss::max_spend_drops_total] == "500");
            BEAST_EXPECT(queue_data.isMember(jss::transactions));
            auto const& queued = queue_data[jss::transactions];
            BEAST_EXPECT(queued.size() == queue_data[jss::txn_count]);
            for (unsigned i = 0; i < queued.size(); ++i)
            {
                auto const& item = queued[i];
                BEAST_EXPECT(item[jss::seq] == data[jss::Sequence].asInt() + i);
                BEAST_EXPECT(item[jss::fee_level] == "2560");

                if (i == queued.size() - 1)
                {
                    BEAST_EXPECT(item.isMember(jss::fee));
                    BEAST_EXPECT(item[jss::fee] == "100");
                    BEAST_EXPECT(item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(item[jss::max_spend_drops] == "100");
                    BEAST_EXPECT(item.isMember(jss::auth_change));
                    BEAST_EXPECT(item[jss::auth_change].asBool());
                    BEAST_EXPECT(item.isMember(jss::LastLedgerSequence));
                    BEAST_EXPECT(item[jss::LastLedgerSequence] == 10);
                }
                else
                {
                    BEAST_EXPECT(item.isMember(jss::fee));
                    BEAST_EXPECT(item[jss::fee] == "100");
                    BEAST_EXPECT(item.isMember(jss::max_spend_drops));
                    BEAST_EXPECT(item[jss::max_spend_drops] == "100");
                    BEAST_EXPECT(item.isMember(jss::auth_change));
                    BEAST_EXPECT(!item[jss::auth_change].asBool());
                    BEAST_EXPECT(!item.isMember(jss::LastLedgerSequence));
                }

            }
        }

        {
            auto const info = env.rpc("json", "account_info", prevLedgerWithQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                RPC::contains_error(info[jss::result]));
        }

        env.close();
        checkMetrics(env, 1, 8, 5, 4, 256);
        env.close();
        checkMetrics(env, 0, 10, 1, 5, 256);

        {
            auto const info = env.rpc("json", "account_info", withQueue);
            BEAST_EXPECT(info.isMember(jss::result) &&
                info[jss::result].isMember(jss::account_data));
            auto const& result = info[jss::result];
            BEAST_EXPECT(result.isMember(jss::queue_data));
            auto const& queue_data = result[jss::queue_data];
            BEAST_EXPECT(queue_data.isObject());
            BEAST_EXPECT(queue_data.isMember(jss::txn_count));
            BEAST_EXPECT(queue_data[jss::txn_count] == 0);
            BEAST_EXPECT(!queue_data.isMember(jss::lowest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::highest_sequence));
            BEAST_EXPECT(!queue_data.isMember(jss::auth_change_queued));
            BEAST_EXPECT(!queue_data.isMember(jss::max_spend_drops_total));
            BEAST_EXPECT(!queue_data.isMember(jss::transactions));
        }
    }

    void testServerInfo()
    {
        using namespace jtx;
        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } }));
        Env_ss envs(env);

        Account const alice{ "alice" };
        env.fund(XRP(1000000), alice);
        env.close();

        {
            auto const server_info = env.rpc("server_info");
            BEAST_EXPECT(server_info.isMember(jss::result) &&
                server_info[jss::result].isMember(jss::info));
            auto const& info = server_info[jss::result][jss::info];
            BEAST_EXPECT(info.isMember(jss::load_factor) &&
                info[jss::load_factor] == 1);
            BEAST_EXPECT(!info.isMember(jss::load_factor_server));
            BEAST_EXPECT(!info.isMember(jss::load_factor_local));
            BEAST_EXPECT(!info.isMember(jss::load_factor_net));
            BEAST_EXPECT(!info.isMember(jss::load_factor_fee_escalation));
        }
        {
            auto const server_state = env.rpc("server_state");
            auto const& state = server_state[jss::result][jss::state];
            BEAST_EXPECT(state.isMember(jss::load_factor) &&
                state[jss::load_factor] == 256);
            BEAST_EXPECT(state.isMember(jss::load_base) &&
                state[jss::load_base] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_server) &&
                state[jss::load_factor_server] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_escalation) &&
                state[jss::load_factor_fee_escalation] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_queue) &&
                state[jss::load_factor_fee_queue] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_reference) &&
                state[jss::load_factor_fee_reference] == 256);
        }

        checkMetrics(env, 0, 6, 0, 3, 256);

        fillQueue(env, alice);
        checkMetrics(env, 0, 6, 4, 3, 256);

        auto aliceSeq = env.seq(alice);
        auto submitParams = Json::Value(Json::objectValue);
        for (auto i = 0; i < 4; ++i)
            envs(noop(alice), fee(100), seq(aliceSeq + i),
                ter(terQUEUED))(submitParams);
        checkMetrics(env, 4, 6, 4, 3, 256);

        {
            auto const server_info = env.rpc("server_info");
            BEAST_EXPECT(server_info.isMember(jss::result) &&
                server_info[jss::result].isMember(jss::info));
            auto const& info = server_info[jss::result][jss::info];
//通过与一个范围比较，避免双重取整问题。
            BEAST_EXPECT(info.isMember(jss::load_factor) &&
                info[jss::load_factor] > 888.88 &&
                    info[jss::load_factor] < 888.89);
            BEAST_EXPECT(info.isMember(jss::load_factor_server) &&
                info[jss::load_factor_server] == 1);
            BEAST_EXPECT(!info.isMember(jss::load_factor_local));
            BEAST_EXPECT(!info.isMember(jss::load_factor_net));
            BEAST_EXPECT(info.isMember(jss::load_factor_fee_escalation) &&
                info[jss::load_factor_fee_escalation] > 888.88 &&
                    info[jss::load_factor_fee_escalation] < 888.89);
        }
        {
            auto const server_state = env.rpc("server_state");
            auto const& state = server_state[jss::result][jss::state];
            BEAST_EXPECT(state.isMember(jss::load_factor) &&
                state[jss::load_factor] == 227555);
            BEAST_EXPECT(state.isMember(jss::load_base) &&
                state[jss::load_base] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_server) &&
                state[jss::load_factor_server] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_escalation) &&
                state[jss::load_factor_fee_escalation] == 227555);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_queue) &&
                state[jss::load_factor_fee_queue] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_reference) &&
                state[jss::load_factor_fee_reference] == 256);
        }

        env.app().getFeeTrack().setRemoteFee(256000);

        {
            auto const server_info = env.rpc("server_info");
            BEAST_EXPECT(server_info.isMember(jss::result) &&
                server_info[jss::result].isMember(jss::info));
            auto const& info = server_info[jss::result][jss::info];
//通过与一个范围比较，避免双重取整问题。
            BEAST_EXPECT(info.isMember(jss::load_factor) &&
                info[jss::load_factor] == 1000);
            BEAST_EXPECT(!info.isMember(jss::load_factor_server));
            BEAST_EXPECT(!info.isMember(jss::load_factor_local));
            BEAST_EXPECT(info.isMember(jss::load_factor_net) &&
                info[jss::load_factor_net] == 1000);
            BEAST_EXPECT(info.isMember(jss::load_factor_fee_escalation) &&
                info[jss::load_factor_fee_escalation] > 888.88 &&
                    info[jss::load_factor_fee_escalation] < 888.89);
        }
        {
            auto const server_state = env.rpc("server_state");
            auto const& state = server_state[jss::result][jss::state];
            BEAST_EXPECT(state.isMember(jss::load_factor) &&
                state[jss::load_factor] == 256000);
            BEAST_EXPECT(state.isMember(jss::load_base) &&
                state[jss::load_base] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_server) &&
                state[jss::load_factor_server] == 256000);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_escalation) &&
                state[jss::load_factor_fee_escalation] == 227555);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_queue) &&
                state[jss::load_factor_fee_queue] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_reference) &&
                state[jss::load_factor_fee_reference] == 256);
        }

        env.app().getFeeTrack().setRemoteFee(256);

//增加服务器负载
        for (int i = 0; i < 5; ++i)
            env.app().getFeeTrack().raiseLocalFee();
        BEAST_EXPECT(env.app().getFeeTrack().getLoadFactor() == 625);

        {
            auto const server_info = env.rpc("server_info");
            BEAST_EXPECT(server_info.isMember(jss::result) &&
                server_info[jss::result].isMember(jss::info));
            auto const& info = server_info[jss::result][jss::info];
//通过与一个范围比较，避免双重取整问题。
            BEAST_EXPECT(info.isMember(jss::load_factor) &&
                info[jss::load_factor] > 888.88 &&
                    info[jss::load_factor] < 888.89);
//LoadManager降低费用之间可能存在竞争，
//以及对服务器信息的调用，所以检查范围很广。
//重要的是它不是1。
            BEAST_EXPECT(info.isMember(jss::load_factor_server) &&
                info[jss::load_factor_server] > 1.245 &&
                info[jss::load_factor_server] < 2.4415);
            BEAST_EXPECT(info.isMember(jss::load_factor_local) &&
                info[jss::load_factor_local] > 1.245 &&
                info[jss::load_factor_local] < 2.4415);
            BEAST_EXPECT(!info.isMember(jss::load_factor_net));
            BEAST_EXPECT(info.isMember(jss::load_factor_fee_escalation) &&
                info[jss::load_factor_fee_escalation] > 888.88 &&
                    info[jss::load_factor_fee_escalation] < 888.89);
        }
        {
            auto const server_state = env.rpc("server_state");
            auto const& state = server_state[jss::result][jss::state];
            BEAST_EXPECT(state.isMember(jss::load_factor) &&
                state[jss::load_factor] == 227555);
            BEAST_EXPECT(state.isMember(jss::load_base) &&
                state[jss::load_base] == 256);
//LoadManager降低费用之间可能存在竞争，
//以及对服务器信息的调用，所以检查范围很广。
//重要的是它不是256。
            BEAST_EXPECT(state.isMember(jss::load_factor_server) &&
                state[jss::load_factor_server] >= 320 &&
                state[jss::load_factor_server] <= 625);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_escalation) &&
                state[jss::load_factor_fee_escalation] == 227555);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_queue) &&
                state[jss::load_factor_fee_queue] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_reference) &&
                state[jss::load_factor_fee_reference] == 256);
        }

        env.close();

        {
            auto const server_info = env.rpc("server_info");
            BEAST_EXPECT(server_info.isMember(jss::result) &&
                server_info[jss::result].isMember(jss::info));
            auto const& info = server_info[jss::result][jss::info];
//通过与一个范围比较，避免双重取整问题。

//LoadManager降低费用之间可能存在竞争，
//以及对服务器信息的调用，所以检查范围很广。
//重要的是它不是1。
            BEAST_EXPECT(info.isMember(jss::load_factor) &&
                info[jss::load_factor] > 1.245 &&
                info[jss::load_factor] < 2.4415);
            BEAST_EXPECT(!info.isMember(jss::load_factor_server));
            BEAST_EXPECT(info.isMember(jss::load_factor_local) &&
                info[jss::load_factor_local] > 1.245 &&
                info[jss::load_factor_local] < 2.4415);
            BEAST_EXPECT(!info.isMember(jss::load_factor_net));
            BEAST_EXPECT(!info.isMember(jss::load_factor_fee_escalation));
        }
        {
            auto const server_state = env.rpc("server_state");
            auto const& state = server_state[jss::result][jss::state];
            BEAST_EXPECT(state.isMember(jss::load_factor) &&
                state[jss::load_factor] >= 320 &&
                state[jss::load_factor] <= 625);
            BEAST_EXPECT(state.isMember(jss::load_base) &&
                state[jss::load_base] == 256);
//LoadManager降低费用之间可能存在竞争，
//以及对服务器信息的调用，所以检查范围很广。
//重要的是它不是256。
            BEAST_EXPECT(state.isMember(jss::load_factor_server) &&
                state[jss::load_factor_server] >= 320 &&
                state[jss::load_factor_server] <= 625);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_escalation) &&
                state[jss::load_factor_fee_escalation] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_queue) &&
                state[jss::load_factor_fee_queue] == 256);
            BEAST_EXPECT(state.isMember(jss::load_factor_fee_reference) &&
                state[jss::load_factor_fee_reference] == 256);
        }
    }

    void testServerSubscribe()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } }));

        Json::Value stream;
        stream[jss::streams] = Json::arrayValue;
        stream[jss::streams].append("server");
        auto wsc = makeWSClient(env.app().config());
        {
            auto jv = wsc->invoke("subscribe", stream);
            BEAST_EXPECT(jv[jss::status] == "success");
        }

        Account a{"a"}, b{"b"}, c{"c"}, d{"d"}, e{"e"}, f{"f"},
            g{"g"}, h{"h"}, i{"i"};


//以非升级费用为前几个账户提供资金
        env.fund(XRP(50000), noripple(a,b,c,d));
        checkMetrics(env, 0, boost::none, 4, 3, 256);

//第一个事务建立消息传递
        using namespace std::chrono_literals;
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
        {
            return jv[jss::type] == "serverStatus" &&
                jv.isMember(jss::load_factor) &&
                jv[jss::load_factor] == 256 &&
                jv.isMember(jss::load_base) &&
                jv[jss::load_base] == 256 &&
                jv.isMember(jss::load_factor_server) &&
                jv[jss::load_factor_server] == 256 &&
                jv.isMember(jss::load_factor_fee_escalation) &&
                jv[jss::load_factor_fee_escalation] == 256 &&
                jv.isMember(jss::load_factor_fee_queue) &&
                jv[jss::load_factor_fee_queue] == 256 &&
                jv.isMember(jss::load_factor_fee_reference) &&
                jv[jss::load_factor_fee_reference] == 256;
        }));
//上一笔交易会增加费用
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
        {
            return jv[jss::type] == "serverStatus" &&
                jv.isMember(jss::load_factor) &&
                jv[jss::load_factor] == 227555 &&
                jv.isMember(jss::load_base) &&
                jv[jss::load_base] == 256 &&
                jv.isMember(jss::load_factor_server) &&
                jv[jss::load_factor_server] == 256 &&
                jv.isMember(jss::load_factor_fee_escalation) &&
                jv[jss::load_factor_fee_escalation] == 227555 &&
                jv.isMember(jss::load_factor_fee_queue) &&
                jv[jss::load_factor_fee_queue] == 256 &&
                jv.isMember(jss::load_factor_fee_reference) &&
                jv[jss::load_factor_fee_reference] == 256;
        }));

        env.close();

//结算分类帐应发布状态更新
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
            {
                return jv[jss::type] == "serverStatus" &&
                    jv.isMember(jss::load_factor) &&
                        jv[jss::load_factor] == 256 &&
                    jv.isMember(jss::load_base) &&
                        jv[jss::load_base] == 256 &&
                    jv.isMember(jss::load_factor_server) &&
                        jv[jss::load_factor_server] == 256 &&
                    jv.isMember(jss::load_factor_fee_escalation) &&
                        jv[jss::load_factor_fee_escalation] == 256 &&
                    jv.isMember(jss::load_factor_fee_queue) &&
                        jv[jss::load_factor_fee_queue] == 256 &&
                    jv.isMember(jss::load_factor_fee_reference) &&
                        jv[jss::load_factor_fee_reference] == 256;
            }));

        checkMetrics(env, 0, 8, 0, 4, 256);

//以非升级费用为接下来的几个账户提供资金
        env.fund(XRP(50000), noripple(e,f,g,h,i));

//低费用的额外交易排队
        auto queued = ter(terQUEUED);
        env(noop(a), fee(10), queued);
        env(noop(b), fee(10), queued);
        env(noop(c), fee(10), queued);
        env(noop(d), fee(10), queued);
        env(noop(e), fee(10), queued);
        env(noop(f), fee(10), queued);
        env(noop(g), fee(10), queued);
        checkMetrics(env, 7, 8, 5, 4, 256);

//上一笔交易会增加费用
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
        {
            return jv[jss::type] == "serverStatus" &&
                jv.isMember(jss::load_factor) &&
                jv[jss::load_factor] == 200000 &&
                jv.isMember(jss::load_base) &&
                jv[jss::load_base] == 256 &&
                jv.isMember(jss::load_factor_server) &&
                jv[jss::load_factor_server] == 256 &&
                jv.isMember(jss::load_factor_fee_escalation) &&
                jv[jss::load_factor_fee_escalation] == 200000 &&
                jv.isMember(jss::load_factor_fee_queue) &&
                jv[jss::load_factor_fee_queue] == 256 &&
                jv.isMember(jss::load_factor_fee_reference) &&
                jv[jss::load_factor_fee_reference] == 256;
        }));

        env.close();
//分类帐结算与已上报的排队交易费用一起发布
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
            {
                return jv[jss::type] == "serverStatus" &&
                    jv.isMember(jss::load_factor) &&
                        jv[jss::load_factor] == 184320 &&
                    jv.isMember(jss::load_base) &&
                        jv[jss::load_base] == 256 &&
                    jv.isMember(jss::load_factor_server) &&
                        jv[jss::load_factor_server] == 256 &&
                    jv.isMember(jss::load_factor_fee_escalation) &&
                        jv[jss::load_factor_fee_escalation] == 184320 &&
                    jv.isMember(jss::load_factor_fee_queue) &&
                        jv[jss::load_factor_fee_queue] == 256 &&
                    jv.isMember(jss::load_factor_fee_reference) &&
                        jv[jss::load_factor_fee_reference] == 256;
            }));

        env.close();
//分类帐关闭清除队列，使费用恢复正常
        BEAST_EXPECT(wsc->findMsg(5s,
            [&](auto const& jv)
            {
                return jv[jss::type] == "serverStatus" &&
                    jv.isMember(jss::load_factor) &&
                        jv[jss::load_factor] == 256 &&
                    jv.isMember(jss::load_base) &&
                        jv[jss::load_base] == 256 &&
                    jv.isMember(jss::load_factor_server) &&
                        jv[jss::load_factor_server] == 256 &&
                    jv.isMember(jss::load_factor_fee_escalation) &&
                        jv[jss::load_factor_fee_escalation] == 256 &&
                    jv.isMember(jss::load_factor_fee_queue) &&
                        jv[jss::load_factor_fee_queue] == 256 &&
                    jv.isMember(jss::load_factor_fee_reference) &&
                        jv[jss::load_factor_fee_reference] == 256;
            }));

        BEAST_EXPECT(!wsc->findMsg(1s,
            [&](auto const& jv)
            {
                return jv[jss::type] == "serverStatus";
            }));

        auto jv = wsc->invoke("unsubscribe", stream);
        BEAST_EXPECT(jv[jss::status] == "success");

    }

    void testClearQueuedAccountTxs()
    {
        using namespace jtx;

        Env env(*this,
            makeConfig({ { "minimum_txn_in_ledger_standalone", "3" } }));
        auto alice = Account("alice");
        auto bob = Account("bob");

        checkMetrics(env, 0, boost::none, 0, 3, 256);
        env.fund(XRP(50000000), alice, bob);

        fillQueue(env, alice);

        auto calcTotalFee = [&](
            std::int64_t alreadyPaid, boost::optional<std::size_t> numToClear = boost::none)
                -> std::uint64_t {
            auto totalFactor = 0;
            auto const metrics = env.app ().getTxQ ().getMetrics (
                *env.current ());
            if (!numToClear)
                numToClear.emplace(metrics.txCount + 1);
            for (int i = 0; i < *numToClear; ++i)
            {
                auto inLedger = metrics.txInLedger + i;
                totalFactor += inLedger * inLedger;
            }
            auto result =
                mulDiv (metrics.medFeeLevel * totalFactor /
                        (metrics.txPerLedger * metrics.txPerLedger),
                    env.current ()->fees ().base, metrics.referenceFeeLevel)
                    .second;
//减去已经支付的费用
            result -= alreadyPaid;
//围捕
            ++result;
            return result;
        };

        testcase("straightfoward positive case");
        {
//以太低的费用排队处理一些事务。
            auto aliceSeq = env.seq(alice);
            for (int i = 0; i < 2; ++i)
            {
                env(noop(alice), fee(100), seq(aliceSeq++), ter(terQUEUED));
            }

//排队支付未结分类帐费用的交易
//这将是第一个调用操作函数的Tx，
//但不会成功。
            env(noop(alice), openLedgerFee(env), seq(aliceSeq++),
                ter(terQUEUED));

            checkMetrics(env, 3, boost::none, 4, 3, 256);

//计算出覆盖所有
//排队的TXS+本身
            std::uint64_t totalFee1 = calcTotalFee (100 * 2 + 8889);
            --totalFee1;

            BEAST_EXPECT(totalFee1 == 60911);
//提交包含该费用的交易。它会排队的
//因为费用水平计算向下取整。这是
//边缘案例测试。
            env(noop(alice), fee(totalFee1), seq(aliceSeq++),
                ter(terQUEUED));

            checkMetrics(env, 4, boost::none, 4, 3, 256);

//现在重复这个过程，包括新的Tx
//避免舍入误差
            std::uint64_t const totalFee2 = calcTotalFee (100 * 2 + 8889 + 60911);
            BEAST_EXPECT(totalFee2 == 35556);
//提交包含该费用的交易。它会成功的。
            env(noop(alice), fee(totalFee2), seq(aliceSeq++));

            checkMetrics(env, 0, boost::none, 9, 3, 256);
        }

        testcase("replace last tx with enough to clear queue");
        {
//以太低的费用排队处理一些事务。
            auto aliceSeq = env.seq(alice);
            for (int i = 0; i < 2; ++i)
            {
                env(noop(alice), fee(100), seq(aliceSeq++), ter(terQUEUED));
            }

//排队支付未结分类帐费用的交易
//这将是第一个调用操作函数的Tx，
//但不会成功。
            env(noop(alice), openLedgerFee(env), seq(aliceSeq++),
                ter(terQUEUED));

            checkMetrics(env, 3, boost::none, 9, 3, 256);

//计算出覆盖所有
//排队的TXS+本身
            auto const metrics = env.app ().getTxQ ().getMetrics (
                *env.current ());
            std::uint64_t const totalFee =
                calcTotalFee (100 * 2, metrics.txCount);
            BEAST_EXPECT(totalFee == 167578);
//用高额费用替换上一个Tx成功。
            --aliceSeq;
            env(noop(alice), fee(totalFee), seq(aliceSeq++));

//队列已清空
            checkMetrics(env, 0, boost::none, 12, 3, 256);

            env.close();
            checkMetrics(env, 0, 24, 0, 12, 256);
        }

        testcase("replace middle tx with enough to clear queue");
        {
            fillQueue(env, alice);
//以太低的费用排队处理一些事务。
            auto aliceSeq = env.seq(alice);
            for (int i = 0; i < 5; ++i)
            {
                env(noop(alice), fee(100), seq(aliceSeq++), ter(terQUEUED));
            }

            checkMetrics(env, 5, 24, 13, 12, 256);

//计算出3个TXN的费用
            std::uint64_t const totalFee = calcTotalFee(100 * 2, 3);
            BEAST_EXPECT(totalFee == 20287);
//用高额费用替换上一个Tx成功。
            aliceSeq -= 3;
            env(noop(alice), fee(totalFee), seq(aliceSeq++));

            checkMetrics(env, 2, 24, 16, 12, 256);
            auto const aliceQueue = env.app().getTxQ().getAccountTxs(
                alice.id(), *env.current());
            BEAST_EXPECT(aliceQueue.size() == 2);
            auto seq = aliceSeq;
            for (auto const& tx : aliceQueue)
            {
                BEAST_EXPECT(tx.first == seq);
                BEAST_EXPECT(tx.second.feeLevel == 2560);
                ++seq;
            }

//关闭分类帐以清除队列
            env.close();
            checkMetrics(env, 0, 32, 2, 16, 256);
        }

        testcase("clear queue failure (load)");
        {
            fillQueue(env, alice);
//以太低的费用排队处理一些事务。
            auto aliceSeq = env.seq(alice);
            for (int i = 0; i < 2; ++i)
            {
                env(noop(alice), fee(200), seq(aliceSeq++), ter(terQUEUED));
            }
            for (int i = 0; i < 2; ++i)
            {
                env(noop(alice), fee(22), seq(aliceSeq++), ter(terQUEUED));
            }

            checkMetrics(env, 4, 32, 17, 16, 256);

//计算出覆盖所有TXN的成本
//+ 1
            std::uint64_t const totalFee = calcTotalFee (200 * 2 + 22 * 2);
            BEAST_EXPECT(totalFee == 35006);
//这个费用应该足够了，但哦不！服务器负载上升！
            auto& feeTrack = env.app().getFeeTrack();
            auto const origFee = feeTrack.getRemoteFee();
            feeTrack.setRemoteFee(origFee * 5);
//相反，Tx排队，所有排队的
//TXS保持在队列中。
            env(noop(alice), fee(totalFee), seq(aliceSeq++), ter(terQUEUED));

//原始的最后一个事务仍在队列中
            checkMetrics(env, 5, 32, 17, 16, 256);

//在高负载情况下，一些TXS仍在队列中
            env.close();
            checkMetrics(env, 3, 34, 2, 17, 256);

//负载下降
            feeTrack.setRemoteFee(origFee);

//由于前面的失败，爱丽丝无法清除队列，
//不管费用多高
            fillQueue(env, bob);
            checkMetrics(env, 3, 34, 18, 17, 256);

            env(noop(alice), fee(XRP(1)), seq(aliceSeq++), ter(terQUEUED));
            checkMetrics(env, 4, 34, 18, 17, 256);

//正常加载时，这些TXS进入分类帐
            env.close();
            checkMetrics(env, 0, 36, 4, 18, 256);
        }
    }

    void
    testScaling()
    {
        using namespace jtx;
        using namespace std::chrono_literals;

        {
            Env env(*this,
                makeConfig({ { "minimum_txn_in_ledger_standalone", "3" },
                    { "normal_consensus_increase_percent", "25" },
                    { "slow_consensus_decrease_percent", "50" },
                    { "target_txn_in_ledger", "10" },
                    { "maximum_txn_per_account", "200" } }));
            auto alice = Account("alice");

            checkMetrics(env, 0, boost::none, 0, 3, 256);
            env.fund(XRP(50000000), alice);

            fillQueue(env, alice);
            checkMetrics(env, 0, boost::none, 4, 3, 256);
            auto seqAlice = env.seq(alice);
            auto txCount = 140;
            for (int i = 0; i < txCount; ++i)
                env(noop(alice), seq(seqAlice++), ter(terQUEUED));
            checkMetrics(env, txCount, boost::none, 4, 3, 256);

//成功关闭一些分类帐，因此限制增加

            env.close();
//4＋25%＝5
            txCount -= 6;
            checkMetrics(env, txCount, 10, 6, 5, 257);

            env.close();
//6＋25%＝7
            txCount -= 8;
            checkMetrics(env, txCount, 14, 8, 7, 257);

            env.close();
//8＋25%＝10
            txCount -= 11;
            checkMetrics(env, txCount, 20, 11, 10, 257);

            env.close();
//11＋25%＝13
            txCount -= 14;
            checkMetrics(env, txCount, 26, 14, 13, 257);

            env.close();
//14＋25%＝17
            txCount -= 18;
            checkMetrics(env, txCount, 34, 18, 17, 257);

            env.close();
//18＋25%＝22
            txCount -= 23;
            checkMetrics(env, txCount, 44, 23, 22, 257);

            env.close();
//23＋25%＝28
            txCount -= 29;
            checkMetrics(env, txCount, 56, 29, 28, 256);

//预计从3到28在7“快速”分类账。

//延迟关闭分类帐。
            env.close(env.now() + 5s, 10000ms);
            txCount -= 15;
            checkMetrics(env, txCount, 56, 15, 14, 256);

//延迟关闭分类帐。
            env.close(env.now() + 5s, 10000ms);
            txCount -= 8;
            checkMetrics(env, txCount, 56, 8, 7, 256);

//延迟关闭分类帐。
            env.close(env.now() + 5s, 10000ms);
            txCount -= 4;
            checkMetrics(env, txCount, 56, 4, 3, 256);

//预计从28降到3分之3的“慢”分类账。

//确认最小杆数
            env.close(env.now() + 5s, 10000ms);
            txCount -= 4;
            checkMetrics(env, txCount, 56, 4, 3, 256);

            BEAST_EXPECT(!txCount);
        }

        {
            Env env(*this,
                makeConfig({ { "minimum_txn_in_ledger_standalone", "3" },
                    { "normal_consensus_increase_percent", "150" },
                    { "slow_consensus_decrease_percent", "150" },
                    { "target_txn_in_ledger", "10" },
                    { "maximum_txn_per_account", "200" } }));
            auto alice = Account("alice");

            checkMetrics(env, 0, boost::none, 0, 3, 256);
            env.fund(XRP(50000000), alice);

            fillQueue(env, alice);
            checkMetrics(env, 0, boost::none, 4, 3, 256);
            auto seqAlice = env.seq(alice);
            auto txCount = 43;
            for (int i = 0; i < txCount; ++i)
                env(noop(alice), seq(seqAlice++), ter(terQUEUED));
            checkMetrics(env, txCount, boost::none, 4, 3, 256);

//成功关闭一些分类帐，因此限制增加

            env.close();
//4＋150%＝10
            txCount -= 11;
            checkMetrics(env, txCount, 20, 11, 10, 257);

            env.close();
//11＋150%＝27
            txCount -= 28;
            checkMetrics(env, txCount, 54, 28, 27, 256);

//预计从3到28在7“快速”分类账。

//延迟关闭分类帐。
            env.close(env.now() + 5s, 10000ms);
            txCount -= 4;
            checkMetrics(env, txCount, 54, 4, 3, 256);

//预计从28降到3分之3的“慢”分类账。

            BEAST_EXPECT(!txCount);
        }
    }
    
    void run() override
    {
        testQueue();
        testLocalTxRetry();
        testLastLedgerSeq();
        testZeroFeeTxn();
        testPreclaimFailures();
        testQueuedFailure();
        testMultiTxnPerAccount();
        testTieBreaking();
        testAcctTxnID();
        testMaximum();
        testUnexpectedBalanceChange();
        testBlockers();
        testInFlightBalance();
        testConsequences();
        testRPC();
        testExpirationReplacement();
        testSignAndSubmitSequence();
        testAccountInfo();
        testServerInfo();
        testServerSubscribe();
        testClearQueuedAccountTxs();
        testScaling();
    }
};

BEAST_DEFINE_TESTSUITE_PRIO(TxQ,app,ripple,1);

}
}
