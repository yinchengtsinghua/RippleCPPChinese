
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

#ifndef RIPPLE_SHAMAP_SHAMAP_H_INCLUDED
#define RIPPLE_SHAMAP_SHAMAP_H_INCLUDED

#include <ripple/shamap/Family.h>
#include <ripple/shamap/FullBelowCache.h>
#include <ripple/shamap/SHAMapAddNode.h>
#include <ripple/shamap/SHAMapItem.h>
#include <ripple/shamap/SHAMapMissingNode.h>
#include <ripple/shamap/SHAMapNodeID.h>
#include <ripple/shamap/SHAMapSyncFilter.h>
#include <ripple/shamap/SHAMapTreeNode.h>
#include <ripple/shamap/TreeNodeCache.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/nodestore/Database.h>
#include <ripple/nodestore/NodeObject.h>
#include <ripple/beast/utility/Journal.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <cassert>
#include <stack>
#include <vector>

namespace ripple {

enum class SHAMapState
{
Modifying = 0,       //可以添加和删除对象（如未结分类帐）
Immutable = 1,       //无法更改映射（如已关闭的分类帐）
Synching  = 2,       //映射的哈希已锁定，可以添加有效节点（如对等方的结算分类帐）
Floating  = 3,       //映射可以自由更改哈希（就像同步打开的分类帐）
Invalid   = 4,       //已知映射无效（通常同步损坏的分类帐）
};

/*处理缺失节点的函数对象。*/
using MissingNodeHandler = std::function <void (std::uint32_t refNum)>;

/*沙马既是一棵有16个扇子的根树，又是一棵梅克尔树。

    基树是具有两个属性的树：

      1。节点的键由节点在树中的位置表示。
         （前缀属性）。
      2。只有一个子节点的节点将与该子节点合并
         （合并财产）

    对于根目录树，这些属性的内存占用要小得多。

    16个节点中有一个扇子意味着树中的每个节点最多有16个子节点。
    参见https://en.wikipedia.org/wiki/基数树

    Merkle树是一个树，其中每个非叶节点都用哈希标记。
    子节点的组合标签。

    Merkle树的一个关键属性是测试节点包含
    o（log（n）），其中n是树中的节点数。

    参见https://en.wikipedia.org/wiki/merkle_tree
 **/

class SHAMap
{
private:
    Family&                         f_;
    beast::Journal                  journal_;
    std::uint32_t                   seq_;
std::uint32_t                   ledgerSeq_ = 0; //这是分类帐的序列号的一部分
    std::shared_ptr<SHAMapAbstractNode> root_;
    mutable SHAMapState             state_;
    SHAMapType                      type_;
bool                            backed_ = true; //映射由数据库支持
bool                            full_ = false; //地图在数据库中被认为是完整的

public:
    class version
    {
        int v_;
    public:
        explicit version(int v) : v_(v) {}

        friend bool operator==(version const& x, version const& y)
            {return x.v_ == y.v_;}
        friend bool operator!=(version const& x, version const& y)
            {return !(x == y);}
    };

    using DeltaItem = std::pair<std::shared_ptr<SHAMapItem const>,
                                std::shared_ptr<SHAMapItem const>>;
    using Delta     = std::map<uint256, DeltaItem>;

    ~SHAMap ();
    SHAMap(SHAMap const&) = delete;
    SHAMap& operator=(SHAMap const&) = delete;

//新建地图
    SHAMap (
        SHAMapType t,
        Family& f,
        version v
        );

    SHAMap (
        SHAMapType t,
        uint256 const& hash,
        Family& f,
        version v);

    Family const&
    family() const
    {
        return f_;
    }

    Family&
    family()
    {
        return f_;
    }

//——————————————————————————————————————————————————————————————

    /*萨马普叶的迭代器
        这始终是一个常量迭代器。
        满足前进档的要求。
    **/

    class const_iterator;

    const_iterator begin() const;
    const_iterator end() const;

//——————————————————————————————————————————————————————————————

//返回此地图的快照。
//处理可变快照的写时复制。
    std::shared_ptr<SHAMap> snapShot (bool isMutable) const;

    /*将此shamap标记为“应满”，表示
        本地服务器需要所有相应的节点
        存放在耐用的地方。
    **/

    void setFull ();

    void setLedgerSeq (std::uint32_t lseq);

