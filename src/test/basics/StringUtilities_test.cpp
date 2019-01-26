
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

#include <ripple/basics/Slice.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/ToString.h>
#include <ripple/beast/unit_test.h>

namespace ripple {

class StringUtilities_test : public beast::unit_test::suite
{
public:
    void testUnHexSuccess (std::string const& strIn, std::string const& strExpected)
    {
        auto rv = strUnHex (strIn);
        BEAST_EXPECT(rv.second);
        BEAST_EXPECT(makeSlice(rv.first) == makeSlice(strExpected));
    }

    void testUnHexFailure (std::string const& strIn)
    {
        auto rv = strUnHex (strIn);
        BEAST_EXPECT(! rv.second);
        BEAST_EXPECT(rv.first.empty());
    }

    void testUnHex ()
    {
        testcase ("strUnHex");

        testUnHexSuccess ("526970706c6544", "RippleD");
        testUnHexSuccess ("A", "\n");
        testUnHexSuccess ("0A", "\n");
        testUnHexSuccess ("D0A", "\r\n");
        testUnHexSuccess ("0D0A", "\r\n");
        testUnHexSuccess ("200D0A", " \r\n");
        testUnHexSuccess ("282A2B2C2D2E2F29", "(*+,-./)");

//检查是否包含某些或仅包含无效字符的内容
        testUnHexFailure ("123X");
        testUnHexFailure ("V");
        testUnHexFailure ("XRP");
    }

    void testParseUrl ()
    {
        testcase ("parseUrl");

//预期通过。
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://“）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain.empty());
            BEAST_EXPECT(! pUrl.port);
//RFC 3986：
//>一般来说，使用通用语法进行授权的URI
//如果路径为空，则应将其规范化为“/”。
//我们要规范化路径吗？
            BEAST_EXPECT(pUrl.path.empty());
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://）
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain.empty());
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "lower://域“）；
            BEAST_EXPECT(pUrl.scheme == "lower");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path.empty());
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "UPPER://域：234 /“”）；
            BEAST_EXPECT(pUrl.scheme == "upper");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(*pUrl.port == 234);
            BEAST_EXPECT(pUrl.path == "/");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "Mixed://域/路径“）；
            BEAST_EXPECT(pUrl.scheme == "mixed");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/path");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://[：：1]：123/路径”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "::1");
            BEAST_EXPECT(*pUrl.port == 123);
            BEAST_EXPECT(pUrl.path == "/path");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://user:pass@domain:123/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username == "user");
            BEAST_EXPECT(pUrl.password == "pass");
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(*pUrl.port == 123);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://user@domain:123/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username == "user");
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(*pUrl.port == 123);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://：pass@domain:123/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password == "pass");
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(*pUrl.port == 123);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://域名：123/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(*pUrl.port == 123);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://用户：pass@domain/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username == "user");
            BEAST_EXPECT(pUrl.password == "pass");
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://用户@domain/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username == "user");
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://：pass@domain/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password == "pass");
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://域/abc:321”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/abc:321");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme:///path/to/file”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain.empty());
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/path/to/file");
        }
        {
            parsedURL pUrl;
            BEAST_EXPECT(parseUrl (
pUrl, "scheme://用户：pass@domain/path/with/an@sign”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username == "user");
            BEAST_EXPECT(pUrl.password == "pass");
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/path/with/an@sign");
        }
        {
            parsedURL pUrl;
            BEAST_EXPECT(parseUrl (
pUrl, "scheme://域/路径/带/an@符号”）；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain == "domain");
            BEAST_EXPECT(! pUrl.port);
            BEAST_EXPECT(pUrl.path == "/path/with/an@sign");
        }
        {
            parsedURL pUrl;
BEAST_EXPECT(parseUrl (pUrl, "scheme://：999（/））；
            BEAST_EXPECT(pUrl.scheme == "scheme");
            BEAST_EXPECT(pUrl.username.empty());
            BEAST_EXPECT(pUrl.password.empty());
            BEAST_EXPECT(pUrl.domain.empty());
            BEAST_EXPECT(*pUrl.port == 999);
            BEAST_EXPECT(pUrl.path == "/");
        }

//预期失败。
        {
            parsedURL pUrl;
            BEAST_EXPECT(! parseUrl (pUrl, ""));
            BEAST_EXPECT(! parseUrl (pUrl, "nonsense"));
BEAST_EXPECT(! parseUrl (pUrl, "://“）；
BEAST_EXPECT(! parseUrl (pUrl, "://）
        }
    }

    void testToString ()
    {
        testcase ("toString");
        auto result = to_string("hello");
        BEAST_EXPECT(result == "hello");
    }

    void run () override
    {
        testParseUrl ();
        testUnHex ();
        testToString ();
    }
};

BEAST_DEFINE_TESTSUITE(StringUtilities, ripple_basics, ripple);

} //涟漪
