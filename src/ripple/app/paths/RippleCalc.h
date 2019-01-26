
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

#ifndef RIPPLE_APP_PATHS_RIPPLECALC_H_INCLUDED
#define RIPPLE_APP_PATHS_RIPPLECALC_H_INCLUDED

#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/app/paths/PathState.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/TER.h>

#include <boost/container/flat_set.hpp>

namespace ripple {
class Config;
namespace path {

namespace detail {
struct FlowDebugInfo;
}

/*RippleCalc计算付款路径的质量。

    质量是产生给定输出所需的输入量，
    指定的路径-另一个名称是汇率。
**/

class RippleCalc
{
public:
    struct Input
    {
        explicit Input() = default;

        bool partialPaymentAllowed = false;
        bool defaultPathsAllowed = true;
        bool limitQuality = false;
        bool isLedgerOpen = true;
    };
    struct Output
    {
        explicit Output() = default;

//计算的输入量。
        STAmount actualAmountIn;

//计算的输出量。
        STAmount actualAmountOut;

//已过期或未提供资金的报价集合。付款时
//成功、无资金和过期的报价将被删除。付款时
//失败，它们不会被删除。这个向量包含了
//可能已被删除，但不是因为付款失败。它是
//对于报价交叉很有用，这会删除报价。
        boost::container::flat_set<uint256> removableOffers;
    private:
        TER calculationResult_ = temUNKNOWN;

    public:
        TER result () const
        {
            return calculationResult_;
        }
        void setResult (TER const value)
        {
            calculationResult_ = value;
        }

    };

    static
    Output
    rippleCalculate(
        PaymentSandbox& view,

//使用此分类账分录集计算路径。实际上取决于呼叫方
//应用于分类帐。

//发行人：
//xrp:xrpaccount（）。
//非xrp:usrcaccounted（针对任何发行人）或其他账户
//信任节点。
STAmount const& saMaxAmountReq,             //-->-1=无限制。

//发行人：
//xrp:xrpaccount（）。
//非xrp:udstaccounted（针对任何发行人）或其他账户
//信任节点。
        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,

//一组包含在事务中的路径，我们将
//探索流动性。
        STPathSet const& spsPaths,
        Logs& l,
        Input const* const pInputs = nullptr);

//我们当前正在处理的视图
    PaymentSandbox& view;

//如果事务不能满足某些约束，仍然需要删除
//以确定的顺序（因此是订购的容器）提供无资金的报价。
//
//被发现没有资金的提议。
    boost::container::flat_set<uint256> permanentlyUnfundedOffers_;

//第一次反向工作提到了资金来源。源可能
//只能在那里使用。

//货币、发行人到节点索引的映射。
    AccountIssueToNodeIndex mumSource_;
    beast::Journal j_;
    Logs& logs_;

private:
    RippleCalc (
        PaymentSandbox& view_,
STAmount const& saMaxAmountReq,             //-->-1=无限制。
        STAmount const& saDstAmountReq,

        AccountID const& uDstAccountID,
        AccountID const& uSrcAccountID,
        STPathSet const& spsPaths,
        Logs& l)
            : view (view_),
              j_ (l.journal ("RippleCalc")),
              logs_ (l),
              saDstAmountReq_(saDstAmountReq),
              saMaxAmountReq_(saMaxAmountReq),
              uDstAccountID_(uDstAccountID),
              uSrcAccountID_(uSrcAccountID),
              spsPaths_(spsPaths)
    {
    }

    /*通过这些路径集计算流动性。*/
    TER rippleCalculate (detail::FlowDebugInfo* flowDebugInfo=nullptr);

    /*添加单个路径状态。成功时返回true*/
    bool addPathState(STPath const&, TER&);

    STAmount const& saDstAmountReq_;
    STAmount const& saMaxAmountReq_;
    AccountID const& uDstAccountID_;
    AccountID const& uSrcAccountID_;
    STPathSet const& spsPaths_;

//计算的输入量。
    STAmount actualAmountIn_;

//计算的输出量。
    STAmount actualAmountOut_;

//扩展路径，其中包含所有实际节点。
//路径以源帐户开始，以目标帐户结束
//浏览其他帐户或订购书籍。
    PathState::List pathStateList_;

    Input inputFlags;
};

} //路径
} //涟漪

#endif
