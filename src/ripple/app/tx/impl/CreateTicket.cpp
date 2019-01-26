
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
    版权所有（c）2014 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/CreateTicket.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

NotTEC
CreateTicket::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureTickets))
        return temDISABLED;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    if (ctx.tx.isFieldPresent (sfExpiration))
    {
        if (ctx.tx.getFieldU32 (sfExpiration) == 0)
        {
            JLOG(ctx.j.warn()) <<
                "Malformed transaction: bad expiration";
            return temBAD_EXPIRATION;
        }
    }

    return preflight2 (ctx);
}

TER
CreateTicket::doApply ()
{
    auto const sle = view().peek(keylet::account(account_));

//一张票等于开证账户的准备金，但我们
//检查起始平衡，因为我们要允许
//准备支付费用。
    {
        auto const reserve = view().fees().accountReserve(
            sle->getFieldU32(sfOwnerCount) + 1);

        if (mPriorBalance < reserve)
            return tecINSUFFICIENT_RESERVE;
    }

    NetClock::time_point expiration{};

    if (ctx_.tx.isFieldPresent (sfExpiration))
    {
        expiration = NetClock::time_point(NetClock::duration(ctx_.tx[sfExpiration]));

        if (view().parentCloseTime() >= expiration)
            return tesSUCCESS;
    }

    SLE::pointer sleTicket = std::make_shared<SLE>(ltTICKET,
        getTicketIndex (account_, ctx_.tx.getSequence ()));
    sleTicket->setAccountID (sfAccount, account_);
    sleTicket->setFieldU32 (sfSequence, ctx_.tx.getSequence ());
    if (expiration != NetClock::time_point{})
        sleTicket->setFieldU32 (sfExpiration, expiration.time_since_epoch().count());
    view().insert (sleTicket);

    if (ctx_.tx.isFieldPresent (sfTarget))
    {
        AccountID const target_account (ctx_.tx.getAccountID (sfTarget));

        SLE::pointer sleTarget = view().peek (keylet::account(target_account));

//目标帐户不存在。
        if (!sleTarget)
            return tecNO_TARGET;

//发行帐户是票据的默认帐户
//应用，所以如果指定了，就不要费心保存它。
        if (target_account != account_)
            sleTicket->setAccountID (sfTarget, target_account);
    }

    auto viewJ = ctx_.app.journal ("View");

    auto const page = dirAdd(view(), keylet::ownerDir (account_),
        sleTicket->key(), false, describeOwnerDir (account_), viewJ);

    JLOG(j_.trace()) <<
        "Creating ticket " << to_string (sleTicket->key()) <<
        ": " << (page ? "success" : "failure");

    if (!page)
        return tecDIR_FULL;

    sleTicket->setFieldU64(sfOwnerNode, *page);

//如果我们成功，新条目将与
//创造者的储备。
    adjustOwnerCount(view(), sle, 1, viewJ);
    return tesSUCCESS;
}

}
