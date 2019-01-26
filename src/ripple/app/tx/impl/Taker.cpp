
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
    版权所有（c）2014 Ripple Labs Inc.

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

#include <ripple/app/tx/impl/Taker.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>

namespace ripple {

static
std::string
format_amount (STAmount const& amount)
{
    std::string txt = amount.getText ();
    txt += "/";
    txt += to_string (amount.issue().currency);
    return txt;
}

BasicTaker::BasicTaker (
        CrossType cross_type, AccountID const& account, Amounts const& amount,
        Quality const& quality, std::uint32_t flags, Rate const& rate_in,
        Rate const& rate_out, beast::Journal journal)
    : account_ (account)
    , quality_ (quality)
    , threshold_ (quality_)
    , sell_ (flags & tfSell)
    , original_ (amount)
    , remaining_ (amount)
    , issue_in_ (remaining_.in.issue ())
    , issue_out_ (remaining_.out.issue ())
    , m_rate_in (rate_in)
    , m_rate_out (rate_out)
    , cross_type_ (cross_type)
    , journal_ (journal)
{
    assert (remaining_.in > beast::zero);
    assert (remaining_.out > beast::zero);

    assert (m_rate_in.value != 0);
    assert (m_rate_out.value != 0);

//如果我们要处理的是一种特殊的口味，确保它是
//我们期望的口味：
    assert (cross_type != CrossType::XrpToIou ||
        (isXRP (issue_in ()) && !isXRP (issue_out ())));

    assert (cross_type != CrossType::IouToXrp ||
        (!isXRP (issue_in ()) && isXRP (issue_out ())));

//确保我们不会越过xrp换xrp
    assert (!isXRP (issue_in ()) || !isXRP (issue_out ()));

//如果这是被动订单，我们会调整质量以防止报价。
//在相同的质量水平上。
    if (flags & tfPassive)
        ++threshold_;
}

Rate
BasicTaker::effective_rate (
    Rate const& rate, Issue const &issue,
    AccountID const& from, AccountID const& to)
{
//如果有转移率，发行人不参与。
//发送者和接收者不同，返回
//实际传输速率。
    if (rate != parityRate &&
        from != to &&
        from != issue.account &&
        to != issue.account)
    {
        return rate;
    }

    return parityRate;
}

bool
BasicTaker::unfunded () const
{
    if (get_funds (account(), remaining_.in) > beast::zero)
        return false;

    JLOG(journal_.debug()) << "Unfunded: taker is out of funds.";
    return true;
}

bool
BasicTaker::done () const
{
//如果我们消耗了所有输入货币，我们就完成了
    if (remaining_.in <= beast::zero)
    {
        JLOG(journal_.debug()) << "Done: all the input currency has been consumed.";
        return true;
    }

//如果使用购买语义并接收到
//所需输出货币金额
    if (!sell_ && (remaining_.out <= beast::zero))
    {
        JLOG(journal_.debug()) << "Done: the desired amount has been received.";
        return true;
    }

//如果接受者没有钱，我们就完了
    if (unfunded ())
    {
        JLOG(journal_.debug()) << "Done: taker out of funds.";
        return true;
    }

    return false;
}

Amounts
BasicTaker::remaining_offer () const
{
//如果接受者完成了，那么就没有出价。
    if (done ())
        return Amounts (remaining_.in.zeroed(), remaining_.out.zeroed());

//如果我们不交叉，就完全避免数学。
    if (original_ == remaining_)
       return original_;

    if (sell_)
    {
        assert (remaining_.in > beast::zero);

//我们根据剩余输入缩放输出：
        return Amounts (remaining_.in, divRound (
            remaining_.in, quality_.rate (), issue_out_, true));
    }

    assert (remaining_.out > beast::zero);

//我们根据剩余输出缩放输入：
    return Amounts (mulRound (
        remaining_.out, quality_.rate (), issue_in_, true),
        remaining_.out);
}

Amounts const&
BasicTaker::original_offer () const
{
    return original_;
}

//TODO：“output”的存在是由于
//金额包含应分离的问题信息。
static
STAmount
qual_div (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = divide (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

static
STAmount
qual_mul (STAmount const& amount, Quality const& quality, STAmount const& output)
{
    auto result = multiply (amount, quality.rate (), output.issue ());
    return std::min (result, output);
}

void
BasicTaker::log_flow (char const* description, Flow const& flow)
{
    auto stream = journal_.debug();
    if (!stream)
        return;

    stream << description;

    if (isXRP (issue_in ()))
        stream << "   order in: " << format_amount (flow.order.in);
    else
        stream << "   order in: " << format_amount (flow.order.in) <<
            " (issuer: " << format_amount (flow.issuers.in) << ")";

    if (isXRP (issue_out ()))
        stream << "  order out: " << format_amount (flow.order.out);
    else
        stream << "  order out: " << format_amount (flow.order.out) <<
            " (issuer: " << format_amount (flow.issuers.out) << ")";
}

BasicTaker::Flow
BasicTaker::flow_xrp_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.out = multiply (f.order.out, rate_out);

    log_flow ("flow_xrp_to_iou", f);

//夹住所有者平衡
    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = divide (f.issuers.out, rate_out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        log_flow ("(clamped on owner balance)", f);
    }

//如果接受者想要限制输出，则夹紧
    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker output)", f);
    }

//限制接受者的资金
    if (taker_funds < f.order.in)
    {
        f.order.in = taker_funds;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker funds)", f);
    }

