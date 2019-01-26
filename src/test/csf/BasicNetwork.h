
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

#ifndef RIPPLE_TEST_CSF_BASICNETWORK_H_INCLUDED
#define RIPPLE_TEST_CSF_BASICNETWORK_H_INCLUDED

#include <test/csf/Scheduler.h>
#include <test/csf/Digraph.h>

namespace ripple {
namespace test {
namespace csf {
/*点对点网络模拟器。

    网络由一组表示
    表示边的顶点和可配置连接。
    调用者负责在前面创建对等对象
    时间。

    一旦basicnetwork
    构建。要处理在线和离线的同龄人，
    呼叫者只需断开所有链接并重新连接即可。
    后来。连接是定向的，一端是入站
    另一个是出站对等机。

    对等端可以通过其连接发送消息。模拟
    延迟的影响，这些消息可以被
    建立链接时设置的可配置持续时间。
    邮件总是按发送顺序到达
    特殊连接。

    消息是使用lambda函数建模的。呼叫者
    提供传递消息时要执行的代码。
    如果对等端断开连接，则所有等待传递的邮件
    在连接的任何一端都不会被传递。

    创建对等集时，调用方需要提供
    用于管理时间和传递的计划程序对象
    消息的。网络建设后，建立
    连接，调用方使用调度程序的步骤函数
    通过网络传输消息。

    对等点和连接图在内部表示。
    使用digraph<peer，basicnetwork:：link_type>。客户有
    常量访问该图以执行其他操作而不是
    由basicnetwork直接提供。

    同行要求：

        对等机应该是轻量级的类型，复制成本低
        和/或移动。一个好的候选人就是一个简单的指向
        模拟中的基础用户定义类型。

        表达式类型要求
        ——————————————————
        P对等
        P型的U、V值
        P U（V）可复制构造
        u.~p（）可销毁
        u==v布尔等同性可比
        u<v布尔不可比
        std:：hash<p>类std:：hash是为p定义的
        ！如果你不是同龄人，你就错了。

**/

template <class Peer>
class BasicNetwork
{
    using peer_type = Peer;

    using clock_type = Scheduler::clock_type;

    using duration = typename clock_type::duration;

    using time_point = typename clock_type::time_point;

    struct link_type
    {
        bool inbound = false;
        duration delay{};
        time_point established{};
        link_type() = default;
        link_type(bool inbound_, duration delay_, time_point established_)
            : inbound(inbound_), delay(delay_), established(established_)
        {
        }
    };

    Scheduler& scheduler;
    Digraph<Peer, link_type> links_;

public:
    BasicNetwork(BasicNetwork const&) = delete;
    BasicNetwork&
    operator=(BasicNetwork const&) = delete;

    BasicNetwork(Scheduler& s);

    /*连接两个对等机。

        链接是定向的，其中'from'正在建立
        出站连接和“to”接收
        传入连接。

        先决条件：

            从！=to（不允许自连接）。

            从到之间的链接不
            已经存在（不允许重复）。

        影响：

            在“从”和“到”之间创建链接。

        @param`from`传出连接的源
        @param`to`传入连接的收件人
        @param`delay`所有已传递消息的时间延迟
        @return`true`如果建立了新的连接
    **/

    bool
    connect(
        Peer const& from,
        Peer const& to,
        duration const& delay = std::chrono::seconds{0});

    /*断开一个链接。

        影响：

            如果存在连接，则两端
            断开的。

            连接上的任何挂起消息
            被丢弃。

        @如果连接断开，返回'true'。
    **/

    bool
    disconnect(Peer const& peer1, Peer const& peer2);

    /*向对等端发送消息。

        先决条件：

            从到之间存在链接。

        影响：

            如果链接没有断开，当
            链接的“延迟”时间已过，
            将用调用函数
            没有参数。

        @请注意，来电者有责任
        确保函数体执行
        与“发件人”收到的
        从“到”的消息。
    **/

    template <class Function>
    void
    send(Peer const& from, Peer const& to, Function&& f);


    /*返回活动链接的范围。

        @返回有向图：：边缘实例的随机访问范围
    **/

    auto
    links(Peer const& from)
    {
        return links_.outEdges(from);
    }

    /*返回基础有向图
    **/

    Digraph<Peer, link_type> const &
    graph() const
    {
        return links_;
    }
};
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
template <class Peer>
BasicNetwork<Peer>::BasicNetwork(Scheduler& s) : scheduler(s)
{
}

template <class Peer>
inline bool
BasicNetwork<Peer>::connect(
    Peer const& from,
    Peer const& to,
    duration const& delay)
{
    if (to == from)
        return false;
    time_point const now = scheduler.now();
    if(!links_.connect(from, to, link_type{false, delay, now}))
        return false;
    auto const result = links_.connect(to, from, link_type{true, delay, now});
    (void)result;
    assert(result);
    return true;
}

template <class Peer>
inline bool
BasicNetwork<Peer>::disconnect(Peer const& peer1, Peer const& peer2)
{
    if (! links_.disconnect(peer1, peer2))
        return false;
    bool r = links_.disconnect(peer2, peer1);
    (void)r;
    assert(r);
    return true;
}

template <class Peer>
template <class Function>
inline void
BasicNetwork<Peer>::send(Peer const& from, Peer const& to, Function&& f)
{
    auto link = links_.edge(from,to);
    if(!link)
        return;
    time_point const sent = scheduler.now();
    scheduler.in(
        link->delay,
        [ from, to, sent, f = std::forward<Function>(f), this ] {
//仅当仍然连接且连接为
//邮件发送后未损坏
            auto link = links_.edge(from, to);
            if (link && link->established <= sent)
                f();
        });
}

}  //命名空间CSF
}  //命名空间测试
}  //命名空间波纹

#endif
