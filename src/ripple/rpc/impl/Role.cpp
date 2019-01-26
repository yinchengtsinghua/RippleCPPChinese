
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

#include <ripple/rpc/Role.h>

namespace ripple {

bool
passwordUnrequiredOrSentCorrect (Port const& port,
                                 Json::Value const& params) {

    assert(! port.admin_ip.empty ());
    bool const passwordRequired = (!port.admin_user.empty() ||
                                   !port.admin_password.empty());

    return !passwordRequired  ||
            ((params["admin_password"].isString() &&
              params["admin_password"].asString() == port.admin_password) &&
             (params["admin_user"].isString() &&
              params["admin_user"].asString() == port.admin_user));
}

bool
ipAllowed (beast::IP::Address const& remoteIp,
           std::vector<beast::IP::Address> const& adminIp)
{
    return std::find_if (adminIp.begin (), adminIp.end (),
        [&remoteIp](beast::IP::Address const& ip) { return ip.is_unspecified () ||
            ip == remoteIp; }) != adminIp.end ();
}

bool
isAdmin (Port const& port, Json::Value const& params,
         beast::IP::Address const& remoteIp)
{
    return ipAllowed (remoteIp, port.admin_ip) &&
        passwordUnrequiredOrSentCorrect (port, params);
}

Role
requestRole (Role const& required, Port const& port,
             Json::Value const& params, beast::IP::Endpoint const& remoteIp,
             std::string const& user)
{
    if (isAdmin(port, params, remoteIp.address()))
        return Role::ADMIN;

    if (required == Role::ADMIN)
        return Role::FORBID;

    if (isIdentified(port, remoteIp.address(), user))
        return Role::IDENTIFIED;

    return Role::GUEST;
}

/*
 *管理和确定的角色应具有无限的资源。
 **/

bool
isUnlimited (Role const& required, Port const& port,
    Json::Value const&params, beast::IP::Endpoint const& remoteIp,
    std::string const& user)
{
    Role role = requestRole(required, port, params, remoteIp, user);

    if (role == Role::ADMIN || role == Role::IDENTIFIED)
        return true;
    else
        return false;
}

bool
isUnlimited (Role const& role)
{
    return role == Role::ADMIN || role == Role::IDENTIFIED;
}

Resource::Consumer
requestInboundEndpoint (Resource::Manager& manager,
    beast::IP::Endpoint const& remoteAddress,
        Port const& port, std::string const& user)
{
    if (isUnlimited (Role::GUEST, port, Json::Value(), remoteAddress, user))
        return manager.newUnlimitedEndpoint (to_string (remoteAddress));

    return manager.newInboundEndpoint(remoteAddress);
}

bool
isIdentified (Port const& port, beast::IP::Address const& remoteIp,
        std::string const& user)
{
    return ! user.empty() && ipAllowed (remoteIp, port.secure_gateway_ip);
}

}
