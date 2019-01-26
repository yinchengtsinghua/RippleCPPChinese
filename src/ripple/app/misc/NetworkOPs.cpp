
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

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/consensus/Consensus.h>
#include <ripple/app/consensus/RCLConsensus.h>
#include <ripple/app/consensus/RCLValidations.h>
#include <ripple/app/ledger/AcceptedLedger.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/consensus/ConsensusParms.h>
#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LocalTxs.h>
#include <ripple/app/ledger/OpenLedger.h>
#include <ripple/app/ledger/OrderBookDB.h>
#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/main/LoadManager.h>
#include <ripple/app/misc/HashRouter.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/app/misc/ValidatorKeys.h>
#include <ripple/app/misc/ValidatorList.h>
#include <ripple/app/misc/impl/AccountTxPaging.h>
#include <ripple/app/tx/apply.h>
#include <ripple/basics/base64.h>
#include <ripple/basics/mulDiv.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/crypto/csprng.h>
#include <ripple/crypto/RFC1751.h>
#include <ripple/json/to_string.h>
#include <ripple/overlay/Cluster.h>
#include <ripple/overlay/Overlay.h>
#include <ripple/overlay/predicates.h>
#include <ripple/protocol/BuildInfo.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/beast/rfc2616.h>
#include <ripple/beast/core/LexicalCast.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/basics/make_lock.h>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/ip/host_name.hpp>
#include <string>
#include <tuple>
#include <utility>

