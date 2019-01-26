
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

#ifndef RIPPLE_SERVER_SERVERIMPL_H_INCLUDED
#define RIPPLE_SERVER_SERVERIMPL_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/server/Server.h>
#include <ripple/server/impl/Door.h>
#include <ripple/server/impl/io_list.h>
#include <ripple/beast/core/List.h>
#include <boost/asio.hpp>
#include <boost/optional.hpp>
#include <array>
#include <chrono>
#include <mutex>

namespace ripple {

using Endpoints = std::vector<boost::asio::ip::tcp::endpoint>;

/*多协议服务器。

    此服务器维护多个已配置的侦听端口，
    每个监听端口允许多种协议，包括
    HTTP、HTTP/S、WebSocket、安全WebSocket和对等协议。
**/

class Server
{
public:
    /*销毁服务器。
        如果服务器尚未关闭，则它将关闭。这个电话
        在服务器停止之前阻止。
    **/

    virtual
    ~Server() = default;

    /*返回与服务器关联的日志。*/
    virtual
    beast::Journal
    journal() = 0;

    /*设置监听端口设置。
        只能调用一次。
    **/

    virtual
    Endpoints
    ports (std::vector<Port> const& v) = 0;

    /*关闭服务器。
        关闭是异步执行的。将通知处理程序
        当服务器停止时。当
        没有挂起的I/O完成处理程序和所有连接
        已经关闭。
        线程安全：
            可以从任何线程同时调用。
    **/

    virtual
    void
    close() = 0;
};

template<class Handler>
class ServerImpl : public Server
{
private:
    using clock_type = std::chrono::system_clock;

    enum
    {
        historySize = 100
    };

    Handler& handler_;
    beast::Journal j_;
    boost::asio::io_service& io_service_;
    boost::asio::io_service::strand strand_;
    boost::optional <boost::asio::io_service::work> work_;

    std::mutex m_;
    std::vector<Port> ports_;
    std::vector<std::weak_ptr<Door<Handler>>> list_;
    int high_ = 0;
    std::array <std::size_t, 64> hist_;

    io_list ios_;

public:
    ServerImpl(Handler& handler,
        boost::asio::io_service& io_service, beast::Journal journal);

    ~ServerImpl();

    beast::Journal
    journal() override
    {
        return j_;
    }

    Endpoints
    ports (std::vector<Port> const& ports) override;

    void
    close() override;

    io_list&
    ios()
    {
        return ios_;
    }

    boost::asio::io_service&
    get_io_service()
    {
        return io_service_;
    }

    bool
    closed();

private:
    static
    int
    ceil_log2 (unsigned long long x);
};

template<class Handler>
ServerImpl<Handler>::
ServerImpl(Handler& handler,
        boost::asio::io_service& io_service, beast::Journal journal)
    : handler_(handler)
    , j_(journal)
    , io_service_(io_service)
    , strand_(io_service_)
    , work_(io_service_)
{
}

template<class Handler>
ServerImpl<Handler>::
~ServerImpl()
{
//不会调用处理程序：：OnStop
    work_ = boost::none;
    ios_.close();
    ios_.join();
}

template<class Handler>
Endpoints
ServerImpl<Handler>::
ports (std::vector<Port> const& ports)
{
    if (closed())
        Throw<std::logic_error> ("ports() on closed Server");
    ports_.reserve(ports.size());
    Endpoints eps;
    eps.reserve(ports.size());
    for(auto const& port : ports)
    {
        ports_.push_back(port);
        if(auto sp = ios_.emplace<Door<Handler>>(handler_,
            io_service_, ports_.back(), j_))
        {
            list_.push_back(sp);
            eps.push_back(sp->get_endpoint());
            sp->run();
        }
    }
    return eps;
}

template<class Handler>
void
ServerImpl<Handler>::
close()
{
    ios_.close(
    [&]
    {
        work_ = boost::none;
        handler_.onStopped(*this);
    });
}

template<class Handler>
bool
ServerImpl<Handler>::
closed()
{
    return ios_.closed();
}
} //涟漪

#endif
