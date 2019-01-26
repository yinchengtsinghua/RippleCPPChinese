
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

#include <ripple/basics/Log.h>
#include <ripple/app/tx/apply.h>
#include <ripple/app/tx/applySteps.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/protocol/Feature.h>

namespace ripple {

//这些标志与hashrouter.h中定义为sf_private1-4的标志相同。
#define SF_SIGBAD      SF_PRIVATE1    //签名错误
#define SF_SIGGOOD     SF_PRIVATE2    //签名很好
#define SF_LOCALBAD    SF_PRIVATE3    //本地检查失败
#define SF_LOCALGOOD   SF_PRIVATE4    //本地检查通过

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::pair<Validity, std::string>
checkValidity(HashRouter& router,
    STTx const& tx, Rules const& rules,
        Config const& config)
{
    auto const allowMultiSign =
        rules.enabled(featureMultiSign);

    auto const id = tx.getTransactionID();
    auto const flags = router.getFlags(id);
    if (flags & SF_SIGBAD)
//签名已知错误
        return {Validity::SigBad, "Transaction has bad signature."};

    if (!(flags & SF_SIGGOOD))
    {
//不知道签名状态。检查一下。
        auto const sigVerify = tx.checkSign(allowMultiSign);
        if (! sigVerify.first)
        {
            router.setFlags(id, SF_SIGBAD);
            return {Validity::SigBad, sigVerify.second};
        }
        router.setFlags(id, SF_SIGGOOD);
    }

//签名现在已知良好
    if (flags & SF_LOCALBAD)
//…但是当地的支票
//是坏的。
        return {Validity::SigGoodOnly, "Local checks failed."};

    if (flags & SF_LOCALGOOD)
//…还有当地的支票
//众所周知。
        return {Validity::Valid, ""};

//本地检查吗
    std::string reason;
    if (!passesLocalChecks(tx, reason))
    {
        router.setFlags(id, SF_LOCALBAD);
        return {Validity::SigGoodOnly, reason};
    }
    router.setFlags(id, SF_LOCALGOOD);
    return {Validity::Valid, ""};
}

void
forceValidity(HashRouter& router, uint256 const& txid,
    Validity validity)
{
    int flags = 0;
    switch (validity)
    {
    case Validity::Valid:
        flags |= SF_LOCALGOOD;
//跌倒
    case Validity::SigGoodOnly:
        flags |= SF_SIGGOOD;
//跌倒
    case Validity::SigBad:
//直接打电话是愚蠢的
        break;
    }
    if (flags)
        router.setFlags(txid, flags);
}

std::pair<TER, bool>
apply (Application& app, OpenView& view,
    STTx const& tx, ApplyFlags flags,
        beast::Journal j)
{
    STAmountSO saved(view.info().parentCloseTime);
    auto pfresult = preflight(app, view.rules(), tx, flags, j);
    auto pcresult = preclaim(pfresult, app, view);
    return doApply(pcresult, app, view);
}

ApplyResult
applyTransaction (Application& app, OpenView& view,
    STTx const& txn,
        bool retryAssured, ApplyFlags flags,
            beast::Journal j)
{
//如果不需要重试事务，则返回false。
    if (retryAssured)
        flags = flags | tapRETRY;

    JLOG (j.debug()) << "TXN " << txn.getTransactionID ()
        << (retryAssured ? "/retry" : "/final");

    try
    {
        auto const result = apply(app, view, txn, flags, j);
        if (result.second)
        {
            JLOG (j.debug())
                << "Transaction applied: " << transHuman (result.first);
            return ApplyResult::Success;
        }

        if (isTefFailure (result.first) || isTemMalformed (result.first) ||
            isTelLocal (result.first))
        {
//失败
            JLOG (j.debug())
                << "Transaction failure: " << transHuman (result.first);
            return ApplyResult::Fail;
        }

        JLOG (j.debug())
            << "Transaction retry: " << transHuman (result.first);
        return ApplyResult::Retry;
    }
    catch (std::exception const&)
    {
        JLOG (j.warn()) << "Throws";
        return ApplyResult::Fail;
    }
}

} //涟漪
