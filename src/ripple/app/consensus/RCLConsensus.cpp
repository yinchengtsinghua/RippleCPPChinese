
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

#include <ripple/app/consensus/RCLConsensus.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/BuildLedger.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/InboundTransactions.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/LocalTxs.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/misc/ValidatorKeys.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/basics/make_lock.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/predicates.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/digest.h>
#include <algorithm>

namespace ripple {

RCLConsensus::RCLConsensus(
    Application& app,
    std::unique_ptr<FeeVote>&& feeVote,
    LedgerMaster& ledgerMaster,
    LocalTxs& localTxs,
    InboundTransactions& inboundTransactions,
    Consensus<Adaptor>::clock_type const& clock,
    ValidatorKeys const& validatorKeys,
    beast::Journal journal)
    : adaptor_(
          app,
          std::move(feeVote),
          ledgerMaster,
          localTxs,
          inboundTransactions,
          validatorKeys,
          journal)
    , consensus_(clock, adaptor_, journal)
    , j_(journal)

{
}

RCLConsensus::Adaptor::Adaptor(
    Application& app,
    std::unique_ptr<FeeVote>&& feeVote,
    LedgerMaster& ledgerMaster,
    LocalTxs& localTxs,
    InboundTransactions& inboundTransactions,
    ValidatorKeys const& validatorKeys,
    beast::Journal journal)
    : app_(app)
        , feeVote_(std::move(feeVote))
        , ledgerMaster_(ledgerMaster)
        , localTxs_(localTxs)
        , inboundTransactions_{inboundTransactions}
        , j_(journal)
        , nodeID_{validatorKeys.nodeID}
        , valPublic_{validatorKeys.publicKey}
        , valSecret_{validatorKeys.secretKey}
{
}

boost::optional<RCLCxLedger>
RCLConsensus::Adaptor::acquireLedger(LedgerHash const& hash)
{
//我们需要把我们正在处理的账本换掉
    auto built = ledgerMaster_.getLedgerByHash(hash);
    if (!built)
    {
        if (acquiringLedger_ != hash)
        {
//需要开始获得正确的共识LCL
            JLOG(j_.warn()) << "Need consensus ledger " << hash;

//告诉分类帐获取系统我们需要共识分类帐
            acquiringLedger_ = hash;

            app_.getJobQueue().addJob(jtADVANCE, "getConsensusLedger",
                [id = hash, &app = app_](Job&)
                {
                    app.getInboundLedgers().acquire(id, 0,
                        InboundLedger::Reason::CONSENSUS);
                });
        }
        return boost::none;
    }

    assert(!built->open() && built->isImmutable());
    assert(built->info().hash == hash);

//通知新分类帐序列号的入站交易记录
    inboundTransactions_.newRound(built->info().seq);

//使用已获取分类帐的分类帐计时规则
    parms_.useRoundedCloseTime = built->rules().enabled(fix1528);

    return RCLCxLedger(built);
}

void
RCLConsensus::Adaptor::share(RCLCxPeerPos const& peerPos)
{
    protocol::TMProposeSet prop;

    auto const& proposal = peerPos.proposal();

    prop.set_proposeseq(proposal.proposeSeq());
    prop.set_closetime(proposal.closeTime().time_since_epoch().count());

    prop.set_currenttxhash(
        proposal.position().begin(), proposal.position().size());
    prop.set_previousledger(
        proposal.prevLedger().begin(), proposal.position().size());

    auto const pk = peerPos.publicKey().slice();
    prop.set_nodepubkey(pk.data(), pk.size());

    auto const sig = peerPos.signature();
    prop.set_signature(sig.data(), sig.size());

    app_.overlay().relay(prop, peerPos.suppressionID());
}

void
RCLConsensus::Adaptor::share(RCLCxTx const& tx)
{
//如果我们最近没有转发此事务，请将其转发给所有对等方
    if (app_.getHashRouter().shouldRelay(tx.id()))
    {
        JLOG(j_.debug()) << "Relaying disputed tx " << tx.id();
        auto const slice = tx.tx_.slice();
        protocol::TMTransaction msg;
        msg.set_rawtransaction(slice.data(), slice.size());
        msg.set_status(protocol::tsNEW);
        msg.set_receivetimestamp(
            app_.timeKeeper().now().time_since_epoch().count());
        app_.overlay().foreach (send_always(
            std::make_shared<Message>(msg, protocol::mtTRANSACTION)));
    }
    else
    {
        JLOG(j_.debug()) << "Not relaying disputed tx " << tx.id();
    }
}
void
RCLConsensus::Adaptor::propose(RCLCxPeerPos::Proposal const& proposal)
{
    JLOG(j_.trace()) << "We propose: "
                     << (proposal.isBowOut()
                             ? std::string("bowOut")
                             : ripple::to_string(proposal.position()));

    protocol::TMProposeSet prop;

    prop.set_currenttxhash(
        proposal.position().begin(), proposal.position().size());
    prop.set_previousledger(
        proposal.prevLedger().begin(), proposal.position().size());
    prop.set_proposeseq(proposal.proposeSeq());
    prop.set_closetime(proposal.closeTime().time_since_epoch().count());

    prop.set_nodepubkey(valPublic_.data(), valPublic_.size());

    auto signingHash = sha512Half(
        HashPrefix::proposal,
        std::uint32_t(proposal.proposeSeq()),
        proposal.closeTime().time_since_epoch().count(),
        proposal.prevLedger(),
        proposal.position());

    auto sig = signDigest(valPublic_, valSecret_, signingHash);

    prop.set_signature(sig.data(), sig.size());

    auto const suppression = proposalUniqueId(
        proposal.position(),
        proposal.prevLedger(),
        proposal.proposeSeq(),
        proposal.closeTime(),
        valPublic_,
        sig);

    app_.getHashRouter ().addSuppression (suppression);

    app_.overlay().send(prop);
}

void
RCLConsensus::Adaptor::share(RCLTxSet const& txns)
{
    inboundTransactions_.giveSet(txns.id(), txns.map_, false);
}

boost::optional<RCLTxSet>
RCLConsensus::Adaptor::acquireTxSet(RCLTxSet::ID const& setId)
{
    if (auto txns = inboundTransactions_.getSet(setId, true))
    {
        return RCLTxSet{std::move(txns)};
    }
    return boost::none;
}

bool
RCLConsensus::Adaptor::hasOpenTransactions() const
{
    return !app_.openLedger().empty();
}

std::size_t
RCLConsensus::Adaptor::proposersValidated(LedgerHash const& h) const
{
    return app_.getValidations().numTrustedForLedger(h);
}

std::size_t
RCLConsensus::Adaptor::proposersFinished(
    RCLCxLedger const& ledger,
    LedgerHash const& h) const
{
    RCLValidations& vals = app_.getValidations();
    return vals.getNodesAfter(
        RCLValidatedLedger(ledger.ledger_, vals.adaptor().journal()), h);
}

uint256
RCLConsensus::Adaptor::getPrevLedger(
    uint256 ledgerID,
    RCLCxLedger const& ledger,
    ConsensusMode mode)
{
    RCLValidations& vals = app_.getValidations();
    uint256 netLgr = vals.getPreferred(
        RCLValidatedLedger{ledger.ledger_, vals.adaptor().journal()},
        ledgerMaster_.getValidLedgerIndex());

    if (netLgr != ledgerID)
    {
        if (mode != ConsensusMode::wrongLedger)
            app_.getOPs().consensusViewChange();

        JLOG(j_.debug())<< Json::Compact(app_.getValidations().getJsonTrie());
    }

    return netLgr;
}

auto
RCLConsensus::Adaptor::onClose(
    RCLCxLedger const& ledger,
    NetClock::time_point const& closeTime,
    ConsensusMode mode) -> Result
{
    const bool wrongLCL = mode == ConsensusMode::wrongLedger;
    const bool proposing = mode == ConsensusMode::proposing;

    notify(protocol::neCLOSING_LEDGER, ledger, !wrongLCL);

    auto const& prevLedger = ledger.ledger_;

    ledgerMaster_.applyHeldTransactions();
//告诉分类帐管理员不要获取我们可能正在建立的分类帐
    ledgerMaster_.setBuildingLedger(prevLedger->info().seq + 1);

    auto initialLedger = app_.openLedger().current();

    auto initialSet = std::make_shared<SHAMap>(
        SHAMapType::TRANSACTION, app_.family(), SHAMap::version{1});
    initialSet->setUnbacked();

//构建包含我们未结分类账中所有交易的shamap
    for (auto const& tx : initialLedger->txs)
    {
        JLOG(j_.trace()) << "Adding open ledger TX " <<
            tx.first->getTransactionID();
        Serializer s(2048);
        tx.first->add(s);
        initialSet->addItem(
            SHAMapItem(tx.first->getTransactionID(), std::move(s)),
            true,
            false);
    }

//向集合添加伪事务
    if ((app_.config().standalone() || (proposing && !wrongLCL)) &&
        ((prevLedger->info().seq % 256) == 0))
    {
//上一个分类帐是标记分类帐，添加伪交易
        auto const validations =
            app_.getValidations().getTrustedForLedger (
                prevLedger->info().parentHash);

        if (validations.size() >= app_.validators ().quorum ())
        {
            feeVote_->doVoting(prevLedger, validations, initialSet);
            app_.getAmendmentTable().doVoting(
                prevLedger, validations, initialSet);
        }
    }

//现在我们需要一个不变的快照
    initialSet = initialSet->snapShot(false);

    if (!wrongLCL)
    {
        std::vector<TxID> proposed;

        initialSet->visitLeaves(
            [&proposed](std::shared_ptr<SHAMapItem const> const& item)
            {
                proposed.push_back(item->key());
            });

        censorshipDetector_.propose(prevLedger->info().seq + 1, std::move(proposed));
    }

//因为下面的移动而需要。
    auto const setHash = initialSet->getHash().as_uint256();

    return Result{
        std::move(initialSet),
        RCLCxPeerPos::Proposal{
            initialLedger->info().parentHash,
            RCLCxPeerPos::Proposal::seqJoin,
            setHash,
            closeTime,
            app_.timeKeeper().closeTime(),
            nodeID_}};
}

void
RCLConsensus::Adaptor::onForceAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration const& closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    doAccept(
        result,
        prevLedger,
        closeResolution,
        rawCloseTimes,
        mode,
        std::move(consensusJson));
}

