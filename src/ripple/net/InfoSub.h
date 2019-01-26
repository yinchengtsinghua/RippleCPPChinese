
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

#ifndef RIPPLE_NET_INFOSUB_H_INCLUDED
#define RIPPLE_NET_INFOSUB_H_INCLUDED

#include <ripple/basics/CountedObject.h>
#include <ripple/json/json_value.h>
#include <ripple/app/misc/Manifest.h>
#include <ripple/resource/Consumer.h>
#include <ripple/protocol/Book.h>
#include <ripple/core/Stoppable.h>
#include <mutex>

namespace ripple {

//客户端可能希望对网络执行的操作
//主操作处理器、服务器序列器、网络跟踪器

class PathRequest;

/*管理客户端对数据源的订阅。
**/

class InfoSub
    : public CountedObject <InfoSub>
{
public:
    static char const* getCountedObjectName () { return "InfoSub"; }

    using pointer = std::shared_ptr<InfoSub>;

//vvalco todo标准化弱/强指针类型别名的名称。
    using wptr = std::weak_ptr<InfoSub>;

    using ref = const std::shared_ptr<InfoSub>&;

    using Consumer = Resource::Consumer;

public:
    /*提取订阅数据的源。
    **/

    class Source : public Stoppable
    {
    protected:
        Source (char const* name, Stoppable& parent);

    public:

//出于某种原因，这些最初被称为“RT”
//用于“实时”。它们实际上是指
//当交易发生时，或者一旦交易发生
//结果已确认
        virtual void subAccount (ref ispListener,
            hash_set<AccountID> const& vnaAccountIDs,
            bool realTime) = 0;

//对于正常使用，从infosub和server中删除
        virtual void unsubAccount (ref isplistener,
            hash_set<AccountID> const& vnaAccountIDs,
            bool realTime) = 0;

//在信息子系统销毁期间使用
//仅从服务器中删除
        virtual void unsubAccountInternal (std::uint64_t uListener,
            hash_set<AccountID> const& vnaAccountIDs,
            bool realTime) = 0;

//vfalc todo记录bool返回值
        virtual bool subLedger (ref ispListener, Json::Value& jvResult) = 0;
        virtual bool unsubLedger (std::uint64_t uListener) = 0;

        virtual bool subManifests (ref ispListener) = 0;
        virtual bool unsubManifests (std::uint64_t uListener) = 0;
        virtual void pubManifest (Manifest const&) = 0;

        virtual bool subServer (ref ispListener, Json::Value& jvResult,
            bool admin) = 0;
        virtual bool unsubServer (std::uint64_t uListener) = 0;

        virtual bool subBook (ref ispListener, Book const&) = 0;
        virtual bool unsubBook (std::uint64_t uListener, Book const&) = 0;

        virtual bool subTransactions (ref ispListener) = 0;
        virtual bool unsubTransactions (std::uint64_t uListener) = 0;

        virtual bool subRTTransactions (ref ispListener) = 0;
        virtual bool unsubRTTransactions (std::uint64_t uListener) = 0;

        virtual bool subValidations (ref ispListener) = 0;
        virtual bool unsubValidations (std::uint64_t uListener) = 0;

        virtual bool subPeerStatus (ref ispListener) = 0;
        virtual bool unsubPeerStatus (std::uint64_t uListener) = 0;
        virtual void pubPeerStatus (std::function<Json::Value(void)> const&) = 0;

//vfalc todo删除
//这是为一个特定的合作伙伴添加的，它
//将订阅数据“推送”到特定的URL。
//
        virtual pointer findRpcSub (std::string const& strUrl) = 0;
        virtual pointer addRpcSub (std::string const& strUrl, ref rspEntry) = 0;
        virtual bool tryRemoveRpcSub (std::string const& strUrl) = 0;
    };

public:
    InfoSub (Source& source);
    InfoSub (Source& source, Consumer consumer);

    virtual ~InfoSub ();

    Consumer& getConsumer();

    virtual void send (Json::Value const& jvObj, bool broadcast) = 0;

    std::uint64_t getSeq ();

    void onSendEmpty ();

    void insertSubAccountInfo (
        AccountID const& account,
        bool rt);

    void deleteSubAccountInfo (
        AccountID const& account,
        bool rt);

    void clearPathRequest ();

    void setPathRequest (const std::shared_ptr<PathRequest>& req);

    std::shared_ptr <PathRequest> const& getPathRequest ();

protected:
    using LockType = std::mutex;
    using ScopedLockType = std::lock_guard <LockType>;
    LockType mLock;

private:
    Consumer                      m_consumer;
    Source&                       m_source;
    hash_set <AccountID> realTimeSubscriptions_;
    hash_set <AccountID> normalSubscriptions_;
    std::shared_ptr <PathRequest> mPathRequest;
    std::uint64_t                 mSeq;

    static
    int
    assign_id()
    {
        static std::atomic<std::uint64_t> id(0);
        return ++id;
    }
};

} //涟漪

#endif
