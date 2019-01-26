
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

#ifndef RIPPLE_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_RCLCONSENSUS_H_INCLUDED

#include <ripple/app/consensus/RCLCxLedger.h>
#include <ripple/app/consensus/RCLCxPeerPos.h>
#include <ripple/app/consensus/RCLCxTx.h>
#include <ripple/app/consensus/RCLCensorshipDetector.h>
#include <ripple/app/misc/FeeVote.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/core/JobQueue.h>
#include <ripple/overlay/Message.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/shamap/SHAMap.h>
#include <atomic>
#include <mutex>
#include <set>
namespace ripple {

class InboundTransactions;
class LocalTxs;
class LedgerMaster;
class ValidatorKeys;

/*管理RCL使用的通用共识算法。
**/

class RCLConsensus
{
    /*对未包含如此多分类帐的交易发出警告。*/
    constexpr static unsigned int censorshipWarnInternal = 15;

    /*多次警告后停止警告。*/
    constexpr static unsigned int censorshipMaxWarnings = 5;

//实现共识所需的适配器模板接口。
    class Adaptor
    {
        Application& app_;
        std::unique_ptr<FeeVote> feeVote_;
        LedgerMaster& ledgerMaster_;
        LocalTxs& localTxs_;
        InboundTransactions& inboundTransactions_;
        beast::Journal j_;

        NodeID const nodeID_;
        PublicKey const valPublic_;
        SecretKey const valSecret_;

//我们最近需要获得的分类帐
        LedgerHash acquiringLedger_;
        ConsensusParms parms_;

//上次验证的时间戳
        NetClock::time_point lastValidationTime_;

//这些成员通过公共帐户进行查询，并且是
//线程安全性。
        std::atomic<bool> validating_{false};
        std::atomic<std::size_t> prevProposers_{0};
        std::atomic<std::chrono::milliseconds> prevRoundTime_{
            std::chrono::milliseconds{0}};
        std::atomic<ConsensusMode> mode_{ConsensusMode::observing};

        RCLCensorshipDetector<TxID, LedgerIndex> censorshipDetector_;

    public:
        using Ledger_t = RCLCxLedger;
        using NodeID_t = NodeID;
        using TxSet_t = RCLTxSet;
        using PeerPosition_t = RCLCxPeerPos;

        using Result = ConsensusResult<Adaptor>;

        Adaptor(
            Application& app,
            std::unique_ptr<FeeVote>&& feeVote,
            LedgerMaster& ledgerMaster,
            LocalTxs& localTxs,
            InboundTransactions& inboundTransactions,
            ValidatorKeys const & validatorKeys,
            beast::Journal journal);

        bool
        validating() const
        {
            return validating_;
        }

        std::size_t
        prevProposers() const
        {
            return prevProposers_;
        }

        std::chrono::milliseconds
        prevRoundTime() const
        {
            return prevRoundTime_;
        }

        ConsensusMode
        mode() const
        {
            return mode_;
        }

        /*在开始新一轮共识之前打电话。

            @param prevledger分类账，将是下一轮的前一个分类账
            @返回是否进入回合提议
        **/

        bool
        preStartRound(RCLCxLedger const & prevLedger);

        /*共识模拟参数
         **/

        ConsensusParms const&
        parms() const
        {
            return parms_;
        }

    private:
//————————————————————————————————————————————————————————————————
//下列成员执行一般共识要求
//且标记为“私有”，表示只有共识<adapter>才会调用
//他们（通过友谊）。因为它们只从consenus<adapter>
//方法和，因为rclcommersion:：consension_u只能访问
//在锁下，这些将仅在锁下调用。
//
//一般来说，其思想是只有一个线程正在运行
//随时协商一致。唯一的特殊情况是
//OnAccept调用，它不加锁，依赖于协商一致，而不是
//更改状态，直到将来调用StartRound。
        friend class Consensus<Adaptor>;

        /*尝试获取特定的分类帐。

            如果不可用，则从网络异步获取。

            @param hash分类帐获取的ID/hash
            @返回可选的分类账，如果我们在本地拥有分类账，将就座。
        **/

