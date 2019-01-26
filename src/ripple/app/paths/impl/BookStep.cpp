
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

#include <ripple/app/paths/impl/Steps.h>
#include <ripple/app/paths/Credit.h>
#include <ripple/app/paths/NodeDirectory.h>
#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/Directory.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/Book.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace ripple {

template<class TIn, class TOut, class TDerived>
class BookStep : public StepImp<TIn, TOut, BookStep<TIn, TOut, TDerived>>
{
protected:
    uint32_t const maxOffersToConsume_;
    Book book_;
    AccountID strandSrc_;
    AccountID strandDst_;
//上一步赎回时收取过户费
    Step const* const prevStep_ = nullptr;
    bool const ownerPaysTransferFee_;
//如果消费了太多的产品，请标记为不活动（干燥）
    bool inactive_ = false;
    beast::Journal j_;

    struct Cache
    {
        TIn in;
        TOut out;

        Cache (TIn const& in_, TOut const& out_)
            : in (in_), out (out_)
        {
        }
    };

    boost::optional<Cache> cache_;

    static
    uint32_t getMaxOffersToConsume(StrandContext const& ctx)
    {
        if (ctx.view.rules().enabled(fix1515))
            return 1000;
        return 2000;
    }

public:
    BookStep (StrandContext const& ctx,
        Issue const& in,
        Issue const& out)
        : maxOffersToConsume_ (getMaxOffersToConsume(ctx))
        , book_ (in, out)
        , strandSrc_ (ctx.strandSrc)
        , strandDst_ (ctx.strandDst)
        , prevStep_ (ctx.prevStep)
        , ownerPaysTransferFee_ (ctx.ownerPaysTransferFee)
        , j_ (ctx.j)
    {
    }

    Book const& book() const
    {
        return book_;
    }

    boost::optional<EitherAmount>
    cachedIn () const override
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

    bool
    redeems (ReadView const& sb, bool fwd) const override
    {
        return !ownerPaysTransferFee_;
    }

    boost::optional<Book>
    bookStepBook () const override
    {
        return book_;
    }

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<TIn, TOut>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        TOut const& out);

    std::pair<TIn, TOut>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        TIn const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

//检查冻结约束是否有错误。
    TER check(StrandContext const& ctx) const;

    bool inactive() const override {
        return inactive_;
    }

protected:
    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\ninIss: " << book_.in.account <<
            "\noutIss: " << book_.out.account <<
            "\ninCur: " << book_.in.currency <<
            "\noutCur: " << book_.out.currency;
        return ostr.str ();
    }

private:
    friend bool operator==(BookStep const& lhs, BookStep const& rhs)
    {
        return lhs.book_ == rhs.book_;
    }

    friend bool operator!=(BookStep const& lhs, BookStep const& rhs)
    {
        return ! (lhs == rhs);
    }

    bool equal (Step const& rhs) const override;

//在一本书中以最佳的质量反复阅读这些产品。
//未提供资金的报价和不好的报价被跳过（并被退回）。
//回电是通过提供SLE来调用的，接受方付费，接受方付费。
//如果回调返回false，则不再处理任何报价。
//返回未提供资金和坏报价以及消耗的报价数量。
    template <class Callback>
    std::pair<boost::container::flat_set<uint256>, std::uint32_t>
    forEachOffer (
        PaymentSandbox& sb,
        ApplyView& afView,
        bool prevStepRedeems,
        Callback& callback) const;

    void consumeOffer (PaymentSandbox& sb,
        TOffer<TIn, TOut>& offer,
        TAmounts<TIn, TOut> const& ofrAmt,
        TAmounts<TIn, TOut> const& stepAmt,
        TOut const& ownerGives) const;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//在两种不同的情况下，流动用于转移资金：
//付款，以及
//o提供交叉路口。
//在这两种情况下，处理资金的规则几乎是，但不是
//完全一样。

