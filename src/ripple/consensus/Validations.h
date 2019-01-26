
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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

#ifndef RIPPLE_CONSENSUS_VALIDATIONS_H_INCLUDED
#define RIPPLE_CONSENSUS_VALIDATIONS_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/chrono.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <ripple/consensus/LedgerTrie.h>
#include <boost/optional.hpp>
#include <mutex>
#include <utility>
#include <vector>

namespace ripple {

/*控制验证过期和过期的时间参数。

    @注意，这些是协议级参数，在没有
          仔细考虑。它们*不是*实现为静态constexpr
          允许模拟代码测试备用参数设置。
 **/

struct ValidationParms
{
    explicit ValidationParms() = default;

    /*验证在其分类帐之后保持当前状态的秒数
        关闭时间。

        这是一种安全措施，可以防止非常古老的验证和时间
        需要调整关闭时间精度窗口。
    **/

    std::chrono::seconds validationCURRENT_WALL = std::chrono::minutes{5};

    /*持续时间首次观察后，验证仍为当前状态。

        验证在我们
        首先看到它。这在极少数情况下提供了更快的恢复，
        网络生成的验证数低于正常值
    **/

    std::chrono::seconds validationCURRENT_LOCAL = std::chrono::minutes{3};

    /*可接受验证的预关闭时间。

        我们考虑验证的关闭时间之前的秒数
        可接受的。这可以防止极端的时钟错误
    **/

    std::chrono::seconds validationCURRENT_EARLY = std::chrono::minutes{3};

    /*持续时间给定分类帐哈希的一组验证仍然有效

        给定分类帐的一组验证之前的秒数
        哈希可以过期。这将保持最近分类账的有效性
        在合理的时间间隔内。
    **/

    std::chrono::seconds validationSET_EXPIRES = std::chrono::minutes{10};
};

/*执行验证，增加序列要求。

    用于强制验证必须大于所有验证的帮助程序类
    验证程序以前发布的未过期验证序列号
    由此类的实例跟踪。
**/

template <class Seq>
class SeqEnforcer
{
    using time_point = std::chrono::steady_clock::time_point;
    Seq seq_{0};
    time_point when_;

public:
    /*尝试推进观察到的最大验证分类帐序列

        尝试设置观察到的最大验证序列，但返回false
        如果它违反了不变量，则验证必须大于所有
        未过期的验证序列号。

        @param现在是当前时间
        @param s我们要验证的序列号
        @param p验证参数

        @返回验证是否满足不变量
    **/

    bool
    operator()(time_point now, Seq s, ValidationParms const& p)
    {
        if (now > (when_ + p.validationSET_EXPIRES))
            seq_ = Seq{0};
        if (s <= seq_)
            return false;
        seq_ = s;
        when_ = now;
        return true;
    }

