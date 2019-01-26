
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

#ifndef BEAST_TEST_YIELD_TO_HPP
#define BEAST_TEST_YIELD_TO_HPP

#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/optional.hpp>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace beast {
namespace test {

/*混合使用asio协程来支持测试。

    从此类派生并使用yield_启动测试
    协同程序内的函数。这对测试很方便
    异步ASIO代码。
**/

class enable_yield_to
{
protected:
    boost::asio::io_service ios_;

private:
    boost::optional<boost::asio::io_service::work> work_;
    std::vector<std::thread> threads_;
    std::mutex m_;
    std::condition_variable cv_;
    std::size_t running_ = 0;

public:
///传递给函数的yield上下文的类型。
    using yield_context =
        boost::asio::yield_context;

    explicit
    enable_yield_to(std::size_t concurrency = 1)
        : work_(ios_)
    {
        threads_.reserve(concurrency);
        while(concurrency--)
            threads_.emplace_back(
                [&]{ ios_.run(); });
    }

    ~enable_yield_to()
    {
        work_ = boost::none;
        for(auto& t : threads_)
            t.join();
    }

///返回与对象关联的“IO服务”
    boost::asio::io_service&
    get_io_service()
    {
        return ios_;
    }

    /*运行一个或多个函数，每个函数都在协程中。

        此调用将被阻止，直到所有协程终止。

        每个函数都应具有此签名：
        @代码
            void f（收益率上下文）；
        @端码

        PARAM FN…要调用的一个或多个函数。
    **/

#if BEAST_DOXYGEN
    template<class... FN>
    void
    yield_to(FN&&... fn)
#else
    template<class F0, class... FN>
    void
    yield_to(F0&& f0, FN&&... fn);
#endif

private:
    void
    spawn()
    {
    }

    template<class F0, class... FN>
    void
    spawn(F0&& f, FN&&... fn);
};

template<class F0, class... FN>
void
enable_yield_to::
yield_to(F0&& f0, FN&&... fn)
{
    running_ = 1 + sizeof...(FN);
    spawn(f0, fn...);
    std::unique_lock<std::mutex> lock{m_};
    cv_.wait(lock, [&]{ return running_ == 0; });
}

template<class F0, class... FN>
inline
void
enable_yield_to::
spawn(F0&& f, FN&&... fn)
{
    boost::asio::spawn(ios_,
        [&](yield_context yield)
        {
            f(yield);
            std::lock_guard<std::mutex> lock{m_};
            if(--running_ == 0)
                cv_.notify_all();
        }
        , boost::coroutines::attributes(2 * 1024 * 1024));
    spawn(fn...);
}

} //测试
} //野兽

#endif
