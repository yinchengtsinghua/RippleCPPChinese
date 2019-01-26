
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

#include <ripple/app/tx/impl/ApplyContext.h>
#include <ripple/app/tx/impl/InvariantCheck.h>
#include <ripple/app/tx/impl/Transactor.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/Feature.h>
#include <cassert>

namespace ripple {

ApplyContext::ApplyContext(Application& app_,
    OpenView& base, STTx const& tx_, TER preclaimResult_,
        std::uint64_t baseFee_, ApplyFlags flags,
            beast::Journal journal_)
    : app(app_)
    , tx(tx_)
    , preclaimResult(preclaimResult_)
    , baseFee(baseFee_)
    , journal(journal_)
    , base_ (base)
    , flags_(flags)
{
    view_.emplace(&base_, flags_);
}

void
ApplyContext::discard()
{
    view_.emplace(&base_, flags_);
}

void
ApplyContext::apply(TER ter)
{
    view_->apply(base_, tx, ter, journal);
}

std::size_t
ApplyContext::size()
{
    return view_->size();
}

void
ApplyContext::visit (std::function <void (
    uint256 const&, bool,
    std::shared_ptr<SLE const> const&,
    std::shared_ptr<SLE const> const&)> const& func)
{
    view_->visit(base_, func);
}

TER
ApplyContext::failInvariantCheck (TER const result)
{
//如果我们之前已经失败了不变量检查，现在正在尝试
//只收取一笔费用，即使不能通过不变检查
//非常错误。我们切换到tefinvariant_失败，不包括
//在分类帐中。

    return (result == tecINVARIANT_FAILED || result == tefINVARIANT_FAILED)
        ? TER{tefINVARIANT_FAILED}
        : TER{tecINVARIANT_FAILED};
}

template<std::size_t... Is>
TER
ApplyContext::checkInvariantsHelper(
    TER const result,
    XRPAmount const fee,
    std::index_sequence<Is...>)
{
    if (view_->rules().enabled(featureEnforceInvariants))
    {
        auto checkers = getInvariantChecks();
        try
        {
//调用每个支票的每个条目方法
            visit (
                [&checkers](
                    uint256 const& index,
                    bool isDelete,
                    std::shared_ptr <SLE const> const& before,
                    std::shared_ptr <SLE const> const& after)
                {
//肖恩的父母为你的每一个争论把戏
                    (void)std::array<int, sizeof...(Is)>{
                        {((std::get<Is>(checkers).
                            visitEntry(index, isDelete, before, after)), 0)...}
                    };
                });

//肖恩的父母为你的每一个辩论技巧（一个折叠表达与&&`
//当我们移动到C++ 17时，这里会很好
            std::array<bool, sizeof...(Is)> finalizers {{
                std::get<Is>(checkers).finalize(tx, result, fee, journal)...}};

//调用每个支票的终结器以查看它是否通过
            if (! std::all_of( finalizers.cbegin(), finalizers.cend(),
                    [](auto const& b) { return b; }))
            {
                JLOG(journal.fatal()) <<
                    "Transaction has failed one or more invariants: " <<
                    to_string(tx.getJson (0));

                return failInvariantCheck (result);
            }
        }
        catch(std::exception const& ex)
        {
            JLOG(journal.fatal()) <<
                "Transaction caused an exception in an invariant" <<
                ", ex: " << ex.what() <<
                ", tx: " << to_string(tx.getJson (0));

            return failInvariantCheck (result);
        }
    }

    return result;
}

TER
ApplyContext::checkInvariants(TER const result, XRPAmount const fee)
{
    assert (isTesSuccess(result) || isTecClaim(result));

    return checkInvariantsHelper(result, fee,
        std::make_index_sequence<std::tuple_size<InvariantChecks>::value>{});
}

} //涟漪