//如果我们不处理第二个问题，请接受剩余报价。
//一条高速公路。
    if (cross_type_ == CrossType::XrpToIou && (remaining_.in < f.order.in))
    {
        f.order.in = remaining_.in;
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker input)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_xrp (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in)
{
    Flow f;
    f.order = order;
    f.issuers.in = multiply (f.order.in, rate_in);

    log_flow ("flow_iou_to_xrp", f);

//限制业主资金
    if (owner_funds < f.order.out)
    {
        f.order.out = owner_funds;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on owner funds)", f);
    }

//如果接受者想要限制输出，而我们不是
//高速公路的第一段。
    if (!sell_ && cross_type_ == CrossType::IouToXrp)
    {
        if (remaining_.out < f.order.out)
        {
            f.order.out = remaining_.out;
            f.order.in = qual_mul (f.order.out, quality, f.order.in);
            f.issuers.in = multiply (f.order.in, rate_in);
            log_flow ("(clamped on taker output)", f);
        }
    }

//限制接受方的输入报价
    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = multiply (f.order.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker input)", f);
    }

//夹住接受者的输入平衡
    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = divide (f.issuers.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

BasicTaker::Flow
BasicTaker::flow_iou_to_iou (
    Amounts const& order, Quality quality,
    STAmount const& owner_funds, STAmount const& taker_funds,
    Rate const& rate_in, Rate const& rate_out)
{
    Flow f;
    f.order = order;
    f.issuers.in = multiply (f.order.in, rate_in);
    f.issuers.out = multiply (f.order.out, rate_out);

    log_flow ("flow_iou_to_iou", f);

//夹住所有者平衡
    if (owner_funds < f.issuers.out)
    {
        f.issuers.out = owner_funds;
        f.order.out = divide (f.issuers.out, rate_out);
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on owner funds)", f);
    }

//接受方报价
    if (!sell_ && remaining_.out < f.order.out)
    {
        f.order.out = remaining_.out;
        f.order.in = qual_mul (f.order.out, quality, f.order.in);
        f.issuers.out = multiply (f.order.out, rate_out);
        f.issuers.in = multiply (f.order.in, rate_in);
        log_flow ("(clamped on taker output)", f);
    }

//限制接受方的输入报价
    if (remaining_.in < f.order.in)
    {
        f.order.in = remaining_.in;
        f.issuers.in = multiply (f.order.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker input)", f);
    }

//夹住接受者的输入平衡
    if (taker_funds < f.issuers.in)
    {
        f.issuers.in = taker_funds;
        f.order.in = divide (f.issuers.in, rate_in);
        f.order.out = qual_div (f.order.in, quality, f.order.out);
        f.issuers.out = multiply (f.order.out, rate_out);
        log_flow ("(clamped on taker funds)", f);
    }

    return f;
}

