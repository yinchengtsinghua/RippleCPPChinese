
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
    版权所有（c）2017 Ripple Labs Inc.

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

#ifndef RIPPLE_APP_CONSENSUS_LEDGERS_TRIE_H_INCLUDED
#define RIPPLE_APP_CONSENSUS_LEDGERS_TRIE_H_INCLUDED

#include <ripple/json/json_value.h>
#include <boost/optional.hpp>
#include <algorithm>
#include <memory>
#include <sstream>
#include <stack>
#include <vector>

namespace ripple {

/*分类帐血统跨度的尖端
 **/

template <class Ledger>
class SpanTip
{
public:
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

    SpanTip(Seq s, ID i, Ledger const lgr)
        : seq{s}, id{i}, ledger{std::move(lgr)}
    {
    }

//小费分类帐的序列号
    Seq seq;
//小费分类帐的ID
    ID id;

    /*查找提示分类帐的上级的ID

        @param s祖先的序列号
        @返回具有该序列号的祖先ID

        @注释s必须小于或等于
              小费分类帐
    **/

    ID
    ancestor(Seq const& s) const
    {
        assert(s <= seq);
        return ledger[s];
    }

private:
    Ledger const ledger;
};

namespace ledger_trie_detail {

//表示分类帐的祖先范围
template <class Ledger>
class Span
{
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

//跨度是分类帐的半开放时间间隔[开始，结束]
    Seq start_{0};
    Seq end_{1};
    Ledger ledger_;

public:
    Span() : ledger_{typename Ledger::MakeGenesis{}}
    {
//要求默认分类帐为Genesis Seq
        assert(ledger_.seq() == start_);
    }

    Span(Ledger ledger)
        : start_{0}, end_{ledger.seq() + Seq{1}}, ledger_{std::move(ledger)}
    {
    }

    Span(Span const& s) = default;
    Span(Span&& s) = default;
    Span&
    operator=(Span const&) = default;
    Span&
    operator=(Span&&) = default;

    Seq
    start() const
    {
        return start_;
    }

    Seq
    end() const
    {
        return end_;
    }

//从[spot，end_uu]返回跨度，如果没有有效的跨度，则返回无。
    boost::optional<Span>
    from(Seq spot) const
    {
        return sub(spot, end_);
    }

//从[开始点]返回跨度，如果没有有效的跨度，则返回无。
    boost::optional<Span>
    before(Seq spot) const
    {
        return sub(start_, spot);
    }

//返回开始此范围的分类帐的ID
    ID
    startID() const
    {
        return ledger_[start_];
    }

//返回第一个可能差异的分类帐序列号
//在这个范围和一个给定的分类帐之间。
    Seq
    diff(Ledger const& o) const
    {
        return clamp(mismatch(ledger_, o));
    }

//这个跨度的尖端
    SpanTip<Ledger>
    tip() const
    {
        Seq tipSeq{end_ - Seq{1}};
        return SpanTip<Ledger>{tipSeq, ledger_[tipSeq], ledger_};
    }

private:
    Span(Seq start, Seq end, Ledger const& l)
        : start_{start}, end_{end}, ledger_{l}
    {
//跨度不能为空
        assert(start < end);
    }

    Seq
    clamp(Seq val) const
    {
        return std::min(std::max(start_, val), end_);
    }

//在半开放时间间隔内返回此范围[从、到]
    boost::optional<Span>
    sub(Seq from, Seq to) const
    {
        Seq newFrom = clamp(from);
        Seq newTo = clamp(to);
        if (newFrom < newTo)
            return Span(newFrom, newTo, ledger_);
        return boost::none;
    }

    friend std::ostream&
    operator<<(std::ostream& o, Span const& s)
    {
        return o << s.tip().id << "[" << s.start_ << "," << s.end_ << ")";
    }

    friend Span
    merge(Span const& a, Span const& b)
    {
//返回组合跨度，使用较高序列跨度中的分类帐
        if (a.end_ < b.end_)
            return Span(std::min(a.start_, b.start_), b.end_, b.ledger_);

        return Span(std::min(a.start_, b.start_), a.end_, a.ledger_);
    }
};

//trie中的节点
template <class Ledger>
struct Node
{
    Node() = default;

    explicit Node(Ledger const& l) : span{l}, tipSupport{1}, branchSupport{1}
    {
    }

    explicit Node(Span<Ledger> s) : span{std::move(s)}
    {
    }

