
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

#ifndef RIPPLE_CONSENSUS_CONSENSUS_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/consensus/ConsensusParms.h>
#include <ripple/consensus/ConsensusTypes.h>
#include <ripple/consensus/LedgerTiming.h>
#include <ripple/consensus/DisputedTx.h>
#include <ripple/json/json_writer.h>

namespace ripple {


/*确定当前分类帐此时是否应关闭。

    当分类帐打开且没有关闭时，应调用此函数
    正在进行中，或当接收到事务且没有正在进行的关闭时。

    @param any transactions指示是否已收到任何事务
    @param prepropositers上一个结束语中的propositers
    @param propositioners已关闭当前已关闭此分类帐的建议者
    @param proposers验证了最后一次结束验证的proposers
                              分类帐
    @param上一个分类账达成共识的前一个时间
    @param timesinceprevclose time since the previous ledger's（possible rounded）
                        关闭时间
    @param opentime duration此分类帐已打开
    @param idle interval网络所需的空闲时间间隔
    @param parms共识常量参数
    @param j日志
**/

bool
shouldCloseLedger(
    bool anyTransactions,
    std::size_t prevProposers,
    std::size_t proposersClosed,
    std::size_t proposersValidated,
    std::chrono::milliseconds prevRoundTime,
    std::chrono::milliseconds timeSincePrevClose,
    std::chrono::milliseconds openTime,
    std::chrono::milliseconds idleInterval,
    ConsensusParms const & parms,
    beast::Journal j);

/*确定网络是否达成共识，我们是否加入。

    @param prepropositers上一次收盘时的提议者（不包括我们）
    目前为止，@param currentproposers在此结束语中的建议者（不包括我们）
    @param current同意我们的提议者
    @param currentfinished proposers，在这之后验证了分类账
    @param previousagreetime在
                             最后分类帐
    @param currentagreetime我们试图
                            同意
    @param parms共识常量参数
    @param提议我们是否应该数一数
    @param j日志
**/

ConsensusState
checkConsensus(
    std::size_t prevProposers,
    std::size_t currentProposers,
    std::size_t currentAgree,
    std::size_t currentFinished,
    std::chrono::milliseconds previousAgreeTime,
    std::chrono::milliseconds currentAgreeTime,
    ConsensusParms const & parms,
    bool proposing,
    beast::Journal j);

/*共识算法的一般实现。

  在下一个分类账上达成共识。

  有两件事需要达成共识：

    1。包括在分类帐中的一组交易记录。
    2。分类帐的关闭时间。

  基本流程：

    1。调用“startround”会将节点置于“open”阶段。在这
       阶段，节点正在等待事务包括在其打开状态中
       分类帐。
    2。连续调用“TimeRentry”检查节点是否可以关闭分类帐。
       一旦节点“关闭”为打开的分类帐，它将转换为
       “建立”阶段。在此阶段，节点共享/接收对等端
       有关应在已结算分类帐中接受哪些交易的建议。
    三。在随后调用“timerentry”的过程中，节点确定它
       就交易内容与同行达成共识。它
       过渡到“accept”阶段。在这个阶段，节点工作在
       将交易应用于上一个分类帐以生成新的已结算
       分类帐。新分类帐完成后，节点共享已验证的
       与网络进行分类帐，做一些簿记，然后打电话给
       `startround`重新开始循环。

  这个类使用一个通用接口来允许为特定的
  应用。适配器模板实现一组助手函数，这些函数
  将共识算法插入特定的应用程序。它还可以识别
  在共识（交易、分类账等）中起重要作用的类型。
  下面的代码存根概述了接口和类型要求。性状
  类型必须是可复制构造和可分配的。

  @警告通用实现不是线程安全的，公共方法
  不打算同时运行。在并发环境中，
  应用程序负责确保线程安全。简单锁定
  无论何时接触共识实例都是一种选择。

  @代码
  //单个事务
  TX结构
  {
    //事务的唯一标识符
    使用ID＝…

    ID-（）

  }；

  //一组事务
  结构集
  {
    //txset的唯一ID（不属于tx）
    使用ID＝…
    //包含txset的单个事务的类型
    使用TX＝Tx；

    bool exists（tx:：id const&）const；
    //返回值的语义应该类似于tx const*
    tx const*查找（tx:：id const&）const；
    id const&id（）常量；

    //返回此集合或其他集合不通用的事务集
    //布尔值指示它所在的集合
    std:：map<tx:：id，bool>比较（txset const&other）const；

    //事务的可变视图
    结构可变TxSet
    {
        可变txset（txset const&）；
        布尔插入（tx const&）；
        布尔擦除（tx:：id const&）；
    }；

    //从可变视图构造。
    txset（可变txset const&）；

    //或者，如果txset本身是可变的
    //仅别名mutabletxset=txset

  }；

  //同意声明共识交易将修改
  Ledger结构
  {
    使用ID＝…
    使用seq=…；

    //Ledgerr的唯一标识符
    id const id（）常量；
    seq seq（）常量；
    自动关闭时间分辨率（）常量；
    auto closeAgree（）常量；
    自动关闭时间（）常量；
    auto parentclosetime（）常量；
    json:：value getjson（）常量；
  }；

  //包装同伴的共识
  结构对等位置
  {
    一致意见
        std:：uint32_t，//节点ID，
        类型名称分类帐：：ID，
        类型名txset:：id>const&
    建议（）常量；

  }；


  类适配器
  {
  公众：
      //——————————————————————————————————————————————————————————————————————————————————————————————————————————————————
      //定义共识类型
      使用分类帐_t=分类帐；
      使用nodeid_t=std:：uint32_t；
      使用txset_t=txset；
      使用对等位置_t=对等位置；

      //——————————————————————————————————————————————————————————————————————————————————————————————————————————————————
      / /
      //尝试获取特定的分类帐。
      boost：：可选<ledger>AcquireReledger（ledger:：id const&ledger id）；

      //获取与建议位置关联的事务集。
      boost：：可选<txset>acquiretxset（txset:：id const&setid）；

      //是否有任何交易记录在未结分类账中
      bool hasOpenTransactions（）常量；

      //已验证给定分类帐的建议者数
      std：：大小建议已验证（分类帐：：id const&prevledger）const；

      //验证了从
      //给定分类帐；如果prevledger.id（）！=prevelegrid，使用prevelegrid
      //用于确定
      std：：大小建议完成（分类帐常量和前分类帐，
                                    分类帐：：id const&prevledger）const；

      //返回上一个已关闭（和已验证）分类帐的ID，
      //应用程序认为共识应用作上一个分类账。
      分类帐：：id getPrevLedger（分类帐：：id const&prevLedgerID，
                      分类帐构造和前分类帐，
                      模式模式）；

      //在共识操作模式更改时调用
      模式变更无效（之前达成共识，之后达成共识）；

      //分类帐关闭时调用
      结果onclose（分类帐const&，分类帐const&prev，模式模式）；

      //一致接受分类帐时调用
      接受无效（结果常量和结果，
        RCLCX分类帐构造和预览分类帐，
        netclock：：持续时间关闭分辨率，
        关闭时间常数和关闭时间，
        模式常数和模式）；

      //通过模拟强制接受分类帐时调用
      //函数。
      void onforceAccept（结果常量和结果，
        RCLCX分类帐构造和预览分类帐，
        netclock：：持续时间关闭分辨率，
        关闭时间常数和关闭时间，
        模式常数和模式）；

      向同行提出职位建议。
      无效提议（同意的提议<…>const&pos）；

      //与其他对等方共享收到的对等方建议。
      无效份额（对等位置构建与支持）；

      //与对等方共享有争议的事务
      无效股份（txn const&tx）；

      //与对等方共享给定的事务集
      无效份额（txset const&s）；

      //一致定时参数和常量
      协商一致
      PARMS（）；
  }；
  @端码

  @tparam adapter定义类型，并提供适应
                  对更大的应用达成共识。
**/

template <class Adaptor>
class Consensus
{
    using Ledger_t = typename Adaptor::Ledger_t;
    using TxSet_t = typename Adaptor::TxSet_t;
    using NodeID_t = typename Adaptor::NodeID_t;
    using Tx_t = typename TxSet_t::Tx;
    using PeerPosition_t = typename Adaptor::PeerPosition_t;
    using Proposal_t = ConsensusProposal<
        NodeID_t,
        typename Ledger_t::ID,
        typename TxSet_t::ID>;

