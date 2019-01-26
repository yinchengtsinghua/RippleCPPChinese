
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
    版权所有（c）2012-2016 Ripple Labs Inc.

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
#include <test/csf/random.h>
#include <utility>

namespace ripple {
namespace test {

class ScaleFreeSim_test : public beast::unit_test::suite
{
    void
    run() override
    {
        using namespace std::chrono;
        using namespace csf;

//生成准随机无标度网络并模拟一致性
//因为我们改变了交易提交率


int const N = 100;  //同龄人

int const numUNLs = 15;  //UNL列表
        int const minUNLSize = N / 4, maxUNLSize = N / 2;

        ConsensusParms const parms{};
        Sim sim;
        PeerGroup network = sim.createGroup(N);

//生成信任等级
        std::vector<double> const ranks =
            sample(network.size(), PowerLawDistribution{1, 3}, sim.rng);

//生成无标度信任图
        randomRankedTrust(network, ranks, numUNLs,
            std::uniform_int_distribution<>{minUNLSize, maxUNLSize},
            sim.rng);

//在任一方向上具有信任线的节点都是网络连接的
        network.connectFromTrust(
            date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

//初始化收集器以跟踪要报告的统计信息
        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

//初始轮设置先验状态
        sim.run(1);

//初始化计时器
        HeartbeatTimer heart(sim.scheduler, seconds(10s));

//运行10分钟，提交100 Tx/秒
        std::chrono::nanoseconds const simDuration = 10min;
        std::chrono::nanoseconds const quiet = 10s;
        Rate const rate{100, 1000ms};

//TXS，启动/停止/步骤，目标
        auto peerSelector = makeSelector(network.begin(),
                                     network.end(),
                                     ranks,
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                          sim.scheduler.now() + quiet,
                          sim.scheduler.now() + (simDuration - quiet),
                          peerSelector,
                          sim.scheduler,
                          sim.rng);

//在给定的持续时间内运行模拟
        heart.start();
        sim.run(simDuration);

        BEAST_EXPECT(sim.branches() == 1);
        BEAST_EXPECT(sim.synchronized());

//托多：清理这个格式混乱！！

        log << "Peers: " << network.size() << std::endl;
        log << "Simulated Duration: "
            << duration_cast<milliseconds>(simDuration).count()
            << " ms" << std::endl;
        log << "Branches: " << sim.branches() << std::endl;
        log << "Synchronized: " << (sim.synchronized() ? "Y" : "N")
            << std::endl;
        log << std::endl;

        txCollector.report(simDuration, log);
        ledgerCollector.report(simDuration, log);
//打印摘要？
//叉子？LCL的？
//对等体
//提交的TX
//账簿/秒等？
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(ScaleFreeSim, consensus, ripple, 80);

}  //命名空间测试
}  //命名空间波纹
