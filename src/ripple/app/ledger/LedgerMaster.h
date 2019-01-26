
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

#ifndef RIPPLE_APP_LEDGER_LEDGERMASTER_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERMASTER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/AbstractFetchPackContainer.h>
#include <ripple/app/ledger/InboundLedgers.h>
#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/ledger/LedgerCleaner.h>
#include <ripple/app/ledger/LedgerHistory.h>
#include <ripple/app/ledger/LedgerHolder.h>
#include <ripple/app/ledger/LedgerReplay.h>
#include <ripple/app/misc/CanonicalTXSet.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/RangeSet.h>
#include <ripple/basics/ScopedLock.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/RippleLedgerHash.h>
#include <ripple/protocol/STValidation.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/core/Stoppable.h>
#include <ripple/protocol/Protocol.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <mutex>

#include <ripple/protocol/messages.h>

namespace ripple {

class Peer;
class Transaction;



//跟踪当前分类帐和结算过程中的任何分类帐
//跟踪分类帐历史记录
//跟踪保留的交易

//vfalc todo重命名为分类帐
//这听起来像是所有的分类帐…
//
class LedgerMaster
    : public Stoppable
    , public AbstractFetchPackContainer
{
public:
    explicit
    LedgerMaster(Application& app, Stopwatch& stopwatch,
        Stoppable& parent,
            beast::insight::Collector::ptr const& collector,
                beast::Journal journal);

    virtual ~LedgerMaster () = default;

    LedgerIndex getCurrentLedgerIndex ();
    LedgerIndex getValidLedgerIndex ();

    bool isCompatible (
        ReadView const&,
        beast::Journal::Stream,
        char const* reason);

    std::recursive_mutex& peekMutex ();

//当前分类账是我们认为新交易应该进入的分类账。
    std::shared_ptr<ReadView const>
    getCurrentLedger();

//最终确定的分类帐是最后一个已关闭/已接受的分类帐
    std::shared_ptr<Ledger const>
    getClosedLedger()
    {
        return mClosedLedger.get();
    }

//已验证的分类帐是最后一个完全验证的分类帐
    std::shared_ptr<Ledger const>
    getValidatedLedger ()
    {
        return mValidLedger.get();
    }

//如果有的话，这些规则在最后一个完全有效的分类帐中。
    Rules getValidatedRules();

//这是我们向客户发布的最后一个分类帐，可能会延迟验证
//分类帐
    std::shared_ptr<ReadView const>
    getPublishedLedger();

    std::chrono::seconds getPublishedLedgerAge ();
    std::chrono::seconds getValidatedLedgerAge ();
    bool isCaughtUp(std::string& reason);

    std::uint32_t getEarliestFetch ();

    bool storeLedger (std::shared_ptr<Ledger const> ledger);

    void setFullLedger (
        std::shared_ptr<Ledger const> const& ledger,
            bool isSynchronous, bool isCurrent);

    /*检查的序列号和父级关闭时间
        根据我们的时钟分类账和上次验证的分类账
        看看它是否是网络的当前分类帐
    **/

    bool canBeCurrent (std::shared_ptr<Ledger const> const& ledger);

    void switchLCL (std::shared_ptr<Ledger const> const& lastClosed);

    void failedSave(std::uint32_t seq, uint256 const& hash);

    std::string getCompleteLedgers ();

    /*将保留的交易记录应用于未结分类帐
        这通常在我们关闭分类帐时调用。
        未结分类帐保持未结状态以处理新交易记录
        直到建立新的未结分类帐。
    **/

    void applyHeldTransactions ();

    /*获取为特定帐户保留的所有交易记录。
        这通常在事务处理时调用
        帐户已成功应用于打开
        分类帐，以便在没有
        正在等待分类帐关闭。
    **/

    std::vector<std::shared_ptr<STTx const>>
    pruneHeldTransactions(AccountID const& account,
        std::uint32_t const seq);

    /*使用缓存按序列号获取分类帐哈希
    **/

    uint256 getHashBySeq (std::uint32_t index);

    /*使用跳过列表走到分类帐哈希*/
    boost::optional<LedgerHash> walkHashBySeq (std::uint32_t index);

    /*浏览分类帐哈希链以确定
        具有指定索引的分类帐。referenceledger用作
        链条的底座应经过充分验证，不得
        在目标索引之前。如果节点
        从参考分类帐或任何以前的分类帐不存在
        在节点存储区中。
    **/

    boost::optional<LedgerHash> walkHashBySeq (
        std::uint32_t index,
        std::shared_ptr<ReadView const> const& referenceLedger);

    std::shared_ptr<Ledger const>
    getLedgerBySeq (std::uint32_t index);

    std::shared_ptr<Ledger const>
    getLedgerByHash (uint256 const& hash);

    void setLedgerRangePresent (
        std::uint32_t minV, std::uint32_t maxV);

    boost::optional<LedgerHash> getLedgerHash(
        std::uint32_t desiredSeq,
        std::shared_ptr<ReadView const> const& knownGoodLedger);

    boost::optional <NetClock::time_point> getCloseTimeBySeq (
        LedgerIndex ledgerIndex);

    boost::optional <NetClock::time_point> getCloseTimeByHash (
        LedgerHash const& ledgerHash, LedgerIndex ledgerIndex);

    void addHeldTransaction (std::shared_ptr<Transaction> const& trans);
    void fixMismatch (ReadView const& ledger);

    bool haveLedger (std::uint32_t seq);
    void clearLedger (std::uint32_t seq);
    bool getValidatedRange (
        std::uint32_t& minVal, std::uint32_t& maxVal);
    bool getFullValidatedRange (
        std::uint32_t& minVal, std::uint32_t& maxVal);

    void tune (int size, std::chrono::seconds age);
    void sweep ();
    float getCacheHitRate ();

    void checkAccept (std::shared_ptr<Ledger const> const& ledger);
    void checkAccept (uint256 const& hash, std::uint32_t seq);
    void
    consensusBuilt(
        std::shared_ptr<Ledger const> const& ledger,
        uint256 const& consensusHash,
        Json::Value consensus);

    LedgerIndex getBuildingLedger ();
    void setBuildingLedger (LedgerIndex index);

    void tryAdvance ();
bool newPathRequest (); //如果路径请求成功放置，则返回true。
    bool isNewPathRequest ();
bool newOrderBookDB (); //如果能够满足请求，则返回true。

    bool fixIndex (
        LedgerIndex ledgerIndex, LedgerHash const& ledgerHash);
    void doLedgerCleaner(Json::Value const& parameters);

    beast::PropertyStream::Source& getPropertySource ();

    void clearPriorLedgers (LedgerIndex seq);

    void clearLedgerCachePrior (LedgerIndex seq);

//分类帐重播
    void takeReplay (std::unique_ptr<LedgerReplay> replay);
    std::unique_ptr<LedgerReplay> releaseReplay ();

//取包
    void gotFetchPack (
        bool progress,
        std::uint32_t seq);

    void addFetchPack (
        uint256 const& hash,
        std::shared_ptr<Blob>& data);

    boost::optional<Blob>
    getFetchPack (uint256 const& hash) override;

    void makeFetchPack (
        std::weak_ptr<Peer> const& wPeer,
        std::shared_ptr<protocol::TMGetObjectByHash> const& request,
        uint256 haveLedgerHash,
        UptimeClock::time_point uptime);

    std::size_t getFetchPackCacheSize () const;

private:
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;
    using ScopedUnlockType = GenericScopedUnlock <std::recursive_mutex>;

    void setValidLedger(
        std::shared_ptr<Ledger const> const& l);
    void setPubLedger(
        std::shared_ptr<Ledger const> const& l);

    void tryFill(
        Job& job,
        std::shared_ptr<Ledger const> ledger);

    void getFetchPack(
        LedgerIndex missing, InboundLedger::Reason reason);

    boost::optional<LedgerHash> getLedgerHashForHistory(
        LedgerIndex index, InboundLedger::Reason reason);

    std::size_t getNeededValidations();
    void advanceThread();
    void fetchForHistory(
        std::uint32_t missing,
        bool& progress,
        InboundLedger::Reason reason);
//尝试发布分类帐，获取丢失的分类帐。总是用
//M_Mutex已锁定。传递的ScopedLockType是对调用方的提醒。
    void doAdvance(ScopedLockType&);
    bool shouldAcquire(
        std::uint32_t const currentLedger,
        std::uint32_t const ledgerHistory,
        std::uint32_t const ledgerHistoryIndex,
        std::uint32_t const candidateLedger) const;

    std::vector<std::shared_ptr<Ledger const>>
    findNewLedgersToPublish();

    void updatePaths(Job& job);

//如果开始工作，则返回true。总是在M_mutex锁定的情况下调用。
//传递的ScopedLockType是对调用方的提醒。
    bool newPFWork(const char *name, ScopedLockType&);

private:
    Application& app_;
    beast::Journal m_journal;

    std::recursive_mutex mutable m_mutex;

//最近关闭的分类帐。
    LedgerHolder mClosedLedger;

//我们完全接受的最高顺序分类账。
    LedgerHolder mValidLedger;

//我们出版的最后一本分类帐。
    std::shared_ptr<Ledger const> mPubLedger;

//我们做的最后一个分类帐。
    std::shared_ptr<Ledger const> mPathLedger;

//我们处理的最后一个分类账取数历史
    std::shared_ptr<Ledger const> mHistLedger;

//我们处理的最后一个分类账是一个碎片的提取。
    std::shared_ptr<Ledger const> mShardLedger;

//完全有效的分类帐，无论我们是否有分类帐居民。
    std::pair <uint256, LedgerIndex> mLastValidLedger {uint256(), 0};

    LedgerHistory mLedgerHistory;

    CanonicalTXSet mHeldTransactions {uint256()};

//下一次关闭时要重播的一组事务
    std::unique_ptr<LedgerReplay> replayData;

    std::recursive_mutex mCompleteLock;
    RangeSet<std::uint32_t> mCompleteLedgers;

    std::unique_ptr <detail::LedgerCleaner> mLedgerCleaner;

    uint256 mLastValidateHash;
    std::uint32_t mLastValidateSeq {0};

//发布线程正在运行。
    bool                        mAdvanceThread {false};

//发布线程有工作要做。
    bool                        mAdvanceWork {false};
    int                         mFillInProgress {0};

int     mPathFindThread {0};    //已调度探路者作业
    bool    mPathFindNewRequest {false};

std::atomic_flag mGotFetchPackThread = ATOMIC_FLAG_INIT; //已调度gotfitchpack作业

    std::atomic <std::uint32_t> mPubLedgerClose {0};
    std::atomic <LedgerIndex> mPubLedgerSeq {0};
    std::atomic <std::uint32_t> mValidLedgerSign {0};
    std::atomic <LedgerIndex> mValidLedgerSeq {0};
    std::atomic <LedgerIndex> mBuildingLedgerSeq {0};

//服务器处于独立模式
    bool const standalone_;

//在当前分类账之前，我们允许同行请求多少分类账？
    std::uint32_t const fetch_depth_;

//我们要保留多少历史
    std::uint32_t const ledger_history_;

    std::uint32_t const ledger_fetch_size_;

    TaggedCache<uint256, Blob> fetch_packs_;

    std::uint32_t fetch_seq_ {0};

//尝试阻止验证器从测试切换到实时网络
//不需要先清除数据库。
    LedgerIndex const max_ledger_difference_ {1000000};

};

} //涟漪

#endif
