
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

#include <ripple/app/tx/impl/CancelOffer.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/st.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
CancelOffer::preflight (PreflightContext const& ctx)
{
    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto const uTxFlags = ctx.tx.getFlags();

    if (uTxFlags & tfUniversalMask)
    {
        JLOG(ctx.j.trace()) << "Malformed transaction: " <<
            "Invalid flags set.";
        return temINVALID_FLAG;
    }

    auto const seq = ctx.tx.getFieldU32 (sfOfferSequence);
    if (! seq)
    {
        JLOG(ctx.j.trace()) <<
            "CancelOffer::preflight: missing sequence";
        return temBAD_SEQUENCE;
    }

    return preflight2(ctx);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

TER
CancelOffer::preclaim(PreclaimContext const& ctx)
{
    auto const id = ctx.tx[sfAccount];
    auto const offerSequence = ctx.tx[sfOfferSequence];

    auto const sle = ctx.view.read(
        keylet::account(id));

    if ((*sle)[sfSequence] <= offerSequence)
    {
        JLOG(ctx.j.trace()) << "Malformed transaction: " <<
            "Sequence " << offerSequence << " is invalid.";
        return temBAD_SEQUENCE;
    }

    return tesSUCCESS;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

TER
CancelOffer::doApply ()
{
    auto const offerSequence = ctx_.tx[sfOfferSequence];

    auto const sle = view().read(
        keylet::account(account_));

    uint256 const offerIndex (getOfferIndex (account_, offerSequence));

    auto sleOffer = view().peek (
        keylet::offer(offerIndex));

    if (sleOffer)
    {
        JLOG(j_.debug()) << "Trying to cancel offer #" << offerSequence;
        return offerDelete (view(), sleOffer, ctx_.app.journal("View"));
    }

    JLOG(j_.debug()) << "Offer #" << offerSequence << " can't be found.";
    return tesSUCCESS;
}

}