    Seq
    largest() const
    {
        return seq_;
    }
};
/*验证是否仍然是最新的

    确定是否仍可以将验证视为当前验证
    根据节点的签名时间和第一个
    由这个节点看到。

    带定时参数的@param p validationparms
    @param now当前时间
    验证签名时的@param signtime
    @param seentime首次在本地看到验证时
**/

inline bool
isCurrent(
    ValidationParms const& p,
    NetClock::time_point now,
    NetClock::time_point signTime,
    NetClock::time_point seenTime)
{
//因为这可能是在不可信的情况下调用的
//恶意验证，我们以某种方式进行计算
//避免任何溢出或下溢的机会
//签署时间。

    return (signTime > (now - p.validationCURRENT_EARLY)) &&
        (signTime < (now + p.validationCURRENT_WALL)) &&
        ((seenTime == NetClock::time_point{}) ||
         (seenTime < (now + p.validationCURRENT_LOCAL)));
}

/*新接收验证的状态
 **/

enum class ValStatus {
///这是一个新的验证，已添加
    current,
///not current或早于此节点的current
    stale,
///a验证违反了递增的seq要求。
    badSeq
};

inline std::string
to_string(ValStatus m)
{
    switch (m)
    {
        case ValStatus::current:
            return "current";
        case ValStatus::stale:
            return "stale";
        case ValStatus::badSeq:
            return "badSeq";
        default:
            return "unknown";
    }
}

/*维护当前和最近的分类帐验证。

    管理与网络上接收的验证相关的存储和查询。
    存储最近的节点和集的最新验证
    按分类帐标识符分组的验证。

    存储的验证不一定来自受信任的节点，因此客户端
    并且实现应该注意使用“可信”成员函数或
    检查验证的受信任状态。

    此类使用通用接口允许为
    具体应用。适配器模板实现一组助手
    函数和类型定义。下面的代码存根概述了
    接口和类型要求。


    @警告：适配器：：mutextype用于管理对
             验证的私有成员，但不管理
             适配器实例本身。

    @代码

    //符合Ledgertrie的分类账类型要求
    Ledger结构；

    结构验证
    {
        使用nodeid=…；
        使用nodekey=…；

        //与此验证关联的分类帐ID
        分类帐：：id ledger id（）常量；

        //验证分类账的序列号（0表示无序列号）
        分类帐：：seq seq（）常量

        //签署验证时
        netclock:：time_point signtime（）常量；

        //此节点首次观察到验证时
        netclock:：time_point seentime（）常量；

        //发布验证的节点的签名密钥
        nodekey key（）常量；

        //验证时是否信任发布节点
        /到达
        bool trusted（）常量；

        //将验证设置为可信
        void settrusted（）；

        //将验证设置为不可信
        void setuntrusted（）；

        //这是完全验证还是部分验证
        bool full（）常量；

        //此节点的标识符，即使在旋转时仍保持不变
        //签名密钥
        nodeid nodeid（）常量；

        具体实施
        unwrap（）->返回要包装的特定于实现的类型

        /…具体实施
    }；

    类适配器
    {
        使用mutex=std:：mutex；
        使用验证=验证；
        使用分类账=分类账；

        //处理一个新的过时验证，这应该只做最少的工作，因为
        //当它可能正在迭代验证时，由验证调用
        /下锁
        无效报告（验证和）

        //刷新剩余的验证（通常在关闭时完成）
        void flush（hash_map<nodeid，validation>&&remaining）；

        //返回当前网络时间（用于确定过时）
        netclock:：time_point now（）常量；

        //尝试获取特定的分类帐。
        boost：：可选<ledger>acquire（ledger:：id const&ledger id）；

        /…具体实施
    }；
    @端码

    @tparam适配器提供类型定义和回调
**/

template <class Adaptor>
class Validations
{
    using Mutex = typename Adaptor::Mutex;
    using Validation = typename Adaptor::Validation;
    using Ledger = typename Adaptor::Ledger;
    using ID = typename Ledger::ID;
    using Seq = typename Ledger::Seq;
    using NodeID = typename Validation::NodeID;

    using WrappedValidationType = std::decay_t<
        std::result_of_t<decltype (&Validation::unwrap)(Validation)>>;

    using ScopedLock = std::lock_guard<Mutex>;

//管理对成员的并发访问
    mutable Mutex mutex_;

//当前列出的受信任节点的验证（部分和全部）
    hash_map<NodeID, Validation> current_;

//用于强制本地节点的最大验证不变量
    SeqEnforcer<Seq> localSeqEnforcer_;

//从每个节点接收的最大验证的序列
    hash_map<NodeID, SeqEnforcer<Seq>> seqEnforcers_;

//！从列出的节点验证，按分类帐ID索引（部分和全部）
    beast::aged_unordered_map<
        ID,
        hash_map<NodeID, Validation>,
        std::chrono::steady_clock,
        beast::uhash<>>
        byLedger_;

//表示已验证分类帐的祖先
    LedgerTrie<Ledger> trie_;

//已成功获取最后一个（已验证）分类帐。如果在这张地图上，它是
//在特里亚占了很大的份额。
    hash_map<NodeID, Ledger> lastLedger_;

//从网络中获取的一组分类帐
    hash_map<std::pair<Seq, ID>, hash_set<NodeID>> acquiring_;

//确定验证过期的参数
    ValidationParms const parms_;

//适配器实例
//不是由上面的互斥体管理的
    Adaptor adaptor_;

private:
//删除已验证分类帐的支持
    void
    removeTrie(ScopedLock const&, NodeID const& nodeID, Validation const& val)
    {
        {
            auto it =
                acquiring_.find(std::make_pair(val.seq(), val.ledgerID()));
            if (it != acquiring_.end())
            {
                it->second.erase(nodeID);
                if (it->second.empty())
                    acquiring_.erase(it);
            }
        }
        {
            auto it = lastLedger_.find(nodeID);
            if (it != lastLedger_.end() && it->second.id() == val.ledgerID())
            {
                trie_.remove(it->second);
                lastLedger_.erase(nodeID);
            }
        }
    }

//检查是否完成任何挂起的获取分类帐请求
    void
    checkAcquired(ScopedLock const& lock)
    {
        for (auto it = acquiring_.begin(); it != acquiring_.end();)
        {
            if (boost::optional<Ledger> ledger =
                    adaptor_.acquire(it->first.second))
            {
                for (NodeID const& nodeID : it->second)
                    updateTrie(lock, nodeID, *ledger);

                it = acquiring_.erase(it);
            }
            else
                ++it;
        }
    }

//更新trie以反映新的已验证分类帐
    void
    updateTrie(ScopedLock const&, NodeID const& nodeID, Ledger ledger)
    {
        auto ins = lastLedger_.emplace(nodeID, ledger);
        if (!ins.second)
        {
            trie_.remove(ins.first->second);
            ins.first->second = ledger;
        }
        trie_.insert(ledger);
    }

