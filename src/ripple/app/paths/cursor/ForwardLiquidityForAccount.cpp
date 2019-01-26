
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

#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Quality.h>

namespace ripple {
namespace path {

//反向通行证因可用信贷和费用膨胀而缩小。
//因为它是反向的。现在，对于当前帐户节点，取
//以前的金额并调整远期余额。
//
//在上一个节点和当前节点之间执行平衡调整。
//-上一个节点：指定要推送到当前节点的内容。
//-所有以前的输出都被消耗。
//
//然后，为下一个节点计算当前节点的输出。
//-当前节点：指定要推进到下一个节点的内容。
//-到下一个节点的输出计算为输入减去质量或转移费。
//-如果下一个节点是报价，而输出是非xrp，那么我们是发行人，并且
//不需要推动资金。
//-如果下一个节点是报价，输出是xrp，那么我们需要向
//地狱边境。
TER PathCursor::forwardLiquidityForAccount () const
{
    TER resultCode   = tesSUCCESS;
    auto const lastNodeIndex       = pathState_.nodes().size () - 1;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    std::uint64_t uRateMax = 0;

    AccountID const& previousAccountID =
            previousNode().isAccount() ? previousNode().account_ :
            node().account_;
//优惠总是有问题的。
    AccountID const& nextAccountID =
            nextNode().isAccount() ? nextNode().account_ : node().account_;

    auto const qualityIn = nodeIndex_
        ? quality_in (view(),
            node().account_,
            previousAccountID,
            node().issue_.currency)
        : parityRate;

    auto const qualityOut = (nodeIndex_ == lastNodeIndex)
        ? quality_out (view(),
            node().account_,
            nextAccountID,
            node().issue_.currency)
        : parityRate;

//当我们回头看REQ时，我们关心的只是
//计算：使用FWD。
//当期待（cur）的需求时，我们关心的是什么是需要的：使用
//牧师。

//对于nextnode（）.isaccount（）。
    auto saPrvRedeemAct = previousNode().saFwdRedeem.zeroed();
    auto saPrvIssueAct = previousNode().saFwdIssue.zeroed();

//为了！previousNode（）.isaccount（））
    auto saPrvDeliverAct = previousNode().saFwdDeliver.zeroed ();

    JLOG (j_.trace())
        << "forwardLiquidityForAccount> "
        << "nodeIndex_=" << nodeIndex_ << "/" << lastNodeIndex
        << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
        << " saPrvIssueReq:" << previousNode().saFwdIssue
        << " previousNode.saFwdDeliver:" << previousNode().saFwdDeliver
        << " node.saRevRedeem:" << node().saRevRedeem
        << " node.saRevIssue:" << node().saRevIssue
        << " node.saRevDeliver:" << node().saRevDeliver;

//连锁反应。

    if (previousNode().isAccount() && nextNode().isAccount())
    {
//接下来是一个账户，一定是泛起涟漪。

        if (!nodeIndex_)
        {
//^->账户——>账户

//对于第一个节点，根据
//可用。
            node().saFwdRedeem = node().saRevRedeem;

            if (pathState_.inReq() >= beast::zero)
            {
//最大发送限制
                node().saFwdRedeem = std::min (
                    node().saFwdRedeem, pathState_.inReq() - pathState_.inAct());
            }

            pathState_.setInPass (node().saFwdRedeem);

            node().saFwdIssue = node().saFwdRedeem == node().saRevRedeem
//完全赎回。
                ? node().saRevIssue : STAmount (node().saRevIssue);

            if (node().saFwdIssue && pathState_.inReq() >= beast::zero)
            {
//最大发送限制
                node().saFwdIssue = std::min (
                    node().saFwdIssue,
                    pathState_.inReq() - pathState_.inAct() - node().saFwdRedeem);
            }

            pathState_.setInPass (pathState_.inPass() + node().saFwdIssue);

            JLOG (j_.trace())
                << "forwardLiquidityForAccount: ^ --> "
                << "ACCOUNT --> account :"
                << " saInReq=" << pathState_.inReq()
                << " saInAct=" << pathState_.inAct()
                << " node.saFwdRedeem:" << node().saFwdRedeem
                << " node.saRevIssue:" << node().saRevIssue
                << " node.saFwdIssue:" << node().saFwdIssue
                << " pathState_.saInPass:" << pathState_.inPass();
        }
        else if (nodeIndex_ == lastNodeIndex)
        {
//账户——>账户——-->$
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> $ :"
                << " previousAccountID="
                << to_string (previousAccountID)
                << " node.account_="
                << to_string (node().account_)
                << " previousNode.saFwdRedeem:" << previousNode().saFwdRedeem
                << " previousNode.saFwdIssue:" << previousNode().saFwdIssue;

//最后一个节点。接受所有资金。计算实际贷记金额。

            auto& saCurReceive = pathState_.outPass();
            STAmount saIssueCrd = qualityIn >= parityRate
? previousNode().saFwdIssue  //不收费。
                    : multiplyRound (
                          previousNode().saFwdIssue,
                          qualityIn,
true); //贷方金额。

//贷方金额。少于收到的作为附加费的贷项。
            pathState_.setOutPass (previousNode().saFwdRedeem + saIssueCrd);

            if (saCurReceive)
            {
//实际接收。
                resultCode = rippleCredit(view(),
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ);
            }
            else
            {
//应用质量后，总付款是微观的。
                resultCode   = tecPATH_DRY;
            }
        }
        else
        {
//账户——>账户——>账户
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

//上一次兑换第1部分：兑换->兑换
            if (previousNode().saFwdRedeem && node().saRevRedeem)
//上一个想要兑换。
            {
//费率：1.0：质量问题
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    previousNode().saFwdRedeem,
                    node().saRevRedeem,
                    saPrvRedeemAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

//上期第1部分：发行->赎回
            if (previousNode().saFwdIssue != saPrvIssueAct
//上一个要发布。
                && node().saRevRedeem != node().saFwdRedeem)
//当前有更多要兑换到下一个。
            {
//比率：质量输入：质量输出
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    qualityOut,
                    previousNode().saFwdIssue,
                    node().saRevRedeem,
                    saPrvIssueAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

//前面的兑换第2部分：兑换->发行。
            if (previousNode().saFwdRedeem != saPrvRedeemAct
//上一个仍想兑换。
                && node().saRevRedeem == node().saFwdRedeem
//当前已完成的赎回可以发行。
                && node().saRevIssue)
//current想要发布。
            {
//费率：1.0：转账费率
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevIssue,
                    saPrvRedeemAct,
                    node().saFwdIssue,
                    uRateMax);
            }

//上一期第2部分：发行->发行
            if (previousNode().saFwdIssue != saPrvIssueAct
//上一个要发布。
                && node().saRevRedeem == node().saFwdRedeem
//当前已完成的赎回可以发行。
                && node().saRevIssue)
//current想要发布。
            {
//费率：质量：1.0
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    previousNode().saFwdIssue,
                    node().saRevIssue,
                    saPrvIssueAct,
                    node().saFwdIssue,
                    uRateMax);
            }

            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

//调整prv->cur余额：取所有入站
            resultCode = saProvide
                ? rippleCredit(view(),
                    previousAccountID,
                    node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ)
                : tecPATH_DRY;
        }
    }
    else if (previousNode().isAccount() && !nextNode().isAccount())
    {
//活期账户是下一个报价的发行人。
//确定交货报价金额。
//不要调整出站余额-把资金放在发行人的边缘。
//如果发行人持有的是报价所有者的入站借据，则无需支付任何费用和
//兑换/发行将透明地发生。

        if (nodeIndex_)
        {
//非xrp，当前节点是颁发者。
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: account --> "
                << "ACCOUNT --> offer";

            node().saFwdDeliver.clear (node().saRevDeliver);

//兑换->发行/交付。
//上一个想要兑换。
//往来银行正在发出一个报价，所以把资金留在账户中
//“地狱”。
            if (previousNode().saFwdRedeem)
//上一个想要兑换。
            {
//费率：1.0：转账费率
//这里的转账率是正确的吗？
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdRedeem,
                    node().saRevDeliver,
                    saPrvRedeemAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

//发出->发出/交付
            if (previousNode().saFwdRedeem == saPrvRedeemAct
//上一个已完成的赎回：上一个没有借据。
                && previousNode().saFwdIssue)
//上一个要发布。到下一个必须是好的。
            {
//费率：质量：1.0
                rippleLiquidity (
                    rippleCalc_,
                    qualityIn,
                    parityRate,
                    previousNode().saFwdIssue,
                    node().saRevDeliver,
                    saPrvIssueAct,
                    node().saFwdDeliver,
                    uRateMax);
            }

//调整prv->cur余额：取所有入站
            resultCode   = node().saFwdDeliver
                ? rippleCredit(view(),
                    previousAccountID, node().account_,
                    previousNode().saFwdRedeem + previousNode().saFwdIssue,
                    false, viewJ)
: tecPATH_DRY;  //实际上什么都没送。
        }
        else
        {
//从下游请求的交货量。
            node().saFwdDeliver = node().saRevDeliver;

//如果有限制，则通过发送最大值和可用值进行限制。
            if (pathState_.inReq() >= beast::zero)
            {
//最大发送限制
                node().saFwdDeliver = std::min (
                    node().saFwdDeliver, pathState_.inReq() - pathState_.inAct());

//可用限制xrp。非XRP发行人无限制。
                if (isXRP (node().issue_))
                    node().saFwdDeliver = std::min (
                        node().saFwdDeliver,
                        accountHolds(view(),
                            node().account_,
                            xrpCurrency(),
                            xrpAccount(),
fhIGNORE_FREEZE, viewJ)); //xrp不能冻结

            }

//记录为通过而发送的金额。
            pathState_.setInPass (node().saFwdDeliver);

            if (!node().saFwdDeliver)
            {
                resultCode   = tecPATH_DRY;
            }
            else if (!isXRP (node().issue_))
            {
//非xrp，当前节点是颁发者。
//我们可能会将邮件发送到多个帐户，因此我们不知道
//哪个纹波平衡会被调整。假设只是发行。

                JLOG (j_.trace())
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- !XRP --> offer";

//作为发行人，只会发行。
//不需要实际交付。从休假开始
//发行人处于边缘。
            }
            else
            {
                JLOG (j_.trace())
                    << "forwardLiquidityForAccount: ^ --> "
                    << "ACCOUNT -- XRP --> offer";

//将XRP交付至Limbo。
                resultCode = accountSend(view(),
                    node().account_, xrpAccount(), node().saFwdDeliver, viewJ);
            }
        }
    }
    else if (!previousNode().isAccount() && nextNode().isAccount())
    {
        if (nodeIndex_ == lastNodeIndex)
        {
//报价——>账户---$
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> $ : "
                << previousNode().saFwdDeliver;

//贷方金额。
            pathState_.setOutPass (previousNode().saFwdDeliver);

//无需调整收入余额。付款方在里面
//出价支付给这个账户。
        }
        else
        {
//报价——>账户——>账户
            JLOG (j_.trace())
                << "forwardLiquidityForAccount: offer --> "
                << "ACCOUNT --> account";

            node().saFwdRedeem.clear (node().saRevRedeem);
            node().saFwdIssue.clear (node().saRevIssue);

//交付->兑现
            if (previousNode().saFwdDeliver && node().saRevRedeem)
//上一个要交付并且可以当前兑现。
            {
//费率：1.0：质量问题
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    qualityOut,
                    previousNode().saFwdDeliver,
                    node().saRevRedeem,
                    saPrvDeliverAct,
                    node().saFwdRedeem,
                    uRateMax);
            }

//交货->发货
//想要赎回，现在可以发行。
            if (previousNode().saFwdDeliver != saPrvDeliverAct
//上一个仍想交付。
                && node().saRevRedeem == node().saFwdRedeem
//当前有更多要兑换到下一个。
                && node().saRevIssue)
//当前需要的问题。
            {
//费率：1.0：转账费率
                rippleLiquidity (
                    rippleCalc_,
                    parityRate,
                    transferRate (view(), node().account_),
                    previousNode().saFwdDeliver,
                    node().saRevIssue,
                    saPrvDeliverAct,
                    node().saFwdIssue,
                    uRateMax);
            }

//无需调整收入余额。付款方在里面
//出价已支付，下一个链接将收到。
            STAmount saProvide = node().saFwdRedeem + node().saFwdIssue;

            if (!saProvide)
                resultCode = tecPATH_DRY;
        }
    }
    else
    {
//优惠——>账户——>优惠
//交付/兑换->交付/发行。
        JLOG (j_.trace())
            << "forwardLiquidityForAccount: offer --> ACCOUNT --> offer";

        node().saFwdDeliver.clear (node().saRevDeliver);

        if (previousNode().saFwdDeliver && node().saRevDeliver)
        {
//费率：1.0：转账费率
            rippleLiquidity (
                rippleCalc_,
                parityRate,
                transferRate (view(), node().account_),
                previousNode().saFwdDeliver,
                node().saRevDeliver,
                saPrvDeliverAct,
                node().saFwdDeliver,
                uRateMax);
        }

//无需调整收入余额。付款方在
//出价已支付，下一个链接将收到。
        if (!node().saFwdDeliver)
            resultCode   = tecPATH_DRY;
    }

    return resultCode;
}

} //路径
} //涟漪
