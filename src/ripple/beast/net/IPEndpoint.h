
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

#ifndef BEAST_NET_IPENDPOINT_H_INCLUDED
#define BEAST_NET_IPENDPOINT_H_INCLUDED

#include <ripple/beast/net/IPAddress.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <cstdint>
#include <ios>
#include <string>

namespace beast {
namespace IP {

using Port = std::uint16_t;

/*独立于版本的IP地址和端口组合。*/
class Endpoint
{
public:
    /*创建未指定的终结点。*/
    Endpoint ();

    /*从地址和可选端口创建端点。*/
    explicit Endpoint (Address const& addr, Port port = 0);

    /*从字符串创建端点。
        如果省略端口，则端点将具有零端口。
        @返回一对端点，bool成功时设置为“true”。
    **/

    static std::pair <Endpoint, bool> from_string_checked (std::string const& s);
    static Endpoint from_string (std::string const& s);

    /*返回表示终结点的字符串。*/
    std::string to_string () const;

    /*返回终结点上的端口号。*/
    Port port () const
        { return m_port; }

    /*返回具有不同端口的新终结点。*/
    Endpoint at_port (Port port) const
        { return Endpoint (m_addr, port); }

    /*返回此终结点的地址部分。*/
    Address const& address () const
        { return m_addr; }

    /*地址部分的便利访问器。*/
    /*@ {*/
    bool is_v4 () const
        { return m_addr.is_v4(); }
    bool is_v6 () const
        { return m_addr.is_v6(); }
    AddressV4 const to_v4 () const
        { return m_addr.to_v4 (); }
    AddressV6 const to_v6 () const
        { return m_addr.to_v6 (); }
    /*@ }*/

    /*算术比较。*/
    /*@ {*/
    friend bool operator== (Endpoint const& lhs, Endpoint const& rhs);
    friend bool operator<  (Endpoint const& lhs, Endpoint const& rhs);

    friend bool operator!= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (lhs == rhs); }
    friend bool operator>  (Endpoint const& lhs, Endpoint const& rhs)
        { return rhs < lhs; }
    friend bool operator<= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (lhs > rhs); }
    friend bool operator>= (Endpoint const& lhs, Endpoint const& rhs)
        { return ! (rhs > lhs); }
    /*@ }*/

    template <class Hasher>
    friend
    void
    hash_append (Hasher& h, Endpoint const& endpoint)
    {
        using ::beast::hash_append;
        hash_append(h, endpoint.m_addr, endpoint.m_port);
    }

private:
    Address m_addr;
    Port m_port;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//性质

/*如果终结点是环回地址，则返回“true”。*/
inline bool  is_loopback (Endpoint const& endpoint)
    { return is_loopback (endpoint.address ()); }

/*如果未指定终结点，则返回“true”。*/
inline bool  is_unspecified (Endpoint const& endpoint)
    { return is_unspecified (endpoint.address ()); }

/*如果终结点是多播地址，则返回“true”。*/
inline bool  is_multicast (Endpoint const& endpoint)
    { return is_multicast (endpoint.address ()); }

/*如果终结点是专用的不可路由地址，则返回“true”。*/
inline bool  is_private (Endpoint const& endpoint)
    { return is_private (endpoint.address ()); }

/*如果终结点是公共可路由地址，则返回“true”。*/
inline bool  is_public (Endpoint const& endpoint)
    { return is_public (endpoint.address ()); }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回以字符串表示的终结点。*/
inline std::string to_string (Endpoint const& endpoint)
    { return endpoint.to_string(); }

/*输出流转换。*/
template <typename OutputStream>
OutputStream& operator<< (OutputStream& os, Endpoint const& endpoint)
{
    os << to_string (endpoint);
    return os;
}

/*输入流转换。*/
std::istream& operator>> (std::istream& is, Endpoint& endpoint);

}
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace std {
/*std：：哈希支持。*/
template <>
struct hash <::beast::IP::Endpoint>
{
    explicit hash() = default;

    std::size_t operator() (::beast::IP::Endpoint const& endpoint) const
        { return ::beast::uhash<>{} (endpoint); }
};
}

namespace boost {
/*boost：：哈希支持。*/
template <>
struct hash <::beast::IP::Endpoint>
{
    explicit hash() = default;

    std::size_t operator() (::beast::IP::Endpoint const& endpoint) const
        { return ::beast::uhash<>{} (endpoint); }
};
}

#endif
