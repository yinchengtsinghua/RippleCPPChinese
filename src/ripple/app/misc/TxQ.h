
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
    版权所有（c）2012-14 Ripple Labs Inc.

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

#ifndef RIPPLE_TXQ_H_INCLUDED
#define RIPPLE_TXQ_H_INCLUDED

#include <ripple/app/tx/applySteps.h>
#include <ripple/ledger/OpenView.h>
#include <ripple/ledger/ApplyView.h>
#include <ripple/protocol/TER.h>
#include <ripple/protocol/STTx.h>
#include <boost/intrusive/set.hpp>
#include <boost/circular_buffer.hpp>

namespace ripple {

class Application;
class Config;

/*
    事务队列。用于与
    费用上升。

    将足够的交易记录添加到未结分类帐后，需要
    费用将大幅上涨。如果添加了其他交易，
    费用将从那里呈指数增长。

    没有足够高的费用可用于的交易
    分类帐按从最高费用级别到的顺序添加到队列中。
    最低的。每当新分类帐被接受为已验证时，交易记录
    首先按费用级别顺序从队列应用到未结分类帐
    直到所有交易被应用或费用再次上涨
    剩余交易记录的价格太高。

    有关进一步的信息和事务处理方式的高级概述
    使用“txq”处理，请参见feeescalation.md
**/

class TxQ
{
public:
///单签名参考交易的费用水平。
    static constexpr std::uint64_t baseLevel = 256;

    /*
        用于自定义@ref txq行为的结构。
    **/

    struct Setup
    {
///默认构造函数
        explicit Setup() = default;

        /*允许交易的分类账价值数量
            在队列中。例如，如果最后一个分类帐
            150个事务，然后最多3000个事务可以
            排队等候。

            可以被@ref queuesizemin覆盖
        **/

        std::size_t ledgersInQueue = 20;
        /*允许队列的最小限制。

            将允许队列中有多个“分类帐查询”
            如果分类帐很小。
        **/

        std::size_t queueSizeMin = 2000;
        /*排队的费用级别需要额外的百分比
            用另一个事务替换该事务的事务
            序列号相同。

            如果“alice”账户的排队交易带有seq 45
            费用级别为512，是
            Seq 45的“Alice”必须具有至少为
            512*（1+0.25）=640。
        **/

        std::uint32_t retrySequencePercent = 25;
        /*费用水平所需的额外百分比
            排队的事务以排队处理
            下一个序列号。

            如果“alice”账户的排队交易带有seq 45
            费用级别为512，具有seq 46的交易必须
            收费水平至少为
            512*（1+-0.90）=51.2~=52至
            被考虑。

            @托多·埃亨尼斯。我们能去掉多tx因子吗？
        **/

        std::int32_t multiTxnPercent = -90;
///升级乘数的最小值，无论
///上一个分类帐的中间费用水平。
        std::uint64_t minimumEscalationMultiplier = baseLevel * 500;
///允许进入分类帐的最小交易记录数
///在升级之前，不考虑上一个分类帐的大小。
        std::uint32_t minimumTxnInLedger = 5;
///like@ref minimumtxninleger用于独立模式。
///主要是为了测试不需要担心排队。
        std::uint32_t minimumTxnInLedgerSA = 1000;
///每个分类帐的费用提升“有效的交易数
//“朝着”。
        std::uint32_t targetTxnInLedger = 50;
        /*以前每个分类帐的可选最大允许交易记录值
            费用升级开始。默认情况下，最大值是紧急值
            网络、验证程序和共识性能的属性。这个
            设置可以覆盖该行为以防止费用增加
            允许超过“MaximumTxNinLedger”的“便宜”交易进入
            未结分类帐。

            @托多·埃亨尼斯。这种设置似乎违背了我们的目标，
                价值观。它能被移走吗？
        **/

