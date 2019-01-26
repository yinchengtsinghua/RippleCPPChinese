
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
#include <ripple/basics/Slice.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/ToString.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/algorithm/string.hpp>
#include <ripple/beast/net/IPEndpoint.h>
#include <boost/regex.hpp>
#include <algorithm>
#include <cstdarg>

namespace ripple {

std::pair<Blob, bool> strUnHex (std::string const& strSrc)
{
    Blob out;

    out.reserve ((strSrc.size () + 1) / 2);

    auto iter = strSrc.cbegin ();

    if (strSrc.size () & 1)
    {
        int c = charUnHex (*iter);

        if (c < 0)
            return std::make_pair (Blob (), false);

        out.push_back(c);
        ++iter;
    }

    while (iter != strSrc.cend ())
    {
        int cHigh = charUnHex (*iter);
        ++iter;

        if (cHigh < 0)
            return std::make_pair (Blob (), false);

        int cLow = charUnHex (*iter);
        ++iter;

        if (cLow < 0)
            return std::make_pair (Blob (), false);

        out.push_back (static_cast<unsigned char>((cHigh << 4) | cLow));
    }

    return std::make_pair(std::move(out), true);
}

uint64_t uintFromHex (std::string const& strSrc)
{
    uint64_t uValue (0);

    if (strSrc.size () > 16)
        Throw<std::invalid_argument> ("overlong 64-bit value");

    for (auto c : strSrc)
    {
        int ret = charUnHex (c);

        if (ret == -1)
            Throw<std::invalid_argument> ("invalid hex digit");

        uValue = (uValue << 4) | ret;
    }

    return uValue;
}

bool parseUrl (parsedURL& pUrl, std::string const& strUrl)
{
//scheme://username:password@hostname:port/rest
    static boost::regex reUrl (
        "(?i)\\`\\s*"
//所需方案
        "([[:alpha:]][-+.[:alpha:][:digit:]]*):"
//我们只支持“hier part”具有表单的uri
//`“/”授权路径abempty`。
        "//"
//可选用户信息
        "(?:([^/]*?)(?::([^/]*?))?@)?"
//任选主机
        "([^/]*?)"
//任选端口
        "(?::([[:digit:]]+))?"
//可选路径
        "(/.*)?"
        "\\s*?\\'");
    boost::smatch smMatch;

bool bMatch = boost::regex_match (strUrl, smMatch, reUrl); //匹配状态代码。

    if (bMatch)
    {
        pUrl.scheme = smMatch[1];
        boost::algorithm::to_lower (pUrl.scheme);
        pUrl.username = smMatch[2];
        pUrl.password = smMatch[3];
        const std::string domain = smMatch[4];
//我们需要使用端点将域解析为
//从ipv6地址中去掉括号，
//例如[：：1]=>：：1。
        const auto result {beast::IP::Endpoint::from_string_checked (domain)};
        pUrl.domain = result.second
          ? result.first.address().to_string()
          : domain;
        const std::string port = smMatch[5];
        if (!port.empty())
        {
          pUrl.port = beast::lexicalCast <std::uint16_t> (port);
        }
        pUrl.path = smMatch[6];
    }

    return bMatch;
}

std::string trim_whitespace (std::string str)
{
    boost::trim (str);
    return str;
}

boost::optional<std::uint64_t>
to_uint64(std::string const& s)
{
    std::uint64_t result;
    if (beast::lexicalCastChecked (result, s))
        return result;
    return boost::none;
}

} //涟漪
