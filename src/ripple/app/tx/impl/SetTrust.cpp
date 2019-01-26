
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

#include <ripple/app/tx/impl/SetTrust.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/st.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
SetTrust::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfTrustSetMask)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    STAmount const saLimitAmount (tx.getFieldAmount (sfLimitAmount));

    if (!isLegalNet (saLimitAmount))
        return temBAD_AMOUNT;

    if (saLimitAmount.native ())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: specifies native limit " <<
            saLimitAmount.getFullText ();
        return temBAD_LIMIT;
    }

    if (badCurrency() == saLimitAmount.getCurrency ())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: specifies XRP as IOU";
        return temBAD_CURRENCY;
    }

    if (saLimitAmount < beast::zero)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Negative credit limit.";
        return temBAD_LIMIT;
    }

//检查目的地是否合理。
    auto const& issuer = saLimitAmount.getIssuer ();

    if (!issuer || issuer == noAccount())
    {
        JLOG(j.trace()) <<
            "Malformed transaction: no destination account.";
        return temDST_NEEDED;
    }

    return preflight2 (ctx);
}

TER
SetTrust::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];

    auto const sle = ctx.view.read(
        keylet::account(id));

    std::uint32_t const uTxFlags = ctx.tx.getFlags();

    bool const bSetAuth = (uTxFlags & tfSetfAuth);

    if (bSetAuth && !(sle->getFieldU32(sfFlags) & lsfRequireAuth))
    {
        JLOG(ctx.j.trace()) <<
            "Retry: Auth not required.";
        return tefNO_AUTH_REQUIRED;
    }

    auto const saLimitAmount = ctx.tx[sfLimitAmount];

    auto const currency = saLimitAmount.getCurrency();
    auto const uDstAccountID = saLimitAmount.getIssuer();

    if (id == uDstAccountID)
    {
//防止建立对自己的信任，
//除非已经创造了一个
//（在这种情况下，DOAPPLY将对其进行清理）。
        auto const sleDelete = ctx.view.read(
            keylet::line(id, uDstAccountID, currency));

        if (!sleDelete)
        {
            JLOG(ctx.j.trace()) <<
                "Malformed transaction: Can not extend credit to self.";
            return temDST_IS_SRC;
        }
    }

    return tesSUCCESS;
}

