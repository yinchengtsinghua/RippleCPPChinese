
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

#ifndef RIPPLE_APP_PEERS_PEERSET_H_INCLUDED
#define RIPPLE_APP_PEERS_PEERSET_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/beast/clock/abstract_clock.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/overlay/Peer.h>
#include <boost/asio/basic_waitable_timer.hpp>
#include <mutex>
#include <set>

namespace ripple {

/*通过管理一组对等方来支持数据检索。

    当需要数据时（如分类帐或交易集）
    本地缺失，可通过查询connected获得
    同龄人。此类管理检索的公共方面。
    呼叫者通过添加和删除对等方来维护集合，具体取决于
    关于同龄人是否有有用的信息。

    此类是一个“活动”对象。它保持自己的计时器
    并将工作分派到工作队列。实现派生
    并重写
    基地。

    数据由散列表示。
**/

class PeerSet
{
public:
    using clock_type = beast::abstract_clock <std::chrono::steady_clock>;

    /*返回所需数据的哈希值。*/
    uint256 const& getHash () const
    {
        return mHash;
    }

    /*如果我们得到所有数据，则返回true。*/
    bool isComplete () const
    {
        return mComplete;
    }

    /*如果获取数据失败，则返回false。*/
    bool isFailed () const
    {
        return mFailed;
    }

    /*返回超时的次数。*/
    int getTimeouts () const
    {
        return mTimeouts;
    }

    bool isActive ();

    /*调用以指示已前进。*/
    void progress ()
    {
        mProgress = true;
    }

    void touch ()
    {
        mLastAction = m_clock.now();
    }

    clock_type::time_point getLastAction () const
    {
        return mLastAction;
    }

    /*插入托管集的对等方。
        这将调用派生类挂钩函数。
        @返回'true'（如果添加了对等项）
    **/

    bool insert (std::shared_ptr<Peer> const&);

    virtual bool isDone () const
    {
        return mComplete || mFailed;
    }

    Application&
    app()
    {
        return app_;
    }

protected:
    using ScopedLockType = std::unique_lock <std::recursive_mutex>;

    PeerSet (Application& app, uint256 const& hash, std::chrono::milliseconds interval,
        clock_type& clock, beast::Journal journal);

    virtual ~PeerSet() = 0;

    virtual void newPeer (std::shared_ptr<Peer> const&) = 0;

    virtual void onTimer (bool progress, ScopedLockType&) = 0;

    virtual void execute () = 0;

    virtual std::weak_ptr<PeerSet> pmDowncast () = 0;

    bool isProgress ()
    {
        return mProgress;
    }

    void setComplete ()
    {
        mComplete = true;
    }
    void setFailed ()
    {
        mFailed = true;
    }

    void invokeOnTimer ();

    void sendRequest (const protocol::TMGetLedger& message);

    void sendRequest (const protocol::TMGetLedger& message, std::shared_ptr<Peer> const& peer);

    void setTimer ();

    std::size_t getPeerCount () const;

protected:
    Application& app_;
    beast::Journal m_journal;
    clock_type& m_clock;

    std::recursive_mutex mLock;

    uint256 mHash;
    std::chrono::milliseconds mTimerInterval;
    int mTimeouts;
    bool mComplete;
    bool mFailed;
    clock_type::time_point mLastAction;
    bool mProgress;

//vfalc todo将计时器的职责提升到更高的级别
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> mTimer;

//我们正在跟踪的对等机的标识符。
    std::set <Peer::id_t> mPeers;
};

} //涟漪

#endif
