
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
    版权所有（c）2018 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/DepositPreauth.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/ledger/View.h>

namespace ripple {

NotTEC
DepositPreauth::preflight (PreflightContext const& ctx)
{
    if (! ctx.rules.enabled (featureDepositPreauth))
        return temDISABLED;

    auto const ret = preflight1 (ctx);
    if (!isTesSuccess (ret))
        return ret;

    auto& tx = ctx.tx;
    auto& j = ctx.j;

    if (tx.getFlags() & tfUniversalMask)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Invalid flags set.";
        return temINVALID_FLAG;
    }

    auto const optAuth = ctx.tx[~sfAuthorize];
    auto const optUnauth = ctx.tx[~sfUnauthorize];
    if (static_cast<bool>(optAuth) == static_cast<bool>(optUnauth))
    {
//两个字段都存在或都不存在。在
//无论哪种情况，事务都是格式错误的。
        JLOG(j.trace()) <<
            "Malformed transaction: "
            "Invalid Authorize and Unauthorize field combination.";
        return temMALFORMED;
    }

//确保通过的帐户有效。
    AccountID const target {optAuth ? *optAuth : *optUnauth};
    if (target == beast::zero)
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Authorized or Unauthorized field zeroed.";
        return temINVALID_ACCOUNT_ID;
    }

//帐户不能自行预授权。
    if (optAuth && (target == ctx.tx[sfAccount]))
    {
        JLOG(j.trace()) <<
            "Malformed transaction: Attempting to DepositPreauth self.";
        return temCANNOT_PREAUTH_SELF;
    }

    return preflight2 (ctx);
}

TER
DepositPreauth::preclaim(PreclaimContext const& ctx)
{
//确定我们正在执行的操作：授权或未授权。
    if (ctx.tx.isFieldPresent (sfAuthorize))
    {
//验证授权帐户是否存在于分类帐中。
        AccountID const auth {ctx.tx[sfAuthorize]};
        if (! ctx.view.exists (keylet::account (auth)))
            return tecNO_TARGET;

//验证他们请求添加的预身份验证条目是否尚未
//在分类帐中。
        if (ctx.view.exists (
            keylet::depositPreauth (ctx.tx[sfAccount], auth)))
            return tecDUPLICATE;
    }
    else
    {
//验证他们要求删除的预授权条目是否在分类帐中。
        AccountID const unauth {ctx.tx[sfUnauthorize]};
        if (! ctx.view.exists (
            keylet::depositPreauth (ctx.tx[sfAccount], unauth)))
            return tecNO_ENTRY;
    }
    return tesSUCCESS;
}

TER
DepositPreauth::doApply ()
{
    auto const sleOwner = view().peek (keylet::account (account_));

    if (ctx_.tx.isFieldPresent (sfAuthorize))
    {
//预授权计入发行账户的准备金，但我们
//检查起始平衡，因为我们要允许
//准备支付费用。
        {
            STAmount const reserve {view().fees().accountReserve (
                sleOwner->getFieldU32 (sfOwnerCount) + 1)};

            if (mPriorBalance < reserve)
                return tecINSUFFICIENT_RESERVE;
        }

//preclaim已经验证了preauth条目尚不存在。
//创建并填充预身份验证条目。
        AccountID const auth {ctx_.tx[sfAuthorize]};
        auto slePreauth =
            std::make_shared<SLE>(keylet::depositPreauth (account_, auth));

        slePreauth->setAccountID (sfAccount, account_);
        slePreauth->setAccountID (sfAuthorize, auth);
        view().insert (slePreauth);

        auto viewJ = ctx_.app.journal ("View");
        auto const page = view().dirInsert (keylet::ownerDir (account_),
            slePreauth->key(), describeOwnerDir (account_));

        JLOG(j_.trace())
            << "Adding DepositPreauth to owner directory "
            << to_string (slePreauth->key())
            << ": " << (page ? "success" : "failure");

        if (! page)
            return tecDIR_FULL;

        slePreauth->setFieldU64 (sfOwnerNode, *page);

//如果我们成功了，新条目将与创建者的保留计数。
        adjustOwnerCount (view(), sleOwner, 1, viewJ);
    }
    else
    {
//验证他们请求删除的预身份验证项是否为
//在分类帐中。
        AccountID const unauth {ctx_.tx[sfUnauthorize]};
        uint256 const preauthIndex {getDepositPreauthIndex (account_, unauth)};
        auto slePreauth = view().peek (keylet::depositPreauth (preauthIndex));

        if (! slePreauth)
        {
//在预索赔中应该发现错误。
            JLOG(j_.warn()) << "Selected DepositPreauth does not exist.";
            return tecNO_ENTRY;
        }

        auto viewJ = ctx_.app.journal ("View");
        std::uint64_t const page {(*slePreauth)[sfOwnerNode]};
        if (! view().dirRemove (
            keylet::ownerDir (account_), page, preauthIndex, true))
        {
            JLOG(j_.warn()) << "Unable to delete DepositPreauth from owner.";
            return tefBAD_LEDGER;
        }

//如果成功，请更新存款授权所有者的准备金。
        auto const sleOwner = view().peek (keylet::account (account_));
        adjustOwnerCount (view(), sleOwner, -1, viewJ);

//从分类帐中删除DepositPreauth。
        view().erase (slePreauth);
    }
    return tesSUCCESS;
}

} //命名空间波纹
