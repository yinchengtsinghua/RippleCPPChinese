
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

#ifndef RIPPLE_TX_CREATEOFFER_H_INCLUDED
#define RIPPLE_TX_CREATEOFFER_H_INCLUDED

#include <ripple/app/tx/impl/OfferStream.h>
#include <ripple/app/tx/impl/Taker.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <utility>

namespace ripple {

class PaymentSandbox;
class Sandbox;

/*专门用于在分类帐中创建报价的事务处理程序。*/
class CreateOffer
    : public Transactor
{
public:
    /*构造在分类帐中创建报价的事务处理子类。*/
    explicit CreateOffer (ApplyContext& ctx)
        : Transactor(ctx)
        , stepCounter_ (1000, j_)
    {
    }

    /*重写Transactior基类提供的默认行为。*/
    static
    XRPAmount
    calculateMaxSpend(STTx const& tx);

    /*在Transactior基类之外强制约束。*/
    static
    NotTEC
    preflight (PreflightContext const& ctx);

    /*在Transactior基类之外强制约束。*/
    static
    TER
    preclaim(PreclaimContext const& ctx);

    /*收集事务处理程序基类收集的信息以外的信息。*/
    void
    preCompute() override;

    /*前提条件：可能收取费用。尝试创建报价。*/
    TER
    doApply() override;

private:
    std::pair<TER, bool>
    applyGuts (Sandbox& view, Sandbox& view_cancel);

//确定我们是否有权持有我们想要的资产。
    static
    TER
    checkAcceptAsset(ReadView const& view,
        ApplyFlags const flags, AccountID const id,
            beast::Journal const j, Issue const& issue);

    bool
    dry_offer (ApplyView& view, Offer const& offer);

    static
    std::pair<bool, Quality>
    select_path (
        bool have_direct, OfferStream const& direct,
        bool have_bridge, OfferStream const& leg1, OfferStream const& leg2);

    std::pair<TER, Amounts>
    bridged_cross (
        Taker& taker,
        ApplyView& view,
        ApplyView& view_cancel,
        NetClock::time_point const when);

    std::pair<TER, Amounts>
    direct_cross (
        Taker& taker,
        ApplyView& view,
        ApplyView& view_cancel,
        NetClock::time_point const when);

//越走越长越好，跳过任何报价
//这是从接受者或跨越接受者的门槛。
//如果书中没有报价，则返回“假”，否则返回“真”。
    static
    bool
    step_account (OfferStream& stream, Taker const& taker);

//如果交叉的报价数量为真
//超出限制。
    bool
    reachedOfferCrossingLimit (Taker const& taker) const;

//尽可能多地通过消费书上已有的优惠来填充优惠，
//并相应调整账户余额。
//
//向接受者收取费用。
    std::pair<TER, Amounts>
    takerCross (
        Sandbox& sb,
        Sandbox& sbCancel,
        Amounts const& takerAmount);

//使用付款流代码执行报价交叉。
    std::pair<TER, Amounts>
    flowCross (
        PaymentSandbox& psb,
        PaymentSandbox& psbCancel,
        Amounts const& takerAmount);

//暂时的
//这是一个调用两个版本的cross的中心位置
//所以结果可以比较。最终这一层
//一旦确定流量交叉稳定，就将其移除。
    std::pair<TER, Amounts>
    cross (
        Sandbox& sb,
        Sandbox& sbCancel,
        Amounts const& takerAmount);

    static
    std::string
    format_amount (STAmount const& amount);

private:
//我们提供什么样的报价
    CrossType cross_type_;

//跨越时要通过订单簿的步骤数
    OfferStream::StepCounter stepCounter_;
};

}

#endif
