
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

#ifndef RIPPLE_PROTOCOL_DIGEST_H_INCLUDED
#define RIPPLE_PROTOCOL_DIGEST_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/beast/crypto/ripemd.h>
#include <ripple/beast/crypto/sha2.h>
#include <ripple/beast/hash/endian.h>
#include <algorithm>
#include <array>

namespace ripple {

/*Ripple协议中使用的消息摘要函数

    建模以满足“hasher”在
    `hash_append`接口，当前在建议中：

    N3980“类型不知道”
    http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3980.html
**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*ripemd-160摘要

    @注意，这使用OpenSSL实现
**/

struct openssl_ripemd160_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 20>;

    openssl_ripemd160_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[96];
};

/*沙阿512文摘

    @注意，这使用OpenSSL实现
**/

struct openssl_sha512_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 64>;

    openssl_sha512_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[216];
};

/*沙阿256文摘

    @注意，这使用OpenSSL实现
**/

struct openssl_sha256_hasher
{
public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 32>;

    openssl_sha256_hasher();

    void
    operator()(void const* data,
        std::size_t size) noexcept;

    explicit
    operator result_type() noexcept;

private:
    char ctx_[112];
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//用于选择正确摘要实现的别名

#if RIPPLE_USE_OPENSSL
using ripemd160_hasher = openssl_ripemd160_hasher;
using sha256_hasher = openssl_sha256_hasher;
using sha512_hasher = openssl_sha512_hasher;
#else
using ripemd160_hasher = beast::ripemd160_hasher;
using sha256_hasher = beast::sha256_hasher;
using sha512_hasher = beast::sha512_hasher;
#endif

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*返回消息的sha256哈希的ripemd-160摘要。

    此操作用于计算160位标识符
    代表消息中的Ripple帐户。通常情况下
    消息是帐户的公钥-而不是
    存储在帐户根目录中。

    不管密码如何，使用相同的计算
    公钥所隐含的方案。例如，公钥
    可以是ED25519公钥或secp256k1公钥。支持
    对于新的加密系统，可以使用相同的
    计算帐户标识符的公式。

    满足哈希的要求（在hash_append中）
**/

struct ripesha_hasher
{
private:
    sha256_hasher h_;

public:
    static beast::endian const endian =
        beast::endian::native;

    using result_type =
        std::array<std::uint8_t, 20>;

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        h_(data, size);
    }

    explicit
    operator result_type() noexcept
    {
        auto const d0 =
            sha256_hasher::result_type(h_);
        ripemd160_hasher rh;
        rh(d0.data(), d0.size());
        return ripemd160_hasher::result_type(rh);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace detail {

/*返回消息的sha512半摘要。

    sha512的一半是
    SHA-512消息摘要。
**/

template <bool Secure>
struct basic_sha512_half_hasher
{
private:
    sha512_hasher h_;

public:
    static beast::endian const endian =
        beast::endian::big;

    using result_type = uint256;

    ~basic_sha512_half_hasher()
    {
        erase(std::integral_constant<
            bool, Secure>{});
    }

    void
    operator()(void const* data,
        std::size_t size) noexcept
    {
        h_(data, size);
    }

    explicit
    operator result_type() noexcept
    {
        auto const digest =
            sha512_hasher::result_type(h_);
        result_type result;
        std::copy(digest.begin(),
            digest.begin() + 32, result.begin());
        return result;
    }

private:
    inline
    void
    erase (std::false_type)
    {
    }

    inline
    void
    erase (std::true_type)
    {
        beast::secure_erase(&h_, sizeof(h_));
    }
};

} //细节

using sha512_half_hasher =
    detail::basic_sha512_half_hasher<false>;

//安全版本
using sha512_half_hasher_s =
    detail::basic_sha512_half_hasher<true>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

#ifdef _MSC_VER
//从MAIN呼叫修复VS2015之前的Magic Statics
inline
void
sha512_deprecatedMSVCWorkaround()
{
    beast::sha512_hasher h;
    auto const digest = static_cast<
        beast::sha512_hasher::result_type>(h);
}
#endif

/*返回一系列对象的sha512半。*/
template <class... Args>
sha512_half_hasher::result_type
sha512Half (Args const&... args)
{
    sha512_half_hasher h;
    using beast::hash_append;
    hash_append(h, args...);
    return static_cast<typename
        sha512_half_hasher::result_type>(h);
}

/*返回一系列对象的sha512半。

    Postconditions：
        临时存储器存储
        输入信息将被清除。
**/

template <class... Args>
sha512_half_hasher_s::result_type
sha512Half_s (Args const&... args)
{
    sha512_half_hasher_s h;
    using beast::hash_append;
    hash_append(h, args...);
    return static_cast<typename
        sha512_half_hasher_s::result_type>(h);
}

} //涟漪

#endif
