
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

#include <ripple/app/paths/cursor/EffectiveRate.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {

//对于当前报价，从deliver/limbo获取输入并输出到下一个账户或
//为下一个报价交货。
//
//<--node.safwddeliver:for forwardliquidityforaccount to know
//经历了多少
//-->node.sarevdeliver:不要超过。

TER PathCursor::deliverNodeForward (
AccountID const& uInAccountID,    //-->输入所有者的帐户。
STAmount const& saInReq,        //>交货量。
STAmount& saInAct,              //<--已交付的金额，此调用。
STAmount& saInFees,             //<--收费，此调用。
    bool callerHasLiquidity) const
{
    TER resultCode   = tesSUCCESS;

//不要交付超过想要的。
//反向通过时归零。
    node().directory.restart(multiQuality_);

    saInAct.clear (saInReq);
    saInFees.clear (saInReq);

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

//XXX可能要确保不要超过node（）。sarevdeliver是
//停下来？
    while (resultCode == tesSUCCESS && saInAct + saInFees < saInReq)
    {
//没有花掉所有的入站交货资金。
        if (++loopCount >
            (multiQuality_ ?
                CALC_NODE_DELIVER_MAX_LOOPS_MQ :
                CALC_NODE_DELIVER_MAX_LOOPS))
        {
            JLOG (j_.warn())
                << "deliverNodeForward: max loops cndf";
            return telFAILED_PROCESSING;
        }

//确定调整Sainact、Sainfees和
//node（）.safwddeliver。
        advanceNode (saInAct, false, callerHasLiquidity);

//如果需要，请提前到下一个资助的报价。

        if (resultCode != tesSUCCESS)
        {
        }
        else if (!node().offerIndex_)
        {
            JLOG (j_.warn())
                << "deliverNodeForward: INTERNAL ERROR: Ran out of offers.";
            return telFAILED_PROCESSING;
        }
        else if (resultCode == tesSUCCESS)
        {
            auto const xferRate = effectiveRate (
                previousNode().issue_,
                uInAccountID,
                node().offerOwnerAccount_,
                previousNode().transferRate_);

//首先计算假设没有输出费用：sainpassact，
//SainPassfees，SaoutPassact。

//提供最高限额-由基金限制，不收取费用。
            auto saOutFunded = std::min (
                node().saOfferFunds, node().saTakerGets);

//提供最多可交付的最大超出限制。
            auto saOutPassFunded = std::min (
                saOutFunded,
                node().saRevDeliver - node().saFwdDeliver);

//最高出价-受付款限制。
            auto saInFunded = mulRound (
                saOutPassFunded,
                node().saOfrRate,
                node().saTakerPays.issue (),
                true);

//提供最高收费。
            auto saInTotal = multiplyRound (
                saInFunded, xferRate, true);
            auto saInRemaining = saInReq - saInAct - saInFees;

            if (saInRemaining < beast::zero)
                saInRemaining.clear();

//受剩余的限制。
            auto saInSum = std::min (saInTotal, saInRemaining);

//免费入住。
            auto saInPassAct = std::min (
                node().saTakerPays,
                divideRound (saInSum, xferRate, true));

//输出受输入剩余限制。
            auto outPass = divRound (
                saInPassAct, node().saOfrRate, node().saTakerGets.issue (), true);
            STAmount saOutPassMax    = std::min (saOutPassFunded, outPass);

            STAmount saInPassFeesMax = saInSum - saInPassAct;

//将由下一个节点（）确定。
            STAmount saOutPassAct;

//将由调整后的SAINPASSACT决定。
            STAmount saInPassFees;

            JLOG (j_.trace())
                << "deliverNodeForward:"
                << " nodeIndex_=" << nodeIndex_
                << " saOutFunded=" << saOutFunded
                << " saOutPassFunded=" << saOutPassFunded
                << " node().saOfferFunds=" << node().saOfferFunds
                << " node().saTakerGets=" << node().saTakerGets
                << " saInReq=" << saInReq
                << " saInAct=" << saInAct
                << " saInFees=" << saInFees
                << " saInFunded=" << saInFunded
                << " saInTotal=" << saInTotal
                << " saInSum=" << saInSum
                << " saInPassAct=" << saInPassAct
                << " saOutPassMax=" << saOutPassMax;

//修理工：如果我们不想得到任何东西，我们就取消报价？
            if (!node().saTakerPays || saInSum <= beast::zero)
            {
                JLOG (j_.debug())
                    << "deliverNodeForward: Microscopic offer unfunded.";

//数学报价实际上没有资金支持。
                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance = true;
                continue;
            }

            if (!saInFunded)
            {
//上一次检查应该可以捕捉到这一点。
                JLOG (j_.warn())
                    << "deliverNodeForward: UNREACHABLE REACHED";

//数学报价实际上没有资金支持。
                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance = true;
                continue;
            }

            if (!isXRP(nextNode().account_))
            {
//？-->offer-->帐户
//输入费用：根据所消费产品的所有者而变化。
//输出费用：无，因为xrp或目的地帐户是
//发行人。

                saOutPassAct = saOutPassMax;
                saInPassFees = saInPassFeesMax;

                JLOG (j_.trace())
                    << "deliverNodeForward: ? --> OFFER --> account:"
                    << " offerOwnerAccount_="
                    << node().offerOwnerAccount_
                    << " nextNode().account_="
                    << nextNode().account_
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutFunded=" << saOutFunded;

//输出：借记报价所有者，发送xrp或非xpr到下一个
//帐户。
                resultCode = accountSend(view(),
                    node().offerOwnerAccount_,
                    nextNode().account_,
                    saOutPassAct, viewJ);

                if (resultCode != tesSUCCESS)
                    break;
            }
            else
            {
//？-->报价-->报价
//
//报价是指当前订单簿的输出货币和
//发卡机构匹配下一个订单簿的输入电流和发卡机构。
//
//输出费用：如果发行人有费用，并且没有
//一边。
                STAmount saOutPassFees;

//输出费用随下一个节点提供的所有者的不同而变化。
//因此，立即推送当前报价的输出。
                resultCode = increment().deliverNodeForward (
node().offerOwnerAccount_,  //>电流保持架。
saOutPassMax,             //>可用金额。
saOutPassAct,             //<--交货金额。
saOutPassFees,            //<--收费。
                    saInAct > beast::zero);

                if (resultCode != tesSUCCESS)
                    break;

                if (saOutPassAct == saOutPassMax)
                {
//没有费用和全部产出金额。

                    saInPassFees = saInPassFeesMax;
                }
                else
                {
//输出量的分数。
//输出费用由报价所有人支付，不传递给
//以前的。

                    assert (saOutPassAct < saOutPassMax);
                    auto inPassAct = mulRound (
                        saOutPassAct, node().saOfrRate, saInReq.issue (), true);
                    saInPassAct = std::min (node().saTakerPays, inPassAct);
                    auto inPassFees = multiplyRound (
                        saInPassAct, xferRate, true);
                    saInPassFees    = std::min (saInPassFeesMax, inPassFees);
                }

//进行出站借记。
//发送给发卡行/LIMBO总金额，包括费用（发卡行获得
//费用）。
                auto const& id = isXRP(node().issue_) ?
                        xrpAccount() : node().issue_.account;
                auto outPassTotal = saOutPassAct + saOutPassFees;
                accountSend(view(),
                    node().offerOwnerAccount_,
                    id,
                    outPassTotal,
                    viewJ);

                JLOG (j_.trace())
                    << "deliverNodeForward: ? --> OFFER --> offer:"
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutPassFees=" << saOutPassFees;
            }

            JLOG (j_.trace())
                << "deliverNodeForward: "
                << " nodeIndex_=" << nodeIndex_
                << " node().saTakerGets=" << node().saTakerGets
                << " node().saTakerPays=" << node().saTakerPays
                << " saInPassAct=" << saInPassAct
                << " saInPassFees=" << saInPassFees
                << " saOutPassAct=" << saOutPassAct
                << " saOutFunded=" << saOutFunded;

//资金被花光了。
            node().bFundsDirty = true;

//进行入站贷记。
//
//来自发行人/Limbo的信贷要约所有人（剩余输入转让费用）
//与业主）。不要试图让别人相信自己，它
//是多余的。
            if (isXRP (previousNode().issue_.currency)
                || uInAccountID != node().offerOwnerAccount_)
            {
                auto id = !isXRP(previousNode().issue_.currency) ?
                        uInAccountID : xrpAccount();
                resultCode = accountSend(view(),
                    id,
                    node().offerOwnerAccount_,
                    saInPassAct,
                    viewJ);

                if (resultCode != tesSUCCESS)
                    break;
            }

//调整报价。
//
//费用视为从单独的预算中支付，不列明
//在报价中。
            STAmount saTakerGetsNew  = node().saTakerGets - saOutPassAct;
            STAmount saTakerPaysNew  = node().saTakerPays - saInPassAct;

            if (saTakerPaysNew < beast::zero || saTakerGetsNew < beast::zero)
            {
                JLOG (j_.warn())
                    << "deliverNodeForward: NEGATIVE:"
                    << " saTakerPaysNew=" << saTakerPaysNew
                    << " saTakerGetsNew=" << saTakerGetsNew;

                resultCode = telFAILED_PROCESSING;
                break;
            }

            node().sleOffer->setFieldAmount (sfTakerGets, saTakerGetsNew);
            node().sleOffer->setFieldAmount (sfTakerPays, saTakerPaysNew);

            view().update (node().sleOffer);

            if (saOutPassAct == saOutFunded || saTakerGetsNew == beast::zero)
            {
//提议没有资金支持。

                JLOG (j_.debug())
                    << "deliverNodeForward: unfunded:"
                    << " saOutPassAct=" << saOutPassAct
                    << " saOutFunded=" << saOutFunded;

                pathState_.unfundedOffers().push_back (node().offerIndex_);
                node().bEntryAdvance   = true;
            }
            else
            {
                if (saOutPassAct >= saOutFunded)
                {
                    JLOG (j_.warn())
                        << "deliverNodeForward: TOO MUCH:"
                        << " saOutPassAct=" << saOutPassAct
                        << " saOutFunded=" << saOutFunded;
                }

                assert (saOutPassAct < saOutFunded);
            }

            saInAct += saInPassAct;
            saInFees += saInPassFees;

//调整下一个节点的可用数量（）。
            node().saFwdDeliver = std::min (node().saRevDeliver,
                                        node().saFwdDeliver + saOutPassAct);
        }
    }

    JLOG (j_.trace())
        << "deliverNodeForward<"
        << " nodeIndex_=" << nodeIndex_
        << " saInAct=" << saInAct
        << " saInFees=" << saInFees;

    return resultCode;
}

} //路径
} //涟漪
