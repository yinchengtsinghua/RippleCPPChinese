
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

#ifndef RIPPLE_CORE_JOBTYPEDATA_H_INCLUDED
#define RIPPLE_CORE_JOBTYPEDATA_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/core/JobTypeInfo.h>
#include <ripple/beast/insight/Collector.h>

namespace ripple
{

struct JobTypeData
{
private:
    LoadMonitor m_load;

    /*支持洞察力*/
    beast::insight::Collector::ptr m_collector;

public:
    /*我们所代表的工作类别*/
    JobTypeInfo const& info;

    /*等待的作业数*/
    int waiting;

    /*当前正在运行的号码*/
    int running;

    /*以及由于工作限制而延迟执行的数量*/
    int deferred;

    /*通知回调*/
    beast::insight::Event dequeue;
    beast::insight::Event execute;

    JobTypeData (JobTypeInfo const& info_,
            beast::insight::Collector::ptr const& collector, Logs& logs) noexcept
        : m_load (logs.journal ("LoadMonitor"))
        , m_collector (collector)
        , info (info_)
        , waiting (0)
        , running (0)
        , deferred (0)
    {
        m_load.setTargetLatency (
            info.getAverageLatency (),
            info.getPeakLatency());

        if (!info.special ())
        {
            dequeue = m_collector->make_event (info.name () + "_q");
            execute = m_collector->make_event (info.name ());
        }
    }

    /*不可复制构造或分配*/
    JobTypeData (JobTypeData const& other) = delete;
    JobTypeData& operator= (JobTypeData const& other) = delete;

    std::string name () const
    {
        return info.name ();
    }

    JobType type () const
    {
        return info.type ();
    }

    LoadMonitor& load ()
    {
        return m_load;
    }

    LoadMonitor::Stats stats ()
    {
        return m_load.getStats ();
    }
};

}

#endif
