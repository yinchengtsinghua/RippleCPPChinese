
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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/ledger/PendingSaves.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/TaggedCache.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/Peer.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/resource/Fees.h>
#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace ripple {

using namespace std::chrono_literals;

//不要超过100个分类账（不能超过256个）
#define MAX_LEDGER_GAP          100

//如果分类帐太旧，不要获取历史记录
auto constexpr MAX_LEDGER_AGE_ACQUIRE = 1min;

//如果写入负载太高，不要获取历史记录
auto constexpr MAX_WRITE_LOAD_ACQUIRE = 8192;

LedgerMaster::LedgerMaster (Application& app, Stopwatch& stopwatch,
    Stoppable& parent,
    beast::insight::Collector::ptr const& collector, beast::Journal journal)
    : Stoppable ("LedgerMaster", parent)
    , app_ (app)
    , m_journal (journal)
    , mLedgerHistory (collector, app)
    , mLedgerCleaner (detail::make_LedgerCleaner (
        app, *this, app_.journal("LedgerCleaner")))
    , standalone_ (app_.config().standalone())
    , fetch_depth_ (app_.getSHAMapStore ().clampFetchDepth (
        app_.config().FETCH_DEPTH))
    , ledger_history_ (app_.config().LEDGER_HISTORY)
    , ledger_fetch_size_ (app_.config().getSize (siLedgerFetch))
    , fetch_packs_ ("FetchPack", 65536, 45s, stopwatch,
        app_.journal("TaggedCache"))
{
}

LedgerIndex
LedgerMaster::getCurrentLedgerIndex ()
{
    return app_.openLedger().current()->info().seq;
}

LedgerIndex
LedgerMaster::getValidLedgerIndex ()
{
    return mValidLedgerSeq;
}

bool
LedgerMaster::isCompatible (
    ReadView const& view,
    beast::Journal::Stream s,
    char const* reason)
{
    auto validLedger = getValidatedLedger();

    if (validLedger &&
        ! areCompatible (*validLedger, view, s, reason))
    {
        return false;
    }

    {
        ScopedLockType sl (m_mutex);

        if ((mLastValidLedger.second != 0) &&
            ! areCompatible (mLastValidLedger.first,
                mLastValidLedger.second, view, s, reason))
        {
            return false;
        }
    }

    return true;
}

std::chrono::seconds
LedgerMaster::getPublishedLedgerAge()
{
    std::chrono::seconds pubClose{mPubLedgerClose.load()};
    if (pubClose == 0s)
    {
        JLOG (m_journal.debug()) << "No published ledger";
        return weeks{2};
    }

    std::chrono::seconds ret = app_.timeKeeper().closeTime().time_since_epoch();
    ret -= pubClose;
    ret = (ret > 0s) ? ret : 0s;

    JLOG (m_journal.trace()) << "Published ledger age is " << ret.count();
    return ret;
}

std::chrono::seconds
LedgerMaster::getValidatedLedgerAge()
{
    std::chrono::seconds valClose{mValidLedgerSign.load()};
    if (valClose == 0s)
    {
        JLOG (m_journal.debug()) << "No validated ledger";
        return weeks{2};
    }

    std::chrono::seconds ret = app_.timeKeeper().closeTime().time_since_epoch();
    ret -= valClose;
    ret = (ret > 0s) ? ret : 0s;

    JLOG (m_journal.trace()) << "Validated ledger age is " << ret.count();
    return ret;
}

bool
LedgerMaster::isCaughtUp(std::string& reason)
{
    if (getPublishedLedgerAge() > 3min)
    {
        reason = "No recently-published ledger";
        return false;
    }
    std::uint32_t validClose = mValidLedgerSign.load();
    std::uint32_t pubClose = mPubLedgerClose.load();
    if (!validClose || !pubClose)
    {
        reason = "No published ledger";
        return false;
    }
    if (validClose  > (pubClose + 90))
    {
        reason = "Published ledger lags validated ledger";
        return false;
    }
    return true;
}

void
LedgerMaster::setValidLedger(
    std::shared_ptr<Ledger const> const& l)
{

    std::vector <NetClock::time_point> times;
    boost::optional<uint256> consensusHash;

    if (! standalone_)
    {
        auto const vals = app_.getValidations().getTrustedForLedger(l->info().hash);
        times.reserve(vals.size());
        for(auto const& val: vals)
            times.push_back(val->getSignTime());

        if(!vals.empty())
            consensusHash = vals.front()->getConsensusHash();
    }

    NetClock::time_point signTime;

    if (! times.empty () && times.size() >= app_.validators ().quorum ())
    {
//计算样本中位数
        std::sort (times.begin (), times.end ());
        auto const t0 = times[(times.size() - 1) / 2];
        auto const t1 = times[times.size() / 2];
        signTime = t0 + (t1 - t0)/2;
    }
    else
    {
        signTime = l->info().closeTime;
    }

    mValidLedger.set (l);
    mValidLedgerSign = signTime.time_since_epoch().count();
    assert (mValidLedgerSeq ||
            !app_.getMaxDisallowedLedger() ||
            l->info().seq + max_ledger_difference_ >
                    app_.getMaxDisallowedLedger());
    (void) max_ledger_difference_;
    mValidLedgerSeq = l->info().seq;

    app_.getOPs().updateLocalTx (*l);
    app_.getSHAMapStore().onLedgerClosed (getValidatedLedger());
    mLedgerHistory.validatedLedger (l, consensusHash);
    app_.getAmendmentTable().doValidatedLedger (l);
    if (!app_.getOPs().isAmendmentBlocked() &&
        app_.getAmendmentTable().hasUnsupportedEnabled ())
    {
        JLOG (m_journal.error()) <<
            "One or more unsupported amendments activated: server blocked.";
        app_.getOPs().setAmendmentBlocked();
    }
}

void
LedgerMaster::setPubLedger(
    std::shared_ptr<Ledger const> const& l)
{
    mPubLedger = l;
    mPubLedgerClose = l->info().closeTime.time_since_epoch().count();
    mPubLedgerSeq = l->info().seq;
}

void
LedgerMaster::addHeldTransaction (
    std::shared_ptr<Transaction> const& transaction)
{
    ScopedLockType ml (m_mutex);
    mHeldTransactions.insert (transaction->getSTransaction ());
}

