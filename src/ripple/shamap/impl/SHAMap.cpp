
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

SHAMap::SHAMap (
    SHAMapType t,
    Family& f,
    version v)
    : f_ (f)
    , journal_(f.journal())
    , seq_ (1)
    , state_ (SHAMapState::Modifying)
    , type_ (t)
{
    if (v == version{2})
        root_ = std::make_shared<SHAMapInnerNodeV2>(seq_, 0);
    else
        root_ = std::make_shared<SHAMapInnerNode>(seq_);
}

SHAMap::SHAMap (
    SHAMapType t,
    uint256 const& hash,
    Family& f,
    version v)
    : f_ (f)
    , journal_(f.journal())
    , seq_ (1)
    , state_ (SHAMapState::Synching)
    , type_ (t)
{
    if (v == version{2})
        root_ = std::make_shared<SHAMapInnerNodeV2>(seq_, 0);
    else
        root_ = std::make_shared<SHAMapInnerNode>(seq_);
}

SHAMap::~SHAMap ()
{
    state_ = SHAMapState::Invalid;
}

std::shared_ptr<SHAMap>
SHAMap::snapShot (bool isMutable) const
{
    auto ret = std::make_shared<SHAMap> (type_, f_, get_version());
    SHAMap& newMap = *ret;

    if (!isMutable)
        newMap.state_ = SHAMapState::Immutable;

    newMap.seq_ = seq_ + 1;
    newMap.ledgerSeq_ = ledgerSeq_;
    newMap.root_ = root_;
    newMap.backed_ = backed_;

    if ((state_ != SHAMapState::Immutable) || !isMutable)
    {
//如果其中一个映射可能发生更改，则它们不能共享节点
        newMap.unshare ();
    }

    return ret;
}

std::shared_ptr<SHAMap>
SHAMap::make_v2() const
{
    assert(!is_v2());
    auto ret = std::make_shared<SHAMap>(type_, f_, version{2});
    ret->seq_ = seq_ + 1;
    SharedPtrNodeStack stack;
    for (auto leaf = peekFirstItem(stack); leaf != nullptr;
         leaf = peekNextItem(leaf->peekItem()->key(), stack))
    {
        auto node_type = leaf->getType();
        ret->addGiveItem(leaf->peekItem(),
                         node_type != SHAMapTreeNode::tnACCOUNT_STATE,
                         node_type == SHAMapTreeNode::tnTRANSACTION_MD);
    }
    NodeObjectType t;
    switch (type_)
    {
    case SHAMapType::TRANSACTION:
        t = hotTRANSACTION_NODE;
        break;
    case SHAMapType::STATE:
        t = hotACCOUNT_NODE;
        break;
    default:
        t = hotUNKNOWN;
        break;
    }
    ret->flushDirty(t, ret->seq_);
    ret->unshare();
    return ret;
}

std::shared_ptr<SHAMap>
SHAMap::make_v1() const
{
    assert(is_v2());
    auto ret = std::make_shared<SHAMap>(type_, f_, version{1});
    ret->seq_ = seq_ + 1;
    SharedPtrNodeStack stack;
    for (auto leaf = peekFirstItem(stack); leaf != nullptr;
         leaf = peekNextItem(leaf->peekItem()->key(), stack))
    {
        auto node_type = leaf->getType();
        ret->addGiveItem(leaf->peekItem(),
                         node_type != SHAMapTreeNode::tnACCOUNT_STATE,
                         node_type == SHAMapTreeNode::tnTRANSACTION_MD);
    }
    NodeObjectType t;
    switch (type_)
    {
    case SHAMapType::TRANSACTION:
        t = hotTRANSACTION_NODE;
        break;
    case SHAMapType::STATE:
        t = hotACCOUNT_NODE;
        break;
    default:
        t = hotUNKNOWN;
        break;
    }
    ret->flushDirty(t, ret->seq_);
    ret->unshare();
    return ret;
}

void
SHAMap::dirtyUp (SharedPtrNodeStack& stack,
                 uint256 const& target, std::shared_ptr<SHAMapAbstractNode> child)
{
//将树从内部节点向上移动到根节点
//更新哈希和链接
//堆栈是内部节点到（但不包括）子节点的路径
//子节点可以是内部节点或叶

    assert ((state_ != SHAMapState::Synching) && (state_ != SHAMapState::Immutable));
    assert (child && (child->getSeq() == seq_));

    while (!stack.empty ())
    {
        auto node = std::dynamic_pointer_cast<SHAMapInnerNode>(stack.top ().first);
        SHAMapNodeID nodeID = stack.top ().second;
        stack.pop ();
        assert (node != nullptr);

        int branch = nodeID.selectBranch (target);
        assert (branch >= 0);

        node = unshareNode(std::move(node), nodeID);
        node->setChild (branch, child);

        child = std::move (node);
    }
}

