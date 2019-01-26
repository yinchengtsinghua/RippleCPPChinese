
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

#ifndef RIPPLE_TEST_CSF_UNL_H_INCLUDED
#define RIPPLE_TEST_CSF_UNL_H_INCLUDED

#include <test/csf/random.h>
#include <boost/container/flat_set.hpp>
#include <boost/optional.hpp>
#include <chrono>
#include <numeric>
#include <random>
#include <vector>

namespace ripple {
namespace test {
namespace csf {

/*信任图

    信任是从节点i到节点j的定向关系。
    如果节点i信任节点j，那么节点i的unl中有节点j。
    此类包装一个有向图，表示所有对等方的信任关系
    在模拟中。
**/

template <class Peer>
class TrustGraph
{
    using Graph = Digraph<Peer>;

    Graph graph_;

public:
    /*创建空信任关系图
     **/

    TrustGraph() = default;

    Graph const&
    graph()
    {
        return graph_;
    }

    /*创建信任

        在“从”和“到”之间建立信任；就像“从”和“放”到
        在它的UNL中。

        来自授予信任的对等方的@param
        @param到接受信任的对等机

    **/

    void
    trust(Peer const& from, Peer const& to)
    {
        graph_.connect(from, to);
    }

    /*移除信任

        从“从”到“从”的对等方撤消信任；就像“从”到“从”一样
        从它的UNL。

        对等撤销信任的@param
        被撤销的对等方的@param
    **/

    void
    untrust(Peer const& from, Peer const& to)
    {
        graph_.disconnect(from, to);
    }

//<是否从信托到
    bool
    trusts(Peer const& from, Peer const& to) const
    {
        return graph_.connected(from, to);
    }

    /*范围超过受信任的对等

        @param a授予信任的节点
        @return boost在节点“a”信任上转换的范围，即节点
                在它的UNL中
    **/

    auto
    trustedPeers(Peer const & a) const
    {
        return graph_.outVertices(a);
    }

    /*不通过白皮书无分叉条件的节点示例
    **/

    struct ForkInfo
    {
        std::set<Peer> unlA;
        std::set<Peer> unlB;
        int overlap;
        double required;
    };

//<返回不符合白皮书无分叉条件的节点
    std::vector<ForkInfo>
    forkablePairs(double quorum) const
    {
//通过查看交叉口检查分叉情况
//所有节点对之间的UNL。

//TODO:使用改进的装订而不是白皮书装订。

        using UNL = std::set<Peer>;
        std::set<UNL> unique;
        for (Peer const & peer : graph_.outVertices())
        {
            unique.emplace(
                std::begin(trustedPeers(peer)), std::end(trustedPeers(peer)));
        }

        std::vector<UNL> uniqueUNLs(unique.begin(), unique.end());
        std::vector<ForkInfo> res;

//循环所有uniqueunls对
        for (int i = 0; i < uniqueUNLs.size(); ++i)
        {
            for (int j = (i + 1); j < uniqueUNLs.size(); ++j)
            {
                auto const& unlA = uniqueUNLs[i];
                auto const& unlB = uniqueUNLs[j];
                double rhs =
                    2.0 * (1. - quorum) * std::max(unlA.size(), unlB.size());

                int intersectionSize = std::count_if(
                    unlA.begin(), unlA.end(), [&](Peer p) {
                        return unlB.find(p) != unlB.end();
                    });

                if (intersectionSize < rhs)
                {
                    res.emplace_back(ForkInfo{unlA, unlB, intersectionSize, rhs});
                }
            }
        }
        return res;
    }

    /*检查此信任图是否满足白皮书“不分叉”
        条件
    **/

    bool
    canFork(double quorum) const
    {
        return !forkablePairs(quorum).empty();
    }
};

}  //脑脊液
}  //测试
}  //涟漪

#endif
