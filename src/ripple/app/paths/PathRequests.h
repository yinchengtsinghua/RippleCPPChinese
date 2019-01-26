
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

#ifndef RIPPLE_APP_PATHS_PATHREQUESTS_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHREQUESTS_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/app/paths/PathRequest.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/core/Job.h>
#include <atomic>
#include <mutex>
#include <vector>

namespace ripple {

class PathRequests
{
public:
    /*所有PathRequest实例的集合。*/
    PathRequests (Application& app,
            beast::Journal journal, beast::insight::Collector::ptr const& collector)
        : app_ (app)
        , mJournal (journal)
        , mLastIdentifier (0)
    {
        mFast = collector->make_event ("pathfind_fast");
        mFull = collector->make_event ("pathfind_full");
    }

    /*更新包含的所有PathRequest实例。

        @param ledger ledger我们正在寻找路径。
        @param shouldcancel invocable返回是否取消。
     **/

    void updateAll (std::shared_ptr<ReadView const> const& ledger,
                    Job::CancelCallback shouldCancel);

    std::shared_ptr<RippleLineCache> getLineCache (
        std::shared_ptr <ReadView const> const& ledger, bool authoritative);

//创建一个新的样式的路径请求
//订阅服务器的更新
    Json::Value makePathRequest (
        std::shared_ptr <InfoSub> const& subscriber,
        std::shared_ptr<ReadView const> const& ledger,
        Json::Value const& request);

//创建旧样式的路径请求
//由协作程序管理并由更新
//路径引擎
    Json::Value makeLegacyPathRequest (
        PathRequest::pointer& req,
        std::function <void (void)> completion,
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request);

//立即执行旧样式的路径请求
//使用呼叫者指定的分类帐
    Json::Value doLegacyPathRequest (
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request);

    void reportFast (std::chrono::milliseconds ms)
    {
        mFast.notify (ms);
    }

    void reportFull (std::chrono::milliseconds ms)
    {
        mFull.notify (ms);
    }

private:
    void insertPathRequest (PathRequest::pointer const&);

    Application& app_;
    beast::Journal                   mJournal;

    beast::insight::Event            mFast;
    beast::insight::Event            mFull;

//跟踪所有请求
    std::vector<PathRequest::wptr> requests_;

//使用RippleLineCache
    std::shared_ptr<RippleLineCache>         mLineCache;

    std::atomic<int>                 mLastIdentifier;

    using ScopedLockType = std::lock_guard <std::recursive_mutex>;
    std::recursive_mutex mLock;

};

} //涟漪

#endif
