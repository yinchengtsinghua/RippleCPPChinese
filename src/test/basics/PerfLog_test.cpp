
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

#include <ripple/basics/PerfLog.h>
#include <ripple/basics/random.h>
#include <ripple/beast/unit_test.h>
#include <ripple/json/json_reader.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/impl/Handler.h>
#include <test/jtx/Env.h>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace ripple {

class PerfLog_test : public beast::unit_test::suite
{
    enum class WithFile : bool
    {
        no = false,
        yes = true
    };

    using path = boost::filesystem::path;

//我们只是在使用env作为它的日志。那本日记写得更好
//单元测试的覆盖范围。
    test::jtx::Env env_ {*this};
    beast::Journal j_ {env_.app().journal ("PerfLog_test")};

//PerfLog需要一个可停止的父级和一个函数来
//如果要关闭系统，请调用。这个类同时提供这两种功能。
    struct PerfLogParent : public RootStoppable
    {
        bool stopSignaled {false};
        beast::Journal j_;

        explicit PerfLogParent(beast::Journal const& j)
        : RootStoppable ("testRootStoppable")
        , j_ (j)
        { }

        ~PerfLogParent() override
        {
            doStop();
            cleanupPerfLogDir();
        }

//应用程序：：SignalStop（）的良性替换。
        void signalStop()
        {
            stopSignaled = true;
        }

    private:
//rootstoppable的接口。私有化以鼓励使用
//DoStart（）和DoStop（）的。
        void onPrepare() override
        {
        }

        void onStart() override
        {
        }

        void onStop() override
        {
            if (areChildrenStopped())
                stopped();
        }

        void onChildrenStopped() override
        {
            onStop();
        }

    public:
//测试启动和停止PerfLog的接口
        void doStart ()
        {
            prepare();
            start();
        }

        void doStop ()
        {
            if (started())
                stop(j_);
        }

//PerfLog文件管理接口
        static path getPerfLogPath()
        {
            using namespace boost::filesystem;
            return temp_directory_path() / "perf_log_test_dir";
        }

        static path getPerfLogFileName()
        {
            return {"perf_log.txt"};
        }

        static std::chrono::milliseconds getLogInterval()
        {
            return std::chrono::milliseconds {10};
        }

        static perf::PerfLog::Setup getSetup (WithFile withFile)
        {
            return perf::PerfLog::Setup
            {
                withFile == WithFile::no ?
                    "" : getPerfLogPath() / getPerfLogFileName(),
                getLogInterval()
            };
        }

        static void cleanupPerfLogDir ()
        {
            using namespace boost::filesystem;

            auto const perfLogPath {getPerfLogPath()};
            auto const fullPath = perfLogPath / getPerfLogFileName();
            if (exists (fullPath))
                remove (fullPath);

            if (!exists (perfLogPath)
                || !is_directory (perfLogPath)
                || !is_empty (perfLogPath))
            {
                return;
            }
            remove (perfLogPath);
        }
    };

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//返回PerfLog的便利函数
    std::unique_ptr<perf::PerfLog> getPerfLog (
        PerfLogParent& parent, WithFile withFile)
    {
        return perf::make_PerfLog (parent.getSetup(withFile), parent, j_,
            [&parent] () { return parent.signalStop(); });
    }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//方便函数返回给定JSON：：值的uint64，该值包含
//一个字符串。
    static std::uint64_t jsonToUint64 (Json::Value const& jsonUintAsString)
    {
        return std::stoull (jsonUintAsString.asString());
    }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//如果
//持续时间从字符串转换为整数。以下结构
//是一种考虑已转换条目的方法。
    struct Cur
    {
        std::uint64_t dur;
        std::string name;

