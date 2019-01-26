
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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
#ifndef RIPPLE_TEST_CSF_PEER_H_INCLUDED
#define RIPPLE_TEST_CSF_PEER_H_INCLUDED

#include <ripple/beast/utility/WrappedSink.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/consensus/Validations.h>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <algorithm>
#include <test/csf/CollectorRef.h>
#include <test/csf/Scheduler.h>
#include <test/csf/TrustGraph.h>
#include <test/csf/Tx.h>
#include <test/csf/Validation.h>
#include <test/csf/events.h>
#include <test/csf/ledgers.h>

namespace ripple {
namespace test {
namespace csf {

namespace bc = boost::container;

/*模拟中的单个对等点。

    这是共识模拟框架的主要工作，是
    集成了许多其他组件。同辈

     -执行协商一致要求的回调
     -管理与其他对等方的信任和网络连接
     -根据其分析操作将事件发回模拟
       收藏家
     -显示大多数内部状态以强制模拟任意场景
**/

struct Peer
{
    /*对等方所采取的建议位置的基本包装。

        为了达成真正的共识，这将为序列化添加额外的数据。
        并签署。对于模拟，不需要额外的东西。
    **/

    class Position
    {
    public:
        Position(Proposal const& p) : proposal_(p)
        {
        }

        Proposal const&
        proposal() const
        {
            return proposal_;
        }

        Json::Value
        getJson() const
        {
            return proposal_.getJson();
        }

    private:
        Proposal proposal_;
    };


    /*内部对等处理中的模拟延迟。
     **/

    struct ProcessingDelays
    {
//！延迟协商一致要求DOACCEPT接受和签发
//！验证
//！TODO:这应该是事务数的函数
        std::chrono::milliseconds ledgerAccept{0};

//！延迟处理来自远程对等方的验证
        std::chrono::milliseconds recvValidation{0};

//返回消息类型m的接收延迟，默认为无延迟
//接收延迟是指从接收消息到实际
//处理它。
        template <class M>
        SimDuration
        onReceive(M const&) const
        {
            return SimDuration{};
        }

        SimDuration
        onReceive(Validation const&) const
        {
            return recvValidation;
        }
    };

    /*只忽略最近过时验证的通用验证适配器
    **/

    class ValAdaptor
    {
        Peer& p_;

    public:
        struct Mutex
        {
            void
            lock()
            {
            }

            void
            unlock()
            {
            }
        };

        using Validation = csf::Validation;
        using Ledger = csf::Ledger;

        ValAdaptor(Peer& p) : p_{p}
        {
        }

        NetClock::time_point
        now() const
        {
            return p_.now();
        }

        void
        onStale(Validation&& v)
        {
        }

        void
        flush(hash_map<PeerID, Validation>&& remaining)
        {
        }