    /*处理新的验证

        从验证器处理新的可信验证。这将是
        只有在已验证的分类帐被成功获取后才会反映
        本地节点。在此期间，在此之前已验证的分类帐
        节点仍然存在。

        @param lock现有互斥锁\u
        @param nodeid验证节点的节点标识符
        @param val节点发出的可信验证
        @param prior if not none，the last current validated ledger seq，id of
                     钥匙
    **/

    void
    updateTrie(
        ScopedLock const& lock,
        NodeID const& nodeID,
        Validation const& val,
        boost::optional<std::pair<Seq, ID>> prior)
    {
        assert(val.trusted());

//清除此节点的任何以前的收单分类账
        if (prior)
        {
            auto it = acquiring_.find(*prior);
            if (it != acquiring_.end())
            {
                it->second.erase(nodeID);
                if (it->second.empty())
                    acquiring_.erase(it);
            }
        }

        checkAcquired(lock);

        std::pair<Seq, ID> valPair{val.seq(), val.ledgerID()};
        auto it = acquiring_.find(valPair);
        if (it != acquiring_.end())
        {
            it->second.insert(nodeID);
        }
        else
        {
            if (boost::optional<Ledger> ledger =
                    adaptor_.acquire(val.ledgerID()))
                updateTrie(lock, nodeID, *ledger);
            else
                acquiring_[valPair].insert(nodeID);
        }
    }

    /*使用trie进行计算

        通过此助手访问trie可确保获取验证
        检查并从trie中清除所有过时的验证。

        @param lock现有互斥锁\u
        @param f invocate with signature（ledgertrie<ledger>>）

        @警告可调用的“f”应该是
                 它的参数，并将在锁下使用mutex_u调用。

    **/

    template <class F>
    auto
    withTrie(ScopedLock const& lock, F&& f)
    {
//调用current刷新任何过时的验证
        current(lock, [](auto) {}, [](auto, auto) {});
        checkAcquired(lock);
        return f(trie_);
    }

    /*迭代当前验证。

        迭代当前验证，刷新任何过时的验证。

        @param lock现有互斥锁\u
        @param pre invokeable with signature（std:：size_t）called prior to
                   循环。
        @param f可使用签名调用（nodeid const&，validations const&）
                 对于每个当前验证。

        @注意，可调用的“pre”被称为“pre”，用于检查是否过时。
              并反映对'f'的调用数的上限。
        @警告可调用的“f”应该是
                 它的参数，并将在锁下使用mutex_u调用。
    **/


    template <class Pre, class F>
    void
    current(ScopedLock const& lock, Pre&& pre, F&& f)
    {
        NetClock::time_point t = adaptor_.now();
        pre(current_.size());
        auto it = current_.begin();
        while (it != current_.end())
        {
//检查是否老化
            if (!isCurrent(
                    parms_, t, it->second.signTime(), it->second.seenTime()))
            {
                removeTrie(lock, it->first, it->second);
                adaptor_.onStale(std::move(it->second));
                it = current_.erase(it);
            }
            else
            {
                auto cit = typename decltype(current_)::const_iterator{it};
//包含实时记录
                f(cit->first, cit->second);
                ++it;
            }
        }
    }

