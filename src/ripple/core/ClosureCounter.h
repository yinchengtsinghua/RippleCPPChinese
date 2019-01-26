
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Rippled的一部分：https://github.com/ripple/rippled
    版权所有（c）2017 Ripple Labs Inc.

    使用、复制、修改和/或分发本软件的权限
    特此授予免费或不收费的目的，前提是
    版权声明和本许可声明出现在所有副本中。

    本软件按“原样”提供，作者不作任何保证。
    关于本软件，包括
    适销性和适用性。在任何情况下，作者都不对
    任何特殊、直接、间接或后果性损害或任何损害
    因使用、数据或利润损失而导致的任何情况，无论是在
    合同行为、疏忽或其他侵权行为
    或与本软件的使用或性能有关。
**/

//==============================================================

#ifndef RIPPLE_CORE_CLOSURE_COUNTER_H_INCLUDED
#define RIPPLE_CORE_CLOSURE_COUNTER_H_INCLUDED

#include <ripple/basics/Log.h>
#include <boost/optional.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace ripple {

//一个对延迟的闭包进行引用计数的类——闭包
//谁的执行被计时器或队列延迟。参考计数
//允许停止以确保所有此类延迟关闭
//在stoppable声明自己已停止（）之前完成。
//
//ret_t是计数结束返回的类型。
//args_t是将传递给闭包的参数类型。
template <typename Ret_t, typename... Args_t>
class ClosureCounter
{
private:
    std::mutex mutable mutex_ {};
std::condition_variable allClosuresDoneCond_ {};  //带静音的警卫
bool waitForClosures_ {false};                    //带静音的警卫
    std::atomic<int> closureCount_ {0};

//增加计数。
    ClosureCounter& operator++()
    {
        ++closureCount_;
        return *this;
    }

//减少计数。如果我们停下来数到零
//通知所有关闭状态。
    ClosureCounter&  operator--()
    {
//尽管closureCount_u是原子的，但我们将其值递减到
//一把锁。这将删除一个小的计时窗口，如果
//等待线程正在处理关闭计数时的虚假唤醒
//下降到零。
        std::lock_guard<std::mutex> lock {mutex_};

//更新闭包计数。如果停止和关闭计数为0，则通知。
        if ((--closureCount_ == 0) && waitForClosures_)
            allClosuresDoneCond_.notify_all();
        return *this;
    }

//一个私有模板类，帮助计算闭包的数量
//在飞行中。这允许stoppers推迟声明stopped（）。
//直到所有延迟的关闭被发送。
    template <typename Closure>
    class Wrapper
    {
    private:
        ClosureCounter& counter_;
        std::remove_reference_t<Closure> closure_;

        static_assert (
            std::is_same<decltype(
                closure_(std::declval<Args_t>()...)), Ret_t>::value,
                "Closure arguments don't match ClosureCounter Ret_t or Args_t");

    public:
        Wrapper() = delete;

        Wrapper (Wrapper const& rhs)
        : counter_ (rhs.counter_)
        , closure_ (rhs.closure_)
        {
            ++counter_;
        }

        Wrapper (Wrapper&& rhs) noexcept(
          std::is_nothrow_move_constructible<Closure>::value)
        : counter_ (rhs.counter_)
        , closure_ (std::move (rhs.closure_))
        {
            ++counter_;
        }

        Wrapper (ClosureCounter& counter, Closure&& closure)
        : counter_ (counter)
        , closure_ (std::forward<Closure> (closure))
        {
            ++counter_;
        }

        Wrapper& operator=(Wrapper const& rhs) = delete;
        Wrapper& operator=(Wrapper&& rhs) = delete;

        ~Wrapper()
        {
            --counter_;
        }

//注意args_t不是推导出来的，它是显式的。所以阿格斯泰特& &
//将是右值引用，而不是转发引用。我们
//希望准确转发用户声明的内容。
        Ret_t operator ()(Args_t... args)
        {
            return closure_ (std::forward<Args_t>(args)...);
        }
    };

public:
    ClosureCounter() = default;
//不能复制或移动。未完成的计数很难分类。
    ClosureCounter (ClosureCounter const&) = delete;

    ClosureCounter& operator=(ClosureCounter const&) = delete;

    /*析构函数检验所有飞行中关闭是否完成。*/
    ~ClosureCounter()
    {
        using namespace std::chrono_literals;
        join ("ClosureCounter", 1s, debugLog());
    }

    /*在所有计数的飞行中关闭被销毁后返回。

        @param name name报告了加入时间是否超过等待时间。
        @param wait if join（）超过此持续时间，请向日志报告。
        如果超过等待时间，则写入@param j journal。
     **/

    void join (char const* name,
        std::chrono::milliseconds wait, beast::Journal j)
    {
        std::unique_lock<std::mutex> lock {mutex_};
        waitForClosures_ = true;
        if (closureCount_ > 0)
        {
            if (! allClosuresDoneCond_.wait_for (
                lock, wait, [this] { return closureCount_ == 0; }))
            {
                if (auto stream = j.error())
                    stream << name
                        << " waiting for ClosureCounter::join().";
                allClosuresDoneCond_.wait (
                    lock, [this] { return closureCount_ == 0; });
            }
        }
    }

    /*用引用计数器包装传递的闭包。

        @param closure闭包，接受参数并返回ret_t。
        @return if join（）已被调用，则返回boost：：none。否则
                返回Boost：：可选的，用
                参考计数器。
    **/

    template <class Closure>
    boost::optional<Wrapper<Closure>>
    wrap (Closure&& closure)
    {
        boost::optional<Wrapper<Closure>> ret;

        std::lock_guard<std::mutex> lock {mutex_};
        if (! waitForClosures_)
            ret.emplace (*this, std::forward<Closure> (closure));

        return ret;
    }

    /*当前未完成的关闭数。只对测试有用。*/
    int count() const
    {
        return closureCount_;
    }

    /*如果已联接，则返回true。

        即使返回“真”，计数的关闭仍可能在飞行中。
        但是，如果（joined（）&（count（）==0）），则不应该有更多
        计算飞行中的关闭次数。
    **/

    bool joined() const
    {
        std::lock_guard<std::mutex> lock {mutex_};
        return waitForClosures_;
    }
};

} //涟漪

#endif //包含Ripple_Core_Closing_Counter_H_