        boost::optional<Ledger>
        acquire(Ledger::ID const & id)
        {
            if(Ledger const * ledger = p_.acquireLedger(id))
                return *ledger;
            return boost::none;
        }
    };

//！通用共识的类型定义
    using Ledger_t = Ledger;
    using NodeID_t = PeerID;
    using TxSet_t = TxSet;
    using PeerPosition_t = Position;
    using Result = ConsensusResult<Peer>;

//！以对等ID作为消息前缀的日志记录支持
    beast::WrappedSink sink;
    beast::Journal j;

//！一般共识
    Consensus<Peer> consensus;

//！我们独特的身份证
    PeerID id;

//！当前签名密钥
    PeerKey key;

//！管理独特分类账的Oracle
    LedgerOracle& oracle;

//！事件调度程序
    Scheduler& scheduler;

//！用于发送消息的网络句柄
    BasicNetwork<Peer*>& net;

//！网络信任图处理
    TrustGraph<Peer*>& trustGraph;

//！尚未在分类帐中关闭的OpenTXS
    TxSetType openTxs;

//！此节点关闭的最后一个分类帐
    Ledger lastClosedLedger;

//！此节点已关闭或从网络加载的分类帐
    hash_map<Ledger::ID, Ledger> ledgers;

//！来自受信任节点的验证
    Validations<ValAdaptor> validations;

//！网络已从中完全验证的最新分类帐
//！这位同行的观点
    Ledger fullyValidatedLedger;

//————————————————————————————————————————————————————————————————
//存储大多数网络消息；如果使用内存，则可以清除这些消息
//变得有问题

//！从分类帐映射：ID到具有该分类帐的位置矢量
//！作为上一个分类帐
    bc::flat_map<Ledger::ID, std::vector<Proposal>> peerPositions;
//！与txset:：id关联的txset
    bc::flat_map<TxSet::ID, TxSet> txSets;

//我们正在获取的分类账/txset，当请求超时时
    bc::flat_map<Ledger::ID,SimTime> acquiringLedgers;
    bc::flat_map<TxSet::ID,SimTime> acquiringTxSets;

//！此对等方已完成的分类帐数目
    int completedLedgers = 0;

//！在停止运行之前，此对等方应完成的分类帐的数目
    int targetLedgers = std::numeric_limits<int>::max();

//！相对于公共调度程序时钟的时间偏差
    std::chrono::seconds clockSkew{0};

//！用于内部处理的模拟延迟
    ProcessingDelays delays;

//！是否模拟作为验证程序或跟踪节点运行
    bool runAsValidator = true;

//！对验证序列号强制不变量
    SeqEnforcer<Ledger::Seq> seqEnforcer;

//托多：考虑删除这两个，它们只是测试的一个方便。
//上一轮提案人数量
    std::size_t prevProposers = 0;
//上一轮的持续时间
    std::chrono::milliseconds prevRoundTime;

//完全验证分类帐所需的验证法定人数
//TODO:使用validatorlist中的逻辑动态设置
    std::size_t quorum = 0;

//仿真参数
    ConsensusParms consensusParms;

//！要向其报告事件的收集器
    CollectorRefs & collectors;

    /*构造函数

        @param i唯一的peerid
        @param s模拟调度程序
        @param o模拟Oracle
        @param n模拟网络
        @param tg模拟信任图
        @param c模拟收集器
        @param jin模拟期刊

    **/

    Peer(
        PeerID i,
        Scheduler& s,
        LedgerOracle& o,
        BasicNetwork<Peer*>& n,
        TrustGraph<Peer*>& tg,
        CollectorRefs& c,
        beast::Journal jIn)
        : sink(jIn, "Peer " + to_string(i) + ": ")
        , j(sink)
        , consensus(s.clock(), *this, j)
        , id{i}
        , key{id, 0}
        , oracle{o}
        , scheduler{s}
        , net{n}
        , trustGraph(tg)
        , lastClosedLedger{Ledger::MakeGenesis{}}
        , validations{ValidationParms{}, s.clock(), *this}
        , fullyValidatedLedger{Ledger::MakeGenesis{}}
        , collectors{c}
    {
//所有对等方都从默认构造的Genesis Ledger开始
        ledgers[lastClosedLedger.id()] = lastClosedLedger;

//节点总是信任自己。他们应该吗？
        trustGraph.trust(this, this);
    }

    /*在“when”持续时间中计划提供的回调，但如果
        `when`为0，立即调用
    **/

    template <class T>
    void
    schedule(std::chrono::nanoseconds when, T&& what)
    {
        using namespace std::chrono_literals;

        if (when == 0ns)
            what();
        else
            scheduler.in(when, std::forward<T>(what));
    }

//向收集器发布新事件
    template <class E>
    void
    issue(E const & event)
    {
//使用调度程序时间而不是对等机的（倾斜的）本地时间
        collectors.on(id, scheduler.now(), event);
    }

//——————————————————————————————————————————————————————————————
//信任和网络成员
//修改和查询网络图和信任图的方法
//这位同行的观点

//<将信任扩展到对等端
    void
    trust(Peer & o)
    {
        trustGraph.trust(this, &o);
    }

//<从对等方撤消信任
    void
    untrust(Peer & o)
    {
        trustGraph.untrust(this, &o);
    }

//<检查我们是否信任对等
    bool
    trusts(Peer & o)
    {
        return trustGraph.trusts(this, &o);
    }

//<检查我们是否信任基于其ID的对等方
    bool
    trusts(PeerID const & oId)
    {
        for(auto const & p : trustGraph.trustedPeers(this))
            if(p->id == oId)
                return true;
        return false;
    }

