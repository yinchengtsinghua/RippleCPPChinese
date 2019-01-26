
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated"
#endif


#include <ripple/basics/contract.h>
#include <ripple/basics/ByteUtilities.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/core/SociDB.h>
#include <ripple/core/Config.h>
#include <memory>
#include <soci/sqlite3/soci-sqlite3.h>
#include <boost/filesystem.hpp>

namespace ripple {

static auto checkpointPageCount = 1000;

namespace detail {

std::pair<std::string, soci::backend_factory const&>
getSociSqliteInit (std::string const& name,
                   std::string const& dir,
                   std::string const& ext)
{
    if (name.empty ())
    {
        Throw<std::runtime_error> (
            "Sqlite databases must specify a dir and a name. Name: " +
                name + " Dir: " + dir);
    }
    boost::filesystem::path file (dir);
    if (is_directory (file))
        file /= name + ext;
    return std::make_pair (file.string (), std::ref (soci::sqlite3));
}

std::pair<std::string, soci::backend_factory const&>
getSociInit (BasicConfig const& config,
             std::string const& dbName)
{
    auto const& section = config.section ("sqdb");
    auto const backendName = get(section, "backend", "sqlite");

    if (backendName != "sqlite")
        Throw<std::runtime_error> ("Unsupported soci backend: " + backendName);

    auto const path = config.legacy ("database_path");
    auto const ext = dbName == "validators" || dbName == "peerfinder"
                ? ".sqlite" : ".db";
    return detail::getSociSqliteInit(dbName, path, ext);
}

} //细节

SociConfig::SociConfig (
    std::pair<std::string, soci::backend_factory const&> init)
    : connectionString_ (std::move (init.first)),
      backendFactory_ (init.second)
{
}

SociConfig::SociConfig (BasicConfig const& config, std::string const& dbName)
    : SociConfig (detail::getSociInit (config, dbName))
{
}

std::string SociConfig::connectionString () const
{
    return connectionString_;
}

void SociConfig::open (soci::session& s) const
{
    s.open (backendFactory_, connectionString ());
}

void open  (soci::session& s,
            BasicConfig const& config,
            std::string const& dbName)
{
    SociConfig (config, dbName).open(s);
}

void open (soci::session& s,
           std::string const& beName,
           std::string const& connectionString)
{
    if (beName == "sqlite")
        s.open(soci::sqlite3, connectionString);
    else
        Throw<std::runtime_error> ("Unsupported soci backend: " + beName);
}

static
sqlite_api::sqlite3* getConnection (soci::session& s)
{
    sqlite_api::sqlite3* result = nullptr;
    auto be = s.get_backend ();
    if (auto b = dynamic_cast<soci::sqlite3_session_backend*> (be))
        result = b->conn_;

    if (! result)
        Throw<std::logic_error> ("Didn't get a database connection.");

    return result;
}

size_t getKBUsedAll (soci::session& s)
{
    if (! getConnection (s))
        Throw<std::logic_error> ("No connection found.");
    return static_cast <size_t> (sqlite_api::sqlite3_memory_used () / kilobytes(1));
}

size_t getKBUsedDB (soci::session& s)
{
//当添加其他后端时，必须自定义此功能
    if (auto conn = getConnection (s))
    {
        int cur = 0, hiw = 0;
        sqlite_api::sqlite3_db_status (
            conn, SQLITE_DBSTATUS_CACHE_USED, &cur, &hiw, 0);
        return cur / kilobytes(1);
    }
    Throw<std::logic_error> ("");
return 0; //静默编译器警告。
}

void convert (soci::blob& from, std::vector<std::uint8_t>& to)
{
    to.resize (from.get_len ());
    if (to.empty ())
        return;
    from.read (0, reinterpret_cast<char*>(&to[0]), from.get_len ());
}

void convert (soci::blob& from, std::string& to)
{
    std::vector<std::uint8_t> tmp;
    convert (from, tmp);
    to.assign (tmp.begin (), tmp.end());
}

void convert (std::vector<std::uint8_t> const& from, soci::blob& to)
{
    if (!from.empty ())
        to.write (0, reinterpret_cast<char const*>(&from[0]), from.size ());
    else
        to.trim (0);
}

void convert (std::string const& from, soci::blob& to)
{
    if (!from.empty ())
        to.write (0, from.data (), from.size ());
    else
        to.trim (0);
}

namespace {

/*运行一个线程检查用于
    给定的soci:：session每1000页。这只是实现
    用于sqlite数据库。

    注：根据：https://www.sqlite.org/wal.html ckpt
    是SQLite的默认行为。我们可以把这个拿走
    班级。
**/

class WALCheckpointer : public Checkpointer
{
public:
    WALCheckpointer (sqlite_api::sqlite3& conn, JobQueue& q, Logs& logs)
            : conn_ (conn), jobQueue_ (q), j_ (logs.journal ("WALCheckpointer"))
    {
        sqlite_api::sqlite3_wal_hook (&conn_, &sqliteWALHook, this);
    }

    ~WALCheckpointer () override = default;

private:
    sqlite_api::sqlite3& conn_;
    std::mutex mutex_;
    JobQueue& jobQueue_;

    bool running_ = false;
    beast::Journal j_;

    static
    int sqliteWALHook (
        void* cp, sqlite_api::sqlite3*, const char* dbName, int walSize)
    {
        if (walSize >= checkpointPageCount)
        {
            if (auto checkpointer = reinterpret_cast <WALCheckpointer*> (cp))
                checkpointer->scheduleCheckpoint();
            else
                Throw<std::logic_error> ("Didn't get a WALCheckpointer");
        }
        return SQLITE_OK;
    }

    void scheduleCheckpoint ()
    {
        {
            std::lock_guard <std::mutex> lock (mutex_);
            if (running_)
                return;
            running_ = true;
        }

//如果作业没有添加到作业队列中，那么我们就没有运行。
        if (! jobQueue_.addJob (
            jtWAL, "WAL", [this] (Job&) { checkpoint(); }))
        {
            std::lock_guard <std::mutex> lock (mutex_);
            running_ = false;
        }
    }

    void checkpoint ()
    {
        int log = 0, ckpt = 0;
        int ret = sqlite3_wal_checkpoint_v2 (
            &conn_, nullptr, SQLITE_CHECKPOINT_PASSIVE, &log, &ckpt);

        auto fname = sqlite3_db_filename (&conn_, "main");
        if (ret != SQLITE_OK)
        {
            auto jm = (ret == SQLITE_LOCKED) ? j_.trace() : j_.warn();
            JLOG (jm)
                << "WAL(" << fname << "): error " << ret;
        }
        else
        {
            JLOG (j_.trace())
                << "WAL(" << fname << "): frames="
                << log << ", written=" << ckpt;
        }

        std::lock_guard <std::mutex> lock (mutex_);
        running_ = false;
    }
};

} //命名空间

std::unique_ptr <Checkpointer> makeCheckpointer (
    soci::session& session, JobQueue& queue, Logs& logs)
{
    if (auto conn = getConnection (session))
        return std::make_unique <WALCheckpointer> (*conn, queue, logs);
    return {};
}

}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
