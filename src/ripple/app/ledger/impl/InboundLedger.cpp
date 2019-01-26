
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

#include <ripple/app/ledger/InboundLedger.h>
#include <ripple/shamap/SHAMapNodeID.h>
#include <ripple/app/ledger/AccountStateSF.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/TransactionStateSF.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/resource/Fees.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/nodestore/DatabaseShard.h>

#include <algorithm>

namespace ripple {

using namespace std::chrono_literals;

enum
{
//要开始的对等数
    peerCountStart = 4

//超时时要添加的对等数
    ,peerCountAdd = 2

//在我们放弃之前有多少次超时
    ,ledgerTimeoutRetriesMax = 10

//在我们开始攻击之前有多少次超时
    ,ledgerBecomeAggressiveThreshold = 6

//最初要查找的节点数
    ,missingNodesFind = 256

//请求答复的节点数
    ,reqNodesReply = 128

//盲目请求的节点数
    ,reqNodes = 8
};

//每个分类帐超时的毫秒数
auto constexpr ledgerAcquireTimeout = 2500ms;

InboundLedger::InboundLedger(Application& app, uint256 const& hash,
    std::uint32_t seq, Reason reason, clock_type& clock)
    : PeerSet (app, hash, ledgerAcquireTimeout, clock,
        app.journal("InboundLedger"))
    , mHaveHeader (false)
    , mHaveState (false)
    , mHaveTransactions (false)
    , mSignaled (false)
    , mByHash (true)
    , mSeq (seq)
    , mReason (reason)
    , mReceiveDispatched (false)
{
    JLOG (m_journal.trace()) << "Acquiring ledger " << mHash;
}

void
InboundLedger::init(ScopedLockType& collectionLock)
{
    ScopedLockType sl (mLock);
    collectionLock.unlock();
    tryDB(app_.family());
    if (mFailed)
        return;
    if (! mComplete)
    {
        auto shardStore = app_.getShardStore();
        if (mReason == Reason::SHARD)
        {
            if (! shardStore || ! app_.shardFamily())
            {
                JLOG(m_journal.error()) <<
                    "Acquiring shard with no shard store available";
                mFailed = true;
                return;
            }
            mHaveHeader = false;
            mHaveTransactions = false;
            mHaveState = false;
            mLedger.reset();
            tryDB(*app_.shardFamily());
            if (mFailed)
                return;
        }
        else if (shardStore && mSeq >= shardStore->earliestSeq())
        {
            if (auto l = shardStore->fetchLedger(mHash, mSeq))
            {
                mHaveHeader = true;
                mHaveTransactions = true;
                mHaveState = true;
                mComplete = true;
                mLedger = std::move(l);
            }
        }
    }
    if (! mComplete)
    {
        addPeers();
        execute();
        return;
    }

    JLOG (m_journal.debug()) <<
        "Acquiring ledger we already have in " <<
        " local store. " << mHash;
    mLedger->setImmutable(app_.config());

    if (mReason == Reason::HISTORY || mReason == Reason::SHARD)
        return;

    app_.getLedgerMaster().storeLedger(mLedger);

//检查这是否可以是更新的完全验证的分类帐
    if (mReason == Reason::CONSENSUS)
        app_.getLedgerMaster().checkAccept(mLedger);
}

void InboundLedger::execute ()
{
    if (app_.getJobQueue ().getJobCountTotal (jtLEDGER_DATA) > 4)
    {
        JLOG (m_journal.debug()) <<
            "Deferring InboundLedger timer due to load";
        setTimer ();
        return;
    }

    app_.getJobQueue ().addJob (
        jtLEDGER_DATA, "InboundLedger",
        [ptr = shared_from_this()] (Job&)
        {
            ptr->invokeOnTimer ();
        });
}
void InboundLedger::update (std::uint32_t seq)
{
    ScopedLockType sl (mLock);

//如果我们不知道序列号，但现在知道了，保存它。
    if ((seq != 0) && (mSeq == 0))
        mSeq = seq;

//防止这个被清扫
    touch ();
}

bool InboundLedger::checkLocal ()
{
    ScopedLockType sl (mLock);
    if (! isDone())
    {
        if (mLedger)
            tryDB(mLedger->stateMap().family());
        else if(mReason == Reason::SHARD)
            tryDB(*app_.shardFamily());
        else
            tryDB(app_.family());
        if (mFailed || mComplete)
        {
            done();
            return true;
        }
    }
    return false;
}

InboundLedger::~InboundLedger ()
{
//将收到的数据另存为未处理的数据。可能有用
//用于填充不同的分类帐
    for (auto& entry : mReceivedData)
    {
        if (entry.second->type () == protocol::liAS_NODE)
            app_.getInboundLedgers().gotStaleData(entry.second);
    }
    if (! isDone())
    {
        JLOG (m_journal.debug()) <<
            "Acquire " << mHash << " abort " <<
            ((getTimeouts () == 0) ? std::string() :
                (std::string ("timeouts:") +
                to_string (getTimeouts ()) + " ")) <<
            mStats.get ();
    }
}

std::vector<uint256>
InboundLedger::neededTxHashes (
    int max, SHAMapSyncFilter* filter) const
{
    std::vector<uint256> ret;

    if (mLedger->info().txHash.isNonZero ())
    {
        if (mLedger->txMap().getHash().isZero ())
            ret.push_back (mLedger->info().txHash);
        else
            ret = mLedger->txMap().getNeededHashes (max, filter);
    }

    return ret;
}

std::vector<uint256>
InboundLedger::neededStateHashes (
    int max, SHAMapSyncFilter* filter) const
{
    std::vector<uint256> ret;

    if (mLedger->info().accountHash.isNonZero ())
    {
        if (mLedger->stateMap().getHash().isZero ())
            ret.push_back (mLedger->info().accountHash);
        else
            ret = mLedger->stateMap().getNeededHashes (max, filter);
    }

    return ret;
}

LedgerInfo
InboundLedger::deserializeHeader (
    Slice data,
    bool hasPrefix)
{
    SerialIter sit (data.data(), data.size());

    if (hasPrefix)
        sit.get32 ();

    LedgerInfo info;

    info.seq = sit.get32 ();
    info.drops = sit.get64 ();
    info.parentHash = sit.get256 ();
    info.txHash = sit.get256 ();
    info.accountHash = sit.get256 ();
    info.parentCloseTime = NetClock::time_point{NetClock::duration{sit.get32()}};
    info.closeTime = NetClock::time_point{NetClock::duration{sit.get32()}};
    info.closeTimeResolution = NetClock::duration{sit.get8()};
    info.closeFlags = sit.get8 ();

    return info;
}

//查看有多少分类帐数据存储在本地
//将存储在提取包中找到的数据
void
InboundLedger::tryDB(Family& f)
{
    if (! mHaveHeader)
    {
        auto makeLedger = [&, this](Blob const& data)
            {
                JLOG(m_journal.trace()) <<
                    "Ledger header found in fetch pack";
                mLedger = std::make_shared<Ledger>(
                    deserializeHeader(makeSlice(data), true),
                        app_.config(), f);
                if (mLedger->info().hash != mHash ||
                    (mSeq != 0 && mSeq != mLedger->info().seq))
                {
//我们知道一个事实，那本分类账永远也得不到。
                    JLOG(m_journal.warn()) <<
                        "hash " << mHash <<
                        " seq " << std::to_string(mSeq) <<
                        " cannot be a ledger";
                    mLedger.reset();
                    mFailed = true;
                }
            };

//尝试从数据库中获取分类帐标题
        auto node = f.db().fetch(mHash, mSeq);
        if (! node)
        {
            auto data = app_.getLedgerMaster().getFetchPack(mHash);
            if (! data)
                return;
            JLOG (m_journal.trace()) <<
                "Ledger header found in fetch pack";
            makeLedger(*data);
            if (mLedger)
                f.db().store(hotLEDGER, std::move(*data),
                    mHash, mLedger->info().seq);
        }
        else
        {
            JLOG (m_journal.trace()) <<
                "Ledger header found in node store";
            makeLedger(node->getData());
        }
        if (mFailed)
            return;
        if (mSeq == 0)
            mSeq = mLedger->info().seq;
        mLedger->stateMap().setLedgerSeq(mSeq);
        mLedger->txMap().setLedgerSeq(mSeq);
        mHaveHeader = true;
    }

    if (! mHaveTransactions)
    {
        if (mLedger->info().txHash.isZero())
        {
            JLOG (m_journal.trace()) << "No TXNs to fetch";
            mHaveTransactions = true;
        }
        else
        {
            TransactionStateSF filter(mLedger->txMap().family().db(),
                app_.getLedgerMaster());
            if (mLedger->txMap().fetchRoot(
                SHAMapHash{mLedger->info().txHash}, &filter))
            {
                if (neededTxHashes(1, &filter).empty())
                {
                    JLOG(m_journal.trace()) <<
                        "Had full txn map locally";
                    mHaveTransactions = true;
                }
            }
        }
    }

    if (! mHaveState)
    {
        if (mLedger->info().accountHash.isZero())
        {
            JLOG (m_journal.fatal()) <<
                "We are acquiring a ledger with a zero account hash";
            mFailed = true;
            return;
        }
        AccountStateSF filter(mLedger->stateMap().family().db(),
            app_.getLedgerMaster());
        if (mLedger->stateMap().fetchRoot(
            SHAMapHash{mLedger->info().accountHash}, &filter))
        {
            if (neededStateHashes(1, &filter).empty())
            {
                JLOG(m_journal.trace()) <<
                    "Had full AS map locally";
                mHaveState = true;
            }
        }
    }

    if (mHaveTransactions && mHaveState)
    {
        JLOG(m_journal.debug()) <<
            "Had everything locally";
        mComplete = true;
        mLedger->setImmutable(app_.config());
    }
}

/*当计时器过期时由对等集用锁调用
**/

void InboundLedger::onTimer (bool wasProgress, ScopedLockType&)
{
    mRecentNodes.clear ();

    if (isDone())
    {
        JLOG (m_journal.info()) <<
            "Already done " << mHash;
        return;
    }

    if (getTimeouts () > ledgerTimeoutRetriesMax)
    {
        if (mSeq != 0)
        {
            JLOG (m_journal.warn()) <<
                getTimeouts() << " timeouts for ledger " << mSeq;
        }
        else
        {
            JLOG (m_journal.warn()) <<
                getTimeouts() << " timeouts for ledger " << mHash;
        }
        setFailed ();
        done ();
        return;
    }

    if (!wasProgress)
    {
        checkLocal();

        mByHash = true;

        std::size_t pc = getPeerCount ();
        JLOG (m_journal.debug()) <<
            "No progress(" << pc <<
            ") for ledger " << mHash;

//如果原因不是历史记录，则addpeers触发器
//因此，如果原因是历史，需要在添加后触发
//否则，我们需要在添加
//所以每个对等点触发一次
        if (mReason != Reason::HISTORY)
            trigger (nullptr, TriggerReason::timeout);
        addPeers ();
        if (mReason == Reason::HISTORY)
            trigger (nullptr, TriggerReason::timeout);
    }
}

/*如果可能，向集合中添加更多对等点*/
void InboundLedger::addPeers ()
{
    app_.overlay().selectPeers (*this,
        (getPeerCount() == 0) ? peerCountStart : peerCountAdd,
        ScoreHasLedger (getHash(), mSeq));
}

std::weak_ptr<PeerSet> InboundLedger::pmDowncast ()
{
    return std::dynamic_pointer_cast<PeerSet> (shared_from_this ());
}

void InboundLedger::done ()
{
    if (mSignaled)
        return;

    mSignaled = true;
    touch ();

    JLOG (m_journal.debug()) <<
        "Acquire " << mHash <<
        (mFailed ? " fail " : " ") <<
        ((getTimeouts () == 0) ? std::string() :
            (std::string ("timeouts:") +
            to_string (getTimeouts ()) + " ")) <<
        mStats.get ();

    assert (mComplete || mFailed);

    if (mComplete && ! mFailed && mLedger)
    {
        mLedger->setImmutable (app_.config());
        switch (mReason)
        {
        case Reason::SHARD:
            app_.getShardStore()->setStored(mLedger);
//todoc++ 17：[ [谬误] ]
        case Reason::HISTORY:
            app_.getInboundLedgers().onLedgerFetched();
            break;
        default:
            app_.getLedgerMaster().storeLedger(mLedger);
            break;
        }
    }

//我们持有对等锁，所以必须调度
    app_.getJobQueue ().addJob (
        jtLEDGER_DATA, "AcquisitionDone",
        [self = shared_from_this()](Job&)
        {
            if (self->mComplete && !self->mFailed)
            {
                self->app().getLedgerMaster().checkAccept(
                    self->getLedger());
                self->app().getLedgerMaster().tryAdvance();
            }
            else
                self->app().getInboundLedgers().logFailure (
                    self->getHash(), self->getSeq());
        });
}

/*请求更多节点，可能来自特定的对等点
**/

void InboundLedger::trigger (std::shared_ptr<Peer> const& peer, TriggerReason reason)
{
    ScopedLockType sl (mLock);

    if (isDone ())
    {
        JLOG (m_journal.debug()) <<
            "Trigger on ledger: " << mHash <<
            (mComplete ? " completed" : "") <<
            (mFailed ? " failed" : "");
        return;
    }

    if (auto stream = m_journal.trace())
    {
        if (peer)
            stream <<
                "Trigger acquiring ledger " << mHash << " from " << peer;
        else
            stream <<
                "Trigger acquiring ledger " << mHash;

        if (mComplete || mFailed)
            stream <<
                "complete=" << mComplete << " failed=" << mFailed;
        else
            stream <<
                "header=" << mHaveHeader << " tx=" << mHaveTransactions <<
                    " as=" << mHaveState;
    }

    if (! mHaveHeader)
    {
        tryDB(mReason == Reason::SHARD ?
            *app_.shardFamily() : app_.family());
        if (mFailed)
        {
            JLOG (m_journal.warn()) <<
                " failed local for " << mHash;
            return;
        }
    }

    protocol::TMGetLedger tmGL;
    tmGL.set_ledgerhash (mHash.begin (), mHash.size ());

    if (getTimeouts () != 0)
{ //如果我们至少超时一次，就要更有侵略性
        tmGL.set_querytype (protocol::qtINDIRECT);

        if (! isProgress () && ! mFailed && mByHash &&
            (getTimeouts () > ledgerBecomeAggressiveThreshold))
        {
            auto need = getNeededHashes ();

            if (!need.empty ())
            {
                protocol::TMGetObjectByHash tmBH;
                bool typeSet = false;
                tmBH.set_query (true);
                tmBH.set_ledgerhash (mHash.begin (), mHash.size ());
                for (auto const& p : need)
                {
                    JLOG (m_journal.warn()) <<
                        "Want: " << p.second;

                    if (!typeSet)
                    {
                        tmBH.set_type (p.first);
                        typeSet = true;
                    }

                    if (p.first == tmBH.type ())
                    {
                        protocol::TMIndexedObject* io = tmBH.add_objects ();
                        io->set_hash (p.second.begin (), p.second.size ());
                        if (mSeq != 0)
                            io->set_ledgerseq(mSeq);
                    }
                }

                auto packet = std::make_shared <Message> (
                    tmBH, protocol::mtGET_OBJECTS);

                for (auto id : mPeers)
                {
                    if (auto p = app_.overlay ().findPeerByShortID (id))
                    {
                        mByHash = false;
                        p->send (packet);
                    }
                }
            }
            else
            {
                JLOG (m_journal.info()) <<
                    "getNeededHashes says acquire is complete";
                mHaveHeader = true;
                mHaveTransactions = true;
                mHaveState = true;
                mComplete = true;
            }
        }
    }

//没有头数据我们不能做太多，因为我们不知道
//状态或事务根散列。
    if (!mHaveHeader && !mFailed)
    {
        tmGL.set_itype (protocol::liBASE);
        if (mSeq != 0)
            tmGL.set_ledgerseq (mSeq);
        JLOG (m_journal.trace()) <<
            "Sending header request to " <<
             (peer ? "selected peer" : "all peers");
        sendRequest (tmGL, peer);
        return;
    }

    if (mLedger)
        tmGL.set_ledgerseq (mLedger->info().seq);

    if (reason != TriggerReason::reply)
    {
//如果我们是盲目查询，不要深入查询
        tmGL.set_querydepth (0);
    }
    else if (peer && peer->isHighLatency ())
    {
//如果对等机具有很高的延迟，则需要进行额外的深度查询。
        tmGL.set_querydepth (2);
    }
    else
        tmGL.set_querydepth (1);

//首先获取状态数据，因为它最可能有用
//如果我们最终放弃了这次的收获。
    if (mHaveHeader && !mHaveState && !mFailed)
    {
        assert (mLedger);

        if (!mLedger->stateMap().isValid ())
        {
            mFailed = true;
        }
        else if (mLedger->stateMap().getHash ().isZero ())
        {
//我们需要根节点
            tmGL.set_itype (protocol::liAS_NODE);
            *tmGL.add_nodeids () = SHAMapNodeID ().getRawString ();
            JLOG (m_journal.trace()) <<
                "Sending AS root request to " <<
                (peer ? "selected peer" : "all peers");
            sendRequest (tmGL, peer);
            return;
        }
        else
        {
            AccountStateSF filter(mLedger->stateMap().family().db(),
                app_.getLedgerMaster());

//在处理大型状态图时释放锁
            sl.unlock();
            auto nodes = mLedger->stateMap().getMissingNodes (
                missingNodesFind, &filter);
            sl.lock();

//确保释放锁时没有发生任何事情
            if (!mFailed && !mComplete && !mHaveState)
            {
                if (nodes.empty ())
                {
                    if (!mLedger->stateMap().isValid ())
                        mFailed = true;
                    else
                    {
                        mHaveState = true;

                        if (mHaveTransactions)
                            mComplete = true;
                    }
                }
                else
                {
                    filterNodes (nodes, reason);

                    if (!nodes.empty ())
                    {
                        tmGL.set_itype (protocol::liAS_NODE);
                        for (auto const& id : nodes)
                        {
                            * (tmGL.add_nodeids ()) = id.first.getRawString ();
                        }

                        JLOG (m_journal.trace()) <<
                            "Sending AS node request (" <<
                            nodes.size () << ") to " <<
                            (peer ? "selected peer" : "all peers");
                        sendRequest (tmGL, peer);
                        return;
                    }
                    else
                    {
                        JLOG (m_journal.trace()) <<
                            "All AS nodes filtered";
                    }
                }
            }
        }
    }

    if (mHaveHeader && !mHaveTransactions && !mFailed)
    {
        assert (mLedger);

        if (!mLedger->txMap().isValid ())
        {
            mFailed = true;
        }
        else if (mLedger->txMap().getHash ().isZero ())
        {
//我们需要根节点
            tmGL.set_itype (protocol::liTX_NODE);
            * (tmGL.add_nodeids ()) = SHAMapNodeID ().getRawString ();
            JLOG (m_journal.trace()) <<
                "Sending TX root request to " << (
                    peer ? "selected peer" : "all peers");
            sendRequest (tmGL, peer);
            return;
        }
        else
        {
            TransactionStateSF filter(mLedger->txMap().family().db(),
                app_.getLedgerMaster());

            auto nodes = mLedger->txMap().getMissingNodes (
                missingNodesFind, &filter);

            if (nodes.empty ())
            {
                if (!mLedger->txMap().isValid ())
                    mFailed = true;
                else
                {
                    mHaveTransactions = true;

                    if (mHaveState)
                        mComplete = true;
                }
            }
            else
            {
                filterNodes (nodes, reason);

                if (!nodes.empty ())
                {
                    tmGL.set_itype (protocol::liTX_NODE);
                    for (auto const& n : nodes)
                    {
                        * (tmGL.add_nodeids ()) = n.first.getRawString ();
                    }
                    JLOG (m_journal.trace()) <<
                        "Sending TX node request (" <<
                        nodes.size () << ") to " <<
                        (peer ? "selected peer" : "all peers");
                    sendRequest (tmGL, peer);
                    return;
                }
                else
                {
                    JLOG (m_journal.trace()) <<
                        "All TX nodes filtered";
                }
            }
        }
    }

    if (mComplete || mFailed)
    {
        JLOG (m_journal.debug()) <<
            "Done:" << (mComplete ? " complete" : "") <<
                (mFailed ? " failed " : " ") <<
            mLedger->info().seq;
        sl.unlock ();
        done ();
    }
}

void InboundLedger::filterNodes (
    std::vector<std::pair<SHAMapNodeID, uint256>>& nodes,
    TriggerReason reason)
{
//对节点进行排序，使我们最近没有的节点
//我们要求先到。
    auto dup = std::stable_partition (
        nodes.begin(), nodes.end(),
        [this](auto const& item)
        {
            return mRecentNodes.count (item.second) == 0;
        });

//如果一切都是重复的，我们不想发送
//任何查询，除了我们需要的超时
//查询所有人：
    if (dup == nodes.begin ())
    {
        JLOG (m_journal.trace()) <<
            "filterNodes: all duplicates";

        if (reason != TriggerReason::timeout)
        {
            nodes.clear ();
            return;
        }
    }
    else
    {
        JLOG (m_journal.trace()) <<
            "filterNodes: pruning duplicates";

        nodes.erase (dup, nodes.end());
    }

    std::size_t const limit = (reason == TriggerReason::reply)
        ? reqNodesReply
        : reqNodes;

    if (nodes.size () > limit)
        nodes.resize (limit);

    for (auto const& n : nodes)
        mRecentNodes.insert (n.second);
}

/*采用分类帐表头数据
    用锁呼叫
**/

//数据不能有哈希前缀
bool InboundLedger::takeHeader (std::string const& data)
{
//返回值：真=正常，假=错误数据
    JLOG (m_journal.trace()) <<
        "got header acquiring ledger " << mHash;

    if (mComplete || mFailed || mHaveHeader)
        return true;

    auto* f = mReason == Reason::SHARD ?
        app_.shardFamily() : &app_.family();
    mLedger = std::make_shared<Ledger>(deserializeHeader(
        makeSlice(data), false), app_.config(), *f);
    if (mLedger->info().hash != mHash ||
        (mSeq != 0 && mSeq != mLedger->info().seq))
    {
        JLOG (m_journal.warn()) <<
            "Acquire hash mismatch: " << mLedger->info().hash <<
            "!=" << mHash;
        mLedger.reset ();
        return false;
    }
    if (mSeq == 0)
        mSeq = mLedger->info().seq;
    mLedger->stateMap().setLedgerSeq(mSeq);
    mLedger->txMap().setLedgerSeq(mSeq);
    mHaveHeader = true;

    Serializer s (data.size () + 4);
    s.add32 (HashPrefix::ledgerMaster);
    s.addRaw (data.data(), data.size());
    f->db().store(hotLEDGER, std::move (s.modData ()), mHash, mSeq);

    if (mLedger->info().txHash.isZero ())
        mHaveTransactions = true;

    if (mLedger->info().accountHash.isZero ())
        mHaveState = true;

    mLedger->txMap().setSynching ();
    mLedger->stateMap().setSynching ();

    return true;
}

/*处理从对等端接收的Tx数据
    用锁呼叫
**/

bool InboundLedger::takeTxNode (const std::vector<SHAMapNodeID>& nodeIDs,
    const std::vector< Blob >& data, SHAMapAddNode& san)
{
    if (!mHaveHeader)
    {
        JLOG (m_journal.warn()) <<
            "TX node without header";
        san.incInvalid();
        return false;
    }

    if (mHaveTransactions || mFailed)
    {
        san.incDuplicate();
        return true;
    }

    auto nodeIDit = nodeIDs.cbegin ();
    auto nodeDatait = data.begin ();
    TransactionStateSF filter(mLedger->txMap().family().db(),
        app_.getLedgerMaster());

    while (nodeIDit != nodeIDs.cend ())
    {
        if (nodeIDit->isRoot ())
        {
            san += mLedger->txMap().addRootNode (
                SHAMapHash{mLedger->info().txHash},
                    makeSlice(*nodeDatait), snfWIRE, &filter);
            if (!san.isGood())
                return false;
        }
        else
        {
            san +=  mLedger->txMap().addKnownNode (
                *nodeIDit, makeSlice(*nodeDatait), &filter);
            if (!san.isGood())
                return false;
        }

        ++nodeIDit;
        ++nodeDatait;
    }

    if (!mLedger->txMap().isSynching ())
    {
        mHaveTransactions = true;

        if (mHaveState)
        {
            mComplete = true;
            done ();
        }
    }

    return true;
}

/*作为从对等方接收的数据进行处理
    用锁呼叫
**/

bool InboundLedger::takeAsNode (const std::vector<SHAMapNodeID>& nodeIDs,
    const std::vector< Blob >& data, SHAMapAddNode& san)
{
    JLOG (m_journal.trace()) <<
        "got ASdata (" << nodeIDs.size () <<
        ") acquiring ledger " << mHash;
    if (nodeIDs.size () == 1)
    {
        JLOG(m_journal.trace()) <<
            "got AS node: " << nodeIDs.front ();
    }

    ScopedLockType sl (mLock);

    if (!mHaveHeader)
    {
        JLOG (m_journal.warn()) <<
            "Don't have ledger header";
        san.incInvalid();
        return false;
    }

    if (mHaveState || mFailed)
    {
        san.incDuplicate();
        return true;
    }

    auto nodeIDit = nodeIDs.cbegin ();
    auto nodeDatait = data.begin ();
    AccountStateSF filter(mLedger->stateMap().family().db(),
        app_.getLedgerMaster());

    while (nodeIDit != nodeIDs.cend ())
    {
        if (nodeIDit->isRoot ())
        {
            san += mLedger->stateMap().addRootNode (
                SHAMapHash{mLedger->info().accountHash},
                    makeSlice(*nodeDatait), snfWIRE, &filter);
            if (!san.isGood ())
            {
                JLOG (m_journal.warn()) <<
                    "Bad ledger header";
                return false;
            }
        }
        else
        {
            san += mLedger->stateMap().addKnownNode (
                *nodeIDit, makeSlice(*nodeDatait), &filter);
            if (!san.isGood ())
            {
                JLOG (m_journal.warn()) <<
                    "Unable to add AS node";
                return false;
            }
        }

        ++nodeIDit;
        ++nodeDatait;
    }

    if (!mLedger->stateMap().isSynching ())
    {
        mHaveState = true;

        if (mHaveTransactions)
        {
            mComplete = true;
            done ();
        }
    }

    return true;
}

/*作为从对等机接收的根节点处理
    用锁呼叫
**/

bool InboundLedger::takeAsRootNode (Slice const& data, SHAMapAddNode& san)
{
    if (mFailed || mHaveState)
    {
        san.incDuplicate();
        return true;
    }

    if (!mHaveHeader)
    {
        assert(false);
        return false;
    }

    AccountStateSF filter(mLedger->stateMap().family().db(),
        app_.getLedgerMaster());
    san += mLedger->stateMap().addRootNode (
        SHAMapHash{mLedger->info().accountHash}, data, snfWIRE, &filter);
    return san.isGood();
}

/*作为从对等机接收的根节点处理
    用锁呼叫
**/

bool InboundLedger::takeTxRootNode (Slice const& data, SHAMapAddNode& san)
{
    if (mFailed || mHaveTransactions)
    {
        san.incDuplicate();
        return true;
    }

    if (!mHaveHeader)
    {
        assert(false);
        return false;
    }

    TransactionStateSF filter(mLedger->txMap().family().db(),
        app_.getLedgerMaster());
    san += mLedger->txMap().addRootNode (
        SHAMapHash{mLedger->info().txHash}, data, snfWIRE, &filter);
    return san.isGood();
}

std::vector<InboundLedger::neededHash_t>
InboundLedger::getNeededHashes ()
{
    std::vector<neededHash_t> ret;

    if (!mHaveHeader)
    {
        ret.push_back (std::make_pair (
            protocol::TMGetObjectByHash::otLEDGER, mHash));
        return ret;
    }

    if (!mHaveState)
    {
        AccountStateSF filter(mLedger->stateMap().family().db(),
            app_.getLedgerMaster());
        for (auto const& h : neededStateHashes (4, &filter))
        {
            ret.push_back (std::make_pair (
                protocol::TMGetObjectByHash::otSTATE_NODE, h));
        }
    }

    if (!mHaveTransactions)
    {
        TransactionStateSF filter(mLedger->txMap().family().db(),
            app_.getLedgerMaster());
        for (auto const& h : neededTxHashes (4, &filter))
        {
            ret.push_back (std::make_pair (
                protocol::TMGetObjectByHash::otTRANSACTION_NODE, h));
        }
    }

    return ret;
}

/*存储从对等机接收的tmledgerdata以供以后处理
    如果需要调度，则返回“true”
**/

bool
InboundLedger::gotData(std::weak_ptr<Peer> peer,
    std::shared_ptr<protocol::TMLedgerData> const& data)
{
    std::lock_guard<std::mutex> sl (mReceivedDataLock);

    if (isDone ())
        return false;

    mReceivedData.emplace_back (peer, data);

    if (mReceiveDispatched)
        return false;

    mReceiveDispatched = true;
    return true;
}

/*处理一个tmledgerdata
    返回有用节点的数目
**/

//vvalco注意到，不需要通过整个对等体，
//我们只需要一个resource：：consumer端点就可以摆脱困境。
//
//TODO更改点对消费者
//
int InboundLedger::processData (std::shared_ptr<Peer> peer,
    protocol::TMLedgerData& packet)
{
    ScopedLockType sl (mLock);