TER
SetTrust::doApply ()
{
    TER terResult = tesSUCCESS;

    STAmount const saLimitAmount (ctx_.tx.getFieldAmount (sfLimitAmount));
    bool const bQualityIn (ctx_.tx.isFieldPresent (sfQualityIn));
    bool const bQualityOut (ctx_.tx.isFieldPresent (sfQualityOut));

    Currency const currency (saLimitAmount.getCurrency ());
    AccountID uDstAccountID (saLimitAmount.getIssuer ());

//是的，如果当前是高帐户。
    bool const bHigh = account_ > uDstAccountID;

    auto const sle = view().peek(
        keylet::account(account_));

    std::uint32_t const uOwnerCount = sle->getFieldU32 (sfOwnerCount);

//创建行所需的保留。注释
//尽管储备随着每一项的增加而增加
//一个账户拥有，在信托额度的情况下，我们
//*强制*如果用户拥有两个以上
//项目。
//
//我们这样做是因为能够兑换货币，
//它需要信任线，是一个强大的纹波功能。
//因此，我们希望让一个网关能够轻松地为
//用户的帐户，不用担心被欺骗。
//
//如果没有这个逻辑，一个希望
//新用户使用其服务时，必须提供
//用户足够的xrp不仅覆盖账户储备
//但信托额度的增量准备金
//好。不打算使用网关的人
//可以将额外的xrp用于自己的目的。

    XRPAmount const reserveCreate ((uOwnerCount < 2)
        ? XRPAmount (beast::zero)
        : view().fees().accountReserve(uOwnerCount + 1));

    std::uint32_t uQualityIn (bQualityIn ? ctx_.tx.getFieldU32 (sfQualityIn) : 0);
    std::uint32_t uQualityOut (bQualityOut ? ctx_.tx.getFieldU32 (sfQualityOut) : 0);

    if (bQualityOut && QUALITY_ONE == uQualityOut)
        uQualityOut = 0;

    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();

    bool const bSetAuth = (uTxFlags & tfSetfAuth);
    bool const bSetNoRipple = (uTxFlags & tfSetNoRipple);
    bool const bClearNoRipple  = (uTxFlags & tfClearNoRipple);
    bool const bSetFreeze = (uTxFlags & tfSetFreeze);
    bool const bClearFreeze = (uTxFlags & tfClearFreeze);

    auto viewJ = ctx_.app.journal ("View");

    if (account_ == uDstAccountID)
    {
//这里的唯一目的是允许错误地创建
//信任自己要删除的行。如果没有这样的信任
//行现在存在，为什么不删除此代码并简单地
//返回错误？
        SLE::pointer sleDelete = view().peek (
            keylet::line(account_, uDstAccountID, currency));

        JLOG(j_.warn()) <<
            "Clearing redundant line.";

        return trustDelete (view(),
            sleDelete, account_, uDstAccountID, viewJ);
    }

    SLE::pointer sleDst =
        view().peek (keylet::account(uDstAccountID));

    if (!sleDst)
    {
        JLOG(j_.trace()) <<
            "Delay transaction: Destination account does not exist.";
        return tecNO_DST;
    }

    STAmount saLimitAllow = saLimitAmount;
    saLimitAllow.setIssuer (account_);

    SLE::pointer sleRippleState = view().peek (
        keylet::line(account_, uDstAccountID, currency));

    if (sleRippleState)
    {
        STAmount        saLowBalance;
        STAmount        saLowLimit;
        STAmount        saHighBalance;
        STAmount        saHighLimit;
        std::uint32_t   uLowQualityIn;
        std::uint32_t   uLowQualityOut;
        std::uint32_t   uHighQualityIn;
        std::uint32_t   uHighQualityOut;
        auto const& uLowAccountID   = !bHigh ? account_ : uDstAccountID;
        auto const& uHighAccountID  =  bHigh ? account_ : uDstAccountID;
        SLE::ref        sleLowAccount   = !bHigh ? sle : sleDst;
        SLE::ref        sleHighAccount  =  bHigh ? sle : sleDst;

//
//天平
//

        saLowBalance    = sleRippleState->getFieldAmount (sfBalance);
        saHighBalance   = -saLowBalance;

//
//限制
//

        sleRippleState->setFieldAmount (!bHigh ? sfLowLimit : sfHighLimit, saLimitAllow);

        saLowLimit  = !bHigh ? saLimitAllow : sleRippleState->getFieldAmount (sfLowLimit);
        saHighLimit =  bHigh ? saLimitAllow : sleRippleState->getFieldAmount (sfHighLimit);

//
//质量在
//

        if (!bQualityIn)
        {
//不设置。你就明白了。

            uLowQualityIn   = sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  = sleRippleState->getFieldU32 (sfHighQualityIn);
        }
        else if (uQualityIn)
        {
//设置。

            sleRippleState->setFieldU32 (!bHigh ? sfLowQualityIn : sfHighQualityIn, uQualityIn);

            uLowQualityIn   = !bHigh ? uQualityIn : sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  =  bHigh ? uQualityIn : sleRippleState->getFieldU32 (sfHighQualityIn);
        }
        else
        {
//清除。

            sleRippleState->makeFieldAbsent (!bHigh ? sfLowQualityIn : sfHighQualityIn);

            uLowQualityIn   = !bHigh ? 0 : sleRippleState->getFieldU32 (sfLowQualityIn);
            uHighQualityIn  =  bHigh ? 0 : sleRippleState->getFieldU32 (sfHighQualityIn);
        }

        if (QUALITY_ONE == uLowQualityIn)   uLowQualityIn   = 0;

        if (QUALITY_ONE == uHighQualityIn)  uHighQualityIn  = 0;

//
//质量出局
//

        if (!bQualityOut)
        {
//不设置。你就明白了。

            uLowQualityOut  = sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut = sleRippleState->getFieldU32 (sfHighQualityOut);
        }
        else if (uQualityOut)
        {
//设置。

            sleRippleState->setFieldU32 (!bHigh ? sfLowQualityOut : sfHighQualityOut, uQualityOut);

            uLowQualityOut  = !bHigh ? uQualityOut : sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut =  bHigh ? uQualityOut : sleRippleState->getFieldU32 (sfHighQualityOut);
        }
        else
        {
//清除。

            sleRippleState->makeFieldAbsent (!bHigh ? sfLowQualityOut : sfHighQualityOut);

            uLowQualityOut  = !bHigh ? 0 : sleRippleState->getFieldU32 (sfLowQualityOut);
            uHighQualityOut =  bHigh ? 0 : sleRippleState->getFieldU32 (sfHighQualityOut);
        }

        std::uint32_t const uFlagsIn (sleRippleState->getFieldU32 (sfFlags));
        std::uint32_t uFlagsOut (uFlagsIn);

        if (bSetNoRipple && !bClearNoRipple)
        {
            if ((bHigh ? saHighBalance : saLowBalance) >= beast::zero)
                uFlagsOut |= (bHigh ? lsfHighNoRipple : lsfLowNoRipple);

            else if (view().rules().enabled(fix1578))
//不能将noripple设置为负平衡。
                return tecNO_PERMISSION;
        }
        else if (bClearNoRipple && !bSetNoRipple)
        {
            uFlagsOut &= ~(bHigh ? lsfHighNoRipple : lsfLowNoRipple);
        }

        if (bSetFreeze && !bClearFreeze && !sle->isFlag  (lsfNoFreeze))
        {
            uFlagsOut           |= (bHigh ? lsfHighFreeze : lsfLowFreeze);
        }
        else if (bClearFreeze && !bSetFreeze)
        {
            uFlagsOut           &= ~(bHigh ? lsfHighFreeze : lsfLowFreeze);
        }

        if (QUALITY_ONE == uLowQualityOut)  uLowQualityOut  = 0;

        if (QUALITY_ONE == uHighQualityOut) uHighQualityOut = 0;

        bool const bLowDefRipple        = sleLowAccount->getFlags() & lsfDefaultRipple;
        bool const bHighDefRipple       = sleHighAccount->getFlags() & lsfDefaultRipple;

        bool const  bLowReserveSet      = uLowQualityIn || uLowQualityOut ||
                                            ((uFlagsOut & lsfLowNoRipple) == 0) != bLowDefRipple ||
                                            (uFlagsOut & lsfLowFreeze) ||
                                            saLowLimit || saLowBalance > beast::zero;
        bool const  bLowReserveClear    = !bLowReserveSet;

        bool const  bHighReserveSet     = uHighQualityIn || uHighQualityOut ||
                                            ((uFlagsOut & lsfHighNoRipple) == 0) != bHighDefRipple ||
                                            (uFlagsOut & lsfHighFreeze) ||
                                            saHighLimit || saHighBalance > beast::zero;
        bool const  bHighReserveClear   = !bHighReserveSet;

        bool const  bDefault            = bLowReserveClear && bHighReserveClear;

        bool const  bLowReserved = (uFlagsIn & lsfLowReserve);
        bool const  bHighReserved = (uFlagsIn & lsfHighReserve);

        bool        bReserveIncrease    = false;

        if (bSetAuth)
        {
            uFlagsOut |= (bHigh ? lsfHighAuth : lsfLowAuth);
        }

        if (bLowReserveSet && !bLowReserved)
        {
//为低账户设置准备金。
            adjustOwnerCount(view(),
                sleLowAccount, 1, viewJ);
            uFlagsOut |= lsfLowReserve;

            if (!bHigh)
                bReserveIncrease = true;
        }

        if (bLowReserveClear && bLowReserved)
        {
//清除低账户准备金。
            adjustOwnerCount(view(),
                sleLowAccount, -1, viewJ);
            uFlagsOut &= ~lsfLowReserve;
        }

        if (bHighReserveSet && !bHighReserved)
        {
//为大额账户设置准备金。
            adjustOwnerCount(view(),
                sleHighAccount, 1, viewJ);
            uFlagsOut |= lsfHighReserve;

            if (bHigh)
                bReserveIncrease    = true;
        }

        if (bHighReserveClear && bHighReserved)
        {
//结清大额准备金。
            adjustOwnerCount(view(),
                sleHighAccount, -1, viewJ);
            uFlagsOut &= ~lsfHighReserve;
        }

        if (uFlagsIn != uFlagsOut)
            sleRippleState->setFieldU32 (sfFlags, uFlagsOut);

        if (bDefault || badCurrency() == currency)
        {
//删除。

            terResult = trustDelete (view(),
                sleRippleState, uLowAccountID, uHighAccountID, viewJ);
        }
//储备不按负荷缩放。
        else if (bReserveIncrease && mPriorBalance < reserveCreate)
        {
            JLOG(j_.trace()) <<
                "Delay transaction: Insufficent reserve to add trust line.";

//另一笔交易可以向账户提供XRP，然后
//此事务将成功。
            terResult = tecINSUF_RESERVE_LINE;
        }
        else
        {
            view().update (sleRippleState);

            JLOG(j_.trace()) << "Modify ripple line";
        }
    }
//行不存在。
else if (! saLimitAmount &&                          //设置默认限制。
(! bQualityIn || ! uQualityIn) &&           //未在中设置质量或在中设置默认质量。
(! bQualityOut || ! uQualityOut) &&         //不设置质量或设置默认质量。
        (! (view().rules().enabled(featureTrustSetAuth)) || ! bSetAuth))
    {
        JLOG(j_.trace()) <<
            "Redundant: Setting non-existent ripple line to defaults.";
        return tecNO_LINE_REDUNDANT;
    }
else if (mPriorBalance < reserveCreate) //储备不按负荷缩放。
    {
        JLOG(j_.trace()) <<
            "Delay transaction: Line does not exist. Insufficent reserve to create line.";

//另一个事务可以创建帐户，然后该事务将成功。
        terResult = tecNO_LINE_INSUF_RESERVE;
    }
    else
    {
//货币余额为零。
        STAmount saBalance ({currency, noAccount()});

        uint256 index (getRippleStateIndex (
            account_, uDstAccountID, currency));

        JLOG(j_.trace()) <<
            "doTrustSet: Creating ripple line: " <<
            to_string (index);

//创建新的波纹线。
        terResult = trustCreate (view(),
            bHigh,
            account_,
            uDstAccountID,
            index,
            sle,
            bSetAuth,
            bSetNoRipple && !bClearNoRipple,
            bSetFreeze && !bClearFreeze,
            saBalance,
saLimitAllow,       //对被指控人的限制。
            uQualityIn,
            uQualityOut, viewJ);
    }

    return terResult;
}

}
