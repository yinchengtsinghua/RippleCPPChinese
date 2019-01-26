
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

#ifndef RIPPLE_PROTOCOL_BOOK_H_INCLUDED
#define RIPPLE_PROTOCOL_BOOK_H_INCLUDED

#include <ripple/protocol/Issue.h>
#include <boost/utility/base_from_member.hpp>

namespace ripple {

/*指定订单簿。
    订单簿是一对调用的问题。
    参见问题。
**/

class Book
{
public:
    Issue in;
    Issue out;

    Book ()
    {
    }

    Book (Issue const& in_, Issue const& out_)
        : in (in_)
        , out (out_)
    {
    }
};

bool
isConsistent (Book const& book);

std::string
to_string (Book const& book);

std::ostream&
operator<< (std::ostream& os, Book const& x);

template <class Hasher>
void
hash_append (Hasher& h, Book const& b)
{
    using beast::hash_append;
    hash_append(h, b.in, b.out);
}

Book
reversed (Book const& book);

/*有序比较。*/
int
compare (Book const& lhs, Book const& rhs);

/*平等比较。*/
/*@ {*/
bool
operator== (Book const& lhs, Book const& rhs);
bool
operator!= (Book const& lhs, Book const& rhs);
/*@ }*/

/*严格的弱排序。*/
/*@ {*/
bool
operator< (Book const& lhs,Book const& rhs);
bool
operator> (Book const& lhs, Book const& rhs);
bool
operator>= (Book const& lhs, Book const& rhs);
bool
operator<= (Book const& lhs, Book const& rhs);
/*@ }*/

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace std {

template <>
struct hash <ripple::Issue>
    : private boost::base_from_member <std::hash <ripple::Currency>, 0>
    , private boost::base_from_member <std::hash <ripple::AccountID>, 1>
{
private:
    using currency_hash_type = boost::base_from_member <
        std::hash <ripple::Currency>, 0>;
    using issuer_hash_type = boost::base_from_member <
        std::hash <ripple::AccountID>, 1>;

public:
    explicit hash() = default;

    using value_type = std::size_t;
    using argument_type = ripple::Issue;

    value_type operator() (argument_type const& value) const
    {
        value_type result (currency_hash_type::member (value.currency));
        if (!isXRP (value.currency))
            boost::hash_combine (result,
                issuer_hash_type::member (value.account));
        return result;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <>
struct hash <ripple::Book>
{
private:
    using hasher = std::hash <ripple::Issue>;

    hasher m_hasher;

public:
    explicit hash() = default;

    using value_type = std::size_t;
    using argument_type = ripple::Book;

    value_type operator() (argument_type const& value) const
    {
        value_type result (m_hasher (value.in));
        boost::hash_combine (result, m_hasher (value.out));
        return result;
    }
};

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace boost {

template <>
struct hash <ripple::Issue>
    : std::hash <ripple::Issue>
{
    explicit hash() = default;

    using Base = std::hash <ripple::Issue>;
//vvalco票据在VS2012中损坏
//使用base:：base；//继承ctors
};

template <>
struct hash <ripple::Book>
    : std::hash <ripple::Book>
{
    explicit hash() = default;

    using Base = std::hash <ripple::Book>;
//vvalco票据在VS2012中损坏
//使用base:：base；//继承ctors
};

}

#endif
