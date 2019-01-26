
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

#include <ripple/protocol/Issue.h>

namespace ripple {

bool
isConsistent (Issue const& ac)
{
    return isXRP (ac.currency) == isXRP (ac.account);
}

std::string
to_string (Issue const& ac)
{
    if (isXRP (ac.account))
        return to_string (ac.currency);

    return to_string(ac.account) + "/" + to_string(ac.currency);
}

std::ostream&
operator<< (std::ostream& os, Issue const& x)
{
    os << to_string (x);
    return os;
}

/*有序比较。
    资产先按货币订购，然后按账户订购。
    如果货币不是XRP。
**/

int
compare (Issue const& lhs, Issue const& rhs)
{
    int diff = compare (lhs.currency, rhs.currency);
    if (diff != 0)
        return diff;
    if (isXRP (lhs.currency))
        return 0;
    return compare (lhs.account, rhs.account);
}

/*平等比较。*/
/*@ {*/
bool
operator== (Issue const& lhs, Issue const& rhs)
{
    return compare (lhs, rhs) == 0;
}

bool
operator!= (Issue const& lhs, Issue const& rhs)
{
    return ! (lhs == rhs);
}
/*@ }*/

/*严格的弱排序。*/
/*@ {*/
bool
operator< (Issue const& lhs, Issue const& rhs)
{
    return compare (lhs, rhs) < 0;
}

bool
operator> (Issue const& lhs, Issue const& rhs)
{
    return rhs < lhs;
}

bool
operator>= (Issue const& lhs, Issue const& rhs)
{
    return ! (lhs < rhs);
}

bool
operator<= (Issue const& lhs, Issue const& rhs)
{
    return ! (rhs < lhs);
}

} //涟漪
