
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

#ifndef BEAST_UNIT_TEST_THREAD_HPP
#define BEAST_UNIT_TEST_THREAD_HPP

#include <beast/unit_test/suite.hpp>
#include <functional>
#include <thread>
#include <utility>

namespace beast {
namespace unit_test {

/*替换处理单元测试中异常的std:：thread。*/
class thread
{
private:
    suite* s_ = nullptr;
    std::thread t_;

public:
    using id = std::thread::id;
    using native_handle_type = std::thread::native_handle_type;

    thread() = default;
    thread(thread const&) = delete;
    thread& operator=(thread const&) = delete;

    thread(thread&& other)
        : s_(other.s_)
        , t_(std::move(other.t_))
    {
    }

    thread& operator=(thread&& other)
    {
        s_ = other.s_;
        t_ = std::move(other.t_);
        return *this;
    }

    template<class F, class... Args>
    explicit
    thread(suite& s, F&& f, Args&&... args)
        : s_(&s)
    {
        std::function<void(void)> b =
            std::bind(std::forward<F>(f),
                std::forward<Args>(args)...);
        t_ = std::thread(&thread::run, this,
            std::move(b));
    }

    bool
    joinable() const
    {
        return t_.joinable();
    }

    std::thread::id
    get_id() const
    {
        return t_.get_id();
    }

    static
    unsigned
    hardware_concurrency() noexcept
    {
        return std::thread::hardware_concurrency();
    }

    void
    join()
    {
        t_.join();
        s_->propagate_abort();
    }

    void
    detach()
    {
        t_.detach();
    }

    void
    swap(thread& other)
    {
        std::swap(s_, other.s_);
        std::swap(t_, other.t_);
    }

private:
    void
    run(std::function <void(void)> f)
    {
        try
        {
            f();
        }
        catch(suite::abort_exception const&)
        {
        }
        catch(std::exception const& e)
        {
            s_->fail("unhandled exception: " +
                std::string(e.what()));
        }
        catch(...)
        {
            s_->fail("unhandled exception");
        }
    }
};

} //单位试验
} //野兽

#endif
