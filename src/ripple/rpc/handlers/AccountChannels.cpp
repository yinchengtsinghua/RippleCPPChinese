
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
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/View.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>
namespace ripple {

void addChannel (Json::Value& jsonLines, SLE const& line)
{
    Json::Value& jDst (jsonLines.append (Json::objectValue));
    jDst[jss::channel_id] = to_string (line.key ());
    jDst[jss::account] = to_string (line[sfAccount]);
    jDst[jss::destination_account] = to_string (line[sfDestination]);
    jDst[jss::amount] = line[sfAmount].getText ();
    jDst[jss::balance] = line[sfBalance].getText ();
    if (publicKeyType(line[sfPublicKey]))
    {
        PublicKey const pk (line[sfPublicKey]);
        jDst[jss::public_key] = toBase58 (TokenType::AccountPublic, pk);
        jDst[jss::public_key_hex] = strHex (pk);
    }
    jDst[jss::settle_delay] = line[sfSettleDelay];
    if (auto const& v = line[~sfExpiration])
        jDst[jss::expiration] = *v;
    if (auto const& v = line[~sfCancelAfter])
        jDst[jss::cancel_after] = *v;
    if (auto const& v = line[~sfSourceTag])
        jDst[jss::source_tag] = *v;
    if (auto const& v = line[~sfDestinationTag])
        jDst[jss::destination_tag] = *v;
}

//{
//账户：<account><account_public_key>
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//限制：整数//可选
//marker:opaque//可选，继续上一个查询
//}
Json::Value doAccountChannels (RPC::Context& context)
{
    auto const& params (context.params);
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);
    if (! ledger)
        return result;

    std::string strIdent (params[jss::account].asString ());
    AccountID accountID;

    if (auto const actResult = RPC::accountFromString (accountID, strIdent))
        return actResult;

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    std::string strDst;
    if (params.isMember (jss::destination_account))
        strDst = params[jss::destination_account].asString ();
    auto hasDst = ! strDst.empty ();

    AccountID raDstAccount;
    if (hasDst)
    {
        if (auto const actResult = RPC::accountFromString (raDstAccount, strDst))
            return actResult;
    }

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountChannels, context))
        return *err;

    Json::Value jsonChannels{Json::arrayValue};
    struct VisitData
    {
        std::vector <std::shared_ptr<SLE const>> items;
        AccountID const& accountID;
        bool hasDst;
        AccountID const& raDstAccount;
    };
    VisitData visitData = {{}, accountID, hasDst, raDstAccount};
    unsigned int reserve (limit);
    uint256 startAfter;
    std::uint64_t startHint;

    if (params.isMember (jss::marker))
    {
//我们有一个起点。使用结果中的限制-1并使用
//简历的最后一份。
        Json::Value const& marker (params[jss::marker]);

        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        startAfter.SetHex (marker.asString ());
        auto const sleChannel = ledger->read({ltPAYCHAN, startAfter});

        if (! sleChannel)
            return rpcError (rpcINVALID_PARAMS);

        if (sleChannel->getFieldAmount (sfLowLimit).getIssuer () == accountID)
            startHint = sleChannel->getFieldU64 (sfLowNode);
        else if (sleChannel->getFieldAmount (sfHighLimit).getIssuer () == accountID)
            startHint = sleChannel->getFieldU64 (sfHighNode);
        else
            return rpcError (rpcINVALID_PARAMS);

        addChannel (jsonChannels, *sleChannel);
        visitData.items.reserve (reserve);
    }
    else
    {
        startHint = 0;
//我们没有起点，限值应该比要求的高一点。
        visitData.items.reserve (++reserve);
    }

    if (! forEachItemAfter(*ledger, accountID,
            startAfter, startHint, reserve,
        [&visitData](std::shared_ptr<SLE const> const& sleCur)
        {

            if (sleCur && sleCur->getType () == ltPAYCHAN &&
                (! visitData.hasDst ||
                 visitData.raDstAccount == (*sleCur)[sfDestination]))
            {
                visitData.items.emplace_back (sleCur);
                return true;
            }

            return false;
        }))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    if (visitData.items.size () == reserve)
    {
        result[jss::limit] = limit;

        result[jss::marker] = to_string (visitData.items.back()->key());
        visitData.items.pop_back ();
    }

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);

    for (auto const& item : visitData.items)
        addChannel (jsonChannels, *item);

    context.loadType = Resource::feeMediumBurdenRPC;
    result[jss::channels] = std::move(jsonChannels);
    return result;
}

} //涟漪