//计算通过指定报价的直接流量
BasicTaker::Flow
BasicTaker::do_cross (Amounts offer, Quality quality, AccountID const& owner)
{
    auto const owner_funds = get_funds (owner, offer.out);
    auto const taker_funds = get_funds (account (), offer.in);

    Flow result;

    if (cross_type_ == CrossType::XrpToIou)
    {
        result = flow_xrp_to_iou (offer, quality, owner_funds, taker_funds,
            out_rate (owner, account ()));
    }
    else if (cross_type_ == CrossType::IouToXrp)
    {
        result = flow_iou_to_xrp (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()));
    }
    else
    {
        result = flow_iou_to_iou (offer, quality, owner_funds, taker_funds,
            in_rate (owner, account ()), out_rate (owner, account ()));
    }

    if (!result.sanity_check ())
        Throw<std::logic_error> ("Computed flow fails sanity check.");

    remaining_.out -= result.order.out;
    remaining_.in -= result.order.in;

    assert (remaining_.in >= beast::zero);

    return result;
}

//计算通过指定优惠的桥接流
std::pair<BasicTaker::Flow, BasicTaker::Flow>
BasicTaker::do_cross (
    Amounts offer1, Quality quality1, AccountID const& owner1,
    Amounts offer2, Quality quality2, AccountID const& owner2)
{
    assert (!offer1.in.native ());
    assert (offer1.out.native ());
    assert (offer2.in.native ());
    assert (!offer2.out.native ());

//如果接受者拥有报价的第一部分，那么接受者就可以
//资金不是投入的限制因素——提供本身就是。
    auto leg1_in_funds = get_funds (account (), offer1.in);

    if (account () == owner1)
    {
        JLOG(journal_.trace()) << "The taker owns the first leg of a bridge.";
        leg1_in_funds = std::max (leg1_in_funds, offer1.in);
    }

//如果接受者拥有报价的第二部分，那么接受者就可以
//资金并不是产出的限制因素——报价本身就是。
    auto leg2_out_funds = get_funds (owner2, offer2.out);

    if (account () == owner2)
    {
        JLOG(journal_.trace()) << "The taker owns the second leg of a bridge.";
        leg2_out_funds = std::max (leg2_out_funds, offer2.out);
    }

//可通过xrp流动的金额是
//桥的第一段有，直到第一段的输出。
//
//但是，当一座桥的两条腿都属于同一个人时，
//在两条腿之间流动的xrp基本上是无限的。
//因为所有的主人都在从他的左口袋里取出xrp
//把它放在他的右口袋里。在这种情况下，我们设置
//XRP是这两个报价中最大的一个。
    auto xrp_funds = get_funds (owner1, offer1.out);

    if (owner1 == owner2)
    {
        JLOG(journal_.trace()) <<
            "The bridge endpoints are owned by the same account.";
        xrp_funds = std::max (offer1.out, offer2.in);
    }

    if (auto stream = journal_.debug())
    {
        stream << "Available bridge funds:";
        stream << "  leg1 in: " << format_amount (leg1_in_funds);
        stream << " leg2 out: " << format_amount (leg2_out_funds);
        stream << "      xrp: " << format_amount (xrp_funds);
    }

    auto const leg1_rate = in_rate (owner1, account ());
    auto const leg2_rate = out_rate (owner2, account ());

//尝试确定可以通过每个管道达到的最大流量
//腿独立于另一条腿。
    auto flow1 = flow_iou_to_xrp (offer1, quality1, xrp_funds, leg1_in_funds, leg1_rate);

    if (!flow1.sanity_check ())
        Throw<std::logic_error> ("Computed flow1 fails sanity check.");

    auto flow2 = flow_xrp_to_iou (offer2, quality2, leg2_out_funds, xrp_funds, leg2_rate);

    if (!flow2.sanity_check ())
        Throw<std::logic_error> ("Computed flow2 fails sanity check.");

//现在，我们可以单独计算每个支腿的最大流量。我们需要
//使它们相等，以便从第一个支腿流出的xrp量
//与流入第二个支腿的XRP数量相同。我们采取
//限制因素（如有）的一侧，并调整另一侧。
    if (flow1.order.out < flow2.order.in)
    {
//向下调整报价的第二段：
        flow2.order.in = flow1.order.out;
        flow2.order.out = qual_div (flow2.order.in, quality2, flow2.order.out);
        flow2.issuers.out = multiply (flow2.order.out, leg2_rate);
        log_flow ("Balancing: adjusted second leg down", flow2);
    }
    else if (flow1.order.out > flow2.order.in)
    {
//向下调整报价的第一段：
        flow1.order.out = flow2.order.in;
        flow1.order.in = qual_mul (flow1.order.out, quality1, flow1.order.in);
        flow1.issuers.in = multiply (flow1.order.in, leg1_rate);
        log_flow ("Balancing: adjusted first leg down", flow2);
    }

    if (flow1.order.out != flow2.order.in)
        Throw<std::logic_error> ("Bridged flow is out of balance.");

    remaining_.out -= flow2.order.out;
    remaining_.in -= flow1.order.in;

    return std::make_pair (flow1, flow2);
}

