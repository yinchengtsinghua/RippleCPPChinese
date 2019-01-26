
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

#ifndef RIPPLE_PEERFINDER_CHECKER_H_INCLUDED
#define RIPPLE_PEERFINDER_CHECKER_H_INCLUDED

#include <ripple/beast/net/IPAddressConversion.h>
#include <boost/asio/detail/handler_invoke_helpers.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/intrusive/list.hpp>
#include <boost/system/error_code.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

namespace ripple {
namespace PeerFinder {

/*测试远程侦听套接字以确保它们是可连接的。*/
    template <class Protocol = boost::asio::ip::tcp>
class Checker
{
private:
    using error_code = boost::system::error_code;

    struct basic_async_op : boost::intrusive::list_base_hook <
        boost::intrusive::link_mode <boost::intrusive::normal_link>>
    {
        virtual
        ~basic_async_op() = default;

        virtual
        void
        stop() = 0;

        virtual
        void
        operator() (error_code const& ec) = 0;
    };

    template <class Handler>
    struct async_op : basic_async_op
    {
        using socket_type = typename Protocol::socket;
        using endpoint_type = typename Protocol::endpoint;

        Checker& checker_;
        socket_type socket_;
        Handler handler_;

        async_op (Checker& owner, boost::asio::io_service& io_service,
            Handler&& handler);

        ~async_op();

        void
        stop() override;

        void
        operator() (error_code const& ec) override;
    };

//——————————————————————————————————————————————————————————————

    using list_type = typename boost::intrusive::make_list <basic_async_op,
        boost::intrusive::constant_time_size <true>>::type;

    std::mutex mutex_;
    std::condition_variable cond_;
    boost::asio::io_service& io_service_;
    list_type list_;
    bool stop_ = false;

public:
    explicit
    Checker (boost::asio::io_service& io_service);

    /*破坏服务。
        将取消所有挂起的I/O操作。此呼叫阻止到
        所有挂起的操作都已完成（成功或
        操作已中止），相关线程和IO服务
        没有剩余工作。
    **/

    ~Checker();

    /*停止服务。
        将取消挂起的I/O操作。
        此问题将取消所有待定I/O操作的订单，然后
        立即返回。处理程序将接收操作\中止的错误，
        或者如果他们已经排队，他们将正常完成。
    **/

    void
    stop();

    /*阻止，直到所有挂起的I/O完成。*/
    void
    wait();

    /*对指定的终结点执行异步连接测试。
        端口必须为非零。注意执行保证
        由ASIO处理程序提供的不会强制执行。
    **/

    template <class Handler>
    void
    async_connect (beast::IP::Endpoint const& endpoint, Handler&& handler);

private:
    void
    remove (basic_async_op& op);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Protocol>
template <class Handler>
Checker<Protocol>::async_op<Handler>::async_op (Checker& owner,
        boost::asio::io_service& io_service, Handler&& handler)
    : checker_ (owner)
    , socket_ (io_service)
    , handler_ (std::forward<Handler>(handler))
{
}

template <class Protocol>
template <class Handler>
Checker<Protocol>::async_op<Handler>::~async_op()
{
    checker_.remove (*this);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_op<Handler>::stop()
{
    error_code ec;
    socket_.cancel(ec);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_op<Handler>::operator() (
    error_code const& ec)
{
    handler_(ec);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Protocol>
Checker<Protocol>::Checker (boost::asio::io_service& io_service)
    : io_service_(io_service)
{
}

template <class Protocol>
Checker<Protocol>::~Checker()
{
    wait();
}

template <class Protocol>
void
Checker<Protocol>::stop()
{
    std::lock_guard<std::mutex> lock (mutex_);
    if (! stop_)
    {
        stop_ = true;
        for (auto& c : list_)
            c.stop();
    }
}

template <class Protocol>
void
Checker<Protocol>::wait()
{
    std::unique_lock<std::mutex> lock (mutex_);
    while (! list_.empty())
        cond_.wait (lock);
}

template <class Protocol>
template <class Handler>
void
Checker<Protocol>::async_connect (
    beast::IP::Endpoint const& endpoint, Handler&& handler)
{
    auto const op = std::make_shared<async_op<Handler>> (
        *this, io_service_, std::forward<Handler>(handler));
    {
        std::lock_guard<std::mutex> lock (mutex_);
        list_.push_back (*op);
    }
    op->socket_.async_connect (beast::IPAddressConversion::to_asio_endpoint (
        endpoint), std::bind (&basic_async_op::operator(), op,
            std::placeholders::_1));
}

template <class Protocol>
void
Checker<Protocol>::remove (basic_async_op& op)
{
    std::lock_guard <std::mutex> lock (mutex_);
    list_.erase (list_.iterator_to (op));
    if (list_.size() == 0)
        cond_.notify_all();
}

}
}

#endif
