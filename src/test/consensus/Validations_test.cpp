
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

#include <ripple/basics/tagged_integer.h>
#include <ripple/beast/clock/manual_clock.h>
#include <ripple/beast/unit_test.h>
#include <ripple/consensus/Validations.h>
#include <test/csf/Validation.h>

#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>

namespace ripple {
namespace test {
namespace csf {
class Validations_test : public beast::unit_test::suite
{
    using clock_type = beast::abstract_clock<std::chrono::steady_clock> const;

//帮助将稳定时钟转换为合理的网络时钟
//这允许在单元测试中使用一个手动时钟。
    static NetClock::time_point
    toNetClock(clock_type const& c)
    {
//我们不关心实际的时代，但确实希望
//生成的netclock时间远远超过其时代，以确保
//任何减法都是正数
        using namespace std::chrono;
        return NetClock::time_point(duration_cast<NetClock::duration>(
            c.now().time_since_epoch() + 86400s));
    }

//表示可以发出验证的节点
    class Node
    {
        clock_type const& c_;
        PeerID nodeID_;
        bool trusted_ = true;
        std::size_t signIdx_ = 1;
        boost::optional<std::uint32_t> loadFee_;

    public:
        Node(PeerID nodeID, clock_type const& c) : c_(c), nodeID_(nodeID)
        {
        }

        void
        untrust()
        {
            trusted_ = false;
        }

        void
        trust()
        {
            trusted_ = true;
        }

        void
        setLoadFee(std::uint32_t fee)
        {
            loadFee_ = fee;
        }

        PeerID
        nodeID() const
        {
            return nodeID_;
        }

        void
        advanceKey()
        {
            signIdx_++;
        }

        PeerKey
        currKey() const
        {
            return std::make_pair(nodeID_, signIdx_);
        }

        PeerKey
        masterKey() const
        {
            return std::make_pair(nodeID_, 0);
        }
        NetClock::time_point
        now() const
        {
            return toNetClock(c_);
        }

//使用给定的序列号和ID和
//带有签名和可见时间，与普通时钟偏移
        Validation
        validate(
            Ledger::ID id,
            Ledger::Seq seq,
            NetClock::duration signOffset,
            NetClock::duration seenOffset,
            bool full) const
        {
            Validation v{id,
                         seq,
                         now() + signOffset,
                         now() + seenOffset,
                         currKey(),
                         nodeID_,
                         full,
                         loadFee_};
            if (trusted_)
                v.setTrusted();
            return v;
        }

        Validation
        validate(
            Ledger ledger,
            NetClock::duration signOffset,
            NetClock::duration seenOffset) const
        {
            return validate(
                ledger.id(), ledger.seq(), signOffset, seenOffset, true);
        }

        Validation
        validate(Ledger ledger) const
        {
            return validate(
                ledger.id(),
                ledger.seq(),
                NetClock::duration{0},
                NetClock::duration{0},
                true);
        }

        Validation
        partial(Ledger ledger) const
        {
            return validate(
                ledger.id(),
                ledger.seq(),
                NetClock::duration{0},
                NetClock::duration{0},
                false);
        }
    };

//保存旧数据以便在测试中检查
    struct StaleData
    {
        std::vector<Validation> stale;
        hash_map<PeerID, Validation> flushed;
    };

//通用验证适配器，用于将过时/刷新的数据保存到
//过时的数据实例。
    class Adaptor
    {
        StaleData& staleData_;
        clock_type& c_;
        LedgerOracle& oracle_;

    public:
//非锁定互斥体以避免在一般验证中锁定
        struct Mutex
        {
            void
            lock()
            {
            }

            void
            unlock()
            {
            }
        };

        using Validation = csf::Validation;
        using Ledger = csf::Ledger;

        Adaptor(StaleData& sd, clock_type& c, LedgerOracle& o)
            : staleData_{sd}, c_{c}, oracle_{o}
        {
        }

        NetClock::time_point
        now() const
        {
            return toNetClock(c_);
        }

        void
        onStale(Validation&& v)
        {
            staleData_.stale.emplace_back(std::move(v));
        }

        void
        flush(hash_map<PeerID, Validation>&& remaining)
        {
            staleData_.flushed = std::move(remaining);
        }