namespace ripple {

class NetworkOPsImp final
    : public NetworkOPs
{
    /*
     *带有输入标志和结果的事务将分批应用。
     **/


    class TransactionStatus
    {
    public:
        std::shared_ptr<Transaction> transaction;
        bool admin;
        bool local;
        FailHard failType;
        bool applied;
        TER result;

        TransactionStatus (
                std::shared_ptr<Transaction> t,
                bool a,
                bool l,
                FailHard f)
            : transaction (t)
            , admin (a)
            , local (l)
            , failType (f)
        {}
    };

    /*
     *事务批处理的同步状态。
     **/

    enum class DispatchState : unsigned char
    {
        none,
        scheduled,
        running,
    };

    static std::array<char const*, 5> const states_;

    /*
     *状态记帐为每个可能的服务器状态记录两个属性：
     *1）在每个状态下花费的时间（以微秒为单位）。这个值是
     *每次状态转换时更新。
     *2）转换到每个状态的次数。
     *
     *此数据可以通过服务器信息进行轮询，并由
     *监控系统类似于带宽、CPU和其他
     *管理基于计数器的指标。
     *
     *状态统计比服务器的定期采样更准确。
     ＊状态。对于周期性采样，很可能是状态转换
     *被忽略，在每个状态下花费的时间的准确性非常粗糙。
     **/

    class StateAccounting
    {
        struct Counters
        {
            explicit Counters() = default;

            std::uint32_t transitions = 0;
            std::chrono::microseconds dur = std::chrono::microseconds (0);
        };

        OperatingMode mode_ = omDISCONNECTED;
        std::array<Counters, 5> counters_;
        mutable std::mutex mutex_;
        std::chrono::system_clock::time_point start_ =
            std::chrono::system_clock::now();
        static std::array<Json::StaticString const, 5> const states_;
        static Json::StaticString const transitions_;
        static Json::StaticString const dur_;

    public:
        explicit StateAccounting ()
        {
            counters_[omDISCONNECTED].transitions = 1;
        }

        /*
         *记录状态转换。更新在上一个中花费的持续时间
         ＊状态。
         *
         *@param om新状态。
         **/

        void mode (OperatingMode om);

        /*
         *JSON格式的状态会计数据。
         *第一成员：国家核算对象。
         *第二个成员：当前状态下的持续时间。
         **/

        using StateCountersJson = std::pair <Json::Value, std::string>;

        /*
         *以JSON格式输出状态计数器。
         *
         *@返回json对象。
         **/

        StateCountersJson json() const;
    };

//！在'server'订阅上发布的服务器费用
    struct ServerFeeSummary
    {
        ServerFeeSummary() = default;

        ServerFeeSummary(std::uint64_t fee,
                         TxQ::Metrics&& escalationMetrics,
                         LoadFeeTrack const & loadFeeTrack);
        bool
        operator !=(ServerFeeSummary const & b) const;

        bool
        operator ==(ServerFeeSummary const & b) const
        {
            return !(*this != b);
        }

        std::uint32_t loadFactorServer = 256;
        std::uint32_t loadBaseServer = 256;
        std::uint64_t baseFee = 10;
        boost::optional<TxQ::Metrics> em = boost::none;
    };


public:
//vfalc要使分类帐管理器成为sharedptr或引用。
//
    NetworkOPsImp (Application& app, NetworkOPs::clock_type& clock,
        bool standalone, std::size_t network_quorum, bool start_valid,
        JobQueue& job_queue, LedgerMaster& ledgerMaster, Stoppable& parent,
        ValidatorKeys const & validatorKeys, boost::asio::io_service& io_svc,
        beast::Journal journal)
        : NetworkOPs (parent)
        , app_ (app)
        , m_clock (clock)
        , m_journal (journal)
        , m_localTX (make_LocalTxs ())
        , mMode (start_valid ? omFULL : omDISCONNECTED)
        , heartbeatTimer_ (io_svc)
        , clusterTimer_ (io_svc)
        , mConsensus (app,
            make_FeeVote(setup_FeeVote (app_.config().section ("voting")),
                                        app_.logs().journal("FeeVote")),
            ledgerMaster,
            *m_localTX,
            app.getInboundTransactions(),
            beast::get_abstract_clock<std::chrono::steady_clock>(),
            validatorKeys,
            app_.logs().journal("LedgerConsensus"))
        , m_ledgerMaster (ledgerMaster)
        , m_job_queue (job_queue)
        , m_standalone (standalone)
        , m_network_quorum (start_valid ? 0 : network_quorum)
    {
    }

    ~NetworkOPsImp() override
    {
//此clear（）是确保此映射中的共享指针
//现在销毁，因为此映射中的对象调用此
//当它们被销毁时
        mRpcSubMap.clear();
    }

public:
    OperatingMode getOperatingMode () const override
    {
        return mMode;
    }
    std::string strOperatingMode () const override;

//
//交易操作。
//

//必须立即完成。
    void submitTransaction (std::shared_ptr<STTx const> const&) override;

    void processTransaction (
        std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, FailHard failType) override;

    /*
     *对于客户直接提交的交易，应用
     *事务并等待此事务完成。
     *
     *@param transaction事务对象。
     *@param bunli限制特权客户端连接是否提交了它。
     *@param failtype失败\事务提交的硬设置。
     **/

    void doTransactionSync (std::shared_ptr<Transaction> transaction,
        bool bUnlimited, FailHard failType);

    /*
     *对于本地连接客户未提交的交易，Fire和
     忘记。添加到批处理中，如果没有批处理，则触发该批处理
     *目前正在应用。
     *
     *@param transaction事务对象
     *@param bunlimited是否提交了特权客户端连接。
     *@param failtype失败\事务提交的硬设置。
     **/

    void doTransactionAsync (std::shared_ptr<Transaction> transaction,
        bool bUnlimited, FailHard failtype);

    /*
     *批量应用交易。继续，直到没有排队。
     **/

    void transactionBatch();

    /*
     *尝试根据结果应用事务和后处理。
     *
     *@param锁，用于保护事务批处理
     **/

    void apply (std::unique_lock<std::mutex>& batchLock);

//
//所有者功能。
//

    Json::Value getOwnerInfo (
        std::shared_ptr<ReadView const> lpLedger,
        AccountID const& account) override;

//
//图书功能。
//

    void getBookPage (std::shared_ptr<ReadView const>& lpLedger,
                      Book const&, AccountID const& uTakerID, const bool bProof,
                      unsigned int iLimit,
                      Json::Value const& jvMarker, Json::Value& jvResult)
            override;

//分类帐建议/关闭功能。
    void processTrustedProposal (
        RCLCxPeerPos proposal,
        std::shared_ptr<protocol::TMProposeSet> set) override;

    bool recvValidation (
        STValidation::ref val, std::string const& source) override;

    std::shared_ptr<SHAMap> getTXMap (uint256 const& hash);
    bool hasTXSet (
        const std::shared_ptr<Peer>& peer, uint256 const& set,
        protocol::TxSetStatus status);

    void mapComplete (
        std::shared_ptr<SHAMap> const& map,
        bool fromAcquire) override;

//网络状态机。

//vfalc todo试图将所有这些私有化，因为它们似乎…私有化
//

//用于“跳跃”情况。
private:
    void switchLastClosedLedger (
        std::shared_ptr<Ledger const> const& newLCL);
    bool checkLastClosedLedger (
        const Overlay::PeerSequence&, uint256& networkClosed);

public:
    bool beginConsensus (uint256 const& networkClosed) override;
    void endConsensus () override;
    void setStandAlone () override
    {
        setMode (omFULL);
    }

    /*打电话开始计时。
        不需要独立模式。
    **/

    void setStateTimer () override;

    void setNeedNetworkLedger () override
    {
        needNetworkLedger_ = true;
    }
    void clearNeedNetworkLedger () override
    {
        needNetworkLedger_ = false;
    }
    bool isNeedNetworkLedger () override
    {
        return needNetworkLedger_;
    }
    bool isFull () override
    {
        return !needNetworkLedger_ && (mMode == omFULL);
    }
    bool isAmendmentBlocked () override
    {
        return amendmentBlocked_;
    }
    void setAmendmentBlocked () override;
    void consensusViewChange () override;

    Json::Value getConsensusInfo () override;
    Json::Value getServerInfo (bool human, bool admin, bool counters) override;
    void clearLedgerFetch () override;
    Json::Value getLedgerFetchInfo () override;
    std::uint32_t acceptLedger (
        boost::optional<std::chrono::milliseconds> consensusDelay) override;
    uint256 getConsensusLCL () override;
    void reportFeeChange () override;

    void updateLocalTx (ReadView const& view) override
    {
        m_localTX->sweep (view);
    }
    std::size_t getLocalTxCount () override
    {
        return m_localTX->size ();
    }

//用于生成SQL查询以获取事务的helper函数。
    std::string transactionsSQL (
        std::string selection, AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger,
        bool descending, std::uint32_t offset, int limit,
        bool binary, bool count, bool bUnlimited);

//客户端信息检索功能。
    using NetworkOPs::AccountTxs;
    AccountTxs getAccountTxs (
        AccountID const& account,
        std::int32_t minLedger, std::int32_t maxLedger, bool descending,
        std::uint32_t offset, int limit, bool bUnlimited) override;

    AccountTxs getTxsAccount (
        AccountID const& account, std::int32_t minLedger,
        std::int32_t maxLedger, bool forward, Json::Value& token, int limit,
        bool bUnlimited) override;

    using NetworkOPs::txnMetaLedgerType;
    using NetworkOPs::MetaTxsList;

    MetaTxsList
    getAccountTxsB (
        AccountID const& account, std::int32_t minLedger,
        std::int32_t maxLedger,  bool descending, std::uint32_t offset,
        int limit, bool bUnlimited) override;

    MetaTxsList
    getTxsAccountB (
        AccountID const& account, std::int32_t minLedger,
        std::int32_t maxLedger,  bool forward, Json::Value& token,
        int limit, bool bUnlimited) override;

//
//监视：发布服务器端。
//
    void pubLedger (
        std::shared_ptr<ReadView const> const& lpAccepted) override;
    void pubProposedTransaction (
        std::shared_ptr<ReadView const> const& lpCurrent,
        std::shared_ptr<STTx const> const& stTxn, TER terResult) override;
    void pubValidation (
        STValidation::ref val) override;

//——————————————————————————————————————————————————————————————
//
//信息子：：源。
//
    void subAccount (
        InfoSub::ref ispListener,
        hash_set<AccountID> const& vnaAccountIDs, bool rt) override;
    void unsubAccount (
        InfoSub::ref ispListener,
        hash_set<AccountID> const& vnaAccountIDs,
        bool rt) override;

//只需从跟踪中删除订阅
//不是从InfoSub。信息子销毁所需
    void unsubAccountInternal (
        std::uint64_t seq,
        hash_set<AccountID> const& vnaAccountIDs,
        bool rt) override;

    bool subLedger (InfoSub::ref ispListener, Json::Value& jvResult) override;
    bool unsubLedger (std::uint64_t uListener) override;

    bool subServer (
        InfoSub::ref ispListener, Json::Value& jvResult, bool admin) override;
    bool unsubServer (std::uint64_t uListener) override;

    bool subBook (InfoSub::ref ispListener, Book const&) override;
    bool unsubBook (std::uint64_t uListener, Book const&) override;

    bool subManifests (InfoSub::ref ispListener) override;
    bool unsubManifests (std::uint64_t uListener) override;
    void pubManifest (Manifest const&) override;

    bool subTransactions (InfoSub::ref ispListener) override;
    bool unsubTransactions (std::uint64_t uListener) override;

    bool subRTTransactions (InfoSub::ref ispListener) override;
    bool unsubRTTransactions (std::uint64_t uListener) override;

    bool subValidations (InfoSub::ref ispListener) override;
    bool unsubValidations (std::uint64_t uListener) override;

    bool subPeerStatus (InfoSub::ref ispListener) override;
    bool unsubPeerStatus (std::uint64_t uListener) override;
    void pubPeerStatus (std::function<Json::Value(void)> const&) override;

    InfoSub::pointer findRpcSub (std::string const& strUrl) override;
    InfoSub::pointer addRpcSub (
        std::string const& strUrl, InfoSub::ref) override;
    bool tryRemoveRpcSub (std::string const& strUrl) override;

//——————————————————————————————————————————————————————————————
//
//可停止的。

    void onStop () override
    {
        mAcquiringLedger.reset();
        {
            boost::system::error_code ec;
            heartbeatTimer_.cancel (ec);
            if (ec)
            {
                JLOG (m_journal.error())
                    << "NetworkOPs: heartbeatTimer cancel error: "
                    << ec.message();
            }

            ec.clear();
            clusterTimer_.cancel (ec);
            if (ec)
            {
                JLOG (m_journal.error())
                    << "NetworkOPs: clusterTimer cancel error: "
                    << ec.message();
            }
        }
//确保在计时器中挂起的所有等待处理程序都已完成
//在我们宣布停止之前。
        using namespace std::chrono_literals;
        waitHandlerCounter_.join("NetworkOPs", 1s, m_journal);
        stopped ();
    }

private:
    void setHeartbeatTimer ();
    void setClusterTimer ();
    void processHeartbeatTimer ();
    void processClusterTimer ();

    void setMode (OperatingMode);

    Json::Value transJson (
        const STTx& stTxn, TER terResult, bool bValidated,
        std::shared_ptr<ReadView const> const& lpCurrent);

    void pubValidatedTransaction (
        std::shared_ptr<ReadView const> const& alAccepted,
        const AcceptedLedgerTx& alTransaction);
    void pubAccountTransaction (
        std::shared_ptr<ReadView const> const& lpCurrent,
        const AcceptedLedgerTx& alTransaction,
        bool isAccepted);

    void pubServer ();

    std::string getHostId (bool forAdmin);

private:
    using SubMapType = hash_map <std::uint64_t, InfoSub::wptr>;
    using SubInfoMapType = hash_map <AccountID, SubMapType>;
    using subRpcMapType = hash_map<std::string, InfoSub::pointer>;

//XXX分成了更多的锁。
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    Application& app_;
    clock_type& m_clock;
    beast::Journal m_journal;

    std::unique_ptr <LocalTxs> m_localTX;

    std::recursive_mutex mSubLock;

    std::atomic<OperatingMode> mMode;

    std::atomic <bool> needNetworkLedger_ {false};
    std::atomic <bool> amendmentBlocked_ {false};

    ClosureCounter<void, boost::system::error_code const&> waitHandlerCounter_;
    boost::asio::steady_timer heartbeatTimer_;
    boost::asio::steady_timer clusterTimer_;

    RCLConsensus mConsensus;

    LedgerMaster& m_ledgerMaster;
    std::shared_ptr<InboundLedger> mAcquiringLedger;

    SubInfoMapType mSubAccount;
    SubInfoMapType mSubRTAccount;

    subRpcMapType mRpcSubMap;

    enum SubTypes
    {
sLedger,                    //接受的分类帐。
sManifests,                 //收到验证程序清单。
sServer,                    //当服务器更改连接状态时。
sTransactions,              //所有接受的交易。
sRTTransactions,            //所有提议和接受的交易。
sValidations,               //收到验证。
sPeerStatus,                //对等状态更改。

sLastEntry = sPeerStatus    //顾名思义，任何新条目都必须
//加在这个上面
    };
    std::array<SubMapType, SubTypes::sLastEntry+1> mStreamMaps;

    ServerFeeSummary mLastFeeSummary;


    JobQueue& m_job_queue;

//是否处于独立模式。
    bool const m_standalone;

//需要将自己视为已连接的节点数。
    std::size_t const m_network_quorum;

//事务批处理。
    std::condition_variable mCond;
    std::mutex mMutex;
    DispatchState mDispatchState = DispatchState::none;
    std::vector <TransactionStatus> mTransactions;

    StateAccounting accounting_ {};
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

static std::array<char const*, 5> const stateNames {{
    "disconnected",
    "connected",
    "syncing",
    "tracking",
    "full"}};

#ifndef __INTELLISENSE__
static_assert (NetworkOPs::omDISCONNECTED == 0, "");
static_assert (NetworkOPs::omCONNECTED == 1, "");
static_assert (NetworkOPs::omSYNCING == 2, "");
static_assert (NetworkOPs::omTRACKING == 3, "");
static_assert (NetworkOPs::omFULL == 4, "");
#endif

std::array<char const*, 5> const NetworkOPsImp::states_ = stateNames;

std::array<Json::StaticString const, 5> const
NetworkOPsImp::StateAccounting::states_ = {{
    Json::StaticString(stateNames[0]),
    Json::StaticString(stateNames[1]),
    Json::StaticString(stateNames[2]),
    Json::StaticString(stateNames[3]),
    Json::StaticString(stateNames[4])}};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
std::string
NetworkOPsImp::getHostId (bool forAdmin)
{
    static std::string const hostname = boost::asio::ip::host_name();

    if (forAdmin)
        return hostname;

//对于非管理员，将节点公钥散列到
//单个rfc1751字：
    static std::string const shroudedHostId =
        [this]()
        {
            auto const& id = app_.nodeIdentity();

            return RFC1751::getWordFromBlob (
                id.first.data (),
                id.first.size ());
        }();

    return shroudedHostId;
}

void NetworkOPsImp::setStateTimer ()
{
    setHeartbeatTimer ();
    setClusterTimer ();
}

void NetworkOPsImp::setHeartbeatTimer ()
{
//仅当WaithandlerCounter_u尚未加入时才启动计时器。
    if (auto optionalCountedHandler = waitHandlerCounter_.wrap (
        [this] (boost::system::error_code const& e)
        {
            if ((e.value() == boost::system::errc::success) &&
                (! m_job_queue.isStopped()))
            {
                m_job_queue.addJob (jtNETOP_TIMER, "NetOPs.heartbeat",
                    [this] (Job&) { processHeartbeatTimer(); });
            }
//如果发生意外错误，请尽可能恢复。
            if (e.value() != boost::system::errc::success &&
                e.value() != boost::asio::error::operation_aborted)
            {
//稍后再试，希望一切顺利。
                JLOG (m_journal.error())
                   << "Heartbeat timer got error '" << e.message()
                   << "'.  Restarting timer.";
                setHeartbeatTimer();
            }
        }))
    {
        heartbeatTimer_.expires_from_now (
            mConsensus.parms().ledgerGRANULARITY);
        heartbeatTimer_.async_wait (std::move (*optionalCountedHandler));
    }
}

void NetworkOPsImp::setClusterTimer ()
{
//仅当WaithandlerCounter_u尚未加入时才启动计时器。
    if (auto optionalCountedHandler = waitHandlerCounter_.wrap (
        [this] (boost::system::error_code const& e)
        {
            if ((e.value() == boost::system::errc::success) &&
                (! m_job_queue.isStopped()))
            {
                m_job_queue.addJob (jtNETOP_CLUSTER, "NetOPs.cluster",
                    [this] (Job&) { processClusterTimer(); });
            }
//如果发生意外错误，请尽可能恢复。
            if (e.value() != boost::system::errc::success &&
                e.value() != boost::asio::error::operation_aborted)
            {
//稍后再试，希望一切顺利。
                JLOG (m_journal.error())
                   << "Cluster timer got error '" << e.message()
                   << "'.  Restarting timer.";
                setClusterTimer();
            }
        }))
    {
        using namespace std::chrono_literals;
        clusterTimer_.expires_from_now (10s);
        clusterTimer_.async_wait (std::move (*optionalCountedHandler));
    }
}

void NetworkOPsImp::processHeartbeatTimer ()
{
    {
        auto lock = make_lock(app_.getMasterMutex());

//vvalco注：这是用于诊断出口碰撞
        LoadManager& mgr (app_.getLoadManager ());
        mgr.resetDeadlockDetector ();

        std::size_t const numPeers = app_.overlay ().size ();

//我们有足够的同龄人吗？如果没有，我们就断开了。
        if (numPeers < m_network_quorum)
        {
            if (mMode != omDISCONNECTED)
            {
                setMode (omDISCONNECTED);
                JLOG(m_journal.warn())
                    << "Node count (" << numPeers << ") "
                    << "has fallen below quorum (" << m_network_quorum << ").";
            }
//在那之前我们不叫MconSensures.Timerentry
//是否有足够的同行为共识提供有意义的投入？
            setHeartbeatTimer ();

            return;
        }

        if (mMode == omDISCONNECTED)
        {
            setMode (omCONNECTED);
            JLOG(m_journal.info())
                << "Node count (" << numPeers << ") is sufficient.";
        }

//检查上次验证的分类帐是否强制在这些分类帐之间进行更改
//国家。
        if (mMode == omSYNCING)
            setMode (omSYNCING);
        else if (mMode == omCONNECTED)
            setMode (omCONNECTED);

    }

    mConsensus.timerEntry (app_.timeKeeper().closeTime());

    setHeartbeatTimer ();
}

void NetworkOPsImp::processClusterTimer ()
{
    using namespace std::chrono_literals;
    bool const update = app_.cluster().update(
        app_.nodeIdentity().first,
        "",
        (m_ledgerMaster.getValidatedLedgerAge() <= 4min)
            ? app_.getFeeTrack().getLocalFee()
            : 0,
        app_.timeKeeper().now());

    if (!update)
    {
        JLOG(m_journal.debug()) << "Too soon to send cluster update";
        setClusterTimer ();
        return;
    }

    protocol::TMCluster cluster;
    app_.cluster().for_each(
        [&cluster](ClusterNode const& node)
        {
            protocol::TMClusterNode& n = *cluster.add_clusternodes();
            n.set_publickey(toBase58 (
                TokenType::NodePublic,
                node.identity()));
            n.set_reporttime(
                node.getReportTime().time_since_epoch().count());
            n.set_nodeload(node.getLoadFee());
            if (!node.name().empty())
                n.set_nodename(node.name());
        });

    Resource::Gossip gossip = app_.getResourceManager().exportConsumers();
    for (auto& item: gossip.items)
    {
        protocol::TMLoadSource& node = *cluster.add_loadsources();
        node.set_name (to_string (item.address));
        node.set_cost (item.balance);
    }
    app_.overlay ().foreach (send_if (
        std::make_shared<Message>(cluster, protocol::mtCLUSTER),
        peer_in_cluster ()));
    setClusterTimer ();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————


std::string NetworkOPsImp::strOperatingMode () const
{
    if (mMode == omFULL)
    {
        auto const mode = mConsensus.mode();
        if (mode != ConsensusMode::wrongLedger)
        {
            if (mode == ConsensusMode::proposing)
                return "proposing";

            if (mConsensus.validating())
                return "validating";
        }
    }

    return states_[mMode];
}

void NetworkOPsImp::submitTransaction (std::shared_ptr<STTx const> const& iTrans)
{
    if (isNeedNetworkLedger ())
    {
//如果我们从未同步过，我们什么都做不了
        return;
    }

//这是一个异步接口
    auto const trans = sterilize(*iTrans);

    auto const txid = trans->getTransactionID ();
    auto const flags = app_.getHashRouter().getFlags(txid);

    if ((flags & SF_BAD) != 0)
    {
        JLOG(m_journal.warn()) << "Submitted transaction cached bad";
        return;
    }

    try
    {
        auto const validity = checkValidity(
            app_.getHashRouter(), *trans,
                m_ledgerMaster.getValidatedRules(),
                    app_.config());

        if (validity.first != Validity::Valid)
        {
            JLOG(m_journal.warn()) <<
                "Submitted transaction invalid: " <<
                validity.second;
            return;
        }
    }
    catch (std::exception const&)
    {
        JLOG(m_journal.warn()) << "Exception checking transaction" << txid;

        return;
    }

    std::string reason;

    auto tx = std::make_shared<Transaction> (
        trans, reason, app_);

    m_job_queue.addJob (
        jtTRANSACTION, "submitTxn",
        [this, tx] (Job&) {
            auto t = tx;
            processTransaction(t, false, false, FailHard::no);
        });
}

void NetworkOPsImp::processTransaction (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, FailHard failType)
{
    auto ev = m_job_queue.makeLoadEvent (jtTXN_PROC, "ProcessTXN");
    auto const newFlags = app_.getHashRouter ().getFlags (transaction->getID ());

    if ((newFlags & SF_BAD) != 0)
    {
//缓存坏
        transaction->setStatus (INVALID);
        transaction->setResult (temBAD_SIGNATURE);
        return;
    }

//注：eahennis-我认为这项检查是多余的，
//但我还不完全确定。
//如果是这样，唯一的开销就是查找hashrouter标志。
    auto const view = m_ledgerMaster.getCurrentLedger();
    auto const validity = checkValidity(
        app_.getHashRouter(),
            *transaction->getSTransaction(),
                view->rules(), app_.config());
    assert(validity.first == Validity::Valid);

//目前不涉及本地检查。
    if (validity.first == Validity::SigBad)
    {
        JLOG(m_journal.info()) << "Transaction has bad signature: " <<
            validity.second;
        transaction->setStatus(INVALID);
        transaction->setResult(temBAD_SIGNATURE);
        app_.getHashRouter().setFlags(transaction->getID(),
            SF_BAD);
        return;
    }

//规范化可以更改指针
    app_.getMasterTransaction ().canonicalize (&transaction);

    if (bLocal)
        doTransactionSync (transaction, bUnlimited, failType);
    else
        doTransactionAsync (transaction, bUnlimited, failType);
}

void NetworkOPsImp::doTransactionAsync (std::shared_ptr<Transaction> transaction,
        bool bUnlimited, FailHard failType)
{
    std::lock_guard<std::mutex> lock (mMutex);

    if (transaction->getApplying())
        return;

    mTransactions.push_back (TransactionStatus (transaction, bUnlimited, false,
        failType));
    transaction->setApplying();

    if (mDispatchState == DispatchState::none)
    {
        if (m_job_queue.addJob (
            jtBATCH, "transactionBatch",
            [this] (Job&) { transactionBatch(); }))
        {
            mDispatchState = DispatchState::scheduled;
        }
    }
}

void NetworkOPsImp::doTransactionSync (std::shared_ptr<Transaction> transaction,
        bool bUnlimited, FailHard failType)
{
    std::unique_lock<std::mutex> lock (mMutex);

    if (! transaction->getApplying())
    {
        mTransactions.push_back (TransactionStatus (transaction, bUnlimited,
            true, failType));
        transaction->setApplying();
    }

    do
    {
        if (mDispatchState == DispatchState::running)
        {
//批处理作业已在运行，请稍候。
            mCond.wait (lock);
        }
        else
        {
            apply (lock);

            if (mTransactions.size())
            {
//需要应用更多的事务，但需要另一个作业。
                if (m_job_queue.addJob (
                    jtBATCH, "transactionBatch",
                    [this] (Job&) { transactionBatch(); }))
                {
                    mDispatchState = DispatchState::scheduled;
                }
            }
        }
    }
    while (transaction->getApplying());
}

void NetworkOPsImp::transactionBatch()
{
    std::unique_lock<std::mutex> lock (mMutex);

    if (mDispatchState == DispatchState::running)
        return;

    while (mTransactions.size())
    {
        apply (lock);
    }
}

void NetworkOPsImp::apply (std::unique_lock<std::mutex>& batchLock)
{
    std::vector<TransactionStatus> submit_held;
    std::vector<TransactionStatus> transactions;
    mTransactions.swap (transactions);
    assert (! transactions.empty());

    assert (mDispatchState != DispatchState::running);
    mDispatchState = DispatchState::running;

    batchLock.unlock();

    {
        auto lock = make_lock(app_.getMasterMutex());
        bool changed = false;
        {
            std::lock_guard <std::recursive_mutex> lock (
                m_ledgerMaster.peekMutex());

            app_.openLedger().modify(
                [&](OpenView& view, beast::Journal j)
            {
                for (TransactionStatus& e : transactions)
                {
//我们在添加到批之前检查
                    ApplyFlags flags = tapNONE;
                    if (e.admin)
                        flags = flags | tapUNLIMITED;

                    auto const result = app_.getTxQ().apply(
                        app_, view, e.transaction->getSTransaction(),
                        flags, j);
                    e.result = result.first;
                    e.applied = result.second;
                    changed = changed || result.second;
                }
                return changed;
            });
        }
        if (changed)
            reportFeeChange();

        auto newOL = app_.openLedger().current();
        for (TransactionStatus& e : transactions)
        {
            if (e.applied)
            {
                pubProposedTransaction (newOL,
                    e.transaction->getSTransaction(), e.result);
            }

            e.transaction->setResult (e.result);

            if (isTemMalformed (e.result))
                app_.getHashRouter().setFlags (e.transaction->getID(), SF_BAD);

    #ifdef BEAST_DEBUG
            if (e.result != tesSUCCESS)
            {
                std::string token, human;

                if (transResultInfo (e.result, token, human))
                {
                    JLOG(m_journal.info()) << "TransactionResult: "
                            << token << ": " << human;
                }
            }
    #endif

            bool addLocal = e.local;

            if (e.result == tesSUCCESS)
            {
                JLOG(m_journal.debug())
                    << "Transaction is now included in open ledger";
                e.transaction->setStatus (INCLUDED);

                auto txCur = e.transaction->getSTransaction();
                for (auto const& tx : m_ledgerMaster.pruneHeldTransactions(
                    txCur->getAccountID(sfAccount), txCur->getSequence() + 1))
                {
                    std::string reason;
                    auto const trans = sterilize(*tx);
                    auto t = std::make_shared<Transaction>(
                        trans, reason, app_);
                    submit_held.emplace_back(
                        t, false, false, FailHard::no);
                    t->setApplying();
                }
            }
            else if (e.result == tefPAST_SEQ)
            {
//重复或冲突
                JLOG(m_journal.info()) << "Transaction is obsolete";
                e.transaction->setStatus (OBSOLETE);
            }
            else if (e.result == terQUEUED)
            {
                JLOG(m_journal.debug()) << "Transaction is likely to claim a " <<
                    "fee, but is queued until fee drops";
                e.transaction->setStatus(HELD);
//添加到保留的事务，因为它可以
//被踢出队列，这将尝试
//把它放回去。
                m_ledgerMaster.addHeldTransaction(e.transaction);
            }
            else if (isTerRetry (e.result))
            {
                if (e.failType == FailHard::yes)
                {
                    addLocal = false;
                }
                else
                {
//应保留交易记录
                    JLOG(m_journal.debug())
                        << "Transaction should be held: " << e.result;
                    e.transaction->setStatus (HELD);
                    m_ledgerMaster.addHeldTransaction (e.transaction);
                }
            }
            else
            {
                JLOG(m_journal.debug())
                    << "Status other than success " << e.result;
                e.transaction->setStatus (INVALID);
            }

            if (addLocal)
            {
                m_localTX->push_back (
                    m_ledgerMaster.getCurrentLedgerIndex(),
                    e.transaction->getSTransaction());
            }

            if (e.applied || ((mMode != omFULL) &&
                (e.failType != FailHard::yes) && e.local) ||
                    (e.result == terQUEUED))
            {
                auto const toSkip = app_.getHashRouter().shouldRelay(
                    e.transaction->getID());

                if (toSkip)
                {
                    protocol::TMTransaction tx;
                    Serializer s;

                    e.transaction->getSTransaction()->add (s);
                    tx.set_rawtransaction (s.data(), s.size());
                    tx.set_status (protocol::tsCURRENT);
                    tx.set_receivetimestamp (app_.timeKeeper().now().time_since_epoch().count());
                    tx.set_deferred(e.result == terQUEUED);
//这个应该是我们收到的时候
                    app_.overlay().foreach (send_if_not (
                        std::make_shared<Message> (tx, protocol::mtTRANSACTION),
                        peer_in_set(*toSkip)));
                }
            }
        }
    }

    batchLock.lock();

    for (TransactionStatus& e : transactions)
        e.transaction->clearApplying();

    if (! submit_held.empty())
    {
        if (mTransactions.empty())
            mTransactions.swap(submit_held);
        else
            for (auto& e : submit_held)
                mTransactions.push_back(std::move(e));
    }

    mCond.notify_all();

    mDispatchState = DispatchState::none;
}

//
//所有者函数
//

Json::Value NetworkOPsImp::getOwnerInfo (
    std::shared_ptr<ReadView const> lpLedger, AccountID const& account)
{
    Json::Value jvObjects (Json::objectValue);
    auto uRootIndex = getOwnerDirIndex (account);
    auto sleNode = lpLedger->read (keylet::page (uRootIndex));
    if (sleNode)
    {
        std::uint64_t  uNodeDir;

        do
        {
            for (auto const& uDirEntry : sleNode->getFieldV256 (sfIndexes))
            {
                auto sleCur = lpLedger->read (keylet::child (uDirEntry));
                assert (sleCur);

                switch (sleCur->getType ())
                {
                case ltOFFER:
                    if (!jvObjects.isMember (jss::offers))
                        jvObjects[jss::offers] = Json::Value (Json::arrayValue);

                    jvObjects[jss::offers].append (sleCur->getJson (0));
                    break;

                case ltRIPPLE_STATE:
                    if (!jvObjects.isMember (jss::ripple_lines))
                    {
                        jvObjects[jss::ripple_lines] =
                                Json::Value (Json::arrayValue);
                    }

                    jvObjects[jss::ripple_lines].append (sleCur->getJson (0));
                    break;

                case ltACCOUNT_ROOT:
                case ltDIR_NODE:
                default:
                    assert (false);
                    break;
                }
            }

            uNodeDir = sleNode->getFieldU64 (sfIndexNext);

            if (uNodeDir)
            {
                sleNode = lpLedger->read (keylet::page (uRootIndex, uNodeDir));
                assert (sleNode);
            }
        }
        while (uNodeDir);
    }

    return jvObjects;
}

//
//其他
//

void NetworkOPsImp::setAmendmentBlocked ()
{
    amendmentBlocked_ = true;
    setMode (omTRACKING);
}

bool NetworkOPsImp::checkLastClosedLedger (
    const Overlay::PeerSequence& peerList, uint256& networkClosed)
{
//如果存在*异常*分类帐问题，则返回“真”，在
//跟踪模式应返回false。我们有足够的证明吗
//我们最后一次结帐？还是足够的节点同意？我们没有吗
//有更好的分类帐吗？如果是的话，我们要么跟踪要么满了。

    JLOG(m_journal.trace()) << "NetworkOPsImp::checkLastClosedLedger";

    auto const ourClosed = m_ledgerMaster.getClosedLedger ();

    if (!ourClosed)
        return false;

    uint256 closedLedger = ourClosed->info().hash;
    uint256 prevClosedLedger = ourClosed->info().parentHash;
    JLOG(m_journal.trace()) << "OurClosed:  " << closedLedger;
    JLOG(m_journal.trace()) << "PrevClosed: " << prevClosedLedger;

//————————————————————————————————————————————————————————————————
//确定首选上次关闭的分类帐

    auto & validations = app_.getValidations();
    JLOG(m_journal.debug())
        << "ValidationTrie " << Json::Compact(validations.getJsonTrie());

//如果不存在可信验证，将依赖对等LCL
    hash_map<uint256, std::uint32_t> peerCounts;
    peerCounts[closedLedger] = 0;
    if (mMode >= omTRACKING)
        peerCounts[closedLedger]++;

    for (auto& peer : peerList)
    {
        uint256 peerLedger = peer->getClosedLedgerHash();

        if (peerLedger.isNonZero())
            ++peerCounts[peerLedger];
    }

    for(auto const & it: peerCounts)
        JLOG(m_journal.debug()) << "L: " << it.first << " n=" << it.second;

    uint256 preferredLCL = validations.getPreferredLCL(
        RCLValidatedLedger{ourClosed, validations.adaptor().journal()},
        m_ledgerMaster.getValidLedgerIndex(),
        peerCounts);

    bool switchLedgers = preferredLCL != closedLedger;
    if(switchLedgers)
        closedLedger = preferredLCL;
//————————————————————————————————————————————————————————————————
    if (switchLedgers && (closedLedger == prevClosedLedger))
    {
//不要切换到我们自己以前的分类帐
        JLOG(m_journal.info()) << "We won't switch to our own previous ledger";
        networkClosed = ourClosed->info().hash;
        switchLedgers = false;
    }
    else
        networkClosed = closedLedger;

    if (!switchLedgers)
        return false;

    auto consensus = m_ledgerMaster.getLedgerByHash(closedLedger);

    if (!consensus)
        consensus = app_.getInboundLedgers().acquire(
            closedLedger, 0, InboundLedger::Reason::CONSENSUS);

    if (consensus &&
        (!m_ledgerMaster.canBeCurrent (consensus) ||
            !m_ledgerMaster.isCompatible(
                *consensus, m_journal.debug(), "Not switching")))
    {
//不要切换到不在已验证链上的分类帐
//或关闭时间或顺序无效
        networkClosed = ourClosed->info().hash;
        return false;
    }

    JLOG(m_journal.warn()) << "We are not running on the consensus ledger";
    JLOG(m_journal.info()) << "Our LCL: " << getJson(*ourClosed);
    JLOG(m_journal.info()) << "Net LCL " << closedLedger;

    if ((mMode == omTRACKING) || (mMode == omFULL))
        setMode(omCONNECTED);

    if (consensus)
    {
//Fixme：如果这倒回分类帐序列，或具有相同的序列
//顺序，我们应该更新任何存储事务的状态
//在失效的分类帐中。
        switchLastClosedLedger(consensus);
    }

    return true;
}

void NetworkOPsImp::switchLastClosedLedger (
    std::shared_ptr<Ledger const> const& newLCL)
{
//将newlcl设置为上次关闭的分类帐--这是异常代码
    JLOG(m_journal.error()) <<
        "JUMP last closed ledger to " << newLCL->info().hash;

    clearNeedNetworkLedger ();

//更新费用计算。
    app_.getTxQ().processClosedLedger(app_, *newLCL, true);

//调用方必须拥有主锁
    {
//将旧的未结分类帐中的TX应用于新的
//开式分类帐然后应用本地Tx。

        auto retries = m_localTX->getTxSet();
        auto const lastVal =
            app_.getLedgerMaster().getValidatedLedger();
        boost::optional<Rules> rules;
        if (lastVal)
            rules.emplace(*lastVal, app_.config().features);
        else
            rules.emplace(app_.config().features);
        app_.openLedger().accept(app_, *rules,
            newLCL, OrderedTxs({}), false, retries,
                tapNONE, "jump",
                    [&](OpenView& view, beast::Journal j)
                    {
//在分类帐中填入队列中的交易记录。
                        return app_.getTxQ().accept(app_, view);
                    });
    }

    m_ledgerMaster.switchLCL (newLCL);

    protocol::TMStatusChange s;
    s.set_newevent (protocol::neSWITCHED_LEDGER);
    s.set_ledgerseq (newLCL->info().seq);
    s.set_networktime (app_.timeKeeper().now().time_since_epoch().count());
    s.set_ledgerhashprevious (
        newLCL->info().parentHash.begin (),
        newLCL->info().parentHash.size ());
    s.set_ledgerhash (
        newLCL->info().hash.begin (),
        newLCL->info().hash.size ());

    app_.overlay ().foreach (send_always (
        std::make_shared<Message> (s, protocol::mtSTATUS_CHANGE)));
}

bool NetworkOPsImp::beginConsensus (uint256 const& networkClosed)
{
    assert (networkClosed.isNonZero ());

    auto closingInfo = m_ledgerMaster.getCurrentLedger()->info();

    JLOG(m_journal.info()) <<
        "Consensus time for #" << closingInfo.seq <<
        " with LCL " << closingInfo.parentHash;

    auto prevLedger = m_ledgerMaster.getLedgerByHash(
        closingInfo.parentHash);

    if(! prevLedger)
    {
//除非我们跳过分类账，否则不会发生这种情况。
        if (mMode == omFULL)
        {
            JLOG(m_journal.warn()) << "Don't have LCL, going to tracking";
            setMode (omTRACKING);
        }

        return false;
    }

    assert (prevLedger->info().hash == closingInfo.parentHash);
    assert (closingInfo.parentHash ==
            m_ledgerMaster.getClosedLedger()->info().hash);

    TrustChanges const changes = app_.validators().updateTrusted(
        app_.getValidations().getCurrentNodeIDs());

    if (!changes.added.empty() || !changes.removed.empty())
        app_.getValidations().trustChanged(changes.added, changes.removed);

    mConsensus.startRound(
        app_.timeKeeper().closeTime(),
        networkClosed,
        prevLedger,
        changes.removed);

    JLOG(m_journal.debug()) << "Initiating consensus engine";
    return true;
}

uint256 NetworkOPsImp::getConsensusLCL ()
{
    return mConsensus.prevLedgerID ();
}

void NetworkOPsImp::processTrustedProposal (
    RCLCxPeerPos peerPos,
    std::shared_ptr<protocol::TMProposeSet> set)
{
    if (mConsensus.peerProposal(
            app_.timeKeeper().closeTime(), peerPos))
    {
        app_.overlay().relay(*set, peerPos.suppressionID());
    }
    else
        JLOG(m_journal.info()) << "Not relaying trusted proposal";
}

void
NetworkOPsImp::mapComplete (
    std::shared_ptr<SHAMap> const& map, bool fromAcquire)
{
//我们现在有一个附加的事务集
//在协商一致过程中在本地创建
//或者从同伴那里获得

//通知同行我们有这套
    protocol::TMHaveTransactionSet msg;
    msg.set_hash (map->getHash().as_uint256().begin(), 256 / 8);
    msg.set_status (protocol::tsHAVE);
    app_.overlay().foreach (send_always (
        std::make_shared<Message> (
            msg, protocol::mtHAVE_SET)));

//我们获得它是因为共识要求我们
    if (fromAcquire)
        mConsensus.gotTxSet (
            app_.timeKeeper().closeTime(),
            RCLTxSet{map});
}

void NetworkOPsImp::endConsensus ()
{
    uint256 deadLedger = m_ledgerMaster.getClosedLedger ()->info().parentHash;

    for (auto const& it : app_.overlay ().getActivePeers ())
    {
        if (it && (it->getClosedLedgerHash () == deadLedger))
        {
            JLOG(m_journal.trace()) << "Killing obsolete peer status";
            it->cycleStatus ();
        }
    }

    uint256 networkClosed;
    bool ledgerChange = checkLastClosedLedger (
        app_.overlay ().getActivePeers (), networkClosed);

    if (networkClosed.isZero ())
        return;

//写信给我：除非我们正在全力以赴，在达成共识的过程中，
//我们必须计算有多少节点共享LCL，有多少节点不一致
//我们的LCL，以及我们的LCL有多少个验证。我们还要查一下
//确保没有更新的LCL的时间安排。我们需要这个
//接下来三个测试的信息。

    if (((mMode == omCONNECTED) || (mMode == omSYNCING)) && !ledgerChange)
    {
//计算与我们和unl节点一致且
//我们对LCL的验证。如果分类帐足够好，请转到
//OMTracking-TODO（目标跟踪）
        if (!needNetworkLedger_)
            setMode (omTRACKING);
    }

    if (((mMode == omCONNECTED) || (mMode == omTRACKING)) && !ledgerChange)
    {
//检查分类帐是否足够好，可以转到omfull。
//注意：如果我们没有上一个分类账，请不要转到omfull。
//检查分类帐是否足以转到omconnected--todo
        auto current = m_ledgerMaster.getCurrentLedger();
        if (app_.timeKeeper().now() <
            (current->info().parentCloseTime + 2* current->info().closeTimeResolution))
        {
            setMode (omFULL);
        }
    }

    beginConsensus (networkClosed);
}

void NetworkOPsImp::consensusViewChange ()
{
    if ((mMode == omFULL) || (mMode == omTRACKING))
        setMode (omCONNECTED);
}

void NetworkOPsImp::pubManifest (Manifest const& mo)
{
//vfalc考虑std:：shared\u mutex
    ScopedLockType sl (mSubLock);

    if (!mStreamMaps[sManifests].empty ())
    {
        Json::Value jvObj (Json::objectValue);

        jvObj [jss::type]             = "manifestReceived";
        jvObj [jss::master_key]       = toBase58(
            TokenType::NodePublic, mo.masterKey);
        jvObj [jss::signing_key]      = toBase58(
            TokenType::NodePublic, mo.signingKey);
        jvObj [jss::seq]              = Json::UInt (mo.sequence);
        jvObj [jss::signature]        = strHex (mo.getSignature ());
        jvObj [jss::master_signature] = strHex (mo.getMasterSignature ());

        for (auto i = mStreamMaps[sManifests].begin ();
            i != mStreamMaps[sManifests].end (); )
        {
            if (auto p = i->second.lock())
            {
                p->send (jvObj, true);
                ++i;
            }
            else
            {
                i = mStreamMaps[sManifests].erase (i);
            }
        }
    }
}

NetworkOPsImp::ServerFeeSummary::ServerFeeSummary(
        std::uint64_t fee,
        TxQ::Metrics&& escalationMetrics,
        LoadFeeTrack const & loadFeeTrack)
    : loadFactorServer{loadFeeTrack.getLoadFactor()}
    , loadBaseServer{loadFeeTrack.getLoadBase()}
    , baseFee{fee}
    , em{std::move(escalationMetrics)}
{

}


bool
NetworkOPsImp::ServerFeeSummary::operator !=(NetworkOPsImp::ServerFeeSummary const & b) const
{
    if(loadFactorServer != b.loadFactorServer ||
       loadBaseServer != b.loadBaseServer ||
       baseFee != b.baseFee ||
       em.is_initialized() != b.em.is_initialized())
            return true;

    if(em && b.em)
    {
        return (em->minProcessingFeeLevel != b.em->minProcessingFeeLevel ||
                em->openLedgerFeeLevel != b.em->openLedgerFeeLevel ||
                em->referenceFeeLevel != b.em->referenceFeeLevel);
    }

    return false;
}

void NetworkOPsImp::pubServer ()
{
//vfalc todo不要在要发送的呼叫之间保持锁定…复制
//在保持锁的同时将其列出到本地数组中，然后释放
//锁定并呼叫所有人。
//
    ScopedLockType sl (mSubLock);

    if (!mStreamMaps[sServer].empty ())
    {
        Json::Value jvObj (Json::objectValue);

        ServerFeeSummary f{app_.openLedger().current()->fees().base,
            app_.getTxQ().getMetrics(*app_.openLedger().current()),
            app_.getFeeTrack()};

//由于JSON限制，需要将上限设置为uint64到uint32
        auto clamp = [](std::uint64_t v)
        {
            constexpr std::uint64_t max32 =
                std::numeric_limits<std::uint32_t>::max();

            return static_cast<std::uint32_t>(std::min(max32, v));
        };


        jvObj [jss::type] = "serverStatus";
        jvObj [jss::server_status] = strOperatingMode ();
        jvObj [jss::load_base] = f.loadBaseServer;
        jvObj [jss::load_factor_server] = f.loadFactorServer;
        jvObj [jss::base_fee] = clamp(f.baseFee);

        if(f.em)
        {
            auto const loadFactor =
                std::max(safe_cast<std::uint64_t>(f.loadFactorServer),
                    mulDiv(f.em->openLedgerFeeLevel, f.loadBaseServer,
                        f.em->referenceFeeLevel).second);

            jvObj [jss::load_factor]   = clamp(loadFactor);
            jvObj [jss::load_factor_fee_escalation] = clamp(f.em->openLedgerFeeLevel);
            jvObj [jss::load_factor_fee_queue] = clamp(f.em->minProcessingFeeLevel);
            jvObj [jss::load_factor_fee_reference]
                = clamp(f.em->referenceFeeLevel);

        }
        else
            jvObj [jss::load_factor] = f.loadFactorServer;

        mLastFeeSummary = f;

        for (auto i = mStreamMaps[sServer].begin ();
            i != mStreamMaps[sServer].end (); )
        {
            InfoSub::pointer p = i->second.lock ();

//vfalc todo研究使用线程队列和
//使用
//发送JSON数据。
            if (p)
            {
                p->send (jvObj, true);
                ++i;
            }
            else
            {
                i = mStreamMaps[sServer].erase (i);
            }
        }
    }
}


void NetworkOPsImp::pubValidation (STValidation::ref val)
{
//vfalc考虑std:：shared\u mutex
    ScopedLockType sl (mSubLock);

    if (!mStreamMaps[sValidations].empty ())
    {
        Json::Value jvObj (Json::objectValue);

        jvObj [jss::type]                  = "validationReceived";
        jvObj [jss::validation_public_key] = toBase58(
            TokenType::NodePublic,
            val->getSignerPublic());
        jvObj [jss::ledger_hash]           = to_string (val->getLedgerHash ());
        jvObj [jss::signature]             = strHex (val->getSignature ());
        jvObj [jss::full]                  = val->isFull();
        jvObj [jss::flags]                 = val->getFlags();
        jvObj [jss::signing_time]          = *(*val)[~sfSigningTime];

        if (auto const seq = (*val)[~sfLedgerSequence])
            jvObj [jss::ledger_index] = to_string (*seq);

        if (val->isFieldPresent (sfAmendments))
        {
            jvObj[jss::amendments] = Json::Value (Json::arrayValue);
            for (auto const& amendment : val->getFieldV256(sfAmendments))
                jvObj [jss::amendments].append (to_string (amendment));
        }

        if (auto const closeTime = (*val)[~sfCloseTime])
            jvObj [jss::close_time] = *closeTime;

        if (auto const loadFee = (*val)[~sfLoadFee])
            jvObj [jss::load_fee] = *loadFee;

        if (auto const baseFee = (*val)[~sfBaseFee])
            jvObj [jss::base_fee] = static_cast<double> (*baseFee);

        if (auto const reserveBase = (*val)[~sfReserveBase])
            jvObj [jss::reserve_base] = *reserveBase;

        if (auto const reserveInc = (*val)[~sfReserveIncrement])
            jvObj [jss::reserve_inc] = *reserveInc;

        for (auto i = mStreamMaps[sValidations].begin ();
            i != mStreamMaps[sValidations].end (); )
        {
            if (auto p = i->second.lock())
            {
                p->send (jvObj, true);
                ++i;
            }
            else
            {
                i = mStreamMaps[sValidations].erase (i);
            }
        }
    }
}

void NetworkOPsImp::pubPeerStatus (
    std::function<Json::Value(void)> const& func)
{
    ScopedLockType sl (mSubLock);

    if (!mStreamMaps[sPeerStatus].empty ())
    {
        Json::Value jvObj (func());

        jvObj [jss::type]                  = "peerStatusChange";

        for (auto i = mStreamMaps[sPeerStatus].begin ();
            i != mStreamMaps[sPeerStatus].end (); )
        {
            InfoSub::pointer p = i->second.lock ();

            if (p)
            {
                p->send (jvObj, true);
                ++i;
            }
            else
            {
                i = mStreamMaps[sPeerStatus].erase (i);
            }
        }
    }
}

void NetworkOPsImp::setMode (OperatingMode om)
{
    using namespace std::chrono_literals;
    if (om == omCONNECTED)
    {
        if (app_.getLedgerMaster ().getValidatedLedgerAge () < 1min)
            om = omSYNCING;
    }
    else if (om == omSYNCING)
    {
        if (app_.getLedgerMaster ().getValidatedLedgerAge () >= 1min)
            om = omCONNECTED;
    }

    if ((om > omTRACKING) && amendmentBlocked_)
        om = omTRACKING;

    if (mMode == om)
        return;

    mMode = om;

    accounting_.mode (om);

    JLOG(m_journal.info()) << "STATE->" << strOperatingMode ();
    pubServer ();
}


std::string
NetworkOPsImp::transactionsSQL (
    std::string selection, AccountID const& account,
    std::int32_t minLedger, std::int32_t maxLedger, bool descending,
    std::uint32_t offset, int limit,
    bool binary, bool count, bool bUnlimited)
{
    std::uint32_t NONBINARY_PAGE_LENGTH = 200;
    std::uint32_t BINARY_PAGE_LENGTH = 500;

    std::uint32_t numberOfResults;

    if (count)
    {
        numberOfResults = 1000000000;
    }
    else if (limit < 0)
    {
        numberOfResults = binary ? BINARY_PAGE_LENGTH : NONBINARY_PAGE_LENGTH;
    }
    else if (!bUnlimited)
    {
        numberOfResults = std::min (
            binary ? BINARY_PAGE_LENGTH : NONBINARY_PAGE_LENGTH,
            static_cast<std::uint32_t> (limit));
    }
    else
    {
        numberOfResults = limit;
    }

    std::string maxClause = "";
    std::string minClause = "";

    if (maxLedger != -1)
    {
        maxClause = boost::str (boost::format (
            "AND AccountTransactions.LedgerSeq <= '%u'") % maxLedger);
    }

    if (minLedger != -1)
    {
        minClause = boost::str (boost::format (
            "AND AccountTransactions.LedgerSeq >= '%u'") % minLedger);
    }

    std::string sql;

    if (count)
        sql =
            boost::str (boost::format (
                "SELECT %s FROM AccountTransactions "
                "WHERE Account = '%s' %s %s LIMIT %u, %u;")
            % selection
            % app_.accountIDCache().toBase58(account)
            % maxClause
            % minClause
            % beast::lexicalCastThrow <std::string> (offset)
            % beast::lexicalCastThrow <std::string> (numberOfResults)
        );
    else
        sql =
            boost::str (boost::format (
                "SELECT %s FROM "
                "AccountTransactions INNER JOIN Transactions "
                "ON Transactions.TransID = AccountTransactions.TransID "
                "WHERE Account = '%s' %s %s "
                "ORDER BY AccountTransactions.LedgerSeq %s, "
                "AccountTransactions.TxnSeq %s, AccountTransactions.TransID %s "
                "LIMIT %u, %u;")
                    % selection
                    % app_.accountIDCache().toBase58(account)
                    % maxClause
                    % minClause
                    % (descending ? "DESC" : "ASC")
                    % (descending ? "DESC" : "ASC")
                    % (descending ? "DESC" : "ASC")
                    % beast::lexicalCastThrow <std::string> (offset)
                    % beast::lexicalCastThrow <std::string> (numberOfResults)
                   );
    JLOG(m_journal.trace()) << "txSQL query: " << sql;
    return sql;
}

NetworkOPs::AccountTxs NetworkOPsImp::getAccountTxs (
    AccountID const& account,
    std::int32_t minLedger, std::int32_t maxLedger, bool descending,
    std::uint32_t offset, int limit, bool bUnlimited)
{
//可以在没有锁的情况下调用
    AccountTxs ret;

    std::string sql = transactionsSQL (
        "AccountTransactions.LedgerSeq,Status,RawTxn,TxnMeta", account,
        minLedger, maxLedger, descending, offset, limit, false, false,
        bUnlimited);

    {
        auto db = app_.getTxnDB ().checkoutDb ();

        boost::optional<std::uint64_t> ledgerSeq;
        boost::optional<std::string> status;
        soci::blob sociTxnBlob (*db), sociTxnMetaBlob (*db);
        soci::indicator rti, tmi;
        Blob rawTxn, txnMeta;

        soci::statement st =
                (db->prepare << sql,
                 soci::into(ledgerSeq),
                 soci::into(status),
                 soci::into(sociTxnBlob, rti),
                 soci::into(sociTxnMetaBlob, tmi));

        st.execute ();
        while (st.fetch ())
        {
            if (soci::i_ok == rti)
                convert(sociTxnBlob, rawTxn);
            else
                rawTxn.clear ();

            if (soci::i_ok == tmi)
                convert (sociTxnMetaBlob, txnMeta);
            else
                txnMeta.clear ();

            auto txn = Transaction::transactionFromSQL (
                ledgerSeq, status, rawTxn, app_);

            if (txnMeta.empty ())
{ //解决可能导致元数据丢失的错误
                auto const seq = rangeCheckedCast<std::uint32_t>(
                    ledgerSeq.value_or (0));

                JLOG(m_journal.warn()) <<
                    "Recovering ledger " << seq <<
                    ", txn " << txn->getID();

                if (auto l = m_ledgerMaster.getLedgerBySeq(seq))
                    pendSaveValidated(app_, l, false, false);
            }

            if (txn)
                ret.emplace_back (txn, std::make_shared<TxMeta> (
                    txn->getID (), txn->getLedger (), txnMeta,
                        app_.journal("TxMeta")));
        }
    }

    return ret;
}

std::vector<NetworkOPsImp::txnMetaLedgerType> NetworkOPsImp::getAccountTxsB (
    AccountID const& account,
    std::int32_t minLedger, std::int32_t maxLedger, bool descending,
    std::uint32_t offset, int limit, bool bUnlimited)
{
//可以在没有锁的情况下调用
    std::vector<txnMetaLedgerType> ret;

    std::string sql = transactionsSQL (
        "AccountTransactions.LedgerSeq,Status,RawTxn,TxnMeta", account,
        /*分类帐，最大分类帐，降序，偏移量，限制，真/*二进制*/，假，
        有限公司；

    {
        auto-db=app_uu.gettxndb（）.checkoutb（）；

        boost:：optional<std:：uint64_t>ledgerseq；
        boost:：optional<std:：string>status；
        soci：：blob socitxnblob（*db），socitxnmetablob（*db）；
        社会：指标RTI、TMI；

        soci：：语句st=
                （db->prepare<<sql，
                 入（LedgerSeq）
                 进入（状态）
                 入（socitxnblob，rti）
                 soci：：into（socitxnmetabob，tmi））；

        执行（）；
        while（st.fetch（））
        {
            斑点；
            如果（soci:：i_ok==rti）
                转换（socitxnblob，rawtxn）；
            BLB TXNMETA；
            如果（soci:：i_ok==tmi）
                转换（socitxnmetabob、txnmeta）；

            自动常数序列=
                rangecheckedcast<std:：uint32_t>（ledgerseq.value_or（0））；

            返回模板（
                strhex（rawtxn），strhex（txnmeta），seq）；
        }
    }

    返回RET；
}

网络opsimp:：accounttxs
networkopsimp:：gettxsaccount（）。
    accountID const&account，std:：int32_t minledger，
    std:：int32_t maxledger，bool forward，json:：value&token，
    国际限额，布尔本有限公司）
{
    静态标准：：uint32_t const page_length（200）；

    应用程序&app=app_uu；
    networkopsimp:：accounttxs ret；

    自动绑定=[&ret，&app]（
        标准：uint32分类帐索引，
        std：：字符串常量和状态，
        Blob常量和rawtxn，
        Blob常量和rawmeta）
    {
        convertblobstoxresult（
            ret，分类帐索引，状态，rawtxn，rawmeta，app）；
    }；

    accounttxpage（app_.gettxndb（），app_.accountidcache（），
        std：：bind（saveledgerasync，std：：ref（app_uu）），
            标准：：占位符：：1），绑定，帐户，minledger，
                MaxLedger、Forward、Token、Limit、BunLimited、
                    页长度；

    返回RET；
}

网络opsimp:：metatxslist
networkopsimp:：gettxsaccountb（
    accountID const&account，std:：int32_t minledger，
    std:：int32_t maxledger，bool forward，json:：value&token，
    国际限额，布尔本有限公司）
{
    静态常量std:：uint32_t page_length（500）；

    metatxslist ret；

    自动绑定=[&ret]（
        标准：uint32_t ledgerindex，
        std：：字符串常量和状态，
        Blob常量和rawtxn，
        Blob常量和rawmeta）
    {
        返回模板（strhex（rawtxn）、strhex（rawmeta）、ledgerindex）；
    }；

    accounttxpage（app_.gettxndb（），app_.accountidcache（），
        std：：bind（saveledgerasync，std：：ref（app_uu）），
            标准：：占位符：：1），绑定，帐户，minledger，
                MaxLedger、Forward、Token、Limit、BunLimited、
                    页长度；
    返回RET；
}

bool networkopsimp：：重新验证（
    stvalidation:：ref val，std:：string const&source）
{
    jlog（m_journal.debug（））<“recvvalidation”<<val->getledgerhash（）。
                          <“From”<<来源；
    公共验证（VAL）；
    返回handlenewvalidation（app_uuval，source）；
}

json:：value networkopsimp:：getconsenssinfo（）。
{
    返回mconssensus.getjson（true）；
}

json:：value networkopsimp:：getserverinfo（bool-human、bool-admin、bool-counters）
{
    json:：value info=json:：objectValue；

    //hostid:描述计算机的唯一字符串
    如果（人）
        info[jss:：hostid]=gethostid（admin）；

    info[jss:：build_version]=build info:：getversionString（）；

    信息[jss:：server_state]=strOperationMode（）；

    信息[JSS:：Time]=到字符串（日期：：floor<std:：chrono:：microseconds>）
        std:：chrono:：system_clock:：now（））；

    如果（NeedNetworkLedger）
        info[jss:：network_ledger]=“等待”；

    info[jss:：validation_quorum]=static_cast<json:：uint>。
        应用程序验证程序（）.quorum（））；

    如果（管理员）
    {
        auto when=app_.validators（）.expires（）；

        如果（！）人）
        {
            如果（何时）
                信息[JSS:：validator_list_expires]=
                    safe_cast<json:：uint>（when->time_since_epoch（）.count（））；
            其他的
                info[jss:：validator_list_expires]=0；
        }
        其他的
        {
            auto&x=（info[jss:：validator_list]=json:：objectValue）；

            x[jss:：count]=static_cast<json:：uint>（app_.validators（）.count（））；

            如果（何时）
            {
                if（*when==timekeeper:：time_point:：max（））
                {
                    x[jss:：expiration]=“从不”；
                    x[jss:：status]=“活动”；
                }
                其他的
                {
                    x[jss:：expiration]=到字符串（*when）；

                    if（*when>app_.timekeeper（）.now（））
                        x[jss:：status]=“活动”；
                    其他的
                        x[jss:：status]=“过期”；
                }
            }
            其他的
            {
                x[jss:：status]=“未知”；
                x[jss:：expiration]=“未知”；
            }
        }
    }
    信息[jss:：io_latency_ms]=静态\u cast<json:：uint>（）
        app_.getIOlatency（）.count（））；

    如果（管理员）
    {
        如果（！）应用程序getValidationPublicKey（）.Empty（））
        {
            信息[jss:：pubkey_validator]=tobase58（
                标记类型：：nodepublic，
                app_.validators（）.localpublickey（））；
        }
        其他的
        {
            info[jss:：pubkey_validator]=“无”；
        }
    }

    如果（计数器）
    {
        info[jss:：counters]=app_.getperflog（）.countersjson（）；
        info[jss:：current_activities]=app_u.getperflog（）.currentjson（）；
    }

    信息[jss:：pubkey_node]=tobase58（
        标记类型：：nodepublic，
        app_.nodeidentity（）.first）；

    信息[JSS：：完整账簿]=
            应用程序getledgermaster（）.getCompleteledgers（）；

    如果（修正阻塞）
        info[jss:：modification_blocked]=true；

    auto const fp=m_ledgermaster.getFetchPackCacheSize（）；

    如果（FP）！= 0）
        信息[jss:：fetch_pack]=json:：uint（fp）；

    信息[jss:：peers]=json:：uint（app_.overlay（）.size（））；

    json:：value lastclose=json:：objectValue；
    lastclose[jss:：proposers]=json:：uint（mconssensus.preproposers（））；

    如果（人）
    {
        lastclose[jss:：converge_time_s]=
            std:：chrono:：duration<double>
                mconssens.prevroundtime（）.count（）；
    }
    其他的
    {
        lastclose[jss:：converge_time]=
                json:：int（mconssensus.prevroundtime（）.count（））；
    }

    info[jss:：last_close]=last close；

    //info[jss:：consensus]=mconssensus.getjson（）；

    如果（管理员）
        信息[jss:：load]=m_job_queue.getjson（）；

    auto const escalationMetrics=app_.gettxq（）.getMetrics（）。
        *应用程序openledger（）.current（））；

    auto const loadforgerserver=app_.getfeetrack（）.getloadforer（）；
    auto const loadbaseserver=app_.getfeetrack（）.getloadbase（）；
    自动常量LoadFactorFeeEscalation=
        EscalationMetrics.OpenledgerFeel级别；
    自动常量LoadBaseFeeScallation=
        升级度量。参考级别；

    auto const loadfactor=std:：max（安全强制转换<std:：uint64）（loadfactorserver），
        muldiv（loadforfeescalation，loadbaseserver，loadbasefeescalation）.second）；

    如果（！）人）
    {
        constexpr标准：：uint64_t max32=
            std:：numeric_limits<std:：uint32_t>：：max（）；

        info[jss:：load_base]=loadbaseserver；
        info[jss:：load_factor]=static_cast<std:：uint32_t>（）
            std：：最小（max32，加载因子））；
        info[jss:：load_factor_server]=load factor server；

        /*json:：value不支持uint64，所以钳位到max
            UIT32值。这主要是理论上的，因为
            可能现有的xrp不足以驱动因素
            那么高。
        **/

        info[jss::load_factor_fee_escalation] =
            static_cast<std::uint32_t> (std::min(
                max32, loadFactorFeeEscalation));
        info[jss::load_factor_fee_queue] =
            static_cast<std::uint32_t> (std::min(
                max32, escalationMetrics.minProcessingFeeLevel));
        info[jss::load_factor_fee_reference] =
            static_cast<std::uint32_t> (std::min(
                max32, loadBaseFeeEscalation));
    }
    else
    {
        info[jss::load_factor] = static_cast<double> (loadFactor) / loadBaseServer;

        if (loadFactorServer != loadFactor)
            info[jss::load_factor_server] =
                static_cast<double> (loadFactorServer) / loadBaseServer;

        if (admin)
        {
            std::uint32_t fee = app_.getFeeTrack().getLocalFee();
            if (fee != loadBaseServer)
                info[jss::load_factor_local] =
                    static_cast<double> (fee) / loadBaseServer;
            fee = app_.getFeeTrack ().getRemoteFee();
            if (fee != loadBaseServer)
                info[jss::load_factor_net] =
                    static_cast<double> (fee) / loadBaseServer;
            fee = app_.getFeeTrack().getClusterFee();
            if (fee != loadBaseServer)
                info[jss::load_factor_cluster] =
                    static_cast<double> (fee) / loadBaseServer;
        }
        if (loadFactorFeeEscalation !=
                escalationMetrics.referenceFeeLevel &&
                    (admin || loadFactorFeeEscalation != loadFactor))
            info[jss::load_factor_fee_escalation] =
                static_cast<double> (loadFactorFeeEscalation) /
                    escalationMetrics.referenceFeeLevel;
        if (escalationMetrics.minProcessingFeeLevel !=
                escalationMetrics.referenceFeeLevel)
            info[jss::load_factor_fee_queue] =
                static_cast<double> (
                    escalationMetrics.minProcessingFeeLevel) /
                        escalationMetrics.referenceFeeLevel;
    }

    bool valid = false;
    auto lpClosed = m_ledgerMaster.getValidatedLedger ();

    if (lpClosed)
        valid = true;
    else
        lpClosed = m_ledgerMaster.getClosedLedger ();

    if (lpClosed)
    {
        std::uint64_t baseFee = lpClosed->fees().base;
        std::uint64_t baseRef = lpClosed->fees().units;
        Json::Value l (Json::objectValue);
        l[jss::seq] = Json::UInt (lpClosed->info().seq);
        l[jss::hash] = to_string (lpClosed->info().hash);

        if (!human)
        {
            l[jss::base_fee] = Json::Value::UInt (baseFee);
            l[jss::reserve_base] = Json::Value::UInt (lpClosed->fees().accountReserve(0).drops());
            l[jss::reserve_inc] =
                    Json::Value::UInt (lpClosed->fees().increment);
            l[jss::close_time] =
                    Json::Value::UInt (lpClosed->info().closeTime.time_since_epoch().count());
        }
        else
        {
            l[jss::base_fee_xrp] = static_cast<double> (baseFee) /
                    SYSTEM_CURRENCY_PARTS;
            l[jss::reserve_base_xrp]   =
                static_cast<double> (Json::UInt (
                    lpClosed->fees().accountReserve(0).drops() * baseFee / baseRef))
                    / SYSTEM_CURRENCY_PARTS;
            l[jss::reserve_inc_xrp]    =
                static_cast<double> (Json::UInt (
                    lpClosed->fees().increment * baseFee / baseRef))
                    / SYSTEM_CURRENCY_PARTS;

            auto const nowOffset = app_.timeKeeper().nowOffset();
            if (std::abs (nowOffset.count()) >= 60)
                l[jss::system_time_offset] = nowOffset.count();

            auto const closeOffset = app_.timeKeeper().closeOffset();
            if (std::abs (closeOffset.count()) >= 60)
                l[jss::close_time_offset] = closeOffset.count();

            auto lCloseTime = lpClosed->info().closeTime;
            auto closeTime = app_.timeKeeper().closeTime();
            if (lCloseTime <= closeTime)
            {
                using namespace std::chrono_literals;
                auto age = closeTime - lCloseTime;
                if (age < 1000000s)
                    l[jss::age] = Json::UInt (age.count());
                else
                    l[jss::age] = 0;
            }
        }

        if (valid)
            info[jss::validated_ledger] = l;
        else
            info[jss::closed_ledger] = l;

        auto lpPublished = m_ledgerMaster.getPublishedLedger ();
        if (! lpPublished)
            info[jss::published_ledger] = "none";
        else if (lpPublished->info().seq != lpClosed->info().seq)
            info[jss::published_ledger] = lpPublished->info().seq;
    }

    std::tie(info[jss::state_accounting],
        info[jss::server_state_duration_us]) = accounting_.json();
    info[jss::uptime] = UptimeClock::now().time_since_epoch().count();
    info[jss::jq_trans_overflow] = std::to_string(
        app_.overlay().getJqTransOverflow());
    info[jss::peer_disconnects] = std::to_string(
        app_.overlay().getPeerDisconnect());
    info[jss::peer_disconnects_resources] = std::to_string(
        app_.overlay().getPeerDisconnectCharges());

    return info;
}

void NetworkOPsImp::clearLedgerFetch ()
{
    app_.getInboundLedgers().clearFailures();
}

Json::Value NetworkOPsImp::getLedgerFetchInfo ()
{
    return app_.getInboundLedgers().getInfo();
}

void NetworkOPsImp::pubProposedTransaction (
    std::shared_ptr<ReadView const> const& lpCurrent,
    std::shared_ptr<STTx const> const& stTxn, TER terResult)
{
    Json::Value jvObj   = transJson (*stTxn, terResult, false, lpCurrent);

    {
        ScopedLockType sl (mSubLock);

        auto it = mStreamMaps[sRTTransactions].begin ();
        while (it != mStreamMaps[sRTTransactions].end ())
        {
            InfoSub::pointer p = it->second.lock ();

            if (p)
            {
                p->send (jvObj, true);
                ++it;
            }
            else
            {
                it = mStreamMaps[sRTTransactions].erase (it);
            }
        }
    }
    AcceptedLedgerTx alt (lpCurrent, stTxn, terResult,
        app_.accountIDCache(), app_.logs());
    JLOG(m_journal.trace()) << "pubProposed: " << alt.getJson ();
    pubAccountTransaction (lpCurrent, alt, false);
}

void NetworkOPsImp::pubLedger (
    std::shared_ptr<ReadView const> const& lpAccepted)
{
//只有当分类帐获得足够的验证时才发布分类帐。
//在连接损失或其他灾难中填充孔

    std::shared_ptr<AcceptedLedger> alpAccepted =
        app_.getAcceptedLedgerCache().fetch (lpAccepted->info().hash);
    if (! alpAccepted)
    {
        alpAccepted = std::make_shared<AcceptedLedger> (
            lpAccepted, app_.accountIDCache(), app_.logs());
        app_.getAcceptedLedgerCache().canonicalize (
            lpAccepted->info().hash, alpAccepted);
    }

    {
        ScopedLockType sl (mSubLock);

        if (!mStreamMaps[sLedger].empty ())
        {
            Json::Value jvObj (Json::objectValue);

            jvObj[jss::type] = "ledgerClosed";
            jvObj[jss::ledger_index] = lpAccepted->info().seq;
            jvObj[jss::ledger_hash] = to_string (lpAccepted->info().hash);
            jvObj[jss::ledger_time]
                    = Json::Value::UInt (lpAccepted->info().closeTime.time_since_epoch().count());

            jvObj[jss::fee_ref]
                    = Json::UInt (lpAccepted->fees().units);
            jvObj[jss::fee_base] = Json::UInt (lpAccepted->fees().base);
            jvObj[jss::reserve_base] = Json::UInt (lpAccepted->fees().accountReserve(0).drops());
            jvObj[jss::reserve_inc] = Json::UInt (lpAccepted->fees().increment);

            jvObj[jss::txn_count] = Json::UInt (alpAccepted->getTxnCount ());

            if (mMode >= omSYNCING)
            {
                jvObj[jss::validated_ledgers]
                        = app_.getLedgerMaster ().getCompleteLedgers ();
            }

            auto it = mStreamMaps[sLedger].begin ();
            while (it != mStreamMaps[sLedger].end ())
            {
                InfoSub::pointer p = it->second.lock ();
                if (p)
                {
                    p->send (jvObj, true);
                    ++it;
                }
                else
                    it = mStreamMaps[sLedger].erase (it);
            }
        }
    }

//不要锁定，因为PubAcceptedTransaction正在锁定。
    for (auto const& vt : alpAccepted->getMap ())
    {
        JLOG(m_journal.trace()) << "pubAccepted: " << vt.second->getJson ();
        pubValidatedTransaction (lpAccepted, *vt.second);
    }
}

void NetworkOPsImp::reportFeeChange ()
{
    ServerFeeSummary f{app_.openLedger().current()->fees().base,
        app_.getTxQ().getMetrics(*app_.openLedger().current()),
        app_.getFeeTrack()};

//只有在发生变化时才安排作业
    if (f != mLastFeeSummary)
    {
        m_job_queue.addJob (
            jtCLIENT, "reportFeeChange->pubServer",
            [this] (Job&) { pubServer(); });
    }
}

//此例程只能用于发布已接受或已验证的
//交易。
Json::Value NetworkOPsImp::transJson(
    const STTx& stTxn, TER terResult, bool bValidated,
    std::shared_ptr<ReadView const> const& lpCurrent)
{
    Json::Value jvObj (Json::objectValue);
    std::string sToken;
    std::string sHuman;

    transResultInfo (terResult, sToken, sHuman);

    jvObj[jss::type]           = "transaction";
    jvObj[jss::transaction]    = stTxn.getJson (0);

    if (bValidated)
    {
        jvObj[jss::ledger_index]           = lpCurrent->info().seq;
        jvObj[jss::ledger_hash]            = to_string (lpCurrent->info().hash);
        jvObj[jss::transaction][jss::date] =
            lpCurrent->info().closeTime.time_since_epoch().count();
        jvObj[jss::validated]              = true;

//写下：在这里输入下一个序列号

    }
    else
    {
        jvObj[jss::validated]              = false;
        jvObj[jss::ledger_current_index]   = lpCurrent->info().seq;
    }

    jvObj[jss::status]                 = bValidated ? "closed" : "proposed";
    jvObj[jss::engine_result]          = sToken;
    jvObj[jss::engine_result_code]     = terResult;
    jvObj[jss::engine_result_message]  = sHuman;

    if (stTxn.getTxnType() == ttOFFER_CREATE)
    {
        auto const account = stTxn.getAccountID(sfAccount);
        auto const amount = stTxn.getFieldAmount (sfTakerGets);

//如果创建的报价不是自筹资金，则添加所有者余额
        if (account != amount.issue ().account)
        {
            auto const ownerFunds = accountFunds(*lpCurrent,
                account, amount, fhIGNORE_FREEZE, app_.journal ("View"));
            jvObj[jss::transaction][jss::owner_funds] = ownerFunds.getText ();
        }
    }

    return jvObj;
}

void NetworkOPsImp::pubValidatedTransaction (
    std::shared_ptr<ReadView const> const& alAccepted,
    const AcceptedLedgerTx& alTx)
{
    Json::Value jvObj = transJson (
        *alTx.getTxn (), alTx.getResult (), true, alAccepted);
    jvObj[jss::meta] = alTx.getMeta ()->getJson (0);

    {
        ScopedLockType sl (mSubLock);

        auto it = mStreamMaps[sTransactions].begin ();
        while (it != mStreamMaps[sTransactions].end ())
        {
            InfoSub::pointer p = it->second.lock ();

            if (p)
            {
                p->send (jvObj, true);
                ++it;
            }
            else
                it = mStreamMaps[sTransactions].erase (it);
        }

        it = mStreamMaps[sRTTransactions].begin ();

        while (it != mStreamMaps[sRTTransactions].end ())
        {
            InfoSub::pointer p = it->second.lock ();

            if (p)
            {
                p->send (jvObj, true);
                ++it;
            }
            else
                it = mStreamMaps[sRTTransactions].erase (it);
        }
    }
    app_.getOrderBookDB ().processTxn (alAccepted, alTx, jvObj);
    pubAccountTransaction (alAccepted, alTx, true);
}

void NetworkOPsImp::pubAccountTransaction (
    std::shared_ptr<ReadView const> const& lpCurrent,
    const AcceptedLedgerTx& alTx,
    bool bAccepted)
{
    hash_set<InfoSub::pointer>  notify;
    int                             iProposed   = 0;
    int                             iAccepted   = 0;

    {
        ScopedLockType sl (mSubLock);

        if (!bAccepted && mSubRTAccount.empty ()) return;

        if (!mSubAccount.empty () || (!mSubRTAccount.empty ()) )
        {
            for (auto const& affectedAccount: alTx.getAffected ())
            {
                auto simiIt
                        = mSubRTAccount.find (affectedAccount);
                if (simiIt != mSubRTAccount.end ())
                {
                    auto it = simiIt->second.begin ();

                    while (it != simiIt->second.end ())
                    {
                        InfoSub::pointer p = it->second.lock ();

                        if (p)
                        {
                            notify.insert (p);
                            ++it;
                            ++iProposed;
                        }
                        else
                            it = simiIt->second.erase (it);
                    }
                }

                if (bAccepted)
                {
                    simiIt  = mSubAccount.find (affectedAccount);

                    if (simiIt != mSubAccount.end ())
                    {
                        auto it = simiIt->second.begin ();
                        while (it != simiIt->second.end ())
                        {
                            InfoSub::pointer p = it->second.lock ();

                            if (p)
                            {
                                notify.insert (p);
                                ++it;
                                ++iAccepted;
                            }
                            else
                                it = simiIt->second.erase (it);
                        }
                    }
                }
            }
        }
    }
    JLOG(m_journal.trace()) << "pubAccountTransaction:" <<
        " iProposed=" << iProposed <<
        " iAccepted=" << iAccepted;

    if (!notify.empty ())
    {
        Json::Value jvObj = transJson (
            *alTx.getTxn (), alTx.getResult (), bAccepted, lpCurrent);

        if (alTx.isApplied ())
            jvObj[jss::meta] = alTx.getMeta ()->getJson (0);

        for (InfoSub::ref isrListener : notify)
            isrListener->send (jvObj, true);
    }
}

//
//监测
//

void NetworkOPsImp::subAccount (
    InfoSub::ref isrListener,
    hash_set<AccountID> const& vnaAccountIDs, bool rt)
{
    SubInfoMapType& subMap = rt ? mSubRTAccount : mSubAccount;

    for (auto const& naAccountID : vnaAccountIDs)
    {
        JLOG(m_journal.trace()) <<
            "subAccount: account: " << toBase58(naAccountID);

        isrListener->insertSubAccountInfo (naAccountID, rt);
    }

    ScopedLockType sl (mSubLock);

    for (auto const& naAccountID : vnaAccountIDs)
    {
        auto simIterator = subMap.find (naAccountID);
        if (simIterator == subMap.end ())
        {
//找不到，请注意该帐户有一个新的单列表器。
            SubMapType  usisElement;
            usisElement[isrListener->getSeq ()] = isrListener;
//vfalc注：这是一份不必要的NAaccountID副本。
            subMap.insert (simIterator,
                make_pair(naAccountID, usisElement));
        }
        else
        {
//找到，请注意该帐户有另一个侦听器。
            simIterator->second[isrListener->getSeq ()] = isrListener;
        }
    }
}

void NetworkOPsImp::unsubAccount (
    InfoSub::ref isrListener,
    hash_set<AccountID> const& vnaAccountIDs,
    bool rt)
{
    for (auto const& naAccountID : vnaAccountIDs)
    {
//从infosub中删除
        isrListener->deleteSubAccountInfo(naAccountID, rt);
    }

//从服务器中删除
    unsubAccountInternal (isrListener->getSeq(), vnaAccountIDs, rt);
}

void NetworkOPsImp::unsubAccountInternal (
    std::uint64_t uSeq,
    hash_set<AccountID> const& vnaAccountIDs,
    bool rt)
{
    ScopedLockType sl (mSubLock);

    SubInfoMapType& subMap = rt ? mSubRTAccount : mSubAccount;

    for (auto const& naAccountID : vnaAccountIDs)
    {
        auto simIterator = subMap.find (naAccountID);

        if (simIterator != subMap.end ())
        {
//发现
            simIterator->second.erase (uSeq);

            if (simIterator->second.empty ())
            {
//不需要哈希项。
                subMap.erase (simIterator);
            }
        }
    }
}

bool NetworkOPsImp::subBook (InfoSub::ref isrListener, Book const& book)
{
    if (auto listeners = app_.getOrderBookDB ().makeBookListeners (book))
        listeners->addSubscriber (isrListener);
    else
        assert (false);
    return true;
}

bool NetworkOPsImp::unsubBook (std::uint64_t uSeq, Book const& book)
{
    if (auto listeners = app_.getOrderBookDB ().getBookListeners (book))
        listeners->removeSubscriber (uSeq);

    return true;
}

std::uint32_t NetworkOPsImp::acceptLedger (
    boost::optional<std::chrono::milliseconds> consensusDelay)
{
//此代码路径仅在服务器处于独立状态时使用
//模式通过“分类帐接受”
    assert (m_standalone);

    if (!m_standalone)
        Throw<std::runtime_error> ("Operation only possible in STANDALONE mode.");

//Fixme我们能改进一下吗，不再需要专门的
//API达成共识？
    beginConsensus (m_ledgerMaster.getClosedLedger()->info().hash);
    mConsensus.simulate (app_.timeKeeper().closeTime(), consensusDelay);
    return m_ledgerMaster.getCurrentLedger ()->info().seq;
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subLedger (InfoSub::ref isrListener, Json::Value& jvResult)
{
    if (auto lpClosed = m_ledgerMaster.getValidatedLedger ())
    {
        jvResult[jss::ledger_index]    = lpClosed->info().seq;
        jvResult[jss::ledger_hash]     = to_string (lpClosed->info().hash);
        jvResult[jss::ledger_time]
            = Json::Value::UInt(lpClosed->info().closeTime.time_since_epoch().count());
        jvResult[jss::fee_ref]
                = Json::UInt (lpClosed->fees().units);
        jvResult[jss::fee_base]        = Json::UInt (lpClosed->fees().base);
        jvResult[jss::reserve_base]    = Json::UInt (lpClosed->fees().accountReserve(0).drops());
        jvResult[jss::reserve_inc]     = Json::UInt (lpClosed->fees().increment);
    }

    if ((mMode >= omSYNCING) && !isNeedNetworkLedger ())
    {
        jvResult[jss::validated_ledgers]
                = app_.getLedgerMaster ().getCompleteLedgers ();
    }

    ScopedLockType sl (mSubLock);
    return mStreamMaps[sLedger].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubLedger (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sLedger].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subManifests (InfoSub::ref isrListener)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sManifests].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubManifests (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sManifests].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subServer (InfoSub::ref isrListener, Json::Value& jvResult,
    bool admin)
{
    uint256 uRandom;

    if (m_standalone)
        jvResult[jss::stand_alone] = m_standalone;

//检查我：这里有必要提供一个随机数吗？
    beast::rngfill (
        uRandom.begin(),
        uRandom.size(),
        crypto_prng());

    auto const& feeTrack = app_.getFeeTrack();
    jvResult[jss::random]          = to_string (uRandom);
    jvResult[jss::server_status]   = strOperatingMode ();
    jvResult[jss::load_base]       = feeTrack.getLoadBase ();
    jvResult[jss::load_factor]     = feeTrack.getLoadFactor ();
    jvResult [jss::hostid]         = getHostId (admin);
    jvResult[jss::pubkey_node]     = toBase58 (
        TokenType::NodePublic,
        app_.nodeIdentity().first);

    ScopedLockType sl (mSubLock);
    return mStreamMaps[sServer].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubServer (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sServer].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subTransactions (InfoSub::ref isrListener)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sTransactions].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubTransactions (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sTransactions].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subRTTransactions (InfoSub::ref isrListener)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sRTTransactions].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubRTTransactions (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sRTTransactions].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subValidations (InfoSub::ref isrListener)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sValidations].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubValidations (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sValidations].erase (uSeq);
}

//<--bool:true=已添加，false=已存在
bool NetworkOPsImp::subPeerStatus (InfoSub::ref isrListener)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sPeerStatus].emplace (
        isrListener->getSeq (), isrListener).second;
}

