
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

#ifndef RIPPLE_BASICS_PERFLOG_H
#define RIPPLE_BASICS_PERFLOG_H

#include <ripple/core/JobTypes.h>
#include <boost/filesystem.hpp>
#include <ripple/json/json_value.h>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace beast { class Journal; }

namespace ripple {
namespace perf {

/*
 *单例类，用于维护性能计数器和可选的
 *将JSON格式的数据写入不同的日志。它应该存在于
 *应用程序启动的其他对象，以便
 *性能记录。
 **/


class PerfLog
{
public:
    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;
    using steady_time_point = std::chrono::time_point<steady_clock>;
    using system_time_point = std::chrono::time_point<system_clock>;
    using seconds = std::chrono::seconds;
    using milliseconds = std::chrono::milliseconds;
    using microseconds = std::chrono::microseconds;

    /*
     *配置来自rippled.cfg的[perf]部分。
     **/

    struct Setup
    {
        boost::filesystem::path perfLog;
//日志间隔以毫秒为单位，以支持更快的测试。
        milliseconds logInterval {seconds(1)};
    };

    virtual ~PerfLog() = default;

    /*
     *记录RPC调用的开始。
     *
     *@param method rpc命令
     *@param requestid跟踪命令的唯一标识符
     **/

    virtual void rpcStart(std::string const& method,
        std::uint64_t requestId) = 0;

    /*
     *记录成功完成的RPC调用
     *
     *@param method rpc命令
     *@param requestid跟踪命令的唯一标识符
     **/

    virtual void rpcFinish(std::string const& method,
        std::uint64_t requestId) = 0;

    /*
     *记录错误的RPC调用
     *
     *@param method rpc命令
     *@param requestid跟踪命令的唯一标识符
     **/

    virtual void rpcError(std::string const& method,
        std::uint64_t requestId) = 0;

    /*
     *记录排队作业
     *
     *@param类型作业类型
     **/

    virtual void jobQueue(JobType const type) = 0;

    /*
     *日志作业正在执行
     *
     *@param类型作业类型
     *@param dur duration以微秒为单位排队
     *@param starttime执行开始的时间
     *@param instance jobqueue工作线程实例
     **/

    virtual void jobStart(JobType const type,
        microseconds dur,
        steady_time_point startTime,
        int instance) = 0;

    /*
     *日志作业完成
     *
     *@param类型作业类型
     *@param dur duration以微秒为单位运行
     *@param instance jobqueue工作线程实例
     **/

    virtual void jobFinish(JobType const type,
        microseconds dur, int instance) = 0;

    /*
     *在JSON中呈现性能计数器
     *
     *@return counters json对象
     **/

    virtual Json::Value countersJson() const = 0;

    /*
     *在JSON中呈现当前正在执行的作业和RPC调用以及持续时间
     *
     *@返回当前正在执行的作业和RPC调用以及持续时间
     **/

    virtual Json::Value currentJson() const = 0;

    /*
     *确保有足够的空间存储当前执行的每个作业
     *
     *@param调整作业队列工作线程的大小
     **/

    virtual void resizeJobs(int const resize) = 0;

    /*
     *旋转性能日志文件
     **/

    virtual void rotate() = 0;
};

} //珀尔夫

class Section;
class Stoppable;

namespace perf {

PerfLog::Setup setup_PerfLog(Section const& section,
    boost::filesystem::path const& configDir);

std::unique_ptr<PerfLog> make_PerfLog(
    PerfLog::Setup const& setup,
    Stoppable& parent,
    beast::Journal journal,
    std::function<void()>&& signalStop);

} //珀尔夫
} //涟漪

#endif //Ripple_基础知识
