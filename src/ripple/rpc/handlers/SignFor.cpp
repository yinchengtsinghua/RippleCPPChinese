
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Feature.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/TransactionSign.h>

namespace ripple {

//{
//tx_json:<object>，
//账户：<signing account>
//秘密：<secret of signing account>
//}
Json::Value doSignFor (RPC::Context& context)
{
    if (context.role != Role::ADMIN && !context.app.config().canSign())
    {
        return RPC::make_error (rpcNOT_SUPPORTED,
            "Signing is not supported by this server.");
    }

//如果未启用多重签名，则保释。
    if (! context.app.getLedgerMaster().getValidatedRules().
        enabled (featureMultiSign))
    {
        RPC::inject_error (rpcNOT_ENABLED, context.params);
        return context.params;
    }
    context.loadType = Resource::feeHighBurdenRPC;
    auto const failHard = context.params[jss::fail_hard].asBool();
    auto const failType = NetworkOPs::doFailHard (failHard);

    auto ret = RPC::transactionSignFor (
        context.params, failType, context.role,
        context.ledgerMaster.getValidatedLedgerAge(), context.app);

    ret[jss::deprecated] = "This command has been deprecated and will be "
                           "removed in a future version of the server. Please "
                           "migrate to a standalone signing tool.";
    return ret;
}

} //涟漪
