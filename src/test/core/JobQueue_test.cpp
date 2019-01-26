
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
    版权所有（c）2017 Ripple Labs Inc.

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

#include <ripple/core/JobQueue.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx/Env.h>

namespace ripple {
namespace test {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

class JobQueue_test : public beast::unit_test::suite
{
    void testAddJob()
    {
        jtx::Env env {*this};

        JobQueue& jQueue = env.app().getJobQueue();
        {
//addJob（）应运行该作业（并返回true）。
            std::atomic<bool> jobRan {false};
            BEAST_EXPECT (jQueue.addJob (jtCLIENT, "JobAddTest1",
                [&jobRan] (Job&) { jobRan = true; }) == true);

//等待作业运行。
            while (jobRan == false);
        }
        {
//如果jobQueue的jobCounter是join（）ed，我们不应该
//能够添加作业的时间更长（并且调用addJob（）应该
//返回错误）。
            using namespace std::chrono_literals;
            beast::Journal j {env.app().journal ("JobQueue_test")};
            JobCounter& jCounter = jQueue.jobCounter();
            jCounter.join("JobQueue_test", 1s, j);

//作业不应该运行，因此让作业访问此
//堆栈上未受保护的变量应该是完全安全的。
//不推荐用于心脏病患者…
            bool unprotected;
            BEAST_EXPECT (jQueue.addJob (jtCLIENT, "JobAddTest2",
                [&unprotected] (Job&) { unprotected = false; }) == false);
        }
    }

    void testPostCoro()
    {
        jtx::Env env {*this};

        JobQueue& jQueue = env.app().getJobQueue();
        {
//测试重复的post（）s，直到coro完成。
            std::atomic<int> yieldCount {0};
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest1",
                [&yieldCount] (std::shared_ptr<JobQueue::Coro> const& coroCopy)
                {
                    while (++yieldCount < 4)
                        coroCopy->yield();
                });
            BEAST_EXPECT (coro != nullptr);

//等待作业运行并让步。
            while (yieldCount == 0);

//现在重新发布，直到coro说完成。
            int old = yieldCount;
            while (coro->runnable())
            {
                BEAST_EXPECT (coro->post());
                while (old == yieldCount) { }
                coro->join();
                BEAST_EXPECT (++old == yieldCount);
            }
            BEAST_EXPECT (yieldCount == 4);
        }
        {
//测试重复的resume（）s，直到coro完成。
            int yieldCount {0};
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest2",
                [&yieldCount] (std::shared_ptr<JobQueue::Coro> const& coroCopy)
                {
                    while (++yieldCount < 4)
                        coroCopy->yield();
                });
            if (! coro)
            {
//我们没有什么好理由不买科罗，但是我们
//没有它就无法继续。
                BEAST_EXPECT (false);
                return;
            }

//等待作业运行并让步。
            coro->join();

//现在继续，直到CORO说完成为止。
            int old = yieldCount;
            while (coro->runnable())
            {
coro->resume(); //恢复在此线程上同步运行。
                BEAST_EXPECT (++old == yieldCount);
            }
            BEAST_EXPECT (yieldCount == 4);
        }
        {
//如果jobQueue的jobCounter是join（）ed，我们不应该
//更长的时间可以添加CORO（调用postcoro（）应该
//返回错误）。
            using namespace std::chrono_literals;
            beast::Journal j {env.app().journal ("JobQueue_test")};
            JobCounter& jCounter = jQueue.jobCounter();
            jCounter.join("JobQueue_test", 1s, j);

//CORO不应该运行，所以让CORO访问这个
//堆栈上未受保护的变量应该是完全安全的。
//不推荐用于心脏病患者…
            bool unprotected;
            auto const coro = jQueue.postCoro (jtCLIENT, "PostCoroTest3",
                [&unprotected] (std::shared_ptr<JobQueue::Coro> const&)
                { unprotected = false; });
            BEAST_EXPECT (coro == nullptr);
        }
    }

public:
    void run() override
    {
        testAddJob();
        testPostCoro();
    }
};

BEAST_DEFINE_TESTSUITE(JobQueue, core, ripple);

} //测试
} //涟漪
