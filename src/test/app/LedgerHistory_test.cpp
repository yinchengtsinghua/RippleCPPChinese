
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
    版权所有（c）2018 Ripple Labs Inc.

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

#include <ripple/app/ledger/LedgerHistory.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/tx/apply.h>
#include <ripple/beast/insight/NullCollector.h>
#include <ripple/beast/unit_test.h>
#include <ripple/ledger/OpenView.h>
#include <chrono>
#include <memory>
#include <sstream>
#include <test/jtx.h>

namespace ripple {
namespace test {

class LedgerHistory_test : public beast::unit_test::suite
{
public:
    /*搜索特定消息子字符串的日志管理器
     **/

    class CheckMessageLogs : public Logs
    {
        std::string msg_;
        bool& found_;

        class CheckMessageSink : public beast::Journal::Sink
        {
            CheckMessageLogs& owner_;

        public:
            CheckMessageSink(
                beast::severities::Severity threshold,
                CheckMessageLogs& owner)
                : beast::Journal::Sink(threshold, false), owner_(owner)
            {
            }

            void
            write(beast::severities::Severity level, std::string const& text)
                override
            {
                if (text.find(owner_.msg_) != std::string::npos)
                    owner_.found_ = true;
            }
        };

    public:
        /*构造函数

            @param msg要搜索的消息字符串
            @param找到一个变量，如果找到消息，该变量将设置为true。
        **/

        CheckMessageLogs(std::string msg, bool& found)
            : Logs{beast::severities::kDebug}
            , msg_{std::move(msg)}
            , found_{found}
        {
        }

        std::unique_ptr<beast::Journal::Sink>
        makeSink(
            std::string const& partition,
            beast::severities::Severity threshold) override
        {
            return std::make_unique<CheckMessageSink>(threshold, *this);
        }
    };

    /*手工生成新分类帐，应用特定的结算时间偏移量
        以及可选地插入事务。

        如果prev为nullptr，则生成Genesis分类账，不进行抵消或
        已应用事务。

    **/

    static std::shared_ptr<Ledger>
    makeLedger(
        std::shared_ptr<Ledger const> const& prev,
        jtx::Env& env,
        LedgerHistory& lh,
        NetClock::duration closeOffset,
        std::shared_ptr<STTx const> stx = {})
    {
        if (!prev)
        {
            assert(!stx);
            return std::make_shared<Ledger>(
                create_genesis,
                env.app().config(),
                std::vector<uint256>{},
                env.app().family());
        }
        auto res = std::make_shared<Ledger>(
            *prev, prev->info().closeTime + closeOffset);

        if (stx)
        {
            OpenView accum(&*res);
            applyTransaction(
                env.app(), accum, *stx, false, tapNONE, env.journal);
            accum.apply(*res);
        }
        res->updateSkipList();

        {
            res->stateMap().flushDirty(hotACCOUNT_NODE, res->info().seq);
            res->txMap().flushDirty(hotTRANSACTION_NODE, res->info().seq);
        }
        res->unshare();

//收款分类帐
        res->setAccepted(
            res->info().closeTime,
            res->info().closeTimeResolution,
            /*E/*关闭时间正确*/，
            env.app（）.config（））；
        lh.插入（res，false）；
        收益率；
    }

    无效
    测试句柄监视（）
    {
        测试用例（“LedgerHistory不匹配”）；
        使用名称空间jtx；
        使用命名空间std:：chrono；

        / /无错配
        {
            bool found=false；
            这是，
                    EnvCuffic（）
                    std:：make_unique<checkmessagelogs>（“mismatch”，found）；
            LedgerHistory左侧Beast:：Insight:：NullCollector:：New（），env.app（）
            auto const genesis=makeledger（，env，lh，0s）；
            uint256常量dummytxshash 1
            建筑分类帐（Genesis，DummyTxHash，）；
            验证分类账（Genesis，DummyTxHash）；

            期待！发现）；
        }

        //关闭时间不匹配
        {
            bool found=false；
            这是，
                    EnvCuffic（）
                    std：：使唯一
                        “关闭时间不匹配”，找到）；
            LedgerHistory左侧Beast:：Insight:：NullCollector:：New（），env.app（）
            auto const genesis=makeledger（，env，lh，0s）；
            auto const ledgera=生成分类账（Genesis，env，lh，4s）；
            auto const ledgerb=生成分类账（Genesis，env，lh，40s）；

            uint256常量dummytxshash 1
            左侧.内部分类账（Ledgera，DummyTxHash，）；
            验证分类账（LedgerB、DummyTxHash）；

            野兽期待（找到）；
        }

        //上一个分类帐不匹配
        {
            bool found=false；
            这是，
                    EnvCuffic（）
                    std：：使唯一
                        “上一个分类账不匹配”，找到）；
            LedgerHistory左侧Beast:：Insight:：NullCollector:：New（），env.app（）
            auto const genesis=makeledger（，env，lh，0s）；
            auto const ledgera=生成分类账（Genesis，env，lh，4s）；
            auto const ledgerb=生成分类账（Genesis，env，lh，40s）；
            auto const ledgerac=生成分类帐（分类帐，env，lh，4s）；
            auto const ledgerbd=生成分类账（分类账rb、env、lh、4s）；

            uint256常量dummytxshash 1
            左侧.内部分类账（LedgerAC，DummyTxHash，）；
            已验证的分类账（分类账、DummyTxHash）；

            野兽期待（找到）；
        }

        //模拟一个bug，在这个bug中一致同意事务，但是
        //以某种方式生成不同的分类帐
        for（bool const txbug：真，假）
        {
            std:：string const msg=txbug
                “与同一共识事务集不匹配”
                ：“共识交易集不匹配”；
            bool found=false；
            这是，
                    EnvCuffic（）
                    std:：make_unique<checkmessagelogs>（msg，found）；
            LedgerHistory左侧Beast:：Insight:：NullCollector:：New（），env.app（）

            账户Alice“A1”
            账户Bob“A2”
            环境基金（XRP（1000），Alice，Bob）；
            关闭（）；

            自动构造分类数据库=
                env.app（）.getledgermaster（）.getClosedledger（）；

            jtx txalice=env.jt（noop（alice））；
            自动构造=
                制作分类账（ledgerbase、env、lh、4s、txalice.stx）；

            jtx txbob=env.jt（noop（bob））；
            auto const ledgerb=生成分类帐（ledgerbase、env、lh、4s、txbob.stx）；

            lh.builtLedger（Ledgera，txalice.stx->GetTransactionId（），）；
            //通过声称Ledgerb具有相同的共识哈希来模拟bug
            //作为分类账，但不知何故产生了不同的分类账
            确认分类账（
                莱格尔布
                TXBUG？txalice.stx->getTransactionId（）。
                      ：txbob.stx->GetTransactionId（））；

            野兽期待（找到）；
        }
    }

    无效
    Run（）重写
    {
        testHandleMismatch（）；
    }
}；

Beast定义测试套件（LedgerHistory、app、Ripple）；

//命名空间测试
//名称空间波纹
