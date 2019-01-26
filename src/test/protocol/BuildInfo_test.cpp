
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

#include <ripple/protocol/BuildInfo.h>
#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class BuildInfo_test : public beast::unit_test::suite
{
public:
    ProtocolVersion
    from_version (std::uint16_t major, std::uint16_t minor)
    {
        return ProtocolVersion (major, minor);
    }

    void testValues ()
    {
        testcase ("comparison");

        BEAST_EXPECT(from_version (1,2) == from_version (1,2));
        BEAST_EXPECT(from_version (3,4) >= from_version (3,4));
        BEAST_EXPECT(from_version (5,6) <= from_version (5,6));
        BEAST_EXPECT(from_version (7,8) >  from_version (6,7));
        BEAST_EXPECT(from_version (7,8) <  from_version (8,9));
        BEAST_EXPECT(from_version (65535,0) <  from_version (65535,65535));
        BEAST_EXPECT(from_version (65535,65535) >= from_version (65535,65535));
    }

    void testStringVersion ()
    {
        testcase ("string version");

        for (std::uint16_t major = 0; major < 8; major++)
        {
            for (std::uint16_t minor = 0; minor < 8; minor++)
            {
                BEAST_EXPECT(to_string (from_version (major, minor)) ==
                    std::to_string (major) + "." + std::to_string (minor));
            }
        }
    }

    void testVersionPacking ()
    {
        testcase ("version packing");

        BEAST_EXPECT(to_packed (from_version (0, 0)) == 0);
        BEAST_EXPECT(to_packed (from_version (0, 1)) == 1);
        BEAST_EXPECT(to_packed (from_version (0, 255)) == 255);
        BEAST_EXPECT(to_packed (from_version (0, 65535)) == 65535);

        BEAST_EXPECT(to_packed (from_version (1, 0)) == 65536);
        BEAST_EXPECT(to_packed (from_version (1, 1)) == 65537);
        BEAST_EXPECT(to_packed (from_version (1, 255)) == 65791);
        BEAST_EXPECT(to_packed (from_version (1, 65535)) == 131071);

        BEAST_EXPECT(to_packed (from_version (255, 0)) == 16711680);
        BEAST_EXPECT(to_packed (from_version (255, 1)) == 16711681);
        BEAST_EXPECT(to_packed (from_version (255, 255)) == 16711935);
        BEAST_EXPECT(to_packed (from_version (255, 65535)) == 16777215);

        BEAST_EXPECT(to_packed (from_version (65535, 0)) == 4294901760);
        BEAST_EXPECT(to_packed (from_version (65535, 1)) == 4294901761);
        BEAST_EXPECT(to_packed (from_version (65535, 255)) == 4294902015);
        BEAST_EXPECT(to_packed (from_version (65535, 65535)) == 4294967295);
    }

    void run () override
    {
        testValues ();
        testStringVersion ();
        testVersionPacking ();

        auto const current_protocol = BuildInfo::getCurrentProtocol ();
        auto const minimum_protocol = BuildInfo::getMinimumProtocol ();

        BEAST_EXPECT(current_protocol >= minimum_protocol);

        log <<
            "   Ripple Version: " << BuildInfo::getVersionString() << '\n' <<
            " Protocol Version: " << to_string (current_protocol) << std::endl;
    }
};

BEAST_DEFINE_TESTSUITE(BuildInfo,ripple_data,ripple);

} //涟漪
