
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

#ifndef RIPPLE_CONSENSUS_CONSENSUS_TYPES_H_INCLUDED
#define RIPPLE_CONSENSUS_CONSENSUS_TYPES_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <ripple/consensus/DisputedTx.h>
#include <chrono>
#include <map>

namespace ripple {

/*表示节点当前如何参与协商一致。

    一个节点以不同的模式参与共识，这取决于
    节点由其操作员配置，并保持同步的程度
    与网络协商一致。

    @代码
      提议观察
         \/
          \->错误分类帐<---/
                     ^
                     γ
                     γ
                     V
                转帐分类帐
   @端码

   我们进入建议或观察回合。如果我们发现我们在工作
   在错误的上一个分类帐上，我们转到错误的分类帐并尝试获取
   正确的。一旦我们得到了正确的一个，我们就去交换账本。
   模式。我们有可能再次落后，发现有一个新的更好的
   分类帐，在错误分类帐和切换分类帐之间来回移动
   我们试图赶上。
**/

enum class ConsensusMode {
//！我们是共识的正常参与者，并提出我们的立场
    proposing,
//！我们在观察同行的立场，但没有提出我们的立场
    observing,
//！我们有错误的分类帐，正在试图取得它
    wrongLedger,
//！自从我们开始这轮共识谈判以来，我们就换了分类账，但现在
//！我们相信的是正确的分类账。此模式为
//！如果我们进入观察圈，但被用来表明
//！在某个时候把错误的分类帐记下来。
    switchedLedger
};

inline std::string
to_string(ConsensusMode m)
{
    switch (m)
    {
        case ConsensusMode::proposing:
            return "proposing";
        case ConsensusMode::observing:
            return "observing";
        case ConsensusMode::wrongLedger:
            return "wrongLedger";
        case ConsensusMode::switchedLedger:
            return "switchedLedger";
        default:
            return "unknown";
    }
}

/*单一分类账轮的共识阶段。

    @代码
          关闭“接受”
     打开----->建立----->接受
       _
       ---------------
       ^“开始回合”
       －－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－－
   @端码

   典型的转变是从开放到建立到接受和
   然后调用startround重新开始该过程。但是，如果之前的错误
   在建立或接受阶段检测并恢复分类帐，
   共识将在内部重新打开（见共识：：handlewrongledger）。
**/

enum class ConsensusPhase {
//！我们还没有关闭分类帐，但其他人可能已经
    open,

//！通过与同行交换建议来达成共识
    establish,

//！我们接受了一个新的最后一次结帐，正在等电话
//！开始下一轮协商一致。没有变化
//！在这个阶段，达成共识的阶段就发生了。
    accepted,
};

inline std::string
to_string(ConsensusPhase p)
{
    switch (p)
    {
        case ConsensusPhase::open:
            return "open";
        case ConsensusPhase::establish:
            return "establish";
        case ConsensusPhase::accepted:
            return "accepted";
        default:
            return "unknown";
    }
}

/*衡量共识阶段的持续时间
 **/

class ConsensusTimer
{
    using time_point = std::chrono::steady_clock::time_point;
    time_point start_;
    std::chrono::milliseconds dur_;

public:
    std::chrono::milliseconds
    read() const
    {
        return dur_;
    }

    void
    tick(std::chrono::milliseconds fixed)
    {
        dur_ += fixed;
    }

    void
    reset(time_point tp)
    {
        start_ = tp;
        dur_ = std::chrono::milliseconds{0};
    }

    void
    tick(time_point tp)
    {
        using namespace std::chrono;
        dur_ = duration_cast<milliseconds>(tp - start_);
    }
};

/*存储初始关闭时间集

    每一位同行的初步一致意见建议都有该同行的观点
    当分类帐关闭时。此对象存储所有关闭时间
    分析同龄人之间的时钟漂移。
**/

struct ConsensusCloseTimes
{
    explicit ConsensusCloseTimes() = default;

//！闭合时间估计，保持可预测导线的顺序
    std::map<NetClock::time_point, int> peers;

//！我们的接近时间估计
    NetClock::time_point self;
};

/*我们是否有共识*/
enum class ConsensusState {
No,       //！<我们没有共识
MovedOn,  //！网络没有我们就有共识
Yes       //！<我们与网络达成共识
};

/*概括了共识的结果。

    将所有相关数据存储在一个
   分类帐。

    @tparam traits traits class定义使用的具体共识类型
                   通过应用程序。
**/

template <class Traits>
struct ConsensusResult
{
    using Ledger_t = typename Traits::Ledger_t;
    using TxSet_t = typename Traits::TxSet_t;
    using NodeID_t = typename Traits::NodeID_t;

    using Tx_t = typename TxSet_t::Tx;
    using Proposal_t = ConsensusProposal<
        NodeID_t,
        typename Ledger_t::ID,
        typename TxSet_t::ID>;
    using Dispute_t = DisputedTx<Tx_t, NodeID_t>;

    ConsensusResult(TxSet_t&& s, Proposal_t&& p)
        : txns{std::move(s)}, position{std::move(p)}
    {
        assert(txns.id() == position.position());
    }

//！一致同意的一组交易记入分类账。
    TxSet_t txns;

//！我们对交易/结算时间的建议头寸
    Proposal_t position;

//！与同行有争议的交易
    hash_map<typename Tx_t::ID, Dispute_t> disputes;

//我们已经比较/创建的TxSet ID集
    hash_set<typename TxSet_t::ID> compares;

//衡量这一共识回合建立阶段的持续时间
    ConsensusTimer roundTime;

//表示共识结束的状态。一旦进入接受阶段
//将是Yes或Movedon
    ConsensusState state = ConsensusState::No;

//本轮提议的同行人数
    std::size_t proposers = 0;
};
}  //命名空间波纹

#endif
