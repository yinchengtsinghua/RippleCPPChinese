
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

#ifndef RIPPLE_APP_PATHS_IMPL_STRANDFLOW_H_INCLUDED
#define RIPPLE_APP_PATHS_IMPL_STRANDFLOW_H_INCLUDED

#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/Flow.h>
#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/app/paths/impl/FlowDebugInfo.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <sstream>

namespace ripple {

/*流（）执行单个流的结果。*/
template<class TInAmt, class TOutAmt>
struct StrandResult
{
TER ter = temUNKNOWN;                         ///<结果代码
TInAmt in = beast::zero;                      ///<货币金额
TOutAmt out = beast::zero;                    ///<货币金额
boost::optional<PaymentSandbox> sandbox;      ///<结果沙盒状态
boost::container::flat_set<uint256> ofrsToRm; ///<提供删除
//如果没有更多的流动性或消耗了太多的报价，则Strand可能处于非活动状态。
bool inactive = false; ///<Strand不应被视为流动性的进一步来源（干）

    /*链结果构造函数*/
    StrandResult () = default;

    StrandResult (TInAmt const& in_,
        TOutAmt const& out_,
        PaymentSandbox&& sandbox_,
        boost::container::flat_set<uint256> ofrsToRm_,
        bool inactive_)
        : ter (tesSUCCESS)
        , in (in_)
        , out (out_)
        , sandbox (std::move (sandbox_))
        , ofrsToRm (std::move (ofrsToRm_))
        , inactive(inactive_)
    {
    }

    StrandResult (TER ter_, boost::container::flat_set<uint256> ofrsToRm_)
        : ter (ter_), ofrsToRm (std::move (ofrsToRm_))
    {
    }
};

/*
   从流中请求“out”金额

   @param baseview信任线和余额
   @param strend要浏览和提供书籍使用的帐户步骤
   @param maxin允许的最大输入量
   @param out从流请求的输出量
   @param j日志将日志消息写入
   @返回钢绞线的实际进出量、错误、提议移除，
           和付款沙盒
 **/

template<class TInAmt, class TOutAmt>
StrandResult <TInAmt, TOutAmt>
flow (
    PaymentSandbox const& baseView,
    Strand const& strand,
    boost::optional<TInAmt> const& maxIn,
    TOutAmt const& out,
    beast::Journal j)
{
    using Result = StrandResult<TInAmt, TOutAmt>;
    if (strand.empty ())
    {
        JLOG (j.warn()) << "Empty strand passed to Liquidity";
        return {};
    }

    boost::container::flat_set<uint256> ofrsToRm;

    if (isDirectXrpToXrp<TInAmt, TOutAmt> (strand))
    {
//当前的实现没有返回xrp->xrp传输的\u行。
//保持这种行为
        return {tecNO_LINE, std::move (ofrsToRm)};
    }

    try
    {
        std::size_t const s = strand.size ();

        std::size_t limitingStep = strand.size ();
        boost::optional<PaymentSandbox> sb (&baseView);
//“所有资金”视图确定报价是否变为无资金或
//发现资金不足
//这些是执行流之前的帐户余额
        boost::optional<PaymentSandbox> afView (&baseView);
        EitherAmount limitStepOut;
        {
            EitherAmount stepOut (out);
            for (auto i = s; i--;)
            {
                auto r = strand[i]->rev (*sb, *afView, ofrsToRm, stepOut);
                if (strand[i]->isZero (r.second))
                {
                    JLOG (j.trace()) << "Strand found dry in rev";
                    return {tecPATH_DRY, std::move (ofrsToRm)};
                }

                if (i == 0 && maxIn && *maxIn < get<TInAmt> (r.first))
                {
//限制-超过最大值
//放弃以前的结果
                    sb.emplace (&baseView);
                    limitingStep = i;

//重新执行限制步骤
                    r = strand[i]->fwd (
                        *sb, *afView, ofrsToRm, EitherAmount (*maxIn));
                    limitStepOut = r.second;

                    if (strand[i]->isZero (r.second) ||
                        get<TInAmt> (r.first) != *maxIn)
                    {
//出了大问题
//扔掉沙盒只能增加流动性
//然而，限制仍然是有限的。
                        JLOG (j.fatal()) << "Re-executed limiting step failed";
                        assert (0);
                        return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                    }
                }
                else if (!strand[i]->equalOut (r.second, stepOut))
                {
//限制
//放弃以前的结果
                    sb.emplace (&baseView);
                    afView.emplace (&baseView);
                    limitingStep = i;

//重新执行限制步骤
                    stepOut = r.second;
                    r = strand[i]->rev (*sb, *afView, ofrsToRm, stepOut);
                    limitStepOut = r.second;

                    if (strand[i]->isZero (r.second) ||
                        !strand[i]->equalOut (r.second, stepOut))
                    {
//出了大问题
//扔掉沙盒只能增加流动性
//然而，限制仍然是有限的。
                        JLOG (j.fatal()) << "Re-executed limiting step failed";
                        assert (0);
                        return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                    }
                }

//上一个节点需要生成此节点要使用的内容
                stepOut = r.first;
            }
        }

        {
            EitherAmount stepIn (limitStepOut);
            for (auto i = limitingStep + 1; i < s; ++i)
            {
                auto const r = strand[i]->fwd (*sb, *afView, ofrsToRm, stepIn);
                if (strand[i]->isZero (r.second) ||
                    !strand[i]->equalIn (r.first, stepIn))
                {
//应该已经找到了限制，所以向前执行一个链
//从限制步骤中不应找到新的限制
                    JLOG (j.fatal()) << "Re-executed forward pass failed";
                    assert (0);
                    return {telFAILED_PROCESSING, std::move (ofrsToRm)};
                }
                stepIn = r.second;
            }
        }

        auto const strandIn = *strand.front ()->cachedIn ();
        auto const strandOut = *strand.back ()->cachedOut ();

#ifndef NDEBUG
        {
//检查钢绞线是否按预期执行。
//重新执行流将更改缓存的值
            PaymentSandbox checkSB (&baseView);
            PaymentSandbox checkAfView (&baseView);
            EitherAmount stepIn (*strand[0]->cachedIn ());
            for (auto i = 0; i < s; ++i)
            {
                bool valid;
                std::tie (valid, stepIn) =
                    strand[i]->validFwd (checkSB, checkAfView, stepIn);
                if (!valid)
                {
                    JLOG (j.error())
                        << "Strand re-execute check failed. Step: " << i;
                    break;
                }
            }
        }
#endif

        bool const inactive = std::any_of(
            strand.begin(),
            strand.end(),
            [](std::unique_ptr<Step> const& step) { return step->inactive(); });

        return Result(
            get<TInAmt>(strandIn),
            get<TOutAmt>(strandOut),
            std::move(*sb),
            std::move(ofrsToRm),
            inactive);
    }
    catch (FlowException const& e)
    {
        return {e.ter, std::move (ofrsToRm)};
    }
}

///@cond内部
template<class TInAmt, class TOutAmt>
struct FlowResult
{
    TInAmt in = beast::zero;
    TOutAmt out = beast::zero;
    boost::optional<PaymentSandbox> sandbox;
    boost::container::flat_set<uint256> removableOffers;
    TER ter = temUNKNOWN;