    if (packet.type () == protocol::liBASE)
    {
        if (packet.nodes_size () < 1)
        {
            JLOG (m_journal.warn()) <<
                "Got empty header data";
            peer->charge (Resource::feeInvalidRequest);
            return -1;
        }

        SHAMapAddNode san;

        if (!mHaveHeader)
        {
            if (takeHeader (packet.nodes (0).nodedata ()))
                san.incUseful ();
            else
            {
                JLOG (m_journal.warn()) <<
                    "Got invalid header data";
                peer->charge (Resource::feeInvalidRequest);
                return -1;
            }
        }


        if (!mHaveState && (packet.nodes ().size () > 1) &&
            !takeAsRootNode (makeSlice(packet.nodes(1).nodedata ()), san))
        {
            JLOG (m_journal.warn()) <<
                "Included AS root invalid";
        }

        if (!mHaveTransactions && (packet.nodes ().size () > 2) &&
            !takeTxRootNode (makeSlice(packet.nodes(2).nodedata ()), san))
        {
            JLOG (m_journal.warn()) <<
                "Included TX root invalid";
        }

        if (san.isUseful ())
            progress ();

        mStats += san;
        return san.getGood ();
    }

    if ((packet.type () == protocol::liTX_NODE) || (
        packet.type () == protocol::liAS_NODE))
    {
        if (packet.nodes ().size () == 0)
        {
            JLOG (m_journal.info()) <<
                "Got response with no nodes";
            peer->charge (Resource::feeInvalidRequest);
            return -1;
        }

        std::vector<SHAMapNodeID> nodeIDs;
        nodeIDs.reserve(packet.nodes().size());
        std::vector< Blob > nodeData;
        nodeData.reserve(packet.nodes().size());

        for (int i = 0; i < packet.nodes ().size (); ++i)
        {
            const protocol::TMLedgerNode& node = packet.nodes (i);

            if (!node.has_nodeid () || !node.has_nodedata ())
            {
                JLOG (m_journal.warn()) <<
                    "Got bad node";
                peer->charge (Resource::feeInvalidRequest);
                return -1;
            }

            nodeIDs.push_back (SHAMapNodeID (node.nodeid ().data (),
                node.nodeid ().size ()));
            nodeData.push_back (Blob (node.nodedata ().begin (),
                node.nodedata ().end ()));
        }

        SHAMapAddNode san;

        if (packet.type () == protocol::liTX_NODE)
        {
            takeTxNode (nodeIDs, nodeData, san);
            JLOG (m_journal.debug()) <<
                "Ledger TX node stats: " << san.get();
        }
        else
        {
            takeAsNode (nodeIDs, nodeData, san);
            JLOG (m_journal.debug()) <<
                "Ledger AS node stats: " << san.get();
        }

        if (san.isUseful ())
            progress ();

        mStats += san;
        return san.getGood ();
    }

