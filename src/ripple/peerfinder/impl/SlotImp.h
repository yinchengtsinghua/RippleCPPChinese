
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

#ifndef RIPPLE_PEERFINDER_SLOTIMP_H_INCLUDED
#define RIPPLE_PEERFINDER_SLOTIMP_H_INCLUDED

#include <ripple/peerfinder/Slot.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/beast/container/aged_unordered_map.h>
#include <ripple/beast/container/aged_container_utility.h>
#include <boost/optional.hpp>
#include <atomic>

namespace ripple {
namespace PeerFinder {

class SlotImp : public Slot
{
private:
    using recent_type = beast::aged_unordered_map <beast::IP::Endpoint, int>;

public:
    using ptr = std::shared_ptr <SlotImp>;

//入站
    SlotImp (beast::IP::Endpoint const& local_endpoint,
        beast::IP::Endpoint const& remote_endpoint, bool fixed,
            clock_type& clock);

//出站
    SlotImp (beast::IP::Endpoint const& remote_endpoint,
        bool fixed, clock_type& clock);

    bool inbound () const override
    {
        return m_inbound;
    }

    bool fixed () const override
    {
        return m_fixed;
    }

    bool cluster () const override
    {
        return m_cluster;
    }

    State state () const override
    {
        return m_state;
    }

    beast::IP::Endpoint const& remote_endpoint () const override
    {
        return m_remote_endpoint;
    }

    boost::optional <beast::IP::Endpoint> const& local_endpoint () const override
    {
        return m_local_endpoint;
    }

    boost::optional <PublicKey> const& public_key () const override
    {
        return m_public_key;
    }

    boost::optional<std::uint16_t> listening_port () const override
    {
        std::uint32_t const value = m_listening_port;
        if (value == unknownPort)
            return boost::none;
        return value;
    }

    void set_listening_port (std::uint16_t port)
    {
        m_listening_port = port;
    }

    void local_endpoint (beast::IP::Endpoint const& endpoint)
    {
        m_local_endpoint = endpoint;
    }

    void remote_endpoint (beast::IP::Endpoint const& endpoint)
    {
        m_remote_endpoint = endpoint;
    }

    void public_key (PublicKey const& key)
    {
        m_public_key = key;
    }

    void cluster (bool cluster_)
    {
        m_cluster = cluster_;
    }

//——————————————————————————————————————————————————————————————

    void state (State state_);

    void activate (clock_type::time_point const& now);

//“成员空间”
//
//我们从该对等端看到的所有最近的地址集。
//我们尽量避免把他们给我们的地址发送给同龄人。
//
    class recent_t
    {
    public:
        explicit recent_t (clock_type& clock);

        /*为接收到的插槽的每个有效终结点调用。
            我们还插入发送到插槽的消息以防止
            发送同一地址的插槽太频繁。
        **/

        void insert (beast::IP::Endpoint const& ep, int hops);

        /*如果不应将端点发送到槽，则返回“true”。*/
        bool filter (beast::IP::Endpoint const& ep, int hops);

    private:
        void expire ();

        friend class SlotImp;
        recent_type cache;
    } recent;

    void expire()
    {
        recent.expire();
    }

private:
    bool const m_inbound;
    bool const m_fixed;
    bool m_cluster;
    State m_state;
    beast::IP::Endpoint m_remote_endpoint;
    boost::optional <beast::IP::Endpoint> m_local_endpoint;
    boost::optional <PublicKey> m_public_key;

    static std::int32_t constexpr unknownPort = -1;
    std::atomic <std::int32_t> m_listening_port;

public:
//已弃用的公共数据成员

//告诉我们是否检查了连接。出站连接
//因为我们成功连接，所以总是被认为是检查过的。
    bool checked;

//设置为指示连接是否可以在
//在mtendpoints中公布的地址。仅当选中为真时有效。
    bool canAccept;

//设置为指示此对等机的连接检查在
//进展。永远有效。
    bool connectivityCheckInProgress;

//从对等端接收mtendpoints的时间
//这是为了防止洪水或垃圾。接收MTEndpoints
//在规定的时间之前，应收取装载费。
//
    clock_type::time_point whenAcceptEndpoints;
};

}
}

#endif
