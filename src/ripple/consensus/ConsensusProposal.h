
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
#ifndef RIPPLE_CONSENSUS_ConsensusProposal_H_INCLUDED
#define RIPPLE_CONSENSUS_ConsensusProposal_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/json/json_value.h>
#include <ripple/protocol/JsonFields.h>
#include <cstdint>

namespace ripple {
/*代表在一轮协商一致期间提出的立场。

    在协商一致的过程中，同行寻求就一系列交易达成一致意见
    应用于上一个分类帐以生成下一个分类帐。每一位同行
    关于是否包括或排除潜在交易的立场。
    交易集上的位置作为一个
    两厢情愿的政治阶级的例子。

    达成共识的一个例子可以是我们自己的提议，也可以是
    我们同伴的

    随着共识的推进，同行可能会改变他们在交易中的立场，
    或者选择弃权。每一个连续的提案都包括
    单调增加的数字（或者，如果同伴选择弃权，
    特殊值“seqleave”）。

    有关模板参数的要求，请参阅@ref consensive。

    @tparam nodeid_t用于唯一标识节点/对等点的类型
    @tparam ledgerid_t用于唯一标识分类帐的类型
    @tparam position_t类型，用于表示交易中的头寸
                       在本轮协商一致期间正在审议中
 **/

template <class NodeID_t, class LedgerID_t, class Position_t>
class ConsensusProposal
{
public:
    using NodeID = NodeID_t;

//<对等方最初加入共识时的序列值
    static std::uint32_t const seqJoin = 0;

//<SEQUENCE NUMBER when a peer wants to bow out and leave consensive同伴想退出并保持一致时的序列号
    static std::uint32_t const seqLeave = 0xffffffff;

    /*构造函数

        @param prevledger此建议所基于的上一个分类帐。
        @param seq此建议的序列号。
        @param position本轮交易中的头寸。
        @param closetime此分类帐关闭时的位置。
        @param现在是提议的时间。
        接受此位置的节点/对等机的@param node id id id。
    **/

    ConsensusProposal(
        LedgerID_t const& prevLedger,
        std::uint32_t seq,
        Position_t const& position,
        NetClock::time_point closeTime,
        NetClock::time_point now,
        NodeID_t const& nodeID)
        : previousLedger_(prevLedger)
        , position_(position)
        , closeTime_(closeTime)
        , time_(now)
        , proposeSeq_(seq)
        , nodeID_(nodeID)
    {
    }

//！确定哪个同事担任了这个职位。
    NodeID_t const&
    nodeID() const
    {
        return nodeID_;
    }

//！获取建议的职位。
    Position_t const&
    position() const
    {
        return position_;
    }

//！获取此职位所基于的以前接受的分类帐。
    LedgerID_t const&
    prevLedger() const
    {
        return previousLedger_;
    }

    /*获取此建议的序列号

        从初始序列号“seqjoin”开始，连续
        来自同行的建议将增加序列号。

        @返回序列号
    **/

    std::uint32_t
    proposeSeq() const
    {
        return proposeSeq_;
    }

//！当前的共识立场关闭时间。
    NetClock::time_point const&
    closeTime() const
    {
        return closeTime_;
    }

//！当这个位置被占据的时候。
    NetClock::time_point const&
    seenTime() const
    {
        return time_;
    }

    /*这是否是当前
        协商一致。
    **/

    bool
    isInitial() const
    {
        return proposeSeq_ == seqJoin;
    }

//！获取此节点是否离开协商进程
    bool
    isBowOut() const
    {
        return proposeSeq_ == seqLeave;
    }

//！获取此位置相对于提供的截止点是否过时
    bool
    isStale(NetClock::time_point cutoff) const
    {
        return time_ <= cutoff;
    }

    /*在协商一致过程中更新立场。这将增加
        如果提案还没有被否决，它的序号。

        @param new position获得的新位置。
        @param new close time新的关闭时间。
        @param现在是新位置被占据的时间
     **/

    void
    changePosition(
        Position_t const& newPosition,
        NetClock::time_point newCloseTime,
        NetClock::time_point now)
    {
        position_ = newPosition;
        closeTime_ = newCloseTime;
        time_ = now;
        if (proposeSeq_ != seqLeave)
            ++proposeSeq_;
    }

    /*离开共识

        更新位置以指示节点左一致。

        @param现在是该节点离开协商一致时的时间。
     **/

    void
    bowOut(NetClock::time_point now)
    {
        time_ = now;
        proposeSeq_ = seqLeave;
    }

//！获取用于调试的JSON表示形式
    Json::Value
    getJson() const
    {
        using std::to_string;

        Json::Value ret = Json::objectValue;
        ret[jss::previous_ledger] = to_string(prevLedger());

        if (!isBowOut())
        {
            ret[jss::transaction_hash] = to_string(position());
            ret[jss::propose_seq] = proposeSeq();
        }

        ret[jss::close_time] =
            to_string(closeTime().time_since_epoch().count());

        return ret;
    }

private:
//！此建议所基于的上一个分类帐的唯一标识符
    LedgerID_t previousLedger_;

//！此建议所处位置的唯一标识符
    Position_t position_;

//！此职位的分类帐关闭时间
    NetClock::time_point closeTime_;

//！上次更新此职位的时间
    NetClock::time_point time_;

//！此节点所采取的这些位置的序列号
    std::uint32_t proposeSeq_;

//！接受此位置的节点的标识符
    NodeID_t nodeID_;
};

template <class NodeID_t, class LedgerID_t, class Position_t>
bool
operator==(
    ConsensusProposal<NodeID_t, LedgerID_t, Position_t> const& a,
    ConsensusProposal<NodeID_t, LedgerID_t, Position_t> const& b)
{
    return a.nodeID() == b.nodeID() && a.proposeSeq() == b.proposeSeq() &&
        a.prevLedger() == b.prevLedger() && a.position() == b.position() &&
        a.closeTime() == b.closeTime() && a.seenTime() == b.seenTime();
}
}
#endif