//验证分类帐的关闭时间和序列号，如果我们正在考虑
//跳到那个分类帐上。这有助于防御一些罕见的敌意或
//疯狂的多数情况。
bool
LedgerMaster::canBeCurrent (std::shared_ptr<Ledger const> const& ledger)
{
    assert (ledger);

//千万不要跳到我们前面的候选分类账上。
//上次验证的分类帐

    auto validLedger = getValidatedLedger();
    if (validLedger &&
        (ledger->info().seq < validLedger->info().seq))
    {
        JLOG (m_journal.trace()) << "Candidate for current ledger has low seq "
            << ledger->info().seq << " < " << validLedger->info().seq;
        return false;
    }

//确保此分类帐的父分类帐关闭时间在
//我们现在的时间。如果我们已经有了一个已知的完全有效的分类账
//我们进行检查。否则，我们只有在
//当我们第一次启动的时候，很少有像我们的时钟这样的分类帐可以关闭。

    auto closeTime = app_.timeKeeper().closeTime();
    auto ledgerClose = ledger->info().parentCloseTime;

    if ((validLedger || (ledger->info().seq > 10)) &&
        ((std::max (closeTime, ledgerClose) - std::min (closeTime, ledgerClose))
            > 5min))
    {
        JLOG (m_journal.warn()) << "Candidate for current ledger has close time "
            << to_string(ledgerClose) << " at network time "
            << to_string(closeTime) << " seq " << ledger->info().seq;
        return false;
    }

    if (validLedger)
    {
//序列号不能太高。我们允许十个分类帐
//对于时间不准确加上一个分类账的最大运行率
//每两秒钟。目的是防止恶意分类账
//从增加我们的顺序不合理的高

        LedgerIndex maxSeq = validLedger->info().seq + 10;

        if (closeTime > validLedger->info().parentCloseTime)
            maxSeq += std::chrono::duration_cast<std::chrono::seconds>(
                closeTime - validLedger->info().parentCloseTime).count() / 2;

        if (ledger->info().seq > maxSeq)
        {
            JLOG (m_journal.warn()) << "Candidate for current ledger has high seq "
                << ledger->info().seq << " > " << maxSeq;
            return false;
        }

        JLOG (m_journal.trace()) << "Acceptable seq range: " <<
            validLedger->info().seq << " <= " <<
            ledger->info().seq << " <= " << maxSeq;
    }

    return true;
}

void
LedgerMaster::switchLCL(std::shared_ptr<Ledger const> const& lastClosed)
{
    assert (lastClosed);
    if(! lastClosed->isImmutable())
        LogicError("mutable ledger in switchLCL");

    if (lastClosed->open())
        LogicError ("The new last closed ledger is open!");

    {
        ScopedLockType ml (m_mutex);
        mClosedLedger.set (lastClosed);
    }

    if (standalone_)
    {
        setFullLedger (lastClosed, true, false);
        tryAdvance();
    }
    else
    {
        checkAccept (lastClosed);
    }
}

bool
LedgerMaster::fixIndex (LedgerIndex ledgerIndex, LedgerHash const& ledgerHash)
{
    return mLedgerHistory.fixIndex (ledgerIndex, ledgerHash);
}

bool
LedgerMaster::storeLedger (std::shared_ptr<Ledger const> ledger)
{
//如果我们已经有了分类帐，则返回true
    return mLedgerHistory.insert(std::move(ledger), false);
}

/*将保留的交易记录应用于未结分类帐
    这通常在我们关闭分类帐时调用。
    未结分类帐保持未结状态以处理新交易记录
    直到建立新的未结分类帐。
**/

void
LedgerMaster::applyHeldTransactions ()
{
    ScopedLockType sl (m_mutex);

    app_.openLedger().modify(
        [&](OpenView& view, beast::Journal j)
        {
            bool any = false;
            for (auto const& it : mHeldTransactions)
            {
                ApplyFlags flags = tapNONE;
                auto const result = app_.getTxQ().apply(
                    app_, view, it.second, flags, j);
                if (result.second)
                    any = true;
            }
            return any;
        });

//vfalc todo重新创建canonicaltxset对象，而不是重置
//它。
//vfalc注意，未结分类帐的哈希未定义，因此我们使用
//合理的替代品。
    mHeldTransactions.reset (
        app_.openLedger().current()->info().parentHash);
}

std::vector<std::shared_ptr<STTx const>>
LedgerMaster::pruneHeldTransactions(AccountID const& account,
    std::uint32_t const seq)
{
    ScopedLockType sl(m_mutex);

    return mHeldTransactions.prune(account, seq);
}

LedgerIndex
LedgerMaster::getBuildingLedger ()
{
//我们目前正在建立的分类帐，0个
    return mBuildingLedgerSeq.load ();
}

void
LedgerMaster::setBuildingLedger (LedgerIndex i)
{
    mBuildingLedgerSeq.store (i);
}

bool
LedgerMaster::haveLedger (std::uint32_t seq)
{
    ScopedLockType sl (mCompleteLock);
    return boost::icl::contains(mCompleteLedgers, seq);
}

void
LedgerMaster::clearLedger (std::uint32_t seq)
{
    ScopedLockType sl (mCompleteLock);
    mCompleteLedgers.erase (seq);
}

//返回分类帐我们有所有的节点
bool
LedgerMaster::getFullValidatedRange (std::uint32_t& minVal, std::uint32_t& maxVal)
{
//验证的分类帐可能尚未存储在数据库中，因此我们使用
//已发布的分类帐。
    maxVal = mPubLedgerSeq.load();

    if (!maxVal)
        return false;

    boost::optional<std::uint32_t> maybeMin;
    {
        ScopedLockType sl (mCompleteLock);
        maybeMin = prevMissing(mCompleteLedgers, maxVal);
    }

    if (maybeMin == boost::none)
        minVal = maxVal;
    else
        minVal = 1 + *maybeMin;

    return true;
}

//返回分类帐我们有所有的节点，并为其编制了索引
bool
LedgerMaster::getValidatedRange (std::uint32_t& minVal, std::uint32_t& maxVal)
{
    if (!getFullValidatedRange(minVal, maxVal))
        return false;

//从已验证的范围中删除任何可能不是
//数据库中已完全更新

    auto const pendingSaves =
        app_.pendingSaves().getSnapshot();

    if (!pendingSaves.empty() && ((minVal != 0) || (maxVal != 0)))
    {
//确保我们尽可能地缩小尖端。如果我们有7-9和
//8,9无效，我们不想看到8，缩小到9
//因为那样我们就什么都没有了。
        while (pendingSaves.count(maxVal) > 0)
            --maxVal;
        while (pendingSaves.count(minVal) > 0)
            ++minVal;

//尽最大努力保留例外
        for(auto v : pendingSaves)
        {
            if ((v.first >= minVal) && (v.first <= maxVal))
            {
                if (v.first > ((minVal + maxVal) / 2))
                    maxVal = v.first - 1;
                else
                    minVal = v.first + 1;
            }
        }

        if (minVal > maxVal)
            minVal = maxVal = 0;
    }

    return true;
}

//获取最早的分类账，我们将让同行获取
std::uint32_t
LedgerMaster::getEarliestFetch()
{
//我们让人们取的最早的分类账是零分类账，
//除非这会造成比允许范围更大的范围
    std::uint32_t e = getClosedLedger()->info().seq;

    if (e > fetch_depth_)
        e -= fetch_depth_;
    else
        e = 0;
    return e;
}

