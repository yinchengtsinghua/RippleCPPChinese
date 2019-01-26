
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

#include <ripple/app/main/Application.h>
#include <ripple/basics/strHex.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//{
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//…
//}
Json::Value doLedgerEntry (RPC::Context& context)
{
    std::shared_ptr<ReadView const> lpLedger;
    auto jvResult = RPC::lookupLedger (lpLedger, context);

    if (!lpLedger)
        return jvResult;

    uint256         uNodeIndex;
    bool            bNodeBinary = false;
    LedgerEntryType expectedType = ltANY;

    if (context.params.isMember (jss::index))
    {
        uNodeIndex.SetHex (context.params[jss::index].asString());
    }
    else if (context.params.isMember (jss::account_root))
    {
        expectedType = ltACCOUNT_ROOT;
        auto const account = parseBase58<AccountID>(
            context.params[jss::account_root].asString());
        if (! account || account->isZero())
            jvResult[jss::error] = "malformedAddress";
        else
            uNodeIndex = keylet::account(*account).key;
    }
    else if (context.params.isMember (jss::check))
    {
        expectedType = ltCHECK;
        uNodeIndex.SetHex (context.params[jss::check].asString());
    }
    else if (context.params.isMember (jss::deposit_preauth))
    {
        expectedType = ltDEPOSIT_PREAUTH;

        if (!context.params[jss::deposit_preauth].isObject())
        {
            if (! context.params[jss::deposit_preauth].isString() ||
                ! uNodeIndex.SetHex (
                    context.params[jss::deposit_preauth].asString()))
            {
                uNodeIndex = beast::zero;
                jvResult[jss::error] = "malformedRequest";
            }
        }
        else if (!context.params[jss::deposit_preauth].isMember (jss::owner)
            || !context.params[jss::deposit_preauth][jss::owner].isString()
            || !context.params[jss::deposit_preauth].isMember (jss::authorized)
            || !context.params[jss::deposit_preauth][jss::authorized].isString())
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else
        {
            auto const owner = parseBase58<AccountID>(context.params[
                jss::deposit_preauth][jss::owner].asString());

            auto const authorized = parseBase58<AccountID>(context.params[
                jss::deposit_preauth][jss::authorized].asString());

            if (! owner)
                jvResult[jss::error] = "malformedOwner";
            else if (! authorized)
                jvResult[jss::error] = "malformedAuthorized";
            else
                uNodeIndex = keylet::depositPreauth (*owner, *authorized).key;
        }
    }
    else if (context.params.isMember (jss::directory))
    {
        expectedType = ltDIR_NODE;
        if (context.params[jss::directory].isNull())
        {
            jvResult[jss::error]   = "malformedRequest";
        }
        else if (!context.params[jss::directory].isObject())
        {
            uNodeIndex.SetHex (context.params[jss::directory].asString ());
        }
        else if (context.params[jss::directory].isMember (jss::sub_index)
            && !context.params[jss::directory][jss::sub_index].isIntegral ())
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else
        {
            std::uint64_t  uSubIndex
                = context.params[jss::directory].isMember (jss::sub_index)
                ? context.params[jss::directory][jss::sub_index].asUInt () : 0;

            if (context.params[jss::directory].isMember (jss::dir_root))
            {
                if (context.params[jss::directory].isMember (jss::owner))
                {
//不能同时指定dir_root和owner。
                    jvResult[jss::error] = "malformedRequest";
                }
                else
                {
                    uint256 uDirRoot;
                    uDirRoot.SetHex (
                        context.params[jss::directory][jss::dir_root].asString());

                    uNodeIndex = getDirNodeIndex (uDirRoot, uSubIndex);
                }
            }
            else if (context.params[jss::directory].isMember (jss::owner))
            {
                auto const ownerID = parseBase58<AccountID>(
                    context.params[jss::directory][jss::owner].asString());

                if (! ownerID)
                {
                    jvResult[jss::error] = "malformedAddress";
                }
                else
                {
                    uint256 uDirRoot = getOwnerDirIndex (*ownerID);
                    uNodeIndex = getDirNodeIndex (uDirRoot, uSubIndex);
                }
            }
            else
            {
                jvResult[jss::error] = "malformedRequest";
            }
        }
    }
    else if (context.params.isMember (jss::escrow))
    {
        expectedType = ltESCROW;
        if (!context.params[jss::escrow].isObject ())
        {
            uNodeIndex.SetHex (context.params[jss::escrow].asString ());
        }
        else if (!context.params[jss::escrow].isMember (jss::owner)
            || !context.params[jss::escrow].isMember (jss::seq)
            || !context.params[jss::escrow][jss::seq].isIntegral ())
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else
        {
            auto const id = parseBase58<AccountID>(
                context.params[jss::escrow][jss::owner].asString());
            if (! id)
                jvResult[jss::error] = "malformedOwner";
            else
                uNodeIndex = keylet::escrow (*id,
                    context.params[jss::escrow][jss::seq].asUInt()).key;
        }
    }
    else if (context.params.isMember (jss::generator))
    {
        jvResult[jss::error] = "deprecatedFeature";
    }
    else if (context.params.isMember (jss::offer))
    {
        expectedType = ltOFFER;
        if (!context.params[jss::offer].isObject())
        {
            uNodeIndex.SetHex (context.params[jss::offer].asString ());
        }
        else if (!context.params[jss::offer].isMember (jss::account)
            || !context.params[jss::offer].isMember (jss::seq)
            || !context.params[jss::offer][jss::seq].isIntegral ())
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else
        {
            auto const id = parseBase58<AccountID>(
                context.params[jss::offer][jss::account].asString());
            if (! id)
                jvResult[jss::error] = "malformedAddress";
            else
                uNodeIndex = getOfferIndex (*id,
                    context.params[jss::offer][jss::seq].asUInt ());
        }
    }
    else if (context.params.isMember (jss::payment_channel))
    {
        expectedType = ltPAYCHAN;
        uNodeIndex.SetHex (context.params[jss::payment_channel].asString ());
    }
    else if (context.params.isMember (jss::ripple_state))
    {
        expectedType = ltRIPPLE_STATE;
        Currency         uCurrency;
        Json::Value     jvRippleState   = context.params[jss::ripple_state];

        if (!jvRippleState.isObject()
            || !jvRippleState.isMember (jss::currency)
            || !jvRippleState.isMember (jss::accounts)
            || !jvRippleState[jss::accounts].isArray()
            || 2 != jvRippleState[jss::accounts].size ()
            || !jvRippleState[jss::accounts][0u].isString ()
            || !jvRippleState[jss::accounts][1u].isString ()
            || (jvRippleState[jss::accounts][0u].asString ()
                == jvRippleState[jss::accounts][1u].asString ())
           )
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else
        {
            auto const id1 = parseBase58<AccountID>(
                jvRippleState[jss::accounts][0u].asString());
            auto const id2 = parseBase58<AccountID>(
                jvRippleState[jss::accounts][1u].asString());
            if (! id1 || ! id2)
            {
                jvResult[jss::error] = "malformedAddress";
            }
            else if (!to_currency (uCurrency,
                jvRippleState[jss::currency].asString()))
            {
                jvResult[jss::error] = "malformedCurrency";
            }
            else
            {
                uNodeIndex = getRippleStateIndex(
                    *id1, *id2, uCurrency);
            }
        }
    }
    else
    {
        jvResult[jss::error] = "unknownOption";
    }

    if (uNodeIndex.isNonZero ())
    {
        auto const sleNode = lpLedger->read(keylet::unchecked(uNodeIndex));
        if (context.params.isMember(jss::binary))
            bNodeBinary = context.params[jss::binary].asBool();

        if (!sleNode)
        {
//找不到。
            jvResult[jss::error] = "entryNotFound";
        }
        else if ((expectedType != ltANY) &&
            (expectedType != sleNode->getType()))
        {
            jvResult[jss::error] = "malformedRequest";
        }
        else if (bNodeBinary)
        {
            Serializer s;

            sleNode->add (s);

            jvResult[jss::node_binary] = strHex (s.peekData ());
            jvResult[jss::index]       = to_string (uNodeIndex);
        }
        else
        {
            jvResult[jss::node]        = sleNode->getJson (0);
            jvResult[jss::index]       = to_string (uNodeIndex);
        }
    }

    return jvResult;
}

} //涟漪