    return -1;
}

/*处理挂起的tmledgerdata
    查询“最佳”对等
**/

void InboundLedger::runData ()
{
    std::shared_ptr<Peer> chosenPeer;
    int chosenPeerCount = -1;

    std::vector <PeerDataPairType> data;

    for (;;)
    {
        data.clear();
        {
            std::lock_guard<std::mutex> sl (mReceivedDataLock);

            if (mReceivedData.empty ())
            {
                mReceiveDispatched = false;
                break;
            }

            data.swap(mReceivedData);
        }

//选择给我们提供最有用节点的对等节点，
//打破关系，支持最先回应的同伴。
        for (auto& entry : data)
        {
            if (auto peer = entry.first.lock())
            {
                int count = processData (peer, *(entry.second));
                if (count > chosenPeerCount)
                {
                    chosenPeerCount = count;
                    chosenPeer = std::move (peer);
                }
            }
        }
    }

    if (chosenPeer)
        trigger (chosenPeer, TriggerReason::reply);
}

Json::Value InboundLedger::getJson (int)
{
    Json::Value ret (Json::objectValue);

    ScopedLockType sl (mLock);

    ret[jss::hash] = to_string (mHash);

    if (mComplete)
        ret[jss::complete] = true;

    if (mFailed)
        ret[jss::failed] = true;

    if (!mComplete && !mFailed)
        ret[jss::peers] = static_cast<int>(mPeers.size());

    ret[jss::have_header] = mHaveHeader;

    if (mHaveHeader)
    {
        ret[jss::have_state] = mHaveState;
        ret[jss::have_transactions] = mHaveTransactions;
    }

    ret[jss::timeouts] = getTimeouts ();

    if (mHaveHeader && !mHaveState)
    {
        Json::Value hv (Json::arrayValue);
        for (auto const& h : neededStateHashes (16, nullptr))
        {
            hv.append (to_string (h));
        }
        ret[jss::needed_state_hashes] = hv;
    }

    if (mHaveHeader && !mHaveTransactions)
    {
        Json::Value hv (Json::arrayValue);
        for (auto const& h : neededTxHashes (16, nullptr))
        {
            hv.append (to_string (h));
        }
        ret[jss::needed_transaction_hashes] = hv;
    }

    return ret;
}

} //涟漪
