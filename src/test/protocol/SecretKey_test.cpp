
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

#include <ripple/crypto/csprng.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/rngfill.h>
#include <algorithm>
#include <string>
#include <vector>

#include <ripple/protocol/impl/secp256k1.h>

namespace ripple {

class SecretKey_test : public beast::unit_test::suite
{
public:
    using blob = std::vector<std::uint8_t>;

    template <class FwdIter, class Container>
    static
    void
    hex_to_binary (FwdIter first, FwdIter last, Container& out)
    {
        struct Table
        {
            int val[256];
            Table ()
            {
                std::fill (val, val+256, 0);
                for (int i = 0; i < 10; ++i)
                    val ['0'+i] = i;
                for (int i = 0; i < 6; ++i)
                {
                    val ['A'+i] = 10 + i;
                    val ['a'+i] = 10 + i;
                }
            }
            int operator[] (int i)
            {
               return val[i];
            }
        };

        static Table lut;
        out.reserve (std::distance (first, last) / 2);
        while (first != last)
        {
            auto const hi (lut[(*first++)]);
            auto const lo (lut[(*first++)]);
            out.push_back ((hi*16)+lo);
        }
    }

    static
    uint256
    hex_to_digest(std::string const& s)
    {
        blob b;
        hex_to_binary (s.begin (), s.end (), b);
        return uint256::fromVoid(b.data());
    }

    static
    PublicKey
    hex_to_pk(std::string const& s)
    {
        blob b;
        hex_to_binary (s.begin (), s.end (), b);
        return PublicKey{Slice{b.data(), b.size()}};
    }

    static
    SecretKey
    hex_to_sk(std::string const& s)
    {
        blob b;
        hex_to_binary (s.begin (), s.end (), b);
        return SecretKey{Slice{b.data(), b.size()}};
    }

    static
    Buffer
    hex_to_sig(std::string const& s)
    {
        blob b;
        hex_to_binary (s.begin (), s.end (), b);
        return Buffer{Slice{b.data(), b.size()}};
    }

//vfalc我们可以删除这个注释掉的代码
//稍后，当我们对向量有信心时。

    /*
    缓冲区
    makenoncanonical（缓冲常数和信号）
    {
        secp256k1_ecdsa_签名sigin；
        Beast_Expect（secp256k1_ecdsa_signature_parse_der（
            secp256kContext（），
            和Sigin，
            reinterpret_cast<unsigned char const*>（）
                Sig.DATA（））
            sig.size（））==1）；
        secp256k1_ecdsa_签名信号；
        Beast_Expect（secp256k1_ecdsa_signature_unnormalize（
            secp256kContext（），
            和SigOUT，
            和SIGIN＝1）；
        无符号字符buf[72]；
        尺寸_t len=尺寸（buf）；
        Beast_Expect（secp256k1_ecdsa_signature_serialize_der（
            secp256kContext（），
            缓冲器，
            和伦，
            和SigOUT＝1）；
        返回缓冲区buf，len
    }

    无效
    生成规范性测试向量（）
    {
        UTIN 256摘要；
        野兽：：rngfill（
            摘要，数据（）
            摘要：
            隐式PRNG.（））；
        日志<“digest”<<strhex（digest.data（），digest.size（））<<std:：endl；

        auto const sk=randomscretkey（）；
        auto const pk=派生发布键（keytype:：secp256k1，sk）；
        日志<“public”<<pk<<std:：endl；
        日志<“secret”<<sk.to_string（）<<std:：endl；

        auto sig=符号摘要（pk，sk，digest）；
        日志<“canonical sig”<<strhex（sig）<<std:：endl；

        auto const non=makenoncanonical（sig）；
        日志<“non canon sig”<<strhex（non）<<std:：endl；

        {
            auto const规范性=ecdsacanonicality（sig）；
            野兽期待（圣典）；
            Beast_Expect（*Canonicality==ecdsacanonicality:：fullyCanonicall）；
        }

        {
            自动常数规范性=ECDSA规范性（非）；
            野兽期待（圣典）；
            野兽期待（*canonicality！=ecdsacanonicality:：fullycanonicall）；
        }

        Beast_Expect（verifydigest（pk，digest，sig，false））；
        Beast_Expect（verifydigest（pk，digest，sig，true））；
        Beast_Expect（VerifyDigest（pk，Digest，non，false））；
        期待！verifydigest（pk，digest，non，true））；
    }
    **/


//确保验证的正确性
//关于规范性变量的矩阵。
    void
    testCanonicality()
    {
        testcase ("secp256k1 canonicality");

#if 0
        makeCanonicalityTestVectors();
#else
        auto const digest = hex_to_digest("34C19028C80D21F3F48C9354895F8D5BF0D5EE7FF457647CF655F5530A3022A7");
        auto const pk = hex_to_pk("025096EB12D3E924234E7162369C11D8BF877EDA238778E7A31FF0AAC5D0DBCF37");
        auto const sk = hex_to_sk("AA921417E7E5C299DA4EEC16D1CAA92F19B19F2A68511F68EC73BBB2F5236F3D");
        auto const sig = hex_to_sig("3045022100B49D07F0E934BA468C0EFC78117791408D1FB8B63A6492AD395AC2F360F246600220508739DB0A2EF81676E39F459C8BBB07A09C3E9F9BEB696294D524D479D62740");
        auto const non = hex_to_sig("3046022100B49D07F0E934BA468C0EFC78117791408D1FB8B63A6492AD395AC2F360F24660022100AF78C624F5D107E9891C60BA637444F71A129E47135D36D92AFD39B856601A01");

        {
            auto const canonicality = ecdsaCanonicality(sig);
            BEAST_EXPECT(canonicality);
            BEAST_EXPECT(*canonicality == ECDSACanonicality::fullyCanonical);
        }

        {
            auto const canonicality = ecdsaCanonicality(non);
            BEAST_EXPECT(canonicality);
            BEAST_EXPECT(*canonicality != ECDSACanonicality::fullyCanonical);
        }

        BEAST_EXPECT(verifyDigest(pk, digest, sig, false));
        BEAST_EXPECT(verifyDigest(pk, digest, sig, true));
        BEAST_EXPECT(verifyDigest(pk, digest, non, false));
        BEAST_EXPECT(! verifyDigest(pk, digest, non, true));
#endif
    }

