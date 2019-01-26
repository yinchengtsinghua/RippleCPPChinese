
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

#include <ripple/basics/safe_cast.h>
#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <cassert>
#include <cmath>
#include <iomanip>

namespace ripple {
namespace test {
namespace jtx {

#if 0
std::ostream&
operator<<(std::ostream&& os,
    AnyAmount const& amount)
{
    if (amount.is_any)
    {
        os << amount.value.getText() << "/" <<
            to_string(amount.value.issue().currency) <<
                "*";
        return os;
    }
    os << amount.value.getText() << "/" <<
        to_string(amount.value.issue().currency) <<
            "(" << amount.name() << ")";
    return os;
}
#endif

PrettyAmount::operator AnyAmount() const
{
    return { amount_ };
}

template <typename T>
static
std::string
to_places(const T d, std::uint8_t places)
{
    assert(places <= std::numeric_limits<T>::digits10);

    std::ostringstream oss;
    oss << std::setprecision(places) << std::fixed << d;

    std::string out = oss.str();
    out.erase(out.find_last_not_of('0') + 1, std::string::npos);
    if (out.back() == '.')
        out.pop_back();

    return out;
}

std::ostream&
operator<< (std::ostream& os,
    PrettyAmount const& amount)
{
    if (amount.value().native())
    {
//百分位数
        auto const c =
            dropsPerXRP<int>::value / 100;
        auto const n = amount.value().mantissa();
        if(n < c)
        {
            if (amount.value().negative())
                os << "-" << n << " drops";
            else
                os << n << " drops";
            return os;
        }
        auto const d = double(n) /
            dropsPerXRP<int>::value;
        if (amount.value().negative())
            os << "-";

        os << to_places(d, 6) << " XRP";
    }
    else
    {
        os <<
            amount.value().getText() << "/" <<
                to_string(amount.value().issue().currency) <<
                    "(" << amount.name() << ")";
    }
    return os;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

XRP_t const XRP {};

PrettyAmount
IOU::operator()(epsilon_t) const
{
    return { STAmount(issue(), 1, -81),
        account.name() };
}

PrettyAmount
IOU::operator()(detail::epsilon_multiple m) const
{
    return { STAmount(issue(),
        safe_cast<std::uint64_t>(m.n), -81),
            account.name() };
}

std::ostream&
operator<<(std::ostream& os,
    IOU const& iou)
{
    os <<
        to_string(iou.issue().currency) <<
            "(" << iou.account.name() << ")";
    return os;
}

any_t const any { };

} //JTX
} //测试
} //涟漪
