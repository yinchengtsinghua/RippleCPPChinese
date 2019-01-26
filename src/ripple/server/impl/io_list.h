
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
    版权所有（c）2012，2013 Ripple Labs Inc.

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

#ifndef RIPPLE_SERVER_IO_LIST_H_INCLUDED
#define RIPPLE_SERVER_IO_LIST_H_INCLUDED

#include <boost/container/flat_map.hpp>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ripple {

/*管理执行异步I/O的一组对象。*/
class io_list final
{
public:
    class work
    {
        template<class = void>
        void destroy();

        friend class io_list;
        io_list* ios_ = nullptr;

    public:
        virtual ~work()
        {
            destroy();
        }

        /*返回与工作相关的IO_列表。

            要求：
                调用io_list：：emplace to
                创建工作已返回。
        **/

        io_list&
        ios()
        {
            return *ios_;
        }

        virtual void close() = 0;
    };

private:
    template<class = void>
    void destroy();

    std::mutex m_;
    std::size_t n_ = 0;
    bool closed_ = false;
    std::condition_variable cv_;
    boost::container::flat_map<work*,
        std::weak_ptr<work>> map_;
    std::function<void(void)> f_;

public:
    io_list() = default;

    /*销毁列表。

        影响：
            关闭IO列表（如果它以前不是
                关闭。在这种情况下，不调用分页装订器。

            封锁，直到所有工作被破坏。
    **/

    ~io_list()
    {
        destroy();
    }

    /*如果列表已关闭，则返回“true”。

        线程安全：
            如果同时调用，则返回未定义的结果
            与关闭（）。
    **/

    bool
    closed() const
    {
        return closed_;
    }

    /*如果未关闭，则创建关联工作。

        要求：
            `std:：is_base_of_v<work，t>==true`

        线程安全：
            可以同时调用。

        影响：
            自动创建、插入和返回新的
            如果IO列表为，则返回nullptr。
            关闭，

        如果调用成功并返回新对象，
        保证随后的关闭调用
        将在对象上调用Work:：Close。

    **/

    template <class T, class... Args>
    std::shared_ptr<T>
    emplace(Args&&... args);

    /*取消活动I/O。

        线程安全：
            不能同时调用。

        影响：
            关联工作已关闭。

            如果提供分页装订器，将在
            所有相关工作都将被销毁。整理者
            可以从外部线程调用，也可以在
            对该函数的调用。

            只有第一个要关闭的调用将设置
            整理者。

            第一次呼叫后无效。
    **/

    template<class Finisher>
    void
    close(Finisher&& f);

    void
    close()
    {
        close([]{});
    }

    /*阻止，直到IO列表停止。
        
        影响：
            在IO列表
            关闭并销毁所有相关工作。

        线程安全：
            可以同时调用。

        先决条件：
            不调用IO服务：在任何IO服务上运行
            由与此IO列表关联的工作对象使用
            存在于调用方的调用堆栈中。
    **/

    template<class = void>
    void
    join();
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class>
void
io_list::work::destroy()
{
    if(! ios_)
        return;
    std::function<void(void)> f;
    {
        std::lock_guard<
            std::mutex> lock(ios_->m_);
        ios_->map_.erase(this);
        if(--ios_->n_ == 0 &&
            ios_->closed_)
        {
            std::swap(f, ios_->f_);
            ios_->cv_.notify_all();
        }
    }
    if(f)
        f();
}

template<class>
void
io_list::destroy()
{
    close();
    join();
}

template <class T, class... Args>
std::shared_ptr<T>
io_list::emplace(Args&&... args)
{
    static_assert(std::is_base_of<work, T>::value,
        "T must derive from io_list::work");
    if(closed_)
        return nullptr;
    auto sp = std::make_shared<T>(
        std::forward<Args>(args)...);
    decltype(sp) dead;

    std::lock_guard<std::mutex> lock(m_);
    if(! closed_)
    {
        ++n_;
        sp->work::ios_ = this;
        map_.emplace(sp.get(), sp);
    }
    else
    {
        std::swap(sp, dead);
    }
    return sp;
}

template<class Finisher>
void
io_list::close(Finisher&& f)
{
    std::unique_lock<std::mutex> lock(m_);
    if(closed_)
        return;
    closed_ = true;
    auto map = std::move(map_);
    if(! map.empty())
    {
        f_ = std::forward<Finisher>(f);
        lock.unlock();
        for(auto const& p : map)
            if(auto sp = p.second.lock())
                sp->close();
    }
    else
    {
        lock.unlock();
        f();
    }
}

template<class>
void
io_list::join()
{
    std::unique_lock<std::mutex> lock(m_);
    cv_.wait(lock,
        [&]
        {
            return closed_ && n_ == 0;
        });
}

} //涟漪

#endif
