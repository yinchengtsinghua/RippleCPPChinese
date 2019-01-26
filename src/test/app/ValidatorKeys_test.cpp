
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
    版权所有2017 Ripple Labs Inc.

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

#include <ripple/app/misc/ValidatorKeys.h>
#include <ripple/app/misc/Manifest.h>
#include <ripple/basics/base64.h>
#include <ripple/beast/unit_test.h>
#include <ripple/core/Config.h>
#include <ripple/core/ConfigSections.h>
#include <test/unit_test/SuiteJournal.h>
#include <string>

namespace ripple {
namespace test {

class ValidatorKeys_test : public beast::unit_test::suite
{
//与[验证种子]一起使用
    const std::string seed = "shUwVw52ofnCUX5m7kPTKzJdr4HEH";

//与[验证标记]一起使用
    const std::string tokenSecretStr =
        "paQmjZ37pKKPMrgadBLsuf9ab7Y7EUNzh27LQrZqoexpAs31nJi";

    const std::vector<std::string> tokenBlob = {
        "    "
        "eyJ2YWxpZGF0aW9uX3NlY3JldF9rZXkiOiI5ZWQ0NWY4NjYyNDFjYzE4YTI3NDdiNT\n",
        " \tQzODdjMDYyNTkwNzk3MmY0ZTcxOTAyMzFmYWE5Mzc0NTdmYTlkYWY2IiwibWFuaWZl "
        "    \n",
        "\tc3QiOiJKQUFBQUFGeEllMUZ0d21pbXZHdEgyaUNjTUpxQzlnVkZLaWxHZncxL3ZDeE"
        "\n",
        "\t "
        "hYWExwbGMyR25NaEFrRTFhZ3FYeEJ3RHdEYklENk9NU1l1TTBGREFscEFnTms4U0tG\t  "
        "\t\n",
        "bjdNTzJmZGtjd1JRSWhBT25ndTlzQUtxWFlvdUorbDJWMFcrc0FPa1ZCK1pSUzZQU2\n",
        "hsSkFmVXNYZkFpQnNWSkdlc2FhZE9KYy9hQVpva1MxdnltR21WcmxIUEtXWDNZeXd1\n",
        "NmluOEhBU1FLUHVnQkQ2N2tNYVJGR3ZtcEFUSGxHS0pkdkRGbFdQWXk1QXFEZWRGdj\n",
        "VUSmEydzBpMjFlcTNNWXl3TFZKWm5GT3I3QzBrdzJBaVR6U0NqSXpkaXRROD0ifQ==\n"};

