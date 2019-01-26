
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
#include <ripple/core/TimeKeeper.h>
#include <ripple/core/impl/SNTPClock.h>
#include <memory>
#include <mutex>

namespace ripple {

class TimeKeeperImpl : public TimeKeeper
{
private:
    beast::Journal j_;
    std::mutex mutable mutex_;
    std::chrono::duration<std::int32_t> closeOffset_;
    std::unique_ptr<SNTPClock> clock_;

//调整系统时钟：netclock epoch的时间点
    static
    time_point
    adjust (std::chrono::system_clock::time_point when)
    {
        return time_point(
            std::chrono::duration_cast<duration>(
                when.time_since_epoch() -
                    days(10957)));
    }

public:
    explicit
    TimeKeeperImpl (beast::Journal j)
        : j_ (j)
        , closeOffset_ {}
        , clock_ (make_SNTPClock(j))
    {
    }

    void
    run (std::vector<
        std::string> const& servers) override
    {
        clock_->run(servers);
    }

    time_point
    now() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return adjust(clock_->now());
    }

    time_point
    closeTime() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return adjust(clock_->now()) + closeOffset_;
    }

    void
    adjustCloseTime(
        std::chrono::duration<std::int32_t> amount) override
    {
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
        if (closeOffset_.count() != 0)
        {
            if (std::abs (closeOffset_.count()) < 60)
            {
                JLOG(j_.info()) <<
                    "TimeKeeper: Close time offset now " <<
                        closeOffset_.count();
            }
            else
            {
                JLOG(j_.warn()) <<
                    "TimeKeeper: Large close time offset = " <<
                        closeOffset_.count();
            }
        }
    }

    std::chrono::duration<std::int32_t>
    nowOffset() const override
    {
        using namespace std::chrono;
        using namespace std;
        lock_guard<mutex> lock(mutex_);
        return duration_cast<chrono::duration<int32_t>>(clock_->offset());
    }

    std::chrono::duration<std::int32_t>
    closeOffset() const override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closeOffset_;
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<TimeKeeper>
make_TimeKeeper (beast::Journal j)
{
    return std::make_unique<TimeKeeperImpl>(j);
}

} //涟漪
