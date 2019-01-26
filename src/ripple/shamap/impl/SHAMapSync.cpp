
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

#include <ripple/basics/random.h>
#include <ripple/shamap/SHAMap.h>
#include <ripple/nodestore/Database.h>

namespace ripple {

void
SHAMap::visitLeaves(std::function<void (
    std::shared_ptr<SHAMapItem const> const& item)> const& leafFunction) const
{
    visitNodes(
        [&leafFunction](SHAMapAbstractNode& node)
        {
            if (! node.isInner())
                leafFunction(static_cast<SHAMapTreeNode&>(node).peekItem());
            return true;
        });
}

void
SHAMap::visitNodes(std::function<bool (
    SHAMapAbstractNode&)> const& function) const
{
//访问shamap中的每个节点
    assert (root_->isValid ());

    if (! root_)
        return;

    function (*root_);

    if (! root_->isInner ())
        return;

    using StackEntry = std::pair <int, std::shared_ptr<SHAMapInnerNode>>;
    std::stack <StackEntry, std::vector <StackEntry>> stack;

    auto node = std::static_pointer_cast<SHAMapInnerNode>(root_);
    int pos = 0;

    while (1)
    {
        while (pos < 16)
        {
            uint256 childHash;
            if (! node->isEmptyBranch (pos))
            {
                std::shared_ptr<SHAMapAbstractNode> child = descendNoStore (node, pos);
                if (! function (*child))
                    return;

                if (child->isLeaf ())
                    ++pos;
                else
                {
//如果没有更多的子节点，不要推这个节点
                    while ((pos != 15) && (node->isEmptyBranch (pos + 1)))
                           ++pos;

                    if (pos != 15)
                    {
//保存下一个位置以继续
                        stack.push (std::make_pair(pos + 1, std::move (node)));
                    }

//下降到孩子的第一个位置
                    node = std::static_pointer_cast<SHAMapInnerNode>(child);
                    pos = 0;
                }
            }
            else
            {
++pos; //移到下一个位置
            }
        }

        if (stack.empty ())
            break;

        std::tie(pos, node) = stack.top ();
        stack.pop ();
    }
}

void
SHAMap::visitDifferences(SHAMap const* have,
    std::function<bool (SHAMapAbstractNode&)> function) const
{
//访问此shamap中不存在的每个节点
//在指定的shamap中
    assert (root_->isValid ());

    if (! root_)
        return;

    if (root_->getNodeHash ().isZero ())
        return;

    if (have && (root_->getNodeHash () == have->root_->getNodeHash ()))
        return;

    if (root_->isLeaf ())
    {
        auto leaf = std::static_pointer_cast<SHAMapTreeNode>(root_);
        if (! have || ! have->hasLeafNode(leaf->peekItem()->key(), leaf->getNodeHash()))
            function (*root_);
        return;
    }
//包含未发掘的不匹配内部节点项
    using StackEntry = std::pair <SHAMapInnerNode*, SHAMapNodeID>;
    std::stack <StackEntry, std::vector<StackEntry>> stack;

    stack.push ({static_cast<SHAMapInnerNode*>(root_.get()), SHAMapNodeID{}});

    while (! stack.empty())
    {
        SHAMapInnerNode* node;
        SHAMapNodeID nodeID;
        std::tie (node, nodeID) = stack.top ();
        stack.pop ();

//1）将此节点添加到包中
        if (! function (*node))
            return;

//2）推送不匹配的子内部节点
        for (int i = 0; i < 16; ++i)
        {
            if (! node->isEmptyBranch (i))
            {
                auto const& childHash = node->getChildHash (i);
                SHAMapNodeID childID = nodeID.getChildNodeID (i);
                auto next = descendThrow(node, i);

                if (next->isInner ())
                {
                    if (! have || ! have->hasInnerNode(childID, childHash))
                        stack.push ({static_cast<SHAMapInnerNode*>(next), childID});
                }
                else if (! have || ! have->hasLeafNode(
                         static_cast<SHAMapTreeNode*>(next)->peekItem()->key(),
                         childHash))
                {
                    if (! function (*next))
                        return;
                }
            }
        }
    }
}

//从规范所指位置开始
//stackentry，处理该节点及其第一个驻留节点
//孩子们，下山直到我们完成
//处理一个节点。
void SHAMap::gmn_ProcessNodes (MissingNodes& mn, MissingNodes::StackEntry& se)
{
    SHAMapInnerNode*& node = std::get<0>(se);
    SHAMapNodeID&     nodeID = std::get<1>(se);
    int&              firstChild = std::get<2>(se);
    int&              currentChild = std::get<3>(se);
    bool&             fullBelow = std::get<4>(se);

    while (currentChild < 16)
    {
        int branch = (firstChild + currentChild++) % 16;
        if (node->isEmptyBranch (branch))
            continue;

        auto const& childHash = node->getChildHash (branch);

        if (mn.missingHashes_.count (childHash) != 0)
        {
//我们已经知道这个子节点丢失了
            fullBelow = false;
        }
        else if (! backed_ || ! f_.fullbelow().touch_if_exists (childHash.as_uint256()))
        {
            SHAMapNodeID childID = nodeID.getChildNodeID (branch);
            bool pending = false;
            auto d = descendAsync (node, branch, mn.filter_, pending);

            if (!d)
            {
fullBelow = false; //目前还不清楚

                if (! pending)
{ //节点不在数据库中
                    mn.missingHashes_.insert (childHash);
                    mn.missingNodes_.emplace_back (
                        childID, childHash.as_uint256());

                    if (--mn.max_ <= 0)
                        return;
                }
                else
                    mn.deferredReads_.emplace_back (node, nodeID, branch);
            }
            else if (d->isInner() &&
                 ! static_cast<SHAMapInnerNode*>(d)->isFullBelow(mn.generation_))
            {
                mn.stack_.push (se);

//切换到处理子节点
                node = static_cast<SHAMapInnerNode*>(d);
                if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(node))
                    nodeID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
                else
                    nodeID = childID;
                firstChild = rand_int(255);
                currentChild = 0;
                fullBelow = true;
            }
        }
    }

//我们已经处理完一个内部节点
//因此（现在）它所有的孩子