    /*创建网络连接

        如果不存在，则创建到另一个对等机的新出站连接

        @param o具有入站连接的对等机
        @param dur两个对等方之间消息的固定延迟
        @返回是否创建了连接。
    **/


    bool
    connect(Peer & o, SimDuration dur)
    {
        return net.connect(this, &o, dur);
    }

    /*删除网络连接

        删除对等端之间的连接（如果存在）

        @param o与我们断开连接的对等机
        @返回连接是否被删除
    **/

    bool
    disconnect(Peer & o)
    {
        return net.disconnect(this, &o);
    }

//——————————————————————————————————————————————————————————————
//一般共识成员

//尝试获取与给定ID关联的分类帐
    Ledger const*
    acquireLedger(Ledger::ID const& ledgerID)
    {
        auto it = ledgers.find(ledgerID);
        if (it != ledgers.end())
            return &(it->second);

//没有对等体
        if(net.links(this).empty())
            return nullptr;

//如果我们已经在获取它，但尚未超时，请不要重试。
        auto aIt = acquiringLedgers.find(ledgerID);
        if(aIt!= acquiringLedgers.end())
        {
            if(scheduler.now() < aIt->second)
                return nullptr;
        }

        using namespace std::chrono_literals;
        SimDuration minDuration{10s};
        for (auto const& link : net.links(this))
        {
            minDuration = std::min(minDuration, link.data.delay);

//给邻居们送一封信去查账本
            net.send(
                this, link.target, [ to = link.target, from = this, ledgerID ]() {
                    auto it = to->ledgers.find(ledgerID);
                    if (it != to->ledgers.end())
                    {
//如果找到分类帐，请将其发回原始分类帐
//请求将其添加到可用的
//分类帐
                        to->net.send(to, from, [ from, ledger = it->second ]() {
                            from->acquiringLedgers.erase(ledger.id());
                            from->ledgers.emplace(ledger.id(), ledger);
                        });
                    }
                });
        }
        acquiringLedgers[ledgerID] = scheduler.now() + 2 * minDuration;
        return nullptr;
    }

//尝试获取与给定ID关联的TxSet
    TxSet const*
    acquireTxSet(TxSet::ID const& setId)
    {
        auto it = txSets.find(setId);
        if (it != txSets.end())
            return &(it->second);

//没有对等体
        if(net.links(this).empty())
            return nullptr;

//如果我们已经在获取它，但尚未超时，请不要重试。
        auto aIt = acquiringTxSets.find(setId);
        if(aIt!= acquiringTxSets.end())
        {
            if(scheduler.now() < aIt->second)
                return nullptr;
        }

        using namespace std::chrono_literals;
        SimDuration minDuration{10s};
        for (auto const& link : net.links(this))
        {
            minDuration = std::min(minDuration, link.data.delay);
//向邻居发送消息以查找Tx集
            net.send(
                this, link.target, [ to = link.target, from = this, setId ]() {
                    auto it = to->txSets.find(setId);
                    if (it != to->txSets.end())
                    {
//如果找到了txset，请将其发送回原始
//请求对等机，在这里它像一个TxSet一样被处理
//那是通过网络广播的
                        to->net.send(to, from, [ from, txSet = it->second ]() {
                            from->acquiringTxSets.erase(txSet.id());
                            from->handle(txSet);
                        });
                    }
                });
        }
        acquiringTxSets[setId] = scheduler.now() + 2 * minDuration;
        return nullptr;
    }

    bool
    hasOpenTransactions() const
    {
        return !openTxs.empty();
    }

    std::size_t
    proposersValidated(Ledger::ID const& prevLedger)
    {
        return validations.numTrustedForLedger(prevLedger);
    }

    std::size_t
    proposersFinished(Ledger const & prevLedger, Ledger::ID const& prevLedgerID)
    {
        return validations.getNodesAfter(prevLedger, prevLedgerID);
    }

