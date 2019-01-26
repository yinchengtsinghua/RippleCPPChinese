
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

#include <ripple/app/main/Application.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/main/DBInit.h>
#include <ripple/app/main/BasicApp.h>
#include <ripple/app/main/Tuning.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/ledger/PendingSaves.h>
#include <ripple/app/ledger/InboundTransactions.h>
#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/main/LoadManager.h>
#include <ripple/app/main/NodeIdentity.h>
#include <ripple/app/main/NodeStoreScheduler.h>
#include <ripple/app/misc/AmendmentTable.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/misc/ValidatorSite.h>
#include <ripple/app/misc/ValidatorKeys.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/app/tx/apply.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/basics/ResolverAsio.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/Sustain.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/json/json_reader.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/make_Overlay.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/STParsedJSON.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/resource/Fees.h>
#include <ripple/beast/asio/io_latency_probe.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

namespace ripple {

//204/256约80%
static int const MAJORITY_FRACTION (204);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

class AppFamily : public Family
{
private:
    Application& app_;
    TreeNodeCache treecache_;
    FullBelowCache fullbelow_;
    NodeStore::Database& db_;
    bool const shardBacked_;
    beast::Journal j_;

//缺少节点处理程序
    LedgerIndex maxSeq = 0;
    std::mutex maxSeqLock;

    void acquire (
        uint256 const& hash,
        std::uint32_t seq)
    {
        if (hash.isNonZero())
        {
            auto j = app_.journal ("Ledger");

            JLOG (j.error()) <<
                "Missing node in " << to_string (hash);

            app_.getInboundLedgers ().acquire (
                hash, seq, shardBacked_ ?
                InboundLedger::Reason::SHARD :
                InboundLedger::Reason::GENERIC);
        }
    }

public:
    AppFamily (AppFamily const&) = delete;
    AppFamily& operator= (AppFamily const&) = delete;

    AppFamily (Application& app, NodeStore::Database& db,
            CollectorManager& collectorManager)
        : app_ (app)
        , treecache_ ("TreeNodeCache", 65536, std::chrono::minutes {1},
            stopwatch(), app.journal("TaggedCache"))
        , fullbelow_ ("full_below", stopwatch(),
            collectorManager.collector(),
                fullBelowTargetSize, fullBelowExpiration)
        , db_ (db)
        , shardBacked_ (
            dynamic_cast<NodeStore::DatabaseShard*>(&db) != nullptr)
        , j_ (app.journal("SHAMap"))
    {
    }

    beast::Journal const&
    journal() override
    {
        return j_;
    }

    FullBelowCache&
    fullbelow() override
    {
        return fullbelow_;
    }

    FullBelowCache const&
    fullbelow() const override
    {
        return fullbelow_;
    }

    TreeNodeCache&
    treecache() override
    {
        return treecache_;
    }

    TreeNodeCache const&
    treecache() const override
    {
        return treecache_;
    }

    NodeStore::Database&
    db() override
    {
        return db_;
    }

    NodeStore::Database const&
    db() const override
    {
        return db_;
    }

    bool
    isShardBacked() const override
    {
        return shardBacked_;
    }

    void
    missing_node (std::uint32_t seq) override
    {
        auto j = app_.journal ("Ledger");

        JLOG (j.error()) <<
            "Missing node in " << seq;

//防止递归调用
        std::unique_lock <std::mutex> lock (maxSeqLock);

        if (maxSeq == 0)
        {
            maxSeq = seq;

            do
            {
//尝试获取最近丢失的分类帐
                seq = maxSeq;

                lock.unlock();

//这可以调用缺少的节点处理程序
                acquire (
                    app_.getLedgerMaster().getHashBySeq (seq),
                    seq);

                lock.lock();
            }
            while (maxSeq != seq);
        }
        else if (maxSeq < seq)
        {
//我们发现了一个最近的分类账
//缺失节点
            maxSeq = seq;
        }
    }

    void
    missing_node (uint256 const& hash, std::uint32_t seq) override
    {
        acquire (hash, seq);
    }

