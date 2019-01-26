
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
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/Role.h>

namespace ripple {

//{
//帐户：帐户，
//分类帐索引最小：分类帐索引，
//分类帐索引最大值：分类帐索引，
//binary:boolean，//可选，默认为false
//count:boolean，//可选，默认为false
//降序：布尔值，//可选，默认为false
//offset:integer，//可选，默认为0
//限制：整数//可选
//}
Json::Value doAccountTxOld (RPC::Context& context)
{
    std::uint32_t offset
            = context.params.isMember (jss::offset)
            ? context.params[jss::offset].asUInt () : 0;
    int limit = context.params.isMember (jss::limit)
            ? context.params[jss::limit].asUInt () : -1;
    bool bBinary = context.params.isMember (jss::binary)
            && context.params[jss::binary].asBool ();
    bool bDescending = context.params.isMember (jss::descending)
            && context.params[jss::descending].asBool ();
    bool bCount = context.params.isMember (jss::count)
            && context.params[jss::count].asBool ();
    std::uint32_t   uLedgerMin;
    std::uint32_t   uLedgerMax;
    std::uint32_t   uValidatedMin;
    std::uint32_t   uValidatedMax;
    bool bValidated  = context.ledgerMaster.getValidatedRange (
        uValidatedMin, uValidatedMax);

    if (!context.params.isMember (jss::account))
        return rpcError (rpcINVALID_PARAMS);

    auto const raAccount = parseBase58<AccountID>(
        context.params[jss::account].asString());
    if (! raAccount)
        return rpcError (rpcACT_MALFORMED);

    if (offset > 3000)
        return rpcError (rpcATX_DEPRECATED);

    context.loadType = Resource::feeHighBurdenRPC;

//贬低
    if (context.params.isMember (jss::ledger_min))
    {
        context.params[jss::ledger_index_min]   = context.params[jss::ledger_min];
        bDescending = true;
    }

//贬低
    if (context.params.isMember (jss::ledger_max))
    {
        context.params[jss::ledger_index_max]   = context.params[jss::ledger_max];
        bDescending = true;
    }

    if (context.params.isMember (jss::ledger_index_min)
        || context.params.isMember (jss::ledger_index_max))
    {
        std::int64_t iLedgerMin  = context.params.isMember (jss::ledger_index_min)
                ? context.params[jss::ledger_index_min].asInt () : -1;
        std::int64_t iLedgerMax  = context.params.isMember (jss::ledger_index_max)
                ? context.params[jss::ledger_index_max].asInt () : -1;

        if (!bValidated && (iLedgerMin == -1 || iLedgerMax == -1))
        {
//没有有效的分类帐范围。
            return rpcError (rpcLGR_IDXS_INVALID);
        }

        uLedgerMin  = iLedgerMin == -1 ? uValidatedMin : iLedgerMin;
        uLedgerMax  = iLedgerMax == -1 ? uValidatedMax : iLedgerMax;

        if (uLedgerMax < uLedgerMin)
        {
            return rpcError (rpcLGR_IDXS_INVALID);
        }
    }
    else
    {
        std::shared_ptr<ReadView const> ledger;
        auto ret = RPC::lookupLedger (ledger, context);

        if (!ledger)
            return ret;

        if (! ret[jss::validated].asBool() ||
            (ledger->info().seq > uValidatedMax) ||
            (ledger->info().seq < uValidatedMin))
        {
            return rpcError (rpcLGR_NOT_VALIDATED);
        }

        uLedgerMin = uLedgerMax = ledger->info().seq;
    }

    int count = 0;

#ifndef BEAST_DEBUG

    try
    {
#endif

        Json::Value ret (Json::objectValue);

        ret[jss::account] = context.app.accountIDCache().toBase58(*raAccount);
        Json::Value& jvTxns = (ret[jss::transactions] = Json::arrayValue);

        if (bBinary)
        {
            auto txns = context.netOps.getAccountTxsB (
                *raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                isUnlimited (context.role));

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value& jvObj = jvTxns.append (Json::objectValue);

                std::uint32_t  uLedgerIndex = std::get<2> (*it);
                jvObj[jss::tx_blob]            = std::get<0> (*it);
                jvObj[jss::meta]               = std::get<1> (*it);
                jvObj[jss::ledger_index]       = uLedgerIndex;
                jvObj[jss::validated]
                        = bValidated
                        && uValidatedMin <= uLedgerIndex
                        && uValidatedMax >= uLedgerIndex;

            }
        }
        else
        {
            auto txns = context.netOps.getAccountTxs (
                *raAccount, uLedgerMin, uLedgerMax, bDescending, offset, limit,
                isUnlimited (context.role));

            for (auto it = txns.begin (), end = txns.end (); it != end; ++it)
            {
                ++count;
                Json::Value&    jvObj = jvTxns.append (Json::objectValue);

                if (it->first)
                    jvObj[jss::tx]             = it->first->getJson (1);

                if (it->second)
                {
                    std::uint32_t uLedgerIndex = it->second->getLgrSeq ();

                    auto meta = it->second->getJson(0);
                    addPaymentDeliveredAmount(meta, context, it->first, it->second);
                    jvObj[jss::meta] = std::move(meta);

                    jvObj[jss::validated]
                            = bValidated
                            && uValidatedMin <= uLedgerIndex
                            && uValidatedMax >= uLedgerIndex;
                }

            }
        }

//添加有关原始查询的信息
        ret[jss::ledger_index_min] = uLedgerMin;
        ret[jss::ledger_index_max] = uLedgerMax;
        ret[jss::validated]
                = bValidated
                && uValidatedMin <= uLedgerMin
                && uValidatedMax >= uLedgerMax;
        ret[jss::offset]           = offset;

//我们不再返回完整的计数，而是只返回
//交易。计算这个计数是两个昂贵的，而这个API是
//无论如何都不推荐。
        if (bCount)
            ret[jss::count]        = count;

        if (context.params.isMember (jss::limit))
            ret[jss::limit]        = limit;


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
