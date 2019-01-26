
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

#ifndef BEAST_TEST_FAIL_COUNTER_HPP
#define BEAST_TEST_FAIL_COUNTER_HPP

#include <beast/core/error.hpp>
#include <boost/throw_exception.hpp>

namespace beast {
namespace test {

enum class error
{
    fail_error = 1
};

namespace detail {

class fail_error_category : public boost::system::error_category
{
public:
    const char*
    name() const noexcept override
    {
        return "test";
    }

    std::string
    message(int ev) const override
    {
        switch(static_cast<error>(ev))
        {
        default:
        case error::fail_error:
            return "test error";
        }
    }

    boost::system::error_condition
    default_error_condition(int ev) const noexcept override
    {
        return boost::system::error_condition{ev, *this};
    }

    bool
    equivalent(int ev,
        boost::system::error_condition const& condition
            ) const noexcept override
    {
        return condition.value() == ev &&
            &condition.category() == this;
    }

    bool
    equivalent(error_code const& error, int ev) const noexcept override
    {
        return error.value() == ev &&
            &error.category() == this;
    }
};

inline
boost::system::error_category const&
get_error_category()
{
    static fail_error_category const cat{};
    return cat;
}

} //细节

inline
error_code
make_error_code(error ev)
{
    return error_code{
        static_cast<std::underlying_type<error>::type>(ev),
            detail::get_error_category()};
}

/*在默认构造上设置了错误的错误代码

    此对象的默认构造版本将具有
    立即设置错误代码。这有助于测试查找代码
    成功时忘记清除错误代码。
**/

struct fail_error_code : error_code
{
    fail_error_code()
        : error_code(make_error_code(error::fail_error))
    {
    }

    template<class Arg0, class... ArgN>
    fail_error_code(Arg0&& arg0, ArgN&&... argn)
        : error_code(arg0, std::forward<ArgN>(argn)...)
    {
    }
};

/*模拟失败的倒计时。

    在第n个操作上，类将失败，指定的
    错误代码，或@ref error:：fail_error的默认错误代码。
**/

class fail_counter
{
    std::size_t n_;
    error_code ec_;

public:
    fail_counter(fail_counter&&) = default;

    /*构建一个计数器。

        @param要在或之后失败的操作的从0开始的索引。
    **/

    explicit
    fail_counter(std::size_t n,
            error_code ev = make_error_code(error::fail_error))
        : n_(n)
        , ec_(ev)
    {
    }

///在第n次失败时引发异常
    void
    fail()
    {
        if(n_ > 0)
            --n_;
        if(! n_)
            BOOST_THROW_EXCEPTION(system_error{ec_});
    }

///在第n次失败时设置错误代码
    bool
    fail(error_code& ec)
    {
        if(n_ > 0)
            --n_;
        if(! n_)
        {
            ec = ec_;
            return true;
        }
        ec.assign(0, ec.category());
        return false;
    }
};

} //测试
} //野兽

namespace boost {
namespace system {
template<>
struct is_error_code_enum<beast::test::error>
{
    static bool const value = true;
};
} //系统
} //促进

#endif
