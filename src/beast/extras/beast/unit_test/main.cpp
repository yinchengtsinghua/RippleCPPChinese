
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#include <beast/unit_test/amount.hpp>
#include <beast/unit_test/dstream.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/match.hpp>
#include <beast/unit_test/reporter.hpp>
#include <beast/unit_test/suite.hpp>
#include <boost/config.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>

#ifdef BOOST_MSVC
# ifndef WIN32_LEAN_AND_MEAN //特拉利亚
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef WIN32_LEAN_AND_MEAN
# else
#  include <windows.h>
# endif
#endif

namespace beast {
namespace unit_test {

static
std::string
prefix(suite_info const& s)
{
    if(s.manual())
        return "|M| ";
    return "    ";
}

static
void
print(std::ostream& os, suite_list const& c)
{
    std::size_t manual = 0;
    for(auto const& s : c)
    {
        os << prefix(s) << s.full_name() << '\n';
        if(s.manual())
            ++manual;
    }
    os <<
        amount(c.size(), "suite") << " total, " <<
        amount(manual, "manual suite") <<
        '\n'
        ;
}

//打印套房列表
//与--print命令行选项一起使用
static
void
print(std::ostream& os)
{
    os << "------------------------------------------\n";
    print(os, global_suites());
    os << "------------------------------------------" <<
        std::endl;
}

} //单位试验
} //野兽

//用于生产支架的简单主体
//单独运行单元测试的可执行文件。
int main(int ac, char const* av[])
{
    using namespace std;
    using namespace beast::unit_test;

#if BOOST_MSVC
    {
        int flags = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
        flags |= _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
    }
#endif

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
       ("help,h",  "Produce a help message")
       ("print,p", "Print the list of available test suites")
       ("suites,s", po::value<string>(), "suites to run")
        ;

    po::positional_options_description p;
    po::variables_map vm;
    po::store(po::parse_command_line(ac, av, desc), vm);
    po::notify(vm);

    dstream log(std::cerr);
    std::unitbuf(log);

    if(vm.count("help"))
    {
        log << desc << std::endl;
    }
    else if(vm.count("print"))
    {
        print(log);
    }
    else
    {
        std::string suites;
        if(vm.count("suites") > 0)
            suites = vm["suites"].as<string>();
        reporter r(log);
        bool failed;
        if(! suites.empty())
            failed = r.run_each_if(global_suites(),
                match_auto(suites));
        else
            failed = r.run_each(global_suites());
        if(failed)
            return EXIT_FAILURE;
        return EXIT_SUCCESS;
    }
}
