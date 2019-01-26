
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
    版权所有（c）2012，2013 Ripple Labs Inc.

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

#include <ripple/app/ledger/LedgerCleaner.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/beast/core/CurrentThreadName.h>

namespace ripple {
namespace detail {

/*

洗洁精机

清理分类帐。具体来说，解决这些问题：

1。旧版本可能会将sqlite帐户和事务数据库保留在
   不一致的状态。清洁工识别出这些不一致，并且
   解决它们。

2。根据请求，检查分类帐中丢失的节点并触发提取。

**/


class LedgerCleanerImp : public LedgerCleaner
{
    Application& app_;
    beast::Journal j_;
    mutable std::mutex mutex_;

    mutable std::condition_variable wakeup_;

    std::thread thread_;

    enum class State : char {
        readyToClean = 0,
        startCleaning,
        cleaning
    };
    State state_ = State::readyToClean;
    bool shouldExit_ = false;

//我们正在检查的范围内最低的分类帐。
    LedgerIndex  minRange_ = 0;

//我们正在检查的范围内最高的分类帐
    LedgerIndex  maxRange_ = 0;

//检查所有状态/事务节点
    bool checkNodes_ = false;

//重写SQL数据库
    bool fixTxns_ = false;

//自上次成功以来遇到的错误数
    int failures_ = 0;

//——————————————————————————————————————————————————————————————
public:
    LedgerCleanerImp (
        Application& app,
        Stoppable& stoppable,
        beast::Journal journal)
        : LedgerCleaner (stoppable)
        , app_ (app)
        , j_ (journal)
    {
    }

    ~LedgerCleanerImp () override
    {
        if (thread_.joinable())
            LogicError ("LedgerCleanerImp::onStop not called.");
    }

//——————————————————————————————————————————————————————————————
//
//可停止的
//
//——————————————————————————————————————————————————————————————

    void onPrepare () override
    {
    }

    void onStart () override
    {
        thread_ = std::thread {&LedgerCleanerImp::run, this};
    }

    void onStop () override
    {
        JLOG (j_.info()) << "Stopping";
        {
            std::lock_guard<std::mutex> lock (mutex_);
            shouldExit_ = true;
            wakeup_.notify_one();
        }
        thread_.join();
    }

//——————————————————————————————————————————————————————————————
//
//属性流
//
//——————————————————————————————————————————————————————————————

    void onWrite (beast::PropertyStream::Map& map) override
    {
        std::lock_guard<std::mutex> lock (mutex_);

        if (maxRange_ == 0)
            map["status"] = "idle";
        else
        {
            map["status"] = "running";
            map["min_ledger"] = minRange_;
            map["max_ledger"] = maxRange_;
            map["check_nodes"] = checkNodes_ ? "true" : "false";
            map["fix_txns"] = fixTxns_ ? "true" : "false";
            if (failures_ > 0)
                map["fail_counts"] = failures_;
        }
    }

//——————————————————————————————————————————————————————————————
//
//洗洁精机
//
//——————————————————————————————————————————————————————————————

    void doClean (Json::Value const& params) override
    {
        LedgerIndex minRange = 0;
        LedgerIndex maxRange = 0;
        app_.getLedgerMaster().getFullValidatedRange (minRange, maxRange);

        {
            std::lock_guard<std::mutex> lock (mutex_);

            maxRange_ = maxRange;
            minRange_ = minRange;
            checkNodes_ = false;
            fixTxns_ = false;
            failures_ = 0;

            /*
            JSON参数：

                所有参数都是可选的。默认情况下，清理器会清理
                它认为必要的东西。此行为可以修改
                使用通过JSON RPC提供的以下选项：

                “分类帐”
                    表示单个无符号整数
                    要清除的分类帐。

                “最小分类账”，“最大分类账”
                    表示起始和结束的无符号整数
                    要清除的分类帐编号。如果未指定，清洁所有分类帐。

                “满”
                    布尔值。如果是真的，就意味着要尽可能地清理一切。

                “FixiTxNS”
                    一个布尔值，指示是否修复
                    数据库中的事务。

                “检查节点”
                    布尔值，当设置为真时表示检查节点。

                “停止”
                    一个布尔值，如果为true，则通知清洗器优雅地
                    如果正在进行任何清理，则停止当前的活动。
            **/


//快速修复单个分类帐的方法
            if (params.isMember(jss::ledger))
            {
                maxRange_ = params[jss::ledger].asUInt();
                minRange_ = params[jss::ledger].asUInt();
                fixTxns_ = true;
                checkNodes_ = true;
            }

            if (params.isMember(jss::max_ledger))
                 maxRange_ = params[jss::max_ledger].asUInt();

            if (params.isMember(jss::min_ledger))
                minRange_ = params[jss::min_ledger].asUInt();

            if (params.isMember(jss::full))
                fixTxns_ = checkNodes_ = params[jss::full].asBool();

            if (params.isMember(jss::fix_txns))
                fixTxns_ = params[jss::fix_txns].asBool();

            if (params.isMember(jss::check_nodes))
                checkNodes_ = params[jss::check_nodes].asBool();

            if (params.isMember(jss::stop) && params[jss::stop].asBool())
                minRange_ = maxRange_ = 0;

            if (state_ == State::readyToClean)
            {
                state_ = State::startCleaning;
                wakeup_.notify_one();
            }
        }
    }

//——————————————————————————————————————————————————————————————
//
//窗台清理
//
//——————————————————————————————————————————————————————————————
private:
    void init ()
    {
        JLOG (j_.debug()) << "Initializing";
    }

