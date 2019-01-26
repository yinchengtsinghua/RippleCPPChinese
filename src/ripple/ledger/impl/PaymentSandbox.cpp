
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

#include <ripple/app/paths/impl/AmountSpec.h>
#include <ripple/ledger/PaymentSandbox.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/STAccount.h>
#include <boost/optional.hpp>

#include <cassert>

namespace ripple {

namespace detail {

auto DeferredCredits::makeKey (AccountID const& a1,
    AccountID const& a2,
    Currency const& c) -> Key
{
    if (a1 < a2)
        return std::make_tuple(a1, a2, c);
    else
        return std::make_tuple(a2, a1, c);
}

void
DeferredCredits::credit (AccountID const& sender,
    AccountID const& receiver,
    STAmount const& amount,
    STAmount const& preCreditSenderBalance)
{
    assert (sender != receiver);
    assert (!amount.negative());

    auto const k = makeKey (sender, receiver, amount.getCurrency ());
    auto i = credits_.find (k);
    if (i == credits_.end ())
    {
        Value v;

        if (sender < receiver)
        {
            v.highAcctCredits = amount;
            v.lowAcctCredits = amount.zeroed ();
            v.lowAcctOrigBalance = preCreditSenderBalance;
        }
        else
        {
            v.highAcctCredits = amount.zeroed ();
            v.lowAcctCredits = amount;
            v.lowAcctOrigBalance = -preCreditSenderBalance;
        }

        credits_[k] = v;
    }
    else
    {
//第一次只记录余额，不要在这里记录
        auto& v = i->second;
        if (sender < receiver)
            v.highAcctCredits += amount;
        else
            v.lowAcctCredits += amount;
    }
}

void
DeferredCredits::ownerCount (AccountID const& id,
    std::uint32_t cur,
    std::uint32_t next)
{
    auto const v = std::max (cur, next);
    auto r = ownerCounts_.emplace (std::make_pair (id, v));
    if (!r.second)
    {
        auto& mapVal = r.first->second;
        mapVal = std::max (v, mapVal);
    }
}

boost::optional<std::uint32_t>
DeferredCredits::ownerCount (AccountID const& id) const
{
    auto i = ownerCounts_.find (id);
    if (i != ownerCounts_.end ())
        return i->second;
    return boost::none;
}

//得到主平衡和其他平衡的调整。
auto
DeferredCredits::adjustments (AccountID const& main,
    AccountID const& other,
    Currency const& currency) const -> boost::optional<Adjustment>
{
    boost::optional<Adjustment> result;

    Key const k = makeKey (main, other, currency);
    auto i = credits_.find (k);
    if (i == credits_.end ())
        return result;

    auto const& v = i->second;

    if (main < other)
    {
        result.emplace (v.highAcctCredits, v.lowAcctCredits, v.lowAcctOrigBalance);
        return result;
    }
    else
    {
        result.emplace (v.lowAcctCredits, v.highAcctCredits, -v.lowAcctOrigBalance);
        return result;
    }
}

void DeferredCredits::apply(
    DeferredCredits& to)
{
    for (auto const& i : credits_)
    {
        auto r = to.credits_.emplace (i);
        if (!r.second)
        {
            auto& toVal = r.first->second;
            auto const& fromVal = i.second;
            toVal.lowAcctCredits += fromVal.lowAcctCredits;
            toVal.highAcctCredits += fromVal.highAcctCredits;
//不要更新原始余额，它已经正确了
        }
    }

    for (auto const& i : ownerCounts_)
    {
        auto r = to.ownerCounts_.emplace (i);
        if (!r.second)
        {
            auto& toVal = r.first->second;
            auto const& fromVal = i.second;
            toVal = std::max (toVal, fromVal);
        }
    }
}

} //细节

STAmount
PaymentSandbox::balanceHook (AccountID const& account,
    AccountID const& issuer,
        STAmount const& amount) const
{
    /*
    这里有两种算法。预切换算法采用
    当前金额并减去记录的贷方。后切换
    算法记住原始余额，并减去借方。这个
    切换后的算法应该在数值上更稳定。考虑一下
    初始余额很小的大额信贷。预切换算法
    计算（b+c）-c（其中b+c是传入的金额）。这个
    后切换算法返回b。当b和c相差较大时
    震级，（b+c）-c可能不等于b。
    **/


    auto const currency = amount.getCurrency ();
    auto const switchover = fix1141 (info ().parentCloseTime);

    auto adjustedAmt = amount;
    if (switchover)
    {
        auto delta = amount.zeroed ();
        auto lastBal = amount;
        auto minBal = amount;
        for (auto curSB = this; curSB; curSB = curSB->ps_)
        {
            if (auto adj = curSB->tab_.adjustments (account, issuer, currency))
            {
                delta += adj->debits;
                lastBal = adj->origBalance;
                if (lastBal < minBal)
                    minBal = lastBal;
            }
        }
        adjustedAmt = std::min(amount, lastBal - delta);
        if (rules().enabled(fix1368))
        {
//调整后的金额不得大于余额。在
//在某些情况下，可以使用延期信用表
//计算可用余额略高于分类帐
//计算（但始终小于实际余额）。
            adjustedAmt = std::min(adjustedAmt, minBal);
        }
        if (fix1274 (info ().parentCloseTime))
            adjustedAmt.setIssuer(amount.getIssuer());
    }
    else
    {
        for (auto curSB = this; curSB; curSB = curSB->ps_)
        {
            if (auto adj = curSB->tab_.adjustments (account, issuer, currency))
            {
                adjustedAmt -= adj->credits;
            }
        }
    }

    if (isXRP(issuer) && adjustedAmt < beast::zero)
//计算出的负xrp余额不是错误情况。考虑一下
//贷记大量XRP金额然后借记
//相同数量。信用证不能用，但我们减去借方，然后
//计算负值。这不是一个错误案例。
        adjustedAmt.clear();

    return adjustedAmt;
}

std::uint32_t
PaymentSandbox::ownerCountHook (AccountID const& account,
    std::uint32_t count) const
{
    std::uint32_t result = count;
    for (auto curSB = this; curSB; curSB = curSB->ps_)
    {
        if (auto adj = curSB->tab_.ownerCount (account))
            result = std::max (result, *adj);
    }
    return result;
}

void
PaymentSandbox::creditHook (AccountID const& from,
    AccountID const& to,
        STAmount const& amount,
            STAmount const& preCreditBalance)
{
    tab_.credit (from, to, amount, preCreditBalance);
}

void
PaymentSandbox::adjustOwnerCountHook (AccountID const& account,
    std::uint32_t cur,
        std::uint32_t next)
{
    tab_.ownerCount (account, cur, next);
}

void
PaymentSandbox::apply (RawView& to)
{
    assert(! ps_);
    items_.apply(to);
}

void
PaymentSandbox::apply (PaymentSandbox& to)
{
    assert(ps_ == &to);
    items_.apply(to);
    tab_.apply(to.tab_);
}

std::map<std::tuple<AccountID, AccountID, Currency>, STAmount>
PaymentSandbox::balanceChanges (ReadView const& view) const
{
    using key = std::tuple<AccountID, AccountID, Currency>;
//三角洲信托线路图。作为一种特殊情况，当信托的两端
//行是相同的货币，那么它是发行者的三角洲货币。到
//获取xrp余额的更改，account==root，issuer==root，currency==xrp
    std::map<key, STAmount> result;

//用低/高/货币/增量填充字典。这可以
//与其他版本的付款代码比较。
    auto each = [&result](uint256 const& key, bool isDelete,
        std::shared_ptr<SLE const> const& before,
        std::shared_ptr<SLE const> const& after) {

        STAmount oldBalance;
        STAmount newBalance;
        AccountID lowID;
        AccountID highID;

//从上一视图读取之前
        if (isDelete)
        {
            if (!before)
                return;

            auto const bt = before->getType ();
            switch(bt)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*before)[sfAccount];
                    oldBalance = (*before)[sfBalance];
                    newBalance = oldBalance.zeroed();
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*before)[sfLowLimit].getIssuer();
                    highID = (*before)[sfHighLimit].getIssuer();
                    oldBalance = (*before)[sfBalance];
                    newBalance = oldBalance.zeroed();
                    break;
                case ltOFFER:
//TBD
                    break;
                default:
                    break;
            }
        }
        else if (!before)
        {
//插入
            auto const at = after->getType ();
            switch(at)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*after)[sfAccount];
                    newBalance = (*after)[sfBalance];
                    oldBalance = newBalance.zeroed();
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*after)[sfLowLimit].getIssuer();
                    highID = (*after)[sfHighLimit].getIssuer();
                    newBalance = (*after)[sfBalance];
                    oldBalance = newBalance.zeroed();
                    break;
                case ltOFFER:
