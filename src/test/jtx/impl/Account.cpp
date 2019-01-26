
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

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {
namespace test {
namespace jtx {

std::unordered_map<
    std::pair<std::string, KeyType>,
        Account, beast::uhash<>> Account::cache_;

Account const Account::master("master",
    generateKeyPair(KeyType::secp256k1,
        generateSeed("masterpassphrase")), Account::privateCtorTag{});

Account::Account(std::string name,
        std::pair<PublicKey, SecretKey> const& keys, Account::privateCtorTag)
    : name_(std::move(name))
    , pk_ (keys.first)
    , sk_ (keys.second)
    , id_ (calcAccountID(pk_))
    , human_ (toBase58(id_))
{
}

Account Account::fromCache(std::string name, KeyType type)
{
    auto const p = std::make_pair (name, type);
    auto const iter = cache_.find (p);
    if (iter != cache_.end ())
        return iter->second;

    auto const keys = generateKeyPair (type, generateSeed (name));
    auto r = cache_.emplace (std::piecewise_construct,
        std::forward_as_tuple (std::move (p)),
        std::forward_as_tuple (std::move (name), keys, privateCtorTag{}));
    return r.first->second;
}

Account::Account (std::string name, KeyType type)
    : Account (fromCache (std::move (name), type))
{
}

IOU
Account::operator[](std::string const& s) const
{
    auto const currency = to_currency(s);
    assert(currency != noCurrency());
    return IOU(*this, currency);
}

} //JTX
} //测试
} //涟漪