    Result
    onClose(
        Ledger const& prevLedger,
        NetClock::time_point closeTime,
        ConsensusMode mode)
    {
        issue(CloseLedger{prevLedger, openTxs});

        return Result(
            TxSet{openTxs},
            Proposal(
                prevLedger.id(),
                Proposal::seqJoin,
                TxSet::calcID(openTxs),
                closeTime,
                now(),
                id));
    }

    void
    onForceAccept(
        Result const& result,
        Ledger const& prevLedger,
        NetClock::duration const& closeResolution,
        ConsensusCloseTimes const& rawCloseTimes,
        ConsensusMode const& mode,
        Json::Value&& consensusJson)
    {
        onAccept(
            result,
            prevLedger,
            closeResolution,
            rawCloseTimes,
            mode,
            std::move(consensusJson));
    }

    void
    onAccept(
        Result const& result,
        Ledger const& prevLedger,
        NetClock::duration const& closeResolution,
        ConsensusCloseTimes const& rawCloseTimes,
        ConsensusMode const& mode,
        Json::Value&& consensusJson)
    {
        schedule(delays.ledgerAccept, [=]() {
            const bool proposing = mode == ConsensusMode::proposing;
            const bool consensusFail = result.state == ConsensusState::MovedOn;

            TxSet const acceptedTxs = injectTxs(prevLedger, result.txns);
            Ledger const newLedger = oracle.accept(
                prevLedger,
                acceptedTxs.txs(),
                closeResolution,
                result.position.closeTime());
            ledgers[newLedger.id()] = newLedger;

            issue(AcceptLedger{newLedger, lastClosedLedger});
            prevProposers = result.proposers;
            prevRoundTime = result.roundTime.read();
            lastClosedLedger = newLedger;

            auto const it = std::remove_if(
                openTxs.begin(), openTxs.end(), [&](Tx const& tx) {
                    return acceptedTxs.exists(tx.id());
                });
            openTxs.erase(it, openTxs.end());

//仅当新分类帐与我们的
//完全验证的分类帐
            bool const isCompatible =
                newLedger.isAncestor(fullyValidatedLedger);

//每个序列只能发送一个已验证的分类帐
            if (runAsValidator && isCompatible && !consensusFail &&
                seqEnforcer(
                    scheduler.now(), newLedger.seq(), validations.parms()))
            {
                bool isFull = proposing;

                Validation v{newLedger.id(),
                             newLedger.seq(),
                             now(),
                             now(),
                             key,
                             id,
                             isFull};
//共享新的验证；它受接收者信任
                share(v);
//我们相信自己
                addTrustedValidation(v);
            }

            checkFullyValidated(newLedger);

//开始下一轮……
//在实际的实现中，这会传递回
//网络操作系统
            ++completedLedgers;
//startround设置LCL状态，因此我们需要在
//最后一轮请求完成
            if (completedLedgers <= targetLedgers)
            {
                startRound();
            }
        });
    }

//检查具有更多的分类帐时允许的最早序列号
//比我们当前的分类账有效
    Ledger::Seq
    earliestAllowedSeq() const
    {
        return fullyValidatedLedger.seq();
    }

    Ledger::ID
    getPrevLedger(
        Ledger::ID const& ledgerID,
        Ledger const& ledger,
        ConsensusMode mode)
    {
//只有当我们通过创世纪分类账
        if (ledger.seq() == Ledger::Seq{0})
            return ledgerID;

        Ledger::ID const netLgr =
            validations.getPreferred(ledger, earliestAllowedSeq());

        if (netLgr != ledgerID)
        {
            JLOG(j.trace()) << Json::Compact(validations.getJsonTrie());
            issue(WrongPrevLedger{ledgerID, netLgr});
        }

        return netLgr;
    }

    void
    propose(Proposal const& pos)
    {
        share(pos);
    }

