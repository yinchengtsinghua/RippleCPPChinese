
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

#ifndef BEAST_UNIT_TEST_SUITE_HPP
#define BEAST_UNIT_TEST_SUITE_HPP

#include <beast/unit_test/runner.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include <ostream>
#include <sstream>
#include <string>

namespace beast {
namespace unit_test {

namespace detail {

template<class String>
static
std::string
make_reason(String const& reason,
    char const* file, int line)
{
    std::string s(reason);
    if(! s.empty())
        s.append(": ");
    namespace fs = boost::filesystem;
    s.append(fs::path{file}.filename().string());
    s.append("(");
    s.append(boost::lexical_cast<std::string>(line));
    s.append(")");
    return s;
}

} //细节

class thread;

enum abort_t
{
    no_abort_on_fail,
    abort_on_fail
};

/*TestSuite类。

    派生类执行一系列测试用例，其中每个测试用例
    一系列合格/不合格测试。要使用这个类提供单元测试，
    从中派生并在
    翻译单位。
**/

class suite
{
private:
    bool abort_ = false;
    bool aborted_ = false;
    runner* runner_ = nullptr;

//在内部引发此异常以停止当前套件
//在发生故障时，如果设置了停止选项。
    struct abort_exception : public std::exception
    {
        char const*
        what() const noexcept override
        {
            return "test suite aborted";
        }
    };

    template<class CharT, class Traits, class Allocator>
    class log_buf
        : public std::basic_stringbuf<CharT, Traits, Allocator>
    {
        suite& suite_;

    public:
        explicit
        log_buf(suite& self)
            : suite_(self)
        {
        }

        ~log_buf()
        {
            sync();
        }

        int
        sync() override
        {
            auto const& s = this->str();
            if(s.size() > 0)
                suite_.runner_->log(s);
            this->str("");
            return 0;
        }
    };

    template<
        class CharT,
        class Traits = std::char_traits<CharT>,
        class Allocator = std::allocator<CharT>
    >
    class log_os : public std::basic_ostream<CharT, Traits>
    {
        log_buf<CharT, Traits, Allocator> buf_;

    public:
        explicit
        log_os(suite& self)
            : std::basic_ostream<CharT, Traits>(&buf_)
            , buf_(self)
        {
        }
    };

    class scoped_testcase;

    class testcase_t
    {
        suite& suite_;
        std::stringstream ss_;

    public:
        explicit
        testcase_t(suite& self)
            : suite_(self)
        {
        }

        /*打开新的测试用例。

            测试用例是一系列经过评估的测试条件。测试
            套件可能有多个测试用例。测试与
            上次打开的测试用例。当测试首次运行时，默认
            未命名的案例已打开。只有一种情况的测试可以省略
            调用测试用例。

            @param abort确定套件失败后是否继续运行。
        **/

        void
        operator()(std::string const& name,
            abort_t abort = no_abort_on_fail);

        scoped_testcase
        operator()(abort_t abort);

        template<class T>
        scoped_testcase
        operator<<(T const& t);
    };

public:
    /*记录输出流。

        发送到日志输出流的文本将转发到
        与运行程序关联的输出流。
    **/

    log_os<char> log;

    /*用于声明测试用例的成员空间。*/
    testcase_t testcase;

    /*返回“当前”运行套件。
        如果没有运行套件，则返回nullptr。
    **/

    static
    suite*
    this_suite()
    {
        return *p_this_suite();
    }

    suite()
        : log(*this)
        , testcase(*this)
    {
    }

    virtual ~suite() = default;
    suite(suite const&) = delete;
    suite& operator=(suite const&) = delete;

    /*使用指定的运行程序调用测试。

        在此处设置数据成员，而不是将构造函数设置为
        编写派生类以避免重复
        已将构造函数参数转发到基。
        通常这是由框架为您调用的。
    **/

    template<class = void>
    void
    operator()(runner& r);

    /*记录成功的测试条件。*/
    template<class = void>
    void
    pass();

    /*记录故障。

        @param reason在失败时将可选文本添加到输出中。

        @param file测试失败的源代码文件。

        @param line测试失败的源代码行号。
    **/

    /*@ {*/
    template<class String>
    void
    fail(String const& reason, char const* file, int line);

    template<class = void>
    void
    fail(std::string const& reason = "");
    /*@ }*/