        boost::optional<RCLCxLedger>
        acquireLedger(LedgerHash const& hash);

        /*与所有同行分享给出的建议

            @param peerpos要共享的对等位置。
         **/

        void
        share(RCLCxPeerPos const& peerPos);

        /*与同行分享有争议的交易。

            仅在最近未共享提供的事务时共享。

            @param tx要共享的有争议的事务。
        **/

        void
        share(RCLCxTx const& tx);

        /*获取与建议关联的事务集。

            如果事务集在本地不可用，将尝试
            从网络上获取。

            @param set id与建议关联的事务集id
            @返回可选的事务集，如果可用，就座。
       **/

        boost::optional<RCLTxSet>
        acquireTxSet(RCLTxSet::ID const& setId);

        /*未结分类帐是否有任何交易记录
         **/

        bool
        hasOpenTransactions() const;

        /*已对给定分类帐进行有效验证的建议者数

            @param h利息分类账的散列值
            @返回验证分类账的提议者数量
        **/

        std::size_t
        proposersValidated(LedgerHash const& h) const;

        /*已验证分类帐的建议者数目，该分类帐是从
           请求的分类帐。

            @param ledger当前工作分类帐
            @param h首选工作分类账的哈希
            @返回已验证分类帐的验证对等数
                    从首选工作分类帐派生。
        **/

        std::size_t
        proposersFinished(RCLCxLedger const & ledger, LedgerHash const& h) const;

        /*向我的同龄人推荐这个职位。

            @param proposal我们提议的职位
        **/

        void
        propose(RCLCxPeerPos::Proposal const& proposal);

        /*将给定的Tx集共享给对等机。

            @param txns要共享的txset。
        **/

        void
        share(RCLTxSet const& txns);

        /*获取上一个分类帐/上一个已关闭分类帐（LCL）的ID
           网络

            协商一致使用的上一个分类帐的@param ledger id id
            @param ledger上一个分类帐共识可用
            @param模式当前共识模式
            @返回上次关闭网络的ID

            @note ledger id可能与ledger.id（）不匹配，如果我们没有获得
                  与网络中的LedgerID匹配的分类帐
         **/

        uint256
        getPrevLedger(
            uint256 ledgerID,
            RCLCxLedger const& ledger,
            ConsensusMode mode);

        /*协商一致模式变更通知

            先验共识模式前@param
            新共识模式后的@param
        **/

        void
        onModeChange(
            ConsensusMode before,
            ConsensusMode after);

        /*关闭未结分类帐并返回初始共识位置。

           @param ledger我们要改成的分类帐
           当协商一致关闭分类帐时@param closetime
           @param模式当前共识模式
           @返回暂定共识结果
        **/

        Result
        onClose(
            RCLCxLedger const& ledger,
            NetClock::time_point const& closeTime,
            ConsensusMode mode);

        /*处理已接受的分类帐。

            @param result共识的结果
            @param prevledger关闭的分类帐共识的工作来源
            @param closeresolution用于同意
                                   有效关闭时间
            @param rawclosetimes我们自己和我们的无根据的关闭时间
                                 同龄人
            @param模式我们在协商一致时的参与模式
                        宣布
            @param conensusjson json代表共识状态
        **/

        void
        onAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /*处理作为模拟/强制结果的已接受分类帐
            接受。

            请接受
        **/

        void
        onForceAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration const& closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /*通知对等方一致状态更改

            用于通知的@param ne事件类型
            @param ledger状态更改时的分类帐
            @param have correct lcl无论我们是否相信我们有正确的lcl。
        **/

        void
        notify(
            protocol::NodeEvent ne,
            RCLCxLedger const& ledger,
            bool haveCorrectLCL);

        /*接受基于给定交易记录的新分类帐。

            请接受
         **/

        void
        doAccept(
            Result const& result,
            RCLCxLedger const& prevLedger,
            NetClock::duration closeResolution,
            ConsensusCloseTimes const& rawCloseTimes,
            ConsensusMode const& mode,
            Json::Value&& consensusJson);

