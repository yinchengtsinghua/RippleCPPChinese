
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

#include <ripple/basics/contract.h>
#include <ripple/beast/crypto/secure_erase.h>
#include <ripple/crypto/GenerateDeterministicKey.h>
#include <ripple/crypto/impl/ec_key.h>
#include <ripple/crypto/impl/openssl.h>
#include <ripple/protocol/digest.h>
#include <array>
#include <string>
#include <openssl/pem.h>
#include <openssl/sha.h>

namespace ripple {

namespace openssl {

struct secp256k1_data
{
    EC_GROUP const* group;
    bignum order;

    secp256k1_data ()
    {
        group = EC_GROUP_new_by_curve_name (NID_secp256k1);

        if (!group)
            LogicError ("The OpenSSL library on this system lacks elliptic curve support.");

        bn_ctx ctx;
        order = get_order (group, ctx);
    }
};

static secp256k1_data const& secp256k1curve()
{
    static secp256k1_data const curve {};
    return curve;
}

}  //命名空间OpenSSL

using namespace openssl;

static Blob serialize_ec_point (ec_point const& point)
{
    Blob result (33);

    serialize_ec_point (point, &result[0]);

    return result;
}

template <class FwdIt>
void
copy_uint32 (FwdIt out, std::uint32_t v)
{
    *out++ =  v >> 24;
    *out++ = (v >> 16) & 0xff;
    *out++ = (v >>  8) & 0xff;
    *out   =  v        & 0xff;
}

//用于添加对确定性EC键支持的函数

//>种子
//<--私有根生成器+公共根生成器
static bignum generateRootDeterministicKey (uint128 const& seed)
{
//查找小于曲线顺序的非零私钥
    bignum privKey;
    std::uint32_t seq = 0;

    do
    {
//buf:0种子16序列20
//<----------------------------------><------>
        std::array<std::uint8_t, 20> buf;
        std::copy(seed.begin(), seed.end(), buf.begin());
        copy_uint32 (buf.begin() + 16, seq++);
        auto root = sha512Half(buf);
        beast::secure_erase(buf.data(), buf.size());
        privKey.assign (root.data(), root.size());
        beast::secure_erase(root.data(), root.size());
    }
    while (privKey.is_zero() || privKey >= secp256k1curve().order);
    beast::secure_erase(&seq, sizeof(seq));
    return privKey;
}

//>种子
//<--私有根生成器+公共根生成器
Blob generateRootDeterministicPublicKey (uint128 const& seed)
{
    bn_ctx ctx;

    bignum privKey = generateRootDeterministicKey (seed);

//计算相应的公钥点
    ec_point pubKey = multiply (secp256k1curve().group, privKey, ctx);

privKey.clear();  //安全擦除

    return serialize_ec_point (pubKey);
}

uint256 generateRootDeterministicPrivateKey (uint128 const& seed)
{
    bignum key = generateRootDeterministicKey (seed);

    return uint256_from_bignum_clear (key);
}

//取Ripple地址。
//->根公共生成器（消耗）
//<--EC格式的根公共生成器
static ec_point generateRootPubKey (bignum&& pubGenerator)
{
    ec_point pubPoint = bn2point (secp256k1curve().group, pubGenerator.get());

    return pubPoint;
}

//>公共生成器
static bignum makeHash (Blob const& pubGen, int seq, bignum const& order)
{
    int subSeq = 0;

    bignum result;

    assert(pubGen.size() == 33);
    do
    {
//buf:0 pubgen 33 seq 37 subseq 41
//<----------------------------------><----------->
        std::array<std::uint8_t, 41> buf;
        std::copy (pubGen.begin(), pubGen.end(), buf.begin());
        copy_uint32 (buf.begin() + 33, seq);
        copy_uint32 (buf.begin() + 37, subSeq++);
        auto root = sha512Half_s(buf);
        beast::secure_erase(buf.data(), buf.size());
        result.assign (root.data(), root.size());
        beast::secure_erase(root.data(), root.size());
    }
    while (result.is_zero()  ||  result >= order);

    return result;
}

//>公共生成器
Blob generatePublicDeterministicKey (Blob const& pubGen, int seq)
{
//publickey（n）=rootpublickey ec_point_+hash（pubhash seq）*点
    ec_point rootPubKey = generateRootPubKey (bignum (pubGen));

    bn_ctx ctx;

//计算专用附加密钥。
    bignum hash = makeHash (pubGen, seq, secp256k1curve().order);

//计算相应的公钥。
    ec_point newPoint = multiply (secp256k1curve().group, hash, ctx);

//添加主公钥并设置。
    add_to (secp256k1curve().group, rootPubKey, newPoint, ctx);

    return serialize_ec_point (newPoint);
}

//->根私钥
uint256 generatePrivateDeterministicKey (
    Blob const& pubGen, uint128 const& seed, int seq)
{
//privatekey（n）=（rootprivatekey+hash（pubhash seq））%顺序
    bignum rootPrivKey = generateRootDeterministicKey (seed);

    bn_ctx ctx;

//计算专用附加密钥
    bignum privKey = makeHash (pubGen, seq, secp256k1curve().order);

//计算最终私钥
    add_to (rootPrivKey, privKey, secp256k1curve().order, ctx);

rootPrivKey.clear();  //安全擦除

    return uint256_from_bignum_clear (privKey);
}

} //涟漪