//<--bool:true=已删除，false=不存在
bool NetworkOPsImp::unsubPeerStatus (std::uint64_t uSeq)
{
    ScopedLockType sl (mSubLock);
    return mStreamMaps[sPeerStatus].erase (uSeq);
}

InfoSub::pointer NetworkOPsImp::findRpcSub (std::string const& strUrl)
{
    ScopedLockType sl (mSubLock);

    subRpcMapType::iterator it = mRpcSubMap.find (strUrl);

    if (it != mRpcSubMap.end ())
        return it->second;

    return InfoSub::pointer ();
}

InfoSub::pointer NetworkOPsImp::addRpcSub (
    std::string const& strUrl, InfoSub::ref rspEntry)
{
    ScopedLockType sl (mSubLock);

    mRpcSubMap.emplace (strUrl, rspEntry);

    return rspEntry;
}

bool NetworkOPsImp::tryRemoveRpcSub (std::string const& strUrl)
{
    ScopedLockType sl (mSubLock);
    auto pInfo = findRpcSub(strUrl);

    if (!pInfo)
        return false;

//检查是否有任何流映射仍然包含弱引用
//此条目在删除之前
    for (SubMapType const& map : mStreamMaps)
    {
        if (map.find(pInfo->getSeq()) != map.end())
            return false;
    }
    mRpcSubMap.erase(strUrl);
    return true;
}