    if (fullBelow)
{ //在此节点下未遇到部分节点
        node->setFullBelowGen (mn.generation_);
        if (backed_)
            f_.fullbelow().insert (node->getNodeHash ().as_uint256());
    }

    node = nullptr;
}

//等待延迟读取完成，然后
//处理他们的结果
void SHAMap::gmn_ProcessDeferredReads (MissingNodes& mn)
{
//等待延迟读取完成
    auto const before = std::chrono::steady_clock::now();
    f_.db().waitReads();
    auto const after = std::chrono::steady_clock::now();

    auto const elapsed = std::chrono::duration_cast
        <std::chrono::milliseconds> (after - before);
    auto const count = mn.deferredReads_.size ();

//处理所有延迟读取
    int hits = 0;
    for (auto const& deferredNode : mn.deferredReads_)
    {
        auto parent = std::get<0>(deferredNode);
        auto const& parentID = std::get<1>(deferredNode);
        auto branch = std::get<2>(deferredNode);
        auto const& nodeHash = parent->getChildHash (branch);

        auto nodePtr = fetchNodeNT(nodeHash, mn.filter_);
        if (nodePtr)
{ //得到节点
            ++hits;
            if (backed_)
                canonicalize (nodeHash, nodePtr);
            nodePtr = parent->canonicalizeChild (branch, std::move(nodePtr));

//当我们完成这个堆栈时，我们需要重新启动
//与此节点的父级
            mn.resumes_[parent] = parentID;
        }
        else if ((mn.max_ > 0) &&
            (mn.missingHashes_.insert (nodeHash).second))
        {
            mn.missingNodes_.emplace_back (
                parentID.getChildNodeID (branch),
                nodeHash.as_uint256());

            --mn.max_;
        }
    }
    mn.deferredReads_.clear();

    auto const process_time = std::chrono::duration_cast
        <std::chrono::milliseconds> (std::chrono::steady_clock::now() - after);

    using namespace std::chrono_literals;
    if ((count > 50) || (elapsed > 50ms))
    {
        JLOG(journal_.debug()) << "getMissingNodes reads " <<
            count << " nodes (" << hits << " hits) in "
            << elapsed.count() << " + " << process_time.count()  << " ms";
    }
}

