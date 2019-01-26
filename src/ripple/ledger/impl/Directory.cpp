
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2012，2015 Ripple Labs Inc.

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

#include <ripple/ledger/Directory.h>

namespace ripple {

using const_iterator = Dir::const_iterator;

Dir::Dir(ReadView const& view,
        Keylet const& key)
    : view_(&view)
    , root_(key)
    , sle_(view_->read(root_))
{
    if (sle_ != nullptr)
        indexes_ = &sle_->getFieldV256(sfIndexes);
}

auto
Dir::begin() const ->
    const_iterator
{
    auto it = const_iterator(*view_, root_, root_);
    if (sle_ != nullptr)
    {
        it.sle_ = sle_;
        if (! indexes_->empty())
        {
            it.indexes_ = indexes_;
            it.it_ = std::begin(*indexes_);
            it.index_ = *it.it_;
        }
    }

    return it;
}

auto
Dir::end() const  ->
    const_iterator
{
    return const_iterator(*view_, root_, root_);
}

bool
const_iterator::operator==(const_iterator const& other) const
{
    if (view_ == nullptr || other.view_ == nullptr)
        return false;

    assert(view_ == other.view_ && root_.key == other.root_.key);
    return page_.key == other.page_.key && index_ == other.index_;
}

const_iterator::reference
const_iterator::operator*() const
{
    assert(index_ != beast::zero);
    if (! cache_)
        cache_ = view_->read(keylet::child(index_));
    return *cache_;
}

const_iterator&
const_iterator::operator++()
{
    assert(index_ != beast::zero);
    if (++it_ != std::end(*indexes_))
    {
        index_ = *it_;
    }
    else
    {
        auto const next =
            sle_->getFieldU64(sfIndexNext);
        if (next == 0)
        {
            page_.key = root_.key;
            index_ = beast::zero;
        }
        else
        {
            page_ = keylet::page(root_, next);
            sle_ = view_->read(page_);
            assert(sle_);
            indexes_ = &sle_->getFieldV256(sfIndexes);
            if (indexes_->empty())
            {
                index_ = beast::zero;
            }
            else
            {
                it_ = std::begin(*indexes_);
                index_ = *it_;
            }
        }
    }

    cache_ = boost::none;
    return *this;
}

const_iterator
const_iterator::operator++(int)
{
    assert(index_ != beast::zero);
    const_iterator tmp(*this);
    ++(*this);
    return tmp;
}

} //涟漪