//==============================================================

Taker::Taker (CrossType cross_type, ApplyView& view,
    AccountID const& account, Amounts const& offer,
        std::uint32_t flags,
            beast::Journal journal)
    : BasicTaker (cross_type, account, offer, Quality(offer), flags,
        calculateRate(view, offer.in.getIssuer(), account),
        calculateRate(view, offer.out.getIssuer(), account), journal)
    , view_ (view)
    , xrp_flow_ (0)
    , direct_crossings_ (0)
    , bridge_crossings_ (0)
{
    assert (issue_in () == offer.in.issue ());
    assert (issue_out () == offer.out.issue ());

    if (auto stream = journal_.debug())
    {
        stream << "Crossing as: " << to_string (account);

        if (isXRP (issue_in ()))
            stream << "   Offer in: " << format_amount (offer.in);
        else
            stream << "   Offer in: " << format_amount (offer.in) <<
                " (issuer: " << issue_in ().account << ")";

        if (isXRP (issue_out ()))
            stream << "  Offer out: " << format_amount (offer.out);
        else
            stream << "  Offer out: " << format_amount (offer.out) <<
                " (issuer: " << issue_out ().account << ")";

        stream <<
            "    Balance: " << format_amount (get_funds (account, offer.in));
    }
}

void
Taker::consume_offer (Offer& offer, Amounts const& order)
{
    if (order.in < beast::zero)
        Throw<std::logic_error> ("flow with negative input.");

    if (order.out < beast::zero)
        Throw<std::logic_error> ("flow with negative output.");

    JLOG(journal_.debug()) << "Consuming from offer " << offer;

    if (auto stream = journal_.trace())
    {
        auto const& available = offer.amount ();

        stream << "   in:" << format_amount (available.in);
        stream << "  out:" << format_amount(available.out);
    }

    offer.consume (view_, order);
}

STAmount
Taker::get_funds (AccountID const& account, STAmount const& amount) const
{
    return accountFunds(view_, account, amount, fhZERO_IF_FROZEN, journal_);
}

TER Taker::transferXRP (
    AccountID const& from,
    AccountID const& to,
    STAmount const& amount)
{
    if (!isXRP (amount))
        Throw<std::logic_error> ("Using transferXRP with IOU");

    if (from == to)
        return tesSUCCESS;

//转移零等于不转移
    if (amount == beast::zero)
        return tesSUCCESS;

    return ripple::transferXRP (view_, from, to, amount, journal_);
}

TER Taker::redeemIOU (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXRP (amount))
        Throw<std::logic_error> ("Using redeemIOU with XRP");

    if (account == issue.account)
        return tesSUCCESS;

//转移零等于不转移
    if (amount == beast::zero)
        return tesSUCCESS;

//如果我们要赎回一些金额，那么帐户
//必须有贷方余额。
    if (get_funds (account, amount) <= beast::zero)
        Throw<std::logic_error> ("redeemIOU has no funds to redeem");

    auto ret = ripple::redeemIOU (view_, account, amount, issue, journal_);

    if (get_funds (account, amount) < beast::zero)
        Throw<std::logic_error> ("redeemIOU redeemed more funds than available");

    return ret;
}

