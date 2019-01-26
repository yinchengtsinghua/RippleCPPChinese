
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

#include <ripple/basics/Log.h>
#include <ripple/consensus/Consensus.h>

namespace ripple {

bool
shouldCloseLedger(
    bool anyTransactions,
    std::size_t prevProposers,
    std::size_t proposersClosed,
    std::size_t proposersValidated,
    std::chrono::milliseconds prevRoundTime,
    std::chrono::milliseconds
timeSincePrevClose,              //自上次分类帐关闭时间以来的时间
std::chrono::milliseconds openTime,  //等待关闭此分类帐的时间
    std::chrono::milliseconds idleInterval,
    ConsensusParms const& parms,
    beast::Journal j)
{
    using namespace std::chrono_literals;
    if ((prevRoundTime < -1s) || (prevRoundTime > 10min) ||
        (timeSincePrevClose > 10min))
    {
//这些都是意外情况，我们只需关闭分类帐
        JLOG(j.warn()) << "shouldCloseLedger Trans="
                       << (anyTransactions ? "yes" : "no")
                       << " Prop: " << prevProposers << "/" << proposersClosed
                       << " Secs: " << timeSincePrevClose.count()
                       << " (last: " << prevRoundTime.count() << ")";
        return true;
    }

    if ((proposersClosed + proposersValidated) > (prevProposers / 2))
    {
//如果超过一半的网络已经关闭，我们将关闭
        JLOG(j.trace()) << "Others have closed";
        return true;
    }

    if (!anyTransactions)
    {
//仅在空闲间隔结束时关闭
return timeSincePrevClose >= idleInterval;  //正常空闲
    }

//保留最小分类帐打开时间
    if (openTime < parms.ledgerMIN_CLOSE)
    {
        JLOG(j.debug()) << "Must wait minimum time before closing";
        return false;
    }

//不要让这本分类帐比上一本快两倍。
//Ledger达成共识，以便速度较慢的验证器可以减慢速度
//网络
    if (openTime < (prevRoundTime / 2))
    {
        JLOG(j.debug()) << "Ledger has not been open long enough";
        return false;
    }

//关闭分类帐
    return true;
}

bool
checkConsensusReached(
    std::size_t agreeing,
    std::size_t total,
    bool count_self,
    std::size_t minConsensusPct)
{
//如果我们是一个人，我们有一个共识
    if (total == 0)
        return true;

    if (count_self)
    {
        ++agreeing;
        ++total;
    }

    std::size_t currentPercentage = (agreeing * 100) / total;

    return currentPercentage >= minConsensusPct;
}

ConsensusState
checkConsensus(
    std::size_t prevProposers,
    std::size_t currentProposers,
    std::size_t currentAgree,
    std::size_t currentFinished,
    std::chrono::milliseconds previousAgreeTime,
    std::chrono::milliseconds currentAgreeTime,
    ConsensusParms const& parms,
    bool proposing,
    beast::Journal j)
{
    JLOG(j.trace()) << "checkConsensus: prop=" << currentProposers << "/"
                    << prevProposers << " agree=" << currentAgree
                    << " validated=" << currentFinished
                    << " time=" << currentAgreeTime.count() << "/"
                    << previousAgreeTime.count();

    if (currentAgreeTime <= parms.ledgerMIN_CONSENSUS)
        return ConsensusState::No;

    if (currentProposers < (prevProposers * 3 / 4))
    {
//不到最后一个分类账提议者的3/4；不要
//拉什：我们可能需要更多的时间。
        if (currentAgreeTime < (previousAgreeTime + parms.ledgerMIN_CONSENSUS))
        {
            JLOG(j.trace()) << "too fast, not enough proposers";
            return ConsensusState::No;
        }
    }

//我们和unl列表上的节点一起达到了阈值吗？
//宣布达成共识？
    if (checkConsensusReached(
            currentAgree, currentProposers, proposing, parms.minCONSENSUS_PCT))
    {
        JLOG(j.debug()) << "normal consensus";
        return ConsensusState::Yes;
    }

//在我们的UNL列表上有足够的节点移动并达到阈值
//宣布达成共识？
    if (checkConsensusReached(
            currentFinished, currentProposers, false, parms.minCONSENSUS_PCT))
    {
        JLOG(j.warn()) << "We see no consensus, but 80% of nodes have moved on";
        return ConsensusState::MovedOn;
    }

//还没有达成共识
    JLOG(j.trace()) << "no consensus";
    return ConsensusState::No;
}

}  //命名空间波纹
