
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

#ifndef RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED
#define RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED

#include <cstdint>
#include <string>

namespace ripple {

/*描述Ripple/RTXP协议版本。*/
using ProtocolVersion = std::pair<std::uint16_t, std::uint16_t>;

/*此生成的版本信息。*/
//vfalc命名空间已弃用
namespace BuildInfo {

/*服务器版本。
    遵循语义版本化规范：
    http://semver.org/
**/

std::string const&
getVersionString();

/*完整服务器版本字符串。
    这包括服务器的名称。它在对等机中使用
    协议hello消息和一些HTTP回复的头。
**/

std::string const&
getFullVersionString();

/*从打包的32位协议标识符构造协议版本*/
ProtocolVersion
make_protocol (std::uint32_t version);

/*我们所说的并且更喜欢的协议版本。*/
ProtocolVersion const&
getCurrentProtocol();

/*我们将接受的最旧协议版本。*/
ProtocolVersion const& getMinimumProtocol ();

} //buildinfo（已弃用）

std::string
to_string (ProtocolVersion const& p);

std::uint32_t
to_packed (ProtocolVersion const& p);

} //涟漪

#endif
