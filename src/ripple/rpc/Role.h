
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

#ifndef RIPPLE_SERVER_ROLE_H_INCLUDED
#define RIPPLE_SERVER_ROLE_H_INCLUDED

#include <ripple/server/Port.h>
#include <ripple/json/json_value.h>
#include <ripple/resource/ResourceManager.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <vector>

namespace ripple {

/*指示要授予的管理权限级别。
 *已识别的角色具有无限的资源，但无法执行某些
 *rpc命令。
 *管理员角色拥有无限的资源，能够执行所有RPC
 *命令。
 **/

enum class Role
{
    GUEST,
    USER,
    IDENTIFIED,
    ADMIN,
    FORBID
};

/*返回允许的特权角色。
    json rpc必须满足json-rpc的要求
    规范。它必须是对象类型，包含键参数
    它是一个至少有一个对象的数组。在这个物体内
    是否使用了可选的“admin-user”和“admin-password”键
    验证凭据。
**/

Role
requestRole (Role const& required, Port const& port,
    Json::Value const& jsonRPC, beast::IP::Endpoint const& remoteIp,
    std::string const& user);

Resource::Consumer
requestInboundEndpoint (Resource::Manager& manager,
    beast::IP::Endpoint const& remoteAddress,
        Port const& port, std::string const& user);

/*
 *检查角色是否允许用户使用无限资源。
 **/

bool
isUnlimited (Role const& role);

/*
 *如果存在具有非空值的HTTP头x-user，则由IP传递
 *配置为安全\网关，则可以正确识别用户。
 **/

bool
isIdentified (Port const& port, beast::IP::Address const& remoteIp,
        std::string const& user);

} //涟漪

#endif
