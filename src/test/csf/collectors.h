
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
#ifndef RIPPLE_TEST_CSF_COLLECTORS_H_INCLUDED
#define RIPPLE_TEST_CSF_COLLECTORS_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <boost/optional.hpp>
#include <chrono>
#include <ostream>
#include <test/csf/Histogram.h>
#include <test/csf/SimTime.h>
#include <test/csf/events.h>
#include <tuple>

namespace ripple {
namespace test {
namespace csf {

//收集器是实现
//
//打开（nodeid、simtime、event）
//
//对于对等机发出的所有事件。
//
//此文件包含用于组合不同收集器的帮助程序函数
//还定义了几个可用于模拟的标准收集器。


/*收藏组。

    将一组收集器表示为处理事件的单个收集器
    按顺序调用每个收集器。这是对管理员的分析。
    在collectorRef.h中，但*不*删除组合的类型信息
    收藏家。
 **/

template <class... Cs>
class Collectors
{
    std::tuple<Cs&...> cs;

    template <class C, class E>
    static void
    apply(C& c, PeerID who, SimTime when, E e)
    {
        c.on(who, when, e);
    }

    template <std::size_t... Is, class E>
    static void
    apply(
        std::tuple<Cs&...>& cs,
        PeerID who,
        SimTime when,
        E e,
        std::index_sequence<Is...>)
    {
//肖恩父FosieAch参数技巧（C++折叠表达式）
//这里很好）
        (void)std::array<int, sizeof...(Cs)>{
            {((apply(std::get<Is>(cs), who, when, e)), 0)...}};
    }

public:
    /*构造函数

        @param cs引用要一起调用的收集器
    **/

    Collectors(Cs&... cs) : cs(std::tie(cs...))
    {
    }

    template <class E>
    void
    on(PeerID who, SimTime when, E e)
    {
        apply(cs, who, when, e, std::index_sequence_for<Cs...>{});
    }
};

/*创建Collectors的实例*/
template <class... Cs>
Collectors<Cs...>
makeCollectors(Cs&... cs)
{
    return Collectors<Cs...>(cs...);
}

/*为每个对等端维护收集器的实例

    对于发出事件的每个对等机，该类维护一个对应的
    CollectorType的实例，仅将对等方发出的事件转发到
    相关实例。

    CollectorType应该是默认的可构造类型。
**/

template <class CollectorType>
struct CollectByNode
{
    std::map<PeerID, CollectorType> byNode;

    CollectorType&
    operator[](PeerID who)
    {
        return byNode[who];
    }

    CollectorType const&
    operator[](PeerID who) const
    {
        return byNode[who];
    }
    template <class E>
    void
    on(PeerID who, SimTime when, E const& e)
    {
        byNode[who].on(who, when, e);
    }

};

/*忽略所有事件的收集器*/
struct NullCollector
{
    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }
};

/*跟踪模拟的总持续时间*/
struct SimDurationCollector
{
    bool init = false;
    SimTime start;
    SimTime stop;

    template <class E>
    void
    on(PeerID, SimTime when, E const& e)
    {
        if (!init)
        {
            start = when;
            init = true;
        }
        else
            stop = when;
    }
};

/*跟踪提交->接受->已验证的交易演变。

    此收集器通过监视
    *第一次*网络中任何节点看到事务时，或
    由任何节点的已接受或完全验证的分类帐看到。

    如果提交到网络的事务没有唯一的ID，则此
    收集器将不跟踪后续提交。
**/

struct TxCollector
{
//计数
    std::size_t submitted{0};
    std::size_t accepted{0};
    std::size_t validated{0};

    struct Tracker
    {
        Tx tx;
        SimTime submitted;
        boost::optional<SimTime> accepted;
        boost::optional<SimTime> validated;

        Tracker(Tx tx_, SimTime submitted_) : tx{tx_}, submitted{submitted_}
        {
        }
    };

    hash_map<Tx::ID, Tracker> txs;

