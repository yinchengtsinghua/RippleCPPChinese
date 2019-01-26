
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

#ifndef RIPPLE_APP_LEDGER_LEDGERTIMING_H_INCLUDED
#define RIPPLE_APP_LEDGER_LEDGERTIMING_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/beast/utility/Journal.h>
#include <chrono>
#include <cstdint>

namespace ripple {

/*可能的分类帐关闭时间解决方案。

    值不应重复。
    @见GetNextledgertimeResolution
**/

std::chrono::seconds constexpr ledgerPossibleTimeResolutions[] =
    {
        std::chrono::seconds { 10},
        std::chrono::seconds { 20},
        std::chrono::seconds { 30},
        std::chrono::seconds { 60},
        std::chrono::seconds { 90},
        std::chrono::seconds {120}
    };

//！分类帐关闭时间的初始解决方案。
auto constexpr ledgerDefaultTimeResolution = ledgerPossibleTimeResolutions[2];

//！我们多久增加一次接近时间分辨率（以分类账的数量计）
auto constexpr increaseLedgerTimeResolutionEvery = 8;

//！我们多久减少一次接近时间分辨率（以分类账的数量计）
auto constexpr decreaseLedgerTimeResolutionEvery = 1;

/*计算指定分类帐的关闭时间解决方案。

    Ripple协议使用binning来表示仅使用一个时间间隔
    时间戳。这使得服务器可以为下一个分类帐导出一个公共时间，
    不需要完美同步的时钟。
    时间分辨率（即间隔大小）是动态调整的。
    根据上一个分类账中发生的事情，尽量避免意见分歧。

    @param previousresolution用于上一个分类帐的分辨率
    @param previousAgree是否就之前的结束时间达成共识
    分类帐
    @param ledgerseq新分类帐的序列号

    @previousResolution必须是有效的bin
         来自@Ref LedgerPossibleTimeResolutions

    @tparam rep type表示std:：chrono:：duration中的刻度数
    @tparam period表示刻度周期的std:：ratio
                   标准：：时间：：持续时间
    @tparam seq unsigned integer-like类型对应于分类帐序列
                号码。它应该与0相当，支持模块化
                除法。支持内置和标记的_整数。
**/

template <class Rep, class Period, class Seq>
std::chrono::duration<Rep, Period>
getNextLedgerTimeResolution(
    std::chrono::duration<Rep, Period> previousResolution,
    bool previousAgree,
    Seq ledgerSeq)
{
    assert(ledgerSeq != Seq{0});

    using namespace std::chrono;
//查找当前分辨率：
    auto iter = std::find(
        std::begin(ledgerPossibleTimeResolutions),
        std::end(ledgerPossibleTimeResolutions),
        previousResolution);
    assert(iter != std::end(ledgerPossibleTimeResolutions));

//这不应该发生，但作为预防措施
    if (iter == std::end(ledgerPossibleTimeResolutions))
        return previousResolution;

//如果我们之前不同意，我们尝试将分辨率降低到
//提高我们现在达成一致的机会。
    if (!previousAgree &&
        (ledgerSeq % Seq{decreaseLedgerTimeResolutionEvery} == Seq{0}))
    {
        if (++iter != std::end(ledgerPossibleTimeResolutions))
            return *iter;
    }

//如果我们之前同意，我们会增加决议以确定
//如果我们能继续同意的话。
    if (previousAgree &&
        (ledgerSeq % Seq{increaseLedgerTimeResolutionEvery} == Seq{0}))
    {
        if (iter-- != std::begin(ledgerPossibleTimeResolutions))
            return *iter;
    }

    return previousResolution;
}

/*计算分类帐的关闭时间，给出关闭时间解决方案。

    @param closetime要路由的时间。
    @param closeresolution分辨率
    @return@b closetime四舍五入为@b closeresolution的最接近倍数。
    如果@b closetime在@b closeresolution的倍数之间的中间，则向上取整。
**/

template <class Clock, class Duration, class Rep, class Period>
std::chrono::time_point<Clock, Duration>
roundCloseTime(
    std::chrono::time_point<Clock, Duration> closeTime,
    std::chrono::duration<Rep, Period> closeResolution)
{
    using time_point = decltype(closeTime);
    if (closeTime == time_point{})
        return closeTime;

    closeTime += (closeResolution / 2);
    return closeTime - (closeTime.time_since_epoch() % closeResolution);
}

/*计算有效分类帐关闭时间

    根据当前解决方案调整分类帐关闭时间后，还
    确保与之前的关闭时间充分分离。

    @param close time原始分类帐关闭时间
    @param resolution当前关闭时间分辨率
    @param prior close time上一个分类帐的关闭时间
**/

template <class Clock, class Duration, class Rep, class Period>
std::chrono::time_point<Clock, Duration>
effCloseTime(
    std::chrono::time_point<Clock, Duration> closeTime,
    std::chrono::duration<Rep, Period> resolution,
    std::chrono::time_point<Clock, Duration> priorCloseTime)
{
    using namespace std::chrono_literals;
    using time_point = decltype(closeTime);

    if (closeTime == time_point{})
        return closeTime;

    return std::max<time_point>(
        roundCloseTime(closeTime, resolution), (priorCloseTime + 1s));
}

}
#endif
