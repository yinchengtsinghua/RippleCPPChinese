
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
#include <ripple/app/paths/impl/StepChecks.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Quality.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace ripple {

template <class TDerived>
class DirectStepI : public StepImp<IOUAmount, IOUAmount, DirectStepI<TDerived>>
{
protected:
    AccountID src_;
    AccountID dst_;
    Currency currency_;

//上一步赎回时收取过户费
    Step const* const prevStep_ = nullptr;
    bool const isLast_;
    beast::Journal j_;

    struct Cache
    {
        IOUAmount in;
        IOUAmount srcToDst;
        IOUAmount out;
        bool srcRedeems;

        Cache (
            IOUAmount const& in_,
            IOUAmount const& srcToDst_,
            IOUAmount const& out_,
            bool srcRedeems_)
            : in(in_)
            , srcToDst(srcToDst_)
            , out(out_)
            , srcRedeems(srcRedeems_)
        {}
    };

    boost::optional<Cache> cache_;

//计算可从SRC->DST AT流出的最大值
//最好的质量。
//返回：第一个元素是可以流动的最大量，
//其次，如果DST持有SRC的IOU，则为真。
    std::pair<IOUAmount, bool>
    maxPaymentFlow (
        ReadView const& sb) const;

//返回srcqout，dstqin
    std::pair <std::uint32_t, std::uint32_t>
    qualities (
        ReadView const& sb,
        bool srcRedeems,
        bool fwd) const;

public:
    DirectStepI (
        StrandContext const& ctx,
        AccountID const& src,
        AccountID const& dst,
        Currency const& c)
            : src_(src)
            , dst_(dst)
            , currency_ (c)
            , prevStep_ (ctx.prevStep)
            , isLast_ (ctx.isLast)
            , j_ (ctx.j)
    {}

    AccountID const& src () const
    {
        return src_;
    }
    AccountID const& dst () const
    {
        return dst_;
    }
    Currency const& currency () const
    {
        return currency_;
    }

    boost::optional<EitherAmount> cachedIn () const override
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (cache_->in);
    }

    boost::optional<EitherAmount>
    cachedOut () const override
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (cache_->out);
    }

    boost::optional<AccountID>
    directStepSrcAcct () const override
    {
        return src_;
    }

    boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const override
    {
        return std::make_pair(src_, dst_);
    }

    bool
    redeems (ReadView const& sb, bool fwd) const override;

    std::uint32_t
    lineQualityIn (ReadView const& v) const override;

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<IOUAmount, IOUAmount>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        IOUAmount const& out);

    std::pair<IOUAmount, IOUAmount>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        IOUAmount const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

//检查错误、现有流动性和违反授权/冻结的行为
//约束条件。
    TER check (StrandContext const& ctx) const;

    void setCacheLimiting (
        IOUAmount const& fwdIn,
        IOUAmount const& fwdSrcToDst,
        IOUAmount const& fwdOut,
        bool srcRedeems);

    friend bool operator==(DirectStepI const& lhs, DirectStepI const& rhs)
    {
        return lhs.src_ == rhs.src_ &&
            lhs.dst_ == rhs.dst_ &&
            lhs.currency_ == rhs.currency_;
    }

    friend bool operator!=(DirectStepI const& lhs, DirectStepI const& rhs)
    {
        return ! (lhs == rhs);
    }

protected:
    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\nSrc: " << src_ <<
            "\nDst: " << dst_;
        return ostr.str ();
    }

