
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

#include <ripple/app/paths/PathRequests.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/basics/Log.h>
#include <ripple/core/JobQueue.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <algorithm>

namespace ripple {

/*获取当前的RippleLineCache，必要时进行更新。
    找到正确的分类帐。
**/

std::shared_ptr<RippleLineCache>
PathRequests::getLineCache (
    std::shared_ptr <ReadView const> const& ledger,
    bool authoritative)
{
    ScopedLockType sl (mLock);

    std::uint32_t lineSeq = mLineCache ? mLineCache->getLedger()->seq() : 0;
    std::uint32_t lgrSeq = ledger->seq();

if ( (lineSeq == 0) ||                                 //无分类帐
(authoritative && (lgrSeq > lineSeq)) ||          //更新的权威分类账
(authoritative && ((lgrSeq + 8)  < lineSeq)) ||   //我们出于某种原因跳回去了
(lgrSeq > (lineSeq + 8)))                         //我们出于某种原因向前跳了一步
    {
        mLineCache = std::make_shared<RippleLineCache> (ledger);
    }
    return mLineCache;
}

void PathRequests::updateAll (std::shared_ptr <ReadView const> const& inLedger,
                              Job::CancelCallback shouldCancel)
{
    auto event =
        app_.getJobQueue().makeLoadEvent(
            jtPATH_FIND, "PathRequest::updateAll");

    std::vector<PathRequest::wptr> requests;
    std::shared_ptr<RippleLineCache> cache;

//获取我们应该使用的分类帐和缓存
    {
        ScopedLockType sl (mLock);
        requests = requests_;
        cache = getLineCache (inLedger, true);
    }

    bool newRequests = app_.getLedgerMaster().isNewPathRequest();
    bool mustBreak = false;

    JLOG (mJournal.trace()) <<
        "updateAll seq=" << cache->getLedger()->seq() <<
        ", " << requests.size() << " requests";

    int processed = 0, removed = 0;

    do
    {
        for (auto const& wr : requests)
        {
            if (shouldCancel())
                break;

            auto request = wr.lock ();
            bool remove = true;

            if (request)
            {
                if (!request->needsUpdate (newRequests, cache->getLedger()->seq()))
                    remove = false;
                else
                {
                    if (auto ipSub = request->getSubscriber ())
                    {
                        if (!ipSub->getConsumer ().warn ())
                        {
                            Json::Value update = request->doUpdate (cache, false);
                            request->updateComplete ();
                            update[jss::type] = "path_find";
                            ipSub->send (update, false);
                            remove = false;
                            ++processed;
                        }
                    }
                    else if (request->hasCompletion ())
                    {
//具有完成功能的一次性请求
                        request->doUpdate (cache, false);
                        request->updateComplete();
                        ++processed;
                    }
                }
            }

            if (remove)
            {
                ScopedLockType sl (mLock);

//移除任何悬空的弱指针或弱指针
//指向此路径请求的指针。
                auto ret = std::remove_if (
                    requests_.begin(), requests_.end(),
                    [&removed,&request](auto const& wl)
                    {
                        auto r = wl.lock();

                        if (r && r != request)
                            return false;
                        ++removed;
                        return true;
                    });

                requests_.erase (ret, requests_.end());
            }

            mustBreak = !newRequests &&
                app_.getLedgerMaster().isNewPathRequest();

//我们没有处理新的请求，然后
//有一个新的请求
            if (mustBreak)
                break;

        }

        if (mustBreak)
{ //我们工作的时候有一个新的要求进来了
            newRequests = true;
        }
        else if (newRequests)
{ //我们只做了新的请求，所以我们总是需要最后一个通行证
            newRequests = app_.getLedgerMaster().isNewPathRequest();
        }
        else
{ //如果没有新的请求，我们就完成了
            newRequests = app_.getLedgerMaster().isNewPathRequest();
            if (!newRequests)
                break;
        }

        {
//获取下一个通行证的最新请求、缓存和分类帐
            ScopedLockType sl (mLock);

            if (requests_.empty())
                break;
            requests = requests_;
            cache = getLineCache (cache->getLedger(), false);
        }
    }
    while (!shouldCancel ());

    JLOG (mJournal.debug()) <<
        "updateAll complete: " << processed << " processed and " <<
        removed << " removed";
}

void PathRequests::insertPathRequest (
    PathRequest::pointer const& req)
{
    ScopedLockType sl (mLock);

//在任何旧的未服务请求之后但在之前插入
//任何服务请求
    auto ret = std::find_if (
        requests_.begin(), requests_.end(),
        [](auto const& wl)
        {
            auto r = wl.lock();

//我们在处理请求之前就来了
            return r && !r->isNew();
        });

    requests_.emplace (ret, req);
}

//创建一个新样式的路径\查找请求
Json::Value
PathRequests::makePathRequest(
    std::shared_ptr <InfoSub> const& subscriber,
    std::shared_ptr<ReadView const> const& inLedger,
    Json::Value const& requestJson)
{
    auto req = std::make_shared<PathRequest> (
        app_, subscriber, ++mLastIdentifier, *this, mJournal);

    auto result = req->doCreate (
        getLineCache (inLedger, false), requestJson);

    if (result.first)
    {
        subscriber->setPathRequest (req);
        insertPathRequest (req);
        app_.getLedgerMaster().newPathRequest();
    }
    return std::move (result.second);
}

//提出一个老式的Ripple路径查找请求
Json::Value
PathRequests::makeLegacyPathRequest(
    PathRequest::pointer& req,
    std::function <void (void)> completion,
    Resource::Consumer& consumer,
    std::shared_ptr<ReadView const> const& inLedger,
    Json::Value const& request)
{
//此任务必须在
//调用完成函数
    req = std::make_shared<PathRequest> (
        app_, completion, consumer, ++mLastIdentifier,
            *this, mJournal);

    auto result = req->doCreate (
        getLineCache (inLedger, false), request);

    if (!result.first)
    {
        req.reset();
    }
    else
    {
        insertPathRequest (req);
        if (! app_.getLedgerMaster().newPathRequest())
        {
//newPathRequest失败。告诉来电者。
            result.second = rpcError (rpcTOO_BUSY);
            req.reset();
        }
    }

    return std::move (result.second);
}

Json::Value
PathRequests::doLegacyPathRequest (
        Resource::Consumer& consumer,
        std::shared_ptr<ReadView const> const& inLedger,
        Json::Value const& request)
{
    auto cache = std::make_shared<RippleLineCache> (inLedger);

    auto req = std::make_shared<PathRequest> (app_, []{},
        consumer, ++mLastIdentifier, *this, mJournal);

    auto result = req->doCreate (cache, request);
    if (result.first)
        result.second = req->doUpdate (cache, false);
    return std::move (result.second);
}

} //涟漪