        boost::optional<std::uint32_t> maximumTxnInLedger;
        /*当分类帐的交易超过“预期”时，以及
            业绩表现良好，达到了预期的分类账规模。
            更新为上一个分类帐大小加上此百分比。

            计算以配置的限制为准，最近
            事务计数缓冲区。

            示例：如果“预期”是针对500个交易，并且
            分类帐通常通过501个交易进行验证，然后
            预计分类帐大小将更新为601。
        **/

        std::uint32_t normalConsensusIncreasePercent = 20;
        /*当达成共识的时间比适当的时间长时，预期的
            分类帐大小更新为上一分类帐的较小值
            大小和当前预期的分类帐大小减去此
            百分比。

            计算以配置的限制为准。

            示例：如果分类帐有15000个交易记录，并且
            验证缓慢，则预期的分类帐大小将为
            更新至7500。如果只有6个交易，
            预计分类帐大小将更新为5，假设
            默认最小值。
        **/

        std::uint32_t slowConsensusDecreasePercent = 50;
///一个帐户可以排队的最大事务数。
        std::uint32_t maximumTxnPerAccount = 10;
        /*当前分类帐序列和A之间的最小差异
            事务的“lastledgersequence”，用于
            可排队的。减少事务排队的机会
            广播只会在它有机会
            处理。
        **/

        std::uint32_t minimumLastLedgerBuffer = 2;
        /*所以我们不处理“无限”的收费水平，请
            任何基本费用为0的交易（即setRegularKey
            密码恢复）。
            未来网络行为是否会发生变化？
            无法处理这些事务，
            我们可以让事情变得更复杂。但避免
            现在骑自行车。
        **/

        std::uint64_t zeroBaseFeeTransactionFeeLevel = 256000;
///使用独立模式行为。
        bool standAlone = false;
    };

    /*
        @ref txq:：getmetrics返回的结构，表示为
        参考费用水平单位。
    **/

    struct Metrics
    {
///默认构造函数
        explicit Metrics() = default;

///队列中的事务数
        std::size_t txCount;
///max队列中当前允许的事务数
        boost::optional<std::size_t> txQMaxSize;
///未结分类帐中当前的交易记录数
        std::size_t txInLedger;
///每个分类帐预期的交易记录数
        std::size_t txPerLedger;
///参考交易费用水平
        std::uint64_t referenceFeeLevel;
///要考虑的交易的最低费用水平
///打开的分类帐或队列
        std::uint64_t minProcessingFeeLevel;
///上一个分类帐的中间费用水平
        std::uint64_t medFeeLevel;
///进入当前未结分类账的最低费用水平，
///绕过队列
        std::uint64_t openLedgerFeeLevel;
    };

    /*
        @ref txq:：getaccounttxs返回的结构以描述
        帐户队列中的事务。
    **/

    struct AccountTxDetails
    {
///默认构造函数
        explicit AccountTxDetails() = default;

///排队事务的费用级别
        uint64_t feeLevel;
//队列事务的/lastvalidledger字段（如果有）
        boost::optional<LedgerIndex const> lastValid;
        /*应用排队事务的潜在@ref tx后果
            到未结分类帐，如果知道的话。

            @note`results`是懒惰计算的，因此可能一点都不知道
            给定时间。
        **/

        boost::optional<TxConsequences const> consequences;
    };

    /*
        描述队列中事务的结构
        正在等待应用于当前未结分类帐。
        @ref txq:：gettxs返回了一个这些的集合。
    **/

    struct TxDetails : AccountTxDetails
    {
///默认构造函数
        explicit TxDetails() = default;

///事务排队等待的帐户
        AccountID account;
///完整事务
        std::shared_ptr<STTx const> txn;
        /*事务处理程序返回重试结果的次数
            尝试将此交易记录应用于未结分类帐时
            从队列中。如果事务处理程序返回“ter”，并且没有重试
            左，此事务将被删除。
        **/

