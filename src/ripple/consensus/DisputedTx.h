
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

#ifndef RIPPLE_APP_CONSENSUS_IMPL_DISPUTEDTX_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_IMPL_DISPUTEDTX_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <ripple/basics/Log.h>
#include <ripple/basics/base_uint.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/consensus/ConsensusParms.h>
#include <ripple/json/json_writer.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <memory>

namespace ripple {

/*在随后的交易中发现有争议的交易。

    在协商一致期间，当一个事务
    被发现有争议。对象仅在
    争论。

    无可争议的事务没有对应的@ref disputedtx对象。

    有关模板类型要求的详细信息，请参阅@ref consension。

    @tparam tx_t事务的类型
    @tparam nodeid“节点标识符的类型”
**/


template <class Tx_t, class NodeID_t>
class DisputedTx
{
    using TxID_t = typename Tx_t::ID;
    using Map_t = boost::container::flat_map<NodeID_t, bool>;
public:
    /*构造函数

        @param tx争议交易
        @param our投票决定是否包括tx
        @param numpeers预期的对等投票数
        @param j调试日志
    **/

    DisputedTx(Tx_t const& tx, bool ourVote, std::size_t numPeers, beast::Journal j)
        : yays_(0), nays_(0), ourVote_(ourVote), tx_(tx), j_(j)
    {
        votes_.reserve(numPeers);
    }

//！有争议事务的唯一ID/哈希。
    TxID_t const&
    ID() const
    {
        return tx_.id();
    }

//！我们就是否应包括交易进行表决。
    bool
    getOurVote() const
    {
        return ourVote_;
    }

//！有争议的交易。
    Tx_t const&
    tx() const
    {
        return tx_;
    }

//！改变我们的投票
    void
    setOurVote(bool o)
    {
        ourVote_ = o;
    }

    /*改变同伴的投票

        @param peer对等标识符。
        @param votesyes是否对等投票包括有争议的交易。
    **/

    void
    setVote(NodeID_t const& peer, bool votesYes);

    /*删除同伴的投票

        @param peer对等标识符。
    **/

    void
    unVote(NodeID_t const& peer);

    /*根据共识的进展更新我们的投票。

        根据同行的投票结果更新我们对该争议交易的投票
        以及达成共识的进展。

        @param percenttime通过协商取得的进展百分比，例如50%
               通过或90%。
        @param提议我们是否在这一轮向我们的同行求婚。
        @param p共识参数控制投票阈值
        @返回我们的投票是否改变
    **/

    bool
    updateVote(int percentTime, bool proposing, ConsensusParms const& p);

//！json争议表示，用于调试
    Json::Value
    getJson() const;

private:
int yays_;      //<赞成票数量
int nays_;      //<无票数
bool ourVote_;  //<我们的投票（对是）
Tx_t tx_;       //<争议交易
Map_t votes_;   //<从nodeid映射到投票
    beast::Journal j_;
};

//跟踪同伴对某个有争议的德克萨斯州的赞成/反对票
template <class Tx_t, class NodeID_t>
void
DisputedTx<Tx_t, NodeID_t>::setVote(NodeID_t const& peer, bool votesYes)
{
    auto res = votes_.insert(std::make_pair(peer, votesYes));

//新投票
    if (res.second)
    {
        if (votesYes)
        {
            JLOG(j_.debug()) << "Peer " << peer << " votes YES on " << tx_.id();
            ++yays_;
        }
        else
        {
            JLOG(j_.debug()) << "Peer " << peer << " votes NO on " << tx_.id();
            ++nays_;
        }
    }
//将投票更改为“是”
    else if (votesYes && !res.first->second)
    {
        JLOG(j_.debug()) << "Peer " << peer << " now votes YES on " << tx_.id();
        --nays_;
        ++yays_;
        res.first->second = true;
    }
//将投票改为否
    else if (!votesYes && res.first->second)
    {
        JLOG(j_.debug()) << "Peer " << peer << " now votes NO on " << tx_.id();
        ++nays_;
        --yays_;
        res.first->second = false;
    }
}

//取消对这一有争议的交易的投票
template <class Tx_t, class NodeID_t>
void
DisputedTx<Tx_t, NodeID_t>::unVote(NodeID_t const& peer)
{
    auto it = votes_.find(peer);

    if (it != votes_.end())
    {
        if (it->second)
            --yays_;
        else
            --nays_;

        votes_.erase(it);
    }
}

template <class Tx_t, class NodeID_t>
bool
DisputedTx<Tx_t, NodeID_t>::updateVote(
    int percentTime,
    bool proposing,
    ConsensusParms const& p)
{
    if (ourVote_ && (nays_ == 0))
        return false;

    if (!ourVote_ && (yays_ == 0))
        return false;

    bool newPosition;
    int weight;

if (proposing)  //全力以赴
    {
//这基本上是节点投票“是”的百分比（包括我们）
        weight = (yays_ * 100 + (ourVote_ ? 100 : 0)) / (nays_ + yays_ + 1);

//为了防止雪崩停止，我们稍微增加所需的重量。
//随着时间的推移。
        if (percentTime < p.avMID_CONSENSUS_TIME)
            newPosition = weight > p.avINIT_CONSENSUS_PCT;
        else if (percentTime < p.avLATE_CONSENSUS_TIME)
            newPosition = weight > p.avMID_CONSENSUS_PCT;
        else if (percentTime < p.avSTUCK_CONSENSUS_TIME)
            newPosition = weight > p.avLATE_CONSENSUS_PCT;
        else
            newPosition = weight > p.avSTUCK_CONSENSUS_PCT;
    }
    else
    {
//不要让我们超过求婚节点，只要承认共识
        weight = -1;
        newPosition = yays_ > nays_;
    }

    if (newPosition == ourVote_)
    {
        JLOG(j_.info()) << "No change (" << (ourVote_ ? "YES" : "NO")
                        << ") : weight " << weight << ", percent "
                        << percentTime;
        JLOG(j_.debug()) << Json::Compact{getJson()};
        return false;
    }

    ourVote_ = newPosition;
    JLOG(j_.debug()) << "We now vote " << (ourVote_ ? "YES" : "NO") << " on "
                     << tx_.id();
    JLOG(j_.debug()) << Json::Compact{getJson()};
    return true;
}

template <class Tx_t, class NodeID_t>
Json::Value
DisputedTx<Tx_t, NodeID_t>::getJson() const
{
    using std::to_string;

    Json::Value ret(Json::objectValue);

    ret["yays"] = yays_;
    ret["nays"] = nays_;
    ret["our_vote"] = ourVote_;

    if (!votes_.empty())
    {
        Json::Value votesj(Json::objectValue);
        for (auto& vote : votes_)
            votesj[to_string(vote.first)] = vote.second;
        ret["votes"] = std::move(votesj);
    }

    return ret;
}

}  //涟漪

#endif
