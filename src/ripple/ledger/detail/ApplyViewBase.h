
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

#ifndef RIPPLE_LEDGER_APPLYVIEWBASE_H_INCLUDED
#define RIPPLE_LEDGER_APPLYVIEWBASE_H_INCLUDED

#include <ripple/ledger/ApplyView.h>
#include <ripple/ledger/CashDiff.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/ledger/detail/ApplyStateTable.h>
#include <ripple/protocol/XRPAmount.h>

namespace ripple {
namespace detail {

class ApplyViewBase
    : public ApplyView
    , public RawView
{
public:
    ApplyViewBase() = delete;
    ApplyViewBase (ApplyViewBase const&) = delete;
    ApplyViewBase& operator= (ApplyViewBase&&) = delete;
    ApplyViewBase& operator= (ApplyViewBase const&) = delete;


    ApplyViewBase (ApplyViewBase&&) = default;

    ApplyViewBase(
        ReadView const* base, ApplyFlags flags);

//读取视图
    bool
    open() const override;

    LedgerInfo const&
    info() const override;

    Fees const&
    fees() const override;

    Rules const&
    rules() const override;

    bool
    exists (Keylet const& k) const override;

    boost::optional<key_type>
    succ (key_type const& key, boost::optional<
        key_type> const& last = boost::none) const override;

    std::shared_ptr<SLE const>
    read (Keylet const& k) const override;

    std::unique_ptr<sles_type::iter_base>
    slesBegin() const override;

    std::unique_ptr<sles_type::iter_base>
    slesEnd() const override;

    std::unique_ptr<sles_type::iter_base>
    slesUpperBound(uint256 const& key) const override;

    std::unique_ptr<txs_type::iter_base>
    txsBegin() const override;

    std::unique_ptr<txs_type::iter_base>
    txsEnd() const override;

    bool
    txExists (key_type const& key) const override;

    tx_type
    txRead (key_type const& key) const override;

//应用程序视图

    ApplyFlags
    flags() const override;

    std::shared_ptr<SLE>
    peek (Keylet const& k) override;

    void
    erase (std::shared_ptr<
        SLE> const& sle) override;

    void
    insert (std::shared_ptr<
        SLE> const& sle) override;

    void
    update (std::shared_ptr<
        SLE> const& sle) override;

//罗维尤

    void
    rawErase (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawInsert (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawReplace (std::shared_ptr<
        SLE> const& sle) override;

    void
    rawDestroyXRP (
        XRPAmount const& feeDrops) override;

    friend
    CashDiff cashFlowDiff (
        CashFilter lhsFilter, ApplyViewBase const& lhs,
        CashFilter rhsFilter, ApplyViewBase const& rhs);

protected:
    ApplyFlags flags_;
    ReadView const* base_;
    detail::ApplyStateTable items_;
};

} //细节
} //涟漪

#endif
