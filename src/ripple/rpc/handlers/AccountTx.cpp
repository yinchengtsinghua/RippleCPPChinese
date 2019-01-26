
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
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/json/json_value.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/Role.h>

namespace ripple {

//{
//帐户：帐户，
//分类帐索引最小：分类帐索引//可选，默认为最早
//Ledger_index_max:Ledger_index，//可选，默认为最新
//binary:boolean，//可选，默认为false
//forward:boolean，//可选，默认为false
//限制：整数，//可选
//marker:opaque//可选，继续上一个查询
//}
Json::Value doAccountTx (RPC::Context& context)
{
    auto& params = context.params;

    int limit = params.isMember (jss::limit) ?
            params[jss::limit].asUInt () : -1;
    bool bBinary = params.isMember (jss::binary) && params[jss::binary].asBool ();
    bool bForward = params.isMember (jss::forward) && params[jss::forward].asBool ();
    std::uint32_t   uLedgerMin;
    std::uint32_t   uLedgerMax;
    std::uint32_t   uValidatedMin;
    std::uint32_t   uValidatedMax;
    bool bValidated = context.ledgerMaster.getValidatedRange (
        uValidatedMin, uValidatedMax);

    if (!bValidated)
    {
//没有有效的分类帐范围。
        return rpcError (rpcLGR_IDXS_INVALID);
    }

    if (!params.isMember (jss::account))
        return rpcError (rpcINVALID_PARAMS);

    auto const account = parseBase58<AccountID>(
        params[jss::account].asString());
    if (! account)
        return rpcError (rpcACT_MALFORMED);

    context.loadType = Resource::feeMediumBurdenRPC;

    if (params.isMember (jss::ledger_index_min) ||
        params.isMember (jss::ledger_index_max))
    {
        std::int64_t iLedgerMin  = params.isMember (jss::ledger_index_min)
                ? params[jss::ledger_index_min].asInt () : -1;
        std::int64_t iLedgerMax  = params.isMember (jss::ledger_index_max)
                ? params[jss::ledger_index_max].asInt () : -1;

        uLedgerMin  = iLedgerMin == -1 ? uValidatedMin :
            ((iLedgerMin >= uValidatedMin) ? iLedgerMin : uValidatedMin);
        uLedgerMax  = iLedgerMax == -1 ? uValidatedMax :
            ((iLedgerMax <= uValidatedMax) ? iLedgerMax : uValidatedMax);

        if (uLedgerMax < uLedgerMin)
            return rpcError (rpcLGR_IDXS_INVALID);
    }
    else if(params.isMember (jss::ledger_hash) ||
            params.isMember (jss::ledger_index))
    {
        std::shared_ptr<ReadView const> ledger;
        auto ret = RPC::lookupLedger (ledger, context);

        if (! ledger)
            return ret;

        if (! ret[jss::validated].asBool() ||
            (ledger->info().seq > uValidatedMax) ||
            (ledger->info().seq < uValidatedMin))
        {
            return rpcError (rpcLGR_NOT_VALIDATED);
        }

        uLedgerMin = uLedgerMax = ledger->info().seq;
    }
    else
    {
        uLedgerMin = uValidatedMin;
        uLedgerMax = uValidatedMax;
    }

    Json::Value resumeToken;

    if (params.isMember(jss::marker))
         resumeToken = params[jss::marker];

#ifndef BEAST_DEBUG

    try
    {
#endif
        Json::Value ret (Json::objectValue);

        ret[jss::account] = context.app.accountIDCache().toBase58(*account);
        Json::Value& jvTxns = (ret[jss::transactions] = Json::arrayValue);

        if (bBinary)
        {
            auto txns = context.netOps.getTxsAccountB (
                *account, uLedgerMin, uLedgerMax, bForward, resumeToken, limit,
                isUnlimited (context.role));

            for (auto& it: txns)
            {
                Json::Value& jvObj = jvTxns.append (Json::objectValue);

                jvObj[jss::tx_blob] = std::get<0> (it);
                jvObj[jss::meta] = std::get<1> (it);

                std::uint32_t uLedgerIndex = std::get<2> (it);

                jvObj[jss::ledger_index] = uLedgerIndex;
                jvObj[jss::validated] = bValidated &&
                    uValidatedMin <= uLedgerIndex &&
                    uValidatedMax >= uLedgerIndex;
            }
        }
        else
        {
            auto txns = context.netOps.getTxsAccount (
                *account, uLedgerMin, uLedgerMax, bForward, resumeToken, limit,
                isUnlimited (context.role));

            for (auto& it: txns)
            {
                Json::Value& jvObj = jvTxns.append (Json::objectValue);

                if (it.first)
                    jvObj[jss::tx] = it.first->getJson (1);

                if (it.second)
                {
                    auto meta = it.second->getJson (1);
                    addPaymentDeliveredAmount (meta, context, it.first, it.second);
                    jvObj[jss::meta] = std::move(meta);

                    std::uint32_t uLedgerIndex = it.second->getLgrSeq ();

                    jvObj[jss::validated] = bValidated &&
                        uValidatedMin <= uLedgerIndex &&
                        uValidatedMax >= uLedgerIndex;
                }

            }
        }

//添加有关原始查询的信息
        ret[jss::ledger_index_min] = uLedgerMin;
        ret[jss::ledger_index_max] = uLedgerMax;
        if (params.isMember (jss::limit))
            ret[jss::limit]        = limit;
        if (resumeToken)
            ret[jss::marker] = resumeToken;

        return ret;
#ifndef BEAST_DEBUG
    }
    catch (std::exception const&)
    {
        return rpcError (rpcINTERNAL);
    }

#endif
}

} //涟漪
