
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

#include <ripple/basics/Log.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/Seed.h>
#include <ripple/rpc/Context.h>

namespace ripple {

static
boost::optional<Seed>
validationSeed (Json::Value const& params)
{
    if (!params.isMember (jss::secret))
        return randomSeed ();

    return parseGenericSeed (params[jss::secret].asString ());
}

//{
//机密：<string>>/可选
//}
//
//此命令需要角色：：admin访问权限，因为它使
//没有必要为此要求不受信任的服务器。
Json::Value doValidationCreate (RPC::Context& context)
{
    Json::Value     obj (Json::objectValue);

    auto seed = validationSeed(context.params);

    if (!seed)
        return rpcError (rpcBAD_SEED);

    auto const private_key = generateSecretKey (KeyType::secp256k1, *seed);

    obj[jss::validation_public_key] = toBase58 (
        TokenType::NodePublic,
        derivePublicKey (KeyType::secp256k1, private_key));

    obj[jss::validation_private_key] = toBase58 (
        TokenType::NodePrivate, private_key);

    obj[jss::validation_seed] = toBase58 (*seed);
    obj[jss::validation_key] = seedAs1751 (*seed);

    return obj;
}

} //涟漪