SHAMapTreeNode*
SHAMap::walkTowardsKey(uint256 const& id, SharedPtrNodeStack* stack) const
{
    assert(stack == nullptr || stack->empty());
    auto inNode = root_;
    SHAMapNodeID nodeID;
    auto const isv2 = is_v2();

    while (inNode->isInner())
    {
        if (stack != nullptr)
            stack->push({inNode, nodeID});

        if (isv2)
        {
            auto n = std::static_pointer_cast<SHAMapInnerNodeV2>(inNode);
            if (!n->has_common_prefix(id))
                return nullptr;
        }
        auto const inner = std::static_pointer_cast<SHAMapInnerNode>(inNode);
        auto const branch = nodeID.selectBranch (id);
        if (inner->isEmptyBranch (branch))
            return nullptr;

        inNode = descendThrow (inner, branch);
        if (isv2)
        {
            if (inNode->isInner())
            {
                auto n = std::dynamic_pointer_cast<SHAMapInnerNodeV2>(inNode);
                if (n == nullptr)
                {
                    assert (false);
                    return nullptr;
                }
                nodeID = SHAMapNodeID{n->depth(), n->common()};
            }
            else
            {
                nodeID = SHAMapNodeID{64, inNode->key()};
            }
        }
        else
        {
            nodeID = nodeID.getChildNodeID (branch);
        }
    }

    if (stack != nullptr)
        stack->push({inNode, nodeID});
    return static_cast<SHAMapTreeNode*>(inNode.get());
}

SHAMapTreeNode*
SHAMap::findKey(uint256 const& id) const
{
    SHAMapTreeNode* leaf = walkTowardsKey(id);
    if (leaf && leaf->peekItem()->key() != id)
        leaf = nullptr;
    return leaf;
}

std::shared_ptr<SHAMapAbstractNode>
SHAMap::fetchNodeFromDB (SHAMapHash const& hash) const
{
    std::shared_ptr<SHAMapAbstractNode> node;

    if (backed_)
    {
        if (auto obj = f_.db().fetch(hash.as_uint256(), ledgerSeq_))
        {
            try
            {
                node = SHAMapAbstractNode::make(makeSlice(obj->getData()),
                    0, snfPREFIX, hash, true, f_.journal());
                if (node && node->isInner())
                {
                    bool isv2 = std::dynamic_pointer_cast<SHAMapInnerNodeV2>(node) != nullptr;
                    if (isv2 != is_v2())
                    {
                        auto root =  std::dynamic_pointer_cast<SHAMapInnerNode>(root_);
                        assert(root);
                        assert(root->isEmpty());
                        if (isv2)
                        {
                            auto temp = make_v2();
                            swap(temp->root_, const_cast<std::shared_ptr<SHAMapAbstractNode>&>(root_));
                        }
                        else
                        {
                            auto temp = make_v1();
                            swap(temp->root_, const_cast<std::shared_ptr<SHAMapAbstractNode>&>(root_));
                        }
                    }
                }
                if (node)
                    canonicalize (hash, node);
            }
            catch (std::exception const&)
            {
                JLOG(journal_.warn()) <<
                    "Invalid DB node " << hash;
                return std::shared_ptr<SHAMapTreeNode> ();
            }
        }
        else if (full_)
        {
            f_.missing_node(ledgerSeq_);
            const_cast<bool&>(full_) = false;
        }
    }

    return node;
}

//查看同步筛选器是否具有节点
std::shared_ptr<SHAMapAbstractNode>
SHAMap::checkFilter(SHAMapHash const& hash,
                    SHAMapSyncFilter* filter) const
{
    std::shared_ptr<SHAMapAbstractNode> node;
    if (auto nodeData = filter->getNode (hash))
    {
        node = SHAMapAbstractNode::make(
            makeSlice(*nodeData), 0, snfPREFIX, hash, true, f_.journal ());
        if (node)
        {
            filter->gotNode (true, hash, ledgerSeq_,
                std::move(*nodeData), node->getType ());
            if (backed_)
                canonicalize (hash, node);
        }
    }
    return node;
}

//获取节点而不引发
//用于预期缺少节点的映射
std::shared_ptr<SHAMapAbstractNode> SHAMap::fetchNodeNT(
    SHAMapHash const& hash,
    SHAMapSyncFilter* filter) const
{
    std::shared_ptr<SHAMapAbstractNode> node = getCache (hash);
    if (node)
        return node;

    if (backed_)
    {
        node = fetchNodeFromDB (hash);
        if (node)
        {
            canonicalize (hash, node);
            return node;
        }
    }

    if (filter)
        node = checkFilter (hash, filter);

    return node;
}

std::shared_ptr<SHAMapAbstractNode> SHAMap::fetchNodeNT (SHAMapHash const& hash) const
{
    auto node = getCache (hash);

    if (!node && backed_)
        node = fetchNodeFromDB (hash);

    return node;
}

//如果节点丢失则引发
std::shared_ptr<SHAMapAbstractNode> SHAMap::fetchNode (SHAMapHash const& hash) const
{
    auto node = fetchNodeNT (hash);

    if (!node)
        Throw<SHAMapMissingNode> (type_, hash);

    return node;
}

