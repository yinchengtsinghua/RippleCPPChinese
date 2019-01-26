
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

#ifndef RIPPLE_APP_PATHS_PATHREQUEST_H_INCLUDED
#define RIPPLE_APP_PATHS_PATHREQUEST_H_INCLUDED

#include <ripple/app/ledger/Ledger.h>
#include <ripple/app/paths/Pathfinder.h>
#include <ripple/app/paths/RippleLineCache.h>
#include <ripple/json/json_value.h>
#include <ripple/net/InfoSub.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/optional.hpp>
#include <map>
#include <mutex>
#include <set>
#include <utility>

namespace ripple {

//由客户端提交的寻路请求
//请求颁发者必须维护一个强指针

class RippleLineCache;
class PathRequests;

//parsejson的返回值<0=无效，>0=有效
#define PFR_PJ_INVALID              -1
#define PFR_PJ_NOCHANGE             0
#define PFR_PJ_CHANGE               1

class PathRequest :
    public std::enable_shared_from_this <PathRequest>,
    public CountedObject <PathRequest>
{
public:
    static char const* getCountedObjectName () { return "PathRequest"; }

    using wptr    = std::weak_ptr<PathRequest>;
    using pointer = std::shared_ptr<PathRequest>;
    using ref     = const pointer&;
    using wref    = const wptr&;

public:
//vfalc todo中断对infosub的循环依赖

//路径查找语义
//订阅服务器已更新
    PathRequest (
        Application& app,
        std::shared_ptr <InfoSub> const& subscriber,
        int id,
        PathRequests&,
        beast::Journal journal);

//纹波路径找到语义
//完成路径更新后调用完成函数
    PathRequest (
        Application& app,
        std::function <void (void)> const& completion,
        Resource::Consumer& consumer,
        int id,
        PathRequests&,
        beast::Journal journal);

    ~PathRequest ();

    bool isNew ();
    bool needsUpdate (bool newOnly, LedgerIndex index);

//在PathRequest更新完成时调用。
    void updateComplete ();

    std::pair<bool, Json::Value> doCreate (
        std::shared_ptr<RippleLineCache> const&,
        Json::Value const&);

    Json::Value doClose (Json::Value const&);
    Json::Value doStatus (Json::Value const&);

//更新JVStand
    Json::Value doUpdate (
        std::shared_ptr<RippleLineCache> const&, bool fast);
    InfoSub::pointer getSubscriber ();
    bool hasCompletion ();

private:
    using ScopedLockType = std::lock_guard <std::recursive_mutex>;

    bool isValid (std::shared_ptr<RippleLineCache> const& crCache);
    void setValid ();

    std::unique_ptr<Pathfinder> const&
    getPathFinder(std::shared_ptr<RippleLineCache> const&,
        hash_map<Currency, std::unique_ptr<Pathfinder>>&, Currency const&,
            STAmount const&, int const);

    /*在json参数中查找并设置路径集。
        如果源货币无效，则返回false。
    **/

    bool
    findPaths (std::shared_ptr<RippleLineCache> const&, int const, Json::Value&);

    int parseJson (Json::Value const&);

    Application& app_;
    beast::Journal m_journal;

    std::recursive_mutex mLock;

    PathRequests& mOwner;

std::weak_ptr<InfoSub> wpSubscriber; //这个请求来自谁
    std::function <void (void)> fCompletion;
Resource::Consumer& consumer_; //按源货币收费

    Json::Value jvId;
Json::Value jvStatus; //最后结果

//客户端请求参数
    boost::optional<AccountID> raSrcAccount;
    boost::optional<AccountID> raDstAccount;
    STAmount saDstAmount;
    boost::optional<STAmount> saSendMax;

    std::set<Issue> sciSourceCurrencies;
    std::map<Issue, STPathSet> mContext;

    bool convert_all_;

    std::recursive_mutex mIndexLock;
    LedgerIndex mLastIndex;
    bool mInProgress;

    int iLevel;
    bool bLastSuccess;

    int iIdentifier;

    std::chrono::steady_clock::time_point const created_;
    std::chrono::steady_clock::time_point quick_reply_;
    std::chrono::steady_clock::time_point full_reply_;

    static unsigned int const max_paths_ = 4;
};

} //涟漪

#endif
