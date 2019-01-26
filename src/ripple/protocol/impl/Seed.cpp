
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

#include <ripple/protocol/Seed.h>
#include <ripple/basics/Buffer.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/basics/contract.h>
#include <ripple/crypto/RFC1751.h>
#include <ripple/crypto/csprng.h>
#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/beast/utility/rngfill.h>
#include <algorithm>
#include <cstring>
#include <iterator>

namespace ripple {

Seed::~Seed()
{
    beast::secure_erase(buf_.data(), buf_.size());
}

Seed::Seed (Slice const& slice)
{
    if (slice.size() != buf_.size())
        LogicError("Seed::Seed: invalid size");
    std::memcpy(buf_.data(),
        slice.data(), buf_.size());
}

Seed::Seed (uint128 const& seed)
{
    if (seed.size() != buf_.size())
        LogicError("Seed::Seed: invalid size");
    std::memcpy(buf_.data(),
        seed.data(), buf_.size());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Seed
randomSeed()
{
    std::array <std::uint8_t, 16> buffer;
    beast::rngfill (
        buffer.data(),
        buffer.size(),
        crypto_prng());
    Seed seed (makeSlice (buffer));
    beast::secure_erase(buffer.data(), buffer.size());
    return seed;
}

Seed
generateSeed (std::string const& passPhrase)
{
    sha512_half_hasher_s h;
    h(passPhrase.data(), passPhrase.size());
    auto const digest =
        sha512_half_hasher::result_type(h);
    return Seed({ digest.data(), 16 });
}

template <>
boost::optional<Seed>
parseBase58 (std::string const& s)
{
    auto const result = decodeBase58Token(s, TokenType::FamilySeed);
    if (result.empty())
        return boost::none;
    if (result.size() != 16)
        return boost::none;
    return Seed(makeSlice(result));
}

boost::optional<Seed>
parseGenericSeed (std::string const& str)
{
    if (str.empty ())
        return boost::none;

    if (parseBase58<AccountID>(str) ||
        parseBase58<PublicKey>(TokenType::NodePublic, str) ||
        parseBase58<PublicKey>(TokenType::AccountPublic, str) ||
        parseBase58<SecretKey>(TokenType::NodePrivate, str) ||
        parseBase58<SecretKey>(TokenType::AccountSecret, str))
    {
        return boost::none;
    }

    {
        uint128 seed;

        if (seed.SetHexExact (str))
            return Seed { Slice(seed.data(), seed.size()) };
    }

    if (auto seed = parseBase58<Seed> (str))
        return seed;

    {
        std::string key;
        if (RFC1751::getKeyFromEnglish (key, str) == 1)
        {
            Blob const blob (key.rbegin(), key.rend());
            return Seed{ uint128{blob} };
        }
    }

    return generateSeed (str);
}

std::string
seedAs1751 (Seed const& seed)
{
    std::string key;

    std::reverse_copy (
        seed.data(),
        seed.data() + 16,
        std::back_inserter(key));

    std::string encodedKey;
    RFC1751::getEnglishFromKey (encodedKey, key);
    return encodedKey;
}

}