void
LedgerMaster::tryFill (
    Job& job,
    std::shared_ptr<Ledger const> ledger)
{
    std::uint32_t seq = ledger->info().seq;
    uint256 prevHash = ledger->info().parentHash;

    std::map< std::uint32_t, std::pair<uint256, uint256> > ledgerHashes;

    std::uint32_t minHas = seq;
    std::uint32_t maxHas = seq;

    NodeStore::Database& nodeStore {app_.getNodeStore()};
    while (! job.shouldCancel() && seq > 0)
    {
        {
            ScopedLockType ml (m_mutex);
            minHas = seq;
            --seq;

            if (haveLedger (seq))
                break;
        }

        auto it (ledgerHashes.find (seq));

        if (it == ledgerHashes.end ())
        {
            if (app_.isShutdown ())
                return;

            {
                ScopedLockType ml(mCompleteLock);
                mCompleteLedgers.insert(range(minHas, maxHas));
            }
            maxHas = minHas;
            ledgerHashes = getHashesByIndex ((seq < 500)
                ? 0
                : (seq - 499), seq, app_);
            it = ledgerHashes.find (seq);

            if (it == ledgerHashes.end ())
                break;

            if (!nodeStore.fetch(ledgerHashes.begin()->second.first,
                ledgerHashes.begin()->first))
            {
//节点存储不支持分类帐
                JLOG(m_journal.warn()) <<
                    "SQL DB ledger sequence " << seq <<
                    " mismatches node store";
                break;
            }
        }

        if (it->second.first != prevHash)
            break;

        prevHash = it->second.second;
    }

    {
        ScopedLockType ml (mCompleteLock);
        mCompleteLedgers.insert(range(minHas, maxHas));
    }
    {
        ScopedLockType ml (m_mutex);
        mFillInProgress = 0;
        tryAdvance();
    }
}

/*请求获取包以获取指定的分类帐
**/

void
LedgerMaster::getFetchPack (LedgerIndex missing,
    InboundLedger::Reason reason)
{
    auto haveHash {getLedgerHashForHistory(missing + 1, reason)};
    if (!haveHash || haveHash->isZero())
    {
        if (reason == InboundLedger::Reason::SHARD)
        {
            auto const shardStore {app_.getShardStore()};
            auto const shardIndex {shardStore->seqToShardIndex(missing)};
            if (missing < shardStore->lastLedgerSeq(shardIndex))
            {
                 JLOG(m_journal.error())
                    << "No hash for fetch pack. "
                    << "Missing ledger sequence " << missing
                    << " while acquiring shard " << shardIndex;
            }
        }
        else
        {
            JLOG(m_journal.error()) <<
                "No hash for fetch pack. Missing Index " << missing;
        }
        return;
    }

//根据最高分数选择目标对等。分数是随机的
//但倾向于低延迟的同龄人。
    std::shared_ptr<Peer> target;
    {
        int maxScore = 0;
        auto peerList = app_.overlay ().getActivePeers();
        for (auto const& peer : peerList)
        {
            if (peer->hasRange (missing, missing + 1))
            {
                int score = peer->getScore (true);
                if (! target || (score > maxScore))
                {
                    target = peer;
                    maxScore = score;
                }
            }
        }
    }

    if (target)
    {
        protocol::TMGetObjectByHash tmBH;
        tmBH.set_query (true);
        tmBH.set_type (protocol::TMGetObjectByHash::otFETCH_PACK);
        tmBH.set_ledgerhash (haveHash->begin(), 32);
        auto packet = std::make_shared<Message> (
            tmBH, protocol::mtGET_OBJECTS);

        target->send (packet);
        JLOG(m_journal.trace()) << "Requested fetch pack for " << missing;
    }
    else
        JLOG (m_journal.debug()) << "No peer for fetch pack";
}

void
LedgerMaster::fixMismatch (ReadView const& ledger)
{
    int invalidate = 0;
    boost::optional<uint256> hash;

    for (std::uint32_t lSeq = ledger.info().seq - 1; lSeq > 0; --lSeq)
    {
        if (haveLedger (lSeq))
        {
            try
            {
                hash = hashOfSeq(ledger, lSeq, m_journal);
            }
            catch (std::exception const&)
            {
                JLOG (m_journal.warn()) <<
                    "fixMismatch encounters partial ledger";
                clearLedger(lSeq);
                return;
            }

            if (hash)
            {
//试着缝合接缝
                auto otherLedger = getLedgerBySeq (lSeq);

                if (otherLedger && (otherLedger->info().hash == *hash))
                {
//我们合上了缝
                    if (invalidate != 0)
                    {
                        JLOG (m_journal.warn())
                            << "Match at " << lSeq
                            << ", " << invalidate
                            << " prior ledgers invalidated";
                    }

                    return;
                }
            }

            clearLedger (lSeq);
            ++invalidate;
        }
    }

//所有以前的分类账无效
    if (invalidate != 0)
    {
        JLOG (m_journal.warn()) <<
            "All " << invalidate << " prior ledgers invalidated";
    }
}

void
LedgerMaster::setFullLedger (
    std::shared_ptr<Ledger const> const& ledger,
        bool isSynchronous, bool isCurrent)
{
//新的分类帐已被接受为受信任链的一部分
    JLOG (m_journal.debug()) <<
        "Ledger " << ledger->info().seq <<
        " accepted :" << ledger->info().hash;
    assert (ledger->stateMap().getHash ().isNonZero ());

    ledger->setValidated();
    ledger->setFull();

    if (isCurrent)
        mLedgerHistory.insert(ledger, true);

    {
//检查SQL数据库的条目中是否有此之前的序列
//分类帐，如果不是该分类帐的父级，则使其无效
        uint256 prevHash = getHashByIndex (ledger->info().seq - 1, app_);
        if (prevHash.isNonZero () && prevHash != ledger->info().parentHash)
            clearLedger (ledger->info().seq - 1);
    }


    pendSaveValidated (app_, ledger, isSynchronous, isCurrent);

    {
        ScopedLockType ml (mCompleteLock);
        mCompleteLedgers.insert (ledger->info().seq);
    }

    {
        ScopedLockType ml (m_mutex);

        if (ledger->info().seq > mValidLedgerSeq)
            setValidLedger(ledger);
        if (!mPubLedger)
        {
            setPubLedger(ledger);
            app_.getOrderBookDB().setup(ledger);
        }

        if (ledger->info().seq != 0 && haveLedger (ledger->info().seq - 1))
        {
//我们想我们有上一个分类账，请核对一下。
            auto prevLedger = getLedgerBySeq (ledger->info().seq - 1);

            if (!prevLedger ||
                (prevLedger->info().hash != ledger->info().parentHash))
            {
                JLOG (m_journal.warn())
                    << "Acquired ledger invalidates previous ledger: "
                    << (prevLedger ? "hashMismatch" : "missingLedger");
                fixMismatch (*ledger);
            }
        }
    }
}

void
LedgerMaster::failedSave(std::uint32_t seq, uint256 const& hash)
{
    clearLedger(seq);
    app_.getInboundLedgers().acquire(
        hash, seq, InboundLedger::Reason::GENERIC);
}

