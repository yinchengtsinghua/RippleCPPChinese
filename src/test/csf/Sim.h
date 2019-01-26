
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

#ifndef RIPPLE_TEST_CSF_SIM_H_INCLUDED
#define RIPPLE_TEST_CSF_SIM_H_INCLUDED

#include <test/csf/Digraph.h>
#include <test/csf/SimTime.h>
#include <test/csf/BasicNetwork.h>
#include <test/csf/Scheduler.h>
#include <test/csf/Peer.h>
#include <test/csf/PeerGroup.h>
#include <test/csf/TrustGraph.h>
#include <test/csf/CollectorRef.h>

#include <iostream>
#include <deque>
#include <random>

namespace ripple {
namespace test {
namespace csf {

/*将模拟时间提前到消息的接收器*/
class BasicSink : public beast::Journal::Sink
{
    Scheduler::clock_type const & clock_;
public:
    BasicSink (Scheduler::clock_type const & clock)
        : Sink (beast::severities::kDisabled, false)
        , clock_{clock}
    {
    }

    void
    write (beast::severities::Severity level,
        std::string const& text) override
    {
        if (level < threshold())
            return;

        std::cout << clock_.now().time_since_epoch().count() << " " << text
                  << std::endl;
    }
};

class Sim
{
//即使动态添加对等点，也要使用deque来拥有稳定的指针。
//-或者考虑使用竞技场分配的唯一资源
    std::deque<Peer> peers;
    PeerGroup allPeers;

public:
    std::mt19937_64 rng;
    Scheduler scheduler;
    BasicSink sink;
    beast::Journal j;
    LedgerOracle oracle;
    BasicNetwork<Peer*> net;
    TrustGraph<Peer*> trustGraph;
    CollectorRefs collectors;

    /*创建模拟

        创建新模拟。模拟没有对等点，没有信任链接
        没有网络连接。

    **/

    Sim() : sink{scheduler.clock()}, j{sink}, net{scheduler}
    {
    }

    /*创建新的对等组。

        创建新的对等组。同行之间没有任何信任关系
        或默认的网络连接。这些必须由客户机配置。

        @param numpeers组中的对等数
        @返回代表这些新对等的对等组

        @注意，这会增加numpeers模拟中的对等数。
    **/

    PeerGroup
    createGroup(std::size_t numPeers)
    {
        std::vector<Peer*> newPeers;
        newPeers.reserve(numPeers);
        for (std::size_t i = 0; i < numPeers; ++i)
        {
            peers.emplace_back(
                PeerID{static_cast<std::uint32_t>(peers.size())},
                scheduler,
                oracle,
                net,
                trustGraph,
                collectors,
                j);
            newPeers.emplace_back(&peers.back());
        }
        PeerGroup res{newPeers};
        allPeers = allPeers + res;
        return res;
    }

//！模拟中的对等数
    std::size_t
    size() const
    {
        return peers.size();
    }

    /*运行共识协议以生成所提供的分类账数量。

        让每一位同行达成共识，直到他们关闭更多的分类账。

        @param ledgers要关闭的附加分类帐的数目
    **/

    void
    run(int ledgers);

    /*在给定的持续时间内达成共识*/
    void
    run(SimDuration const& dur);

    /*检查组中的所有对等机是否同步。

        如果组中的节点最后共享同一个节点，则同步这些节点
        完全验证和最后生成的分类帐。
    **/

    bool
    synchronized(PeerGroup const& g) const;

    /*检查网络中的所有对等端是否同步
    **/

    bool
    synchronized() const;

    /*计算组中的分支数。

        如果组中的两个节点具有完全有效的分类帐，则会发生分支。
        它们不在同一个分类账链上。
    **/

    std::size_t
    branches(PeerGroup const& g) const;

    /*计算网络中的分支数
    **/

    std::size_t
    branches() const;

};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