        int retriesRemaining;
        /*@ref preflight之前返回的*中间*结果
            此事务已排队，或在排队之后，但在
            尝试将其“应用”到打开的分类帐失败。本遗嘱
            通常是“tessuccess”，但也有一些边缘情况
            它还有另一个价值。那些边缘案例足够有趣了
            此值在此处可用。具体来说，如果
            `rules`change between attempts，`preflight`将再次运行
            在“txq:：maybetx:：apply”中。
        **/

        TER preflightResult;
        /*如果事务处理程序试图将该事务应用于打开的
            来自队列的分类帐和*失败*，则这是事务处理程序
            上次尝试的结果。不应该是“tec”，“tef”，
            “tem”或“tessuccess”，因为这些结果导致
            要从队列中删除的事务。
        **/

        boost::optional<TER> lastResult;
    };

///构造函数
    TxQ(Setup const& setup,
        beast::Journal j);

///析构函数
    virtual ~TxQ();

    /*
        将新交易添加到未结分类帐，将其保留在队列中，
        或者拒绝它。

        @返回一对“ter”和“bool”表示
                交易是否适用于
                未结分类帐。如果事务排队，
                将返回terqueued，false `。
    **/

    std::pair<TER, bool>
    apply(Application& app, OpenView& view,
        std::shared_ptr<STTx const> const& tx,
            ApplyFlags flags, beast::Journal j);

    /*
        用队列中的交易记录填充新的未结分类帐。

        @注意：当更多的交易应用于分类帐时，
        所需费用可能会增加。所需费用可能高于
        清空队列前排队项目的费用水平，
        这将结束该进程，并将其留在队列中
        下一个未结分类帐。

        @返回是否将任何事务添加到“视图”中。
    **/

    bool
    accept(Application& app, OpenView& view);

    /*
        更新费用指标并清理队列，为
        下一个分类帐。

        @注意：根据
        确认的分类账中的TXS以及共识是否缓慢。
        将最大队列大小调整为足以容纳
        `LEdgersinqueue`分类帐或'queuesizemin'交易。
        “lastledgersequence”具有的任何交易
        传递的将从队列中移除，并且任何帐户对象
        在他们下面没有候选人的将被除名。
    **/

    void
    processClosedLedger(Application& app,
        ReadView const& view, bool timeLeap);

    /*返回参考费用级别单位中的费用度量。
    **/

    Metrics
    getMetrics(OpenView const& view) const;

    /*返回有关当前事务的信息
        在帐户的队列中。

        @返回空的“map”，如果
        帐户在队列中没有交易记录。
    **/

    std::map<TxSeq, AccountTxDetails const>
    getAccountTxs(AccountID const& account, ReadView const& view) const;

    /*返回当前所有事务的信息
        在队列中。

        @如果没有事务，返回空的“vector”
        在队列中。
    **/

    std::vector<TxDetails>
    getTxs(ReadView const& view) const;

    /*总结“fee”rpc命令的当前费用指标。

        @返回“json objectvalue”
    **/

    Json::Value
    doRPC(Application& app) const;

private:
    /*
        跟踪和使用
        当前未结分类帐。按比例收费的工作
        随着未结分类帐的增长。
    **/

    class FeeMetrics
    {
    private:
///txnsexpected的最小值。
        std::size_t const minimumTxnCount_;
///每个分类帐的费用提升“有效的交易数
//“朝着”。
        std::size_t const targetTxnCount_;
///txnsexpected的最大值
        boost::optional<std::size_t> const maximumTxnCount_;
///每个分类帐所需的交易记录数。
///将接受一个以上的值
///在升级开始之前。
        std::size_t txnsExpected_;
///recent history of transaction计数为
///超过targetxncount_
        boost::circular_buffer<std::size_t> recentTxnCounts_;
///基于LCL的中间费用。使用
///当费用增加开始时。
        std::uint64_t escalationMultiplier_;
//学报
        beast::Journal j_;

