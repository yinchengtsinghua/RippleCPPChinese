
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
#include <ripple/beast/unit_test.h>
#include <ripple/beast/core/SemanticVersion.h>
namespace beast {

class SemanticVersion_test : public unit_test::suite
{
    using identifier_list = SemanticVersion::identifier_list;

public:
    void checkPass(std::string const& input, bool shouldPass = true)
    {
        SemanticVersion v;

        if (shouldPass)
        {
            BEAST_EXPECT(v.parse(input));
            BEAST_EXPECT(v.print() == input);
        }
        else
        {
            BEAST_EXPECT(!v.parse(input));
        }
    }

    void checkFail(std::string const& input)
    {
        checkPass(input, false);
    }

//使用附加的元数据检查输入和输入
    void checkMeta(std::string const& input, bool shouldPass)
    {
        checkPass(input, shouldPass);

        checkPass(input + "+a", shouldPass);
        checkPass(input + "+1", shouldPass);
        checkPass(input + "+a.b", shouldPass);
        checkPass(input + "+ab.cd", shouldPass);

        checkFail(input + "!");
        checkFail(input + "+");
        checkFail(input + "++");
        checkFail(input + "+!");
        checkFail(input + "+.");
        checkFail(input + "+a.!");
    }

    void checkMetaFail(std::string const& input)
    {
        checkMeta(input, false);
    }

//检查输入，输入附加的发布数据，
//带有附加元数据的输入，以及带有两个元数据的输入
//附加发布数据和附加元数据
//
    void checkRelease(std::string const& input, bool shouldPass = true)
    {
        checkMeta(input, shouldPass);

        checkMeta(input + "-1", shouldPass);
        checkMeta(input + "-a", shouldPass);
        checkMeta(input + "-a1", shouldPass);
        checkMeta(input + "-a1.b1", shouldPass);
        checkMeta(input + "-ab.cd", shouldPass);
        checkMeta(input + "--", shouldPass);

        checkMetaFail(input + "+");
        checkMetaFail(input + "!");
        checkMetaFail(input + "-");
        checkMetaFail(input + "-!");
        checkMetaFail(input + "-.");
        checkMetaFail(input + "-a.!");
        checkMetaFail(input + "-0.a");
    }

//单独检查major.minor.version字符串
//发布标识符和元数据的可能组合。
//
    void check(std::string const& input, bool shouldPass = true)
    {
        checkRelease(input, shouldPass);
    }

    void negcheck(std::string const& input)
    {
        check(input, false);
    }

    void testParse()
    {
        testcase("parsing");

        check("0.0.0");
        check("1.2.3");
check("2147483647.2147483647.2147483647"); //最大整数

//负值
        negcheck("-1.2.3");
        negcheck("1.-2.3");
        negcheck("1.2.-3");

//缺少零件
        negcheck("");
        negcheck("1");
        negcheck("1.");
        negcheck("1.2");
        negcheck("1.2.");
        negcheck(".2.3");

//空白区
        negcheck(" 1.2.3");
        negcheck("1 .2.3");
        negcheck("1.2 .3");
        negcheck("1.2.3 ");

//前导零点
        negcheck("01.2.3");
        negcheck("1.02.3");
        negcheck("1.2.03");
    }

    static identifier_list ids()
    {
        return identifier_list();
    }

    static identifier_list ids(
        std::string const& s1)
    {
        identifier_list v;
        v.push_back(s1);
        return v;
    }

    static identifier_list ids(
        std::string const& s1, std::string const& s2)
    {
        identifier_list v;
        v.push_back(s1);
        v.push_back(s2);
        return v;
    }

    static identifier_list ids(
        std::string const& s1, std::string const& s2, std::string const& s3)
    {
        identifier_list v;
        v.push_back(s1);
        v.push_back(s2);
        v.push_back(s3);
        return v;
    }

//检查输入分解成适当的值
    void checkValues(std::string const& input,
        int majorVersion,
        int minorVersion,
        int patchVersion,
        identifier_list const& preReleaseIdentifiers = identifier_list(),
        identifier_list const& metaData = identifier_list())
    {
        SemanticVersion v;

        BEAST_EXPECT(v.parse(input));

        BEAST_EXPECT(v.majorVersion == majorVersion);
        BEAST_EXPECT(v.minorVersion == minorVersion);
        BEAST_EXPECT(v.patchVersion == patchVersion);

        BEAST_EXPECT(v.preReleaseIdentifiers == preReleaseIdentifiers);
        BEAST_EXPECT(v.metaData == metaData);
    }

    void testValues()
    {
        testcase("values");

        checkValues("0.1.2", 0, 1, 2);
        checkValues("1.2.3", 1, 2, 3);
        checkValues("1.2.3-rc1", 1, 2, 3, ids("rc1"));
        checkValues("1.2.3-rc1.debug", 1, 2, 3, ids("rc1", "debug"));
        checkValues("1.2.3-rc1.debug.asm", 1, 2, 3, ids("rc1", "debug", "asm"));
        checkValues("1.2.3+full", 1, 2, 3, ids(), ids("full"));
        checkValues("1.2.3+full.prod", 1, 2, 3, ids(), ids("full", "prod"));
        checkValues("1.2.3+full.prod.x86", 1, 2, 3, ids(), ids("full", "prod", "x86"));
        checkValues("1.2.3-rc1.debug.asm+full.prod.x86", 1, 2, 3,
            ids("rc1", "debug", "asm"), ids("full", "prod", "x86"));
    }

//确保左侧版本小于右侧版本
    void checkLessInternal(std::string const& lhs, std::string const& rhs)
    {
        SemanticVersion left;
        SemanticVersion right;

        BEAST_EXPECT(left.parse(lhs));
        BEAST_EXPECT(right.parse(rhs));

        BEAST_EXPECT(compare(left, left) == 0);
        BEAST_EXPECT(compare(right, right) == 0);
        BEAST_EXPECT(compare(left, right) < 0);
        BEAST_EXPECT(compare(right, left) > 0);

        BEAST_EXPECT(left < right);
        BEAST_EXPECT(right > left);
        BEAST_EXPECT(left == left);
        BEAST_EXPECT(right == right);
    }

    void checkLess(std::string const& lhs, std::string const& rhs)
    {
        checkLessInternal(lhs, rhs);
        checkLessInternal(lhs + "+meta", rhs);
        checkLessInternal(lhs, rhs + "+meta");
        checkLessInternal(lhs + "+meta", rhs + "+meta");
    }

    void testCompare()
    {
        testcase("comparisons");

        checkLess("1.0.0-alpha", "1.0.0-alpha.1");
        checkLess("1.0.0-alpha.1", "1.0.0-alpha.beta");
        checkLess("1.0.0-alpha.beta", "1.0.0-beta");
        checkLess("1.0.0-beta", "1.0.0-beta.2");
        checkLess("1.0.0-beta.2", "1.0.0-beta.11");
        checkLess("1.0.0-beta.11", "1.0.0-rc.1");
        checkLess("1.0.0-rc.1", "1.0.0");
        checkLess("0.9.9", "1.0.0");
    }

    void run() override
    {
        testParse();
        testValues();
        testCompare();
    }
};

BEAST_DEFINE_TESTSUITE(SemanticVersion, beast_core, beast);
}