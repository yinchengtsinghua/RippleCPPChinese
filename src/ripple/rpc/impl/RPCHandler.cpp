
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

#include <ripple/app/main/Application.h>
#include <ripple/rpc/RPCHandler.h>
#include <ripple/rpc/impl/Tuning.h>
#include <ripple/rpc/impl/Handler.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/PerfLog.h>
#include <ripple/core/Config.h>
#include <ripple/core/JobQueue.h>
#include <ripple/json/Object.h>
#include <ripple/json/to_string.h>
#include <ripple/net/InfoSub.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Role.h>
#include <ripple/resource/Fees.h>
#include <atomic>
#include <chrono>

namespace ripple {
namespace RPC {

namespace {

/*
   从HTTP RPC处理程序和WebSockets调用此代码。

   返回的JSON的形式在这两个服务之间有些不同。

   HTML：
     成功：
        {
           “结果”：{
              “分类帐”：{
                 “已接受”：错误，
                 “事务_hash”：“…”
              }
              “分类帐索引”：10300865，
              “已验证”：错误，
              “status”：“success”状态在结果中。
           }
        }

     失败：
        {
           “结果”：{
              “错误”：“非网络”，
              “错误代码”：16，
              “error_message”：“未同步到Ripple网络。”，
              “请求”：{
                 “command”：“账本”，
                 “分类帐索引”：10300865
              }
              “status”：“错误”
           }
        }

   Websocket：
     成功：
        {
           “结果”：{
              “分类帐”：{
                 “已接受”：错误，
                 “事务_hash”：“…”
              }
              “分类帐索引”：10300865，
              “已验证”：错误
           }
           “type”：“响应”，
           “status”：“成功”，状态在结果之外！
           “id”：“客户端ID”，可选
           “警告”：3.14可选
        }

     失败：
        {
          “错误”：“非网络”，
          “错误代码”：16，
          “error_message”：“未同步到Ripple网络。”，
          “请求”：{
             “command”：“账本”，
             “分类帐索引”：10300865
          }
          “type”：“响应”，
          “status”：“错误”，
          “id”：“客户端id”可选
        }

 **/


error_code_i fillHandler (Context& context,
                          Handler const * & result)
{
    if (! isUnlimited (context.role))
    {
//vfalc注意：我们还应该添加jtrpc作业吗？
//
        int jc = context.app.getJobQueue ().getJobCountGE (jtCLIENT);
        if (jc > Tuning::maxJobQueueClients)
        {
            JLOG (context.j.debug()) << "Too busy for command: " << jc;
            return rpcTOO_BUSY;
        }
    }

    if (!context.params.isMember(jss::command) && !context.params.isMember(jss::method))
        return rpcCOMMAND_MISSING;
    if (context.params.isMember(jss::command) && context.params.isMember(jss::method))
    {
        if (context.params[jss::command].asString() !=
            context.params[jss::method].asString())
            return rpcUNKNOWN_COMMAND;
    }

    std::string strCommand  = context.params.isMember(jss::command) ?
                              context.params[jss::command].asString() :
                              context.params[jss::method].asString();

    JLOG (context.j.trace()) << "COMMAND:" << strCommand;
    JLOG (context.j.trace()) << "REQUEST:" << context.params;
    auto handler = getHandler(strCommand);

    if (!handler)
        return rpcUNKNOWN_COMMAND;

    if (handler->role_ == Role::ADMIN && context.role != Role::ADMIN)
        return rpcNO_PERMISSION;

    if ((handler->condition_ & NEEDS_NETWORK_CONNECTION) &&
        (context.netOps.getOperatingMode () < NetworkOPs::omSYNCING))
    {
        JLOG (context.j.info())
            << "Insufficient network mode for RPC: "
            << context.netOps.strOperatingMode ();

        return rpcNO_NETWORK;
    }

    if (context.app.getOPs().isAmendmentBlocked() &&
         (handler->condition_ & NEEDS_CURRENT_LEDGER ||
          handler->condition_ & NEEDS_CLOSED_LEDGER))
    {
        return rpcAMENDMENT_BLOCKED;
    }

    if (!context.app.config().standalone() &&
        handler->condition_ & NEEDS_CURRENT_LEDGER)
    {
        if (context.ledgerMaster.getValidatedLedgerAge () >
            Tuning::maxValidatedLedgerAge)
        {
            return rpcNO_CURRENT;
        }

        auto const cID = context.ledgerMaster.getCurrentLedgerIndex ();
        auto const vID = context.ledgerMaster.getValidLedgerIndex ();

        if (cID + 10 < vID)
        {
            JLOG (context.j.debug()) << "Current ledger ID(" << cID <<
                ") is less than validated ledger ID(" << vID << ")";
            return rpcNO_CURRENT;
        }
    }

    if ((handler->condition_ & NEEDS_CLOSED_LEDGER) &&
        !context.ledgerMaster.getClosedLedger ())
    {
        return rpcNO_CLOSED;
    }

    result = handler;
    return rpcSUCCESS;
}

template <class Object, class Method>
Status callMethod (
    Context& context, Method method, std::string const& name, Object& result)
{
    static std::atomic<std::uint64_t> requestId {0};
    auto& perfLog = context.app.getPerfLog();
    std::uint64_t const curId = ++requestId;
    try
    {
        perfLog.rpcStart(name, curId);
        auto v = context.app.getJobQueue().makeLoadEvent(
            jtGENERIC, "cmd:" + name);

        auto ret = method (context, result);
        perfLog.rpcFinish(name, curId);
        return ret;
    }
    catch (std::exception& e)
    {
        perfLog.rpcError(name, curId);
        JLOG (context.j.info()) << "Caught throw: " << e.what ();

        if (context.loadType == Resource::feeReferenceRPC)
            context.loadType = Resource::feeExceptionRPC;

        inject_error (rpcINTERNAL, result);
        return rpcINTERNAL;
    }
}

template <class Method, class Object>
void getResult (
    Context& context, Method method, Object& object, std::string const& name)
{
    auto&& result = Json::addObject (object, jss::result);
    if (auto status = callMethod (context, method, name, result))
    {
        JLOG (context.j.debug()) << "rpcError: " << status.toString();
        result[jss::status] = jss::error;

        auto rq = context.params;

        if (rq.isObject())
        {
            if (rq.isMember(jss::passphrase.c_str()))
                rq[jss::passphrase.c_str()] = "<masked>";
            if (rq.isMember(jss::secret.c_str()))
                rq[jss::secret.c_str()] = "<masked>";
            if (rq.isMember(jss::seed.c_str()))
                rq[jss::seed.c_str()] = "<masked>";
            if (rq.isMember(jss::seed_hex.c_str()))
                rq[jss::seed_hex.c_str()] = "<masked>";
        }

        result[jss::request] = rq;
    }
    else
    {
        result[jss::status] = jss::success;
    }
}

} //命名空间

Status doCommand (
    RPC::Context& context, Json::Value& result)
{
    Handler const * handler = nullptr;
    if (auto error = fillHandler (context, handler))
    {
        inject_error (error, result);
        return error;
    }

    if (auto method = handler->valueMethod_)
    {
        if (! context.headers.user.empty() ||
            ! context.headers.forwardedFor.empty())
        {
            JLOG(context.j.debug()) << "start command: " << handler->name_ <<
                ", X-User: " << context.headers.user << ", X-Forwarded-For: " <<
                    context.headers.forwardedFor;

            auto ret = callMethod (context, method, handler->name_, result);

            JLOG(context.j.debug()) << "finish command: " << handler->name_ <<
                ", X-User: " << context.headers.user << ", X-Forwarded-For: " <<
                    context.headers.forwardedFor;

            return ret;
        }
        else
        {
            return callMethod (context, method, handler->name_, result);
        }
    }

    return rpcUNKNOWN_COMMAND;
}

Role roleRequired (std::string const& method)
{
    auto handler = RPC::getHandler(method);

    if (!handler)
        return Role::FORBID;

    return handler->role_;
}

} //RPC
} //涟漪
