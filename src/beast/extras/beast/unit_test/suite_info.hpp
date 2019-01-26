
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

#ifndef BEAST_UNIT_TEST_SUITE_INFO_HPP
#define BEAST_UNIT_TEST_SUITE_INFO_HPP

#include <cstring>
#include <functional>
#include <string>
#include <utility>

namespace beast {
namespace unit_test {

class runner;

/*将单元测试类型与元数据关联。*/
class suite_info
{
    using run_type = std::function<void(runner&)>;

    std::string name_;
    std::string module_;
    std::string library_;
    bool manual_;
    int priority_;
    run_type run_;

public:
    suite_info(
            std::string name,
            std::string module,
            std::string library,
            bool manual,
            int priority,
            run_type run)
        : name_(std::move(name))
        , module_(std::move(module))
        , library_(std::move(library))
        , manual_(manual)
        , priority_(priority)
        , run_(std::move(run))
    {
    }

    std::string const&
    name() const
    {
        return name_;
    }

    std::string const&
    module() const
    {
        return module_;
    }

    std::string const&
    library() const
    {
        return library_;
    }

///如果此套件只能手动运行，则返回'true'。
    bool
    manual() const
    {
        return manual_;
    }

///以字符串形式返回规范套件名称。
    std::string
    full_name() const
    {
        return library_ + "." + module_ + "." + name_;
    }

///运行关联测试套件的新实例。
    void
    run(runner& r) const
    {
        run_(r);
    }

    friend
    bool
    operator<(suite_info const& lhs, suite_info const& rhs)
    {
//我们希望优先级更高的套房先排序，因此
//这里是优先值
        return std::forward_as_tuple(-lhs.priority_, lhs.library_, lhs.module_, lhs.name_) <
               std::forward_as_tuple(-rhs.priority_, rhs.library_, rhs.module_, rhs.name_);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

///方便为给定测试类型生成套件信息。
template<class Suite>
suite_info
make_suite_info(
    std::string name,
    std::string module,
    std::string library,
    bool manual,
    int priority)
{
    return suite_info(
        std::move(name),
        std::move(module),
        std::move(library),
        manual,
        priority,
        [](runner& r)
        {
            Suite{}(r);
        }
    );
}

} //单位试验
} //野兽

#endif