    ConsensusParms const&
    parms() const
    {
        return consensusParms;
    }

//目前对跟踪共识模式变化不感兴趣
    void
    onModeChange(ConsensusMode, ConsensusMode)
    {
    }

//通过向所有连接的对等端广播来共享消息
    template <class M>
    void
    share(M const& m)
    {
        issue(Share<M>{m});
        send(BroadcastMesg<M>{m,router.nextSeq++, this->id}, this->id);
    }

//打开职位并分享原始提议
    void
    share(Position const & p)
    {
        share(p.proposal());
    }

//——————————————————————————————————————————————————————————————
//验证成员

    /*添加可信验证，如果值得转发，则返回true*/
    bool
    addTrustedValidation(Validation v)
    {
        v.setTrusted();
        v.setSeen(now());
        ValStatus const res = validations.add(v.nodeID(), v);

        if(res == ValStatus::stale)
            return false;

//如果不是本地的，Acquire将尝试从网络获取
        if (Ledger const* lgr = acquireLedger(v.ledgerID()))
            checkFullyValidated(*lgr);
        return true;
    }

    /*检查新分类账是否可以被视为完全有效。*/
    void
    checkFullyValidated(Ledger const& ledger)
    {
//只考虑比上次完全验证的分类帐更新的分类帐
        if (ledger.seq() <= fullyValidatedLedger.seq())
            return;

        std::size_t const count = validations.numTrustedForLedger(ledger.id());
        std::size_t const numTrustedPeers = trustGraph.graph().outDegree(this);
        quorum = static_cast<std::size_t>(std::ceil(numTrustedPeers * 0.8));
        if (count >= quorum && ledger.isAncestor(fullyValidatedLedger))
        {
            issue(FullyValidateLedger{ledger, fullyValidatedLedger});
            fullyValidatedLedger = ledger;
        }
    }

//————————————————————————————————————————————————————————————————
//对等消息成员

//基本序列号路由器
//将在整个网络中被淹没的消息被标记为
//按广播中的源节点列出的影响数。接收者意志
//如果消息已经处理了更新的序列，则将其忽略为过时消息
//或者将处理并可能转发消息。
//
//各种bool句柄（messagetype）成员执行实际处理
//如果消息继续发送给对等方，则返回true。
//
//警告：这假定消息是按照它们的顺序接收和处理的。
//发送，以便对等端从节点0接收带有seq 1的消息
//在节点0的seq 2之前，等等。
//TODO:将其分解为类并标识类型接口以允许
//备选路由策略
    template <class M>
    struct BroadcastMesg
    {
        M mesg;
        std::size_t seq;
        PeerID origin;
    };

    struct Router
    {
        std::size_t nextSeq = 1;
        bc::flat_map<PeerID, std::size_t> lastObservedSeq;
    };

    Router router;

//向所有对等端发送广播消息
    template <class M>
    void
    send(BroadcastMesg<M> const& bm, PeerID from)
    {
        for (auto const& link : net.links(this))
        {
            if (link.target->id != from && link.target->id != bm.origin)
            {
//如果我们知道它已经存在，就不要再发送了
//在另一端使用
                if (link.target->router.lastObservedSeq[bm.origin] < bm.seq)
                {
                    issue(Relay<M>{link.target->id, bm.mesg});
                    net.send(
                        this, link.target, [to = link.target, bm, id = this->id ] {
                            to->receive(bm, id);
                        });
                }
            }
        }
    }

//接收共享消息，处理它并考虑继续转发它
    template <class M>
    void
    receive(BroadcastMesg<M> const& bm, PeerID from)
    {
        issue(Receive<M>{from, bm.mesg});
        if (router.lastObservedSeq[bm.origin] < bm.seq)
        {
            router.lastObservedSeq[bm.origin] = bm.seq;
            schedule(delays.onReceive(bm.mesg), [this, bm, from]
            {
                if (handle(bm.mesg))
                    send(bm, from);
            });
        }
    }

//类型特定的接收处理程序，如果消息应该
//继续向同行广播
    bool
    handle(Proposal const& p)
    {
//仅在同一分类帐上中继不受信任的建议
        if(!trusts(p.nodeID()))
            return p.prevLedger() == lastClosedLedger.id();

//TODO:这总是抑制已经看到的对等位置的中继
//如果是最近的分类帐，是否允许转发？
        auto& dest = peerPositions[p.prevLedger()];
        if (std::find(dest.begin(), dest.end(), p) != dest.end())
            return false;

        dest.push_back(p);

//依靠协商一致决定是否转播
        return consensus.peerProposal(now(), Position{p});
    }