//检查指定的分类帐是否可以成为上次完全验证的新分类帐
//分类帐。
void
LedgerMaster::checkAccept (uint256 const& hash, std::uint32_t seq)
{
    std::size_t valCount = 0;

    if (seq != 0)
    {
//分类帐太旧
        if (seq < mValidLedgerSeq)
            return;

        valCount =
            app_.getValidations().numTrustedForLedger (hash);

        if (valCount >= app_.validators ().quorum ())
        {
            ScopedLockType ml (m_mutex);
            if (seq > mLastValidLedger.second)
                mLastValidLedger = std::make_pair (hash, seq);
        }

        if (seq == mValidLedgerSeq)
            return;

//分类帐可以和我们已经建立的分类帐相匹配
        if (seq == mBuildingLedgerSeq)
            return;
    }

    auto ledger = mLedgerHistory.getLedgerByHash (hash);

    if (!ledger)
    {
        if ((seq != 0) && (getValidLedgerIndex() == 0))
        {
//如果可以的话，让同龄人早点清醒起来
            if (valCount >= app_.validators ().quorum ())
                app_.overlay().checkSanity (seq);
        }

//我们可能不想只取一本分类帐
//可信验证
        ledger = app_.getInboundLedgers().acquire(
            hash, seq, InboundLedger::Reason::GENERIC);
    }

    if (ledger)
        checkAccept (ledger);
}

/*
    *确定完全验证分类帐需要多少验证
    *
    *@返回需要的验证数
    **/

std::size_t
LedgerMaster::getNeededValidations ()
{
    return standalone_ ? 0 : app_.validators().quorum ();
}

void
LedgerMaster::checkAccept (
    std::shared_ptr<Ledger const> const& ledger)
{
//我们能接受这个分类帐作为我们最后一个完全有效的分类帐吗

    if (! canBeCurrent (ledger))
        return;

//我们可以预付最后一个完全有效的分类账吗？如果是这样，我们能
//发布？
    ScopedLockType ml (m_mutex);

    if (ledger->info().seq <= mValidLedgerSeq)
        return;

    auto const minVal = getNeededValidations();
    auto const tvc = app_.getValidations().numTrustedForLedger(ledger->info().hash);
if (tvc < minVal) //我们无能为力
    {
        JLOG (m_journal.trace()) <<
            "Only " << tvc <<
            " validations for " << ledger->info().hash;
        return;
    }

    JLOG (m_journal.info())
        << "Advancing accepted ledger to " << ledger->info().seq
        << " with >= " << minVal << " validations";

    mLastValidateHash = ledger->info().hash;
    mLastValidateSeq = ledger->info().seq;

    ledger->setValidated();
    ledger->setFull();
    setValidLedger(ledger);
    if (!mPubLedger)
    {
        pendSaveValidated(app_, ledger, true, true);
        setPubLedger(ledger);
        app_.getOrderBookDB().setup(ledger);
    }

    std::uint32_t const base = app_.getFeeTrack().getLoadBase();
    auto fees = app_.getValidations().fees (ledger->info().hash, base);
    {
        auto fees2 = app_.getValidations().fees (
            ledger->info(). parentHash, base);
        fees.reserve (fees.size() + fees2.size());
        std::copy (fees2.begin(), fees2.end(), std::back_inserter(fees));
    }
    std::uint32_t fee;
    if (! fees.empty())
    {
        std::sort (fees.begin(), fees.end());
fee = fees[fees.size() / 2]; //中值的
    }
    else
    {
        fee = base;
    }

    app_.getFeeTrack().setRemoteFee(fee);

    tryAdvance ();
}

/*报告共识过程建立了一个特定的分类账*/
void
LedgerMaster::consensusBuilt(
    std::shared_ptr<Ledger const> const& ledger,
    uint256 const& consensusHash,
    Json::Value consensus)
{

//因为我们刚刚建立了一个分类帐，我们不再建立一个分类帐。
    setBuildingLedger (0);

//无需在独立模式下处理验证
    if (standalone_)
        return;

    mLedgerHistory.builtLedger (ledger, consensusHash, std::move (consensus));

    if (ledger->info().seq <= mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").info();
        JLOG (stream)
            << "Consensus built old ledger: "
            << ledger->info().seq << " <= " << mValidLedgerSeq;
        return;
    }

//查看此分类帐是否可以是新的完全验证的分类帐
    checkAccept (ledger);

    if (ledger->info().seq <= mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").debug();
        JLOG (stream)
            << "Consensus ledger fully validated";
        return;
    }

//此分类帐不能是新的完全验证的分类帐，但是
//也许我们为其他分类账保存了验证

    auto const val =
        app_.getValidations().currentTrusted();

//使用序列号跟踪验证计数
    class valSeq
    {
        public:

        valSeq () : valCount_ (0), ledgerSeq_ (0) { ; }

        void mergeValidation (LedgerIndex seq)
        {
            valCount_++;

//如果我们还不知道序列，现在就知道了
            if (ledgerSeq_ == 0)
                ledgerSeq_ = seq;
        }

        std::size_t valCount_;
        LedgerIndex ledgerSeq_;
    };

//计算当前可信验证的数量
    hash_map <uint256, valSeq> count;
    for (auto const& v : val)
    {
        valSeq& vs = count[v->getLedgerHash()];
        vs.mergeValidation (v->getFieldU32 (sfLedgerSequence));
    }

    auto const neededValidations = getNeededValidations ();
    auto maxSeq = mValidLedgerSeq.load();
    auto maxLedger = ledger->info().hash;

//充分验证的分类账，
//找到序列最高的那个
    for (auto& v : count)
        if (v.second.valCount_ > neededValidations)
        {
//如果我们还不知道顺序，就去拿
            if (v.second.ledgerSeq_ == 0)
            {
                if (auto ledger = getLedgerByHash (v.first))
                    v.second.ledgerSeq_ = ledger->info().seq;
            }

            if (v.second.ledgerSeq_ > maxSeq)
            {
                maxSeq = v.second.ledgerSeq_;
                maxLedger = v.first;
            }
        }

    if (maxSeq > mValidLedgerSeq)
    {
        auto stream = app_.journal ("LedgerConsensus").debug();
        JLOG (stream)
            << "Consensus triggered check of ledger";
        checkAccept (maxLedger, maxSeq);
    }
}

void
LedgerMaster::advanceThread()
{
    ScopedLockType sl (m_mutex);
    assert (!mValidLedger.empty () && mAdvanceThread);

    JLOG (m_journal.trace()) << "advanceThread<";

    try
    {
        doAdvance(sl);
    }
    catch (std::exception const&)
    {
        JLOG (m_journal.fatal()) << "doAdvance throws an exception";
    }

    mAdvanceThread = false;
    JLOG (m_journal.trace()) << "advanceThread>";
}

boost::optional<LedgerHash>
LedgerMaster::getLedgerHashForHistory(
    LedgerIndex index, InboundLedger::Reason reason)
{
//尝试获取我们需要为历史获取的分类帐的哈希值
    boost::optional<LedgerHash> ret;
    auto const& l {reason == InboundLedger::Reason::SHARD ?
        mShardLedger : mHistLedger};

    if (l && l->info().seq >= index)
    {
        ret = hashOfSeq(*l, index, m_journal);
        if (! ret)
            ret = walkHashBySeq (index, l);
    }

    if (! ret)
        ret = walkHashBySeq (index);

    return ret;
}

