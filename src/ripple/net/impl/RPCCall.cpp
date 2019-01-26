
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

#include <ripple/app/main/Application.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/net/RPCCall.h>
#include <ripple/net/RPCErr.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/json/Object.h>
#include <ripple/net/HTTPClient.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/rpc/ServerHandler.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/beast/core/string.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/optional.hpp>
#include <boost/regex.hpp>
#include <array>
#include <iostream>
#include <type_traits>

namespace ripple {

class RPCParser;

//
//HTTP协议
//
//这不是阿帕奇。我们只是在长度字段中使用HTTP头
//与其他JSON-RPC实现兼容。
//

std::string createHTTPPost (
    std::string const& strHost,
    std::string const& strPath,
    std::string const& strMsg,
    std::map<std::string, std::string> const& mapRequestHeaders)
{
    std::ostringstream s;

//请勾选此选项，它使用的版本与下面的答复不同。是
//这是由于设计或意外，或者应该使用
//buildinfo：：getfullversionString（）也是吗？

    s << "POST "
      << (strPath.empty () ? "/" : strPath)
      << " HTTP/1.0\r\n"
      << "User-Agent: " << systemName () << "-json-rpc/v1\r\n"
      << "Host: " << strHost << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << strMsg.size () << "\r\n"
      << "Accept: application/json\r\n";

    for (auto const& item : mapRequestHeaders)
        s << item.first << ": " << item.second << "\r\n";

    s << "\r\n" << strMsg;

    return s.str ();
}

class RPCParser
{
private:
    beast::Journal j_;

//为了解析分类帐参数，需要使用新的例程，其他例程应该对此进行标准化。
    static bool jvParseLedger (Json::Value& jvRequest, std::string const& strLedger)
    {
        if (strLedger == "current" || strLedger == "closed" || strLedger == "validated")
        {
            jvRequest[jss::ledger_index]   = strLedger;
        }
        else if (strLedger.length () == 64)
        {
//YYY可以确认这是一个uint256。
            jvRequest[jss::ledger_hash]    = strLedger;
        }
        else
        {
            jvRequest[jss::ledger_index]   = beast::lexicalCast <std::uint32_t> (strLedger);
        }

        return true;
    }

//构建一个对象“currency”：“xyz”，“issuer”：“rxyx”
    static Json::Value jvParseCurrencyIssuer (std::string const& strCurrencyIssuer)
    {
        static boost::regex reCurIss ("\\`([[:alpha:]]{3})(?:/(.+))?\\'");

        boost::smatch   smMatch;

        if (boost::regex_match (strCurrencyIssuer, smMatch, reCurIss))
        {
            Json::Value jvResult (Json::objectValue);
            std::string strCurrency = smMatch[1];
            std::string strIssuer   = smMatch[2];

            jvResult[jss::currency]    = strCurrency;

            if (strIssuer.length ())
            {
//可以确认颁发者是有效的Ripple地址。
                jvResult[jss::issuer]      = strIssuer;
            }

            return jvResult;
        }
        else
        {
            return RPC::make_param_error (std::string ("Invalid currency/issuer '") +
                    strCurrencyIssuer + "'");
        }
    }

private:
    using parseFuncPtr = Json::Value (RPCParser::*) (Json::Value const& jvParams);

    Json::Value parseAsIs (Json::Value const& jvParams)
    {
        Json::Value v (Json::objectValue);

        if (jvParams.isArray() && (jvParams.size () > 0))
            v[jss::params] = jvParams;

        return v;
    }

    Json::Value parseDownloadShard(Json::Value const& jvParams)
    {
        Json::Value jvResult(Json::objectValue);
        unsigned int sz {jvParams.size()};
        unsigned int i {0};

//如果参数的奇数，则可能指定了“novalidate”
        if (sz & 1)
        {
            using namespace boost::beast::detail;
            if (iequals(jvParams[0u].asString(), "novalidate"))
                ++i;
            else if (!iequals(jvParams[--sz].asString(), "novalidate"))
                return rpcError(rpcINVALID_PARAMS);
            jvResult[jss::validate] = false;
        }

//创建“碎片”数组
        Json::Value shards(Json::arrayValue);
        for (; i < sz; i += 2)
        {
            Json::Value shard(Json::objectValue);
            shard[jss::index] = jvParams[i].asUInt();
            shard[jss::url] = jvParams[i + 1].asString();
            shards.append(std::move(shard));
        }
        jvResult[jss::shards] = std::move(shards);

        return jvResult;
    }