SHAMapAbstractNode* SHAMap::descendThrow (SHAMapInnerNode* parent, int branch) const
{
    SHAMapAbstractNode* ret = descend (parent, branch);

    if (! ret && ! parent->isEmptyBranch (branch))
        Throw<SHAMapMissingNode> (type_, parent->getChildHash (branch));

    return ret;
}

std::shared_ptr<SHAMapAbstractNode>
SHAMap::descendThrow (std::shared_ptr<SHAMapInnerNode> const& parent, int branch) const
{
    std::shared_ptr<SHAMapAbstractNode> ret = descend (parent, branch);

    if (! ret && ! parent->isEmptyBranch (branch))
        Throw<SHAMapMissingNode> (type_, parent->getChildHash (branch));

    return ret;
}

SHAMapAbstractNode* SHAMap::descend (SHAMapInnerNode* parent, int branch) const
{
    SHAMapAbstractNode* ret = parent->getChildPointer (branch);
    if (ret || !backed_)
        return ret;

    std::shared_ptr<SHAMapAbstractNode> node = fetchNodeNT (parent->getChildHash (branch));
    if (!node || isInconsistentNode(node))
        return nullptr;

    node = parent->canonicalizeChild (branch, std::move(node));
    return node.get ();
}

std::shared_ptr<SHAMapAbstractNode>
SHAMap::descend (std::shared_ptr<SHAMapInnerNode> const& parent, int branch) const
{
    std::shared_ptr<SHAMapAbstractNode> node = parent->getChild (branch);
    if (node || !backed_)
        return node;

    node = fetchNode (parent->getChildHash (branch));
    if (!node || isInconsistentNode(node))
        return nullptr;

    node = parent->canonicalizeChild (branch, std::move(node));
    return node;
}

//获取将挂接到此分支的节点，
//但没有连接。
std::shared_ptr<SHAMapAbstractNode>
SHAMap::descendNoStore (std::shared_ptr<SHAMapInnerNode> const& parent, int branch) const
{
    std::shared_ptr<SHAMapAbstractNode> ret = parent->getChild (branch);
    if (!ret && backed_)
        ret = fetchNode (parent->getChildHash (branch));
    return ret;
}

std::pair <SHAMapAbstractNode*, SHAMapNodeID>
SHAMap::descend (SHAMapInnerNode * parent, SHAMapNodeID const& parentID,
    int branch, SHAMapSyncFilter * filter) const
{
    assert (parent->isInner ());
    assert ((branch >= 0) && (branch < 16));
    assert (!parent->isEmptyBranch (branch));

    SHAMapAbstractNode* child = parent->getChildPointer (branch);
    auto const& childHash = parent->getChildHash (branch);

    if (!child)
    {
        std::shared_ptr<SHAMapAbstractNode> childNode = fetchNodeNT (childHash, filter);

        if (childNode)
        {
            childNode = parent->canonicalizeChild (branch, std::move(childNode));
            child = childNode.get ();
        }

        if (child && isInconsistentNode(childNode))
            child = nullptr;
    }

    if (child && is_v2())
    {
        if (child->isInner())
        {
            auto n = static_cast<SHAMapInnerNodeV2*>(child);
            return std::make_pair(child, SHAMapNodeID{n->depth(), n->key()});
        }
        return std::make_pair(child, SHAMapNodeID{64, child->key()});
    }

    return std::make_pair (child, parentID.getChildNodeID (branch));
}

SHAMapAbstractNode*
SHAMap::descendAsync (SHAMapInnerNode* parent, int branch,
    SHAMapSyncFilter * filter, bool & pending) const
{
    pending = false;

    SHAMapAbstractNode* ret = parent->getChildPointer (branch);
    if (ret)
        return ret;

    auto const& hash = parent->getChildHash (branch);

    std::shared_ptr<SHAMapAbstractNode> ptr = getCache (hash);
    if (!ptr)
    {
        if (filter)
            ptr = checkFilter (hash, filter);

        if (!ptr && backed_)
        {
            std::shared_ptr<NodeObject> obj;
            if (! f_.db().asyncFetch (hash.as_uint256(), ledgerSeq_, obj))
            {
                pending = true;
                return nullptr;
            }
            if (!obj)
                return nullptr;

            ptr = SHAMapAbstractNode::make(makeSlice(obj->getData()), 0, snfPREFIX,
                                           hash, true, f_.journal());
            if (ptr && backed_)
                canonicalize (hash, ptr);
        }
    }

    if (ptr && isInconsistentNode(ptr))
        ptr = nullptr;
    if (ptr)
        ptr = parent->canonicalizeChild (branch, std::move(ptr));

    return ptr.get ();
}

template <class Node>
std::shared_ptr<Node>
SHAMap::unshareNode (std::shared_ptr<Node> node, SHAMapNodeID const& nodeID)
{
//确保节点适合预期操作（写时复制）
    assert (node->isValid ());
    assert (node->getSeq () <= seq_);
    if (node->getSeq () != seq_)
    {
//有一头母牛
        assert (state_ != SHAMapState::Immutable);
        node = std::static_pointer_cast<Node>(node->clone(seq_));
        assert (node->isValid ());
        if (nodeID.isRoot ())
            root_ = node;
    }
    return node;
}

