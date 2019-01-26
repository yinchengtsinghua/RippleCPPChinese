
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

#include <ripple/ledger/BookDirs.h>
#include <ripple/ledger/View.h>
#include <ripple/protocol/Indexes.h>

namespace ripple {

BookDirs::BookDirs(ReadView const& view, Book const& book)
    : view_(&view)
    , root_(keylet::page(getBookBase(book)).key)
    , next_quality_(getQualityNext(root_))
    , key_(view_->succ(root_, next_quality_).value_or(beast::zero))
{
    assert(root_ != beast::zero);
    if (key_ != beast::zero)
    {
        if (! cdirFirst(*view_, key_, sle_, entry_, index_,
            beast::Journal {beast::Journal::getNullSink()}))
        {
            assert(false);
        }
    }
}

auto
BookDirs::begin() const ->
    BookDirs::const_iterator
{
    auto it = BookDirs::const_iterator(*view_, root_, key_);
    if (key_ != beast::zero)
    {
        it.next_quality_ = next_quality_;
        it.sle_ = sle_;
        it.entry_ = entry_;
        it.index_ = index_;
    }
    return it;
}

auto
BookDirs::end() const  ->
    BookDirs::const_iterator
{
    return BookDirs::const_iterator(*view_, root_, key_);
}


beast::Journal BookDirs::const_iterator::j_ =
    beast::Journal{beast::Journal::getNullSink()};

bool
BookDirs::const_iterator::operator==
    (BookDirs::const_iterator const& other) const
{
    if (view_ == nullptr || other.view_ == nullptr)
        return false;

    assert(view_ == other.view_ && root_ == other.root_);
    return entry_ == other.entry_ &&
        cur_key_ == other.cur_key_ &&
            index_ == other.index_;
}

BookDirs::const_iterator::reference
BookDirs::const_iterator::operator*() const
{
    assert(index_ != beast::zero);
    if (! cache_)
        cache_ = view_->read(keylet::offer(index_));
    return *cache_;
}

BookDirs::const_iterator&
BookDirs::const_iterator::operator++()
{
    using beast::zero;

    assert(index_ != zero);
    if (! cdirNext(*view_, cur_key_, sle_, entry_, index_, j_))
    {
        if (index_ != 0 ||
            (cur_key_ = view_->succ(++cur_key_,
                next_quality_).value_or(zero)) == zero)
        {
            cur_key_ = key_;
            entry_ = 0;
            index_ = zero;
        }
        else if (! cdirFirst(*view_, cur_key_,
            sle_, entry_, index_, j_))
        {
            assert(false);
        }
    }

    cache_ = boost::none;
    return *this;
}

BookDirs::const_iterator
BookDirs::const_iterator::operator++(int)
{
    assert(index_ != beast::zero);
    const_iterator tmp(*this);
    ++(*this);
    return tmp;
}

} //涟漪
