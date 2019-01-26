
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

#ifndef RIPPLE_APP_MISC_HASHROUTER_H_INCLUDED
#define RIPPLE_APP_MISC_HASHROUTER_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/CountedObject.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <boost/optional.hpp>

namespace ripple {

//vvalco注：这些是旗子吗？为什么不使用压缩结构？
//vfalc todo将这些宏转换为int常量
//vfalc注意如何在哈希表上设置好与坏？
#define SF_BAD          0x02    //暂时不良
#define SF_SAVED        0x04
#define SF_TRUSTED      0x10    //来自可信来源
//私有标志，在apply.cpp中内部使用。
//不要试图读取、设置或重用。
#define SF_PRIVATE1     0x0100
#define SF_PRIVATE2     0x0200
#define SF_PRIVATE3     0x0400
#define SF_PRIVATE4     0x0800
#define SF_PRIVATE5     0x1000
#define SF_PRIVATE6     0x2000

/*哈希标识的对象的路由表。

    此表跟踪哪些对等方接收了哪些哈希。
    它用于管理对等端中消息的路由和广播
    对等覆盖。
**/

class HashRouter
{
public:
//此处的类型*必须*与peer:：id\u t的类型匹配
    using PeerShortID = std::uint32_t;

private:
    /*路由表中的一个条目。
    **/

    class Entry : public CountedObject <Entry>
    {
    public:
        static char const* getCountedObjectName () { return "HashRouterEntry"; }

        Entry ()
        {
        }

        void addPeer (PeerShortID peer)
        {
            if (peer != 0)
                peers_.insert (peer);
        }

        int getFlags (void) const
        {
            return flags_;
        }

        void setFlags (int flagsToSet)
        {
            flags_ |= flagsToSet;
        }

        /*返回中继到的对等点集并重置跟踪*/
        std::set<PeerShortID> releasePeerSet()
        {
            return std::move(peers_);
        }

        /*确定是否应中继此项。

            检查项目是否最近被中继。
            如果有，返回false。如果没有，请更新
            最后一个中继时间戳并返回true。
        **/

        bool shouldRelay (Stopwatch::time_point const& now,
            std::chrono::seconds holdTime)
        {
            if (relayed_ && *relayed_ + holdTime > now)
                return false;
            relayed_.emplace(now);
            return true;
        }

        /*确定是否应从打开的分类帐中恢复此项。

            统计项目已恢复的次数。
            每调用一次“limit”，返回false。
            否则返回true。

            @注意限制必须大于0
        **/

        bool shouldRecover(std::uint32_t limit)
        {
            return ++recoveries_ % limit != 0;
        }

        bool shouldProcess(Stopwatch::time_point now, std::chrono::seconds interval)
        {
             if (processed_ && ((*processed_ + interval) > now))
                 return false;
             processed_.emplace (now);
             return true;
        }

    private:
        int flags_ = 0;
        std::set <PeerShortID> peers_;
//如果更多的话，这可以推广到地图上。
//多个标志需要独立过期。
        boost::optional<Stopwatch::time_point> relayed_;
        boost::optional<Stopwatch::time_point> processed_;
        std::uint32_t recoveries_ = 0;
    };

public:
    static inline std::chrono::seconds getDefaultHoldTime ()
    {
        using namespace std::chrono;

        return 300s;
    }

    static inline std::uint32_t getDefaultRecoverLimit()
    {
        return 1;
    }

    HashRouter (Stopwatch& clock, std::chrono::seconds entryHoldTimeInSeconds,
        std::uint32_t recoverLimit)
        : suppressionMap_(clock)
        , holdTime_ (entryHoldTimeInSeconds)
        , recoverLimit_ (recoverLimit + 1u)
    {
    }

    HashRouter& operator= (HashRouter const&) = delete;

    virtual ~HashRouter() = default;

//vfalc todo用更多的东西来代替“抑制”术语
//语义上有意义。
    void addSuppression(uint256 const& key);

    bool addSuppressionPeer (uint256 const& key, PeerShortID peer);

    bool addSuppressionPeer (uint256 const& key, PeerShortID peer,
                             int& flags);

//添加对等抑制并返回是否应处理该条目
    bool shouldProcess (uint256 const& key, PeerShortID peer, int& flags,
        std::chrono::seconds tx_interval);

    /*在哈希上设置标志。

        @如果标志被更改，返回'true'。如果不变，则为'false'。
    **/

    bool setFlags (uint256 const& key, int flags);

    int getFlags (uint256 const& key);

    /*确定是否应中继哈希项。

        影响：

            如果应该中继该项，则此函数将不会
            再次返回“true”，直到等待时间结束。
            内部对等点集也将被重置。

        @返回一组“boost:：optional”对等点，这些对等点不需要
            转达。如果结果未初始化，则该项应
            不要被中继。
    **/

    boost::optional<std::set<PeerShortID>> shouldRelay(uint256 const& key);

    /*确定是否应恢复哈希项

        @return`bool`指示是否应中继项目
    **/

    bool shouldRecover(uint256 const& key);

private:
//pair.second指示条目是否已创建
    std::pair<Entry&, bool> emplace (uint256 const&);

    std::mutex mutable mutex_;

//存储所有禁止散列及其过期时间
    beast::aged_unordered_map<uint256, Entry, Stopwatch::clock_type,
        hardened_hash<strong_hash>> suppressionMap_;

    std::chrono::seconds const holdTime_;

    std::uint32_t const recoverLimit_;
};

} //涟漪

#endif
