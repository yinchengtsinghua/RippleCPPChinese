
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

#ifndef RIPPLE_APP_TX_TRANSACTOR_H_INCLUDED
#define RIPPLE_APP_TX_TRANSACTOR_H_INCLUDED

#include <ripple/app/tx/applySteps.h>
#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>

namespace ripple {

/*预点火时的状态信息。*/
struct PreflightContext
{
public:
    Application& app;
    STTx const& tx;
    Rules const rules;
    ApplyFlags flags;
    beast::Journal j;

    PreflightContext(Application& app_, STTx const& tx_,
        Rules const& rules_, ApplyFlags flags_,
            beast::Journal j_);

    PreflightContext& operator=(PreflightContext const&) = delete;
};

/*确定发送是否可能收取费用时说明信息。*/
struct PreclaimContext
{
public:
    Application& app;
    ReadView const& view;
    TER preflightResult;
    STTx const& tx;
    ApplyFlags flags;
    beast::Journal j;

    PreclaimContext(Application& app_, ReadView const& view_,
        TER preflightResult_, STTx const& tx_,
            ApplyFlags flags_,
            beast::Journal j_ = beast::Journal{beast::Journal::getNullSink()})
        : app(app_)
        , view(view_)
        , preflightResult(preflightResult_)
        , tx(tx_)
        , flags(flags_)
        , j(j_)
    {
    }

    PreclaimContext& operator=(PreclaimContext const&) = delete;
};

struct TxConsequences;
struct PreflightResult;

class Transactor
{
protected:
    ApplyContext& ctx_;
    beast::Journal j_;

    AccountID     account_;
XRPAmount     mPriorBalance;  //费用前余额。
XRPAmount     mSourceBalance; //费用后余额。

    virtual ~Transactor() = default;
    Transactor (Transactor const&) = delete;
    Transactor& operator= (Transactor const&) = delete;

public:
    /*处理事务。*/
    std::pair<TER, bool>
    operator()();

    ApplyView&
    view()
    {
        return ctx_.view();
    }

    ApplyView const&
    view() const
    {
        return ctx_.view();
    }

/////////////////////////////////////////
    /*
    这些静态函数是从invoke_preclaim<tx>
    使用名称隐藏来完成编译时多态性，
    因此派生类可以为不同的或额外的
    功能。小心使用，因为这些不是真的
    所以没有编译器时间保护
    随之而来。
    **/


    static
    NotTEC
    checkSeq (PreclaimContext const& ctx);

    static
    TER
    checkFee (PreclaimContext const& ctx, std::uint64_t baseFee);

    static
    NotTEC
    checkSign (PreclaimContext const& ctx);

//以费用单位返回费用，而不是按负荷缩放。
    static
    std::uint64_t
    calculateBaseFee (
        ReadView const& view,
        STTx const& tx);

    static
    bool
    affectsSubsequentTransactionAuth(STTx const& tx)
    {
        return false;
    }

    static
    XRPAmount
    calculateFeePaid(STTx const& tx);

    static
    XRPAmount
    calculateMaxSpend(STTx const& tx);

    static
    TER
    preclaim(PreclaimContext const &ctx)
    {
//大多数交易人什么都不做
//在检查完Seq/费用/签名后。
        return tesSUCCESS;
    }
/////////////////////////////////////////

protected:
    TER
    apply();

    explicit
    Transactor (ApplyContext& ctx);

    virtual void preCompute();

    virtual TER doApply () = 0;

    /*计算处理交易所需的最低费用
        基于当前服务器负载的给定基本费用。

        @param app托管服务器的应用程序
        @param base fee候选事务的基本费用
            @见Ripple:：CalculateBaseFee
        @param当前分类账的费用设置
        @param标志事务处理费用
     **/

    static
    XRPAmount
    minimumFee (Application& app, std::uint64_t baseFee,
        Fees const& fees, ApplyFlags flags);

private:
    XRPAmount reset(XRPAmount fee);

    void setSeq ();
    TER payFee ();
    static NotTEC checkSingleSign (PreclaimContext const& ctx);
    static NotTEC checkMultiSign (PreclaimContext const& ctx);
};

/*对txid执行早期健全性检查*/
NotTEC
preflight0(PreflightContext const& ctx);

/*对帐户和费用字段执行早期健全性检查*/
NotTEC
preflight1 (PreflightContext const& ctx);

/*检查签名是否有效*/
NotTEC
preflight2 (PreflightContext const& ctx);

}

#endif
