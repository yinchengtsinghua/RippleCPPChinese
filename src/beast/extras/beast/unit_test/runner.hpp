
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

#ifndef BEAST_UNIT_TEST_RUNNER_H_INCLUDED
#define BEAST_UNIT_TEST_RUNNER_H_INCLUDED

#include <beast/unit_test/suite_info.hpp>
#include <boost/assert.hpp>
#include <mutex>
#include <ostream>
#include <string>

namespace beast {
namespace unit_test {

/*单元测试运行程序接口。

    派生类可以自定义报告行为。这个接口是
    注入单元测试类以接收测试结果。
**/

class runner
{
    std::string arg_;
    bool default_ = false;
    bool failed_ = false;
    bool cond_ = false;
    std::recursive_mutex mutex_;

public:
    runner() = default;
    virtual ~runner() = default;
    runner(runner const&) = delete;
    runner& operator=(runner const&) = delete;

    /*设置参数字符串。

        参数字符串可用于套件和
        允许自定义测试。每套房
        为argumnet字符串定义自己的语法。
        相同的参数传递给所有套件。
    **/

    void
    arg(std::string const& s)
    {
        arg_ = s;
    }

    /*返回参数字符串。*/
    std::string const&
    arg() const
    {
        return arg_;
    }

    /*运行指定的套件。
        @如果任何条件失败，返回'true'。
    **/

    template<class = void>
    bool
    run(suite_info const& s);

    /*运行一系列套件。
        表达式
            `fwditer:：value_类型'
        必须可转换为“套房信息”。
        @如果任何条件失败，返回'true'。
    **/

    template<class FwdIter>
    bool
    run(FwdIter first, FwdIter last);

    /*有条件地运行一系列套件。
        PRED将被称为：
        @代码
            bool pred（suite_info const&）；
        @端码
        @如果任何条件失败，返回'true'。
    **/

    template<class FwdIter, class Pred>
    bool
    run_if(FwdIter first, FwdIter last, Pred pred = Pred{});

    /*在容器中运行所有套件。
        @如果任何条件失败，返回'true'。
    **/

    template<class SequenceContainer>
    bool
    run_each(SequenceContainer const& c);

    /*在容器中有条件地运行套件。
        PRED将被称为：
        @代码
            bool pred（suite_info const&）；
        @端码
        @如果任何条件失败，返回'true'。
    **/

    template<class SequenceContainer, class Pred>
    bool
    run_each_if(SequenceContainer const& c, Pred pred = Pred{});

protected:
///在新套件启动时调用。
    virtual
    void
    on_suite_begin(suite_info const&)
    {
    }

///套件结束时调用。
    virtual
    void
    on_suite_end()
    {
    }

///在新案例开始时调用。
    virtual
    void
    on_case_begin(std::string const&)
    {
    }

///在新案例结束时调用。
    virtual
    void
    on_case_end()
    {
    }

///为每个传递条件调用。
    virtual
    void
    on_pass()
    {
    }

///为每个失败条件调用。
    virtual
    void
    on_fail(std::string const&)
    {
    }

///在测试记录输出时调用。
    virtual
    void
    on_log(std::string const&)
    {
    }

private:
    friend class suite;

//启动新的测试用例。
    template<class = void>
    void
    testcase(std::string const& name);

    template<class = void>
    void
    pass();

    template<class = void>
    void
    fail(std::string const& reason);

    template<class = void>
    void
    log(std::string const& s);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class>
bool
runner::run(suite_info const& s)
{
//启用“默认”测试用例
    default_ = true;
    failed_ = false;
    on_suite_begin(s);
    s.run(*this);
//忘记呼叫通过或失败。
    BOOST_ASSERT(cond_);
    on_case_end();
    on_suite_end();
    return failed_;
}

template<class FwdIter>
bool
runner::run(FwdIter first, FwdIter last)
{
    bool failed(false);
    for(;first != last; ++first)
        failed = run(*first) || failed;
    return failed;
}

template<class FwdIter, class Pred>
bool
runner::run_if(FwdIter first, FwdIter last, Pred pred)
{
    bool failed(false);
    for(;first != last; ++first)
        if(pred(*first))
            failed = run(*first) || failed;
    return failed;
}

template<class SequenceContainer>
bool
runner::run_each(SequenceContainer const& c)
{
    bool failed(false);
    for(auto const& s : c)
        failed = run(s) || failed;
    return failed;
}

template<class SequenceContainer, class Pred>
bool
runner::run_each_if(SequenceContainer const& c, Pred pred)
{
    bool failed(false);
    for(auto const& s : c)
        if(pred(s))
            failed = run(s) || failed;
    return failed;
}

template<class>
void
runner::testcase(std::string const& name)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
//名称不能为空
    BOOST_ASSERT(default_ || ! name.empty());
//忘记呼叫通过或失败
    BOOST_ASSERT(default_ || cond_);
    if(! default_)
        on_case_end();
    default_ = false;
    cond_ = false;
    on_case_begin(name);
}

template<class>
void
runner::pass()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_pass();
    cond_ = true;
}

template<class>
void
runner::fail(std::string const& reason)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_fail(reason);
    failed_ = true;
    cond_ = true;
}

template<class>
void
runner::log(std::string const& s)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(default_)
        testcase("");
    on_log(s);
}

} //单位试验
} //野兽

#endif
