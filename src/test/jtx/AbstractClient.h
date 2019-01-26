
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
    版权所有（c）2016 Ripple Labs Inc.

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

#ifndef RIPPLE_TEST_ABSTRACTCLIENT_H_INCLUDED
#define RIPPLE_TEST_ABSTRACTCLIENT_H_INCLUDED

#include <ripple/json/json_value.h>

namespace ripple {
namespace test {

/*抽象Ripple客户端接口。

   这抽象了传输层，允许
   要提交到波纹服务器的命令。
**/

class AbstractClient
{
public:
    virtual ~AbstractClient() = default;
    AbstractClient() = default;
    AbstractClient(AbstractClient const&) = delete;
    AbstractClient& operator=(AbstractClient const&) = delete;

    /*同步提交命令。

        函数的参数和返回的JSON
        是规范化格式，与客户端
        正在通过HTTP/S或WebSocket传输使用JSON-RPC。

        @param cmd要执行的命令
        @param params json：：空值或对象类型的值
                      具有零个或多个键/值对。
        @以规范化格式返回服务器响应。
    **/

    virtual
    Json::Value
    invoke(std::string const& cmd,
        Json::Value const& params = {}) = 0;

///get rpc 1.0或rpc 2.0
    virtual unsigned version() const = 0;
};

} //测试
} //涟漪

#endif