        boost::optional<Ledger>
        acquire(Ledger::ID const& id)
        {
            return oracle_.lookup(id);
        }
    };

//使用上述类型专门化通用验证
    using TestValidations = Validations<Adaptor>;

//在单个类中收集测试验证的依赖项并提供
//用于简化测试逻辑的访问器
    class TestHarness
    {
        StaleData staleData_;
        ValidationParms p_;
        beast::manual_clock<std::chrono::steady_clock> clock_;
        TestValidations tv_;
        PeerID nextNodeId_{0};

    public:
        explicit TestHarness(LedgerOracle& o)
            : tv_(p_, clock_, staleData_, clock_, o)
        {
        }

        ValStatus
        add(Validation const& v)
        {
            return tv_.add(v.nodeID(), v);
        }

        TestValidations&
        vals()
        {
            return tv_;
        }

        Node
        makeNode()
        {
            return Node(nextNodeId_++, clock_);
        }

        ValidationParms
        parms() const
        {
            return p_;
        }

        auto&
        clock()
        {
            return clock_;
        }

        std::vector<Validation> const&
        stale() const
        {
            return staleData_.stale;
        }

        hash_map<PeerID, Validation> const&
        flushed() const
        {
            return staleData_.flushed;
        }
    };

    Ledger const genesisLedger{Ledger::MakeGenesis{}};

    void
    testAddValidation()
    {
        using namespace std::chrono_literals;

        testcase("Add validation");
        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];
        Ledger ledgerAZ = h["az"];
        Ledger ledgerABC = h["abc"];
        Ledger ledgerABCD = h["abcd"];
        Ledger ledgerABCDE = h["abcde"];

        {
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            auto const v = n.validate(ledgerA);

//添加当前验证
            BEAST_EXPECT(ValStatus::current == harness.add(v));

//重新添加违反了增加的seq对full的要求
//验证
            BEAST_EXPECT(ValStatus::badSeq == harness.add(v));

            harness.clock().advance(1s);
//替换为新验证并确保旧验证已过时
            BEAST_EXPECT(harness.stale().empty());

            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerAB)));

            BEAST_EXPECT(harness.stale().size() == 1);

            BEAST_EXPECT(harness.stale()[0].ledgerID() == ledgerA.id());

//测试节点更改签名密钥

//确认现有的旧分类帐，但不确认新分类帐
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerAB.id()) == 1);
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerABC.id()) == 0);

//旋转签名键
            n.advanceKey();

            harness.clock().advance(1s);

//无法重新执行相同的完整验证序列
            BEAST_EXPECT(
                ValStatus::badSeq == harness.add(n.validate(ledgerAB)));
//无法发送相同的部分验证序列
            BEAST_EXPECT(ValStatus::badSeq == harness.add(n.partial(ledgerAB)));

//现在也信任最新的分类帐
            harness.clock().advance(1s);
            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerABC)));
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerAB.id()) == 1);
            BEAST_EXPECT(
                harness.vals().numTrustedForLedger(ledgerABC.id()) == 1);

//处理顺序错误的验证应忽略旧的
//验证
            harness.clock().advance(2s);
            auto const valABCDE = n.validate(ledgerABCDE);

            harness.clock().advance(4s);
            auto const valABCD = n.validate(ledgerABCD);

            BEAST_EXPECT(ValStatus::current == harness.add(valABCD));

            BEAST_EXPECT(ValStatus::stale == harness.add(valABCDE));
        }

        {
//处理有移位时间的无序验证

            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

//建立新的当前验证
            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerA)));

//处理具有“晚”序列但早签名时间的验证
            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(ledgerAB, -1s, -1s)));

