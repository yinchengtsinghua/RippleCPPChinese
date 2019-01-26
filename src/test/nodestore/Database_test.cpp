
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

#include <test/nodestore/TestBase.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/beast/utility/temp_dir.h>
#include <test/unit_test/SuiteJournal.h>

namespace ripple {
namespace NodeStore {

class Database_test : public TestBase
{
    test::SuiteJournal journal_;

public:
    Database_test ()
    : journal_ ("Database_test", *this)
    { }

    void testImport (std::string const& destBackendType,
        std::string const& srcBackendType, std::int64_t seedValue)
    {
        DummyScheduler scheduler;
        RootStoppable parent ("TestRootStoppable");

        beast::temp_dir node_db;
        Section srcParams;
        srcParams.set ("type", srcBackendType);
        srcParams.set ("path", node_db.path());

//创建批量
        auto batch = createPredictableBatch (
            numObjectsToTest, seedValue);

//写入源数据库
        {
            std::unique_ptr <Database> src = Manager::instance().make_Database (
                "test", scheduler, 2, parent, srcParams, journal_);
            storeBatch (*src, batch);
        }

        Batch copy;

        {
//重新打开数据库
            std::unique_ptr <Database> src = Manager::instance().make_Database (
                "test", scheduler, 2, parent, srcParams, journal_);

//设置目标数据库
            beast::temp_dir dest_db;
            Section destParams;
            destParams.set ("type", destBackendType);
            destParams.set ("path", dest_db.path());

            std::unique_ptr <Database> dest = Manager::instance().make_Database (
                "test", scheduler, 2, parent, destParams, journal_);

            testcase ("import into '" + destBackendType +
                "' from '" + srcBackendType + "'");

//做进口
            dest->import (*src);

//获取导入结果
            fetchCopyOfBatch (*dest, &copy, batch);
        }

//将源批和目标批规范化
        std::sort (batch.begin (), batch.end (), LessThan{});
        std::sort (copy.begin (), copy.end (), LessThan{});
        BEAST_EXPECT(areBatchesEqual (batch, copy));
    }

//——————————————————————————————————————————————————————————————

    void testNodeStore (std::string const& type,
                        bool const testPersistence,
                        std::int64_t const seedValue,
                        int numObjectsToTest = 2000)
    {
        DummyScheduler scheduler;
        RootStoppable parent ("TestRootStoppable");

        std::string s = "NodeStore backend '" + type + "'";

        testcase (s);

        beast::temp_dir node_db;
        Section nodeParams;
        nodeParams.set ("type", type);
        nodeParams.set ("path", node_db.path());

        beast::xor_shift_engine rng (seedValue);

//创建批量
        auto batch = createPredictableBatch (
            numObjectsToTest, rng());

        {
//打开数据库
            std::unique_ptr <Database> db = Manager::instance().make_Database (
                "test", scheduler, 2, parent, nodeParams, journal_);

//编写批次
            storeBatch (*db, batch);

            {
//读回
                Batch copy;
                fetchCopyOfBatch (*db, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }

            {
//重新排序并重新阅读副本
                std::shuffle (
                    batch.begin(),
                    batch.end(),
                    rng);
                Batch copy;
                fetchCopyOfBatch (*db, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }
        }

        if (testPersistence)
        {
//在没有临时数据库的情况下重新打开数据库
            std::unique_ptr <Database> db = Manager::instance().make_Database (
                "test", scheduler, 2, parent, nodeParams, journal_);

//读回
            Batch copy;
            fetchCopyOfBatch (*db, &copy, batch);

//将源批和目标批规范化
            std::sort (batch.begin (), batch.end (), LessThan{});
            std::sort (copy.begin (), copy.end (), LessThan{});
            BEAST_EXPECT(areBatchesEqual (batch, copy));
        }

        if (type == "memory")
        {
//最早的分类帐序列测试
            {
//验证默认的最早分类帐序列
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
                BEAST_EXPECT(db->earliestSeq() == XRP_LEDGER_EARLIEST_SEQ);
            }

//设置无效的最早分类帐序列
            try
            {
                nodeParams.set("earliest_seq", "0");
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
            }
            catch (std::runtime_error const& e)
            {
                BEAST_EXPECT(std::strcmp(e.what(),
                    "Invalid earliest_seq") == 0);
            }

            {
//设置有效的最早分类帐序列
                nodeParams.set("earliest_seq", "1");
                std::unique_ptr<Database> db =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);

//验证数据库是否使用最早的分类帐序列设置
                BEAST_EXPECT(db->earliestSeq() == 1);
            }


//创建另一个试图再次设置值的数据库
            try
            {
//设置为默认的最早分类帐序列
                nodeParams.set("earliest_seq",
                    std::to_string(XRP_LEDGER_EARLIEST_SEQ));
                std::unique_ptr<Database> db2 =
                    Manager::instance().make_Database(
                        "test", scheduler, 2, parent, nodeParams, journal_);
            }
            catch (std::runtime_error const& e)
            {
                BEAST_EXPECT(std::strcmp(e.what(),
                    "earliest_seq set more than once") == 0);
            }
        }
    }

//——————————————————————————————————————————————————————————————

    void run () override
    {
        std::int64_t const seedValue = 50;

        testNodeStore ("memory", false, seedValue);

//持久后端测试
        {
            testNodeStore ("nudb", true, seedValue);

        #if RIPPLE_ROCKSDB_AVAILABLE
            testNodeStore ("rocksdb", true, seedValue);
        #endif
        }

//进口测试
        {
            testImport ("nudb", "nudb", seedValue);

        #if RIPPLE_ROCKSDB_AVAILABLE
            testImport ("rocksdb", "rocksdb", seedValue);
        #endif

        #if RIPPLE_ENABLE_SQLITE_BACKEND_TESTS
            testImport ("sqlite", "sqlite", seedValue);
        #endif
        }
    }
};

BEAST_DEFINE_TESTSUITE(Database,NodeStore,ripple);

}
}