/*获取属于此shamap的节点的节点ID和哈希列表
    但在本地不可用。过滤器可以容纳
    不是永久存储在本地的节点
**/

std::vector<std::pair<SHAMapNodeID, uint256>>
SHAMap::getMissingNodes(int max, SHAMapSyncFilter* filter)
{
    assert (root_->isValid ());
    assert (root_->getNodeHash().isNonZero ());
    assert (max > 0);

    MissingNodes mn (max, filter,
        f_.db().getDesiredAsyncReadCount(ledgerSeq_),
        f_.fullbelow().getGeneration());

    if (! root_->isInner () ||
            std::static_pointer_cast<SHAMapInnerNode>(root_)->
                isFullBelow (mn.generation_))
    {
        clearSynching ();
        return std::move (mn.missingNodes_);
    }

//从根开始。
//第一个子值是随机选择的，因此如果多个线程
//正在遍历映射，每个线程将从不同的
//（随机选择）内部节点。这增加了可能性
//两个线程将产生不同的请求集（即
//比发送相同的请求更有效）。
    MissingNodes::StackEntry pos {
        static_cast<SHAMapInnerNode*>(root_.get()), SHAMapNodeID(),
        rand_int(255), 0, true };
    auto& node = std::get<0>(pos);
    auto& nextChild = std::get<3>(pos);
    auto& fullBelow = std::get<4>(pos);

//在不阻塞的情况下遍历地图
    do
    {

        while ((node != nullptr) &&
            (mn.deferredReads_.size() <= mn.maxDefer_))
        {
            gmn_ProcessNodes (mn, pos);

            if (mn.max_ <= 0)
                return std::move(mn.missingNodes_);

            if ((node == nullptr) && ! mn.stack_.empty ())
            {
//从这个节点的父节点离开的位置开始
bool was = fullBelow; //全部填满

                pos = mn.stack_.top();
                mn.stack_.pop ();
                if (nextChild == 0)
                {
//这是我们第一次处理的节点
                    fullBelow = true;
                }
                else
                {
//这是我们正在继续处理的节点
fullBelow = fullBelow && was; //过去和现在
                }
                assert (node);
            }
        }

//我们要么清空了烟囱，要么
//发布尽可能多的延迟读取

        if (! mn.deferredReads_.empty ())
            gmn_ProcessDeferredReads(mn);

        if (mn.max_ <= 0)
            return std::move(mn.missingNodes_);

        if (node == nullptr)
{ //我们没有在处理节点的过程中

            if (mn.stack_.empty() && ! mn.resumes_.empty())
            {
//重新检查之前无法完成的节点
                for (auto& r : mn.resumes_)
                    if (! r.first->isFullBelow (mn.generation_))
                        mn.stack_.push (std::make_tuple (
                            r.first, r.second, rand_int(255), 0, true));

                mn.resumes_.clear();
            }

            if (! mn.stack_.empty())
            {
//在堆栈顶部继续
                pos = mn.stack_.top();
                mn.stack_.pop();
                assert (node != nullptr);
            }
        }

//只有当
//我们完成了当前节点，堆栈是空的
//我们没有节点可以恢复

    } while (node != nullptr);

    if (mn.missingNodes_.empty ())
        clearSynching ();

    return std::move(mn.missingNodes_);
}

std::vector<uint256> SHAMap::getNeededHashes (int max, SHAMapSyncFilter* filter)
{
    auto ret = getMissingNodes(max, filter);

    std::vector<uint256> hashes;
    hashes.reserve (ret.size());

    for (auto const& n : ret)
        hashes.push_back (n.second);

    return hashes;
}