TER Taker::issueIOU (
    AccountID const& account,
    STAmount const& amount,
    Issue const& issue)
{
    if (isXRP (amount))
        Throw<std::logic_error> ("Using issueIOU with XRP");

    if (account == issue.account)
        return tesSUCCESS;

//转移零等于不转移
    if (amount == beast::zero)
        return tesSUCCESS;

    return ripple::issueIOU (view_, account, amount, issue, journal_);
}

//执行资金转移以满足给定的报价并调整报价。
TER
Taker::fill (BasicTaker::Flow const& flow, Offer& offer)
{
//调整要约
    consume_offer (offer, flow.order);

    TER result = tesSUCCESS;

    if (cross_type () != CrossType::XrpToIou)
    {
        assert (!isXRP (flow.order.in));

        if(result == tesSUCCESS)
            result = redeemIOU (account (), flow.issuers.in, flow.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (offer.owner (), flow.order.in, flow.order.in.issue ());
    }
    else
    {
        assert (isXRP (flow.order.in));

        if (result == tesSUCCESS)
            result = transferXRP (account (), offer.owner (), flow.order.in);
    }

//现在从我们接受报价的账户中汇款
    if (cross_type () != CrossType::IouToXrp)
    {
        assert (!isXRP (flow.order.out));

        if(result == tesSUCCESS)
            result = redeemIOU (offer.owner (), flow.issuers.out, flow.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (account (), flow.order.out, flow.order.out.issue ());
    }
    else
    {
        assert (isXRP (flow.order.out));

        if (result == tesSUCCESS)
            result = transferXRP (offer.owner (), account (), flow.order.out);
    }

    if (result == tesSUCCESS)
        direct_crossings_++;

    return result;
}

//执行桥接资金转移以满足给定的报价并调整报价。
TER
Taker::fill (
    BasicTaker::Flow const& flow1, Offer& leg1,
    BasicTaker::Flow const& flow2, Offer& leg2)
{
//相应调整报价
    consume_offer (leg1, flow1.order);
    consume_offer (leg2, flow2.order);

    TER result = tesSUCCESS;

//获取LEG1:IOU
    if (leg1.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeemIOU (account (), flow1.issuers.in, flow1.issuers.in.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (leg1.owner (), flow1.order.in, flow1.order.in.issue ());
    }

//leg1到leg2:xrp上的桥接
    if (result == tesSUCCESS)
        result = transferXRP (leg1.owner (), leg2.owner (), flow1.order.out);

//LEG2收件人：IOU
    if (leg2.owner () != account ())
    {
        if (result == tesSUCCESS)
            result = redeemIOU (leg2.owner (), flow2.issuers.out, flow2.issuers.out.issue ());

        if (result == tesSUCCESS)
            result = issueIOU (account (), flow2.order.out, flow2.order.out.issue ());
    }

    if (result == tesSUCCESS)
    {
        bridge_crossings_++;
        xrp_flow_ += flow1.order.out;
    }

    return result;
}

TER
Taker::cross (Offer& offer)
{
//在直行交叉口，至少有一条腿不能是xrp。
    if (isXRP (offer.amount ().in) && isXRP (offer.amount ().out))
        return tefINTERNAL;

    auto const amount = do_cross (
        offer.amount (), offer.quality (), offer.owner ());

    return fill (amount, offer);
}

TER
Taker::cross (Offer& leg1, Offer& leg2)
{
//在桥接交叉口中，xrp不能是第一段的输入
//或第二个支腿的输出。
    if (isXRP (leg1.amount ().in) || isXRP (leg2.amount ().out))
        return tefINTERNAL;

    auto ret = do_cross (
        leg1.amount (), leg1.quality (), leg1.owner (),
        leg2.amount (), leg2.quality (), leg2.owner ());

    return fill (ret.first, leg1, ret.second, leg2);
}

Rate
Taker::calculateRate (
    ApplyView const& view,
        AccountID const& issuer,
            AccountID const& account)
{
    return isXRP (issuer) || (account == issuer)
        ? parityRate
        : transferRate (view, issuer);
}

} //涟漪

