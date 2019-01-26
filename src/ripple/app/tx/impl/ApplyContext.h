
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

#ifndef RIPPLE_TX_APPLYCONTEXT_H_INCLUDED
#define RIPPLE_TX_APPLYCONTEXT_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/core/Config.h>
#include <ripple/protocol/STTx.h>
#include <ripple/protocol/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/optional.hpp>
#include <utility>

namespace ripple {

//TXYNEABLE测试

/*应用Tx时的状态信息。*/
class ApplyContext
{
public:
    explicit
    ApplyContext (Application& app, OpenView& base,
        STTx const& tx, TER preclaimResult,
            std::uint64_t baseFee, ApplyFlags flags,
                beast::Journal = beast::Journal{beast::Journal::getNullSink()});

    Application& app;
    STTx const& tx;
    TER const preclaimResult;
    std::uint64_t const baseFee;
    beast::Journal const journal;

    ApplyView&
    view()
    {
        return *view_;
    }

    ApplyView const&
    view() const
    {
        return *view_;
    }

//很遗憾，这是必要的
    RawView&
    rawView()
    {
        return *view_;
    }

    /*设置元数据中的DeliveredAmount字段*/
    void
    deliver (STAmount const& amount)
    {
        view_->deliver(amount);
    }

    /*放弃更改并重新开始。*/
    void
    discard();

    /*将事务结果应用于基。*/
    void
    apply (TER);

    /*获取未应用的更改数。*/
    std::size_t
    size ();

    /*访问未应用的更改。*/
    void
    visit (std::function <void (
        uint256 const& key,
        bool isDelete,
        std::shared_ptr <SLE const> const& before,
        std::shared_ptr <SLE const> const& after)> const& func);

    void
    destroyXRP (XRPAmount const& fee)
    {
        view_->rawDestroyXRP(fee);
    }

    /*逐个应用所有不变的检查程序。

        @param result处理此事务生成的结果。
        @param fee本次交易收取的费用
        @返回此事务应返回的结果代码。
     **/

    TER
    checkInvariants(TER const result, XRPAmount const fee);

private:
    TER
    failInvariantCheck (TER const result);

    template<std::size_t... Is>
    TER
    checkInvariantsHelper(TER const result, XRPAmount const fee, std::index_sequence<Is...>);

    OpenView& base_;
    ApplyFlags flags_;
    boost::optional<ApplyViewImpl> view_;
};

} //涟漪

#endif