bool SHAMap::getNodeFat (SHAMapNodeID wanted,
    std::vector<SHAMapNodeID>& nodeIDs,
        std::vector<Blob>& rawNodes, bool fatLeaves,
            std::uint32_t depth) const
{
//获取一个节点及其部分子节点
//达到指定深度

    auto node = root_.get();
    SHAMapNodeID nodeID;

    while (node && node->isInner () && (nodeID.getDepth() < wanted.getDepth()))
    {
        int branch = nodeID.selectBranch (wanted.getNodeID());
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
            return false;

        node = descendThrow(inner, branch);
        if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(node))
            nodeID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
        else
            nodeID = nodeID.getChildNodeID (branch);
    }

    if (node == nullptr ||
           (dynamic_cast<SHAMapInnerNodeV2*>(node) != nullptr &&
                !wanted.has_common_prefix(nodeID)) ||
           (dynamic_cast<SHAMapInnerNodeV2*>(node) == nullptr && wanted != nodeID))
    {
        JLOG(journal_.warn())
            << "peer requested node that is not in the map:\n"
            << wanted << " but found\n" << nodeID;
        return false;
    }

    if (node->isInner() && static_cast<SHAMapInnerNode*>(node)->isEmpty())
    {
        JLOG(journal_.warn()) << "peer requests empty node";
        return false;
    }

    std::stack<std::tuple <SHAMapAbstractNode*, SHAMapNodeID, int>> stack;
    stack.emplace (node, nodeID, depth);

    while (! stack.empty ())
    {
        std::tie (node, nodeID, depth) = stack.top ();
        stack.pop ();

//将此节点添加到答复
        Serializer s;
        node->addRaw (s, snfWIRE);
        nodeIDs.push_back (nodeID);
        rawNodes.push_back (std::move (s.peekData()));

        if (node->isInner())
        {
//我们只带着一个子节点下降内部节点
//不降低深度
            auto inner = static_cast<SHAMapInnerNode*>(node);
            int bc = inner->getBranchCount();
            if ((depth > 0) || (bc == 1))
            {
//我们需要处理这个节点的子节点
                for (int i = 0; i < 16; ++i)
                {
                    if (! inner->isEmptyBranch (i))
                    {
                        auto childNode = descendThrow (inner, i);
                        SHAMapNodeID childID;
                        if (auto v2Node = dynamic_cast<SHAMapInnerNodeV2*>(childNode))
                            childID = SHAMapNodeID{v2Node->depth(), v2Node->key()};
                        else
                            childID = nodeID.getChildNodeID (i);

                        if (childNode->isInner () &&
                            ((depth > 1) || (bc == 1)))
                        {
//如果有不止一个孩子，减少深度
//如果只有一个孩子，就跟着链子走
                            stack.emplace (childNode, childID,
                                (bc > 1) ? (depth - 1) : depth);
                        }
                        else if (childNode->isInner() || fatLeaves)
                        {
//只包括这个节点
                            Serializer ns;
                            childNode->addRaw (ns, snfWIRE);
                            nodeIDs.push_back (childID);
                            rawNodes.push_back (std::move (ns.peekData ()));
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool SHAMap::getRootNode (Serializer& s, SHANodeFormat format) const
{
    root_->addRaw (s, format);
    return true;
}

SHAMapAddNode SHAMap::addRootNode (SHAMapHash const& hash, Slice const& rootNode, SHANodeFormat format,
                                   SHAMapSyncFilter* filter)
{
//我们已经有了一个根节点
    if (root_->getNodeHash ().isNonZero ())
    {
        JLOG(journal_.trace()) << "got root node, already have one";
        assert (root_->getNodeHash () == hash);
        return SHAMapAddNode::duplicate ();
    }

    assert (seq_ >= 1);
    auto node = SHAMapAbstractNode::make(
        rootNode, 0, format, SHAMapHash{}, false, f_.journal ());
    if (!node || !node->isValid() || node->getNodeHash () != hash)
        return SHAMapAddNode::invalid ();

    if (backed_)
        canonicalize (hash, node);

    root_ = node;

    if (root_->isLeaf())
        clearSynching ();

    if (filter)
    {
        Serializer s;
        root_->addRaw (s, snfPREFIX);
        filter->gotNode (false, root_->getNodeHash (), ledgerSeq_,
                         std::move(s.modData ()), root_->getType ());
    }

    return SHAMapAddNode::useful ();
}

SHAMapAddNode
SHAMap::addKnownNode (const SHAMapNodeID& node, Slice const& rawNode,
                      SHAMapSyncFilter* filter)
{
//返回值：真=好，假=错误
    assert (!node.isRoot ());

    if (!isSynching ())
    {
        JLOG(journal_.trace()) << "AddKnownNode while not synching";
        return SHAMapAddNode::duplicate ();
    }

    std::uint32_t generation = f_.fullbelow().getGeneration();
    auto newNode = SHAMapAbstractNode::make(rawNode, 0, snfWIRE,
                      SHAMapHash{}, false, f_.journal(), node);
    SHAMapNodeID iNodeID;
    auto iNode = root_.get();

    while (iNode->isInner () &&
           !static_cast<SHAMapInnerNode*>(iNode)->isFullBelow(generation) &&
           (iNodeID.getDepth () < node.getDepth ()))
    {
        int branch = iNodeID.selectBranch (node.getNodeID ());
        assert (branch >= 0);
        auto inner = static_cast<SHAMapInnerNode*>(iNode);
        if (inner->isEmptyBranch (branch))
        {
            JLOG(journal_.warn()) << "Add known node for empty branch" << node;
            return SHAMapAddNode::invalid ();
        }

        auto childHash = inner->getChildHash (branch);
        if (f_.fullbelow().touch_if_exists (childHash.as_uint256()))
            return SHAMapAddNode::duplicate ();

        auto prevNode = inner;
        std::tie(iNode, iNodeID) = descend(inner, iNodeID, branch, filter);

        if (iNode == nullptr)
        {
            if (!newNode || !newNode->isValid() || childHash != newNode->getNodeHash ())
            {
                JLOG(journal_.warn()) << "Corrupt node received";
                return SHAMapAddNode::invalid ();
            }

            if (!newNode->isInBounds (iNodeID))
            {
//地图可以证明是无效的
                state_ = SHAMapState::Invalid;
                return SHAMapAddNode::useful ();
            }

            if (newNode && isInconsistentNode(newNode))
            {
                state_ = SHAMapState::Invalid;
                return SHAMapAddNode::useful();
            }

            if ((std::dynamic_pointer_cast<SHAMapInnerNodeV2>(newNode) && !iNodeID.has_common_prefix(node)) ||
               (!std::dynamic_pointer_cast<SHAMapInnerNodeV2>(newNode) && iNodeID != node))
            {
//此节点已损坏或我们尚未请求它
                JLOG(journal_.warn()) << "unable to hook node " << node;
                JLOG(journal_.info()) << " stuck at " << iNodeID;
                JLOG(journal_.info()) <<
                    "got depth=" << node.getDepth () <<
                        ", walked to= " << iNodeID.getDepth ();
                return SHAMapAddNode::useful ();
            }

            if (backed_)
                canonicalize (childHash, newNode);

            newNode = prevNode->canonicalizeChild (branch, std::move(newNode));

            if (filter)
            {
                Serializer s;
                newNode->addRaw (s, snfPREFIX);
                filter->gotNode (false, childHash, ledgerSeq_,
                                 std::move(s.modData ()), newNode->getType ());
            }

            return SHAMapAddNode::useful ();
        }
    }

    JLOG(journal_.trace()) << "got node, already had it (late)";
    return SHAMapAddNode::duplicate ();
}

bool SHAMap::deepCompare (SHAMap& other) const
{
//仅用于调试/测试
    std::stack <std::pair <SHAMapAbstractNode*, SHAMapAbstractNode*> > stack;

    stack.push ({root_.get(), other.root_.get()});

    while (!stack.empty ())
    {
        SHAMapAbstractNode* node;
        SHAMapAbstractNode* otherNode;
        std::tie(node, otherNode) = stack.top ();
        stack.pop ();

        if (!node || !otherNode)
        {
            JLOG(journal_.info()) << "unable to fetch node";
            return false;
        }
        else if (otherNode->getNodeHash () != node->getNodeHash ())
        {
            JLOG(journal_.warn()) << "node hash mismatch";
            return false;
        }

        if (node->isLeaf ())
        {
            if (!otherNode->isLeaf ())
                 return false;
            auto& nodePeek = static_cast<SHAMapTreeNode*>(node)->peekItem();
            auto& otherNodePeek = static_cast<SHAMapTreeNode*>(otherNode)->peekItem();
            if (nodePeek->key() != otherNodePeek->key())
                return false;
            if (nodePeek->peekData() != otherNodePeek->peekData())
                return false;
        }
        else if (node->isInner ())
        {
            if (!otherNode->isInner ())
                return false;
            auto node_inner = static_cast<SHAMapInnerNode*>(node);
            auto other_inner = static_cast<SHAMapInnerNode*>(otherNode);
            for (int i = 0; i < 16; ++i)
            {
                if (node_inner->isEmptyBranch (i))
                {
                    if (!other_inner->isEmptyBranch (i))
                        return false;
                }
                else
                {
                    if (other_inner->isEmptyBranch (i))
                       return false;

                    auto next = descend(node_inner, i);
                    auto otherNext = other.descend(other_inner, i);
                    if (!next || !otherNext)
                    {
                        JLOG(journal_.warn()) << "unable to fetch inner node";
                        return false;
                    }
                    stack.push ({next, otherNext});
                }
            }
        }
    }

    return true;
}

/*这个地图有这个内部节点吗？
**/

bool
SHAMap::hasInnerNode (SHAMapNodeID const& targetNodeID,
                      SHAMapHash const& targetNodeHash) const
{
    auto node = root_.get();
    SHAMapNodeID nodeID;

    while (node->isInner () && (nodeID.getDepth () < targetNodeID.getDepth ()))
    {
        int branch = nodeID.selectBranch (targetNodeID.getNodeID ());
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
            return false;

        node = descendThrow (inner, branch);
        nodeID = nodeID.getChildNodeID (branch);
    }

    return (node->isInner()) && (node->getNodeHash() == targetNodeHash);
}

/*这个地图有这个叶节点吗？
**/

bool
SHAMap::hasLeafNode (uint256 const& tag, SHAMapHash const& targetNodeHash) const
{
    auto node = root_.get();
    SHAMapNodeID nodeID;

if (!node->isInner()) //树中只有一个叶节点
        return node->getNodeHash() == targetNodeHash;

    do
    {
        int branch = nodeID.selectBranch (tag);
        auto inner = static_cast<SHAMapInnerNode*>(node);
        if (inner->isEmptyBranch (branch))
return false;   //死角，节点不能在这里

if (inner->getChildHash (branch) == targetNodeHash) //匹配叶，无需检索
            return true;

        node = descendThrow(inner, branch);
        nodeID = nodeID.getChildNodeID (branch);
    }
    while (node->isInner());

return false; //如果这是一片相配的叶子，我们早就抓住了
}

/*
@param有一个指向收件人已经拥有的映射的指针（如果有的话）。
如果应该包括叶节点，@param includeleaves true。
@param max返回的最大节点数。
@param func为添加到fetchpack的每个节点调用的函数。

注意：对于事务树，调用者应该将includeLeaves设置为false。
包含事务树的叶子是没有意义的。
**/

void SHAMap::getFetchPack (SHAMap const* have, bool includeLeaves, int max,
                           std::function<void (SHAMapHash const&, const Blob&)> func) const
{
    if (have != nullptr && have->is_v2() != is_v2())
    {
        JLOG(journal_.info()) << "Can not get fetch pack when versions are different.";
        return;
    }
    visitDifferences (have,
        [includeLeaves, &max, &func] (SHAMapAbstractNode& smn) -> bool
        {
            if (includeLeaves || smn.isInner ())
            {
                Serializer s;
                smn.addRaw (s, snfPREFIX);
                func (smn.getNodeHash(), s.peekData());

                if (--max <= 0)
                    return false;
            }
            return true;
        });
}

} //涟漪
