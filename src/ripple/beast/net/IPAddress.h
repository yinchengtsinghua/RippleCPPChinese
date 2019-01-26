
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

#ifndef BEAST_NET_IPADDRESS_H_INCLUDED
#define BEAST_NET_IPADDRESS_H_INCLUDED

#include <ripple/beast/net/IPAddressV4.h>
#include <ripple/beast/net/IPAddressV6.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <boost/functional/hash.hpp>
#include <boost/asio/ip/address.hpp>
#include <cassert>
#include <cstdint>
#include <ios>
#include <string>
#include <sstream>
#include <typeinfo>

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace beast {
namespace IP {

using Address = boost::asio::ip::address;

/*返回以字符串表示的地址。*/
inline
std::string
to_string (Address const& addr)
{
    return addr.to_string ();
}

/*如果这是环回地址，则返回“true”。*/
inline
bool
is_loopback (Address const& addr)
{
    return addr.is_loopback();
}

/*如果未指定地址，则返回“true”。*/
inline
bool
is_unspecified (Address const& addr)
{
    return addr.is_unspecified();
}

/*如果地址是多播地址，则返回“true”。*/
inline
bool
is_multicast (Address const& addr)
{
    return addr.is_multicast();
}

/*如果地址是私人不可路由地址，则返回“true”。*/
inline
bool
is_private (Address const& addr)
{
    return (addr.is_v4 ())
        ? is_private (addr.to_v4 ())
        : is_private (addr.to_v6 ());
}

/*如果地址是公共可路由地址，则返回“true”。*/
inline
bool
is_public (Address const& addr)
{
    return (addr.is_v4 ())
        ? is_public (addr.to_v4 ())
        : is_public (addr.to_v6 ());
}

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Hasher>
void
hash_append(Hasher& h, beast::IP::Address const& addr) noexcept
{
    using beast::hash_append;
    if (addr.is_v4 ())
        hash_append(h, addr.to_v4().to_bytes());
    else if (addr.is_v6 ())
        hash_append(h, addr.to_v6().to_bytes());
    else
        assert (false);
}
}

namespace std {
template <>
struct hash <beast::IP::Address>
{
    explicit hash() = default;

    std::size_t
    operator() (beast::IP::Address const& addr) const
    {
        return beast::uhash<>{} (addr);
    }
};
}

namespace boost {
template <>
struct hash <::beast::IP::Address>
{
    explicit hash() = default;

    std::size_t
    operator() (::beast::IP::Address const& addr) const
    {
        return ::beast::uhash<>{} (addr);
    }
};
}

#endif
