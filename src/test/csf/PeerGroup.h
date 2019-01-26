
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
#ifndef RIPPLE_TEST_CSF_PEERGROUP_H_INCLUDED
#define RIPPLE_TEST_CSF_PEERGROUP_H_INCLUDED

#include <algorithm>
#include <test/csf/Peer.h>
#include <test/csf/random.h>
#include <vector>

namespace ripple {
namespace test {
namespace csf {

/*一组模拟对等体

    PeerGroup是一个方便的处理程序，可以将对等机逻辑分组在一起，
    然后为整个团队创建信任或网络关系。同龄人
    组也可以组合在一起以构建更复杂的结构。

    PeerGroup提供随机访问样式迭代器和运算符[]
**/

class PeerGroup
{
    using peers_type = std::vector<Peer*>;
    peers_type peers_;
public:
    using iterator = peers_type::iterator;
    using const_iterator = peers_type::const_iterator;
    using reference = peers_type::reference;
    using const_reference = peers_type::const_reference;

    PeerGroup() = default;
    PeerGroup(Peer* peer) : peers_{1, peer}
    {
    }
    PeerGroup(std::vector<Peer*>&& peers) : peers_{std::move(peers)}
    {
        std::sort(peers_.begin(), peers_.end());
    }
    PeerGroup(std::vector<Peer*> const& peers) : peers_{peers}
    {
        std::sort(peers_.begin(), peers_.end());
    }

    PeerGroup(std::set<Peer*> const& peers) : peers_{peers.begin(), peers.end()}
    {
    }

    iterator
    begin()
    {
        return peers_.begin();
    }

    iterator
    end()
    {
        return peers_.end();
    }

    const_iterator
    begin() const
    {
        return peers_.begin();
    }

    const_iterator
    end() const
    {
        return peers_.end();
    }

    const_reference
    operator[](std::size_t i) const
    {
        return peers_[i];
    }

    bool
    contains(Peer const * p)
    {
        return std::find(peers_.begin(), peers_.end(), p) != peers_.end();
    }

    bool
    contains(PeerID id)
    {
        return std::find_if(peers_.begin(), peers_.end(), [id](Peer const* p) {
                   return p->id == id;
               }) != peers_.end();
    }

    std::size_t
    size() const
    {
        return peers_.size();
    }

    /*建立信任

        建立此组中所有对等方对O中所有对等方的信任

        @param o要信任的对等组
    **/

    void
    trust(PeerGroup const & o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->trust(*target);
            }
        }
    }

    /*撤销信托

        撤消此组中所有对等方对O中所有对等方的信任

        @param o不可信的对等组
    **/

    void
    untrust(PeerGroup const & o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->untrust(*target);
            }
        }
    }

    /*建立网络连接

        建立从该组中所有对等机到中所有对等机的出站连接
        o.如果连接已经存在，则不会建立新的连接。

        @param o要连接的对等组（将获得入站连接）
        @param delay所有已建立连接的固定消息传递延迟


    **/

    void
    connect(PeerGroup const& o, SimDuration delay)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
//无法通过网络向自身发送消息
                if(p != target)
                    p->connect(*target, delay);
            }
        }
    }

    /*破坏网络连接

        破坏此组中所有对等机与O中所有对等机之间的连接

        @param o要断开连接的对等组
    **/

    void
    disconnect(PeerGroup const &o)
    {
        for(Peer * p : peers_)
        {
            for (Peer * target : o.peers_)
            {
                p->disconnect(*target);
            }
        }
    }

    /*建立信任和网络连接

        建立信任并创建具有固定延迟的网络连接
        从该组中的所有对等机到O中的所有对等机

        @param o要信任并连接到的对等组
        @param delay所有已建立连接的固定消息传递延迟
    **/

    void
    trustAndConnect(PeerGroup const & o, SimDuration delay)
    {
        trust(o);
        connect(o, delay);
    }

    /*建立基于信任关系的网络连接

        对于此组中的每个对等端，创建出站网络连接
        它信任的一组同龄人。如果连接已经存在，则为
        没有重新创建。

        @param delay所有已建立连接的固定消息传递延迟

    **/

    void
    connectFromTrust(SimDuration delay)
    {
        for (Peer * peer : peers_)
        {
            for (Peer * to : peer->trustGraph.trustedPeers(peer))
            {
                peer->connect(*to, delay);
            }
        }
    }