    /*迭代与给定分类帐ID关联的验证集

        @param锁定互斥体上的现有锁\u
        @param ledgerid分类帐的标识符
        @param pre invokeable with signature（标准：：大小）
        @param f可使用签名调用（nodeid const&，validation const&）

        @注意，可调用的'pre'在迭代验证之前被调用。这个
              参数是调用“f”的次数。
        @警告可调用的f应该是
       它的参数，并将在锁下使用mutex_u调用。
    **/

    template <class Pre, class F>
    void
    byLedger(ScopedLock const&, ID const& ledgerID, Pre&& pre, F&& f)
    {
        auto it = byLedger_.find(ledgerID);
        if (it != byLedger_.end())
        {
//更新自使用以来的设置时间
            byLedger_.touch(it);
            pre(it->second.size());
            for (auto const& keyVal : it->second)
                f(keyVal.first, keyVal.second);
        }
    }

public:
    /*构造函数

        @param p validationparms控制验证的过时/过期
        @param c时钟用于分类帐存储的过期验证
        用于构造适配器实例的@param ts参数
    **/

    template <class... Ts>
    Validations(
        ValidationParms const& p,
        beast::abstract_clock<std::chrono::steady_clock>& c,
        Ts&&... ts)
        : byLedger_(c), parms_(p), adaptor_(std::forward<Ts>(ts)...)
    {
    }

    /*返回适配器实例
     **/

    Adaptor const&
    adaptor() const
    {
        return adaptor_;
    }

    /*返回验证时间参数
     **/

    ValidationParms const&
    parms() const
    {
        return parms_;
    }

    /*返回本地节点是否可以为给定的
       序列号

        @param s节点要验证的分类帐的序列号
        @返回验证是否满足不变量，更新
                相应的最大序列号
    **/

    bool
    canValidateSeq(Seq const s)
    {
        ScopedLock lock{mutex_};
        return localSeqEnforcer_(byLedger_.clock().now(), s, parms_);
    }

    /*添加新验证

        尝试添加新验证。

        @param nodeid发出此验证的节点的标识
        @param val要存储的验证
        @返回结果
    **/

    ValStatus
    add(NodeID const& nodeID, Validation const& val)
    {
        if (!isCurrent(parms_, adaptor_.now(), val.signTime(), val.seenTime()))
            return ValStatus::stale;

        {
            ScopedLock lock{mutex_};

//检查验证序列是否大于任何未过期的序列
//来自该验证器的验证序列
            auto const now = byLedger_.clock().now();
            SeqEnforcer<Seq>& enforcer = seqEnforcers_[nodeID];
            if (!enforcer(now, val.seq(), parms_))
                return ValStatus::badSeq;

//当C++ 17支持时使用插入式
            auto byLedgerIt = byLedger_[val.ledgerID()].emplace(nodeID, val);
            if (!byLedgerIt.second)
                byLedgerIt.first->second = val;

            auto const ins = current_.emplace(nodeID, val);
            if (!ins.second)
            {
//仅当此版本更新时替换现有版本
                Validation& oldVal = ins.first->second;
                if (val.signTime() > oldVal.signTime())
                {
                    std::pair<Seq, ID> old(oldVal.seq(), oldVal.ledgerID());
                    adaptor_.onStale(std::move(oldVal));
                    ins.first->second = val;
                    if (val.trusted())
                        updateTrie(lock, nodeID, val, old);
                }
                else
                    return ValStatus::stale;
            }
            else if (val.trusted())
            {
                updateTrie(lock, nodeID, val, boost::none);
            }
        }
        return ValStatus::current;
    }

    /*过期旧验证集

        删除访问次数超过
        validationset_过期。
    **/

    void
    expire()
    {
        ScopedLock lock{mutex_};
        beast::expire(byLedger_, parms_.validationSET_EXPIRES);
    }

    /*更新验证的信任状态

        将已知验证的受信任状态更新为节点的帐户
        已从UNL添加或删除的。这也会更新trie
        以确保只使用当前受信任节点的验证。

        @param添加了现在受信任的节点的标识符
        @param删除了不再受信任的节点的标识符
    **/