    Json::Value parseInternal (Json::Value const& jvParams)
    {
        Json::Value v (Json::objectValue);
        v[jss::internal_command] = jvParams[0u];

        Json::Value params (Json::arrayValue);

        for (unsigned i = 1; i < jvParams.size (); ++i)
            params.append (jvParams[i]);

        v[jss::params] = params;

        return v;
    }

//获取信息[清除]
    Json::Value parseFetchInfo (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        if (iParams != 0)
            jvRequest[jvParams[0u].asString()] = true;

        return jvRequest;
    }

//account_tx accountid[ledger_min[ledger_max[limit[offset]][二进制][计数][降序]
    Json::Value
    parseAccountTransactions (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        auto const account =
            parseBase58<AccountID>(jvParams[0u].asString());
        if (! account)
            return rpcError (rpcACT_MALFORMED);

        jvRequest[jss::account]= toBase58(*account);

        bool            bDone   = false;

        while (!bDone && iParams >= 2)
        {
//vfalc为什么json:：staticString出现在右侧？
            if (jvParams[iParams - 1].asString () == jss::binary)
            {
                jvRequest[jss::binary]     = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::count)
            {
                jvRequest[jss::count]      = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::descending)
            {
                jvRequest[jss::descending] = true;
                --iParams;
            }
            else
            {
                bDone   = true;
            }
        }

        if (1 == iParams)
        {
        }
        else if (2 == iParams)
        {
            if (!jvParseLedger (jvRequest, jvParams[1u].asString ()))
                return jvRequest;
        }
        else
        {
            std::int64_t   uLedgerMin  = jvParams[1u].asInt ();
            std::int64_t   uLedgerMax  = jvParams[2u].asInt ();

            if (uLedgerMax != -1 && uLedgerMax < uLedgerMin)
            {
                return rpcError (rpcLGR_IDXS_INVALID);
            }

            jvRequest[jss::ledger_index_min]   = jvParams[1u].asInt ();
            jvRequest[jss::ledger_index_max]   = jvParams[2u].asInt ();

            if (iParams >= 4)
                jvRequest[jss::limit]  = jvParams[3u].asInt ();

            if (iParams >= 5)
                jvRequest[jss::offset] = jvParams[4u].asInt ();
        }

        return jvRequest;
    }

//tx_accountID[分类帐最小值[分类帐最大值[限制]][二进制][计数][转发]
    Json::Value parseTxAccount (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        unsigned int    iParams = jvParams.size ();

        auto const account =
            parseBase58<AccountID>(jvParams[0u].asString());
        if (! account)
            return rpcError (rpcACT_MALFORMED);

        jvRequest[jss::account]    = toBase58(*account);

        bool            bDone   = false;

        while (!bDone && iParams >= 2)
        {
            if (jvParams[iParams - 1].asString () == jss::binary)
            {
                jvRequest[jss::binary]     = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::count)
            {
                jvRequest[jss::count]      = true;
                --iParams;
            }
            else if (jvParams[iParams - 1].asString () == jss::forward)
            {
                jvRequest[jss::forward] = true;
                --iParams;
            }
            else
            {
                bDone   = true;
            }
        }

        if (1 == iParams)
        {
        }
        else if (2 == iParams)
        {
            if (!jvParseLedger (jvRequest, jvParams[1u].asString ()))
                return jvRequest;
        }
        else
        {
            std::int64_t   uLedgerMin  = jvParams[1u].asInt ();
            std::int64_t   uLedgerMax  = jvParams[2u].asInt ();

            if (uLedgerMax != -1 && uLedgerMax < uLedgerMin)
            {
                return rpcError (rpcLGR_IDXS_INVALID);
            }

            jvRequest[jss::ledger_index_min]   = jvParams[1u].asInt ();
            jvRequest[jss::ledger_index_max]   = jvParams[2u].asInt ();

            if (iParams >= 4)
                jvRequest[jss::limit]  = jvParams[3u].asInt ();
        }

        return jvRequest;
    }

//Book_Offers<Taker_Pays>
//极限：0=无极限
//证明：0或1
//
//助记法：接受者支付——>出价——>接受者获得
    Json::Value parseBookOffers (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        Json::Value     jvTakerPays = jvParseCurrencyIssuer (jvParams[0u].asString ());
        Json::Value     jvTakerGets = jvParseCurrencyIssuer (jvParams[1u].asString ());

        if (isRpcError (jvTakerPays))
        {
            return jvTakerPays;
        }
        else
        {
            jvRequest[jss::taker_pays] = jvTakerPays;
        }

        if (isRpcError (jvTakerGets))
        {
            return jvTakerGets;
        }
        else
        {
            jvRequest[jss::taker_gets] = jvTakerGets;
        }

        if (jvParams.size () >= 3)
        {
            jvRequest[jss::issuer] = jvParams[2u].asString ();
        }

        if (jvParams.size () >= 4 && !jvParseLedger (jvRequest, jvParams[3u].asString ()))
            return jvRequest;

        if (jvParams.size () >= 5)
        {
            int     iLimit  = jvParams[5u].asInt ();

            if (iLimit > 0)
                jvRequest[jss::limit]  = iLimit;
        }

        if (jvParams.size () >= 6 && jvParams[5u].asInt ())
        {
            jvRequest[jss::proof]  = true;
        }

        if (jvParams.size () == 7)
            jvRequest[jss::marker] = jvParams[6u];

        return jvRequest;
    }

//可以删除[<ledgerid><ledgerhash>now always never]
    Json::Value parseCanDelete (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (!jvParams.size ())
            return jvRequest;

        std::string input = jvParams[0u].asString();
        if (input.find_first_not_of("0123456789") ==
                std::string::npos)
            jvRequest["can_delete"] = jvParams[0u].asUInt();
        else
            jvRequest["can_delete"] = input;

        return jvRequest;
    }

//连接<ip>[端口]
    Json::Value parseConnect (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        jvRequest[jss::ip] = jvParams[0u].asString ();

        if (jvParams.size () == 2)
            jvRequest[jss::port]   = jvParams[1u].asUInt ();

        return jvRequest;
    }

//存款授权账户
    Json::Value parseDepositAuthorized (Json::Value const& jvParams)
    {
        Json::Value jvRequest (Json::objectValue);
        jvRequest[jss::source_account] = jvParams[0u].asString ();
        jvRequest[jss::destination_account] = jvParams[1u].asString ();

        if (jvParams.size () == 3)
            jvParseLedger (jvRequest, jvParams[2u].asString ());

        return jvRequest;
    }

//返回尝试通过RPC订阅/取消订阅的错误。
    Json::Value parseEvented (Json::Value const& jvParams)
    {
        return rpcError (rpcNO_EVENTS);
    }

//功能[功能][接受拒绝]
    Json::Value parseFeature (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size () > 0)
            jvRequest[jss::feature] = jvParams[0u].asString ();

