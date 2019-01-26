
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


#include <ripple/basics/Log.h>
#include <ripple/protocol/digest.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/main/DBInit.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/Sustain.h>
#include <ripple/core/Config.h>
#include <ripple/core/ConfigSections.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/core/TimeKeeper.h>
#include <ripple/json/to_string.h>
#include <ripple/net/RPCCall.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/protocol/BuildInfo.h>
#include <ripple/beast/clock/basic_seconds_clock.h>
#include <ripple/beast/core/CurrentThreadName.h>

#include <beast/unit_test/dstream.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/match.hpp>
#include <beast/unit_test/reporter.hpp>
#include <test/unit_test/multi_runner.h>

#include <google/protobuf/stubs/common.h>

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
#include <boost/version.hpp>

#include <cstdlib>
#include <iostream>
#include <utility>
#include <stdexcept>

#ifdef _MSC_VER
#include <sys/types.h>
#include <sys/timeb.h>
#endif

namespace po = boost::program_options;

namespace ripple {

bool
adjustDescriptorLimit(int needed, beast::Journal j)
{
#ifdef RLIMIT_NOFILE
//得到电流限制，然后根据需要调整它。
    struct rlimit rl;

    int available = 0;

    if (getrlimit(RLIMIT_NOFILE, &rl) == 0)
    {
//如果极限是无限的，那么我们是好的。
        if (rl.rlim_cur == RLIM_INFINITY)
            available = needed;
        else
            available = rl.rlim_cur;

        if (available < needed)
        {
//忽略rlim_max，因为过程可能
//配置为无论如何都要覆盖它。我们
//询问我们需要的数字描述符。
            rl.rlim_cur = needed;

            if (setrlimit(RLIMIT_NOFILE, &rl) == 0)
                available = rl.rlim_cur;
        }
    }

    if (needed > available)
    {
        j.fatal() <<
            "Insufficient number of file descriptors: " <<
            needed << " are needed, but only " <<
            available << " are available.";

        std::cerr <<
            "Insufficient number of file descriptors: " <<
            needed << " are needed, but only " <<
            available << " are available.\n";

        return false;
    }
#endif

    return true;
}

void printHelp (const po::options_description& desc)
{
    std::cerr
        << systemName () << "d [options] <command> <params>\n"
        << desc << std::endl
        << "Commands: \n"
           "     account_currencies <account> [<ledger>] [strict]\n"
           "     account_info <account>|<seed>|<pass_phrase>|<key> [<ledger>] [strict]\n"
           "     account_lines <account> <account>|\"\" [<ledger>]\n"
           "     account_channels <account> <account>|\"\" [<ledger>]\n"
           "     account_objects <account> [<ledger>] [strict]\n"
           "     account_offers <account>|<account_public_key> [<ledger>]\n"
           "     account_tx accountID [ledger_min [ledger_max [limit [offset]]]] [binary] [count] [descending]\n"
           "     book_offers <taker_pays> <taker_gets> [<taker [<ledger> [<limit> [<proof> [<marker>]]]]]\n"
           "     can_delete [<ledgerid>|<ledgerhash>|now|always|never]\n"
           "     channel_authorize <private_key> <channel_id> <drops>\n"
           "     channel_verify <public_key> <channel_id> <drops> <signature>\n"
           "     connect <ip> [<port>]\n"
           "     consensus_info\n"
           "     deposit_authorized <source_account> <destination_account> [<ledger>]\n"
           "     download_shard [[<index> <url>]] <validate>\n"
           "     feature [<feature> [accept|reject]]\n"
           "     fetch_info [clear]\n"
           "     gateway_balances [<ledger>] <issuer_account> [ <hotwallet> [ <hotwallet> ]]\n"
           "     get_counts\n"
           "     json <method> <json>\n"
           "     ledger [<id>|current|closed|validated] [full]\n"
           "     ledger_accept\n"
           "     ledger_closed\n"
           "     ledger_current\n"
           "     ledger_request <ledger>\n"
           "     log_level [[<partition>] <severity>]\n"
           "     logrotate \n"
           "     peers\n"
           "     ping\n"
           "     random\n"
           "     ripple ...\n"
           "     ripple_path_find <json> [<ledger>]\n"
           "     version\n"
           "     server_info [counters]\n"
           "     server_state [counters]\n"
           "     sign <private_key> <tx_json> [offline]\n"
           "     sign_for <signer_address> <signer_private_key> <tx_json> [offline]\n"
           "     stop\n"
           "     submit <tx_blob>|[<private_key> <tx_json>]\n"
           "     submit_multisigned <tx_json>\n"
           "     tx <id>\n"
           "     validation_create [<seed>|<pass_phrase>|<key>]\n"
           "     wallet_propose [<passphrase>]\n";
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*允许逗号分隔列表的简单单元测试选择器
 *选择器
 **/

class multi_selector
{
private:
    std::vector<beast::unit_test::selector> selectors_;
public:
    explicit
    multi_selector(std::string const& patterns = "")
    {
        std::vector<std::string> v;
        boost::split (v, patterns, boost::algorithm::is_any_of (","));
        selectors_.reserve(v.size());
        std::for_each(v.begin(), v.end(),
            [this](std::string s)
            {
                boost::trim (s);
                if (selectors_.empty() || !s.empty())
                    selectors_.emplace_back(
                        beast::unit_test::selector::automatch, s);
            });
    }

    bool
    operator()(beast::unit_test::suite_info const& s)
    {
        for (auto& sel : selectors_)
            if (sel(s))
                return true;
        return false;
    }
};

namespace test{ extern std::atomic<bool> envUseIPv4; }

static int runUnitTests(
    std::string const& pattern,
    std::string const& argument,
    bool quiet,
    bool log,
    bool child,
    bool ipv6,
    std::size_t num_jobs,
    int argc,
    char** argv)
{
    using namespace beast::unit_test;
    using namespace ripple::test;

    ripple::test::envUseIPv4 = (! ipv6);

    if (!child && num_jobs == 1)
    {
        multi_runner_parent parent_runner;

        multi_runner_child child_runner{num_jobs, quiet, log};
        child_runner.arg(argument);
        auto const any_failed = child_runner.run_multi(multi_selector(pattern));

        if (any_failed)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
    if (!child)
    {
        multi_runner_parent parent_runner;
        std::vector<boost::process::child> children;

        std::string const exe_name = argv[0];
        std::vector<std::string> args;
        {
            args.reserve(argc);
            for (int i = 1; i < argc; ++i)
                args.emplace_back(argv[i]);
            args.emplace_back("--unittest-child");
        }

        for (std::size_t i = 0; i < num_jobs; ++i)
            children.emplace_back(
                boost::process::exe = exe_name, boost::process::args = args);

        int bad_child_exits = 0;
        for(auto& c : children)
        {
            try
            {
                c.wait();
                if (c.exit_code())
                    ++bad_child_exits;
            }
            catch (...)
            {
//如果进程被一个信号终止，则等待抛出
                ++bad_child_exits;
            }
        }

        if (parent_runner.any_failed() || bad_child_exits)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
    else
    {
//小孩
        multi_runner_child runner{num_jobs, quiet, log};
        runner.arg(argument);
        auto const anyFailed = runner.run_multi(multi_selector(pattern));

        if (anyFailed)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

int run (int argc, char** argv)
{
    using namespace std;

    beast::setCurrentThreadName ("rippled: main");

    po::variables_map vm;

    std::string importText;
    {
        importText += "Import an existing node database (specified in the [";
        importText += ConfigSection::importNodeDatabase ();
        importText += "] configuration file section) into the current ";
        importText += "node database (specified in the [";
        importText += ConfigSection::nodeDatabase ();
        importText += "] configuration file section).";
    }
    std::string shardsText;
    {
        shardsText += "Validate an existing shard database (specified in the [";
        shardsText += ConfigSection::shardDatabase();
        shardsText += "] configuration file section).";
    }

//设置选项分析。
//
    po::options_description gen ("General Options");
    gen.add_options ()
    ("conf", po::value<std::string> (), "Specify the configuration file.")
    ("debug", "Enable normally suppressed debug logging")
    ("fg", "Run in the foreground.")
    ("help,h", "Display this message.")
    ("quorum", po::value <std::size_t> (),
        "Override the minimum validation quorum.")
    ("silent", "No output to the console after startup.")
    ("standalone,a", "Run with no peers.")
    ("verbose,v", "Verbose logging.")
    ("version", "Display the build version.")
    ;

    po::options_description data ("Ledger/Data Options");
    data.add_options ()
    ("import", importText.c_str ())
    ("ledger", po::value<std::string> (),
        "Load the specified ledger and start from the value given.")
    ("ledgerfile", po::value<std::string> (), "Load the specified ledger file.")
    ("load", "Load the current ledger from the local DB.")
    ("net", "Get the initial ledger from the network.")
    ("nodetoshard", "Import node store into shards")
    ("replay","Replay a ledger close.")
    ("start", "Start from a fresh Ledger.")
    ("vacuum", po::value<std::string>(),
        "VACUUM the transaction db. Mandatory string argument specifies "
        "temporary directory path.")
    ("valid", "Consider the initial ledger a valid network ledger.")
    ("validateShards", shardsText.c_str ())
    ;

    po::options_description rpc ("RPC Client Options");
    rpc.add_options()
    ("rpc",
        "Perform rpc command - see below for available commands. "
        "This is assumed if any positional parameters are provided.")
    ("rpc_ip", po::value <std::string> (),
        "Specify the IP address for RPC command. "
        "Format: <ip-address>[':'<port-number>]")
    ("rpc_port", po::value <std::uint16_t> (),
        "DEPRECATED: include with rpc_ip instead. "
        "Specify the port number for RPC command.")
    ;

    po::options_description test ("Unit Test Options");
    test.add_options()
    ("quiet,q",
        "Suppress test suite messages, "
        "including suite/case name (at start) and test log messages.")
    ("unittest,u", po::value <std::string> ()->implicit_value (""),
        "Perform unit tests. The optional argument specifies one or "
        "more comma-separated selectors. Each selector specifies a suite name, "
        "full-name (lib.module.suite), module, or library "
        "(checked in that ""order).")
    ("unittest-arg", po::value <std::string> ()->implicit_value (""),
        "Supplies an argument string to unit tests. If provided, this argument "
        "is made available to each suite that runs. Interpretation of the "
        "argument is handled individually by any suite that accesses it -- "
        "as such, it typically only make sense to provide this when running "
        "a single suite.")
    ("unittest-ipv6", "Use IPv6 localhost when running unittests (default is IPv4).")
    ("unittest-log",
        "Force unit test log message output. Only useful in combination with "
        "--quiet, in which case log messages will print but suite/case names "
        "will not.")
    ("unittest-jobs", po::value <std::size_t> (),
        "Number of unittest jobs to run in parallel (child processes).")
    ;

//这些是隐藏选项，不会显示在用法/帮助消息中。
    po::options_description hidden ("Hidden Options");
    hidden.add_options()
    ("parameters", po::value< vector<string> > (),
        "Specify rpc command and parameters. This option must be repeated "
        "for each command/param. Positional parameters also serve this purpose, "
        "so this option is not needed for users")
    ("unittest-child",
        "For internal use only when spawning child unit test processes.")
    ;

//将位置参数解释为--参数。
    po::positional_options_description p;
    p.add ("parameters", -1);

    po::options_description all;
    all.add(gen).add(rpc).add(data).add(test).add(hidden);

    po::options_description desc;
    desc.add(gen).add(rpc).add(data).add(test);

//如果没有错误，则分析选项。
    try
    {
        po::store (po::command_line_parser (argc, argv)
.options (all)                //解析选项。
.positional (p)               //余数作为参数。
            .run (),
            vm);
po::notify (vm);                  //调用选项通知函数。
    }
    catch (std::exception const&)
    {
        std::cerr << "rippled: Incorrect command line syntax." << std::endl;
        std::cerr << "Use '--help' for a list of options." << std::endl;
        return 1;
    }

    if (vm.count ("help"))
    {
        printHelp (desc);
        return 0;
    }

    if (vm.count ("version"))
    {
        std::cout << "rippled version " <<
            BuildInfo::getVersionString () << std::endl;
        return 0;
    }

//如果需要，运行单元测试。
//单元测试将使用适当的返回代码退出应用程序。
//
    if (vm.count ("unittest"))
    {
        std::string argument;

        if (vm.count("unittest-arg"))
            argument = vm["unittest-arg"].as<std::string>();

        std::size_t numJobs = 1;
        bool unittestChild = false;
        if (vm.count("unittest-jobs"))
            numJobs = std::max(numJobs, vm["unittest-jobs"].as<std::size_t>());
        unittestChild = bool (vm.count("unittest-child"));

        return runUnitTests(
            vm["unittest"].as<std::string>(), argument,
            bool (vm.count ("quiet")),
            bool (vm.count ("unittest-log")),
            unittestChild,
            bool (vm.count ("unittest-ipv6")),
            numJobs,
            argc,
            argv);
    }
    else
    {
        if (vm.count("unittest-jobs"))
        {
//UnitTest作业只对“UnitTest”有意义
            std::cerr << "rippled: '--unittest-jobs' specified without '--unittest'.\n";
            std::cerr << "To run the unit tests the '--unittest' option must be present.\n";
            return 1;
        }
    }

    auto config = std::make_unique<Config>();

    auto configFile = vm.count ("conf") ?
            vm["conf"].as<std::string> () : std::string();

//配置文件，安静标志。
    config->setup (configFile, bool (vm.count ("quiet")),
        bool(vm.count("silent")), bool(vm.count("standalone")));

    if (vm.count("vacuum"))
    {
        DatabaseCon::Setup dbSetup = setup_DatabaseCon(*config);
        if (dbSetup.standAlone)
        {
            std::cerr << "vacuum not applicable in standalone mode.\n";
            return -1;
        }
        boost::filesystem::path dbPath = dbSetup.dataDir / TxnDBName;
        auto txnDB = std::make_unique<DatabaseCon> (dbSetup, TxnDBName,
            TxnDBInit, TxnDBCount);
        if (txnDB.get() == nullptr)
        {
            std::cerr << "Cannot create connection to " << dbPath.string() <<
                '\n';
            return -1;
        }
        boost::system::error_code ec;
        uintmax_t dbSize = boost::filesystem::file_size(dbPath, ec);
        if (ec)
        {
            std::cerr << "Error checking size of " << dbPath.string() << ": "
                << ec.message() << '\n';
            return -1;
        }

        assert(dbSize != static_cast<uintmax_t>(-1));

        std::string tmpDir = vm["vacuum"].as<std::string>();
        boost::filesystem::path tmpPath = tmpDir;
        if (boost::filesystem::space(tmpPath, ec).available < dbSize)
        {
            if (ec)
            {
                std::cerr << "Error checking status of " << tmpPath.string()
                    << ": " << ec.message() << '\n';
            }
            else
            {
                std::cerr << "A valid directory for vacuuming must be "
                             "specified on a filesystem with at least "
                             "as much free space as the size of " <<
                             dbPath.string() << ", which is " <<
                             dbSize << " bytes. The filesystem for " <<
                             tmpPath.string() << " only has " <<
                             boost::filesystem::space(tmpPath, ec).available
                             << " bytes.\n";
            }
            return -1;
        }

        auto db = txnDB->checkoutDb();
        std::uint32_t pageSize;
        try
        {
            *db << "PRAGMA page_size;", soci::into(pageSize);
            std::cout << "VACUUM beginning. page_size: " << pageSize
                << std::endl;
            *db << "PRAGMA journal_mode=OFF;";
            *db << "PRAGMA temp_store_directory=\"" << tmpPath.string()
                << "\";";
            *db << "VACUUM;";
            *db << "PRAGMA journal_mode=WAL;";
            *db << "PRAGMA page_size;", soci::into(pageSize);
        }
        catch (soci::soci_error const& e)
        {
            std::cerr << "SQLite error: " << e.what() << '\n';
            return 1;
        }
        std::cout << "VACUUM finished. page_size: " << pageSize << std::endl;

        return 0;
    }

    if (vm.count ("start"))
        config->START_UP = Config::FRESH;

    if (vm.count ("import"))
        config->doImport = true;

    if (vm.count("nodetoshard"))
        config->nodeToShard = true;

    if (vm.count ("validateShards"))
        config->validateShards = true;

    if (vm.count ("ledger"))
    {
        config->START_LEDGER = vm["ledger"].as<std::string> ();
        if (vm.count("replay"))
            config->START_UP = Config::REPLAY;
        else
            config->START_UP = Config::LOAD;
    }
    else if (vm.count ("ledgerfile"))
    {
        config->START_LEDGER = vm["ledgerfile"].as<std::string> ();
        config->START_UP = Config::LOAD_FILE;
    }
    else if (vm.count ("load"))
    {
        config->START_UP = Config::LOAD;
    }

    if (vm.count ("valid"))
    {
        config->START_VALID = true;
    }

    if (vm.count ("net"))
    {
        if ((config->START_UP == Config::LOAD) ||
            (config->START_UP == Config::REPLAY))
        {
            std::cerr <<
                "Net and load/reply options are incompatible" << std::endl;
            return -1;
        }

        config->START_UP = Config::NETWORK;
    }

//覆盖RPC目标IP地址。这必须
//在加载配置文件后发生。
    if (vm.count ("rpc_ip"))
    {
        auto res = beast::IP::Endpoint::from_string_checked(
            vm["rpc_ip"].as<std::string>());
        if (! res.second)
        {
            std::cerr << "Invalid rpc_ip = " <<
                vm["rpc_ip"].as<std::string>() << "\n";
            return -1;
        }

        if (res.first.port() == 0)
        {
            std::cerr << "No port specified in rpc_ip.\n";
            if (vm.count ("rpc_port"))
            {
                std::cerr << "WARNING: using deprecated rpc_port param.\n";
                try
                {
                    res.first = res.first.at_port(
                        vm["rpc_port"].as<std::uint16_t>());
                    if (res.first.port() == 0)
                        throw std::domain_error("0");
                }
                catch(std::exception const& e)
                {
                    std::cerr << "Invalid rpc_port = " << e.what() << "\n";
                    return -1;
                }
            }
            else
                return -1;
        }

        config->rpc_ip = std::move(res.first);
    }

    if (vm.count ("quorum"))
    {
        try
        {
            config->VALIDATION_QUORUM = vm["quorum"].as <std::size_t> ();
            if (config->VALIDATION_QUORUM == std::size_t{})
            {
                throw std::domain_error("0");
            }
        }
        catch(std::exception const& e)
        {
            std::cerr << "Invalid value specified for --quorum ("
                      << e.what() << ")\n";
            return -1;
        }
    }

//以配置的严重性构造logs对象
    using namespace beast::severities;
    Severity thresh = kInfo;

    if (vm.count ("quiet"))
        thresh = kFatal;
    else if (vm.count ("verbose"))
        thresh = kTrace;

    auto logs = std::make_unique<Logs>(thresh);

//没有参数。运行服务器。
    if (!vm.count ("parameters"))
    {
//我们需要至少1024个文件描述符。我们将
//进一步调整。
        if (!adjustDescriptorLimit(1024, logs->journal("Application")))
            return -1;

        if (HaveSustain() && !vm.count ("fg") && !config->standalone())
        {
            auto const ret = DoSustain ();

            if (!ret.empty ())
                std::cerr << "Watchdog: " << ret << std::endl;
        }

        if (vm.count ("debug"))
        {
            setDebugLogSink (logs->makeSink (
                "Debug", beast::severities::kTrace));
        }

        auto timeKeeper = make_TimeKeeper(
            logs->journal("TimeKeeper"));

        auto app = make_Application(
            std::move(config),
            std::move(logs),
            std::move(timeKeeper));

        if (!app->setup ())
        {
            StopSustain();
            return -1;
        }

//分析配置后，确保
//提供足够的文件描述符：
        if (!adjustDescriptorLimit(
            app->fdlimit(),
            app->logs().journal("Application")))
        {
            StopSustain();
            return -1;
        }

//启动服务器
        /*->DoStart（真/*启动计时器*/）；

        //阻止，直到我们得到一个停止RPC。
        App-> Run（）；

        返回0；
    }

    //我们有一个要处理的rpc命令：
    Beast:：setCurrentThreadName（“Rippled:rpc”）；
    返回rpccall:：fromcommandline（
        *配置，
        vm[“参数”].as<std:：vector<std:：string>>（），
        *日志）；
}

} /纹波

//必须在命名空间之外，原因很明显
/ /
int main（int argc，char**argv）
{
γIFIFF
    {
        //为https://svn.boost.org/trac/boost/ticket/10657工作
        //根据1.56.0版的Boost报告。如果应用程序
        //对getTimeZoneInformation的第一个调用来自coroutine，一个
        //生成未处理的异常。解决方法是
        //在启动任何
        //协同作战。在写这篇文章的时候，电话是
        //用于初始化时区信息。
        结构时间t；
    定义时间
            Y-fTimeSys（&t）；
    否则
            Y-FTIME（&t）；
    第二节
    }
    Ripple:：sha512_deprecatedmsvcworkaround（）；
第二节

如果定义了（GNUC）&！定义（uu clang_uuuu）
    auto constexpr gccver=（u gnuc_uux 100*100）+
                            （uu gnuc_minor_uux 100）+
                            _uu-gnuc_-patchlevel_uuu；

    静态断言（gccver>=50100，
        “编译Rippled需要GCC 5.1.0或更高版本。”）；
第二节

    退出（&google:：protobuf:：shutdownprotobflibrary）；

    自动构造结果（Ripple:：Run（argC，argV））；

    Beast:：basic_seconds_clock_main_hook（）；

    返回结果；
}