    void
    reset () override
    {
        {
            std::lock_guard<std::mutex> l(maxSeqLock);
            maxSeq = 0;
        }
        fullbelow_.reset();
        treecache_.reset();
    }
};

} //细节

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//vfalc todo将函数定义移动到类声明中
class ApplicationImp
    : public Application
    , public RootStoppable
    , public BasicApp
{
private:
    class io_latency_sampler
    {
    private:
        beast::insight::Event m_event;
        beast::Journal m_journal;
        beast::io_latency_probe <std::chrono::steady_clock> m_probe;
        std::atomic<std::chrono::milliseconds> lastSample_;

    public:
        io_latency_sampler (
            beast::insight::Event ev,
            beast::Journal journal,
            std::chrono::milliseconds interval,
            boost::asio::io_service& ios)
            : m_event (ev)
            , m_journal (journal)
            , m_probe (interval, ios)
            , lastSample_ {}
        {
        }

        void
        start()
        {
            m_probe.sample (std::ref(*this));
        }

        template <class Duration>
        void operator() (Duration const& elapsed)
        {
            using namespace std::chrono;
            auto const lastSample = date::ceil<milliseconds>(elapsed);

            lastSample_ = lastSample;

            if (lastSample >= 10ms)
                m_event.notify (lastSample);
            if (lastSample >= 500ms)
            {
                JLOG(m_journal.warn()) <<
                    "io_service latency = " << lastSample.count();
            }
        }

        std::chrono::milliseconds
        get () const
        {
            return lastSample_.load();
        }

        void
        cancel ()
        {
            m_probe.cancel ();
        }

        void cancel_async ()
        {
            m_probe.cancel_async ();
        }
    };

public:
    std::unique_ptr<Config> config_;
    std::unique_ptr<Logs> logs_;
    std::unique_ptr<TimeKeeper> timeKeeper_;

    beast::Journal m_journal;
    std::unique_ptr<perf::PerfLog> perfLog_;
    Application::MutexType m_masterMutex;

//Shamapstore要求
    TransactionMaster m_txMaster;

    NodeStoreScheduler m_nodeStoreScheduler;
    std::unique_ptr <SHAMapStore> m_shaMapStore;
    PendingSaves pendingSaves_;
    AccountIDCache accountIDCache_;
    boost::optional<OpenLedger> openLedger_;

//这些是不可停止的派生
    NodeCache m_tempNodeCache;
    std::unique_ptr <CollectorManager> m_collectorManager;
    CachedSLEs cachedSLEs_;
    std::pair<PublicKey, SecretKey> nodeIdentity_;
    ValidatorKeys const validatorKeys_;

    std::unique_ptr <Resource::Manager> m_resourceManager;

//这些是可停止的相关
    std::unique_ptr <JobQueue> m_jobQueue;
    std::unique_ptr <NodeStore::Database> m_nodeStore;
    std::unique_ptr <NodeStore::DatabaseShard> shardStore_;
    detail::AppFamily family_;
    std::unique_ptr <detail::AppFamily> sFamily_;
//vfalc todo make orderbookdb摘要
    OrderBookDB m_orderBookDB;
    std::unique_ptr <PathRequests> m_pathRequests;
    std::unique_ptr <LedgerMaster> m_ledgerMaster;
    std::unique_ptr <InboundLedgers> m_inboundLedgers;
    std::unique_ptr <InboundTransactions> m_inboundTransactions;
    TaggedCache <uint256, AcceptedLedger> m_acceptedLedgerCache;
    std::unique_ptr <NetworkOPs> m_networkOPs;
    std::unique_ptr <Cluster> cluster_;
    std::unique_ptr <ManifestCache> validatorManifests_;
    std::unique_ptr <ManifestCache> publisherManifests_;
    std::unique_ptr <ValidatorList> validators_;
    std::unique_ptr <ValidatorSite> validatorSites_;
    std::unique_ptr <ServerHandler> serverHandler_;
    std::unique_ptr <AmendmentTable> m_amendmentTable;
    std::unique_ptr <LoadFeeTrack> mFeeTrack;
    std::unique_ptr <HashRouter> mHashRouter;
    RCLValidations mValidations;
    std::unique_ptr <LoadManager> m_loadManager;
    std::unique_ptr <TxQ> txQ_;
    ClosureCounter<void, boost::system::error_code const&> waitHandlerCounter_;
    boost::asio::steady_timer sweepTimer_;
    boost::asio::steady_timer entropyTimer_;
    bool startTimers_;

    std::unique_ptr <DatabaseCon> mTxnDB;
    std::unique_ptr <DatabaseCon> mLedgerDB;
    std::unique_ptr <DatabaseCon> mWalletDB;
    std::unique_ptr <Overlay> m_overlay;
    std::vector <std::unique_ptr<Stoppable>> websocketServers_;

    boost::asio::signal_set m_signals;

    std::condition_variable cv_;
    std::mutex mut_;
    bool isTimeToStop = false;

    std::atomic<bool> checkSigs_;

    std::unique_ptr <ResolverAsio> m_resolver;

    io_latency_sampler m_io_latency_sampler;

//——————————————————————————————————————————————————————————————

    static
    std::size_t
    numberOfThreads(Config const& config)
    {
    #if RIPPLE_SINGLE_IO_SERVICE_THREAD
        return 1;
    #else
        return (config.NODE_SIZE >= 2) ? 2 : 1;
    #endif
    }

//——————————————————————————————————————————————————————————————

    ApplicationImp (
            std::unique_ptr<Config> config,
            std::unique_ptr<Logs> logs,
            std::unique_ptr<TimeKeeper> timeKeeper)
        : RootStoppable ("Application")
        , BasicApp (numberOfThreads(*config))
        , config_ (std::move(config))
        , logs_ (std::move(logs))
        , timeKeeper_ (std::move(timeKeeper))

        , m_journal (logs_->journal("Application"))

//必须在启动任何其他线程之前启动PerfLog。
        , perfLog_ (perf::make_PerfLog(
            perf::setup_PerfLog(config_->section("perf"), config_->CONFIG_DIR),
            *this, logs_->journal("PerfLog"), [this] () { signalStop(); }))

        , m_txMaster (*this)

        , m_nodeStoreScheduler (*this)

        , m_shaMapStore (make_SHAMapStore (*this, setup_SHAMapStore (*config_),
            *this, m_nodeStoreScheduler, logs_->journal("SHAMapStore"),
            logs_->journal("NodeObject"), m_txMaster, *config_))

        , accountIDCache_(128000)

        , m_tempNodeCache ("NodeCache", 16384, std::chrono::seconds {90},
            stopwatch(), logs_->journal("TaggedCache"))

        , m_collectorManager (CollectorManager::New (
            config_->section (SECTION_INSIGHT), logs_->journal("Collector")))
        , cachedSLEs_ (std::chrono::minutes(1), stopwatch())
        , validatorKeys_(*config_, m_journal)

        , m_resourceManager (Resource::make_Manager (
            m_collectorManager->collector(), logs_->journal("Resource")))

//从那以后，排队的工作就得很早。
//几乎每件事都是工作队列中一个可停止的子项。
//
        , m_jobQueue (std::make_unique<JobQueue>(
            m_collectorManager->group ("jobq"), m_nodeStoreScheduler,
            logs_->journal("JobQueue"), *logs_, *perfLog_))

//
//调用AddJob的任何内容都必须是JobQueue的后代
//
        , m_nodeStore (
            m_shaMapStore->makeDatabase ("NodeStore.main", 4, *m_jobQueue))

        , shardStore_ (
            m_shaMapStore->makeDatabaseShard ("ShardStore", 4, *m_jobQueue))

        , family_ (*this, *m_nodeStore, *m_collectorManager)

        , m_orderBookDB (*this, *m_jobQueue)

        , m_pathRequests (std::make_unique<PathRequests> (
            *this, logs_->journal("PathRequest"), m_collectorManager->collector ()))

        , m_ledgerMaster (std::make_unique<LedgerMaster> (*this, stopwatch (),
            *m_jobQueue, m_collectorManager->collector (),
            logs_->journal("LedgerMaster")))

//vfalc注释必须在networkops之前出现，以防止由于
//到析构函数中的依赖项。
//
        , m_inboundLedgers (make_InboundLedgers (*this, stopwatch(),
            *m_jobQueue, m_collectorManager->collector ()))

        , m_inboundTransactions (make_InboundTransactions
            ( *this, stopwatch()
            , *m_jobQueue
            , m_collectorManager->collector ()
            , [this](std::shared_ptr <SHAMap> const& set,
                bool fromAcquire)
            {
                gotTXSet (set, fromAcquire);
            }))

        , m_acceptedLedgerCache ("AcceptedLedger", 4, std::chrono::minutes {1},
            stopwatch(), logs_->journal("TaggedCache"))

        , m_networkOPs (make_NetworkOPs (*this, stopwatch(),
            config_->standalone(), config_->NETWORK_QUORUM, config_->START_VALID,
            *m_jobQueue, *m_ledgerMaster, *m_jobQueue, validatorKeys_,
            get_io_service(), logs_->journal("NetworkOPs")))

        , cluster_ (std::make_unique<Cluster> (
            logs_->journal("Overlay")))

        , validatorManifests_ (std::make_unique<ManifestCache> (
            logs_->journal("ManifestCache")))

        , publisherManifests_ (std::make_unique<ManifestCache> (
            logs_->journal("ManifestCache")))

        , validators_ (std::make_unique<ValidatorList> (
            *validatorManifests_, *publisherManifests_, *timeKeeper_,
            logs_->journal("ValidatorList"), config_->VALIDATION_QUORUM))

        , validatorSites_ (std::make_unique<ValidatorSite> (
            get_io_service (), *validators_, logs_->journal("ValidatorSite")))

        , serverHandler_ (make_ServerHandler (*this, *m_networkOPs, get_io_service (),
            *m_jobQueue, *m_networkOPs, *m_resourceManager,
            *m_collectorManager))

        , mFeeTrack (std::make_unique<LoadFeeTrack>(logs_->journal("LoadManager")))

        , mHashRouter (std::make_unique<HashRouter>(
            stopwatch(), HashRouter::getDefaultHoldTime (),
            HashRouter::getDefaultRecoverLimit ()))

        , mValidations (ValidationParms(),stopwatch(), *this, logs_->journal("Validations"))

        , m_loadManager (make_LoadManager (*this, *this, logs_->journal("LoadManager")))

        , txQ_(make_TxQ(setup_TxQ(*config_), logs_->journal("TxQ")))

        , sweepTimer_ (get_io_service())

        , entropyTimer_ (get_io_service())

        , startTimers_ (false)

        , m_signals (get_io_service())

        , checkSigs_(true)

        , m_resolver (ResolverAsio::New (get_io_service(), logs_->journal("Resolver")))

        , m_io_latency_sampler (m_collectorManager->collector()->make_event ("ios_latency"),
            logs_->journal("Application"), std::chrono::milliseconds (100), get_io_service())
    {
        if (shardStore_)
            sFamily_ = std::make_unique<detail::AppFamily>(
                *this, *shardStore_, *m_collectorManager);
        add (m_resourceManager.get ());

//
//vvalco-读这个！
//
//不要启动线程、打开套接字或执行任何“真正的工作”
//在构造函数内部。把它放在开始。或者，如果你必须，
//将其放入安装程序（但安装程序中的所有内容都应移到“开始”位置
//不管怎样。
//
//原因是单元测试要求应用程序对象
//被创造。但我们并没有真正启动所有的线程、套接字，
//以及运行单元测试时的服务。所以任何
//如果需要停止，将无法正确停止
//在此构造函数中启动。
//

//弗法科黑客
        m_nodeStoreScheduler.setJobQueue (*m_jobQueue);

        add (m_ledgerMaster->getPropertySource ());
    }

//——————————————————————————————————————————————————————————————

    bool setup() override;
    void doStart(bool withTimers) override;
    void run() override;
    bool isShutdown() override;
    void signalStop() override;
    bool checkSigs() const override;
    void checkSigs(bool) override;
    int fdlimit () const override;

//——————————————————————————————————————————————————————————————

    Logs&
    logs() override
    {
        return *logs_;
    }

    Config&
    config() override
    {
        return *config_;
    }

    CollectorManager& getCollectorManager () override
    {
        return *m_collectorManager;
    }

    Family& family() override
    {
        return family_;
    }

    Family* shardFamily() override
    {
        return sFamily_.get();
    }

    TimeKeeper&
    timeKeeper() override
    {
        return *timeKeeper_;
    }

    JobQueue& getJobQueue () override
    {
        return *m_jobQueue;
    }

    std::pair<PublicKey, SecretKey> const&
    nodeIdentity () override
    {
        return nodeIdentity_;
    }

    PublicKey const &
    getValidationPublicKey() const override
    {
        return validatorKeys_.publicKey;
    }

    NetworkOPs& getOPs () override
    {
        return *m_networkOPs;
    }

    boost::asio::io_service& getIOService () override
    {
        return get_io_service();
    }

    std::chrono::milliseconds getIOLatency () override
    {
        return m_io_latency_sampler.get ();
    }

    LedgerMaster& getLedgerMaster () override
    {
        return *m_ledgerMaster;
    }

    InboundLedgers& getInboundLedgers () override
    {
        return *m_inboundLedgers;
    }

    InboundTransactions& getInboundTransactions () override
    {
        return *m_inboundTransactions;
    }

    TaggedCache <uint256, AcceptedLedger>& getAcceptedLedgerCache () override
    {
        return m_acceptedLedgerCache;
    }

    void gotTXSet (std::shared_ptr<SHAMap> const& set, bool fromAcquire)
    {
        if (set)
            m_networkOPs->mapComplete (set, fromAcquire);
    }

    TransactionMaster& getMasterTransaction () override
    {
        return m_txMaster;
    }

    perf::PerfLog& getPerfLog () override
    {
        return *perfLog_;
    }

    NodeCache& getTempNodeCache () override
    {
        return m_tempNodeCache;
    }

    NodeStore::Database& getNodeStore () override
    {
        return *m_nodeStore;
    }

    NodeStore::DatabaseShard* getShardStore () override
    {
        return shardStore_.get();
    }

    Application::MutexType& getMasterMutex () override
    {
        return m_masterMutex;
    }

    LoadManager& getLoadManager () override
    {
        return *m_loadManager;
    }

    Resource::Manager& getResourceManager () override
    {
        return *m_resourceManager;
    }

    OrderBookDB& getOrderBookDB () override
    {
        return m_orderBookDB;
    }

    PathRequests& getPathRequests () override
    {
        return *m_pathRequests;
    }

    CachedSLEs&
    cachedSLEs() override
    {
        return cachedSLEs_;
    }

    AmendmentTable& getAmendmentTable() override
    {
        return *m_amendmentTable;
    }

    LoadFeeTrack& getFeeTrack () override
    {
        return *mFeeTrack;
    }

    HashRouter& getHashRouter () override
    {
        return *mHashRouter;
    }

    RCLValidations& getValidations () override
    {
        return mValidations;
    }

    ValidatorList& validators () override
    {
        return *validators_;
    }

    ValidatorSite& validatorSites () override
    {
        return *validatorSites_;
    }

    ManifestCache& validatorManifests() override
    {
        return *validatorManifests_;
    }

    ManifestCache& publisherManifests() override
    {
        return *publisherManifests_;
    }

    Cluster& cluster () override
    {
        return *cluster_;
    }

    SHAMapStore& getSHAMapStore () override
    {
        return *m_shaMapStore;
    }

    PendingSaves& pendingSaves() override
    {
        return pendingSaves_;
    }

    AccountIDCache const&
    accountIDCache() const override
    {
        return accountIDCache_;
    }

    OpenLedger&
    openLedger() override
    {
        return *openLedger_;
    }

    OpenLedger const&
    openLedger() const override
    {
        return *openLedger_;
    }

    Overlay& overlay () override
    {
        return *m_overlay;
    }

    TxQ& getTxQ() override
    {
        assert(txQ_.get() != nullptr);
        return *txQ_;
    }

    DatabaseCon& getTxnDB () override
    {
        assert (mTxnDB.get() != nullptr);
        return *mTxnDB;
    }
    DatabaseCon& getLedgerDB () override
    {
        assert (mLedgerDB.get() != nullptr);
        return *mLedgerDB;
    }
    DatabaseCon& getWalletDB () override
    {
        assert (mWalletDB.get() != nullptr);
        return *mWalletDB;
    }

    bool serverOkay (std::string& reason) override;

    beast::Journal journal (std::string const& name) override;

//——————————————————————————————————————————————————————————————
    bool initSqliteDbs ()
    {
        assert (mTxnDB.get () == nullptr);
        assert (mLedgerDB.get () == nullptr);
        assert (mWalletDB.get () == nullptr);

        DatabaseCon::Setup setup = setup_DatabaseCon (*config_);
        mTxnDB = std::make_unique <DatabaseCon> (setup, TxnDBName,
                TxnDBInit, TxnDBCount);
        mLedgerDB = std::make_unique <DatabaseCon> (setup, "ledger.db",
                LedgerDBInit, LedgerDBCount);
        mWalletDB = std::make_unique <DatabaseCon> (setup, "wallet.db",
                WalletDBInit, WalletDBCount);

        return
            mTxnDB.get () != nullptr &&
            mLedgerDB.get () != nullptr &&
            mWalletDB.get () != nullptr;
    }

    void signalled(const boost::system::error_code& ec, int signal_number)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
//指示信号处理程序已中止
//什么也不做
        }
        else if (ec)
        {
            JLOG(m_journal.error()) << "Received signal: " << signal_number
                                  << " with error: " << ec.message();
        }
        else
        {
            JLOG(m_journal.debug()) << "Received signal: " << signal_number;
            signalStop();
        }
    }