    using Hist = Histogram<SimTime::duration>;
    Hist submitToAccept;
    Hist submitToValidate;

//默认情况下忽略大多数事件
    template <class E>
    void
    on(PeerID, SimTime when, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, SubmitTx const& e)
    {

//第一次看到它就省钱
        if (txs.emplace(e.tx.id(), Tracker{e.tx, when}).second)
        {
            submitted++;
        }
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        for (auto const& tx : e.ledger.txs())
        {
            auto it = txs.find(tx.id());
            if (it != txs.end() && !it->second.accepted)
            {
                Tracker& tracker = it->second;
                tracker.accepted = when;
                accepted++;

                submitToAccept.insert(*tracker.accepted - tracker.submitted);
            }
        }
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        for (auto const& tx : e.ledger.txs())
        {
            auto it = txs.find(tx.id());
            if (it != txs.end() && !it->second.validated)
            {
                Tracker& tracker = it->second;
//只应验证以前接受的Tx
                assert(tracker.accepted);

                tracker.validated = when;
                validated++;
                submitToValidate.insert(*tracker.validated - tracker.submitted);
            }
        }
    }

//返回从未接受的Tx数
    std::size_t
    orphaned() const
    {
        return std::count_if(txs.begin(), txs.end(), [](auto const& it) {
            return !it.second.accepted;
        });
    }

//返回从未验证过的Tx数
    std::size_t
    unvalidated() const
    {
        return std::count_if(txs.begin(), txs.end(), [](auto const& it) {
            return !it.second.validated;
        });
    }

    template <class T>
    void
    report(SimDuration simDuration, T& log, bool printBreakline = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if (printBreakline)
        {
            log << std::setw(11) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(36) << std::setfill('-') << "-"
                << std::endl;
            log << std::setfill(' ');
        }

        log << std::left
            << std::setw(11) << "TxStats" <<  "|"
            << std::setw(7) << "Count" <<  "|"
            << std::setw(7) << "Per Sec" <<  "|"
            << std::setw(15) << "Latency (sec)"
            << std::right
            << std::setw(7) << "10-ile"
            << std::setw(7) << "50-ile"
            << std::setw(7) << "90-ile"
            << std::left
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');

        log << std::left <<
            std::setw(11) << "Submit " << "|"
            << std::right
            << std::setw(7) << submitted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(submitted) << "|"
            << std::setw(36) << "" << std::endl;

        log << std::left
            << std::setw(11) << "Accept " << "|"
            << std::right
            << std::setw(7) << accepted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(accepted) << "|"
            << std::setw(15) << std::left << "From Submit" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToAccept.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Validate " << "|"
            << std::right
            << std::setw(7) << validated << "|"
            << std::setw(7) << std::setprecision(2) << perSec(validated) << "|"
            << std::setw(15) << std::left << "From Submit" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(submitToValidate.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Orphan" << "|"
            << std::right
            << std::setw(7) << orphaned() << "|"
            << std::setw(7) << "" << "|"
            << std::setw(36) << std::endl;

        log << std::left
            << std::setw(11) << "Unvalidated" << "|"
            << std::right
            << std::setw(7) << unvalidated() << "|"
            << std::setw(7) << "" << "|"
            << std::setw(43) << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');
    }

    template <class T, class Tag>
    void
    csv(SimDuration simDuration, T& log, Tag const& tag, bool printHeaders = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if(printHeaders)
        {
            log << "tag" << ","
                << "txNumSubmitted" << ","
                << "txNumAccepted" << ","
                << "txNumValidated" << ","
                << "txNumOrphaned" << ","
                << "txUnvalidated" << ","
                << "txRateSumbitted" << ","
                << "txRateAccepted" << ","
                << "txRateValidated" << ","
                << "txLatencySubmitToAccept10Pctl" << ","
                << "txLatencySubmitToAccept50Pctl" << ","
                << "txLatencySubmitToAccept90Pctl" << ","
                << "txLatencySubmitToValidatet10Pctl" << ","
                << "txLatencySubmitToValidatet50Pctl" << ","
                << "txLatencySubmitToValidatet90Pctl"
                << std::endl;
        }


        log << tag << ","
//TXNUMSTRADION
            << submitted << ","
//TXNUMIT
            << accepted << ","
//TXNUMID
            << validated << ","
//被遗弃的
            << orphaned() << ","
//未验证txnum
            << unvalidated() << ","
//TXRATION
            << std::setprecision(2) << perSec(submitted) << ","
//被接受的
            << std::setprecision(2) << perSec(accepted) << ","
//TxRATALVALIDATION
            << std::setprecision(2) << perSec(validated) << ","
//x最迟提交10%验收
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.1f)) << ","
//TX最迟提交给CCEPT 50PCTL
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.5f)) << ","
//TxLatencysubmittoaccept90pctl
            << std::setprecision(2) << fmtS(submitToAccept.percentile(0.9f)) << ","
//TX最新提交的10%有效期
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.1f)) << ","
//TX最新提交的50%有效期
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.5f)) << ","
//Tx最新提交日期：90pctl
            << std::setprecision(2) << fmtS(submitToValidate.percentile(0.9f)) << ","
            << std::endl;
    }
};

