
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
    版权所有（c）2014 Ripple Labs Inc.

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

#ifndef RIPPLE_PROTOCOL_UINTTYPES_H_INCLUDED
#define RIPPLE_PROTOCOL_UINTTYPES_H_INCLUDED

#include <ripple/basics/UnorderedContainers.h>
#include <ripple/basics/base_uint.h>
#include <ripple/protocol/AccountID.h>
#include <ripple/beast/utility/Zero.h>

namespace ripple {
namespace detail {

class CurrencyTag
{
public:
    explicit CurrencyTag() = default;
};

class DirectoryTag
{
public:
    explicit DirectoryTag() = default;
};

class NodeIDTag
{
public:
    explicit NodeIDTag() = default;
};

} //细节

/*目录是提供图书目录的索引。
    最后64位是质量。*/

using Directory = base_uint<256, detail::DirectoryTag>;

/*货币是表示特定货币的哈希。*/
using Currency = base_uint<160, detail::CurrencyTag>;

/*nodeid是表示一个节点的160位哈希。*/
using NodeID = base_uint<160, detail::NodeIDTag>;

/*XRP货币。*/
Currency const& xrpCurrency();

/*空货币的占位符。*/
Currency const& noCurrency();

/*我们故意不允许使用看起来像“xrp”的货币，因为
    很多人用它来代替正确的瑞波币。*/

Currency const& badCurrency();

inline bool isXRP(Currency const& c)
{
    return c == beast::zero;
}

/*返回“”、“xrp”或三个字母的ISO代码。*/
std::string to_string(Currency const& c);

/*尝试将字符串转换为货币，成功时返回true。*/
bool to_currency(Currency&, std::string const&);

/*尝试将字符串转换为货币，失败时返回nocurrency（）。*/
Currency to_currency(std::string const&);

inline std::ostream& operator<< (std::ostream& os, Currency const& x)
{
    os << to_string (x);
    return os;
}

} //涟漪

namespace std {

template <>
struct hash <ripple::Currency> : ripple::Currency::hasher
{
    explicit hash() = default;
};

template <>
struct hash <ripple::NodeID> : ripple::NodeID::hasher
{
    explicit hash() = default;
};

template <>
struct hash <ripple::Directory> : ripple::Directory::hasher
{
    explicit hash() = default;
};

} //性病

#endif
