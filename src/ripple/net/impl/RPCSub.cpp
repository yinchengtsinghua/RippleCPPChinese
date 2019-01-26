
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

#include <ripple/net/RPCSub.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/to_string.h>
#include <ripple/net/RPCCall.h>
#include <deque>

namespace ripple {

//JSON-RPC的订阅对象
class RPCSubImp
    : public RPCSub
{
public:
    RPCSubImp (InfoSub::Source& source, boost::asio::io_service& io_service,
        JobQueue& jobQueue, std::string const& strUrl, std::string const& strUsername,
             std::string const& strPassword, Logs& logs)
        : RPCSub (source)
        , m_io_service (io_service)
        , m_jobQueue (jobQueue)
        , mUrl (strUrl)
        , mSSL (false)
        , mUsername (strUsername)
        , mPassword (strPassword)
        , mSending (false)
        , j_ (logs.journal ("RPCSub"))
        , logs_ (logs)
    {
        parsedURL pUrl;

        if (!parseUrl (pUrl, strUrl))
            Throw<std::runtime_error> ("Failed to parse url.");
        else if (pUrl.scheme == "https")
            mSSL = true;
        else if (pUrl.scheme != "http")
            Throw<std::runtime_error> ("Only http and https is supported.");

        mSeq = 1;

        mIp = pUrl.domain;
        mPort = (! pUrl.port) ? (mSSL ? 443 : 80) : *pUrl.port;
        mPath = pUrl.path;

        JLOG (j_.info()) <<
            "RPCCall::fromNetwork sub: ip=" << mIp <<
            " port=" << mPort <<
            " ssl= "<< (mSSL ? "yes" : "no") <<
            " path='" << mPath << "'";
    }

    ~RPCSubImp() = default;

    void send (Json::Value const& jvObj, bool broadcast) override
    {
        ScopedLockType sl (mLock);

        if (mDeque.size () >= eventQueueMax)
        {
//删除上一个事件。
            JLOG (j_.warn()) << "RPCCall::fromNetwork drop";
            mDeque.pop_back ();
        }

        auto jm = broadcast ? j_.debug() : j_.info();
        JLOG (jm) <<
            "RPCCall::fromNetwork push: " << jvObj;

        mDeque.push_back (std::make_pair (mSeq++, jvObj));

        if (!mSending)
        {
//启动发送线程。
            JLOG (j_.info()) << "RPCCall::fromNetwork start";

            mSending = m_jobQueue.addJob (
                jtCLIENT, "RPCSub::sendThread", [this] (Job&) {
                    sendThread();
                });
        }
    }

    void setUsername (std::string const& strUsername) override
    {
        ScopedLockType sl (mLock);

        mUsername = strUsername;
    }

    void setPassword (std::string const& strPassword) override
    {
        ScopedLockType sl (mLock);

        mPassword = strPassword;
    }

private:
//XXX可以在一次锁中创建一组发送作业。
    void sendThread ()
    {
        Json::Value jvEvent;
        bool bSend;

        do
        {
            {
//获取锁以操作队列和更改发送。
                ScopedLockType sl (mLock);

                if (mDeque.empty ())
                {
                    mSending    = false;
                    bSend       = false;
                }
                else
                {
                    std::pair<int, Json::Value> pEvent  = mDeque.front ();

                    mDeque.pop_front ();

                    jvEvent     = pEvent.second;
                    jvEvent["seq"]  = pEvent.first;

                    bSend       = true;
                }
            }

//发送到锁外。
            if (bSend)
            {
//在一次尝试中，XXX可能不需要这个。
                try
                {
                    JLOG (j_.info()) << "RPCCall::fromNetwork: " << mIp;

                    RPCCall::fromNetwork (
                        m_io_service,
                        mIp, mPort,
                        mUsername, mPassword,
                        mPath, "event",
                        jvEvent,
                        mSSL,
                        true,
                        logs_);
                }
                catch (const std::exception& e)
                {
                    JLOG (j_.info()) << "RPCCall::fromNetwork exception: " << e.what ();
                }
            }
        }
        while (bSend);
    }

private:
    enum
    {
        eventQueueMax = 32
    };

    boost::asio::io_service& m_io_service;
    JobQueue& m_jobQueue;

    std::string             mUrl;
    std::string             mIp;
    std::uint16_t           mPort;
    bool                    mSSL;
    std::string             mUsername;
    std::string             mPassword;
    std::string             mPath;

int                     mSeq;                       //下一个要分配的ID。

bool                    mSending;                   //发送三线已激活。

    std::deque<std::pair<int, Json::Value> >    mDeque;

    beast::Journal j_;
    Logs& logs_;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

RPCSub::RPCSub (InfoSub::Source& source)
    : InfoSub (source, Consumer())
{
}

std::shared_ptr<RPCSub> make_RPCSub (
    InfoSub::Source& source, boost::asio::io_service& io_service,
    JobQueue& jobQueue, std::string const& strUrl,
    std::string const& strUsername, std::string const& strPassword,
    Logs& logs)
{
    return std::make_shared<RPCSubImp> (std::ref (source),
        std::ref (io_service), std::ref (jobQueue),
            strUrl, strUsername, strPassword, logs);
}

} //涟漪