//付款bookstep模板类（不提供交叉）。
template<class TIn, class TOut>
class BookPaymentStep
    : public BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>
{
public:
    explicit BookPaymentStep() = default;

    using BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>::BookStep;
    using BookStep<TIn, TOut, BookPaymentStep<TIn, TOut>>::qualityUpperBound;

//不要限制付款的自交叉质量。
    bool limitSelfCrossQuality (AccountID const&, AccountID const&,
        TOffer<TIn, TOut> const& offer, boost::optional<Quality>&,
        FlowOfferStream<TIn, TOut>&, bool) const
    {
        return false;
    }

//付款可以查看任何质量的报价。
    bool checkQualityThreshold(TOffer<TIn, TOut> const& offer) const
    {
        return true;
    }

//因为RinRate的付款总是和Trin一样的。
    std::uint32_t getOfrInRate (
        Step const*, TOffer<TIn, TOut> const&, std::uint32_t trIn) const
    {
        return trIn;
    }

//因为Routrate的付款总是和Trout一样的。
    std::uint32_t getOfrOutRate (Step const*, TOffer<TIn, TOut> const&,
        AccountID const&, std::uint32_t trOut) const
    {
        return trOut;
    }

    Quality
    qualityUpperBound(ReadView const& v,
        Quality const& ofrQ,
        bool prevStepRedeems) const
    {
//向发盘人收费，而不是向发盘人收费
//即使所有者与发行人相同，也要收取费用。
//（旧代码不收费）
//计算到接受者的金额和收取的金额
//发盘人
        auto rate = [&](AccountID const& id) {
            if (isXRP(id) || id == this->strandDst_)
                return parityRate;
            return transferRate(v, id);
        };

        auto const trIn =
            prevStepRedeems ? rate(this->book_.in.account) : parityRate;
//始终收取转让费，即使所有人是发行人
        auto const trOut =
            this->ownerPaysTransferFee_
            ? rate(this->book_.out.account)
            : parityRate;

        Quality const q1{getRate(STAmount(trOut.value), STAmount(trIn.value))};
        return composed_quality(q1, ofrQ);
    }

    std::string logString () const override
    {
        return this->logStringImpl ("BookPaymentStep");
    }
};