    Span<Ledger> span;
    std::uint32_t tipSupport = 0;
    std::uint32_t branchSupport = 0;

    std::vector<std::unique_ptr<Node>> children;
    Node* parent = nullptr;

    /*从此节点的子节点中删除给定节点

        @param child要删除的子节点的地址
        @注意，子元素必须是向量的成员。传递的指针
              将由于此呼叫而挂起
    **/

    void
    erase(Node const* child)
    {
        auto it = std::find_if(
            children.begin(),
            children.end(),
            [child](std::unique_ptr<Node> const& curr) {
                return curr.get() == child;
            });
        assert(it != children.end());
        std::swap(*it, children.back());
        children.pop_back();
    }

    friend std::ostream&
    operator<<(std::ostream& o, Node const& s)
    {
        return o << s.span << "(T:" << s.tipSupport << ",B:" << s.branchSupport
                 << ")";
    }

    Json::Value
    getJson() const
    {
        Json::Value res;
        std::stringstream sps;
        sps << span;
        res["span"] = sps.str();
        res["startID"] = to_string(span.startID());
        res["seq"] = static_cast<std::uint32_t>(span.tip().seq);
        res["tipSupport"] = tipSupport;
        res["branchSupport"] = branchSupport;
        if (!children.empty())
        {
            Json::Value& cs = (res["children"] = Json::arrayValue);
            for (auto const& child : children)
            {
                cs.append(child->getJson());
            }
        }
        return res;
    }
};
}  //命名空间分类帐详细信息

/*历代志

    一个压缩的trie树，用于维护最近分类账的验证支持。
    基于他们的祖先。

    压缩的trie结构来自于识别分类帐历史
    可以作为分类帐ID字母表上的字符串查看。也就是说，
    具有序列号“seq”的给定分类帐定义长度“seq”字符串，
    第i个条目等于具有序列的上级分类账的ID
    数字I：带有共同前缀的“sequence”字符串共享这些祖先
    共同的分类帐。跟踪这个祖先的信息和关系
    所有已验证的分类账都可以在压缩的trie中方便地完成。一个节点
    特里亚人是其所有子女的祖先。如果父节点具有序列
    编号“seq”，每个子节点都有一个不同的分类账，从“seq+1”开始。
    压缩来自一个不变量，即任何具有0 tip的非根节点
    支持没有子项或有多个子项。换句话说，a
    非根0-tip支持节点可以与其单个子节点组合。

    每个节点都有一个tipsupport，它是
    那个特别的分类帐。节点的分支支持是提示的总和
    该节点子节点的支持和分支支持：

        @代码
        node->branchsupport=node->tipsupport；
        for（子：节点->子）
           node->branchsupport+=child->branchsupport；
        @端码

    模板化分类帐类型表示具有唯一历史记录的分类帐。
    它应该是轻量级和廉价的复制。

       @代码
       //应相等、可比较且可复制的标识符类型
       结构ID；
       结构；

       Ledger结构
       {
          结构制造

          //Genesis分类帐表示在所有其他分类帐之前的分类帐
          /分类帐
          分类帐（MakeGenesis）；

          分类帐（分类帐构造和）；
          分类帐和操作员=（分类帐构造和）；

          //返回该分类账的序号
          seq seq（）常量；

          //返回具有给定序列号的该分类帐祖先的ID
          //或ID 0如果未知
          身份证件
          操作员[]（顺序）；

       }；

       //返回第一个可能不匹配祖先的序列号
       //在两个分类帐之间
       SEQ
       不匹配（Ledgera、Ledgerb）；
       @端码

    分类帐的唯一历史不变量要求任何同意的分类帐
    在给定序列号的ID上，同意之前的所有祖先
    分类帐：

        @代码
        Ledger A，B；
        //对于所有序列：
        如果（a[s]==b[s]）；
            对于（seq p=0；p<s；+p）
                断言（a[p]==b[p]）；
        @端码

    @tparam ledger表示分类帐及其历史记录的类型
**/

template <class Ledger>
class LedgerTrie
{
    using Seq = typename Ledger::Seq;
    using ID = typename Ledger::ID;

    using Node = ledger_trie_detail::Node<Ledger>;
    using Span = ledger_trie_detail::Span<Ledger>;

//trie的根。允许根破坏no-single子级
//不变量。
    std::unique_ptr<Node> root;

//每个序列号的提示支持计数
    std::map<Seq, std::uint32_t> seqSupport;