    void run ()
    {
        beast::setCurrentThreadName ("LedgerCleaner");
        JLOG (j_.debug()) << "Started";

        init();

        while (true)
        {
            {
                std::unique_lock<std::mutex> lock (mutex_);
                wakeup_.wait(lock, [this]()
                    {
                        return (
                            shouldExit_ ||
                            state_ == State::startCleaning);
                    });
                if (shouldExit_)
                    break;

                state_ = State::cleaning;
            }
            doLedgerCleaner();
        }

        stopped();
    }

//vvalco todo this should return boost:：optional<uint256>
    LedgerHash getLedgerHash(
        std::shared_ptr<ReadView const>& ledger,
        LedgerIndex index)
    {
        boost::optional<LedgerHash> hash;
        try
        {
            hash = hashOfSeq(*ledger, index, j_);
        }
        catch (SHAMapMissingNode &)
        {
            JLOG (j_.warn()) <<
                "Node missing from ledger " << ledger->info().seq;
            app_.getInboundLedgers().acquire (
                ledger->info().hash, ledger->info().seq,
                InboundLedger::Reason::GENERIC);
        }
return hash ? *hash : beast::zero; //克莱
    }

    /*处理单个分类帐
        @param ledger index要处理的分类帐索引。
        @param ledger hash分类帐的已知正确哈希。
        @param donodes确保所有分类帐节点都在节点数据库中。
        @param dotxns将（帐户）事务重新处理到SQL数据库。
        @如果清除了分类帐，返回'true'。
    **/