void
RCLConsensus::Adaptor::onAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration const& closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    app_.getJobQueue().addJob(
        jtACCEPT,
        "acceptLedger",
        [=, cj = std::move(consensusJson) ](auto&) mutable {
//请注意，在此作业期间没有持有或获取任何锁。
//这是因为一般共识保证一旦一个分类账
//被接受，协商一致的结果和按引用状态捕获
//直到调用StartRound（通过
//最终一致性）。
            this->doAccept(
                result,
                prevLedger,
                closeResolution,
                rawCloseTimes,
                mode,
                std::move(cj));
            this->app_.getOPs().endConsensus();
        });
}

void
RCLConsensus::Adaptor::doAccept(
    Result const& result,
    RCLCxLedger const& prevLedger,
    NetClock::duration closeResolution,
    ConsensusCloseTimes const& rawCloseTimes,
    ConsensusMode const& mode,
    Json::Value && consensusJson)
{
    prevProposers_ = result.proposers;
    prevRoundTime_ = result.roundTime.read();

    bool closeTimeCorrect;

    const bool proposing = mode == ConsensusMode::proposing;
    const bool haveCorrectLCL = mode != ConsensusMode::wrongLedger;
    const bool consensusFail = result.state == ConsensusState::MovedOn;

    auto consensusCloseTime = result.position.closeTime();

    if (consensusCloseTime == NetClock::time_point{})
    {
//我们同意在结束时间上意见分歧。
        using namespace std::chrono_literals;
        consensusCloseTime = prevLedger.closeTime() + 1s;
        closeTimeCorrect = false;
    }
    else
    {
//我们约定了一个接近的时间
        consensusCloseTime = effCloseTime(
            consensusCloseTime, closeResolution, prevLedger.closeTime());
        closeTimeCorrect = true;
    }

    JLOG(j_.debug()) << "Report: Prop=" << (proposing ? "yes" : "no")
                     << " val=" << (validating_ ? "yes" : "no")
                     << " corLCL=" << (haveCorrectLCL ? "yes" : "no")
                     << " fail=" << (consensusFail ? "yes" : "no");
    JLOG(j_.debug()) << "Report: Prev = " << prevLedger.id() << ":"
                     << prevLedger.seq();

//——————————————————————————————————————————————————————————————
    std::set<TxID> failed;

//我们希望将事务置于不可预测但具有确定性的顺序中：
//我们使用集合的散列值。
//
//Fixme：使用std:：vector和自定义排序器而不是canonicaltxset？
    CanonicalTXSet retriableTxs{ result.txns.map_->getHash().as_uint256() };

    JLOG(j_.debug()) << "Building canonical tx set: " << retriableTxs.key();

    for (auto const& item : *result.txns.map_)
    {
        try
        {
            retriableTxs.insert(std::make_shared<STTx const>(SerialIter{item.slice()}));
            JLOG(j_.debug()) << "    Tx: " << item.key();
        }
        catch (std::exception const&)
        {
            failed.insert(item.key());
            JLOG(j_.warn()) << "    Tx: " << item.key() << " throws!";
        }
    }

    auto built = buildLCL(prevLedger, retriableTxs, consensusCloseTime,
        closeTimeCorrect, closeResolution, result.roundTime.read(), failed);

    auto const newLCLHash = built.id();
    JLOG(j_.debug()) << "Built ledger #" << built.seq() << ": " << newLCLHash;

//告诉直接连接的对等机我们有一个新的LCL
    notify(protocol::neACCEPTED_LEDGER, built, haveCorrectLCL);

//只要我们与网络同步，就尝试检测尝试
//在审查交易时，通过跟踪哪些交易不能进入
//一段时间后。
    if (haveCorrectLCL && result.state == ConsensusState::Yes)
    {
        std::vector<TxID> accepted;

        result.txns.map_->visitLeaves (
            [&accepted](std::shared_ptr<SHAMapItem const> const& item)
            {
                accepted.push_back(item->key());
            });

//跟踪所有失败或标记为可重试的事务
        for (auto const& r : retriableTxs)
            failed.insert (r.first.getTXID());

        censorshipDetector_.check(std::move(accepted),
            [curr = built.seq(), j = app_.journal("CensorshipDetector"), &failed]
            (uint256 const& id, LedgerIndex seq)
            {
                if (failed.count(id))
                    return true;

                auto const wait = curr - seq;

                if (wait && (wait % censorshipWarnInternal == 0))
                {
                    std::ostringstream ss;
                    ss << "Potential Censorship: Eligible tx " << id
                       << ", which we are tracking since ledger " << seq
                       << " has not been included as of ledger " << curr
                       << ".";

                    if (wait / censorshipWarnInternal == censorshipMaxWarnings)
                    {
                        JLOG(j.error()) << ss.str() << " Additional warnings suppressed.";
                    }
                    else
                    {
                        JLOG(j.warn()) << ss.str();
                    }
                }

                return false;
            });
    }

    if (validating_)
        validating_ = ledgerMaster_.isCompatible(
            *built.ledger_, j_.warn(), "Not validating");

    if (validating_ && !consensusFail &&
        app_.getValidations().canValidateSeq(built.seq()))
    {
        validate(built, result.txns, proposing);
        JLOG(j_.info()) << "CNF Val " << newLCLHash;
    }
    else
        JLOG(j_.info()) << "CNF buildLCL " << newLCLHash;

//看看我们是否可以接受一个分类帐作为完全有效的
    ledgerMaster_.consensusBuilt(
        built.ledger_, result.txns.id(), std::move(consensusJson));

//————————————————————————————————————————————————————————————————
    {
//应用未进入的有争议交易
//
//第一次进入新的交易
//未结分类帐转到验证程序建议的交易记录
//我们信任但不包括在共识中。
//
//这些都是先做的，因为它们最有可能
//在协商一致期间获得同意。他们也是
//在逻辑上比未提及的交易“早”排序
//在上一轮共识中。
//
        bool anyDisputes = false;
        for (auto& it : result.disputes)
        {
            if (!it.second.getOurVote())
            {
//我们投了反对票
                try
                {
                    JLOG(j_.debug())
                        << "Test applying disputed transaction that did"
                        << " not get in " << it.second.tx().id();

                    SerialIter sit(it.second.tx().tx_.slice());
                    auto txn = std::make_shared<STTx const>(sit);

//未被接受的有争议的伪交易
//无法在下一个分类帐中成功应用
                    if (isPseudoTx(*txn))
                        continue;

                    retriableTxs.insert(txn);

                    anyDisputes = true;
                }
                catch (std::exception const&)
                {
                    JLOG(j_.debug())
                        << "Failed to apply transaction we voted NO on";
                }
            }
        }

//建立新的未结分类帐
        auto lock = make_lock(app_.getMasterMutex(), std::defer_lock);
        auto sl = make_lock(ledgerMaster_.peekMutex(), std::defer_lock);
        std::lock(lock, sl);

        auto const lastVal = ledgerMaster_.getValidatedLedger();
        boost::optional<Rules> rules;
        if (lastVal)
            rules.emplace(*lastVal, app_.config().features);
        else
            rules.emplace(app_.config().features);
        app_.openLedger().accept(
            app_,
            *rules,
            built.ledger_,
            localTxs_.getTxSet(),
            anyDisputes,
            retriableTxs,
            tapNONE,
            "consensus",
            [&](OpenView& view, beast::Journal j) {
//在分类帐中填入队列中的交易记录。
                return app_.getTxQ().accept(app_, view);
            });

//在未结分类帐之后向订户发出潜在费用变化的信号
//创建
        app_.getOPs().reportFeeChange();
    }

//————————————————————————————————————————————————————————————————
    {
        ledgerMaster_.switchLCL(built.ledger_);

//这些需要存在吗？
        assert(ledgerMaster_.getClosedLedger()->info().hash == built.id());
        assert(
            app_.openLedger().current()->info().parentHash == built.id());
    }

//————————————————————————————————————————————————————————————————
//我们加入了网络，
//看看我们的关闭时间离其他节点有多近
//关闭时间报告，并更新我们的时钟。
    if ((mode == ConsensusMode::proposing || mode == ConsensusMode::observing) && !consensusFail)
    {
        auto closeTime = rawCloseTimes.self;

        JLOG(j_.info()) << "We closed at "
                        << closeTime.time_since_epoch().count();
        using usec64_t = std::chrono::duration<std::uint64_t>;
        usec64_t closeTotal =
            std::chrono::duration_cast<usec64_t>(closeTime.time_since_epoch());
        int closeCount = 1;

        for (auto const& p : rawCloseTimes.peers)
        {
//Fixme：使用中间值，而不是平均值
            JLOG(j_.info())
                << std::to_string(p.second) << " time votes for "
                << std::to_string(p.first.time_since_epoch().count());
            closeCount += p.second;
            closeTotal += std::chrono::duration_cast<usec64_t>(
                              p.first.time_since_epoch()) *
                p.second;
        }

closeTotal += usec64_t(closeCount / 2);  //四舍五入
        closeTotal /= closeCount;

//使用有符号的时间，因为我们要减去
        using duration = std::chrono::duration<std::int32_t>;
        using time_point = std::chrono::time_point<NetClock, duration>;
        auto offset = time_point{closeTotal} -
            std::chrono::time_point_cast<duration>(closeTime);
        JLOG(j_.info()) << "Our close offset is estimated at " << offset.count()
                        << " (" << closeCount << ")";

        app_.timeKeeper().adjustCloseTime(offset);
    }
}