std::vector<std::shared_ptr<Ledger const>>
LedgerMaster::findNewLedgersToPublish ()
{
    std::vector<std::shared_ptr<Ledger const>> ret;

    JLOG (m_journal.trace()) << "findNewLedgersToPublish<";

//没有有效的分类帐，不做任何事
    if (mValidLedger.empty ())
    {
        JLOG(m_journal.trace()) <<
            "No valid journal, nothing to publish.";
        return {};
    }

    if (! mPubLedger)
    {
        JLOG(m_journal.info()) <<
            "First published ledger will be " << mValidLedgerSeq;
        return { mValidLedger.get () };
    }

    if (mValidLedgerSeq > (mPubLedgerSeq + MAX_LEDGER_GAP))
    {
        JLOG(m_journal.warn()) <<
            "Gap in validated ledger stream " << mPubLedgerSeq <<
            " - " << mValidLedgerSeq - 1;

        auto valLedger = mValidLedger.get ();
        ret.push_back (valLedger);
        setPubLedger (valLedger);
        app_.getOrderBookDB().setup(valLedger);

        return { valLedger };
    }

    if (mValidLedgerSeq <= mPubLedgerSeq)
    {
        JLOG(m_journal.trace()) <<
            "No valid journal, nothing to publish.";
        return {};
    }

    int acqCount = 0;

auto pubSeq = mPubLedgerSeq + 1; //下一个要发布的序列
    auto valLedger = mValidLedger.get ();
    std::uint32_t valSeq = valLedger->info().seq;

    ScopedUnlockType sul(m_mutex);
    try
    {
        for (std::uint32_t seq = pubSeq; seq <= valSeq; ++seq)
        {
            JLOG(m_journal.trace())
                << "Trying to fetch/publish valid ledger " << seq;

            std::shared_ptr<Ledger const> ledger;
//这可以扔
            auto hash = hashOfSeq(*valLedger, seq, m_journal);
//
//使用。
            if (! hash)
hash = beast::zero; //克莱
            if (seq == valSeq)
            {
//我们需要发布我们刚刚完全验证过的分类账
                ledger = valLedger;
            }
            else if (hash->isZero())
            {
                JLOG (m_journal.fatal())
                    << "Ledger: " << valSeq
                    << " does not have hash for " << seq;
                assert (false);
            }
            else
            {
                ledger = mLedgerHistory.getLedgerByHash (*hash);
            }

//我们能设法取得我们需要的分类帐吗？
            if (! ledger && (++acqCount < ledger_fetch_size_))
                ledger = app_.getInboundLedgers ().acquire(
                    *hash, seq, InboundLedger::Reason::GENERIC);

//我们是否获得了下一个需要公布的分类账？
            if (ledger && (ledger->info().seq == pubSeq))
            {
                ledger->setValidated();
                ret.push_back (ledger);
                ++pubSeq;
            }
        }

        JLOG(m_journal.trace()) <<
            "ready to publish " << ret.size() << " ledgers.";
    }
    catch (std::exception const&)
    {
        JLOG(m_journal.error()) <<
            "Exception while trying to find ledgers to publish.";
    }

    return ret;
}

void
LedgerMaster::tryAdvance()
{
    ScopedLockType ml (m_mutex);

//至少有一个完全有效的分类帐才能预付
    mAdvanceWork = true;
    if (!mAdvanceThread && !mValidLedger.empty ())
    {
        mAdvanceThread = true;
        app_.getJobQueue ().addJob (
            jtADVANCE, "advanceLedger",
            [this] (Job&) { advanceThread(); });
    }
}

//返回具有特定序列的有效分类帐的哈希，给定
//后续分类帐已知有效。
boost::optional<LedgerHash>
LedgerMaster::getLedgerHash(
    std::uint32_t desiredSeq,
    std::shared_ptr<ReadView const> const& knownGoodLedger)
{
    assert(desiredSeq < knownGoodLedger->info().seq);

    auto hash = hashOfSeq(*knownGoodLedger, desiredSeq, m_journal);

//不直接在给定的分类帐中
    if (! hash)
    {
        std::uint32_t seq = (desiredSeq + 255) % 256;
        assert(seq < desiredSeq);

        hash = hashOfSeq(*knownGoodLedger, seq, m_journal);
        if (hash)
        {
            if (auto l = getLedgerByHash(*hash))
            {
                hash = hashOfSeq(*l, desiredSeq, m_journal);
                assert (hash);
            }
        }
        else
        {
            assert(false);
        }
    }

    return hash;
}

void
LedgerMaster::updatePaths (Job& job)
{
    {
        ScopedLockType ml (m_mutex);
        if (app_.getOPs().isNeedNetworkLedger())
        {
            --mPathFindThread;
            return;
        }
    }


    while (! job.shouldCancel())
    {
        std::shared_ptr<ReadView const> lastLedger;
        {
            ScopedLockType ml (m_mutex);

            if (!mValidLedger.empty() &&
                (!mPathLedger ||
                    (mPathLedger->info().seq != mValidLedgerSeq)))
{ //我们有一个新的有效的分类账，自从上次完整的寻路
                mPathLedger = mValidLedger.get ();
                lastLedger = mPathLedger;
            }
            else if (mPathFindNewRequest)
{ //我们有新的要求，但没有新的分类帐
                lastLedger = app_.openLedger().current();
            }
            else
{ //无事可做
                --mPathFindThread;
                return;
            }
        }

        if (!standalone_)
{ //别用超过60秒的账本找路
            using namespace std::chrono;
            auto age = time_point_cast<seconds>(app_.timeKeeper().closeTime())
                - lastLedger->info().closeTime;
            if (age > 1min)
            {
                JLOG (m_journal.debug())
                    << "Published ledger too old for updating paths";
                ScopedLockType ml (m_mutex);
                --mPathFindThread;
                return;
            }
        }

        try
        {
            app_.getPathRequests().updateAll(
                lastLedger, job.getCancelCallback());
        }
        catch (SHAMapMissingNode&)
        {
            JLOG (m_journal.info())
                << "Missing node detected during pathfinding";
            if (lastLedger->open())
            {
//我们的父母是问题所在
                app_.getInboundLedgers().acquire(
                    lastLedger->info().parentHash,
                    lastLedger->info().seq - 1,
                    InboundLedger::Reason::GENERIC);
            }
            else
            {
//这个账本有问题
                app_.getInboundLedgers().acquire(
                    lastLedger->info().hash,
                    lastLedger->info().seq,
                    InboundLedger::Reason::GENERIC);
            }
        }
    }
}

bool
LedgerMaster::newPathRequest ()
{
    ScopedLockType ml (m_mutex);
    mPathFindNewRequest = newPFWork("pf:newRequest", ml);
    return mPathFindNewRequest;
}

bool
LedgerMaster::isNewPathRequest ()
{
    ScopedLockType ml (m_mutex);
    bool const ret = mPathFindNewRequest;
    mPathFindNewRequest = false;
    return ret;
}

