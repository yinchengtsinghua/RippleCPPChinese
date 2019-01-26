
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

#include <ripple/overlay/PeerSet.h>
#include <ripple/app/main/Application.h>
#include <ripple/core/JobQueue.h>
#include <ripple/overlay/Overlay.h>

namespace ripple {

using namespace std::chrono_literals;

class InboundLedger;

//vfalc注意txtona构造函数参数是一种代码味道。
//如果我们是交易的基础，这是真的，
//如果我们是内部分类账的基础，则为假。它所做的一切
//是否更改计时器的行为取决于
//派生类。为什么不直接进行计时器回调呢
//纯虚函数？
//
PeerSet::PeerSet (Application& app, uint256 const& hash,
    std::chrono::milliseconds interval, clock_type& clock,
    beast::Journal journal)
    : app_ (app)
    , m_journal (journal)
    , m_clock (clock)
    , mHash (hash)
    , mTimerInterval (interval)
    , mTimeouts (0)
    , mComplete (false)
    , mFailed (false)
    , mProgress (false)
    , mTimer (app_.getIOService ())
{
    mLastAction = m_clock.now();
    assert ((mTimerInterval > 10ms) && (mTimerInterval < 30s));
}

PeerSet::~PeerSet() = default;

bool PeerSet::insert (std::shared_ptr<Peer> const& ptr)
{
    ScopedLockType sl (mLock);

    if (!mPeers.insert (ptr->id ()).second)
        return false;

    newPeer (ptr);
    return true;
}

void PeerSet::setTimer ()
{
    mTimer.expires_from_now(mTimerInterval);
    mTimer.async_wait (
        [wptr=pmDowncast()](boost::system::error_code const& ec)
        {
            if (ec == boost::asio::error::operation_aborted)
                return;

            if (auto ptr = wptr.lock ())
                ptr->execute ();
        });
}

void PeerSet::invokeOnTimer ()
{
    ScopedLockType sl (mLock);

    if (isDone ())
        return;

    if (!isProgress())
    {
        ++mTimeouts;
        JLOG (m_journal.debug()) << "Timeout(" << mTimeouts
            << ") pc=" << mPeers.size () << " acquiring " << mHash;
        onTimer (false, sl);
    }
    else
    {
        mProgress = false;
        onTimer (true, sl);
    }

    if (!isDone ())
        setTimer ();
}

bool PeerSet::isActive ()
{
    ScopedLockType sl (mLock);
    return !isDone ();
}

void PeerSet::sendRequest (const protocol::TMGetLedger& tmGL, std::shared_ptr<Peer> const& peer)
{
    if (!peer)
        sendRequest (tmGL);
    else
        peer->send (std::make_shared<Message> (tmGL, protocol::mtGET_LEDGER));
}

void PeerSet::sendRequest (const protocol::TMGetLedger& tmGL)
{
    ScopedLockType sl (mLock);

    if (mPeers.empty ())
        return;

    Message::pointer packet (
        std::make_shared<Message> (tmGL, protocol::mtGET_LEDGER));

    for (auto id : mPeers)
    {
        if (auto peer = app_.overlay ().findPeerByShortID (id))
            peer->send (packet);
    }
}

std::size_t PeerSet::getPeerCount () const
{
    std::size_t ret (0);

    for (auto id : mPeers)
    {
        if (app_.overlay ().findPeerByShortID (id))
            ++ret;
    }

    return ret;
}

} //涟漪
