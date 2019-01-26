
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

#include <ripple/ledger/detail/RawStateTable.h>
#include <ripple/basics/contract.h>

namespace ripple {
namespace detail {

class RawStateTable::sles_iter_impl
    : public ReadView::sles_type::iter_base
{
private:
    std::shared_ptr<SLE const> sle0_;
    ReadView::sles_type::iterator iter0_;
    ReadView::sles_type::iterator end0_;
    std::shared_ptr<SLE const> sle1_;
    items_t::const_iterator iter1_;
    items_t::const_iterator end1_;

public:
    sles_iter_impl (sles_iter_impl const&) = default;

    sles_iter_impl (items_t::const_iterator iter1,
        items_t::const_iterator end1,
            ReadView::sles_type::iterator iter0,
                ReadView::sles_type::iterator end0)
        : iter0_ (iter0)
        , end0_ (end0)
        , iter1_ (iter1)
        , end1_ (end1)
    {
        if (iter0_ != end0_)
            sle0_ = *iter0_;
        if (iter1_ != end1)
        {
            sle1_ = iter1_->second.second;
            skip ();
        }
    }

    std::unique_ptr<base_type>
    copy() const override
    {
        return std::make_unique<sles_iter_impl>(*this);
    }

    bool
    equal (base_type const& impl) const override
    {
        auto const& other = dynamic_cast<
            sles_iter_impl const&>(impl);
        assert(end1_ == other.end1_ &&
            end0_ == other.end0_);
        return iter1_ == other.iter1_ &&
            iter0_ == other.iter0_;
    }

    void
    increment() override
    {
        assert (sle1_ || sle0_);

        if (sle1_ && !sle0_)
        {
            inc1();
            return;
        }

        if (sle0_ && !sle1_)
        {
            inc0();
            return;
        }

        if (sle1_->key () == sle0_->key())
        {
            inc1();
            inc0();
        }
        else if (sle1_->key () < sle0_->key())
        {
            inc1();
        }
        else
        {
            inc0();
        }
        skip();
    }

    value_type
    dereference() const override
    {
        if (! sle1_)
            return sle0_;
        else if (! sle0_)
            return sle1_;
        if (sle1_->key() <= sle0_->key())
            return sle1_;
        return sle0_;
    }
private:
    void inc0()
    {
        ++iter0_;
        if (iter0_ == end0_)
            sle0_ = nullptr;
        else
            sle0_ = *iter0_;
    }
    
    void inc1()
    {
        ++iter1_;
        if (iter1_ == end1_)
            sle1_ = nullptr;
        else
            sle1_ = iter1_->second.second;
    }
    
    void skip()
    {
        while (iter1_ != end1_ &&
            iter1_->second.first == Action::erase &&
               sle0_->key() == sle1_->key())
        {
            inc1();
            inc0();
            if (! sle0_)
                return;
        }
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//基不变量由基在apply（）期间检查

void
RawStateTable::apply (RawView& to) const
{
    to.rawDestroyXRP(dropsDestroyed_);
    for (auto const& elem : items_)
    {
        auto const& item = elem.second;
        switch(item.first)
        {
        case Action::erase:
            to.rawErase(item.second);
            break;
        case Action::insert:
            to.rawInsert(item.second);
            break;
        case Action::replace:
            to.rawReplace(item.second);
            break;
        }
    }
}

bool
RawStateTable::exists (ReadView const& base,
    Keylet const& k) const
{
    assert(k.key.isNonZero());
    auto const iter = items_.find(k.key);
    if (iter == items_.end())
        return base.exists(k);
    auto const& item = iter->second;
    if (item.first == Action::erase)
        return false;
    if (! k.check(*item.second))
        return false;
    return true;
}

/*这是通过首先计算父级的suc（）来实现的，
    然后计算我们的内部列表，并
    两者中的较低者。
**/

auto
RawStateTable::succ (ReadView const& base,
    key_type const& key, boost::optional<
        key_type> const& last) const ->
            boost::optional<key_type>
{
    boost::optional<key_type> next = key;
    items_t::const_iterator iter;
//查找基本后续任务
//未在我们的列表中删除
    do
    {
        next = base.succ(*next, last);
        if (! next)
            break;
        iter = items_.find(*next);
    }
    while (iter != items_.end() &&
        iter->second.first == Action::erase);
//在列表中查找未删除的继任人
    for (iter = items_.upper_bound(key);
        iter != items_.end (); ++iter)
    {
        if (iter->second.first != Action::erase)
        {
//找到两者，返回下键
            if (! next || next > iter->first)
                next = iter->first;
            break;
        }
    }
//我们的单子里没有，回来
//我们从父母那里得到的。
    if (last && next >= last)
        return boost::none;
    return next;
}

void
RawStateTable::erase(
    std::shared_ptr<SLE> const& sle)
{
//在应用期间检查基不变量
    auto const result = items_.emplace(
        std::piecewise_construct,
            std::forward_as_tuple(sle->key()),
                std::forward_as_tuple(
                    Action::erase, sle));
    if (result.second)
        return;
    auto& item = result.first->second;
    switch(item.first)
    {
    case Action::erase:
        LogicError("RawStateTable::erase: already erased");
        break;
    case Action::insert:
        items_.erase(result.first);
        break;
    case Action::replace:
        item.first = Action::erase;
        item.second = sle;
        break;
    }
}

void
RawStateTable::insert(
    std::shared_ptr<SLE> const& sle)
{
    auto const result = items_.emplace(
        std::piecewise_construct,
            std::forward_as_tuple(sle->key()),
                std::forward_as_tuple(
                    Action::insert, sle));
    if (result.second)
        return;
    auto& item = result.first->second;
    switch(item.first)
    {
    case Action::erase:
        item.first = Action::replace;
        item.second = sle;
        break;
    case Action::insert:
        LogicError("RawStateTable::insert: already inserted");
        break;
    case Action::replace:
        LogicError("RawStateTable::insert: already exists");
        break;
    }
}

void
RawStateTable::replace(
    std::shared_ptr<SLE> const& sle)
{
    auto const result = items_.emplace(
        std::piecewise_construct,
            std::forward_as_tuple(sle->key()),
                std::forward_as_tuple(
                    Action::replace, sle));
    if (result.second)
        return;
    auto& item = result.first->second;
    switch(item.first)
    {
    case Action::erase:
        LogicError("RawStateTable::replace: was erased");
        break;
    case Action::insert:
    case Action::replace:
        item.second = sle;
        break;
    }
}

std::shared_ptr<SLE const>
RawStateTable::read (ReadView const& base,
    Keylet const& k) const
{
    auto const iter =
        items_.find(k.key);
    if (iter == items_.end())
        return base.read(k);
    auto const& item = iter->second;
    if (item.first == Action::erase)
        return nullptr;
//转换为SLE常量
    std::shared_ptr<
        SLE const> sle = item.second;
    if (! k.check(*sle))
        return nullptr;
    return sle;
}

void
RawStateTable::destroyXRP(XRPAmount const& fee)
{
    dropsDestroyed_ += fee;
}

std::unique_ptr<ReadView::sles_type::iter_base>
RawStateTable::slesBegin (ReadView const& base) const
{
    return std::make_unique<sles_iter_impl>(
        items_.begin(), items_.end(),
            base.sles.begin(), base.sles.end());
}

std::unique_ptr<ReadView::sles_type::iter_base>
RawStateTable::slesEnd (ReadView const& base) const
{
    return std::make_unique<sles_iter_impl>(
        items_.end(), items_.end(),
            base.sles.end(), base.sles.end());
}

std::unique_ptr<ReadView::sles_type::iter_base>
RawStateTable::slesUpperBound (ReadView const& base, uint256 const& key) const
{
    return std::make_unique<sles_iter_impl>(
            items_.upper_bound(key), items_.end(),
                base.sles.upper_bound(key), base.sles.end());
}

} //细节
} //涟漪
