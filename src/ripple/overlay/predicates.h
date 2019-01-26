
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

#ifndef RIPPLE_OVERLAY_PREDICATES_H_INCLUDED
#define RIPPLE_OVERLAY_PREDICATES_H_INCLUDED

#include <ripple/overlay/Message.h>
#include <ripple/overlay/Peer.h>

#include <set>

namespace ripple {

/*向所有对等方发送消息*/
struct send_always
{
    using return_type = void;

    Message::pointer const& msg;

    send_always(Message::pointer const& m)
        : msg(m)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        peer->send (msg);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*发送消息以匹配对等方*/
template <typename Predicate>
struct send_if_pred
{
    using return_type = void;

    Message::pointer const& msg;
    Predicate const& predicate;

    send_if_pred(Message::pointer const& m, Predicate const& p)
    : msg(m), predicate(p)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        if (predicate (peer))
            peer->send (msg);
    }
};

/*帮助类型推导的帮助函数*/
template <typename Predicate>
send_if_pred<Predicate> send_if (
    Message::pointer const& m,
        Predicate const &f)
{
    return send_if_pred<Predicate>(m, f);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*向不匹配的对等发送消息*/
template <typename Predicate>
struct send_if_not_pred
{
    using return_type = void;

    Message::pointer const& msg;
    Predicate const& predicate;

    send_if_not_pred(Message::pointer const& m, Predicate const& p)
        : msg(m), predicate(p)
    { }

    void operator()(std::shared_ptr<Peer> const& peer) const
    {
        if (!predicate (peer))
            peer->send (msg);
    }
};

/*帮助类型推导的帮助函数*/
template <typename Predicate>
send_if_not_pred<Predicate> send_if_not (
    Message::pointer const& m,
        Predicate const &f)
{
    return send_if_not_pred<Predicate>(m, f);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*选择特定的对等*/
struct match_peer
{
    Peer const* matchPeer;

    match_peer (Peer const* match = nullptr)
        : matchPeer (match)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if(matchPeer && (peer.get () == matchPeer))
            return true;

        return false;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*选择群集中的所有对等端（可选除外）*/
struct peer_in_cluster
{
    match_peer skipPeer;

    peer_in_cluster (Peer const* skip = nullptr)
        : skipPeer (skip)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if (skipPeer (peer))
            return false;

        if (! peer->cluster())
            return false;

        return true;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*选择指定集中的所有对等机*/
struct peer_in_set
{
    std::set <Peer::id_t> const& peerSet;

    peer_in_set (std::set<Peer::id_t> const& peers)
        : peerSet (peers)
    { }

    bool operator() (std::shared_ptr<Peer> const& peer) const
    {
        if (peerSet.count (peer->id ()) == 0)
            return false;

        return true;
    }
};

}

#endif