    using Result = ConsensusResult<Adaptor>;

//帮助程序类，以确保在达成一致意见时通知适配器
//变化
    class MonitoredMode
    {
        ConsensusMode mode_;

    public:
        MonitoredMode(ConsensusMode m) : mode_{m}
        {
        }
        ConsensusMode
        get() const
        {
            return mode_;
        }

        void
        set(ConsensusMode mode, Adaptor& a)
        {
            a.onModeChange(mode_, mode);
            mode_ = mode;
        }
    };
public:
//！在一致性代码内测量时间的时钟类型
    using clock_type = beast::abstract_clock<std::chrono::steady_clock>;

    Consensus(Consensus&&) noexcept = default;

    /*构造函数。

        @param clock用于内部抽样协商进程的时钟
        @param adapter适配器类的实例
        @param j记录调试输出的日志
    **/

    Consensus(clock_type const& clock, Adaptor& adaptor, beast::Journal j);

    /*启动下一轮共识。

        由客户端代码调用以开始每一轮协商。

        @param现在网络调整时间
        @param prevledgeid最后一个分类帐的ID
        @param prevledger最后一个分类帐
        @param nowuntrusted此轮新不受信任的节点的ID
        @param提议我们是否要向本轮同行发送提案。

        @note@b prevledgeid对于@b prevledged的ID不是必需的，因为
        在分类帐内容到达之前，可以在本地知道ID。
    **/

    void
    startRound(
        NetClock::time_point const& now,
        typename Ledger_t::ID const& prevLedgerID,
        Ledger_t prevLedger,
        hash_set<NodeID_t> const & nowUntrusted,
        bool proposing);

    /*一位同行提出了一个新的立场，调整我们的跟踪。

        @param现在网络调整时间
        @param new proposal来自同行的新建议
        @返回我们是否应延迟传递此建议。
    **/

    bool
    peerProposal(
        NetClock::time_point const& now,
        PeerPosition_t const& newProposal);

    /*定期呼吁推动共识向前发展。

        @param现在网络调整时间
    **/

    void
    timerEntry(NetClock::time_point const& now);

    /*处理从网络获取的事务集

        @param现在网络调整时间
        @param txtset事务集
    **/

    void
    gotTxSet(NetClock::time_point const& now, TxSet_t const& txSet);

    /*模拟没有任何网络流量的协商过程。

       最终的结果是，共识就像每个人一样开始和结束
       同意我们的建议。

       此函数仅从rpc“ledger\u accept”路径调用
       服务器处于独立模式，正常情况下不应使用
       协商一致的过程。

       由于客户端正在手动驾驶，因此Simulate将调用OnForceAccept
       同意接受阶段。

       @param现在是当前网络调整时间。
       @param consensusdelay duration to delay between closing and accepting the
                             分类帐。如果未指定，则使用100毫秒。
    **/