    const std::string tokenManifest =
        "JAAAAAFxIe1FtwmimvGtH2iCcMJqC9gVFKilGfw1/vCxHXXLplc2GnMhAkE1agqXxBwD"
        "wDbID6OMSYuM0FDAlpAgNk8SKFn7MO2fdkcwRQIhAOngu9sAKqXYouJ+l2V0W+sAOkVB"
        "+ZRS6PShlJAfUsXfAiBsVJGesaadOJc/aAZokS1vymGmVrlHPKWX3Yywu6in8HASQKPu"
        "gBD67kMaRFGvmpATHlGKJdvDFlWPYy5AqDedFv5TJa2w0i21eq3MYywLVJZnFOr7C0kw"
        "2AiTzSCjIzditQ8=";

//清单与私钥不匹配
    const std::vector<std::string> invalidTokenBlob = {
        "eyJtYW5pZmVzdCI6IkpBQUFBQVZ4SWUyOVVBdzViZFJudHJ1elVkREk4aDNGV1JWZl\n",
        "k3SXVIaUlKQUhJd3MxdzZzM01oQWtsa1VXQWR2RnFRVGRlSEpvS1pNY0hlS0RzOExo\n",
        "b3d3bDlHOEdkVGNJbmFka1l3UkFJZ0h2Q01lQU1aSzlqQnV2aFhlaFRLRzVDQ3BBR1\n",
        "k0bGtvZHRXYW84UGhzR3NDSUREVTA1d1c3bWNiMjlVNkMvTHBpZmgvakZPRGhFR21i\n",
        "NWF6dTJMVHlqL1pjQkpBbitmNGhtQTQ0U0tYbGtTTUFqak1rSWRyR1Rxa21SNjBzVG\n",
        "JaTjZOOUYwdk9UV3VYcUZ6eDFoSGIyL0RqWElVZXhDVGlITEcxTG9UdUp1eXdXbk55\n",
        "RFE9PSIsInZhbGlkYXRpb25fc2VjcmV0X2tleSI6IjkyRDhCNDBGMzYwMTc5MTkwMU\n",
        "MzQTUzMzI3NzBDMkUwMTA4MDI0NTZFOEM2QkI0NEQ0N0FFREQ0NzJGMDQ2RkYifQ==\n"};

public:
    void
    run() override
    {
        SuiteJournal journal ("ValidatorKeys_test", *this);

//使用[验证种子]时的密钥/ID
        SecretKey const seedSecretKey =
            generateSecretKey(KeyType::secp256k1, *parseBase58<Seed>(seed));
        PublicKey const seedPublicKey =
            derivePublicKey(KeyType::secp256k1, seedSecretKey);
        NodeID const seedNodeID = calcNodeID(seedPublicKey);

//使用[validation_token]时的密钥
        auto const tokenSecretKey = *parseBase58<SecretKey>(
            TokenType::NodePrivate, tokenSecretStr);

        auto const tokenPublicKey =
            derivePublicKey(KeyType::secp256k1, tokenSecretKey);

        auto const m = Manifest::make_Manifest(
            base64_decode(tokenManifest));
        BEAST_EXPECT(m);
        NodeID const tokenNodeID = calcNodeID(m->masterKey);

        {
//无配置->无键但有效
            Config c;
            ValidatorKeys k{c, journal};
            BEAST_EXPECT(k.publicKey.size() == 0);
            BEAST_EXPECT(k.manifest.empty());
            BEAST_EXPECT(!k.configInvalid());

        }
        {
//验证种子部分->空清单和有效种子
            Config c;
            c.section(SECTION_VALIDATION_SEED).append(seed);

            ValidatorKeys k{c, journal};
            BEAST_EXPECT(k.publicKey == seedPublicKey);
            BEAST_EXPECT(k.secretKey == seedSecretKey);
            BEAST_EXPECT(k.nodeID == seedNodeID);
            BEAST_EXPECT(k.manifest.empty());
            BEAST_EXPECT(!k.configInvalid());
        }

        {
//验证种子错误种子->无效
            Config c;
            c.section(SECTION_VALIDATION_SEED).append("badseed");

            ValidatorKeys k{c, journal};
            BEAST_EXPECT(k.configInvalid());
            BEAST_EXPECT(k.publicKey.size() == 0);
            BEAST_EXPECT(k.manifest.empty());
        }

        {
//验证令牌
            Config c;
            c.section(SECTION_VALIDATOR_TOKEN).append(tokenBlob);
            ValidatorKeys k{c, journal};

            BEAST_EXPECT(k.publicKey == tokenPublicKey);
            BEAST_EXPECT(k.secretKey == tokenSecretKey);
            BEAST_EXPECT(k.nodeID == tokenNodeID);
            BEAST_EXPECT(k.manifest == tokenManifest);
            BEAST_EXPECT(!k.configInvalid());
        }
        {
//无效的验证程序令牌
            Config c;
            c.section(SECTION_VALIDATOR_TOKEN).append("badtoken");
            ValidatorKeys k{c, journal};
            BEAST_EXPECT(k.configInvalid());
            BEAST_EXPECT(k.publicKey.size() == 0);
            BEAST_EXPECT(k.manifest.empty());
        }

        {
//不能同时指定
            Config c;
            c.section(SECTION_VALIDATION_SEED).append(seed);
            c.section(SECTION_VALIDATOR_TOKEN).append(tokenBlob);
            ValidatorKeys k{c, journal};

            BEAST_EXPECT(k.configInvalid());
            BEAST_EXPECT(k.publicKey.size() == 0);
            BEAST_EXPECT(k.manifest.empty());
        }

        {
//令牌清单和私钥必须匹配
            Config c;
            c.section(SECTION_VALIDATOR_TOKEN).append(invalidTokenBlob);
            ValidatorKeys k{c, journal};

            BEAST_EXPECT(k.configInvalid());
            BEAST_EXPECT(k.publicKey.size() == 0);
            BEAST_EXPECT(k.manifest.empty());
        }

    }
};  //命名空间测试

BEAST_DEFINE_TESTSUITE(ValidatorKeys, app, ripple);

}  //命名空间测试
}  //命名空间波纹