void
RCLConsensus::Adaptor::notify(
    protocol::NodeEvent ne,
    RCLCxLedger const& ledger,
    bool haveCorrectLCL)
{
    protocol::TMStatusChange s;

    if (!haveCorrectLCL)
        s.set_newevent(protocol::neLOST_SYNC);
    else
        s.set_newevent(ne);

    s.set_ledgerseq(ledger.seq());
    s.set_networktime(app_.timeKeeper().now().time_since_epoch().count());
    s.set_ledgerhashprevious(
        ledger.parentID().begin(),
        std::decay_t<decltype(ledger.parentID())>::bytes);
    s.set_ledgerhash(
        ledger.id().begin(), std::decay_t<decltype(ledger.id())>::bytes);

    std::uint32_t uMin, uMax;
    if (!ledgerMaster_.getFullValidatedRange(uMin, uMax))
    {
        uMin = 0;
        uMax = 0;
    }
    else
    {
//不要宣传我们不愿意服务的分类帐
        uMin = std::max(uMin, ledgerMaster_.getEarliestFetch());
    }
    s.set_firstseq(uMin);
    s.set_lastseq(uMax);
    app_.overlay ().foreach (send_always (
        std::make_shared <Message> (
            s, protocol::mtSTATUS_CHANGE)));
    JLOG (j_.trace()) << "send status change to peer";
}

