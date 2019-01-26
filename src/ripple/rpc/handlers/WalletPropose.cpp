
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

#include <ripple/basics/strHex.h>
#include <ripple/crypto/KeyType.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/handlers/WalletPropose.h>
#include <ed25519-donna/ed25519.h>
#include <boost/optional.hpp>
#include <cmath>
#include <map>

namespace ripple {

double
estimate_entropy (std::string const& input)
{
//首先，我们计算香农熵。这给了
//每个符号的平均位数
//需要对输入进行编码。
    std::map<int, double> freq;

    for (auto const& c : input)
        freq[c]++;

    double se = 0.0;

    for (auto const& f : freq)
    {
        auto x = f.second / input.length();
        se += (x) * log2(x);
    }

//我们用它乘以长度，得到
//输入中的位数。我们落地是因为它
//最好保守一点。
    return std::floor (-se * input.length());
}

//{
//密码短语：<string>
//}
Json::Value doWalletPropose (RPC::Context& context)
{
    return walletPropose (context.params);
}

Json::Value walletPropose (Json::Value const& params)
{
    boost::optional<KeyType> keyType;
    boost::optional<Seed> seed;
    bool rippleLibSeed = false;

    if (params.isMember (jss::key_type))
    {
        if (! params[jss::key_type].isString())
        {
            return RPC::expected_field_error (
                jss::key_type, "string");
        }

        keyType = keyTypeFromString (
            params[jss::key_type].asString());

        if (!keyType)
            return rpcError(rpcINVALID_PARAMS);
    }

//Ripple lib编码用于生成ED25519钱包的种子
//非标准方式。虽然我们从不这样编码种子，但我们尝试
//检测这些密钥以避免用户混淆。
    {
        if (params.isMember(jss::passphrase))
            seed = RPC::parseRippleLibSeed(params[jss::passphrase]);
        else if (params.isMember(jss::seed))
            seed = RPC::parseRippleLibSeed(params[jss::seed]);

        if(seed)
        {
            rippleLibSeed = true;

//如果用户*显式*请求除
//ED25519返回错误。
            if (keyType.value_or(KeyType::ed25519) != KeyType::ed25519)
                return rpcError(rpcBAD_SEED);

            keyType = KeyType::ed25519;
        }
    }

    if (!seed)
    {
        if (params.isMember(jss::passphrase) ||
            params.isMember(jss::seed) ||
            params.isMember(jss::seed_hex))
        {
            Json::Value err;

            seed = RPC::getSeedFromRPC(params, err);

            if (!seed)
                return err;
        }
        else
        {
            seed = randomSeed();
        }
    }

    if (!keyType)
        keyType = KeyType::secp256k1;

    auto const publicKey = generateKeyPair (*keyType, *seed).first;

    Json::Value obj (Json::objectValue);

    auto const seed1751 = seedAs1751 (*seed);
    auto const seedHex = strHex (*seed);
    auto const seedBase58 = toBase58 (*seed);

    obj[jss::master_seed] = seedBase58;
    obj[jss::master_seed_hex] = seedHex;
    obj[jss::master_key] = seed1751;
    obj[jss::account_id] = toBase58(calcAccountID(publicKey));
    obj[jss::public_key] = toBase58(TokenType::AccountPublic, publicKey);
    obj[jss::key_type] = to_string (*keyType);
    obj[jss::public_key_hex] = strHex (publicKey);

//如果指定了密码短语，并将其散列并用作种子
//运行一个快速的熵检查并添加一个适当的警告，因为
//“大脑钱包”很容易被攻击。
    if (!rippleLibSeed && params.isMember (jss::passphrase))
    {
        auto const passphrase = params[jss::passphrase].asString();

        if (passphrase != seed1751 &&
            passphrase != seedBase58 &&
            passphrase != seedHex)
        {
//80比特的熵并不坏，但最好
//谨小慎微。
            if (estimate_entropy (passphrase) < 80.0)
                obj[jss::warning] =
                    "This wallet was generated using a user-supplied "
                    "passphrase that has low entropy and is vulnerable "
                    "to brute-force attacks.";
            else
                obj[jss::warning] =
                    "This wallet was generated using a user-supplied "
                    "passphrase. It may be vulnerable to brute-force "
                    "attacks.";
        }
    }

    return obj;
}

} //涟漪
