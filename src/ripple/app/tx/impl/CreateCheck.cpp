
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
    版权所有（c）2017 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/CreateCheck.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

NotTEC
CreateCheck::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled (featureChecks))
        return temDISABLED;

    NotTEC const ret {preflight1 (ctx)};
    if (! isTesSuccess (ret))
        return ret;

    if (ctx.tx.getFlags() & tfUniversalMask)
    {
//还没有用于CreateCheck的标志（通用标志除外）。
        JLOG(ctx.j.warn()) << "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }
    if (ctx.tx[sfAccount] == ctx.tx[sfDestination])
    {
//他们给自己开了一张支票。
        JLOG(ctx.j.warn()) << "Malformed transaction: Check to self.";
        return temREDUNDANT;
    }

    {
        STAmount const sendMax {ctx.tx.getFieldAmount (sfSendMax)};
        if (!isLegalNet (sendMax) || sendMax.signum() <= 0)
        {
            JLOG(ctx.j.warn()) << "Malformed transaction: bad sendMax amount: "
                << sendMax.getFullText();
            return temBAD_AMOUNT;
        }

        if (badCurrency() == sendMax.getCurrency())
        {
            JLOG(ctx.j.warn()) <<"Malformed transaction: Bad currency.";
            return temBAD_CURRENCY;
        }
    }

    if (auto const optExpiry = ctx.tx[~sfExpiration])
    {
        if (*optExpiry == 0)
        {
            JLOG(ctx.j.warn()) << "Malformed transaction: bad expiration";
            return temBAD_EXPIRATION;
        }
    }

    return preflight2 (ctx);
}

TER
CreateCheck::preclaim (PreclaimContext const& ctx)
{
    AccountID const dstId {ctx.tx[sfDestination]};
    auto const sleDst = ctx.view.read (keylet::account (dstId));
    if (! sleDst)
    {
        JLOG(ctx.j.warn()) << "Destination account does not exist.";
        return tecNO_DST;
    }

    if ((sleDst->getFlags() & lsfRequireDestTag) &&
        !ctx.tx.isFieldPresent (sfDestinationTag))
    {
//标签基本上是特定于帐户的信息，我们没有
//明白，但我们可以要求有人填写。
        JLOG(ctx.j.warn()) << "Malformed transaction: DestinationTag required.";
        return tecDST_TAG_NEEDED;
    }

    {
        STAmount const sendMax {ctx.tx[sfSendMax]};
        if (! sendMax.native())
        {
//货币不可全球冻结
            AccountID const& issuerId {sendMax.getIssuer()};
            if (isGlobalFrozen (ctx.view, issuerId))
            {
                JLOG(ctx.j.warn()) << "Creating a check for frozen asset";
                return tecFROZEN;
            }
//如果这个帐户有货币的托管行，那么
//信任线不能被冻结。
//
//请注意，我们确实允许为以下货币创建支票：
//帐户还没有到的信任线。
            AccountID const srcId {ctx.tx.getAccountID (sfAccount)};
            if (issuerId != srcId)
            {
//检查发卡行是否冻结行
                auto const sleTrust = ctx.view.read (
                    keylet::line (srcId, issuerId, sendMax.getCurrency()));
                if (sleTrust &&
                    sleTrust->isFlag (
                        (issuerId > srcId) ? lsfHighFreeze : lsfLowFreeze))
                {
                    JLOG(ctx.j.warn())
                        << "Creating a check for frozen trustline.";
                    return tecFROZEN;
                }
            }
            if (issuerId != dstId)
            {
//检查DST是否冻结了线路。
                auto const sleTrust = ctx.view.read (
                    keylet::line (issuerId, dstId, sendMax.getCurrency()));
                if (sleTrust &&
                    sleTrust->isFlag (
                        (dstId > issuerId) ? lsfHighFreeze : lsfLowFreeze))
                {
                    JLOG(ctx.j.warn())
                        << "Creating a check for destination frozen trustline.";
                    return tecFROZEN;
                }
            }
        }
    }
    {
        using duration = NetClock::duration;
        using timepoint = NetClock::time_point;
        auto const optExpiry = ctx.tx[~sfExpiration];

//到期是根据父级的关闭时间定义的。
//Ledger，因为我们明确知道它关闭的时间，但是
//我们不知道下面的分类帐的结算时间
//建设。
        if (optExpiry &&
            (ctx.view.parentCloseTime() >= timepoint {duration {*optExpiry}}))
        {
            JLOG(ctx.j.warn()) << "Creating a check that has already expired.";
            return tecEXPIRED;
        }
    }
    return tesSUCCESS;
}

TER
CreateCheck::doApply ()
{
    auto const sle = view().peek (keylet::account (account_));

//支票与开证账户的准备金相抵，但我们
//检查起始平衡，因为我们要允许
//准备支付费用。
    {
        STAmount const reserve {view().fees().accountReserve (
            sle->getFieldU32 (sfOwnerCount) + 1)};

        if (mPriorBalance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    AccountID const dstAccountId {ctx_.tx[sfDestination]};
    std::uint32_t const seq {ctx_.tx.getSequence()};
    auto sleCheck =
        std::make_shared<SLE>(ltCHECK, getCheckIndex (account_, seq));

    sleCheck->setAccountID (sfAccount, account_);
    sleCheck->setAccountID (sfDestination, dstAccountId);
    sleCheck->setFieldU32 (sfSequence, seq);
    sleCheck->setFieldAmount (sfSendMax, ctx_.tx[sfSendMax]);
    if (auto const srcTag = ctx_.tx[~sfSourceTag])
        sleCheck->setFieldU32 (sfSourceTag, *srcTag);
    if (auto const dstTag = ctx_.tx[~sfDestinationTag])
        sleCheck->setFieldU32 (sfDestinationTag, *dstTag);
    if (auto const invoiceId = ctx_.tx[~sfInvoiceID])
        sleCheck->setFieldH256 (sfInvoiceID, *invoiceId);
    if (auto const expiry = ctx_.tx[~sfExpiration])
        sleCheck->setFieldU32 (sfExpiration, *expiry);

    view().insert (sleCheck);

    auto viewJ = ctx_.app.journal ("View");
//如果不是自动发送（不应该是），请将支票添加到
//目的地的所有者目录。
    if (dstAccountId != account_)
    {
        auto const page = dirAdd (view(), keylet::ownerDir (dstAccountId),
            sleCheck->key(), false, describeOwnerDir (dstAccountId), viewJ);

        JLOG(j_.trace())
            << "Adding Check to destination directory "
            << to_string (sleCheck->key())
            << ": " << (page ? "success" : "failure");

        if (! page)
            return tecDIR_FULL;

        sleCheck->setFieldU64 (sfDestinationNode, *page);
    }

    {
        auto const page = dirAdd (view(), keylet::ownerDir (account_),
            sleCheck->key(), false, describeOwnerDir (account_), viewJ);

        JLOG(j_.trace())
            << "Adding Check to owner directory "
            << to_string (sleCheck->key())
            << ": " << (page ? "success" : "failure");

        if (! page)
            return tecDIR_FULL;

        sleCheck->setFieldU64 (sfOwnerNode, *page);
    }
//如果我们成功了，新条目将与创建者的保留计数。
    adjustOwnerCount (view(), sle, 1, viewJ);
    return tesSUCCESS;
}

} //命名空间波纹
