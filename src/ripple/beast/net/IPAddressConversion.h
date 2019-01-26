
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

#ifndef BEAST_NET_IPADDRESSCONVERSION_H_INCLUDED
#define BEAST_NET_IPADDRESSCONVERSION_H_INCLUDED

#include <ripple/beast/net/IPEndpoint.h>

#include <sstream>

#include <boost/asio.hpp>

namespace beast {
namespace IP {

/*转换为终结点。
    端口设置为零。
**/

Endpoint from_asio (boost::asio::ip::address const& address);

/*转换为终结点。*/
Endpoint from_asio (boost::asio::ip::tcp::endpoint const& endpoint);

/*转换为asio:：ip:：address。
    端口被忽略。
**/

boost::asio::ip::address to_asio_address (Endpoint const& endpoint);

/*转换为asio:：ip:：tcp:：endpoint。*/
boost::asio::ip::tcp::endpoint to_asio_endpoint (Endpoint const& endpoint);

}
}

namespace beast {

//贬低
struct IPAddressConversion
{
    explicit IPAddressConversion() = default;

    static IP::Endpoint from_asio (boost::asio::ip::address const& address)
        { return IP::from_asio (address); }
    static IP::Endpoint from_asio (boost::asio::ip::tcp::endpoint const& endpoint)
        { return IP::from_asio (endpoint); }
    static boost::asio::ip::address to_asio_address (IP::Endpoint const& address)
        { return IP::to_asio_address (address); }
    static boost::asio::ip::tcp::endpoint to_asio_endpoint (IP::Endpoint const& address)
        { return IP::to_asio_endpoint (address); }
};

}

#endif
