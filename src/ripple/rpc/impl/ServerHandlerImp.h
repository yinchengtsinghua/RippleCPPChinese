
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

#ifndef RIPPLE_RPC_SERVERHANDLERIMP_H_INCLUDED
#define RIPPLE_RPC_SERVERHANDLERIMP_H_INCLUDED

#include <ripple/core/JobQueue.h>
#include <ripple/rpc/impl/WSInfoSub.h>
#include <ripple/server/Server.h>
#include <ripple/server/Session.h>
#include <ripple/server/WSSession.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/app/main/CollectorManager.h>
#include <ripple/json/Output.h>
#include <map>
#include <mutex>
#include <vector>

namespace ripple {

inline
bool operator< (Port const& lhs, Port const& rhs)
{
    return lhs.name < rhs.name;
}

class ServerHandlerImp
    : public Stoppable
{
public:
    struct Setup
    {
        explicit Setup() = default;

        std::vector<Port> ports;

//成员空间
        struct client_t
        {
            explicit client_t() = default;

            bool secure = false;
            std::string ip;
            std::uint16_t port = 0;
            std::string user;
            std::string password;
            std::string admin_user;
            std::string admin_password;
        };

//在担任客户端角色时的配置
        client_t client;

//覆盖的配置
        struct overlay_t
        {
            explicit overlay_t() = default;

            boost::asio::ip::address ip;
            std::uint16_t port = 0;
        };

        overlay_t overlay;

        void
        makeContexts();
    };

private:

    Application& app_;
    Resource::Manager& m_resourceManager;
    beast::Journal m_journal;
    NetworkOPs& m_networkOPs;
    std::unique_ptr<Server> m_server;
    Setup setup_;
    JobQueue& m_jobQueue;
    beast::insight::Counter rpc_requests_;
    beast::insight::Event rpc_size_;
    beast::insight::Event rpc_time_;
    std::mutex countlock_;
    std::map<std::reference_wrapper<Port const>, int> count_;

public:
    ServerHandlerImp (Application& app, Stoppable& parent,
        boost::asio::io_service& io_service, JobQueue& jobQueue,
            NetworkOPs& networkOPs, Resource::Manager& resourceManager,
                CollectorManager& cm);

    ~ServerHandlerImp();

    using Output = Json::Output;

    void
    setup (Setup const& setup, beast::Journal journal);

    Setup const&
    setup() const
    {
        return setup_;
    }

//
//可停止的
//

    void
    onStop() override;

//
//处理程序
//

    bool
    onAccept (Session& session,
        boost::asio::ip::tcp::endpoint endpoint);

    Handoff
    onHandoff(
        Session& session,
        std::unique_ptr<beast::asio::ssl_bundle>&& bundle,
        http_request_type&& request,
        boost::asio::ip::tcp::endpoint const& remote_address);

    Handoff
    onHandoff(
        Session& session,
        http_request_type&& request,
        boost::asio::ip::tcp::endpoint const& remote_address)
    {
        return onHandoff(
            session,
            {},
            std::forward<http_request_type>(request),
            remote_address);
    }

    void
    onRequest (Session& session);

    void
    onWSMessage(std::shared_ptr<WSSession> session,
        std::vector<boost::asio::const_buffer> const& buffers);

    void
    onClose (Session& session,
        boost::system::error_code const&);

    void
    onStopped (Server&);

private:
    Json::Value
    processSession(
        std::shared_ptr<WSSession> const& session,
            std::shared_ptr<JobQueue::Coro> const& coro,
                Json::Value const& jv);

    void
    processSession (std::shared_ptr<Session> const&,
        std::shared_ptr<JobQueue::Coro> coro);

    void
    processRequest (Port const& port, std::string const& request,
        beast::IP::Endpoint const& remoteIPAddress, Output&&,
        std::shared_ptr<JobQueue::Coro> coro,
        std::string forwardedFor, std::string user);

    Handoff
    statusResponse(http_request_type const& request) const;
};

}

#endif