//提供交叉bookstep模板类（不是付款）。
template<class TIn, class TOut>
class BookOfferCrossingStep
    : public BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>>
{
    using BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>>::qualityUpperBound;
private:
//如果可选参数传递给构造函数，则引发的helper函数
//一个也没有。
    static Quality getQuality (boost::optional<Quality> const& limitQuality)
    {
//如果质量不好，那真是一个编程错误。
        assert (limitQuality);
        if (!limitQuality)
            Throw<FlowException> (tefINTERNAL, "Offer requires quality.");
        return *limitQuality;
    }

public:
    BookOfferCrossingStep (
        StrandContext const& ctx, Issue const& in, Issue const& out)
    : BookStep<TIn, TOut, BookOfferCrossingStep<TIn, TOut>> (ctx, in, out)
    , defaultPath_ (ctx.isDefaultPath)
    , qualityThreshold_ (getQuality (ctx.limitQuality))
    {
    }

    bool limitSelfCrossQuality (AccountID const& strandSrc,
        AccountID const& strandDst, TOffer<TIn, TOut> const& offer,
        boost::optional<Quality>& ofrQ, FlowOfferStream<TIn, TOut>& offers,
        bool const offerAttempted) const
    {
//这个方法支持一些正确但有点令人吃惊的方法
//报价交叉行为。情景：
//
//o Alice已经创建了一个或多个报价。
//o Alice创建了另一个可以直接交叉（而不是
//由她以前创建的一个或多个报价自动完成。
//
//交叉报价有什么作用？
//
//o报价交叉口可以继续进行，交叉报价离开。
//一个减价（部分交叉）或零报价
//（精确交叉）在分类帐中。我们不这样做。而且，真的，
//报价创建者可能不想让我们这么做。
//
//o我们可以跳过书中的自我介绍，只需交叉
//不是我们自己的报价。这很有道理，
//但我们不这么做。部分理由是我们只能
//操作订单的顶部。我们不能留下报价
//在后面——它会坐在顶端，阻止其他人进入。
//提供。
//
//o我们可以从
//预订并继续报价交叉。我们就是这么做的。
//
//为了支持这个场景，提供交叉有一个特殊的规则。如果：
//A.我们提供使用默认路径的交叉（无自动桥接），以及
//B.报盘的质量至少和我们的质量一样好，而且
//C.那我们就要越过自己的报价了。
//d.从分类帐中删除旧报价。
        if (defaultPath_ && offer.quality() >= qualityThreshold_ &&
            strandSrc == offer.owner() && strandDst == offer.owner())
        {
//即使没有交叉，也要取消此报价。
            offers.permRmOffer (offer.key());

//如果还没有尝试过提供服务，那么可以转到
//不同的品质。
            if (!offerAttempted)
                ofrQ = boost::none;

//返回true，以便删除当前报价。
            return true;
        }
        return false;
    }

//offer crossing可以用
//质量阈值。
    bool checkQualityThreshold(TOffer<TIn, TOut> const& offer) const
    {
        return !defaultPath_ || offer.quality() >= qualityThreshold_;
    }

//如果艾丽丝付钱给艾丽丝，交叉报价不付过户费。
//定期（非报价交叉）付款不适用此规则。
    std::uint32_t getOfrInRate (Step const* prevStep,
        TOffer<TIn, TOut> const& offer, std::uint32_t trIn) const
    {
        auto const srcAcct = prevStep ?
            prevStep->directStepSrcAcct() :
            boost::none;

return                                        //如果报价交叉
srcAcct &&                                //上一步是直接的（&&P）
offer.owner() == *srcAcct                 //&&SRC是报价所有者
? QUALITY_ONE : trIn;                     //那么比率=质量
    }

//请参见GetOfrinRate（）上的注释。
    std::uint32_t getOfrOutRate (
        Step const* prevStep, TOffer<TIn, TOut> const& offer,
        AccountID const& strandDst, std::uint32_t trOut) const
    {
return                                        //如果报价交叉
prevStep && prevStep->bookStepBook() &&   //上一步是bookstep（&&P）
offer.owner() == strandDst                //&&dest是报价所有者
? QUALITY_ONE : trOut;                    //那么比率=质量
    }

    Quality
    qualityUpperBound(ReadView const& v,
        Quality const& ofrQ,
        bool prevStepRedeems) const
    {
//优惠X-ing不收取转让费，当优惠的所有者
//与链DST相同。“质量上界”很重要
//是质量的上界（用于忽略股）
//其质量不能达到最低阈值）。计算时
//质量假设不收取费用，否则估计将不再
//做一个上界。
        return ofrQ;
    }

    std::string logString () const override
    {
        return this->logStringImpl ("BookOfferCrossingStep");
    }

private:
    bool const defaultPath_;
    Quality const qualityThreshold_;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class TIn, class TOut, class TDerived>
bool BookStep<TIn, TOut, TDerived>::equal (Step const& rhs) const
{
    if (auto bs = dynamic_cast<BookStep<TIn, TOut, TDerived> const*>(&rhs))
        return book_ == bs->book_;
    return false;
}

template <class TIn, class TOut, class TDerived>
boost::optional<Quality>
BookStep<TIn, TOut, TDerived>::qualityUpperBound(
    ReadView const& v, bool& redeems) const
{
    auto const prevStepRedeems = redeems;
    redeems = this->redeems(v, true);

//如果目录从不为空，则可以简化（并加快速度）。
    Sandbox sb(&v, tapNONE);
    BookTip bt(sb, book_);
    if (!bt.step(j_))
        return boost::none;

    return static_cast<TDerived const*>(this)->qualityUpperBound(
        v, bt.quality(), prevStepRedeems);
}

//根据给定的输入限制调整报价金额和步进金额
template <class TIn, class TOut>
static
void limitStepIn (Quality const& ofrQ,
    TAmounts<TIn, TOut>& ofrAmt,
    TAmounts<TIn, TOut>& stpAmt,
    TOut& ownerGives,
    std::uint32_t transferRateIn,
    std::uint32_t transferRateOut,
    TIn const& limit)
{
    if (limit < stpAmt.in)
    {
        stpAmt.in = limit;
        auto const inLmt = mulRatio (
            /*amt.in，quality_one，transferratein，/*roundup*/false）；
        oframt=ofrq.ceil_in（oframt，inlmt）；
        stpamt.out=oframt.out；
        所有者给出=多重比率（
            oframt.out、transferrateout、quality_one、/*roundu*/ false);

    }
}

//根据给定的输出限制调整报价金额和步进金额
template <class TIn, class TOut>
static
void limitStepOut (Quality const& ofrQ,
    TAmounts<TIn, TOut>& ofrAmt,
    TAmounts<TIn, TOut>& stpAmt,
    TOut& ownerGives,
    std::uint32_t transferRateIn,
    std::uint32_t transferRateOut,
    TOut const& limit)
{
    if (limit < stpAmt.out)
    {
        stpAmt.out = limit;
        ownerGives = mulRatio (
            /*amt.out、transferrateout、quality_one、/*roundup*/false）；
        oframt=ofrq.ceil-out（oframt，stpamt.out）；
        stpamt.in=多比率（
            oframt.in，transferratein，quality_one，/*roundu*/ true);

    }
}

template <class TIn, class TOut, class TDerived>
template <class Callback>
std::pair<boost::container::flat_set<uint256>, std::uint32_t>
BookStep<TIn, TOut, TDerived>::forEachOffer (
    PaymentSandbox& sb,
    ApplyView& afView,
    bool prevStepRedeems,
    Callback& callback) const
{
//向发盘人收费，而不是向发盘人收费
//即使所有者与发行人相同，也要收取费用。
//（旧代码不收费）
//计算给接受者的金额和报价所有者收取的金额
    auto rate = [this, &sb](AccountID const& id)->std::uint32_t
    {
        if (isXRP (id) || id == this->strandDst_)
            return QUALITY_ONE;
        return transferRate (sb, id).value;
    };

    std::uint32_t const trIn = prevStepRedeems
        ? rate (book_.in.account)
        : QUALITY_ONE;
//始终收取转让费，即使所有人是发行人
    std::uint32_t const trOut = ownerPaysTransferFee_
        ? rate (book_.out.account)
        : QUALITY_ONE;

    typename FlowOfferStream<TIn, TOut>::StepCounter
        counter (maxOffersToConsume_, j_);

    FlowOfferStream<TIn, TOut> offers (
        sb, afView, book_, sb.parentCloseTime (), counter, j_);

    bool const flowCross = afView.rules().enabled(featureFlowCross);
    bool offerAttempted = false;
    boost::optional<Quality> ofrQ;
    while (offers.step ())
    {
        auto& offer = offers.tip ();

//请注意，offer.quality（）返回（非可选）质量。所以
//OFRQ在循环的这个点以下使用总是安全的。
        if (!ofrQ)
            ofrQ = offer.quality ();
        else if (*ofrQ != offer.quality ())
            break;

        if (static_cast<TDerived const*>(this)->limitSelfCrossQuality (
            strandSrc_, strandDst_, offer, ofrQ, offers, offerAttempted))
                continue;

//确保要约所有人有权拥有发行人的借据。
//帐户可以始终拥有XRP或其自己的IOU。
        if (flowCross &&
            (!isXRP (offer.issueIn().currency)) &&
            (offer.owner() != offer.issueIn().account))
        {
            auto const& issuerID = offer.issueIn().account;
            auto const issuer = afView.read (keylet::account (issuerID));
            if (issuer && ((*issuer)[sfFlags] & lsfRequireAuth))
            {
//颁发者需要授权。看看报价人是否有。
                auto const& ownerID = offer.owner();
                auto const authFlag =
                    issuerID > ownerID ? lsfHighAuth : lsfLowAuth;

                auto const line = afView.read (keylet::line (
                    ownerID, issuerID, offer.issueIn().currency));

                if (!line || (((*line)[sfFlags] & authFlag) == 0))
                {
//要约所有人未被授权持有发行人的IOU。
//即使没有交叉，也要取消此报价。
                    offers.permRmOffer (offer.key());
                    if (!offerAttempted)
//只有在以前没有尝试过的情况下才能更改质量。
                        ofrQ = boost::none;
//此继续操作将导致offers.step（）删除该offere。
                    continue;
                }
            }
        }

        if (! static_cast<TDerived const*>(this)->checkQualityThreshold(offer))
            break;

        auto const ofrInRate =
            static_cast<TDerived const*>(this)->getOfrInRate (
                prevStep_, offer, trIn);

        auto const ofrOutRate =
            static_cast<TDerived const*>(this)->getOfrOutRate (
                prevStep_, offer, strandDst_, trOut);

        auto ofrAmt = offer.amount ();
        auto stpAmt = make_Amounts (
            /*比率（oframt.in，ofrinrate，quality_one，/*roundup*/true）
            OfRAMT；out；

        //业主支付过户费。
        自动所有者给出=
            多重比率（OFRAmt.out，OFROUTRATE，Quality_One，/*Roundu*/ false);


        auto const funds =
            (offer.owner () == offer.issueOut ().account)
? ownerGives //要约所有人是发行人；他们有无限的资金
            : offers.ownerFunds ();

        if (funds < ownerGives)
        {
//我们已经知道了offer.owner（）！=offer.issueout（）。帐户
            ownerGives = funds;
            stpAmt.out = mulRatio (
                /*Ergives，Quality_One，Ofroutrate，/*Roundup*/false）；
            oframt=ofrq->ceil_out（oframt，stpamt.out）；
            stpamt.in=多比率（
                Oframt.in，Ofrinrate，Quality_One，/*圆形*/ true);

        }

        offerAttempted = true;
        if (!callback (
            offer, ofrAmt, stpAmt, ownerGives, ofrInRate, ofrOutRate))
            break;
    }

    return {offers.permToRemove (), counter.count()};
}

template <class TIn, class TOut, class TDerived>
void BookStep<TIn, TOut, TDerived>::consumeOffer (
    PaymentSandbox& sb,
    TOffer<TIn, TOut>& offer,
    TAmounts<TIn, TOut> const& ofrAmt,
    TAmounts<TIn, TOut> const& stepAmt,
    TOut const& ownerGives) const
{
//报价的所有者得到了Oframt。oframt和stepamt的区别
//是转到book\in.帐户的转账费
    {
        auto const dr = accountSend (sb, book_.in.account, offer.owner (),
            toSTAmount (ofrAmt.in, book_.in), j_);
        if (dr != tesSUCCESS)
            Throw<FlowException> (dr);
    }

//要约所有人支付“所有者赠予”。所有者与
//stepamt是一笔转到book_uuu.out.account的转账费用。
    {
        auto const cr = accountSend (sb, offer.owner (), book_.out.account,
            toSTAmount (ownerGives, book_.out), j_);
        if (cr != tesSUCCESS)
            Throw<FlowException> (cr);
    }

    offer.consume (sb, ofrAmt);
}

template<class TCollection>
static
auto sum (TCollection const& col)
{
    using TResult = std::decay_t<decltype (*col.begin ())>;
    if (col.empty ())
        return TResult{beast::zero};
    return std::accumulate (col.begin () + 1, col.end (), *col.begin ());
};

template<class TIn, class TOut, class TDerived>
std::pair<TIn, TOut>
BookStep<TIn, TOut, TDerived>::revImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    TOut const& out)
{
    cache_.reset ();

    TAmounts<TIn, TOut> result (beast::zero, beast::zero);

    auto remainingOut = out;

    boost::container::flat_multiset<TIn> savedIns;
    savedIns.reserve(64);
    boost::container::flat_multiset<TOut> savedOuts;
    savedOuts.reserve(64);

    /*联邦金额将由业主资金调整（可能与报价不同）
      金额-tho始终<=）
      返回“真”继续接收报价，返回“假”停止接收报价。
    **/

    auto eachOffer =
        [&](TOffer<TIn, TOut>& offer,
            TAmounts<TIn, TOut> const& ofrAmt,
            TAmounts<TIn, TOut> const& stpAmt,
            TOut const& ownerGives,
            std::uint32_t transferRateIn,
            std::uint32_t transferRateOut) mutable -> bool
    {
        if (remainingOut <= beast::zero)
            return false;

        if (stpAmt.out <= remainingOut)
        {
            savedIns.insert(stpAmt.in);
            savedOuts.insert(stpAmt.out);
            result = TAmounts<TIn, TOut>(sum (savedIns), sum(savedOuts));
            remainingOut = out - result.out;
            this->consumeOffer (sb, offer, ofrAmt, stpAmt, ownerGives);
//即使付款满意，也要退回真实的信用证。
//我们需要接受这个提议
            return true;
        }
        else
        {
            auto ofrAdjAmt = ofrAmt;
            auto stpAdjAmt = stpAmt;
            auto ownerGivesAdj = ownerGives;
            limitStepOut (offer.quality (), ofrAdjAmt, stpAdjAmt, ownerGivesAdj,
                transferRateIn, transferRateOut, remainingOut);
            remainingOut = beast::zero;
            savedIns.insert (stpAdjAmt.in);
            savedOuts.insert (remainingOut);
            result.in = sum(savedIns);
            result.out = out;
            this->consumeOffer (sb, offer, ofrAdjAmt, stpAdjAmt, ownerGivesAdj);

//如果两个借据金额的尾数相差不到十，那么
//减去它们会得到零的结果。这可能导致支票
//（stpamt.out>remainingout）错误地认为报价将得到资助
//减去Remainingin后。
            if (fix1298(sb.parentCloseTime()))
                return offer.fully_consumed();
            else
                return false;
        }
    };

    {
        auto const prevStepRedeems = prevStep_ && prevStep_->redeems (sb, false);
        auto const r = forEachOffer (sb, afView, prevStepRedeems, eachOffer);
        boost::container::flat_set<uint256> toRm = std::move(std::get<0>(r));
        std::uint32_t const offersConsumed = std::get<1>(r);
        ofrsToRm.insert (boost::container::ordered_unique_range_t{},
            toRm.begin (), toRm.end ());

        if (offersConsumed >= maxOffersToConsume_)
        {
//迭代太多，将此链标记为非活动
            if (!afView.rules().enabled(fix1515))
            {
//不要使用流动性
                cache_.emplace(beast::zero, beast::zero);
                return {beast::zero, beast::zero};
            }

//使用流动性，但使用它将链标记为不活动，因此
//它不用了
            inactive_ = true;
        }
    }

    switch(remainingOut.signum())
    {
        case -1:
        {
//出了大问题
            JLOG (j_.error ())
                << "BookStep remainingOut < 0 " << to_string (remainingOut);
            assert (0);
            cache_.emplace (beast::zero, beast::zero);
            return {beast::zero, beast::zero};
        }
        case 0:
        {
//由于标准化，remainingout可以为零
//result.out==输出。force result.out==在这种情况下为out
            result.out = out;
        }
    }

    cache_.emplace (result.in, result.out);
    return {result.in, result.out};
}

