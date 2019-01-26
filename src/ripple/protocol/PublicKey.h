
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

#ifndef RIPPLE_PROTOCOL_PUBLICKEY_H_INCLUDED
#define RIPPLE_PROTOCOL_PUBLICKEY_H_INCLUDED

#include <ripple/basics/Slice.h>
#include <ripple/crypto/KeyType.h> //移动到协议/
#include <ripple/protocol/STExchange.h>
#include <ripple/protocol/tokens.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <utility>

namespace ripple {

/*公钥。

    公钥用于公钥加密
    用于验证附加到消息的签名的系统。

    公钥的格式是特定于Ripple的，
    确定密码系统所需的信息
    使用的参数存储在密钥中。

    在撰写本文时，支持两个系统：

        SECP256K1
        ED25519

    secp256k1公钥由33字节组成
    压缩公钥，前导字节等于
    到0x02或0x03。

    ED25519公钥由1个字节组成
    前缀常量0xed，后跟32字节
    公钥数据。
**/

class PublicKey
{
protected:
    std::size_t size_ = 0;
std::uint8_t buf_[33]; //应该足够大

public:
    using const_iterator = std::uint8_t const*;

    PublicKey() = default;
    PublicKey (PublicKey const& other);
    PublicKey& operator= (PublicKey const& other);

    /*创建公钥。

        先决条件：
            PublicKeyType（切片）！= Boo:：没有
    **/

    explicit
    PublicKey (Slice const& slice);

    std::uint8_t const*
    data() const noexcept
    {
        return buf_;
    }

    std::size_t
    size() const noexcept
    {
        return size_;
    }

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
        return buf_ + size_;
    }

    const_iterator
    cend() const noexcept
    {
        return buf_ + size_;
    }

    bool
    empty() const noexcept
    {
        return size_ == 0;
    }

    Slice
    slice() const noexcept
    {
        return { buf_, size_ };
    }

    operator Slice() const noexcept
    {
        return slice();
    }
};

/*将公钥打印到流。
**/

std::ostream&
operator<<(std::ostream& os, PublicKey const& pk);

inline
bool
operator== (PublicKey const& lhs,
    PublicKey const& rhs)
{
    return lhs.size() == rhs.size() &&
        std::memcmp(lhs.data(),
            rhs.data(), rhs.size()) == 0;
}

inline
bool
operator< (PublicKey const& lhs,
    PublicKey const& rhs)
{
    return std::lexicographical_compare(
        lhs.data(), lhs.data() + lhs.size(),
            rhs.data(), rhs.data() + rhs.size());
}

template <class Hasher>
void
hash_append (Hasher& h,
    PublicKey const& pk)
{
    h(pk.data(), pk.size());
}

template<>
struct STExchange<STBlob, PublicKey>
{
    explicit STExchange() = default;

    using value_type = PublicKey;

    static
    void
    get (boost::optional<value_type>& t,
        STBlob const& u)
    {
        t.emplace (Slice(u.data(), u.size()));
    }

    static
    std::unique_ptr<STBlob>
    set (SField const& f, PublicKey const& t)
    {
        return std::make_unique<STBlob>(
            f, t.data(), t.size());
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline
std::string
toBase58 (TokenType type, PublicKey const& pk)
{
    return base58EncodeToken(
        type, pk.data(), pk.size());
}

template<>
boost::optional<PublicKey>
parseBase58 (TokenType type, std::string const& s);

enum class ECDSACanonicality
{
    canonical,
    fullyCanonical
};

/*确定签名的规范性。

    规范签名的形式是最简化的。
    例如，R和S组件不包含
    其他前导零。然而，即使在
    规范形式（r，s）和（r，g-s）都是
    消息M的有效签名。

    因此，为了防止延展性攻击，我们
    将完全规范签名定义为一个签名，其中：

        R＜G—S

    其中，g是曲线顺序。

    如果格式为
    的签名无效（例如，
    点编码错误）。

    @return boost:：签名失败无
            有效性检查。

    @注意只检查签名的格式，
          未执行验证加密。
**/

boost::optional<ECDSACanonicality>
ecdsaCanonicality (Slice const& sig);

/*返回公钥的类型。

    @return boost：：如果公钥没有
            表示已知类型。
**/

/*@ {*/
boost::optional<KeyType>
publicKeyType (Slice const& slice);

inline
boost::optional<KeyType>
publicKeyType (PublicKey const& publicKey)
{
    return publicKeyType (publicKey.slice());
}
/*@ }*/

/*验证消息摘要上的secp256k1签名。*/
bool
verifyDigest (PublicKey const& publicKey,
    uint256 const& digest,
    Slice const& sig,
    bool mustBeFullyCanonical = true);

/*验证邮件上的签名。
    对于secp256k1签名，数据首先使用
    sha512一半，结果摘要已签名。
**/

bool
verify (PublicKey const& publicKey,
    Slice const& m,
    Slice const& sig,
    bool mustBeFullyCanonical = true);

/*从节点公钥计算160位节点ID。*/
NodeID
calcNodeID (PublicKey const&);

//vfalc这属于accountid.h，但是
//这里是因为标题问题
AccountID
calcAccountID (PublicKey const& pk);

} //涟漪

#endif
