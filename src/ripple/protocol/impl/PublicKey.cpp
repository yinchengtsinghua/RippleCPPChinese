
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

#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/impl/secp256k1.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/strHex.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <ed25519-donna/ed25519.h>
#include <type_traits>

namespace ripple {

std::ostream&
operator<<(std::ostream& os, PublicKey const& pk)
{
    os << strHex(pk);
    return os;
}

template<>
boost::optional<PublicKey>
parseBase58 (TokenType type, std::string const& s)
{
    auto const result = decodeBase58Token(s, type);
    auto const pks = makeSlice(result);
    if (!publicKeyType(pks))
        return boost::none;
    return PublicKey(pks);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//解析带前缀的长度数字
//格式：0x02<length byte><number>
static
boost::optional<Slice>
sigPart (Slice& buf)
{
    if (buf.size() < 3 || buf[0] != 0x02)
        return boost::none;
    auto const len = buf[1];
    buf += 2;
    if (len > buf.size() || len < 1 || len > 33)
        return boost::none;
//不能是否定的
    if ((buf[0] & 0x80) != 0)
        return boost::none;
    if (buf[0] == 0)
    {
//不能为零
        if (len == 1)
            return boost::none;
//不能填补
        if ((buf[1] & 0x80) == 0)
            return boost::none;
    }
    boost::optional<Slice> number = Slice(buf.data(), len);
    buf += len;
    return number;
}

static
std::string
sliceToHex (Slice const& slice)
{
    std::string s;
    if (slice[0] & 0x80)
    {
        s.reserve(2 * (slice.size() + 2));
        s = "0x00";
    }
    else
    {
        s.reserve(2 * (slice.size() + 1));
        s = "0x";
    }
    for(int i = 0; i < slice.size(); ++i)
    {
        s += "0123456789ABCDEF"[((slice[i]&0xf0)>>4)];
        s += "0123456789ABCDEF"[((slice[i]&0x0f)>>0)];
    }
    return s;
}

/*确定签名是否规范。
    规范签名对于防止签名变形很重要
    攻击。
    @param vsig签名数据
    @param siglen签名的长度
    @param strict_param是否强制严格规范语义

    @注：详情请参见：
    https://ripple.com/wiki/transaction\u可扩展性
    https://bitcointalk.org/index.php？主题=8392.MSG127623 MSG127623
    https://github.com/sipa/bitcoin/commit/58bc86e37fdaaec270bccb3df6c20fbd2a6591c
**/

boost::optional<ECDSACanonicality>
ecdsaCanonicality (Slice const& sig)
{
    using uint264 = boost::multiprecision::number<
        boost::multiprecision::cpp_int_backend<
            264, 264, boost::multiprecision::signed_magnitude,
            boost::multiprecision::unchecked, void>>;

    static uint264 const G(
        "0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141");

//签名的格式应为：
//<30><len>[<02><len r><r>][<02><len s><s>]
    if ((sig.size() < 8) || (sig.size() > 72))
        return boost::none;
    if ((sig[0] != 0x30) || (sig[1] != (sig.size() - 2)))
        return boost::none;
    Slice p = sig + 2;
    auto r = sigPart(p);
    auto s = sigPart(p);
    if (! r || ! s || ! p.empty())
        return boost::none;

    uint264 R(sliceToHex(*r));
    if (R >= G)
        return boost::none;

    uint264 S(sliceToHex(*s));
    if (S >= G)
        return boost::none;

//（r，s）和（r，g-s）是规范的，
//但当s<=g-s时完全规范
    auto const Sp = G - S;
    if (S > Sp)
        return ECDSACanonicality::canonical;
    return ECDSACanonicality::fullyCanonical;
}

static
bool
ed25519Canonical (Slice const& sig)
{
    if (sig.size() != 64)
        return false;
//大尾数阶，ED25519子群阶
    std::uint8_t const Order[] =
    {
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x14, 0xDE, 0xF9, 0xDE, 0xA2, 0xF7, 0x9C, 0xD6,
        0x58, 0x12, 0x63, 0x1A, 0x5C, 0xF5, 0xD3, 0xED,
    };
//取签名的后半部分
//字节倒转为big-endian。
    auto const le = sig.data() + 32;
    std::uint8_t S[32];
    std::reverse_copy(le, le + 32, S);
//必须小于订单
    return std::lexicographical_compare(
        S, S + 32, Order, Order + 32);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

PublicKey::PublicKey (Slice const& slice)
{
    if(! publicKeyType(slice))
        LogicError("PublicKey::PublicKey invalid type");
    size_ = slice.size();
    std::memcpy(buf_, slice.data(), size_);
}

PublicKey::PublicKey (PublicKey const& other)
    : size_ (other.size_)
{
    std::memcpy(buf_, other.buf_, size_);
};

PublicKey&
PublicKey::operator=(PublicKey const& other)
{
    size_ = other.size_;
    std::memcpy(buf_, other.buf_, size_);
    return *this;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

boost::optional<KeyType>
publicKeyType (Slice const& slice)
{
    if (slice.size() == 33)
    {
        if (slice[0] == 0xED)
            return KeyType::ed25519;

        if (slice[0] == 0x02 || slice[0] == 0x03)
            return KeyType::secp256k1;
    }

    return boost::none;
}

bool
verifyDigest (PublicKey const& publicKey,
    uint256 const& digest,
    Slice const& sig,
    bool mustBeFullyCanonical)
{
    if (publicKeyType(publicKey) != KeyType::secp256k1)
        LogicError("sign: secp256k1 required for digest signing");
    auto const canonicality = ecdsaCanonicality(sig);
    if (! canonicality)
        return false;
    if (mustBeFullyCanonical &&
        (*canonicality != ECDSACanonicality::fullyCanonical))
        return false;

    secp256k1_pubkey pubkey_imp;
    if(secp256k1_ec_pubkey_parse(
            secp256k1Context(),
            &pubkey_imp,
            reinterpret_cast<unsigned char const*>(
                publicKey.data()),
            publicKey.size()) != 1)
        return false;

    secp256k1_ecdsa_signature sig_imp;
    if(secp256k1_ecdsa_signature_parse_der(
            secp256k1Context(),
            &sig_imp,
            reinterpret_cast<unsigned char const*>(
                sig.data()),
            sig.size()) != 1)
        return false;
    if (*canonicality != ECDSACanonicality::fullyCanonical)
    {
        secp256k1_ecdsa_signature sig_norm;
        if(secp256k1_ecdsa_signature_normalize(
                secp256k1Context(),
                &sig_norm,
                &sig_imp) != 1)
            return false;
        return secp256k1_ecdsa_verify(
            secp256k1Context(),
            &sig_norm,
            reinterpret_cast<unsigned char const*>(
                digest.data()),
            &pubkey_imp) == 1;
    }
    return secp256k1_ecdsa_verify(
        secp256k1Context(),
        &sig_imp,
        reinterpret_cast<unsigned char const*>(
            digest.data()),
        &pubkey_imp) == 1;
}

bool
verify (PublicKey const& publicKey,
    Slice const& m,
    Slice const& sig,
    bool mustBeFullyCanonical)
{
    if (auto const type = publicKeyType(publicKey))
    {
        if (*type == KeyType::secp256k1)
        {
            return verifyDigest (publicKey,
                sha512Half(m), sig, mustBeFullyCanonical);
        }
        else if (*type == KeyType::ed25519)
        {
            if (! ed25519Canonical(sig))
                return false;

//我们在内部为ED25519键加上0xED前缀
//区分它们与secp256k1键的字节
//所以在验证签名时，我们需要
//首先去掉那个前缀。
            return ed25519_sign_open(
                m.data(), m.size(), publicKey.data() + 1,
                    sig.data()) == 0;
        }
    }
    return false;
}

NodeID
calcNodeID (PublicKey const& pk)
{
    ripesha_hasher h;
    h(pk.data(), pk.size());
    auto const digest = static_cast<
        ripesha_hasher::result_type>(h);
    static_assert(NodeID::bytes ==
        sizeof(ripesha_hasher::result_type), "");
    NodeID result;
    std::memcpy(result.data(),
        digest.data(), digest.size());
    return result;
}

} //涟漪