#ifndef USE_NEW_BOOK_PAGE

//Nikb Fixme应该看看这个。没有理由不这么做
//工作，但表现不佳。
//
void NetworkOPsImp::getBookPage (
    std::shared_ptr<ReadView const>& lpLedger,
    Book const& book,
    AccountID const& uTakerID,
    bool const bProof,
    unsigned int iLimit,
    Json::Value const& jvMarker,
    Json::Value& jvResult)
{ //注意：这是旧的获取书籍页面逻辑
    Json::Value& jvOffers =
            (jvResult[jss::offers] = Json::Value (Json::arrayValue));

    std::map<AccountID, STAmount> umBalance;
    const uint256   uBookBase   = getBookBase (book);
    const uint256   uBookEnd    = getQualityNext (uBookBase);
    uint256         uTipIndex   = uBookBase;

    if (auto stream = m_journal.trace())
    {
        stream << "getBookPage:" << book;
        stream << "getBookPage: uBookBase=" << uBookBase;
        stream << "getBookPage: uBookEnd=" << uBookEnd;
        stream << "getBookPage: uTipIndex=" << uTipIndex;
    }

    ReadView const& view = *lpLedger;

    bool const bGlobalFreeze =
        isGlobalFrozen(view, book.out.account) ||
            isGlobalFrozen(view, book.in.account);

    bool            bDone           = false;
    bool            bDirectAdvance  = true;

    std::shared_ptr<SLE const> sleOfferDir;
    uint256         offerIndex;
    unsigned int    uBookEntry;
    STAmount        saDirRate;

    auto const rate = transferRate(view, book.out.account);
    auto viewJ = app_.journal ("View");

    while (! bDone && iLimit-- > 0)
    {
        if (bDirectAdvance)
        {
            bDirectAdvance  = false;

            JLOG(m_journal.trace()) << "getBookPage: bDirectAdvance";

            auto const ledgerIndex = view.succ(uTipIndex, uBookEnd);
            if (ledgerIndex)
                sleOfferDir = view.read(keylet::page(*ledgerIndex));
            else
                sleOfferDir.reset();

            if (!sleOfferDir)
            {
                JLOG(m_journal.trace()) << "getBookPage: bDone";
                bDone           = true;
            }
            else
            {
                uTipIndex = sleOfferDir->key();
                saDirRate = amountFromQuality (getQuality (uTipIndex));

                cdirFirst (view,
                    uTipIndex, sleOfferDir, uBookEntry, offerIndex, viewJ);

                JLOG(m_journal.trace())
                    << "getBookPage:   uTipIndex=" << uTipIndex;
                JLOG(m_journal.trace())
                    << "getBookPage: offerIndex=" << offerIndex;
            }
        }

        if (!bDone)
        {
            auto sleOffer = view.read(keylet::offer(offerIndex));

            if (sleOffer)
            {
                auto const uOfferOwnerID =
                        sleOffer->getAccountID (sfAccount);
                auto const& saTakerGets =
                        sleOffer->getFieldAmount (sfTakerGets);
                auto const& saTakerPays =
                        sleOffer->getFieldAmount (sfTakerPays);
                STAmount saOwnerFunds;
                bool firstOwnerOffer (true);

                if (book.out.account == uOfferOwnerID)
                {
//如果要约出售发行人自己的借据，则完全
//提供资金。
                    saOwnerFunds    = saTakerGets;
                }
                else if (bGlobalFreeze)
                {
//如果其中一项资产被全球冻结，考虑所有报价
//完全没有资金的不是我们的
                    saOwnerFunds.clear (book.out);
                }
                else
                {
                    auto umBalanceEntry  = umBalance.find (uOfferOwnerID);
                    if (umBalanceEntry != umBalance.end ())
                    {
//在运行平衡表中找到。

                        saOwnerFunds    = umBalanceEntry->second;
                        firstOwnerOffer = false;
                    }
                    else
                    {
//在表中找不到平衡。

                        saOwnerFunds = accountHolds (view,
                            uOfferOwnerID, book.out.currency,
                                book.out.account, fhZERO_IF_FROZEN, viewJ);

                        if (saOwnerFunds < beast::zero)
                        {
//将负资金视为零。

                            saOwnerFunds.clear ();
                        }
                    }
                }

                Json::Value jvOffer = sleOffer->getJson (0);

                STAmount saTakerGetsFunded;
                STAmount saOwnerFundsLimit = saOwnerFunds;
                Rate offerRate = parityRate;

                if (rate != parityRate
//交过户费。
                    && uTakerID != book.out.account
//不接受自己的借据。
                    && book.out.account != uOfferOwnerID)
//要约所有人不发行自有资金
                {
//需要向业主收取转让费。
                    offerRate = rate;
                    saOwnerFundsLimit = divide (
                        saOwnerFunds, offerRate);
                }

                if (saOwnerFundsLimit >= saTakerGets)
                {
//资金充足，没有恶作剧。
                    saTakerGetsFunded   = saTakerGets;
                }
                else
                {
//只有在资金不足的情况下才提供。

                    saTakerGetsFunded = saOwnerFundsLimit;

                    saTakerGetsFunded.setJson (jvOffer[jss::taker_gets_funded]);
                    std::min (
                        saTakerPays, multiply (
                            saTakerGetsFunded, saDirRate, saTakerPays.issue ())).setJson
                            (jvOffer[jss::taker_pays_funded]);
                }

                STAmount saOwnerPays = (parityRate == offerRate)
                    ? saTakerGetsFunded
                    : std::min (
                        saOwnerFunds,
                        multiply (saTakerGetsFunded, offerRate));

                umBalance[uOfferOwnerID]    = saOwnerFunds - saOwnerPays;

//包括所有已资助和未资助的报价
                Json::Value& jvOf = jvOffers.append (jvOffer);
                jvOf[jss::quality] = saDirRate.getText ();

                if (firstOwnerOffer)
                    jvOf[jss::owner_funds] = saOwnerFunds.getText ();
            }
            else
            {
                JLOG(m_journal.warn()) << "Missing offer";
            }

            if (! cdirNext(view,
                    uTipIndex, sleOfferDir, uBookEntry, offerIndex, viewJ))
            {
                bDirectAdvance  = true;
            }
            else
            {
                JLOG(m_journal.trace())
                    << "getBookPage: offerIndex=" << offerIndex;
            }
        }
    }

//jvresult[jss:：marker]=json:：value（json:：arrayvalue）；
//jvresult[jss:：nodes]=json:：value（json:：arrayvalue）；
}


