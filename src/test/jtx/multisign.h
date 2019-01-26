
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

#ifndef RIPPLE_TEST_JTX_MULTISIGN_H_INCLUDED
#define RIPPLE_TEST_JTX_MULTISIGN_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <test/jtx/owners.h>
#include <test/jtx/tags.h>
#include <cstdint>

namespace ripple {
namespace test {
namespace jtx {

/*签名者列表中的签名者*/
struct signer
{
    std::uint32_t weight;
    Account account;

    signer (Account account_,
            std::uint32_t weight_ = 1)
        : weight(weight_)
        , account(std::move(account_))
    {
    }
};

Json::Value
signers (Account const& account,
    std::uint32_t quorum,
        std::vector<signer> const& v);

/*删除签名者列表。*/
Json::Value
signers (Account const& account, none_t);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*在JTX上设置多签名。*/
class msig
{
public:
    struct Reg
    {
        Account acct;
        Account sig;

        Reg (Account const& masterSig)
        : acct (masterSig)
        , sig (masterSig)
        { }

        Reg (Account const& acct_, Account const& regularSig)
        : acct (acct_)
        , sig (regularSig)
        { }

        Reg (char const* masterSig)
        : acct (masterSig)
        , sig (masterSig)
        { }

        Reg (char const* acct_, char const* regularSig)
        : acct (acct_)
        , sig (regularSig)
        { }

        bool operator< (Reg const& rhs) const
        {
            return acct < rhs.acct;
        }
    };

    std::vector<Reg> signers;

public:
    msig (std::vector<Reg> signers_);

    template <class AccountType, class... Accounts>
    msig (AccountType&& a0, Accounts&&... aN)
        : msig(make_vector(
            std::forward<AccountType>(a0),
                std::forward<Accounts>(aN)...))
    {
    }

    void
    operator()(Env&, JTx& jt) const;

private:
    template <class AccountType>
    static
    void
    helper (std::vector<Reg>& v,
        AccountType&& account)
    {
        v.emplace_back(std::forward<
            Reg>(account));
    }

    template <class AccountType, class... Accounts>
    static
    void
    helper (std::vector<Reg>& v,
        AccountType&& a0, Accounts&&... aN)
    {
        helper(v, std::forward<AccountType>(a0));
        helper(v, std::forward<Accounts>(aN)...);
    }

    template <class... Accounts>
    static
    std::vector<Reg>
    make_vector(Accounts&&... signers)
    {
        std::vector<Reg> v;
        v.reserve(sizeof...(signers));
        helper(v, std::forward<
            Accounts>(signers)...);
        return v;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*签名者列表的数量匹配。*/
using siglists = owner_count<ltSIGNER_LIST>;

} //JTX
} //测试
} //涟漪

#endif
