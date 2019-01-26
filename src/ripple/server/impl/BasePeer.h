
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

#ifndef RIPPLE_SERVER_BASEPEER_H_INCLUDED
#define RIPPLE_SERVER_BASEPEER_H_INCLUDED

#include <ripple/server/Port.h>
#include <ripple/server/impl/io_list.h>
#include <ripple/beast/utility/WrappedSink.h>
#include <boost/asio.hpp>
#include <atomic>
#include <cassert>
#include <functional>
#include <string>

namespace ripple {

//所有同龄人的共同部分
template<class Handler, class Impl>
class BasePeer
    : public io_list::work
{
protected:
    using clock_type = std::chrono::system_clock;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using waitable_timer = boost::asio::basic_waitable_timer <clock_type>;

    Port const& port_;
    Handler& handler_;
    endpoint_type remote_address_;
    beast::WrappedSink sink_;
    beast::Journal j_;

    boost::asio::io_service::work work_;
    boost::asio::io_service::strand strand_;

public:
    BasePeer(Port const& port, Handler& handler,
        endpoint_type remote_address,
            boost::asio::io_service& io_service,
                beast::Journal journal);

    void
    close() override;

private:
    Impl&
    impl()
    {
        return *static_cast<Impl*>(this);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<class Handler, class Impl>
BasePeer<Handler, Impl>::
BasePeer(Port const& port, Handler& handler,
    endpoint_type remote_address,
        boost::asio::io_service& io_service,
            beast::Journal journal)
    : port_(port)
    , handler_(handler)
    , remote_address_(remote_address)
    , sink_(journal.sink(),
        []
        {
            static std::atomic<unsigned> id{0};
            return "##" + std::to_string(++id) + " ";
        }())
    , j_(sink_)
    , work_(io_service)
    , strand_(io_service)
{
}

template<class Handler, class Impl>
void
BasePeer<Handler, Impl>::
close()
{
    if (! strand_.running_in_this_thread())
        return strand_.post(std::bind(
            &BasePeer::close, impl().shared_from_this()));
    error_code ec;
    impl().ws_.lowest_layer().close(ec);
}

} //涟漪

#endif
