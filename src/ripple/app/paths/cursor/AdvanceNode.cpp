
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
#include <ripple/ledger/View.h>

namespace ripple {
namespace path {

TER PathCursor::advanceNode (STAmount const& amount, bool reverse, bool callerHasLiquidity) const
{
    bool const multi = fix1141 (view ().info ().parentCloseTime)
        ? (multiQuality_ || (!callerHasLiquidity && amount == beast::zero))
        : (multiQuality_ || amount == beast::zero);

//如果多质量_u不变，请使用我们现在使用的路径光标。
    if (multi == multiQuality_)
        return advanceNode (reverse);

//否则，将新的路径光标与新的多质量_uuu一起使用。
    PathCursor withMultiQuality {rippleCalc_, pathState_, multi, j_, nodeIndex_};
    return withMultiQuality.advanceNode (reverse);
}

//优化：计算路径增量时，请注意增量是否消耗所有
//流动性。如果所有的流动性都被使用了，未来就不需要重新审视这条道路。
//
TER PathCursor::advanceNode (bool const bReverse) const
{
    TER resultCode = tesSUCCESS;

//接受方是对分类账中报价的积极参与方-实体
//这是在利用订货单上的报价。
    JLOG (j_.trace())
            << "advanceNode: TakerPays:"
            << node().saTakerPays << " TakerGets:" << node().saTakerGets;

    int loopCount = 0;
    auto viewJ = rippleCalc_.logs_.journal ("View");

    do
    {
//vvalco注意：为什么不使用for（）循环？
//vfalc todo循环迭代的限制
//不同质量数量上限
//将为一条路径考虑的级别（工资比率：GET）。
//更改此值会对验证和共识产生影响。
//
        if (++loopCount > NODE_ADVANCE_MAX_LOOPS)
        {
            JLOG (j_.warn()) << "Loop count exceeded";
            return tefEXCEPTION;
        }

        bool bDirectDirDirty = node().directory.initialize (
            { previousNode().issue_, node().issue_},
            view());

        if (auto advance = node().directory.advance (view()))
        {
            bDirectDirDirty = true;
            if (advance == NodeDirectory::NEW_QUALITY)
            {
//我们没有跑过这本订单的末尾，发现
//另一个质量目录。
                JLOG (j_.trace())
                    << "advanceNode: Quality advance: node.directory.current="
                    << node().directory.current;
            }
            else if (bReverse)
            {
                JLOG (j_.trace())
                    << "advanceNode: No more offers.";

                node().offerIndex_ = 0;
                break;
            }
            else
            {
//没有更多优惠。应该做而不是从
//书。
                JLOG (j_.warn())
                    << "advanceNode: Unreachable: "
                    << "Fell off end of order book.";
//为什么？
                return telFAILED_PROCESSING;
            }
        }

        if (bDirectDirDirty)
        {
//自上次迭代以来，我们的质量发生了变化。
//使用目录中的速率。
            node().saOfrRate = amountFromQuality (
                getQuality (node().directory.current));
//正确的比率
            node().uEntry = 0;
            node().bEntryAdvance   = true;

            JLOG (j_.trace())
                << "advanceNode: directory dirty: node.saOfrRate="
                << node().saOfrRate;
        }

        if (!node().bEntryAdvance)
        {
            if (node().bFundsDirty)
            {
//再次调用我们可能只是为了更新结构
//变量。
                node().saTakerPays
                        = node().sleOffer->getFieldAmount (sfTakerPays);
                node().saTakerGets
                        = node().sleOffer->getFieldAmount (sfTakerGets);

//剩下的资金。
                node().saOfferFunds = accountFunds(view(),
                    node().offerOwnerAccount_,
                    node().saTakerGets,
                    fhZERO_IF_FROZEN, viewJ);
                node().bFundsDirty = false;

                JLOG (j_.trace())
                    << "advanceNode: funds dirty: node().saOfrRate="
                    << node().saOfrRate;
            }
            else
            {
                JLOG (j_.trace()) << "advanceNode: as is";
            }
        }
        else if (!dirNext (view(),
            node().directory.current,
            node().directory.ledgerEntry,
            node().uEntry,
            node().offerIndex_, viewJ))
//这是唯一一个提供索引修改的地方。
        {
//在目录中找不到条目。
//做另一个cur目录iff multiquality\u
            if (multiQuality_)
            {
//如果这是
//只有路径。
                JLOG (j_.trace())
                    << "advanceNode: next quality";
node().directory.advanceNeeded  = true;  //处理下一个质量。
            }
            else if (!bReverse)
            {
//我们没有倒着跑干-为什么要倒着跑干
//前进-这应该是不可能的！
//托多（汤姆）：这些警告发生在生产中！他们
//不应该。
                JLOG (j_.warn())
                    << "advanceNode: unreachable: ran out of offers";
                return telFAILED_PROCESSING;
            }
            else
            {
//报盘结束了。
node().bEntryAdvance   = false;        //完成。
node().offerIndex_ = 0;            //不再报告条目。
            }
        }
        else
        {
//得到了一个新的提议。
            node().sleOffer = view().peek (keylet::offer(node().offerIndex_));

            if (!node().sleOffer)
            {
//指向不存在项的目录已损坏。
//这是在生产中发生的。
                JLOG (j_.warn()) <<
                    "Missing offer in directory";
                node().bEntryAdvance = true;
            }
            else
            {
                node().offerOwnerAccount_
                        = node().sleOffer->getAccountID (sfAccount);
                node().saTakerPays
                        = node().sleOffer->getFieldAmount (sfTakerPays);
                node().saTakerGets
                        = node().sleOffer->getFieldAmount (sfTakerGets);

                AccountIssue const accountIssue (
                    node().offerOwnerAccount_, node().issue_);

                JLOG (j_.trace())
                    << "advanceNode: offerOwnerAccount_="
                    << to_string (node().offerOwnerAccount_)
                    << " node.saTakerPays=" << node().saTakerPays
                    << " node.saTakerGets=" << node().saTakerGets
                    << " node.offerIndex_=" << node().offerIndex_;

                if (node().sleOffer->isFieldPresent (sfExpiration) &&
                        (node().sleOffer->getFieldU32 (sfExpiration) <=
                            view().parentCloseTime().time_since_epoch().count()))
                {
//优惠已到期。
                    JLOG (j_.trace())
                        << "advanceNode: expired offer";
                    rippleCalc_.permanentlyUnfundedOffers_.insert(
                        node().offerIndex_);
                    continue;
                }

                if (node().saTakerPays <= beast::zero || node().saTakerGets <= beast::zero)
                {
//报盘金额有误。报价不应该有坏处
//数量。
                    auto const index = node().offerIndex_;
                    if (bReverse)
                    {
//过去的内部错误，报价金额有误。
//这是在生产中发生的。
                        JLOG (j_.warn())
                            << "advanceNode: PAST INTERNAL ERROR"
                            << " REVERSE: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

//将报价标记为始终删除。
                        rippleCalc_.permanentlyUnfundedOffers_.insert (
                            node().offerIndex_);
                    }
                    else if (rippleCalc_.permanentlyUnfundedOffers_.find (index)
                             != rippleCalc_.permanentlyUnfundedOffers_.end ())
                    {
//过去的内部错误，发现报价失败
//这是永久性的不自由。
//跳过它。它将被删除。
                        JLOG (j_.debug())
                            << "advanceNode: PAST INTERNAL ERROR "
                            << " FORWARD CONFIRM: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

                    }
                    else
                    {
//反向应该已经把坏报放在名单上了。
//以前的一个内部错误留下了一个不好的报价。
                        JLOG (j_.warn())
                            << "advanceNode: INTERNAL ERROR"

                            <<" FORWARD NEWLY FOUND: OFFER NON-POSITIVE:"
                            << " node.saTakerPays=" << node().saTakerPays
                            << " node.saTakerGets=" << node().saTakerGets;

//完全不处理，事情都在意外之中
//此交易记录的状态。
                        resultCode = tefEXCEPTION;
                    }

                    continue;
                }

//是否允许从此节点访问源？
//
//这可以在一个
//行，缓存结果会很好。
//
//接下来，我们能为更糟的事情提供资金吗？
//以前跳过的质量？可能需要检查
//质量。
                auto itForward = pathState_.forward().find (accountIssue);
                const bool bFoundForward =
                        itForward != pathState_.forward().end ();

//只允许在第一个节点中使用一次源
//在初始路径扫描中遇到。这防止
//反向时相同余额的冲突使用vs
//向前地。
                if (bFoundForward &&
                    itForward->second != nodeIndex_ &&
                    node().offerOwnerAccount_ != node().issue_.account)
                {
//暂时没有资金。另一个节点使用此源，
//在此报价中忽略。
                    JLOG (j_.trace())
                        << "advanceNode: temporarily unfunded offer"
                        << " (forward)";
                    continue;
                }

//这太严格了。对过去的贡献。我们应该
//仅统计实际使用的源。
                auto itReverse = pathState_.reverse().find (accountIssue);
                bool bFoundReverse = itReverse != pathState_.reverse().end ();

//对于此质量增量，只允许使用源
//从单个节点，在遇到的第一个节点中
//反向申请报价。
                if (bFoundReverse &&
                    itReverse->second != nodeIndex_ &&
                    node().offerOwnerAccount_ != node().issue_.account)
                {
//暂时没有资金。另一个节点使用此源，
//在此报价中忽略。
                    JLOG (j_.trace())
                        << "advanceNode: temporarily unfunded offer"
                        <<" (reverse)";
                    continue;
                }

//确定过去是否使用过。
//我们只需要知道它是否需要标记为无资金。
                auto itPast = rippleCalc_.mumSource_.find (accountIssue);
                bool bFoundPast = (itPast != rippleCalc_.mumSource_.end ());

//只允许当前节点使用源。

                node().saOfferFunds = accountFunds(view(),
                    node().offerOwnerAccount_,
                    node().saTakerGets,
                    fhZERO_IF_FROZEN, viewJ);
//持有的资金。

                if (node().saOfferFunds <= beast::zero)
                {
//报价没有资金。
                    JLOG (j_.trace())
                        << "advanceNode: unfunded offer";

                    if (bReverse && !bFoundReverse && !bFoundPast)
                    {
//以前从来没有提到过，显然只是：发现没有资金。
//也就是说，即使这个提议由于填充或杀戮而失败
//还是删除。
//将报价标记为始终删除。
                        rippleCalc_.permanentlyUnfundedOffers_.insert (node().offerIndex_);
                    }
                    else
                    {
//向前移动，不需要再次插入
//或者，已经找到了。
                    }

//YYY可以验证报价是否为未兑现的正确位置。
                    continue;
                }

if (bReverse            //需要记住反向提及。
&& !bFoundPast      //在之前的通行证中没有提到。
&& !bFoundReverse)  //新通过。
                {
//考虑当前路径状态提到的源。
                    JLOG (j_.trace())
                        << "advanceNode: remember="
                        <<  node().offerOwnerAccount_
                        << "/"
                        << node().issue_;

                    pathState_.insertReverse (accountIssue, nodeIndex_);
                }

                node().bFundsDirty = false;
                node().bEntryAdvance = false;
            }
        }
    }
    while (resultCode == tesSUCCESS &&
           (node().bEntryAdvance || node().directory.advanceNeeded));

    if (resultCode == tesSUCCESS)
    {
        JLOG (j_.trace())
            << "advanceNode: node.offerIndex_=" << node().offerIndex_;
    }
    else
    {
        JLOG (j_.debug())
            << "advanceNode: resultCode=" << transToken (resultCode);
    }

    return resultCode;
}

}  //路径
}  //涟漪