    /*在trie中查找代表最长共同祖先的节点
        与给定的分类帐。

        找到的节点的@返回对和第一个节点的序列号
                分类帐差异。
    **/

    std::pair<Node*, Seq>
    find(Ledger const& ledger) const
    {
        Node* curr = root.get();

//根总是被定义的，并且与所有分类帐都是相同的。
        assert(curr);
        Seq pos = curr->span.diff(ledger);

        bool done = false;

//继续搜索更好的跨度，只要当前位置
//匹配整个范围
        while (!done && pos == curr->span.end())
        {
            done = true;
//找到有最长祖先匹配的孩子
            for (std::unique_ptr<Node> const& child : curr->children)
            {
                auto const childPos = child->span.diff(ledger);
                if (childPos > pos)
                {
                    done = false;
                    pos = childPos;
                    curr = child.get();
                    break;
                }
            }
        }
        return std::make_pair(curr, pos);
    }

    void
    dumpImpl(std::ostream& o, std::unique_ptr<Node> const& curr, int offset)
        const
    {
        if (curr)
        {
            if (offset > 0)
                o << std::setw(offset) << "|-";

            std::stringstream ss;
            ss << *curr;
            o << ss.str() << std::endl;
            for (std::unique_ptr<Node> const& child : curr->children)
                dumpImpl(o, child, offset + 1 + ss.str().size() + 2);
        }
    }

public:
    LedgerTrie() : root{std::make_unique<Node>()}
    {
    }

    /*插入和/或增加对给定分类帐的支持。

        @param ledger一本分类帐及其祖先
        @param count此分类帐的支持计数
     **/

    void
    insert(Ledger const& ledger, std::uint32_t count = 1)
    {
        Node* loc;
        Seq diffSeq;
        std::tie(loc, diffSeq) = find(ledger);

//总有地方可以插入
        assert(loc);

//要从中开始增加分支支持的节点
        Node* incNode = loc;

//loc->span具有最长的公共前缀，其中span ledger_
//trie中的现有节点。下面的可选的
//loc->SPAN和SPAN LEdger之间可能的通用后缀。
//
//LOC->跨度
//A B C D D E F
//前缀oldsuffix
//
//跨度{分类帐}
//A B C·G G H I
//前缀newsuffix

        boost::optional<Span> prefix = loc->span.before(diffSeq);
        boost::optional<Span> oldSuffix = loc->span.from(diffSeq);
        boost::optional<Span> newSuffix = Span{ledger}.from(diffSeq);

        if (oldSuffix)
        {
//有
//ABCDEF- >…
//插入
//abc
//变成
//ABC->DEF->

//创建接管loc的oldsuffix节点
            auto newNode = std::make_unique<Node>(*oldSuffix);
            newNode->tipSupport = loc->tipSupport;
            newNode->branchSupport = loc->branchSupport;
            newNode->children = std::move(loc->children);
            assert(loc->children.empty());
            for (std::unique_ptr<Node>& child : newNode->children)
                child->parent = newNode.get();

//loc截断为前缀，newnode是其子级
            assert(prefix);
            loc->span = *prefix;
            newNode->parent = loc;
            loc->children.emplace_back(std::move(newNode));
            loc->tipSupport = 0;
        }
        if (newSuffix)
        {
//有
//ABC->…
//插入
//ABCDEF-＞…
//变成
//ABC->…
//\> DEF

            auto newNode = std::make_unique<Node>(*newSuffix);
            newNode->parent = loc;
//从新节点开始增加支持
            incNode = newNode.get();
            loc->children.push_back(std::move(newNode));
        }

        incNode->tipSupport += count;
        while (incNode)
        {
            incNode->branchSupport += count;
            incNode = incNode->parent;
        }

        seqSupport[ledger.seq()] += count;
    }

    /*减少对分类帐的支持，如果可能，请删除和压缩。

        @param ledger要删除的分类帐历史记录
        @param count要删除的尖端支持量

        @返回匹配的节点是否减量并可能被删除。
    **/

