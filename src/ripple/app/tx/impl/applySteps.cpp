
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

#include <ripple/app/tx/applySteps.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/app/tx/impl/CancelCheck.h>
#include <ripple/app/tx/impl/CancelOffer.h>
#include <ripple/app/tx/impl/CancelTicket.h>
#include <ripple/app/tx/impl/CashCheck.h>
#include <ripple/app/tx/impl/Change.h>
#include <ripple/app/tx/impl/CreateCheck.h>
#include <ripple/app/tx/impl/CreateOffer.h>
#include <ripple/app/tx/impl/CreateTicket.h>
#include <ripple/app/tx/impl/DepositPreauth.h>
#include <ripple/app/tx/impl/Escrow.h>
#include <ripple/app/tx/impl/Payment.h>
#include <ripple/app/tx/impl/SetAccount.h>
#include <ripple/app/tx/impl/SetRegularKey.h>
#include <ripple/app/tx/impl/SetSignerList.h>
#include <ripple/app/tx/impl/SetTrust.h>
#include <ripple/app/tx/impl/PayChan.h>

namespace ripple {

static
NotTEC
invoke_preflight (PreflightContext const& ctx)
{
    switch(ctx.tx.getTxnType())
    {
    case ttACCOUNT_SET:     return SetAccount       ::preflight(ctx);
    case ttCHECK_CANCEL:    return CancelCheck      ::preflight(ctx);
    case ttCHECK_CASH:      return CashCheck        ::preflight(ctx);
    case ttCHECK_CREATE:    return CreateCheck      ::preflight(ctx);
    case ttDEPOSIT_PREAUTH: return DepositPreauth   ::preflight(ctx);
    case ttOFFER_CANCEL:    return CancelOffer      ::preflight(ctx);
    case ttOFFER_CREATE:    return CreateOffer      ::preflight(ctx);
    case ttESCROW_CREATE:   return EscrowCreate     ::preflight(ctx);
    case ttESCROW_FINISH:   return EscrowFinish     ::preflight(ctx);
    case ttESCROW_CANCEL:   return EscrowCancel     ::preflight(ctx);
    case ttPAYCHAN_CLAIM:   return PayChanClaim     ::preflight(ctx);
    case ttPAYCHAN_CREATE:  return PayChanCreate    ::preflight(ctx);
    case ttPAYCHAN_FUND:    return PayChanFund      ::preflight(ctx);
    case ttPAYMENT:         return Payment          ::preflight(ctx);
    case ttREGULAR_KEY_SET: return SetRegularKey    ::preflight(ctx);
    case ttSIGNER_LIST_SET: return SetSignerList    ::preflight(ctx);
    case ttTICKET_CANCEL:   return CancelTicket     ::preflight(ctx);
    case ttTICKET_CREATE:   return CreateTicket     ::preflight(ctx);
    case ttTRUST_SET:       return SetTrust         ::preflight(ctx);
    case ttAMENDMENT:
    case ttFEE:             return Change           ::preflight(ctx);
    default:
        assert(false);
        return temUNKNOWN;
    }
}

/*invoke_preclaim<t>使用名称隐藏来完成
    静态（大概）的编译时多态性
    事务处理类和派生类的类函数。
**/

template<class T>
static
TER
invoke_preclaim(PreclaimContext const& ctx)
{
//如果交易人要求有效的账户，而交易没有
//首先，飞行前已经有一个故障标志。
    auto const id = ctx.tx.getAccountID(sfAccount);

    if (id != beast::zero)
    {
        TER result = T::checkSeq(ctx);

        if (result != tesSUCCESS)
            return result;

        result = T::checkFee(ctx,
            calculateBaseFee(ctx.view, ctx.tx));

        if (result != tesSUCCESS)
            return result;

        result = T::checkSign(ctx);

        if (result != tesSUCCESS)
            return result;

    }

    return T::preclaim(ctx);
}

static
TER
invoke_preclaim (PreclaimContext const& ctx)
{
    switch(ctx.tx.getTxnType())
    {
    case ttACCOUNT_SET:     return invoke_preclaim<SetAccount>(ctx);
    case ttCHECK_CANCEL:    return invoke_preclaim<CancelCheck>(ctx);
    case ttCHECK_CASH:      return invoke_preclaim<CashCheck>(ctx);
    case ttCHECK_CREATE:    return invoke_preclaim<CreateCheck>(ctx);
    case ttDEPOSIT_PREAUTH: return invoke_preclaim<DepositPreauth>(ctx);
    case ttOFFER_CANCEL:    return invoke_preclaim<CancelOffer>(ctx);
    case ttOFFER_CREATE:    return invoke_preclaim<CreateOffer>(ctx);
    case ttESCROW_CREATE:   return invoke_preclaim<EscrowCreate>(ctx);
    case ttESCROW_FINISH:   return invoke_preclaim<EscrowFinish>(ctx);
    case ttESCROW_CANCEL:   return invoke_preclaim<EscrowCancel>(ctx);
    case ttPAYCHAN_CLAIM:   return invoke_preclaim<PayChanClaim>(ctx);
    case ttPAYCHAN_CREATE:  return invoke_preclaim<PayChanCreate>(ctx);
    case ttPAYCHAN_FUND:    return invoke_preclaim<PayChanFund>(ctx);
    case ttPAYMENT:         return invoke_preclaim<Payment>(ctx);
    case ttREGULAR_KEY_SET: return invoke_preclaim<SetRegularKey>(ctx);
    case ttSIGNER_LIST_SET: return invoke_preclaim<SetSignerList>(ctx);
    case ttTICKET_CANCEL:   return invoke_preclaim<CancelTicket>(ctx);
    case ttTICKET_CREATE:   return invoke_preclaim<CreateTicket>(ctx);
    case ttTRUST_SET:       return invoke_preclaim<SetTrust>(ctx);
    case ttAMENDMENT:
    case ttFEE:             return invoke_preclaim<Change>(ctx);
    default:
        assert(false);
        return temUNKNOWN;
    }
}

static
std::uint64_t
invoke_calculateBaseFee(
    ReadView const& view,
    STTx const& tx)
{
    switch (tx.getTxnType())
    {
    case ttACCOUNT_SET:     return SetAccount::calculateBaseFee(view, tx);
    case ttCHECK_CANCEL:    return CancelCheck::calculateBaseFee(view, tx);
    case ttCHECK_CASH:      return CashCheck::calculateBaseFee(view, tx);
    case ttCHECK_CREATE:    return CreateCheck::calculateBaseFee(view, tx);
    case ttDEPOSIT_PREAUTH: return DepositPreauth::calculateBaseFee(view, tx);
    case ttOFFER_CANCEL:    return CancelOffer::calculateBaseFee(view, tx);
    case ttOFFER_CREATE:    return CreateOffer::calculateBaseFee(view, tx);
    case ttESCROW_CREATE:   return EscrowCreate::calculateBaseFee(view, tx);
    case ttESCROW_FINISH:   return EscrowFinish::calculateBaseFee(view, tx);
    case ttESCROW_CANCEL:   return EscrowCancel::calculateBaseFee(view, tx);
    case ttPAYCHAN_CLAIM:   return PayChanClaim::calculateBaseFee(view, tx);
    case ttPAYCHAN_CREATE:  return PayChanCreate::calculateBaseFee(view, tx);
    case ttPAYCHAN_FUND:    return PayChanFund::calculateBaseFee(view, tx);
    case ttPAYMENT:         return Payment::calculateBaseFee(view, tx);
    case ttREGULAR_KEY_SET: return SetRegularKey::calculateBaseFee(view, tx);
    case ttSIGNER_LIST_SET: return SetSignerList::calculateBaseFee(view, tx);
    case ttTICKET_CANCEL:   return CancelTicket::calculateBaseFee(view, tx);
    case ttTICKET_CREATE:   return CreateTicket::calculateBaseFee(view, tx);
    case ttTRUST_SET:       return SetTrust::calculateBaseFee(view, tx);
    case ttAMENDMENT:
    case ttFEE:             return Change::calculateBaseFee(view, tx);
    default:
        assert(false);
        return 0;
    }
}

template<class T>
static
TxConsequences
invoke_calculateConsequences(STTx const& tx)
{
    auto const category = T::affectsSubsequentTransactionAuth(tx) ?
        TxConsequences::blocker : TxConsequences::normal;
    auto const feePaid = T::calculateFeePaid(tx);
    auto const maxSpend = T::calculateMaxSpend(tx);

    return{ category, feePaid, maxSpend };
}

static
TxConsequences
invoke_calculateConsequences(STTx const& tx)
{
    switch (tx.getTxnType())
    {
    case ttACCOUNT_SET:     return invoke_calculateConsequences<SetAccount>(tx);
    case ttCHECK_CANCEL:    return invoke_calculateConsequences<CancelCheck>(tx);
    case ttCHECK_CASH:      return invoke_calculateConsequences<CashCheck>(tx);
    case ttCHECK_CREATE:    return invoke_calculateConsequences<CreateCheck>(tx);
    case ttDEPOSIT_PREAUTH: return invoke_calculateConsequences<DepositPreauth>(tx);
    case ttOFFER_CANCEL:    return invoke_calculateConsequences<CancelOffer>(tx);
    case ttOFFER_CREATE:    return invoke_calculateConsequences<CreateOffer>(tx);
    case ttESCROW_CREATE:   return invoke_calculateConsequences<EscrowCreate>(tx);
    case ttESCROW_FINISH:   return invoke_calculateConsequences<EscrowFinish>(tx);
    case ttESCROW_CANCEL:   return invoke_calculateConsequences<EscrowCancel>(tx);
    case ttPAYCHAN_CLAIM:   return invoke_calculateConsequences<PayChanClaim>(tx);
    case ttPAYCHAN_CREATE:  return invoke_calculateConsequences<PayChanCreate>(tx);
    case ttPAYCHAN_FUND:    return invoke_calculateConsequences<PayChanFund>(tx);
    case ttPAYMENT:         return invoke_calculateConsequences<Payment>(tx);
    case ttREGULAR_KEY_SET: return invoke_calculateConsequences<SetRegularKey>(tx);
    case ttSIGNER_LIST_SET: return invoke_calculateConsequences<SetSignerList>(tx);
    case ttTICKET_CANCEL:   return invoke_calculateConsequences<CancelTicket>(tx);
    case ttTICKET_CREATE:   return invoke_calculateConsequences<CreateTicket>(tx);
    case ttTRUST_SET:       return invoke_calculateConsequences<SetTrust>(tx);
    case ttAMENDMENT:
    case ttFEE:
//陷入违约
    default:
        assert(false);
        return { TxConsequences::blocker, Transactor::calculateFeePaid(tx),
            beast::zero };
    }
}

static
std::pair<TER, bool>
invoke_apply (ApplyContext& ctx)
{
    switch(ctx.tx.getTxnType())
    {
    case ttACCOUNT_SET:     { SetAccount     p(ctx); return p(); }
    case ttCHECK_CANCEL:    { CancelCheck    p(ctx); return p(); }
    case ttCHECK_CASH:      { CashCheck      p(ctx); return p(); }
    case ttCHECK_CREATE:    { CreateCheck    p(ctx); return p(); }
    case ttDEPOSIT_PREAUTH: { DepositPreauth p(ctx); return p(); }
    case ttOFFER_CANCEL:    { CancelOffer    p(ctx); return p(); }
    case ttOFFER_CREATE:    { CreateOffer    p(ctx); return p(); }
    case ttESCROW_CREATE:   { EscrowCreate   p(ctx); return p(); }
    case ttESCROW_FINISH:   { EscrowFinish   p(ctx); return p(); }
    case ttESCROW_CANCEL:   { EscrowCancel   p(ctx); return p(); }
    case ttPAYCHAN_CLAIM:   { PayChanClaim   p(ctx); return p(); }
    case ttPAYCHAN_CREATE:  { PayChanCreate  p(ctx); return p(); }
    case ttPAYCHAN_FUND:    { PayChanFund    p(ctx); return p(); }
    case ttPAYMENT:         { Payment        p(ctx); return p(); }
    case ttREGULAR_KEY_SET: { SetRegularKey  p(ctx); return p(); }
    case ttSIGNER_LIST_SET: { SetSignerList  p(ctx); return p(); }
    case ttTICKET_CANCEL:   { CancelTicket   p(ctx); return p(); }
    case ttTICKET_CREATE:   { CreateTicket   p(ctx); return p(); }
    case ttTRUST_SET:       { SetTrust       p(ctx); return p(); }
    case ttAMENDMENT:
    case ttFEE:             { Change         p(ctx); return p(); }
    default:
        assert(false);
        return { temUNKNOWN, false };
    }
}

PreflightResult
preflight(Application& app, Rules const& rules,
    STTx const& tx, ApplyFlags flags,
        beast::Journal j)
{
    PreflightContext const pfctx(app, tx,
        rules, flags, j);
    try
    {
        return{ pfctx, invoke_preflight(pfctx) };
    }
    catch (std::exception const& e)
    {
        JLOG(j.fatal()) <<
            "apply: " << e.what();
        return{ pfctx, tefEXCEPTION };
    }
}

PreclaimResult
preclaim (PreflightResult const& preflightResult,
    Application& app, OpenView const& view)
{
    boost::optional<PreclaimContext const> ctx;
    if (preflightResult.rules != view.rules())
    {
        auto secondFlight = preflight(app, view.rules(),
            preflightResult.tx, preflightResult.flags,
                preflightResult.j);
        ctx.emplace(app, view, secondFlight.ter, secondFlight.tx,
            secondFlight.flags, secondFlight.j);
    }
    else
    {
        ctx.emplace(
            app, view, preflightResult.ter, preflightResult.tx,
                preflightResult.flags, preflightResult.j);
    }
    try
    {
        if (ctx->preflightResult != tesSUCCESS)
            return { *ctx, ctx->preflightResult };
        return{ *ctx, invoke_preclaim(*ctx) };
    }
    catch (std::exception const& e)
    {
        JLOG(ctx->j.fatal()) <<
            "apply: " << e.what();
        return{ *ctx, tefEXCEPTION };
    }
}

std::uint64_t
calculateBaseFee(ReadView const& view,
    STTx const& tx)
{
    return invoke_calculateBaseFee(view, tx);
}

TxConsequences
calculateConsequences(PreflightResult const& preflightResult)
{
    assert(preflightResult.ter == tesSUCCESS);
    if (preflightResult.ter != tesSUCCESS)
        return{ TxConsequences::blocker,
            Transactor::calculateFeePaid(preflightResult.tx),
                beast::zero };
    return invoke_calculateConsequences(preflightResult.tx);
}

std::pair<TER, bool>
doApply(PreclaimResult const& preclaimResult,
    Application& app, OpenView& view)
{
    if (preclaimResult.view.seq() != view.seq())
    {
//来自调用方的逻辑错误。没有足够的
//要恢复的信息。
        return{ tefEXCEPTION, false };
    }
    try
    {
        if (!preclaimResult.likelyToClaimFee)
            return{ preclaimResult.ter, false };
        ApplyContext ctx(app, view,
            preclaimResult.tx, preclaimResult.ter,
                calculateBaseFee(view, preclaimResult.tx),
                    preclaimResult.flags, preclaimResult.j);
        return invoke_apply(ctx);
    }
    catch (std::exception const& e)
    {
        JLOG(preclaimResult.j.fatal()) <<
            "apply: " << e.what();
        return { tefEXCEPTION, false };
    }
}

} //涟漪
