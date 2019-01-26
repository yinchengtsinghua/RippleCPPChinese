
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

#include <ripple/app/tx/impl/Payment.h>
#include <ripple/app/paths/RippleCalc.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/protocol/JsonFields.h>

namespace ripple {

//请参见https://ripple.com/wiki/transaction_format payment_u280.29

XRPAmount
Payment::calculateMaxSpend(STTx const& tx)
{
    if (tx.isFieldPresent(sfSendMax))
    {
        auto const& sendMax = tx[sfSendMax];
        return sendMax.native() ? sendMax.xrp() : beast::zero;
    }
    /*如果xrp中没有sfsendmax，而sfamount没有
    在xrp中，则事务不能发送xrp。*/

    auto const& saDstAmount = tx.getFieldAmount(sfAmount);
    return saDstAmount.native() ? saDstAmount.xrp() : beast::zero;
}

NotTEC
Payment::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    std::uint32_t const uTxFlags = tx.getFlags ();

    if (uTxFlags & tfPaymentMask)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Invalid flags set.";
        return temINVALID_FLAG;
    }

    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoRippleDirect);
    bool const bPaths = tx.isFieldPresent (sfPaths);
    bool const bMax = tx.isFieldPresent (sfSendMax);

    STAmount const saDstAmount (tx.getFieldAmount (sfAmount));

    STAmount maxSourceAmount;
    auto const account = tx.getAccountID(sfAccount);

    if (bMax)
        maxSourceAmount = tx.getFieldAmount (sfSendMax);
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            { saDstAmount.getCurrency (), account },
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < beast::zero);

    auto const& uSrcCurrency = maxSourceAmount.getCurrency ();
    auto const& uDstCurrency = saDstAmount.getCurrency ();