//如果订单簿被彻底更新，我们需要重新处理所有
//寻路请求。
bool
LedgerMaster::newOrderBookDB ()
{
    ScopedLockType ml (m_mutex);
    mPathLedger.reset();

    return newPFWork("pf:newOBDB", ml);
}

/*需要调度一个线程来处理某种类型的寻径工作。
**/

bool
LedgerMaster::newPFWork (const char *name, ScopedLockType&)
{
    if (mPathFindThread < 2)
    {
        if (app_.getJobQueue().addJob (
            jtUPDATE_PF, name,
            [this] (Job& j) { updatePaths(j); }))
        {
            ++mPathFindThread;
        }
    }
//如果我们停止通话，不要让来电者期望
//即使可以提供服务，也将满足请求。
    return mPathFindThread > 0 && !isStopping();
}

std::recursive_mutex&
LedgerMaster::peekMutex ()
{
    return m_mutex;
}

//当前分类账是我们认为新交易应该进入的分类账。
std::shared_ptr<ReadView const>
LedgerMaster::getCurrentLedger ()
{
    return app_.openLedger().current();
}

Rules
LedgerMaster::getValidatedRules ()
{
//一旦我们有了保证，总会有最后一次验证
//然后我们就可以不使用if了。

//从上次验证的分类帐返回规则。
    if (auto const ledger = getValidatedLedger())
        return ledger->rules();

    return Rules(app_.config().features);
}

//这是我们向客户发布的最后一个分类帐，可能会延迟验证
//分类帐。
std::shared_ptr<ReadView const>
LedgerMaster::getPublishedLedger ()
{
    ScopedLockType lock(m_mutex);
    return mPubLedger;
}

std::string
LedgerMaster::getCompleteLedgers ()
{
    ScopedLockType sl (mCompleteLock);
    return to_string(mCompleteLedgers);
}

boost::optional <NetClock::time_point>
LedgerMaster::getCloseTimeBySeq (LedgerIndex ledgerIndex)
{
    uint256 hash = getHashBySeq (ledgerIndex);
    return hash.isNonZero() ? getCloseTimeByHash(
        hash, ledgerIndex) : boost::none;
}

boost::optional <NetClock::time_point>
LedgerMaster::getCloseTimeByHash(LedgerHash const& ledgerHash,
    std::uint32_t index)
{
    auto node = app_.getNodeStore().fetch(ledgerHash, index);
    if (node &&
        (node->getData().size() >= 120))
    {
        SerialIter it (node->getData().data(), node->getData().size());
        if (it.get32() == HashPrefix::ledgerMaster)
        {
            it.skip (
4+8+32+    //seq删除parenthash
32+32+4);  //Txhash Accthash父关闭
            return NetClock::time_point{NetClock::duration{it.get32()}};
        }
    }

    return boost::none;
}

uint256
LedgerMaster::getHashBySeq (std::uint32_t index)
{
    uint256 hash = mLedgerHistory.getLedgerHash (index);

    if (hash.isNonZero ())
        return hash;

    return getHashByIndex (index, app_);
}

boost::optional<LedgerHash>
LedgerMaster::walkHashBySeq (std::uint32_t index)
{
    boost::optional<LedgerHash> ledgerHash;

    if (auto referenceLedger = mValidLedger.get ())
        ledgerHash = walkHashBySeq (index, referenceLedger);

    return ledgerHash;
}

boost::optional<LedgerHash>
LedgerMaster::walkHashBySeq (
    std::uint32_t index,
    std::shared_ptr<ReadView const> const& referenceLedger)
{
    if (!referenceLedger || (referenceLedger->info().seq < index))
    {
//我们无能为力。没有已验证的分类帐。
        return boost::none;
    }

//看看我们需要的分类账哈希是否在参考分类账中
    auto ledgerHash = hashOfSeq(*referenceLedger, index, m_journal);
    if (ledgerHash)
        return ledgerHash;

//哈希不在参考分类帐中。再拿一个分类帐
//易于定位，并应包含哈希。
    LedgerIndex refIndex = getCandidateLedger(index);
    auto const refHash = hashOfSeq(*referenceLedger, refIndex, m_journal);
    assert(refHash);
    if (refHash)
    {
//尝试刚才找到的更好的参考分类账的哈希和序列
        auto ledger = mLedgerHistory.getLedgerByHash (*refHash);

        if (ledger)
        {
            try
            {
                ledgerHash = hashOfSeq(*ledger, index, m_journal);
            }
            catch(SHAMapMissingNode&)
            {
                ledger.reset();
            }
        }

//尝试获取完整的分类帐
        if (!ledger)
        {
            auto const ledger = app_.getInboundLedgers().acquire (
                *refHash, refIndex, InboundLedger::Reason::GENERIC);
            if (ledger)
            {
                ledgerHash = hashOfSeq(*ledger, index, m_journal);
                assert (ledgerHash);
            }
        }
    }
    return ledgerHash;
}

std::shared_ptr<Ledger const>
LedgerMaster::getLedgerBySeq (std::uint32_t index)
{
    if (index <= mValidLedgerSeq)
    {
//总是喜欢经过验证的分类帐
        if (auto valid = mValidLedger.get ())
        {
            if (valid->info().seq == index)
                return valid;

            try
            {
                auto const hash = hashOfSeq(*valid, index, m_journal);

                if (hash)
                    return mLedgerHistory.getLedgerByHash (*hash);
            }
            catch (std::exception const&)
            {
//已处理丢失的节点
            }
        }
    }

    if (auto ret = mLedgerHistory.getLedgerBySeq (index))
        return ret;

    auto ret = mClosedLedger.get ();
    if (ret && (ret->info().seq == index))
        return ret;

    clearLedger (index);
    return {};
}

std::shared_ptr<Ledger const>
LedgerMaster::getLedgerByHash (uint256 const& hash)
{
    if (auto ret = mLedgerHistory.getLedgerByHash (hash))
        return ret;

    auto ret = mClosedLedger.get ();
    if (ret && (ret->info().hash == hash))
        return ret;

    return {};
}

void
LedgerMaster::doLedgerCleaner(Json::Value const& parameters)
{
    mLedgerCleaner->doClean (parameters);
}

void
LedgerMaster::setLedgerRangePresent (std::uint32_t minV, std::uint32_t maxV)
{
    ScopedLockType sl (mCompleteLock);
    mCompleteLedgers.insert(range(minV, maxV));
}

void
LedgerMaster::tune (int size, std::chrono::seconds age)
{
    mLedgerHistory.tune (size, age);
}

void
LedgerMaster::sweep ()
{
    mLedgerHistory.sweep ();
    fetch_packs_.sweep ();
}

float
LedgerMaster::getCacheHitRate ()
{
    return mLedgerHistory.getCacheHitRate ();
}

beast::PropertyStream::Source&
LedgerMaster::getPropertySource ()
{
    return *mLedgerCleaner;
}