        if (jvParams.size () > 1)
        {
            auto const action = jvParams[1u].asString ();

//这看起来可能颠倒了，但它是故意的：JSS：：被否决
//确定修订是否被否决-因此“拒绝”是指
//被否决的JSS是真的。
            if (boost::beast::detail::iequals(action, "reject"))
                jvRequest[jss::vetoed] = Json::Value (true);
            else if (boost::beast::detail::iequals(action, "accept"))
                jvRequest[jss::vetoed] = Json::Value (false);
            else
                return rpcError (rpcINVALID_PARAMS);
        }

        return jvRequest;
    }

//获取计数
    Json::Value parseGetCounts (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size ())
            jvRequest[jss::min_count]  = jvParams[0u].asUInt ();

        return jvRequest;
    }

//为离线登录
//为<account><secret><json>签名
    Json::Value parseSignFor (Json::Value const& jvParams)
    {
        bool const bOffline = 4 == jvParams.size () && jvParams[3u].asString () == "offline";

        if (3 == jvParams.size () || bOffline)
        {
            Json::Value txJSON;
            Json::Reader reader;
            if (reader.parse (jvParams[2u].asString (), txJSON))
            {
//为txjson签名。
                Json::Value jvRequest{Json::objectValue};

                jvRequest[jss::account] = jvParams[0u].asString ();
                jvRequest[jss::secret]  = jvParams[1u].asString ();
                jvRequest[jss::tx_json] = txJSON;

                if (bOffline)
                    jvRequest[jss::offline] = true;

                return jvRequest;
            }
        }
        return rpcError (rpcINVALID_PARAMS);
    }

//json<command><json>
    Json::Value parseJson (Json::Value const& jvParams)
    {
        Json::Reader    reader;
        Json::Value     jvRequest;

        JLOG (j_.trace()) << "RPC method: " << jvParams[0u];
        JLOG (j_.trace()) << "RPC json: " << jvParams[1u];

        if (reader.parse (jvParams[1u].asString (), jvRequest))
        {
            if (!jvRequest.isObjectOrNull ())
                return rpcError (rpcINVALID_PARAMS);

            jvRequest[jss::method] = jvParams[0u];

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

    bool isValidJson2(Json::Value const& jv)
    {
        if (jv.isArray())
        {
            if (jv.size() == 0)
                return false;
            for (auto const& j : jv)
            {
                if (!isValidJson2(j))
                    return false;
            }
            return true;
        }
        if (jv.isObject())
        {
            if (jv.isMember(jss::jsonrpc) && jv[jss::jsonrpc] == "2.0" &&
                jv.isMember(jss::ripplerpc) && jv[jss::ripplerpc] == "2.0" &&
                jv.isMember(jss::id) && jv.isMember(jss::method))
            {
                if (jv.isMember(jss::params) &&
                      !(jv[jss::params].isNull() || jv[jss::params].isArray() ||
                                                    jv[jss::params].isObject()))
                    return false;
                return true;
            }
        }
        return false;
    }

    Json::Value parseJson2(Json::Value const& jvParams)
    {
        Json::Reader reader;
        Json::Value jv;
        bool valid_parse = reader.parse(jvParams[0u].asString(), jv);
        if (valid_parse && isValidJson2(jv))
        {
            if (jv.isObject())
            {
                Json::Value jv1{Json::objectValue};
                if (jv.isMember(jss::params))
                {
                    auto const& params = jv[jss::params];
                    for (auto i = params.begin(); i != params.end(); ++i)
                        jv1[i.key().asString()] = *i;
                }
                jv1[jss::jsonrpc] = jv[jss::jsonrpc];
                jv1[jss::ripplerpc] = jv[jss::ripplerpc];
                jv1[jss::id] = jv[jss::id];
                jv1[jss::method] = jv[jss::method];
                return jv1;
            }
//其他合资企业.isarray（）
            Json::Value jv1{Json::arrayValue};
            for (Json::UInt j = 0; j < jv.size(); ++j)
            {
                if (jv[j].isMember(jss::params))
                {
                    auto const& params = jv[j][jss::params];
                    for (auto i = params.begin(); i != params.end(); ++i)
                        jv1[j][i.key().asString()] = *i;
                }
                jv1[j][jss::jsonrpc] = jv[j][jss::jsonrpc];
                jv1[j][jss::ripplerpc] = jv[j][jss::ripplerpc];
                jv1[j][jss::id] = jv[j][jss::id];
                jv1[j][jss::method] = jv[j][jss::method];
            }
            return jv1;
        }
        auto jv_error = rpcError(rpcINVALID_PARAMS);
        if (jv.isMember(jss::jsonrpc))
            jv_error[jss::jsonrpc] = jv[jss::jsonrpc];
        if (jv.isMember(jss::ripplerpc))
            jv_error[jss::ripplerpc] = jv[jss::ripplerpc];
        if (jv.isMember(jss::id))
            jv_error[jss::id] = jv[jss::id];
        return jv_error;
    }

//分类帐[ID索引当前已关闭已验证][完整Tx]
    Json::Value parseLedger (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (!jvParams.size ())
        {
            return jvRequest;
        }

        jvParseLedger (jvRequest, jvParams[0u].asString ());

        if (2 == jvParams.size ())
        {
            if (jvParams[1u].asString () == "full")
            {
                jvRequest[jss::full]   = true;
            }
            else if (jvParams[1u].asString () == "tx")
            {
                jvRequest[jss::transactions] = true;
                jvRequest[jss::expand] = true;
            }
        }

        return jvRequest;
    }

//分类帐标题
    Json::Value parseLedgerId (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        std::string     strLedger   = jvParams[0u].asString ();

        if (strLedger.length () == 64)
        {
            jvRequest[jss::ledger_hash]    = strLedger;
        }
        else
        {
            jvRequest[jss::ledger_index]   = beast::lexicalCast <std::uint32_t> (strLedger);
        }

        return jvRequest;
    }

//日志级别：获取日志级别
//日志级别<严重性>：将主日志级别设置为指定的严重性
//日志级别：将指定的分区设置为指定的严重性
    Json::Value parseLogLevel (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);

        if (jvParams.size () == 1)
        {
            jvRequest[jss::severity] = jvParams[0u].asString ();
        }
        else if (jvParams.size () == 2)
        {
            jvRequest[jss::partition] = jvParams[0u].asString ();
            jvRequest[jss::severity] = jvParams[1u].asString ();
        }

        return jvRequest;
    }

//所有者信息
//所有者信息
//账户信息
//账户信息
//账户提供
    Json::Value parseAccountItems (Json::Value const& jvParams)
    {
        return parseAccountRaw1 (jvParams);
    }

    Json::Value parseAccountCurrencies (Json::Value const& jvParams)
    {
        return parseAccountRaw1 (jvParams);
    }

//账户行
    Json::Value parseAccountLines (Json::Value const& jvParams)
    {
        return parseAccountRaw2 (jvParams, jss::peer);
    }

//账户渠道
    Json::Value parseAccountChannels (Json::Value const& jvParams)
    {
        return parseAccountRaw2 (jvParams, jss::destination_account);
    }

//channel_authorize<private_key><channel_id><drops>
    Json::Value parseChannelAuthorize (Json::Value const& jvParams)
    {
        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::secret] = jvParams[0u];
        {
//验证通道ID是否为有效的256位数字
            uint256 channelId;
            if (!channelId.SetHexExact (jvParams[1u].asString ()))
                return rpcError (rpcCHANNEL_MALFORMED);
        }
        jvRequest[jss::channel_id] = jvParams[1u].asString ();

        if (!jvParams[2u].isString() || !to_uint64(jvParams[2u].asString()))
            return rpcError(rpcCHANNEL_AMT_MALFORMED);
        jvRequest[jss::amount] = jvParams[2u];

        return jvRequest;
    }

