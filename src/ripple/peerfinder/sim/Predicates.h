
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

#ifndef RIPPLE_PEERFINDER_SIM_PREDICATES_H_INCLUDED
#define RIPPLE_PEERFINDER_SIM_PREDICATES_H_INCLUDED

namespace ripple {
namespace PeerFinder {
namespace Sim {

/*unaryPredicate，如果链接上的“to”节点匹配，则返回“true”。*/
/*@ {*/
template <typename Node>
class is_remote_node_pred
{
public:
    is_remote_node_pred (Node const& node)
        : node (node)
        { }
    template <typename Link>
    bool operator() (Link const& l) const
        { return &node == &l.remote_node(); }
private:
    Node const& node;
};

template <typename Node>
is_remote_node_pred <Node> is_remote_node (Node const& node)
{
    return is_remote_node_pred <Node> (node);
}

template <typename Node>
is_remote_node_pred <Node> is_remote_node (Node const* node)
{
    return is_remote_node_pred <Node> (*node);
}
/*@ }*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*unaryPredicate，如果远程地址匹配，则为“true”。*/
class is_remote_endpoint
{
public:
    explicit is_remote_endpoint (beast::IP::Endpoint const& address)
        : m_endpoint (address)
        { }
    template <typename Link>
    bool operator() (Link const& link) const
    {
        return link.remote_endpoint() == m_endpoint;
    }
private:
    beast::IP::Endpoint const m_endpoint;
};

}
}
}

#endif
