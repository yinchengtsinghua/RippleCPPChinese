
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
#include <ripple/consensus/Consensus.h>
#include <ripple/consensus/ConsensusProposal.h>
#include <test/csf.h>
#include <test/unit_test/SuiteJournal.h>
#include <utility>

namespace ripple {
namespace test {

class Consensus_test : public beast::unit_test::suite
{
    SuiteJournal journal_;

public:
    Consensus_test ()
    : journal_ ("Consensus_test", *this)
    { }

    void
    testShouldCloseLedger()
    {
        using namespace std::chrono_literals;

//使用默认参数
        ConsensusParms const p{};

//离奇时刻被迫关闭
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, -10s, 10s, 1s, 1s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, 100h, 10s, 1s, 1s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 10, 10, 10s, 100h, 1s, 1s, p, journal_));

//网络其余部分已关闭
        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 3, 5, 10s, 10s, 10s, 10s, p, journal_));

//没有交易意味着等待Interval结束
        BEAST_EXPECT(
            !shouldCloseLedger(false, 10, 0, 0, 1s, 1s, 1s, 10s, p, journal_));
        BEAST_EXPECT(
            shouldCloseLedger(false, 10, 0, 0, 1s, 10s, 1s, 10s, p, journal_));

//强制最小分类帐打开时间
        BEAST_EXPECT(
            !shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 1s, 10s, p, journal_));

//不要走得比上次快太多
        BEAST_EXPECT(
            !shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 3s, 10s, p, journal_));

        BEAST_EXPECT(
            shouldCloseLedger(true, 10, 0, 0, 10s, 10s, 10s, 10s, p, journal_));
    }

    void
    testCheckConsensus()
    {
        using namespace std::chrono_literals;

//使用默认参数
        ConsensusParms const p{};

//时间不够了
        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 2, 0, 3s, 2s, p, true, journal_));

//如果没有足够的同龄人支持，确保
//有更多的时间提出建议
        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 2, 0, 3s, 4s, p, true, journal_));

//时间已经过去了，我们都同意
        BEAST_EXPECT(
            ConsensusState::Yes ==
            checkConsensus(10, 2, 2, 0, 3s, 10s, p, true, journal_));

//已经过了足够的时间，我们还不同意
        BEAST_EXPECT(
            ConsensusState::No ==
            checkConsensus(10, 2, 1, 0, 3s, 10s, p, true, journal_));

//我们的同龄人继续前进
//时间已经过去了，我们都同意
        BEAST_EXPECT(
            ConsensusState::MovedOn ==
            checkConsensus(10, 2, 1, 8, 3s, 10s, p, true, journal_));

//没有同龄人能轻易达成一致
        BEAST_EXPECT(
            ConsensusState::Yes ==
            checkConsensus(0, 0, 0, 0, 3s, 10s, p, true, journal_));
    }

    void
    testStandalone()
    {
        using namespace std::chrono_literals;
        using namespace csf;

        Sim s;
        PeerGroup peers = s.createGroup(1);
        Peer * peer = peers[0];
        peer->targetLedgers = 1;
        peer->start();
        peer->submit(Tx{1});

        s.scheduler.step();

//检查是否创建了正确的分类帐
        auto const& lcl = peer->lastClosedLedger;
        BEAST_EXPECT(peer->prevLedgerID() == lcl.id());
        BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
        BEAST_EXPECT(lcl.txs().size() == 1);
        BEAST_EXPECT(lcl.txs().find(Tx{1}) != lcl.txs().end());
        BEAST_EXPECT(peer->prevProposers == 0);
    }

    void
    testPeersAgree()
    {
        using namespace csf;
        using namespace std::chrono;

        ConsensusParms const parms{};
        Sim sim;
        PeerGroup peers = sim.createGroup(5);

//单固定延迟的连通信任图和网络图
        peers.trustAndConnect(
            peers, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

//每个人都将自己的ID作为TX提交
        for (Peer * p : peers)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));

        sim.run(1);

