
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

#ifndef RIPPLE_OPENSSL_H
#define RIPPLE_OPENSSL_H

#include <ripple/basics/base_uint.h>
#include <ripple/crypto/impl/ec_key.h>
#include <openssl/bn.h>
#include <openssl/ec.h>

namespace ripple  {
namespace openssl {

class bignum
{
private:
    BIGNUM* ptr;

//不可复制
    bignum           (bignum const&) = delete;
    bignum& operator=(bignum const&) = delete;

    void assign_new (uint8_t const* data, size_t size);

public:
    bignum();

    ~bignum()
    {
        if ( ptr != nullptr)
        {
            BN_free (ptr);
        }
    }

    bignum (uint8_t const* data, size_t size)
    {
        assign_new (data, size);
    }

    template <class T>
    explicit bignum (T const& thing)
    {
        assign_new (thing.data(), thing.size());
    }

    bignum(bignum&& that) noexcept : ptr( that.ptr )
    {
        that.ptr = nullptr;
    }

    bignum& operator= (bignum&& that) noexcept
    {
        using std::swap;

        swap( ptr, that.ptr );

        return *this;
    }

    BIGNUM      * get()        { return ptr; }
    BIGNUM const* get() const  { return ptr; }

    bool is_zero() const  { return BN_is_zero (ptr); }

    void clear()  { BN_clear (ptr); }

    void assign (uint8_t const* data, size_t size);
};

inline bool operator< (bignum const& a, bignum const& b)
{
    return BN_cmp (a.get(), b.get()) < 0;
}

inline bool operator>= (bignum const& a, bignum const& b)
{
    return !(a < b);
}

inline uint256 uint256_from_bignum_clear (bignum& number)
{
    uint256 result;
    result.zero();

    BN_bn2bin (number.get(), result.end() - BN_num_bytes (number.get()));

    number.clear();

    return result;
}

class bn_ctx
{
private:
    BN_CTX* ptr;

//不可复制
    bn_ctx           (bn_ctx const&);
    bn_ctx& operator=(bn_ctx const&);

public:
    bn_ctx();

    ~bn_ctx()
    {
        BN_CTX_free (ptr);
    }

    BN_CTX      * get()        { return ptr; }
    BN_CTX const* get() const  { return ptr; }
};

bignum get_order (EC_GROUP const* group, bn_ctx& ctx);

inline void add_to (bignum const& a,
                    bignum& b,
                    bignum const& modulus,
                    bn_ctx& ctx)
{
    BN_mod_add (b.get(), a.get(), b.get(), modulus.get(), ctx.get());
}

class ec_point
{
public:
    using pointer_t = EC_POINT*;

private:
    pointer_t ptr;

    ec_point (pointer_t raw) : ptr(raw)
    {
    }

public:
    static ec_point acquire (pointer_t raw)
    {
        return ec_point (raw);
    }

    ec_point (EC_GROUP const* group);

    ~ec_point()  { EC_POINT_free (ptr); }

    ec_point           (ec_point const&) = delete;
    ec_point& operator=(ec_point const&) = delete;

    ec_point(ec_point&& that) noexcept
    {
        ptr      = that.ptr;
        that.ptr = nullptr;
    }

    EC_POINT      * get()        { return ptr; }
    EC_POINT const* get() const  { return ptr; }
};

void add_to (EC_GROUP const* group,
             ec_point const& a,
             ec_point& b,
             bn_ctx& ctx);

ec_point multiply (EC_GROUP const* group,
                   bignum const& n,
                   bn_ctx& ctx);

ec_point bn2point (EC_GROUP const* group, BIGNUM const* number);

//输出缓冲区必须容纳33个字节
void serialize_ec_point (ec_point const& point, std::uint8_t* ptr);

} //OpenSSL
} //涟漪

#endif
