
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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/net/RPCErr.h>
#include <ripple/rpc/Context.h>
#include <ripple/beast/core/LexicalCast.h>

namespace ripple {


//{
//功能：<feature>
//否决：真/假
//}
Json::Value doFeature (RPC::Context& context)
{

//获取多数修正状态
    majorityAmendments_t majorities;

    if (auto const valLedger = context.ledgerMaster.getValidatedLedger())
        majorities = getMajorityAmendments (*valLedger);

    auto& table = context.app.getAmendmentTable ();

    if (!context.params.isMember (jss::feature))
    {
        auto features = table.getJson(0);

        for (auto const& m : majorities)
        {
            features[to_string(m.first)][jss::majority] =
                m.second.time_since_epoch().count();
        }

        Json::Value jvReply = Json::objectValue;
        jvReply[jss::features] = features;
        return jvReply;
    }

    auto feature = table.find (
        context.params[jss::feature].asString());

    if (!feature &&
        !feature.SetHexExact (context.params[jss::feature].asString ()))
        return rpcError (rpcBAD_FEATURE);

    if (context.params.isMember (jss::vetoed))
    {
        if (context.params[jss::vetoed].asBool ())
            context.app.getAmendmentTable().veto (feature);
        else
            context.app.getAmendmentTable().unVeto(feature);
    }

    Json::Value jvReply = table.getJson(feature);

    auto m = majorities.find (feature);
    if (m != majorities.end())
        jvReply [jss::majority] =
            m->second.time_since_epoch().count();

    return jvReply;
}


} //涟漪
