
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

#include <ripple/basics/contract.h>
#include <ripple/crypto/impl/openssl.h>
#include <openssl/hmac.h>

namespace ripple  {
namespace openssl {

bignum::bignum()
{
    ptr = BN_new();
    if (ptr == nullptr)
        Throw<std::runtime_error> ("BN_new() failed");
}

void bignum::assign (uint8_t const* data, size_t size)
{
//这将重用和分配ptr
    BIGNUM* bn = BN_bin2bn (data, size, ptr);
    if (bn == nullptr)
        Throw<std::runtime_error> ("BN_bin2bn() failed");
}

void bignum::assign_new (uint8_t const* data, size_t size)
{
//不得分配ptr

    ptr = BN_bin2bn (data, size, nullptr);
    if (ptr == nullptr)
        Throw<std::runtime_error> ("BN_bin2bn() failed");
}

bn_ctx::bn_ctx()
{
    ptr = BN_CTX_new();
    if (ptr == nullptr)
        Throw<std::runtime_error> ("BN_CTX_new() failed");
}

bignum get_order (EC_GROUP const* group, bn_ctx& ctx)
{
    bignum result;
    if (! EC_GROUP_get_order (group, result.get(), ctx.get()))
        Throw<std::runtime_error> ("EC_GROUP_get_order() failed");

    return result;
}

ec_point::ec_point (EC_GROUP const* group)
{
    ptr = EC_POINT_new (group);
    if (ptr == nullptr)
        Throw<std::runtime_error> ("EC_POINT_new() failed");
}

void add_to (EC_GROUP const* group,
             ec_point const& a,
             ec_point& b,
             bn_ctx& ctx)
{
    if (!EC_POINT_add (group, b.get(), a.get(), b.get(), ctx.get()))
        Throw<std::runtime_error> ("EC_POINT_add() failed");
}

ec_point multiply (EC_GROUP const* group,
                   bignum const& n,
                   bn_ctx& ctx)
{
    ec_point result (group);
    if (! EC_POINT_mul (group, result.get(), n.get(), nullptr, nullptr, ctx.get()))
        Throw<std::runtime_error> ("EC_POINT_mul() failed");

    return result;
}

ec_point bn2point (EC_GROUP const* group, BIGNUM const* number)
{
    EC_POINT* result = EC_POINT_bn2point (group, number, nullptr, nullptr);
    if (result == nullptr)
        Throw<std::runtime_error> ("EC_POINT_bn2point() failed");

    return ec_point::acquire (result);
}

static ec_key ec_key_new_secp256k1_compressed()
{
    EC_KEY* key = EC_KEY_new_by_curve_name (NID_secp256k1);

    if (key == nullptr)  Throw<std::runtime_error> ("EC_KEY_new_by_curve_name() failed");

    EC_KEY_set_conv_form (key, POINT_CONVERSION_COMPRESSED);

    return ec_key((ec_key::pointer_t) key);
}

void serialize_ec_point (ec_point const& point, std::uint8_t* ptr)
{
    ec_key key = ec_key_new_secp256k1_compressed();
    if (EC_KEY_set_public_key((EC_KEY*) key.get(), point.get()) <= 0)
        Throw<std::runtime_error> ("EC_KEY_set_public_key() failed");

    int const size = i2o_ECPublicKey ((EC_KEY*) key.get(), &ptr);

    assert (size <= 33);
    (void) size;
}

} //OpenSSL
} //涟漪

#include <stdio.h>
#ifdef _MSC_VER
FILE _iob[] = {*stdin, *stdout, *stderr};
extern "C" FILE * __cdecl __iob_func(void)
{
    return _iob;
}
#endif