//——————————————————————————————————————————————————————————————
//
//可停止的
//

    void onPrepare() override
    {
    }

    void onStart () override
    {
        JLOG(m_journal.info())
            << "Application starting. Version is " << BuildInfo::getVersionString();

        using namespace std::chrono_literals;
        if(startTimers_)
        {
            setSweepTimer();
            setEntropyTimer();
        }

        m_io_latency_sampler.start();

        m_resolver->start ();
    }

//调用以指示关闭。
    void onStop () override
    {
        JLOG(m_journal.debug()) << "Application stopping";

        m_io_latency_sampler.cancel_async ();

//vfalco巨大的入侵，我们必须迫使探测器取消
//在我们停止IO服务队列之前
//在其析构函数中解除阻塞。解决办法是
//IO对象优雅地处理退出，这样我们可以
//从IO服务：：run（）自然返回，而不是
//强制调用IO服务：：stop（）。
        m_io_latency_sampler.cancel ();

        m_resolver->stop_async ();

//这是一个黑客-我们需要等待解析器
//停下来。在我们停止IO服务器队列或奇怪之前
//事情会发生的。
        m_resolver->stop ();

        {
            boost::system::error_code ec;
            sweepTimer_.cancel (ec);
            if (ec)
            {
                JLOG (m_journal.error())
                    << "Application: sweepTimer cancel error: "
                    << ec.message();
            }

            ec.clear();
            entropyTimer_.cancel (ec);
            if (ec)
            {
                JLOG (m_journal.error())
                    << "Application: entropyTimer cancel error: "
                    << ec.message();
            }
        }
//确保在计时器中挂起的所有等待处理程序都已完成
//在我们宣布停止之前。
        using namespace std::chrono_literals;
        waitHandlerCounter_.join("Application", 1s, m_journal);

        JLOG(m_journal.debug()) << "Flushing validations";
        mValidations.flush ();
        JLOG(m_journal.debug()) << "Validations flushed";

        validatorSites_->stop ();

//todo将清单存储在manifests.sqlite而不是wallet.db中
        validatorManifests_->save (getWalletDB (), "ValidatorManifests",
            [this](PublicKey const& pubKey)
            {
                return validators().listed (pubKey);
            });

        publisherManifests_->save (getWalletDB (), "PublisherManifests",
            [this](PublicKey const& pubKey)
            {
                return validators().trustedPublisher (pubKey);
            });

        stopped ();
    }

