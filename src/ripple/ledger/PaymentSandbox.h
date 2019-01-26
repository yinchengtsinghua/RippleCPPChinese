
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

#ifndef RIPPLE_LEDGER_PAYMENTSANDBOX_H_INCLUDED
#define RIPPLE_LEDGER_PAYMENTSANDBOX_H_INCLUDED

#include <ripple/ledger/RawView.h>
#include <ripple/ledger/Sandbox.h>
#include <ripple/ledger/detail/ApplyViewBase.h>
#include <ripple/protocol/AccountID.h>
#include <map>
#include <utility>

namespace ripple {

namespace detail {

//vfalc todo内联此实现
//到PaymentSandbox类本身
class DeferredCredits
{
public:
    struct Adjustment
    {
        Adjustment (STAmount const& d, STAmount const& c, STAmount const& b)
            : debits (d), credits (c), origBalance (b) {}
        STAmount debits;
        STAmount credits;
        STAmount origBalance;
    };

//得到主平衡和其他平衡的调整。
//返回借方、贷方和原始余额
    boost::optional<Adjustment> adjustments (
        AccountID const& main,
        AccountID const& other,
        Currency const& currency) const;

    void credit (AccountID const& sender,
        AccountID const& receiver,
        STAmount const& amount,
        STAmount const& preCreditSenderBalance);

    void ownerCount (AccountID const& id,
        std::uint32_t cur,
            std::uint32_t next);

//获取调整后的所有者计数。因为延期信贷是用来
//在付款中，付款只减少所有者计数，返回最大值
//记住的所有者计数。
    boost::optional<std::uint32_t>
    ownerCount (AccountID const& id) const;

    void apply (DeferredCredits& to);
private:
//低账户，高账户
    using Key = std::tuple<
        AccountID, AccountID, Currency>;
    struct Value
    {
        explicit Value() = default;

        STAmount lowAcctCredits;
        STAmount highAcctCredits;
        STAmount lowAcctOrigBalance;
    };

    static
    Key
    makeKey (AccountID const& a1,
        AccountID const& a2,
            Currency const& c);

    std::map<Key, Value> credits_;
    std::map<AccountID, std::uint32_t> ownerCounts_;
};

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*使贷方不能用于余额的包装器。

    这是用于付款和寻路，以便消费
    一条路径的流动性不会导致该路径的一部分或
    其他获得流动性的途径。

    应用程序视图API中某些自由函数的行为
    将通过BalanceHook和CreditHook覆盖进行更改
    付款沙盒。

    @注释显示为客户端的ApplyView
**/

class PaymentSandbox final
    : public detail::ApplyViewBase
{
public:
    PaymentSandbox() = delete;
    PaymentSandbox (PaymentSandbox const&) = delete;
    PaymentSandbox& operator= (PaymentSandbox&&) = delete;
    PaymentSandbox& operator= (PaymentSandbox const&) = delete;

    PaymentSandbox (PaymentSandbox&&) = default;

    PaymentSandbox (ReadView const* base, ApplyFlags flags)
        : ApplyViewBase (base, flags)
    {
    }

    PaymentSandbox (ApplyView const* base)
        : ApplyViewBase (base, base->flags())
    {
    }

    /*在现有PaymentSandbox上进行构造。

        当
        调用了Apply（）。

        @param parent指向父级的非空指针。

        @注意：指针用于防止混淆
              具有复制结构。
    **/

//vfalc如果我们在付费沙箱的基础上进行构建，
//或者是PaymentSandbox派生类，我们必须
//其中一个构造函数或不变量将被破坏。
    /*@ {*/
    explicit
    PaymentSandbox (PaymentSandbox const* base)
        : ApplyViewBase(base, base->flags())
        , ps_ (base)
    {
    }

    explicit
    PaymentSandbox (PaymentSandbox* base)
        : ApplyViewBase(base, base->flags())
        , ps_ (base)
    {
    }
    /*@ }*/

    STAmount
    balanceHook (AccountID const& account,
        AccountID const& issuer,
            STAmount const& amount) const override;

    void
    creditHook (AccountID const& from,
        AccountID const& to,
            STAmount const& amount,
                STAmount const& preCreditBalance) override;

    void
    adjustOwnerCountHook (AccountID const& account,
        std::uint32_t cur,
            std::uint32_t next) override;

    std::uint32_t
    ownerCountHook (AccountID const& account,
        std::uint32_t count) const override;

    /*将更改应用于基础视图。

        `to`必须包含与父级相同的内容
        视图在构造时传递，否则未定义
        会导致行为。
    **/

    /*@ {*/
    void apply (RawView& to);

    void
    apply (PaymentSandbox& to);
    /*@ }*/

//返回信任行上的余额更改映射。低账户是
//密钥中的第一个帐户。如果两个帐户相等，则映射包含
//不管发行人是谁，货币的总变化。这对
//XRP余额的总变化。
    std::map<std::tuple<AccountID, AccountID, Currency>, STAmount>
    balanceChanges (ReadView const& view) const;

    XRPAmount xrpDestroyed () const;

private:
    detail::DeferredCredits tab_;
    PaymentSandbox const* ps_ = nullptr;
};

}  //涟漪

#endif
