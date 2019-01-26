
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

#ifndef BEAST_UNIT_TEST_RESULTS_HPP
#define BEAST_UNIT_TEST_RESULTS_HPP

#include <beast/unit_test/detail/const_container.hpp>

#include <string>
#include <vector>

namespace beast {
namespace unit_test {

/*在测试用例中保存一组测试条件结果。*/
class case_results
{
public:
    /*保存评估一个测试条件的结果。*/
    struct test
    {
        explicit test(bool pass_)
            : pass(pass_)
        {
        }

        test(bool pass_, std::string const& reason_)
            : pass(pass_)
            , reason(reason_)
        {
        }

        bool pass;
        std::string reason;
    };

private:
    class tests_t
        : public detail::const_container <std::vector <test>>
    {
    private:
        std::size_t failed_;

    public:
        tests_t()
            : failed_(0)
        {
        }

        /*返回测试条件的总数。*/
        std::size_t
        total() const
        {
            return cont().size();
        }

        /*返回失败的测试条件数。*/
        std::size_t
        failed() const
        {
            return failed_;
        }

        /*注册成功的测试条件。*/
        void
        pass()
        {
            cont().emplace_back(true);
        }

        /*注册失败的测试条件。*/
        void
        fail(std::string const& reason = "")
        {
            ++failed_;
            cont().emplace_back(false, reason);
        }
    };

    class log_t
        : public detail::const_container <std::vector <std::string>>
    {
    public:
        /*在日志中插入字符串。*/
        void
        insert(std::string const& s)
        {
            cont().push_back(s);
        }
    };

    std::string name_;

public:
    explicit case_results(std::string const& name = "")
        : name_(name)
    {
    }

    /*返回此测试用例的名称。*/
    std::string const&
    name() const
    {
        return name_;
    }

    /*测试条件结果容器的成员空间。*/
    tests_t tests;

    /*测试用例日志消息容器的成员空间。*/
    log_t log;
};

//——————————————————————————————————————————————————————————————

/*将一组测试用例结果保存在一个套件中。*/
class suite_results
    : public detail::const_container <std::vector <case_results>>
{
private:
    std::string name_;
    std::size_t total_ = 0;
    std::size_t failed_ = 0;

public:
    explicit suite_results(std::string const& name = "")
        : name_(name)
    {
    }

    /*返回此套件的名称。*/
    std::string const&
    name() const
    {
        return name_;
    }

    /*返回测试条件的总数。*/
    std::size_t
    total() const
    {
        return total_;
    }

    /*返回失败数。*/
    std::size_t
    failed() const
    {
        return failed_;
    }

    /*插入一组测试用例结果。*/
    /*@ {*/
    void
    insert(case_results&& r)
    {
        cont().emplace_back(std::move(r));
        total_ += r.tests.total();
        failed_ += r.tests.failed();
    }

    void
    insert(case_results const& r)
    {
        cont().push_back(r);
        total_ += r.tests.total();
        failed_ += r.tests.failed();
    }
    /*@ }*/
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//vfalc todo使用作用域分配器将其设置为模板类
/*保存运行一组测试套件的结果。*/
class results
    : public detail::const_container <std::vector <suite_results>>
{
private:
    std::size_t m_cases;
    std::size_t total_;
    std::size_t failed_;

public:
    results()
        : m_cases(0)
        , total_(0)
        , failed_(0)
    {
    }

    /*返回测试用例总数。*/
    std::size_t
    cases() const
    {
        return m_cases;
    }

    /*返回测试条件的总数。*/
    std::size_t
    total() const
    {
        return total_;
    }

    /*返回失败数。*/
    std::size_t
    failed() const
    {
        return failed_;
    }

    /*插入一组套件结果。*/
    /*@ {*/
    void
    insert(suite_results&& r)
    {
        m_cases += r.size();
        total_ += r.total();
        failed_ += r.failed();
        cont().emplace_back(std::move(r));
    }

    void
    insert(suite_results const& r)
    {
        m_cases += r.size();
        total_ += r.total();
        failed_ += r.failed();
        cont().push_back(r);
    }
    /*@ }*/
};

} //单位试验
} //野兽

#endif
