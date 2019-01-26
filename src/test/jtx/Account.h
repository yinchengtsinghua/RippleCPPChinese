
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

#ifndef RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED
#define RIPPLE_TEST_JTX_ACCOUNT_H_INCLUDED

#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/crypto/KeyType.h>
#include <ripple/beast/hash/uhash.h>
#include <unordered_map>
#include <string>

namespace ripple {
namespace test {
namespace jtx {

class IOU;

/*不可变的加密帐户描述符。*/
class Account
{
private:
//进入私人控制的标签
    struct privateCtorTag{};
public:
    /*主账户。*/
    static Account const master;

    Account() = default;
    Account (Account&&) = default;
    Account (Account const&) = default;
    Account& operator= (Account const&) = default;
    Account& operator= (Account&&) = default;

    /*从简单的字符串名称创建帐户。*/
    /*@ {*/
    Account (std::string name,
        KeyType type = KeyType::secp256k1);

    Account (char const* name,
        KeyType type = KeyType::secp256k1)
        : Account(std::string(name), type)
    {
    }

//此构造函数必须是公共的，因此“std:：pair”在放置时可以使用它
//进入缓存。但是，它在逻辑上是“私有”的。这是强制执行的
//`privatetag`参数。
    Account (std::string name,
             std::pair<PublicKey, SecretKey> const& keys, Account::privateCtorTag);

    /*@ }*/

    /*返回名字*/
    std::string const&
    name() const
    {
        return name_;
    }

    /*返回公钥。*/
    PublicKey const&
    pk() const
    {
        return pk_;
    }

    /*返回密钥。*/
    SecretKey const&
    sk() const
    {
        return sk_;
    }

    /*返回帐户ID。

        帐户ID是公钥的uint160哈希。
    **/

    AccountID
    id() const
    {
        return id_;
    }

    /*返回可读的公钥。*/
    std::string const&
    human() const
    {
        return human_;
    }

    /*隐式转换为accountID。

        这允许通过一个帐户
        其中需要accountID。
    **/

    operator AccountID() const
    {
        return id_;
    }

    /*返回指定网关货币的IOU。*/
    IOU
    operator[](std::string const& s) const;

private:
    static std::unordered_map<
        std::pair<std::string, KeyType>,
            Account, beast::uhash<>> cache_;


//从缓存中返回帐户并在需要时将其添加到缓存中
    static Account fromCache(std::string name, KeyType type);

    std::string name_;
    PublicKey pk_;
    SecretKey sk_;
    AccountID id_;
std::string human_; //base58公钥字符串
};

inline
bool
operator== (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() == rhs.id();
}

template <class Hasher>
void
hash_append (Hasher& h,
    Account const& v) noexcept
{
    hash_append(h, v.id());
}

inline
bool
operator< (Account const& lhs,
    Account const& rhs) noexcept
{
    return lhs.id() < rhs.id();
}

} //JTX
} //测试
} //涟漪

#endif
