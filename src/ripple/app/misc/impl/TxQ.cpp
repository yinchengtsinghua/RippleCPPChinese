
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

#include <ripple/app/misc/TxQ.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/tx/apply.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/basics/mulDiv.h>
#include <boost/algorithm/clamp.hpp>
#include <limits>
#include <numeric>
#include <algorithm>

namespace ripple {

////////////////////////////////////////////////

static
std::uint64_t
getFeeLevelPaid(
    STTx const& tx,
    std::uint64_t baseRefLevel,
    std::uint64_t refTxnCostDrops,
    TxQ::Setup const& setup)
{
    if (refTxnCostDrops == 0)
//如果不需要，或者成本为0，
//水平实际上是无限的。
        return setup.zeroBaseFeeTransactionFeeLevel;

//如果数学溢出，则返回剪裁的
//盲目的结果。这是不太可能的
//发生。
    return mulDiv(tx[sfFee].xrp().drops(),
        baseRefLevel,
            refTxnCostDrops).second;
}

static
boost::optional<LedgerIndex>
getLastLedgerSequence(STTx const& tx)
{
    if (!tx.isFieldPresent(sfLastLedgerSequence))
        return boost::none;
    return tx.getFieldU32(sfLastLedgerSequence);
}

static
std::uint64_t
increase(std::uint64_t level,
    std::uint32_t increasePercent)
{
    return mulDiv(
        level, 100 + increasePercent, 100).second;
}


////////////////////////////////////////////////

std::size_t
TxQ::FeeMetrics::update(Application& app,
    ReadView const& view, bool timeLeap,
    TxQ::Setup const& setup)
{
    std::vector<uint64_t> feeLevels;
    auto const txBegin = view.txs.begin();
    auto const txEnd = view.txs.end();
    auto const size = std::distance(txBegin, txEnd);
    feeLevels.reserve(size);
    std::for_each(txBegin, txEnd,
        [&](auto const& tx)
        {
            auto const baseFee = calculateBaseFee(view, *tx.first);
            feeLevels.push_back(getFeeLevelPaid(*tx.first,
                baseLevel, baseFee, setup));
        }
    );
    std::sort(feeLevels.begin(), feeLevels.end());
    assert(size == feeLevels.size());

    JLOG(j_.debug()) << "Ledger " << view.info().seq <<
        " has " << size << " transactions. " <<
        "Ledgers are processing " <<
        (timeLeap ? "slowly" : "as expected") <<
        ". Expected transactions is currently " <<
        txnsExpected_ << " and multiplier is " <<
        escalationMultiplier_;

    if (timeLeap)
    {
//分类帐需要很长时间才能处理，
//所以要限制极限。
        auto const cutPct = 100 - setup.slowConsensusDecreasePercent;
        txnsExpected_ = boost::algorithm::clamp(
            mulDiv(size, cutPct, 100).second,
            minimumTxnCount_,
            mulDiv(txnsExpected_, cutPct, 100).second);
        recentTxnCounts_.clear();
    }
    else if (size > txnsExpected_ ||
        size > targetTxnCount_)
    {
        recentTxnCounts_.push_back(
            mulDiv(size, 100 + setup.normalConsensusIncreasePercent,
                100).second);
        auto const iter = std::max_element(recentTxnCounts_.begin(),
            recentTxnCounts_.end());
        BOOST_ASSERT(iter != recentTxnCounts_.end());
        auto const next = [&]
        {
//快速增长：如果max_元素大于等于
//当前大小限制，使用它。
            if (*iter >= txnsExpected_)
                return *iter;
//缓慢收缩：如果max_元素小于
//当前大小限制，使用的限制为
//从max_元素到
//当前大小限制。
            return (txnsExpected_ * 9 + *iter) / 10;
        }();
//账本处理及时，
//所以保持上限，但不要让它
//无拘无束地成长。
        txnsExpected_ = std::min(next,
            maximumTxnCount_.value_or(next));
    }

    if (!size)
    {
        escalationMultiplier_ = setup.minimumEscalationMultiplier;
    }
    else
    {
//如果元素数目为奇数，则
//计算为中间元素；对于偶数
//元素数量，它将添加两个元素
//在“中间”的两边取平均值。
        escalationMultiplier_ = (feeLevels[size / 2] +
            feeLevels[(size - 1) / 2] + 1) / 2;
        escalationMultiplier_ = std::max(escalationMultiplier_,
            setup.minimumEscalationMultiplier);
    }
    JLOG(j_.debug()) << "Expected transactions updated to " <<
        txnsExpected_ << " and multiplier updated to " <<
        escalationMultiplier_;

    return size;
}

std::uint64_t
TxQ::FeeMetrics::scaleFeeLevel(Snapshot const& snapshot,
    OpenView const& view)
{
//迄今未结分类帐中的交易记录
    auto const current = view.txCount();

    auto const target = snapshot.txnsExpected;
    auto const multiplier = snapshot.escalationMultiplier;

//一旦打开的分类帐绕过目标，
//迅速提高费用。
    if (current > target)
    {
//计算升级的费用水平
//不关心溢出标志
        return mulDiv(multiplier, current * current,
            target * target).second;
    }

    return baseLevel;
}

namespace detail {

static
std::pair<bool, std::uint64_t>
sumOfFirstSquares(std::size_t x)
{
//总和（n=1->x）：n*n=x（x+1）（2x+1）/6

//如果x在2 ^^21的顺序上的任何位置，它将
//完全控制计算
//足以溢出我们假设的
//是的。如果我们有接近2^21的交易
//在分类帐中，这是我们遇到的最少的问题。
    if (x >= (1 << 21))
        return std::make_pair(false,
            std::numeric_limits<std::uint64_t>::max());
    return std::make_pair(true, (x * (x + 1) * (2 * x + 1)) / 6);
}

}

std::pair<bool, std::uint64_t>
TxQ::FeeMetrics::escalatedSeriesFeeLevel(Snapshot const& snapshot,
    OpenView const& view, std::size_t extraCount,
    std::size_t seriesSize)
{
    /*到目前为止，未结分类帐中的交易记录。
        即在以下情况下将在未结分类帐中的交易：
        尝试系列中的第一个Tx。
    **/

    auto const current = view.txCount() + extraCount;
    /*在以下情况下将在未结分类帐中的交易记录：
        尝试序列中的最后一个Tx。
    **/

    auto const last = current + seriesSize - 1;

    auto const target = snapshot.txnsExpected;
    auto const multiplier = snapshot.escalationMultiplier;

    assert(current > target);

    /*计算（为糟糕的符号道歉）
        求和（n=当前值->上一个值）：乘数*n*n/（目标值*目标值）
        乘数/（目标*目标）*（总和（n=当前>最后）：n*n）
        乘数/（目标*目标）*（（总和（n=1->last）：n*n）-
            （总和（n=1->电流-1）：n*n）
    **/

    auto const sumNlast = detail::sumOfFirstSquares(last);
    auto const sumNcurrent = detail::sumOfFirstSquares(current - 1);
//因为“last”更大，如果其中一个和溢出，则
//‘sumnlast’肯定溢出了。还有这个机会
//几乎等于零。
    if (!sumNlast.first)
        return sumNlast;
    auto const totalFeeLevel = mulDiv(multiplier,
        sumNlast.second - sumNcurrent.second, target * target);

    return totalFeeLevel;
}


TxQ::MaybeTx::MaybeTx(
    std::shared_ptr<STTx const> const& txn_,
        TxID const& txID_, std::uint64_t feeLevel_,
            ApplyFlags const flags_,
                PreflightResult const& pfresult_)
    : txn(txn_)
    , feeLevel(feeLevel_)
    , txID(txID_)
    , account(txn_->getAccountID(sfAccount))
    , sequence(txn_->getSequence())
    , retriesRemaining(retriesAllowed)
    , flags(flags_)
    , pfresult(pfresult_)
{
    lastValid = getLastLedgerSequence(*txn);

    if (txn->isFieldPresent(sfAccountTxnID))
        priorTxID = txn->getFieldH256(sfAccountTxnID);
}

std::pair<TER, bool>
TxQ::MaybeTx::apply(Application& app, OpenView& view, beast::Journal j)
{
    boost::optional<STAmountSO> saved;
    if (view.rules().enabled(fix1513))
        saved.emplace(view.info().parentCloseTime);
//如果规则或旗子改变了，再次预燃
    assert(pfresult);
    if (pfresult->rules != view.rules() ||
        pfresult->flags != flags)
    {
        JLOG(j.debug()) << "Queued transaction " <<
            txID << " rules or flags have changed. Flags from " <<
            pfresult->flags << " to " << flags ;

        pfresult.emplace(
            preflight(app, view.rules(),
                pfresult->tx,
                flags,
                pfresult->j));
    }

    auto pcresult = preclaim(
        *pfresult, app, view);

    return doApply(pcresult, app, view);
}

TxQ::TxQAccount::TxQAccount(std::shared_ptr<STTx const> const& txn)
    :TxQAccount(txn->getAccountID(sfAccount))
{
}

TxQ::TxQAccount::TxQAccount(const AccountID& account_)
    : account(account_)
{
}

auto
TxQ::TxQAccount::add(MaybeTx&& txn)
    -> MaybeTx&
{
    auto sequence = txn.sequence;

    auto result = transactions.emplace(sequence, std::move(txn));
    assert(result.second);
    assert(&result.first->second != &txn);

    return result.first->second;
}

bool
TxQ::TxQAccount::remove(TxSeq const& sequence)
{
    return transactions.erase(sequence) != 0;
}

////////////////////////////////////////////////

TxQ::TxQ(Setup const& setup,
    beast::Journal j)
    : setup_(setup)
    , j_(j)
    , feeMetrics_(setup, j)
    , maxSize_(boost::none)
{
}

TxQ::~TxQ()
{
    byFee_.clear();
}

template<size_t fillPercentage>
bool
TxQ::isFull() const
{
    static_assert(fillPercentage > 0 &&
        fillPercentage <= 100,
            "Invalid fill percentage");
    return maxSize_ && byFee_.size() >=
        (*maxSize_ * fillPercentage / 100);
}

bool
TxQ::canBeHeld(STTx const& tx, OpenView const& view,
    AccountMap::iterator accountIter,
        boost::optional<FeeMultiSet::iterator> replacementIter)
{
//previoustxnid已弃用，不应使用
//事务不支持accounttxnid
//尚未排队，但应在将来添加
    bool canBeHeld =
        ! tx.isFieldPresent(sfPreviousTxnID) &&
        ! tx.isFieldPresent(sfAccountTxnID);
    if (canBeHeld)
    {
        /*要排队和中继，事务需要
            保证能坚持足够长的时间
            进入分类帐的现实机会。
        **/

        auto const lastValid = getLastLedgerSequence(tx);
        canBeHeld = !lastValid || *lastValid >=
            view.info().seq + setup_.minimumLastLedgerBuffer;
    }
    if (canBeHeld)
    {
        /*限制单个帐户的交易数
            可以排队。降低继电保护的损失成本
            一个早期的失败或被丢弃。
        **/


//如果帐户根本不在队列中，则允许
        canBeHeld = accountIter == byAccount_.end();

        if(!canBeHeld)
        {
//允许此Tx替换另一个Tx
            canBeHeld = replacementIter.is_initialized();
        }

        if (!canBeHeld)
        {
//如果小于限制，则允许
            canBeHeld = accountIter->second.getTxnCount() <
                setup_.maximumTxnPerAccount;
        }

        if (!canBeHeld)
        {
//如果事务在任何
//排队的事务。启用打开的恢复
//分类帐交易记录和冻结交易记录。
            auto const tSeq = tx.getSequence();
            canBeHeld = tSeq < accountIter->second.transactions.rbegin()->first;
        }
    }
    return canBeHeld;
}

auto
TxQ::erase(TxQ::FeeMultiSet::const_iterator_type candidateIter)
    -> FeeMultiSet::iterator_type
{
    auto& txQAccount = byAccount_.at(candidateIter->account);
    auto const sequence = candidateIter->sequence;
    auto const newCandidateIter = byFee_.erase(candidateIter);
//现在候选人已经从
//侵入列表将其从txqaccount中删除
//这样就可以释放内存。
    auto const found = txQAccount.remove(sequence);
    (void)found;
    assert(found);

    return newCandidateIter;
}

auto
TxQ::eraseAndAdvance(TxQ::FeeMultiSet::const_iterator_type candidateIter)
    -> FeeMultiSet::iterator_type
{
    auto& txQAccount = byAccount_.at(candidateIter->account);
    auto const accountIter = txQAccount.transactions.find(
        candidateIter->sequence);
    assert(accountIter != txQAccount.transactions.end());
    assert(accountIter == txQAccount.transactions.begin());
    assert(byFee_.iterator_to(accountIter->second) == candidateIter);
    auto const accountNextIter = std::next(accountIter);
    /*检查此帐户的下一个交易记录是否具有
        下一个序列号和更高的费用水平，这意味着
        我们之前跳过了，需要再试一次。
        边缘案例：如果下一个账户tx的收费水平较低，
            费用排在后面，所以我们没有
            跳过了。
            如果下一个德克萨斯州的收费水平相同，则
            稍后提交，因此也将在
            费用队列，或重新提交当前项目以增加
            费用水平，我们跳过了下一个德克萨斯州。
            后一种情况下，继续通过收费队列
            避免潜在的命令操作问题。
    **/

    auto const feeNextIter = std::next(candidateIter);
    bool const useAccountNext = accountNextIter != txQAccount.transactions.end() &&
        accountNextIter->first == candidateIter->sequence + 1 &&
            (feeNextIter == byFee_.end() ||
                accountNextIter->second.feeLevel > feeNextIter->feeLevel);
    auto const candidateNextIter = byFee_.erase(candidateIter);
    txQAccount.transactions.erase(accountIter);
    return useAccountNext ?
        byFee_.iterator_to(accountNextIter->second) :
            candidateNextIter;

}

auto
TxQ::erase(TxQ::TxQAccount& txQAccount,
    TxQ::TxQAccount::TxMap::const_iterator begin,
        TxQ::TxQAccount::TxMap::const_iterator end)
            -> TxQAccount::TxMap::iterator
{
    for (auto it = begin; it != end; ++it)
    {
        byFee_.erase(byFee_.iterator_to(it->second));
    }
    return txQAccount.transactions.erase(begin, end);
}

std::pair<TER, bool>
TxQ::tryClearAccountQueue(Application& app, OpenView& view,
    STTx const& tx, TxQ::AccountMap::iterator const& accountIter,
        TxQAccount::TxMap::iterator beginTxIter, std::uint64_t feeLevelPaid,
            PreflightResult const& pfresult, std::size_t const txExtraCount,
                ApplyFlags flags, FeeMetrics::Snapshot const& metricsSnapshot,
                    beast::Journal j)
{
    auto const tSeq = tx.getSequence();
    assert(beginTxIter != accountIter->second.transactions.end());
    auto const aSeq = beginTxIter->first;

    auto const requiredTotalFeeLevel = FeeMetrics::escalatedSeriesFeeLevel(
        metricsSnapshot, view, txExtraCount, tSeq - aSeq + 1);
    /*如果总量的计算成功溢出（无论多么极端
        不太可能），那么我们就无法自信地验证队列是否
        可以清除。
    **/

    if (!requiredTotalFeeLevel.first)
        return std::make_pair(telINSUF_FEE_P, false);

//与multix不同，此检查只涉及范围
//来自[ASEQ，TSEQ]
    auto endTxIter = accountIter->second.transactions.lower_bound(tSeq);

    auto const totalFeeLevelPaid = std::accumulate(beginTxIter, endTxIter,
        feeLevelPaid,
        [](auto const& total, auto const& tx)
        {
            return total + tx.second.feeLevel;
        });

//这笔交易没有支付足够的费用，因此回到正常的过程。
    if (totalFeeLevelPaid < requiredTotalFeeLevel.second)
        return std::make_pair(telINSUF_FEE_P, false);

//这个事务支付了足够的钱来清除队列。
//尝试应用排队的事务。
    for (auto it = beginTxIter; it != endTxIter; ++it)
    {
        auto txResult = it->second.apply(app, view, j);
//成功或失败，请使用重试，因为如果
//进程失败，我们希望尝试计数。如果一切
//成功了，梅贝塔号会被摧毁，所以
//莫特。
        --it->second.retriesRemaining;
        it->second.lastResult = txResult.first;
        if (!txResult.second)
        {
//无法应用事务。回到正常过程。
            return std::make_pair(txResult.first, false);
        }
    }
//应用当前Tx。因为视图的状态已更改
//通过排队的TXS，我们还需要再次预索赔。
    auto txResult = [&]{
        boost::optional<STAmountSO> saved;
        if (view.rules().enabled(fix1513))
            saved.emplace(view.info().parentCloseTime);
        auto const pcresult = preclaim(pfresult, app, view);
        return doApply(pcresult, app, view);
    }();

    if (txResult.second)
    {
//应用了所有排队的事务，因此将它们从队列中删除。
        endTxIter = erase(accountIter->second, beginTxIter, endTxIter);
//如果“tx”正在替换排队的tx，也要删除它。
        if (endTxIter != accountIter->second.transactions.end() &&
            endTxIter->first == tSeq)
            erase(accountIter->second, endTxIter, std::next(endTxIter));
    }

    return txResult;
}


/*
    如何决定应用、排队或拒绝：
    0。是否启用了“FeatureFeeScallation”？
        是：继续下一步。
        否：回退到“Ripple:：Apply”。停下来。
    1。“飞行前”是否表示Tx有效？
        否：从“飞行前”返回“ter”。停下来。
        是：继续下一步。
    2。是否已经有一个与
            队列中的序列号相同？
        是：'txn'的费用'retrySequencePercent'是否高于
                排队交易的费用？这是最后一次发短信吗？
                在该帐户的队列中，或者两者都是TXS
                非阻断剂？
            是：删除排队的事务。继续下一步
                步骤。
            否：用“telcan-not-queue-fee”拒绝“txn”。停下来。
        否：继续下一步。
    三。此Tx是否具有预期的序列号
            帐户？
        是：继续下一步。
        否：所有中间序列号是否也在
                排队？
            否：继续下一步。（我们期待下一个
                步骤返回“terpre_seq”，但不会缩短
                逻辑电路。）
            是的：费用比“multixnpercent”高吗？
                    比上一次发送？
                否：用“telinsuf”拒绝。停下来。
                是：以前的序列有没有TXS阻滞剂？
                    是：用“telcan-not-queue-blocked”拒绝。停下来。
                    不，是另一个航班的费用吗
                            排队的txs大于等于帐户
                            余额还是最低存款准备金？
                        是：用“telcan-not-queue-balance”拒绝。停下来。
                        否：创建一次性沙盒“view”。修改
                            要匹配的帐户序列号
                            tx（避免'terpre_seq`），并减少
                            按总费用和
                            其他飞行中TXS的最大花费。
                            继续下一步。
    4。“preclaim”是否表示账户可能会索赔
            一笔费用（使用上面创建的一次性沙盒“view”，
            如果合适的话）？
        否：从“preclaim”返回“ter”。停下来。
        是：继续下一步。
    5。我们是否创建了一个一次性沙盒“view”？
        是：继续下一步。
        否：'txn'的费用水平是否大于等于所需的费用水平？
            是：`txn`可应用于未结分类账。通过
                它将返回到“doapply（）”并返回该结果。
            否：继续下一步。
    6。Tx是否可以保留在队列中？（请参阅txq:：canbeheald）。
            否：拒绝“txn”和“telcan-not-queue-full”
                如果不是。停下来。
            是：继续下一步。
    7。队列是否已满？
        否：继续下一步。
        是：'txn'的费用水平是否高于末尾/
                最低费用水平项目的费用水平？
            是：删除结束项。继续下一步。
            否：拒绝“txn”，费用较低。
    8。将“txn”放入队列。
**/

std::pair<TER, bool>
TxQ::apply(Application& app, OpenView& view,
    std::shared_ptr<STTx const> const& tx,
        ApplyFlags flags, beast::Journal j)
{
    auto const account = (*tx)[sfAccount];
    auto const transactionID = tx->getTransactionID();
    auto const tSeq = tx->getSequence();

    boost::optional<STAmountSO> saved;
    if (view.rules().enabled(fix1513))
        saved.emplace(view.info().parentCloseTime);

//查看事务是否有效、格式是否正确，
//等等，在排队之前
//替换和多事务操作。
    auto const pfresult = preflight(app, view.rules(),
            *tx, flags, j);
    if (pfresult.ter != tesSUCCESS)
        return{ pfresult.ter, false };

    struct MultiTxn
    {
        explicit MultiTxn() = default;

        boost::optional<ApplyViewImpl> applyView;
        boost::optional<OpenView> openView;

        TxQAccount::TxMap::iterator nextTxIter;

        XRPAmount fee = beast::zero;
        XRPAmount potentialSpend = beast::zero;
        bool includeCurrentFee = false;
    };

    boost::optional<MultiTxn> multiTxn;
    boost::optional<TxConsequences const> consequences;
    boost::optional<FeeMultiSet::iterator> replacedItemDeleteIter;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const metricsSnapshot = feeMetrics_.getSnapshot();

//我们可能需要多个交易的基本费用
//或者事务替换，现在就把它拉上来。
//托多：我们想避免在
//预告？
    auto const baseFee = calculateBaseFee(view, *tx);
    auto const feeLevelPaid = getFeeLevelPaid(*tx,
        baseLevel, baseFee, setup_);
    auto const requiredFeeLevel = [&]()
    {
        auto feeLevel = FeeMetrics::scaleFeeLevel(metricsSnapshot, view);
        if ((flags & tapPREFER_QUEUE) && byFee_.size())
        {
            return std::max(feeLevel, byFee_.begin()->feeLevel);
        }
        return feeLevel;
    }();

    auto accountIter = byAccount_.find(account);
    bool const accountExists = accountIter != byAccount_.end();

//同一账户有交易吗
//队列中已存在相同的序列号？
    if (accountExists)
    {
        auto& txQAcct = accountIter->second;
        auto existingIter = txQAcct.transactions.find(tSeq);
        if (existingIter != txQAcct.transactions.end())
        {
//当前交易的费用是否高于
//排队事务的费用+百分比
            auto requiredRetryLevel = increase(
                existingIter->second.feeLevel,
                    setup_.retrySequencePercent);
            JLOG(j_.trace()) << "Found transaction in queue for account " <<
                account << " with sequence number " << tSeq <<
                " new txn fee level is " << feeLevelPaid <<
                ", old txn fee level is " <<
                existingIter->second.feeLevel <<
                ", new txn needs fee level of " <<
                requiredRetryLevel;
            if (feeLevelPaid > requiredRetryLevel
                || (existingIter->second.feeLevel < requiredFeeLevel &&
                    feeLevelPaid >= requiredFeeLevel &&
                    existingIter == txQAcct.transactions.begin()))
            {
                /*费用可能高到可以重试，或者
                    之前的txn是该账户的第一个，并且
                    无法进入打开的分类帐，但此分类帐可以。
                **/


                /*正常的Tx不能被阻滞剂替代，除非它
                    帐户队列中的最后一个Tx。
                **/

                if (std::next(existingIter) != txQAcct.transactions.end())
                {
//通常，只有队列中的最后一个Tx
//！结果，但过期的事务可以
//换了，换了的就没办法了，
//没关系。
                    if (!existingIter->second.consequences)
                        existingIter->second.consequences.emplace(
                            calculateConsequences(
                                *existingIter->second.pfresult));

                    if (existingIter->second.consequences->category ==
                        TxConsequences::normal)
                    {
                        assert(!consequences);
                        consequences.emplace(calculateConsequences(
                            pfresult));
                        if (consequences->category ==
                            TxConsequences::blocker)
                        {
//无法替换中的正常事务
//队列中间有一个拦截器。
                            JLOG(j_.trace()) <<
                                "Ignoring blocker transaction " <<
                                transactionID <<
                                " in favor of normal queued " <<
                                existingIter->second.txID;
                            return {telCAN_NOT_QUEUE_BLOCKS, false };
                        }
                    }
                }


//删除排队的事务并继续
                JLOG(j_.trace()) <<
                    "Removing transaction from queue " <<
                    existingIter->second.txID <<
                    " in favor of " << transactionID;
//然后保存排队的tx以从队列中删除，如果
//新的Tx成功或排队。不删除
//如果新的Tx失败，因为可能还有其他Tx
//在队列中依赖于它。
                auto deleteIter = byFee_.iterator_to(existingIter->second);
                assert(deleteIter != byFee_.end());
                assert(&existingIter->second == &*deleteIter);
                assert(deleteIter->sequence == tSeq);
                assert(deleteIter->account == txQAcct.account);
                replacedItemDeleteIter = deleteIter;
            }
            else
            {
//删除当前交易记录
                JLOG(j_.trace()) <<
                    "Ignoring transaction " <<
                    transactionID <<
                    " in favor of queued " <<
                    existingIter->second.txID;
                return{ telCAN_NOT_QUEUE_FEE, false };
            }
        }
    }

//如果队列中有其他事务
//对于这个账户，在预检查之前先说明一下，
//所以我们不会得到错误的解释。
    if (accountExists)
    {
        auto const sle = view.read(keylet::account(account));

        if (sle)
        {
            auto& txQAcct = accountIter->second;
            auto const aSeq = (*sle)[sfSequence];

            if (aSeq < tSeq)
            {
//如果事务是可排队的，则创建multixn
//对象来保存我们需要调整的信息
//前TXNS。否则，让预索赔失败，就好像
//我们根本没有排队。
                if (canBeHeld(*tx, view, accountIter, replacedItemDeleteIter))
                    multiTxn.emplace();
            }

            if (multiTxn)
            {
                /*查看队列是否包含所有
                    序列输入[ASEQ，TSEQ]。全部加起来
                    我们检查的结果。如果一
                    出现丢失或是阻滞剂，中止。
                **/

                multiTxn->nextTxIter = txQAcct.transactions.find(aSeq);
                auto workingIter = multiTxn->nextTxIter;
                auto workingSeq = aSeq;
                for (; workingIter != txQAcct.transactions.end();
                    ++workingIter, ++workingSeq)
                {
                    if (workingSeq < tSeq &&
                        workingIter->first != workingSeq)
                    {
//如果在'tx'之前缺少任何事务，则中止。
                        multiTxn.reset();
                        break;
                    }
                    if (workingIter->first == tSeq - 1)
                    {
//当前交易的费用是否高于
//上一笔交易的费用+百分比
                        auto requiredMultiLevel = increase(
                            workingIter->second.feeLevel,
                            setup_.multiTxnPercent);

                        if (feeLevelPaid <= requiredMultiLevel)
                        {
//删除当前交易记录
                            JLOG(j_.trace()) <<
                                "Ignoring transaction " <<
                                transactionID <<
                                ". Needs fee level of " <<

                                requiredMultiLevel <<
                                ". Only paid " <<
                                feeLevelPaid;
                            return{ telINSUF_FEE_P, false };
                        }
                    }
                    if (workingIter->first == tSeq)
                    {
//如果我们要替换这个交易，不要
//数一数。
                        assert(replacedItemDeleteIter);
                        multiTxn->includeCurrentFee =
                            std::next(workingIter) !=
                                txQAcct.transactions.end();
                        continue;
                    }
                    if (!workingIter->second.consequences)
                        workingIter->second.consequences.emplace(
                            calculateConsequences(
                                *workingIter->second.pfresult));
//不要担心TXS的阻滞剂状态
//在电流之后。
                    if (workingIter->first < tSeq &&
                        workingIter->second.consequences->category ==
                            TxConsequences::blocker)
                    {
//删除当前事务，因为它
//被工作人员阻止。
                        JLOG(j_.trace()) <<
                            "Ignoring transaction " <<
                            transactionID <<
                            ". A blocker-type transaction " <<
                            "is in the queue.";
                        return{ telCAN_NOT_QUEUE_BLOCKED, false };
                    }
                    multiTxn->fee +=
                        workingIter->second.consequences->fee;
                    multiTxn->potentialSpend +=
                        workingIter->second.consequences->potentialSpend;
                }
                if (workingSeq < tSeq)
//在'tx'之前缺少事务。
                    multiTxn.reset();
            }

            if (multiTxn)
            {
                /*检查飞行中的总费用是否更高
                    比账户的当前余额，或
                    最低准备金。如果是，那就有风险
                    费用不会付，所以把这个放下
                    带有telcan-not-queue-balance结果的事务。
                    TODO:决定是否计算当前txn费用
                        如果它是
                        这个帐户。目前不算，
                        因为同样的原因，它没有被选中
                        第一笔交易。
                    假设：最低账户准备金为20 XRP。
                    例1：如果我有1000000个xrp，我可以排队
                        100万瑞波币的交易。在
                        同时，其他一些交易可能
                        降低我的余额（如接受报价）。什么时候？
                        事务执行，我将
                        花费1000000 XRP，或交易
                        会被卡在队列中
                        ‘特瑞苏费’
                    例2：如果我有1000000个xrp，并且我排队
                        10笔交易0.1 XRP费用，我有1 XRP
                        在飞行中。我现在可以用
                        999999 XRP费用。当前10次执行时，
                        他们保证会支付他们的费用，因为
                        没有什么可以吃进我的储备。最后
                        同样，事务将花费
                        999999 xrp，或卡在队列中。
                    例3：如果我有1000000个xrp，并且我排队
                        7笔交易，3个xrp费用，我有21个xrp
                        在飞行中。我不能再排队了，
                        不管费用有多小或多大。
                    阻塞在队列中的事务通过
                    lastledgerseq和maybetx:：retriessremaining。
                **/

                auto const balance = (*sle)[sfBalance].xrp();
                /*获得最低可能储备。如果费用超过
                   此金额，交易不能排队。
                   考虑到典型的费用是几个订单
                   小于任何电流或预期的量级
                   未来储量，这个计算比
                   试图找出
                   帐户可能发生的所有者计数
                   作为这些事务的结果，并删除
                   任何需要说明的其他交易
                   可能会影响排队时的所有者计数。
                **/

                auto const reserve = view.fees().accountReserve(0);
                auto totalFee = multiTxn->fee;
                if (multiTxn->includeCurrentFee)
                    totalFee += (*tx)[sfFee].xrp();
                if (totalFee >= balance ||
                    totalFee >= reserve)
                {
//删除当前交易记录
                    JLOG(j_.trace()) <<
                        "Ignoring transaction " <<
                        transactionID <<
                        ". Total fees in flight too high.";
                    return{ telCAN_NOT_QUEUE_BALANCE, false };
                }

//从当前视图创建测试视图
                multiTxn->applyView.emplace(&view, flags);
                multiTxn->openView.emplace(&*multiTxn->applyView);

                auto const sleBump = multiTxn->applyView->peek(
                    keylet::account(account));

                auto const potentialTotalSpend = multiTxn->fee +
                    std::min(balance - std::min(balance, reserve),
                    multiTxn->potentialSpend);
                assert(potentialTotalSpend > 0);
                sleBump->setFieldAmount(sfBalance,
                    balance - potentialTotalSpend);
                sleBump->setFieldU32(sfSequence, tSeq);
            }
        }
    }

//查看交易是否可能收取费用。
    assert(!multiTxn || multiTxn->openView);
    auto const pcresult = preclaim(pfresult, app,
            multiTxn ? *multiTxn->openView : view);
    if (!pcresult.likelyToClaimFee)
        return{ pcresult.ter, false };

//太低的费用应该被预先索赔抓住。
    assert(feeLevelPaid >= baseLevel);

    JLOG(j_.trace()) << "Transaction " <<
        transactionID <<
        " from account " << account <<
        " has fee level of " << feeLevelPaid <<
        " needs at least " << requiredFeeLevel <<
        " to get in the open ledger, which has " <<
        view.txCount() << " entries.";

    /*快速启发式检查看看是否值得检查
        Tx有足够高的费用来清除队列中的所有Tx。
        1）交易正在尝试进入未结分类账
        2）必须是队列中已存在的帐户。
        3）必须通过多重XN检查（Tx不是下一个
            account seq，跳过的seq在队列中，保留
            不会筋疲力尽。
        4）下一个事务之前一定没有尝试过并且失败
            应用于未结分类帐。
        5）Tx必须支付的费用超过所需的费用水平
            进入队列。
        6）费用水平必须高于违约水平（如果不是，
            那么第一个tx肯定没能处理“accept”
            因为其他原因。Tx允许排队，以防
            条件改变了，但不要浪费清理的努力）。
        7）无论费用水平如何，TX都不是免费/免费交易。
    **/

    if (!(flags & tapPREFER_QUEUE) && accountExists &&
        multiTxn.is_initialized() &&
        multiTxn->nextTxIter->second.retriesRemaining ==
            MaybeTx::retriesAllowed &&
                feeLevelPaid > requiredFeeLevel &&
                    requiredFeeLevel > baseLevel && baseFee != 0)
    {
        OpenView sandbox(open_ledger, &view, view.rules());

        auto result = tryClearAccountQueue(app, sandbox, *tx, accountIter,
            multiTxn->nextTxIter, feeLevelPaid, pfresult, view.txCount(),
                flags, metricsSnapshot, j);
        if (result.second)
        {
            sandbox.apply(view);
            /*无法删除此处的（*replacedItemDeleteIter），因为成功
                表示它已被删除。
            **/

            return result;
        }
    }

//交易记录可以进入未结分类帐吗？
    if (!multiTxn && feeLevelPaid >= requiredFeeLevel)
    {
//交易费用足以立即进入未结分类账。

        JLOG(j_.trace()) << "Applying transaction " <<
            transactionID <<
            " to open ledger.";
        ripple::TER txnResult;
        bool didApply;

        std::tie(txnResult, didApply) = doApply(pcresult, app, view);

        JLOG(j_.trace()) << "New transaction " <<
            transactionID <<
                (didApply ? " applied successfully with " :
                    " failed with ") <<
                        transToken(txnResult);

        if (didApply && replacedItemDeleteIter)
            erase(*replacedItemDeleteIter);
        return { txnResult, didApply };
    }

//如果“multitxn”有值，那么“canbeheald”已经过验证
    if (! multiTxn &&
        ! canBeHeld(*tx, view, accountIter, replacedItemDeleteIter))
    {
//保释，交易无法进行
        JLOG(j_.trace()) << "Transaction " <<
            transactionID <<
            " can not be held";
        return { telCAN_NOT_QUEUE, false };
    }

//如果队列已满，请决定是否删除当前队列
//交易或帐户的最后一个交易
//费用最低。
    if (!replacedItemDeleteIter && isFull())
    {
        auto lastRIter = byFee_.rbegin();
        if (lastRIter->account == account)
        {
            JLOG(j_.warn()) << "Queue is full, and transaction " <<
                transactionID <<
                " would kick a transaction from the same account (" <<
                account << ") out of the queue.";
            return { telCAN_NOT_QUEUE_FULL, false };
        }
        auto const& endAccount = byAccount_.at(lastRIter->account);
        auto endEffectiveFeeLevel = [&]()
        {
//计算Endaccount的所有Txs的平均值，
//但只有当队列中的最后一个Tx的费用较低时
//比这个候选德克萨斯州的水平。
            if (lastRIter->feeLevel > feeLevelPaid
                || endAccount.transactions.size() == 1)
                return lastRIter->feeLevel;

            constexpr std::uint64_t max =
                std::numeric_limits<std::uint64_t>::max();
            auto endTotal = std::accumulate(endAccount.transactions.begin(),
                endAccount.transactions.end(),
                    std::pair<std::uint64_t, std::uint64_t>(0, 0),
                        [&](auto const& total, auto const& tx)
                        {
//检查是否溢出。
                            auto next = tx.second.feeLevel /
                                endAccount.transactions.size();
                            auto mod = tx.second.feeLevel %
                                endAccount.transactions.size();
                            if (total.first >= max - next ||
                                    total.second >= max - mod)
                                return std::make_pair(max, std::uint64_t(0));
                            return std::make_pair(total.first + next, total.second + mod);
                        });
            return endTotal.first + endTotal.second /
                endAccount.transactions.size();
        }();
        if (feeLevelPaid > endEffectiveFeeLevel)
        {
//队列已满，此事务更多
//很有价值，所以把最便宜的交易踢出去。
            auto dropRIter = endAccount.transactions.rbegin();
            assert(dropRIter->second.account == lastRIter->account);
            JLOG(j_.warn()) <<
                "Removing last item of account " <<
                lastRIter->account <<
                " from queue with average fee of " <<
                endEffectiveFeeLevel << " in favor of " <<
                transactionID << " with fee of " <<
                feeLevelPaid;
            erase(byFee_.iterator_to(dropRIter->second));
        }
        else
        {
            JLOG(j_.warn()) << "Queue is full, and transaction " <<
                transactionID <<
                " fee is lower than end item's account average fee";
            return { telCAN_NOT_QUEUE_FULL, false };
        }
    }

//将事务保存在队列中。
    if (replacedItemDeleteIter)
        erase(*replacedItemDeleteIter);
    if (!accountExists)
    {
//创建新的txqaccount对象并添加byaccount查找。
        bool created;
        std::tie(accountIter, created) = byAccount_.emplace(
            account, TxQAccount(tx));
        (void)created;
        assert(created);
    }
//修改从队列中出来时使用的标志。
//这些变化可能导致额外的“飞行前”，但只要
//“hashrouter”仍然知道事务、签名
//不会再检查，所以成本应该是最低的。

//不允许软故障，这可能导致重试
    flags &= ~tapRETRY;

//不要排队，因为我们已经在排队了
    flags &= ~tapPREFER_QUEUE;

    auto& candidate = accountIter->second.add(
        { tx, transactionID, feeLevelPaid, flags, pfresult });
    /*通常我们会推迟到
        后来有事情需要我们去做，但是如果我们知道
        现在的后果，留到以后再说。
    **/

    if (consequences)
        candidate.consequences.emplace(*consequences);
//然后将其索引到ByFee查找中。
    byFee_.insert(candidate);
    JLOG(j_.debug()) << "Added transaction " << candidate.txID <<
        " with result " << transToken(pfresult.ter) <<
        " from " << (accountExists ? "existing" : "new") <<
            " account " << candidate.account << " to queue." <<
            " Flags: " << flags;

    return { terQUEUED, false };
}

/*
    0。是否启用了“FeatureFeeScallation”？
        是：继续下一步。
        不：停下来。
    1。根据
        确认的分类账中的TXS以及是否达成一致意见
        慢。
    2。将最大队列大小调整为足以容纳
        '分类帐查询'分类帐。
    三。从队列中删除
        “LastLedgerSequence”已通过。
    4。删除没有候选的任何帐户对象
        在他们下面。

**/

void
TxQ::processClosedLedger(Application& app,
    ReadView const& view, bool timeLeap)
{
    std::lock_guard<std::mutex> lock(mutex_);

    feeMetrics_.update(app, view, timeLeap, setup_);
    auto const& snapshot = feeMetrics_.getSnapshot();

    auto ledgerSeq = view.info().seq;

    if (!timeLeap)
        maxSize_ = std::max (snapshot.txnsExpected * setup_.ledgersInQueue,
             setup_.queueSizeMin);

//删除最后一个分类序列已过的所有排队的候选者。
    for(auto candidateIter = byFee_.begin(); candidateIter != byFee_.end(); )
    {
        if (candidateIter->lastValid
            && *candidateIter->lastValid <= ledgerSeq)
        {
            byAccount_.at(candidateIter->account).dropPenalty = true;
            candidateIter = erase(candidateIter);
        }
        else
        {
            ++candidateIter;
        }
    }

//删除所有没有候选项的txqaccounts
//在他们下面
    for (auto txQAccountIter = byAccount_.begin();
        txQAccountIter != byAccount_.end();)
    {
        if (txQAccountIter->second.empty())
            txQAccountIter = byAccount_.erase(txQAccountIter);
        else
            ++txQAccountIter;
    }
}

/*
    如何将TXS从队列移动到新的未结分类帐。

    0。是否启用了“FeatureFeeScallation”？
        是：继续下一步。
        不：不要对未结分类帐做任何操作。停下来。
    1。从最高收费水平到最低收费水平迭代TXS。
        对于每个TX：
        a）这是该帐户队列中的第一个Tx吗？
            不：跳过这个德克萨斯州。我们稍后再谈。
            是：继续下一步。
        b）Tx费用水平是否低于当前要求？
                收费水平？
            是：停止迭代。继续下一步。
            否：尝试应用事务。是否适用？
                是的：把它从队列中拿出来。继续
                    下一个合适的候选人（见下文）。
                没有：是不是有tef，tem，tel，还是有？
                        retried`maybetx:：retriesallowed`
                        时代已经来临了？
                    是的：把它从队列中拿出来。继续
                        下一个合适的候选人
                        （见下文）。
                    否：将其留在队列中，跟踪重试次数，
                        继续迭代。
    2。返回未结分类帐是否已修改的指示器。

    “适当的候选人”定义为具有
        最高收费水平：
        *下一个序列的当前账户的Tx。
        *队列中的下一个Tx，只需按费用订购即可。
**/

bool
TxQ::accept(Application& app,
    OpenView& view)
{
    /*将事务从队列中的最大费用级别移动到最小费用级别。
       随着我们添加更多交易，所需费用水平将增加。
       当交易费用水平低于所需费用时停止
       水平。
    **/


    auto ledgerChanged = false;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const metricSnapshot = feeMetrics_.getSnapshot();

    for (auto candidateIter = byFee_.begin(); candidateIter != byFee_.end();)
    {
        auto& account = byAccount_.at(candidateIter->account);
        if (candidateIter->sequence >
            account.transactions.begin()->first)
        {
//这不是这个账户的第一笔交易，所以跳过它。
//它还不能成功。
            JLOG(j_.trace()) << "Skipping queued transaction " <<
                candidateIter->txID << " from account " <<
                candidateIter->account << " as it is not the first.";
            candidateIter++;
            continue;
        }
        auto const requiredFeeLevel = FeeMetrics::scaleFeeLevel(
            metricSnapshot, view);
        auto const feeLevelPaid = candidateIter->feeLevel;
        JLOG(j_.trace()) << "Queued transaction " <<
            candidateIter->txID << " from account " <<
            candidateIter->account << " has fee level of " <<
            feeLevelPaid << " needs at least " <<
            requiredFeeLevel;
        if (feeLevelPaid >= requiredFeeLevel)
        {
            auto firstTxn = candidateIter->txn;

            JLOG(j_.trace()) << "Applying queued transaction " <<
                candidateIter->txID << " to open ledger.";

            TER txnResult;
            bool didApply;
            std::tie(txnResult, didApply) = candidateIter->apply(app, view, j_);

            if (didApply)
            {
//从队列中删除候选人
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID <<
                    " applied successfully with " <<
                    transToken(txnResult) << ". Remove from queue.";

                candidateIter = eraseAndAdvance(candidateIter);
                ledgerChanged = true;
            }
            else if (isTefFailure(txnResult) || isTemMalformed(txnResult) ||
                candidateIter->retriesRemaining <= 0)
            {
                if (candidateIter->retriesRemaining <= 0)
                    account.retryPenalty = true;
                else
                    account.dropPenalty = true;
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID << " failed with " <<
                    transToken(txnResult) << ". Remove from queue.";
                candidateIter = eraseAndAdvance(candidateIter);
            }
            else
            {
                JLOG(j_.debug()) << "Queued transaction " <<
                    candidateIter->txID << " failed with " <<
                    transToken(txnResult) << ". Leave in queue." <<
                    " Applied: " << didApply <<
                    ". Flags: " <<
                    candidateIter->flags;
                if (account.retryPenalty &&
                        candidateIter->retriesRemaining > 2)
                    candidateIter->retriesRemaining = 1;
                else
                    --candidateIter->retriesRemaining;
                candidateIter->lastResult = txnResult;
                if (account.dropPenalty &&
                    account.transactions.size() > 1 && isFull<95>())
                {
                    /*队列已接近满，此帐户有多个
                        TXS排队，此帐户有一个事务
                        失败了。即使我们要再做一次交易
                        机会，机会是它不会恢复的。所以我们不能
                        更糟的是，删除这个账户的最后一笔交易。
                    **/

                    auto dropRIter = account.transactions.rbegin();
                    assert(dropRIter->second.account == candidateIter->account);
                    JLOG(j_.warn()) <<
                        "Queue is nearly full, and transaction " <<
                        candidateIter->txID << " failed with " <<
                        transToken(txnResult) <<
                        ". Removing last item of account " <<
                        account.account;
                    auto endIter = byFee_.iterator_to(dropRIter->second);
                    assert(endIter != candidateIter);
                    erase(endIter);

                }
                ++candidateIter;
            }

        }
        else
        {
            break;
        }
    }

    return ledgerChanged;
}

TxQ::Metrics
TxQ::getMetrics(OpenView const& view) const
{
    Metrics result;

    std::lock_guard<std::mutex> lock(mutex_);

    auto const snapshot = feeMetrics_.getSnapshot();

    result.txCount = byFee_.size();
    result.txQMaxSize = maxSize_;
    result.txInLedger = view.txCount();
    result.txPerLedger = snapshot.txnsExpected;
    result.referenceFeeLevel = baseLevel;
    result.minProcessingFeeLevel = isFull() ? byFee_.rbegin()->feeLevel + 1 :
        baseLevel;
    result.medFeeLevel = snapshot.escalationMultiplier;
    result.openLedgerFeeLevel = FeeMetrics::scaleFeeLevel(snapshot, view);

    return result;
}

auto
TxQ::getAccountTxs(AccountID const& account, ReadView const& view) const
    -> std::map<TxSeq, AccountTxDetails const>
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto accountIter = byAccount_.find(account);
    if (accountIter == byAccount_.end() ||
        accountIter->second.transactions.empty())
        return {};

