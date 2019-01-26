
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

#include <ripple/net/InfoSub.h>
#include <atomic>

namespace ripple {

//这是进入程序“客户机”部分的主要接口。
//希望在网络上执行正常操作的代码，例如
//创建和监控帐户、创建事务等
//应该使用这个接口。RPC代码主要是一个轻量级包装器
//通过这个代码。

//最后，它将检查节点的操作模式（同步、未同步、
//并遵循正确的处理方法。电流
//代码假定此节点已同步（并将继续同步，直到
//有一个功能强大的网络。

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

InfoSub::Source::Source (char const* name, Stoppable& parent)
    : Stoppable (name, parent)
{
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

InfoSub::InfoSub(Source& source)
    : m_source(source)
    , mSeq(assign_id())
{
}

InfoSub::InfoSub(Source& source, Consumer consumer)
    : m_consumer(consumer)
    , m_source(source)
    , mSeq(assign_id())
{
}

InfoSub::~InfoSub ()
{
    m_source.unsubTransactions (mSeq);
    m_source.unsubRTTransactions (mSeq);
    m_source.unsubLedger (mSeq);
    m_source.unsubManifests (mSeq);
    m_source.unsubServer (mSeq);
    m_source.unsubValidations (mSeq);
    m_source.unsubPeerStatus (mSeq);

//使用内部取消订阅，这样它就不会调用
//回到我们这里，修改它自己的参数
    if (! realTimeSubscriptions_.empty ())
        m_source.unsubAccountInternal
            (mSeq, realTimeSubscriptions_, true);

    if (! normalSubscriptions_.empty ())
        m_source.unsubAccountInternal
            (mSeq, normalSubscriptions_, false);
}

Resource::Consumer& InfoSub::getConsumer()
{
    return m_consumer;
}

std::uint64_t InfoSub::getSeq ()
{
    return mSeq;
}

void InfoSub::onSendEmpty ()
{
}

void InfoSub::insertSubAccountInfo (AccountID const& account, bool rt)
{
    ScopedLockType sl (mLock);

    if (rt)
        realTimeSubscriptions_.insert (account);
    else
        normalSubscriptions_.insert (account);
}

void InfoSub::deleteSubAccountInfo (AccountID const& account, bool rt)
{
    ScopedLockType sl (mLock);

    if (rt)
        realTimeSubscriptions_.erase (account);
    else
        normalSubscriptions_.erase (account);
}

void InfoSub::clearPathRequest ()
{
    mPathRequest.reset ();
}

void InfoSub::setPathRequest (const std::shared_ptr<PathRequest>& req)
{
    mPathRequest = req;
}

const std::shared_ptr<PathRequest>& InfoSub::getPathRequest ()
{
    return mPathRequest;
}

} //涟漪