void
LedgerMaster::clearPriorLedgers (LedgerIndex seq)
{
    ScopedLockType sl(mCompleteLock);
    if (seq > 0)
        mCompleteLedgers.erase(range(0u, seq - 1));
}

void
LedgerMaster::clearLedgerCachePrior (LedgerIndex seq)
{
    mLedgerHistory.clearLedgerCachePrior (seq);
}

void
LedgerMaster::takeReplay (std::unique_ptr<LedgerReplay> replay)
{
    replayData = std::move (replay);
}

std::unique_ptr<LedgerReplay>
LedgerMaster::releaseReplay ()
{
    return std::move (replayData);
}

bool
LedgerMaster::shouldAcquire (
    std::uint32_t const currentLedger,
    std::uint32_t const ledgerHistory,
    std::uint32_t const ledgerHistoryIndex,
    std::uint32_t const candidateLedger) const
{

//如果可能是当前分类帐，则提取分类帐，
//被建议删除设置请求，或
//在我们配置的历史范围内

    bool ret (candidateLedger >= currentLedger ||
        ((ledgerHistoryIndex > 0) &&
            (candidateLedger > ledgerHistoryIndex)) ||
        (currentLedger - candidateLedger) <= ledgerHistory);

    JLOG (m_journal.trace())
        << "Missing ledger "
        << candidateLedger
        << (ret ? " should" : " should NOT")
        << " be acquired";
    return ret;
}

void
LedgerMaster::fetchForHistory(
    std::uint32_t missing,
    bool& progress,
    InboundLedger::Reason reason)
{
    ScopedUnlockType sl(m_mutex);
    if (auto hash = getLedgerHashForHistory(missing, reason))
    {
        assert(hash->isNonZero());
        auto ledger = getLedgerByHash(*hash);
        if (! ledger)
        {
            if (!app_.getInboundLedgers().isFailure(*hash))
            {
                ledger = app_.getInboundLedgers().acquire(
                    *hash, missing, reason);
                if (!ledger &&
                    missing != fetch_seq_ &&
                    missing > app_.getNodeStore().earliestSeq())
                {
                    JLOG(m_journal.trace())
                        << "fetchForHistory want fetch pack " << missing;
                    fetch_seq_ = missing;
                    getFetchPack(missing, reason);
                }
                else
                    JLOG(m_journal.trace())
                        << "fetchForHistory no fetch pack for " << missing;
            }
            else
                JLOG(m_journal.debug())
                    << "fetchForHistory found failed acquire";
        }
        if (ledger)
        {
            auto seq = ledger->info().seq;
            assert(seq == missing);
            JLOG(m_journal.trace()) <<
                "fetchForHistory acquired " << seq;
            if (reason == InboundLedger::Reason::SHARD)
            {
                ledger->setFull();
                {
                    ScopedLockType lock(m_mutex);
                    mShardLedger = ledger;
                }
                if (!ledger->stateMap().family().isShardBacked())
                    app_.getShardStore()->copyLedger(ledger);
            }
            else
            {
                setFullLedger(ledger, false, false);
                int fillInProgress;
                {
                    ScopedLockType lock(m_mutex);
                    mHistLedger = ledger;
                    fillInProgress = mFillInProgress;
                }
                if (fillInProgress == 0 &&
                    getHashByIndex(seq - 1, app_) == ledger->info().parentHash)
                {
                    {
//上一个分类帐在数据库中
                        ScopedLockType lock(m_mutex);
                        mFillInProgress = seq;
                    }
                    app_.getJobQueue().addJob(jtADVANCE, "tryFill",
                        [this, ledger](Job& j) { tryFill(j, ledger); });
                }
            }
            progress = true;
        }
        else
        {
            std::uint32_t fetchSz;
            if (reason == InboundLedger::Reason::SHARD)
//不提取较低的分类帐序列
//比碎片的第一个分类帐序列
                fetchSz = app_.getShardStore()->firstLedgerSeq(
                    app_.getShardStore()->seqToShardIndex(missing));
            else
//不提取较低的分类帐序列
//比最早的分类帐序列
                fetchSz = app_.getNodeStore().earliestSeq();
            fetchSz = missing >= fetchSz ?
                std::min(ledger_fetch_size_, (missing - fetchSz) + 1) : 0;
            try
            {
                for (std::uint32_t i = 0; i < fetchSz; ++i)
                {
                    std::uint32_t seq = missing - i;
                    if (auto h = getLedgerHashForHistory(seq, reason))
                    {
                        assert(h->isNonZero());
                        app_.getInboundLedgers().acquire(*h, seq, reason);
                    }
                }
            }
            catch (std::exception const&)
            {
                JLOG(m_journal.warn()) << "Threw while prefetching";
            }
        }
    }
    else
    {
        JLOG(m_journal.fatal()) << "Can't find ledger following prevMissing "
                                << missing;
        JLOG(m_journal.fatal()) << "Pub:" << mPubLedgerSeq
                                << " Val:" << mValidLedgerSeq;
        JLOG(m_journal.fatal()) << "Ledgers: "
                                << app_.getLedgerMaster().getCompleteLedgers();
        JLOG(m_journal.fatal()) << "Acquire reason: "
            << (reason == InboundLedger::Reason::HISTORY ? "HISTORY" : "SHARD");
        clearLedger(missing + 1);
        progress = true;
    }
}

//尝试发布分类帐，获取丢失的分类帐
void LedgerMaster::doAdvance (ScopedLockType& sl)
{
    do
    {
mAdvanceWork = false; //如果有工作要做，我们会取得进展的
        bool progress = false;

        auto const pubLedgers = findNewLedgersToPublish ();
        if (pubLedgers.empty())
        {
            if (!standalone_ && !app_.getFeeTrack().isLoadedLocal() &&
                (app_.getJobQueue().getJobCount(jtPUBOLDLEDGER) < 10) &&
                (mValidLedgerSeq == mPubLedgerSeq) &&
                (getValidatedLedgerAge() < MAX_LEDGER_AGE_ACQUIRE) &&
                (app_.getNodeStore().getWriteLoad() < MAX_WRITE_LOAD_ACQUIRE))
            {
//我们是同步的，所以可以
                InboundLedger::Reason reason = InboundLedger::Reason::HISTORY;
                boost::optional<std::uint32_t> missing;
                {
                    ScopedLockType sl(mCompleteLock);
                    missing = prevMissing(mCompleteLedgers,
                        mPubLedger->info().seq,
                        app_.getNodeStore().earliestSeq());
                }
                if (missing)
                {
                    JLOG(m_journal.trace()) <<
                        "tryAdvance discovered missing " << *missing;
                    if ((mFillInProgress == 0 || *missing > mFillInProgress) &&
                        shouldAcquire(mValidLedgerSeq, ledger_history_,
                            app_.getSHAMapStore().getCanDelete(), *missing))
                    {
                        JLOG(m_journal.trace()) <<
                            "advanceThread should acquire";
                    }
                    else
                        missing = boost::none;
                }
                if (! missing && mFillInProgress == 0)
                {
                    if (auto shardStore = app_.getShardStore())
                    {
                        missing = shardStore->prepareLedger(mValidLedgerSeq);
                        if (missing)
                            reason = InboundLedger::Reason::SHARD;
                    }
                }
                if(missing)
                {
                    fetchForHistory(*missing, progress, reason);
                    if (mValidLedgerSeq != mPubLedgerSeq)
                    {
                        JLOG (m_journal.debug()) <<
                            "tryAdvance found last valid changed";
                        progress = true;
                    }
                }
            }
            else
            {
                mHistLedger.reset();
                mShardLedger.reset();
                JLOG (m_journal.trace()) <<
                    "tryAdvance not fetching history";
            }
        }
        else
        {
            JLOG (m_journal.trace()) <<
                "tryAdvance found " << pubLedgers.size() <<
                " ledgers to publish";
            for(auto ledger : pubLedgers)
            {
                {
                    ScopedUnlockType sul (m_mutex);
                    JLOG (m_journal.debug()) <<
                        "tryAdvance publishing seq " << ledger->info().seq;
                    setFullLedger(ledger, true, true);
                }

                setPubLedger(ledger);

                {
                    ScopedUnlockType sul(m_mutex);
                    app_.getOPs().pubLedger(ledger);
                }
            }

            app_.getOPs().clearNeedNetworkLedger();
            progress = newPFWork ("pf:newLedger", sl);
        }
        if (progress)
            mAdvanceWork = true;
    } while (mAdvanceWork);
}

