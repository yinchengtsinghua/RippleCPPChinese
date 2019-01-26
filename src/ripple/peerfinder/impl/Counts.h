
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

#ifndef RIPPLE_PEERFINDER_COUNTS_H_INCLUDED
#define RIPPLE_PEERFINDER_COUNTS_H_INCLUDED

#include <ripple/basics/random.h>
#include <ripple/peerfinder/PeerfinderManager.h>
#include <ripple/peerfinder/Slot.h>
#include <ripple/peerfinder/impl/Tuning.h>

namespace ripple {
namespace PeerFinder {

/*管理各种插槽的可用连接数。*/
class Counts
{
public:
    Counts ()
        : m_attempts (0)
        , m_active (0)
        , m_in_max (0)
        , m_in_active (0)
        , m_out_max (0)
        , m_out_active (0)
        , m_fixed (0)
        , m_fixed_active (0)
        , m_cluster (0)

        , m_acceptCount (0)
        , m_closingCount (0)
    {
        m_roundingThreshold =
            std::generate_canonical <double, 10> (
                default_prng());
    }

//——————————————————————————————————————————————————————————————

    /*将插槽状态和属性添加到插槽计数中。*/
    void add (Slot const& s)
    {
        adjust (s, 1);
    }

    /*从插槽计数中删除插槽状态和属性。*/
    void remove (Slot const& s)
    {
        adjust (s, -1);
    }

    /*如果槽可以变为活动状态，则返回“true”。*/
    bool can_activate (Slot const& s) const
    {
//必须握手并处于正确的状态
        assert (s.state() == Slot::connected || s.state() == Slot::accept);

        if (s.fixed () || s.cluster ())
            return true;

        if (s.inbound ())
            return m_in_active < m_in_max;

        return m_out_active < m_out_max;
    }

    /*返回使我们达到最大值所需的尝试次数。*/
    std::size_t attempts_needed () const
    {
        if (m_attempts >= Tuning::maxConnectAttempts)
            return 0;
        return Tuning::maxConnectAttempts - m_attempts;
    }

    /*返回出站连接尝试的次数。*/
    std::size_t attempts () const
    {
        return m_attempts;
    }

    /*返回出站插槽的总数。*/
    int out_max () const
    {
        return m_out_max;
    }

    /*返回分配给打开插槽的出站对等机的数目。
        固定对等点不计入使用的出站插槽。
    **/

    int out_active () const
    {
        return m_out_active;
    }

    /*返回固定连接数。*/
    std::size_t fixed () const
    {
        return m_fixed;
    }

    /*返回活动的固定连接数。*/
    std::size_t fixed_active () const
    {
        return m_fixed_active;
    }

//——————————————————————————————————————————————————————————————

    /*在设置或更改配置时调用。*/
    void onConfig (Config const& config)
    {
//计算我们想要的出站对等机的数量。如果我们不想或不能
//接受传入，这将简单地等于maxpeers。否则
//我们根据百分比和伪随机计算分数。
//向上或向下取整。
//
        if (config.wantIncoming)
        {
//使用伯努利分布向上舍入
            m_out_max = std::floor (config.outPeers);
            if (m_roundingThreshold < (config.outPeers - m_out_max))
                ++m_out_max;
        }
        else
        {
            m_out_max = config.maxPeers;
        }

//计算我们可以接受的最大入站连接数。
        if (config.maxPeers >= m_out_max)
            m_in_max = config.maxPeers - m_out_max;
        else
            m_in_max = 0;
    }

    /*返回未握手的已接受连接数。*/
    int acceptCount() const
    {
        return m_acceptCount;
    }

    /*返回当前活动的连接尝试次数。*/
    int connectCount() const
    {
        return m_attempts;
    }

    /*返回正常关闭的连接数。*/
    int closingCount () const
    {
        return m_closingCount;
    }

    /*返回入站插槽的总数。*/
    int inboundSlots () const
    {
        return m_in_max;
    }

