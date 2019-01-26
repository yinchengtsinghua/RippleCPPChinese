
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

#include <ripple/app/ledger/BuildLedger.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerReplay.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/app/tx/apply.h>
#include <ripple/protocol/Feature.h>

namespace ripple {

/*发送到带签名的ApplyTxs invocable的通用buildgerimpl
    void（openview&，std:：shared_ptr<ledger>const&）
   它负责将事务添加到打开的视图以生成
   新分类帐。这是一般性的，因为机制因共识不同而不同
   生成的分类帐与重放的分类帐。
**/

template <class ApplyTxs>
std::shared_ptr<Ledger>
buildLedgerImpl(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    beast::Journal j,
    ApplyTxs&& applyTxs)
{
    auto built = std::make_shared<Ledger>(*parent, closeTime);

    if (built->rules().enabled(featureSHAMapV2) && !built->stateMap().is_v2())
        built->make_v2();

//设置为将shamap更改写入数据库，
//执行更新，提取更改

    {
        OpenView accum(&*built);
        assert(!accum.open());
        applyTxs(accum, built);
        accum.apply(*built);
    }

    built->updateSkipList();
    {
//编写所有修改后的shamap的最终版本
//节点到节点存储以保留新的LCL

        int const asf = built->stateMap().flushDirty(
            hotACCOUNT_NODE, built->info().seq);
        int const tmf = built->txMap().flushDirty(
            hotTRANSACTION_NODE, built->info().seq);
        JLOG(j.debug()) << "Flushed " << asf << " accounts and " << tmf
                        << " transaction nodes";
    }
    built->unshare();

//收款分类帐
    built->setAccepted(
        closeTime, closeResolution, closeTimeCorrect, app.config());

    return built;
}

/*将一组共识交易应用于分类帐。

  应用程序的@param app句柄
  @param txns要应用的事务集，
  @param失败了一组无法应用的事务
  @param查看要应用的分类帐
  @param j日志
  @返回应用的事务数；要重试的事务留在txns中
**/


std::size_t
applyTransactions(
    Application& app,
    std::shared_ptr<Ledger const> const& built,
    CanonicalTXSet& txns,
    std::set<TxID>& failed,
    OpenView& view,
    beast::Journal j)
{
    bool certainRetry = true;
    std::size_t count = 0;

//尝试应用所有可检索的事务
    for (int pass = 0; pass < LEDGER_TOTAL_PASSES; ++pass)
    {
        JLOG(j.debug())
            << (certainRetry ? "Pass: " : "Final pass: ") << pass
            << " begins (" << txns.size() << " transactions)";
        int changes = 0;

        auto it = txns.begin();

        while (it != txns.end())
        {
            auto const txid = it->first.getTXID();

            try
            {
                if (pass == 0 && built->txExists(txid))
                {
                    it = txns.erase(it);
                    continue;
                }

                switch (applyTransaction(
                    app, view, *it->second, certainRetry, tapNONE, j))
                {
                    case ApplyResult::Success:
                        it = txns.erase(it);
                        ++changes;
                        break;

                    case ApplyResult::Fail:
                        failed.insert(txid);
                        it = txns.erase(it);
                        break;

                    case ApplyResult::Retry:
                        ++it;
                }
            }
            catch (std::exception const&)
            {
                JLOG(j.warn()) << "Transaction " << txid << " throws";
                failed.insert(txid);
                it = txns.erase(it);
            }
        }

        JLOG(j.debug())
            << (certainRetry ? "Pass: " : "Final pass: ") << pass
            << " completed (" << changes << " changes)";

//非重试通行证未做任何更改
        if (!changes && !certainRetry)
            break;

//停止可重审通行证
        if (!changes || (pass >= LEDGER_RETRY_PASSES))
            certainRetry = false;
    }

//如果还有交易，我们必须
//至少在最后一次传球中尝试过
    assert(txns.empty() || !certainRetry);
    return count;
}

//根据共识交易建立分类帐
std::shared_ptr<Ledger>
buildLedger(
    std::shared_ptr<Ledger const> const& parent,
    NetClock::time_point closeTime,
    const bool closeTimeCorrect,
    NetClock::duration closeResolution,
    Application& app,
    CanonicalTXSet& txns,
    std::set<TxID>& failedTxns,
    beast::Journal j)
{
    JLOG(j.debug()) << "Report: Transaction Set = " << txns.key()
                    << ", close " << closeTime.time_since_epoch().count()
                    << (closeTimeCorrect ? "" : " (incorrect)");

    return buildLedgerImpl(parent, closeTime, closeTimeCorrect,
        closeResolution, app, j,
        [&](OpenView& accum, std::shared_ptr<Ledger> const& built)
        {
            JLOG(j.debug())
                << "Attempting to apply " << txns.size()
                << " transactions";

            auto const applied = applyTransactions(app, built, txns,
                failedTxns, accum, j);

            if (txns.size() || txns.size())
                JLOG(j.debug())
                    << "Applied " << applied << " transactions; "
                    << failedTxns.size() << " failed and "
                    << txns.size() << " will be retried.";
            else
                JLOG(j.debug())
                    << "Applied " << applied
                    << " transactions.";
        });
}

//通过重放建立分类帐
std::shared_ptr<Ledger>
buildLedger(
    LedgerReplay const& replayData,
    ApplyFlags applyFlags,
    Application& app,
    beast::Journal j)
{
    auto const& replayLedger = replayData.replay();

    JLOG(j.debug()) << "Report: Replay Ledger " << replayLedger->info().hash;

    return buildLedgerImpl(
        replayData.parent(),
        replayLedger->info().closeTime,
        ((replayLedger->info().closeFlags & sLCF_NoConsensusTime) == 0),
        replayLedger->info().closeTimeResolution,
        app,
        j,
        [&](OpenView& accum, std::shared_ptr<Ledger> const& built)
        {
            for (auto& tx : replayData.orderedTxns())
                applyTransaction(app, accum, *tx.second, false, applyFlags, j);
        });
}

}  //命名空间波纹