    void
    simulate(
        NetClock::time_point const& now,
        boost::optional<std::chrono::milliseconds> consensusDelay);

    /*获取上一个分类帐ID。

        上一个分类账是共识代码看到的最后一个分类账，并且
        应与此对等方看到的最近验证的分类帐相对应。

        @上一个分类帐的返回ID
    **/

    typename Ledger_t::ID
    prevLedgerID() const
    {
        return prevLedgerID_;
    }

    /*获取共识过程的JSON状态。

        由协商一致的RPC调用。

        如果需要详细的响应，@param full true。
        @返回json状态。
    **/

    Json::Value
    getJson(bool full) const;

private:
    void
    startRoundInternal(
        NetClock::time_point const& now,
        typename Ledger_t::ID const& prevLedgerID,
        Ledger_t const& prevLedger,
        ConsensusMode mode);

//更改上一个分类帐的视图
    void
    handleWrongLedger(typename Ledger_t::ID const& lgrId);

    /*检查我们以前的分类帐是否与网络相符。

        如果上一个分类帐不同，我们将不再与
        网络和需要退出/切换模式。
    **/

    void
    checkLedger();

    /*如果我们出于某种原因彻底改变了我们的共识，
        我们需要重放最近的提议，以免它们丢失。
    **/

    void
    playbackProposals();

    /*处理重播或新的对等建议。
    **/

    bool
    peerProposalInternal(
        NetClock::time_point const& now,
        PeerPosition_t const& newProposal);

    /*处理预关闭阶段。

        在预结算阶段，分类帐在等待新的
        交易。经过足够的时间后，我们将关闭分类帐，
        切换到建立阶段并开始协商一致的过程。
    **/

    void
    phaseOpen();

    /*处理建立阶段。

        在建立阶段，分类帐已关闭，我们与同行合作
        达成共识。只在计时器上更新我们的位置
        相位。

        如果我们达成共识，就进入接受阶段。
    **/

    void
    phaseEstablish();

//关闭未结分类帐并建立初始位置。
    void
    closeLedger();

//调整我们的立场，试图与其他验证者达成一致。
    void
    updateOurPositions();

    bool
    haveConsensus();

//在我们的立场和提供的立场之间制造争端。
    void
    createDisputes(TxSet_t const& o);

//考虑到该节点采用了新的位置，请更新我们的争议。
//将根据需要调用CreateDefauses。
    void
    updateDisputes(NodeID_t const& node, TxSet_t const& other);

//撤销我方未完成的提案（如有），并停止提案
//直到这一轮结束。
    void
    leaveConsensus();

//从一个提议者那里得到的四舍五入的或有效的结束时间估计。
    NetClock::time_point
    asCloseTime(NetClock::time_point raw) const;

private:
    Adaptor& adaptor_;

    ConsensusPhase phase_{ConsensusPhase::accepted};
    MonitoredMode mode_{ConsensusMode::observing};
    bool firstRound_ = true;
    bool haveCloseTimeConsensus_ = false;

    clock_type const& clock_;

//协商一致需要多长时间，表示为
//占我们预期时间的百分比。
    int convergePercent_{0};

//这一轮有多长时间了
    ConsensusTimer openTime_;

    NetClock::duration closeResolution_ = ledgerDefaultTimeResolution;

//最后一轮协商一致需要多长时间才能达成一致
    std::chrono::milliseconds prevRoundTime_;

//————————————————————————————————————————————————————————————————
//共识进展的网络时间测量

//当前网络调整时间。这是网络时间
//如果现在关闭分类帐，它将关闭
    NetClock::time_point now_;
    NetClock::time_point prevCloseTime_;

//————————————————————————————————————————————————————————————————
//非对等（自）共识数据

//最后验证的分类帐ID提供给协商一致
    typename Ledger_t::ID prevLedgerID_;
//经协商一致最后确认的分类账
    Ledger_t previousLedger_;

//事务集，按事务树的哈希索引
    hash_map<typename TxSet_t::ID, const TxSet_t> acquired_;