//处理具有后续序列号和后续符号的验证
//时间
            BEAST_EXPECT(
                ValStatus::current ==
                harness.add(n.validate(ledgerABC, 1s, 1s)));
        }

        {
//到达时测试过时验证
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, -harness.parms().validationCURRENT_EARLY, 0s)));

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, harness.parms().validationCURRENT_WALL, 0s)));

            BEAST_EXPECT(
                ValStatus::stale ==
                harness.add(n.validate(
                    ledgerA, 0s, harness.parms().validationCURRENT_LOCAL)));
        }

        {
//测试不能为旧序列发送完整或部分内容
//数字，除非发生超时
            for (bool doFull : {true, false})
            {
                TestHarness harness(h.oracle);
                Node n = harness.makeNode();

                auto process = [&](Ledger& lgr) {
                    if (doFull)
                        return harness.add(n.validate(lgr));
                    return harness.add(n.partial(lgr));
                };

                BEAST_EXPECT(ValStatus::current == process(ledgerABC));
                harness.clock().advance(1s);
                BEAST_EXPECT(ledgerAB.seq() < ledgerABC.seq());
                BEAST_EXPECT(ValStatus::badSeq == process(ledgerAB));

//如果我们向前推进到AB到期，我们可以
//再次验证或部分验证序列号
                BEAST_EXPECT(ValStatus::badSeq == process(ledgerAZ));
                harness.clock().advance(
                    harness.parms().validationSET_EXPIRES + 1ms);
                BEAST_EXPECT(ValStatus::current == process(ledgerAZ));
            }
        }
    }

    void
    testOnStale()
    {
        testcase("Stale validation");
//验证验证是否仅基于时间传递而过时，但是
//使用不同的函数触发过时检查

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];

        using Trigger = std::function<void(TestValidations&)>;

        std::vector<Trigger> triggers = {
            [&](TestValidations& vals) { vals.currentTrusted(); },
            [&](TestValidations& vals) { vals.getCurrentNodeIDs(); },
            [&](TestValidations& vals) { vals.getPreferred(genesisLedger); },
            [&](TestValidations& vals) {
                vals.getNodesAfter(ledgerA, ledgerA.id());
            }};
        for (Trigger trigger : triggers)
        {
            TestHarness harness(h.oracle);
            Node n = harness.makeNode();

            BEAST_EXPECT(
                ValStatus::current == harness.add(n.validate(ledgerAB)));
            trigger(harness.vals());
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 1);
            BEAST_EXPECT(
                harness.vals().getPreferred(genesisLedger) ==
                std::make_pair(ledgerAB.seq(), ledgerAB.id()));
            BEAST_EXPECT(harness.stale().empty());
            harness.clock().advance(harness.parms().validationCURRENT_LOCAL);

//失效触发检查
            trigger(harness.vals());

            BEAST_EXPECT(harness.stale().size() == 1);
            BEAST_EXPECT(harness.stale()[0].ledgerID() == ledgerAB.id());
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 0);
            BEAST_EXPECT(
                harness.vals().getPreferred(genesisLedger) == boost::none);
        }
    }

    void
    testGetNodesAfter()
    {
//测试获取正在进行降序验证的节点数
//规定的。此计数应仅适用于受信任的节点，但
//包括部分和完全验证

        using namespace std::chrono_literals;
        testcase("Get nodes after");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];
        Ledger ledgerABC = h["abc"];
        Ledger ledgerAD = h["ad"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode();
        c.untrust();

//第一轮A，B，C同意，D有部分
        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerA)));

        for (Ledger const& ledger : {ledgerA, ledgerAB, ledgerABC, ledgerAD})
            BEAST_EXPECT(
                harness.vals().getNodesAfter(ledger, ledger.id()) == 0);

        harness.clock().advance(5s);

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerAB)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerABC)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerAB)));
        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerABC)));

        BEAST_EXPECT(harness.vals().getNodesAfter(ledgerA, ledgerA.id()) == 3);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAB, ledgerAB.id()) == 2);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerABC, ledgerABC.id()) == 0);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAD, ledgerAD.id()) == 0);

//如果给定的分类帐与ID不一致，仍然可以检查
//使用较慢的方法
        BEAST_EXPECT(harness.vals().getNodesAfter(ledgerAD, ledgerA.id()) == 1);
        BEAST_EXPECT(
            harness.vals().getNodesAfter(ledgerAD, ledgerAB.id()) == 2);
    }

    void
    testCurrentTrusted()
    {
        using namespace std::chrono_literals;
        testcase("Current trusted validations");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode();
        b.untrust();

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerB)));

//只有一个是可信的
        BEAST_EXPECT(harness.vals().currentTrusted().size() == 1);
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].ledgerID() == ledgerA.id());
        BEAST_EXPECT(harness.vals().currentTrusted()[0].seq() == ledgerA.seq());

        harness.clock().advance(3s);

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerAC)));

