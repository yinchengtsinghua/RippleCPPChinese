
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

#include <ripple/basics/random.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Seed.h>
#include <ripple/beast/unit_test.h>
#include <ripple/beast/utility/rngfill.h>
#include <ripple/beast/xor_shift_engine.h>
#include <algorithm>


namespace ripple {

class Seed_test : public beast::unit_test::suite
{
    static
    bool equal(Seed const& lhs, Seed const& rhs)
    {
        return std::equal (
            lhs.data(), lhs.data() + lhs.size(),
            rhs.data(), rhs.data() + rhs.size());
    }

public:
    void testConstruction ()
    {
        testcase ("construction");

        {
            std::uint8_t src[16];

            for (std::uint8_t i = 0; i < 64; i++)
            {
                beast::rngfill (
                    src,
                    sizeof(src),
                    default_prng());
                Seed const seed ({ src, sizeof(src) });
                BEAST_EXPECT(memcmp (seed.data(), src, sizeof(src)) == 0);
            }
        }

        for (int i = 0; i < 64; i++)
        {
            uint128 src;
            beast::rngfill (
                src.data(),
                src.size(),
                default_prng());
            Seed const seed (src);
            BEAST_EXPECT(memcmp (seed.data(), src.data(), src.size()) == 0);
        }
    }

    std::string testPassphrase(std::string passphrase)
    {
        auto const seed1 = generateSeed (passphrase);
        auto const seed2 = parseBase58<Seed>(toBase58(seed1));

        BEAST_EXPECT(static_cast<bool>(seed2));
        BEAST_EXPECT(equal (seed1, *seed2));
        return toBase58(seed1);
    }

    void testPassphrase()
    {
        testcase ("generation from passphrase");
        BEAST_EXPECT(testPassphrase ("masterpassphrase") ==
            "snoPBrXtMeMyMHUVTgbuqAfg1SUTb");
        BEAST_EXPECT(testPassphrase ("Non-Random Passphrase") ==
            "snMKnVku798EnBwUfxeSD8953sLYA");
        BEAST_EXPECT(testPassphrase ("cookies excitement hand public") ==
            "sspUXGrmjQhq6mgc24jiRuevZiwKT");
    }

