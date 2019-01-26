
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

#include <ripple/ledger/ApplyView.h>
#include <ripple/basics/contract.h>
#include <ripple/protocol/Protocol.h>
#include <cassert>

namespace ripple {

boost::optional<std::uint64_t>
ApplyView::dirAdd (
    bool preserveOrder,
    Keylet const& directory,
    uint256 const& key,
    std::function<void(std::shared_ptr<SLE> const&)> const& describe)
{
    auto root = peek(directory);

    if (! root)
    {
//没有根，做吧。
        root = std::make_shared<SLE>(directory);
        root->setFieldH256 (sfRootIndex, directory.key);
        describe (root);

        STVector256 v;
        v.push_back (key);
        root->setFieldV256 (sfIndexes, v);

        insert (root);
        return std::uint64_t{0};
    }

    std::uint64_t page = root->getFieldU64(sfIndexPrevious);

    auto node = root;

    if (page)
    {
        node = peek (keylet::page(directory, page));
        if (!node)
            LogicError ("Directory chain: root back-pointer broken.");
    }

    auto indexes = node->getFieldV256(sfIndexes);

//如果有空间，我们就用它：
    if (indexes.size () < dirNodeMaxEntries)
    {
        if (preserveOrder)
        {
            if (std::find(indexes.begin(), indexes.end(), key) != indexes.end())
                LogicError ("dirInsert: double insertion");

            indexes.push_back(key);
        }
        else
        {
//无法确定此页是否已排序，因为
//这可能是一个我们还没有接触过的传统页面。采取
//是时候整理了。
            std::sort (indexes.begin(), indexes.end());

            auto pos = std::lower_bound(indexes.begin(), indexes.end(), key);

            if (pos != indexes.end() && key == *pos)
                LogicError ("dirInsert: double insertion");

            indexes.insert (pos, key);
        }

        node->setFieldV256 (sfIndexes, indexes);
        update(node);
        return page;
    }

//检查我们是否缺页。
    if (++page >= dirNodeMaxPages)
        return boost::none;

//我们将要创建一个新节点；我们将其链接到
//链优先：
    node->setFieldU64 (sfIndexNext, page);
    update(node);

    root->setFieldU64 (sfIndexPrevious, page);
    update(root);

//插入新密钥：
    indexes.clear();
    indexes.push_back (key);

    node = std::make_shared<SLE>(keylet::page(directory, page));
    node->setFieldH256 (sfRootIndex, directory.key);
    node->setFieldV256 (sfIndexes, indexes);

//通过不指定值0来节省一些空间，因为
//这是默认值。
    if (page != 1)
        node->setFieldU64 (sfIndexPrevious, page - 1);
    describe (node);
    insert (node);

    return page;
}

bool
ApplyView::dirRemove (
    Keylet const& directory,
    std::uint64_t page,
    uint256 const& key,
    bool keepRoot)
{
    auto node = peek(keylet::page(directory, page));

    if (!node)
        return false;

    std::uint64_t constexpr rootPage = 0;

    {
        auto entries = node->getFieldV256(sfIndexes);

        auto it = std::find(entries.begin(), entries.end(), key);

        if (entries.end () == it)
            return false;

//当我们移除时，我们总是保持相对的顺序。
        entries.erase(it);

        node->setFieldV256(sfIndexes, entries);
        update(node);

        if (!entries.empty())
            return true;
    }

//当前页现在为空；请检查是否可以
//已删除，如果是，则是否删除整个目录
//现在可以移除。
    auto prevPage = node->getFieldU64(sfIndexPrevious);
    auto nextPage = node->getFieldU64(sfIndexNext);

//第一页是目录的根节点，并且
//特别处理：即使
//它是空的，除非我们计划移除整个
//目录。
    if (page == rootPage)
    {
        if (nextPage == page && prevPage != page)
            LogicError ("Directory chain: fwd link broken");

        if (prevPage == page && nextPage != page)
            LogicError ("Directory chain: rev link broken");

//在某些情况下，旧版本的代码会
//允许最后一页为空。删除此类
//如果我们偶然发现它们：
        if (nextPage == prevPage && nextPage != page)
        {
            auto last = peek(keylet::page(directory, nextPage));
            if (!last)
                LogicError ("Directory chain: fwd link broken.");

            if (last->getFieldV256 (sfIndexes).empty())
            {
//更新第一页的链接列表并
//标记为已更新。
                node->setFieldU64 (sfIndexNext, page);
                node->setFieldU64 (sfIndexPrevious, page);
                update(node);

//删除空白的最后一页：
                erase(last);

//确保我们的当地价值反映
//更新信息：
                nextPage = page;
                prevPage = page;
            }
        }

        if (keepRoot)
            return true;

//如果没有其他页面，请删除根目录：
        if (nextPage == page && prevPage == page)
            erase(node);

        return true;
    }

//对于除根节点以外的节点，这永远不会发生：
    if (nextPage == page)
        LogicError ("Directory chain: fwd link broken");

    if (prevPage == page)
        LogicError ("Directory chain: rev link broken");

//此节点不是根节点，因此它可以位于
//在列表的中间或结尾。首先把它解开
//然后检查列表中是否只有
//根：
    auto prev = peek(keylet::page(directory, prevPage));
    if (!prev)
        LogicError ("Directory chain: fwd link broken.");
//修复previous以指向其新的next。
    prev->setFieldU64(sfIndexNext, nextPage);
    update (prev);

    auto next = peek(keylet::page(directory, nextPage));
    if (!next)
        LogicError ("Directory chain: rev link broken.");
//修复next以指向其新的previous。
    next->setFieldU64(sfIndexPrevious, prevPage);
    update(next);

//该页不再链接。删除它。
    erase(node);

//检查下一页是否为最后一页，如果
//所以，不管它是空的。如果是，删除它。
    if (nextPage != rootPage &&
        next->getFieldU64 (sfIndexNext) == rootPage &&
        next->getFieldV256 (sfIndexes).empty())
    {
//因为next不指向根，所以
//不能指向上一个。
        erase(next);

//上一页现在是最后一页：
        prev->setFieldU64(sfIndexNext, rootPage);
        update (prev);

//根目录指向最后一页：
        auto root = peek(keylet::page(directory, rootPage));
        if (!root)
            LogicError ("Directory chain: root link broken.");
        root->setFieldU64(sfIndexPrevious, prevPage);
        update (root);

        nextPage = rootPage;
    }

//如果我们不保留根，那么检查
//它是空的。如果是，也删除它。
    if (!keepRoot && nextPage == rootPage && prevPage == rootPage)
    {
        if (prev->getFieldV256 (sfIndexes).empty())
            erase(prev);
    }

    return true;
}

} //涟漪
