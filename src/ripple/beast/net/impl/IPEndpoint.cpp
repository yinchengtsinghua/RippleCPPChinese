
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

#include <ripple/beast/net/IPEndpoint.h>
#include <boost/algorithm/string.hpp>

namespace beast {
namespace IP {

Endpoint::Endpoint ()
    : m_port (0)
{
}

Endpoint::Endpoint (Address const& addr, Port port)
    : m_addr (addr)
    , m_port (port)
{
}

std::pair <Endpoint, bool> Endpoint::from_string_checked (std::string const& s)
{
    std::stringstream is (boost::trim_copy(s));
    Endpoint endpoint;
    is >> endpoint;
    if (! is.fail() && is.rdbuf()->in_avail() == 0)
        return std::make_pair (endpoint, true);
    return std::make_pair (Endpoint {}, false);
}

Endpoint Endpoint::from_string (std::string const& s)
{
    std::pair <Endpoint, bool> const result (
        from_string_checked (s));
    if (result.second)
        return result.first;
    return Endpoint {};
}

std::string Endpoint::to_string () const
{
    std::string s;
    s.reserve(
        (address().is_v6() ? INET6_ADDRSTRLEN-1 : 15) +
        (port() == 0 ? 0 : 6 + (address().is_v6() ? 2 : 0)));

    if (port() != 0 && address().is_v6())
        s += '[';
    s += address ().to_string();
    if (port())
    {
        if (address().is_v6())
            s += ']';
        s += ":" + std::to_string (port());
    }

    return s;
}

bool operator== (Endpoint const& lhs, Endpoint const& rhs)
{
    return lhs.address() == rhs.address() &&
           lhs.port() == rhs.port();
}

bool operator<  (Endpoint const& lhs, Endpoint const& rhs)
{
    if (lhs.address() < rhs.address())
        return true;
    if (lhs.address() > rhs.address())
        return false;
    return lhs.port() < rhs.port();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::istream& operator>> (std::istream& is, Endpoint& endpoint)
{
    std::string addrStr;
//有效地址只需要inet6_addrstrlen-1个字符，但允许额外的
//检查无效长度的字符
    addrStr.reserve(INET6_ADDRSTRLEN);
    char i {0};
    char readTo {0};
    is.get(i);
if (i == '[') //我们是IPv6端点
        readTo = ']';
    else
        addrStr+=i;

    while (is && is.rdbuf()->in_avail() > 0 && is.get(i))
    {
//注意：存在旧数据格式
//允许将空间用作地址/端口分隔符
//所以我们在这里继续尊重这一点，假设我们在最后
//如果我们碰到一个空格（或分隔符
//我们期待看到）
        if (isspace(static_cast<unsigned char>(i)) || (readTo && i == readTo))
            break;

        if ((i == '.') ||
            (i >= '0' && i <= ':') ||
            (i >= 'a' && i <= 'f') ||
            (i >= 'A' && i <= 'F'))
        {
            addrStr+=i;

//不要超过合理的长度…
            if ( addrStr.size() == INET6_ADDRSTRLEN ||
                (readTo && readTo == ':' && addrStr.size() > 15))
            {
                is.setstate (std::ios_base::failbit);
                return is;
            }

            if (! readTo && (i == '.' || i == ':'))
            {
//如果我们先看到一个点，必须是IPv4
//否则必须是无括号的IPv6
                readTo = (i == '.') ? ':' : ' ';
            }
        }
else //无效字符
        {
            is.unget();
            is.setstate (std::ios_base::failbit);
            return is;
        }
    }

    if (readTo == ']' && is.rdbuf()->in_avail() > 0)
    {
        is.get(i);
        if (! (isspace(static_cast<unsigned char>(i)) || i == ':'))
        {
            is.unget();
            is.setstate (std::ios_base::failbit);
            return is;
        }
    }

    boost::system::error_code ec;
    auto addr = Address::from_string(addrStr, ec);
    if (ec)
    {
        is.setstate (std::ios_base::failbit);
        return is;
    }

    if (is.rdbuf()->in_avail() > 0)
    {
        Port port;
        is >> port;
        if (is.fail())
            return is;
        endpoint = Endpoint (addr, port);
    }
    else
        endpoint = Endpoint (addr);

    return is;
}

}
}