    bool
    handle(TxSet const& txs)
    {
        auto const it = txSets.insert(std::make_pair(txs.id(), txs));
        if (it.second)
            consensus.gotTxSet(now(), txs);
//只有新的继电器
        return it.second;
    }

    bool
    handle(Tx const& tx)
    {
//忽略并禁止中继上一个分类帐中已存在的交易记录
        TxSetType const& lastClosedTxs = lastClosedLedger.txs();
        if (lastClosedTxs.find(tx) != lastClosedTxs.end())
            return false;

//只有当它是我们的未结分类帐的新的时候才转帐
        return openTxs.insert(tx).second;

    }

    bool
    handle(Validation const& v)
    {
//TODO:这不是中继不受信任的验证
        if (!trusts(v.nodeID()))
            return false;

//只有当电流
        return addTrustedValidation(v);
    }

//——————————————————————————————————————————————————————————————
//本地提交的事务
    void
    submit(Tx const& tx)
    {
        issue(SubmitTx{tx});
        if(handle(tx))
            share(tx);
    }

//——————————————————————————————————————————————————————————————
//模拟“驾驶员”成员

//！心跳计时器呼叫
    void
    timerEntry()
    {
        consensus.timerEntry(now());
//仅在未完成时重新安排
        if (completedLedgers < targetLedgers)
            scheduler.in(parms().ledgerGRANULARITY, [this]() { timerEntry(); });
    }

//打电话开始下一轮
    void
    startRound()
    {
//在两轮之间，我们取多数分类账
//将来，如果没有验证，考虑采用对等主分类账。
//然而
        Ledger::ID bestLCL =
            validations.getPreferred(lastClosedLedger, earliestAllowedSeq());
        if(bestLCL == Ledger::ID{0})
            bestLCL = lastClosedLedger.id();

        issue(StartRound{bestLCL, lastClosedLedger});

//尚未建模动态UNL。
        hash_set<PeerID> nowUntrusted;
        consensus.startRound(
            now(), bestLCL, lastClosedLedger, nowUntrusted, runAsValidator);
    }

//假设协商进程尚未运行，则启动协商进程。
//除非指定TargetLedgers，否则将永远运行
    void
    start()
    {
//TODO:验证过期的频率较低？
        validations.expire();
        scheduler.in(parms().ledgerGRANULARITY, [&]() { timerEntry(); });
        startRound();
    }

    NetClock::time_point
    now() const
    {
//我们不关心实际的时代，但确实希望
//生成的netclock时间远远超过其时代，以确保
//共识中两个netclock:：time_点的任何减法
//代码是正的。（例如，提议性）
        using namespace std::chrono;
        using namespace std::chrono_literals;
        return NetClock::time_point(duration_cast<NetClock::duration>(
            scheduler.now().time_since_epoch() + 86400s + clockSkew));
    }


    Ledger::ID
    prevLedgerID() const
    {
        return consensus.prevLedgerID();
    }

//————————————————————————————————————————————————————————————————
//在生成以下分类帐时插入特定交易记录
//提供的序列。这允许模拟拜占庭式故障
//哪一个节点生成了错误的分类账，即使在协商一致的情况下
//适当地。
//托多：让这个更强大
    hash_map<Ledger::Seq, Tx> txInjections;

    /*注入非协商一致的Tx

        按照上一个分类账的顺序将交易注入分类账
        号码。

        @param prevledger我们正在创建的新分类帐位于
        @param src共识txset
        @返回共识txtset，如果prevledger.seq，则添加inject事务
                与以前注册的Tx匹配。
    **/

    TxSet
    injectTxs(Ledger prevLedger, TxSet const & src)
    {
        auto const it = txInjections.find(prevLedger.seq());

        if(it == txInjections.end())
            return src;
        TxSetType res{src.txs()};
        res.insert(it->second);

        return TxSet{res};

    }
};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹
#endif

