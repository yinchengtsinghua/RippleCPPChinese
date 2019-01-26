
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

#include <ripple/app/main/Application.h>
#include <ripple/net/RPCErr.h>
#include <ripple/nodestore/DatabaseShard.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>

namespace ripple {

/*报告按节点存储的碎片的rpc命令。
    {
        //确定结果是否包含节点公钥。
        //可选，默认为假
        PUBKEY：< BOOL >

        //要尝试的最大对等跃点数。
        //可选，默认为零，最大为3
        限制：<integer>
    }
**/

Json::Value
doCrawlShards(RPC::Context& context)
{
    if (context.role != Role::ADMIN)
        return rpcError(rpcNO_PERMISSION);

    std::uint32_t hops {0};
    if (auto const& jv = context.params[jss::limit])
    {
        if (!(jv.isUInt() || (jv.isInt() && jv.asInt() >= 0)))
        {
            return RPC::expected_field_error(
                jss::limit, "unsigned integer");
        }

        hops = std::min(jv.asUInt(), csHopLimit);
    }

    bool const pubKey {context.params.isMember(jss::public_key) &&
        context.params[jss::public_key].asBool()};

//从连接到此服务器的对等端收集碎片信息
    Json::Value jvResult {context.app.overlay().crawlShards(pubKey, hops)};

//从此服务器收集碎片信息
    if (auto shardStore = context.app.getShardStore())
    {
        if (pubKey)
            jvResult[jss::public_key] = toBase58(
                TokenType::NodePublic, context.app.nodeIdentity().first);
        jvResult[jss::complete_shards] = shardStore->getCompleteShards();
    }

    if (hops == 0)
        context.loadType = Resource::feeMediumBurdenRPC;
    else
        context.loadType = Resource::feeHighBurdenRPC;

    return jvResult;
}

} //涟漪
