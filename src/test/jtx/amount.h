
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

#ifndef RIPPLE_TEST_JTX_AMOUNT_H_INCLUDED
#define RIPPLE_TEST_JTX_AMOUNT_H_INCLUDED

#include <test/jtx/Account.h>
#include <test/jtx/amount.h>
#include <test/jtx/tags.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/basics/contract.h>
#include <cstdint>
#include <ostream>
#include <string>
#include <type_traits>

namespace ripple {
namespace test {
namespace jtx {

/*

决定接受水滴和xrp的数量。
使用int类型，因为xrp的范围是1000亿
同时具有有符号和无符号重载会创建
导致过载分辨率模糊的复杂代码。

**/


struct AnyAmount;

//表示货币的“无金额”
//这与零或平衡不同。
//例如，没有美元表示信托额度
//根本不存在。在一个
//不适当的上下文将生成
//编译错误。
//
struct None
{
    Issue issue;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class T>
struct dropsPerXRP
{
    static T const value = 1000000;
};

/*表示xrp或iou数量
    这将自定义字符串转换并支持
    从整数和浮点进行xrp转换。
**/

struct PrettyAmount
{
private:
//vfalc todo应为金额
    STAmount amount_;
    std::string name_;

public:
    PrettyAmount() = default;
    PrettyAmount (PrettyAmount const&) = default;
    PrettyAmount& operator=(PrettyAmount const&) = default;

    PrettyAmount (STAmount const& amount,
            std::string const& name)
        : amount_(amount)
        , name_(name)
    {
    }

    /*滴*/
    template <class T>
    PrettyAmount (T v, std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_integral<T>::value &&
                std::is_signed<T>::value>* = nullptr)
        : amount_((v > 0) ?
            v : -v, v < 0)
    {
    }

    /*滴*/
    template <class T>
    PrettyAmount (T v, std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_integral<T>::value &&
                std::is_unsigned<T>::value>* = nullptr)
        : amount_(v)
    {
    }

    std::string const&
    name() const
    {
        return name_;
    }

    STAmount const&
    value() const
    {
        return amount_;
    }

    operator STAmount const&() const
    {
        return amount_;
    }

    operator AnyAmount() const;
};

inline
bool
operator== (PrettyAmount const& lhs,
    PrettyAmount const& rhs)
{
    return lhs.value() == rhs.value();
}

std::ostream&
operator<< (std::ostream& os,
    PrettyAmount const& amount);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//指定订单簿
struct BookSpec
{
    AccountID account;
    ripple::Currency currency;

    BookSpec(AccountID const& account_,
        ripple::Currency const& currency_)
        : account(account_)
        , currency(currency_)
    {
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

struct XRP_t
{
    /*隐式转换为问题。

        这允许通过xrp
        需要一个问题。
    **/

    operator Issue() const
    {
        return xrpIssue();
    }

    /*返回作为stamount的xrp数量

        @param v xrp的数目（不是drops）
    **/

    /*@ {*/
    template <class T, class = std::enable_if_t<
        std::is_integral<T>::value>>
    PrettyAmount
    operator()(T v) const
    {
        return { std::conditional_t<
            std::is_signed<T>::value,
                std::int64_t, std::uint64_t>{v} *
                    dropsPerXRP<T>::value };
    }

    PrettyAmount
    operator()(double v) const
    {
        auto const c =
            dropsPerXRP<int>::value;
        if (v >= 0)
        {
            auto const d = std::uint64_t(
                std::round(v * c));
            if (double(d) / c != v)
                Throw<std::domain_error> (
                    "unrepresentable");
            return { d };
        }
        auto const d = std::int64_t(
            std::round(v * c));
        if (double(d) / c != v)
            Throw<std::domain_error> (
                "unrepresentable");
        return { d };
    }
    /*@ }*/

    /*不返回任何xrp*/
    None
    operator()(none_t) const
    {
        return { xrpIssue() };
    }

    friend
    BookSpec
    operator~ (XRP_t const&)
    {        
        return BookSpec( xrpAccount(),
            xrpCurrency() );
    }
};

/*转换为xrp issue或stamount。

    实例：
        xrp转换为xrp问题
        xrp（10）返回10 xrp的stamount
**/

extern XRP_t const XRP;

/*返回xrp stamount。

    例子：
        drops（10）返回10个drops的stamount
**/

template <class Integer,
    class = std::enable_if_t<
        std::is_integral<Integer>::value>>
PrettyAmount
drops (Integer i)
{
    return { i };
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

struct epsilon_multiple
{
    std::size_t n;
};

} //细节

//最小可能的借据金额
struct epsilon_t
{
    epsilon_t()
    {
    }

    detail::epsilon_multiple
    operator()(std::size_t n) const
    {
        return { n };
    }
};

static epsilon_t const epsilon;

/*转换为IOU问题或stamount。

    实例：
        IOU转换为基础问题
        IOU（10）返回的stamount为10
                        根本问题。
**/

class IOU
{
public:
    Account account;
    ripple::Currency currency;

    IOU(Account const& account_,
            ripple::Currency const& currency_)
        : account(account_)
        , currency(currency_)
    {
    }

    Issue
    issue() const
    {
        return { currency, account.id() };
    }

    /*隐式转换为问题。

        这允许传递IOU
        需要问题的值。
    **/

    operator Issue() const
    {
        return issue();
    }

    template <class T, class = std::enable_if_t<
        sizeof(T) >= sizeof(int) &&
            std::is_arithmetic<T>::value>>
    PrettyAmount operator()(T v) const
    {
//如果
//V的表示不准确。
        return { amountFromString(issue(),
            std::to_string(v)), account.name() };
    }

    PrettyAmount operator()(epsilon_t) const;
    PrettyAmount operator()(detail::epsilon_multiple) const;

//沃尔法科托多
//stamount operator（）（char const*s）const；

    /*不返回任何问题*/
    None operator()(none_t) const
    {
        return { issue() };
    }

    friend
    BookSpec
    operator~ (IOU const& iou)
    {
        return BookSpec(iou.account.id(), iou.currency);
    }
};

std::ostream&
operator<<(std::ostream& os,
    IOU const& iou);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

struct any_t
{
    inline
    AnyAmount
    operator()(STAmount const& sta) const;
};

/*带有任何颁发者选项的金额说明符。*/
struct AnyAmount
{
    bool is_any;
    STAmount value;

    AnyAmount() = delete;
    AnyAmount (AnyAmount const&) = default;
    AnyAmount& operator= (AnyAmount const&) = default;

    AnyAmount (STAmount const& amount)
        : is_any(false)
        , value(amount)
    {
    }

    AnyAmount (STAmount const& amount,
            any_t const*)
        : is_any(true)
        , value(amount)
    {
    }

//将问题重置为特定帐户
    void
    to (AccountID const& id)
    {
        if (! is_any)
            return;
        value.setIssuer(id);
    }
};

inline
AnyAmount
any_t::operator()(STAmount const& sta) const
{
    return AnyAmount(sta, this);
}

/*返回表示“任何发行人”的金额
    @注意收件人将接受的内容
**/

extern any_t const any;

} //JTX
} //测试
} //涟漪

#endif