    /*评估测试条件。

        此函数通过合并
        失败时将文件名和行号输入到报告的输出中，
        以及调用方指定的附加文本。

        @param应返回要测试的条件。条件
        在布尔上下文中计算。

        @param reason可选在失败时向输出添加了文本。

        @param file测试失败的源代码文件。

        @param line测试失败的源代码行号。

        @return`true`如果测试条件指示成功。
    **/

    /*@ {*/
    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue)
    {
        return expect(shouldBeTrue, "");
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue, String const& reason);

    template<class Condition>
    bool
    expect(Condition const& shouldBeTrue,
        char const* file, int line)
    {
        return expect(shouldBeTrue, "", file, line);
    }

    template<class Condition, class String>
    bool
    expect(Condition const& shouldBeTrue,
        String const& reason, char const* file, int line);
    /*@ }*/

//
//贬低
//
//期望F（）出现异常
    template<class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class F>
    bool
    except(F&& f)
    {
        return except(f, "");
    }
    template<class E, class F, class String>
    bool
    except(F&& f, String const& reason);
    template<class E, class F>
    bool
    except(F&& f)
    {
        return except<E>(f, "");
    }
    template<class F, class String>
    bool
    unexcept(F&& f, String const& reason);
    template<class F>
    bool
    unexcept(F&& f)
    {
        return unexcept(f, "");
    }

    /*返回与运行程序关联的参数。*/
    std::string const&
    arg() const
    {
        return runner_->arg();
    }

//贬低
//@return`true`如果测试条件指示成功（错误值）
    template<class Condition, class String>
    bool
    unexpected(Condition shouldBeFalse,
        String const& reason);

    template<class Condition>
    bool
    unexpected(Condition shouldBeFalse)
    {
        return unexpected(shouldBeFalse, "");
    }

private:
    friend class thread;

    static
    suite**
    p_this_suite()
    {
        static suite* pts = nullptr;
        return &pts;
    }

    /*运行套房。*/
    virtual
    void
    run() = 0;

    void
    propagate_abort();

    template<class = void>
    void
    run(runner& r);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//流式测试用例名称帮助器
class suite::scoped_testcase
{
private:
    suite& suite_;
    std::stringstream& ss_;

public:
    scoped_testcase& operator=(scoped_testcase const&) = delete;

    ~scoped_testcase()
    {
        auto const& name = ss_.str();
        if(! name.empty())
            suite_.runner_->testcase(name);
    }

    scoped_testcase(suite& self, std::stringstream& ss)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
    }

    template<class T>
    scoped_testcase(suite& self,
            std::stringstream& ss, T const& t)
        : suite_(self)
        , ss_(ss)
    {
        ss_.clear();
        ss_.str({});
        ss_ << t;
    }