void
LedgerMaster::addFetchPack (
    uint256 const& hash,
    std::shared_ptr< Blob >& data)
{
    fetch_packs_.canonicalize (hash, data);
}

boost::optional<Blob>
LedgerMaster::getFetchPack (
    uint256 const& hash)
{
    Blob data;
    if (fetch_packs_.retrieve(hash, data))
    {
        fetch_packs_.del(hash, false);
        if (hash == sha512Half(makeSlice(data)))
            return data;
    }
    return boost::none;
}

void
LedgerMaster::gotFetchPack (
    bool progress,
    std::uint32_t seq)
{
    if (!mGotFetchPackThread.test_and_set(std::memory_order_acquire))
    {
        app_.getJobQueue().addJob (
            jtLEDGER_DATA, "gotFetchPack",
            [&] (Job&)
            {
                app_.getInboundLedgers().gotFetchPack();
                mGotFetchPackThread.clear(std::memory_order_release);
            });
    }
}

void
LedgerMaster::makeFetchPack (
    std::weak_ptr<Peer> const& wPeer,
    std::shared_ptr<protocol::TMGetObjectByHash> const& request,
    uint256 haveLedgerHash,
     UptimeClock::time_point uptime)
{
    if (UptimeClock::now() > uptime + 1s)
    {
        JLOG(m_journal.info()) << "Fetch pack request got stale";
        return;
    }

    if (app_.getFeeTrack ().isLoadedLocal () ||
        (getValidatedLedgerAge() > 40s))
    {
        JLOG(m_journal.info()) << "Too busy to make fetch pack";
        return;
    }

    auto peer = wPeer.lock ();

    if (!peer)
        return;

    auto haveLedger = getLedgerByHash (haveLedgerHash);

    if (!haveLedger)
    {
        JLOG(m_journal.info())
            << "Peer requests fetch pack for ledger we don't have: "
            << haveLedger;
        peer->charge (Resource::feeRequestNoReply);
        return;
    }

    if (haveLedger->open())
    {
        JLOG(m_journal.warn())
            << "Peer requests fetch pack from open ledger: "
            << haveLedger;
        peer->charge (Resource::feeInvalidRequest);
        return;
    }

    if (haveLedger->info().seq < getEarliestFetch())
    {
        JLOG(m_journal.debug())
            << "Peer requests fetch pack that is too early";
        peer->charge (Resource::feeInvalidRequest);
        return;
    }

    auto wantLedger = getLedgerByHash (haveLedger->info().parentHash);

    if (!wantLedger)
    {
        JLOG(m_journal.info())
            << "Peer requests fetch pack for ledger whose predecessor we "
            << "don't have: " << haveLedger;
        peer->charge (Resource::feeRequestNoReply);
        return;
    }


    auto fpAppender = [](
        protocol::TMGetObjectByHash* reply,
        std::uint32_t ledgerSeq,
        SHAMapHash const& hash,
        const Blob& blob)
    {
        protocol::TMIndexedObject& newObj = * (reply->add_objects ());
        newObj.set_ledgerseq (ledgerSeq);
        newObj.set_hash (hash.as_uint256().begin (), 256 / 8);
        newObj.set_data (&blob[0], blob.size ());
    };

    try
    {
        protocol::TMGetObjectByHash reply;
        reply.set_query (false);

        if (request->has_seq ())
            reply.set_seq (request->seq ());

        reply.set_ledgerhash (request->ledgerhash ());
        reply.set_type (protocol::TMGetObjectByHash::otFETCH_PACK);

//生成获取包：
//1。添加请求的分类帐的标题。
//2。为该分类帐的AccountStateMap添加节点。
//三。如果存在事务，请添加
//分类帐的交易记录。
//4。如果fetchpack现在包含大于或等于
//256个条目，然后停止。
//5。如果经过的时间不多，则返回并重复
//将上一个分类帐添加到fetchpack的过程相同。
        do
        {
            std::uint32_t lSeq = wantLedger->info().seq;

            protocol::TMIndexedObject& newObj = *reply.add_objects ();
            newObj.set_hash (
                wantLedger->info().hash.data(), 256 / 8);
            Serializer s (256);
            s.add32 (HashPrefix::ledgerMaster);
            addRaw(wantLedger->info(), s);
            newObj.set_data (s.getDataPtr (), s.getLength ());
            newObj.set_ledgerseq (lSeq);

            wantLedger->stateMap().getFetchPack
                (&haveLedger->stateMap(), true, 16384,
                    std::bind (fpAppender, &reply, lSeq, std::placeholders::_1,
                               std::placeholders::_2));

            if (wantLedger->info().txHash.isNonZero ())
                wantLedger->txMap().getFetchPack (
                    nullptr, true, 512,
                    std::bind (fpAppender, &reply, lSeq, std::placeholders::_1,
                               std::placeholders::_2));

            if (reply.objects ().size () >= 512)
                break;

//移动可以保存引用/取消引用
            haveLedger = std::move (wantLedger);
            wantLedger = getLedgerByHash (haveLedger->info().parentHash);
        }
        while (wantLedger &&
               UptimeClock::now() <= uptime + 1s);

        JLOG(m_journal.info())
            << "Built fetch pack with " << reply.objects ().size () << " nodes";
        auto msg = std::make_shared<Message> (reply, protocol::mtGET_OBJECTS);
        peer->send (msg);
    }
    catch (std::exception const&)
    {
        JLOG(m_journal.warn()) << "Exception building fetch pach";
    }
}

std::size_t
LedgerMaster::getFetchPackCacheSize () const
{
    return fetch_packs_.getCacheSize ();
}

} //涟漪