private:
    bool equal (Step const& rhs) const override
    {
        if (auto ds = dynamic_cast<DirectStepI const*> (&rhs))
        {
            return *this == *ds;
        }
        return false;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//在两种不同的情况下，流动用于转移资金：
//付款，以及
//o提供交叉路口。
//在这两种情况下，处理资金的规则几乎是，但不是
//完全一样。

//付款直接步骤类（不提供交叉）。
class DirectIPaymentStep : public DirectStepI<DirectIPaymentStep>
{
public:
    explicit DirectIPaymentStep() = default;

    using DirectStepI<DirectIPaymentStep>::DirectStepI;
    using DirectStepI<DirectIPaymentStep>::check;

    bool verifyPrevStepRedeems (bool) const
    {
//付款不关心是否提前赎回。
        return true;
    }

    bool verifyDstQualityIn (std::uint32_t dstQIn) const
    {
//付款对dstchin的未来没有特别的期望。
        return true;
    }

    std::uint32_t
    quality (ReadView const& sb,
//输入为真，输出为假
        bool qin) const;

//计算可从SRC->DST AT流出的最大值
//最好的质量。
//返回：第一个元素是可以流动的最大量，
//其次，如果DST持有SRC的IOU，则为真。
    std::pair<IOUAmount, bool>
    maxFlow (ReadView const& sb, IOUAmount const& desired) const;

//验证步骤的一致性。这些支票是特定于
//付款并假设已执行一般检查。
    TER
    check (StrandContext const& ctx,
        std::shared_ptr<const SLE> const& sleSrc) const;

    std::string logString () const override
    {
        return logStringImpl ("DirectIPaymentStep");
    }
};

//提供交叉DirectStep类（不是付款）。
class DirectIOfferCrossingStep : public DirectStepI<DirectIOfferCrossingStep>
{
public:
    explicit DirectIOfferCrossingStep() = default;

    using DirectStepI<DirectIOfferCrossingStep>::DirectStepI;
    using DirectStepI<DirectIOfferCrossingStep>::check;

    bool verifyPrevStepRedeems (bool prevStepRedeems) const
    {
//在报价交叉过程中，我们依赖的事实是
//将永远是错误的。那是因为：
//o如果有预告步骤u，它将永远是一个预定步骤。
//o bookstep:：redeems（）aways在提供交叉时返回false。
//基于此返回值的断言将告诉我们
//行为改变。
        return !prevStepRedeems;
    }

    bool verifyDstQualityIn (std::uint32_t dstQIn) const
    {
//由于以下几个因素，DSTCIN始终是
//提供交叉路口。如果这改变了，我们需要知道。
        return dstQIn == QUALITY_ONE;
    }

    std::uint32_t
    quality (ReadView const& sb,
//输入为真，输出为假
        bool qin) const;

//计算可从SRC->DST AT流出的最大值
//最好的质量。
//返回：第一个元素是可以流动的最大量，
//其次，如果DST持有SRC的IOU，则为真。
    std::pair<IOUAmount, bool>
    maxFlow (ReadView const& sb, IOUAmount const& desired) const;

//验证步骤的一致性。这些支票是特定于
//提供交叉口，并假设已执行一般检查。
    TER
    check (StrandContext const& ctx,
        std::shared_ptr<const SLE> const& sleSrc) const;


    std::string logString () const override
    {
        return logStringImpl ("DirectIOfferCrossingStep");
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::uint32_t
DirectIPaymentStep::quality (ReadView const& sb,
//输入为真，输出为假
    bool qin) const
{
    if (src_ == dst_)
        return QUALITY_ONE;

    auto const sle = sb.read (keylet::line (dst_, src_, currency_));

    if (!sle)
        return QUALITY_ONE;

    auto const& field = [this, qin]() -> SF_U32 const&
    {
        if (qin)
        {
//计算DST质量
            if (this->dst_ < this->src_)
                return sfLowQualityIn;
            else
                return sfHighQualityIn;
        }
        else
        {
//计算SRC质量输出
            if (this->src_ < this->dst_)
                return sfLowQualityOut;
            else
                return sfHighQualityOut;
        }
    }();

    if (! sle->isFieldPresent (field))
        return QUALITY_ONE;

    auto const q = (*sle)[field];
    if (!q)
        return QUALITY_ONE;
    return q;
}

std::uint32_t
DirectIOfferCrossingStep::quality (ReadView const&,
//输入为真，输出为假
    bool) const
{
//如果提供交叉，则忽略信任线质量字段。这个
//保持着悠久的传统。
    return QUALITY_ONE;
}

std::pair<IOUAmount, bool>
DirectIPaymentStep::maxFlow (ReadView const& sb, IOUAmount const&) const
{
    return maxPaymentFlow (sb);
}

std::pair<IOUAmount, bool>
DirectIOfferCrossingStep::maxFlow (
    ReadView const& sb, IOUAmount const& desired) const
{
//当islast和offer交叉时，则忽略信任线限制。要约
//交叉具有超过信任线设置的限制的能力。
//我们假定，如果有人提出报价，那么他们打算
//尽可能多地填写报价单，即使报价超过
//信任线设置的限制。
//
//关于使用“out”作为MaxFlow所需参数的说明。在一些
//在付款过程中，我们最终需要一个大于
//“out”表示“maxsrctodst”。但到目前为止（2016年6月），这种情况从未发生过
//报价交叉时。这是因为，由于一些因素，
//“dstqin”始终是优质的“offer crossing”。

    if (isLast_)
        return {desired, false};

    return maxPaymentFlow (sb);
}

TER
DirectIPaymentStep::check (
    StrandContext const& ctx, std::shared_ptr<const SLE> const& sleSrc) const
{
//因为这是一笔付款，所以必须存在一个信托额度。全部执行
//与信任行相关的检查。
    {
        auto const sleLine = ctx.view.read (keylet::line (src_, dst_, currency_));
        if (!sleLine)
        {
            JLOG (j_.trace()) << "DirectStepI: No credit line. " << *this;
            return terNO_LINE;
        }

        auto const authField = (src_ > dst_) ? lsfHighAuth : lsfLowAuth;

        if (((*sleSrc)[sfFlags] & lsfRequireAuth) &&
            !((*sleLine)[sfFlags] & authField) &&
            (*sleLine)[sfBalance] == beast::zero)
        {
            JLOG (j_.warn())
                << "DirectStepI: can't receive IOUs from issuer without auth."
                << " src: " << src_;
            return terNO_AUTH;
        }

        if (ctx.prevStep &&
            fix1449(ctx.view.info().parentCloseTime))
        {
            if (ctx.prevStep->bookStepBook())
            {
                auto const noRippleSrcToDst =
                    ((*sleLine)[sfFlags] &
                     ((src_ > dst_) ? lsfHighNoRipple : lsfLowNoRipple));
                if (noRippleSrcToDst)
                    return terNO_RIPPLE;
            }
        }
    }

    {
        auto const owed = creditBalance (ctx.view, dst_, src_, currency_);
        if (owed <= beast::zero)
        {
            auto const limit = creditLimit (ctx.view, dst_, src_, currency_);
            if (-owed >= limit)
            {
                JLOG (j_.debug())
                    << "DirectStepI: dry: owed: " << owed << " limit: " << limit;
                return tecPATH_DRY;
            }
        }
    }
    return tesSUCCESS;
}

TER
DirectIOfferCrossingStep::check (
    StrandContext const&, std::shared_ptr<const SLE> const&) const
{
//标准支票是我们能做的，因为剩下的支票
//需要建立信任关系。报价交叉不能
//需要预先存在的信任线。
    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class TDerived>
std::pair<IOUAmount, bool>
DirectStepI<TDerived>::maxPaymentFlow (ReadView const& sb) const
{
    auto const srcOwed = toAmount<IOUAmount> (
        accountHolds (sb, src_, currency_, dst_, fhIGNORE_FREEZE, j_));

    if (srcOwed.signum () > 0)
        return {srcOwed, true};

//srcouded为负或零
    return {creditLimit2 (sb, dst_, src_, currency_) + srcOwed, false};
}

template <class TDerived>
bool
DirectStepI<TDerived>::redeems (ReadView const& sb, bool fwd) const
{
    if (fwd && cache_)
        return cache_->srcRedeems;

    auto const srcOwed = accountHolds (
        sb, src_, currency_, dst_, fhIGNORE_FREEZE, j_);
    return srcOwed.signum () > 0;
}

template <class TDerived>
std::pair<IOUAmount, IOUAmount>
DirectStepI<TDerived>::revImp (
    PaymentSandbox& sb,
    /*lyview和/*afview*/，
    Boost：：container：：flat_set<uint256>&/*ofrstor*/,

    IOUAmount const& out)
{
    cache_.reset ();

    bool srcRedeems;
    IOUAmount maxSrcToDst;

    std::tie (maxSrcToDst, srcRedeems) =
        static_cast<TDerived const*>(this)->maxFlow (sb, out);

    std::uint32_t srcQOut, dstQIn;
    std::tie (srcQOut, dstQIn) = qualities (sb, srcRedeems, false);
    assert (static_cast<TDerived const*>(this)->verifyDstQualityIn (dstQIn));

    Issue const srcToDstIss (currency_, srcRedeems ? dst_ : src_);

    JLOG (j_.trace()) <<
        "DirectStepI::rev" <<
        " srcRedeems: " << srcRedeems <<
        " outReq: " << to_string (out) <<
        " maxSrcToDst: " << to_string (maxSrcToDst) <<
        " srcQOut: " << srcQOut <<
        " dstQIn: " << dstQIn;

    if (maxSrcToDst.signum () <= 0)
    {
        JLOG (j_.trace()) << "DirectStepI::rev: dry";
        cache_.emplace (
            IOUAmount (beast::zero),
            IOUAmount (beast::zero),
            IOUAmount (beast::zero),
            srcRedeems);
        return {beast::zero, beast::zero};
    }

    IOUAmount const srcToDst = mulRatio (
        /*，Quality_One，Dstqin，/*Roundup*/true）；

    if（srctodst<=maxsrctodst）
    {
        iouamount const in=multratio（
            srctodst、srcqout、quality_one、/*roundu*/ true);

        cache_.emplace (in, srcToDst, out, srcRedeems);
        rippleCredit (sb,
                      src_, dst_, toSTAmount (srcToDst, srcToDstIss),
                      /*heckissuer*/true，j_uu）；
        jLog（j_.trace（））<<
            “directstepi:：rev:non-limiting”<<
            “srcredems：”<<srcredems<<
            “in：”<<to_string（in）<<
            “srctodst：”<<to_string（srctodst）<<
            “out：”<<to_string（out）；
        返回输入，输出
    }

    //限制节点
    iouamount const in=multratio（
        Maxsrctodst、srcqout、Quality_One、/*Roundu*/ true);

    IOUAmount const actualOut = mulRatio (
        /*srctodst，dstchin，quality_one，/*roundup*/false）；
    缓存模板（in，maxsrctodst，actualout，srcreems）；
    RippleCredit（某人，
                  SRC_uuu，DST_u，Tostamount（最大SRCTodst，SRCTodstiss）
                  检查问题*/ true, j_);

    JLOG (j_.trace()) <<
        "DirectStepI::rev: Limiting" <<
        " srcRedeems: " << srcRedeems <<
        " in: " << to_string (in) <<
        " srcToDst: " << to_string (maxSrcToDst) <<
        " out: " << to_string (out);
    return {in, actualOut};
}

//远期信用证的流动性永远不应超过反向信用证的流动性。
//通过。但有时四舍五入的差异会导致向前传球
//提供更多的流动性。使用反向传递中的缓存值
//为了防止这种情况。
template <class TDerived>
void
DirectStepI<TDerived>::setCacheLimiting (
    IOUAmount const& fwdIn,
    IOUAmount const& fwdSrcToDst,
    IOUAmount const& fwdOut,
    bool srcRedeems)
{
    if (cache_->in < fwdIn)
    {
        IOUAmount const smallDiff(1, -9);
        auto const diff = fwdIn - cache_->in;
        if (diff > smallDiff)
        {
            if (fwdIn.exponent () != cache_->in.exponent () ||
                !cache_->in.mantissa () ||
                (double(fwdIn.mantissa ()) /
                    double(cache_->in.mantissa ())) > 1.01)
            {
//在向前通过时检测大的差异，以便进行调查
                JLOG (j_.warn())
                    << "DirectStepI::fwd: setCacheLimiting"
                    << " fwdIn: " << to_string (fwdIn)
                    << " cacheIn: " << to_string (cache_->in)
                    << " fwdSrcToDst: " << to_string (fwdSrcToDst)
                    << " cacheSrcToDst: " << to_string (cache_->srcToDst)
                    << " fwdOut: " << to_string (fwdOut)
                    << " cacheOut: " << to_string (cache_->out);
                cache_.emplace (fwdIn, fwdSrcToDst, fwdOut, srcRedeems);
                return;
            }
        }
    }
    cache_->in = fwdIn;
    if (fwdSrcToDst < cache_->srcToDst)
        cache_->srcToDst = fwdSrcToDst;
    if (fwdOut < cache_->out)
        cache_->out = fwdOut;
    cache_->srcRedeems = srcRedeems;
};

template <class TDerived>
std::pair<IOUAmount, IOUAmount>
DirectStepI<TDerived>::fwdImp (
    PaymentSandbox& sb,
    /*lyview和/*afview*/，
    Boost：：container：：flat_set<uint256>&/*ofrstor*/,

    IOUAmount const& in)
{
    assert (cache_);

    bool srcRedeems;
    IOUAmount maxSrcToDst;
    std::tie (maxSrcToDst, srcRedeems) =
        static_cast<TDerived const*>(this)->maxFlow (sb, cache_->srcToDst);

    std::uint32_t srcQOut, dstQIn;
    std::tie (srcQOut, dstQIn) = qualities (sb, srcRedeems, true);

    Issue const srcToDstIss (currency_, srcRedeems ? dst_ : src_);

    JLOG (j_.trace()) <<
            "DirectStepI::fwd" <<
            " srcRedeems: " << srcRedeems <<
            " inReq: " << to_string (in) <<
            " maxSrcToDst: " << to_string (maxSrcToDst) <<
            " srcQOut: " << srcQOut <<
            " dstQIn: " << dstQIn;

    if (maxSrcToDst.signum () <= 0)
    {
        JLOG (j_.trace()) << "DirectStepI::fwd: dry";
        cache_.emplace (
            IOUAmount (beast::zero),
            IOUAmount (beast::zero),
            IOUAmount (beast::zero),
            srcRedeems);
        return {beast::zero, beast::zero};
    }

    IOUAmount const srcToDst = mulRatio (
        /*Quality_One，srcqout，/*Roundup*/false）；

    if（srctodst<=maxsrctodst）
    {
        iouamount const out=多比率（
            srctodst、dstchin、quality_one、/*roundu*/ false);

        setCacheLimiting (in, srcToDst, out, srcRedeems);
        rippleCredit (sb,
            src_, dst_, toSTAmount (cache_->srcToDst, srcToDstIss),
            /*heckissuer*/true，j_uu）；
        jLog（j_.trace（））<<
                “directstepi:：fwd:非限制”<<
                “srcredems：”<<srcredems<<
                “in：”<<to_string（in）<<
                “srctodst：”<<to_string（srctodst）<<
                “out：”<<to_string（out）；
    }
    其他的
    {
        //限制节点
        iouamount const actuann=multratio（
            Maxsrctodst、srcqout、Quality_One、/*Roundu*/ true);

        IOUAmount const out = mulRatio (
            /*srctodst，dstchin，quality_one，/*roundup*/false）；
        setcachelimiting（actuann、maxsrctodst、out、srcreems）；
        RippleCredit（某人，
            SRC_uuu，DST_u，Tostamount（高速缓存->srctodst，srctodstiss）
            检查问题*/ true, j_);

        JLOG (j_.trace()) <<
                "DirectStepI::rev: Limiting" <<
                " srcRedeems: " << srcRedeems <<
                " in: " << to_string (actualIn) <<
                " srcToDst: " << to_string (srcToDst) <<
                " out: " << to_string (out);
    }
    return {cache_->in, cache_->out};
}

template <class TDerived>
std::pair<bool, EitherAmount>
DirectStepI<TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.trace()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (IOUAmount (beast::zero))};
    }


    auto const savCache = *cache_;

    assert (!in.native);

    bool srcRedeems;
    IOUAmount maxSrcToDst;
    std::tie (maxSrcToDst, srcRedeems) =
        static_cast<TDerived const*>(this)->maxFlow (sb, cache_->srcToDst);

    try
    {
        boost::container::flat_set<uint256> dummy;
fwdImp (sb, afView, dummy, in.iou);  //更改缓存
    }
    catch (FlowException const&)
    {
        return {false, EitherAmount (IOUAmount (beast::zero))};
    }

    if (maxSrcToDst < cache_->srcToDst)
    {
        JLOG (j_.error()) <<
            "DirectStepI: Strand re-execute check failed." <<
            " Exceeded max src->dst limit" <<
            " max src->dst: " << to_string (maxSrcToDst) <<
            " actual src->dst: " << to_string (cache_->srcToDst);
        return {false, EitherAmount(cache_->out)};
    }

    if (!(checkNear (savCache.in, cache_->in) &&
          checkNear (savCache.out, cache_->out)))
    {
        JLOG (j_.error()) <<
            "DirectStepI: Strand re-execute check failed." <<
            " ExpectedIn: " << to_string (savCache.in) <<
            " CachedIn: " << to_string (cache_->in) <<
            " ExpectedOut: " << to_string (savCache.out) <<
            " CachedOut: " << to_string (cache_->out);
        return {false, EitherAmount (cache_->out)};
    }
    return {true, EitherAmount (cache_->out)};
}

//返回srcqout，dstqin
template <class TDerived>
std::pair<std::uint32_t, std::uint32_t>
DirectStepI<TDerived>::qualities (
    ReadView const& sb,
    bool srcRedeems,
    bool fwd) const
{
    if (srcRedeems)
    {
        if (!prevStep_)
            return {QUALITY_ONE, QUALITY_ONE};

        auto const prevStepQIn = prevStep_->lineQualityIn (sb);
        auto srcQOut = static_cast<TDerived const*>(this)->quality (
            /*/*SRC质量输出*/错误）；

        如果（prevstepqin>srcqout）
            srcqout=prevstepqin；
        退货srcqout，质量一
    }
    其他的
    {
        //在发行和上一步赎回时收取转移率
        auto const prevstep redeems=prevstep&&prevstep->redeems（sb，fwd）；
        断言（static_cast<tderived const*>（this）->verifyPrevStepRedeems（
            PrevStepRedems））；

        标准：：uint32常数srcqout=
            预防措施？传输率（sb，src_uu）。值：质量_1；
        auto dstqin=static_cast<tderived const*>（this）->质量（
            SB，/*DST质量输入*/ true);


        if (isLast_ && dstQIn > QUALITY_ONE)
            dstQIn = QUALITY_ONE;
        return {srcQOut, dstQIn};
    }
}

template <class TDerived>
std::uint32_t
DirectStepI<TDerived>::lineQualityIn (ReadView const& v) const
{
//DST质量
    return static_cast<TDerived const*>(this)->quality (
        /*/*DST质量为*/真）；
}

template<class tderived>
boost：：可选<quality>
directstepi<tderive>：质量上界（readview const&v，bool&redeems）const
{
    auto const prevredems=赎回；
    redeems=this->redeems（v，true）；
    标准：：uint32常数srcqout=
        （预防和&！赎回）？传输率（V，SRC_uu）。值：质量_1；
    auto dstqin=static_cast<tderived const*>（this）->质量（
        V，/*DST质量输入*/ true);


    if (isLast_ && dstQIn > QUALITY_ONE)
        dstQIn = QUALITY_ONE;
    Issue const iss{currency_, src_};
    return Quality(getRate(STAmount(iss, srcQOut), STAmount(iss, dstQIn)));
}

template <class TDerived>
TER DirectStepI<TDerived>::check (StrandContext const& ctx) const
{
//以下支票既适用于付款，也适用于报价交叉。
    if (!src_ || !dst_)
    {
        JLOG (j_.debug()) << "DirectStepI: specified bad account.";
        return temBAD_PATH;
    }

    if (src_ == dst_)
    {
        JLOG (j_.debug()) << "DirectStepI: same src and dst.";
        return temBAD_PATH;
    }

    auto const sleSrc = ctx.view.read (keylet::account (src_));
    if (!sleSrc)
    {
        JLOG (j_.warn())
            << "DirectStepI: can't receive IOUs from non-existent issuer: "
            << src_;
        return terNO_ACCOUNT;
    }

//纯发行/兑换不能冻结
    if (!(ctx.isLast && ctx.isFirst))
    {
        auto const ter = checkFreeze(ctx.view, src_, dst_, currency_);
        if (ter != tesSUCCESS)
            return ter;
    }

//如果前一步是直接步骤，那么我们需要检查
//没有波纹标志。
    if (ctx.prevStep)
    {
        if (auto prevSrc = ctx.prevStep->directStepSrcAcct())
        {
            auto const ter = checkNoRipple(
                ctx.view, *prevSrc, src_, dst_, currency_, j_);
            if (ter != tesSUCCESS)
                return ter;
        }
    }
    {
        Issue const srcIssue{currency_, src_};
        Issue const dstIssue{currency_, dst_};

        if (ctx.seenBookOuts.count (srcIssue))
        {
            if (!ctx.prevStep)
            {
assert(0); //上一个看到的书没有上一步！？！
                return temBAD_PATH_LOOP;
            }

//如果上一步是输出此问题的书籍步骤，则可以执行此操作。
            if (auto book = ctx.prevStep->bookStepBook())
            {
                if (book->out != srcIssue)
                    return temBAD_PATH_LOOP;
            }
        }

        if (!ctx.seenDirectIssues[0].insert (srcIssue).second ||
            !ctx.seenDirectIssues[1].insert (dstIssue).second)
        {
            JLOG (j_.debug ())
                << "DirectStepI: loop detected: Index: " << ctx.strandSize
                << ' ' << *this;
            return temBAD_PATH_LOOP;
        }
    }

    return static_cast<TDerived const*>(this)->check (ctx, sleSrc);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace test
{
//测试所需
bool directStepEqual (Step const& step,
    AccountID const& src,
    AccountID const& dst,
    Currency const& currency)
{
    if (auto ds =
        dynamic_cast<DirectStepI<DirectIPaymentStep> const*> (&step))
    {
        return ds->src () == src && ds->dst () == dst &&
            ds->currency () == currency;
    }
    return false;
}
}  //测试

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::pair<TER, std::unique_ptr<Step>>
make_DirectStepI (
    StrandContext const& ctx,
    AccountID const& src,
    AccountID const& dst,
    Currency const& c)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<DirectIOfferCrossingStep> (ctx, src, dst, c);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
else //付款
    {
        auto paymentStep =
            std::make_unique<DirectIPaymentStep> (ctx, src, dst, c);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move (r)};
}

} //涟漪
