
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

#include <ripple/app/tx/impl/CancelTicket.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
CancelTicket::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled(featureTickets))
        return temDISABLED;

    if (ctx.tx.getFlags() & tfUniversalMask)
        return temINVALID_FLAG;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    return preflight2 (ctx);
}

TER
CancelTicket::doApply ()
{
    uint256 const ticketId = ctx_.tx.getFieldH256 (sfTicketID);

//vfalc这是高度可疑的，我们要求
//事务是否提供GetTicketIndex的返回值？
    SLE::pointer sleTicket = view().peek (keylet::ticket(ticketId));

    if (!sleTicket)
        return tecNO_ENTRY;

    auto const ticket_owner =
        sleTicket->getAccountID (sfAccount);

    bool authorized =
        account_ == ticket_owner;

//目标也可以始终删除一张票据
    if (!authorized && sleTicket->isFieldPresent (sfTarget))
        authorized = (account_ == sleTicket->getAccountID (sfTarget));

//最后，任何人都可以删除过期的票据
    if (!authorized && sleTicket->isFieldPresent (sfExpiration))
    {
        using tp = NetClock::time_point;
        using d = tp::duration;
        auto const expiration = tp{d{sleTicket->getFieldU32(sfExpiration)}};

        if (view().parentCloseTime() >= expiration)
            authorized = true;
    }

    if (!authorized)
        return tecNO_PERMISSION;

    std::uint64_t const hint (sleTicket->getFieldU64 (sfOwnerNode));

    if (! ctx_.view().dirRemove(
            keylet::ownerDir(ticket_owner), hint, ticketId, false))
    {
        return tefBAD_LEDGER;
    }

    auto viewJ = ctx_.app.journal ("View");
    adjustOwnerCount(view(), view().peek(
        keylet::account(ticket_owner)), -1, viewJ);
    ctx_.view ().erase (sleTicket);

    return tesSUCCESS;
}

}
