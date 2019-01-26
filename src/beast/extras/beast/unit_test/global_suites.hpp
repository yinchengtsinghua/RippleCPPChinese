
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

#ifndef BEAST_UNIT_TEST_GLOBAL_SUITES_HPP
#define BEAST_UNIT_TEST_GLOBAL_SUITES_HPP

#include <beast/unit_test/suite_list.hpp>

namespace beast {
namespace unit_test {

namespace detail {

///保留静态初始化期间注册的测试套件。
inline
suite_list&
global_suites()
{
    static suite_list s;
    return s;
}

template<class Suite>
struct insert_suite
{
    insert_suite(char const* name, char const* module,
        char const* library, bool manual, int priority)
    {
        global_suites().insert<Suite>(
            name, module, library, manual, priority);
    }
};

} //细节

///保留静态初始化期间注册的测试套件。
inline
suite_list const&
global_suites()
{
    return detail::global_suites();
}

} //单位试验
} //野兽

#endif
