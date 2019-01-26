
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

#ifndef RIPPLE_PROTOCOL_TOKENS_H_INCLUDED
#define RIPPLE_PROTOCOL_TOKENS_H_INCLUDED

#include <boost/optional.hpp>
#include <cstdint>
#include <string>

namespace ripple {

enum class TokenType : std::uint8_t
{
None             = 1,       //未使用的
    NodePublic       = 28,
    NodePrivate      = 32,
    AccountID        = 0,
    AccountPublic    = 35,
    AccountSecret    = 34,
FamilyGenerator  = 41,      //未使用的
    FamilySeed       = 33
};

template <class T>
boost::optional<T>
parseBase58 (std::string const& s);

template<class T>
boost::optional<T>
parseBase58 (TokenType type, std::string const& s);

template <class T>
boost::optional<T>
parseHex (std::string const& s);

template <class T>
boost::optional<T>
parseHexOrBase58 (std::string const& s);

//用于转换Ripple令牌的工具
//从人类可读的字符串

/*Base-58编码纹波令牌

    Ripple标记有一个单字节前缀，表示
    标记的类型，后跟
    令牌，最后是4字节校验和。

    令牌包括以下内容：

        钱包种子
        帐户公钥
        账号标识

    @param type表示标记类型的单个字节
    @param token指向存储非的指针
                 小于2*（大小+6）字节
    @param size令牌缓冲区的大小（以字节为单位）
**/

std::string
base58EncodeToken (TokenType type, void const* token, std::size_t size);

/*Base-58编码比特币代币
 *
 *这里提供对称，但不应需要
 *测试除外。
 *
 *@有关格式说明，请参阅base58encodetoken。
 *
 **/

std::string
base58EncodeTokenBitcoin (TokenType type, void const* token, std::size_t size);

/*解码base58令牌

    类型和校验和必须匹配或
    返回空字符串。
**/

std::string
decodeBase58Token(std::string const& s, TokenType type);

/*使用比特币字母解码base58令牌

    类型和校验和必须匹配或
    返回空字符串。

    用于检测用户错误。明确地，
    当使用错误的
    以58为基数的字母表，以便更好地显示错误信息
    可以退回。
**/

std::string
decodeBase58TokenBitcoin(std::string const& s, TokenType type);

} //涟漪

#endif