    public:
///构造函数
        FeeMetrics(Setup const& setup, beast::Journal j)
            : minimumTxnCount_(setup.standAlone ?
                setup.minimumTxnInLedgerSA :
                setup.minimumTxnInLedger)
            , targetTxnCount_(setup.targetTxnInLedger < minimumTxnCount_ ?
                minimumTxnCount_ : setup.targetTxnInLedger)
            , maximumTxnCount_(setup.maximumTxnInLedger ?
                *setup.maximumTxnInLedger < targetTxnCount_ ?
                    targetTxnCount_ : *setup.maximumTxnInLedger :
                        boost::optional<std::size_t>(boost::none))
            , txnsExpected_(minimumTxnCount_)
            , recentTxnCounts_(setup.ledgersInQueue)
            , escalationMultiplier_(setup.minimumEscalationMultiplier)
            , j_(j)
        {
        }

        /*
            根据readview中的交易更新费用标准
            用于费用提升计算。

            @param app波纹应用程序对象。
            @param查看刚刚关闭或接收的LCL的视图。
            @param timeleap表示涟漪正在加载，所以费用
            应该生长得更快。
            @param设置自定义参数。
        **/

        std::size_t
        update(Application& app,
            ReadView const& view, bool timeLeap,
            TxQ::Setup const& setup);

///与外部相关的度量的快照
///字段。
        struct Snapshot
        {
//每个分类帐预期的交易记录数。
//超过此值的一个将被接受
//在升级开始之前。
            std::size_t const txnsExpected;
//基于LCL的中间费用。使用
//当费用上涨开始时。
            std::uint64_t const escalationMultiplier;
        };

///get当前的@ref快照
        Snapshot
        getSnapshot() const
        {
            return {
                txnsExpected_,
                escalationMultiplier_
            };
        }

        /*使用当前未结分类帐中的交易记录数
            要计算费用水平，交易必须支付以绕过
            排队。

            @param查看当前打开的分类帐。

            @返回费用级别值。
        **/

        static
        std::uint64_t
        scaleFeeLevel(Snapshot const& snapshot, OpenView const& view);

        /*
            计算系列中所有交易的总费用水平。
            假设已经存在超过@ref txnsexpected的txns
            在视图和“Extracount”之间。如果没有，结果
            将是明智的（例如，不会出现任何下溢或
            溢出），但液位将高于实际要求。

            @注意，“系列”是同一账户的一组交易。
                有序列号。在这种情况下
                函数，序列已经在队列中，并且序列
                从帐户的当前序列号开始。这个
                函数由@ref trycearaccountqueue调用
                如果新提交的交易支付的金额足以
                将所有排队的事务及其本身从
                列队进入未结分类帐，同时计算
                每笔费用都会随着处理而增加。想法是
                如果一系列交易需要很长时间才能退出
                在队列中，用户无需
                以更高的费用重新提交每一份。

            @param查看当前打开/工作分类帐。（可能是沙盒。）
            @param extraccount要计为的附加事务数
                在分类帐中。（如果“view”是沙盒，则应为
                父分类帐中的交易记录。）
            @param series将要执行的序列中的事务总数
                处理。

            @return a`std:：pair`从@ref`muldiv'返回，指示
                计算结果是否溢出。
        **/

        static
        std::pair<bool, std::uint64_t>
        escalatedSeriesFeeLevel(Snapshot const& snapshot, OpenView const& view,
            std::size_t extraCount, std::size_t seriesSize);
    };

    /*
        表示队列中可以应用的事务
        稍后到未结分类帐。
    **/

