
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

#include <ripple/protocol/Book.h>

namespace ripple {

bool
isConsistent (Book const& book)
{
    return isConsistent(book.in) && isConsistent (book.out)
            && book.in != book.out;
}

std::string
to_string (Book const& book)
{
    return to_string(book.in) + "->" + to_string(book.out);
}

std::ostream&
operator<< (std::ostream& os, Book const& x)
{
    os << to_string (x);
    return os;
}

Book
reversed (Book const& book)
{
    return Book (book.out, book.in);
}

/*有序比较。*/
int
compare (Book const& lhs, Book const& rhs)
{
    int const diff (compare (lhs.in, rhs.in));
    if (diff != 0)
        return diff;
    return compare (lhs.out, rhs.out);
}

/*平等比较。*/
/*@ {*/
bool
operator== (Book const& lhs, Book const& rhs)
{
    return (lhs.in == rhs.in) &&
           (lhs.out == rhs.out);
}

bool
operator!= (Book const& lhs, Book const& rhs)
{
    return (lhs.in != rhs.in) ||
           (lhs.out != rhs.out);
}
/*@ }*/

/*严格的弱排序。*/
/*@ {*/
bool
operator< (Book const& lhs,Book const& rhs)
{
    int const diff (compare (lhs.in, rhs.in));
    if (diff != 0)
        return diff < 0;
    return lhs.out < rhs.out;
}

bool
operator> (Book const& lhs, Book const& rhs)
{
    return rhs < lhs;
}

bool
operator>= (Book const& lhs, Book const& rhs)
{
    return ! (lhs < rhs);
}

bool
operator<= (Book const& lhs, Book const& rhs)
{
    return ! (rhs < lhs);
}

} //涟漪
