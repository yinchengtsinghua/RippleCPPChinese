
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

#ifndef RIPPLE_APP_PATHS_CURSOR_PATHCURSOR_H_INCLUDED
#define RIPPLE_APP_PATHS_CURSOR_PATHCURSOR_H_INCLUDED

#include <ripple/app/paths/RippleCalc.h>

namespace ripple {
namespace path {

//下一节包含计算路径流动性的方法，
//要么向后，要么向前。
//
//我们需要做两次这些计算-一次向后计算出
//沿着路径的最大可能寿命，然后向前计算
//我们实际选择的路径的实际流动性。
//
//其中一些例程使用递归来循环路径中的所有节点。
//TODO（汤姆）：用循环替换这个递归。

class PathCursor
{
public:
    PathCursor(
        RippleCalc& rippleCalc,
        PathState& pathState,
        bool multiQuality,
        beast::Journal j,
        NodeIndex nodeIndex = 0)
            : rippleCalc_(rippleCalc),
              pathState_(pathState),
              multiQuality_(multiQuality),
              nodeIndex_(restrict(nodeIndex)),
              j_ (j)
    {
    }

    void nextIncrement() const;

private:
    PathCursor(PathCursor const&) = default;

    PathCursor increment(int delta = 1) const
    {
        return {rippleCalc_, pathState_, multiQuality_, j_, nodeIndex_ + delta};
    }

    TER liquidity() const;
    TER reverseLiquidity () const;
    TER forwardLiquidity () const;

    TER forwardLiquidityForAccount () const;
    TER reverseLiquidityForOffer () const;
    TER forwardLiquidityForOffer () const;
    TER reverseLiquidityForAccount () const;

//把钱从帐户里寄出去。
    /*AdvanceNode通过订单中的报价进行预付。
        如果需要，请提前到下一个资助的报价。
     -自动提前到第一个报价。
     -->BentryAdvance:true，前进到下一个条目。假，重新计算。
      <--Uofferindex:0=列表末尾。
    **/

    TER advanceNode (bool reverse) const;
    TER advanceNode (STAmount const& amount, bool reverse, bool callerHasLiquidity) const;

//在计算时，从订单簿交付
    TER deliverNodeReverse (
        AccountID const& uOutAccountID,
        STAmount const& saOutReq,
        STAmount& saOutAct) const;

//在计算时，从订单簿交付
    TER deliverNodeReverseImpl (
        AccountID const& uOutAccountID,
        STAmount const& saOutReq,
        STAmount& saOutAct,
        bool callerHasLiquidity) const;

    TER deliverNodeForward (
        AccountID const& uInAccountID,
        STAmount const& saInReq,
        STAmount& saInAct,
        STAmount& saInFees,
        bool callerHasLiquidity) const;

//vvalco todo将此重命名为view（）。
    PaymentSandbox&
    view() const
    {
        return pathState_.view();
    }

    NodeIndex nodeSize() const
    {
        return pathState_.nodes().size();
    }

    NodeIndex restrict(NodeIndex i) const
    {
        return std::min (i, nodeSize() - 1);
    }

    Node& node(NodeIndex i) const
    {
        return pathState_.nodes()[i];
    }

    Node& node() const
    {
        return node (nodeIndex_);
    }

    Node& previousNode() const
    {
        return node (restrict (nodeIndex_ - 1));
    }

    Node& nextNode() const
    {
        return node (restrict (nodeIndex_ + 1));
    }

    RippleCalc& rippleCalc_;
    PathState& pathState_;
    bool multiQuality_;
    NodeIndex nodeIndex_;
    beast::Journal j_;
};

} //路径
} //涟漪

#endif
