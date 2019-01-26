
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

#ifndef RIPPLE_BASICS_CHRONO_H_INCLUDED
#define RIPPLE_BASICS_CHRONO_H_INCLUDED

#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/clock/basic_seconds_clock.h>
#include <ripple/beast/clock/manual_clock.h>
#include <chrono>
#include <cstdint>
#include <string>

namespace ripple {

//一些方便的别名

using days = std::chrono::duration<
    int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>>;

using weeks = std::chrono::duration<
    int, std::ratio_multiply<days::period, std::ratio<7>>>;

/*用于测量波纹网络时间的时钟。

    时代是2000年1月1日
    epoch_offset=days（10957）；//2000-01-01
**/

class NetClock
{
public:
    explicit NetClock() = default;

    using rep        = std::uint32_t;
    using period     = std::ratio<1>;
    using duration   = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<NetClock>;

    static bool const is_steady = false;
};

template <class Duration>
std::string
to_string(date::sys_time<Duration> tp)
{
    return date::format("%Y-%b-%d %T", tp);
}

inline
std::string
to_string(NetClock::time_point tp)
{
    using namespace std::chrono;
    return to_string(
        system_clock::time_point{tp.time_since_epoch() + 946684800s});
}

/*用来测量经过时间的时钟。

    时代未定。
**/

using Stopwatch = beast::abstract_clock<std::chrono::steady_clock>;

/*用于单元测试的手动秒表。*/
using TestStopwatch = beast::manual_clock<std::chrono::steady_clock>;

/*返回墙上时钟的实例。*/
inline
Stopwatch&
stopwatch()
{
    return beast::get_abstract_clock<
        std::chrono::steady_clock,
        beast::basic_seconds_clock<std::chrono::steady_clock>>();
}

} //涟漪

#endif
