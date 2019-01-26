
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

#include <ripple/ledger/detail/ApplyViewBase.h>
#include <ripple/basics/contract.h>
#include <ripple/ledger/CashDiff.h>

namespace ripple {
namespace detail {

ApplyViewBase::ApplyViewBase(
    ReadView const* base, ApplyFlags flags)
    : flags_ (flags)
    , base_ (base)
{
}

//---

bool
ApplyViewBase::open() const
{
    return base_->open();
}

LedgerInfo const&
ApplyViewBase::info() const
{
    return base_->info();
}

Fees const&
ApplyViewBase::fees() const
{
    return base_->fees();
}

Rules const&
ApplyViewBase::rules() const
{
    return base_->rules();
}

bool
ApplyViewBase::exists (Keylet const& k) const
{
    return items_.exists(*base_, k);
}

auto
ApplyViewBase::succ (key_type const& key,
    boost::optional<key_type> const& last) const ->
        boost::optional<key_type>
{
    return items_.succ(*base_, key, last);
}

std::shared_ptr<SLE const>
ApplyViewBase::read (Keylet const& k) const
{
    return items_.read(*base_, k);
}

auto
ApplyViewBase::slesBegin() const ->
    std::unique_ptr<sles_type::iter_base>
{
    return base_->slesBegin();
}

auto
ApplyViewBase::slesEnd() const ->
    std::unique_ptr<sles_type::iter_base>
{
    return base_->slesEnd();
}

auto
ApplyViewBase::slesUpperBound(uint256 const& key) const ->
    std::unique_ptr<sles_type::iter_base>
{
    return base_->slesUpperBound(key);
}

auto
ApplyViewBase::txsBegin() const ->
    std::unique_ptr<txs_type::iter_base>
{
    return base_->txsBegin();
}

auto
ApplyViewBase::txsEnd() const ->
    std::unique_ptr<txs_type::iter_base>
{
    return base_->txsEnd();
}

bool
ApplyViewBase::txExists (key_type const& key) const
{
    return base_->txExists(key);
}

auto
ApplyViewBase::txRead(
    key_type const& key) const ->
        tx_type
{
    return base_->txRead(key);
}

//---

ApplyFlags
ApplyViewBase::flags() const
{
    return flags_;
}

std::shared_ptr<SLE>
ApplyViewBase::peek (Keylet const& k)
{
    return items_.peek(*base_, k);
}

void
ApplyViewBase::erase(
    std::shared_ptr<SLE> const& sle)
{
    items_.erase(*base_, sle);
}

void
ApplyViewBase::insert(
    std::shared_ptr<SLE> const& sle)
{
    items_.insert(*base_, sle);
}

void
ApplyViewBase::update(
    std::shared_ptr<SLE> const& sle)
{
    items_.update(*base_, sle);
}

//---

void
ApplyViewBase::rawErase(
    std::shared_ptr<SLE> const& sle)
{
    items_.rawErase(*base_, sle);
}

void
ApplyViewBase::rawInsert(
    std::shared_ptr<SLE> const& sle)
{
    items_.insert(*base_, sle);
}

void
ApplyViewBase::rawReplace(
    std::shared_ptr<SLE> const& sle)
{
    items_.replace(*base_, sle);
}

void
ApplyViewBase::rawDestroyXRP(
    XRPAmount const& fee)
{
    items_.destroyXRP(fee);
}

//---

CashDiff cashFlowDiff (
    CashFilter lhsFilter, ApplyViewBase const& lhs,
    CashFilter rhsFilter, ApplyViewBase const& rhs)
{
    assert (lhs.base_ == rhs.base_);
    return CashDiff {
        *lhs.base_, lhsFilter, lhs.items_, rhsFilter, rhs.items_ };
}

} //细节
} //涟漪