    FlowResult () = default;

    FlowResult (TInAmt const& in_,
        TOutAmt const& out_,
        PaymentSandbox&& sandbox_,
        boost::container::flat_set<uint256> ofrsToRm)
        : in (in_)
        , out (out_)
        , sandbox (std::move (sandbox_))
        , removableOffers(std::move (ofrsToRm))
        , ter (tesSUCCESS)
    {
    }

    FlowResult (TER ter_, boost::container::flat_set<uint256> ofrsToRm)
        : removableOffers(std::move (ofrsToRm))
        , ter (ter_)
    {
    }

    FlowResult (TER ter_,
        TInAmt const& in_,
        TOutAmt const& out_,
        boost::container::flat_set<uint256> ofrsToRm)
        : in (in_)
        , out (out_)
        , removableOffers (std::move (ofrsToRm))
        , ter (ter_)
    {
    }
};
///EndCOND

///@cond内部
/*跟踪非干股

   Flow将搜索非干股（存储在“cur”中）以获得最佳
   可用流动性如果流动性不使用链的所有流动性，那么
   流添加到“下一个”。“下一个”中的线在
   使用当前最佳流动性。
 **/

class ActiveStrands
{
private:
//流动性探索股
    std::vector<Strand const*> cur_;
//可在下一次迭代中探索流动性的链
    std::vector<Strand const*> next_;

public:
    ActiveStrands (std::vector<Strand> const& strands)
    {
        cur_.reserve (strands.size ());
        next_.reserve (strands.size ());
        for (auto& strand : strands)
            next_.push_back (&strand);
    }

//在流动性搜索中开始新的迭代
//将当前股线设置为“下一个”中的股线
    void
    activateNext ()
    {
//交换，不要移动，所以我们将保留在下一个
        cur_.clear ();
        std::swap (cur_, next_);
    }

    void
    push (Strand const* s)
    {
        next_.push_back (s);
    }

    auto begin ()
    {
        return cur_.begin ();
    }

    auto end ()
    {
        return cur_.end ();
    }

    auto begin () const
    {
        return cur_.begin ();
    }

    auto end () const
    {
        return cur_.end ();
    }