    /*返回分配给打开插槽的入站对等机的数目。*/
    int inboundActive () const
    {
        return m_in_active;
    }

    /*返回不包括固定对等的活动对等的总数。*/
    int totalActive () const
    {
        return m_in_active + m_out_active;
    }

    /*返回未使用的入站插槽数。
        固定对等点不从入站时段中扣除，也不计入总数。
    **/

    int inboundSlotsFree () const
    {
        if (m_in_active < m_in_max)
            return m_in_max - m_in_active;
        return 0;
    }

    /*返回未使用的出站插槽数。
        固定对等点不从出站时段中扣除，也不计入总数。
    **/

    int outboundSlotsFree () const
    {
        if (m_out_active < m_out_max)
            return m_out_max - m_out_active;
        return 0;
    }

//——————————————————————————————————————————————————————————————

    /*如果插槽逻辑认为我们“已连接”到网络，则返回true。*/
    bool isConnectedToNetwork () const
    {
//如果我们已经
//所需的传出连接数，或者如果连接
//自动为假。
//
//固定对等点不计入活动传出总数。

        if (m_out_max > 0)
            return false;

        return true;
    }

    /*输出统计。*/
    void onWrite (beast::PropertyStream::Map& map)
    {
        map ["accept"]  = acceptCount ();
        map ["connect"] = connectCount ();
        map ["close"]   = closingCount ();
        map ["in"]      << m_in_active << "/" << m_in_max;
        map ["out"]     << m_out_active << "/" << m_out_max;
        map ["fixed"]   = m_fixed_active;
        map ["cluster"] = m_cluster;
        map ["total"]   = m_active;
    }

    /*记录诊断状态。*/
    std::string state_string () const
    {
        std::stringstream ss;
        ss <<
            m_out_active << "/" << m_out_max << " out, " <<
            m_in_active << "/" << m_in_max << " in, " <<
            connectCount() << " connecting, " <<
            closingCount() << " closing"
            ;
        return ss.str();
    }

//——————————————————————————————————————————————————————————————
private:
//根据指定的插槽，按指示的方向调整计数。
    void adjust (Slot const& s, int const n)
    {
        if (s.fixed ())
            m_fixed += n;

        if (s.cluster ())
            m_cluster += n;

        switch (s.state ())
        {
        case Slot::accept:
            assert (s.inbound ());
            m_acceptCount += n;
            break;

        case Slot::connect:
        case Slot::connected:
            assert (! s.inbound ());
            m_attempts += n;
            break;

        case Slot::active:
            if (s.fixed ())
                m_fixed_active += n;
            if (! s.fixed () && ! s.cluster ())
            {
                if (s.inbound ())
                    m_in_active += n;
                else
                    m_out_active += n;
            }
            m_active += n;
            break;

        case Slot::closing:
            m_closingCount += n;
            break;

        default:
            assert (false);
            break;
        };
    }

private:
    /*出站连接尝试。*/
    int m_attempts;

    /*活动连接，包括固定和群集。*/
    std::size_t m_active;

    /*入站插槽总数。*/
    std::size_t m_in_max;

    /*分配给活动对等机的入站插槽数。*/
    std::size_t m_in_active;

    /*所需的最大出站插槽数。*/
    std::size_t m_out_max;

    /*活动出站插槽。*/
    std::size_t m_out_active;

    /*固定连接。*/
    std::size_t m_fixed;

    /*活动的固定连接。*/
    std::size_t m_fixed_active;

    /*群集连接。*/
    std::size_t m_cluster;




//入站连接数
//未激活或未优雅关闭。
    int m_acceptCount;

//正常关闭的连接数。
    int m_closingCount;

    /*分数门限，低于它我们四舍五入。
        这用于向上或向下舍入config:：outpeers的值
        这样网络范围内的平均传出数
        连接近似于建议的分数值。
    **/

    double m_roundingThreshold;
};

}
}

#endif