    bool
    remove(Ledger const& ledger, std::uint32_t count = 1)
    {
        Node* loc;
        Seq diffSeq;
        std::tie(loc, diffSeq) = find(ledger);

        if (loc)
        {
//必须与叶尖支撑完全匹配
            if (diffSeq == loc->span.end() && diffSeq > ledger.seq() &&
                loc->tipSupport > 0)
            {
                count = std::min(count, loc->tipSupport);
                loc->tipSupport -= count;

                auto const it = seqSupport.find(ledger.seq());
                assert(it != seqSupport.end() && it->second >= count);
                it->second -= count;
                if (it->second == 0)
                    seqSupport.erase(it->first);

                Node* decNode = loc;
                while (decNode)
                {
                    decNode->branchSupport -= count;
                    decNode = decNode->parent;
                }

                while (loc->tipSupport == 0 && loc != root.get())
                {
                    Node* parent = loc->parent;
                    if (loc->children.empty())
                    {
//此节点可以擦除
                        parent->erase(loc);
                    }
                    else if (loc->children.size() == 1)
                    {
//此节点可以与其子节点组合
                        std::unique_ptr<Node> child =
                            std::move(loc->children.front());
                        child->span = merge(loc->span, child->span);
                        child->parent = parent;
                        parent->children.emplace_back(std::move(child));
                        parent->erase(loc);
                    }
                    else
                        break;
                    loc = parent;
                }
                return true;
            }
        }
        return false;
    }

    /*返回特定分类帐的提示支持计数。

        @param ledger要查找的分类帐
        @返回此*精确*分类账的trie条目数
     **/

    std::uint32_t
    tipSupport(Ledger const& ledger) const
    {
        Node const* loc;
        Seq diffSeq;
        std::tie(loc, diffSeq) = find(ledger);

//精确匹配
        if (loc && diffSeq == loc->span.end() && diffSeq > ledger.seq())
            return loc->tipSupport;
        return 0;
    }

    /*返回特定分类帐的分支机构支持计数

        @param ledger要查找的分类帐
        @返回此分类帐或
                后裔
     **/

    std::uint32_t
    branchSupport(Ledger const& ledger) const
    {
        Node const* loc;
        Seq diffSeq;
        std::tie(loc, diffSeq) = find(ledger);

//检查分类帐是否完全匹配或正确。
//LOC前缀
        if (loc && diffSeq > ledger.seq() && ledger.seq() < loc->span.end())
        {
            return loc->branchSupport;
        }
        return 0;
    }

    /*返回首选分类帐ID

        首选分类账用于确定工作分类账
        在竞争对手之间达成共识。

        回想一下，每个验证器通常都在验证一系列分类账，
        例如A->B->C->D。但是，如果由于网络连接或其他原因
        问题，验证程序生成不同的链

        @代码
               /-C
           A-＞B
               -> d>
        @端码

        我们需要一种方法让验证器在链上与
        支持。我们称之为首选分类账。直觉上，我们的想法是
        保守一点，只有当你看到
        足够的对等验证来*知道*另一个分支将没有首选
        支持。

        通过浏览经过验证的分类账树，可以找到首选分类账。
        从共同祖先分类帐开始。

        在每个序列号，我们都有

           -优先顺序首选分类账，例如B.
           -具有此序列号的分类账的（提示）支持，例如
             上次验证为C或D的验证程序数。
           -当前所有子代的（分支）总支持
             序列号分类账，例如D的分支支持是
             D的叶尖支撑加E的叶尖支撑；
             C只是C的尖端支撑。
           -尚未验证分类帐的验证程序数
             使用此序列号（未提交的支持）。未提交的
             包括最后序列号小于的所有验证程序
             我们上次发布的序列号，由于异步，我们可以
             还没有收到这些节点的消息。

        此序列号的首选分类帐是分类帐
        有相对多数的支持，其中未承诺的支持
        任何分类账都可以用这个序列号。
        （包括一个还不知道的）。如果不存在此类首选分类账，则
        优先顺序优先分类账是总体优先分类账。

        在这个例子中，对于d来说，验证器的数量是首选的。
        支持它或其后代必须超过验证程序的数量
        支持当前未承诺的支持。这是因为如果
        所有未提交的验证程序最终都会验证C，新的支持必须
        小于d为首选。

        如果存在首选分类账，则继续下一个分类账
        使用该分类帐作为根的序列。

        @param largestissued最大验证的序列号
                             由该节点颁发。
        @与首选分类账的序列号和ID的返回对，或
                Boost：：如果不存在首选分类帐，则无
    **/

