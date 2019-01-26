
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

#ifndef BEAST_UNIT_TEST_MATCH_HPP
#define BEAST_UNIT_TEST_MATCH_HPP

#include <beast/unit_test/suite_info.hpp>
#include <string>

namespace beast {
namespace unit_test {

//用于实现匹配的谓词
class selector
{
public:
    enum mode_t
    {
//运行除手动测试以外的所有测试
        all,

//在任何字段中运行匹配的测试
        automatch,

//组曲比赛
        suite,

//在库上匹配
        library,

//模块匹配（内部使用）
        module,

//不匹配（内部使用）
        none
    };

private:
    mode_t mode_;
    std::string pat_;
    std::string library_;

public:
    template<class = void>
    explicit
    selector(mode_t mode, std::string const& pattern = "");

    template<class = void>
    bool
    operator()(suite_info const& s);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class>
selector::selector(mode_t mode, std::string const& pattern)
    : mode_(mode)
    , pat_(pattern)
{
    if(mode_ == automatch && pattern.empty())
        mode_ = all;
}

template<class>
bool
selector::operator()(suite_info const& s)
{
    switch(mode_)
    {
    case automatch:
//套房或全名
        if(s.name() == pat_ || s.full_name() == pat_)
        {
            mode_ = none;
            return true;
        }

//检查模块
        if(pat_ == s.module())
        {
            mode_ = module;
            library_ = s.library();
            return ! s.manual();
        }

//检查库
        if(pat_ == s.library())
        {
            mode_ = library;
            return ! s.manual();
        }

        return false;

    case suite:
        return pat_ == s.name();

    case module:
        return pat_ == s.module() && ! s.manual();

    case library:
        return pat_ == s.library() && ! s.manual();

    case none:
        return false;

    case all:
    default:
//跌倒
        break;
    };

    return ! s.manual();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于生成谓词以选择套件的实用函数。

/*返回实现智能匹配规则的谓词。
    谓词检查
    套房信息按顺序排列。当找到匹配项时，它会更改模式
    根据发现的情况：

        如果先匹配一个套件，则只选择该套件。这个
        套房可标记为“手动”。

        如果首先匹配某个模块，则只匹配该模块中的套件
        从那时起选择未标记为“手动”的库。

        如果首先匹配一个库，则只匹配该库中的套件
        从那时起选择未标记的手动。

**/

inline
selector
match_auto(std::string const& name)
{
    return selector(selector::automatch, name);
}

/*返回与所有未标记为Manual的套件匹配的谓词。*/
inline
selector
match_all()
{
    return selector(selector::all);
}

/*返回与特定套件匹配的谓词。*/
inline
selector
match_suite(std::string const& name)
{
    return selector(selector::suite, name);
}

/*返回与库中所有套件匹配的谓词。*/
inline
selector
match_library(std::string const& name)
{
    return selector(selector::library, name);
}

} //单位试验
} //野兽

#endif