RCLCxLedger
RCLConsensus::Adaptor::buildLCL(
    RCLCxLedger const& previousLedger,
    CanonicalTXSet& retriableTxs,
    NetClock::time_point closeTime,
    bool closeTimeCorrect,
    NetClock::duration closeResolution,
    std::chrono::milliseconds roundTime,
    std::set<TxID>& failedTxs)
{
    std::shared_ptr<Ledger> built = [&]()
    {
        if (auto const replayData = ledgerMaster_.releaseReplay())
        {
            assert(replayData->parent()->info().hash == previousLedger.id());
            return buildLedger(*replayData, tapNONE, app_, j_);
        }
        return buildLedger(previousLedger.ledger_, closeTime, closeTimeCorrect,
            closeResolution, app_, retriableTxs, failedTxs, j_);
    }();

//根据接受的TXS更新费用计算
    using namespace std::chrono_literals;
    app_.getTxQ().processClosedLedger(app_, *built, roundTime > 5s);

//把账本藏在账本里
    if (ledgerMaster_.storeLedger(built))
        JLOG(j_.debug()) << "Consensus built ledger we already had";
    else if (app_.getInboundLedgers().find(built->info().hash))
        JLOG(j_.debug()) << "Consensus built ledger we were acquiring";
    else
        JLOG(j_.debug()) << "Consensus built new ledger";
    return RCLCxLedger{std::move(built)};
}