    void testDigestSigning()
    {
        testcase ("secp256k1 digest");

        for (std::size_t i = 0; i < 32; i++)
        {
            auto const keypair = randomKeyPair (KeyType::secp256k1);

            BEAST_EXPECT(keypair.first == derivePublicKey (KeyType::secp256k1, keypair.second));
            BEAST_EXPECT(*publicKeyType (keypair.first) == KeyType::secp256k1);

            for (std::size_t j = 0; j < 32; j++)
            {
                uint256 digest;
                beast::rngfill (
                    digest.data(),
                    digest.size(),
                    crypto_prng());

                auto sig = signDigest (
                    keypair.first, keypair.second, digest);

                BEAST_EXPECT(sig.size() != 0);
                BEAST_EXPECT(verifyDigest (keypair.first,
                    digest, sig, true));

//错误文摘：
                BEAST_EXPECT(!verifyDigest (keypair.first,
                    ~digest, sig, true));

//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[j % sig.size()]++;

//签名错误：
                BEAST_EXPECT(!verifyDigest (keypair.first,
                    digest, sig, true));

//摘要和签名错误：
                BEAST_EXPECT(!verifyDigest (keypair.first,
                    ~digest, sig, true));
            }
        }
    }

    void testSigning (KeyType type)
    {
        for (std::size_t i = 0; i < 32; i++)
        {
            auto const keypair = randomKeyPair (type);

            BEAST_EXPECT(keypair.first == derivePublicKey (type, keypair.second));
            BEAST_EXPECT(*publicKeyType (keypair.first) == type);

            for (std::size_t j = 0; j < 32; j++)
            {
                std::vector<std::uint8_t> data (64 + (8 * i) + j);
                beast::rngfill (
                    data.data(),
                    data.size(),
                    crypto_prng());

                auto sig = sign (
                    keypair.first, keypair.second,
                    makeSlice (data));

                BEAST_EXPECT(sig.size() != 0);
                BEAST_EXPECT(verify(keypair.first,
                    makeSlice(data), sig, true));

//构造错误的数据：
                auto badData = data;

//交换缓冲区中最小和最大的元素
                std::iter_swap (
                    std::min_element (badData.begin(), badData.end()),
                    std::max_element (badData.begin(), badData.end()));

//错误数据：应失败
                BEAST_EXPECT(!verify (keypair.first,
                    makeSlice(badData), sig, true));

//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[j % sig.size()]++;

//签名错误：应失败
                BEAST_EXPECT(!verify (keypair.first,
                    makeSlice(data), sig, true));

//错误的数据和签名：应失败
                BEAST_EXPECT(!verify (keypair.first,
                    makeSlice(badData), sig, true));
            }
        }
    }

