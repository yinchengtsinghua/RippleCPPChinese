
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

#ifndef RIPPLE_OVERLAY_OVERLAY_H_INCLUDED
#define RIPPLE_OVERLAY_OVERLAY_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/overlay/Peer.h>
#include <ripple/overlay/PeerSet.h>
#include <ripple/server/Handoff.h>
#include <ripple/beast/asio/ssl_bundle.h>
#include <boost/beast/http/message.hpp>
#include <ripple/core/Stoppable.h>
#include <ripple/beast/utility/PropertyStream.h>
#include <memory>
#include <type_traits>
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>

namespace boost { namespace asio { namespace ssl { class context; } } }

namespace ripple {

/*管理连接的对等机集。*/
class Overlay
    : public Stoppable
    , public beast::PropertyStream::Source
{
protected:
//vfalc注意，此构造函数的要求是
//的API出现了不幸的问题
//可停止和属性流
//
    Overlay (Stoppable& parent)
        : Stoppable ("Overlay", parent)
        , beast::PropertyStream::Source ("peers")
    {
    }

public:
    enum class Promote
    {
        automatic,
        never,
        always
    };

    struct Setup
    {
        explicit Setup() = default;

        std::shared_ptr<boost::asio::ssl::context> context;
        bool expire = false;
        beast::IP::Address public_ip;
        int ipLimit = 0;
        std::uint32_t crawlOptions = 0;
    };

    using PeerSequence = std::vector <std::shared_ptr<Peer>>;

    virtual ~Overlay() = default;

    /*有条件地接受传入的HTTP请求。*/
    virtual
    Handoff
    onHandoff (std::unique_ptr <beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
            boost::asio::ip::tcp::endpoint remote_address) = 0;

    /*建立到指定端点的对等连接。
        调用立即返回，连接尝试为
        异步执行。
    **/

    virtual
    void
    connect (beast::IP::Endpoint const& address) = 0;

    /*返回配置为允许的最大对等数。*/
    virtual
    int
    limit () = 0;

    /*返回活动对等数。
        活动对等方仅是那些已完成
        握手和正在使用对等协议。
    **/

    virtual
    std::size_t
    size () = 0;

    /*返回对所有对等机状态的诊断。
        @已弃用这将被PropertyStream取代
    **/

    virtual
    Json::Value
    json () = 0;

    /*返回表示当前对等方列表的序列。
        快照是在调用时生成的。
    **/

    virtual
    PeerSequence
    getActivePeers () = 0;

    /*在每个对等机上调用checksanity函数
        @param index传递给对等方checksanity函数的值
    **/

    virtual
    void
    checkSanity (std::uint32_t index) = 0;

    /*在每个对等机上调用检查函数
    **/

    virtual
    void
    check () = 0;

    /*返回具有匹配的短ID或空的对等机。*/
    virtual
    std::shared_ptr<Peer>
    findPeerByShortID (Peer::id_t const& id) = 0;

    /*广播一个建议。*/
    virtual
    void
    send (protocol::TMProposeSet& m) = 0;

    /*广播验证。*/
    virtual
    void
    send (protocol::TMValidation& m) = 0;

    /*转达建议。*/
    virtual
    void
    relay (protocol::TMProposeSet& m,
        uint256 const& uid) = 0;

    /*中继验证。*/
    virtual
    void
    relay (protocol::TMValidation& m,
        uint256 const& uid) = 0;

    /*访问每个活跃的对等点并返回一个值
        函数必须：
        -可调用为：
            void operator（）（std:：shared_ptr<peer>const&peer）；
         -必须具有以下类型别名：
            使用返回类型=void；
         -可调用为：
            函数：：返回_type operator（）（）const；

        @param f要与每个对等方调用的函数
        @返回f-（）

        @注意函数是按值传递的！
    **/

    template <typename UnaryFunc>
    std::enable_if_t<! std::is_void<
            typename UnaryFunc::return_type>::value,
                typename UnaryFunc::return_type>
    foreach (UnaryFunc f)
    {
        for (auto const& p : getActivePeers())
            f (p);
        return f();
    }

    /*访问每个活跃的同伴
        访客职能部门必须：
         -可调用为：
            void operator（）（std:：shared_ptr<peer>const&peer）；
         -必须具有以下类型别名：
            使用返回类型=void；

        @param f要与每个对等方调用的函数
    **/

    template <class Function>
    std::enable_if_t <
        std::is_void <typename Function::return_type>::value,
        typename Function::return_type
    >
    foreach(Function f)
    {
        for (auto const& p : getActivePeers())
            f (p);
    }

    /*从活动对等机中选择

        为所有活跃的同龄人打分。
        尝试接受得分最高的同龄人，达到要求的次数，
        返回接受的选定对等数。

        分数函数必须：
        -可调用为：
           布尔（PeerImp:：ptr）
        -如果首选对等项，则返回true

        接受功能必须：
        -可调用为：
           布尔（PeerImp:：ptr）
        -如果接受对等方，则返回true

    **/

    virtual
    std::size_t
    selectPeers (PeerSet& set, std::size_t limit, std::function<
        bool(std::shared_ptr<Peer> const&)> score) = 0;

    /*为事务作业队列溢出增加和检索计数器。*/
    virtual void incJqTransOverflow() = 0;
    virtual std::uint64_t getJqTransOverflow() const = 0;

    /*递增和检索计数器，以获取全部对等机断开连接，以及
     *由于过度的资源消耗，我们开始断开连接。
    **/

    virtual void incPeerDisconnect() = 0;
    virtual std::uint64_t getPeerDisconnect() const = 0;
    virtual void incPeerDisconnectCharges() = 0;
    virtual std::uint64_t getPeerDisconnectCharges() const = 0;

    /*返回报告给crawl shard rpc命令的信息。

        @param跳跃爬虫将尝试的最大跳跃次数。
        无法保证实现的跳数。
    **/

    virtual
    Json::Value
    crawlShards(bool pubKey, std::uint32_t hops) = 0;
};

struct ScoreHasLedger
{
    uint256 const& hash_;
    std::uint32_t seq_;
    bool operator()(std::shared_ptr<Peer> const&) const;

    ScoreHasLedger (uint256 const& hash, std::uint32_t seq)
        : hash_ (hash), seq_ (seq)
    {}
};

struct ScoreHasTxSet
{
    uint256 const& hash_;
    bool operator()(std::shared_ptr<Peer> const&) const;

    ScoreHasTxSet (uint256 const& hash) : hash_ (hash)
    {}
};

}

#endif
