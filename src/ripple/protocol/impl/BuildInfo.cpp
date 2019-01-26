
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
#include <ripple/beast/core/PlatformConfig.h>
#include <ripple/beast/core/SemanticVersion.h>
#include <ripple/protocol/BuildInfo.h>

namespace ripple {

namespace BuildInfo {

//——————————————————————————————————————————————————————————————
//内部版本号。您必须为每个版本编辑此
//按照http://semver.org/
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
char const* const versionString = "1.2.0-b11"

#if defined(DEBUG) || defined(SANITIZER)
       "+"
#ifdef DEBUG
        "DEBUG"
#ifdef SANITIZER
        "."
#endif
#endif

#ifdef SANITIZER
        BEAST_PP_STR1_(SANITIZER)
#endif
#endif

//——————————————————————————————————————————————————————————————
    ;

ProtocolVersion const&
getCurrentProtocol ()
{
    static ProtocolVersion currentProtocol (
//——————————————————————————————————————————————————————————————
//
//我们所说的首选协议版本（如有必要，请编辑此版本）
//
1,  //专业
2   //少数的
//
//——————————————————————————————————————————————————————————————
    );

    return currentProtocol;
}

ProtocolVersion const&
getMinimumProtocol ()
{
    static ProtocolVersion minimumProtocol (

//——————————————————————————————————————————————————————————————
//
//我们将接受的最旧协议版本。（如有必要，请编辑此项）
//
1,  //专业
2   //少数的
//
//——————————————————————————————————————————————————————————————
    );

    return minimumProtocol;
}

//
//
//不要碰这条线下的任何东西
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::string const&
getVersionString ()
{
    static std::string const value = [] {
        std::string const s = versionString;
        beast::SemanticVersion v;
        if (!v.parse (s) || v.print () != s)
            LogicError (s + ": Bad server version string");
        return s;
    }();
    return value;
}

std::string const& getFullVersionString ()
{
    static std::string const value =
        "rippled-" + getVersionString();
    return value;
}

ProtocolVersion
make_protocol (std::uint32_t version)
{
    return ProtocolVersion(
        static_cast<std::uint16_t> ((version >> 16) & 0xffff),
        static_cast<std::uint16_t> (version & 0xffff));
}

}

std::string
to_string (ProtocolVersion const& p)
{
    return std::to_string (p.first) + "." + std::to_string (p.second);
}

std::uint32_t
to_packed (ProtocolVersion const& p)
{
    return (static_cast<std::uint32_t> (p.first) << 16) + p.second;
}

} //涟漪