//所有对等机都处于同步状态
        if (BEAST_EXPECT(sim.synchronized()))
        {
            for (Peer const* peer : peers)
            {
                auto const& lcl = peer->lastClosedLedger;
                BEAST_EXPECT(lcl.id() == peer->prevLedgerID());
                BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
//建议所有同行
                BEAST_EXPECT(peer->prevProposers == peers.size() - 1);
//接受所有交易
                for (std::uint32_t i = 0; i < peers.size(); ++i)
                    BEAST_EXPECT(lcl.txs().find(Tx{i}) != lcl.txs().end());
            }
        }
    }

    void
    testSlowPeers()
    {
        using namespace csf;
        using namespace std::chrono;

//带同位子集的完全信任图的几个测试
//对其余的
//网络

//当一个缓慢的对等方不延迟协商一致的法定人数时进行测试（4/5同意）
        {
            ConsensusParms const parms{};
            Sim sim;
            PeerGroup slow = sim.createGroup(1);
            PeerGroup fast = sim.createGroup(4);
            PeerGroup network = fast + slow;

//完全连接信任图
            network.trust(network);

//快速和慢速网络连接
            fast.connect(
                fast, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

            slow.connect(
                network,
                date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

//所有对等方都将自己的ID作为事务提交
            for (Peer* peer : network)
                peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});

            sim.run(1);

//验证所有对等机具有相同的LCL，但缺少事务0
//所有对等端都与较慢的对等端0同步
            if (BEAST_EXPECT(sim.synchronized()))
            {
                for (Peer* peer : network)
                {
                    auto const& lcl = peer->lastClosedLedger;
                    BEAST_EXPECT(lcl.id() == peer->prevLedgerID());
                    BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});

                    BEAST_EXPECT(peer->prevProposers == network.size() - 1);
                    BEAST_EXPECT(
                        peer->prevRoundTime == network[0]->prevRoundTime);

                    BEAST_EXPECT(lcl.txs().find(Tx{0}) == lcl.txs().end());
                    for (std::uint32_t i = 2; i < network.size(); ++i)
                        BEAST_EXPECT(lcl.txs().find(Tx{i}) != lcl.txs().end());

//tx 0没有成功
                    BEAST_EXPECT(
                        peer->openTxs.find(Tx{0}) != peer->openTxs.end());
                }
            }
        }

