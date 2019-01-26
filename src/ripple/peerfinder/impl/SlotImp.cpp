
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

#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {

SlotImp::SlotImp (beast::IP::Endpoint const& local_endpoint,
    beast::IP::Endpoint const& remote_endpoint, bool fixed,
        clock_type& clock)
    : recent (clock)
    , m_inbound (true)
    , m_fixed (fixed)
    , m_cluster (false)
    , m_state (accept)
    , m_remote_endpoint (remote_endpoint)
    , m_local_endpoint (local_endpoint)
    , m_listening_port (unknownPort)
    , checked (false)
    , canAccept (false)
    , connectivityCheckInProgress (false)
{
}

SlotImp::SlotImp (beast::IP::Endpoint const& remote_endpoint,
    bool fixed, clock_type& clock)
    : recent (clock)
    , m_inbound (false)
    , m_fixed (fixed)
    , m_cluster (false)
    , m_state (connect)
    , m_remote_endpoint (remote_endpoint)
    , m_listening_port (unknownPort)
    , checked (true)
    , canAccept (true)
    , connectivityCheckInProgress (false)
{
}

void
SlotImp::state (State state_)
{
//必须通过activate（）设置活动状态
    assert (state_ != active);

//状态必须不同
    assert (state_ != m_state);

//你不能转换到初始状态
    assert (state_ != accept && state_ != connect);

//只能从出站连接状态连接
    assert (state_ != connected || (! m_inbound && m_state == connect));

//无法正常关闭出站连接尝试
    assert (state_ != closing || m_state != connect);

    m_state = state_;
}

void
SlotImp::activate (clock_type::time_point const& now)
{
//只能从接受或连接状态变为活动状态
    assert (m_state == accept || m_state == connected);

    m_state = active;
    whenAcceptEndpoints = now;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Slot::~Slot() = default;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

SlotImp::recent_t::recent_t (clock_type& clock)
    : cache (clock)
{
}

void
SlotImp::recent_t::insert (beast::IP::Endpoint const& ep, int hops)
{
    auto const result (cache.emplace (ep, hops));
    if (! result.second)
    {
//注：其他逻辑依赖于这个<=不等式。
        if (hops <= result.first->second)
        {
            result.first->second = hops;
            cache.touch (result.first);
        }
    }
}

bool
SlotImp::recent_t::filter (beast::IP::Endpoint const& ep, int hops)
{
    auto const iter (cache.find (ep));
    if (iter == cache.end())
        return false;
//如果我们听到了，就避免发送端点。
//从他们最近在相同或更低的跳数。
//注：其他逻辑依赖于这个<=不等式。
    return iter->second <= hops;
}

void
SlotImp::recent_t::expire ()
{
    beast::expire (cache,
        Tuning::liveCacheSecondsToLive);
}

}
}
