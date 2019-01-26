
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
#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/app/paths/impl/Steps.h>
#include <ripple/app/paths/impl/StepChecks.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/XRPAmount.h>

#include <boost/container/flat_set.hpp>

#include <numeric>
#include <sstream>

namespace ripple {

template <class TDerived>
class XRPEndpointStep : public StepImp<
    XRPAmount, XRPAmount, XRPEndpointStep<TDerived>>
{
private:
    AccountID acc_;
    bool const isLast_;
    beast::Journal j_;

//因为此步骤始终是链中的端点
//（第一步或最后一步）使用相同的缓存
//对于cachedin和cachedout，将只使用一个
    boost::optional<XRPAmount> cache_;

    boost::optional<EitherAmount>
    cached () const
    {
        if (!cache_)
            return boost::none;
        return EitherAmount (*cache_);
    }

public:
    XRPEndpointStep (
        StrandContext const& ctx,
        AccountID const& acc)
            : acc_(acc)
            , isLast_(ctx.isLast)
            , j_ (ctx.j) {}

    AccountID const& acc () const
    {
        return acc_;
    }

    boost::optional<std::pair<AccountID,AccountID>>
    directStepAccts () const override
    {
        if (isLast_)
            return std::make_pair(xrpAccount(), acc_);
        return std::make_pair(acc_, xrpAccount());
    }

    boost::optional<EitherAmount>
    cachedIn () const override
    {
        return cached ();
    }

    boost::optional<EitherAmount>
    cachedOut () const override
    {
        return cached ();
    }

    boost::optional<Quality>
    qualityUpperBound(ReadView const& v, bool& redeems) const override;

    std::pair<XRPAmount, XRPAmount>
    revImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        XRPAmount const& out);

    std::pair<XRPAmount, XRPAmount>
    fwdImp (
        PaymentSandbox& sb,
        ApplyView& afView,
        boost::container::flat_set<uint256>& ofrsToRm,
        XRPAmount const& in);

    std::pair<bool, EitherAmount>
    validFwd (
        PaymentSandbox& sb,
        ApplyView& afView,
        EitherAmount const& in) override;

//检查是否存在错误和违反冻结约束的情况。
    TER check (StrandContext const& ctx) const;

protected:
    XRPAmount
    xrpLiquidImpl (ReadView& sb, std::int32_t reserveReduction) const
    {
        return ripple::xrpLiquid (sb, acc_, reserveReduction, j_);
    }

    std::string logStringImpl (char const* name) const
    {
        std::ostringstream ostr;
        ostr <<
            name << ": " <<
            "\nAcc: " << acc_;
        return ostr.str ();
    }

private:
    template <class P>
    friend bool operator==(
        XRPEndpointStep<P> const& lhs,
        XRPEndpointStep<P> const& rhs);

    friend bool operator!=(
        XRPEndpointStep const& lhs,
        XRPEndpointStep const& rhs)
    {
        return ! (lhs == rhs);
    }

