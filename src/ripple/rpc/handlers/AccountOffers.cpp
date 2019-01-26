
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
#include <ripple/json/json_value.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/View.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

namespace ripple {

void appendOfferJson (std::shared_ptr<SLE const> const& offer,
                      Json::Value& offers)
{
    STAmount dirRate = amountFromQuality (
          getQuality (offer->getFieldH256 (sfBookDirectory)));
    Json::Value& obj (offers.append (Json::objectValue));
    offer->getFieldAmount (sfTakerPays).setJson (obj[jss::taker_pays]);
    offer->getFieldAmount (sfTakerGets).setJson (obj[jss::taker_gets]);
    obj[jss::seq] = offer->getFieldU32 (sfSequence);
    obj[jss::flags] = offer->getFieldU32 (sfFlags);
    obj[jss::quality] = dirRate.getText ();
    if (offer->isFieldPresent(sfExpiration))
        obj[jss::expiration] = offer->getFieldU32(sfExpiration);
};

//{
//账户：<account><account_public_key>
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//限制：整数//可选
//marker:opaque//可选，继续上一个查询
//}
Json::Value doAccountOffers (RPC::Context& context)
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
            result[it.memberName ()] = (*it);

        return result;
    }

//获取有关帐户的信息。
    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountOffers, context))
        return *err;

    Json::Value& jsonOffers (result[jss::offers] = Json::arrayValue);
    std::vector <std::shared_ptr<SLE const>> offers;
    unsigned int reserve (limit);
    uint256 startAfter;
    std::uint64_t startHint;

    if (params.isMember(jss::marker))
    {
//我们有一个起点。使用结果中的限制-1并使用
//简历的最后一份。
        Json::Value const& marker (params[jss::marker]);

        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        startAfter.SetHex (marker.asString ());
        auto const sleOffer = ledger->read({ltOFFER, startAfter});

        if (! sleOffer || accountID != sleOffer->getAccountID (sfAccount))
        {
            return rpcError (rpcINVALID_PARAMS);
        }

        startHint = sleOffer->getFieldU64(sfOwnerNode);
//调用方提供了第一个报价（startafter），将其作为第一个结果添加
        appendOfferJson(sleOffer, jsonOffers);
        offers.reserve (reserve);
    }
    else
    {
        startHint = 0;
//我们没有起点，限值应该比要求的高一点。
        offers.reserve (++reserve);
    }

    if (! forEachItemAfter(*ledger, accountID,
            startAfter, startHint, reserve,
        [&offers](std::shared_ptr<SLE const> const& offer)
        {
            if (offer->getType () == ltOFFER)
            {
                offers.emplace_back (offer);
                return true;
            }

            return false;
        }))
    {
        return rpcError (rpcINVALID_PARAMS);
    }

    if (offers.size () == reserve)
    {
        result[jss::limit] = limit;

        result[jss::marker] = to_string (offers.back ()->key ());
        offers.pop_back ();
    }

    for (auto const& offer : offers)
        appendOfferJson(offer, jsonOffers);

    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

} //涟漪
