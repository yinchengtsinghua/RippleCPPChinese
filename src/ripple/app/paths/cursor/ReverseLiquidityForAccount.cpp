
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

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace path {

//计算saprverredeemreq、saprvisuereq、saprvdelivery from sacur，基于
//所需的可交付成果、宣传兑换、发行（用于账户）和交付
//向上一个节点请求（对于订单簿）。
//
//夸大所需费用要求的金额。
//Reedem的限制基于IOU之前的现有数据。
//根据信用额度和欠款金额限制发行。
//
//货币不能是瑞波币，因为我们正在波动。
//
//没有永久账户余额调整，因为我们不知道会有多少。
//实际上还没有完成-更改只在草稿栏中
//分类帐。
//
//<--Tesuccess或Tecpath_Dry

TER PathCursor::reverseLiquidityForAccount () const
{
    TER terResult = tesSUCCESS;
    auto const lastNodeIndex = nodeSize () - 1;
    auto const isFinalNode = (nodeIndex_ == lastNodeIndex);

//0质量表示尚未确定。
    std::uint64_t uRateMax = 0;

//当前可以兑换到下一个。
    const bool previousNodeIsAccount = !nodeIndex_ ||
            previousNode().isAccount();

    const bool nextNodeIsAccount = isFinalNode || nextNode().isAccount();

    AccountID const& previousAccountID = previousNodeIsAccount
        ? previousNode().account_ : node().account_;
    AccountID const& nextAccountID = nextNodeIsAccount ? nextNode().account_
: node().account_;   //优惠总是有问题的。

//这是从上一个节点到这个节点的质量。
    auto const qualityIn
         = (nodeIndex_ != 0)
            ? quality_in (view(),
                node().account_,
                previousAccountID,
                node().issue_.currency)
            : parityRate;

//这就是从下一个到这一个的质量。
    auto const qualityOut
        = (nodeIndex_ != lastNodeIndex)
            ? quality_out (view(),
                node().account_,
                nextAccountID,
                node().issue_.currency)
            : parityRate;

//对于PreviousNodeIsAccount：
//上一个帐户已到期。
    const STAmount saPrvOwed = (previousNodeIsAccount && nodeIndex_ != 0)
        ? creditBalance (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

//上一个帐户可能欠的限额。
    const STAmount saPrvLimit = (previousNodeIsAccount && nodeIndex_ != 0)
        ? creditLimit (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

//下一个帐户已到期。
    const STAmount saNxtOwed = (nextNodeIsAccount && nodeIndex_ != lastNodeIndex)
        ? creditBalance (view(),
            node().account_,
            nextAccountID,
            node().issue_.currency)
        : STAmount (node().issue_);

    JLOG (j_.trace())
        << "reverseLiquidityForAccount>"
        << " nodeIndex_=" << nodeIndex_ << "/" << lastNodeIndex
        << " previousAccountID=" << previousAccountID
        << " node.account_=" << node().account_
        << " nextAccountID=" << nextAccountID
        << " currency=" << node().issue_.currency
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvOwed=" << saPrvOwed
        << " saPrvLimit=" << saPrvLimit;

//请求被计算为可能的最大流。
//上一个可以赎回它持有的欠条。
    const STAmount saPrvRedeemReq  = (saPrvOwed > beast::zero)
        ? saPrvOwed
        : STAmount (saPrvOwed.issue ());

//上一个可以发出上限减去限制的任何部分
//已使用（不包括可赎回金额）-另一个“最大流量”。
    const STAmount saPrvIssueReq = (saPrvOwed < beast::zero)
        ? saPrvLimit + saPrvOwed : saPrvLimit;

//如果我们有订单，请预先计算这些值。
    auto deliverCurrency = previousNode().saRevDeliver.getCurrency ();
    const STAmount saPrvDeliverReq (
        {deliverCurrency, previousNode().saRevDeliver.getIssuer ()}, -1);
//-1表示无限制交货。

//设置为零，因为我们正试图访问上一个节点。
    auto saCurRedeemAct = node().saRevRedeem.zeroed();

//跟踪我们实际兑换的金额。
    auto saCurIssueAct = node().saRevIssue.zeroed();

//为了！下一个节点帐户
    auto saCurDeliverAct  = node().saRevDeliver.zeroed();

    JLOG (j_.trace())
        << "reverseLiquidityForAccount:"
        << " saPrvRedeemReq:" << saPrvRedeemReq
        << " saPrvIssueReq:" << saPrvIssueReq
        << " previousNode.saRevDeliver:" << previousNode().saRevDeliver
        << " saPrvDeliverReq:" << saPrvDeliverReq
        << " node.saRevRedeem:" << node().saRevRedeem
        << " node.saRevIssue:" << node().saRevIssue
        << " saNxtOwed:" << saNxtOwed;

//vfalc-fixme这会产生错误
//jLog（j_.trace（））<<pathstate_.getjson（）；

//当前兑换请求不能超过手头的借据。
    assert (!node().saRevRedeem || -saNxtOwed >= node().saRevRedeem);
assert (!node().saRevIssue  //如果不签发，罚款。
            || saNxtOwed >= beast::zero
//sanxtown>=0:发送方未持有下一个IOU，sanxtown<0:
//寄件人持有下一个借据。
            || -saNxtOwed == node().saRevRedeem);
//如果发出REQ，那么REQ必须消耗所有欠款。

    if (nodeIndex_ == 0)
    {
//^->账户——>账户报价
//无事可做，无需调整。
//
//托多（汤姆）：我们本可以跳过所有的设置，然后就离开了。
//或者甚至永远不要在nodeindex上调用整个例程。
    }

//接下来的四个案例对应于这个wiki底部的表格
//页面部分：https://ripple.com/wiki/transit-fees实现
    else if (previousNodeIsAccount && nextNodeIsAccount)
    {
        if (isFinalNode)
        {
//账户——>账户——-->$
//总体可交付成果。
            const STAmount saCurWantedReq = std::min (
                pathState_.outReq() - pathState_.outAct(),
                saPrvLimit + saPrvOwed);
            auto saCurWantedAct = saCurWantedReq.zeroed ();

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: account --> "
                << "ACCOUNT --> $ :"
                << " saCurWantedReq=" << saCurWantedReq;

//计算兑换
if (saPrvRedeemReq) //以前有借据要兑换。
            {
//1:1兑换自己的借据

                saCurWantedAct = std::min (saPrvRedeemReq, saCurWantedReq);
                previousNode().saRevRedeem = saCurWantedAct;

                uRateMax = STAmount::uRateOne;

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: Redeem at 1:1"
                    << " saPrvRedeemReq=" << saPrvRedeemReq
                    << " (available) previousNode.saRevRedeem="
                    << previousNode().saRevRedeem
                    << " uRateMax="
                    << amountFromQuality (uRateMax).getText ();
            }
            else
            {
                previousNode().saRevRedeem.clear (saPrvRedeemReq);
            }

//计算发行量。
            previousNode().saRevIssue.clear (saPrvIssueReq);

if (saCurWantedReq != saCurWantedAct //需要更多。
&& saPrvIssueReq)  //将接受以前的借据。
            {
//费率：质量：1.0

//如果我们以前赎回过，而这个兑换率较低，
//不包括当前增量。
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    saPrvIssueReq,
                    saCurWantedReq,
                    previousNode().saRevIssue,
                    saCurWantedAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: Issuing: Rate: "
                    << "quality in : 1.0"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurWantedAct:" << saCurWantedAct;
            }

            if (!saCurWantedAct)
            {
//一定处理过什么。
                terResult   = tecPATH_DRY;
            }
        }
        else
        {
//不是最终节点。
//账户——>账户——>账户
            previousNode().saRevRedeem.clear (saPrvRedeemReq);
            previousNode().saRevIssue.clear (saPrvIssueReq);

//赎回（第1部分）->赎回
            if (node().saRevRedeem
//下一个要从活期账户中赎回欠条。
                && saPrvRedeemReq)
//上一个帐户有借据要兑换到往来帐户。
            {
//托多（汤姆）：加上英语。
//费率：1.0：质量问题-我们必须接受我们自己的IOU
//1:1。
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    saPrvRedeemReq,
                    node().saRevRedeem,
                    previousNode().saRevRedeem,
                    saCurRedeemAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate : 1.0 : quality out"
                    << " previousNode.saRevRedeem:" << previousNode().saRevRedeem
                    << " saCurRedeemAct:" << saCurRedeemAct;
            }

//发行（第1部分）->赎回
            if (node().saRevRedeem != saCurRedeemAct
//当前节点有更多要兑现的IOU。
                && previousNode().saRevRedeem == saPrvRedeemReq)
//上一个节点没有要兑现的借据，所以会出现问题。
            {
//比率：质量输入：质量输出
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    qualityOut,
                    saPrvIssueReq,
                    node().saRevRedeem,
                    previousNode().saRevIssue,
                    saCurRedeemAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate: quality in : quality out:"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurRedeemAct:" << saCurRedeemAct;
            }

//赎回（第2部分）->发行。
if (node().saRevIssue   //下一个要签发借据。
//托多（汤姆）：这种情况似乎是多余的。
                && saCurRedeemAct == node().saRevRedeem
//只有在完成赎回后才能发行。
                && previousNode().saRevRedeem != saPrvRedeemReq)
//没有完成以前的借据的赎回。
            {
//费率：1.0：转账费率
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    saPrvRedeemReq,
                    node().saRevIssue,
                    previousNode().saRevRedeem,
                    saCurIssueAct,
                    uRateMax);

                JLOG (j_.debug())
                    << "reverseLiquidityForAccount: "
                    << "Rate : 1.0 : transfer_rate:"
                    << " previousNode.saRevRedeem:" << previousNode().saRevRedeem
                    << " saCurIssueAct:" << saCurIssueAct;
            }

//问题（第2部分）->问题
            if (node().saRevIssue != saCurIssueAct
//需要更多的借据。
                && saCurRedeemAct == node().saRevRedeem
//只有在完成赎回后才能发行。
                && saPrvRedeemReq == previousNode().saRevRedeem
//以前赎回了所有欠条。
                && saPrvIssueReq)
//上一个可以发布。
            {
//费率：质量：1.0
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    saPrvIssueReq,
                    node().saRevIssue,
                    previousNode().saRevIssue,
                    saCurIssueAct,
                    uRateMax);

                JLOG (j_.trace())
                    << "reverseLiquidityForAccount: "
                    << "Rate: quality in : 1.0:"
                    << " previousNode.saRevIssue:" << previousNode().saRevIssue
                    << " saCurIssueAct:" << saCurIssueAct;
            }

            if (!saCurRedeemAct && !saCurIssueAct)
            {
//没有取得进展。
                terResult = tecPATH_DRY;
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "^|account --> ACCOUNT --> account :"
                << " node.saRevRedeem:" << node().saRevRedeem
                << " node.saRevIssue:" << node().saRevIssue
                << " saPrvOwed:" << saPrvOwed
                << " saCurRedeemAct:" << saCurRedeemAct
                << " saCurIssueAct:" << saCurIssueAct;
        }
    }
    else if (previousNodeIsAccount && !nextNodeIsAccount)
    {
//账户——>账户——>报价
//注：交割总是发行，因为账户是报价的发行人。
//输入。
        JLOG (j_.trace())
            << "reverseLiquidityForAccount: "
            << "account --> ACCOUNT --> offer";

        previousNode().saRevRedeem.clear (saPrvRedeemReq);
        previousNode().saRevIssue.clear (saPrvIssueReq);

//我们有三种情况：NXT报价可由活期账户持有，
//以前的帐户或某些第三方帐户。
//
//此外，经常账户可能有或可能没有可赎回的余额。
//下一个报价的账户，所以我们还不知道
//赎回或发行。
//
//TODO（TOM）：确保发货已清除，或检查实际值为零。
//兑换->交付/发行。
if (saPrvOwed > beast::zero                    //以前有借据要兑换。
&& node().saRevDeliver)                 //需要发行一些。
        {
//费率：1.0：转账费率
            rippleLiquidity (
                rippleCalc_,
                parityRate,
                transferRate (view(), node().account_),
                saPrvRedeemReq,
                node().saRevDeliver,
                previousNode().saRevRedeem,
                saCurDeliverAct,
                uRateMax);
        }

//问题->交付/问题
        if (saPrvRedeemReq == previousNode().saRevRedeem
//以前赎回了所有欠款。
&& node().saRevDeliver != saCurDeliverAct)  //还需要一些发行版。
        {
//费率：质量：1.0
            rippleLiquidity (
                rippleCalc_,
                qualityIn,
                parityRate,
                saPrvIssueReq,
                node().saRevDeliver,
                previousNode().saRevIssue,
                saCurDeliverAct,
                uRateMax);
        }

        if (!saCurDeliverAct)
        {
//一定想要点什么。
            terResult   = tecPATH_DRY;
        }

        JLOG (j_.trace())
            << "reverseLiquidityForAccount: "
            << " node.saRevDeliver:" << node().saRevDeliver
            << " saCurDeliverAct:" << saCurDeliverAct
            << " saPrvOwed:" << saPrvOwed;
    }
    else if (!previousNodeIsAccount && nextNodeIsAccount)
    {
        if (isFinalNode)
        {
//报价——>账户---$
//前一个是出价，没有限制：赎回自己的借据。
//
//这是最后一个节点；我们不能向右寻找值；
//我们必须向上获取整个路径状态的out值。
            STAmount const& saCurWantedReq  =
                    pathState_.outReq() - pathState_.outAct();
            STAmount saCurWantedAct = saCurWantedReq.zeroed();

            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "offer --> ACCOUNT --> $ :"
                << " saCurWantedReq:" << saCurWantedReq
                << " saOutAct:" << pathState_.outAct()
                << " saOutReq:" << pathState_.outReq();

            if (saCurWantedReq <= beast::zero)
            {
                assert(false);
                JLOG (j_.fatal()) << "CurWantReq was not positive";
                return tefEXCEPTION;
            }

//上一个节点是报价；我们收到自己的货币。

//上一个订单的条目可能包含我们的发货；可能
//不保留我们的发行；可能是我们自己的报价。
//
//假设最坏的情况，最昂贵的情况
//通过，这不是我们自己的提议，也不是我们自己的提议
//发行。在前面传球的时候，我们可以做到
//更好。
//
//TODO:此注释通常适用于此节-上移
//给一份文件

//费率：质量：1.0
            rippleLiquidity (
                rippleCalc_,
                qualityIn,
                parityRate,
                saPrvDeliverReq,
                saCurWantedReq,
                previousNode().saRevDeliver,
                saCurWantedAct,
                uRateMax);

            if (!saCurWantedAct)
            {
//一定处理过什么。
                terResult   = tecPATH_DRY;
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount:"
                << " previousNode().saRevDeliver:" << previousNode().saRevDeliver
                << " saPrvDeliverReq:" << saPrvDeliverReq
                << " saCurWantedAct:" << saCurWantedAct
                << " saCurWantedReq:" << saCurWantedReq;
        }
        else
        {
//报价——>账户——>账户
//注：要约总是以账户为发行人交付（赎回）。
            JLOG (j_.trace())
                << "reverseLiquidityForAccount: "
                << "offer --> ACCOUNT --> account :"
                << " node.saRevRedeem:" << node().saRevRedeem
                << " node.saRevIssue:" << node().saRevIssue;

//交付->兑现
//托多（汤姆）：现在我们有更多的支票，这些支票
//可能是多余的。
if (node().saRevRedeem)  //接下来要我们赎回。
            {
//cur持有账户右侧的借据，nxt
//帐户。如果有人在做活期存款，就把它扔掉。
//NXT帐户的IOU，然后收取输入的质量费用
//出来。
//
//费率：1.0：质量问题
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    saPrvDeliverReq,
                    node().saRevRedeem,
                    previousNode().saRevDeliver,
                    saCurRedeemAct,
                    uRateMax);
            }

//交货->发货。
            if (node().saRevRedeem == saCurRedeemAct
//只能在以前全部兑现的情况下发行。
                && node().saRevIssue)
//需要发行一些。
            {
//费率：1.0：转账费率
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    saPrvDeliverReq,
                    node().saRevIssue,
                    previousNode().saRevDeliver,
                    saCurIssueAct,
                    uRateMax);
            }

            JLOG (j_.trace())
                << "reverseLiquidityForAccount:"
                << " saCurRedeemAct:" << saCurRedeemAct
                << " node.saRevRedeem:" << node().saRevRedeem
                << " previousNode.saRevDeliver:" << previousNode().saRevDeliver
                << " node.saRevIssue:" << node().saRevIssue;

            if (!previousNode().saRevDeliver)
            {
//一定想要点什么。
                terResult   = tecPATH_DRY;
            }
        }
    }
    else
    {
//优惠——>账户——>优惠
//交付/兑换->交付/发行。
        JLOG (j_.trace())
            << "reverseLiquidityForAccount: offer --> ACCOUNT --> offer";

//费率：1.0：转账费率
        rippleLiquidity (
            rippleCalc_,
            parityRate,
            transferRate (view(), node().account_),
            saPrvDeliverReq,
            node().saRevDeliver,
            previousNode().saRevDeliver,
            saCurDeliverAct,
            uRateMax);

        if (!saCurDeliverAct)
        {
//一定想要点什么。
            terResult   = tecPATH_DRY;
        }
    }

    return terResult;
}

} //路径
} //涟漪
