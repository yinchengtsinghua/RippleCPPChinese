
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

#include <ripple/basics/contract.h>
#include <ripple/shamap/SHAMap.h>

namespace ripple {

//此代码用于比较其他节点的事务树
//属于我们自己。它返回一个包含所有不同项的映射
//在两个sha地图之间。最好不要从树上下来
//具有相同分支哈希的分支。可以通过一个限制，所以
//如果一个节点向我们发送一个映射，
//完全没有意义。（我们的同步算法将避免
//同步匹配的分支。）

bool SHAMap::walkBranch (SHAMapAbstractNode* node,
                         std::shared_ptr<SHAMapItem const> const& otherMapItem,
                         bool isFirstMap,Delta& differences, int& maxCount) const
{
//在另一个地图中，走一个与空分支或单个项目匹配的shamap分支
    std::stack <SHAMapAbstractNode*, std::vector<SHAMapAbstractNode*>> nodeStack;
    nodeStack.push (node);

    bool emptyBranch = !otherMapItem;

    while (!nodeStack.empty ())
    {
        node = nodeStack.top ();
        nodeStack.pop ();

        if (node->isInner ())
        {
//这是一个内部节点，添加所有非空分支
            auto inner = static_cast<SHAMapInnerNode*>(node);
            for (int i = 0; i < 16; ++i)
                if (!inner->isEmptyBranch (i))
                    nodeStack.push ({descendThrow (inner, i)});
        }
        else
        {
//这是叶节点，处理其项
            auto item = static_cast<SHAMapTreeNode*>(node)->peekItem();

            if (emptyBranch || (item->key() != otherMapItem->key()))
            {
//无与伦比的
                if (isFirstMap)
                    differences.insert (std::make_pair (item->key(),
                        DeltaRef (item, std::shared_ptr<SHAMapItem const> ())));
                else
                    differences.insert (std::make_pair (item->key(),
                        DeltaRef (std::shared_ptr<SHAMapItem const> (), item)));

                if (--maxCount <= 0)
                    return false;
            }
            else if (item->peekData () != otherMapItem->peekData ())
            {
//具有相同标记的不匹配项
                if (isFirstMap)
                    differences.insert (std::make_pair (item->key(),
                                                DeltaRef (item, otherMapItem)));
                else
                    differences.insert (std::make_pair (item->key(),
                                                DeltaRef (otherMapItem, item)));

                if (--maxCount <= 0)
                    return false;

                emptyBranch = true;
            }
            else
            {
//精确匹配
                emptyBranch = true;
            }
        }
    }

    if (!emptyBranch)
    {
//OtherMapItem不匹配，必须添加
if (isFirstMap) //这是第一个地图，所以其他项目是从第二个
            differences.insert(std::make_pair(otherMapItem->key(),
                                              DeltaRef(std::shared_ptr<SHAMapItem const>(),
                                                       otherMapItem)));
        else
            differences.insert(std::make_pair(otherMapItem->key(),
                DeltaRef(otherMapItem, std::shared_ptr<SHAMapItem const>())));

        if (--maxCount <= 0)
            return false;
    }

    return true;
}

bool
SHAMap::compare (SHAMap const& otherMap,
                 Delta& differences, int maxCount) const
{
//比较两个哈希树，将最大计数差异添加到差异表中
//返回值：真=给出完整的差异表，假=差异太多
//向损坏的表或丢失的节点引发
//警告：OtherMap未锁定，必须是不可变的

    assert (isValid () && otherMap.isValid ());

    if (getHash () == otherMap.getHash ())
        return true;

    using StackEntry = std::pair <SHAMapAbstractNode*, SHAMapAbstractNode*>;
std::stack <StackEntry, std::vector<StackEntry>> nodeStack; //跟踪我们推送的节点

    nodeStack.push ({root_.get(), otherMap.root_.get()});
    while (!nodeStack.empty ())
    {
        SHAMapAbstractNode* ourNode = nodeStack.top().first;
        SHAMapAbstractNode* otherNode = nodeStack.top().second;
        nodeStack.pop ();

        if (!ourNode || !otherNode)
        {
            assert (false);
            Throw<SHAMapMissingNode> (type_, uint256 ());
        }

        if (ourNode->isLeaf () && otherNode->isLeaf ())
        {
//两片叶子
            auto ours = static_cast<SHAMapTreeNode*>(ourNode);
            auto other = static_cast<SHAMapTreeNode*>(otherNode);
            if (ours->peekItem()->key() == other->peekItem()->key())
            {
                if (ours->peekItem()->peekData () != other->peekItem()->peekData ())
                {
                    differences.insert (std::make_pair (ours->peekItem()->key(),
                                                 DeltaRef (ours->peekItem (),
                                                 other->peekItem ())));
                    if (--maxCount <= 0)
                        return false;
                }
            }
            else
            {
                differences.insert (std::make_pair(ours->peekItem()->key(),
                                                   DeltaRef(ours->peekItem(),
                                                   std::shared_ptr<SHAMapItem const>())));
                if (--maxCount <= 0)
                    return false;

                differences.insert(std::make_pair(other->peekItem()->key(),
                    DeltaRef(std::shared_ptr<SHAMapItem const>(), other->peekItem ())));
                if (--maxCount <= 0)
                    return false;
            }
        }
        else if (ourNode->isInner () && otherNode->isLeaf ())
        {
            auto ours = static_cast<SHAMapInnerNode*>(ourNode);
            auto other = static_cast<SHAMapTreeNode*>(otherNode);
            if (!walkBranch (ours, other->peekItem (),
                    true, differences, maxCount))
                return false;
        }
        else if (ourNode->isLeaf () && otherNode->isInner ())
        {
            auto ours = static_cast<SHAMapTreeNode*>(ourNode);
            auto other = static_cast<SHAMapInnerNode*>(otherNode);
            if (!otherMap.walkBranch (other, ours->peekItem (),
                                       false, differences, maxCount))
                return false;
        }
        else if (ourNode->isInner () && otherNode->isInner ())
        {
            auto ours = static_cast<SHAMapInnerNode*>(ourNode);
            auto other = static_cast<SHAMapInnerNode*>(otherNode);
            for (int i = 0; i < 16; ++i)
                if (ours->getChildHash (i) != other->getChildHash (i))
                {
                    if (other->isEmptyBranch (i))
                    {
//我们有一根树枝，另一棵树没有
                        SHAMapAbstractNode* iNode = descendThrow (ours, i);
                        if (!walkBranch (iNode,
                                         std::shared_ptr<SHAMapItem const> (), true,
                                         differences, maxCount))
                            return false;
                    }
                    else if (ours->isEmptyBranch (i))
                    {
//另一棵树有一根树枝，我们没有。
                        SHAMapAbstractNode* iNode =
                            otherMap.descendThrow(other, i);
                        if (!otherMap.walkBranch (iNode,
                                                   std::shared_ptr<SHAMapItem const>(),
                                                   false, differences, maxCount))
                            return false;
                    }
else //这两棵树有不同的非空树枝
                        nodeStack.push ({descendThrow (ours, i),
                                        otherMap.descendThrow (other, i)});
                }
        }
        else
            assert (false);
    }

    return true;
}

void SHAMap::walkMap (std::vector<SHAMapMissingNode>& missingNodes, int maxMissing) const
{
if (!root_->isInner ())  //根节点是唯一的节点，我们有它
        return;

    using StackEntry = std::shared_ptr<SHAMapInnerNode>;
    std::stack <StackEntry, std::vector <StackEntry>> nodeStack;

    nodeStack.push (std::static_pointer_cast<SHAMapInnerNode>(root_));

    while (!nodeStack.empty ())
    {
        std::shared_ptr<SHAMapInnerNode> node = std::move (nodeStack.top());
        nodeStack.pop ();

        for (int i = 0; i < 16; ++i)
        {
            if (!node->isEmptyBranch (i))
            {
                std::shared_ptr<SHAMapAbstractNode> nextNode = descendNoStore (node, i);

                if (nextNode)
                {
                    if (nextNode->isInner ())
                        nodeStack.push(
                            std::static_pointer_cast<SHAMapInnerNode>(nextNode));
                }
                else
                {
                    missingNodes.emplace_back (type_, node->getChildHash (i));
                    if (--maxMissing <= 0)
                        return;
                }
            }
        }
    }
}

} //涟漪