    boost::optional<SpanTip<Ledger>>
    getPreferred(Seq const largestIssued) const
    {
        if (empty())
            return boost::none;

        Node* curr = root.get();

        bool done = false;

        std::uint32_t uncommitted = 0;
        auto uncommittedIt = seqSupport.begin();

        while (curr && !done)
        {
//在单个范围内，分支策略的首选是
//继续沿着跨度延伸，直到分支支撑
//下一个分类帐超出了对该分类帐的未提交支持。
            {
//在分类帐之前添加任何初始未承诺支持
//早于nextseq或早于largestissued
                Seq nextSeq = curr->span.start() + Seq{1};
                while (uncommittedIt != seqSupport.end() &&
                       uncommittedIt->first < std::max(nextSeq, largestIssued))
                {
                    uncommitted += uncommittedIt->second;
                    uncommittedIt++;
                }

//沿跨距前进下一步
                while (nextSeq < curr->span.end() &&
                       curr->branchSupport > uncommitted)
                {
//跳到下一个SeqSupport更改
                    if (uncommittedIt != seqSupport.end() &&
                        uncommittedIt->first < curr->span.end())
                    {
                        nextSeq = uncommittedIt->first + Seq{1};
                        uncommitted += uncommittedIt->second;
                        uncommittedIt++;
                    }
else  //否则我们跳到跨度的末端
                        nextSeq = curr->span.end();
                }
//我们没有消耗整个跨度，因此我们发现
//首选分类帐
                if (nextSeq < curr->span.end())
                    return curr->span.before(nextSeq)->tip();
            }

//我们已经到了当前跨度的末尾，所以我们需要
//找到最好的孩子
            Node* best = nullptr;
            std::uint32_t margin = 0;
            if (curr->children.size() == 1)
            {
                best = curr->children[0].get();
                margin = best->branchSupport;
            }
            else if (!curr->children.empty())
            {
//排序放置具有最大分支支持的子级
//前面，与跨度的起始ID断开连接
                std::partial_sort(
                    curr->children.begin(),
                    curr->children.begin() + 2,
                    curr->children.end(),
                    [](std::unique_ptr<Node> const& a,
                       std::unique_ptr<Node> const& b) {
                        return std::make_tuple(
                                   a->branchSupport, a->span.startID()) >
                            std::make_tuple(
                                   b->branchSupport, b->span.startID());
                    });

                best = curr->children[0].get();
                margin = curr->children[0]->branchSupport -
                    curr->children[1]->branchSupport;

//如果最好能保住平局，就可以得到更大的利润。
//因为第二个最好的需要额外的分支支持
//克服平局
                if (best->span.startID() > curr->children[1]->span.startID())
                    margin++;
            }

//如果最好的孩子的保证金超过了未承诺的支持，
//从那个孩子身上继续，否则我们就完了
            if (best && ((margin > uncommitted) || (uncommitted == 0)))
                curr = best;
else  //电流是最好的
                done = true;
        }
        return curr->span.tip();
    }

    /*返回trie是否跟踪任何分类帐
     **/

    bool
    empty() const
    {
        return !root || root->branchSupport == 0;
    }

    /*将trie的ASCII表示形式转储到流中
     **/

    void
    dump(std::ostream& o) const
    {
        dumpImpl(o, root, 0);
    }

    /*转储trie状态的JSON表示
     **/

    Json::Value
    getJson() const
    {
        Json::Value res;
        res["trie"] = root->getJson();
        res["seq_support"] = Json::objectValue;
        for (auto const& mit : seqSupport)
            res["seq_support"][to_string(mit.first)] = mit.second;
        return res;
    }

    /*检查压缩的trie和支持不变量。
     **/

    bool
    checkInvariants() const
    {
        std::map<Seq, std::uint32_t> expectedSeqSupport;

        std::stack<Node const*> nodes;
        nodes.push(root.get());
        while (!nodes.empty())
        {
            Node const* curr = nodes.top();
            nodes.pop();
            if (!curr)
                continue;

//具有0提示支持的节点必须有多个子节点
//除非它是根节点
            if (curr != root.get() && curr->tipSupport == 0 &&
                curr->children.size() < 2)
                return false;

//branchsupport=tipsupport+sum（child->branchsupport）
            std::size_t support = curr->tipSupport;
            if (curr->tipSupport != 0)
                expectedSeqSupport[curr->span.end() - Seq{1}] +=
                    curr->tipSupport;

            for (auto const& child : curr->children)
            {
                if (child->parent != curr)
                    return false;

                support += child->branchSupport;
                nodes.push(child.get());
            }
            if (support != curr->branchSupport)
                return false;
        }
        return expectedSeqSupport == seqSupport;
    }
};

}  //命名空间波纹
#endif
