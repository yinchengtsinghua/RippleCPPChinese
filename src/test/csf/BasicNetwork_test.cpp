
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#include <ripple/beast/unit_test.h>
#include <set>
#include <test/csf/BasicNetwork.h>
#include <test/csf/Scheduler.h>
#include <vector>

namespace ripple {
namespace test {

class BasicNetwork_test : public beast::unit_test::suite
{
public:
    struct Peer
    {
        int id;
        std::set<int> set;

        Peer(Peer const&) = default;
        Peer(Peer&&) = default;

        explicit Peer(int id_) : id(id_)
        {
        }

        template <class Net>
        void
        start(csf::Scheduler& scheduler, Net& net)
        {
            using namespace std::chrono_literals;
            auto t = scheduler.in(1s, [&] { set.insert(0); });
            if (id == 0)
            {
                for (auto const& link : net.links(this))
                    net.send(this, link.target, [&, to = link.target ] {
                        to->receive(net, this, 1);
                    });
            }
            else
            {
                scheduler.cancel(t);
            }
        }

        template <class Net>
        void
        receive(Net& net, Peer* from, int m)
        {
            set.insert(m);
            ++m;
            if (m < 5)
            {
                for (auto const& link : net.links(this))
                    net.send(this, link.target, [&, mm = m, to = link.target ] {
                        to->receive(net, this, mm);
                    });
            }
        }
    };

    void
    testNetwork()
    {
        using namespace std::chrono_literals;
        std::vector<Peer> pv;
        pv.emplace_back(0);
        pv.emplace_back(1);
        pv.emplace_back(2);
        csf::Scheduler scheduler;
        csf::BasicNetwork<Peer*> net(scheduler);
        BEAST_EXPECT(!net.connect(&pv[0], &pv[0]));
        BEAST_EXPECT(net.connect(&pv[0], &pv[1], 1s));
        BEAST_EXPECT(net.connect(&pv[1], &pv[2], 1s));
        BEAST_EXPECT(!net.connect(&pv[0], &pv[1]));
        for (auto& peer : pv)
            peer.start(scheduler, net);
        BEAST_EXPECT(scheduler.step_for(0s));
        BEAST_EXPECT(scheduler.step_for(1s));
        BEAST_EXPECT(scheduler.step());
        BEAST_EXPECT(!scheduler.step());
        BEAST_EXPECT(!scheduler.step_for(1s));
        net.send(&pv[0], &pv[1], [] {});
        net.send(&pv[1], &pv[0], [] {});
        BEAST_EXPECT(net.disconnect(&pv[0], &pv[1]));
        BEAST_EXPECT(!net.disconnect(&pv[0], &pv[1]));
        for (;;)
        {
            auto const links = net.links(&pv[1]);
            if (links.empty())
                break;
            BEAST_EXPECT(net.disconnect(&pv[1], links[0].target));
        }
        BEAST_EXPECT(pv[0].set == std::set<int>({0, 2, 4}));
        BEAST_EXPECT(pv[1].set == std::set<int>({1, 3}));
        BEAST_EXPECT(pv[2].set == std::set<int>({2, 4}));
    }

    void
    testDisconnect()
    {
        using namespace std::chrono_literals;
        csf::Scheduler scheduler;
        csf::BasicNetwork<int> net(scheduler);
        BEAST_EXPECT(net.connect(0, 1, 1s));
        BEAST_EXPECT(net.connect(0, 2, 2s));

        std::set<int> delivered;
        net.send(0, 1, [&]() { delivered.insert(1); });
        net.send(0, 2, [&]() { delivered.insert(2); });

        scheduler.in(1000ms, [&]() { BEAST_EXPECT(net.disconnect(0, 2)); });
        scheduler.in(1100ms, [&]() { BEAST_EXPECT(net.connect(0, 2)); });

        scheduler.step();

//只有第一条消息被传递，因为断开时间为1秒
//清除从0到2的所有挂起消息
        BEAST_EXPECT(delivered == std::set<int>({1}));
    }

    void
    run() override
    {
        testNetwork();
        testDisconnect();

    }
};

BEAST_DEFINE_TESTSUITE(BasicNetwork, test, ripple);

}  //命名空间测试
}  //命名空间波纹
