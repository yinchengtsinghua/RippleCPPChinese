
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
#include <utility>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <string>

namespace ripple {
namespace test {

/*用于多样化和分发验证器的进行中模拟
**/

class DistributedValidators_test : public beast::unit_test::suite
{

    void
    completeTrustCompleteConnectFixedDelay(
            std::size_t numPeers,
            std::chrono::milliseconds delay = std::chrono::milliseconds(200),
            bool printHeaders = false)
    {
        using namespace csf;
        using namespace std::chrono;

//初始化特定于此方法的永久收集器日志
        std::string const prefix =
                "DistributedValidators_"
                "completeTrustCompleteConnectFixedDelay";
        std::fstream
                txLog(prefix + "_tx.csv", std::ofstream::app),
                ledgerLog(prefix + "_ledger.csv", std::ofstream::app);

//标题
        log << prefix << "(" << numPeers << "," << delay.count() << ")"
            << std::endl;

//对等数、UNL、连接数
        BEAST_EXPECT(numPeers >= 1);

        Sim sim;
        PeerGroup peers = sim.createGroup(numPeers);

//完全信任图
        peers.trust(peers);

//具有固定延迟的完整连接图
        peers.connect(peers, delay);

//初始化收集器以跟踪要报告的统计信息
        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

//初始轮设置先验状态
        sim.run(1);

//运行10分钟，提交100 Tx/秒
        std::chrono::nanoseconds const simDuration = 10min;
        std::chrono::nanoseconds const quiet = 10s;
        Rate const rate{100, 1000ms};

//初始化计时器
        HeartbeatTimer heart(sim.scheduler);

//TXS，启动/停止/步骤，目标
        auto peerSelector = makeSelector(peers.begin(),
                                     peers.end(),
                                     std::vector<double>(numPeers, 1.),
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                                     sim.scheduler.now() + quiet,
                                     sim.scheduler.now() + simDuration - quiet,
                                     peerSelector,
                                     sim.scheduler,
                                     sim.rng);

//在给定的持续时间内运行模拟
        heart.start();
        sim.run(simDuration);

//Beast_Expect（sim.branches（）==1）；
//Beast_Expect（sim.synchronized（））；

        log << std::right;
        log << "| Peers: "<< std::setw(2) << peers.size();
        log << " | Duration: " << std::setw(6)
            << duration_cast<milliseconds>(simDuration).count() << " ms";
        log << " | Branches: " << std::setw(1) << sim.branches();
        log << " | Synchronized: " << std::setw(1)
            << (sim.synchronized() ? "Y" : "N");
        log << " |" << std::endl;

        txCollector.report(simDuration, log, true);
        ledgerCollector.report(simDuration, log, false);

        std::string const tag = std::to_string(numPeers);
        txCollector.csv(simDuration, txLog, tag, printHeaders);
        ledgerCollector.csv(simDuration, ledgerLog, tag, printHeaders);

        log << std::endl;
    }

