
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

#ifndef RIPPLE_OVERLAY_CLUSTER_H_INCLUDED
#define RIPPLE_OVERLAY_CLUSTER_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/BasicConfig.h>
#include <ripple/overlay/ClusterNode.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/utility/Journal.h>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <type_traits>

namespace ripple {

class Cluster
{
private:
    struct Comparator
    {
        explicit Comparator() = default;

        using is_transparent = std::true_type;

        bool
        operator() (
            ClusterNode const& lhs,
            ClusterNode const& rhs) const
        {
            return lhs.identity() < rhs.identity();
        }

        bool
        operator() (
            ClusterNode const& lhs,
            PublicKey const& rhs) const
        {
            return lhs.identity() < rhs;
        }

        bool
        operator() (
            PublicKey const& lhs,
            ClusterNode const& rhs) const
        {
            return lhs < rhs.identity();
        }
    };

    std::set<ClusterNode, Comparator> nodes_;
    std::mutex mutable mutex_;
    beast::Journal mutable j_;

public:
    Cluster (beast::Journal j);

    /*确定节点是否属于群集中
        @return boost：：如果节点不是成员，则为无，
                否则，与
                节点（可以是空字符串）。
    **/

    boost::optional<std::string>
    member (PublicKey const& node) const;

    /*群集列表中的节点数。*/
    std::size_t
    size() const;

    /*存储有关群集节点状态的信息。
        @param identity节点的公共标识
        @param name节点的名称（可能为空）
        @如果我们更新了我们的信息，返回true
    **/

    bool
    update (
        PublicKey const& identity,
        std::string name,
        std::uint32_t loadFee = 0,
        NetClock::time_point reportTime = NetClock::time_point{});

    /*对每个群集节点调用一次回调。
        @注意，不允许您从以下位置调用“update”
              在回调中。
    **/

    void
    for_each (
        std::function<void(ClusterNode const&)> func) const;

    /*加载群集节点列表。

        该节包含由base58组成的条目
        编码的节点公钥，可选后跟
        评论

        @如果一个条目无法分析或
                包含无效的节点公钥，
                反之亦然。
    **/

    bool
    load (Section const& nodes);
};

} //涟漪

#endif
