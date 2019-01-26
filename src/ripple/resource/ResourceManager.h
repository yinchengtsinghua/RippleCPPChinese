
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

#ifndef RIPPLE_RESOURCE_MANAGER_H_INCLUDED
#define RIPPLE_RESOURCE_MANAGER_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/resource/Consumer.h>
#include <ripple/resource/Gossip.h>
#include <ripple/beast/insight/Collector.h>
#include <ripple/beast/net/IPEndpoint.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/beast/utility/PropertyStream.h>

namespace ripple {
namespace Resource {

/*跟踪负载和资源消耗。*/
class Manager : public beast::PropertyStream::Source
{
protected:
    Manager ();

public:
    virtual ~Manager() = 0;

    /*创建一个由入站IP地址键控的新终结点。*/
    virtual Consumer newInboundEndpoint (beast::IP::Endpoint const& address) = 0;

    /*创建一个由出站IP地址和端口键控的新端点。*/
    virtual Consumer newOutboundEndpoint (beast::IP::Endpoint const& address) = 0;

    /*创建一个按名称键控的新端点。*/
    virtual Consumer newUnlimitedEndpoint (std::string const& name) = 0;

    /*提取打包的消费者信息以供出口。*/
    virtual Gossip exportConsumers () = 0;

    /*提取消费者信息进行报告。*/
    virtual Json::Value getJson () = 0;
    virtual Json::Value getJson (int threshold) = 0;

    /*导入打包的消费者信息。
        @param origin唯一标记原点的标识符。
    **/

    virtual void importConsumers (std::string const& origin, Gossip const& gossip) = 0;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr <Manager> make_Manager (
    beast::insight::Collector::ptr const& collector,
        beast::Journal journal);

}
}

#endif
