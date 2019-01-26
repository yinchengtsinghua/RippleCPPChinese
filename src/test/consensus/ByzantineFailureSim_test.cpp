
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
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <test/csf.h>
#include <utility>

namespace ripple {
namespace test {

class ByzantineFailureSim_test : public beast::unit_test::suite
{
    void
    run() override
    {
        using namespace csf;
        using namespace std::chrono;

//此测试通过生成节点来模拟特定拓扑
//由于模拟拜占庭式故障（注入
//额外的非协商一致的交易）。

        Sim sim;
        ConsensusParms const parms{};

        SimDuration const delay =
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
        PeerGroup a = sim.createGroup(1);
        PeerGroup b = sim.createGroup(1);
        PeerGroup c = sim.createGroup(1);
        PeerGroup d = sim.createGroup(1);
        PeerGroup e = sim.createGroup(1);
        PeerGroup f = sim.createGroup(1);
        PeerGroup g = sim.createGroup(1);

        a.trustAndConnect(a + b + c + g, delay);
        b.trustAndConnect(b + a + c + d + e, delay);
        c.trustAndConnect(c + a + b + d + e, delay);
        d.trustAndConnect(d + b + c + e + f, delay);
        e.trustAndConnect(e + b + c + d + f, delay);
        f.trustAndConnect(f + d + e + g, delay);
        g.trustAndConnect(g + a + f, delay);

        PeerGroup network = a + b + c + d + e + f + g;

        StreamCollector sc{std::cout};

        sim.collectors.add(sc);

        for (TrustGraph<Peer*>::ForkInfo const& fi :
             sim.trustGraph.forkablePairs(0.8))
        {
            std::cout << "Can fork " << PeerGroup{fi.unlA} << " "
                      << " " << PeerGroup{fi.unlB}
                      << " overlap " << fi.overlap << " required "
                      << fi.required << "\n";
        };

//设置优先状态
        sim.run(1);

        PeerGroup byzantineNodes = a + b + c + g;
//所有同行都看到一些tx 0
        for (Peer * peer : network)
        {
            peer->submit(Tx(0));
//对等方0、1、2、6将通过注入不同方式关闭下一个分类账。
//未经协商一致同意的转运
            if (byzantineNodes.contains(peer))
            {
                peer->txInjections.emplace(
                    peer->lastClosedLedger.seq(), Tx{42});
            }
        }
        sim.run(4);
        std::cout << "Branches: " << sim.branches() << "\n";
        std::cout << "Fully synchronized: " << std::boolalpha
                  << sim.synchronized() << "\n";
//目前没有进行任何细化。
        pass();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(ByzantineFailureSim, consensus, ripple);

}  //命名空间测试
}  //命名空间波纹