//频道验证
    Json::Value parseChannelVerify (Json::Value const& jvParams)
    {
        std::string const strPk = jvParams[0u].asString ();

        bool const validPublicKey = [&strPk]{
            if (parseBase58<PublicKey> (TokenType::AccountPublic, strPk))
                return true;

            std::pair<Blob, bool> pkHex(strUnHex (strPk));
            if (!pkHex.second)
                return false;

            if (!publicKeyType(makeSlice(pkHex.first)))
                return false;

            return true;
        }();

        if (!validPublicKey)
            return rpcError (rpcPUBLIC_MALFORMED);

        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::public_key] = strPk;
        {
//验证通道ID是否为有效的256位数字
            uint256 channelId;
            if (!channelId.SetHexExact (jvParams[1u].asString ()))
                return rpcError (rpcCHANNEL_MALFORMED);
        }
        jvRequest[jss::channel_id] = jvParams[1u].asString ();

        if (!jvParams[2u].isString() || !to_uint64(jvParams[2u].asString()))
            return rpcError(rpcCHANNEL_AMT_MALFORMED);
        jvRequest[jss::amount] = jvParams[2u];

        jvRequest[jss::signature] = jvParams[3u].asString ();

        return jvRequest;
    }

    Json::Value parseAccountRaw2 (Json::Value const& jvParams,
                                  char const * const acc2Field)
    {
        std::array<char const* const, 2> accFields{{jss::account, acc2Field}};
        auto const nParams = jvParams.size ();
        Json::Value jvRequest (Json::objectValue);
        for (auto i = 0; i < nParams; ++i)
        {
            std::string strParam = jvParams[i].asString ();

            if (i==1 && strParam.empty())
                continue;

//参数0和1是帐户
            if (i < 2)
            {
                if (parseBase58<PublicKey> (
                        TokenType::AccountPublic, strParam) ||
                    parseBase58<AccountID> (strParam) ||
                    parseGenericSeed (strParam))
                {
                    jvRequest[accFields[i]] = std::move (strParam);
                }
                else
                {
                    return rpcError (rpcACT_MALFORMED);
                }
            }
            else
            {
                if (jvParseLedger (jvRequest, strParam))
                    return jvRequest;
                return rpcError (rpcLGR_IDX_MALFORMED);
            }
        }

        return jvRequest;
    }

//TODO:从备用语法获取索引：rxyz:<index>
    Json::Value parseAccountRaw1 (Json::Value const& jvParams)
    {
        std::string     strIdent    = jvParams[0u].asString ();
        unsigned int    iCursor     = jvParams.size ();
        bool            bStrict     = false;

        if (iCursor >= 2 && jvParams[iCursor - 1] == jss::strict)
        {
            bStrict = true;
            --iCursor;
        }

        if (! parseBase58<PublicKey>(TokenType::AccountPublic, strIdent) &&
            ! parseBase58<AccountID>(strIdent) &&
            ! parseGenericSeed(strIdent))
            return rpcError (rpcACT_MALFORMED);

//获取有关帐户的信息。
        Json::Value jvRequest (Json::objectValue);

        jvRequest[jss::account]    = strIdent;

        if (bStrict)
            jvRequest[jss::strict]     = 1;

        if (iCursor == 2 && !jvParseLedger (jvRequest, jvParams[1u].asString ()))
            return rpcError (rpcLGR_IDX_MALFORMED);

        return jvRequest;
    }