    bool equal (Step const& rhs) const override
    {
        if (auto ds = dynamic_cast<XRPEndpointStep const*> (&rhs))
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

//付款xrpendpointstep类（不提供交叉）。
class XRPEndpointPaymentStep : public XRPEndpointStep<XRPEndpointPaymentStep>
{
public:
    explicit XRPEndpointPaymentStep() = default;

    using XRPEndpointStep<XRPEndpointPaymentStep>::XRPEndpointStep;

    XRPAmount
    xrpLiquid (ReadView& sb) const
    {
        return xrpLiquidImpl (sb, 0);;
    }

    std::string logString () const override
    {
        return logStringImpl ("XRPEndpointPaymentStep");
    }
};

//提供交叉xrpendpointstep类（不是付款）。
class XRPEndpointOfferCrossingStep :
    public XRPEndpointStep<XRPEndpointOfferCrossingStep>
{
private:

//出于历史原因，允许报价交叉进一步挖掘
//存入xrp储备，而不是普通支付。（我相信这是
//因为信任行是在删除xrp之后创建的。）
//返回应减少的准备金。
//
//请注意，只有当信托额度没有
//当前存在。
    static std::int32_t computeReserveReduction (
        StrandContext const& ctx, AccountID const& acc)
    {
        if (ctx.isFirst &&
            !ctx.view.read (keylet::line (acc, ctx.strandDeliver)))
                return -1;
        return 0;
    }

public:
    XRPEndpointOfferCrossingStep (
        StrandContext const& ctx, AccountID const& acc)
    : XRPEndpointStep<XRPEndpointOfferCrossingStep> (ctx, acc)
    , reserveReduction_ (computeReserveReduction (ctx, acc))
    {
    }

    XRPAmount
    xrpLiquid (ReadView& sb) const
    {
        return xrpLiquidImpl (sb, reserveReduction_);
    }

    std::string logString () const override
    {
        return logStringImpl ("XRPEndpointOfferCrossingStep");
    }

private:
    std::int32_t const reserveReduction_;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class TDerived>
inline bool operator==(XRPEndpointStep<TDerived> const& lhs,
    XRPEndpointStep<TDerived> const& rhs)
{
    return lhs.acc_ == rhs.acc_ && lhs.isLast_ == rhs.isLast_;
}

template <class TDerived>
boost::optional<Quality>
XRPEndpointStep<TDerived>::qualityUpperBound(
    ReadView const& v, bool& redeems) const
{
    redeems = this->redeems(v, true);
    return Quality{STAmount::uRateOne};
}


template <class TDerived>
std::pair<XRPAmount, XRPAmount>
XRPEndpointStep<TDerived>::revImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    XRPAmount const& out)
{
    auto const balance = static_cast<TDerived const*>(this)->xrpLiquid (sb);

    auto const result = isLast_ ? out : std::min (balance, out);

    auto& sender = isLast_ ? xrpAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : xrpAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {XRPAmount{beast::zero}, XRPAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<XRPAmount, XRPAmount>
XRPEndpointStep<TDerived>::fwdImp (
    PaymentSandbox& sb,
    ApplyView& afView,
    boost::container::flat_set<uint256>& ofrsToRm,
    XRPAmount const& in)
{
    assert (cache_);
    auto const balance = static_cast<TDerived const*>(this)->xrpLiquid (sb);

    auto const result = isLast_ ? in : std::min (balance, in);

    auto& sender = isLast_ ? xrpAccount() : acc_;
    auto& receiver = isLast_ ? acc_ : xrpAccount();
    auto ter   = accountSend (sb, sender, receiver, toSTAmount (result), j_);
    if (ter != tesSUCCESS)
        return {XRPAmount{beast::zero}, XRPAmount{beast::zero}};

    cache_.emplace (result);
    return {result, result};
}

template <class TDerived>
std::pair<bool, EitherAmount>
XRPEndpointStep<TDerived>::validFwd (
    PaymentSandbox& sb,
    ApplyView& afView,
    EitherAmount const& in)
{
    if (!cache_)
    {
        JLOG (j_.error()) << "Expected valid cache in validFwd";
        return {false, EitherAmount (XRPAmount (beast::zero))};
    }

    assert (in.native);

    auto const& xrpIn = in.xrp;
    auto const balance = static_cast<TDerived const*>(this)->xrpLiquid (sb);

    if (!isLast_ && balance < xrpIn)
    {
        JLOG (j_.error()) << "XRPEndpointStep: Strand re-execute check failed."
            << " Insufficient balance: " << to_string (balance)
            << " Requested: " << to_string (xrpIn);
        return {false, EitherAmount (balance)};
    }

    if (xrpIn != *cache_)
    {
        JLOG (j_.error()) << "XRPEndpointStep: Strand re-execute check failed."
            << " ExpectedIn: " << to_string (*cache_)
            << " CachedIn: " << to_string (xrpIn);
    }
    return {true, in};
}

template <class TDerived>
TER
XRPEndpointStep<TDerived>::check (StrandContext const& ctx) const
{
    if (!acc_)
    {
        JLOG (j_.debug()) << "XRPEndpointStep: specified bad account.";
        return temBAD_PATH;
    }

    auto sleAcc = ctx.view.read (keylet::account (acc_));
    if (!sleAcc)
    {
        JLOG (j_.warn()) << "XRPEndpointStep: can't send or receive XRP from "
                             "non-existent account: "
                          << acc_;
        return terNO_ACCOUNT;
    }

    if (!ctx.isFirst && !ctx.isLast)
    {
        return temBAD_PATH;
    }

    auto& src = isLast_ ? xrpAccount () : acc_;
    auto& dst = isLast_ ? acc_ : xrpAccount();
    auto ter = checkFreeze (ctx.view, src, dst, xrpCurrency ());
    if (ter != tesSUCCESS)
        return ter;

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace test
{
//测试所需
bool xrpEndpointStepEqual (Step const& step, AccountID const& acc)
{
    if (auto xs =
        dynamic_cast<XRPEndpointStep<XRPEndpointPaymentStep> const*> (&step))
    {
        return xs->acc () == acc;
    }
    return false;
}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::pair<TER, std::unique_ptr<Step>>
make_XRPEndpointStep (
    StrandContext const& ctx,
    AccountID const& acc)
{
    TER ter = tefINTERNAL;
    std::unique_ptr<Step> r;
    if (ctx.offerCrossing)
    {
        auto offerCrossingStep =
            std::make_unique<XRPEndpointOfferCrossingStep> (ctx, acc);
        ter = offerCrossingStep->check (ctx);
        r = std::move (offerCrossingStep);
    }
else //付款
    {
        auto paymentStep =
            std::make_unique<XRPEndpointPaymentStep> (ctx, acc);
        ter = paymentStep->check (ctx);
        r = std::move (paymentStep);
    }
    if (ter != tesSUCCESS)
        return {ter, nullptr};

    return {tesSUCCESS, std::move (r)};
}

} //涟漪
