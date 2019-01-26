
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

#include <ripple/unity/rocksdb.h>
#include <ripple/nodestore/DummyScheduler.h>
#include <ripple/nodestore/Manager.h>
#include <ripple/beast/utility/temp_dir.h>
#include <test/nodestore/TestBase.h>
#include <test/unit_test/SuiteJournal.h>
#include <algorithm>

namespace ripple {
namespace NodeStore {

//测试后端接口
//
class Backend_test : public TestBase
{
public:
    void testBackend (
        std::string const& type,
        std::uint64_t const seedValue,
        int numObjectsToTest = 2000)
    {
        DummyScheduler scheduler;

        testcase ("Backend type=" + type);

        Section params;
        beast::temp_dir tempDir;
        params.set ("type", type);
        params.set ("path", tempDir.path());

        beast::xor_shift_engine rng (seedValue);

//创建批量
        auto batch = createPredictableBatch (
            numObjectsToTest, rng());

        using namespace beast::severities;
        test::SuiteJournal journal ("Backend_test", *this);

        {
//打开后端
            std::unique_ptr <Backend> backend =
                Manager::instance().make_Backend (
                    params, scheduler, journal);
            backend->open();

//编写批次
            storeBatch (*backend, batch);

            {
//读回
                Batch copy;
                fetchCopyOfBatch (*backend, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }

            {
//重新排序并重新阅读副本
                std::shuffle (
                    batch.begin(),
                    batch.end(),
                    rng);
                Batch copy;
                fetchCopyOfBatch (*backend, &copy, batch);
                BEAST_EXPECT(areBatchesEqual (batch, copy));
            }
        }

        {
//重新打开后端
            std::unique_ptr <Backend> backend = Manager::instance().make_Backend (
                params, scheduler, journal);
            backend->open();

//读回
            Batch copy;
            fetchCopyOfBatch (*backend, &copy, batch);
//将源批和目标批规范化
            std::sort (batch.begin (), batch.end (), LessThan{});
            std::sort (copy.begin (), copy.end (), LessThan{});
            BEAST_EXPECT(areBatchesEqual (batch, copy));
        }
    }

//——————————————————————————————————————————————————————————————

    void run () override
    {
        std::uint64_t const seedValue = 50;

        testBackend ("nudb", seedValue);

    #if RIPPLE_ROCKSDB_AVAILABLE
        testBackend ("rocksdb", seedValue);
    #endif

    #ifdef RIPPLE_ENABLE_SQLITE_BACKEND_TESTS
        testBackend ("sqlite", seedValue);
    #endif
    }
};

BEAST_DEFINE_TESTSUITE(Backend,ripple_core,ripple);

}
}