void
RCLConsensus::Adaptor::validate(RCLCxLedger const& ledger,
    RCLTxSet const& txns,
    bool proposing)
{
    using namespace std::chrono_literals;
    auto validationTime = app_.timeKeeper().closeTime();
    if (validationTime <= lastValidationTime_)
        validationTime = lastValidationTime_ + 1s;
    lastValidationTime_ = validationTime;

    STValidation::FeeSettings fees;
    std::vector<uint256> amendments;

    auto const& feeTrack = app_.getFeeTrack();
    std::uint32_t fee =
        std::max(feeTrack.getLocalFee(), feeTrack.getClusterFee());

    if (fee > feeTrack.getLoadBase())
        fees.loadFee = fee;

//下一个分类帐是标志分类帐
    if (((ledger.seq() + 1) % 256) == 0)
    {
//建议费用更改和新功能
        feeVote_->doValidation(ledger.ledger_, fees);
        amendments = app_.getAmendmentTable().doValidation (getEnabledAmendments(*ledger.ledger_));
    }

    auto v = std::make_shared<STValidation>(
        ledger.id(),
        ledger.seq(),
        txns.id(),
        validationTime,
        valPublic_,
        valSecret_,
        nodeID_,
        /*如果提议，摆出/*满的姿势*/，
        费用，
        修正案；

    //如果我们收到它，就取消它
    app_.gethashrouter（）.addsuppression（）添加抑制
        sha512half（makeslice（v->getSerialized（））；
    手动验证（app_uuv，“本地”）；
    blob validation=v->getSerialized（）；
    协议：：tmvalidation val；
    val.set_validation（&validation[0]，validation.size（））；
    //向所有直接连接的对等端发送签名验证
    应用程序覆盖（）.send（val）；
}

无效
rclcommersion:：adapter:：onmodechange（）。
    之前达成共识，
    协商一致）
{
    jLog（j_.info（））<“共识模式更改前=”<<to_string（前）
                    <<“，after=”<<to_string（after）；

    //如果我们正在提议但不再提议，我们需要重置
    //审查跟踪以避免虚假警告。
    如果（（before==conensusmode:：proposing before==conensusmode:：observation）&&before！=后）
        审查检测器重置（）；

    模=后；
}

JSON:：价值
rclcommersion:：getjson（bool full）const
{
    json:：value ret；
    {
      ScopedLockType_123;mutex_125;
      ret=协商一致（完整）；
    }
    ret[“validating”]=适配器_u.validating（）；
    返回RET；
}

无效
rclcommersion:：timerentry（netclock:：time_point const&now）
{
    尝试
    {
        ScopedLockType_123;mutex_125;
        共识时间（现在）；
    }
    捕获（shamapmissingnode const&mn）
    {
        //这不应该发生
        jLog（j_.error（））<“协商过程中缺少节点”<<mn；
        重新抛出（）；
    }
}

无效
rclcommersiste:：gottxset（netclock:：time_point const&now，rcltxset const&txset）
{
    尝试
    {
        ScopedLockType_123;mutex_125;
        协商一致（现在，txset）；
    }
    捕获（shamapmissingnode const&mn）
    {
        //这不应该发生
        jLog（j_.error（））<“协商过程中缺少节点”<<mn；
        重新抛出（）；
    }
}


/ /！@见共识：：模拟

无效
模拟（
    netclock:：time_point const&now，
    boost：：可选<std:：chrono:：millises>conensusdelay）
{
    ScopedLockType_123;mutex_125;
    共识模拟（现在，共识延迟）；
}

布尔
RCL共识：对等提议（
    netclock:：time_point const&now，
    RCLCxPEERPOS施工和新方案）
{
    ScopedLockType_123;mutex_125;
    返回共识对等提案（现在，新提案）；
}

布尔
rclcommersion:：adapter:：prestartround（rclcxleger const&prevlgr）
{
    //我们有一个密钥，不希望在重新启动后进行不同步验证
    //且不阻止修正。
    正在验证_u=valpublic_u.size（）！= 0 & &
                  prevlgr.seq（）>=应用程序getmaxdisallowedledger（）&&
                  ！app_.getops（）.isamendmentBlocked（）；

    //如果我们没有在独立模式下运行，并且有一个已配置的unl，
    //检查以确保它没有过期。
    如果（正在验证&！app_.config（）.standalone（）&&app_u.validators（）.count（））
    {
        auto const when=app_.validators（）.expires（）；

        如果（！）when*when<app timekeeper（）.now（））
        {
            jLog（j_.error（））<“自愿退出协商进程”
                                “因为验证程序列表过期。”；
            验证_=false；
        }
    }

    const bool synced=app_.getops（）.getOperatingMode（）==networkops:：omfull；

    如果（验证）
    {
        jLog（j_.info（））<“Entering consension process，validation，synced=”
                        <（同步）？是的：“不”；
    }
    其他的
    {
        //否则我们只想监视验证过程。
        jLog（j_.info（））<“Entering consension process，watching，synced=”
                        <（同步）？是的：“不”；
    }

    //通知入站分类帐我们正在开始新一轮
    InboundTransactions_.NewRound（prevlgr.seq（））；

    //使用父分类帐规则确定是否使用舍入关闭时间
    parms_u.useroundedCloseTime=prevlgr.ledger_u->rules（）。已启用（fix1528）；

    //仅当我们与网络同步时提出建议（并验证）
    返回验证&&synced；
}

无效
rclcommersiste:：startround（
    netclock:：time_point const&now，
    rccxledger:：id const和prevlgrid，
    RCLCxLedger Const&Prevlgr公司
    哈希集<nodeid>const&nowuntrusted）
{
    ScopedLockType_123;mutex_125;
    共识启动轮（
        现在，prevlgrid，prevlgr，nowuntrusted，adapter_u.prestartround（prevlgr））；
}
}