SHAMapTreeNode*
SHAMap::firstBelow(std::shared_ptr<SHAMapAbstractNode> node,
                   SharedPtrNodeStack& stack, int branch) const
{
//返回此节点或其下方的第一项
    if (node->isLeaf())
    {
        auto n = std::static_pointer_cast<SHAMapTreeNode>(node);
        stack.push({node, {64, n->peekItem()->key()}});
        return n.get();
    }
    auto inner = std::static_pointer_cast<SHAMapInnerNode>(node);
    if (stack.empty())
        stack.push({inner, SHAMapNodeID{}});
    else
    {
        if (is_v2())
        {
            auto inner2 = std::dynamic_pointer_cast<SHAMapInnerNodeV2>(inner);
            assert(inner2 != nullptr);
            stack.push({inner2, {inner2->depth(), inner2->common()}});
        }
        else
        {
            stack.push({inner, stack.top().second.getChildNodeID(branch)});
        }
    }
    for (int i = 0; i < 16;)
    {
        if (!inner->isEmptyBranch(i))
        {
            node = descendThrow(inner, i);
            assert(!stack.empty());
            if (node->isLeaf())
            {
                auto n = std::static_pointer_cast<SHAMapTreeNode>(node);
                stack.push({n, {64, n->peekItem()->key()}});
                return n.get();
            }
            inner = std::static_pointer_cast<SHAMapInnerNode>(node);
            if (is_v2())
            {
                auto inner2 = std::static_pointer_cast<SHAMapInnerNodeV2>(inner);
                stack.push({inner2, {inner2->depth(), inner2->common()}});
            }
            else
            {
                stack.push({inner, stack.top().second.getChildNodeID(branch)});
            }
i = 0;  //扫描此新节点的所有16个分支
        }
        else
++i;  //扫描下一个分支
    }
    return nullptr;
}

static const std::shared_ptr<SHAMapItem const> no_item;

std::shared_ptr<SHAMapItem const> const&
SHAMap::onlyBelow (SHAMapAbstractNode* node) const
{
//如果此节点下只有一个项，则返回该项

    while (!node->isLeaf ())
    {
        SHAMapAbstractNode* nextNode = nullptr;
        auto inner = static_cast<SHAMapInnerNode*>(node);
        for (int i = 0; i < 16; ++i)
        {
            if (!inner->isEmptyBranch (i))
            {
                if (nextNode)
                    return no_item;

                nextNode = descendThrow (inner, i);
            }
        }

        if (!nextNode)
        {
            assert (false);
            return no_item;
        }

        node = nextNode;
    }

//内部节点必须至少有一个叶
//在它下面，除非它是根
    auto leaf = static_cast<SHAMapTreeNode*>(node);
    assert (leaf->hasItem () || (leaf == root_.get ()));

    return leaf->peekItem ();
}

static std::shared_ptr<
    SHAMapItem const> const nullConstSHAMapItem;

SHAMapTreeNode const*
SHAMap::peekFirstItem(SharedPtrNodeStack& stack) const
{
    assert(stack.empty());
    SHAMapTreeNode* node = firstBelow(root_, stack);
    if (!node)
    {
        while (!stack.empty())
            stack.pop();
        return nullptr;
    }
    return node;
}

SHAMapTreeNode const*
SHAMap::peekNextItem(uint256 const& id, SharedPtrNodeStack& stack) const
{
    assert(!stack.empty());
    assert(stack.top().first->isLeaf());
    stack.pop();
    while (!stack.empty())
    {
        auto node = stack.top().first;
        auto nodeID = stack.top().second;
        assert(!node->isLeaf());
        auto inner = std::static_pointer_cast<SHAMapInnerNode>(node);
        for (auto i = nodeID.selectBranch(id) + 1; i < 16; ++i)
        {
            if (!inner->isEmptyBranch(i))
            {
                node = descendThrow(inner, i);
                auto leaf = firstBelow(node, stack, i);
                if (!leaf)
                    Throw<SHAMapMissingNode> (type_, id);
                assert(leaf->isLeaf());
                return leaf;
            }
        }
        stack.pop();
    }
//必须是最后一项
    return nullptr;
}

std::shared_ptr<SHAMapItem const> const&
SHAMap::peekItem (uint256 const& id) const
{
    SHAMapTreeNode* leaf = findKey(id);

    if (!leaf)
        return no_item;

    return leaf->peekItem ();
}

std::shared_ptr<SHAMapItem const> const&
SHAMap::peekItem (uint256 const& id, SHAMapTreeNode::TNType& type) const
{
    SHAMapTreeNode* leaf = findKey(id);

    if (!leaf)
        return no_item;

    type = leaf->getType ();
    return leaf->peekItem ();
}