    bool doLedger(
        LedgerIndex const& ledgerIndex,
        LedgerHash const& ledgerHash,
        bool doNodes,
        bool doTxns)
    {
        auto nodeLedger = app_.getInboundLedgers().acquire (
            ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
        if (!nodeLedger)
        {
            JLOG (j_.debug()) << "Ledger " << ledgerIndex << " not available";
            app_.getLedgerMaster().clearLedger (ledgerIndex);
            app_.getInboundLedgers().acquire(
                ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
            return false;
        }

        auto dbLedger = loadByIndex(ledgerIndex, app_);
        if (! dbLedger ||
            (dbLedger->info().hash != ledgerHash) ||
            (dbLedger->info().parentHash != nodeLedger->info().parentHash))
        {
//理想情况下，我们还可以使用该索引检查多个分类账。
            JLOG (j_.debug()) <<
                "Ledger " << ledgerIndex << " mismatches SQL DB";
            doTxns = true;
        }

        if(! app_.getLedgerMaster().fixIndex(ledgerIndex, ledgerHash))
        {
            JLOG (j_.debug()) << "ledger " << ledgerIndex
                            << " had wrong entry in history";
            doTxns = true;
        }

        if (doNodes && !nodeLedger->walkLedger(app_.journal ("Ledger")))
        {
            JLOG (j_.debug()) << "Ledger " << ledgerIndex << " is missing nodes";
            app_.getLedgerMaster().clearLedger (ledgerIndex);
            app_.getInboundLedgers().acquire(
                ledgerHash, ledgerIndex, InboundLedger::Reason::GENERIC);
            return false;
        }

        if (doTxns && !pendSaveValidated(app_, nodeLedger, true, false))
        {
            JLOG (j_.debug()) << "Failed to save ledger " << ledgerIndex;
            return false;
        }

        return true;
    }

    /*返回指定分类帐的哈希。
        @param ledger index所需分类帐的索引。
        @param referenceledger[out]一个可选的已知良好的后续分类账。
        @返回分类帐的哈希值。如果找不到，这将全部为零位。
    **/

    LedgerHash getHash(
        LedgerIndex const& ledgerIndex,
        std::shared_ptr<ReadView const>& referenceLedger)
    {
        LedgerHash ledgerHash;

        if (!referenceLedger || (referenceLedger->info().seq < ledgerIndex))
        {
            referenceLedger = app_.getLedgerMaster().getValidatedLedger();
            if (!referenceLedger)
            {
                JLOG (j_.warn()) << "No validated ledger";
return ledgerHash; //我们无能为力。没有已验证的分类帐。
            }
        }

        if (referenceLedger->info().seq >= ledgerIndex)
        {
//看看我们需要的分类账哈希是否在参考分类账中
            ledgerHash = getLedgerHash(referenceLedger, ledgerIndex);
            if (ledgerHash.isZero())
            {
//不，试着再找一个分类账，可能有杂凑，我们
//需要：计算将具有
//我们需要的大麻。
                LedgerIndex refIndex = getCandidateLedger (ledgerIndex);
                LedgerHash refHash = getLedgerHash (referenceLedger, refIndex);

                bool const nonzero (refHash.isNonZero ());
                assert (nonzero);
                if (nonzero)
                {
//我们找到了一个更好的引用的哈希和序列
//分类帐。
                    referenceLedger =
                        app_.getInboundLedgers().acquire(
                            refHash, refIndex, InboundLedger::Reason::GENERIC);
                    if (referenceLedger)
                        ledgerHash = getLedgerHash(
                            referenceLedger, ledgerIndex);
                }
            }
        }
        else
            JLOG (j_.warn()) << "Validated ledger is prior to target ledger";

        return ledgerHash;
    }

    /*运行分类帐清除器。*/
    void doLedgerCleaner()
    {
        auto shouldExit = [this]()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return shouldExit_;
        };

        std::shared_ptr<ReadView const> goodLedger;

        while (! shouldExit())
        {
            LedgerIndex ledgerIndex;
            LedgerHash ledgerHash;
            bool doNodes;
            bool doTxns;

            while (app_.getFeeTrack().isLoadedLocal())
            {
                JLOG (j_.debug()) << "Waiting for load to subside";
                std::this_thread::sleep_for(std::chrono::seconds(5));
                if (shouldExit())
                    return;
            }

            {
                std::lock_guard<std::mutex> lock (mutex_);
                if ((minRange_ > maxRange_) ||
                    (maxRange_ == 0) || (minRange_ == 0))
                {
                    minRange_ = maxRange_ = 0;
                    state_ = State::readyToClean;
                    return;
                }
                ledgerIndex = maxRange_;
                doNodes = checkNodes_;
                doTxns = fixTxns_;
            }

            ledgerHash = getHash(ledgerIndex, goodLedger);

            bool fail = false;
            if (ledgerHash.isZero())
            {
                JLOG (j_.info()) << "Unable to get hash for ledger "
                               << ledgerIndex;
                fail = true;
            }
            else if (!doLedger(ledgerIndex, ledgerHash, doNodes, doTxns))
            {
                JLOG (j_.info()) << "Failed to process ledger " << ledgerIndex;
                fail = true;
            }

            if (fail)
            {
                {
                    std::lock_guard<std::mutex> lock (mutex_);
                    ++failures_;
                }
//等着收单赶上我们
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock (mutex_);
                    if (ledgerIndex == minRange_)
                        ++minRange_;
                    if (ledgerIndex == maxRange_)
                        --maxRange_;
                    failures_ = 0;
                }
//降低I/O压力，等待获取赶上我们
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        }
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

LedgerCleaner::LedgerCleaner (Stoppable& parent)
    : Stoppable ("LedgerCleaner", parent)
    , beast::PropertyStream::Source ("ledgercleaner")
{
}

LedgerCleaner::~LedgerCleaner() = default;

std::unique_ptr<LedgerCleaner>
make_LedgerCleaner (Application& app,
    Stoppable& parent, beast::Journal journal)
{
    return std::make_unique<LedgerCleanerImp>(app, parent, journal);
}

} //细节
} //涟漪