//Ripple路径查找
    Json::Value parseRipplePathFind (Json::Value const& jvParams)
    {
        Json::Reader    reader;
        Json::Value     jvRequest{Json::objectValue};
        bool            bLedger     = 2 == jvParams.size ();

        JLOG (j_.trace()) << "RPC json: " << jvParams[0u];

        if (reader.parse (jvParams[0u].asString (), jvRequest))
        {
            if (bLedger)
            {
                jvParseLedger (jvRequest, jvParams[1u].asString ());
            }

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

//向网络签署/提交任何交易
//
//离线签名
//提交
//提交<tx_blob>
    Json::Value parseSignSubmit (Json::Value const& jvParams)
    {
        Json::Value     txJSON;
        Json::Reader    reader;
        bool const      bOffline    = 3 == jvParams.size () && jvParams[2u].asString () == "offline";

        if (1 == jvParams.size ())
        {
//提交tx_blob

            Json::Value jvRequest{Json::objectValue};

            jvRequest[jss::tx_blob]    = jvParams[0u].asString ();

            return jvRequest;
        }
        else if ((2 == jvParams.size () || bOffline)
                 && reader.parse (jvParams[1u].asString (), txJSON))
        {
//签署或提交Tx_json。
            Json::Value jvRequest{Json::objectValue};

            jvRequest[jss::secret]     = jvParams[0u].asString ();
            jvRequest[jss::tx_json]    = txJSON;

            if (bOffline)
                jvRequest[jss::offline]    = true;

            return jvRequest;
        }

        return rpcError (rpcINVALID_PARAMS);
    }

//向网络提交任何多签名交易
//
//提交“多重签名”<json>
    Json::Value parseSubmitMultiSigned (Json::Value const& jvParams)
    {
        if (1 == jvParams.size ())
        {
            Json::Value     txJSON;
            Json::Reader    reader;
            if (reader.parse (jvParams[0u].asString (), txJSON))
            {
                Json::Value jvRequest{Json::objectValue};
                jvRequest[jss::tx_json] = txJSON;
                return jvRequest;
            }
        }

        return rpcError (rpcINVALID_PARAMS);
    }

//交易分录<tx_hash><ledger_hash/ledger_index>
    Json::Value parseTransactionEntry (Json::Value const& jvParams)
    {
//参数计数应该已经过验证。
        assert (jvParams.size() == 2);

        std::string const txHash = jvParams[0u].asString();
        if (txHash.length() != 64)
            return rpcError (rpcINVALID_PARAMS);

        Json::Value jvRequest{Json::objectValue};
        jvRequest[jss::tx_hash] = txHash;

        jvParseLedger (jvRequest, jvParams[1u].asString());

//如果jParseLedger没有插入0的“Ledger_索引”
//找一根火柴。
        if (jvRequest.isMember(jss::ledger_index) &&
            jvRequest[jss::ledger_index] == 0)
                return rpcError (rpcINVALID_PARAMS);

        return jvRequest;
    }


//tx<transaction_id>
    Json::Value parseTx (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size () > 1)
        {
            if (jvParams[1u].asString () == jss::binary)
                jvRequest[jss::binary] = true;
        }

        jvRequest["transaction"]    = jvParams[0u].asString ();
        return jvRequest;
    }

//tx_history<index>
    Json::Value parseTxHistory (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        jvRequest[jss::start]  = jvParams[0u].asUInt ();

        return jvRequest;
    }

//验证创建
//
//注意：在命令行上指定机密信息是不安全的。此信息可能保存在命令中
//shell历史文件（例如bash_history），它可能通过进程状态命令（即ps）泄漏。
    Json::Value parseValidationCreate (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size ())
            jvRequest[jss::secret]     = jvParams[0u].asString ();

        return jvRequest;
    }

//钱包提议
//<passphrase>仅用于测试。主种子只能随机生成。
    Json::Value parseWalletPropose (Json::Value const& jvParams)
    {
        Json::Value jvRequest{Json::objectValue};

        if (jvParams.size ())
            jvRequest[jss::passphrase]     = jvParams[0u].asString ();

        return jvRequest;
    }

//分析网关平衡
//网关余额

    Json::Value parseGatewayBalances (Json::Value const& jvParams)
    {
        unsigned int index = 0;
        const unsigned int size = jvParams.size ();

        Json::Value jvRequest{Json::objectValue};

        std::string param = jvParams[index++].asString ();
        if (param.empty ())
            return RPC::make_param_error ("Invalid first parameter");

        if (param[0] != 'r')
        {
            if (param.size() == 64)
                jvRequest[jss::ledger_hash] = param;
            else
                jvRequest[jss::ledger_index] = param;

            if (size <= index)
                return RPC::make_param_error ("Invalid hotwallet");

            param = jvParams[index++].asString ();
        }

        jvRequest[jss::account] = param;

        if (index < size)
        {
            Json::Value& hotWallets =
                (jvRequest["hotwallet"] = Json::arrayValue);
            while (index < size)
                hotWallets.append (jvParams[index++].asString ());
        }

        return jvRequest;
    }

//服务器信息[计数器]
    Json::Value parseServerInfo (Json::Value const& jvParams)
    {
        Json::Value     jvRequest (Json::objectValue);
        if (jvParams.size() == 1 && jvParams[0u].asString() == "counters")
            jvRequest[jss::counters] = true;
        return jvRequest;
    }

public:
//——————————————————————————————————————————————————————————————

    explicit
    RPCParser (beast::Journal j)
            :j_ (j){}

//——————————————————————————————————————————————————————————————