//——————————————————————————————————————————————————————————————
//
//属性流
//

    void onWrite (beast::PropertyStream::Map& stream) override
    {
    }

//——————————————————————————————————————————————————————————————

    void setSweepTimer ()
    {
//仅当WaithandlerCounter_u尚未加入时才启动计时器。
        if (auto optionalCountedHandler = waitHandlerCounter_.wrap (
            [this] (boost::system::error_code const& e)
            {
                if ((e.value() == boost::system::errc::success) &&
                    (! m_jobQueue->isStopped()))
                {
                    m_jobQueue->addJob(
                        jtSWEEP, "sweep", [this] (Job&) { doSweep(); });
                }
//如果发生意外错误，请尽可能恢复。
                if (e.value() != boost::system::errc::success &&
                    e.value() != boost::asio::error::operation_aborted)
                {
//稍后再试，希望一切顺利。
                    JLOG (m_journal.error())
                       << "Sweep timer got error '" << e.message()
                       << "'.  Restarting timer.";
                    setSweepTimer();
                }
            }))
        {
            using namespace std::chrono;
            sweepTimer_.expires_from_now(
                seconds{config_->getSize(siSweepInterval)});
            sweepTimer_.async_wait (std::move (*optionalCountedHandler));
        }
    }

    void setEntropyTimer ()
    {
//仅当WaithandlerCounter_u尚未加入时才启动计时器。
        if (auto optionalCountedHandler = waitHandlerCounter_.wrap (
            [this] (boost::system::error_code const& e)
            {
                if (e.value() == boost::system::errc::success)
                {
                    crypto_prng().mix_entropy();
                    setEntropyTimer();
                }
//如果发生意外错误，请尽可能恢复。
                if (e.value() != boost::system::errc::success &&
                    e.value() != boost::asio::error::operation_aborted)
                {
//稍后再试，希望一切顺利。
                    JLOG (m_journal.error())
                       << "Entropy timer got error '" << e.message()
                       << "'.  Restarting timer.";
                    setEntropyTimer();
                }
            }))
        {
            using namespace std::chrono_literals;
            entropyTimer_.expires_from_now (5min);
            entropyTimer_.async_wait (std::move (*optionalCountedHandler));
        }
    }

    void doSweep ()
    {
        if (! config_->standalone())
        {
            boost::filesystem::space_info space =
                boost::filesystem::space (config_->legacy ("database_path"));

            if (space.available < megabytes(512))
            {
                JLOG(m_journal.fatal())
                    << "Remaining free disk space is less than 512MB";
                signalStop ();
            }

            DatabaseCon::Setup dbSetup = setup_DatabaseCon(*config_);
            boost::filesystem::path dbPath = dbSetup.dataDir / TxnDBName;
            boost::system::error_code ec;
            boost::optional<std::uint64_t> dbSize = boost::filesystem::file_size(dbPath, ec);
            if (ec)
            {
                JLOG(m_journal.error())
                    << "Error checking transaction db file size: "
                    << ec.message();
                dbSize.reset();
            }

            auto db = mTxnDB->checkoutDb();
            static auto const pageSize = [&]{
                std::uint32_t ps;
                *db << "PRAGMA page_size;", soci::into(ps);
                return ps;
            }();
            static auto const maxPages = [&]{
                std::uint32_t mp;
                *db << "PRAGMA max_page_count;" , soci::into(mp);
                return mp;
            }();
            std::uint32_t pageCount;
            *db << "PRAGMA page_count;", soci::into(pageCount);
            std::uint32_t freePages = maxPages - pageCount;
            std::uint64_t freeSpace =
                safe_cast<std::uint64_t>(freePages) * pageSize;
            JLOG(m_journal.info())
               << "Transaction DB pathname: " << dbPath.string()
               << "; file size: " << dbSize.value_or(-1) << " bytes"
               << "; SQLite page size: " << pageSize  << " bytes"
               << "; Free pages: " << freePages
               << "; Free space: " << freeSpace << " bytes; "
               << "Note that this does not take into account available disk "
                  "space.";

            if (freeSpace < megabytes(512))
            {
                JLOG(m_journal.fatal())
                    << "Free SQLite space for transaction db is less than "
                       "512MB. To fix this, rippled must be executed with the "
                       "vacuum <sqlitetmpdir> parameter before restarting. "
                       "Note that this activity can take multiple days, "
                       "depending on database size.";
                signalStop();
            }
        }

//vvalco注意到通话顺序有关系吗？
//vfalc todo使用观察器修复依赖倒置，
//让听众注册“onsweep（）”通知。

        family().fullbelow().sweep();
        if (sFamily_)
            sFamily_->fullbelow().sweep();
        getMasterTransaction().sweep();
        getNodeStore().sweep();
        if (shardStore_)
            shardStore_->sweep();
        getLedgerMaster().sweep();
        getTempNodeCache().sweep();
        getValidations().expire();
        getInboundLedgers().sweep();
        m_acceptedLedgerCache.sweep();
        family().treecache().sweep();
        if (sFamily_)
            sFamily_->treecache().sweep();
        cachedSLEs_.expire();

//将计时器设置为稍后再进行一次扫描。
        setSweepTimer();
    }

    LedgerIndex getMaxDisallowedLedger() override
    {
        return maxDisallowedLedger_;
    }


