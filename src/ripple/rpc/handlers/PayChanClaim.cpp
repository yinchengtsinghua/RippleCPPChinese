
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
#include <ripple/basics/StringUtilities.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PayChan.h>
#include <ripple/protocol/STAccount.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

#include <boost/optional.hpp>

namespace ripple {

//{
//秘钥：<签署秘钥>
//通道ID:256位通道ID
//删除：64位uint（字符串）
//}
Json::Value doChannelAuthorize (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p : {jss::secret, jss::channel_id, jss::amount})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    Json::Value result;
    auto const keypair = RPC::keypairForSignature (params, result);
    if (RPC::contains_error (result))
        return result;

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, XRPAmount (drops));

    try
    {
        auto const buf = sign (keypair.first, keypair.second, msg.slice ());
        result[jss::signature] = strHex (buf);
    }
    catch (std::exception&)
    {
        result =
            RPC::make_error (rpcINTERNAL, "Exception occurred during signing.");
    }
    return result;
}

//{
//公钥：<public\u key>
//通道ID:256位通道ID
//删除：64位uint（字符串）
//签名：要验证的签名
//}
Json::Value doChannelVerify (RPC::Context& context)
{
    auto const& params (context.params);
    for (auto const& p :
         {jss::public_key, jss::channel_id, jss::amount, jss::signature})
        if (!params.isMember (p))
            return RPC::missing_field_error (p);

    boost::optional<PublicKey> pk;
    {
        std::string const strPk = params[jss::public_key].asString();
        pk = parseBase58<PublicKey>(TokenType::AccountPublic, strPk);

        if (!pk)
        {
            std::pair<Blob, bool> pkHex(strUnHex (strPk));
            if (!pkHex.second)
                return rpcError(rpcPUBLIC_MALFORMED);
            auto const pkType = publicKeyType(makeSlice(pkHex.first));
            if (!pkType)
                return rpcError(rpcPUBLIC_MALFORMED);
            pk.emplace(makeSlice(pkHex.first));
        }
    }

    uint256 channelId;
    if (!channelId.SetHexExact (params[jss::channel_id].asString ()))
        return rpcError (rpcCHANNEL_MALFORMED);

    boost::optional<std::uint64_t> const optDrops =
        params[jss::amount].isString()
        ? to_uint64(params[jss::amount].asString())
        : boost::none;

    if (!optDrops)
        return rpcError (rpcCHANNEL_AMT_MALFORMED);

    std::uint64_t const drops = *optDrops;

    std::pair<Blob, bool> sig(strUnHex (params[jss::signature].asString ()));
    if (!sig.second || !sig.first.size ())
        return rpcError (rpcINVALID_PARAMS);

    Serializer msg;
    serializePayChanAuthorization (msg, channelId, XRPAmount (drops));

    Json::Value result;
    result[jss::signature_verified] =
        /*ify（*pk，msg.slice（），makeslice（sig.first），/*规范*/true）；
    返回结果；
}

} /纹波