    bool fetchRoot (SHAMapHash const& hash, SHAMapSyncFilter * filter);

//普通哈希访问函数
    bool hasItem (uint256 const& id) const;
    bool delItem (uint256 const& id);
    bool addItem (SHAMapItem&& i, bool isTransaction, bool hasMeta);
    SHAMapHash getHash () const;

//如果仍有临时文件，请保存副本
    bool updateGiveItem (std::shared_ptr<SHAMapItem const> const&,
                         bool isTransaction, bool hasMeta);
    bool addGiveItem (std::shared_ptr<SHAMapItem const> const&,
                      bool isTransaction, bool hasMeta);

//如果需要延长寿命，请保存一份副本
//在这个骗局之外
    std::shared_ptr<SHAMapItem const> const& peekItem (uint256 const& id) const;
    std::shared_ptr<SHAMapItem const> const&
        peekItem (uint256 const& id, SHAMapHash& hash) const;
    std::shared_ptr<SHAMapItem const> const&
        peekItem (uint256 const& id, SHAMapTreeNode::TNType & type) const;

//导线测量功能
    const_iterator upper_bound(uint256 const& id) const;

    /*访问此shamap中的每个节点

         在访问每个节点时调用了@param函数。
         如果函数返回false，则退出visitnodes。
    **/

    void visitNodes (std::function<bool (
        SHAMapAbstractNode&)> const& function) const;

    /*访问这个shamap中的每个节点
         不在指定的shamap中

         在访问每个节点时调用了@param函数。
         如果函数返回false，则退出visitdifferences。
    **/

    void visitDifferences(SHAMap const* have,
        std::function<bool (SHAMapAbstractNode&)>) const;

    /*访问此shamap中的每个叶节点

         在访问每个非内部节点时调用了@param函数。
    **/

    void visitLeaves(std::function<void (
        std::shared_ptr<SHAMapItem const> const&)> const&) const;

//比较/同步功能

    /*检查shamap中的节点不可用

        高效地遍历shamap，最大化I/O
        并发性，以发现在
        Shamap，但不在本地提供。

        @param maxnodes要返回的找到的最大节点数
        @param筛选检索节点时要使用的筛选器
        @param返回已知丢失的节点
    **/

    std::vector<std::pair<SHAMapNodeID, uint256>>
    getMissingNodes (int maxNodes, SHAMapSyncFilter *filter);

    bool getNodeFat (SHAMapNodeID node,
        std::vector<SHAMapNodeID>& nodeIDs,
            std::vector<Blob>& rawNode,
                bool fatLeaves, std::uint32_t depth) const;

    bool getRootNode (Serializer & s, SHANodeFormat format) const;
    std::vector<uint256> getNeededHashes (int max, SHAMapSyncFilter * filter);
    SHAMapAddNode addRootNode (SHAMapHash const& hash, Slice const& rootNode,
                               SHANodeFormat format, SHAMapSyncFilter * filter);
    SHAMapAddNode addKnownNode (SHAMapNodeID const& nodeID, Slice const& rawNode,
                                SHAMapSyncFilter * filter);


//状态函数
    void setImmutable ();
    bool isSynching () const;
    void setSynching ();
    void clearSynching ();
    bool isValid () const;

//警告：只能通过此功能访问OtherMap
//返回值：真=成功完成，假=太不同
    bool compare (SHAMap const& otherMap,
                  Delta& differences, int maxCount) const;

    int flushDirty (NodeObjectType t, std::uint32_t seq);
    void walkMap (std::vector<SHAMapMissingNode>& missingNodes, int maxMissing) const;
bool deepCompare (SHAMap & other) const;  //仅用于调试/测试

    using fetchPackEntry_t = std::pair <uint256, Blob>;

    void getFetchPack (SHAMap const* have, bool includeLeaves, int max,
        std::function<void (SHAMapHash const&, const Blob&)>) const;

    void setUnbacked ();
    bool is_v2() const;
    version get_version() const;
    std::shared_ptr<SHAMap> make_v1() const;
    std::shared_ptr<SHAMap> make_v2() const;
    int unshare ();