private:
//对于新启动的验证器，这是最大的持久化分类帐
//新的验证必须大于此。
    std::atomic<LedgerIndex> maxDisallowedLedger_ {0};

    void addTxnSeqField();
    void addValidationSeqFields();
    bool updateTables ();
    bool nodeToShards ();
    bool validateShards ();
    void startGenesisLedger ();

    std::shared_ptr<Ledger>
    getLastFullLedger();

    std::shared_ptr<Ledger>
    loadLedgerFromFile (
        std::string const& ledgerID);

    bool loadOldLedger (
        std::string const& ledgerID,
        bool replay,
        bool isFilename);

    void setMaxDisallowedLedger();
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//vfalc todo将这个函数分解成许多小的初始化段。
//或者更好地将这些初始化重构为raii类
//是应用程序对象的成员。
//
bool ApplicationImp::setup()
{
//vfalc注：0表示使用试探法确定线程计数。
    m_jobQueue->setThreadCount (config_->WORKERS, config_->standalone());

//我们要截取并等待ctrl-c终止进程
    m_signals.add (SIGINT);

    m_signals.async_wait(std::bind(&ApplicationImp::signalled, this,
        std::placeholders::_1, std::placeholders::_2));

    assert (mTxnDB == nullptr);

    auto debug_log = config_->getDebugLogFile ();

    if (!debug_log.empty ())
    {
//允许调试消息转到文件，但只允许警告或更高级别的消息转到
//常规输出（除非详细）

        if (!logs_->open(debug_log))
            std::cerr << "Can't open log file " << debug_log << '\n';

        using namespace beast::severities;
        if (logs_->threshold() > kDebug)
            logs_->threshold (kDebug);
    }

    logs_->silent (config_->silent());

    if (!config_->standalone())
        timeKeeper_->run(config_->SNTP_SERVERS);

    if (!initSqliteDbs ())
    {
        JLOG(m_journal.fatal()) << "Cannot create database connections!";
        return false;
    }

    if (validatorKeys_.publicKey.size())
        setMaxDisallowedLedger();

    getLedgerDB ().getSession ()
        << boost::str (boost::format ("PRAGMA cache_size=-%d;") %
                        (config_->getSize (siLgrDBCache) * kilobytes(1)));

    getTxnDB ().getSession ()
            << boost::str (boost::format ("PRAGMA cache_size=-%d;") %
                            (config_->getSize (siTxnDBCache) * kilobytes(1)));

    mTxnDB->setupCheckpointing (m_jobQueue.get(), logs());
    mLedgerDB->setupCheckpointing (m_jobQueue.get(), logs());

    if (!updateTables ())
        return false;

//配置服务器支持的修订
    {
        auto const& sa = detail::supportedAmendments();
        std::vector<std::string> saHashes;
        saHashes.reserve(sa.size());
        for (auto const& name : sa)
        {
            auto const f = getRegisteredFeature(name);
            BOOST_ASSERT(f);
            if (f)
                saHashes.push_back(to_string(*f) + " " + name);
        }
        Section supportedAmendments ("Supported Amendments");
        supportedAmendments.append (saHashes);

        Section enabledAmendments = config_->section (SECTION_AMENDMENTS);

        m_amendmentTable = make_AmendmentTable (
            weeks{2},
            MAJORITY_FRACTION,
            supportedAmendments,
            enabledAmendments,
            config_->section (SECTION_VETO_AMENDMENTS),
            logs_->journal("Amendments"));
    }

    Pathfinder::initPathTable();

    auto const startUp = config_->START_UP;
    if (startUp == Config::FRESH)
    {
        JLOG(m_journal.info()) << "Starting new Ledger";

        startGenesisLedger ();
    }
    else if (startUp == Config::LOAD ||
                startUp == Config::LOAD_FILE ||
                startUp == Config::REPLAY)
    {
        JLOG(m_journal.info()) <<
            "Loading specified Ledger";

        if (!loadOldLedger (config_->START_LEDGER,
                            startUp == Config::REPLAY,
                            startUp == Config::LOAD_FILE))
        {
            JLOG(m_journal.error()) <<
                "The specified ledger could not be loaded.";
            return false;
        }
    }
    else if (startUp == Config::NETWORK)
    {
//一旦我们有了一个稳定的网络，这可能会成为默认值。
        if (!config_->standalone())
            m_networkOPs->setNeedNetworkLedger();

        startGenesisLedger ();
    }
    else
    {
        startGenesisLedger ();
    }

    m_orderBookDB.setup (getLedgerMaster ().getCurrentLedger ());

    nodeIdentity_ = loadNodeIdentity (*this);

    if (!cluster_->load (config().section(SECTION_CLUSTER_NODES)))
    {
        JLOG(m_journal.fatal()) << "Invalid entry in cluster configuration.";
        return false;
    }

    {
        if(validatorKeys_.configInvalid())
            return false;

        if (!validatorManifests_->load (
            getWalletDB (), "ValidatorManifests", validatorKeys_.manifest,
            config().section (SECTION_VALIDATOR_KEY_REVOCATION).values ()))
        {
            JLOG(m_journal.fatal()) << "Invalid configured validator manifest.";
            return false;
        }

        publisherManifests_->load (
            getWalletDB (), "PublisherManifests");

//设置受信任的验证程序
        if (!validators_->load (
                validatorKeys_.publicKey,
                config().section (SECTION_VALIDATORS).values (),
                config().section (SECTION_VALIDATOR_LIST_KEYS).values ()))
        {
            JLOG(m_journal.fatal()) <<
                "Invalid entry in validator configuration.";
            return false;
        }
    }

    if (!validatorSites_->load (
        config().section (SECTION_VALIDATOR_LIST_SITES).values ()))
    {
        JLOG(m_journal.fatal()) <<
            "Invalid entry in [" << SECTION_VALIDATOR_LIST_SITES << "]";
        return false;
    }

    using namespace std::chrono;
    m_nodeStore->tune(config_->getSize(siNodeCacheSize),
                      seconds{config_->getSize(siNodeCacheAge)});
    m_ledgerMaster->tune(config_->getSize(siLedgerSize),
                         seconds{config_->getSize(siLedgerAge)});
    family().treecache().setTargetSize(config_->getSize (siTreeCacheSize));
    family().treecache().setTargetAge(
        seconds{config_->getSize(siTreeCacheAge)});
    if (shardStore_)
    {
        shardStore_->tune(config_->getSize(siNodeCacheSize),
            seconds{config_->getSize(siNodeCacheAge)});
        sFamily_->treecache().setTargetSize(config_->getSize(siTreeCacheSize));
        sFamily_->treecache().setTargetAge(
            seconds{config_->getSize(siTreeCacheAge)});
    }

//——————————————————————————————————————————————————————————————
//
//服务器
//
//——————————————————————————————————————————————————————————————

//不幸的是，在独立模式下，一些代码仍然
//愚蠢地调用overlay（）。修好后我们可以
//在条件内移动实例化：
//
//如果（！）config.standalone（））
    m_overlay = make_Overlay (*this, setup_Overlay(*config_), *m_jobQueue,
        *serverHandler_, *m_resourceManager, *m_resolver, get_io_service(),
        *config_);
add (*m_overlay); //添加到PropertyStream

    if (!config_->standalone())
    {
//验证和节点导入需要sqlite db
        if (config_->nodeToShard && !nodeToShards())
            return false;

        if (config_->validateShards && !validateShards())
            return false;
    }

    validatorSites_->start ();

//开始第一轮共识
    if (! m_networkOPs->beginConsensus(m_ledgerMaster->getClosedLedger()->info().hash))
    {
        JLOG(m_journal.fatal()) << "Unable to start consensus";
        return false;
    }

    {
        try
        {
            auto setup = setup_ServerHandler(
                *config_, beast::logstream{m_journal.error()});
            setup.makeContexts();
            serverHandler_->setup(setup, m_journal);
        }
        catch (std::exception const& e)
        {
            if (auto stream = m_journal.fatal())
            {
                stream << "Unable to setup server handler";
                if(std::strlen(e.what()) > 0)
                    stream << ": " << e.what();
            }
            return false;
        }
    }

//开始连接到网络。
    if (!config_->standalone())
    {
//从概念上讲，这个消息应该在这里吗？理论上是这样的
//如果显示消息，则应从PeerFinder显示。
        if (config_->PEER_PRIVATE && config_->IPS_FIXED.empty ())
        {
            JLOG(m_journal.warn())
                << "No outbound peer connections will be made";
        }

//vfalc注意到状态计时器重置死锁检测器。
//
        m_networkOPs->setStateTimer ();
    }
    else
    {
        JLOG(m_journal.warn()) << "Running in standalone mode";

        m_networkOPs->setStandAlone ();
    }

    if (config_->canSign())
    {
        JLOG(m_journal.warn()) <<
            "*** The server is configured to allow the 'sign' and 'sign_for'";
        JLOG(m_journal.warn()) <<
            "*** commands. These commands have security implications and have";
        JLOG(m_journal.warn()) <<
            "*** been deprecated. They will be removed in a future release of";
        JLOG(m_journal.warn()) <<
            "*** rippled.";
        JLOG(m_journal.warn()) <<
            "*** If you do not use them to sign transactions please edit your";
        JLOG(m_journal.warn()) <<
            "*** configuration file and remove the [enable_signing] stanza.";
        JLOG(m_journal.warn()) <<
            "*** If you do use them to sign transactions please migrate to a";
        JLOG(m_journal.warn()) <<
            "*** standalone signing solution as soon as possible.";
    }

//
//执行启动RPC命令。
//
    for (auto cmd : config_->section(SECTION_RPC_STARTUP).lines())
    {
        Json::Reader jrReader;
        Json::Value jvCommand;

        if (! jrReader.parse (cmd, jvCommand))
        {
            JLOG(m_journal.fatal()) <<
                "Couldn't parse entry in [" << SECTION_RPC_STARTUP <<
                "]: '" << cmd;
        }

        if (!config_->quiet())
        {
            JLOG(m_journal.fatal()) << "Startup RPC: " << jvCommand << std::endl;
        }

        Resource::Charge loadType = Resource::feeReferenceRPC;
        Resource::Consumer c;
        RPC::Context context { journal ("RPCHandler"), jvCommand, *this,
            loadType, getOPs (), getLedgerMaster(), c, Role::ADMIN };

        Json::Value jvResult;
        RPC::doCommand (context, jvResult);

        if (!config_->quiet())
        {
            JLOG(m_journal.fatal()) << "Result: " << jvResult << std::endl;
        }
    }

    return true;
}

