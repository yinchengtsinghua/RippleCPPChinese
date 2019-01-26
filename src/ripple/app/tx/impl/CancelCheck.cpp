
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

#include <ripple/app/tx/impl/CancelCheck.h>

#include <ripple/app/ledger/Ledger.h>
#include <ripple/basics/Log.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>

namespace ripple {

NotTEC
CancelCheck::preflight (PreflightContext const& ctx)
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

    return preflight2 (ctx);
}

TER
CancelCheck::preclaim (PreclaimContext const& ctx)
{
    auto const sleCheck = ctx.view.read (keylet::check (ctx.tx[sfCheckID]));
    if (! sleCheck)
    {
        JLOG(ctx.j.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

    using duration = NetClock::duration;
    using timepoint = NetClock::time_point;
    auto const optExpiry = (*sleCheck)[~sfExpiration];

//到期是根据父级的关闭时间定义的。
//Ledger，因为我们明确知道它关闭的时间，但是
//我们不知道下面的分类帐的结算时间
//建设。
    if (! optExpiry ||
        (ctx.view.parentCloseTime() < timepoint {duration {*optExpiry}}))
    {
//如果支票尚未过期，则只有创建者或
//目的地可以取消支票。
        AccountID const acctId {ctx.tx[sfAccount]};
        if (acctId != (*sleCheck)[sfAccount] &&
            acctId != (*sleCheck)[sfDestination])
        {
            JLOG(ctx.j.warn()) << "Check is not expired and canceler is "
                "neither check source nor destination.";
            return tecNO_PERMISSION;
        }
    }
    return tesSUCCESS;
}

TER
CancelCheck::doApply ()
{
    uint256 const checkId {ctx_.tx[sfCheckID]};
    auto const sleCheck = view().peek (keylet::check (checkId));
    if (! sleCheck)
    {
//在预索赔中应该发现错误。
        JLOG(j_.warn()) << "Check does not exist.";
        return tecNO_ENTRY;
    }

    AccountID const srcId {sleCheck->getAccountID (sfAccount)};
    AccountID const dstId {sleCheck->getAccountID (sfDestination)};
    auto viewJ = ctx_.app.journal ("View");

//如果支票没有写给自己（不应该写），请删除
//从目标帐户根目录检查。
    if (srcId != dstId)
    {
        std::uint64_t const page {(*sleCheck)[sfDestinationNode]};
        if (! view().dirRemove (keylet::ownerDir(dstId), page, checkId, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from destination.";
            return tefBAD_LEDGER;
        }
    }
    {
        std::uint64_t const page {(*sleCheck)[sfOwnerNode]};
        if (! view().dirRemove (keylet::ownerDir(srcId), page, checkId, true))
        {
            JLOG(j_.warn()) << "Unable to delete check from owner.";
            return tefBAD_LEDGER;
        }
    }

//如果成功，请更新支票所有者的保留。
    auto const sleSrc = view().peek (keylet::account (srcId));
    adjustOwnerCount (view(), sleSrc, -1, viewJ);

//从分类帐中删除支票。
    view().erase (sleCheck);
    return tesSUCCESS;
}

} //命名空间波纹