//的新验证
        BEAST_EXPECT(harness.vals().currentTrusted().size() == 1);
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].ledgerID() == ledgerAC.id());
        BEAST_EXPECT(
            harness.vals().currentTrusted()[0].seq() == ledgerAC.seq());

//给它足够的时间让它变质
        harness.clock().advance(harness.parms().validationCURRENT_LOCAL);
        BEAST_EXPECT(harness.vals().currentTrusted().empty());
    }

    void
    testGetCurrentPublicKeys()
    {
        using namespace std::chrono_literals;
        testcase("Current public keys");

        LedgerHistoryHelper h;
        Ledger ledgerA = h["a"];
        Ledger ledgerAC = h["ac"];

        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode();
        b.untrust();

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerA)));

        {
            hash_set<PeerID> const expectedKeys = {a.nodeID(), b.nodeID()};
            BEAST_EXPECT(harness.vals().getCurrentNodeIDs() == expectedKeys);
        }

        harness.clock().advance(3s);

//更改键并发布部分
        a.advanceKey();
        b.advanceKey();

        for (auto const& node : {a, b})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.partial(ledgerAC)));

        {
            hash_set<PeerID> const expectedKeys = {a.nodeID(), b.nodeID()};
            BEAST_EXPECT(harness.vals().getCurrentNodeIDs() == expectedKeys);
        }

//给他们足够的时间让他们变老
        harness.clock().advance(harness.parms().validationCURRENT_LOCAL);
        BEAST_EXPECT(harness.vals().getCurrentNodeIDs().empty());
    }

    void
    testTrustedByLedgerFunctions()
    {
//测试按分类帐ID计算值的验证功能
        using namespace std::chrono_literals;
        testcase("By ledger functions");

//多个验证函数返回一组关联的值
//受信任的分类帐共享相同的分类帐ID。下面的测试
//通过保存可信验证集来执行此逻辑，以及
//验证validations成员函数是否都计算
//适当转换现有分类账。

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);

        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode(),
             e = harness.makeNode();

        c.untrust();
//负荷费组合
        a.setLoadFee(12);
        b.setLoadFee(1);
        c.setLoadFee(12);
        e.setLoadFee(12);

        hash_map<Ledger::ID, std::vector<Validation>> trustedValidations;

//——————————————————————————————————————————————————————————————
//西洋跳棋
        auto sorted = [](auto vec) {
            std::sort(vec.begin(), vec.end());
            return vec;
        };
        auto compare = [&]() {
            for (auto& it : trustedValidations)
            {
                auto const& id = it.first;
                auto const& expectedValidations = it.second;

                BEAST_EXPECT(
                    harness.vals().numTrustedForLedger(id) ==
                    expectedValidations.size());
                BEAST_EXPECT(
                    sorted(harness.vals().getTrustedForLedger(id)) ==
                    sorted(expectedValidations));

                std::uint32_t baseFee = 0;
                std::vector<uint32_t> expectedFees;
                for (auto const& val : expectedValidations)
                {
                    expectedFees.push_back(val.loadFee().value_or(baseFee));
                }

                BEAST_EXPECT(
                    sorted(harness.vals().fees(id, baseFee)) ==
                    sorted(expectedFees));
            }
        };

//——————————————————————————————————————————————————————————————
        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];

//添加虚拟ID以覆盖未知的分类帐标识符
        trustedValidations[Ledger::ID{100}] = {};

//第一轮A，B，C同意
        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerA);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            if (val.trusted())
                trustedValidations[val.ledgerID()].emplace_back(val);
        }
//D不同意
        {
            auto const val = d.validate(ledgerB);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            trustedValidations[val.ledgerID()].emplace_back(val);
        }
//E只发布部分内容
        {
            BEAST_EXPECT(ValStatus::current == harness.add(e.partial(ledgerA)));
        }

        harness.clock().advance(5s);
//第二轮，A，B，C移到分类帐2
        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerAC);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            if (val.trusted())
                trustedValidations[val.ledgerID()].emplace_back(val);
        }
//D现在认为分类帐1，但不能重新发行以前使用的seq
        {
            BEAST_EXPECT(ValStatus::badSeq == harness.add(d.partial(ledgerA)));
        }