//将rpc方法和参数转换为请求。
//<--方法：xyz，参数：…]或错误：…}
    Json::Value parseCommand (std::string strMethod, Json::Value jvParams, bool allowAnyCommand)
    {
        if (auto stream = j_.trace())
        {
            stream << "Method: '" << strMethod << "'";
            stream << "Params: " << jvParams;
        }

        struct Command
        {
            const char*     name;
            parseFuncPtr    parse;
            int             minParams;
            int             maxParams;
        };

//Fixme:用函数static std:：map和lookup替换它
//当magic statics的问题出现时，用std：：map：：find编码
//Visual Studio已修复。
        static
        Command const commands[] =
        {
//请求-响应方法
//-返回错误或请求。
//-要修改方法，请在请求中提供新方法。
            {   "account_currencies",   &RPCParser::parseAccountCurrencies,     1,  2   },
            {   "account_info",         &RPCParser::parseAccountItems,          1,  2   },
            {   "account_lines",        &RPCParser::parseAccountLines,          1,  5   },
            {   "account_channels",     &RPCParser::parseAccountChannels,       1,  3   },
            {   "account_objects",      &RPCParser::parseAccountItems,          1,  5   },
            {   "account_offers",       &RPCParser::parseAccountItems,          1,  4   },
            {   "account_tx",           &RPCParser::parseAccountTransactions,   1,  8   },
            {   "book_offers",          &RPCParser::parseBookOffers,            2,  7   },
            {   "can_delete",           &RPCParser::parseCanDelete,             0,  1   },
            {   "channel_authorize",    &RPCParser::parseChannelAuthorize,      3,  3   },
            {   "channel_verify",       &RPCParser::parseChannelVerify,         4,  4   },
            {   "connect",              &RPCParser::parseConnect,               1,  2   },
            {   "consensus_info",       &RPCParser::parseAsIs,                  0,  0   },
            {   "deposit_authorized",   &RPCParser::parseDepositAuthorized,     2,  3   },
            {   "download_shard",       &RPCParser::parseDownloadShard,         2,  -1  },
            {   "feature",              &RPCParser::parseFeature,               0,  2   },
            {   "fetch_info",           &RPCParser::parseFetchInfo,             0,  1   },
            {   "gateway_balances",     &RPCParser::parseGatewayBalances,       1, -1   },
            {   "get_counts",           &RPCParser::parseGetCounts,             0,  1   },
            {   "json",                 &RPCParser::parseJson,                  2,  2   },
            {   "json2",                &RPCParser::parseJson2,                 1,  1   },
            {   "ledger",               &RPCParser::parseLedger,                0,  2   },
            {   "ledger_accept",        &RPCParser::parseAsIs,                  0,  0   },
            {   "ledger_closed",        &RPCParser::parseAsIs,                  0,  0   },
            {   "ledger_current",       &RPCParser::parseAsIs,                  0,  0   },
//“分类帐条目”&rpcparser:：parseledgerEntry，-1，-1，
            {   "ledger_header",        &RPCParser::parseLedgerId,              1,  1   },
            {   "ledger_request",       &RPCParser::parseLedgerId,              1,  1   },
            {   "log_level",            &RPCParser::parseLogLevel,              0,  2   },
            {   "logrotate",            &RPCParser::parseAsIs,                  0,  0   },
            {   "owner_info",           &RPCParser::parseAccountItems,          1,  2   },
            {   "peers",                &RPCParser::parseAsIs,                  0,  0   },
            {   "ping",                 &RPCParser::parseAsIs,                  0,  0   },
            {   "print",                &RPCParser::parseAsIs,                  0,  1   },
//“配置文件”，&rpcparser:：parseprofile，1，9，
            {   "random",               &RPCParser::parseAsIs,                  0,  0   },
            {   "ripple_path_find",     &RPCParser::parseRipplePathFind,        1,  2   },
            {   "sign",                 &RPCParser::parseSignSubmit,            2,  3   },
            {   "sign_for",             &RPCParser::parseSignFor,               3,  4   },
            {   "submit",               &RPCParser::parseSignSubmit,            1,  3   },
            {   "submit_multisigned",   &RPCParser::parseSubmitMultiSigned,     1,  1   },
            {   "server_info",          &RPCParser::parseServerInfo,            0,  1   },
            {   "server_state",         &RPCParser::parseServerInfo,            0,  1   },
            {   "crawl_shards",         &RPCParser::parseAsIs,                  0,  2   },
            {   "stop",                 &RPCParser::parseAsIs,                  0,  0   },
            {   "transaction_entry",    &RPCParser::parseTransactionEntry,      2,  2   },
            {   "tx",                   &RPCParser::parseTx,                    1,  2   },
            {   "tx_account",           &RPCParser::parseTxAccount,             1,  7   },
            {   "tx_history",           &RPCParser::parseTxHistory,             1,  1   },
            {   "unl_list",             &RPCParser::parseAsIs,                  0,  0   },
            {   "validation_create",    &RPCParser::parseValidationCreate,      0,  1   },
            {   "version",              &RPCParser::parseAsIs,                  0,  0   },
            {   "wallet_propose",       &RPCParser::parseWalletPropose,         0,  1   },
            {   "internal",             &RPCParser::parseInternal,              1, -1   },

//规避方法
            {   "path_find",            &RPCParser::parseEvented,              -1, -1   },
            {   "subscribe",            &RPCParser::parseEvented,              -1, -1   },
            {   "unsubscribe",          &RPCParser::parseEvented,              -1, -1   },
        };

        auto const count = jvParams.size ();

        for (auto const& command : commands)
        {
            if (strMethod == command.name)
            {
                if ((command.minParams >= 0 && count < command.minParams) ||
                    (command.maxParams >= 0 && count > command.maxParams))
                {
                    JLOG (j_.debug()) <<
                        "Wrong number of parameters for " << command.name <<
                        " minimum=" << command.minParams <<
                        " maximum=" << command.maxParams <<
                        " actual=" << count;

                    return rpcError (rpcBAD_SYNTAX);
                }

                return (this->* (command.parse)) (jvParams);
            }
        }

//找不到命令
        if (!allowAnyCommand)
            return rpcError (rpcUNKNOWN_COMMAND);

        return parseAsIs (jvParams);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//
//JSON-RPC协议。比特币语音版本1.0可实现最大兼容性，
//但是，对于1.0标准的部分，使用json-rpc 1.1/2.0标准
//未指定（HTTP错误和“错误”的内容）。
//
//1.0规范：http://json-rpc.org/wiki/specification
//1.2规格：http://groups.google.com/group/json-rpc/web/json-rpc-over-http
//

std::string JSONRPCRequest (std::string const& strMethod, Json::Value const& params, Json::Value const& id)
{
    Json::Value request;
    request[jss::method] = strMethod;
    request[jss::params] = params;
    request[jss::id] = id;
    return to_string (request) + "\n";
}

namespace
{
//无法分析请求时引发特殊的本地异常类型。
    class RequestNotParseable : public std::runtime_error
    {
using std::runtime_error::runtime_error; //继承构造函数
    };
};

struct RPCCallImp
{
    explicit RPCCallImp() = default;

//vfalc备注：这是待办事项还是Doc备注？
//将异步结果放在有用的地方。
    static void callRPCHandler (Json::Value* jvOutput, Json::Value const& jvInput)
    {
        (*jvOutput) = jvInput;
    }

    static bool onResponse (
        std::function<void (Json::Value const& jvInput)> callbackFuncP,
            const boost::system::error_code& ecResult, int iStatus,
                std::string const& strData, beast::Journal j)
    {
        if (callbackFuncP)
        {
//只关心结果，如果我们想交付它callbackfuncp。

//收到回复
            if (iStatus == 401)
                Throw<std::runtime_error> (
                    "incorrect rpcuser or rpcpassword (authorization failed)");
else if ((iStatus >= 400) && (iStatus != 400) && (iStatus != 404) && (iStatus != 500)) //？
                Throw<std::runtime_error> (
                    std::string ("server returned HTTP error ") +
                        std::to_string (iStatus));
            else if (strData.empty ())
                Throw<std::runtime_error> ("no response from server");

//解析应答
            JLOG (j.debug()) << "RPC reply: " << strData << std::endl;
            if (strData.find("Unable to parse request") == 0)
                Throw<RequestNotParseable> (strData);
            Json::Reader    reader;
            Json::Value     jvReply;
            if (!reader.parse (strData, jvReply))
                Throw<std::runtime_error> ("couldn't parse reply from server");

            if (!jvReply)
                Throw<std::runtime_error> ("expected reply to have result, error and id properties");

            Json::Value     jvResult (Json::objectValue);

            jvResult["result"] = jvReply;

            (callbackFuncP) (jvResult);
        }

        return false;
    }

//生成请求。
    static void onRequest (std::string const& strMethod, Json::Value const& jvParams,
        const std::map<std::string, std::string>& mHeaders, std::string const& strPath,
            boost::asio::streambuf& sb, std::string const& strHost, beast::Journal j)
    {
        JLOG (j.debug()) << "requestRPC: strPath='" << strPath << "'";

        std::ostream    osRequest (&sb);
        osRequest <<
                  createHTTPPost (
                      strHost,
                      strPath,
                      JSONRPCRequest (strMethod, jvParams, Json::Value (1)),
                      mHeaders);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//由rpcclient内部使用。
static Json::Value
rpcCmdLineToJson (std::vector<std::string> const& args,
    Json::Value& retParams, beast::Journal j)
{
    Json::Value jvRequest (Json::objectValue);

    RPCParser   rpParser (j);
    Json::Value jvRpcParams (Json::arrayValue);

    for (int i = 1; i != args.size (); i++)
        jvRpcParams.append (args[i]);

    retParams = Json::Value (Json::objectValue);

    retParams[jss::method] = args[0];
    retParams[jss::params] = jvRpcParams;

    jvRequest   = rpParser.parseCommand (args[0], jvRpcParams, true);

    JLOG (j.trace()) << "RPC Request: " << jvRequest << std::endl;
    return jvRequest;
}

Json::Value
cmdLineToJSONRPC (std::vector<std::string> const& args, beast::Journal j)
{
    Json::Value jv = Json::Value (Json::objectValue);
    auto const paramsObj = rpcCmdLineToJson (args, jv, j);

//重新使用JV返回格式化结果。
    jv.clear();

//允许分析器重写方法。
    jv[jss::method] = paramsObj.isMember (jss::method) ?
        paramsObj[jss::method].asString() : args[0];

//如果paramsobj不是空的，请将其放入[params]数组中。
    if (paramsObj.begin() != paramsObj.end())
    {
        auto& paramsArray = Json::setArray (jv, jss::params);
        paramsArray.append (paramsObj);
    }
    if (paramsObj.isMember(jss::jsonrpc))
        jv[jss::jsonrpc] = paramsObj[jss::jsonrpc];
    if (paramsObj.isMember(jss::ripplerpc))
        jv[jss::ripplerpc] = paramsObj[jss::ripplerpc];
    if (paramsObj.isMember(jss::id))
        jv[jss::id] = paramsObj[jss::id];
    return jv;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::pair<int, Json::Value>
rpcClient(std::vector<std::string> const& args,
    Config const& config, Logs& logs)
{
    static_assert(rpcBAD_SYNTAX == 1 && rpcSUCCESS == 0,
        "Expect specific rpc enum values.");
    if (args.empty ())
return { rpcBAD_SYNTAX, {} }; //rpcbad_syntax=打印用法

    int         nRet = rpcSUCCESS;
    Json::Value jvOutput;
    Json::Value jvRequest (Json::objectValue);

    try
    {
        Json::Value jvRpc   = Json::Value (Json::objectValue);
        jvRequest = rpcCmdLineToJson (args, jvRpc, logs.journal ("RPCParser"));

        if (jvRequest.isMember (jss::error))
        {
            jvOutput            = jvRequest;
            jvOutput["rpc"]     = jvRpc;
        }
        else
        {
            ServerHandler::Setup setup;
            try
            {
                setup = setup_ServerHandler(
                    config,
                    beast::logstream { logs.journal ("HTTPClient").warn() });
            }
            catch (std::exception const&)
            {
//忽略任何异常，因此命令
//行客户机在没有配置文件的情况下工作
            }

            if (config.rpc_ip)
            {
                setup.client.ip = config.rpc_ip->address().to_string();
                setup.client.port = config.rpc_ip->port();
            }

            Json::Value jvParams (Json::arrayValue);

            if (!setup.client.admin_user.empty ())
                jvRequest["admin_user"] = setup.client.admin_user;

            if (!setup.client.admin_password.empty ())
                jvRequest["admin_password"] = setup.client.admin_password;

            if (jvRequest.isObject())
                jvParams.append (jvRequest);
            else if (jvRequest.isArray())
            {
                for (Json::UInt i = 0; i < jvRequest.size(); ++i)
                    jvParams.append(jvRequest[i]);
            }

            {
                boost::asio::io_service isService;
                RPCCall::fromNetwork (
                    isService,
                    setup.client.ip,
                    setup.client.port,
                    setup.client.user,
                    setup.client.password,
                    "",
jvRequest.isMember (jss::method)           //允许分析器重写方法。
                        ? jvRequest[jss::method].asString ()
                        : jvRequest.isArray()
                           ? "batch"
                           : args[0],
jvParams,                               //已分析，执行。
setup.client.secure != 0,                //使用SSL
                    config.quiet(),
                    logs,
                    std::bind (RPCCallImp::callRPCHandler, &jvOutput,
                               std::placeholders::_1));
isService.run(); //在没有其他未完成的异步调用之前，此操作将一直阻塞。
            }
            if (jvOutput.isMember ("result"))
            {
//已成功调用JSON-RPC 2.0。
                jvOutput    = jvOutput["result"];

//jvoutput可能报告服务器端错误。
//它应该报告“状态”。
            }
            else
            {
//传输错误。
                Json::Value jvRpcError  = jvOutput;

                jvOutput            = rpcError (rpcJSON_RPC);
                jvOutput["result"]  = jvRpcError;
            }

//如果有错误，请提供结果中的调用。
            if (jvOutput.isMember (jss::error))
            {
jvOutput["rpc"]             = jvRpc;            //如何将命令视为方法+参数。
jvOutput["request_sent"]    = jvRequest;        //命令的转换方式。
            }
        }

        if (jvOutput.isMember (jss::error))
        {
            jvOutput[jss::status]  = "error";
            if (jvOutput.isMember(jss::error_code))
                nRet = std::stoi(jvOutput[jss::error_code].asString());
            else if (jvOutput[jss::error].isMember(jss::error_code))
                nRet = std::stoi(jvOutput[jss::error][jss::error_code].asString());
            else
                nRet = rpcBAD_SYNTAX;
        }

//我们可以为脚本的单行输出设置命令行标志。
//我们在这里截取输出并简化它。
    }
    catch (RequestNotParseable& e)
    {
        jvOutput                = rpcError(rpcINVALID_PARAMS);
        jvOutput["error_what"]  = e.what();
        nRet                    = rpcINVALID_PARAMS;
    }
    catch (std::exception& e)
    {
        jvOutput                = rpcError (rpcINTERNAL);
        jvOutput["error_what"]  = e.what ();
        nRet                    = rpcINTERNAL;
    }

    return { nRet, std::move(jvOutput) };
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace RPCCall {

int fromCommandLine (
    Config const& config,
    const std::vector<std::string>& vCmd,
    Logs& logs)
{
    auto const result = rpcClient(vCmd, config, logs);

    if (result.first != rpcBAD_SYNTAX)
        std::cout << result.second.toStyledString ();

    return result.first;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void fromNetwork (
    boost::asio::io_service& io_service,
    std::string const& strIp, const std::uint16_t iPort,
    std::string const& strUsername, std::string const& strPassword,
    std::string const& strPath, std::string const& strMethod,
    Json::Value const& jvParams, const bool bSSL, const bool quiet,
    Logs& logs,
    std::function<void (Json::Value const& jvInput)> callbackFuncP)
{
    auto j = logs.journal ("HTTPClient");

//连接到本地主机
    if (!quiet)
    {
        JLOG(j.info()) << (bSSL ? "Securely connecting to " : "Connecting to ") <<
            strIp << ":" << iPort << std::endl;
    }

//HTTP基本身份验证
    auto const auth = base64_encode(strUsername + ":" + strPassword);

    std::map<std::string, std::string> mapRequestHeaders;

    mapRequestHeaders["Authorization"] = std::string ("Basic ") + auth;

//发送请求

//如果没有，则尝试接收的字节数
//接收的内容长度标题
    constexpr auto RPC_REPLY_MAX_BYTES = megabytes(256);

    using namespace std::chrono_literals;
    auto constexpr RPC_NOTIFY = 10min;

    HTTPClient::request (
        bSSL,
        io_service,
        strIp,
        iPort,
        std::bind (
            &RPCCallImp::onRequest,
            strMethod,
            jvParams,
            mapRequestHeaders,
            strPath, std::placeholders::_1, std::placeholders::_2, j),
        RPC_REPLY_MAX_BYTES,
        RPC_NOTIFY,
        std::bind (&RPCCallImp::onResponse, callbackFuncP,
                   std::placeholders::_1, std::placeholders::_2,
                   std::placeholders::_3, j),
        j);
}

} //RPCCALL

} //涟漪