/*跟踪分类账的已接受->已验证的演变。

    此收集器通过监视
    *第一次*任何节点接受或完全验证分类账时。

**/

struct LedgerCollector
{
    std::size_t accepted{0};
    std::size_t fullyValidated{0};

    struct Tracker
    {
        SimTime accepted;
        boost::optional<SimTime> fullyValidated;

        Tracker(SimTime accepted_) : accepted{accepted_}
        {
        }
    };

    hash_map<Ledger::ID, Tracker> ledgers_;

    using Hist = Histogram<SimTime::duration>;
    Hist acceptToFullyValid;
    Hist acceptToAccept;
    Hist fullyValidToFullyValid;

//默认情况下忽略大多数事件
    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
//第一次接受此分类帐
        if (ledgers_.emplace(e.ledger.id(), Tracker{when}).second)
        {
            ++accepted;
//忽略跳跃？
            if (e.prior.id() == e.ledger.parentID())
            {
                auto const it = ledgers_.find(e.ledger.parentID());
                if (it != ledgers_.end())
                {
                    acceptToAccept.insert(when - it->second.accepted);
                }
            }
        }
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
//忽略跳跃
        if (e.prior.id() == e.ledger.parentID())
        {
            auto const it = ledgers_.find(e.ledger.id());
            assert(it != ledgers_.end());
            auto& tracker = it->second;
//首次完全验证
            if (!tracker.fullyValidated)
            {
                ++fullyValidated;
                tracker.fullyValidated = when;
                acceptToFullyValid.insert(when - tracker.accepted);

                auto const parentIt = ledgers_.find(e.ledger.parentID());
                if (parentIt != ledgers_.end())
                {
                    auto& parentTracker = parentIt->second;
                    if (parentTracker.fullyValidated)
                    {
                        fullyValidToFullyValid.insert(
                            when - *parentTracker.fullyValidated);
                    }
                }
            }
        }
    }

    std::size_t
    unvalidated() const
    {
        return std::count_if(
            ledgers_.begin(), ledgers_.end(), [](auto const& it) {
                return !it.second.fullyValidated;
            });
    }

    template <class T>
    void
    report(SimDuration simDuration, T& log, bool printBreakline = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if (printBreakline)
        {
            log << std::setw(11) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(7) << std::setfill('-') << "-" <<  "-"
                << std::setw(36) << std::setfill('-') << "-"
                << std::endl;
            log << std::setfill(' ');
        }

        log << std::left
            << std::setw(11) << "LedgerStats" <<  "|"
            << std::setw(7)  << "Count" <<  "|"
            << std::setw(7)  << "Per Sec" <<  "|"
            << std::setw(15) << "Latency (sec)"
            << std::right
            << std::setw(7) << "10-ile"
            << std::setw(7) << "50-ile"
            << std::setw(7) << "90-ile"
            << std::left
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(7) << std::setfill('-') << "-" <<  "|"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');

         log << std::left
            << std::setw(11) << "Accept " << "|"
            << std::right
            << std::setw(7) << accepted << "|"
            << std::setw(7) << std::setprecision(2) << perSec(accepted) << "|"
            << std::setw(15) << std::left << "From Accept" << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.9f))
            << std::endl;

        log << std::left
            << std::setw(11) << "Validate " << "|"
            << std::right
            << std::setw(7) << fullyValidated << "|"
            << std::setw(7) << std::setprecision(2) << perSec(fullyValidated) << "|"
            << std::setw(15) << std::left << "From Validate " << std::right
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.1f))
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.5f))
            << std::setw(7) << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.9f))
            << std::endl;

        log << std::setw(11) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(7) << std::setfill('-') << "-" <<  "-"
            << std::setw(36) << std::setfill('-') << "-"
            << std::endl;
        log << std::setfill(' ');
    }

    template <class T, class Tag>
    void
    csv(SimDuration simDuration, T& log, Tag const& tag, bool printHeaders = false)
    {
        using namespace std::chrono;
        auto perSec = [&simDuration](std::size_t count)
        {
            return double(count)/duration_cast<seconds>(simDuration).count();
        };

        auto fmtS = [](SimDuration dur)
        {
            return duration_cast<duration<float>>(dur).count();
        };

        if(printHeaders)
        {
            log << "tag" << ","
                << "ledgerNumAccepted" << ","
                << "ledgerNumFullyValidated" << ","
                << "ledgerRateAccepted" << ","
                << "ledgerRateFullyValidated" << ","
                << "ledgerLatencyAcceptToAccept10Pctl" << ","
                << "ledgerLatencyAcceptToAccept50Pctl" << ","
                << "ledgerLatencyAcceptToAccept90Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid10Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid50Pctl" << ","
                << "ledgerLatencyFullyValidToFullyValid90Pctl"
                << std::endl;
        }

        log << tag << ","
//窗台已接受
            << accepted << ","
//已验证LedgerNumfully
            << fullyValidated << ","
//接受的分类帐
            << std::setprecision(2) << perSec(accepted) << ","
//已验证分类帐完整性
            << std::setprecision(2) << perSec(fullyValidated) << ","
//分类账分类账接受10pctl
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.1f)) << ","
//LedgerLatencyAcceptToAccept50PCTL分类账
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.5f)) << ","
//LedgerLatencyAcceptToAccept90PCTL分类账
            << std::setprecision(2) << fmtS(acceptToAccept.percentile(0.9f)) << ","
