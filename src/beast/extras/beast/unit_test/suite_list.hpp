
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

#ifndef BEAST_UNIT_TEST_SUITE_LIST_HPP
#define BEAST_UNIT_TEST_SUITE_LIST_HPP

#include <beast/unit_test/suite_info.hpp>
#include <beast/unit_test/detail/const_container.hpp>
#include <boost/assert.hpp>
#include <typeindex>
#include <set>
#include <unordered_set>

namespace beast {
namespace unit_test {

///a测试套件的容器。
class suite_list
    : public detail::const_container <std::set <suite_info>>
{
private:
#ifndef NDEBUG
    std::unordered_set<std::string> names_;
    std::unordered_set<std::type_index> classes_;
#endif

public:
    /*在集合中插入一个套件。

        套件必须不存在。
    **/

    template<class Suite>
    void
    insert(
        char const* name,
        char const* module,
        char const* library,
        bool manual,
        int priority);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Suite>
void
suite_list::insert(
    char const* name,
    char const* module,
    char const* library,
    bool manual,
    int priority)
{
#ifndef NDEBUG
    {
        std::string s;
        s = std::string(library) + "." + module + "." + name;
        auto const result(names_.insert(s));
BOOST_ASSERT(result.second); //重复名称
    }

    {
        auto const result(classes_.insert(
            std::type_index(typeid(Suite))));
BOOST_ASSERT(result.second); //复制类型
    }
#endif
    cont().emplace(make_suite_info<Suite>(
        name, module, library, manual, priority));
}

} //单位试验
} //野兽

#endif