std::shared_ptr<SHAMapItem const> const&
SHAMap::peekItem (uint256 const& id, SHAMapHash& hash) const
{
    SHAMapTreeNode* leaf = findKey(id);

    if (!leaf)
        return no_item;

    hash = leaf->getNodeHash ();
    return leaf->peekItem ();
}

SHAMap::const_iterator
SHAMap::upper_bound(uint256 const& id) const
{
//获取树中给定项之后的下一项的常量迭代器
//项目不必在树中
    SharedPtrNodeStack stack;
    walkTowardsKey(id, &stack);
    std::shared_ptr<SHAMapAbstractNode> node;
    SHAMapNodeID nodeID;
    auto const isv2 = is_v2();
    while (!stack.empty())
    {
        std::tie(node, nodeID) = stack.top();
        if (node->isLeaf())
        {
            auto leaf = static_cast<SHAMapTreeNode*>(node.get());
            if (leaf->peekItem()->key() > id)
                return const_iterator(this, leaf->peekItem().get(), std::move(stack));
        }
        else
        {
            auto inner = std::static_pointer_cast<SHAMapInnerNode>(node);
            int branch;
            if (isv2)
            {
                auto n = std::static_pointer_cast<SHAMapInnerNodeV2>(inner);
                if (n->has_common_prefix(id))
                    branch = nodeID.selectBranch(id) + 1;
                else if (id < n->common())
                    branch = 0;
                else
                    branch = 16;
            }
            else
            {
                branch = nodeID.selectBranch(id) + 1;
            }
            for (; branch < 16; ++branch)
            {
                if (!inner->isEmptyBranch(branch))
                {
                    node = descendThrow(inner, branch);
                    auto leaf = firstBelow(node, stack, branch);
                    if (!leaf)
                        Throw<SHAMapMissingNode> (type_, id);
                    return const_iterator(this, leaf->peekItem().get(),
                                          std::move(stack));
                }
            }
        }
        stack.pop();
    }
    return end();
}

bool SHAMap::hasItem (uint256 const& id) const
{
//树上有这个ID的项目吗
    SHAMapTreeNode* leaf = findKey(id);
    return (leaf != nullptr);
}

bool SHAMap::delItem (uint256 const& id)
{
//删除具有此ID的项目
    assert (state_ != SHAMapState::Immutable);

    SharedPtrNodeStack stack;
    walkTowardsKey(id, &stack);

    if (stack.empty ())
        Throw<SHAMapMissingNode> (type_, id);

    auto leaf = std::dynamic_pointer_cast<SHAMapTreeNode>(stack.top ().first);
    stack.pop ();

    if (!leaf || (leaf->peekItem ()->key() != id))
        return false;

    SHAMapTreeNode::TNType type = leaf->getType ();

//链条末端连接的是什么？
//（现在，没什么，因为我们删除了叶子）
    std::shared_ptr<SHAMapAbstractNode> prevNode;

    while (!stack.empty ())
    {
        auto node = std::static_pointer_cast<SHAMapInnerNode>(stack.top().first);
        SHAMapNodeID nodeID = stack.top().second;
        stack.pop();

        node = unshareNode(std::move(node), nodeID);
        node->setChild(nodeID.selectBranch(id), prevNode);

        if (!nodeID.isRoot ())
        {
//我们可能已将此节点设置为具有1或0个子节点
//如果是这样的话，我们需要移除这个分支
            int bc = node->getBranchCount();
            if (is_v2())
            {
                assert(bc != 0);
                if (bc == 1)
                {
                    for (int i = 0; i < 16; ++i)
                    {
                        if (!node->isEmptyBranch (i))
                        {
                            prevNode = descendThrow(node, i);
                            break;
                        }
                    }
                }
else  //BC>＝2
                {
//此节点现在是分支的结尾
                    prevNode = std::move(node);
                }
            }
            else
            {
                if (bc == 0)
                {
//这个分支下没有孩子
                    prevNode.reset ();
                }
                else if (bc == 1)
                {
//如果只有一件物品，就把线拉起来。
                    auto item = onlyBelow (node.get ());

                    if (item)
                    {
                        for (int i = 0; i < 16; ++i)
                        {
                            if (!node->isEmptyBranch (i))
                            {
                                node->setChild (i, nullptr);
                                break;
                            }
                        }
                        prevNode = std::make_shared<SHAMapTreeNode>(item, type, node->getSeq());
                    }
                    else
                    {
                        prevNode = std::move (node);
                    }
                }
                else
                {
//此节点现在是分支的结尾
                    prevNode = std::move (node);
                }
            }
        }
    }

    return true;
}

static
uint256
prefix(unsigned depth, uint256 const& key)
{
    uint256 r{};
    auto x = r.begin();
    auto y = key.begin();
    for (auto i = 0; i < depth/2; ++i, ++x, ++y)
        *x = *y;
    if (depth & 1)
        *x = *y & 0xF0;
    return r;
}

