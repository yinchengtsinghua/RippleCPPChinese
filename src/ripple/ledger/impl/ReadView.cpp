
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

#include <ripple/ledger/ReadView.h>
#include <boost/optional.hpp>

namespace ripple {

class Rules::Impl
{
private:
    std::unordered_set<uint256,
        hardened_hash<>> set_;
    boost::optional<uint256> digest_;
    std::unordered_set<uint256, beast::uhash<>> const& presets_;

public:
    explicit Impl(
            std::unordered_set<uint256, beast::uhash<>> const& presets)
        : presets_(presets)
    {
    }

    explicit Impl(
        DigestAwareReadView const& ledger,
            std::unordered_set<uint256, beast::uhash<>> const& presets)
        : presets_(presets)
    {
        auto const k = keylet::amendments();
        digest_ = ledger.digest(k.key);
        if (! digest_)
            return;
        auto const sle = ledger.read(k);
        if (! sle)
        {
//逻辑错误（）？
            return;
        }

        for (auto const& item :
                sle->getFieldV256(sfAmendments))
            set_.insert(item);
    }

    bool
    enabled (uint256 const& feature) const
    {
        if (presets_.count(feature) > 0)
            return true;
        return set_.count(feature) > 0;
    }

    bool
    changed (DigestAwareReadView const& ledger) const
    {
        auto const digest =
            ledger.digest(keylet::amendments().key);
        if (! digest && ! digest_)
            return false;
        if (! digest || ! digest_)
            return true;
        return *digest != *digest_;
    }

    bool
    operator== (Impl const& other) const
    {
        if (! digest_ && ! other.digest_)
            return true;
        if (! digest_ || ! other.digest_)
            return false;
        return *digest_ == *other.digest_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Rules::Rules(
    DigestAwareReadView const& ledger,
        std::unordered_set<uint256, beast::uhash<>> const& presets)
    : impl_(std::make_shared<Impl>(ledger, presets))
{
}

Rules::Rules(std::unordered_set<uint256, beast::uhash<>> const& presets)
    : impl_(std::make_shared<Impl>(presets))
{
}

bool
Rules::enabled (uint256 const& id) const
{
    assert (impl_);
    return impl_->enabled(id);
}

bool
Rules::changed (DigestAwareReadView const& ledger) const
{
    assert (impl_);
    return impl_->changed(ledger);
}

bool
Rules::operator== (Rules const& other) const
{
    assert(impl_ && other.impl_);
    if (impl_.get() == other.impl_.get())
        return true;
    return *impl_ == *other.impl_;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

ReadView::sles_type::sles_type(
        ReadView const& view)
    : ReadViewFwdRange(view)
{
}

auto
ReadView::sles_type::begin() const ->
    iterator
{
    return iterator(view_, view_->slesBegin());
}

auto
ReadView::sles_type::end() const ->
    iterator const&
{
    if (! end_)
        end_ = iterator(view_, view_->slesEnd());
    return *end_;
}

auto
ReadView::sles_type::upper_bound(key_type const& key) const ->
    iterator
{
    return iterator(view_, view_->slesUpperBound(key));
}

ReadView::txs_type::txs_type(
        ReadView const& view)
    : ReadViewFwdRange(view)
{
}

bool
ReadView::txs_type::empty() const
{
    return begin() == end();
}

auto
ReadView::txs_type::begin() const ->
    iterator
{
    return iterator(view_, view_->txsBegin());
}

auto
ReadView::txs_type::end() const ->
    iterator const&
{
    if (! end_)
        end_ = iterator(view_, view_->txsEnd());
    return *end_;
}

} //涟漪
