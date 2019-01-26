
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

#ifndef RIPPLE_APP_MISC_SHAMAPSTOREIMP_H_INCLUDED
#define RIPPLE_APP_MISC_SHAMAPSTOREIMP_H_INCLUDED

#include <ripple/app/misc/SHAMapStore.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/nodestore/DatabaseRotating.h>
#include <condition_variable>
#include <thread>

namespace ripple {

class NetworkOPs;

class SHAMapStoreImp : public SHAMapStore
{
private:
    struct SavedState
    {
        std::string writableDb;
        std::string archiveDb;
        LedgerIndex lastRotated;
    };

    enum Health : std::uint8_t
    {
        ok = 0,
        stopping,
        unhealthy
    };

    class SavedStateDB
    {
    public:
        soci::session session_;
        std::mutex mutex_;
        beast::Journal journal_;

//只需在没有任何逻辑的情况下实例化，以防联机删除
//配置
        explicit SavedStateDB()
        : journal_ {beast::Journal::getNullSink()}
        { }

//打开数据库，如有必要，创建并初始化其表。
        void init (BasicConfig const& config, std::string const& dbName);
//获取/设置我们最多可以删除并包括的分类帐索引
        LedgerIndex getCanDelete();
        LedgerIndex setCanDelete (LedgerIndex canDelete);
        SavedState getState();
        void setState (SavedState const& state);
        void setLastRotated (LedgerIndex seq);
    };

    Application& app_;

//状态数据库名称
    std::string const dbName_ = "state";
//磁盘上节点存储后端实例的前缀
    std::string const dbPrefix_ = "rippledb";
//复制记录时检查运行状况/停止状态
    std::uint64_t const checkHealthInterval_ = 1000;
//保持网络健康的最低分类账
    static std::uint32_t const minimumDeletionInterval_ = 256;
//独立模式所需的最小分类账。
    static std::uint32_t const minimumDeletionIntervalSA_ = 8;

    Setup setup_;
    NodeStore::Scheduler& scheduler_;
    beast::Journal journal_;
    beast::Journal nodeStoreJournal_;
    NodeStore::DatabaseRotating* dbRotating_ = nullptr;
    SavedStateDB state_db_;
    std::thread thread_;
    bool stop_ = false;
    bool healthy_ = true;
    mutable std::condition_variable cond_;
    mutable std::condition_variable rendezvous_;
    mutable std::mutex mutex_;
    std::shared_ptr<Ledger const> newLedger_;
    std::atomic<bool> working_;
    TransactionMaster& transactionMaster_;
    std::atomic <LedgerIndex> canDelete_;
//这些不存在于shamapstore创建时，但确实存在
//从onPrepare（）或之前开始
    NetworkOPs* netOPs_ = nullptr;
    LedgerMaster* ledgerMaster_ = nullptr;
    FullBelowCache* fullBelowCache_ = nullptr;
    TreeNodeCache* treeNodeCache_ = nullptr;
    DatabaseCon* transactionDb_ = nullptr;
    DatabaseCon* ledgerDb_ = nullptr;
    int fdlimit_ = 0;

public:
    SHAMapStoreImp (Application& app,
            Setup const& setup,
            Stoppable& parent,
            NodeStore::Scheduler& scheduler,
            beast::Journal journal,
            beast::Journal nodeStoreJournal,
            TransactionMaster& transactionMaster,
            BasicConfig const& config);

    ~SHAMapStoreImp()
    {
        if (thread_.joinable())
            thread_.join();
    }

    std::uint32_t
    clampFetchDepth (std::uint32_t fetch_depth) const override
    {
        return setup_.deleteInterval ? std::min (fetch_depth,
                setup_.deleteInterval) : fetch_depth;
    }

    std::unique_ptr <NodeStore::Database> makeDatabase (
            std::string const&name,
            std::int32_t readThreads, Stoppable& parent) override;

    std::unique_ptr <NodeStore::DatabaseShard>
    makeDatabaseShard(std::string const& name,
        std::int32_t readThreads, Stoppable& parent) override;

    LedgerIndex
    setCanDelete (LedgerIndex seq) override
    {
        if (setup_.advisoryDelete)
            canDelete_ = seq;
        return state_db_.setCanDelete (seq);
    }

    bool
    advisoryDelete() const override
    {
        return setup_.advisoryDelete;
    }

//在此之前的所有分类帐都是合格的
//下一轮删除
    LedgerIndex
    getLastRotated() override
    {
        return state_db_.getState().lastRotated;
    }

//在此之前和包括此之前的所有分类帐都不受保护
//如果合适的话，在线删除可以删除它们。
    LedgerIndex
    getCanDelete() override
    {
        return canDelete_;
    }

    void onLedgerClosed (std::shared_ptr<Ledger const> const& ledger) override;

    void rendezvous() const override;
    int fdlimit() const override;

private:
//VisitNodes的回调
    bool copyNode (std::uint64_t& nodeCount, SHAMapAbstractNode const &node);
    void run();
    void dbPaths();

    std::unique_ptr<NodeStore::Backend>
    makeBackendRotating (std::string path = std::string());

    template <class CacheInstance>
    bool
    freshenCache (CacheInstance& cache)
    {
        std::uint64_t check = 0;

        for (auto const& key: cache.getKeys())
        {
            dbRotating_->fetch(key, 0);
            if (! (++check % checkHealthInterval_) && health())
                return true;
        }

        return false;
    }

    /*批量从sqlite表中删除，以免过度锁定数据库
     *短暂暂停以延长对其他用户的访问时间
     *在互斥对象未锁定的情况下调用
     *@如果找到任何可删除行，则返回true（尽管没有
     *必须删除。
     **/

    bool clearSql (DatabaseCon& database, LedgerIndex lastRotated,
                   std::string const& minQuery, std::string const& deleteQuery);
    void clearCaches (LedgerIndex validatedSeq);
    void freshenCaches();
    void clearPrior (LedgerIndex lastRotated);

//如果涟漪不健康，则延迟旋转删除。
//如果已经不健康，请不要在进一步检查时更改状态。
//假设一旦不健康，就必须采取
//已中止，因此需要重新启动联机删除进程
//在下一个分类帐上。
    Health health();
//
//可停止的
//
    void
    onPrepare() override
    {
    }

    void
    onStart() override
    {
        if (setup_.deleteInterval)
            thread_ = std::thread (&SHAMapStoreImp::run, this);
    }

//当应用程序开始关闭时调用
    void onStop() override;
//当所有子可停止对象都已停止时调用
    void onChildrenStopped() override;

};

}

#endif