bool
SHAMap::addGiveItem (std::shared_ptr<SHAMapItem const> const& item,
                     bool isTransaction, bool hasMeta)
{
//添加指定项，不更新
    uint256 tag = item->key();
    SHAMapTreeNode::TNType type = !isTransaction ? SHAMapTreeNode::tnACCOUNT_STATE :
        (hasMeta ? SHAMapTreeNode::tnTRANSACTION_MD : SHAMapTreeNode::tnTRANSACTION_NM);

    assert (state_ != SHAMapState::Immutable);

    SharedPtrNodeStack stack;
    walkTowardsKey(tag, &stack);

    if (stack.empty ())
        Throw<SHAMapMissingNode> (type_, tag);

    auto node = stack.top ().first;
    auto nodeID = stack.top ().second;
    stack.pop ();

    if (node->isLeaf())
    {
        auto leaf = std::static_pointer_cast<SHAMapTreeNode>(node);
        if (leaf->peekItem()->key() == tag)
            return false;
    }
    node = unshareNode(std::move(node), nodeID);
    if (is_v2())
    {
        if (node->isInner())
        {
            auto inner = std::static_pointer_cast<SHAMapInnerNodeV2>(node);
            if (inner->has_common_prefix(tag))
            {
                int branch = nodeID.selectBranch(tag);
                assert(inner->isEmptyBranch(branch));
                auto newNode = std::make_shared<SHAMapTreeNode>(item, type, seq_);
                inner->setChild(branch, newNode);
            }
            else
            {
                assert(!stack.empty());
                auto parent = unshareNode(
                    std::static_pointer_cast<SHAMapInnerNodeV2>(stack.top().first),
                    stack.top().second);
                stack.top().first = parent;
                auto parent_depth = parent->depth();
                auto depth = inner->get_common_prefix(tag);
                auto new_inner = std::make_shared<SHAMapInnerNodeV2>(seq_);
                nodeID = SHAMapNodeID{depth, prefix(depth, inner->common())};
                new_inner->setChild(nodeID.selectBranch(inner->common()), inner);
                nodeID = SHAMapNodeID{depth, prefix(depth, tag)};
                new_inner->setChild(nodeID.selectBranch(tag),
                                    std::make_shared<SHAMapTreeNode>(item, type, seq_));
                new_inner->set_common(depth, prefix(depth, tag));
                nodeID = SHAMapNodeID{parent_depth, prefix(parent_depth, tag)};
                parent->setChild(nodeID.selectBranch(tag), new_inner);
                node = new_inner;
            }
        }
        else
        {
            auto leaf = std::static_pointer_cast<SHAMapTreeNode>(node);
            auto inner = std::make_shared<SHAMapInnerNodeV2>(seq_);
            inner->setChildren(leaf, std::make_shared<SHAMapTreeNode>(item, type, seq_));
            assert(!stack.empty());
            auto parent = unshareNode(
                std::static_pointer_cast<SHAMapInnerNodeV2>(stack.top().first),
                stack.top().second);
            stack.top().first = parent;
            node = inner;
        }
    }
else  //！iSyv2-（）
    {
        if (node->isInner ())
        {
//简单来说，我们以一个内部节点结束
            auto inner = std::static_pointer_cast<SHAMapInnerNode>(node);
            int branch = nodeID.selectBranch (tag);
            assert (inner->isEmptyBranch (branch));
            auto newNode = std::make_shared<SHAMapTreeNode> (item, type, seq_);
            inner->setChild (branch, newNode);
        }
        else
        {
//这是一个叶节点，必须将其设置为包含两个项的内部节点
            auto leaf = std::static_pointer_cast<SHAMapTreeNode>(node);
            std::shared_ptr<SHAMapItem const> otherItem = leaf->peekItem ();
            assert (otherItem && (tag != otherItem->key()));

            node = std::make_shared<SHAMapInnerNode>(node->getSeq());

            int b1, b2;

            while ((b1 = nodeID.selectBranch (tag)) ==
                   (b2 = nodeID.selectBranch (otherItem->key())))
            {
                stack.push ({node, nodeID});

//我们需要一个新的内部节点，因为这两个节点都在这个级别上进行相同的分支
                nodeID = nodeID.getChildNodeID (b1);
                node = std::make_shared<SHAMapInnerNode> (seq_);
            }

//我们可以在这里添加两个叶节点
            assert (node->isInner ());

            std::shared_ptr<SHAMapTreeNode> newNode =
                std::make_shared<SHAMapTreeNode> (item, type, seq_);
            assert (newNode->isValid () && newNode->isLeaf ());
            auto inner = std::static_pointer_cast<SHAMapInnerNode>(node);
            inner->setChild (b1, newNode);

            newNode = std::make_shared<SHAMapTreeNode> (otherItem, type, seq_);
            assert (newNode->isValid () && newNode->isLeaf ());
            inner->setChild (b2, newNode);
        }
    }

    dirtyUp (stack, tag, node);
    return true;
}

bool
SHAMap::addItem(SHAMapItem&& i, bool isTransaction, bool hasMetaData)
{
    return addGiveItem(std::make_shared<SHAMapItem const>(std::move(i)),
                                                          isTransaction, hasMetaData);
}