//E只发布部分内容
        {
            BEAST_EXPECT(
                ValStatus::current == harness.add(e.partial(ledgerAC)));
        }

        compare();
    }

    void
    testExpire()
    {
//验证过期清除分类帐存储的验证
        testcase("Expire validations");
        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();

        Ledger ledgerA = h["a"];

        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()));
        harness.clock().advance(harness.parms().validationSET_EXPIRES);
        harness.vals().expire();
        BEAST_EXPECT(!harness.vals().numTrustedForLedger(ledgerA.id()));
    }

    void
    testFlush()
    {
//测试验证的最终刷新
        using namespace std::chrono_literals;
        testcase("Flush validations");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode();
        c.untrust();

        Ledger ledgerA = h["a"];
        Ledger ledgerAB = h["ab"];

        hash_map<PeerID, Validation> expected;
        for (auto const& node : {a, b, c})
        {
            auto const val = node.validate(ledgerA);
            BEAST_EXPECT(ValStatus::current == harness.add(val));
            expected.emplace(node.nodeID(), val);
        }
        Validation staleA = expected.find(a.nodeID())->second;

//为发送新的验证，将新验证保存到预期的
//设置正确的以前分类帐ID后的映射
        harness.clock().advance(1s);
        auto newVal = a.validate(ledgerAB);
        BEAST_EXPECT(ValStatus::current == harness.add(newVal));
        expected.find(a.nodeID())->second = newVal;

//现在冲洗
        harness.vals().flush();

//原始验证已过时
        BEAST_EXPECT(harness.stale().size() == 1);
        BEAST_EXPECT(harness.stale()[0] == staleA);
        BEAST_EXPECT(harness.stale()[0].nodeID() == a.nodeID());

        auto const& flushed = harness.flushed();

        BEAST_EXPECT(flushed == expected);
    }

    void
    testGetPreferredLedger()
    {
        using namespace std::chrono_literals;
        testcase("Preferred Ledger");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode(), b = harness.makeNode(),
             c = harness.makeNode(), d = harness.makeNode();
        c.untrust();

        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerAC = h["ac"];
        Ledger ledgerACD = h["acd"];

        using Seq = Ledger::Seq;
        using ID = Ledger::ID;

        auto pref = [](Ledger ledger) {
            return std::make_pair(ledger.seq(), ledger.id());
        };

//空（无分类帐）
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == boost::none);

//单一分类帐
        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerB)));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerB));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerB));

//最小有效序列
        BEAST_EXPECT(
            harness.vals().getPreferred(ledgerA, Seq{10}) == ledgerA.id());

//不可信不影响首选分类账
//（Ledgerb具有连接断开Ledgra）
        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(ValStatus::current == harness.add(c.validate(ledgerA)));
        BEAST_EXPECT(ledgerB.id() > ledgerA.id());
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerB));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerB));

//部分断开连接
        BEAST_EXPECT(ValStatus::current == harness.add(d.partial(ledgerA)));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerA));
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerA));

        harness.clock().advance(5s);

//首选项的父项->使用分类帐
        for (auto const& node : {a, b, c, d})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerAC)));
//优先入住的父母
        BEAST_EXPECT(harness.vals().getPreferred(ledgerA) == pref(ledgerA));
//早期不同的链条、开关
        BEAST_EXPECT(harness.vals().getPreferred(ledgerB) == pref(ledgerAC));
//稍后在链条上，保持原位
        BEAST_EXPECT(harness.vals().getPreferred(ledgerACD) == pref(ledgerACD));

//以后的孙子或不同的链条是首选
        harness.clock().advance(5s);
        for (auto const& node : {a, b, c, d})
            BEAST_EXPECT(
                ValStatus::current == harness.add(node.validate(ledgerACD)));
        for (auto const& ledger : {ledgerA, ledgerB, ledgerACD})
            BEAST_EXPECT(
                harness.vals().getPreferred(ledger) == pref(ledgerACD));
    }

    void
    testGetPreferredLCL()
    {
        using namespace std::chrono_literals;
        testcase("Get preferred LCL");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();

        Ledger ledgerA = h["a"];
        Ledger ledgerB = h["b"];
        Ledger ledgerC = h["c"];

        using ID = Ledger::ID;
        using Seq = Ledger::Seq;

        hash_map<ID, std::uint32_t> peerCounts;

//当前分类账没有可信的验证或计数。
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerA.id());

        ++peerCounts[ledgerB.id()];