    void
    trustChanged(hash_set<NodeID> const& added, hash_set<NodeID> const& removed)
    {
        ScopedLock lock{mutex_};

        for (auto& it : current_)
        {
            if (added.find(it.first) != added.end())
            {
                it.second.setTrusted();
                updateTrie(lock, it.first, it.second, boost::none);
            }
            else if (removed.find(it.first) != removed.end())
            {
                it.second.setUntrusted();
                removeTrie(lock, it.first, it.second);
            }
        }

        for (auto& it : byLedger_)
        {
            for (auto& nodeVal : it.second)
            {
                if (added.find(nodeVal.first) != added.end())
                {
                    nodeVal.second.setTrusted();
                }
                else if (removed.find(nodeVal.first) != removed.end())
                {
                    nodeVal.second.setUntrusted();
                }
            }
        }
    }

    Json::Value
    getJsonTrie() const
    {
        ScopedLock lock{mutex_};
        return trie_.getJson();
    }

    /*返回首选工作分类帐的序列号和ID

        如果在可信验证器之间有更多的支持，最好使用分类帐。
        并且不是当前工作分类账的祖先；否则
        保留当前工作分类帐。

        @param curr本地节点的当前工作分类帐

        @返回首选工作分类账的序列和ID，
                或boost：：如果没有可信验证可用于
                确定首选分类帐。
    **/

    boost::optional<std::pair<Seq, ID>>
    getPreferred(Ledger const& curr)
    {
        ScopedLock lock{mutex_};
        boost::optional<SpanTip<Ledger>> preferred =
            withTrie(lock, [this](LedgerTrie<Ledger>& trie) {
                return trie.getPreferred(localSeqEnforcer_.largest());
            });
//没有可信任的验证来确定分支
        if (!preferred)
        {
//在收购分类账上回落到多数
            auto it = std::max_element(
                acquiring_.begin(),
                acquiring_.end(),
                [](auto const& a, auto const& b) {
                    std::pair<Seq, ID> const& aKey = a.first;
                    typename hash_set<NodeID>::size_type const& aSize =
                        a.second.size();
                    std::pair<Seq, ID> const& bKey = b.first;
                    typename hash_set<NodeID>::size_type const& bSize =
                        b.second.size();
//按验证该分类帐的受信任对等数排序
//断开与分类帐ID的关系
                    return std::tie(aSize, aKey.second) <
                        std::tie(bSize, bKey.second);
                });
            if (it != acquiring_.end())
                return it->first;
            return boost::none;
        }

//如果我们是首选分类账的母公司，请遵守我们的
//当前分类帐，因为我们可能即将生成它
        if (preferred->seq == curr.seq() + Seq{1} &&
            preferred->ancestor(curr.seq()) == curr.id())
            return std::make_pair(curr.seq(), curr.id());

//不管是不是，我们前面最好有一个分类账。
//我们工作分类账的后代，或者它在不同的链上
        if (preferred->seq > curr.seq())
            return std::make_pair(preferred->seq, preferred->id);

//仅切换到较早或相同的序列号
//如果是不同的链条。
        if (curr[preferred->seq] != preferred->id)
            return std::make_pair(preferred->seq, preferred->id);

//坚持使用当前分类帐
        return std::make_pair(curr.seq(), curr.id());
    }

    /*获取超过最小有效值的首选工作分类帐的ID
        分类帐序列号

        @param curr当前工作分类帐
        @param minvalidseq最小允许序列号

        @返回首选分类帐的ID，如果首选分类帐
                   无效
    **/

    ID
    getPreferred(Ledger const& curr, Seq minValidSeq)
    {
        boost::optional<std::pair<Seq, ID>> preferred = getPreferred(curr);
        if (preferred && preferred->first >= minValidSeq)
            return preferred->second;
        return curr.id();
    }

    /*为下一轮共识确定首选的最后一个结算分类账。

        在开始下一轮分类帐共识之前调用以确定
        首选工作分类帐。如果没有，则使用主对等计数分类帐
        可信验证可用。

        @param lcl此节点上次关闭的分类帐
        @param minseq受信任首选项的最小允许序列号
                      分类帐
        @param peercounts从分类帐ID映射到对等数，并将其作为
                          上次关闭的分类帐
        @返回首选的上次关闭的分类帐ID

        @注意minseq不适用于对等计数，因为此函数
              不知道它们的序列号
    **/

