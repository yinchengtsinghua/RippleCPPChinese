
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

#ifndef RIPPLE_OVERLAY_TMHELLO_H_INCLUDED
#define RIPPLE_OVERLAY_TMHELLO_H_INCLUDED

#include <ripple/protocol/messages.h>
#include <ripple/app/main/Application.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/protocol/BuildInfo.h>

#include <boost/beast/http/message.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/optional.hpp>
#include <utility>

namespace ripple {

enum
{
//我们允许远程对等机拥有的时钟漂移
    clockToleranceDeltaSeconds = 20
};

/*根据SSL连接状态计算共享值。
    当中间没有人时，双方都会计算相同的结果。
    价值。在攻击者在场的情况下，计算的值将
    不同的。
    如果共享值生成失败，则必须删除链接。
    @返回一对。如果共享值生成失败，则第二个值将为false。
**/

boost::optional<uint256>
makeSharedValue (SSL* ssl, beast::Journal journal);

/*建立一个tmhello协议消息。*/
protocol::TMHello
buildHello (uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote, Application& app);

/*根据tmhello协议消息插入HTTP头。*/
void
appendHello (boost::beast::http::fields& h, protocol::TMHello const& hello);

/*将HTTP头解析为tmhello协议消息。
    @成功时返回协议消息；空选项
            如果分析失败。
**/

boost::optional<protocol::TMHello>
parseHello (bool request, boost::beast::http::fields const& h, beast::Journal journal);

/*验证公钥并将其存储在tmhello中。
    这包括对共享值的签名验证。
    @成功时返回远程端公钥；空
            如果检查失败，则为可选。
**/

boost::optional<PublicKey>
verifyHello (protocol::TMHello const& h, uint256 const& sharedValue,
    beast::IP::Address public_ip,
    beast::IP::Endpoint remote,
    beast::Journal journal, Application& app);

/*分析一组协议版本。
    返回的列表不包含重复项，按升序排序。
    任何不能解析为RTXP协议字符串的字符串都是
    从结果集中排除。
**/

std::vector<ProtocolVersion>
parse_ProtocolVersions(boost::beast::string_view const& s);

}

#endif