//会计科目完全有效
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.1f)) << ","
//完全有效的分类账状态
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.5f)) << ","
//90%有效期的分类账
            << std::setprecision(2) << fmtS(fullyValidToFullyValid.percentile(0.9f))
            << std::endl;
    }
};

/*写出分类帐活动流

    将有关每个已接受和完全验证的分类帐的信息写入
    提供std:：ostream。
**/

struct StreamCollector
{
    std::ostream& out;

//默认情况下忽略大多数事件
    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
        out << when.time_since_epoch().count() << ": Node " << who << " accepted "
            << "L" << e.ledger.id() << " " << e.ledger.txs() << "\n";
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
        out << when.time_since_epoch().count() << ": Node " << who
            << " fully-validated " << "L"<< e.ledger.id() << " " << e.ledger.txs()
            << "\n";
    }
};

/*为已关闭和完全验证的分类帐保存有关跳转的信息。一
    当节点关闭/完全验证不是
    以前已关闭/完全验证的分类帐的直接子级。这包括
    在分类帐历史的同一个分支中跨分支跳到前面。
**/

struct JumpCollector
{
    struct Jump
    {
        PeerID id;
        SimTime when;
        Ledger from;
        Ledger to;
    };

    std::vector<Jump> closeJumps;
    std::vector<Jump> fullyValidatedJumps;

//默认情况下忽略大多数事件
    template <class E>
    void
    on(PeerID, SimTime, E const& e)
    {
    }

    void
    on(PeerID who, SimTime when, AcceptLedger const& e)
    {
//不是直接子级->父级开关
        if(e.ledger.parentID() != e.prior.id())
            closeJumps.emplace_back(Jump{who, when, e.prior, e.ledger});
    }

    void
    on(PeerID who, SimTime when, FullyValidateLedger const& e)
    {
//不是直接子级->父级开关
        if (e.ledger.parentID() != e.prior.id())
            fullyValidatedJumps.emplace_back(
                Jump{who, when, e.prior, e.ledger});
    }
};

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
