
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

#ifndef RIPPLE_TEST_JTX_OWNERS_H_INCLUDED
#define RIPPLE_TEST_JTX_OWNERS_H_INCLUDED

#include <test/jtx/Env.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/UintTypes.h>
#include <cstdint>

namespace ripple {
namespace test {
namespace jtx {

namespace detail {

std::uint32_t
owned_count_of (ReadView const& view,
    AccountID const& id,
        LedgerEntryType type);

void
owned_count_helper(Env& env,
    AccountID const& id,
        LedgerEntryType type,
            std::uint32_t value);

} //细节

//别名帮助器
template <LedgerEntryType Type>
class owner_count
{
private:
    Account account_;
    std::uint32_t value_;

public:
    owner_count (Account const& account,
            std::uint32_t value)
        : account_(account)
        , value_(value)
    {
    }

    void
    operator()(Env& env) const
    {
        detail::owned_count_helper(
            env, account_.id(), Type, value_);
    }
};

/*匹配帐户所有者目录中的项目数*/
class owners
{
private:
    Account account_;
    std::uint32_t value_;
public:
    owners (Account const& account,
            std::uint32_t value)
        : account_(account)
        , value_(value)
    {
    }

    void
    operator()(Env& env) const;
};

/*匹配帐户所有者目录中的信任行数*/
using lines = owner_count<ltRIPPLE_STATE>;

/*匹配帐户所有者目录中的报价数量*/
using offers = owner_count<ltOFFER>;

} //JTX
} //测试
} //涟漪

#endif
