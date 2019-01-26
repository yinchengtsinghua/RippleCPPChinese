
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

#include <test/jtx/envconfig.h>
#include <test/jtx/Env.h>
#include <ripple/core/ConfigSections.h>

namespace ripple {
namespace test {

int port_base = 8000;
void incPorts()
{
    port_base += 3;
}

std::atomic<bool> envUseIPv4 {false};

void
setupConfigForUnitTests (Config& cfg)
{
    std::string const port_peer = to_string(port_base);
    std::string port_rpc = to_string(port_base + 1);
    std::string port_ws = to_string(port_base + 2);

    cfg.overwrite (ConfigSection::nodeDatabase (), "type", "memory");
    cfg.overwrite (ConfigSection::nodeDatabase (), "path", "main");
    cfg.deprecatedClearSection (ConfigSection::importNodeDatabase ());
    cfg.legacy("database_path", "");
    cfg.setupControl(true, true, true);
    cfg["server"].append("port_peer");
    cfg["port_peer"].set("ip", getEnvLocalhostAddr());
    cfg["port_peer"].set("port", port_peer);
    cfg["port_peer"].set("protocol", "peer");

    cfg["server"].append("port_rpc");
    cfg["port_rpc"].set("ip", getEnvLocalhostAddr());
    cfg["port_rpc"].set("admin", getEnvLocalhostAddr());
    cfg["port_rpc"].set("port", port_rpc);
    cfg["port_rpc"].set("protocol", "http,ws2");

    cfg["server"].append("port_ws");
    cfg["port_ws"].set("ip", getEnvLocalhostAddr());
    cfg["port_ws"].set("admin", getEnvLocalhostAddr());
    cfg["port_ws"].set("port", port_ws);
    cfg["port_ws"].set("protocol", "ws");
}

namespace jtx {

std::unique_ptr<Config>
no_admin(std::unique_ptr<Config> cfg)
{
    (*cfg)["port_rpc"].set("admin","");
    (*cfg)["port_ws"].set("admin","");
    return cfg;
}

auto constexpr defaultseed = "shUwVw52ofnCUX5m7kPTKzJdr4HEH";

std::unique_ptr<Config>
validator(std::unique_ptr<Config> cfg, std::string const& seed)
{
//如果配置具有有效的验证密钥，那么我们将作为验证程序运行。
    cfg->section(SECTION_VALIDATION_SEED).append(
        std::vector<std::string>{seed.empty() ? defaultseed : seed});
    return cfg;
}

std::unique_ptr<Config>
port_increment(std::unique_ptr<Config> cfg, int increment)
{
    for (auto const sectionName : {"port_peer", "port_rpc", "port_ws"})
    {
        Section& s = (*cfg)[sectionName];
        auto const port = s.get<std::int32_t>("port");
        if (port)
        {
            s.set ("port", std::to_string(*port + increment));
        }
    }
    return cfg;
}

} //JTX
} //测试
} //涟漪
