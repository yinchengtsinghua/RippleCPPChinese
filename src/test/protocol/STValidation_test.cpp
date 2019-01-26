
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

#include <ripple/basics/Log.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/st.h>
#include <ripple/json/json_reader.h>
#include <ripple/json/to_string.h>
#include <ripple/beast/unit_test.h>
#include <test/jtx.h>

#include <memory>
#include <type_traits>

namespace ripple {

class STValidation_test : public beast::unit_test::suite
{
public:
    void testDeserialization ()
    {
        testcase ("Deserialization");

        constexpr std::uint8_t payload1[] =
        {
//指定ED25519公钥。
            0x22, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x51,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x73,
            0x21, 0xed, 0x78, 0x00, 0xe6, 0x73, 0x00, 0x72, 0x00, 0x3c, 0x00,
            0x00, 0x00, 0x88, 0x00, 0xe6, 0x73, 0x38, 0x00, 0x00, 0x8a, 0x00,
            0x88, 0x4e, 0x31, 0x30, 0x5f, 0x5f, 0x63, 0x78, 0x78, 0x61, 0x62,
            0x69
        };

        constexpr std::uint8_t payload2[] =
        {
            0x22, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x51,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x73,
            0x21, 0xed, 0xff, 0x03, 0x1c, 0xbe, 0x65, 0x22, 0x61, 0x9c, 0x5e,
            0x13, 0x12, 0x00, 0x3b, 0x43, 0x00, 0x00, 0x00, 0xf7, 0x3f, 0x3f,
            0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x3f, 0x13, 0x13, 0x13, 0x3a, 0x27,
            0xff
        };

        constexpr std::uint8_t payload3[] =
        {
//完全没有公钥
            0x22, 0x00, 0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 0x51,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a
        };

        try
        {
            SerialIter sit {payload1};
            auto stx = std::make_shared<ripple::STValidation>(sit,
                [](PublicKey const& pk) {
                    return calcNodeID(pk);
                }, false);
            fail("An exception should have been thrown");
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Invalid public key in validation") == 0);
        }

        try
        {
            SerialIter sit {payload2};
            auto stx = std::make_shared<ripple::STValidation>(sit,
                [](PublicKey const& pk) {
                    return calcNodeID(pk);
                }, false);
            fail("An exception should have been thrown");
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Invalid public key in validation") == 0);
        }

        try
        {
            SerialIter sit {payload3};
            auto stx = std::make_shared<ripple::STValidation>(sit,
                [](PublicKey const& pk) {
                    return calcNodeID(pk);
                }, false);
            fail("An exception should have been thrown");
        }
        catch (std::exception const& e)
        {
            BEAST_EXPECT(strcmp(e.what(),
                "Field 'SigningPubKey' is required but missing.") == 0);
        }
    }

    void
    run() override
    {
        testDeserialization();
    }
};

BEAST_DEFINE_TESTSUITE(STValidation,protocol,ripple);

} //涟漪
