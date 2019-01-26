
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

#ifndef RIPPLE_APP_BOOK_TAKER_H_INCLUDED
#define RIPPLE_APP_BOOK_TAKER_H_INCLUDED

#include <ripple/app/tx/impl/Offer.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Quality.h>
#include <ripple/protocol/Rate.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>

namespace ripple {

/*报价交叉的味道*/
enum class CrossType
{
    XrpToIou,
    IouToXrp,
    IouToIou
};

/*订单簿或付款操作期间活动方的状态。*/
class BasicTaker
{
private:
    AccountID account_;
    Quality quality_;
    Quality threshold_;

    bool sell_;

//原始进出数量。
    Amounts const original_;

//剩下的钱我们还可以试着拿走。
    Amounts remaining_;

//输入和输出的发行者
    Issue const& issue_in_;
    Issue const& issue_out_;

//当输入和输出货币
//已转让且不涉及货币发行人：
    Rate const m_rate_in;
    Rate const m_rate_out;

//我们正在执行的交叉类型
    CrossType cross_type_;

protected:
    beast::Journal journal_;

    struct Flow
    {
        explicit Flow() = default;

        Amounts order;
        Amounts issuers;

        bool sanity_check () const
        {
            using beast::zero;

            if (isXRP (order.in) && isXRP (order.out))
                return false;

            return order.in >= zero &&
                order.out >= zero &&
                issuers.in >= zero &&
                issuers.out >= zero;
        }
    };

private:
    void
    log_flow (char const* description, Flow const& flow);

    Flow
    flow_xrp_to_iou (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_out);

    Flow
    flow_iou_to_xrp (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_in);

    Flow
    flow_iou_to_iou (Amounts const& offer, Quality quality,
        STAmount const& owner_funds, STAmount const& taker_funds,
        Rate const& rate_in, Rate const& rate_out);

//计算计算时应使用的转移率
//两个帐户之间的特定问题流。
    static
    Rate
    effective_rate (Rate const& rate, Issue const &issue,
        AccountID const& from, AccountID const& to);

//指定帐户之间输入货币的转移率
    Rate
    in_rate (AccountID const& from, AccountID const& to) const
    {
        return effective_rate (m_rate_in, original_.in.issue (), from, to);
    }

//指定帐户之间输出货币的转移率
    Rate
    out_rate (AccountID const& from, AccountID const& to) const
    {
        return effective_rate (m_rate_out, original_.out.issue (), from, to);
    }

public:
    BasicTaker () = delete;
    BasicTaker (BasicTaker const&) = delete;

    BasicTaker (
        CrossType cross_type, AccountID const& account, Amounts const& amount,
        Quality const& quality, std::uint32_t flags, Rate const& rate_in,
        Rate const& rate_out,
        beast::Journal journal = beast::Journal{beast::Journal::getNullSink()});

    virtual ~BasicTaker () = default;

    /*返回报价中剩余的金额。
        这是报价的金额。它也可以
        在没有交叉报价时全额支付，或为零。
        报价完全交叉时，或两者之间的任何金额。
        始终保持原报价质量（质量）
    **/

    Amounts
    remaining_offer () const;

    /*返回最初报价的金额。*/
    Amounts const&
    original_offer () const;

    /*返回接受方的帐户标识符。*/
    AccountID const&
    account () const noexcept
    {
        return account_;
    }

    /*如果质量不符合接受方的要求，则返回“true”。*/
    bool
    reject (Quality const& quality) const noexcept
    {
        return quality < threshold_;
    }

    /*返回正在执行的交叉类型*/
    CrossType
    cross_type () const
    {
        return cross_type_;
    }

    /*返回与报价输入相关的问题*/
    Issue const&
    issue_in () const
    {
        return issue_in_;
    }

    /*返回与报价输出相关的问题*/
    Issue const&
    issue_out () const
    {
        return issue_out_;
    }

    /*如果接受方资金不足，则返回“true”。*/
    bool
    unfunded () const;

    /*如果顺序交叉不应继续，则返回“true”。
        如果接受方的订单数量
        已联系，或者如果接受方的投入资金已用完。
    **/

    bool
    done () const;

    /*直接穿过给定的报价。
        @返回描述交叉过程中实现的流的“数量”
    **/

    BasicTaker::Flow
    do_cross (Amounts offer, Quality quality, AccountID const& owner);

    /*通过给定的报价执行桥接交叉。
        @返回一对描述交叉过程中实现的流的“数量”
    **/

    std::pair<BasicTaker::Flow, BasicTaker::Flow>
    do_cross (
        Amounts offer1, Quality quality1, AccountID const& owner1,
        Amounts offer2, Quality quality2, AccountID const& owner2);

    virtual
    STAmount
    get_funds (AccountID const& account, STAmount const& funds) const = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class Taker
    : public BasicTaker
{
public:
    Taker () = delete;
    Taker (Taker const&) = delete;

    Taker (CrossType cross_type, ApplyView& view,
        AccountID const& account, Amounts const& offer,
            std::uint32_t flags,
                beast::Journal journal);
    ~Taker () = default;

    void
    consume_offer (Offer& offer, Amounts const& order);

    STAmount
    get_funds (AccountID const& account, STAmount const& funds) const override;

    STAmount const&
    get_xrp_flow () const
    {
        return xrp_flow_;
    }

    std::uint32_t
    get_direct_crossings () const
    {
        return direct_crossings_;
    }

    std::uint32_t
    get_bridge_crossings () const
    {
        return bridge_crossings_;
    }

    /*根据情况进行直接或桥接报价交叉。
        资金将相应转移，报价将进行调整。
        @如果成功，返回tessuccess，否则返回错误代码。
    **/

    /*@ {*/
    TER
    cross (Offer& offer);

    TER
    cross (Offer& leg1, Offer& leg2);
    /*@ }*/

private:
    static
    Rate
    calculateRate (ApplyView const& view,
        AccountID const& issuer,
            AccountID const& account);

    TER
    fill (BasicTaker::Flow const& flow, Offer& offer);

    TER
    fill (
        BasicTaker::Flow const& flow1, Offer& leg1,
        BasicTaker::Flow const& flow2, Offer& leg2);

    TER
    transferXRP (AccountID const& from, AccountID const& to, STAmount const& amount);

    TER
    redeemIOU (AccountID const& account, STAmount const& amount, Issue const& issue);

    TER
    issueIOU (AccountID const& account, STAmount const& amount, Issue const& issue);

private:
//我们正在处理的基本分类账分录
    ApplyView& view_;

//如果我们是自动桥接的话，X射线荧光蛋白的流量
    STAmount xrp_flow_;

//我们直接穿越的次数
    std::uint32_t direct_crossings_;

//我们执行的自动桥接穿越的次数
    std::uint32_t bridge_crossings_;
};

}

#endif
