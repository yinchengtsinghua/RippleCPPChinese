
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

#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/net/RPCErr.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/impl/Tuning.h>

namespace ripple {

//{
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//}
Json::Value doLedgerRequest (RPC::Context& context)
{
    auto const hasHash = context.params.isMember (jss::ledger_hash);
    auto const hasIndex = context.params.isMember (jss::ledger_index);
    std::uint32_t ledgerIndex = 0;

    auto& ledgerMaster = context.app.getLedgerMaster();
    LedgerHash ledgerHash;

    if ((hasHash && hasIndex) || !(hasHash || hasIndex))
    {
        return RPC::make_param_error(
            "Exactly one of ledger_hash and ledger_index can be set.");
    }

    context.loadType = Resource::feeHighBurdenRPC;

    if (hasHash)
    {
        auto const& jsonHash = context.params[jss::ledger_hash];
        if (!jsonHash.isString() || !ledgerHash.SetHex (jsonHash.asString ()))
            return RPC::invalid_field_error (jss::ledger_hash);
    }
    else
    {
        auto const& jsonIndex = context.params[jss::ledger_index];
        if (!jsonIndex.isInt())
            return RPC::invalid_field_error (jss::ledger_index);

//我们需要一个经过验证的分类账来从序列中获取散列值
        if (ledgerMaster.getValidatedLedgerAge() >
            RPC::Tuning::maxValidatedLedgerAge)
            return rpcError (rpcNO_CURRENT);

        ledgerIndex = jsonIndex.asInt();
        auto ledger = ledgerMaster.getValidatedLedger();

        if (ledgerIndex >= ledger->info().seq)
            return RPC::make_param_error("Ledger index too large");
        if (ledgerIndex <= 0)
            return RPC::make_param_error("Ledger index too small");

        auto const j = context.app.journal("RPCHandler");
//尝试从已验证的分类帐中获取所需分类帐的哈希值
        auto neededHash = hashOfSeq(*ledger, ledgerIndex, j);
        if (! neededHash)
        {
//找到一个更可能具有所需分类账哈希的分类账
            auto const refIndex = getCandidateLedger(ledgerIndex);
            auto refHash = hashOfSeq(*ledger, refIndex, j);
            assert(refHash);

            ledger = ledgerMaster.getLedgerByHash (*refHash);
            if (! ledger)
            {
//我们没有分类帐，我们需要弄清楚哪个分类帐
//他们想要。设法得到它。

                if (auto il = context.app.getInboundLedgers().acquire (
                        *refHash, refIndex, InboundLedger::Reason::GENERIC))
                {
                    Json::Value jvResult = RPC::make_error(
                        rpcLGR_NOT_FOUND,
                            "acquiring ledger containing requested index");
                    jvResult[jss::acquiring] = getJson (LedgerFill (*il));
                    return jvResult;
                }

                if (auto il = context.app.getInboundLedgers().find (*refHash))
                {
                    Json::Value jvResult = RPC::make_error(
                        rpcLGR_NOT_FOUND,
                            "acquiring ledger containing requested index");
                    jvResult[jss::acquiring] = il->getJson (0);
                    return jvResult;
                }

//可能应用程序正在关闭
                return Json::Value();
            }

            neededHash = hashOfSeq(*ledger, ledgerIndex, j);
        }
        assert (neededHash);
ledgerHash = neededHash ? *neededHash : beast::zero; //克莱
    }

//尝试获取所需的分类帐
//验证所有节点，即使我们认为我们拥有它
    auto ledger = context.app.getInboundLedgers().acquire (
        ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);

//在独立模式下，从分类帐缓存接受分类帐
    if (! ledger && context.app.config().standalone())
        ledger = ledgerMaster.getLedgerByHash (ledgerHash);

    if (ledger)
    {
//我们已经核实/获取了整个分类账
        Json::Value jvResult;
        jvResult[jss::ledger_index] = ledger->info().seq;
        addJson (jvResult, {*ledger, 0});
        return jvResult;
    }

    if (auto il = context.app.getInboundLedgers().find (ledgerHash))
        return il->getJson (0);

    return RPC::make_error (
        rpcNOT_READY, "findCreate failed to return an inbound ledger");
}

} //涟漪