#else

//这是使用Book迭代器的新代码
//它暂时被禁用

void NetworkOPsImp::getBookPage (
    std::shared_ptr<ReadView const> lpLedger,
    Book const& book,
    AccountID const& uTakerID,
    bool const bProof,
    unsigned int iLimit,
    Json::Value const& jvMarker,
    Json::Value& jvResult)
{
    auto& jvOffers = (jvResult[jss::offers] = Json::Value (Json::arrayValue));

    std::map<AccountID, STAmount> umBalance;

    MetaView  lesActive (lpLedger, tapNONE, true);
    OrderBookIterator obIterator (lesActive, book);

    auto const rate = transferRate(lesActive, book.out.account);

    const bool bGlobalFreeze = lesActive.isGlobalFrozen (book.out.account) ||
                               lesActive.isGlobalFrozen (book.in.account);

    while (iLimit-- > 0 && obIterator.nextOffer ())
    {

        SLE::pointer    sleOffer        = obIterator.getCurrentOffer();
        if (sleOffer)
        {
            auto const uOfferOwnerID = sleOffer->getAccountID (sfAccount);
            auto const& saTakerGets = sleOffer->getFieldAmount (sfTakerGets);
            auto const& saTakerPays = sleOffer->getFieldAmount (sfTakerPays);
            STAmount saDirRate = obIterator.getCurrentRate ();
            STAmount saOwnerFunds;

            if (book.out.account == uOfferOwnerID)
            {
//如果发盘是出售发行人自己的借据，那么它是完全融资的。
                saOwnerFunds    = saTakerGets;
            }
            else if (bGlobalFreeze)
            {
//如果其中一项资产被全球冻结，考虑所有报价
//完全没有资金的不是我们的
                saOwnerFunds.clear (book.out);
            }
            else
            {
                auto umBalanceEntry = umBalance.find (uOfferOwnerID);

                if (umBalanceEntry != umBalance.end ())
                {
//在运行平衡表中找到。

                    saOwnerFunds    = umBalanceEntry->second;
                }
                else
                {
//在表中找不到平衡。

                    saOwnerFunds = lesActive.accountHolds (
                        uOfferOwnerID, book.out.currency, book.out.account, fhZERO_IF_FROZEN);

                    if (saOwnerFunds.isNegative ())
                    {
//将负资金视为零。

                        saOwnerFunds.zero ();
                    }
                }
            }

            Json::Value jvOffer = sleOffer->getJson (0);

            STAmount saTakerGetsFunded;
            STAmount saOwnerFundsLimit = saOwnerFunds;
            Rate offerRate = parityRate;

            if (rate != parityRate
//交过户费。
                && uTakerID != book.out.account
//不接受自己的借据。
                && book.out.account != uOfferOwnerID)
//要约所有人不发行自有资金
            {
//需要向业主收取转让费。
                offerRate = rate;
                saOwnerFundsLimit = divide (saOwnerFunds, offerRate);
            }

            if (saOwnerFundsLimit >= saTakerGets)
            {
//资金充足，没有恶作剧。
                saTakerGetsFunded   = saTakerGets;
            }
            else
            {
//只有在资金不足的情况下才提供。
                saTakerGetsFunded   = saOwnerFundsLimit;

                saTakerGetsFunded.setJson (jvOffer[jss::taker_gets_funded]);

//tood（汤姆）：这个表达式的结果没有使用-什么是
//在这里？
                std::min (saTakerPays, multiply (
                    saTakerGetsFunded, saDirRate, saTakerPays.issue ())).setJson (
                        jvOffer[jss::taker_pays_funded]);
            }

            STAmount saOwnerPays = (parityRate == offerRate)
                ? saTakerGetsFunded
                : std::min (
                    saOwnerFunds,
                    multiply (saTakerGetsFunded, offerRate));

            umBalance[uOfferOwnerID]    = saOwnerFunds - saOwnerPays;

            if (!saOwnerFunds.isZero () || uOfferOwnerID == uTakerID)
            {
//只提供受资助的报价和接受方的报价。
                Json::Value& jvOf   = jvOffers.append (jvOffer);
                jvOf[jss::quality]     = saDirRate.getText ();
            }

        }
    }

//jvresult[jss:：marker]=json:：value（json:：arrayvalue）；
//jvresult[jss:：nodes]=json:：value（json:：arrayvalue）；
}