    auto size () const
    {
        return cur_.size ();
    }

    void
    removeIndex(std::size_t i)
    {
        if (i >= next_.size())
            return;
        next_.erase(next_.begin() + i);
    }
};
///EndCOND

///@cond内部
boost::optional<Quality>
qualityUpperBound(ReadView const& v, Strand const& strand)
{
    Quality q{STAmount::uRateOne};
    bool redeems = false;
    for(auto const& step : strand)
    {
        if (auto const stepQ = step->qualityUpperBound(v, redeems))
            q = composed_quality(q, *stepQ);
        else
            return boost::none;
    }
    return q;
};
///EndCOND

/*
   从股集合中请求“out”金额

   尝试使用股中的流动性按顺序完成付款
   从最便宜到最贵

   @param baseview信任线和余额
   @param strends每个strends都包含要遍历的帐户步骤
                  并提供书供使用
   @param outreq从流请求的输出量
   @param partialpayment如果为true，则允许少于全额付款
   @param offerrocking如果真的offerrocking，不处理标准付款
   @param limitquality如果存在，则为所取任何钢绞线的最低质量。
   @param sendmaxst如果存在，则为要发送的最大stamount
   @param j journal将日志消息写入
   @param flow debug info如果指针非空，请在此处写入流调试信息
   @返回股线、错误和付款沙盒中的实际进出金额
**/

template <class TInAmt, class TOutAmt>
FlowResult<TInAmt, TOutAmt>
flow (PaymentSandbox const& baseView,
    std::vector<Strand> const& strands,
    TOutAmt const& outReq,
    bool partialPayment,
    bool offerCrossing,
    boost::optional<Quality> const& limitQuality,
    boost::optional<STAmount> const& sendMaxST,
    beast::Journal j,
    path::detail::FlowDebugInfo* flowDebugInfo=nullptr)
{
//用于跟踪提供最佳质量的钢绞线（输出/输入比）
    struct BestStrand
    {
        TInAmt in;
        TOutAmt out;
        PaymentSandbox sb;
        Strand const& strand;
        Quality quality;

        BestStrand (TInAmt const& in_,
            TOutAmt const& out_,
            PaymentSandbox&& sb_,
            Strand const& strand_,
            Quality const& quality_)
            : in (in_)
            , out (out_)
            , sb (std::move (sb_))
            , strand (strand_)
            , quality (quality_)
        {
        }
    };

    std::size_t const maxTries = 1000;
    std::size_t curTry = 0;

//gcc中有一个错误，错误地警告使用未初始化的
//通过复制构造函数初始化“remainingin”时的值。我们可以
//如果“sendmax”在大多数情况下被初始化，则会收到类似的警告
//自然的方式。使用“make_optional”，我们可以解决这个bug。
    TInAmt const sendMaxInit =
        sendMaxST ? toAmount<TInAmt>(*sendMaxST) : TInAmt{beast::zero};
    boost::optional<TInAmt> const sendMax =
        boost::make_optional(sendMaxST && sendMaxInit >= beast::zero, sendMaxInit);
    boost::optional<TInAmt> remainingIn =
        boost::make_optional(!!sendMax, sendMaxInit);
//boost：：可选<tinamt>remainingin sendmax

    TOutAmt remainingOut (outReq);

    PaymentSandbox sb (&baseView);

//非干股
    ActiveStrands activeStrands (strands);

//按处理顺序保留金额的连续总和
//不会给出最好的精度。保留一个集合以便汇总
//从最小到最大
    boost::container::flat_multiset<TInAmt> savedIns;
    savedIns.reserve(maxTries);
    boost::container::flat_multiset<TOutAmt> savedOuts;
    savedOuts.reserve(maxTries);

    auto sum = [](auto const& col)
    {
        using TResult = std::decay_t<decltype (*col.begin ())>;
        if (col.empty ())
            return TResult{beast::zero};
        return std::accumulate (col.begin () + 1, col.end (), *col.begin ());
    };

//这些报价只有在付款不为
//成功的
    boost::container::flat_set<uint256> ofrsToRmOnFail;

    while (remainingOut > beast::zero &&
        (!remainingIn || *remainingIn > beast::zero))
    {
        ++curTry;
        if (curTry >= maxTries)
        {
            return {telFAILED_PROCESSING, std::move(ofrsToRmOnFail)};
        }

        activeStrands.activateNext();

        boost::container::flat_set<uint256> ofrsToRm;
        boost::optional<BestStrand> best;
        if (flowDebugInfo) flowDebugInfo->newLiquidityPass();
//要标记为不活动的链的索引（从活动列表中删除），如果
//使用流动性。这用于消耗过多报价的链
//构造为“false，0”，以解决有关未初始化变量的gcc警告
        boost::optional<std::size_t> markInactiveOnUse{false, 0};
        for (auto strand : activeStrands)
        {
            if (offerCrossing && limitQuality)
            {
                auto const strandQ = qualityUpperBound(sb, *strand);
                if (!strandQ || *strandQ < *limitQuality)
                    continue;
            }
            auto f = flow<TInAmt, TOutAmt> (
                sb, *strand, remainingIn, remainingOut, j);

//即使钢绞线出现故障，RM BAD也会提供
            ofrsToRm.insert (boost::container::ordered_unique_range_t{},
                f.ofrsToRm.begin (), f.ofrsToRm.end ());

            if (f.ter != tesSUCCESS || f.out == beast::zero)
                continue;

            if (flowDebugInfo)
                flowDebugInfo->pushLiquiditySrc(EitherAmount(f.in), EitherAmount(f.out));

            assert (f.out <= remainingOut && f.sandbox &&
                (!remainingIn || f.in <= *remainingIn));

            Quality const q (f.out, f.in);

            JLOG (j.trace())
                << "New flow iter (iter, in, out): "
                << curTry-1 << " "
                << to_string(f.in) << " "
                << to_string(f.out);

            if (limitQuality && q < *limitQuality)
            {
                JLOG (j.trace())
                    << "Path rejected by limitQuality"
                    << " limit: " << *limitQuality
                    << " path q: " << q;
                continue;
            }

            activeStrands.push (strand);

            if (!best || best->quality < q ||
                (best->quality == q && best->out < f.out))
            {
//如果此链处于非活动状态（因为它消耗了太多
//提供）并最终获得最佳质量，将其从
//活动开始。如果最后没有得到最好的
//质量，保持活跃。

                if (f.inactive)
                    markInactiveOnUse = activeStrands.size() - 1;
                else
                    markInactiveOnUse.reset();

                best.emplace(f.in, f.out, std::move(*f.sandbox), *strand, q);
            }
        }

        bool const shouldBreak = !best;

        if (best)
        {
            if (markInactiveOnUse)
            {
                activeStrands.removeIndex(*markInactiveOnUse);
                markInactiveOnUse.reset();
            }
            savedIns.insert (best->in);
            savedOuts.insert (best->out);
            remainingOut = outReq - sum (savedOuts);
            if (sendMax)
                remainingIn = *sendMax - sum (savedIns);

            if (flowDebugInfo)
                flowDebugInfo->pushPass (EitherAmount (best->in),
                    EitherAmount (best->out), activeStrands.size ());

            JLOG (j.trace())
                << "Best path: in: " << to_string (best->in)
                << " out: " << to_string (best->out)
                << " remainingOut: " << to_string (remainingOut);

            best->sb.apply (sb);
        }
        else
        {
            JLOG (j.trace()) << "All strands dry.";
        }

best.reset ();  //在修改基础之前，必须先销毁最佳视图
//看法
        if (!ofrsToRm.empty ())
        {
            ofrsToRmOnFail.insert (boost::container::ordered_unique_range_t{},
                ofrsToRm.begin (), ofrsToRm.end ());
            for (auto const& o : ofrsToRm)
            {
                if (auto ok = sb.peek (keylet::offer (o)))
                    offerDelete (sb, ok, j);
            }
        }

        if (shouldBreak)
            break;
    }

    auto const actualOut = sum (savedOuts);
    auto const actualIn = sum (savedIns);

    JLOG (j.trace())
        << "Total flow: in: " << to_string (actualIn)
        << " out: " << to_string (actualOut);

    if (actualOut != outReq)
    {
        if (actualOut > outReq)
        {
            assert (0);
            return {tefEXCEPTION, std::move(ofrsToRmOnFail)};
        }
        if (!partialPayment)
        {
//如果我们提供穿越A！部分付款，那么我们
//处理tffillWorkill。那个案子是在下面处理的，不是在这里。
            if (!offerCrossing)
                return {tecPATH_PARTIAL,
                    actualIn, actualOut, std::move(ofrsToRmOnFail)};
        }
        else if (actualOut == beast::zero)
        {
            return {tecPATH_DRY, std::move(ofrsToRmOnFail)};
        }
    }
    if (offerCrossing && !partialPayment)
    {
//如果我们提供交叉付款，而部分付款是*不是*真的，那么
//我们正在办理补给品的报价。在这种情况下，Remainingin必须
//为零（所有资金必须消耗），否则我们取消报价。
        assert (remainingIn);
        if (remainingIn && *remainingIn != beast::zero)
            return {tecPATH_PARTIAL,
                actualIn, actualOut, std::move(ofrsToRmOnFail)};
    }

    return {actualIn, actualOut, std::move (sb), std::move(ofrsToRmOnFail)};
}

} //涟漪

#endif