//iszero（）是xrp。修复！
    bool const bXRPDirect = uSrcCurrency.isZero () && uDstCurrency.isZero ();

    if (!isLegalNet (saDstAmount) || !isLegalNet (maxSourceAmount))
        return temBAD_AMOUNT;

    auto const uDstAccountID = tx.getAccountID (sfDestination);

    if (!uDstAccountID)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Payment destination account not specified.";
        return temDST_NEEDED;
    }
    if (bMax && maxSourceAmount <= beast::zero)
    {
        JLOG(j.trace()) << "Malformed transaction: " <<
            "bad max amount: " << maxSourceAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (saDstAmount <= beast::zero)
    {
        JLOG(j.trace()) << "Malformed transaction: "<<
            "bad dst amount: " << saDstAmount.getFullText ();
        return temBAD_AMOUNT;
    }
    if (badCurrency() == uSrcCurrency || badCurrency() == uDstCurrency)
    {
        JLOG(j.trace()) <<"Malformed transaction: " <<
            "Bad currency.";
        return temBAD_CURRENCY;
    }
    if (account == uDstAccountID && uSrcCurrency == uDstCurrency && !bPaths)
    {
//你在给自己签名付款。
//如果bpaths是真的，你可能在尝试套利。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Redundant payment from " << to_string (account) <<
            " to self without path for " << to_string (uDstCurrency);
        return temREDUNDANT;
    }
    if (bXRPDirect && bMax)
    {
//一致但冗余的事务。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "SendMax specified for XRP to XRP.";
        return temBAD_SEND_XRP_MAX;
    }
    if (bXRPDirect && bPaths)
    {
//发送xrp时没有路径。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Paths specified for XRP to XRP.";
        return temBAD_SEND_XRP_PATHS;
    }
    if (bXRPDirect && partialPaymentAllowed)
    {
//一致但冗余的事务。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Partial payment specified for XRP to XRP.";
        return temBAD_SEND_XRP_PARTIAL;
    }
    if (bXRPDirect && limitQuality)
    {
//一致但冗余的事务。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "Limit quality specified for XRP to XRP.";
        return temBAD_SEND_XRP_LIMIT;
    }
    if (bXRPDirect && !defaultPathsAllowed)
    {
//一致但冗余的事务。
        JLOG(j.trace()) << "Malformed transaction: " <<
            "No ripple direct specified for XRP to XRP.";
        return temBAD_SEND_XRP_NO_DIRECT;
    }

    auto const deliverMin = tx[~sfDeliverMin];
    if (deliverMin)
    {
        if (! partialPaymentAllowed)
        {
            JLOG(j.trace()) << "Malformed transaction: Partial payment not "
                "specified for " << jss::DeliverMin.c_str() << ".";
            return temBAD_AMOUNT;
        }

        auto const dMin = *deliverMin;
        if (!isLegalNet(dMin) || dMin <= beast::zero)
        {
            JLOG(j.trace()) << "Malformed transaction: Invalid " <<
                jss::DeliverMin.c_str() << " amount. " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin.issue() != saDstAmount.issue())
        {
            JLOG(j.trace()) <<  "Malformed transaction: Dst issue differs "
                "from " << jss::DeliverMin.c_str() << ". " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
        if (dMin > saDstAmount)
        {
            JLOG(j.trace()) << "Malformed transaction: Dst amount less than " <<
                jss::DeliverMin.c_str() << ". " <<
                    dMin.getFullText();
            return temBAD_AMOUNT;
        }
    }

    return preflight2 (ctx);
}

TER
Payment::preclaim(PreclaimContext const& ctx)
{
//如果源或目标不是本机的或有路径，则会发生涟漪。
    std::uint32_t const uTxFlags = ctx.tx.getFlags();
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    auto const paths = ctx.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx.tx[~sfSendMax];

    AccountID const uDstAccountID(ctx.tx[sfDestination]);
    STAmount const saDstAmount(ctx.tx[sfAmount]);

    auto const k = keylet::account(uDstAccountID);
    auto const sleDst = ctx.view.read(k);

    if (!sleDst)
    {
//目标帐户不存在。
        if (!saDstAmount.native())
        {
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Destination account does not exist.";

//另一个事务可以创建帐户，然后
//交易会成功。
            return tecNO_DST;
        }
        else if (ctx.view.open()
            && partialPaymentAllowed)
        {
//您不能用部分付款为帐户提供资金。
//通过拒绝这个，使重试工作更小。
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Partial payment not allowed to create account.";


//另一个事务可以创建帐户，然后
//交易会成功。
            return telNO_DST_PARTIAL;
        }
        else if (saDstAmount < STAmount(ctx.view.fees().accountReserve(0)))
        {
//accountreserve是帐户可以拥有的最小金额。
//储备不按负荷缩放。
            JLOG(ctx.j.trace()) <<
                "Delay transaction: Destination account does not exist. " <<
                "Insufficent payment to create account.";

//待办事项：杜杜
//另一个事务可以创建帐户，然后
//交易会成功。
            return tecNO_DST_INSUF_XRP;
        }
    }
    else if ((sleDst->getFlags() & lsfRequireDestTag) &&
        !ctx.tx.isFieldPresent(sfDestinationTag))
    {
//标签基本上是特定于帐户的信息，我们没有
//明白，但我们可以要求有人填写。

//我们没有对新成立的账户进行测试，因为
//无法设置此字段。
        JLOG(ctx.j.trace()) << "Malformed transaction: DestinationTag required.";

        return tecDST_TAG_NEEDED;
    }

    if (paths || sendMax || !saDstAmount.native())
    {
//Ripple支付至少有一个中间步骤和用途
//可传递余额。

//将路径复制到可编辑类中。
        STPathSet const spsPaths = ctx.tx.getFieldPathSet(sfPaths);

        auto pathTooBig = spsPaths.size() > MaxPathSize;

        if(!pathTooBig)
            for (auto const& path : spsPaths)
                if (path.size() > MaxPathLength)
                {
                    pathTooBig = true;
                    break;
                }

        if (ctx.view.open() && pathTooBig)
        {
return telBAD_PATH_COUNT; //建议的分类帐路径太多。
        }
    }

    return tesSUCCESS;
}


TER
Payment::doApply ()
{
    auto const deliverMin = ctx_.tx[~sfDeliverMin];

//如果源或目标不是本机的或有路径，则会发生涟漪。
    std::uint32_t const uTxFlags = ctx_.tx.getFlags ();
    bool const partialPaymentAllowed = uTxFlags & tfPartialPayment;
    bool const limitQuality = uTxFlags & tfLimitQuality;
    bool const defaultPathsAllowed = !(uTxFlags & tfNoRippleDirect);
    auto const paths = ctx_.tx.isFieldPresent(sfPaths);
    auto const sendMax = ctx_.tx[~sfSendMax];

    AccountID const uDstAccountID (ctx_.tx.getAccountID (sfDestination));
    STAmount const saDstAmount (ctx_.tx.getFieldAmount (sfAmount));
    STAmount maxSourceAmount;
    if (sendMax)
        maxSourceAmount = *sendMax;
    else if (saDstAmount.native ())
        maxSourceAmount = saDstAmount;
    else
        maxSourceAmount = STAmount (
            {saDstAmount.getCurrency (), account_},
            saDstAmount.mantissa(), saDstAmount.exponent (),
            saDstAmount < beast::zero);

    JLOG(j_.trace()) <<
        "maxSourceAmount=" << maxSourceAmount.getFullText () <<
        " saDstAmount=" << saDstAmount.getFullText ();

//打开分类帐进行编辑。
    auto const k = keylet::account(uDstAccountID);
    SLE::pointer sleDst = view().peek (k);

    if (!sleDst)
    {
//创建帐户。
        sleDst = std::make_shared<SLE>(k);
        sleDst->setAccountID(sfAccount, uDstAccountID);
        sleDst->setFieldU32(sfSequence, 1);
        view().insert(sleDst);
    }
    else
    {
//告诉引擎我们打算改变目的地
//帐户。源帐户总是要收费，所以
//标记为已修改。
        view().update (sleDst);
    }

//确定目的地是否需要存款授权。
    bool const reqDepositAuth = sleDst->getFlags() & lsfDepositAuth &&
        view().rules().enabled(featureDepositAuth);

    bool const depositPreauth = view().rules().enabled(featureDepositPreauth);

    bool const bRipple = paths || sendMax || !saDstAmount.native ();

//如果目的地设置了lsfdepositauth，则仅直接xrp
//不允许向目的地付款（无中间步骤）。
    if (!depositPreauth && bRipple && reqDepositAuth)
        return tecNO_PERMISSION;

    if (bRipple)
    {
//Ripple支付至少有一个中间步骤和用途
//可传递余额。

        if (depositPreauth && reqDepositAuth)
        {
//如果启用DepositPreauth，则需要
//授权有两种方式获得借据付款：
//1。如果account==目的地，或
//2。如果帐户是按目的地预先授权的存款。
            if (uDstAccountID != account_)
            {
                if (! view().exists (
                    keylet::depositPreauth (uDstAccountID, account_)))
                    return tecNO_PERMISSION;
            }
        }

//将路径复制到可编辑类中。
        STPathSet spsPaths = ctx_.tx.getFieldPathSet (sfPaths);

        path::RippleCalc::Input rcInput;
        rcInput.partialPaymentAllowed = partialPaymentAllowed;
        rcInput.defaultPathsAllowed = defaultPathsAllowed;
        rcInput.limitQuality = limitQuality;
        rcInput.isLedgerOpen = view().open();

        path::RippleCalc::Output rc;
        {
            PaymentSandbox pv(&view());
            JLOG(j_.debug())
                << "Entering RippleCalc in payment: "
                << ctx_.tx.getTransactionID();
            rc = path::RippleCalc::rippleCalculate (
                pv,
                maxSourceAmount,
                saDstAmount,
                uDstAccountID,
                account_,
                spsPaths,
                ctx_.app.logs(),
                &rcInput);
//vfalc注：我们可能不需要申请，这取决于
//在TER上。但总是应用*应该*
//是安全的。
            pv.apply(ctx_.rawView());
        }

//托多：是这样吗？如果金额正确，则为
//以前设定的交货金额？
        if (rc.result () == tesSUCCESS &&
            rc.actualAmountOut != saDstAmount)
        {
            if (deliverMin && rc.actualAmountOut <
                *deliverMin)
                rc.setResult (tecPATH_PARTIAL);
            else
                ctx_.deliver (rc.actualAmountOut);
        }

        auto terResult = rc.result ();

//因为它的开销，如果RippleCalc
//失败，返回重试代码，收取费用
//相反。也许用户会更多
//下次小心他们的路径规范。
        if (isTerRetry (terResult))
            terResult = tecPATH_DRY;
        return terResult;
    }

    assert (saDstAmount.native ());

//直接XRP付款。

//UownerCount是此分类帐中的分录数。
//需要准备金的账户。
    auto const uOwnerCount = view().read(
        keylet::account(account_))->getFieldU32 (sfOwnerCount);

//这是下降的总准备金。
    auto const reserve = view().fees().accountReserve(uOwnerCount);

//mpriorbalance是发送帐户上
//收费。我们要确保有足够的储备
//发送。允许最终支出使用费用准备金。
    auto const mmm = std::max(reserve,
        ctx_.tx.getFieldAmount (sfFee).xrp ());

    if (mPriorBalance < saDstAmount.xrp () + mmm)
    {
//投反对票。但是，如果应用于
//不同的顺序。
        JLOG(j_.trace()) << "Delay transaction: Insufficient funds: " <<
            " " << to_string (mPriorBalance) <<
            " / " << to_string (saDstAmount.xrp () + mmm) <<
            " (" << to_string (reserve) << ")";

        return tecUNFUNDED_PAYMENT;
    }

//源帐户确实有足够的钱。确定
//源帐户有权存入目的地。
    if (reqDepositAuth)
    {
//如果启用DepositPreauth，则需要
//授权有三种方式获得xrp付款：
//1。如果account==目的地，或
//2。如果账户是按目的地预先授权的存款，或
//三。如果目的地的xrp余额为
//a.小于或等于基本准备金和
//b.存款金额小于或等于基本准备金，
//那么我们就允许押金。
//
//规则3旨在防止帐户被楔入
//如果设置lsfdepositauth标志并且
//然后消耗所有的xrp。没有规则如果
//lsfdepositauth集的帐户已使用其所有xrp，it
//无法获得支付费用所需的更多XRP。
//
//我们选择基本预备役作为我们的边界，因为它是
//很少改变但总是足够的一小部分
//使账户不被冻结。
        if (uDstAccountID != account_)
        {
            if (! view().exists (
                keylet::depositPreauth (uDstAccountID, account_)))
            {
//获得基本储备。
                XRPAmount const dstReserve {view().fees().accountReserve (0)};

                if (saDstAmount > dstReserve ||
                    sleDst->getFieldAmount (sfBalance) > dstReserve)
                        return tecNO_PERMISSION;
            }
        }
    }

//对转账进行算术运算，并对分类帐进行更改。
    view()
        .peek(keylet::account(account_))
        ->setFieldAmount(sfBalance, mSourceBalance - saDstAmount);
    sleDst->setFieldAmount(
        sfBalance, sleDst->getFieldAmount(sfBalance) + saDstAmount);

//如果我们可以并且需要的话，重新支付密码更改费。
    if ((sleDst->getFlags() & lsfPasswordSpent))
        sleDst->clearFlag(lsfPasswordSpent);

    return tesSUCCESS;
}

}  //涟漪
