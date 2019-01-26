
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

#ifndef RIPPLE_TEST_JTX_TICKET_H_INCLUDED
#define RIPPLE_TEST_JTX_TICKET_H_INCLUDED

#include <test/jtx/Env.h>
#include <test/jtx/Account.h>
#include <test/jtx/owners.h>
#include <boost/optional.hpp>
#include <cstdint>

namespace ripple {
namespace test {
namespace jtx {

/*
    这说明了JTX系统如何扩展到其他系统
    发电机、漏斗、条件和操作，
    不更改基声明。
**/


/*票务运营*/
namespace ticket {

namespace detail {

Json::Value
create (Account const& account,
    boost::optional<Account> const& target,
        boost::optional<std::uint32_t> const& expire);

inline
void
create_arg (boost::optional<Account>& opt,
    boost::optional<std::uint32_t>&,
        Account const& value)
{
    opt = value;
}

inline
void
create_arg (boost::optional<Account>&,
    boost::optional<std::uint32_t>& opt,
        std::uint32_t value)
{
    opt = value;
}

inline
void
create_args (boost::optional<Account>&,
    boost::optional<std::uint32_t>&)
{
}

template<class Arg, class... Args>
void
create_args(boost::optional<Account>& account_opt,
    boost::optional<std::uint32_t>& expire_opt,
        Arg const& arg, Args const&... args)
{
    create_arg(account_opt, expire_opt, arg);
    create_args(account_opt, expire_opt, args...);
}

} //细节

/*开罚单*/
template <class... Args>
Json::Value
create (Account const& account,
    Args const&... args)
{
    boost::optional<Account> target;
    boost::optional<std::uint32_t> expire;
    detail::create_args(target, expire, args...);
    return detail::create(
        account, target, expire);
}

/*取消车票*/
Json::Value
cancel(Account const& account, std::string const & ticketId);

} //票

/*匹配帐户上的票数。*/
using tickets = owner_count<ltTICKET>;

} //JTX

} //测试
} //涟漪

#endif
