
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

#include <ripple/basics/Log.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/basics/date.h>
#include <ripple/core/LoadMonitor.h>

namespace ripple {

/*

托多
----

-使用日志记录

**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

LoadMonitor::Stats::Stats()
    : count (0)
    , latencyAvg (0)
    , latencyPeak (0)
    , isOverloaded (false)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

LoadMonitor::LoadMonitor (beast::Journal j)
    : mCounts (0)
    , mLatencyEvents (0)
    , mLatencyMSAvg (0)
    , mLatencyMSPeak (0)
    , mTargetLatencyAvg (0)
    , mTargetLatencyPk (0)
    , mLastUpdate (UptimeClock::now())
    , j_ (j)
{
}

//vvalco注：为什么我们需要“互斥体”？这种依赖
//隐藏的全局，尤其是同步原语，
//是一个有缺陷的设计。
//目前还不清楚需要保护哪些数据。
//
//使用mutex调用
void LoadMonitor::update ()
{
    using namespace std::chrono_literals;
    auto now = UptimeClock::now();
if (now == mLastUpdate) //现在的
        return;

//为什么是8？
    if ((now < mLastUpdate) || (now > (mLastUpdate + 8s)))
    {
//过时的办法
        mCounts = 0;
        mLatencyEvents = 0;
        mLatencyMSAvg = 0ms;
        mLatencyMSPeak = 0ms;
        mLastUpdate = now;
//vfalc todo不从中间回来…
        return;
    }

//做指数衰减
    /*
        戴维：

        “想象一下，如果你每秒钟增加10个。你呢
         也可以每秒减少1/4。它将在40时“空转”。
         每秒10次。”
    **/

    do
    {
        mLastUpdate += 1s;
        mCounts -= ((mCounts + 3) / 4);
        mLatencyEvents -= ((mLatencyEvents + 3) / 4);
        mLatencyMSAvg -= (mLatencyMSAvg / 4);
        mLatencyMSPeak -= (mLatencyMSPeak / 4);
    }
    while (mLastUpdate < now);
}

void LoadMonitor::addLoadSample (LoadEvent const& s)
{
    using namespace std::chrono;

    auto const total = s.runTime() + s.waitTime();
//不要将“抖动”作为延迟的一部分
    auto const latency = total < 2ms ? 0ms : date::round<milliseconds>(total);

    if (latency > 500ms)
    {
        auto mj = (latency > 1s) ? j_.warn() : j_.info();
        JLOG (mj) << "Job: " << s.name() <<
            " run: " << date::round<milliseconds>(s.runTime()).count() <<
            "ms" << " wait: " <<
            date::round<milliseconds>(s.waitTime()).count() << "ms";
    }

    addSamples (1, latency);
}

/*添加多个示例
   @param count要添加的样本数
   @param latencyms毫秒总数
**/

void LoadMonitor::addSamples (int count, std::chrono::milliseconds latency)
{
    std::lock_guard<std::mutex> sl (mutex_);

    update ();
    mCounts += count;
    mLatencyEvents += count;
    mLatencyMSAvg += latency;
    mLatencyMSPeak += latency;

    auto const latencyPeak = mLatencyEvents * latency * 4 / count;

    if (mLatencyMSPeak < latencyPeak)
        mLatencyMSPeak = latencyPeak;
}

void LoadMonitor::setTargetLatency (std::chrono::milliseconds avg,
                                    std::chrono::milliseconds pk)
{
    mTargetLatencyAvg  = avg;
    mTargetLatencyPk = pk;
}

bool LoadMonitor::isOverTarget (std::chrono::milliseconds avg,
                                std::chrono::milliseconds peak)
{
    using namespace std::chrono_literals;
    return (mTargetLatencyPk > 0ms && (peak > mTargetLatencyPk)) ||
           (mTargetLatencyAvg > 0ms && (avg > mTargetLatencyAvg));
}

bool LoadMonitor::isOver ()
{
    std::lock_guard<std::mutex> sl (mutex_);

    update ();

    if (mLatencyEvents == 0)
        return 0;

    return isOverTarget (mLatencyMSAvg / (mLatencyEvents * 4), mLatencyMSPeak / (mLatencyEvents * 4));
}

LoadMonitor::Stats LoadMonitor::getStats ()
{
    using namespace std::chrono_literals;
    Stats stats;

    std::lock_guard<std::mutex> sl (mutex_);

    update ();

    stats.count = mCounts / 4;

    if (mLatencyEvents == 0)
    {
        stats.latencyAvg = 0ms;
        stats.latencyPeak = 0ms;
    }
    else
    {
        stats.latencyAvg = mLatencyMSAvg / (mLatencyEvents * 4);
        stats.latencyPeak = mLatencyMSPeak / (mLatencyEvents * 4);
    }

    stats.isOverloaded = isOverTarget (stats.latencyAvg, stats.latencyPeak);

    return stats;
}

} //涟漪
