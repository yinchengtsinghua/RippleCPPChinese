
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

#ifndef RIPPLE_PEERFINDER_MANAGER_H_INCLUDED
#define RIPPLE_PEERFINDER_MANAGER_H_INCLUDED

#include <ripple/peerfinder/Slot.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <boost/asio/ip/tcp.hpp>

namespace ripple {
namespace PeerFinder {

using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

/*表示一组地址。*/
using IPAddresses = std::vector <beast::IP::Endpoint>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*Peerfinder配置设置。*/
struct Config
{
    /*允许的最大公用对等机插槽数。
        这包括入站和出站，但不包括
        固定对等体
    **/

    int maxPeers;

    /*要维护的自动出站连接数。
        只有在自动连接时才维护出站连接
        是真的。该值可以是小数；决定将其取整
        或者使用每个进程的伪随机数进行下降，并且
        与分数部分成比例的概率。
        例子：
            如果outpeers为9.3，那么30%的节点将保持9个出站
            连接，而70%的节点将保持10个出站
            连接。
    **/

    double outPeers;

    /*如果我们希望我们的IP地址保密，则为“真”。*/
    bool peerPrivate = true;

    /*`true`如果我们要接受传入连接。*/
    bool wantIncoming;

    /*`true`如果我们想自动建立连接*/
    bool autoConnect;

    /*侦听端口号。*/
    std::uint16_t listeningPort;

    /*我们宣传的一组功能。*/
    std::string features;

    /*限制每个IP允许的传入连接数*/
    int ipLimit;

//——————————————————————————————————————————————————————————————

    /*使用默认值创建配置。*/
    Config ();

    /*根据规则为输出者返回适当的值。*/
    double calcOutPeers () const;

    /*调整值，使其符合业务规则。*/
    void applyTuning ();

    /*将配置写入属性流*/
    void onWrite (beast::PropertyStream::Map& map);
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*描述可连接的对等地址和一些元数据。*/
struct Endpoint
{
    Endpoint ();

    Endpoint (beast::IP::Endpoint const& ep, int hops_);

    int hops;
    beast::IP::Endpoint address;
};

bool operator< (Endpoint const& lhs, Endpoint const& rhs);

/*用于连接的一组端点。*/
using Endpoints = std::vector <Endpoint>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*激活插槽的可能结果。*/
enum class Result
{
    duplicate,
    full,
    success
};

/*维护一组用于进入网络的IP地址。*/
class Manager
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
    explicit Manager (Stoppable& parent);

public:
    /*摧毁物体。
        任何挂起的源提取操作都将中止。
        可能在
        析构函数返回。
    **/

    virtual ~Manager() = default;

    /*设置管理器的配置。
        新设置将异步应用。
        线程安全：
            可以随时从任何线程调用。
    **/

    virtual void setConfig (Config const& config) = 0;

    /*返回管理器的配置。*/
    virtual
    Config
    config() = 0;

    /*添加应始终连接的对等机。
        这对于维护一个私有的对等集群很有用。
        字符串是配置中指定的名称。
        文件，以及相应的IP地址集。
    **/

    virtual void addFixedPeer (std::string const& name,
        std::vector <beast::IP::Endpoint> const& addresses) = 0;

    /*添加一组字符串作为回退IP:：Endpoint源。
        @param name用于诊断的标签。
    **/

    virtual void addFallbackStrings (std::string const& name,
        std::vector <std::string> const& strings) = 0;

    /*添加一个URL作为回退位置以获取IP:：Endpoint源。
        @param name用于诊断的标签。
    **/

    /*vfalc注释未实现
    虚拟void addfallbackurl（std:：string const&name，
        std:：string const&url）=0；
    **/


//——————————————————————————————————————————————————————————————

    /*使用指定的远程端点创建新的入站插槽。
        如果返回nullptr，则无法分配插槽。
        通常这是因为检测到了自连接。
    **/

    virtual Slot::ptr new_inbound_slot (
        beast::IP::Endpoint const& local_endpoint,
            beast::IP::Endpoint const& remote_endpoint) = 0;

    /*使用指定的远程端点创建新的出站插槽。
        如果返回nullptr，则无法分配插槽。
        通常这是由于重复的连接。
    **/

    virtual Slot::ptr new_outbound_slot (
        beast::IP::Endpoint const& remote_endpoint) = 0;

    /*在收到mtendpoints时调用。*/
    virtual void on_endpoints (Slot::ptr const& slot,
        Endpoints const& endpoints) = 0;

    /*当槽关闭时调用。
        这总是在套接字关闭时发生，除非套接字
        被取消了。
    **/

    virtual void on_closed (Slot::ptr const& slot) = 0;

    /*当出站连接被视为失败时调用*/
    virtual void on_failure (Slot::ptr const& slot) = 0;

    /*当我们从繁忙的对等端收到重定向IP时调用。*/
    virtual
    void
    onRedirects (boost::asio::ip::tcp::endpoint const& remote_address,
        std::vector<boost::asio::ip::tcp::endpoint> const& eps) = 0;

//——————————————————————————————————————————————————————————————

    /*当出站连接尝试成功时调用。
        本地终结点必须有效。如果呼叫者收到错误
        从套接字检索本地端点时，它应该
        继续，就像通过调用“关闭”而失败的连接尝试一样
        而不是接通。
        @return`true`如果要保持连接
    **/

    virtual
    bool
    onConnected (Slot::ptr const& slot,
        beast::IP::Endpoint const& local_endpoint) = 0;

    /*请求活动插槽类型。*/
    virtual
    Result
    activate (Slot::ptr const& slot,
        PublicKey const& key, bool cluster) = 0;

    /*返回一组适合重定向的终结点。*/
    virtual
    std::vector <Endpoint>
    redirect (Slot::ptr const& slot) = 0;

    /*返回一组我们应该连接的地址。*/
    virtual
    std::vector <beast::IP::Endpoint>
    autoconnect() = 0;

    virtual
    std::vector<std::pair<Slot::ptr, std::vector<Endpoint>>>
    buildEndpointsForPeers() = 0;

    /*执行定期活动。
        这应该每秒调用一次。
    **/

    virtual
    void
    once_per_second() = 0;
};

}
}

#endif
