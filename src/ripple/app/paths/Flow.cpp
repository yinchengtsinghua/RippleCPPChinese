
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
#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/app/paths/impl/StrandFlow.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace ripple {

template<class FlowResult>
static
auto finishFlow (PaymentSandbox& sb,
    Issue const& srcIssue, Issue const& dstIssue,
    FlowResult&& f)
{
    path::RippleCalc::Output result;
    if (f.ter == tesSUCCESS)
        f.sandbox->apply (sb);
    else
        result.removableOffers = std::move (f.removableOffers);

    result.setResult (f.ter);
    result.actualAmountIn = toSTAmount (f.in, srcIssue);
    result.actualAmountOut = toSTAmount (f.out, dstIssue);

    return result;
};

path::RippleCalc::Output
flow (
    PaymentSandbox& sb,
    STAmount const& deliver,
    AccountID const& src,
    AccountID const& dst,
    STPathSet const& paths,
    bool defaultPaths,
    bool partialPayment,
    bool ownerPaysTransferFee,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMax,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo)
{
    Issue const srcIssue = [&] {
        if (sendMax)
            return sendMax->issue ();
        if (!isXRP (deliver.issue ().currency))
            return Issue (deliver.issue ().currency, src);
        return xrpIssue ();
    }();

    Issue const dstIssue = deliver.issue ();

    boost::optional<Issue> sendMaxIssue;
    if (sendMax)
        sendMaxIssue = sendMax->issue ();

//将路径转换为一组链。每一股都是集合
//在本次付款中可能使用的科目->科目步骤和帐簿步骤。
    auto sr = toStrands (sb, src, dst, dstIssue, limitQuality, sendMaxIssue,
        paths, defaultPaths, ownerPaysTransferFee, offerCrossing, j);

    if (sr.first != tesSUCCESS)
    {
        path::RippleCalc::Output result;
        result.setResult (sr.first);
        return result;
    }

    auto& strands = sr.second;

    if (j.trace())
    {
        j.trace() << "\nsrc: " << src << "\ndst: " << dst
            << "\nsrcIssue: " << srcIssue << "\ndstIssue: " << dstIssue;
        j.trace() << "\nNumStrands: " << strands.size ();
        for (auto const& curStrand : strands)
        {
            j.trace() << "NumSteps: " << curStrand.size ();
            for (auto const& step : curStrand)
            {
                j.trace() << '\n' << *step << '\n';
            }
        }
    }

    const bool srcIsXRP = isXRP (srcIssue.currency);
    const bool dstIsXRP = isXRP (dstIssue.currency);

    auto const asDeliver = toAmountSpec (deliver);

//SRC帐户可以发送XRP或IOU。DST账户可能收到
//xrp或iou。因为xrp和iou金额用不同的
//类型，使用模板告诉“流”有关金额类型的信息。
    if (srcIsXRP && dstIsXRP)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<XRPAmount, XRPAmount> (
                sb, strands, asDeliver.xrp, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    if (srcIsXRP && !dstIsXRP)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<XRPAmount, IOUAmount> (
                sb, strands, asDeliver.iou, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    if (!srcIsXRP && dstIsXRP)
    {
        return finishFlow (sb, srcIssue, dstIssue,
            flow<IOUAmount, XRPAmount> (
                sb, strands, asDeliver.xrp, partialPayment, offerCrossing,
                limitQuality, sendMax, j, flowDebugInfo));
    }

    assert (!srcIsXRP && !dstIsXRP);
    return finishFlow (sb, srcIssue, dstIssue,
        flow<IOUAmount, IOUAmount> (
            sb, strands, asDeliver.iou, partialPayment, offerCrossing,
            limitQuality, sendMax, j, flowDebugInfo));

}

} //涟漪