    boost::optional<Result> result_;
    ConsensusCloseTimes rawCloseTimes_;

//————————————————————————————————————————————————————————————————
//与同行相关的共识数据

//当前一轮的同行建议职位
    hash_map<NodeID_t, PeerPosition_t> currPeerPositions_;

//最近接收到的对等职位，在
//分类帐或轮次
    hash_map<NodeID_t, std::deque<PeerPosition_t>> recentPeerPositions_;

//参加上一轮共识的提案人数量
    std::size_t prevProposers_ = 0;

//退出协商进程的节点
    hash_set<NodeID_t> deadNodes_;

//调试日志
    beast::Journal j_;
};

template <class Adaptor>
Consensus<Adaptor>::Consensus(
    clock_type const& clock,
    Adaptor& adaptor,
    beast::Journal journal)
    : adaptor_(adaptor)
    , clock_(clock)
    , j_{journal}
{
    JLOG(j_.debug()) << "Creating consensus object";
}

template <class Adaptor>
void
Consensus<Adaptor>::startRound(
    NetClock::time_point const& now,
    typename Ledger_t::ID const& prevLedgerID,
    Ledger_t prevLedger,
    hash_set<NodeID_t> const& nowUntrusted,
    bool proposing)
{
    if (firstRound_)
    {
//从种子分类账中查看关闭时间
        prevRoundTime_ = adaptor_.parms().ledgerIDLE_INTERVAL;
        prevCloseTime_ = prevLedger.closeTime();
        firstRound_ = false;
    }
    else
    {
        prevCloseTime_ = rawCloseTimes_.self;
    }

    for(NodeID_t const& n : nowUntrusted)
        recentPeerPositions_.erase(n);

    ConsensusMode startMode =
        proposing ? ConsensusMode::proposing : ConsensusMode::observing;

//我们交错了账本
    if (prevLedger.id() != prevLedgerID)
    {
//尝试获得正确的
        if (auto newLedger = adaptor_.acquireLedger(prevLedgerID))
        {
            prevLedger = *newLedger;
        }
else  //无法获取正确的分类帐
        {
            startMode = ConsensusMode::wrongLedger;
            JLOG(j_.info())
                << "Entering consensus with: " << previousLedger_.id();
            JLOG(j_.info()) << "Correct LCL is: " << prevLedgerID;
        }
    }

    startRoundInternal(now, prevLedgerID, prevLedger, startMode);
}
template <class Adaptor>
void
Consensus<Adaptor>::startRoundInternal(
    NetClock::time_point const& now,
    typename Ledger_t::ID const& prevLedgerID,
    Ledger_t const& prevLedger,
    ConsensusMode mode)
{
    phase_ = ConsensusPhase::open;
    mode_.set(mode, adaptor_);
    now_ = now;
    prevLedgerID_ = prevLedgerID;
    previousLedger_ = prevLedger;
    result_.reset();
    convergePercent_ = 0;
    haveCloseTimeConsensus_ = false;
    openTime_.reset(clock_.now());
    currPeerPositions_.clear();
    acquired_.clear();
    rawCloseTimes_.peers.clear();
    rawCloseTimes_.self = {};
    deadNodes_.clear();

    closeResolution_ = getNextLedgerTimeResolution(
        previousLedger_.closeTimeResolution(),
        previousLedger_.closeAgree(),
        previousLedger_.seq() + typename Ledger_t::Seq{1});

    playbackProposals();
    if (currPeerPositions_.size() > (prevProposers_ / 2))
    {
//我们可能落后了，不要等计时器。
//考虑立即关闭分类帐
        timerEntry(now_);
    }
}

template <class Adaptor>
bool
Consensus<Adaptor>::peerProposal(
    NetClock::time_point const& now,
    PeerPosition_t const& newPeerPos)
{
    NodeID_t const& peerID = newPeerPos.proposal().nodeID();

//总是需要存储最近的职位
    {
        auto& props = recentPeerPositions_[peerID];

        if (props.size() >= 10)
            props.pop_front();

        props.push_back(newPeerPos);
    }
    return peerProposalInternal(now, newPeerPos);
}

template <class Adaptor>
bool
Consensus<Adaptor>::peerProposalInternal(
    NetClock::time_point const& now,
    PeerPosition_t const& newPeerPos)
{
//如果我们现在正在处理一个分类帐，现在没有什么可做的。
    if (phase_ == ConsensusPhase::accepted)
        return false;

    now_ = now;

    Proposal_t const& newPeerProp = newPeerPos.proposal();

    NodeID_t const& peerID = newPeerProp.nodeID();

    if (newPeerProp.prevLedger() != prevLedgerID_)
    {
        JLOG(j_.debug()) << "Got proposal for " << newPeerProp.prevLedger()
                         << " but we are on " << prevLedgerID_;
        return false;
    }

    if (deadNodes_.find(peerID) != deadNodes_.end())
    {
        using std::to_string;
        JLOG(j_.info()) << "Position from dead node: " << to_string(peerID);
        return false;
    }

    {
//更新当前位置
        auto peerPosIt = currPeerPositions_.find(peerID);

        if (peerPosIt != currPeerPositions_.end())
        {
            if (newPeerProp.proposeSeq() <=
                peerPosIt->second.proposal().proposeSeq())
            {
                return false;
            }
        }

        if (newPeerProp.isBowOut())
        {
            using std::to_string;

            JLOG(j_.info()) << "Peer bows out: " << to_string(peerID);
            if (result_)
            {
                for (auto& it : result_->disputes)
                    it.second.unVote(peerID);
            }
            if (peerPosIt != currPeerPositions_.end())
                currPeerPositions_.erase(peerID);
            deadNodes_.insert(peerID);

            return true;
        }

        if (peerPosIt != currPeerPositions_.end())
            peerPosIt->second = newPeerPos;
        else
            currPeerPositions_.emplace(peerID, newPeerPos);
    }

    if (newPeerProp.isInitial())
    {
//记录关闭时间估计
        JLOG(j_.trace()) << "Peer reports close time as "
                         << newPeerProp.closeTime().time_since_epoch().count();
        ++rawCloseTimes_.peers[newPeerProp.closeTime()];
    }

    JLOG(j_.trace()) << "Processing peer proposal " << newPeerProp.proposeSeq()
                     << "/" << newPeerProp.position();

    {
        auto const ait = acquired_.find(newPeerProp.position());
        if (ait == acquired_.end())
        {
//AcquireTxset将返回集合（如果可用），或者
//生成一个请求并返回none/nullptr。它会呼叫
//到达后立即到达
            if (auto set = adaptor_.acquireTxSet(newPeerProp.position()))
                gotTxSet(now_, *set);
            else
                JLOG(j_.debug()) << "Don't have tx set for peer";
        }
        else if (result_)
        {
            updateDisputes(newPeerProp.nodeID(), ait->second);
        }
    }

    return true;
}

template <class Adaptor>
void
Consensus<Adaptor>::timerEntry(NetClock::time_point const& now)
{
//如果我们目前正在处理分类帐，则无需执行任何操作
    if (phase_ == ConsensusPhase::accepted)
        return;

    now_ = now;

//检查我们是否在正确的分类账上（这可能会改变阶段）
    checkLedger();

    if (phase_ == ConsensusPhase::open)
    {
        phaseOpen();
    }
    else if (phase_ == ConsensusPhase::establish)
    {
        phaseEstablish();
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::gotTxSet(
    NetClock::time_point const& now,
    TxSet_t const& txSet)
{
//如果我们完成了账本上的工作，就没什么可做的了。
    if (phase_ == ConsensusPhase::accepted)
        return;

    now_ = now;

    auto id = txSet.id();

//如果我们在请求之后已经处理了这个事务集
//从网络上看，现在没什么事可做了
    if (!acquired_.emplace(id, txSet).second)
        return;

    if (!result_)
    {
        JLOG(j_.debug()) << "Not creating disputes: no position yet.";
    }
    else
    {
//一旦我们创建了收购公司，我们的位置就会增加，
//所以这个txtset必须不同
        assert(id != result_->position.position());
        bool any = false;
        for (auto const& it : currPeerPositions_)
        {
            if (it.second.proposal().position() == id)
            {
                updateDisputes(it.first, txSet);
                any = true;
            }
        }

        if (!any)
        {
            JLOG(j_.warn())
                << "By the time we got " << id << " no peers were proposing it";
        }
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::simulate(
    NetClock::time_point const& now,
    boost::optional<std::chrono::milliseconds> consensusDelay)
{
    using namespace std::chrono_literals;
    JLOG(j_.info()) << "Simulating consensus";
    now_ = now;
    closeLedger();
    result_->roundTime.tick(consensusDelay.value_or(100ms));
    result_->proposers = prevProposers_ = currPeerPositions_.size();
    prevRoundTime_ = result_->roundTime.read();
    phase_ = ConsensusPhase::accepted;
    adaptor_.onForceAccept(
        *result_,
        previousLedger_,
        closeResolution_,
        rawCloseTimes_,
        mode_.get(),
        getJson(true));
    JLOG(j_.info()) << "Simulation complete";
}

template <class Adaptor>
Json::Value
Consensus<Adaptor>::getJson(bool full) const
{
    using std::to_string;
    using Int = Json::Value::Int;

    Json::Value ret(Json::objectValue);

    ret["proposing"] = (mode_.get() == ConsensusMode::proposing);
    ret["proposers"] = static_cast<int>(currPeerPositions_.size());

    if (mode_.get() != ConsensusMode::wrongLedger)
    {
        ret["synched"] = true;
        ret["ledger_seq"] = static_cast<std::uint32_t>(previousLedger_.seq())+ 1;
        ret["close_granularity"] = static_cast<Int>(closeResolution_.count());
    }
    else
        ret["synched"] = false;

    ret["phase"] = to_string(phase_);

    if (result_ && !result_->disputes.empty() && !full)
        ret["disputes"] = static_cast<Int>(result_->disputes.size());

    if (result_)
        ret["our_position"] = result_->position.getJson();

    if (full)
    {
        if (result_)
            ret["current_ms"] =
                static_cast<Int>(result_->roundTime.read().count());
        ret["converge_percent"] = convergePercent_;
        ret["close_resolution"] = static_cast<Int>(closeResolution_.count());
        ret["have_time_consensus"] = haveCloseTimeConsensus_;
        ret["previous_proposers"] = static_cast<Int>(prevProposers_);
        ret["previous_mseconds"] = static_cast<Int>(prevRoundTime_.count());

        if (!currPeerPositions_.empty())
        {
            Json::Value ppj(Json::objectValue);

            for (auto const& pp : currPeerPositions_)
            {
                ppj[to_string(pp.first)] = pp.second.getJson();
            }
            ret["peer_positions"] = std::move(ppj);
        }

        if (!acquired_.empty())
        {
            Json::Value acq(Json::arrayValue);
            for (auto const& at : acquired_)
            {
                acq.append(to_string(at.first));
            }
            ret["acquired"] = std::move(acq);
        }

        if (result_ && !result_->disputes.empty())
        {
            Json::Value dsj(Json::objectValue);
            for (auto const& dt : result_->disputes)
            {
                dsj[to_string(dt.first)] = dt.second.getJson();
            }
            ret["disputes"] = std::move(dsj);
        }

        if (!rawCloseTimes_.peers.empty())
        {
            Json::Value ctj(Json::objectValue);
            for (auto const& ct : rawCloseTimes_.peers)
            {
                ctj[std::to_string(ct.first.time_since_epoch().count())] =
                    ct.second;
            }
            ret["close_times"] = std::move(ctj);
        }

        if (!deadNodes_.empty())
        {
            Json::Value dnj(Json::arrayValue);
            for (auto const& dn : deadNodes_)
            {
                dnj.append(to_string(dn));
            }
            ret["dead_nodes"] = std::move(dnj);
        }
    }

    return ret;
}

//在协商一致时处理上一个分类帐的更改
template <class Adaptor>
void
Consensus<Adaptor>::handleWrongLedger(typename Ledger_t::ID const& lgrId)
{
    assert(lgrId != prevLedgerID_ || previousLedger_.id() != lgrId);

//停止建议，因为我们不同步
    leaveConsensus();

//第一次切换到此分类帐
    if (prevLedgerID_ != lgrId)
    {
        prevLedgerID_ = lgrId;

//清除状态
        if (result_)
        {
            result_->disputes.clear();
            result_->compares.clear();
        }

        currPeerPositions_.clear();
        rawCloseTimes_.peers.clear();
        deadNodes_.clear();

//恢复同步，这也会引起争端。
        playbackProposals();
    }

    if (previousLedger_.id() == prevLedgerID_)
        return;

//我们需要把我们正在处理的账本换掉
    if (auto newLedger = adaptor_.acquireLedger(prevLedgerID_))
    {
        JLOG(j_.info()) << "Have the consensus ledger " << prevLedgerID_;
        startRoundInternal(
            now_, lgrId, *newLedger, ConsensusMode::switchedLedger);
    }
    else
    {
        mode_.set(ConsensusMode::wrongLedger, adaptor_);
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::checkLedger()
{
    auto netLgr =
        adaptor_.getPrevLedger(prevLedgerID_, previousLedger_, mode_.get());

    if (netLgr != prevLedgerID_)
    {
        JLOG(j_.warn()) << "View of consensus changed during "
                        << to_string(phase_) << " status=" << to_string(phase_)
                        << ", "
                        << " mode=" << to_string(mode_.get());
        JLOG(j_.warn()) << prevLedgerID_ << " to " << netLgr;
        JLOG(j_.warn()) << Json::Compact{previousLedger_.getJson()};
        JLOG(j_.debug()) << "State on consensus change "
                         << Json::Compact{getJson(true)};
        handleWrongLedger(netLgr);
    }
    else if (previousLedger_.id() != prevLedgerID_)
        handleWrongLedger(netLgr);
}

template <class Adaptor>
void
Consensus<Adaptor>::playbackProposals()
{
    for (auto const& it : recentPeerPositions_)
    {
        for (auto const& pos : it.second)
        {
            if (pos.proposal().prevLedger() == prevLedgerID_)
            {
                if (peerProposalInternal(now_, pos))
                    adaptor_.share(pos);
            }
        }
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::phaseOpen()
{
    using namespace std::chrono;

//就在分类帐关闭时间前不久
    bool anyTransactions = adaptor_.hasOpenTransactions();
    auto proposersClosed = currPeerPositions_.size();
    auto proposersValidated = adaptor_.proposersValidated(prevLedgerID_);

    openTime_.tick(clock_.now());

//这计算上一个分类帐关闭时间以来的时间
    milliseconds sinceClose;
    {
        bool previousCloseCorrect =
            (mode_.get() != ConsensusMode::wrongLedger) &&
            previousLedger_.closeAgree() &&
            (previousLedger_.closeTime() !=
             (previousLedger_.parentCloseTime() + 1s));

        auto lastCloseTime = previousCloseCorrect
? previousLedger_.closeTime()  //使用协商一致的时间安排
: prevCloseTime_;              //利用我们在内部看到的时间

        if (now_ >= lastCloseTime)
            sinceClose = duration_cast<milliseconds>(now_ - lastCloseTime);
        else
            sinceClose = -duration_cast<milliseconds>(lastCloseTime - now_);
    }

    auto const idleInterval = std::max<milliseconds>(
        adaptor_.parms().ledgerIDLE_INTERVAL,
        2 * previousLedger_.closeTimeResolution());

//决定是否关闭分类帐
    if (shouldCloseLedger(
            anyTransactions,
            prevProposers_,
            proposersClosed,
            proposersValidated,
            prevRoundTime_,
            sinceClose,
            openTime_.read(),
            idleInterval,
            adaptor_.parms(),
            j_))
    {
        closeLedger();
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::phaseEstablish()
{
//只有当我们已经采取立场时，才能达成共识
    assert(result_);

    using namespace std::chrono;
    ConsensusParms const & parms = adaptor_.parms();

    result_->roundTime.tick(clock_.now());
    result_->proposers = currPeerPositions_.size();

    convergePercent_ = result_->roundTime.read() * 100 /
        std::max<milliseconds>(prevRoundTime_, parms.avMIN_CONSENSUS_TIME);

//给每个人一个机会去做一个初步的职位
    if (result_->roundTime.read() < parms.ledgerMIN_CONSENSUS)
        return;

    updateOurPositions();

//如果我们不能达成共识，那就什么都不做。
    if (!haveConsensus())
        return;

    if (!haveCloseTimeConsensus_)
    {
        JLOG(j_.info()) << "We have TX consensus but not CT consensus";
        return;
    }

    JLOG(j_.info()) << "Converge cutoff (" << currPeerPositions_.size()
                    << " participants)";
    prevProposers_ = currPeerPositions_.size();
    prevRoundTime_ = result_->roundTime.read();
    phase_ = ConsensusPhase::accepted;
    adaptor_.onAccept(
        *result_,
        previousLedger_,
        closeResolution_,
        rawCloseTimes_,
        mode_.get(),
        getJson(true));
}

template <class Adaptor>
void
Consensus<Adaptor>::closeLedger()
{
//如果我们已经有了职位，就不应该关门
    assert(!result_);

    phase_ = ConsensusPhase::establish;
    rawCloseTimes_.self = now_;

    result_.emplace(adaptor_.onClose(previousLedger_, now_, mode_.get()));
    result_->roundTime.reset(clock_.now());
//如果尚未共享新创建的事务集
//从同伴那里收到的
    if (acquired_.emplace(result_->txns.id(), result_->txns).second)
        adaptor_.share(result_->txns);

    if (mode_.get() == ConsensusMode::proposing)
        adaptor_.propose(result_->position);

//与我们交易的任何同行职位产生争议
    for (auto const& pit : currPeerPositions_)
    {
        auto const& pos = pit.second.proposal().position();
        auto const it = acquired_.find(pos);
        if (it != acquired_.end())
        {
            createDisputes(it->second);
        }
    }
}

/*有多少参与者必须同意达到给定的阈值？

请注意，该数字可能不会精确地产生所请求的百分比。
例如，当size=5，percent=70时，返回3，但是
五分之三计算到60%。没有安全隐患
这个。

@param participants参与者的数量（即验证器）
@param percent我们想要达到的百分比

@返回必须同意的参与者数量
**/

inline int
participantsNeeded(int participants, int percent)
{
    int result = ((participants * percent) + (percent / 2)) / 100;

    return (result == 0) ? 1 : result;
}

template <class Adaptor>
void
Consensus<Adaptor>::updateOurPositions()
{
//如果我们要更新的话，我们必须有一个职位
    assert(result_);
    ConsensusParms const & parms = adaptor_.parms();

//计算截止时间
    auto const peerCutoff = now_ - parms.proposeFRESHNESS;
    auto const ourCutoff = now_ - parms.proposeINTERVAL;

//验证对等位置的新鲜度并计算关闭时间
    std::map<NetClock::time_point, int> closeTimeVotes;
    {
        auto it = currPeerPositions_.begin();
        while (it != currPeerPositions_.end())
        {
            Proposal_t const& peerProp = it->second.proposal();
            if (peerProp.isStale(peerCutoff))
            {
//同行的建议已过时，请将其删除
                NodeID_t const& peerID = peerProp.nodeID();
                JLOG(j_.warn()) << "Removing stale proposal from " << peerID;
                for (auto& dt : result_->disputes)
                    dt.second.unVote(peerID);
                it = currPeerPositions_.erase(it);
            }
            else
            {
//建议仍然是新鲜的
                ++closeTimeVotes[asCloseTime(peerProp.closeTime())];
                ++it;
            }
        }
    }

//除非有任何更改，否则将保持未密封状态
    boost::optional<TxSet_t> ourNewSet;

//更新争议交易的投票
    {
        boost::optional<typename TxSet_t::MutableTxSet> mutableSet;
        for (auto& it : result_->disputes)
        {
//因为包容性的门槛增加了，
//时间可以改变我们在争端上的立场
            if (it.second.updateVote(
                    convergePercent_,
                    mode_.get()== ConsensusMode::proposing,
                    parms))
            {
                if (!mutableSet)
                    mutableSet.emplace(result_->txns);

                if (it.second.getOurVote())
                {
//现在是的
                    mutableSet->insert(it.second.tx());
                }
                else
                {
//现在没有
                    mutableSet->erase(it.first);
                }
            }
        }

        if (mutableSet)
            ourNewSet.emplace(std::move(*mutableSet));
    }

    NetClock::time_point consensusCloseTime = {};
    haveCloseTimeConsensus_ = false;

    if (currPeerPositions_.empty())
    {
//没有其他时间
        haveCloseTimeConsensus_ = true;
        consensusCloseTime = asCloseTime(result_->position.closeTime());
    }
    else
    {
        int neededWeight;

        if (convergePercent_ < parms.avMID_CONSENSUS_TIME)
            neededWeight = parms.avINIT_CONSENSUS_PCT;
        else if (convergePercent_ < parms.avLATE_CONSENSUS_TIME)
            neededWeight = parms.avMID_CONSENSUS_PCT;
        else if (convergePercent_ < parms.avSTUCK_CONSENSUS_TIME)
            neededWeight = parms.avLATE_CONSENSUS_PCT;
        else
            neededWeight = parms.avSTUCK_CONSENSUS_PCT;

        int participants = currPeerPositions_.size();
        if (mode_.get() == ConsensusMode::proposing)
        {
            ++closeTimeVotes[asCloseTime(result_->position.closeTime())];
            ++participants;
        }

//非零投票阈值
        int threshVote = participantsNeeded(participants, neededWeight);

//宣布达成共识的门槛
        int const threshConsensus =
            participantsNeeded(participants, parms.avCT_CONSENSUS_PCT);

        JLOG(j_.info()) << "Proposers:" << currPeerPositions_.size()
                        << " nw:" << neededWeight << " thrV:" << threshVote
                        << " thrC:" << threshConsensus;

        for (auto const& it : closeTimeVotes)
        {
            JLOG(j_.debug())
                << "CCTime: seq "
                << static_cast<std::uint32_t>(previousLedger_.seq()) + 1 << ": "
                << it.first.time_since_epoch().count() << " has " << it.second
                << ", " << threshVote << " required";

            if (it.second >= threshVote)
            {
//临近的时候有足够的选票让我们试图达成一致
                consensusCloseTime = it.first;
                threshVote = it.second;

                if (threshVote >= threshConsensus)
                    haveCloseTimeConsensus_ = true;
            }
        }

        if (!haveCloseTimeConsensus_)
        {
            JLOG(j_.debug())
                << "No CT consensus:"
                << " Proposers:" << currPeerPositions_.size()
                << " Mode:" << to_string(mode_.get())
                << " Thresh:" << threshConsensus
                << " Pos:" << consensusCloseTime.time_since_epoch().count();
        }
    }

    if (!ourNewSet &&
        ((consensusCloseTime != asCloseTime(result_->position.closeTime())) ||
         result_->position.isStale(ourCutoff)))
    {
//关闭时间改变或我们的位置过时
        ourNewSet.emplace(result_->txns);
    }

    if (ourNewSet)
    {
        auto newID = ourNewSet->id();

        result_->txns = std::move(*ourNewSet);

        JLOG(j_.info()) << "Position change: CTime "
                        << consensusCloseTime.time_since_epoch().count()
                        << ", tx " << newID;

        result_->position.changePosition(newID, consensusCloseTime, now_);

//共享我们的新交易集并更新争议
//如果我们还没有收到
        if (acquired_.emplace(newID, result_->txns).second)
        {
            if (!result_->position.isBowOut())
                adaptor_.share(result_->txns);

            for (auto const& it : currPeerPositions_)
            {
                Proposal_t const& p = it.second.proposal();
                if (p.position() == newID)
                    updateDisputes(it.first, result_->txns);
            }
        }

//如果我们仍在参与这一轮，就分享我们的新位置
        if (!result_->position.isBowOut() &&
            (mode_.get() == ConsensusMode::proposing))
            adaptor_.propose(result_->position);
    }
}

template <class Adaptor>
bool
Consensus<Adaptor>::haveConsensus()
{
//如果我们正在检查共识，必须有立场
    assert(result_);

//检查我：应该将不需要的Tx集计算为不同意
    int agree = 0, disagree = 0;

    auto ourPosition = result_->position.position();

//统计与我们职位的协议/分歧数量
    for (auto const& it : currPeerPositions_)
    {
        Proposal_t const& peerProp = it.second.proposal();
        if (peerProp.position() == ourPosition)
        {
            ++agree;
        }
        else
        {
            using std::to_string;

            JLOG(j_.debug()) << to_string(it.first) << " has "
                             << to_string(peerProp.position());
            ++disagree;
        }
    }
    auto currentFinished =
        adaptor_.proposersFinished(previousLedger_, prevLedgerID_);

    JLOG(j_.debug()) << "Checking for TX consensus: agree=" << agree
                     << ", disagree=" << disagree;

//确定我们是否真正达成共识
    result_->state = checkConsensus(
        prevProposers_,
        agree + disagree,
        agree,
        currentFinished,
        prevRoundTime_,
        result_->roundTime.read(),
        adaptor_.parms(),
        mode_.get() == ConsensusMode::proposing,
        j_);

    if (result_->state == ConsensusState::No)
        return false;

//有共识，但我们需要跟踪网络是否继续
//没有我们。
    if (result_->state == ConsensusState::MovedOn)
    {
        JLOG(j_.error()) << "Unable to reach consensus";
        JLOG(j_.error()) << Json::Compact{getJson(true)};
    }

    return true;
}

template <class Adaptor>
void
Consensus<Adaptor>::leaveConsensus()
{
    if (mode_.get() == ConsensusMode::proposing)
    {
        if (result_ && !result_->position.isBowOut())
        {
            result_->position.bowOut(now_);
            adaptor_.propose(result_->position);
        }

        mode_.set(ConsensusMode::observing, adaptor_);
        JLOG(j_.info()) << "Bowing out of consensus";
    }
}

template <class Adaptor>
void
Consensus<Adaptor>::createDisputes(TxSet_t const& o)
{
//没有我们的立场就不能制造争端
    assert(result_);

//只有当这是一个新的集合时才会产生争议
    if (!result_->compares.emplace(o.id()).second)
        return;

//如果我们同意，就没有什么可争议的了
    if (result_->txns.id() == o.id())
        return;

    JLOG(j_.debug()) << "createDisputes " << result_->txns.id() << " to "
                     << o.id();

    auto differences = result_->txns.compare(o);

    int dc = 0;

    for (auto& id : differences)
    {
        ++dc;
//创建有争议的交易（从拥有它们的分类帐）
        assert(
            (id.second && result_->txns.find(id.first) && !o.find(id.first)) ||
            (!id.second && !result_->txns.find(id.first) && o.find(id.first)));

        Tx_t tx = id.second ? *result_->txns.find(id.first) : *o.find(id.first);
        auto txID = tx.id();

        if (result_->disputes.find(txID) != result_->disputes.end())
            continue;

        JLOG(j_.debug()) << "Transaction " << txID << " is disputed";

        typename Result::Dispute_t dtx{tx, result_->txns.exists(txID),
         std::max(prevProposers_, currPeerPositions_.size()), j_};

//更新争议交易的所有可用对等方投票
        for (auto const& pit : currPeerPositions_)
        {
            Proposal_t const& peerProp = pit.second.proposal();
            auto const cit = acquired_.find(peerProp.position());
            if (cit != acquired_.end())
                dtx.setVote(pit.first, cit->second.exists(txID));
        }
        adaptor_.share(dtx.tx());

        result_->disputes.emplace(txID, std::move(dtx));
    }
    JLOG(j_.debug()) << dc << " differences found";
}

template <class Adaptor>
void
Consensus<Adaptor>::updateDisputes(NodeID_t const& node, TxSet_t const& other)
{
//没有我们的立场就无法更新争议
    assert(result_);

//如果我们没有看到，确保我们已经对这一组产生了争议
//它之前
    if (result_->compares.find(other.id()) == result_->compares.end())
        createDisputes(other);

    for (auto& it : result_->disputes)
    {
        auto& d = it.second;
        d.setVote(node, other.exists(d.tx().id()));
    }
}

template <class Adaptor>
NetClock::time_point
Consensus<Adaptor>::asCloseTime(NetClock::time_point raw) const
{
    if (adaptor_.parms().useRoundedCloseTime)
        return roundCloseTime(raw, closeResolution_);
    else
        return effCloseTime(raw, closeResolution_, previousLedger_.closeTime());
}

}  //命名空间波纹

#endif
