
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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

#include <test/jtx/ManualTimeKeeper.h>

namespace ripple {
namespace test {

using namespace std::chrono_literals;

ManualTimeKeeper::ManualTimeKeeper()
    : closeOffset_ {}
    , now_ (0s)
{
}

void
ManualTimeKeeper::run (std::vector<std::string> const& servers)
{
}

auto
ManualTimeKeeper::now() const ->
    time_point
{
    std::lock_guard<std::mutex> lock(mutex_);
    return now_;
}
    
auto
ManualTimeKeeper::closeTime() const ->
    time_point
{
    std::lock_guard<std::mutex> lock(mutex_);
    return now_ + closeOffset_;
}

void
ManualTimeKeeper::adjustCloseTime(
    std::chrono::duration<std::int32_t> amount)
{
//复制自timekeeper:：adjustclosetime
    using namespace std::chrono;
    auto const s = amount.count();
    std::lock_guard<std::mutex> lock(mutex_);
//取大偏移，忽略小偏移，
//把接近的时间推向我们的墙时间。
    if (s > 1)
        closeOffset_ += seconds((s + 3) / 4);
    else if (s < -1)
        closeOffset_ += seconds((s - 3) / 4);
    else
        closeOffset_ = (closeOffset_ * 3) / 4;
}

std::chrono::duration<std::int32_t>
ManualTimeKeeper::nowOffset() const
{
    return {};
}

std::chrono::duration<std::int32_t>
ManualTimeKeeper::closeOffset() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return closeOffset_;
}

void
ManualTimeKeeper::set (time_point now)
{
    std::lock_guard<std::mutex> lock(mutex_);
    now_ = now;
}

auto
ManualTimeKeeper::adjust(
        std::chrono::system_clock::time_point when) ->
    time_point
{
    return time_point(
        std::chrono::duration_cast<duration>(
            when.time_since_epoch() -
                days(10957)));
}
} //测试
} //涟漪
