
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
    版权所有（c）2018 Ripple Labs Inc.

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


#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//{
//源帐户：<ident>
//目的地帐户：<ident>
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//}

Json::Value doDepositAuthorized (RPC::Context& context)
{
    Json::Value const& params = context.params;

//验证源帐户。
    if (! params.isMember (jss::source_account))
        return RPC::missing_field_error (jss::source_account);
    if (! params[jss::source_account].isString())
        return RPC::make_error (rpcINVALID_PARAMS,
            RPC::expected_field_message (jss::source_account,
                "a string"));

    AccountID srcAcct;
    {
        Json::Value const jvAccepted = RPC::accountFromString (
            srcAcct, params[jss::source_account].asString(), true);
        if (jvAccepted)
            return jvAccepted;
    }

//验证目标帐户。
    if (! params.isMember (jss::destination_account))
        return RPC::missing_field_error (jss::destination_account);
    if (! params[jss::destination_account].isString())
        return RPC::make_error (rpcINVALID_PARAMS,
            RPC::expected_field_message (jss::destination_account,
                "a string"));

    AccountID dstAcct;
    {
        Json::Value const jvAccepted = RPC::accountFromString (
            dstAcct, params[jss::destination_account].asString(), true);
        if (jvAccepted)
            return jvAccepted;
    }

//验证分类帐。
    std::shared_ptr<ReadView const> ledger;
    Json::Value result = RPC::lookupLedger (ledger, context);

    if (!ledger)
        return result;

//如果源帐户不在分类帐中，则无法进行授权。
    if (! ledger->exists (keylet::account(srcAcct)))
    {
        RPC::inject_error (rpcSRC_ACT_NOT_FOUND, result);
        return result;
    }

//如果目的地帐户不在分类帐中，您就不能存入它，嗯？
    auto const sleDest = ledger->read (keylet::account(dstAcct));
    if (! sleDest)
    {
        RPC::inject_error (rpcDST_ACT_NOT_FOUND, result);
        return result;
    }

//如果两个账户相同，那么押金应该是罚款的。
    bool depositAuthorized {true};
    if (srcAcct != dstAcct)
    {
//检查depositauth标志的目的地。如果这个标志是
//如果没有设定，那么押金就可以了。
        if (sleDest->getFlags() & lsfDepositAuth)
        {
//查看预授权条目是否在分类帐中。
            auto const sleDepositAuth =
                ledger->read(keylet::depositPreauth (dstAcct, srcAcct));
            depositAuthorized = static_cast<bool>(sleDepositAuth);
        }
    }
    result[jss::source_account] = params[jss::source_account].asString();
    result[jss::destination_account] =
        params[jss::destination_account].asString();

    result[jss::deposit_authorized] = depositAuthorized;
    return result;
}

} //涟漪
