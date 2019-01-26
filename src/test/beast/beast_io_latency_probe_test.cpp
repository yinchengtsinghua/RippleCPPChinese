
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
    版权所有（c）2018 Ripple Labs Inc.

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
#include <ripple/beast/asio/io_latency_probe.h>
#include <ripple/beast/unit_test.h>
#include <beast/test/yield_to.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <chrono>

using namespace std::chrono_literals;

class io_latency_probe_test :
    public beast::unit_test::suite, public beast::test::enable_yield_to
{
    using MyTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

    struct test_sampler
    {
        beast::io_latency_probe <std::chrono::steady_clock> probe_;
        std::vector<std::chrono::steady_clock::duration> durations_;

        test_sampler (
            std::chrono::milliseconds interval,
            boost::asio::io_service& ios)
            : probe_ (interval, ios)
        {
        }

        void
        start()
        {
            probe_.sample (std::ref(*this));
        }

        void
        start_one()
        {
            probe_.sample_one (std::ref(*this));
        }

        void operator() (std::chrono::steady_clock::duration const& elapsed)
        {
            durations_.push_back(elapsed);
        }
    };

    void
    testSampleOne(boost::asio::yield_context& yield)
    {
        testcase << "sample one";
        boost::system::error_code ec;
        test_sampler io_probe {100ms, get_io_service()};
        io_probe.start_one();
        MyTimer timer {get_io_service(), 1s};
        timer.async_wait(yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        BEAST_EXPECT(io_probe.durations_.size() == 1);
        io_probe.probe_.cancel_async();
    }

    void
    testSampleOngoing(boost::asio::yield_context& yield)
    {
        testcase << "sample ongoing";
        boost::system::error_code ec;
        test_sampler io_probe {99ms, get_io_service()};
        io_probe.start();
        MyTimer timer {get_io_service(), 1s};
        timer.async_wait(yield[ec]);
        if(! BEAST_EXPECTS(! ec, ec.message()))
            return;
        auto probes_seen = io_probe.durations_.size();
        BEAST_EXPECTS(probes_seen >=9 && probes_seen <= 11,
            std::string("probe count is ") + std::to_string(probes_seen));
        io_probe.probe_.cancel_async();
//再等一次以冲洗剩余的
//来自工作队列的探测器
        timer.expires_from_now(1s);
        timer.async_wait(yield[ec]);
    }

    void
    testCanceled(boost::asio::yield_context& yield)
    {
        testcase << "canceled";
        test_sampler io_probe {100ms, get_io_service()};
        io_probe.probe_.cancel_async();
        except<std::logic_error>( [&io_probe](){ io_probe.start_one(); });
        except<std::logic_error>( [&io_probe](){ io_probe.start(); });
    }

public:
    void run () override
    {
        yield_to([&](boost::asio::yield_context& yield)
        {
            testSampleOne (yield);
            testSampleOngoing (yield);
            testCanceled (yield);
        });
    }
};

BEAST_DEFINE_TESTSUITE(io_latency_probe, asio, beast);
