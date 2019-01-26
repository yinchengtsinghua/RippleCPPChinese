
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

#ifndef RIPPLE_PROTOCOL_SECRETKEY_H_INCLUDED
#define RIPPLE_PROTOCOL_SECRETKEY_H_INCLUDED

#include <ripple/basics/Buffer.h>
#include <ripple/basics/Slice.h>
#include <ripple/crypto/KeyType.h> //移动到协议/
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/protocol/tokens.h>
#include <array>
#include <string>

namespace ripple {

/*一把秘密钥匙*/
class SecretKey
{
private:
    std::uint8_t buf_[32];

public:
    using const_iterator = std::uint8_t const*;

    SecretKey() = default;
    SecretKey (SecretKey const&) = default;
    SecretKey& operator= (SecretKey const&) = default;

    ~SecretKey();

    SecretKey (std::array<std::uint8_t, 32> const& data);
    SecretKey (Slice const& slice);

    std::uint8_t const*
    data() const
    {
        return buf_;
    }

    std::size_t
    size() const
    {
        return sizeof(buf_);
    }

    /*将密钥转换为十六进制字符串。

        @注意，故意省略了运算符<<function
        避免秘密钥匙材料意外暴露。
    **/

    std::string
    to_string() const;

    const_iterator
    begin() const noexcept
    {
        return buf_;
    }

    const_iterator
    cbegin() const noexcept
    {
        return buf_;
    }

    const_iterator
    end() const noexcept
    {
        return buf_ + sizeof(buf_);
    }

    const_iterator
    cend() const noexcept
    {
        return buf_ + sizeof(buf_);
    }
};

inline
bool
operator== (SecretKey const& lhs,
    SecretKey const& rhs)
{
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.data(),
            rhs.data(), rhs.size()) == 0;
}

inline
bool
operator!= (SecretKey const& lhs,
    SecretKey const& rhs)
{
    return ! (lhs == rhs);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*解析密钥*/
template <>
boost::optional<SecretKey>
parseBase58 (TokenType type, std::string const& s);

inline
std::string
toBase58 (TokenType type, SecretKey const& sk)
{
    return base58EncodeToken(
        type, sk.data(), sk.size());
}

/*使用安全随机数创建密钥。*/
SecretKey
randomSecretKey();

/*确定地生成新的密钥。*/
SecretKey
generateSecretKey (KeyType type, Seed const& seed);

/*从密钥派生公钥。*/
PublicKey
derivePublicKey (KeyType type, SecretKey const& sk);

/*确定地生成密钥对。

    此算法特定于Ripple：

    对于secp256k1密钥对，种子将被转换
    到生成器，用于计算密钥对
    对应于生成器的序号0。
**/

std::pair<PublicKey, SecretKey>
generateKeyPair (KeyType type, Seed const& seed);

/*使用安全随机数创建密钥对。*/
std::pair<PublicKey, SecretKey>
randomKeyPair (KeyType type);

/*为消息摘要生成签名。
    自ED25519以来，此功能只能与secp256k1一起使用。
    安全属性部分来自消息
    是散列的。
**/

/*@ {*/
Buffer
signDigest (PublicKey const& pk, SecretKey const& sk,
    uint256 const& digest);

inline
Buffer
signDigest (KeyType type, SecretKey const& sk,
    uint256 const& digest)
{
    return signDigest (derivePublicKey(type, sk), sk, digest);
}
/*@ }*/

/*为消息生成签名。
    对于secp256k1签名，数据首先使用
    sha512一半，结果摘要已签名。
**/

/*@ {*/
Buffer
sign (PublicKey const& pk,
    SecretKey const& sk, Slice const& message);

inline
Buffer
sign (KeyType type, SecretKey const& sk,
    Slice const& message)
{
    return sign (derivePublicKey(type, sk), sk, message);
}
/*@ }*/

} //涟漪

#endif