//当缓慢的对等方延迟协商一致的法定人数时进行测试（4/6同意）
        {
//运行两个测试
//1。缓慢的同龄人正在达成共识。
//2。迟钝的同龄人只是在观察

            for (auto isParticipant : {true, false})
            {
                ConsensusParms const parms{};

                Sim sim;
                PeerGroup slow = sim.createGroup(2);
                PeerGroup fast = sim.createGroup(4);
                PeerGroup network = fast + slow;

//连接信任图
                network.trust(network);

//快速和慢速网络连接
                fast.connect(
                    fast,
                    date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

                slow.connect(
                    network,
                    date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

                for (Peer* peer : slow)
                    peer->runAsValidator = isParticipant;

//所有对等方都将自己的ID作为事务提交并中继它
//对同行
                for (Peer* peer : network)
                    peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});

                sim.run(1);

                if (BEAST_EXPECT(sim.synchronized()))
                {
//验证所有对等机是否具有相同的LCL，但丢失
//以前所有对等方都没有收到的事务0,1
//分类帐已关闭
                    for (Peer* peer : network)
                    {
//已关闭的分类帐除交易记录0,1外，其余均为已关闭的分类帐。
                        auto const& lcl = peer->lastClosedLedger;
                        BEAST_EXPECT(lcl.seq() == Ledger::Seq{1});
                        BEAST_EXPECT(lcl.txs().find(Tx{0}) == lcl.txs().end());
                        BEAST_EXPECT(lcl.txs().find(Tx{1}) == lcl.txs().end());
                        for (std::uint32_t i = slow.size(); i < network.size();
                             ++i)
                            BEAST_EXPECT(
                                lcl.txs().find(Tx{i}) != lcl.txs().end());

//tx 0-1没有成功
                        BEAST_EXPECT(
                            peer->openTxs.find(Tx{0}) != peer->openTxs.end());
                        BEAST_EXPECT(
                            peer->openTxs.find(Tx{1}) != peer->openTxs.end());
                    }

                    Peer const* slowPeer = slow[0];
                    if (isParticipant)
                        BEAST_EXPECT(
                            slowPeer->prevProposers == network.size() - 1);
                    else
                        BEAST_EXPECT(slowPeer->prevProposers == fast.size());

                    for (Peer* peer : fast)
                    {
//由于网络链路延迟设置
//同伴0最初提出0
//同行1最初提出1
//同行2-5最初提出2,3,4,5
//由于同行2-5同意，4/6>最初需要的50%
//包含有争议的事务，因此对等0/1交换机
//同意那些同行。对等0/1然后关闭
//80%的法定人数同意职位（5/6）匹配。
//
//同龄人2-5不会改变位置，因为tx 0或tx 1
//小于50%的初始阈值。他们也
//无法宣布达成共识，因为4/6同意
//位置小于80%阈值。因此他们需要
//额外的时间呼叫以查看更新的
//来自同行0和1的职位。

                        if (isParticipant)
                        {
                            BEAST_EXPECT(
                                peer->prevProposers == network.size() - 1);
                            BEAST_EXPECT(
                                peer->prevRoundTime > slowPeer->prevRoundTime);
                        }
                        else
                        {
                            BEAST_EXPECT(
                                peer->prevProposers == fast.size() - 1);
//所以所有的同龄人都应该团结在一起
                            BEAST_EXPECT(
                                peer->prevRoundTime == slowPeer->prevRoundTime);
                        }
                    }
                }

            }
        }
    }

    void
    testCloseTimeDisagree()
    {
        using namespace csf;
        using namespace std::chrono;

//这是一个非常专业的测试，以使分类账不同意
//结束时间。不幸的是，它假定人们了解当前的情况
//定时常数。这是一种必要的邪恶来掩盖
//等待更广泛的时间常数重构。

//为了同意在结束时间上的不同意见，必须没有
//明确大多数节点同意关闭时间。这个测试
//设置与对等机内部时钟的相对偏移量，以便
//发送不同时间的建议。

//然而，协议是在有效的关闭时间，而不是
//确切的关闭时间。最小CloseTimeResolution由以下公式给出：
//LedgerPossibleTimeResolutions[0]，当前为10秒。这意味着
//倾斜至少需要10秒才能有不同的效果
//关闭的时间。

//更复杂的是，节点将忽略建议
//在过去，超过命题的次数为20次。所以
//最小粒度，我们最多有3种歪斜
//（0S，10s，20s）。

//因此，该测试有6个节点，其中2个节点具有每种类型的
//歪斜。那么大多数（1/3<1/2）节点都不会同意
//实际关闭时间。

        ConsensusParms const parms{};
        Sim sim;

        PeerGroup groupA = sim.createGroup(2);
        PeerGroup groupB = sim.createGroup(2);
        PeerGroup groupC = sim.createGroup(2);
        PeerGroup network = groupA + groupB + groupC;

        network.trust(network);
        network.connect(
            network, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

//在短时间内达成一致意见
//分辨率
        Peer* firstPeer = *groupA.begin();
        while (firstPeer->lastClosedLedger.closeTimeResolution() >=
               parms.proposeFRESHNESS)
            sim.run(1);

//对2/3的同龄人进行轮班
        for (Peer* peer : groupA)
            peer->clockSkew = parms.proposeFRESHNESS / 2;
        for (Peer* peer : groupB)
            peer->clockSkew = parms.proposeFRESHNESS;

        sim.run(1);

//所有节点同意在关闭时间不一致
        if (BEAST_EXPECT(sim.synchronized()))
        {
            for (Peer* peer : network)
                BEAST_EXPECT(!peer->lastClosedLedger.closeAgree());
        }
    }

    void
    testWrongLCL()
    {
        using namespace csf;
        using namespace std::chrono;
//一种专门的测试，用以锻炼一些同龄人的临时叉子。
//正在处理不正确的上一个分类帐。

        ConsensusParms const parms{};

//更改处理验证所需的时间以进行检测
//不同共识阶段的错误LCL
        for (auto validationDelay : {0ms, parms.ledgerMIN_CLOSE})
        {
//考虑10个同龄人：
//0 1 2 3 4 5 6 7 8 9
//少数民族多数A多数B
//
//节点0-1信任节点0-4
//节点2-9信任节点2-9
//
//通过向节点0-4提交Tx 0，向节点5-9提交Tx 1，
//节点0-1将生成错误的LCL（使用tx 0）。剩余的
//节点将接受带有tx 1的分类帐。

//节点0-1将在下一轮中检测到这种不匹配
//因为节点2-4将验证不同的分类帐。

//节点0-1将从网络获取适当的分类账，并且
//恢复共识，最终形成主导网络
//分类帐。

//此拓扑可能与上述信任关系分叉
//但这是为了测试。

            Sim sim;

            PeerGroup minority = sim.createGroup(2);
            PeerGroup majorityA = sim.createGroup(3);
            PeerGroup majorityB = sim.createGroup(5);

            PeerGroup majority = majorityA + majorityB;
            PeerGroup network = minority + majority;

            SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
            minority.trustAndConnect(minority + majorityA, delay);
            majority.trustAndConnect(majority, delay);

            CollectByNode<JumpCollector> jumps;
            sim.collectors.add(jumps);

            BEAST_EXPECT(sim.trustGraph.canFork(parms.minCONSENSUS_PCT / 100.));

//初始轮设置先验状态
            sim.run(1);

//较小UNL中的节点已看到TX 0，其他UNL中的节点已看到
//德克萨斯州1
            for (Peer* peer : network)
                peer->delays.recvValidation = validationDelay;
            for (Peer* peer : (minority + majorityA))
                peer->openTxs.insert(Tx{0});
            for (Peer* peer : majorityB)
                peer->openTxs.insert(Tx{1});

//再跑几轮
//没有验证延迟，只需要再进行2轮。
//1。循环生成不同的分类帐
//2。循环以检测不同的先前分类账（但仍然生成
//错误的）并在那一轮中恢复，因为错误的LCL
//在我们关闭之前检测到
//
//由于Ledgermin关闭的验证延迟，我们还需要3个
//回合。
//1。循环生成不同的分类帐
//2。循环以检测不同的先前分类账（但仍然生成
//错误的）但最终宣布对错误的LCL达成共识（但是
//使用正确的事务集！）这是因为我们发现
//在我们关闭分类账后，出现了错误的LCL，因此我们声明
//仅基于同行建议的共识。但我们没有
//有时间获得正确的分类帐。
//三。四舍五入修正
            sim.run(3);

//网络从未真正分叉，因为节点0-1从未看到
//完全验证不正确链的验证仲裁。

//但是，对于非零验证延迟，网络不是
//同步，因为节点0和1在后面运行一个分类帐
            if (BEAST_EXPECT(sim.branches() == 1))
            {
                for(Peer const* peer : majority)
                {
//大多数节点没有跳转
                    BEAST_EXPECT(jumps[peer->id].closeJumps.empty());
                    BEAST_EXPECT(jumps[peer->id].fullyValidatedJumps.empty());
                }
                for(Peer const* peer : minority)
                {
                    auto & peerJumps = jumps[peer->id];
//链之间上次关闭的分类帐跳转
                    {
                        if (BEAST_EXPECT(peerJumps.closeJumps.size() == 1))
                        {
                            JumpCollector::Jump const& jump =
                                peerJumps.closeJumps.front();
//跳到另一条链子上
                            BEAST_EXPECT(jump.from.seq() <= jump.to.seq());
                            BEAST_EXPECT(!jump.to.isAncestor(jump.from));
                        }
                    }
//在同一条链中完全验证的向前跳
                    {
                        if (BEAST_EXPECT(
                                peerJumps.fullyValidatedJumps.size() == 1))
                        {
                            JumpCollector::Jump const& jump =
                                peerJumps.fullyValidatedJumps.front();
//跳转到具有相同序列的不同链
                            BEAST_EXPECT(jump.from.seq() < jump.to.seq());
                            BEAST_EXPECT(jump.to.isAncestor(jump.from));
                        }
                    }
                }
            }
        }

        {
//为在建立期间切换LCL而设计的附加测试
//相位。这是为了触发之前
//崩溃，开关LCL从建立切换到打开
//阶段，但仍处理建立阶段逻辑。

//loner节点将接受初始分类账A，但所有其他节点
//稍后接受分类帐B。通过拖延时间
//为了进行验证，loner节点将检测错误的LCL
//在它已经进入下一轮的建立阶段之后。

            Sim sim;
            PeerGroup loner = sim.createGroup(1);
            PeerGroup friends = sim.createGroup(3);
            loner.trust(loner + friends);

            PeerGroup others = sim.createGroup(6);
            PeerGroup clique = friends + others;
            clique.trust(clique);

            PeerGroup network = loner + clique;
            network.connect(
                network,
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));

//初始轮设置先验状态
            sim.run(1);
            for (Peer* peer : (loner + friends))
                peer->openTxs.insert(Tx(0));
            for (Peer* peer : others)
                peer->openTxs.insert(Tx(1));

//延迟验证处理
            for (Peer* peer : network)
                peer->delays.recvValidation = parms.ledgerGRANULARITY;

//产生错误LCL和恢复的额外回合
            sim.run(2);

//检查恢复的所有对等机
            for (Peer * p: network)
                BEAST_EXPECT(p->prevLedgerID() == network[0]->prevLedgerID());
        }
    }

    void
    testConsensusCloseTimeRounding()
    {
        using namespace csf;
        using namespace std::chrono;

//这是一个专门的测试，设计用于产生不同
//即使同龄人认为他们的时间很近
//在分类帐上达成共识。

        for (bool useRoundedCloseTime : {false, true})
        {
            ConsensusParms parms;
            parms.useRoundedCloseTime = useRoundedCloseTime;

            Sim sim;

//这需要一组4个快速和2个慢速对等点来创建
//在这种情况下，一个子集的同龄人需要看到额外的
//宣布达成共识的建议。
            PeerGroup slow = sim.createGroup(2);
            PeerGroup fast = sim.createGroup(4);
            PeerGroup network = fast + slow;

            for (Peer* peer : network)
                peer->consensusParms = parms;

//连接信任图
            network.trust(network);

//快速和慢速网络连接
            fast.connect(
                fast, date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY));
            slow.connect(
                network,
                date::round<milliseconds>(1.1 * parms.ledgerGRANULARITY));

//在*降低分辨率之前运行到分类帐*。
            sim.run(increaseLedgerTimeResolutionEvery - 2);

//为了建立谨慎，我们需要一个案例，如果
//x=efffclosetime（关闭时间、分辨率、父关闭时间）
//X！=effclosetime（x，分辨率，parentclosetime）
//
//也就是说，有效关闭时间不是一个固定点。这个罐头
//如果x=parentCloseTime+1，则发生，但后续舍入
//分辨率的下一个最高倍数。

//所以我们要找到一个偏移量（现在+偏移量）%30s=15
//（现在+偏移量）%20s=15
//这样，下一个分类账将由于
//网络延时设置，达成共识需要5秒，所以
//下一个分类帐的关闭时间将

            NetClock::duration when = network[0]->now().time_since_epoch();

//检查一下我们在30到20岁的过渡期之前
            NetClock::duration resolution =
                network[0]->lastClosedLedger.closeTimeResolution();
            BEAST_EXPECT(resolution == NetClock::duration{30s});

            while (
                ((when % NetClock::duration{30s}) != NetClock::duration{15s}) ||
                ((when % NetClock::duration{20s}) != NetClock::duration{15s}))
                when += 1s;
//在没有协商一致的情况下提前时钟（这是什么？
//在实践中阻止它？）
            sim.scheduler.step_for(
                NetClock::time_point{when} - network[0]->now());

//再运行一个30秒分辨率的分类帐
            sim.run(1);
            if (BEAST_EXPECT(sim.synchronized()))
            {
//关闭时间应该早于时钟时间，因为我们设计了
//围捕的最后时刻
                for (Peer* peer : network)
                {
                    BEAST_EXPECT(
                        peer->lastClosedLedger.closeTime() > peer->now());
                    BEAST_EXPECT(peer->lastClosedLedger.closeAgree());
                }
            }

//所有对等方都将自己的ID作为事务提交
            for (Peer* peer : network)
                peer->submit(Tx{static_cast<std::uint32_t>(peer->id)});

//再跑一圈，这次会减少
//分辨率为20秒。

//对网络延迟进行了设计，以使速度较慢的对等机
//最初有错误的Tx哈希，但他们看到的大多数
//与同行达成一致并宣布达成共识
//
//诀窍是每个人都从一个原始的结束时间开始
//8461S
//哪个有
//有效关闭时间（86481s，20s，86490s）=86491s
//然而，当缓慢的同伴更新他们的位置时，他们会改变
//接近86451秒。快速同行宣布同意
//86481仍然是他们的位置。
//
//接受分类帐时
//-快速同行使用eff（86481s）->86491s作为关闭时间
//-慢同龄人使用eff（eff（86481s））->eff（86491s）->86500s！

            sim.run(1);

            if (parms.useRoundedCloseTime)
            {
                BEAST_EXPECT(sim.synchronized());
            }
            else
            {
//当前未同步
                BEAST_EXPECT(!sim.synchronized());

//所有慢同行都同意LCL
                BEAST_EXPECT(std::all_of(
                    slow.begin(), slow.end(), [&slow](Peer const* p) {
                        return p->lastClosedLedger.id() ==
                            slow[0]->lastClosedLedger.id();
                    }));

//所有快速同行都同意LCL
                BEAST_EXPECT(std::all_of(
                    fast.begin(), fast.end(), [&fast](Peer const* p) {
                        return p->lastClosedLedger.id() ==
                            fast[0]->lastClosedLedger.id();
                    }));

                Ledger const& slowLCL = slow[0]->lastClosedLedger;
                Ledger const& fastLCL = fast[0]->lastClosedLedger;

//同意家长关闭和关闭解决方案
                BEAST_EXPECT(
                    slowLCL.parentCloseTime() == fastLCL.parentCloseTime());
                BEAST_EXPECT(
                    slowLCL.closeTimeResolution() ==
                    fastLCL.closeTimeResolution());

//关闭时间不一致…
                BEAST_EXPECT(slowLCL.closeTime() != fastLCL.closeTime());
//有效关闭时间同意！缓慢的同伴已经被包围了！
                BEAST_EXPECT(
                    effCloseTime(
                        slowLCL.closeTime(),
                        slowLCL.closeTimeResolution(),
                        slowLCL.parentCloseTime()) ==
                    effCloseTime(
                        fastLCL.closeTime(),
                        fastLCL.closeTimeResolution(),
                        fastLCL.parentCloseTime()));
            }
        }
    }

    void
    testFork()
    {
        using namespace csf;
        using namespace std::chrono;

        std::uint32_t numPeers = 10;
//两个UNL之间的重叠变化
        for (std::uint32_t overlap = 0; overlap <= numPeers; ++overlap)
        {
            ConsensusParms const parms{};
            Sim sim;

            std::uint32_t numA = (numPeers - overlap) / 2;
            std::uint32_t numB = numPeers - numA - overlap;

            PeerGroup aOnly = sim.createGroup(numA);
            PeerGroup bOnly = sim.createGroup(numB);
            PeerGroup commonOnly = sim.createGroup(overlap);

            PeerGroup a = aOnly + commonOnly;
            PeerGroup b = bOnly + commonOnly;

            PeerGroup network = a + b;

            SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
            a.trustAndConnect(a, delay);
            b.trustAndConnect(b, delay);

//初始轮设置先验状态
            sim.run(1);
            for (Peer* peer : network)
            {
//节点只看到来自其邻居的事务
                peer->openTxs.insert(Tx{static_cast<std::uint32_t>(peer->id)});
                for (Peer* to : sim.trustGraph.trustedPeers(peer))
                    peer->openTxs.insert(
                        Tx{static_cast<std::uint32_t>(to->id)});
            }
            sim.run(1);

//40%或更大的重叠不应出现分叉。
//因为重叠节点有一个unl，它是
//两个集团，最大规模的unl列表是同龄人的数量
            if (overlap > 0.4 * numPeers)
                BEAST_EXPECT(sim.synchronized());
            else
            {
//即使我们做叉，也不应该超过3个分类账。
//一个用于cliquea，一个用于cliqueb，一个用于两个节点
                BEAST_EXPECT(sim.branches() <= 3);
            }
        }
    }

    void
    testHubNetwork()
    {
        using namespace csf;
        using namespace std::chrono;

//模拟一组5个不直接连接但
//依靠单个集线器节点进行通信

        ConsensusParms const parms{};
        Sim sim;
        PeerGroup validators = sim.createGroup(5);
        PeerGroup center = sim.createGroup(1);
        validators.trust(validators);
        center.trust(validators);

        SimDuration delay =
                date::round<milliseconds>(0.2 * parms.ledgerGRANULARITY);
        validators.connect(center, delay);

        center[0]->runAsValidator = false;

//准备回合以设置初始状态。
        sim.run(1);

//每个人都将自己的ID作为一个Tx提交，并将其转发给同行。
        for (Peer * p : validators)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));

        sim.run(1);

