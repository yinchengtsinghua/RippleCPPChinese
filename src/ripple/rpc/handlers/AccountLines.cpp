
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
#include <ripple/app/paths/RippleState.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

namespace ripple {

struct VisitData
{
    std::vector <RippleState::pointer> items;
    AccountID const& accountID;
    bool hasPeer;
    AccountID const& raPeerAccount;
};

void addLine (Json::Value& jsonLines, RippleState const& line)
{
    STAmount const& saBalance (line.getBalance ());
    STAmount const& saLimit (line.getLimit ());
    STAmount const& saLimitPeer (line.getLimitPeer ());
    Json::Value& jPeer (jsonLines.append (Json::objectValue));

    jPeer[jss::account] = to_string (line.getAccountIDPeer ());
//如果活期账户持有其他账户，则报告的金额为正。
//帐号很贵。
//
//如果其他账户保持当前状态，则报告的金额为负数。
//帐号很贵。
    jPeer[jss::balance] = saBalance.getText ();
    jPeer[jss::currency] = to_string (saBalance.issue ().currency);
    jPeer[jss::limit] = saLimit.getText ();
    jPeer[jss::limit_peer] = saLimitPeer.getText ();
    jPeer[jss::quality_in] = line.getQualityIn ().value;
    jPeer[jss::quality_out] = line.getQualityOut ().value;
    if (line.getAuth ())
        jPeer[jss::authorized] = true;
    if (line.getAuthPeer ())
        jPeer[jss::peer_authorized] = true;
    if (line.getNoRipple ())
        jPeer[jss::no_ripple] = true;
    if (line.getNoRipplePeer ())
        jPeer[jss::no_ripple_peer] = true;
    if (line.getFreeze ())
        jPeer[jss::freeze] = true;
    if (line.getFreezePeer ())
        jPeer[jss::freeze_peer] = true;
}

//{
//账户：<account><account_public_key>
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//限制：整数//可选
//marker:opaque//可选，继续上一个查询
//}
Json::Value doAccountLines (RPC::Context& context)
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

    if (auto jv = RPC::accountFromString (accountID, strIdent))
    {
        for (auto it = jv.begin (); it != jv.end (); ++it)
            result[it.memberName ()] = *it;
        return result;
    }

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    std::string strPeer;
    if (params.isMember (jss::peer))
        strPeer = params[jss::peer].asString ();
    auto hasPeer = ! strPeer.empty ();

    AccountID raPeerAccount;
    if (hasPeer)
    {
        if (auto jv = RPC::accountFromString (raPeerAccount, strPeer))
        {
            for (auto it = jv.begin (); it != jv.end (); ++it)
                result[it.memberName ()] = *it;
            return result;
        }
    }

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountLines, context))
        return *err;

    Json::Value& jsonLines (result[jss::lines] = Json::arrayValue);
    VisitData visitData = {{}, accountID, hasPeer, raPeerAccount};
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
        auto const sleLine = ledger->read({ltRIPPLE_STATE, startAfter});

        if (! sleLine)
            return rpcError (rpcINVALID_PARAMS);

        if (sleLine->getFieldAmount (sfLowLimit).getIssuer () == accountID)
            startHint = sleLine->getFieldU64 (sfLowNode);
        else if (sleLine->getFieldAmount (sfHighLimit).getIssuer () == accountID)
            startHint = sleLine->getFieldU64 (sfHighNode);
        else
            return rpcError (rpcINVALID_PARAMS);

//调用方提供了第一行（startafter），将其作为第一个结果添加
        auto const line = RippleState::makeItem (accountID, sleLine);
        if (line == nullptr)
            return rpcError (rpcINVALID_PARAMS);

        addLine (jsonLines, *line);
        visitData.items.reserve (reserve);
    }
    else
    {
        startHint = 0;
//我们没有起点，限值应该比要求的高一点。
        visitData.items.reserve (++reserve);
    }

    {
        if (! forEachItemAfter(*ledger, accountID,
                startAfter, startHint, reserve,
            [&visitData](std::shared_ptr<SLE const> const& sleCur)
            {
                auto const line =
                    RippleState::makeItem (visitData.accountID, sleCur);
                if (line != nullptr &&
                    (! visitData.hasPeer ||
                     visitData.raPeerAccount == line->getAccountIDPeer ()))
                {
                    visitData.items.emplace_back (line);
                    return true;
                }

                return false;
            }))
        {
            return rpcError (rpcINVALID_PARAMS);
        }
    }

    if (visitData.items.size () == reserve)
    {
        result[jss::limit] = limit;

        RippleState::pointer line (visitData.items.back ());
        result[jss::marker] = to_string (line->key());
        visitData.items.pop_back ();
    }

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);

    for (auto const& item : visitData.items)
        addLine (jsonLines, *item.get ());

    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

} //涟漪