    std::map<TxSeq, AccountTxDetails const> result;

    for (auto const& tx : accountIter->second.transactions)
    {
        result.emplace(tx.first, [&]
        {
            AccountTxDetails resultTx;
            resultTx.feeLevel = tx.second.feeLevel;
            if (tx.second.lastValid)
                resultTx.lastValid.emplace(*tx.second.lastValid);
            if (tx.second.consequences)
                resultTx.consequences.emplace(*tx.second.consequences);
            return resultTx;
        }());
    }
    return result;
}

auto
TxQ::getTxs(ReadView const& view) const
-> std::vector<TxDetails>
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (byFee_.empty())
        return {};

    std::vector<TxDetails> result;
    result.reserve(byFee_.size());

    for (auto const& tx : byFee_)
    {
        result.emplace_back([&]
        {
            TxDetails resultTx;
            resultTx.feeLevel = tx.feeLevel;
            if (tx.lastValid)
                resultTx.lastValid.emplace(*tx.lastValid);
            if (tx.consequences)
                resultTx.consequences.emplace(*tx.consequences);
            resultTx.account = tx.account;
            resultTx.txn = tx.txn;
            resultTx.retriesRemaining = tx.retriesRemaining;
            BOOST_ASSERT(tx.pfresult);
            resultTx.preflightResult = tx.pfresult->ter;
            if (tx.lastResult)
                resultTx.lastResult.emplace(*tx.lastResult);
            return resultTx;
        }());
    }
    return result;
}


