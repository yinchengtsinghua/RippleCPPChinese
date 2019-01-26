
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


#include <ripple/app/ledger/TransactionMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/app/misc/SHAMapStoreImp.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/nodestore/impl/DatabaseRotatingImp.h>
#include <ripple/nodestore/impl/DatabaseShardImp.h>

namespace ripple {
void SHAMapStoreImp::SavedStateDB::init (BasicConfig const& config,
                                         std::string const& dbName)
{
    std::lock_guard<std::mutex> lock (mutex_);

    open(session_, config, dbName);

    session_ << "PRAGMA synchronous=FULL;";

    session_ <<
        "CREATE TABLE IF NOT EXISTS DbState ("
        "  Key                    INTEGER PRIMARY KEY,"
        "  WritableDb             TEXT,"
        "  ArchiveDb              TEXT,"
        "  LastRotatedLedger      INTEGER"
        ");"
        ;

    session_ <<
        "CREATE TABLE IF NOT EXISTS CanDelete ("
        "  Key                    INTEGER PRIMARY KEY,"
        "  CanDeleteSeq           INTEGER"
        ");"
        ;

    std::int64_t count = 0;
    {
        boost::optional<std::int64_t> countO;
        session_ <<
                "SELECT COUNT(Key) FROM DbState WHERE Key = 1;"
                , soci::into (countO);
        if (!countO)
            Throw<std::runtime_error> ("Failed to fetch Key Count from DbState.");
        count = *countO;
    }

    if (!count)
    {
        session_ <<
                "INSERT INTO DbState VALUES (1, '', '', 0);";
    }


    {
        boost::optional<std::int64_t> countO;
        session_ <<
                "SELECT COUNT(Key) FROM CanDelete WHERE Key = 1;"
                , soci::into (countO);
        if (!countO)
            Throw<std::runtime_error> ("Failed to fetch Key Count from CanDelete.");
        count = *countO;
    }

    if (!count)
    {
        session_ <<
                "INSERT INTO CanDelete VALUES (1, 0);";
    }
}

LedgerIndex
SHAMapStoreImp::SavedStateDB::getCanDelete()
{
    LedgerIndex seq;
    std::lock_guard<std::mutex> lock (mutex_);

    session_ <<
            "SELECT CanDeleteSeq FROM CanDelete WHERE Key = 1;"
            , soci::into (seq);
    ;

    return seq;
}

LedgerIndex
SHAMapStoreImp::SavedStateDB::setCanDelete (LedgerIndex canDelete)
{
    std::lock_guard<std::mutex> lock (mutex_);

    session_ <<
            "UPDATE CanDelete SET CanDeleteSeq = :canDelete WHERE Key = 1;"
            , soci::use (canDelete)
            ;

    return canDelete;
}

SHAMapStoreImp::SavedState
SHAMapStoreImp::SavedStateDB::getState()
{
    SavedState state;

    std::lock_guard<std::mutex> lock (mutex_);

    session_ <<
            "SELECT WritableDb, ArchiveDb, LastRotatedLedger"
            " FROM DbState WHERE Key = 1;"
            , soci::into (state.writableDb), soci::into (state.archiveDb)
            , soci::into (state.lastRotated)
            ;

    return state;
}

void
SHAMapStoreImp::SavedStateDB::setState (SavedState const& state)
{
    std::lock_guard<std::mutex> lock (mutex_);
    session_ <<
            "UPDATE DbState"
            " SET WritableDb = :writableDb,"
            " ArchiveDb = :archiveDb,"
            " LastRotatedLedger = :lastRotated"
            " WHERE Key = 1;"
            , soci::use (state.writableDb)
            , soci::use (state.archiveDb)
            , soci::use (state.lastRotated)
            ;
}

void
SHAMapStoreImp::SavedStateDB::setLastRotated (LedgerIndex seq)
{
    std::lock_guard<std::mutex> lock (mutex_);
    session_ <<
            "UPDATE DbState SET LastRotatedLedger = :seq"
            " WHERE Key = 1;"
            , soci::use (seq)
            ;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

SHAMapStoreImp::SHAMapStoreImp (
        Application& app,
        Setup const& setup,
        Stoppable& parent,
        NodeStore::Scheduler& scheduler,
        beast::Journal journal,
        beast::Journal nodeStoreJournal,
        TransactionMaster& transactionMaster,
        BasicConfig const& config)
    : SHAMapStore (parent)
    , app_ (app)
    , setup_ (setup)
    , scheduler_ (scheduler)
    , journal_ (journal)
    , nodeStoreJournal_ (nodeStoreJournal)
    , working_(true)
    , transactionMaster_ (transactionMaster)
    , canDelete_ (std::numeric_limits <LedgerIndex>::max())
{
    if (setup_.deleteInterval)
    {
        auto const minInterval = setup.standalone ?
            minimumDeletionIntervalSA_ : minimumDeletionInterval_;
        if (setup_.deleteInterval < minInterval)
        {
            Throw<std::runtime_error> ("online_delete must be at least " +
                std::to_string (minInterval));
        }

        if (setup_.ledgerHistory > setup_.deleteInterval)
        {
            Throw<std::runtime_error> (
                "online_delete must not be less than ledger_history (currently " +
                std::to_string (setup_.ledgerHistory) + ")");
        }

        state_db_.init (config, dbName_);

        dbPaths();
    }
    if (! setup_.shardDatabase.empty())
    {
//节点和碎片存储必须使用
//相同的最早分类帐序列
        std::array<std::uint32_t, 2> seq;
        if (get_if_exists<std::uint32_t>(
            setup_.nodeDatabase, "earliest_seq", seq[0]))
        {
            if (get_if_exists<std::uint32_t>(
                setup_.shardDatabase, "earliest_seq", seq[1]) &&
                seq[0] != seq[1])
            {
                Throw<std::runtime_error>("earliest_seq set more than once");
            }
        }

        boost::filesystem::path dbPath =
            get<std::string>(setup_.shardDatabase, "path");
        if (dbPath.empty())
            Throw<std::runtime_error>("shard path missing");
        if (boost::filesystem::exists(dbPath))
        {
            if (! boost::filesystem::is_directory(dbPath))
                Throw<std::runtime_error>("shard db path must be a directory.");
        }
        else
            boost::filesystem::create_directories(dbPath);

        auto const maxDiskSpace = get<std::uint64_t>(
            setup_.shardDatabase, "max_size_gb", 0);
//必须足够大才能放一块碎片
        if (maxDiskSpace < 3)
            Throw<std::runtime_error>("max_size_gb too small");
        if ((maxDiskSpace << 30) < maxDiskSpace)
            Throw<std::runtime_error>("overflow max_size_gb");

        std::uint32_t lps;
        if (get_if_exists<std::uint32_t>(
                setup_.shardDatabase, "ledgers_per_shard", lps))
        {
//仅在独立测试中设置分类帐
            if (! setup_.standalone)
                Throw<std::runtime_error>(
                    "ledgers_per_shard only honored in stand alone");
        }
    }
}

std::unique_ptr <NodeStore::Database>
SHAMapStoreImp::makeDatabase (std::string const& name,
        std::int32_t readThreads, Stoppable& parent)
{
    std::unique_ptr <NodeStore::Database> db;
    if (setup_.deleteInterval)
    {
        SavedState state = state_db_.getState();
        auto writableBackend = makeBackendRotating(state.writableDb);
        auto archiveBackend = makeBackendRotating(state.archiveDb);
        if (! state.writableDb.size())
        {
            state.writableDb = writableBackend->getName();
            state.archiveDb = archiveBackend->getName();
            state_db_.setState (state);
        }

//使用两个后端创建nodestore以允许在线删除数据
        auto dbr = std::make_unique<NodeStore::DatabaseRotatingImp>(
            "NodeStore.main", scheduler_, readThreads, parent,
                std::move(writableBackend), std::move(archiveBackend),
                    setup_.nodeDatabase, nodeStoreJournal_);
        fdlimit_ += dbr->fdlimit();
        dbRotating_ = dbr.get();
        db.reset(dynamic_cast<NodeStore::Database*>(dbr.release()));
    }
    else
    {
        db = NodeStore::Manager::instance().make_Database (name, scheduler_,
            readThreads, parent, setup_.nodeDatabase, nodeStoreJournal_);
        fdlimit_ += db->fdlimit();
    }
    return db;
}

std::unique_ptr<NodeStore::DatabaseShard>
SHAMapStoreImp::makeDatabaseShard(std::string const& name,
    std::int32_t readThreads, Stoppable& parent)
{
    std::unique_ptr<NodeStore::DatabaseShard> db;
    if(! setup_.shardDatabase.empty())
    {
        db = std::make_unique<NodeStore::DatabaseShardImp>(
            app_, name, parent, scheduler_, readThreads,
                setup_.shardDatabase, app_.journal("ShardStore"));
        if (db->init())
            fdlimit_ += db->fdlimit();
        else
            db.reset();
    }
    return db;
}

void
SHAMapStoreImp::onLedgerClosed(
    std::shared_ptr<Ledger const> const& ledger)
{
    {
        std::lock_guard <std::mutex> lock (mutex_);
        newLedger_ = ledger;
        working_ = true;
    }
    cond_.notify_one();
}

void
SHAMapStoreImp::rendezvous() const
{
    if (!working_)
        return;

    std::unique_lock <std::mutex> lock(mutex_);
    rendezvous_.wait(lock, [&]
    {
        return !working_;
    });
}

int
SHAMapStoreImp::fdlimit () const
{
    return fdlimit_;
}

bool
SHAMapStoreImp::copyNode (std::uint64_t& nodeCount,
        SHAMapAbstractNode const& node)
{
//将单个记录从节点复制到dbrotating\u
    dbRotating_->fetch(node.getNodeHash().as_uint256(), node.getSeq());
    if (! (++nodeCount % checkHealthInterval_))
    {
        if (health())
            return false;
    }

    return true;
}

void
SHAMapStoreImp::run()
{
    beast::setCurrentThreadName ("SHAMapStore");
    LedgerIndex lastRotated = state_db_.getState().lastRotated;
    netOPs_ = &app_.getOPs();
    ledgerMaster_ = &app_.getLedgerMaster();
    fullBelowCache_ = &app_.family().fullbelow();
    treeNodeCache_ = &app_.family().treecache();
    transactionDb_ = &app_.getTxnDB();
    ledgerDb_ = &app_.getLedgerDB();

    if (setup_.advisoryDelete)
        canDelete_ = state_db_.getCanDelete ();

    while (1)
    {
        healthy_ = true;
        std::shared_ptr<Ledger const> validatedLedger;

        {
            std::unique_lock <std::mutex> lock (mutex_);
            working_ = false;
            rendezvous_.notify_all();
            if (stop_)
            {
                stopped();
                return;
            }
            cond_.wait (lock);
            if (newLedger_)
            {
                validatedLedger = std::move(newLedger_);
            }
            else
                continue;
        }

        LedgerIndex validatedSeq = validatedLedger->info().seq;
        if (!lastRotated)
        {
            lastRotated = validatedSeq;
            state_db_.setLastRotated (lastRotated);
        }

//最多删除（不包括）上次旋转）
        if (validatedSeq >= lastRotated + setup_.deleteInterval
                && canDelete_ >= lastRotated - 1)
        {
            JLOG(journal_.debug()) << "rotating  validatedSeq " << validatedSeq
                    << " lastRotated " << lastRotated << " deleteInterval "
                    << setup_.deleteInterval << " canDelete_ " << canDelete_;

            switch (health())
            {
                case Health::stopping:
                    stopped();
                    return;
                case Health::unhealthy:
                    continue;
                case Health::ok:
                default:
                    ;
            }

            clearPrior (lastRotated);
            switch (health())
            {
                case Health::stopping:
                    stopped();
                    return;
                case Health::unhealthy:
                    continue;
                case Health::ok:
                default:
                    ;
            }

            std::uint64_t nodeCount = 0;
            validatedLedger->stateMap().snapShot (
                    false)->visitNodes (
                    std::bind (&SHAMapStoreImp::copyNode, this,
                    std::ref(nodeCount), std::placeholders::_1));
            JLOG(journal_.debug()) << "copied ledger " << validatedSeq
                    << " nodecount " << nodeCount;
            switch (health())
            {
                case Health::stopping:
                    stopped();
                    return;
                case Health::unhealthy:
                    continue;
                case Health::ok:
                default:
                    ;
            }

            freshenCaches();
            JLOG(journal_.debug()) << validatedSeq << " freshened caches";
            switch (health())
            {
                case Health::stopping:
                    stopped();
                    return;
                case Health::unhealthy:
                    continue;
                case Health::ok:
                default:
                    ;
            }

            auto newBackend = makeBackendRotating();
            JLOG(journal_.debug()) << validatedSeq << " new backend "
                    << newBackend->getName();

            clearCaches (validatedSeq);
            switch (health())
            {
                case Health::stopping:
                    stopped();
                    return;
                case Health::unhealthy:
                    continue;
                case Health::ok:
                default:
                    ;
            }

            std::string nextArchiveDir =
                dbRotating_->getWritableBackend()->getName();
            lastRotated = validatedSeq;
            std::unique_ptr<NodeStore::Backend> oldBackend;
            {
                std::lock_guard <std::mutex> lock (dbRotating_->peekMutex());

                state_db_.setState (SavedState {newBackend->getName(),
                        nextArchiveDir, lastRotated});
                clearCaches (validatedSeq);
                oldBackend = dbRotating_->rotateBackends(
                    std::move(newBackend));
            }
            JLOG(journal_.debug()) << "finished rotation " << validatedSeq;

            oldBackend->setDeletePath();
        }
    }
}

void
SHAMapStoreImp::dbPaths()
{
    boost::filesystem::path dbPath =
            get<std::string>(setup_.nodeDatabase, "path");

    if (boost::filesystem::exists (dbPath))
    {
        if (! boost::filesystem::is_directory (dbPath))
        {
            journal_.error() << "node db path must be a directory. "
                    << dbPath.string();
            Throw<std::runtime_error> (
                    "node db path must be a directory.");
        }
    }
    else
    {
        boost::filesystem::create_directories (dbPath);
    }

    SavedState state = state_db_.getState();
    bool writableDbExists = false;
    bool archiveDbExists = false;

    for (boost::filesystem::directory_iterator it (dbPath);
            it != boost::filesystem::directory_iterator(); ++it)
    {
        if (! state.writableDb.compare (it->path().string()))
            writableDbExists = true;
        else if (! state.archiveDb.compare (it->path().string()))
            archiveDbExists = true;
        else if (! dbPrefix_.compare (it->path().stem().string()))
            boost::filesystem::remove_all (it->path());
    }

    if ((!writableDbExists && state.writableDb.size()) ||
            (!archiveDbExists && state.archiveDb.size()) ||
            (writableDbExists != archiveDbExists) ||
            state.writableDb.empty() != state.archiveDb.empty())
    {
        boost::filesystem::path stateDbPathName = setup_.databasePath;
        stateDbPathName /= dbName_;
        stateDbPathName += "*";

        journal_.error() << "state db error: " << std::endl
                << "  writableDbExists " << writableDbExists
                << " archiveDbExists " << archiveDbExists << std::endl
                << "  writableDb '" << state.writableDb
                << "' archiveDb '" << state.archiveDb << "'"
                << std::endl << std::endl
                << "To resume operation, make backups of and "
                << "remove the files matching "
                << stateDbPathName.string()
                << " and contents of the directory "
                << get<std::string>(setup_.nodeDatabase, "path")
                << std::endl;

        Throw<std::runtime_error> ("state db error");
    }
}

std::unique_ptr <NodeStore::Backend>
SHAMapStoreImp::makeBackendRotating (std::string path)
{
    boost::filesystem::path newPath;
    Section parameters = setup_.nodeDatabase;

    if (path.size())
    {
        newPath = path;
    }
    else
    {
        boost::filesystem::path p = get<std::string>(parameters, "path");
        p /= dbPrefix_;
        p += ".%%%%";
        newPath = boost::filesystem::unique_path (p);
    }
    parameters.set("path", newPath.string());

    auto backend {NodeStore::Manager::instance().make_Backend(
        parameters, scheduler_, nodeStoreJournal_)};
    backend->open();
    return backend;
}

bool
SHAMapStoreImp::clearSql (DatabaseCon& database,
        LedgerIndex lastRotated,
        std::string const& minQuery,
        std::string const& deleteQuery)
{
    LedgerIndex min = std::numeric_limits <LedgerIndex>::max();

    {
        auto db = database.checkoutDb ();
        boost::optional<std::uint64_t> m;
        *db << minQuery, soci::into(m);
        if (!m)
            return false;
        min = *m;
    }

    if(min > lastRotated || health() != Health::ok)
        return false;

    boost::format formattedDeleteQuery (deleteQuery);

    JLOG(journal_.debug()) <<
        "start: " << deleteQuery << " from " << min << " to " << lastRotated;
    while (min < lastRotated)
    {
        min = std::min(lastRotated, min + setup_.deleteBatch);
        {
            auto db =  database.checkoutDb ();
            *db << boost::str (formattedDeleteQuery % min);
        }
        if (health())
            return true;
        if (min < lastRotated)
            std::this_thread::sleep_for (
                    std::chrono::milliseconds (setup_.backOff));
    }
    JLOG(journal_.debug()) << "finished: " << deleteQuery;
    return true;
}

void
SHAMapStoreImp::clearCaches (LedgerIndex validatedSeq)
{
    ledgerMaster_->clearLedgerCachePrior (validatedSeq);
    fullBelowCache_->clear();
}

void
SHAMapStoreImp::freshenCaches()
{
    if (freshenCache (dbRotating_->getPositiveCache()))
        return;
    if (freshenCache (*treeNodeCache_))
        return;
    if (freshenCache (transactionMaster_.getCache()))
        return;
}

void
SHAMapStoreImp::clearPrior (LedgerIndex lastRotated)
{
    if (health())
        return;

    ledgerMaster_->clearPriorLedgers (lastRotated);
    if (health())
        return;

    clearSql (*ledgerDb_, lastRotated,
        "SELECT MIN(LedgerSeq) FROM Ledgers;",
        "DELETE FROM Ledgers WHERE LedgerSeq < %u;");
    if (health())
        return;

    {
        /*
            迁移步骤：
            假设：在线删除=100，上次旋转=1000，
                上次关机是在分类帐1080。
                当前网络验证的分类帐为1090。
            暗示：分类账的分录范围从900到1080。
                验证包含所有1080个分类账的条目，
                包括未包括的孤立验证
                在已验证的分类帐中。
            1）列是在使用默认空值的验证中创建的。
            2）同步期间，1080-1090的分类帐和验证
               从网络接收。记录创建于
               初始值约为1080（精确值）的验证
               没关系），稍后用匹配验证
               LedgerSeq值。
            3）波纹参与了分类账1091-1100。验证
                在该范围内，使用InitialEq创建接收，以及
                适当的分类帐。也许有些分类账是
                不接受，因此LedgerSeq保持为空。
            4）在分类帐1100上，使用
                上次旋转=1000。第一个查询试图删除
                LedgerSeq<1000的行。它找不到任何东西。
            5）第二轮删除不运行。
            6）如前所述，分类账继续从1100-1200预付。
                在步骤3中。
            7）在分类帐1200上，此函数再次使用
                上次旋转=1100。第一个查询再次尝试删除
                LedgerSeq<1100的行。它找到1080-1099的行。
            8）第二轮删除运行。它得到
                其中v.ledgerseq为空，并且
                （V.initialEq为空或V.initialEq<1100）
                找到的行包括（a）所有验证
                对于前1080个分类帐。（b）任何孤儿验证
                在步骤3中创建。
            9）继续。下一个旋转周期与步骤相同
                7和8，但不存在任何原始验证（8a）
                现在，8b从第6步得到孤儿。
        **/


        static auto anyValDeleted = false;
        auto const valDeleted = clearSql(*ledgerDb_, lastRotated,
            "SELECT MIN(LedgerSeq) FROM Validations;",
            "DELETE FROM Validations WHERE LedgerSeq < %u;");
        anyValDeleted |= valDeleted;

        if (health())
            return;

        if (anyValDeleted)
        {
            /*删除旧的空LedgerSeqs-验证
               不会链接到已验证的分类帐-但仅当我们
               在匹配的“clearsql”调用中删除了行，并且仅
               对于那些用旧的首字母eq创建的。
            **/

            using namespace std::chrono;
            auto const deleteBatch = setup_.deleteBatch;
            auto const continueLimit = (deleteBatch + 1) / 2;

            std::string const deleteQuery(
                R"sql(DELETE FROM Validations
                WHERE LedgerHash IN
                (
                    SELECT v.LedgerHash
                    FROM Validations v
                    WHERE v.LedgerSeq is NULL AND
                        (v.InitialSeq IS NULL OR v.InitialSeq < )sql" +
                    std::to_string(lastRotated) +
                    ") LIMIT " +
                    std::to_string (deleteBatch) +
                ");");

            JLOG(journal_.debug()) << "start: " << deleteQuery << " of "
                << deleteBatch << " rows.";
            long long totalRowsAffected = 0;
            long long rowsAffected;
            auto st = [&]
            {
                auto db = ledgerDb_->checkoutDb();
                return soci::statement(db->prepare << deleteQuery);
            }();
            if (health())
                return;
            do
            {
                {
                    auto db = ledgerDb_->checkoutDb();
                    auto const start = high_resolution_clock::now();
                    st.execute(true);
                    rowsAffected = st.get_affected_rows();
                    totalRowsAffected += rowsAffected;
                    auto const ms = duration_cast<milliseconds>(
                        high_resolution_clock::now() - start).count();
                    JLOG(journal_.trace()) << "step: deleted " << rowsAffected
                        << " rows in " << ms << "ms.";
                }
                if (health())
                    return;
                if (rowsAffected >= continueLimit)
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(setup_.backOff));
            }
            while (rowsAffected && rowsAffected >= continueLimit);
            JLOG(journal_.debug()) << "finished: " << deleteQuery << ". Deleted "
                << totalRowsAffected << " rows.";
        }
    }

    if (health())
        return;

    clearSql (*transactionDb_, lastRotated,
        "SELECT MIN(LedgerSeq) FROM Transactions;",
        "DELETE FROM Transactions WHERE LedgerSeq < %u;");
    if (health())
        return;

    clearSql (*transactionDb_, lastRotated,
        "SELECT MIN(LedgerSeq) FROM AccountTransactions;",
        "DELETE FROM AccountTransactions WHERE LedgerSeq < %u;");
    if (health())
        return;
}

SHAMapStoreImp::Health
SHAMapStoreImp::health()
{
    {
        std::lock_guard<std::mutex> lock (mutex_);
        if (stop_)
            return Health::stopping;
    }
    if (!netOPs_)
        return Health::ok;

    NetworkOPs::OperatingMode mode = netOPs_->getOperatingMode();

    auto age = ledgerMaster_->getValidatedLedgerAge();
    if (mode != NetworkOPs::omFULL || age.count() >= setup_.ageThreshold)
    {
        JLOG(journal_.warn()) << "Not deleting. state: " << mode
                               << " age " << age.count()
                               << " age threshold " << setup_.ageThreshold;
        healthy_ = false;
    }

    if (healthy_)
        return Health::ok;
    else
        return Health::unhealthy;
}

void
SHAMapStoreImp::onStop()
{
    if (setup_.deleteInterval)
    {
        {
            std::lock_guard <std::mutex> lock (mutex_);
            stop_ = true;
        }
        cond_.notify_one();
    }
    else
    {
        stopped();
    }
}

void
SHAMapStoreImp::onChildrenStopped()
{
    if (setup_.deleteInterval)
    {
        {
            std::lock_guard <std::mutex> lock (mutex_);
            stop_ = true;
        }
        cond_.notify_one();
    }
    else
    {
        stopped();
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
SHAMapStore::Setup
setup_SHAMapStore (Config const& c)
{
    SHAMapStore::Setup setup;

    setup.standalone = c.standalone();

//获取现有设置并添加一些默认值（如果未指定）：
    setup.nodeDatabase = c.section (ConfigSection::nodeDatabase ());

//这两个参数仅适用于RockSDB。我们想让他们明白
//如果未指定值，则为默认值。
    if (!setup.nodeDatabase.exists ("cache_mb"))
        setup.nodeDatabase.set ("cache_mb", std::to_string (c.getSize (siHashNodeDBCache)));

    if (!setup.nodeDatabase.exists ("filter_bits") && (c.NODE_SIZE >= 2))
        setup.nodeDatabase.set ("filter_bits", "10");

    get_if_exists (setup.nodeDatabase, "online_delete", setup.deleteInterval);

    if (setup.deleteInterval)
        get_if_exists (setup.nodeDatabase, "advisory_delete", setup.advisoryDelete);

    setup.ledgerHistory = c.LEDGER_HISTORY;
    setup.databasePath = c.legacy("database_path");

    get_if_exists (setup.nodeDatabase, "delete_batch", setup.deleteBatch);
    get_if_exists (setup.nodeDatabase, "backOff", setup.backOff);
    get_if_exists (setup.nodeDatabase, "age_threshold", setup.ageThreshold);

    setup.shardDatabase = c.section(ConfigSection::shardDatabase());
    return setup;
}

std::unique_ptr<SHAMapStore>
make_SHAMapStore (Application& app,
        SHAMapStore::Setup const& setup,
        Stoppable& parent,
        NodeStore::Scheduler& scheduler,
        beast::Journal journal,
        beast::Journal nodeStoreJournal,
        TransactionMaster& transactionMaster,
        BasicConfig const& config)
{
    return std::make_unique<SHAMapStoreImp>(app, setup, parent, scheduler,
        journal, nodeStoreJournal, transactionMaster, config);
}

}