SHAMapHash
SHAMap::getHash () const
{
    auto hash = root_->getNodeHash();
    if (hash.isZero())
    {
        const_cast<SHAMap&>(*this).unshare();
        hash = root_->getNodeHash();
    }
    return hash;
}

bool
SHAMap::updateGiveItem (std::shared_ptr<SHAMapItem const> const& item,
                        bool isTransaction, bool hasMeta)
{
//无法更改标记，但可以更改哈希
    uint256 tag = item->key();

    assert (state_ != SHAMapState::Immutable);

    SharedPtrNodeStack stack;
    walkTowardsKey(tag, &stack);

    if (stack.empty ())
        Throw<SHAMapMissingNode> (type_, tag);

    auto node = std::dynamic_pointer_cast<SHAMapTreeNode>(stack.top().first);
    auto nodeID = stack.top ().second;
    stack.pop ();

    if (!node || (node->peekItem ()->key() != tag))
    {
        assert (false);
        return false;
    }

    node = unshareNode(std::move(node), nodeID);

    if (!node->setItem (item, !isTransaction ? SHAMapTreeNode::tnACCOUNT_STATE :
                        (hasMeta ? SHAMapTreeNode::tnTRANSACTION_MD : SHAMapTreeNode::tnTRANSACTION_NM)))
    {
        JLOG(journal_.trace()) <<
            "SHAMap setItem, no change";
        return true;
    }

    dirtyUp (stack, tag, node);
    return true;
}

bool SHAMap::fetchRoot (SHAMapHash const& hash, SHAMapSyncFilter* filter)
{
    if (hash == root_->getNodeHash ())
        return true;

    if (auto stream = journal_.trace())
    {
        if (type_ == SHAMapType::TRANSACTION)
        {
            stream
                << "Fetch root TXN node " << hash;
        }
        else if (type_ == SHAMapType::STATE)
        {
            stream <<
                "Fetch root STATE node " << hash;
        }
        else
        {
            stream <<
                "Fetch root SHAMap node " << hash;
        }
    }

    auto newRoot = fetchNodeNT (hash, filter);

    if (newRoot)
    {
        root_ = newRoot;
        assert (root_->getNodeHash () == hash);
        return true;
    }

    return false;
}

//用可共享节点替换节点。
//
//此代码处理两种情况：
//
//1）非共享、不可共享的节点需要成为可共享的
//所以不可变的shamap可以引用它。
//
//2）共享不可共享的节点。当你
//可变假动作的可变快照。
std::shared_ptr<SHAMapAbstractNode>
SHAMap::writeNode (
    NodeObjectType t, std::uint32_t seq, std::shared_ptr<SHAMapAbstractNode> node) const
{
//节点是我们的，所以我们可以让它共享
    assert (node->getSeq() == seq_);
    assert (backed_);
    node->setSeq (0);

    canonicalize (node->getNodeHash(), node);

    Serializer s;
    node->addRaw (s, snfPREFIX);
    f_.db().store (t, std::move (s.modData ()),
        node->getNodeHash ().as_uint256(), ledgerSeq_);
    return node;
}

//我们无法修改其他人可能拥有的内部节点
//指向的指针，因为刷新会修改内部节点--它
//使它们指向规范/共享节点。
template <class Node>
std::shared_ptr<Node>
SHAMap::preFlushNode (std::shared_ptr<Node> node) const
{
//共享节点不需要刷新
//因为这意味着有人修改了它
    assert (node->getSeq() != 0);

    if (node->getSeq() != seq_)
    {
//节点不是唯一的我们的节点，因此请在之前取消共享
//可能修改它
        node = std::static_pointer_cast<Node>(node->clone(seq_));
    }
    return node;
}

int SHAMap::unshare ()
{
//不与父映射共享节点
    return walkSubTree (false, hotUNKNOWN, 0);
}

/*将所有修改的节点转换为共享节点*/
//如果请求，将它们写入节点存储区
int SHAMap::flushDirty (NodeObjectType t, std::uint32_t seq)
{
    return walkSubTree (true, t, seq);
}

