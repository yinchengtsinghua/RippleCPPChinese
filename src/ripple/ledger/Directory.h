
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

#ifndef RIPPLE_LEDGER_DIR_H_INCLUDED
#define RIPPLE_LEDGER_DIR_H_INCLUDED

#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/Indexes.h>

namespace ripple {

class Dir
{
private:
    ReadView const* view_ = nullptr;
    Keylet root_;
    std::shared_ptr<SLE const> sle_;
    STVector256 const* indexes_ = nullptr;

public:
    class const_iterator;
    using value_type = std::shared_ptr<SLE const>;

    Dir(ReadView const&, Keylet const&);

    const_iterator
    begin() const;

    const_iterator
    end() const;
};

class Dir::const_iterator
{
public:
    using value_type =
        Dir::value_type;
    using pointer =
        value_type const*;
    using reference =
        value_type const&;
    using difference_type =
        std::ptrdiff_t;
    using iterator_category =
        std::forward_iterator_tag;

    const_iterator() = default;

    bool
    operator==(const_iterator const& other) const;

    bool
    operator!=(const_iterator const& other) const
    {
        return ! (*this == other);
    }

    reference
    operator*() const;

    pointer
    operator->() const
    {
        return &**this;
    }

    const_iterator&
    operator++();

    const_iterator
    operator++(int);

    Keylet const&
    page() const
    {
        return page_;
    }

    uint256
    index() const
    {
        return index_;
    }

private:
    friend class Dir;

    const_iterator(ReadView const& view,
        Keylet const& root, Keylet const& page)
    : view_(&view)
    , root_(root)
    , page_(page)
    {
    }

    ReadView const* view_ = nullptr;
    Keylet root_;
    Keylet page_;
    uint256 index_;
    boost::optional<value_type> mutable cache_;
    std::shared_ptr<SLE const> sle_;
    STVector256 const* indexes_ = nullptr;
    std::vector<uint256>::const_iterator it_;
};

} //涟漪

#endif