    void
    completeTrustScaleFreeConnectFixedDelay(
            std::size_t numPeers,
            std::chrono::milliseconds delay = std::chrono::milliseconds(200),
            bool printHeaders = false)
    {
        using namespace csf;
        using namespace std::chrono;

//初始化特定于此方法的永久收集器日志
        std::string const prefix =
                "DistributedValidators__"
                "completeTrustScaleFreeConnectFixedDelay";
        std::fstream
                txLog(prefix + "_tx.csv", std::ofstream::app),
                ledgerLog(prefix + "_ledger.csv", std::ofstream::app);

//标题
        log << prefix << "(" << numPeers << "," << delay.count() << ")"
            << std::endl;

//对等数、UNL、连接数
        int const numCNLs    = std::max(int(1.00 * numPeers), 1);
        int const minCNLSize = std::max(int(0.25 * numCNLs),  1);
        int const maxCNLSize = std::max(int(0.50 * numCNLs),  1);
        BEAST_EXPECT(numPeers >= 1);
        BEAST_EXPECT(numCNLs >= 1);
        BEAST_EXPECT(1 <= minCNLSize
                && minCNLSize <= maxCNLSize
                && maxCNLSize <= numPeers);

        Sim sim;
        PeerGroup peers = sim.createGroup(numPeers);

//完全信任图
        peers.trust(peers);

//固定延迟无标度连接图
        std::vector<double> const ranks =
                sample(peers.size(), PowerLawDistribution{1, 3}, sim.rng);
        randomRankedConnect(peers, ranks, numCNLs,
                std::uniform_int_distribution<>{minCNLSize, maxCNLSize},
                sim.rng, delay);

//初始化收集器以跟踪要报告的统计信息
        TxCollector txCollector;
        LedgerCollector ledgerCollector;
        auto colls = makeCollectors(txCollector, ledgerCollector);
        sim.collectors.add(colls);

//初始轮设置先验状态
        sim.run(1);

//运行10分钟，提交100 Tx/秒
        std::chrono::nanoseconds simDuration = 10min;
        std::chrono::nanoseconds quiet = 10s;
        Rate rate{100, 1000ms};

//初始化计时器
        HeartbeatTimer heart(sim.scheduler);

//TXS，启动/停止/步骤，目标
        auto peerSelector = makeSelector(peers.begin(),
                                     peers.end(),
                                     std::vector<double>(numPeers, 1.),
                                     sim.rng);
        auto txSubmitter = makeSubmitter(ConstantDistribution{rate.inv()},
                                     sim.scheduler.now() + quiet,
                                     sim.scheduler.now() + simDuration - quiet,
                                     peerSelector,
                                     sim.scheduler,
                                     sim.rng);

//在给定的持续时间内运行模拟
        heart.start();
        sim.run(simDuration);

//Beast_Expect（sim.branches（）==1）；
//Beast_Expect（sim.synchronized（））；

        log << std::right;
        log << "| Peers: "<< std::setw(2) << peers.size();
        log << " | Duration: " << std::setw(6)
            << duration_cast<milliseconds>(simDuration).count() << " ms";
        log << " | Branches: " << std::setw(1) << sim.branches();
        log << " | Synchronized: " << std::setw(1)
            << (sim.synchronized() ? "Y" : "N");
        log << " |" << std::endl;

        txCollector.report(simDuration, log, true);
        ledgerCollector.report(simDuration, log, false);

        std::string const tag = std::to_string(numPeers);
        txCollector.csv(simDuration, txLog, tag, printHeaders);
        ledgerCollector.csv(simDuration, ledgerLog, tag, printHeaders);

        log << std::endl;
    }

    void
    run() override
    {
        std::string const defaultArgs = "5 200";
        std::string const args = arg().empty() ? defaultArgs : arg();
        std::stringstream argStream(args);

        int maxNumValidators = 0;
        int delayCount(200);
        argStream >> maxNumValidators;
        argStream >> delayCount;

        std::chrono::milliseconds const delay(delayCount);

        log << "DistributedValidators: 1 to " << maxNumValidators << " Peers"
            << std::endl;

        /*
         *模拟n=1到n
         *完成信任图
         *完整的网络连接
         *网络链接的固定延迟
         **/

        completeTrustCompleteConnectFixedDelay(1, delay, true);
        for(int i = 2; i <= maxNumValidators; i++)
        {
            completeTrustCompleteConnectFixedDelay(i, delay);
        }

        /*
         *模拟n=1到n
         *完成信任图
         *无标度网络连接
         *网络链接的固定延迟
         **/

        completeTrustScaleFreeConnectFixedDelay(1, delay, true);
        for(int i = 2; i <= maxNumValidators; i++)
        {
            completeTrustScaleFreeConnectFixedDelay(i, delay);
        }
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(DistributedValidators, consensus, ripple, 2);

}  //命名空间测试
}  //命名空间波纹
