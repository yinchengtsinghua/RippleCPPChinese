
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//
//版权所有（c）2013-2016 Vinnie Falco（gmail.com上的Vinnie.dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

#include <beast/unit_test/amount.hpp>
#include <beast/unit_test/global_suites.hpp>
#include <beast/unit_test/suite.hpp>
#include <string>

//在项目中包含此.cpp以获得对打印套件的访问权限

namespace beast {
namespace unit_test {

/*打印全局定义的套件列表的套件。*/
class print_test : public suite
{
public:
    void
    run() override
    {
        std::size_t manual = 0;
        std::size_t total = 0;

        auto prefix = [](suite_info const& s)
        {
            return s.manual() ? "|M| " : "    ";
        };

        for (auto const& s : global_suites())
        {
            log << prefix (s) << s.full_name() << '\n';

            if (s.manual())
                ++manual;
            ++total;
        }

        log <<
            amount (total, "suite") << " total, " <<
            amount (manual, "manual suite") << std::endl;

        pass();
    }
};

BEAST_DEFINE_TESTSUITE_MANUAL(print,unit_test,beast);

} //单位试验
} //野兽