        /*创建新的上次关闭的分类帐。

            接受给定的一组协商一致的交易，以及
            建立上次关闭的分类帐。既然一致同意
            要应用的事务，但不包括是否使其进入已关闭状态
            Ledger，此函数还用那些
            可以在下一轮中重试。

            @param previousleger在建立分类帐之前
            @param retailable txs on entry，要应用于的事务集
                                分类帐；返回时，一组交易记录
                                在下一轮中重试。
            @param closetime分类帐关闭的时间
            @param closetimecorrect是否一致同意关闭时间
            @param close resolution resolution用于确定共识关闭
                                   时间
            @param roundtime此共识的持续时间rorund
            @param failedtxs用我们无法完成的事务填充
                             成功应用。
            @归还新建账本
      **/

        RCLCxLedger
        buildLCL(
            RCLCxLedger const& previousLedger,
            CanonicalTXSet& retriableTxs,
            NetClock::time_point closeTime,
            bool closeTimeCorrect,
            NetClock::duration closeResolution,
            std::chrono::milliseconds roundTime,
            std::set<TxID>& failedTxs);

        /*验证给定的分类帐，必要时与同行共享

            @param ledger要验证的分类帐
            @param txns共识事务集
            @param提议是否在
                             生成此分类帐。如果我们不提议，
                             仍然可以发送验证以通知对等方
                             我们知道我们没有完全参与协商一致。
                             但仍在努力追赶。
        **/

        void
        validate(RCLCxLedger const& ledger, RCLTxSet const& txns, bool proposing);
    };

public:
//！构造函数
    RCLConsensus(
        Application& app,
        std::unique_ptr<FeeVote>&& feeVote,
        LedgerMaster& ledgerMaster,
        LocalTxs& localTxs,
        InboundTransactions& inboundTransactions,
        Consensus<Adaptor>::clock_type const& clock,
        ValidatorKeys const & validatorKeys,
        beast::Journal journal);

    RCLConsensus(RCLConsensus const&) = delete;

    RCLConsensus&
    operator=(RCLConsensus const&) = delete;

//！我们是否在验证共识分类账。
    bool
    validating() const
    {
        return adaptor_.validating();
    }

//！获取参与前一个活动的建议同行的数量
//！圆的。
    std::size_t
    prevProposers() const
    {
        return adaptor_.prevProposers();
    }

    /*获取上一轮的持续时间。

        回合的持续时间是建立阶段，从结束时算起。
        打开分类账接受共识结果。

        @返回最后一轮持续时间（毫秒）
    **/

    std::chrono::milliseconds
    prevRoundTime() const
    {
        return adaptor_.prevRoundTime();
    }

//！@见共识：：模式
    ConsensusMode
    mode() const
    {
        return adaptor_.mode();
    }

//！@见共识：：getjson
    Json::Value
    getJson(bool full) const;

//！@见共识：：开始回合
    void
    startRound(
        NetClock::time_point const& now,
        RCLCxLedger::ID const& prevLgrId,
        RCLCxLedger const& prevLgr,
        hash_set<NodeID> const& nowUntrusted);

//！@见共识：时间租赁
    void
    timerEntry(NetClock::time_point const& now);

//！@见共识：：gottxset
    void
    gotTxSet(NetClock::time_point const& now, RCLTxSet const& txSet);

//@见共识：：prevelegrid
    RCLCxLedger::ID
    prevLedgerID() const
    {
        ScopedLockType _{mutex_};
        return consensus_.prevLedgerID();
    }

//！@见共识：：模拟
    void
    simulate(
        NetClock::time_point const& now,
        boost::optional<std::chrono::milliseconds> consensusDelay);

//！@见共识：提案
    bool
    peerProposal(
        NetClock::time_point const& now,
        RCLCxPeerPos const& newProposal);

    ConsensusParms const &
    parms() const
    {
        return adaptor_.parms();
    }

private:
//因为共识不提供内在线程安全，所以这个互斥体
//保护所有要求达成一致意见的人。适配器在内部使用原子
//允许同时访问具有getter的数据成员。
    mutable std::recursive_mutex mutex_;
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    Adaptor adaptor_;
    Consensus<Adaptor> consensus_;
    beast::Journal j_;
};
}

#endif