//没有可信验证，依赖对等方计数
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerB.id());

        ++peerCounts[ledgerC.id()];
//没有可信的验证，绑定的对等方使用更大的ID
        BEAST_EXPECT(ledgerC.id() > ledgerB.id());

        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerC.id());

        peerCounts[ledgerC.id()] += 1000;

//单信任总是胜过对等计数
        BEAST_EXPECT(ValStatus::current == harness.add(a.validate(ledgerA)));
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerA, Seq{0}, peerCounts) ==
            ledgerA.id());
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerB, Seq{0}, peerCounts) ==
            ledgerA.id());
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerC, Seq{0}, peerCounts) ==
            ledgerA.id());

//如果可信验证分类帐太旧，请使用当前分类帐
//序列的
        BEAST_EXPECT(
            harness.vals().getPreferredLCL(ledgerB, Seq{2}, peerCounts) ==
            ledgerB.id());
    }

    void
    testAcquireValidatedLedger()
    {
        using namespace std::chrono_literals;
        testcase("Acquire validated ledger");

        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();
        Node b = harness.makeNode();

        using ID = Ledger::ID;
        using Seq = Ledger::Seq;

//在分类帐实际可用之前验证它
        Validation val = a.validate(ID{2}, Seq{2}, 0s, 0s, true);

        BEAST_EXPECT(ValStatus::current == harness.add(val));
//验证可用
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{2}) == 1);
//但是基于分类帐的数据不是
        BEAST_EXPECT(harness.vals().getNodesAfter(genesisLedger, ID{0}) == 0);
//最初的首选分支机构会退回到我们正在尝试的分类账上。
//获得
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(Seq{2}, ID{2}));

//在添加另一个不可用的验证之后，首选分类帐
//通过更高的ID断开连接
        BEAST_EXPECT(
            ValStatus::current ==
            harness.add(b.validate(ID{3}, Seq{2}, 0s, 0s, true)));
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(Seq{2}, ID{3}));

//创建分类帐
        Ledger ledgerAB = h["ab"];
//现在应该有了
        BEAST_EXPECT(harness.vals().getNodesAfter(genesisLedger, ID{0}) == 1);

//创建不可用的验证
        harness.clock().advance(5s);
        Validation val2 = a.validate(ID{4}, Seq{4}, 0s, 0s, true);
        BEAST_EXPECT(ValStatus::current == harness.add(val2));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{4}) == 1);
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerAB.seq(), ledgerAB.id()));

//另一个节点请求分类帐仍然不更改内容
        Validation val3 = b.validate(ID{4}, Seq{4}, 0s, 0s, true);
        BEAST_EXPECT(ValStatus::current == harness.add(val3));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ID{4}) == 2);
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerAB.seq(), ledgerAB.id()));

