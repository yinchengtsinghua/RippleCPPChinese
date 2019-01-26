
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

#include <ripple/json/json_writer.h>
#include <ripple/app/main/Application.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/impl/Tuning.h>

#include <string>
#include <sstream>

namespace ripple {

/*可以在帐户根目录中检索对象的常规rpc命令。
    {
      账户：<account><account_public_key>
      分类帐哈希：<string>>/可选
      分类帐索引：<string无符号整数>//可选
      类型：<string>>/可选，默认为所有帐户对象类型
      限制：<integer>>/可选
      marker:<opaque>>/可选，继续上一个查询
    }
**/


Json::Value doAccountObjects (RPC::Context& context)
{
    auto const& params = context.params;
    if (! params.isMember (jss::account))
        return RPC::missing_field_error (jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);
    if (ledger == nullptr)
        return result;

    AccountID accountID;
    {
        auto const strIdent = params[jss::account].asString ();
        if (auto jv = RPC::accountFromString (accountID, strIdent))
        {
            for (auto it = jv.begin (); it != jv.end (); ++it)
                result[it.memberName ()] = *it;

            return result;
        }
    }

    if (! ledger->exists(keylet::account (accountID)))
        return rpcError (rpcACT_NOT_FOUND);

    auto type = RPC::chooseLedgerEntryType(params);
    if (type.first)
    {
        result.clear();
        type.first.inject(result);
        return result;
    }

    unsigned int limit;
    if (auto err = readLimitField(limit, RPC::Tuning::accountObjects, context))
        return *err;

    uint256 dirIndex;
    uint256 entryIndex;
    if (params.isMember (jss::marker))
    {
        auto const& marker = params[jss::marker];
        if (! marker.isString ())
            return RPC::expected_field_error (jss::marker, "string");

        std::stringstream ss (marker.asString ());
        std::string s;
        if (!std::getline(ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! dirIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);

        if (! std::getline (ss, s, ','))
            return RPC::invalid_field_error (jss::marker);

        if (! entryIndex.SetHex (s))
            return RPC::invalid_field_error (jss::marker);
    }

    if (! RPC::getAccountObjects (*ledger, accountID, type.second,
        dirIndex, entryIndex, limit, result))
    {
        result[jss::account_objects] = Json::arrayValue;
    }

    result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
    context.loadType = Resource::feeMediumBurdenRPC;
    return result;
}

} //涟漪
