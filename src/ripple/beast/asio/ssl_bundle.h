
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_ASIO_SSL_BUNDLE_H_INCLUDED
#define BEAST_ASIO_SSL_BUNDLE_H_INCLUDED

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <memory>
#include <utility>

namespace beast {
namespace asio {

/*解决不可移动的boost:：ssl:：stream。
    这允许ssl:：stream可移动，并允许stream
    从已存在的套接字构造。
**/

struct ssl_bundle
{
    using socket_type = boost::asio::ip::tcp::socket;
    using stream_type = boost::asio::ssl::stream <socket_type&>;
    using shared_context = std::shared_ptr<boost::asio::ssl::context>;

    template <class... Args>
    ssl_bundle (shared_context const& context_, Args&&... args);

//贬低
    template <class... Args>
    ssl_bundle (boost::asio::ssl::context& context_, Args&&... args);

    shared_context context;
    socket_type socket;
    stream_type stream;
};

template <class... Args>
ssl_bundle::ssl_bundle (shared_context const& context_, Args&&... args)
    : socket(std::forward<Args>(args)...)
    , stream (socket, *context_)
{
}

template <class... Args>
ssl_bundle::ssl_bundle (boost::asio::ssl::context& context_, Args&&... args)
    : socket(std::forward<Args>(args)...)
    , stream (socket, context_)
{
}

} //阿西奥
} //野兽

#endif