    template<class T>
    scoped_testcase&
    operator<<(T const& t)
    {
        ss_ << t;
        return *this;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline
void
suite::testcase_t::operator()(
    std::string const& name, abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    suite_.runner_->testcase(name);
}

inline
suite::scoped_testcase
suite::testcase_t::operator()(abort_t abort)
{
    suite_.abort_ = abort == abort_on_fail;
    return { suite_, ss_ };
}

template<class T>
inline
suite::scoped_testcase
suite::testcase_t::operator<<(T const& t)
{
    return { suite_, ss_, t };
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class>
void
suite::
operator()(runner& r)
{
    *p_this_suite() = this;
    try
    {
        run(r);
        *p_this_suite() = nullptr;
    }
    catch(...)
    {
        *p_this_suite() = nullptr;
        throw;
    }
}

template<class Condition, class String>
bool
suite::
expect(
    Condition const& shouldBeTrue, String const& reason)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(reason);
    return false;
}

template<class Condition, class String>
bool
suite::
expect(Condition const& shouldBeTrue,
    String const& reason, char const* file, int line)
{
    if(shouldBeTrue)
    {
        pass();
        return true;
    }
    fail(detail::make_reason(reason, file, line));
    return false;
}

//贬低

template<class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(...)
    {
        pass();
    }
    return true;
}

template<class E, class F, class String>
bool
suite::
except(F&& f, String const& reason)
{
    try
    {
        f();
        fail(reason);
        return false;
    }
    catch(E const&)
    {
        pass();
    }
    return true;
}

template<class F, class String>
bool
suite::
unexcept(F&& f, String const& reason)
{
    try
    {
        f();
        pass();
        return true;
    }
    catch(...)
    {
        fail(reason);
    }
    return false;
}

template<class Condition, class String>
bool
suite::
unexpected(
    Condition shouldBeFalse, String const& reason)
{
    bool const b =
        static_cast<bool>(shouldBeFalse);
    if(! b)
        pass();
    else
        fail(reason);
    return ! b;
}

template<class>
void
suite::
pass()
{
    propagate_abort();
    runner_->pass();
}

//失败
template<class>
void
suite::
fail(std::string const& reason)
{
    propagate_abort();
    runner_->fail(reason);
    if(abort_)
    {
        aborted_ = true;
        BOOST_THROW_EXCEPTION(abort_exception());
    }
}

template<class String>
void
suite::
fail(String const& reason, char const* file, int line)
{
    fail(detail::make_reason(reason, file, line));
}

inline
void
suite::
propagate_abort()
{
    if(abort_ && aborted_)
        BOOST_THROW_EXCEPTION(abort_exception());
}

template<class>
void
suite::
run(runner& r)
{
    runner_ = &r;

    try
    {
        run();
    }
    catch(abort_exception const&)
    {
//结束套房
    }
    catch(std::exception const& e)
    {
        runner_->fail("unhandled exception: " +
            std::string(e.what()));
    }
    catch(...)
    {
        runner_->fail("unhandled exception");
    }
}

#ifndef BEAST_EXPECT
/*检查前提条件。

    如果条件为假，则报告文件和行号。
**/

#define BEAST_EXPECT(cond) expect(cond, __FILE__, __LINE__)
#endif

#ifndef BEAST_EXPECTS
/*检查前提条件。

    如果条件为假，则报告文件和行号。
**/

#define BEAST_EXPECTS(cond, reason) ((cond) ? (pass(), true) : \
        (fail((reason), __FILE__, __LINE__), false))
#endif

} //单位试验
} //野兽

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//细节：
//这将插入带有给定手动标志的套件
#define BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,manual,priority) \
    static beast::unit_test::detail::insert_suite <Class##_test>   \
        Library ## Module ## Class ## _test_instance(             \
            #Class, #Module, #Library, manual, priority)

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于控制单元测试定义的预处理器指令。

//如果已经定义了它，不要重新定义它。这允许
//为测试套件定义提供自定义行为的程序
//
#ifndef BEAST_DEFINE_TESTSUITE

/*允许将测试套件插入全局容器。
    默认情况下，将所有测试套件定义插入到全局
    容器。如果beast_define_testsuite是用户定义的，则此宏
    没有效果。
**/

#ifndef BEAST_NO_UNIT_TEST_INLINE
#define BEAST_NO_UNIT_TEST_INLINE 0
#endif

/*定义单元测试套件。

    类表示要测试的类的类型。
    模块识别模块。
    库标识库。

    实现测试的类的声明应该相同
    作为U类测试。例如，如果类是老化的\订购的\容器，则
    测试类必须声明为：

    @代码

    结构老化的_有序的_容器_测试：Beast:：Unit_测试：：Suite
    {
        /…
    }；

    @端码

    宏调用必须出现在与测试类相同的命名空间中。
**/


#if BEAST_NO_UNIT_TEST_INLINE
#define BEAST_DEFINE_TESTSUITE(Class,Module,Library)
#define BEAST_DEFINE_TESTSUITE_MANUAL(Class,Module,Library)
#define BEAST_DEFINE_TESTSUITE_PRIO(Class,Module,Library,Priority)
#define BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Class,Module,Library,Priority)

#else
#include <beast/unit_test/global_suites.hpp>
#define BEAST_DEFINE_TESTSUITE(Class,Module,Library) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,false,0)
#define BEAST_DEFINE_TESTSUITE_MANUAL(Class,Module,Library) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,true,0)
#define BEAST_DEFINE_TESTSUITE_PRIO(Class,Module,Library,Priority) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,false,Priority)
#define BEAST_DEFINE_TESTSUITE_MANUAL_PRIO(Class,Module,Library,Priority) \
        BEAST_DEFINE_TESTSUITE_INSERT(Class,Module,Library,true,Priority)
#endif

#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#endif