    class MaybeTx
    {
    public:
///由下面的txq:：feehook和txq:：feemultiset使用
///将每个maybetx对象放入多个
///不带副本、指针等的设置。
        boost::intrusive::set_member_hook<> byFeeListHook;

///完整的事务。
        std::shared_ptr<STTx const> txn;

///potential@ref tx应用此事务的后果
///到未结分类帐。
        boost::optional<TxConsequences const> consequences;

///交易将支付的计算费用水平。
        uint64_t const feeLevel;
///事务ID。
        TxID const txID;
///previous transaction id（`sfaccounttxnid`字段）。
        boost::optional<TxID> priorTxID;
///提交交易的帐户。
        AccountID const account;
///交易记录的到期分类帐
///（`sflastledgersequence`字段）。
        boost::optional<LedgerIndex> lastValid;
///transaction sequence number（`sfsequence`字段）。
        TxSeq const sequence;
        /*
            将给出队列前面的事务
            几次成功的尝试
            排队。如果被撤销，该账户的一项罚款
            将设置标志，其他事务可能
            他们的“重试”作为
            处罚。
        **/

        int retriesRemaining;
///flags提供给'apply`。如果交易迟了
///尝试使用不同的标志，它将需要
//又一次预演了。
        ApplyFlags const flags;
        /*如果事务处理程序试图将该事务应用于打开的
            来自队列的分类帐和*失败*，则这是事务处理程序
            上次尝试的结果。不应该是“tec”，“tef”，
            “tem”或“tessuccess”，因为这些结果导致
            要从队列中删除的事务。
        **/

        boost::optional<TER> lastResult;
        /*“preflight”操作的缓存结果。因为
            “飞行前”很昂贵，尽量减少飞行次数
            这需要做。
            @invariant`pfresult`永远不允许为空。这个
                'boost:：optional'用于允许'emplace'd
                建造和替换，无副本
                分配操作。
        **/

        boost::optional<PreflightResult const> pfresult;

        /*正在为新排队的事务启动重试计数。

            在txq:：accept中，所需费用水平可能较低
            足以让这个交易有机会申请
            到分类帐，但它可能会得到
            另一个原因（如余额不足）。当那
            发生，事务将留在队列中尝试
            以后再说一遍，但不应该让它失败
            无限期地允许的失败数为
            基本上是任意的。它应该足够大
            允许临时故障清除，但足够小
            队列中没有过时的事务
            这将阻止较低的费用级别的交易排队。
        **/

        static constexpr int retriesAllowed = 10;

    public:
///构造函数
        MaybeTx(std::shared_ptr<STTx const> const&,
            TxID const& txID, std::uint64_t feeLevel,
                ApplyFlags const flags,
                    PreflightResult const& pfresult);

///尝试将排队的事务应用于打开的分类帐。
        std::pair<TER, bool>
        apply(Application& app, OpenView& view, beast::Journal j);
    };

///用于按“feelvel”对@ref maybetx进行排序
    class GreaterFee
    {
    public:
///默认构造函数
        explicit GreaterFee() = default;

///lhs的费用水平是否大于rhs的费用水平？
        bool operator()(const MaybeTx& lhs, const MaybeTx& rhs) const
        {
            return lhs.feeLevel > rhs.feeLevel;
        }
    };

    /*用于表示队列中的帐户，并存储
        按顺序为该帐户排队的事务。
    **/

    class TxQAccount
    {
    public:
        using TxMap = std::map <TxSeq, MaybeTx>;

//帐户
        AccountID const account;
///序列号将用作键。
        TxMap transactions;
        /*如果此帐户的任何事务重试次数超过
            “重试”次，以便从
            排队，则此帐户的所有其他事务将
            在移除之前，最多尝试两次。帮助
            防止在更可能的重试上浪费资源
            失败。
        **/

        bool retryPenalty = false;
        /*如果该账户有任何交易失败或到期，
            然后，当队列接近满时，来自
            此帐户将被丢弃。帮助阻止队列
            从填充和楔入。
        **/

        bool dropPenalty = false;

