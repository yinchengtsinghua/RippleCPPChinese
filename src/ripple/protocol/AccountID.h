
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

#ifndef RIPPLE_PROTOCOL_ACCOUNTID_H_INCLUDED
#define RIPPLE_PROTOCOL_ACCOUNTID_H_INCLUDED

#include <ripple/protocol/tokens.h>
//解决标题问题时vfalc取消注释
//include<ripple/protocol/publickey.h>
#include <ripple/basics/base_uint.h>
#include <ripple/basics/UnorderedContainers.h>
#include <ripple/json/json_value.h>
#include <boost/optional.hpp>
#include <cstddef>
#include <mutex>
#include <string>

namespace ripple {

namespace detail {

class AccountIDTag
{
public:
    explicit AccountIDTag() = default;
};

} //细节

/*唯一标识一个帐户的160位无符号。*/
using AccountID = base_uint<
    160, detail::AccountIDTag>;

/*将accountID转换为base58选中字符串*/
std::string
toBase58 (AccountID const& v);

/*从checked分析accountID，base58字符串。
    @return boost：：如果出现分析错误，则无
**/

template<>
boost::optional<AccountID>
parseBase58 (std::string const& s);

//使用比特币字母表解析accountID
//这是为了捕获用户错误。可能不需要
//贬低
boost::optional<AccountID>
deprecatedParseBitcoinAccountID (std::string const& s);

//与旧代码的兼容性
bool
deprecatedParseBase58 (AccountID& account,
    Json::Value const& jv);

/*从十六进制字符串分析accountID

    如果字符串不完全是40
    十六进制数字，boost：：无返回。

    @return boost：：如果出现分析错误，则无
**/

template<>
boost::optional<AccountID>
parseHex (std::string const& s);

/*从十六进制或检查的base58字符串分析accountID。

    @return boost：：如果出现分析错误，则无
**/

template<>
boost::optional<AccountID>
parseHexOrBase58 (std::string const& s);

/*从公钥计算accountID。

    帐户ID计算为
    公钥数据。这不包括版本字节和
    base58表示中包含的保护字节。

**/

//vfalco现在在publickey.h中
//帐户编号
//calccountid（publickey const&pk）；

/*用作xrp“发行人”的特殊账户。*/
AccountID const&
xrpAccount();

/*空帐户的占位符。*/
AccountID const&
noAccount();

/*将十六进制或base58字符串转换为accountID。

    @如果分析成功，返回'true'。
**/

//贬低
bool
to_issuer (AccountID&, std::string const&);

//不推荐使用的应检查货币或本机标志
inline
bool
isXRP(AccountID const& c)
{
    return c == beast::zero;
}

//贬低
inline
std::string
to_string (AccountID const& account)
{
    return toBase58(account);
}

//贬低
inline std::ostream& operator<< (std::ostream& os, AccountID const& x)
{
    os << to_string (x);
    return os;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*缓存accountID的base58表示形式

    此操作发生的频率足以
    证明拥有缓存是正当的。在未来，涟漪应该
    要求客户端接收“二进制”结果，其中
    accountID是十六进制编码的。
**/

class AccountIDCache
{
private:
    std::mutex mutable mutex_;
    std::size_t capacity_;
    hash_map<AccountID,
        std::string> mutable m0_;
    hash_map<AccountID,
        std::string> mutable m1_;

public:
    AccountIDCache(AccountIDCache const&) = delete;
    AccountIDCache& operator= (AccountIDCache const&) = delete;

    explicit
    AccountIDCache (std::size_t capacity);

    /*为accountID返回Ripple:：ToBase58

        线程安全：
            可以安全地同时从任何线程调用

        @注意此函数有意返回
              复制以确保正确。
    **/

    std::string
    toBase58 (AccountID const&) const;
};

} //涟漪

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace std {

//贬低
//vvalco使用Beast：：uHash或硬化容器
template <>
struct hash <ripple::AccountID> : ripple::AccountID::hasher
{
    explicit hash() = default;
};

} //性病

#endif
