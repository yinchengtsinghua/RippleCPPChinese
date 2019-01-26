
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

#include <ripple/core/impl/Workers.h>
#include <ripple/beast/unit_test.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/core/JobTypes.h>
#include <ripple/json/json_value.h>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace ripple {

/*
 *用于单元测试的虚拟类。
 **/


namespace perf {

class PerfLogTest
    : public PerfLog
{
    void rpcStart(std::string const &method, std::uint64_t requestId) override
    {}

    void rpcFinish(std::string const &method, std::uint64_t requestId) override
    {}

    void rpcError(std::string const &method, std::uint64_t dur) override
    {}

    void jobQueue(JobType const type) override
    {}

    void jobStart(JobType const type,
        std::chrono::microseconds dur,
        std::chrono::time_point<std::chrono::steady_clock> startTime,
        int instance) override
    {}

    void jobFinish(JobType const type, std::chrono::microseconds dur,
        int instance) override
    {}

    Json::Value countersJson() const override
    {
        return Json::Value();
    }

    Json::Value currentJson() const override
    {
        return Json::Value();
    }

    void resizeJobs(int const resize) override
    {}

    void rotate() override
    {}
};

} //珀尔夫

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class Workers_test : public beast::unit_test::suite
{
public:
    struct TestCallback : Workers::Callback
    {
        void processTask(int instance) override
        {
            std::lock_guard<std::mutex> lk{mut};
            if (--count == 0)
                cv.notify_all();
        }

        std::condition_variable cv;
        std::mutex              mut;
        int                     count = 0;
    };

    void testThreads(int const tc1, int const tc2, int const tc3)
    {
        testcase("threadCounts: " + std::to_string(tc1) +
            " -> " + std::to_string(tc2) + " -> " + std::to_string(tc3));

        TestCallback cb;
        std::unique_ptr<perf::PerfLog> perfLog =
            std::make_unique<perf::PerfLogTest>();

        Workers w(cb, *perfLog, "Test", tc1);
        BEAST_EXPECT(w.getNumberOfThreads() == tc1);

        auto testForThreadCount = [this, &cb, &w] (int const threadCount)
        {
//准备回拨。
            cb.count = threadCount;

//执行测试。
            w.setNumberOfThreads(threadCount);
            BEAST_EXPECT(w.getNumberOfThreads() == threadCount);

            for (int i = 0; i < threadCount; ++i)
                w.addTask();

//10秒应该足够在任何系统上完成
//
            using namespace std::chrono_literals;
            std::unique_lock<std::mutex> lk{cb.mut};
            bool const signaled = cb.cv.wait_for(lk, 10s, [&cb]{return cb.count == 0;});
            BEAST_EXPECT(signaled);
            BEAST_EXPECT(cb.count == 0);
        };
        testForThreadCount (tc1);
        testForThreadCount (tc2);
        testForThreadCount (tc3);
        w.pauseAllThreadsAndWait();

//我们最好把所有的工作都做完！
        BEAST_EXPECT(cb.count == 0);
    }

    void run() override
    {
        testThreads( 0, 0,  0);
        testThreads( 1, 0,  1);
        testThreads( 2, 1,  2);
        testThreads( 4, 3,  5);
        testThreads(16, 4, 15);
        testThreads(64, 3, 65);
    }
};

BEAST_DEFINE_TESTSUITE(Workers, core, ripple);

}