    void dump (bool withHashes = false) const;
    void invariants() const;

private:
    using SharedPtrNodeStack =
        std::stack<std::pair<std::shared_ptr<SHAMapAbstractNode>, SHAMapNodeID>>;
    using DeltaRef = std::pair<std::shared_ptr<SHAMapItem const> const&,
                               std::shared_ptr<SHAMapItem const> const&>;

//树节点缓存操作
    std::shared_ptr<SHAMapAbstractNode> getCache (SHAMapHash const& hash) const;
    void canonicalize (SHAMapHash const& hash, std::shared_ptr<SHAMapAbstractNode>&) const;

//数据库操作
    std::shared_ptr<SHAMapAbstractNode> fetchNodeFromDB (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNodeNT (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNodeNT (
        SHAMapHash const& hash,
        SHAMapSyncFilter *filter) const;
    std::shared_ptr<SHAMapAbstractNode> fetchNode (SHAMapHash const& hash) const;
    std::shared_ptr<SHAMapAbstractNode> checkFilter(SHAMapHash const& hash,
        SHAMapSyncFilter* filter) const;

    /*将哈希更新到根目录*/
    void dirtyUp (SharedPtrNodeStack& stack,
                  uint256 const& target, std::shared_ptr<SHAMapAbstractNode> terminal);

    /*走向指定的ID，返回节点。呼叫方必须检查
        如果返回值为nullptr，如果不是，则返回值为node->peek item（）->key（）==id*/

    SHAMapTreeNode*
        walkTowardsKey(uint256 const& id, SharedPtrNodeStack* stack = nullptr) const;
    /*如果找不到键，则返回nullptr*/
    SHAMapTreeNode*
        findKey(uint256 const& id) const;

    /*取消共享节点，允许对其进行修改*/
    template <class Node>
        std::shared_ptr<Node>
        unshareNode(std::shared_ptr<Node>, SHAMapNodeID const& nodeID);

    /*刷新前准备要修改的节点*/
    template <class Node>
        std::shared_ptr<Node>
        preFlushNode(std::shared_ptr<Node> node) const;

    /*写入并规范化修改后的节点*/
    std::shared_ptr<SHAMapAbstractNode>
        writeNode(NodeObjectType t, std::uint32_t seq,
                  std::shared_ptr<SHAMapAbstractNode> node) const;

    SHAMapTreeNode* firstBelow (std::shared_ptr<SHAMapAbstractNode>,
                                SharedPtrNodeStack& stack, int branch = 0) const;

//简单下降
//获取指定节点的子节点
    SHAMapAbstractNode* descend (SHAMapInnerNode*, int branch) const;
    SHAMapAbstractNode* descendThrow (SHAMapInnerNode*, int branch) const;
    std::shared_ptr<SHAMapAbstractNode> descend (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;
    std::shared_ptr<SHAMapAbstractNode> descendThrow (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;

//用过滤器下降
    SHAMapAbstractNode* descendAsync (SHAMapInnerNode* parent, int branch,
        SHAMapSyncFilter* filter, bool& pending) const;

    std::pair <SHAMapAbstractNode*, SHAMapNodeID>
        descend (SHAMapInnerNode* parent, SHAMapNodeID const& parentID,
        int branch, SHAMapSyncFilter* filter) const;

//非储存
//不将返回的节点挂接到其父节点
    std::shared_ptr<SHAMapAbstractNode>
        descendNoStore (std::shared_ptr<SHAMapInnerNode> const&, int branch) const;

    /*如果此节点下只有一个叶，则获取其内容*/
    std::shared_ptr<SHAMapItem const> const& onlyBelow (SHAMapAbstractNode*) const;

    bool hasInnerNode (SHAMapNodeID const& nodeID, SHAMapHash const& hash) const;
    bool hasLeafNode (uint256 const& tag, SHAMapHash const& hash) const;

    SHAMapTreeNode const* peekFirstItem(SharedPtrNodeStack& stack) const;
    SHAMapTreeNode const* peekNextItem(uint256 const& id, SharedPtrNodeStack& stack) const;
    bool walkBranch (SHAMapAbstractNode* node,
                     std::shared_ptr<SHAMapItem const> const& otherMapItem,
                     bool isFirstMap, Delta & differences, int & maxCount) const;
    int walkSubTree (bool doWrite, NodeObjectType t, std::uint32_t seq);
    bool isInconsistentNode(std::shared_ptr<SHAMapAbstractNode> const& node) const;

//用于跟踪有关调用的信息的结构
//正在进行时获取MissingNodes
    struct MissingNodes
    {
        MissingNodes() = delete;
        MissingNodes(const MissingNodes&) = delete;
        MissingNodes& operator=(const MissingNodes&) = delete;

//基本参数
        int               max_;
        SHAMapSyncFilter* filter_;
        int const         maxDefer_;
        std::uint32_t     generation_;

//我们发现丢失的节点
        std::vector<std::pair<SHAMapNodeID, uint256>> missingNodes_;
        std::set <SHAMapHash>                         missingHashes_;

//我们正在遍历的节点
        using StackEntry = std::tuple<
SHAMapInnerNode*, //指向节点的指针
SHAMapNodeID,     //节点的ID
int,              //我们先检查孩子的时候
int,              //下一个我们检查哪个孩子
bool>;            //我们是否找到了失踪的孩子

//我们明确地选择在这里指定std:：deque的用法，因为
//我们需要确保指向现有元素的指针和/或引用
//不会在元素插入过程中失效，并且
//去除。不提供此保证的容器，例如
//std:：vector，这里不能使用。
        std::stack <StackEntry, std::deque<StackEntry>> stack_;

//我们可以从延迟读取中获取的节点
        std::vector <std::tuple <SHAMapInnerNode*, SHAMapNodeID, int>> deferredReads_;

//在从延迟读取中获取子节点后，我们需要恢复这些节点
        std::map<SHAMapInnerNode*, SHAMapNodeID> resumes_;

        MissingNodes (
            int max, SHAMapSyncFilter* filter,
            int maxDefer, std::uint32_t generation) :
                max_(max), filter_(filter),
                maxDefer_(maxDefer), generation_(generation)
        {
            missingNodes_.reserve (max);
            deferredReads_.reserve(maxDefer);
        }
    };

//GetMissingNodes帮助程序函数
    void gmn_ProcessNodes (MissingNodes&, MissingNodes::StackEntry& node);
    void gmn_ProcessDeferredReads (MissingNodes&);
};

inline
void
SHAMap::setFull ()
{
    full_ = true;
}

inline
void
SHAMap::setLedgerSeq (std::uint32_t lseq)
{
    ledgerSeq_ = lseq;
}

inline
void
SHAMap::setImmutable ()
{
    assert (state_ != SHAMapState::Invalid);
    state_ = SHAMapState::Immutable;
}

inline
bool
SHAMap::isSynching () const
{
    return (state_ == SHAMapState::Floating) || (state_ == SHAMapState::Synching);
}

inline
void
SHAMap::setSynching ()
{
    state_ = SHAMapState::Synching;
}

inline
void
SHAMap::clearSynching ()
{
    state_ = SHAMapState::Modifying;
}

inline
bool
SHAMap::isValid () const
{
    return state_ != SHAMapState::Invalid;
}

inline
void
SHAMap::setUnbacked ()
{
    backed_ = false;
}

inline
bool
SHAMap::is_v2() const
{
    assert (root_);
    return std::dynamic_pointer_cast<SHAMapInnerNodeV2>(root_) != nullptr;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class SHAMap::const_iterator
{
public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = SHAMapItem;
    using reference         = value_type const&;
    using pointer           = value_type const*;

private:
    SharedPtrNodeStack stack_;
    SHAMap const*      map_  = nullptr;
    pointer            item_ = nullptr;

public:
    const_iterator() = default;

    reference operator*()  const;
    pointer   operator->() const;

    const_iterator& operator++();
    const_iterator  operator++(int);

private:
    explicit const_iterator(SHAMap const* map);
    const_iterator(SHAMap const* map, pointer item);
    const_iterator(SHAMap const* map, pointer item, SharedPtrNodeStack&& stack);

    friend bool operator==(const_iterator const& x, const_iterator const& y);
    friend class SHAMap;
};

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map)
    : map_(map)
    , item_(nullptr)
{
    auto temp = map_->peekFirstItem(stack_);
    if (temp)
        item_ = temp->peekItem().get();
}

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map, pointer item)
    : map_(map)
    , item_(item)
{
}

inline
SHAMap::const_iterator::const_iterator(SHAMap const* map, pointer item,
                                       SharedPtrNodeStack&& stack)
    : stack_(std::move(stack))
    , map_(map)
    , item_(item)
{
}

inline
SHAMap::const_iterator::reference
SHAMap::const_iterator::operator*() const
{
    return *item_;
}

inline
SHAMap::const_iterator::pointer
SHAMap::const_iterator::operator->() const
{
    return item_;
}

inline
SHAMap::const_iterator&
SHAMap::const_iterator::operator++()
{
    auto temp = map_->peekNextItem(item_->key(), stack_);
    if (temp)
        item_ = temp->peekItem().get();
    else
        item_ = nullptr;
    return *this;
}

inline
SHAMap::const_iterator
SHAMap::const_iterator::operator++(int)
{
    auto tmp = *this;
    ++(*this);
    return tmp;
}

inline
bool
operator==(SHAMap::const_iterator const& x, SHAMap::const_iterator const& y)
{
    assert(x.map_ == y.map_);
    return x.item_ == y.item_;
}

inline
bool
operator!=(SHAMap::const_iterator const& x, SHAMap::const_iterator const& y)
{
    return !(x == y);
}

inline
SHAMap::const_iterator
SHAMap::begin() const
{
    return const_iterator(this);
}

inline
SHAMap::const_iterator
SHAMap::end() const
{
    return const_iterator(this, nullptr);
}

}

#endif