Json::Value
TxQ::doRPC(Application& app) const
{
    using std::to_string;

    auto const view = app.openLedger().current();
    if (!view)
    {
        BOOST_ASSERT(false);
        return {};
    }

    auto const metrics = getMetrics(*view);

    Json::Value ret(Json::objectValue);

    auto& levels = ret[jss::levels] = Json::objectValue;

    ret[jss::ledger_current_index] = view->info().seq;
    ret[jss::expected_ledger_size] = to_string(metrics.txPerLedger);
    ret[jss::current_ledger_size] = to_string(metrics.txInLedger);
    ret[jss::current_queue_size] = to_string(metrics.txCount);
    if (metrics.txQMaxSize)
        ret[jss::max_queue_size] = to_string(*metrics.txQMaxSize);

    levels[jss::reference_level] = to_string(metrics.referenceFeeLevel);
    levels[jss::minimum_level] = to_string(metrics.minProcessingFeeLevel);
    levels[jss::median_level] = to_string(metrics.medFeeLevel);
    levels[jss::open_ledger_level] = to_string(metrics.openLedgerFeeLevel);

    auto const baseFee = view->fees().base;
    auto& drops = ret[jss::drops] = Json::Value();

//不关心溢出标志
    drops[jss::base_fee] = to_string(mulDiv(
        metrics.referenceFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    drops[jss::minimum_fee] = to_string(mulDiv(
        metrics.minProcessingFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    drops[jss::median_fee] = to_string(mulDiv(
        metrics.medFeeLevel, baseFee,
            metrics.referenceFeeLevel).second);
    auto escalatedFee = mulDiv(
        metrics.openLedgerFeeLevel, baseFee,
            metrics.referenceFeeLevel).second;
    if (mulDiv(escalatedFee, metrics.referenceFeeLevel,
            baseFee).second < metrics.openLedgerFeeLevel)
        ++escalatedFee;

    drops[jss::open_ledger_fee] = to_string(escalatedFee);

    return ret;
}

////////////////////////////////////////////////

TxQ::Setup
setup_TxQ(Config const& config)
{
    TxQ::Setup setup;
    auto const& section = config.section("transaction_queue");
    set(setup.ledgersInQueue, "ledgers_in_queue", section);
    set(setup.queueSizeMin, "minimum_queue_size", section);
    set(setup.retrySequencePercent, "retry_sequence_percent", section);
    set(setup.multiTxnPercent, "multi_txn_percent", section);
    set(setup.minimumEscalationMultiplier,
        "minimum_escalation_multiplier", section);
    set(setup.minimumTxnInLedger, "minimum_txn_in_ledger", section);
    set(setup.minimumTxnInLedgerSA,
        "minimum_txn_in_ledger_standalone", section);
    set(setup.targetTxnInLedger, "target_txn_in_ledger", section);
    std::uint32_t max;
    if (set(max, "maximum_txn_in_ledger", section))
        setup.maximumTxnInLedger.emplace(max);

    /*数学的计算结果与预期的一样，适用于任何小于或等于
       最大值，但对这个百分比设置合理的限制，以便
       无法将该因素配置为有效呈现升级
       莫特。（还有其他方法，包括
       分类帐中的最小值。）
    **/

    set(setup.normalConsensusIncreasePercent,
        "normal_consensus_increase_percent", section);
    setup.normalConsensusIncreasePercent = boost::algorithm::clamp(
        setup.normalConsensusIncreasePercent, 0, 1000);

    /*如果此百分比超出0-100范围，则结果
       是无意义的（uint溢出发生，因此限制增大
       而不是收缩）。不建议使用0。
    **/

    set(setup.slowConsensusDecreasePercent, "slow_consensus_decrease_percent",
        section);
    setup.slowConsensusDecreasePercent = boost::algorithm::clamp(
        setup.slowConsensusDecreasePercent, 0, 100);

    set(setup.maximumTxnPerAccount, "maximum_txn_per_account", section);
    set(setup.minimumLastLedgerBuffer,
        "minimum_last_ledger_buffer", section);
    set(setup.zeroBaseFeeTransactionFeeLevel,
        "zero_basefee_transaction_feelevel", section);

    setup.standAlone = config.standalone();
    return setup;
}


std::unique_ptr<TxQ>
make_TxQ(TxQ::Setup const& setup, beast::Journal j)
{
    return std::make_unique<TxQ>(setup, std::move(j));
}

} //涟漪
