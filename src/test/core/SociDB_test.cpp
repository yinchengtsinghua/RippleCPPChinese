
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


#include <ripple/core/ConfigSections.h>
#include <ripple/core/SociDB.h>
#include <ripple/basics/contract.h>
#include <test/jtx/TestSuite.h>
#include <ripple/basics/BasicConfig.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace ripple {
class SociDB_test final : public TestSuite
{
private:
    static void setupSQLiteConfig (BasicConfig& config,
                                   boost::filesystem::path const& dbPath)
    {
        config.overwrite ("sqdb", "backend", "sqlite");
        auto value = dbPath.string ();
        if (!value.empty ())
            config.legacy ("database_path", value);
    }

    static void cleanupDatabaseDir (boost::filesystem::path const& dbPath)
    {
        using namespace boost::filesystem;
        if (!exists (dbPath) || !is_directory (dbPath) || !is_empty (dbPath))
            return;
        remove (dbPath);
    }

    static void setupDatabaseDir (boost::filesystem::path const& dbPath)
    {
        using namespace boost::filesystem;
        if (!exists (dbPath))
        {
            create_directory (dbPath);
            return;
        }

        if (!is_directory (dbPath))
        {
//有人创建了一个文件，我们想把它放到目录中
            Throw<std::runtime_error> (
                "Cannot create directory: " + dbPath.string ());
        }
    }
    static boost::filesystem::path getDatabasePath ()
    {
        return boost::filesystem::current_path () / "socidb_test_databases";
    }

public:
    SociDB_test ()
    {
        try
        {
            setupDatabaseDir (getDatabasePath ());
        }
        catch (std::exception const&)
        {
        }
    }
    ~SociDB_test ()
    {
        try
        {
            cleanupDatabaseDir (getDatabasePath ());
        }
        catch (std::exception const&)
        {
        }
    }
    void testSQLiteFileNames ()
    {
//确认文件得到了正确的解释
        testcase ("sqliteFileNames");
        BasicConfig c;
        setupSQLiteConfig (c, getDatabasePath ());
        std::vector<std::pair<std::string, std::string>> const d (
            {{"peerfinder", ".sqlite"},
             {"state", ".db"},
             {"random", ".db"},
             {"validators", ".sqlite"}});

        for (auto const& i : d)
        {
            SociConfig sc (c, i.first);
            BEAST_EXPECT(boost::ends_with (sc.connectionString (),
                                      i.first + i.second));
        }
    }
    void testSQLiteSession ()
    {
        testcase ("open");
        BasicConfig c;
        setupSQLiteConfig (c, getDatabasePath ());
        SociConfig sc (c, "SociTestDB");
        std::vector<std::string> const stringData (
            {"String1", "String2", "String3"});
        std::vector<int> const intData ({1, 2, 3});
        auto checkValues = [this, &stringData, &intData](soci::session& s)
        {
//检查db中的值
            std::vector<std::string> stringResult (20 * stringData.size ());
            std::vector<int> intResult (20 * intData.size ());
            s << "SELECT StringData, IntData FROM SociTestTable;",
                soci::into (stringResult), soci::into (intResult);
            BEAST_EXPECT(stringResult.size () == stringData.size () &&
                    intResult.size () == intData.size ());
            for (int i = 0; i < stringResult.size (); ++i)
            {
                auto si = std::distance (stringData.begin (),
                                         std::find (stringData.begin (),
                                                    stringData.end (),
                                                    stringResult[i]));
                auto ii = std::distance (
                    intData.begin (),
                    std::find (intData.begin (), intData.end (), intResult[i]));
                BEAST_EXPECT(si == ii && si < stringResult.size ());
            }
        };

        {
            soci::session s;
            sc.open (s);
            s << "CREATE TABLE IF NOT EXISTS SociTestTable ("
                 "  Key                    INTEGER PRIMARY KEY,"
                 "  StringData             TEXT,"
                 "  IntData                INTEGER"
                 ");";

            s << "INSERT INTO SociTestTable (StringData, IntData) VALUES "
                 "(:stringData, :intData);",
                soci::use (stringData), soci::use (intData);
            checkValues (s);
        }
        {
//关闭会话后检查数据库中的值
            soci::session s;
            sc.open (s);
            checkValues (s);
        }
        {
            namespace bfs = boost::filesystem;
//删除数据库
            bfs::path dbPath (sc.connectionString ());
            if (bfs::is_regular_file (dbPath))
                bfs::remove (dbPath);
        }
    }