//切换到可用的验证
        harness.clock().advance(5s);
        Ledger ledgerABCDE = h["abcde"];
        BEAST_EXPECT(ValStatus::current == harness.add(a.partial(ledgerABCDE)));
        BEAST_EXPECT(ValStatus::current == harness.add(b.partial(ledgerABCDE)));
        BEAST_EXPECT(
            harness.vals().getPreferred(genesisLedger) ==
            std::make_pair(ledgerABCDE.seq(), ledgerABCDE.id()));
    }

    void
    testNumTrustedForLedger()
    {
        testcase("NumTrustedForLedger");
        LedgerHistoryHelper h;
        TestHarness harness(h.oracle);
        Node a = harness.makeNode();
        Node b = harness.makeNode();
        Ledger ledgerA = h["a"];

        BEAST_EXPECT(ValStatus::current == harness.add(a.partial(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()) == 0);

        BEAST_EXPECT(ValStatus::current == harness.add(b.validate(ledgerA)));
        BEAST_EXPECT(harness.vals().numTrustedForLedger(ledgerA.id()) == 1);
    }

    void
    testSeqEnforcer()
    {
        testcase("SeqEnforcer");
        using Seq = Ledger::Seq;
        using namespace std::chrono;

        beast::manual_clock<steady_clock> clock;
        SeqEnforcer<Seq> enforcer;

        ValidationParms p;

        BEAST_EXPECT(enforcer(clock.now(), Seq{1}, p));
        BEAST_EXPECT(enforcer(clock.now(), Seq{10}, p));
        BEAST_EXPECT(!enforcer(clock.now(), Seq{5}, p));
        BEAST_EXPECT(!enforcer(clock.now(), Seq{9}, p));
        clock.advance(p.validationSET_EXPIRES - 1ms);
        BEAST_EXPECT(!enforcer(clock.now(), Seq{1}, p));
        clock.advance(2ms);
        BEAST_EXPECT(enforcer(clock.now(), Seq{1}, p));
    }

    void
    testTrustChanged()
    {
        testcase("TrustChanged");
        using namespace std::chrono;

        auto checker = [this](
                           TestValidations& vals,
                           hash_set<PeerID> const& listed,
                           std::vector<Validation> const& trustedVals) {
            Ledger::ID testID = trustedVals.empty() ? this->genesisLedger.id()
                                                    : trustedVals[0].ledgerID();
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(vals.getCurrentNodeIDs() == listed);
            BEAST_EXPECT(
                vals.getNodesAfter(this->genesisLedger, genesisLedger.id()) ==
                trustedVals.size());
            if (trustedVals.empty())
                BEAST_EXPECT(
                    vals.getPreferred(this->genesisLedger) == boost::none);
            else
                BEAST_EXPECT(
                    vals.getPreferred(this->genesisLedger)->second == testID);
            BEAST_EXPECT(vals.getTrustedForLedger(testID) == trustedVals);
            BEAST_EXPECT(
                vals.numTrustedForLedger(testID) == trustedVals.size());
        };

        {
//信任到不信任
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            Ledger ledgerAB = h["ab"];
            Validation v = a.validate(ledgerAB);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals({v});
            checker(harness.vals(), listed, trustedVals);

            trustedVals.clear();
            harness.vals().trustChanged({}, {a.nodeID()});
            checker(harness.vals(), listed, trustedVals);
        }

        {
//不受信任
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            a.untrust();
            Ledger ledgerAB = h["ab"];
            Validation v = a.validate(ledgerAB);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals;
            checker(harness.vals(), listed, trustedVals);

            trustedVals.push_back(v);
            harness.vals().trustChanged({a.nodeID()}, {});
            checker(harness.vals(), listed, trustedVals);
        }

        {
//可信但未获得->不可信
            LedgerHistoryHelper h;
            TestHarness harness(h.oracle);
            Node a = harness.makeNode();
            Validation v =
                a.validate(Ledger::ID{2}, Ledger::Seq{2}, 0s, 0s, true);
            BEAST_EXPECT(ValStatus::current == harness.add(v));

            hash_set<PeerID> listed({a.nodeID()});
            std::vector<Validation> trustedVals({v});
            auto& vals = harness.vals();
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(
                vals.getPreferred(genesisLedger)->second == v.ledgerID());
            BEAST_EXPECT(
                vals.getNodesAfter(genesisLedger, genesisLedger.id()) == 0);

            trustedVals.clear();
            harness.vals().trustChanged({}, {a.nodeID()});
//提供收单分类账
            h["ab"];
            BEAST_EXPECT(vals.currentTrusted() == trustedVals);
            BEAST_EXPECT(vals.getPreferred(genesisLedger) == boost::none);
            BEAST_EXPECT(
                vals.getNodesAfter(genesisLedger, genesisLedger.id()) == 0);
        }
    }

    void
    run() override
    {
        testAddValidation();
        testOnStale();
        testGetNodesAfter();
        testCurrentTrusted();
        testGetCurrentPublicKeys();
        testTrustedByLedgerFunctions();
        testExpire();
        testFlush();
        testGetPreferredLedger();
        testGetPreferredLCL();
        testAcquireValidatedLedger();
        testNumTrustedForLedger();
        testSeqEnforcer();
        testTrustChanged();
    }
};

BEAST_DEFINE_TESTSUITE(Validations, consensus, ripple);
}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹
