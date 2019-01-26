
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

//在连续提供节点列表的最右侧节点，给定数量
//请求交付，向左侧节点推送请求的数量
//对于正确的节点，我们可以计算从源代码传递的内容。
//
//在优惠节点之间，收取的费用可能有所不同。因此，工艺一
//一次提供入站优惠。将入站报价的要求传播到
//上一个节点。上一个节点调整金额输出和金额
//花在费用上。继续处理，直到满足请求
//因为利率不会超过初始利率。

//在计算时，从订单簿交付
TER PathCursor::deliverNodeReverseImpl (
AccountID const& uOutAccountID, //-->输出所有者的帐户。
STAmount const& saOutReq,       //>申请的资金
//以增量形式交付。
STAmount& saOutAct,             //<--实际交付的资金
//增量
    bool callerHasLiquidity
                                        ) const
{
    TER resultCode   = tesSUCCESS;

//前一个节点必须传递的内容的累积。
//可能的优化：请注意，理想情况下，这会在每个增量上归零。
//只有在第一个增量上，那么它可能是向前传球的限制。
    saOutAct.clear (saOutReq);

    JLOG (j_.trace())
        << "deliverNodeReverse>"
        << " saOutAct=" << saOutAct
        << " saOutReq=" << saOutReq
        << " saPrvDlvReq=" << previousNode().saRevDeliver;

    assert (saOutReq != beast::zero);

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

//虽然我们没有按要求交付：
    while (saOutAct < saOutReq)
    {
        if (++loopCount >
            (multiQuality_ ?
                CALC_NODE_DELIVER_MAX_LOOPS_MQ :
                CALC_NODE_DELIVER_MAX_LOOPS))
        {
            JLOG (j_.warn()) << "loop count exceeded";
            return telFAILED_PROCESSING;
        }

        resultCode = advanceNode (saOutAct, true, callerHasLiquidity);
//如果需要，请提前到下一个资助的报价。

        if (resultCode != tesSUCCESS || !node().offerIndex_)
//错误或报价不足。
            break;

        auto const xferRate = effectiveRate (
            node().issue_,
            uOutAccountID,
            node().offerOwnerAccount_,
            node().transferRate_);

        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " offerOwnerAccount_=" << node().offerOwnerAccount_
            << " uOutAccountID=" << uOutAccountID
            << " node().issue_.account=" << node().issue_.account
            << " xferRate=" << xferRate;

//仅在不处于多质量模式时使用率
        if (!multiQuality_)
        {
            if (!node().rateMax)
            {
//设置初始速率。
                JLOG (j_.trace())
                    << "Set initial rate";

                node().rateMax = xferRate;
            }
            else if (xferRate > node().rateMax)
            {
//报价超过了初始价格。
                JLOG (j_.trace())
                    << "Offer exceeds initial rate: " << *node().rateMax;

break;  //完成。不要费心寻找更小的转移率。
            }
            else if (xferRate < node().rateMax)
            {
//降低率。其他优惠仅限于
//如果他们
//至少是这样的好。
//
//在这一点上，整体比率正在下降，
//虽然总的比率不是xferate，但是
//加任何有价格的东西都是错的
//高于铁酸盐。
//
//如果电流
//要约来自发行人和上一个
//要约不是。

                JLOG (j_.trace())
                    << "Reducing rate: " << *node().rateMax;

                node().rateMax = xferRate;
            }
        }

//支付给接受者的金额。
        STAmount saOutPassReq = std::min (
            std::min (node().saOfferFunds, node().saTakerGets),
            saOutReq - saOutAct);

//最大输出-假设没有输出费用。
        STAmount saOutPassAct = saOutPassReq;

//向报价所有人收取的金额。
//
//费用由发行人承担。费用由报价人支付，但未通过
//作为接受者的代价。
//
//四舍五入：更喜欢流动性而不是微观费用。
        STAmount saOutPlusFees   = multiplyRound (
            saOutPassAct, xferRate, false);


//提供费用。

        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " saOutReq=" << saOutReq
            << " saOutAct=" << saOutAct
            << " node().saTakerGets=" << node().saTakerGets
            << " saOutPassAct=" << saOutPassAct
            << " saOutPlusFees=" << saOutPlusFees
            << " node().saOfferFunds=" << node().saOfferFunds;

        if (saOutPlusFees > node().saOfferFunds)
        {
//报价所有者不能支付所有费用，请根据
//node（）.saofferFunds。
            saOutPlusFees   = node().saOfferFunds;

//四舍五入：更喜欢流动性而不是微观费用。但是，
//按请求限制。
            auto fee = divideRound (saOutPlusFees, xferRate, true);
            saOutPassAct = std::min (saOutPassReq, fee);

            JLOG (j_.trace())
                << "deliverNodeReverse: Total exceeds fees:"
                << " saOutPassAct=" << saOutPassAct
                << " saOutPlusFees=" << saOutPlusFees
                << " node().saOfferFunds=" << node().saOfferFunds;
        }

//计算需要覆盖实际输出的输入部分。
        auto outputFee = mulRound (
            saOutPassAct, node().saOfrRate, node().saTakerPays.issue (), true);
        if (*stAmountCalcSwitchover == false && ! outputFee)
        {
            JLOG (j_.fatal())
                << "underflow computing outputFee "
                << "saOutPassAct: " << saOutPassAct
                << " saOfrRate: " << node ().saOfrRate;
            return telFAILED_PROCESSING;
        }
        STAmount saInPassReq = std::min (node().saTakerPays, outputFee);
        STAmount saInPassAct;

        JLOG (j_.trace())
            << "deliverNodeReverse:"
            << " outputFee=" << outputFee
            << " saInPassReq=" << saInPassReq
            << " node().saOfrRate=" << node().saOfrRate
            << " saOutPassAct=" << saOutPassAct
            << " saOutPlusFees=" << saOutPlusFees;

if (!saInPassReq) //修理工：这是假的
        {
//舍入后什么都不想要。
            JLOG (j_.debug())
                << "deliverNodeReverse: micro offer is unfunded.";

            node().bEntryAdvance   = true;
            continue;
        }
//找出按当前汇率实际可用的输入金额。
        else if (!isXRP(previousNode().account_))
        {
//账户——>报价——？
//由于节点扩展，上一个节点保证是颁发者。
//
//前一个是发行人和接收人是要约，因此没有费用或
//质量。
//
//前一个是发行人，资金不受限制。
//
//优惠所有者通过优惠获得借据，因此信用额度限制
//被忽略。由于限制被忽略，不需要调整
//上一个帐户的余额。

            saInPassAct = saInPassReq;

            JLOG (j_.trace())
                << "deliverNodeReverse: account --> OFFER --> ? :"
                << " saInPassAct=" << saInPassAct;
        }
        else
        {
//报价——>报价——？
//在上一个提供节点中计算可以进入的数量。

//托多（汤姆）：在这里修复讨厌的递归！
            resultCode = increment(-1).deliverNodeReverseImpl(
                node().offerOwnerAccount_,
                saInPassReq,
                saInPassAct,
                saOutAct > beast::zero);

            if (fix1141(view().info().parentCloseTime))
            {
//这次递归调用是空的，但我们有流动性
//从以前的呼叫
                if (resultCode == tecPATH_DRY && saOutAct > beast::zero)
                {
                    resultCode = tesSUCCESS;
                    break;
                }
            }

            JLOG (j_.trace())
                << "deliverNodeReverse: offer --> OFFER --> ? :"
                << " saInPassAct=" << saInPassAct;
        }

        if (resultCode != tesSUCCESS)
            break;

        if (saInPassAct < saInPassReq)
        {
//调整输出以符合有限输入。
            auto outputRequirements = divRound (saInPassAct, node ().saOfrRate,
                node ().saTakerGets.issue (), true);
            saOutPassAct = std::min (saOutPassReq, outputRequirements);
            auto outputFees = multiplyRound (saOutPassAct, xferRate, true);
            saOutPlusFees   = std::min (node().saOfferFunds, outputFees);

            JLOG (j_.trace())
                << "deliverNodeReverse: adjusted:"
                << " saOutPassAct=" << saOutPassAct
                << " saOutPlusFees=" << saOutPlusFees;
        }
        else
        {
//托多（汤姆）：这里有更多的记录。
            assert (saInPassAct == saInPassReq);
        }

//资金被花光了。
        node().bFundsDirty = true;

//希望在计算反向时扣除输出以限制计算。
//实际上不需要发送。
//
//发送可能很复杂：以前的报价是否还没有融资？
//参观。然而，这些扣除和调整是有保留的。
//
//进行实际转账时必须重置余额。
        resultCode   = accountSend(view(),
            node().offerOwnerAccount_, node().issue_.account, saOutPassAct, viewJ);

        if (resultCode != tesSUCCESS)
            break;

//调整要约
        STAmount saTakerGetsNew  = node().saTakerGets - saOutPassAct;
        STAmount saTakerPaysNew  = node().saTakerPays - saInPassAct;

        if (saTakerPaysNew < beast::zero || saTakerGetsNew < beast::zero)
        {
            JLOG (j_.warn())
                << "deliverNodeReverse: NEGATIVE:"
                << " node().saTakerPaysNew=" << saTakerPaysNew
                << " node().saTakerGetsNew=" << saTakerGetsNew;

            resultCode = telFAILED_PROCESSING;
            break;
        }

        node().sleOffer->setFieldAmount (sfTakerGets, saTakerGetsNew);
        node().sleOffer->setFieldAmount (sfTakerPays, saTakerPaysNew);

        view().update (node().sleOffer);

        if (saOutPassAct == node().saTakerGets)
        {
//提议没有资金支持。
            JLOG (j_.debug())
                << "deliverNodeReverse: offer became unfunded.";

            node().bEntryAdvance   = true;
//我们什么时候不想提前？
        }
        else
        {
            assert (saOutPassAct < node().saTakerGets);
        }

        saOutAct += saOutPassAct;
//累积从上一个节点传递的内容。
        previousNode().saRevDeliver += saInPassAct;
    }

    if (saOutAct > saOutReq)
    {
        JLOG (j_.warn())
            << "deliverNodeReverse: TOO MUCH:"
            << " saOutAct=" << saOutAct
            << " saOutReq=" << saOutReq;
    }

    assert(saOutAct <= saOutReq);

    if (resultCode == tesSUCCESS && !saOutAct)
        resultCode = tecPATH_DRY;
//无法满足要求，请考虑路径干燥。
//设计不变：如果没有实际交付，返回TecPath_Dry。

    JLOG (j_.trace())
        << "deliverNodeReverse<"
        << " saOutAct=" << saOutAct
        << " saOutReq=" << saOutReq
        << " saPrvDlvReq=" << previousNode().saRevDeliver;

    return resultCode;
}

TER PathCursor::deliverNodeReverse (
AccountID const& uOutAccountID, //-->输出所有者的帐户。
STAmount const& saOutReq,       //>申请的资金
//以增量形式交付。
STAmount& saOutAct              //<--实际交付的资金
//增量
                                    ) const
{
    for (int i = nodeIndex_; i >= 0 && !node (i).isAccount(); --i)
        node (i).directory.restart (multiQuality_);

    /*urn delivernodereverseimpl（uoutaccounted、saoutreq、saoutact、/*callerhasliquidity*/false）；
}

}//PATH
} /纹波
