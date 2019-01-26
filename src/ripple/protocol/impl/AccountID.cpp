
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

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/tokens.h>
#include <cstring>

namespace ripple {

std::string
toBase58 (AccountID const& v)
{
    return base58EncodeToken(TokenType::AccountID, v.data(), v.size());
}

template<>
boost::optional<AccountID>
parseBase58 (std::string const& s)
{
    auto const result = decodeBase58Token(s, TokenType::AccountID);
    if (result.empty())
        return boost::none;
    AccountID id;
    if (result.size() != id.size())
        return boost::none;
    std::memcpy(id.data(),
        result.data(), result.size());
    return id;
}

boost::optional<AccountID>
deprecatedParseBitcoinAccountID (std::string const& s)
{
    auto const result = decodeBase58TokenBitcoin(s, TokenType::AccountID);
    if (result.empty())
        return boost::none;
    AccountID id;
    if (result.size() != id.size())
        return boost::none;
    std::memcpy(id.data(),
        result.data(), result.size());
    return id;
}

bool
deprecatedParseBase58 (AccountID& account,
    Json::Value const& jv)
{
    if (! jv.isString())
        return false;
    auto const result =
        parseBase58<AccountID>(jv.asString());
    if (! result)
        return false;
    account = *result;
    return true;
}

template<>
boost::optional<AccountID>
parseHex (std::string const& s)
{
    if (s.size() != 40)
        return boost::none;
    AccountID id;
    if (! id.SetHex(s, true))
        return boost::none;
    return id;
}

template<>
boost::optional<AccountID>
parseHexOrBase58 (std::string const& s)
{
    auto result =
        parseHex<AccountID>(s);
    if (! result)
        result = parseBase58<AccountID>(s);
    return result;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    计算帐户ID

    accountID是一个160位标识符，唯一
    区分帐户。账户可以或不可以
    存在于分类帐中。即使是不在
    可以执行分类帐、加密操作
    这会影响分类帐。例如，指定
    不在分类帐中的帐户作为
    分类帐中的帐户。

    我们为什么要用一半的sha512来做大多数事情，但后来
    sha256后面跟着ripemd160作为帐户ID？为什么没有
    我们做了一半，然后是ripemd160？或者是Sha512然后是Ripemd160？
    因为这个原因，为什么里佩姆160，为什么不留下来呢？
    只有160位？

    回答（David Schwartz）：

        简短的回答是我们保持了比特币的行为。
        更长的答案是：
            1）使用单个哈希可能会导致Ripple
               容易受到长度扩展攻击。
            2）只有ripemd160在160位时才被认为是安全的。

        任何这些计划都是可以接受的。然而，
        所选方案可以避免为所选方案辩护。
        （反对任何批评，除了不必要的复杂性。）

        “历史原因是在早期，
        我们想给人们很少的方法来证明我们
        不如比特币安全。所以没什么好理由
        为了改变某些东西，它没有改变。”
**/

AccountID
calcAccountID (PublicKey const& pk)
{
    ripesha_hasher rsh;
    rsh(pk.data(), pk.size());
    auto const d = static_cast<
        ripesha_hasher::result_type>(rsh);
    AccountID id;
    static_assert(sizeof(d) == id.size(), "");
    std::memcpy(id.data(), d.data(), d.size());
    return id;
}

AccountID const&
xrpAccount()
{
    static AccountID const account(0);
    return account;
}

AccountID const&
noAccount()
{
    static AccountID const account(1);
    return account;
}

bool
to_issuer (AccountID& issuer, std::string const& s)
{
    if (s.size () == (160 / 4))
    {
        issuer.SetHex (s);
        return true;
    }
    auto const account =
        parseBase58<AccountID>(s);
    if (! account)
        return false;
    issuer = *account;
    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*弗法科纸币
    替代实现只能使用一对insert
    每个都使用单个大内存分配的哈希映射
    存储固定大小的哈希表和所有accountID/string
    内存中的对（这里不使用std:：string
    带前缀的长度或以零结尾的数组）。可能使用
    作为无序容器的基础。
    这将减少到每交换一个分配/空闲周期
    地图。
**/


AccountIDCache::AccountIDCache(
        std::size_t capacity)
    : capacity_(capacity)
{
    m1_.reserve(capacity_);
}

std::string
AccountIDCache::toBase58(
    AccountID const& id) const
{
    std::lock_guard<
        std::mutex> lock(mutex_);
    auto iter = m1_.find(id);
    if (iter != m1_.end())
        return iter->second;
    iter = m0_.find(id);
    std::string result;
    if (iter != m0_.end())
    {
        result = iter->second;
//如果
//我们没有从这里抹去。
        m0_.erase(iter);
    }
    else
    {
        result =
            ripple::toBase58(id);
    }
    if (m1_.size() >= capacity_)
    {
        m0_ = std::move(m1_);
        m1_.clear();
        m1_.reserve(capacity_);
    }
    m1_.emplace(id, result);
    return result;
}

} //涟漪
