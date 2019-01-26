
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
    版权所有（c）2012-2014 Ripple Labs Inc.

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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/tx/apply.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>

namespace ripple {

static NetworkOPs::FailHard getFailHard (RPC::Context const& context)
{
    return NetworkOPs::doFailHard (
        context.params.isMember ("fail_hard")
        && context.params["fail_hard"].asBool ());
}

//{
//tx_json:<object>，
//秘密：<secret>
//}
Json::Value doSubmit (RPC::Context& context)
{
    context.loadType = Resource::feeMediumBurdenRPC;

    if (!context.params.isMember (jss::tx_blob))
    {
        auto const failType = getFailHard (context);

        if (context.role != Role::ADMIN && !context.app.config().canSign())
            return RPC::make_error (rpcNOT_SUPPORTED,
                "Signing is not supported by this server.");

        auto ret = RPC::transactionSubmit (
            context.params, failType, context.role,
            context.ledgerMaster.getValidatedLedgerAge(),
            context.app, RPC::getProcessTxnFn (context.netOps));

        ret[jss::deprecated] = "Signing support in the 'submit' command has been "
                               "deprecated and will be removed in a future version "
                               "of the server. Please migrate to a standalone "
                               "signing tool.";

        return ret;
    }

    Json::Value jvResult;

    std::pair<Blob, bool> ret(strUnHex (context.params[jss::tx_blob].asString ()));

    if (!ret.second || !ret.first.size ())
        return rpcError (rpcINVALID_PARAMS);

    SerialIter sitTrans (makeSlice(ret.first));

    std::shared_ptr<STTx const> stpTrans;

    try
    {
        stpTrans = std::make_shared<STTx const> (std::ref (sitTrans));
    }
    catch (std::exception& e)
    {
        jvResult[jss::error]        = "invalidTransaction";
        jvResult[jss::error_exception] = e.what ();

        return jvResult;
    }


    {
        if (!context.app.checkSigs())
            forceValidity(context.app.getHashRouter(),
                stpTrans->getTransactionID(), Validity::SigGoodOnly);
        auto validity = checkValidity(context.app.getHashRouter(),
            *stpTrans, context.ledgerMaster.getCurrentLedger()->rules(),
                context.app.config());
        if (validity.first != Validity::Valid)
        {
            jvResult[jss::error] = "invalidTransaction";
            jvResult[jss::error_exception] = "fails local checks: " + validity.second;

            return jvResult;
        }
    }

    std::string reason;
    auto tpTrans = std::make_shared<Transaction> (
        stpTrans, reason, context.app);
    if (tpTrans->getStatus() != NEW)
    {
        jvResult[jss::error]            = "invalidTransaction";
        jvResult[jss::error_exception] = "fails local checks: " + reason;

        return jvResult;
    }

    try
    {
        auto const failType = getFailHard (context);

        context.netOps.processTransaction (
            tpTrans, isUnlimited (context.role), true, failType);
    }
    catch (std::exception& e)
    {
        jvResult[jss::error]           = "internalSubmit";
        jvResult[jss::error_exception] = e.what ();

        return jvResult;
    }


    try
    {
        jvResult[jss::tx_json] = tpTrans->getJson (0);
        jvResult[jss::tx_blob] = strHex (
            tpTrans->getSTransaction ()->getSerializer ().peekData ());

        if (temUNCERTAIN != tpTrans->getResult ())
        {
            std::string sToken;
            std::string sHuman;

            transResultInfo (tpTrans->getResult (), sToken, sHuman);

            jvResult[jss::engine_result]           = sToken;
            jvResult[jss::engine_result_code]      = tpTrans->getResult ();
            jvResult[jss::engine_result_message]   = sHuman;
        }

        return jvResult;
    }
    catch (std::exception& e)
    {
        jvResult[jss::error]           = "internalJson";
        jvResult[jss::error_exception] = e.what ();

        return jvResult;
    }
}

} //涟漪
