
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

#ifndef RIPPLE_PEERFINDER_HANDOUTS_H_INCLUDED
#define RIPPLE_PEERFINDER_HANDOUTS_H_INCLUDED

#include <ripple/peerfinder/impl/SlotImp.h>
#include <ripple/peerfinder/impl/Tuning.h>
#include <ripple/beast/container/aged_set.h>
#include <cassert>
#include <iterator>
#include <type_traits>

namespace ripple {
namespace PeerFinder {

namespace detail {

/*尝试在目标中插入一个对象。
    当一个物品被分发时，它被移动到容器的末端。
    @返回插入的对象数
**/

//vfalc todo处理sequencecontainer的std:：list的专门化
//使用拼接优化擦除/推回
//
template <class Target, class HopContainer>
std::size_t
handout_one (Target& t, HopContainer& h)
{
    assert (! t.full());
    for (auto it = h.begin(); it != h.end(); ++it)
    {
        auto const& e = *it;
        if (t.try_insert (e))
        {
            h.move_back (it);
            return 1;
        }
    }
    return 0;
}

}

/*根据业务规则将对象分发到目标。
    尽最大努力按顺序均匀分布项目
    容器列表进入目标序列列表。
**/

template <class TargetFwdIter, class SeqFwdIter>
void
handout (TargetFwdIter first, TargetFwdIter last,
    SeqFwdIter seq_first, SeqFwdIter seq_last)
{
    for (;;)
    {
        std::size_t n (0);
        for (auto si = seq_first; si != seq_last; ++si)
        {
            auto c = *si;
            bool all_full (true);
            for (auto ti = first; ti != last; ++ti)
            {
                auto& t = *ti;
                if (! t.full())
                {
                    n += detail::handout_one (t, c);
                    all_full = false;
                }
            }
            if (all_full)
                return;
        }
        if (! n)
            break;
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*接收用于重定向连接的讲义。
    当我们的插槽已满时，将重定向传入的连接请求。
**/

class RedirectHandouts
{
public:
    template <class = void>
    explicit
    RedirectHandouts (SlotImp::ptr const& slot);

    template <class = void>
    bool try_insert (Endpoint const& ep);

    bool full () const
    {
        return list_.size() >= Tuning::redirectEndpointCount;
    }

    SlotImp::ptr const& slot () const
    {
        return slot_;
    }

    std::vector <Endpoint>& list()
    {
        return list_;
    }

    std::vector <Endpoint> const& list() const
    {
        return list_;
    }

private:
    SlotImp::ptr slot_;
    std::vector <Endpoint> list_;
};

template <class>
RedirectHandouts::RedirectHandouts (SlotImp::ptr const& slot)
    : slot_ (slot)
{
    list_.reserve (Tuning::redirectEndpointCount);
}

template <class>
bool
RedirectHandouts::try_insert (Endpoint const& ep)
{
    if (full ())
        return false;

//vfalc注：当我们提供
//对等HTTP握手中的地址，而不是
//tmendpoints消息。
//
    if (ep.hops > Tuning::maxHops)
        return false;

//别把我们的地址寄给他们
    if (ep.hops == 0)
        return false;

//不要给他们自己的地址
    if (slot_->remote_endpoint().address() ==
        ep.address.address())
        return false;

//确保地址不在我们的列表中
    if (std::any_of (list_.begin(), list_.end(),
        [&ep](Endpoint const& other)
        {
//出于安全原因忽略端口
            return other.address.address() == ep.address.address();
        }))
    {
        return false;
    }

    list_.emplace_back (ep.address, ep.hops);

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*在定期讲义期间接收时段的终结点。*/
class SlotHandouts
{
public:
    template <class = void>
    explicit
    SlotHandouts (SlotImp::ptr const& slot);

    template <class = void>
    bool try_insert (Endpoint const& ep);

    bool full () const
    {
        return list_.size() >= Tuning::numberOfEndpoints;
    }

    void insert (Endpoint const& ep)
    {
        list_.push_back (ep);
    }

    SlotImp::ptr const& slot () const
    {
        return slot_;
    }

    std::vector <Endpoint> const& list() const
    {
        return list_;
    }

private:
    SlotImp::ptr slot_;
    std::vector <Endpoint> list_;
};

template <class>
SlotHandouts::SlotHandouts (SlotImp::ptr const& slot)
    : slot_ (slot)
{
    list_.reserve (Tuning::numberOfEndpoints);
}

template <class>
bool
SlotHandouts::try_insert (Endpoint const& ep)
{
    if (full ())
        return false;

    if (ep.hops > Tuning::maxHops)
        return false;

    if (slot_->recent.filter (ep.address, ep.hops))
        return false;

//不要给他们自己的地址
    if (slot_->remote_endpoint().address() ==
        ep.address.address())
        return false;

//确保地址不在我们的列表中
    if (std::any_of (list_.begin(), list_.end(),
        [&ep](Endpoint const& other)
        {
//出于安全原因忽略端口
            return other.address.address() == ep.address.address();
        }))
        return false;

    list_.emplace_back (ep.address, ep.hops);

//插入此插槽的“最近”表。尽管终点
//不是从吃角子老虎机里出来的，把它加到吃角子老虎机的桌子上
//阻止我们再次发送它，直到它从
//另一端的缓存。
//
    slot_->recent.insert (ep.address, ep.hops);

    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*接收用于建立自动连接的讲义。*/
class ConnectHandouts
{
public:
//跟踪已建立传出连接的地址
//为了不经常联系他们。
    using Squelches = beast::aged_set <beast::IP::Address>;

    using list_type = std::vector <beast::IP::Endpoint>;

private:
    std::size_t m_needed;
    Squelches& m_squelches;
    list_type m_list;

public:
    template <class = void>
    ConnectHandouts (std::size_t needed, Squelches& squelches);

    template <class = void>
    bool try_insert (beast::IP::Endpoint const& endpoint);

    bool empty() const
    {
        return m_list.empty();
    }

    bool full() const
    {
        return m_list.size() >= m_needed;
    }

    bool try_insert (Endpoint const& endpoint)
    {
        return try_insert (endpoint.address);
    }

    list_type& list()
    {
        return m_list;
    }

    list_type const& list() const
    {
        return m_list;
    }
};

template <class>
ConnectHandouts::ConnectHandouts (
    std::size_t needed, Squelches& squelches)
    : m_needed (needed)
    , m_squelches (squelches)
{
    m_list.reserve (needed);
}

template <class>
bool
ConnectHandouts::try_insert (beast::IP::Endpoint const& endpoint)
{
    if (full ())
        return false;

//确保地址不在我们的列表中
    if (std::any_of (m_list.begin(), m_list.end(),
        [&endpoint](beast::IP::Endpoint const& other)
        {
//出于安全原因忽略端口
            return other.address() ==
                endpoint.address();
        }))
    {
        return false;
    }

//添加到抑制列表中，这样我们就不会太频繁地尝试它。
//如果它已经存在，那么请尝试插入失败。
    auto const result (m_squelches.insert (
        endpoint.address()));
    if (! result.second)
        return false;

    m_list.push_back (endpoint);

    return true;
}

}
}

#endif