    void testBase58 ()
    {
        testcase ("Base58");

//确保解析某些已知的密钥有效
        {
            auto const sk1 = generateSecretKey (
                KeyType::secp256k1,
                generateSeed ("masterpassphrase"));

            auto const sk2 = parseBase58<SecretKey> (
                TokenType::NodePrivate,
                "pnen77YEeUd4fFKG7iycBWcwKpTaeFRkW2WFostaATy1DSupwXe");
            BEAST_EXPECT(sk2);

            BEAST_EXPECT(sk1 == *sk2);
        }

        {
            auto const sk1 = generateSecretKey (
                KeyType::ed25519,
                generateSeed ("masterpassphrase"));

            auto const sk2 = parseBase58<SecretKey> (
                TokenType::NodePrivate,
                "paKv46LztLqK3GaKz1rG2nQGN6M4JLyRtxFBYFTw4wAVHtGys36");
            BEAST_EXPECT(sk2);

            BEAST_EXPECT(sk1 == *sk2);
        }

//尝试转换短、长和格式错误的数据
        BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, ""));
        BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, " "));
        BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, "!35gty9mhju8nfjl"));

        auto const good = toBase58 (
            TokenType::NodePrivate,
            randomSecretKey());

//短（非空）字符串
        {
            auto s = good;

//随机删除字符串中的所有字符：
            std::hash<std::string> r;

            while (!s.empty())
            {
                s.erase (r(s) % s.size(), 1);
                BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, s));
            }
        }

//长弦
        for (std::size_t i = 1; i != 16; i++)
        {
            auto s = good;
            s.resize (s.size() + i, s[i % s.size()]);
            BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, s));
        }

//具有无效base58字符的字符串
        for (auto c : std::string ("0IOl"))
        {
            for (std::size_t i = 0; i != good.size(); ++i)
            {
                auto s = good;
                s[i % s.size()] = c;
                BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, s));
            }
        }

//前缀不正确的字符串
        {
            auto s = good;

            for (auto c : std::string("ansrJqtv7"))
            {
                s[0] = c;
                BEAST_EXPECT(!parseBase58<SecretKey> (TokenType::NodePrivate, s));
            }
        }

//尝试一些随机密钥
        std::array <SecretKey, 32> keys;

        for (std::size_t i = 0; i != keys.size(); ++i)
            keys[i] = randomSecretKey();

        for (std::size_t i = 0; i != keys.size(); ++i)
        {
            auto const si = toBase58 (
                TokenType::NodePrivate,
                keys[i]);
            BEAST_EXPECT(!si.empty());

            auto const ski = parseBase58<SecretKey> (
                TokenType::NodePrivate, si);
            BEAST_EXPECT(ski && keys[i] == *ski);

            for (std::size_t j = i; j != keys.size(); ++j)
            {
                BEAST_EXPECT((keys[i] == keys[j]) == (i == j));

                auto const sj = toBase58 (
                    TokenType::NodePrivate,
                    keys[j]);

                BEAST_EXPECT((si == sj) == (i == j));

                auto const skj = parseBase58<SecretKey> (
                    TokenType::NodePrivate, sj);
                BEAST_EXPECT(skj && keys[j] == *skj);

                BEAST_EXPECT((*ski == *skj) == (i == j));
            }
        }
    }

    void testMiscOperations ()
    {
        testcase ("Miscellaneous operations");

        auto const sk1 = generateSecretKey (
            KeyType::secp256k1,
            generateSeed ("masterpassphrase"));

        SecretKey sk2 (sk1);
        BEAST_EXPECT(sk1 == sk2);

        SecretKey sk3;
        BEAST_EXPECT(sk3 != sk2);
        sk3 = sk2;
        BEAST_EXPECT(sk3 == sk2);
    }

    void run() override
    {
        testBase58();
        testDigestSigning();
        testMiscOperations();
        testCanonicality();

        testcase ("secp256k1");
        testSigning(KeyType::secp256k1);

        testcase ("ed25519");
        testSigning(KeyType::ed25519);
    }
};

BEAST_DEFINE_TESTSUITE(SecretKey,protocol,ripple);

} //涟漪
