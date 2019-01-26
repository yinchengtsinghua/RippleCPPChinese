
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

#include <ripple/resource/ResourceManager.h>
#include <ripple/resource/impl/Logic.h>
#include <ripple/basics/chrono.h>
#include <ripple/basics/Log.h>
#include <ripple/beast/core/CurrentThreadName.h>
#include <boost/core/ignore_unused.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace ripple {
namespace Resource {

class ManagerImp : public Manager
{
private:
    beast::Journal journal_;
    Logic logic_;
    std::thread thread_;
    bool stop_ = false;
    std::mutex mutex_;
    std::condition_variable cond_;

public:
    ManagerImp (beast::insight::Collector::ptr const& collector,
        beast::Journal journal)
        : journal_ (journal)
        , logic_ (collector, stopwatch(), journal)
    {
boost::ignore_unused (journal_); //保留未使用的日志，以防万一。
        thread_ = std::thread {&ManagerImp::run, this};
    }

    ManagerImp () = delete;
    ManagerImp (ManagerImp const&) = delete;
    ManagerImp& operator= (ManagerImp const&) = delete;

    ~ManagerImp () override
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
            cond_.notify_one();
        }
        thread_.join();
    }

    Consumer newInboundEndpoint (beast::IP::Endpoint const& address) override
    {
        return logic_.newInboundEndpoint (address);
    }

    Consumer newOutboundEndpoint (beast::IP::Endpoint const& address) override
    {
        return logic_.newOutboundEndpoint (address);
    }

    Consumer newUnlimitedEndpoint (std::string const& name) override
    {
        return logic_.newUnlimitedEndpoint (name);
    }

    Gossip exportConsumers () override
    {
        return logic_.exportConsumers();
    }

    void importConsumers (
        std::string const& origin, Gossip const& gossip) override
    {
        logic_.importConsumers (origin, gossip);
    }

//——————————————————————————————————————————————————————————————

    Json::Value getJson () override
    {
        return logic_.getJson ();
    }

    Json::Value getJson (int threshold) override
    {
        return logic_.getJson (threshold);
    }

//——————————————————————————————————————————————————————————————

    void onWrite (beast::PropertyStream::Map& map) override
    {
        logic_.onWrite (map);
    }

//——————————————————————————————————————————————————————————————

private:
    void run ()
    {
        beast::setCurrentThreadName ("Resource::Manager");
        for(;;)
        {
            logic_.periodicActivity();
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::seconds(1));
            if (stop_)
                break;
        }
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

Manager::Manager ()
    : beast::PropertyStream::Source ("resource")
{
}

Manager::~Manager() = default;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::unique_ptr <Manager> make_Manager (
    beast::insight::Collector::ptr const& collector,
        beast::Journal journal)
{
    return std::make_unique <ManagerImp> (collector, journal);
}

}
}
