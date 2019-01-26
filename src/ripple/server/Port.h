
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

#ifndef RIPPLE_SERVER_PORT_H_INCLUDED
#define RIPPLE_SERVER_PORT_H_INCLUDED

#include <ripple/basics/BasicConfig.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/beast/core/string.hpp>
#include <boost/beast/websocket/option.hpp>
#include <boost/asio/ip/address.hpp>
#include <cstdint>
#include <memory>
#include <set>
#include <string>

namespace boost { namespace asio { namespace ssl { class context; } } }

namespace ripple {

/*服务器侦听端口的配置信息。*/
struct Port
{
    explicit Port() = default;

    std::string name;
    boost::asio::ip::address ip;
    std::uint16_t port = 0;
    std::set<std::string, boost::beast::iless> protocol;
    std::vector<beast::IP::Address> admin_ip;
    std::vector<beast::IP::Address> secure_gateway_ip;
    std::string user;
    std::string password;
    std::string admin_user;
    std::string admin_password;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_chain;
    std::string ssl_ciphers;
    boost::beast::websocket::permessage_deflate pmd_options;
    std::shared_ptr<boost::asio::ssl::context> context;

//此上允许多少个传入连接
//端口在[0，65535]范围内，其中0表示无限制。
    int limit = 0;

//如果发送队列超过此限制，WebSocket将断开连接
    std::uint16_t ws_queue_limit;

//如果指定了任何WebSocket协议，则返回“true”
    bool websockets() const;

//如果指定了任何安全协议，则返回“true”
    bool secure() const;

//返回包含协议列表的字符串
    std::string protocols() const;
};

std::ostream&
operator<< (std::ostream& os, Port const& p);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

struct ParsedPort
{
    explicit ParsedPort() = default;

    std::string name;
    std::set<std::string, boost::beast::iless> protocol;
    std::string user;
    std::string password;
    std::string admin_user;
    std::string admin_password;
    std::string ssl_key;
    std::string ssl_cert;
    std::string ssl_chain;
    std::string ssl_ciphers;
    boost::beast::websocket::permessage_deflate pmd_options;
    int limit = 0;
    std::uint16_t ws_queue_limit;

    boost::optional<boost::asio::ip::address> ip;
    boost::optional<std::uint16_t> port;
    boost::optional<std::vector<beast::IP::Address>> admin_ip;
    boost::optional<std::vector<beast::IP::Address>> secure_gateway_ip;
};

void
parse_Port (ParsedPort& port, Section const& section, std::ostream& log);

} //涟漪

#endif
