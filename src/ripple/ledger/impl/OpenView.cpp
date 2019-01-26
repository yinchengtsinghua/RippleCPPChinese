
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

#include <ripple/ledger/OpenView.h>
#include <ripple/basics/contract.h>

namespace ripple {

open_ledger_t const open_ledger {};

class OpenView::txs_iter_impl
    : public txs_type::iter_base
{
private:
    bool metadata_;
    txs_map::const_iterator iter_;

public:
    explicit
    txs_iter_impl (bool metadata,
            txs_map::const_iterator iter)
        : metadata_(metadata)
        , iter_(iter)
    {
    }

    std::unique_ptr<base_type>
    copy() const override
    {
        return std::make_unique<
            txs_iter_impl>(
                metadata_, iter_);
    }

    bool
    equal (base_type const& impl) const override
    {
        auto const& other = dynamic_cast<
            txs_iter_impl const&>(impl);
        return iter_ == other.iter_;
    }

    void
    increment() override
    {
        ++iter_;
    }

    value_type
    dereference() const override
    {
        value_type result;
        {
            SerialIter sit(
                iter_->second.first->slice());
            result.first = std::make_shared<
                STTx const>(sit);
        }
        if (metadata_)
        {
            SerialIter sit(
                iter_->second.second->slice());
            result.second = std::make_shared<
                STObject const>(sit, sfMetadata);
        }
        return result;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

OpenView::OpenView (open_ledger_t,
    ReadView const* base, Rules const& rules,
        std::shared_ptr<void const> hold)
    : rules_ (rules)
    , info_ (base->info())
    , base_ (base)
    , hold_ (std::move(hold))
{
    info_.validated = false;
    info_.accepted = false;
    info_.seq = base_->info().seq + 1;
    info_.parentCloseTime = base_->info().closeTime;
    info_.parentHash = base_->info().hash;
}

OpenView::OpenView (ReadView const* base,
        std::shared_ptr<void const> hold)
    : rules_ (base->rules())
    , info_ (base->info())
    , base_ (base)
    , hold_ (std::move(hold))
    , open_ (base->open())
{
}

std::size_t
OpenView::txCount() const
{
    return txs_.size();
}

void
OpenView::apply (TxsRawView& to) const
{
    items_.apply(to);
    for (auto const& item : txs_)
        to.rawTxInsert (item.first,
            item.second.first,
                item.second.second);
}

//---

LedgerInfo const&
OpenView::info() const
{
    return info_;
}

Fees const&
OpenView::fees() const
{
    return base_->fees();
}

Rules const&
OpenView::rules() const
{
    return rules_;
}

bool
OpenView::exists (Keylet const& k) const
{
    return items_.exists(*base_, k);
}

auto
OpenView::succ (key_type const& key,
    boost::optional<key_type> const& last) const ->
        boost::optional<key_type>
{
    return items_.succ(*base_, key, last);
}

std::shared_ptr<SLE const>
OpenView::read (Keylet const& k) const
{
    return items_.read(*base_, k);
}

auto
OpenView::slesBegin() const ->
    std::unique_ptr<sles_type::iter_base>
{
    return items_.slesBegin(*base_);
}

auto
OpenView::slesEnd() const ->
    std::unique_ptr<sles_type::iter_base>
{
    return items_.slesEnd(*base_);
}

auto
OpenView::slesUpperBound(uint256 const& key) const ->
    std::unique_ptr<sles_type::iter_base>
{
    return items_.slesUpperBound(*base_, key);
}

auto
OpenView::txsBegin() const ->
    std::unique_ptr<txs_type::iter_base>
{
    return std::make_unique<txs_iter_impl>(
        !open(), txs_.cbegin());
}

auto
OpenView::txsEnd() const ->
    std::unique_ptr<txs_type::iter_base>
{
    return std::make_unique<txs_iter_impl>(
        !open(), txs_.cend());
}

bool
OpenView::txExists (key_type const& key) const
{
    return txs_.find(key) != txs_.end();
}

auto
OpenView::txRead (key_type const& key) const ->
    tx_type
{
    auto const iter = txs_.find(key);
    if (iter == txs_.end())
        return base_->txRead(key);
    auto const& item = iter->second;
    auto stx = std::make_shared<STTx const
        >(SerialIter{ item.first->slice() });
    decltype(tx_type::second) sto;
    if (item.second)
        sto = std::make_shared<STObject const>(
                SerialIter{ item.second->slice() },
                    sfMetadata);
    else
        sto = nullptr;
    return { std::move(stx), std::move(sto) };
}

//---

void
OpenView::rawErase(
    std::shared_ptr<SLE> const& sle)
{
    items_.erase(sle);
}

void
OpenView::rawInsert(
    std::shared_ptr<SLE> const& sle)
{
    items_.insert(sle);
}

void
OpenView::rawReplace(
    std::shared_ptr<SLE> const& sle)
{
    items_.replace(sle);
}

void
OpenView::rawDestroyXRP(
    XRPAmount const& fee)
{
    items_.destroyXRP(fee);
//vfalc从信息中扣除？totaldrops？
//孩子的看法呢？
}

//---

void
OpenView::rawTxInsert (key_type const& key,
    std::shared_ptr<Serializer const>
        const& txn, std::shared_ptr<
            Serializer const>
                const& metaData)
{
    auto const result = txs_.emplace (key,
        std::make_pair(txn, metaData));
    if (! result.second)
        LogicError("rawTxInsert: duplicate TX id" +
            to_string(key));
}

} //涟漪