//对等群联盟
    friend
    PeerGroup
    operator+(PeerGroup const & a, PeerGroup const & b)
    {
        PeerGroup res;
        std::set_union(
            a.peers_.begin(),
            a.peers_.end(),
            b.peers_.begin(),
            b.peers_.end(),
            std::back_inserter(res.peers_));
        return res;
    }

//设置对等组的差异
    friend
    PeerGroup
    operator-(PeerGroup const & a, PeerGroup const & b)
    {
        PeerGroup res;

        std::set_difference(
            a.peers_.begin(),
            a.peers_.end(),
            b.peers_.begin(),
            b.peers_.end(),
            std::back_inserter(res.peers_));

        return res;
    }

    friend std::ostream&
    operator<<(std::ostream& o, PeerGroup const& t)
    {
        o << "{";
        bool first = true;
        for (Peer const* p : t)
        {
            if(!first)
                o << ", ";
            first = false;
            o << p->id;
        }
        o << "}";
        return o;
    }
};

/*根据等级随机生成对等组。

    根据提供的对等方排名生成随机对等组。这个
    模拟随机生成UNL的过程，其中更“重要”的同龄人
    更可能出现在UNL中。

    'numgroups'子组是通过随机抽样生成的，没有
    根据“等级”替换同行。



    @param peers对等组
    @param对每个对等点的相对重要性进行排名，必须与
                 同龄人。相对等级越高，抽样的可能性就越大。
    @param numgroups要生成的对等链接组数
    @param sizedist确定链接组大小的分布
    @param g均匀随机位发生器

**/

template <class RandomNumberDistribution, class Generator>
std::vector<PeerGroup>
randomRankedGroups(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g)
{
    assert(peers.size() == ranks.size());

    std::vector<PeerGroup> groups;
    groups.reserve(numGroups);
    std::vector<Peer*> rawPeers(peers.begin(), peers.end());
    std::generate_n(std::back_inserter(groups), numGroups, [&]() {
        std::vector<Peer*> res = random_weighted_shuffle(rawPeers, ranks, g);
        res.resize(sizeDist(g));
        return PeerGroup(std::move(res));
    });

    return groups;
}



/*基于对等排名生成随机信任组。

    @有关参数的描述，请参阅RandomRankedGroups
**/

template <class RandomNumberDistribution, class Generator>
void
randomRankedTrust(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g)
{
    std::vector<PeerGroup> const groups =
        randomRankedGroups(peers, ranks, numGroups, sizeDist, g);

    std::uniform_int_distribution<int> u(0, groups.size() - 1);
    for(auto & peer : peers)
    {
        for(auto & target : groups[u(g)])
             peer->trust(*target);
    }
}

/*基于对等排名生成随机网络组。

    @有关参数的描述，请参阅RandomRankedGroups
**/

template <class RandomNumberDistribution, class Generator>
void
randomRankedConnect(
    PeerGroup & peers,
    std::vector<double> const & ranks,
    int numGroups,
    RandomNumberDistribution sizeDist,
    Generator& g,
    SimDuration delay)
{
    std::vector<PeerGroup> const groups =
        randomRankedGroups(peers, ranks, numGroups, sizeDist, g);

    std::uniform_int_distribution<int> u(0, groups.size() - 1);
    for(auto & peer : peers)
    {
        for(auto & target : groups[u(g)])
             peer->connect(*target, delay);
    }
}

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹
#endif