    void testBase58()
    {
        testcase ("base58 operations");

//成功：
        BEAST_EXPECT(parseBase58<Seed>("snoPBrXtMeMyMHUVTgbuqAfg1SUTb"));
        BEAST_EXPECT(parseBase58<Seed>("snMKnVku798EnBwUfxeSD8953sLYA"));
        BEAST_EXPECT(parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwKT"));

//失败：
        BEAST_EXPECT(!parseBase58<Seed>(""));
        BEAST_EXPECT(!parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwK"));
        BEAST_EXPECT(!parseBase58<Seed>("sspUXGrmjQhq6mgc24jiRuevZiwKTT"));
        BEAST_EXPECT(!parseBase58<Seed>("sspOXGrmjQhq6mgc24jiRuevZiwKT"));
        BEAST_EXPECT(!parseBase58<Seed>("ssp/XGrmjQhq6mgc24jiRuevZiwKT"));
    }

    void testRandom()
    {
        testcase ("random generation");

        for (int i = 0; i < 32; i++)
        {
            auto const seed1 = randomSeed ();
            auto const seed2 = parseBase58<Seed>(toBase58(seed1));

            BEAST_EXPECT(static_cast<bool>(seed2));
            BEAST_EXPECT(equal (seed1, *seed2));
        }
    }

    void testKeypairGenerationAndSigning ()
    {
std::string const message1 = "http://www.ripple.com；
std::string const message2 = "https://www.ripple.com；

        {
            testcase ("Node keypair generation & signing (secp256k1)");

            auto const secretKey = generateSecretKey (
                KeyType::secp256k1, generateSeed ("masterpassphrase"));
            auto const publicKey = derivePublicKey (
                KeyType::secp256k1, secretKey);

            BEAST_EXPECT(toBase58(TokenType::NodePublic, publicKey) ==
                "n94a1u4jAz288pZLtw6yFWVbi89YamiC6JBXPVUj5zmExe5fTVg9");
            BEAST_EXPECT(toBase58(TokenType::NodePrivate, secretKey) ==
                "pnen77YEeUd4fFKG7iycBWcwKpTaeFRkW2WFostaATy1DSupwXe");
            BEAST_EXPECT(to_string(calcNodeID(publicKey)) ==
                "7E59C17D50F5959C7B158FEC95C8F815BF653DC8");

            auto sig = sign (publicKey, secretKey, makeSlice(message1));
            BEAST_EXPECT(sig.size() != 0);
            BEAST_EXPECT(verify (publicKey, makeSlice(message1), sig));

//更正公钥但错误消息
            BEAST_EXPECT(!verify (publicKey, makeSlice(message2), sig));

//使用不正确的公钥进行验证
            {
                auto const otherPublicKey = derivePublicKey (
                    KeyType::secp256k1,
                    generateSecretKey (
                        KeyType::secp256k1,
                        generateSeed ("otherpassphrase")));

                BEAST_EXPECT(!verify (otherPublicKey, makeSlice(message1), sig));
            }

//公钥正确，签名错误
            {
//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[sig.size() / 2]++;

                BEAST_EXPECT(!verify (publicKey, makeSlice(message1), sig));
            }
        }

        {
            testcase ("Node keypair generation & signing (ed25519)");

            auto const secretKey = generateSecretKey (
                KeyType::ed25519, generateSeed ("masterpassphrase"));
            auto const publicKey = derivePublicKey (
                KeyType::ed25519, secretKey);

            BEAST_EXPECT(toBase58(TokenType::NodePublic, publicKey) ==
                "nHUeeJCSY2dM71oxM8Cgjouf5ekTuev2mwDpc374aLMxzDLXNmjf");
            BEAST_EXPECT(toBase58(TokenType::NodePrivate, secretKey) ==
                "paKv46LztLqK3GaKz1rG2nQGN6M4JLyRtxFBYFTw4wAVHtGys36");
            BEAST_EXPECT(to_string(calcNodeID(publicKey)) ==
                "AA066C988C712815CC37AF71472B7CBBBD4E2A0A");

            auto sig = sign (publicKey, secretKey, makeSlice(message1));
            BEAST_EXPECT(sig.size() != 0);
            BEAST_EXPECT(verify (publicKey, makeSlice(message1), sig));

//更正公钥但错误消息
            BEAST_EXPECT(!verify (publicKey, makeSlice(message2), sig));

//使用不正确的公钥进行验证
            {
                auto const otherPublicKey = derivePublicKey (
                    KeyType::ed25519,
                    generateSecretKey (
                        KeyType::ed25519,
                        generateSeed ("otherpassphrase")));

                BEAST_EXPECT(!verify (otherPublicKey, makeSlice(message1), sig));
            }

//公钥正确，签名错误
            {
//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[sig.size() / 2]++;

                BEAST_EXPECT(!verify (publicKey, makeSlice(message1), sig));
            }
        }

        {
            testcase ("Account keypair generation & signing (secp256k1)");

            auto const keyPair = generateKeyPair (
                KeyType::secp256k1,
                generateSeed ("masterpassphrase"));

            BEAST_EXPECT(toBase58(calcAccountID(keyPair.first)) ==
                "rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh");
            BEAST_EXPECT(toBase58(TokenType::AccountPublic, keyPair.first) ==
                "aBQG8RQAzjs1eTKFEAQXr2gS4utcDiEC9wmi7pfUPTi27VCahwgw");
            BEAST_EXPECT(toBase58(TokenType::AccountSecret, keyPair.second) ==
                "p9JfM6HHi64m6mvB6v5k7G2b1cXzGmYiCNJf6GHPKvFTWdeRVjh");

            auto sig = sign (keyPair.first, keyPair.second, makeSlice(message1));
            BEAST_EXPECT(sig.size() != 0);
            BEAST_EXPECT(verify (keyPair.first, makeSlice(message1), sig));

//更正公钥但错误消息
            BEAST_EXPECT(!verify (keyPair.first, makeSlice(message2), sig));

//使用不正确的公钥进行验证
            {
                auto const otherKeyPair = generateKeyPair (
                    KeyType::secp256k1,
                    generateSeed ("otherpassphrase"));

                BEAST_EXPECT(!verify (otherKeyPair.first, makeSlice(message1), sig));
            }

//公钥正确，签名错误
            {
//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[sig.size() / 2]++;

                BEAST_EXPECT(!verify (keyPair.first, makeSlice(message1), sig));
            }
        }

        {
            testcase ("Account keypair generation & signing (ed25519)");

            auto const keyPair = generateKeyPair (
                KeyType::ed25519,
                generateSeed ("masterpassphrase"));

            BEAST_EXPECT(to_string(calcAccountID(keyPair.first)) ==
                "rGWrZyQqhTp9Xu7G5Pkayo7bXjH4k4QYpf");
            BEAST_EXPECT(toBase58(TokenType::AccountPublic, keyPair.first) ==
                "aKGheSBjmCsKJVuLNKRAKpZXT6wpk2FCuEZAXJupXgdAxX5THCqR");
            BEAST_EXPECT(toBase58(TokenType::AccountSecret, keyPair.second) ==
                "pwDQjwEhbUBmPuEjFpEG75bFhv2obkCB7NxQsfFxM7xGHBMVPu9");

            auto sig = sign (keyPair.first, keyPair.second, makeSlice(message1));
            BEAST_EXPECT(sig.size() != 0);
            BEAST_EXPECT(verify (keyPair.first, makeSlice(message1), sig));

//更正公钥但错误消息
            BEAST_EXPECT(!verify (keyPair.first, makeSlice(message2), sig));

//使用不正确的公钥进行验证
            {
                auto const otherKeyPair = generateKeyPair (
                    KeyType::ed25519,
                    generateSeed ("otherpassphrase"));

                BEAST_EXPECT(!verify (otherKeyPair.first, makeSlice(message1), sig));
            }

//公钥正确，签名错误
            {
//稍微更改签名：
                if (auto ptr = sig.data())
                    ptr[sig.size() / 2]++;

                BEAST_EXPECT(!verify (keyPair.first, makeSlice(message1), sig));
            }
        }
    }

    void testSeedParsing ()
    {
        testcase ("Parsing");

//帐户ID和节点以及公用和专用帐户
//键不应被解析为种子。

        auto const node1 = randomKeyPair(KeyType::secp256k1);

        BEAST_EXPECT(!parseGenericSeed (
            toBase58 (TokenType::NodePublic, node1.first)));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58 (TokenType::NodePrivate, node1.second)));

        auto const node2 = randomKeyPair(KeyType::ed25519);

        BEAST_EXPECT(!parseGenericSeed (
            toBase58 (TokenType::NodePublic, node2.first)));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58 (TokenType::NodePrivate, node2.second)));

        auto const account1 = generateKeyPair(
            KeyType::secp256k1, randomSeed ());

        BEAST_EXPECT(!parseGenericSeed (
            toBase58(calcAccountID(account1.first))));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58(TokenType::AccountPublic, account1.first)));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58(TokenType::AccountSecret, account1.second)));

        auto const account2 = generateKeyPair(
            KeyType::ed25519, randomSeed ());

        BEAST_EXPECT(!parseGenericSeed (
            toBase58(calcAccountID(account2.first))));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58(TokenType::AccountPublic, account2.first)));
        BEAST_EXPECT(!parseGenericSeed (
            toBase58(TokenType::AccountSecret, account2.second)));
    }

    void run() override
    {
        testConstruction();
        testPassphrase();
        testBase58();
        testRandom();
        testKeypairGenerationAndSigning();
        testSeedParsing ();
    }
};

BEAST_DEFINE_TESTSUITE(Seed,protocol,ripple);

} //涟漪