template<class TIn, class TOut, class TDerived>
std::pair<TIn, TOut>
BookStep<TIn, TOut, TDerived>::fwdImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    TIn const& in)
{
    assert(cache_);

    TAmounts<TIn, TOut> result (beast::zero, beast::zero);

    auto remainingIn = in;

    boost::container::flat_multiset<TIn> savedIns;
    savedIns.reserve(64);
    boost::container::flat_multiset<TOut> savedOuts;
    savedOuts.reserve(64);

//联邦金额将由业主资金调整（可能与报价不同）
//金额-tho始终<=）
    auto eachOffer =
        [&](TOffer<TIn, TOut>& offer,
            TAmounts<TIn, TOut> const& ofrAmt,
            TAmounts<TIn, TOut> const& stpAmt,
            TOut const& ownerGives,
            std::uint32_t transferRateIn,
            std::uint32_t transferRateOut) mutable -> bool
    {
        assert(cache_);

        if (remainingIn <= beast::zero)
            return false;

        bool processMore = true;
        auto ofrAdjAmt = ofrAmt;
        auto stpAdjAmt = stpAmt;
        auto ownerGivesAdj = ownerGives;

        typename boost::container::flat_multiset<TOut>::const_iterator lastOut;
        if (stpAmt.in <= remainingIn)
        {
            savedIns.insert(stpAmt.in);
            lastOut = savedOuts.insert(stpAmt.out);
            result = TAmounts<TIn, TOut>(sum (savedIns), sum(savedOuts));
//即使stepamt.in==remainingin，也要接受报价
            processMore = true;
        }
        else
        {
            limitStepIn (offer.quality (), ofrAdjAmt, stpAdjAmt, ownerGivesAdj,
                transferRateIn, transferRateOut, remainingIn);
            savedIns.insert (remainingIn);
            lastOut = savedOuts.insert (stpAdjAmt.out);
            result.out = sum (savedOuts);
            result.in = in;

            processMore = false;
        }

        if (result.out > cache_->out && result.in <= cache_->in)
        {
//这一步在向前通过时产生的输出比
//使用相同的输入（或更少）时反向通过。如果我们
//计算生成缓存输出所需的输入
//（在反向步骤中产生）且输入等于
//在前进步骤中消耗的输入，然后消耗
//向前步骤中提供的输入并生成输出
//从反向步骤请求。
            auto const lastOutAmt = *lastOut;
            savedOuts.erase(lastOut);
            auto const remainingOut = cache_->out - sum (savedOuts);
            auto ofrAdjAmtRev = ofrAmt;
            auto stpAdjAmtRev = stpAmt;
            auto ownerGivesAdjRev = ownerGives;
            limitStepOut (offer.quality (), ofrAdjAmtRev, stpAdjAmtRev,
                ownerGivesAdjRev, transferRateIn, transferRateOut,
                remainingOut);

            if (stpAdjAmtRev.in == remainingIn)
            {
                result.in = in;
                result.out = cache_->out;

                savedIns.clear();
                savedIns.insert(result.in);
                savedOuts.clear();
                savedOuts.insert(result.out);

                ofrAdjAmt = ofrAdjAmtRev;
                stpAdjAmt.in = remainingIn;
                stpAdjAmt.out = remainingOut;
                ownerGivesAdj = ownerGivesAdjRev;
            }
            else
            {
//这很可能是个问题，会被抓住的
//以后再检查
                savedOuts.insert (lastOutAmt);
            }
        }

        remainingIn = in - result.in;
        this->consumeOffer (sb, offer, ofrAdjAmt, stpAdjAmt, ownerGivesAdj);

//如果两个借据金额的尾数相差不到十，那么
//减去它们会得到零的结果。这可能导致支票
//（stpamt.in>remainingin）错误地认为报价将得到资助
//减去Remainingin后。
        if (fix1298(sb.parentCloseTime()))
            processMore = processMore || offer.fully_consumed();

        return processMore;
    };

    {
        auto const prevStepRedeems = prevStep_ && prevStep_->redeems (sb, true);
        auto const r = forEachOffer (sb, afView, prevStepRedeems, eachOffer);
        boost::container::flat_set<uint256> toRm = std::move(std::get<0>(r));
        std::uint32_t const offersConsumed = std::get<1>(r);
        ofrsToRm.insert (boost::container::ordered_unique_range_t{},
            toRm.begin (), toRm.end ());

        if (offersConsumed >= maxOffersToConsume_)
        {
//迭代次数太多，将此股标记为不活动（干）
            if (!afView.rules().enabled(fix1515))
            {
//不要使用流动性
                cache_.emplace(beast::zero, beast::zero);
                return {beast::zero, beast::zero};
            }

//使用流动性，但使用它将链标记为不活动，因此
//它不用了
            inactive_ = true;
        }
    }

    switch(remainingIn.signum())
    {
        case -1:
        {
//出了大问题
            JLOG (j_.error ())
                << "BookStep remainingIn < 0 " << to_string (remainingIn);
            assert (0);
            cache_.emplace (beast::zero, beast::zero);
            return {beast::zero, beast::zero};
        }
        case 0:
        {
//由于标准化，remainingin可以为零
//result.in==英寸。对于这种情况，force result.in==in
            result.in = in;
        }
    }

    cache_.emplace (result.in, result.out);
    return {result.in, result.out};
}