void
ApplicationImp::doStart(bool withTimers)
{
    startTimers_ = withTimers;
    prepare ();
    start ();
}

void
ApplicationImp::run()
{
    if (!config_->standalone())
    {
//vvalco注意到这似乎是不必要的。如果我们正确地重构负载
//管理器，那么死锁检测器就可以一直处于“待命”状态。
//
        getLoadManager ().activateDeadlockDetector ();
    }

    {
        std::unique_lock<std::mutex> lk{mut_};
        cv_.wait(lk, [this]{return isTimeToStop;});
    }

//停止服务器。当这一切恢复时，
//应停止可停止的物体。
    JLOG(m_journal.info()) << "Received shutdown request";
    stop (m_journal);
    JLOG(m_journal.info()) << "Done.";
    StopSustain();
}

void
ApplicationImp::signalStop()
{
//取消阻止主线程（位于run（）中）。
//
    std::lock_guard<std::mutex> lk{mut_};
    isTimeToStop = true;
    cv_.notify_all();
}

bool
ApplicationImp::isShutdown()
{
//来自可停止混音
    return isStopped();
}

bool ApplicationImp::checkSigs() const
{
    return checkSigs_;
}

void ApplicationImp::checkSigs(bool check)
{
    checkSigs_ = check;
}

int ApplicationImp::fdlimit() const
{
//标准句柄、配置文件、杂项I/O等：
    int needed = 128;

//对等连接的配置对等限制的1.5倍：
    needed += static_cast<int>(0.5 + (1.5 * m_overlay->limit()));

//后端所需的FDS数量（内部
//如果启用在线删除，则加倍）。
    needed += std::max(5, m_shaMapStore->fdlimit());

//一个端口可以接受的每个传入连接一个fd，或者
//如果没有设置限制，则假定它将处理256个客户机。
    for(auto const& p : serverHandler_->setup().ports)
        needed += std::max (256, p.limit);

//我们需要的最小文件描述符数是1024：
    return std::max(1024, needed);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void
ApplicationImp::startGenesisLedger()
{
    std::vector<uint256> initialAmendments =
        (config_->START_UP == Config::FRESH) ?
            m_amendmentTable->getDesired() :
            std::vector<uint256>{};

    std::shared_ptr<Ledger> const genesis =
        std::make_shared<Ledger>(
            create_genesis,
            *config_,
            initialAmendments,
            family());
    m_ledgerMaster->storeLedger (genesis);

    auto const next = std::make_shared<Ledger>(
        *genesis, timeKeeper().closeTime());
    next->updateSkipList ();
    next->setImmutable (*config_);
    openLedger_.emplace(next, cachedSLEs_,
        logs_->journal("OpenLedger"));
    m_ledgerMaster->storeLedger(next);
    m_ledgerMaster->switchLCL (next);
}

std::shared_ptr<Ledger>
ApplicationImp::getLastFullLedger()
{
    auto j = journal ("Ledger");

    try
    {
        std::shared_ptr<Ledger> ledger;
        std::uint32_t seq;
        uint256 hash;

        std::tie (ledger, seq, hash) =
            loadLedgerHelper (
                "order by LedgerSeq desc limit 1", *this);

        if (!ledger)
            return ledger;

        ledger->setImmutable(*config_);

        if (getLedgerMaster ().haveLedger (seq))
            ledger->setValidated ();

        if (ledger->info().hash == hash)
        {
            JLOG (j.trace()) << "Loaded ledger: " << hash;
            return ledger;
        }

        if (auto stream = j.error())
        {
            stream  << "Failed on ledger";
            Json::Value p;
            addJson (p, {*ledger, LedgerFill::full});
            stream << p;
        }

        return {};
    }
    catch (SHAMapMissingNode& sn)
    {
        JLOG (j.warn()) <<
            "Ledger with missing nodes in database: " << sn;
        return {};
    }
}

std::shared_ptr<Ledger>
ApplicationImp::loadLedgerFromFile (
    std::string const& name)
{
    try
    {
        std::ifstream ledgerFile (name, std::ios::in);

        if (!ledgerFile)
        {
            JLOG(m_journal.fatal()) <<
                "Unable to open file '" << name << "'";
            return nullptr;
        }

        Json::Reader reader;
        Json::Value jLedger;

        if (!reader.parse (ledgerFile, jLedger))
        {
            JLOG(m_journal.fatal()) <<
                "Unable to parse ledger JSON";
            return nullptr;
        }

        std::reference_wrapper<Json::Value> ledger (jLedger);

//接受已包装的分类帐
         if (ledger.get().isMember  ("result"))
            ledger = ledger.get()["result"];

         if (ledger.get().isMember ("ledger"))
            ledger = ledger.get()["ledger"];

        std::uint32_t seq = 1;
        auto closeTime = timeKeeper().closeTime();
        using namespace std::chrono_literals;
        auto closeTimeResolution = 30s;
        bool closeTimeEstimated = false;
        std::uint64_t totalDrops = 0;

        if (ledger.get().isMember ("accountState"))
        {
            if (ledger.get().isMember (jss::ledger_index))
            {
                seq = ledger.get()[jss::ledger_index].asUInt();
            }

            if (ledger.get().isMember ("close_time"))
            {
                using tp = NetClock::time_point;
                using d = tp::duration;
                closeTime = tp{d{ledger.get()["close_time"].asUInt()}};
            }
            if (ledger.get().isMember ("close_time_resolution"))
            {
                using namespace std::chrono;
                closeTimeResolution = seconds{
                    ledger.get()["close_time_resolution"].asUInt()};
            }
            if (ledger.get().isMember ("close_time_estimated"))
            {
                closeTimeEstimated =
                    ledger.get()["close_time_estimated"].asBool();
            }
            if (ledger.get().isMember ("total_coins"))
            {
                totalDrops =
                    beast::lexicalCastThrow<std::uint64_t>
                        (ledger.get()["total_coins"].asString());
            }

            ledger = ledger.get()["accountState"];
        }

        if (!ledger.get().isArrayOrNull ())
        {
            JLOG(m_journal.fatal())
               << "State nodes must be an array";
            return nullptr;
        }

        auto loadLedger = std::make_shared<Ledger> (
            seq, closeTime, *config_, family());
        loadLedger->setTotalDrops(totalDrops);

        for (Json::UInt index = 0; index < ledger.get().size(); ++index)
        {
            Json::Value& entry = ledger.get()[index];

            if (!entry.isObjectOrNull())
            {
                JLOG(m_journal.fatal())
                    << "Invalid entry in ledger";
                return nullptr;
            }

            uint256 uIndex;

            if (!uIndex.SetHex (entry[jss::index].asString()))
            {
                JLOG(m_journal.fatal())
                    << "Invalid entry in ledger";
                return nullptr;
            }

            entry.removeMember (jss::index);

            STParsedJSONObject stp ("sle", ledger.get()[index]);

            if (!stp.object || uIndex.isZero ())
            {
                JLOG(m_journal.fatal())
                   << "Invalid entry in ledger";
                return nullptr;
            }

//这是唯一一个
//已使用构造函数，请尝试将其移除
            STLedgerEntry sle (*stp.object, uIndex);

            if (! loadLedger->addSLE (sle))
            {
                JLOG(m_journal.fatal())
                   << "Couldn't add serialized ledger: "
                   << uIndex;
                return nullptr;
            }
        }

        loadLedger->stateMap().flushDirty (
            hotACCOUNT_NODE, loadLedger->info().seq);

        loadLedger->setAccepted (closeTime,
            closeTimeResolution, ! closeTimeEstimated,
               *config_);

        return loadLedger;
    }
    catch (std::exception const& x)
    {
        JLOG (m_journal.fatal()) <<
            "Ledger contains invalid data: " << x.what();
        return nullptr;
    }
}

bool ApplicationImp::loadOldLedger (
    std::string const& ledgerID, bool replay, bool isFileName)
{
    try
    {
        std::shared_ptr<Ledger const> loadLedger, replayLedger;

        if (isFileName)
        {
            if (!ledgerID.empty())
                loadLedger = loadLedgerFromFile (ledgerID);
        }
        else if (ledgerID.length () == 64)
        {
            uint256 hash;

            if (hash.SetHex (ledgerID))
            {
                loadLedger = loadByHash (hash, *this);

                if (!loadLedger)
                {
//尝试从后端构建分类帐
                    auto il = std::make_shared <InboundLedger> (
                        *this, hash, 0, InboundLedger::Reason::GENERIC,
                        stopwatch());
                    if (il->checkLocal ())
                        loadLedger = il->getLedger ();
                }
            }
        }
        else if (ledgerID.empty () || boost::beast::detail::iequals(ledgerID, "latest"))
        {
            loadLedger = getLastFullLedger ();
        }
        else
        {
//按顺序假定
            std::uint32_t index;

            if (beast::lexicalCastChecked (index, ledgerID))
                loadLedger = loadByIndex (index, *this);
        }

        if (!loadLedger)
            return false;

        if (replay)
        {
//使用相同的先前分类帐和交易记录重播分类帐结算

//此分类帐保存我们要重播的交易记录
            replayLedger = loadLedger;

            JLOG(m_journal.info()) << "Loading parent ledger";

            loadLedger = loadByHash (replayLedger->info().parentHash, *this);
            if (!loadLedger)
            {
                JLOG(m_journal.info()) << "Loading parent ledger from node store";

//尝试从后端构建分类帐
                auto il = std::make_shared <InboundLedger> (
                    *this, replayLedger->info().parentHash,
                    0, InboundLedger::Reason::GENERIC, stopwatch());

                if (il->checkLocal ())
                    loadLedger = il->getLedger ();

                if (!loadLedger)
                {
                    JLOG(m_journal.fatal()) << "Replay ledger missing/damaged";
                    assert (false);
                    return false;
                }
            }
        }

        JLOG(m_journal.info()) <<
            "Loading ledger " << loadLedger->info().hash <<
            " seq:" << loadLedger->info().seq;

        if (loadLedger->info().accountHash.isZero ())
        {
            JLOG(m_journal.fatal()) << "Ledger is empty.";
            assert (false);
            return false;
        }

        if (!loadLedger->walkLedger (journal ("Ledger")))
        {
            JLOG(m_journal.fatal()) << "Ledger is missing nodes.";
            assert(false);
            return false;
        }

        if (!loadLedger->assertSane (journal ("Ledger")))
        {
            JLOG(m_journal.fatal()) << "Ledger is not sane.";
            assert(false);
            return false;
        }

        m_ledgerMaster->setLedgerRangePresent (
            loadLedger->info().seq,
            loadLedger->info().seq);

        m_ledgerMaster->switchLCL (loadLedger);
        loadLedger->setValidated();
        m_ledgerMaster->setFullLedger(loadLedger, true, false);
        openLedger_.emplace(loadLedger, cachedSLEs_,
            logs_->journal("OpenLedger"));

        if (replay)
        {
//将replayedger中的交易插入我们的未结分类帐
//并构建重播结构
            auto replayData =
                std::make_unique<LedgerReplay>(loadLedger, replayLedger);

            for (auto const& it : replayData->orderedTxns())
            {
                std::shared_ptr<STTx const> const& tx = it.second;
                auto txID = tx->getTransactionID();

                auto s = std::make_shared <Serializer> ();
                tx->add(*s);

                forceValidity(getHashRouter(),
                    txID, Validity::SigGoodOnly);

                openLedger_->modify(
                    [&txID, &s](OpenView& view, beast::Journal j)
                    {
                        view.rawTxInsert (txID, std::move (s), nullptr);
                        return true;
                    });
            }

            m_ledgerMaster->takeReplay (std::move (replayData));
        }
    }
    catch (SHAMapMissingNode&)
    {
        JLOG(m_journal.fatal()) <<
            "Data is missing for selected ledger";
        return false;
    }
    catch (boost::bad_lexical_cast&)
    {
        JLOG(m_journal.fatal())
            << "Ledger specified '" << ledgerID << "' is not valid";
        return false;
    }

    return true;
}

bool ApplicationImp::serverOkay (std::string& reason)
{
    if (! config().ELB_SUPPORT)
        return true;

    if (isShutdown ())
    {
        reason = "Server is shutting down";
        return false;
    }

    if (getOPs ().isNeedNetworkLedger ())
    {
        reason = "Not synchronized with network yet";
        return false;
    }

    if (getOPs ().getOperatingMode () < NetworkOPs::omSYNCING)
    {
        reason = "Not synchronized with network";
        return false;
    }

    if (!getLedgerMaster().isCaughtUp(reason))
        return false;

    if (getFeeTrack ().isLoadedLocal ())
    {
        reason = "Too much load";
        return false;
    }

    if (getOPs ().isAmendmentBlocked ())
    {
        reason = "Server version too old";
        return false;
    }

    return true;
}

beast::Journal
ApplicationImp::journal (std::string const& name)
{
    return logs_->journal (name);
}

//vfalc todo清除此项，因为它只是一个包含单个成员函数定义的文件

static
std::vector<std::string>
getSchema (DatabaseCon& dbc, std::string const& dbName)
{
    std::vector<std::string> schema;
    schema.reserve(32);

    std::string sql = "SELECT sql FROM sqlite_master WHERE tbl_name='";
    sql += dbName;
    sql += "';";

    std::string r;
    soci::statement st = (dbc.getSession ().prepare << sql,
                          soci::into(r));
    st.execute ();
    while (st.fetch ())
    {
        schema.emplace_back (r);
    }

    return schema;
}

static bool schemaHas (
    DatabaseCon& dbc, std::string const& dbName, int line,
    std::string const& content, beast::Journal j)
{
    std::vector<std::string> schema = getSchema (dbc, dbName);

    if (static_cast<int> (schema.size ()) <= line)
    {
        JLOG (j.fatal()) << "Schema for " << dbName << " has too few lines";
        Throw<std::runtime_error> ("bad schema");
    }

    return schema[line].find (content) != std::string::npos;
}

void ApplicationImp::addTxnSeqField ()
{
    if (schemaHas (getTxnDB (), "AccountTransactions", 0, "TxnSeq", m_journal))
        return;

    JLOG (m_journal.warn()) << "Transaction sequence field is missing";

    auto& session = getTxnDB ().getSession ();

    std::vector< std::pair<uint256, int> > txIDs;
    txIDs.reserve (300000);

    JLOG (m_journal.info()) << "Parsing transactions";
    int i = 0;
    uint256 transID;

    boost::optional<std::string> strTransId;
    soci::blob sociTxnMetaBlob(session);
    soci::indicator tmi;
    Blob txnMeta;

    soci::statement st =
            (session.prepare <<
             "SELECT TransID, TxnMeta FROM Transactions;",
             soci::into(strTransId),
             soci::into(sociTxnMetaBlob, tmi));

    st.execute ();
    while (st.fetch ())
    {
        if (soci::i_ok == tmi)
            convert (sociTxnMetaBlob, txnMeta);
        else
            txnMeta.clear ();

        std::string tid = strTransId.value_or("");
        transID.SetHex (tid, true);

        if (txnMeta.size () == 0)
        {
            txIDs.push_back (std::make_pair (transID, -1));
            JLOG (m_journal.info()) << "No metadata for " << transID;
        }
        else
        {
            TxMeta m (transID, 0, txnMeta, journal ("TxMeta"));
            txIDs.push_back (std::make_pair (transID, m.getIndex ()));
        }

        if ((++i % 1000) == 0)
        {
            JLOG (m_journal.info()) << i << " transactions read";
        }
    }

    JLOG (m_journal.info()) << "All " << i << " transactions read";

    soci::transaction tr(session);

    JLOG (m_journal.info()) << "Dropping old index";
    session << "DROP INDEX AcctTxIndex;";

    JLOG (m_journal.info()) << "Altering table";
    session << "ALTER TABLE AccountTransactions ADD COLUMN TxnSeq INTEGER;";

    boost::format fmt ("UPDATE AccountTransactions SET TxnSeq = %d WHERE TransID = '%s';");
    i = 0;
    for (auto& t : txIDs)
    {
        session << boost::str (fmt % t.second % to_string (t.first));

        if ((++i % 1000) == 0)
        {
            JLOG (m_journal.info()) << i << " transactions updated";
        }
    }

    JLOG (m_journal.info()) << "Building new index";
    session << "CREATE INDEX AcctTxIndex ON AccountTransactions(Account, LedgerSeq, TxnSeq, TransID);";

    tr.commit ();
}

void ApplicationImp::addValidationSeqFields ()
{
    if (schemaHas(getLedgerDB(), "Validations", 0, "LedgerSeq", m_journal))
    {
        assert(schemaHas(getLedgerDB(), "Validations", 0, "InitialSeq", m_journal));
        return;
    }

    JLOG(m_journal.warn()) << "Validation sequence fields are missing";
    assert(!schemaHas(getLedgerDB(), "Validations", 0, "InitialSeq", m_journal));

    auto& session = getLedgerDB().getSession();

    soci::transaction tr(session);

    JLOG(m_journal.info()) << "Altering table";
    session << "ALTER TABLE Validations "
        "ADD COLUMN LedgerSeq       BIGINT UNSIGNED;";
    session << "ALTER TABLE Validations "
        "ADD COLUMN InitialSeq      BIGINT UNSIGNED;";

//也创建索引，这样我们就不必
//等待下一次启动，可能需要一段时间。
//这些应该与Ledgerdbinit中的相同
    JLOG(m_journal.info()) << "Building new indexes";
    session << "CREATE INDEX IF NOT EXISTS "
        "ValidationsBySeq ON Validations(LedgerSeq);";
    session << "CREATE INDEX IF NOT EXISTS ValidationsByInitialSeq "
        "ON Validations(InitialSeq, LedgerSeq);";

    tr.commit();
}

bool ApplicationImp::updateTables ()
{
    if (config_->section (ConfigSection::nodeDatabase ()).empty ())
    {
        JLOG (m_journal.fatal()) << "The [node_db] configuration setting has been updated and must be set";
        return false;
    }

//执行任何需要的表更新
    assert (schemaHas (getTxnDB (), "AccountTransactions", 0, "TransID", m_journal));
    assert (!schemaHas (getTxnDB (), "AccountTransactions", 0, "foobar", m_journal));
    addTxnSeqField ();

    if (schemaHas (getTxnDB (), "AccountTransactions", 0, "PRIMARY", m_journal))
    {
        JLOG (m_journal.fatal()) << "AccountTransactions database should not have a primary key";
        return false;
    }

    addValidationSeqFields ();

    if (config_->doImport)
    {
        auto j = logs_->journal("NodeObject");
        NodeStore::DummyScheduler scheduler;
        std::unique_ptr <NodeStore::Database> source =
            NodeStore::Manager::instance().make_Database ("NodeStore.import",
                scheduler, 0, *m_jobQueue,
                config_->section(ConfigSection::importNodeDatabase ()), j);

        JLOG (j.warn())
            << "Node import from '" << source->getName () << "' to '"
            << getNodeStore ().getName () << "'.";

        getNodeStore().import (*source);
    }

    return true;
}

bool ApplicationImp::nodeToShards()
{
    assert(m_overlay);
    assert(!config_->standalone());

    if (config_->section(ConfigSection::shardDatabase()).empty())
    {
        JLOG (m_journal.fatal()) <<
            "The [shard_db] configuration setting must be set";
        return false;
    }
    if (!shardStore_)
    {
        JLOG(m_journal.fatal()) <<
            "Invalid [shard_db] configuration";
        return false;
    }
    shardStore_->import(getNodeStore());
    return true;
}

bool ApplicationImp::validateShards()
{
    assert(m_overlay);
    assert(!config_->standalone());

    if (config_->section(ConfigSection::shardDatabase()).empty())
    {
        JLOG (m_journal.fatal()) <<
            "The [shard_db] configuration setting must be set";
        return false;
    }
    if (!shardStore_)
    {
        JLOG(m_journal.fatal()) <<
            "Invalid [shard_db] configuration";
        return false;
    }
    shardStore_->validate();
    return true;
}

void ApplicationImp::setMaxDisallowedLedger()
{
    boost::optional <LedgerIndex> seq;
    {
        auto db = getLedgerDB().checkoutDb();
        *db << "SELECT MAX(LedgerSeq) FROM Ledgers;", soci::into(seq);
    }
    if (seq)
        maxDisallowedLedger_ = *seq;

    JLOG (m_journal.trace()) << "Max persisted ledger is "
                             << maxDisallowedLedger_;
}


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Application::Application ()
    : beast::PropertyStream::Source ("app")
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<Application>
make_Application (
    std::unique_ptr<Config> config,
    std::unique_ptr<Logs> logs,
    std::unique_ptr<TimeKeeper> timeKeeper)
{
    return std::make_unique<ApplicationImp> (
        std::move(config), std::move(logs),
            std::move(timeKeeper));
}

}
