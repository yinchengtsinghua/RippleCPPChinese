
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

#ifndef RIPPLE_NET_RPCCALL_H_INCLUDED
#define RIPPLE_NET_RPCCALL_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/core/Config.h>
#include <ripple/json/json_value.h>
#include <boost/asio/io_service.hpp>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace ripple {

//这是一个受信任的接口，用户需要为其提供有效的输入
//执行有效的请求。错误捕获和报告不是
//命令行界面。
//
//更严格的改进和更好的诊断是受欢迎的。

/*处理Ripple RPC调用。*/
namespace RPCCall {

int fromCommandLine (
    Config const& config,
    const std::vector<std::string>& vCmd,
    Logs& logs);

void fromNetwork (
    boost::asio::io_service& io_service,
    std::string const& strIp, const std::uint16_t iPort,
    std::string const& strUsername, std::string const& strPassword,
    std::string const& strPath, std::string const& strMethod,
    Json::Value const& jvParams, const bool bSSL, bool quiet,
    Logs& logs,
    std::function<void (Json::Value const& jvInput)> callbackFuncP = std::function<void (Json::Value const& jvInput)> ());
}

/*给定一个波纹命令行，返回相应的JSON。
**/

Json::Value
cmdLineToJSONRPC (std::vector<std::string> const& args, beast::Journal j);

/*内部调用RPC客户端。
**/

std::pair<int, Json::Value>
rpcClient(std::vector<std::string> const& args,
    Config const& config, Logs& logs);

} //涟漪

#endif