//TBD
                    break;
                default:
                    break;
            }
        }
        else
        {
//修改
            auto const at = after->getType ();
            assert (at == before->getType ());
            switch(at)
            {
                case ltACCOUNT_ROOT:
                    lowID = xrpAccount();
                    highID = (*after)[sfAccount];
                    oldBalance = (*before)[sfBalance];
                    newBalance = (*after)[sfBalance];
                    break;
                case ltRIPPLE_STATE:
                    lowID = (*after)[sfLowLimit].getIssuer();
                    highID = (*after)[sfHighLimit].getIssuer();
                    oldBalance = (*before)[sfBalance];
                    newBalance = (*after)[sfBalance];
                    break;
                case ltOFFER:
//TBD
                    break;
                default:
                    break;
            }
        }
//现在设置了以下内容，将它们放在地图中
        auto delta = newBalance - oldBalance;
        auto const cur = newBalance.getCurrency();
        result[std::make_tuple(lowID, highID, cur)] = delta;
        auto r = result.emplace(std::make_tuple(lowID, lowID, cur), delta);
        if (r.second)
        {
            r.first->second += delta;
        }

        delta.negate();
        r = result.emplace(std::make_tuple(highID, highID, cur), delta);
        if (r.second)
        {
            r.first->second += delta;
        }
    };
    items_.visit (view, each);
    return result;
}

XRPAmount PaymentSandbox::xrpDestroyed () const
{
    return items_.dropsDestroyed ();
}

}  //涟漪