#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

NetworkOPs::NetworkOPs (Stoppable& parent)
    : InfoSub::Source ("NetworkOPs", parent)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————


void NetworkOPsImp::StateAccounting::mode (OperatingMode om)
{
    auto now = std::chrono::system_clock::now();

    std::lock_guard<std::mutex> lock (mutex_);
    ++counters_[om].transitions;
    counters_[mode_].dur += std::chrono::duration_cast<
        std::chrono::microseconds>(now - start_);

    mode_ = om;
    start_ = now;
}

NetworkOPsImp::StateAccounting::StateCountersJson
NetworkOPsImp::StateAccounting::json() const
{
    std::unique_lock<std::mutex> lock (mutex_);

    auto counters = counters_;
    auto const start = start_;
    auto const mode = mode_;

    lock.unlock();

    auto const current = std::chrono::duration_cast<
        std::chrono::microseconds>(std::chrono::system_clock::now() - start);
    counters[mode].dur += current;

    Json::Value ret = Json::objectValue;

    for (std::underlying_type_t<OperatingMode> i = omDISCONNECTED;
        i <= omFULL; ++i)
    {
        ret[states_[i]] = Json::objectValue;
        auto& state = ret[states_[i]];
        state[jss::transitions] = counters[i].transitions;
        state[jss::duration_us] = std::to_string(counters[i].dur.count());
    }

    return {ret, std::to_string(current.count())};
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<NetworkOPs>
make_NetworkOPs (Application& app, NetworkOPs::clock_type& clock,
    bool standalone, std::size_t network_quorum, bool startvalid,
    JobQueue& job_queue, LedgerMaster& ledgerMaster, Stoppable& parent,
    ValidatorKeys const & validatorKeys, boost::asio::io_service& io_svc,
    beast::Journal journal)
{
    return std::make_unique<NetworkOPsImp> (app, clock, standalone,
        network_quorum, startvalid, job_queue, ledgerMaster, parent,
        validatorKeys, io_svc, journal);
}

} //涟漪
