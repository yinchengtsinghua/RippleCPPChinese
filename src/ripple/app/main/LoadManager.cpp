
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

#include <ripple/app/main/LoadManager.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/UptimeClock.h>
#include <ripple/json/to_string.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <memory>
#include <mutex>
#include <thread>

namespace ripple {

LoadManager::LoadManager (
    Application& app, Stoppable& parent, beast::Journal journal)
    : Stoppable ("LoadManager", parent)
    , app_ (app)
    , journal_ (journal)
    , deadLock_ ()
    , armed_ (false)
    , stop_ (false)
{
}

LoadManager::~LoadManager ()
{
    try
    {
        onStop ();
    }
    catch (std::exception const& ex)
    {
//在析构函数中接受异常。
        JLOG(journal_.warn()) << "std::exception in ~LoadManager.  "
            << ex.what();
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void LoadManager::activateDeadlockDetector ()
{
    std::lock_guard<std::mutex> sl (mutex_);
    armed_ = true;
}

void LoadManager::resetDeadlockDetector ()
{
    auto const elapsedSeconds = UptimeClock::now();
    std::lock_guard<std::mutex> sl (mutex_);
    deadLock_ = elapsedSeconds;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void LoadManager::onPrepare ()
{
}

void LoadManager::onStart ()
{
    JLOG(journal_.debug()) << "Starting";
    assert (! thread_.joinable());

    thread_ = std::thread {&LoadManager::run, this};
}

void LoadManager::onStop ()
{
    if (thread_.joinable())
    {
        JLOG(journal_.debug()) << "Stopping";
        {
            std::lock_guard<std::mutex> sl (mutex_);
            stop_ = true;
        }
        thread_.join();
    }
    stopped ();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void LoadManager::run ()
{
    beast::setCurrentThreadName ("LoadManager");

    using namespace std::chrono_literals;
    using clock_type = std::chrono::system_clock;

    auto t = clock_type::now();
    bool stop = false;

    while (! (stop || isStopping ()))
    {
        {
//在锁下复制共享数据。在锁外使用副本。
            std::unique_lock<std::mutex> sl (mutex_);
            auto const deadLock = deadLock_;
            auto const armed = armed_;
            stop = stop_;
            sl.unlock();

//测量死锁的时间，以秒为单位。
            auto const timeSpentDeadlocked = UptimeClock::now() - deadLock;

            auto const reportingIntervalSeconds = 10s;
            if (armed && (timeSpentDeadlocked >= reportingIntervalSeconds))
            {
//每10秒报告一次死锁情况
                if ((timeSpentDeadlocked % reportingIntervalSeconds) == 0s)
                {
                    JLOG(journal_.warn())
                        << "Server stalled for "
                        << timeSpentDeadlocked.count() << " seconds.";
                }

//如果我们超过500秒就死锁了，这意味着
//死锁解决代码失败，这符合
//作为未定义的行为。
//
                assert (timeSpentDeadlocked < 500s);
            }
        }

        bool change = false;
        if (app_.getJobQueue ().isOverloaded ())
        {
            JLOG(journal_.info()) << app_.getJobQueue ().getJson (0);
            change = app_.getFeeTrack ().raiseLocalFee ();
        }
        else
        {
            change = app_.getFeeTrack ().lowerLocalFee ();
        }

        if (change)
        {
//vfalc todo用一个监听器/观察者替换这个
//在网络或应用程序中订阅。
            app_.getOPs ().reportFeeChange ();
        }

        t += 1s;
        auto const duration = t - clock_type::now();

        if ((duration < 0s) || (duration > 1s))
        {
            JLOG(journal_.warn()) << "time jump";
            t = clock_type::now();
        }
        else
        {
            alertable_sleep_until(t);
        }
    }

    stopped ();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr<LoadManager>
make_LoadManager (Application& app,
    Stoppable& parent, beast::Journal journal)
{
    return std::unique_ptr<LoadManager>{new LoadManager{app, parent, journal}};
}

} //涟漪