        Cur (std::uint64_t d, std::string n)
        : dur (d)
        , name (std::move (n))
        { }
    };

//将JSON转换为cur和sort的便利函数。排序
//从最长持续到最短持续。那样的事情就开始了
//前面是前面。
    static std::vector<Cur> getSortedCurrent (Json::Value const& currentJson)
    {
        std::vector<Cur> currents;
        currents.reserve (currentJson.size());
        for (Json::Value const& cur : currentJson)
        {
            currents.emplace_back (
                jsonToUint64 (cur[jss::duration_us]),
                cur.isMember (jss::job) ?
                    cur[jss::job].asString() : cur[jss::method].asString());
        }

//请注意，最长持续时间应在
//从他们开始的时候就有了载体。
        std::sort (currents.begin(), currents.end(),
            [] (Cur const& lhs, Cur const& rhs)
            {
                if (lhs.dur != rhs.dur)
                    return (rhs.dur < lhs.dur);
                return (lhs.name < rhs.name);
            });
        return currents;
    }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//帮助程序函数，检查PerfLog文件的大小，然后
//当文件变大时返回。这表示性能日志
//已将新值写入文件，并且\应具有最新的
//更新。
    static void waitForFileUpdate (PerfLogParent const& parent)
    {
        using namespace boost::filesystem;

        auto const path = parent.getPerfLogPath() / parent.getPerfLogFileName();
        if (!exists (path))
            return;

//我们等待文件更改两次大小。第一个文件大小
//当我们到达时，变化可能正在进行中。
        std::uintmax_t const firstSize {file_size (path)};
        std::uintmax_t secondSize {firstSize};
        do
        {
            std::this_thread::sleep_for (parent.getLogInterval());
            secondSize = file_size (path);
        } while (firstSize >= secondSize);

        do
        {
            std::this_thread::sleep_for (parent.getLogInterval());
        } while (secondSize >= file_size (path));
    }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

public:
    void testFileCreation()
    {
        using namespace boost::filesystem;

        auto const perfLogPath {PerfLogParent::getPerfLogPath()};
        auto const fullPath = perfLogPath / PerfLogParent::getPerfLogFileName();
        {
//验证PerfLog在构造时是否创建其文件。
            PerfLogParent parent {j_};
            BEAST_EXPECT(! exists (perfLogPath));

            auto perfLog {getPerfLog (parent, WithFile::yes)};

            BEAST_EXPECT(parent.stopSignaled == false);
            BEAST_EXPECT(exists (perfLogPath));
        }
        {
//创建一个PerfLog想要放置其目录的文件。
//确保PerfLog尝试关闭服务器，因为它
//无法打开其文件。
            PerfLogParent parent {j_};
            if (!BEAST_EXPECT(! exists (perfLogPath)))
                return;

            {
//创建一个阻止PerfLog创建其文件的文件。
                std::ofstream nastyFile;
                nastyFile.open (
                    perfLogPath.c_str(), std::ios::out | std::ios::app);
                if (! BEAST_EXPECT(nastyFile))
                    return;
                nastyFile.close();
            }

//现在构造一个PerfLog。PerfLog应尝试关闭
//关闭服务器，因为它无法打开文件。
            BEAST_EXPECT(parent.stopSignaled == false);
            auto perfLog {getPerfLog (parent, WithFile::yes)};
            BEAST_EXPECT(parent.stopSignaled == true);

//启动PerfLog并为PerfLog:：Report（）等待足够长的时间。
//无法写入其文件。那应该不会导致
//问题。
            parent.doStart();
            std::this_thread::sleep_for (parent.getLogInterval() * 10);
            parent.doStop();

//删除文件。
            remove (perfLogPath);
        }
        {
//将写保护文件放在PerfLog要写入其
//文件。确保PerfLog尝试关闭服务器
//因为它不能打开它的文件。
            PerfLogParent parent {j_};
            if (! BEAST_EXPECT(! exists (perfLogPath)))
                return;

//构造和写入保护文件以防止PerfLog
//创建它的文件。
            boost::system::error_code ec;
            boost::filesystem::create_directories (perfLogPath, ec);
            if (! BEAST_EXPECT(! ec))
                return;

            auto fileWriteable = [](boost::filesystem::path const& p) -> bool
            {
                return std::ofstream {
                    p.c_str(), std::ios::out | std::ios::app}.is_open();
            };

            if (! BEAST_EXPECT(fileWriteable (fullPath)))
                return;

            boost::filesystem::permissions (fullPath,
                perms::remove_perms | perms::owner_write |
                perms::others_write | perms::group_write);

//如果测试作为根目录运行，那么写保护可能
//没有效果。确保写保护在继续之前有效。
            if (fileWriteable (fullPath))
            {
                log << "Unable to write protect file.  Test skipped."
                    << std::endl;
                return;
            }

//现在构造一个PerfLog。PerfLog应尝试关闭
//关闭服务器，因为它无法打开文件。
            BEAST_EXPECT(parent.stopSignaled == false);
            auto perfLog {getPerfLog (parent, WithFile::yes)};
            BEAST_EXPECT(parent.stopSignaled == true);

//启动PerfLog并为PerfLog:：Report（）等待足够长的时间。
//无法写入其文件。那应该不会导致
//问题。
            parent.doStart();
            std::this_thread::sleep_for (parent.getLogInterval() * 10);
            parent.doStop();

//修复文件权限，以便清理文件。
            boost::filesystem::permissions (fullPath,
                perms::add_perms | perms::owner_write |
                perms::others_write | perms::group_write);
        }
    }

