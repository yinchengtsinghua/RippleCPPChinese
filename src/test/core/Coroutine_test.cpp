
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

#include <ripple/core/JobQueue.h>
#include <test/jtx.h>
#include <chrono>
#include <condition_variable>
#include <mutex>

namespace ripple {
namespace test {

class Coroutine_test : public beast::unit_test::suite
{
public:
    class gate
    {
    private:
        std::condition_variable cv_;
        std::mutex mutex_;
        bool signaled_ = false;

    public:
//线程安全，阻止，直到发出信号或期限到期。
//如果发出信号，则返回“true”。
        template <class Rep, class Period>
        bool
        wait_for(std::chrono::duration<Rep, Period> const& rel_time)
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto b = cv_.wait_for(lk, rel_time, [=]{ return signaled_; });
            signaled_ = false;
            return b;
        }

        void
        signal()
        {
            std::lock_guard<std::mutex> lk(mutex_);
            signaled_ = true;
            cv_.notify_all();
        }
    };

    void
    correct_order()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto& jq = env.app().getJobQueue();
        jq.setThreadCount(0, false);
        gate g1, g2;
        std::shared_ptr<JobQueue::Coro> c;
        jq.postCoro(jtCLIENT, "Coroutine-Test",
            [&](auto const& cr)
            {
                c = cr;
                g1.signal();
                c->yield();
                g2.signal();
            });
        BEAST_EXPECT(g1.wait_for(5s));
        c->join();
        c->post();
        BEAST_EXPECT(g2.wait_for(5s));
    }

    void
    incorrect_order()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto& jq = env.app().getJobQueue();
        jq.setThreadCount(0, false);
        gate g;
        jq.postCoro(jtCLIENT, "Coroutine-Test",
            [&](auto const& c)
            {
                c->post();
                c->yield();
                g.signal();
            });
        BEAST_EXPECT(g.wait_for(5s));
    }

    void
    thread_specific_storage()
    {
        using namespace std::chrono_literals;
        using namespace jtx;
        Env env(*this);
        auto& jq = env.app().getJobQueue();
        jq.setThreadCount(0, true);
        static int const N = 4;
        std::array<std::shared_ptr<JobQueue::Coro>, N> a;

        LocalValue<int> lv(-1);
        BEAST_EXPECT(*lv == -1);

        gate g;
        jq.addJob(jtCLIENT, "LocalValue-Test",
            [&](auto const& job)
            {
                this->BEAST_EXPECT(*lv == -1);
                *lv = -2;
                this->BEAST_EXPECT(*lv == -2);
                g.signal();
            });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(*lv == -1);

        for(int i = 0; i < N; ++i)
        {
            jq.postCoro(jtCLIENT, "Coroutine-Test",
                [&, id = i](auto const& c)
                {
                    a[id] = c;
                    g.signal();
                    c->yield();

                    this->BEAST_EXPECT(*lv == -1);
                    *lv = id;
                    this->BEAST_EXPECT(*lv == id);
                    g.signal();
                    c->yield();

                    this->BEAST_EXPECT(*lv == id);
                });
            BEAST_EXPECT(g.wait_for(5s));
            a[i]->join();
        }
        for(auto const& c : a)
        {
            c->post();
            BEAST_EXPECT(g.wait_for(5s));
            c->join();
        }
        for(auto const& c : a)
        {
            c->post();
            c->join();
        }

        jq.addJob(jtCLIENT, "LocalValue-Test",
            [&](auto const& job)
            {
                this->BEAST_EXPECT(*lv == -2);
                g.signal();
            });
        BEAST_EXPECT(g.wait_for(5s));
        BEAST_EXPECT(*lv == -1);
    }

    void
    run() override
    {
        correct_order();
        incorrect_order();
        thread_specific_storage();
    }
};

BEAST_DEFINE_TESTSUITE(Coroutine,core,ripple);

} //测试
} //涟漪