    public:
///从事务构造
        explicit TxQAccount(std::shared_ptr<STTx const> const& txn);
///从帐户构造
        explicit TxQAccount(const AccountID& account);

///返回当前为此帐户排队的事务数
        std::size_t
        getTxnCount() const
        {
            return transactions.size();
        }

///检查此帐户是否没有排队的事务
        bool
        empty() const
        {
            return !getTxnCount();
        }

///将事务候选添加到此帐户以进行排队
        MaybeTx&
        add(MaybeTx&&);

        /*从中删除具有给定序列号的候选项
            帐户。

            @返回是否删除候选人
        **/

        bool
        remove(TxSeq const& sequence);
    };

    using FeeHook = boost::intrusive::member_hook
        <MaybeTx, boost::intrusive::set_member_hook<>,
        &MaybeTx::byFeeListHook>;

    using FeeMultiSet = boost::intrusive::multiset
        < MaybeTx, FeeHook,
        boost::intrusive::compare <GreaterFee> >;

    using AccountMap = std::map <AccountID, TxQAccount>;

///setup用于控制队列行为的参数
    Setup const setup_;
//学报
    beast::Journal j_;

    /*跟踪队列的当前状态。
        @注意此成员必须始终且只能在
        锁定互斥锁
    **/

    FeeMetrics feeMetrics_;
    /*队列本身：已排序的事务的集合
        收费水平。
        @注意此成员必须始终且只能在
        锁定互斥锁
    **/

    FeeMultiSet byFee_;
    /*当前有任何交易的所有帐户
        在队列中。动态创建和销毁条目
        当添加和删除事务时。
        @注意此成员必须始终且只能在
        锁定互斥锁
    **/

    AccountMap byAccount_;
    /*基于队列的最大允许事务数
        在当前指标上。如果未初始化，则没有限制，
        但这种情况在实践中不会持续很长时间。
        @注意此成员必须始终且只能在
        锁定互斥锁
    **/

    boost::optional<size_t> maxSize_;

    /*大多数队列操作都是在主锁下完成的，
        但是对于rpc“fee”命令使用这个互斥体，而不是。
    **/

    std::mutex mutable mutex_;

private:
///队列是否至少“fillPercentage”已满？
    template<size_t fillPercentage = 100>
    bool
    isFull() const;

    /*检查指示的事务是否符合条件
        存储在队列中。
    **/

    bool canBeHeld(STTx const&, OpenView const&,
        AccountMap::iterator,
            boost::optional<FeeMultiSet::iterator>);

///删除并返回byfee_uuu中的下一个条目（较低的费用级别）
    FeeMultiSet::iterator_type erase(FeeMultiSet::const_iterator_type);
    /*删除并返回帐户的下一个条目（如果费用水平
        更高），或按费用级别（较低的费用级别）中的下一个条目。
        用于获取下一个“可应用”maybetx for accept（）。
    **/

    FeeMultiSet::iterator_type eraseAndAdvance(FeeMultiSet::const_iterator_type);
///Erase a range of items，based on txqaccount:：txmap迭代器
    TxQAccount::TxMap::iterator
    erase(TxQAccount& txQAccount, TxQAccount::TxMap::const_iterator begin,
        TxQAccount::TxMap::const_iterator end);

    /*
        “all”或“nothing”尝试应用“accountiter”的所有排队tx
        包括“tx”。
    **/

    std::pair<TER, bool>
    tryClearAccountQueue(Application& app, OpenView& view,
        STTx const& tx, AccountMap::iterator const& accountIter,
            TxQAccount::TxMap::iterator, std::uint64_t feeLevelPaid,
                PreflightResult const& pfresult,
                    std::size_t const txExtraCount, ApplyFlags flags,
                        FeeMetrics::Snapshot const& metricsSnapshot,
                            beast::Journal j);

};

/*
    从应用程序配置生成@ref txq:：setup对象。
**/

TxQ::Setup
setup_TxQ(Config const&);

/*
    @ref txq对象工厂。
**/

std::unique_ptr<TxQ>
make_TxQ(TxQ::Setup const&, beast::Journal);

} //涟漪

#endif