//所有对等机都处于同步状态
        BEAST_EXPECT(sim.synchronized());
    }


//TestPreferredByBranch的帮助程序收集器
//在不好的时候侵入性地断开网络以引起分裂
    struct Disruptor
    {
        csf::PeerGroup& network;
        csf::PeerGroup& groupCfast;
        csf::PeerGroup& groupCsplit;
        csf::SimDuration delay;
        bool reconnected = false;

        Disruptor(
            csf::PeerGroup& net,
            csf::PeerGroup& c,
            csf::PeerGroup& split,
            csf::SimDuration d)
            : network(net), groupCfast(c), groupCsplit(split), delay(d)
        {
        }

        template <class E>
        void
        on(csf::PeerID, csf::SimTime, E const&)
        {
        }


        void
        on(csf::PeerID who, csf::SimTime, csf::FullyValidateLedger const& e)
        {
            using namespace std::chrono;
//一旦fastc节点完全验证c，就断开连接
//网络中的所有C节点。快速C节点需要断开连接
//为了防止它转发它看到的验证
            if (who == groupCfast[0]->id &&
                e.ledger.seq() == csf::Ledger::Seq{2})
            {
                network.disconnect(groupCsplit);
                network.disconnect(groupCfast);
            }
        }

        void
        on(csf::PeerID who, csf::SimTime, csf::AcceptLedger const& e)
        {
//一旦任何人生成b或c的子代，重新连接
//通过网络进行验证
            if (!reconnected && e.ledger.seq() == csf::Ledger::Seq{3})
            {
                reconnected = true;
                network.connect(groupCsplit, delay);
            }
        }


    };

    void
    testPreferredByBranch()
    {
        using namespace csf;
        using namespace std::chrono;

//模拟使用时阻止分叉的网络拆分
//特里尔的首选分类账。这是一个做作的例子，涉及到
//过度的网络分裂，但显示出安全性的提高。
//从首选分类账中按三重法。

//考虑10个验证节点，其中包含一个公共UNL
//分类帐历史：
//1：A
/// \
//2:乙丙
//“/”/ \
//3:D C'___（8个不同的分类账）

//-所有节点生成公共分类账A
//-2个节点生成B，8个节点生成C
//-只有1个C节点看到所有C验证并完全
//验证C。其余C节点在正确的时间拆分
//这样他们就不会看到任何C验证，只看到他们自己的验证。
//-C节点继续生成8个不同的子分类账。
//-同时，D节点只看到1个C验证和2个验证
//为B
//-网络重新连接并验证第3代分类账
//观察（D和8 C）
//-在旧方法中，D的2票比C的1票多。
//所以网络会向D雪崩并完全验证它
//即使C被一个节点完全验证
//-在新方法中，2票赞成D票不足以超过
//8对C的隐式投票，因此节点将雪崩到C


        ConsensusParms const parms{};
        Sim sim;

//A-＞B->
        PeerGroup groupABD = sim.createGroup(2);
//在拆分前完全验证C的单个节点
        PeerGroup groupCfast = sim.createGroup(1);
//生成C，但未能在拆分前完全验证
        PeerGroup groupCsplit = sim.createGroup(7);

        PeerGroup groupNotFastC = groupABD + groupCsplit;
        PeerGroup network = groupABD + groupCsplit + groupCfast;

        SimDuration delay = date::round<milliseconds>(
            0.2 * parms.ledgerGRANULARITY);
        SimDuration fDelay = date::round<milliseconds>(
            0.1 * parms.ledgerGRANULARITY);

        network.trust(network);
//C必须有一个较短的延迟，以便在
//其他节点
        network.connect(groupCfast, fDelay);
//网络的其余部分以相同的速度连接
        groupNotFastC.connect(groupNotFastC, delay);

        Disruptor dc(network, groupCfast, groupCsplit, delay);
        sim.collectors.add(dc);

//协商一致生成分类账A
        sim.run(1);
        BEAST_EXPECT(sim.synchronized());

//下一轮产生B和C
//为了强制B，我们向这些节点注入一个额外的事务
        for(Peer * peer : groupABD)
        {
            peer->txInjections.emplace(
                    peer->lastClosedLedger.seq(), Tx{42});
        }
//中断器将确保节点在C之前断开
//除了fastc节点之外，其他所有节点都通过了验证
        sim.run(1);

//我们不再同步，但尚未分叉：
//9个节点考虑最后一个完全验证的分类账，fastc看到c
        BEAST_EXPECT(!sim.synchronized());
        BEAST_EXPECT(sim.branches() == 1);

//再运行一轮以生成8个不同的C'分类账
        for (Peer * p : network)
            p->submit(Tx(static_cast<std::uint32_t>(p->id)));
        sim.run(1);

//仍然没有分叉
        BEAST_EXPECT(!sim.synchronized());
        BEAST_EXPECT(sim.branches() == 1);

//中断器将重新连接除fastc节点之外的所有节点
        sim.run(1);

        if(BEAST_EXPECT(sim.branches() == 1))
        {
            BEAST_EXPECT(sim.synchronized());
        }
else //旧方法引起了分歧
        {
            BEAST_EXPECT(sim.branches(groupNotFastC) == 1);
            BEAST_EXPECT(sim.synchronized(groupNotFastC) == 1);
        }
    }

    void
    run() override
    {
        testShouldCloseLedger();
        testCheckConsensus();

        testStandalone();
        testPeersAgree();
        testSlowPeers();
        testCloseTimeDisagree();
        testWrongLCL();
        testConsensusCloseTimeRounding();
        testFork();
        testHubNetwork();
        testPreferredByBranch();
    }
};

BEAST_DEFINE_TESTSUITE(Consensus, consensus, ripple);
}  //命名空间测试
}  //命名空间波纹
