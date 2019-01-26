
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

#ifndef RIPPLE_PROTOCOL_ISSUE_H_INCLUDED
#define RIPPLE_PROTOCOL_ISSUE_H_INCLUDED

#include <cassert>
#include <functional>
#include <type_traits>

#include <ripple/protocol/UintTypes.h>

namespace ripple {

/*由账户发行的货币。
    @查看货币、账户ID、发行、账簿
**/

class Issue
{
public:
    Currency currency;
    AccountID account;

    Issue ()
    {
    }

    Issue (Currency const& c, AccountID const& a)
            : currency (c), account (a)
    {
    }
};

bool
isConsistent (Issue const& ac);

std::string
to_string (Issue const& ac);

std::ostream&
operator<< (std::ostream& os, Issue const& x);

template <class Hasher>
void
hash_append(Hasher& h, Issue const& r)
{
    using beast::hash_append;
    hash_append(h, r.currency, r.account);
}

/*有序比较。
    资产先按货币订购，然后按账户订购。
    如果货币不是XRP。
**/

int
compare (Issue const& lhs, Issue const& rhs);

/*平等比较。*/
/*@ {*/
bool
operator== (Issue const& lhs, Issue const& rhs);
bool
operator!= (Issue const& lhs, Issue const& rhs);
/*@ }*/

/*严格的弱排序。*/
/*@ {*/
bool
operator< (Issue const& lhs, Issue const& rhs);
bool
operator> (Issue const& lhs, Issue const& rhs);
bool
operator>= (Issue const& lhs, Issue const& rhs);
bool
operator<= (Issue const& lhs, Issue const& rhs);
/*@ }*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回表示xrp的资产说明符。*/
inline Issue const& xrpIssue ()
{
    static Issue issue {xrpCurrency(), xrpAccount()};
    return issue;
}

/*返回不表示帐户和货币的资产说明符。*/
inline Issue const& noIssue ()
{
    static Issue issue {noCurrency(), noAccount()};
    return issue;
}

}

#endif