    void testSQLiteSelect ()
    {
        testcase ("select");
        BasicConfig c;
        setupSQLiteConfig (c, getDatabasePath ());
        SociConfig sc (c, "SociTestDB");
        std::vector<std::uint64_t> const ubid (
            {(std::uint64_t)std::numeric_limits<std::int64_t>::max (), 20, 30});
        std::vector<std::int64_t> const bid ({-10, -20, -30});
        std::vector<std::uint32_t> const uid (
            {std::numeric_limits<std::uint32_t>::max (), 2, 3});
        std::vector<std::int32_t> const id ({-1, -2, -3});

        {
            soci::session s;
            sc.open (s);

            s << "DROP TABLE IF EXISTS STT;";

            s << "CREATE TABLE STT ("
                 "  I              INTEGER,"
                 "  UI             INTEGER UNSIGNED,"
                 "  BI             BIGINT,"
                 "  UBI            BIGINT UNSIGNED"
                 ");";

            s << "INSERT INTO STT (I, UI, BI, UBI) VALUES "
                 "(:id, :idu, :bid, :bidu);",
                soci::use (id), soci::use (uid), soci::use (bid),
                soci::use (ubid);

            try
            {
                std::int32_t ig = 0;
                std::uint32_t uig = 0;
                std::int64_t big = 0;
                std::uint64_t ubig = 0;
                s << "SELECT I, UI, BI, UBI from STT;", soci::into (ig),
                    soci::into (uig), soci::into (big), soci::into (ubig);
                BEAST_EXPECT(ig == id[0] && uig == uid[0] && big == bid[0] &&
                        ubig == ubid[0]);
            }
            catch (std::exception&)
            {
                fail ();
            }
            try
            {
                boost::optional<std::int32_t> ig;
                boost::optional<std::uint32_t> uig;
                boost::optional<std::int64_t> big;
                boost::optional<std::uint64_t> ubig;
                s << "SELECT I, UI, BI, UBI from STT;", soci::into (ig),
                    soci::into (uig), soci::into (big), soci::into (ubig);
                BEAST_EXPECT(*ig == id[0] && *uig == uid[0] && *big == bid[0] &&
                        *ubig == ubid[0]);
            }
            catch (std::exception&)
            {
                fail ();
            }
//使用soci：：row和boost：：tuple时存在太多问题。不要使用
//SOCI行！我有一套解决办法来减少Soci Row的错误发生率
//这些测试以防我试图将soci:：row和boost:：tuple添加回soci。
#if 0
            try
            {
                std::int32_t ig = 0;
                std::uint32_t uig = 0;
                std::int64_t big = 0;
                std::uint64_t ubig = 0;
                soci::row r;
                s << "SELECT I, UI, BI, UBI from STT", soci::into (r);
                ig = r.get<std::int32_t>(0);
                uig = r.get<std::uint32_t>(1);
                big = r.get<std::int64_t>(2);
                ubig = r.get<std::uint64_t>(3);
                BEAST_EXPECT(ig == id[0] && uig == uid[0] && big == bid[0] &&
                        ubig == ubid[0]);
            }
            catch (std::exception&)
            {
                fail ();
            }
            try
            {
                std::int32_t ig = 0;
                std::uint32_t uig = 0;
                std::int64_t big = 0;
                std::uint64_t ubig = 0;
                soci::row r;
                s << "SELECT I, UI, BI, UBI from STT", soci::into (r);
                ig = r.get<std::int32_t>("I");
                uig = r.get<std::uint32_t>("UI");
                big = r.get<std::int64_t>("BI");
                ubig = r.get<std::uint64_t>("UBI");
                BEAST_EXPECT(ig == id[0] && uig == uid[0] && big == bid[0] &&
                        ubig == ubid[0]);
            }
            catch (std::exception&)
            {
                fail ();
            }
            try
            {
                boost::tuple<std::int32_t,
                             std::uint32_t,
                             std::int64_t,
                             std::uint64_t> d;
                s << "SELECT I, UI, BI, UBI from STT", soci::into (d);
                BEAST_EXPECT(get<0>(d) == id[0] && get<1>(d) == uid[0] &&
                        get<2>(d) == bid[0] && get<3>(d) == ubid[0]);
            }
            catch (std::exception&)
            {
                fail ();
            }
#endif
        }
        {
            namespace bfs = boost::filesystem;
//删除数据库
            bfs::path dbPath (sc.connectionString ());
            if (bfs::is_regular_file (dbPath))
                bfs::remove (dbPath);
        }
    }
    void testSQLiteDeleteWithSubselect()
    {
        testcase ("deleteWithSubselect");
        BasicConfig c;
        setupSQLiteConfig (c, getDatabasePath ());
        SociConfig sc (c, "SociTestDB");
        {
            soci::session s;
            sc.open (s);
            const char* dbInit[] = {
                "BEGIN TRANSACTION;",
                "CREATE TABLE Ledgers (                     \
                LedgerHash      CHARACTER(64) PRIMARY KEY,  \
                LedgerSeq       BIGINT UNSIGNED             \
            );",
                "CREATE INDEX SeqLedger ON Ledgers(LedgerSeq);",

                "CREATE TABLE Validations   (  \
                LedgerHash  CHARACTER(64)      \
            );",
                "CREATE INDEX ValidationsByHash ON \
                Validations(LedgerHash);",
                "END TRANSACTION;"};
            int dbInitCount = std::extent<decltype(dbInit)>::value;
            for (int i = 0; i < dbInitCount; ++i)
            {
                s << dbInit[i];
            }
            char lh[65];
            memset (lh, 'a', 64);
            lh[64] = '\0';
            int toIncIndex = 63;
            int const numRows = 16;
            std::vector<std::string> ledgerHashes;
            std::vector<int> ledgerIndexes;
            ledgerHashes.reserve(numRows);
            ledgerIndexes.reserve(numRows);
            for (int i = 0; i < numRows; ++i)
            {
                ++lh[toIncIndex];
                if (lh[toIncIndex] == 'z')
                    --toIncIndex;
                ledgerHashes.emplace_back(lh);
                ledgerIndexes.emplace_back(i);
            }
            s << "INSERT INTO Ledgers (LedgerHash, LedgerSeq) VALUES "
                 "(:lh, :li);",
                soci::use (ledgerHashes), soci::use (ledgerIndexes);
            s << "INSERT INTO Validations (LedgerHash) VALUES "
                 "(:lh);", soci::use (ledgerHashes);

            std::vector<int> ledgersLS (numRows * 2);
            std::vector<std::string> validationsLH (numRows * 2);
            s << "SELECT LedgerSeq FROM Ledgers;", soci::into (ledgersLS);
            s << "SELECT LedgerHash FROM Validations;",
                soci::into (validationsLH);
            BEAST_EXPECT(ledgersLS.size () == numRows &&
                    validationsLH.size () == numRows);
        }
        namespace bfs = boost::filesystem;
//删除数据库
        bfs::path dbPath (sc.connectionString ());
        if (bfs::is_regular_file (dbPath))
            bfs::remove (dbPath);
    }
    void testSQLite ()
    {
        testSQLiteFileNames ();
        testSQLiteSession ();
        testSQLiteSelect ();
        testSQLiteDeleteWithSubselect();
    }
    void run () override
    {
        testSQLite ();
    }
};

BEAST_DEFINE_TESTSUITE(SociDB,core,ripple);

}  //涟漪