template<class TIn, class TOut, class TDerived>
std::pair<bool, EitherAmount>
BookStep<TIn, TOut, TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.trace()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (TOut (beast::zero))};
    }

    auto const savCache = *cache_;

    try
    {
        boost::container::flat_set<uint256> dummy;
fwdImp (sb, afView, dummy, get<TIn> (in));  //更改缓存
    }
    catch (FlowException const&)
    {
        return {false, EitherAmount (TOut (beast::zero))};
    }

    if (!(checkNear (savCache.in, cache_->in) &&
            checkNear (savCache.out, cache_->out)))
    {
        JLOG (j_.error()) <<
            "Strand re-execute check failed." <<
            " ExpectedIn: " << to_string (savCache.in) <<
            " CachedIn: " << to_string (cache_->in) <<
            " ExpectedOut: " << to_string (savCache.out) <<
            " CachedOut: " << to_string (cache_->out);
        return {false, EitherAmount (cache_->out)};
    }
    return {true, EitherAmount (cache_->out)};
}

template<class TIn, class TOut, class TDerived>
TER
BookStep<TIn, TOut, TDerived>::check(StrandContext const& ctx) const
{
    if (book_.in == book_.out)
    {
        JLOG (j_.debug()) << "BookStep: Book with same in and out issuer " << *this;
        return temBAD_PATH;
    }
    if (!isConsistent (book_.in) || !isConsistent (book_.out))
    {
        JLOG (j_.debug()) << "Book: currency is inconsistent with issuer." << *this;
        return temBAD_PATH;
    }

//不允许两本书输出同一期。这可能导致
//一步到位，另一步到位。
    if (!ctx.seenBookOuts.insert (book_.out).second ||
        ctx.seenDirectIssues[0].count (book_.out))
    {
        JLOG (j_.debug()) << "BookStep: loop detected: " << *this;
        return temBAD_PATH_LOOP;
    }

    if (ctx.view.rules().enabled(fix1373) &&
        ctx.seenDirectIssues[1].count(book_.out))
    {
        JLOG(j_.debug()) << "BookStep: loop detected: " << *this;
        return temBAD_PATH_LOOP;
    }

    if (fix1443(ctx.view.info().parentCloseTime))
    {
        if (ctx.prevStep)
        {
            if (auto const prev = ctx.prevStep->directStepSrcAcct())
            {
                auto const& view = ctx.view;
                auto const& cur = book_.in.account;

                auto sle =
                    view.read(keylet::line(*prev, cur, book_.in.currency));
                if (!sle)
                    return terNO_LINE;
                if ((*sle)[sfFlags] &
                    ((cur > *prev) ? lsfHighNoRipple : lsfLowNoRipple))
                    return terNO_RIPPLE;
            }
        }
    }

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace test
{
//测试所需

template <class TIn, class TOut, class TDerived>
static
bool equalHelper (Step const& step, ripple::Book const& book)
{
    if (auto bs = dynamic_cast<BookStep<TIn, TOut, TDerived> const*> (&step))
        return book == bs->book ();
    return false;
}

bool bookStepEqual (Step const& step, ripple::Book const& book)
{
    bool const inXRP = isXRP (book.in.currency);
    bool const outXRP = isXRP (book.out.currency);
    if (inXRP && outXRP)
        return equalHelper<XRPAmount, XRPAmount,
            BookPaymentStep<XRPAmount, XRPAmount>> (step, book);
    if (inXRP && !outXRP)
        return equalHelper<XRPAmount, IOUAmount,
            BookPaymentStep<XRPAmount, IOUAmount>> (step, book);
    if (!inXRP && outXRP)
        return equalHelper<IOUAmount, XRPAmount,
            BookPaymentStep<IOUAmount, XRPAmount>> (step, book);
    if (!inXRP && !outXRP)
        return equalHelper<IOUAmount, IOUAmount,
            BookPaymentStep<IOUAmount, IOUAmount>> (step, book);
    return false;
}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class TIn, class TOut>
static
std::pair<TER, std::unique_ptr<Step>>
make_BookStepHelper (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<BookOfferCrossingStep<TIn, TOut>> (ctx, in, out);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
else //付款
    {
        auto paymentStep =
            std::make_unique<BookPaymentStep<TIn, TOut>> (ctx, in, out);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move(r)};
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepII (
    StrandContext const& ctx,
    Issue const& in,
    Issue const& out)
{
    return make_BookStepHelper<IOUAmount, IOUAmount> (ctx, in, out);
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepIX (
    StrandContext const& ctx,
    Issue const& in)
{
    return make_BookStepHelper<IOUAmount, XRPAmount> (ctx, in, xrpIssue());
}

std::pair<TER, std::unique_ptr<Step>>
make_BookStepXI (
    StrandContext const& ctx,
    Issue const& out)
{
    return make_BookStepHelper<XRPAmount, IOUAmount> (ctx, xrpIssue(), out);
}

} //涟漪