    ID
    getPreferredLCL(
        Ledger const& lcl,
        Seq minSeq,
        hash_map<ID, std::uint32_t> const& peerCounts)
    {
        boost::optional<std::pair<Seq, ID>> preferred = getPreferred(lcl);

//存在可信验证，但如果
//首选是在过去
        if (preferred)
            return (preferred->first >= minSeq) ? preferred->second : lcl.id();

//否则，依赖同行分类账
        auto it = std::max_element(
            peerCounts.begin(), peerCounts.end(), [](auto& a, auto& b) {
//更喜欢更大的计数，而不是领带上更大的ID
//（max_元素希望在a<b时返回true）
                return std::tie(a.second, a.first) <
                    std::tie(b.second, b.first);
            });

        if (it != peerCounts.end())
            return it->first;
        return lcl.id();
    }

    /*计算当前使用分类帐的受信任验证器的数目
        在指定的一个之后。

        @param ledger工作分类帐
        @param ledgerid首选分类帐
        @返回当前在子代上工作的受信任验证器的数目
                首选分类帐的

        @note if ledger.id（）！=LedgerID，只计算
              羽毛球
    **/

    std::size_t
    getNodesAfter(Ledger const& ledger, ID const& ledgerID)
    {
        ScopedLock lock{mutex_};

//如果分类帐是正确的，请使用trie
        if (ledger.id() == ledgerID)
            return withTrie(lock, [&ledger](LedgerTrie<Ledger>& trie) {
                return trie.branchSupport(ledger) - trie.tipSupport(ledger);
            });

//将父分类帐计数为回退
        return std::count_if(
            lastLedger_.begin(),
            lastLedger_.end(),
            [&ledgerID](auto const& it) {
                auto const& curr = it.second;
                return curr.seq() > Seq{0} &&
                    curr[curr.seq() - Seq{1}] == ledgerID;
            });
    }

    /*获取当前受信任的完全验证

        @返回当前受信任验证器的验证向量
    **/

    std::vector<WrappedValidationType>
    currentTrusted()
    {
        std::vector<WrappedValidationType> ret;
        ScopedLock lock{mutex_};
        current(
            lock,
            [&](std::size_t numValidations) { ret.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    ret.push_back(v.unwrap());
            });
        return ret;
    }

    /*获取与当前验证关联的节点ID集

        @返回活动的、列出的验证器的节点ID集
    **/

    auto
    getCurrentNodeIDs() -> hash_set<NodeID>
    {
        hash_set<NodeID> ret;
        ScopedLock lock{mutex_};
        current(
            lock,
            [&](std::size_t numValidations) { ret.reserve(numValidations); },
            [&](NodeID const& nid, Validation const&) { ret.insert(nid); });

        return ret;
    }

    /*计算给定分类帐的可信完全验证数

        @param ledgerid利息分类帐的标识符
        @返回可信验证数
    **/

    std::size_t
    numTrustedForLedger(ID const& ledgerID)
    {
        std::size_t count = 0;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
[&](std::size_t) {},  //无需保留
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    ++count;
            });
        return count;
    }

    /*获取特定分类帐的可信完全验证

         @param ledgerid利息分类帐的标识符
         @返回与分类帐关联的可信验证
    **/

    std::vector<WrappedValidationType>
    getTrustedForLedger(ID const& ledgerID)
    {
        std::vector<WrappedValidationType> res;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
            [&](std::size_t numValidations) { res.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                    res.emplace_back(v.unwrap());
            });

        return res;
    }

    /*返回指定分类帐中受信任的完全验证程序报告的费用

        @param ledgerid利息分类帐的标识符
        @param basefee验证中不存在要报告的费用
        @返回费用向量
    **/

    std::vector<std::uint32_t>
    fees(ID const& ledgerID, std::uint32_t baseFee)
    {
        std::vector<std::uint32_t> res;
        ScopedLock lock{mutex_};
        byLedger(
            lock,
            ledgerID,
            [&](std::size_t numValidations) { res.reserve(numValidations); },
            [&](NodeID const&, Validation const& v) {
                if (v.trusted() && v.full())
                {
                    boost::optional<std::uint32_t> loadFee = v.loadFee();
                    if (loadFee)
                        res.push_back(*loadFee);
                    else
                        res.push_back(baseFee);
                }
            });
        return res;
    }

    /*刷新所有当前验证
     **/

    void
    flush()
    {
        hash_map<NodeID, Validation> flushed;
        {
            ScopedLock lock{mutex_};
            for (auto it : current_)
            {
                flushed.emplace(it.first, std::move(it.second));
            }
            current_.clear();
        }

        adaptor_.flush(std::move(flushed));
    }
};

}  //命名空间波纹
#endif