int
SHAMap::walkSubTree (bool doWrite, NodeObjectType t, std::uint32_t seq)
{
    int flushed = 0;
    Serializer s;

    if (!root_ || (root_->getSeq() == 0))
        return flushed;

    if (root_->isLeaf())
{ //特例——根是叶
        root_ = preFlushNode (std::move(root_));
        root_->updateHash();
        if (doWrite && backed_)
            root_ = writeNode(t, seq, std::move(root_));
        else
            root_->setSeq (0);
        return 1;
    }

    auto node = std::static_pointer_cast<SHAMapInnerNode>(root_);

    if (node->isEmpty ())
{ //用新的空根替换空根
        if (is_v2())
            root_ = std::make_shared<SHAMapInnerNodeV2>(0, 0);
        else
            root_ = std::make_shared<SHAMapInnerNode>(0);
        return 1;
    }

//表示父、索引、子指针的堆栈
//内部节点正在进行刷新
    using StackEntry = std::pair <std::shared_ptr<SHAMapInnerNode>, int>;
    std::stack <StackEntry, std::vector<StackEntry>> stack;

    node = preFlushNode(std::move(node));

    int pos = 0;

//在刷新内部节点的子节点之前，无法刷新该节点
    while (1)
    {
        while (pos < 16)
        {
            if (node->isEmptyBranch (pos))
            {
                ++pos;
            }
            else
            {
//无需进行I/O。如果节点未链接，
//不需要冲洗
                int branch = pos;
                auto child = node->getChild(pos++);

                if (child && (child->getSeq() != 0))
                {
//这是一个需要刷新的节点

                    child = preFlushNode(std::move(child));

                    if (child->isInner ())
                    {
//保存我们的位置并在此节点上工作

                        stack.emplace (std::move (node), branch);

                        node = std::static_pointer_cast<SHAMapInnerNode>(std::move(child));
                        pos = 0;
                    }
                    else
                    {
//冲洗这片叶子
                        ++flushed;

                        assert (node->getSeq() == seq_);
                        child->updateHash();

                        if (doWrite && backed_)
                            child = writeNode(t, seq, std::move(child));
                        else
                            child->setSeq (0);

                        node->shareChild (branch, child);
                    }
                }
            }
        }

//更新此内部节点的哈希
        node->updateHashDeep();

//现在可以共享此内部节点
        if (doWrite && backed_)
            node = std::static_pointer_cast<SHAMapInnerNode>(writeNode(t, seq,
                                                                       std::move(node)));
        else
            node->setSeq (0);

        ++flushed;

        if (stack.empty ())
           break;

        auto parent = std::move (stack.top().first);
        pos = stack.top().second;
        stack.pop();

//将此内部节点挂接到其父节点
        assert (parent->getSeq() == seq_);
        parent->shareChild (pos, node);

//继续父级的下一个子级（如果有）
        node = std::move (parent);
        ++pos;
    }

//最后一个内部节点是新的根节点
    root_ = std::move (node);

    return flushed;
}

void SHAMap::dump (bool hash) const
{
    int leafCount = 0;
    JLOG(journal_.info()) << " MAP Contains";

    std::stack <std::pair <SHAMapAbstractNode*, SHAMapNodeID> > stack;
    stack.push ({root_.get (), SHAMapNodeID ()});

    do
    {
        auto node = stack.top().first;
        auto nodeID = stack.top().second;
        stack.pop();

        JLOG(journal_.info()) << node->getString (nodeID);
        if (hash)
        {
           JLOG(journal_.info()) << "Hash: " << node->getNodeHash();
        }

        if (node->isInner ())
        {
            auto inner = static_cast<SHAMapInnerNode*>(node);
            for (int i = 0; i < 16; ++i)
            {
                if (!inner->isEmptyBranch (i))
                {
                    auto child = inner->getChildPointer (i);
                    if (child)
                    {
                        assert (child->getNodeHash() == inner->getChildHash (i));
                        stack.push ({child, nodeID.getChildNodeID (i)});
                     }
                }
            }
        }
        else
            ++leafCount;
    }
    while (!stack.empty ());

    JLOG(journal_.info()) << leafCount << " resident leaves";
}

std::shared_ptr<SHAMapAbstractNode> SHAMap::getCache (SHAMapHash const& hash) const
{
    auto ret = f_.treecache().fetch (hash.as_uint256());
    assert (!ret || !ret->getSeq());
    return ret;
}

void
SHAMap::canonicalize(SHAMapHash const& hash, std::shared_ptr<SHAMapAbstractNode>& node) const
{
    assert (backed_);
    assert (node->getSeq() == 0);
    assert (node->getNodeHash() == hash);

    f_.treecache().canonicalize (hash.as_uint256(), node);
}

SHAMap::version
SHAMap::get_version() const
{
    if (is_v2())
        return version{2};
    return version{1};
}

void
SHAMap::invariants() const
{
(void)getHash();  //更新节点哈希
    auto node = root_.get();
    assert(node != nullptr);
    assert(!node->isLeaf());
    SharedPtrNodeStack stack;
    for (auto leaf = peekFirstItem(stack); leaf != nullptr;
         leaf = peekNextItem(leaf->peekItem()->key(), stack))
        ;
    node->invariants(is_v2(), true);
}

bool
SHAMap::isInconsistentNode(std::shared_ptr<SHAMapAbstractNode> const& node) const
{
    assert(root_);
    assert(node);
    if (std::dynamic_pointer_cast<SHAMapTreeNode>(node) != nullptr)
        return false;
    bool is_node_v2 = std::dynamic_pointer_cast<SHAMapInnerNodeV2>(node) != nullptr;
    assert (! is_node_v2 || (std::dynamic_pointer_cast<SHAMapInnerNodeV2>(node)->depth() != 0));

    if (is_v2() == is_node_v2)
        return false;

    state_ = SHAMapState::Invalid;
    return true;
}

} //涟漪
