
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
    版权所有（c）2012-2017 Ripple Labs Inc.

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

#ifndef RIPPLE_TEST_JTX_ENVCONFIG_H_INCLUDED
#define RIPPLE_TEST_JTX_ENVCONFIG_H_INCLUDED

#include <ripple/core/Config.h>

namespace ripple {
namespace test {

extern std::atomic<bool> envUseIPv4;

inline
const char *
getEnvLocalhostAddr()
{
    return envUseIPv4 ? "127.0.0.1" : "::1";
}

///@brief初始化配置对象以与jtx:：env一起使用
///
///@param配置要初始化的配置对象
extern
void
setupConfigForUnitTests (Config& config);

namespace jtx {

///@brief创建并初始化默认值
///jtx的配置：：env
///
///@将唯一指针返回到配置实例
inline
std::unique_ptr<Config>
envconfig()
{
    auto p = std::make_unique<Config>();
    setupConfigForUnitTests(*p);
    return p;
}

///@brief创建并初始化jtx:：env和的默认配置
///使用配置对象调用提供的函数/lambda。
///
///@param modfunc可调用函数或lambda以修改默认配置。
///函数的第一个参数必须是std：：unique_ptr to
///Ripple：：配置。该函数拥有唯一指针的所有权，并且
///通过返回唯一的指针放弃所有权。
///
///@param参数将传递给
///config修饰符函数（可选）
///
///@将唯一指针返回到配置实例
template <class F, class... Args>
std::unique_ptr<Config>
envconfig(F&& modfunc, Args&&... args)
{
    return modfunc(envconfig(), std::forward<Args>(args)...);
}

///@brief adjust config以便不启用管理端口
///
///this用于envconfig，如
///envconfig（无管理员）
///
///@param cfg要修改的配置实例
///
///@将唯一指针返回到配置实例
std::unique_ptr<Config>
no_admin(std::unique_ptr<Config>);

///@brief adjust configuration with params needed to be a validator（需要参数作为验证器）
///
///this用于envconfig，如
///envconfig（验证程序，myseed）
///
///@param cfg要修改的配置实例
///@param种子字符串，用于生成密钥。固定违约
//如果此字符串为空，将使用/value
///
///@将唯一指针返回到配置实例
std::unique_ptr<Config>
validator(std::unique_ptr<Config>, std::string const&);

///@brief按指定值调整默认配置的服务器端口
///
///this用于envconfig，如
///envconfig（端口增量，5）
///
///@param cfg要修改的配置实例
///@param int增加现有服务器端口的数量
///配置中的值
///
///@将唯一指针返回到配置实例
std::unique_ptr<Config>
port_increment(std::unique_ptr<Config>, int);

} //JTX
} //测试
} //涟漪

#endif

