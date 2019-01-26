
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

#ifndef RIPPLE_BASICS_PERFLOGIMP_H
#define RIPPLE_BASICS_PERFLOGIMP_H

#include <ripple/basics/chrono.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/core/Stoppable.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/impl/Handler.h>
#include <boost/asio/ip/host_name.hpp>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ripple {
namespace perf {

/*
 *PerfLog的实现类。
 **/

class PerfLogImp
    : public PerfLog, Stoppable
{
    /*
     *跟踪性能计数器和当前执行的任务。
     **/

    struct Counters
    {
    public:
        using MethodStart = std::pair<char const*, steady_time_point>;
        /*
         *RPC性能计数器。
         **/

        struct Rpc
        {
//将所有需要同步的项目保留在一个位置
//以最小化锁定时的复制开销。
            struct Sync
            {
//每次方法启动时计数器，然后
//成功完成或出现异常。
                std::uint64_t started {0};
                std::uint64_t finished {0};
                std::uint64_t errored {0};
//所有已完成和出错方法调用的累计持续时间。
                microseconds duration {0};
            };

            Sync sync;
            mutable std::mutex mut;

            Rpc() = default;

            Rpc(Rpc const& orig)
                : sync (orig.sync)
            {}
        };

        /*
         *作业队列任务性能计数器。
         **/

        struct Jq
        {
//将所有需要同步的项目保留在一个位置
//以最小化锁定时的复制开销。
            struct Sync
            {
//每次作业排队、开始运行时的计数器，
//完成。
                std::uint64_t queued {0};
                std::uint64_t started {0};
                std::uint64_t finished {0};
//所有作业排队和运行时间的累计持续时间。
                microseconds queuedDuration {0};
                microseconds runningDuration {0};
            };

            Sync sync;
            std::string const label;
            mutable std::mutex mut;

            Jq(std::string const& labelArg)
                : label (labelArg)
            {}

            Jq(Jq const& orig)
            : sync (orig.sync)
            , label (orig.label)
            {}
        };

//rpc和jq不需要互斥保护，因为
//在启动更多线程之前创建键和值。
        std::unordered_map<std::string, Rpc> rpc_;
        std::unordered_map<std::underlying_type_t<JobType>, Jq> jq_;
        std::vector<std::pair<JobType, steady_time_point>> jobs_;
        int workers_ {0};
        mutable std::mutex jobsMutex_;
        std::unordered_map<std::uint64_t, MethodStart> methods_;
        mutable std::mutex methodsMutex_;

        Counters(std::vector<char const*> const& labels,
            JobTypes const& jobTypes);
        Json::Value countersJson() const;
        Json::Value currentJson() const;
    };

    Setup const setup_;
    beast::Journal j_;
    std::function<void()> const signalStop_;
    Counters counters_ {ripple::RPC::getHandlerNames(), JobTypes::instance()};
    std::ofstream logFile_;
    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    system_time_point lastLog_;
    std::string const hostname_ {boost::asio::ip::host_name()};
    bool stop_ {false};
    bool rotate_ {false};

    void openLog();
    void run();
    void report();
    void rpcEnd(std::string const& method,
        std::uint64_t const requestId,
        bool finish);

public:
    PerfLogImp(Setup const& setup,
        Stoppable& parent,
        beast::Journal journal,
        std::function<void()>&& signalStop);

    ~PerfLogImp() override;

    void rpcStart(
        std::string const& method, std::uint64_t const requestId) override;

    void rpcFinish(
        std::string const& method,
        std::uint64_t const requestId) override
    {
        rpcEnd(method, requestId, true);
    }

    void rpcError(std::string const& method,
        std::uint64_t const requestId) override
    {
        rpcEnd(method, requestId, false);
    }

    void jobQueue(JobType const type) override;
    void jobStart(
        JobType const type,
        microseconds dur,
        steady_time_point startTime,
        int instance) override;
    void jobFinish(
        JobType const type,
        microseconds dur,
        int instance) override;

    Json::Value
    countersJson() const override
    {
        return counters_.countersJson();
    }

    Json::Value
    currentJson() const override
    {
        return counters_.currentJson();
    }

    void resizeJobs(int const resize) override;
    void rotate() override;

//可停止的
    void onPrepare() override {}

//当应用程序准备启动线程时调用。
    void onStart() override;
//当应用程序开始关闭时调用。
    void onStop() override;
//当所有子可停止对象都已停止时调用。
    void onChildrenStopped() override;
};

} //珀尔夫
} //涟漪

#endif //Ripple_基础\u性能\u h