    void testRPC (WithFile withFile)
    {
//练习PerfLog的RPC接口。
//启动将用于测试的PerfLog。
        PerfLogParent parent {j_};
        auto perfLog {getPerfLog (parent, withFile)};
        parent.doStart();

//获取所有可以用于RPC接口的标签
//导致断言。
        std::vector<char const*> labels {ripple::RPC::getHandlerNames()};
        std::shuffle (labels.begin(), labels.end(), default_prng());

//获取与每个标签关联的两个ID。错误往往发生在
//边界，所以我们选择从零开始到结束的ID
//std:：uint64_t>：：max（）。
        std::vector<std::uint64_t> ids;
        ids.reserve (labels.size() * 2);
        std::generate_n (std::back_inserter (ids), labels.size(),
            [i = std::numeric_limits<std::uint64_t>::min()]()
            mutable { return i++; });
        std::generate_n (std::back_inserter (ids), labels.size(),
            [i = std::numeric_limits<std::uint64_t>::max()]()
            mutable { return i--; });
        std::shuffle (ids.begin(), ids.end(), default_prng());

//启动所有的rpc命令两次以显示它们都可以被跟踪
//同时。
        for (int labelIndex = 0; labelIndex < labels.size(); ++labelIndex)
        {
            for (int idIndex = 0; idIndex < 2; ++idIndex)
            {
                std::this_thread::sleep_for (std::chrono::microseconds (10));
                perfLog->rpcStart (
                    labels[labelIndex], ids[(labelIndex * 2) + idIndex]);
            }
        }
        {
//检查当前的perflog:：counterjson（）值。
            Json::Value const countersJson {perfLog->countersJson()["rpc"]};
            BEAST_EXPECT(countersJson.size() == labels.size() + 1);
            for (auto& label : labels)
            {
//希望标签中的每个标签都具有相同的内容。
                Json::Value const& counter {countersJson[label]};
                BEAST_EXPECT(counter[jss::duration_us] == "0");
                BEAST_EXPECT(counter[jss::errored] == "0");
                BEAST_EXPECT(counter[jss::finished] == "0");
                BEAST_EXPECT(counter[jss::started] == "2");
            }
//希望“total”有很多“started”
            Json::Value const& total {countersJson[jss::total]};
            BEAST_EXPECT(total[jss::duration_us] == "0");
            BEAST_EXPECT(total[jss::errored] == "0");
            BEAST_EXPECT(total[jss::finished] == "0");
            BEAST_EXPECT(jsonToUint64 (total[jss::started]) == ids.size());
        }
        {
//确认标签中的每个条目在电流中出现两次。
//如果按持续时间排序，它们应该是
//已调用rpcstart（）。
            std::vector<Cur> const currents {
                getSortedCurrent (perfLog->currentJson()[jss::methods])};
            BEAST_EXPECT(currents.size() == labels.size() * 2);

            std::uint64_t prevDur = std::numeric_limits<std::uint64_t>::max();
            for (int i = 0; i < currents.size(); ++i)
            {
                BEAST_EXPECT(currents[i].name == labels[i / 2]);
                BEAST_EXPECT(prevDur > currents[i].dur);
                prevDur = currents[i].dur;
            }
        }

//以相反的顺序完成除第一个rpc命令以外的所有命令，以显示
//命令的开始和结束可以交错进行。一半的
//命令完成正确，另一半有错误。
        for (int labelIndex = labels.size() - 1; labelIndex > 0; --labelIndex)
        {
            std::this_thread::sleep_for (std::chrono::microseconds (10));
            perfLog->rpcFinish (labels[labelIndex], ids[(labelIndex * 2) + 1]);
            std::this_thread::sleep_for (std::chrono::microseconds (10));
            perfLog->rpcError (labels[labelIndex], ids[(labelIndex * 2) + 0]);
        }
        perfLog->rpcFinish (labels[0], ids[0 + 1]);
//请注意，标签[0]ID[0]故意未完成。

        auto validateFinalCounters =
            [this, &labels] (Json::Value const& countersJson)
        {
            {
                Json::Value const& jobQueue = countersJson[jss::job_queue];
                BEAST_EXPECT(jobQueue.isObject());
                BEAST_EXPECT(jobQueue.size() == 0);
            }

            Json::Value const& rpc = countersJson[jss::rpc];
            BEAST_EXPECT(rpc.size() == labels.size() + 1);

//验证标签中的每个条目都出现在RPC中。
//如果我们通过标签访问条目，我们应该能够关联
//它们的持续时间带有适当的标签。
            {
//第一个标签是特殊的。它应该有“errored”：“0”。
                Json::Value const& first = rpc[labels[0]];
                BEAST_EXPECT(first[jss::duration_us] != "0");
                BEAST_EXPECT(first[jss::errored] == "0");
                BEAST_EXPECT(first[jss::finished] == "1");
                BEAST_EXPECT(first[jss::started] == "2");
            }

//检查其余标签。
            std::uint64_t prevDur = std::numeric_limits<std::uint64_t>::max();
            for (int i = 1; i < labels.size(); ++i)
            {
                Json::Value const& counter {rpc[labels[i]]};
                std::uint64_t const dur {
                    jsonToUint64 (counter[jss::duration_us])};
                BEAST_EXPECT(dur != 0 && dur < prevDur);
                prevDur = dur;
                BEAST_EXPECT(counter[jss::errored] == "1");
                BEAST_EXPECT(counter[jss::finished] == "1");
                BEAST_EXPECT(counter[jss::started] == "2");
            }

//检查“总数”
            Json::Value const& total {rpc[jss::total]};
            BEAST_EXPECT(total[jss::duration_us] != "0");
            BEAST_EXPECT(
                jsonToUint64 (total[jss::errored]) == labels.size() - 1);
            BEAST_EXPECT(
                jsonToUint64 (total[jss::finished]) == labels.size());
            BEAST_EXPECT(
                jsonToUint64 (total[jss::started]) == labels.size() * 2);
        };

        auto validateFinalCurrent =
            [this, &labels] (Json::Value const& currentJson)
        {
            {
                Json::Value const& job_queue = currentJson[jss::jobs];
                BEAST_EXPECT(job_queue.isArray());
                BEAST_EXPECT(job_queue.size() == 0);
            }

            Json::Value const& methods = currentJson[jss::methods];
            BEAST_EXPECT(methods.size() == 1);
            BEAST_EXPECT(methods.isArray());

            Json::Value const& only = methods[0u];
            BEAST_EXPECT(only.size() == 2);
            BEAST_EXPECT(only.isObject());
            BEAST_EXPECT(only[jss::duration_us] != "0");
            BEAST_EXPECT(only[jss::method] == labels[0]);
        };

//验证PerfLog的最终状态。
        validateFinalCounters (perfLog->countersJson());
        validateFinalCurrent (perfLog->currentJson());

//给PerfLog足够的时间将其状态刷新到文件中。
        waitForFileUpdate (parent);

//礼貌地停止性能日志。
        parent.doStop();

        auto const fullPath =
            parent.getPerfLogPath() / parent.getPerfLogFileName();

        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (fullPath));
        }
        else
        {
//日志文件的最后一行应包含相同的
//countersjson（）和currentjson（）返回的信息。
//验证这一点。

//获取日志的最后一行。
            std::ifstream log (fullPath.c_str());
            std::string lastLine;
            for (std::string line; std::getline (log, line); )
            {
                if (! line.empty())
                    lastLine = std::move (line);
            }

            Json::Value parsedLastLine;
            Json::Reader ().parse (lastLine, parsedLastLine);
            if (! BEAST_EXPECT(! RPC::contains_error (parsedLastLine)))
//避免故障级联
                return;

//验证日志最后一行的内容。
            validateFinalCounters (parsedLastLine[jss::counters]);
            validateFinalCurrent (parsedLastLine[jss::current_activities]);
        }
    }

    void testJobs (WithFile withFile)
    {
        using namespace std::chrono;

//练习PerfLog的作业接口。
//启动将用于测试的PerfLog。
        PerfLogParent parent {j_};
        auto perfLog {getPerfLog (parent, withFile)};
        parent.doStart();

//获取可用于调用作业接口的所有作业类型
//不会引起断言。
        struct JobName
        {
            JobType type;
            std::string typeName;

            JobName (JobType t, std::string name)
            : type (t)
            , typeName (std::move (name))
            { }
        };

        std::vector<JobName> jobs;
        {
            auto const& jobTypes = JobTypes::instance();
            jobs.reserve (jobTypes.size());
            for (auto const& job : jobTypes)
            {
                jobs.emplace_back (job.first, job.second.name());
            }
        }
        std::shuffle (jobs.begin(), jobs.end(), default_prng());

//完成所有的工作，将每个工作排队一次。检查
//每添加一个作业数据。
        for (int i = 0; i < jobs.size(); ++i)
        {
            perfLog->jobQueue (jobs[i].type);
            Json::Value const jq_counters {
                perfLog->countersJson()[jss::job_queue]};

            BEAST_EXPECT(jq_counters.size() == i + 2);
            for (int j = 0; j<= i; ++j)
            {
//验证所有预期的计数器是否存在并包含
//预期值。
                Json::Value const& counter {jq_counters[jobs[j].typeName]};
                BEAST_EXPECT(counter.size() == 5);
                BEAST_EXPECT(counter[jss::queued] == "1");
                BEAST_EXPECT(counter[jss::started] == "0");
                BEAST_EXPECT(counter[jss::finished] == "0");
                BEAST_EXPECT(counter[jss::queued_duration_us] == "0");
                BEAST_EXPECT(counter[jss::running_duration_us] == "0");
            }

//验证JSS:：total是否存在并具有预期值。
            Json::Value const& total {jq_counters[jss::total]};
            BEAST_EXPECT(total.size() == 5);
            BEAST_EXPECT(jsonToUint64 (total[jss::queued]) == i + 1);
            BEAST_EXPECT(total[jss::started] == "0");
            BEAST_EXPECT(total[jss::finished] == "0");
            BEAST_EXPECT(total[jss::queued_duration_us] == "0");
            BEAST_EXPECT(total[jss::running_duration_us] == "0");
        }

//即使作业已排队，PerfLog也不应报告任何最新信息。
        {
            Json::Value current {perfLog->currentJson()};
            BEAST_EXPECT(current.size() == 2);
            BEAST_EXPECT(current.isMember (jss::jobs));
            BEAST_EXPECT(current[jss::jobs].size() == 0);
            BEAST_EXPECT(current.isMember (jss::methods));
            BEAST_EXPECT(current[jss::methods].size() == 0);
        }

//当前的工作是由工作人员ID跟踪的。即使它不是
//现实点，增加工人的数量，这样我们可以有很多
//同时进行的作业没有问题。
        perfLog->resizeJobs (jobs.size() * 2);

//启动每个作业的两个实例，以显示同一作业可以运行
//同时（在不同的工作线程上）。诚然，这是
//会使jss:：queued count看起来有点呆板，因为
//排队的人数是开始排队人数的一半…
        for (int i = 0; i < jobs.size(); ++i)
        {
            perfLog->jobStart (
                jobs[i].type, microseconds {i+1}, steady_clock::now(), i * 2);
            std::this_thread::sleep_for (microseconds (10));

//检查每个作业类型计数器条目。
            Json::Value const jq_counters {
                perfLog->countersJson()[jss::job_queue]};
            for (int j = 0; j< jobs.size(); ++j)
            {
                Json::Value const& counter {jq_counters[jobs[j].typeName]};
                std::uint64_t const queued_dur_us {
                    jsonToUint64 (counter[jss::queued_duration_us])};
                if (j < i)
                {
                    BEAST_EXPECT(counter[jss::started] == "2");
                    BEAST_EXPECT(queued_dur_us == j + 1);
                }
                else if (j == i)
                {
                    BEAST_EXPECT(counter[jss::started] == "1");
                    BEAST_EXPECT(queued_dur_us == j + 1);
                }
                else
                {
                    BEAST_EXPECT(counter[jss::started] == "0");
                    BEAST_EXPECT(queued_dur_us == 0);
                }

                BEAST_EXPECT(counter[jss::queued] == "1");
                BEAST_EXPECT(counter[jss::finished] == "0");
                BEAST_EXPECT(counter[jss::running_duration_us] == "0");
            }
            {
//验证jss:：total中的值是否符合我们的预期。
                Json::Value const& total {jq_counters[jss::total]};
                BEAST_EXPECT(jsonToUint64 (total[jss::queued]) == jobs.size());
                BEAST_EXPECT(
                    jsonToUint64 (total[jss::started]) == (i * 2) + 1);
                BEAST_EXPECT(total[jss::finished] == "0");

//排队的总持续时间是（i+1）的三角形数。
                BEAST_EXPECT(jsonToUint64 (
                    total[jss::queued_duration_us]) == (((i*i) + 3*i + 2) / 2));
                BEAST_EXPECT(total[jss::running_duration_us] == "0");
            }

            perfLog->jobStart (jobs[i].type,
                microseconds {0}, steady_clock::now(), (i * 2) + 1);
            std::this_thread::sleep_for (microseconds {10});

//验证作业中的每个条目在当前状态下出现两次。
//如果按持续时间排序，它们应该是
//已调用rpcstart（）。
            std::vector<Cur> const currents {
                getSortedCurrent (perfLog->currentJson()[jss::jobs])};
            BEAST_EXPECT(currents.size() == (i + 1) * 2);

            std::uint64_t prevDur = std::numeric_limits<std::uint64_t>::max();
            for (int j = 0; j <= i; ++j)
            {
                BEAST_EXPECT(currents[j * 2].name == jobs[j].typeName);
                BEAST_EXPECT(prevDur > currents[j * 2].dur);
                prevDur = currents[j * 2].dur;

                BEAST_EXPECT(currents[(j * 2) + 1].name == jobs[j].typeName);
                BEAST_EXPECT(prevDur > currents[(j * 2) + 1].dur);
                prevDur = currents[(j * 2) + 1].dur;
            }
        }

//完成我们开始的每一项工作。把它们倒过来。
        for (int i = jobs.size() - 1; i >= 0; --i)
        {
//这个循环中的许多计算都关注
//已完成的作业数。把它准备好。
            int const finished = ((jobs.size() - i) * 2) - 1;
            perfLog->jobFinish (
                jobs[i].type, microseconds (finished), (i * 2) + 1);
            std::this_thread::sleep_for (microseconds (10));

            Json::Value const jq_counters {
                perfLog->countersJson()[jss::job_queue]};
            for (int j = 0; j < jobs.size(); ++j)
            {
                Json::Value const& counter {jq_counters[jobs[j].typeName]};
                std::uint64_t const running_dur_us {
                    jsonToUint64 (counter[jss::running_duration_us])};
                if (j < i)
                {
                    BEAST_EXPECT(counter[jss::finished] == "0");
                    BEAST_EXPECT(running_dur_us == 0);
                }
                else if (j == i)
                {
                    BEAST_EXPECT(counter[jss::finished] == "1");
                    BEAST_EXPECT(running_dur_us == ((jobs.size() - j) * 2) - 1);
                }
                else
                {
                    BEAST_EXPECT(counter[jss::finished] == "2");
                    BEAST_EXPECT(running_dur_us == ((jobs.size() - j) * 4) - 1);
                }

                std::uint64_t const queued_dur_us {
                    jsonToUint64 (counter[jss::queued_duration_us])};
                BEAST_EXPECT(queued_dur_us == j + 1);
                BEAST_EXPECT(counter[jss::queued] == "1");
                BEAST_EXPECT(counter[jss::started] == "2");
            }
            {
//验证jss:：total中的值是否符合我们的预期。
                Json::Value const& total {jq_counters[jss::total]};
                BEAST_EXPECT(jsonToUint64 (total[jss::queued]) == jobs.size());
                BEAST_EXPECT(
                    jsonToUint64 (total[jss::started]) == jobs.size() * 2);
                BEAST_EXPECT(
                    jsonToUint64 (total[jss::finished]) == finished);

//排队总持续时间应为三角形
//作业。
                int const queuedDur = ((jobs.size() * (jobs.size() + 1)) / 2);
                BEAST_EXPECT(
                    jsonToUint64 (total[jss::queued_duration_us]) == queuedDur);

//总运行时间应为完成的三角形数。
                int const runningDur = ((finished * (finished + 1)) / 2);
                BEAST_EXPECT(jsonToUint64 (
                    total[jss::running_duration_us]) == runningDur);
            }

            perfLog->jobFinish (
                jobs[i].type, microseconds (finished + 1), (i * 2));
            std::this_thread::sleep_for (microseconds (10));

//验证我们刚完成的两个作业不再出现在
//水流。
            std::vector<Cur> const currents {
                getSortedCurrent (perfLog->currentJson()[jss::jobs])};
            BEAST_EXPECT(currents.size() == i * 2);

            std::uint64_t prevDur = std::numeric_limits<std::uint64_t>::max();
            for (int j = 0; j < i; ++j)
            {
                BEAST_EXPECT(currents[j * 2].name == jobs[j].typeName);
                BEAST_EXPECT(prevDur > currents[j * 2].dur);
                prevDur = currents[j * 2].dur;

                BEAST_EXPECT(currents[(j * 2) + 1].name == jobs[j].typeName);
                BEAST_EXPECT(prevDur > currents[(j * 2) + 1].dur);
                prevDur = currents[(j * 2) + 1].dur;
            }
        }

//验证最终结果。
        auto validateFinalCounters =
            [this, &jobs] (Json::Value const& countersJson)
        {
            {
                Json::Value const& rpc = countersJson[jss::rpc];
                BEAST_EXPECT(rpc.isObject());
                BEAST_EXPECT(rpc.size() == 0);
            }

            Json::Value const& jobQueue = countersJson[jss::job_queue];
            for (int i = jobs.size() - 1; i >= 0; --i)
            {
                Json::Value const& counter {jobQueue[jobs[i].typeName]};
                std::uint64_t const running_dur_us {
                    jsonToUint64 (counter[jss::running_duration_us])};
                BEAST_EXPECT(running_dur_us == ((jobs.size() - i) * 4) - 1);

                std::uint64_t const queued_dur_us {
                    jsonToUint64 (counter[jss::queued_duration_us])};
                BEAST_EXPECT(queued_dur_us == i + 1);

                BEAST_EXPECT(counter[jss::queued] == "1");
                BEAST_EXPECT(counter[jss::started] == "2");
                BEAST_EXPECT(counter[jss::finished] == "2");
            }

//验证jss:：total中的值是否符合我们的预期。
            Json::Value const& total {jobQueue[jss::total]};
            const int finished = jobs.size() * 2;
            BEAST_EXPECT(jsonToUint64 (total[jss::queued]) == jobs.size());
            BEAST_EXPECT(
                jsonToUint64 (total[jss::started]) == finished);
            BEAST_EXPECT(
                jsonToUint64 (total[jss::finished]) == finished);

//排队总持续时间应为三角形
//作业。
            int const queuedDur = ((jobs.size() * (jobs.size() + 1)) / 2);
            BEAST_EXPECT(
                jsonToUint64 (total[jss::queued_duration_us]) == queuedDur);

//总运行时间应为完成的三角形数。
            int const runningDur = ((finished * (finished + 1)) / 2);
            BEAST_EXPECT(jsonToUint64 (
                total[jss::running_duration_us]) == runningDur);
        };

        auto validateFinalCurrent =
            [this] (Json::Value const& currentJson)
        {
            {
                Json::Value const& jobs = currentJson[jss::jobs];
                BEAST_EXPECT(jobs.isArray());
                BEAST_EXPECT(jobs.size() == 0);
            }

            Json::Value const& methods = currentJson[jss::methods];
            BEAST_EXPECT(methods.size() == 0);
            BEAST_EXPECT(methods.isArray());
        };

//验证PerfLog的最终状态。
        validateFinalCounters (perfLog->countersJson());
        validateFinalCurrent (perfLog->currentJson());

//给PerfLog足够的时间将其状态刷新到文件中。
        waitForFileUpdate (parent);

//礼貌地停止性能日志。
        parent.doStop();

//如果合适，请检查文件内容。
        auto const fullPath =
            parent.getPerfLogPath() / parent.getPerfLogFileName();

        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (fullPath));
        }
        else
        {
//日志文件的最后一行应包含相同的
//countersjson（）和currentjson（）返回的信息。
//验证这一点。

//获取日志的最后一行。
            std::ifstream log (fullPath.c_str());
            std::string lastLine;
            for (std::string line; std::getline (log, line); )
            {
                if (! line.empty())
                    lastLine = std::move (line);
            }

            Json::Value parsedLastLine;
            Json::Reader ().parse (lastLine, parsedLastLine);
            if (! BEAST_EXPECT(! RPC::contains_error (parsedLastLine)))
//避免故障级联
                return;

//验证日志最后一行的内容。
            validateFinalCounters (parsedLastLine[jss::counters]);
            validateFinalCurrent (parsedLastLine[jss::current_activities]);
        }
    }

    void testInvalidID (WithFile withFile)
    {
        using namespace std::chrono;

//Worker ID用于标识正在进行的作业。表明
//如果传递的ID无效，则Perlog的行为也尽可能正常。

//启动将用于测试的PerfLog。
        PerfLogParent parent {j_};
        auto perfLog {getPerfLog (parent, withFile)};
        parent.doStart();

//随机选择一个作业类型及其名称。
        JobType jobType;
        std::string jobTypeName;
        {
            auto const& jobTypes = JobTypes::instance();

            std::uniform_int_distribution<> dis(0, jobTypes.size() - 1);
            auto iter {jobTypes.begin()};
            std::advance (iter, dis (default_prng()));

            jobType = iter->second.type();
            jobTypeName = iter->second.name();
        }

//假设有一个工作线程。
        perfLog-> resizeJobs(1);

//lambda验证此测试的countersjson。
        auto verifyCounters =
            [this, jobTypeName] (Json::Value const& countersJson,
            int started, int finished, int queued_us, int running_us)
        {
            BEAST_EXPECT(countersJson.isObject());
            BEAST_EXPECT(countersJson.size() == 2);

            BEAST_EXPECT(countersJson.isMember (jss::rpc));
            BEAST_EXPECT(countersJson[jss::rpc].isObject());
            BEAST_EXPECT(countersJson[jss::rpc].size() == 0);

            BEAST_EXPECT(countersJson.isMember (jss::job_queue));
            BEAST_EXPECT(countersJson[jss::job_queue].isObject());
            BEAST_EXPECT(countersJson[jss::job_queue].size() == 1);
            {
                Json::Value const& job {
                    countersJson[jss::job_queue][jobTypeName]};

                BEAST_EXPECT(job.isObject());
                BEAST_EXPECT(jsonToUint64 (job[jss::queued]) == 0);
                BEAST_EXPECT(jsonToUint64 (job[jss::started]) == started);
                BEAST_EXPECT(jsonToUint64 (job[jss::finished]) == finished);

                BEAST_EXPECT(
                    jsonToUint64 (job[jss::queued_duration_us]) == queued_us);
                BEAST_EXPECT(
                    jsonToUint64 (job[jss::running_duration_us]) == running_us);
            }
        };

//lambda验证此测试的currentjson（始终为空）。
        auto verifyEmptyCurrent = [this] (Json::Value const& currentJson)
        {
            BEAST_EXPECT(currentJson.isObject());
            BEAST_EXPECT(currentJson.size() == 2);

            BEAST_EXPECT(currentJson.isMember (jss::jobs));
            BEAST_EXPECT(currentJson[jss::jobs].isArray());
            BEAST_EXPECT(currentJson[jss::jobs].size() == 0);

            BEAST_EXPECT(currentJson.isMember (jss::methods));
            BEAST_EXPECT(currentJson[jss::methods].isArray());
            BEAST_EXPECT(currentJson[jss::methods].size() == 0);
        };

//启动一个太大的ID。
        perfLog->jobStart (jobType, microseconds {11}, steady_clock::now(), 2);
        std::this_thread::sleep_for (microseconds {10});
        verifyCounters (perfLog->countersJson(), 1, 0, 11, 0);
        verifyEmptyCurrent (perfLog->currentJson());

//开始负ID
        perfLog->jobStart (jobType, microseconds {13}, steady_clock::now(), -1);
        std::this_thread::sleep_for (microseconds {10});
        verifyCounters (perfLog->countersJson(), 2, 0, 24, 0);
        verifyEmptyCurrent (perfLog->currentJson());

//完成过大的ID
        perfLog->jobFinish (jobType, microseconds {17}, 2);
        std::this_thread::sleep_for (microseconds {10});
        verifyCounters (perfLog->countersJson(), 2, 1, 24, 17);
        verifyEmptyCurrent (perfLog->currentJson());

//完成负ID
        perfLog->jobFinish (jobType, microseconds {19}, -1);
        std::this_thread::sleep_for (microseconds {10});
        verifyCounters (perfLog->countersJson(), 2, 2, 24, 36);
        verifyEmptyCurrent (perfLog->currentJson());

//给PerfLog足够的时间将其状态刷新到文件中。
        waitForFileUpdate (parent);

//礼貌地停止性能日志。
        parent.doStop();

//如果合适，请检查文件内容。
        auto const fullPath =
            parent.getPerfLogPath() / parent.getPerfLogFileName();

        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (fullPath));
        }
        else
        {
//日志文件的最后一行应包含相同的
//countersjson（）和currentjson（）返回的信息。
//验证这一点。

//获取日志的最后一行。
            std::ifstream log (fullPath.c_str());
            std::string lastLine;
            for (std::string line; std::getline (log, line); )
            {
                if (! line.empty())
                    lastLine = std::move (line);
            }

            Json::Value parsedLastLine;
            Json::Reader ().parse (lastLine, parsedLastLine);
            if (! BEAST_EXPECT(! RPC::contains_error (parsedLastLine)))
//避免故障级联
                return;

//验证日志最后一行的内容。
            verifyCounters (parsedLastLine[jss::counters], 2, 2, 24, 36);
            verifyEmptyCurrent (parsedLastLine[jss::current_activities]);
        }
    }

    void testRotate (WithFile withFile)
    {
//我们不能完全测试旋转，因为单元测试必须在Windows上运行，
//而Windows没有（可能没有？）支撑旋转。但至少打个电话
//界面，看它不会崩溃。
        using namespace boost::filesystem;

        auto const perfLogPath {PerfLogParent::getPerfLogPath()};
        auto const fullPath = perfLogPath / PerfLogParent::getPerfLogFileName();

        PerfLogParent parent {j_};
        BEAST_EXPECT(! exists (perfLogPath));

        auto perfLog {getPerfLog (parent, withFile)};

        BEAST_EXPECT(parent.stopSignaled == false);
        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (perfLogPath));
        }
        else
        {
            BEAST_EXPECT(exists (fullPath));
            BEAST_EXPECT(file_size (fullPath) == 0);
        }

//启动PerfLog并为PerfLog:：Report（）等待足够长的时间。
//写入其文件。
        parent.doStart();
        waitForFileUpdate (parent);

        decltype (file_size (fullPath)) firstFileSize {0};
        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (perfLogPath));
        }
        else
        {
            firstFileSize = file_size (fullPath);
            BEAST_EXPECT(firstFileSize > 0);
        }

//旋转，然后等待以确保更多的内容写入文件。
        perfLog->rotate();
        waitForFileUpdate (parent);

        parent.doStop();

        if (withFile == WithFile::no)
        {
            BEAST_EXPECT(! exists (perfLogPath));
        }
        else
        {
            BEAST_EXPECT(file_size (fullPath) > firstFileSize);
        }
    }

    void run() override
    {
        testFileCreation();
        testRPC (WithFile::no);
        testRPC (WithFile::yes);
        testJobs (WithFile::no);
        testJobs (WithFile::yes);
        testInvalidID (WithFile::no);
        testInvalidID (WithFile::yes);
        testRotate (WithFile::no);
        testRotate (WithFile::yes);
    }
};

BEAST_DEFINE_TESTSUITE(PerfLog, basics, ripple);

} //命名空间波纹
