
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
    版权所有（c）2012-2014 Ripple Labs Inc.

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

#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/paths/PathRequests.h>
#include <ripple/net/RPCErr.h>
#include <ripple/resource/Fees.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/LegacyPathFind.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//此接口已弃用。
Json::Value doRipplePathFind (RPC::Context& context)
{
    if (context.app.config().PATH_SEARCH_MAX == 0)
        return rpcError (rpcNOT_SUPPORTED);

    context.loadType = Resource::feeHighBurdenRPC;

    std::shared_ptr <ReadView const> lpLedger;
    Json::Value jvResult;

    if (! context.app.config().standalone() &&
        ! context.params.isMember(jss::ledger) &&
        ! context.params.isMember(jss::ledger_index) &&
        ! context.params.isMember(jss::ledger_hash))
    {
//未指定分类帐，请使用路径查找默认值
//并派往寻路引擎
        if (context.app.getLedgerMaster().getValidatedLedgerAge() >
            RPC::Tuning::maxValidatedLedgerAge)
        {
            return rpcError (rpcNO_NETWORK);
        }

        PathRequest::pointer request;
        lpLedger = context.ledgerMaster.getClosedLedger();

//这看起来不太奇怪，但你应该
//请注意，此代码在jobqueue:：coro中运行，这是一个coroutine。
//我们可能在线程之间来回切换。以下是概述：
//
//1。由于调用了
//波纹路径找到。DorippePathFind（）当前正在运行
//在jobqueue:：coro中使用jobqueue线程。
//
//2。DorippePathFind调用MakeLegacyPathRequest（）将
//路径查找请求。这个请求可能会在
//不确定（可能不同）作业队列的未来时间
//线程。
//
//三。作为该路径查找JobQueue线程的延续，
//Coroutine我们目前正在运行！！）发布到
//JobQueue。因为这是一个延续，那篇文章不会
//在路径查找请求完成之前发生。
//
//4。一旦继续排队，我们就有理由思考
//寻找路径的工作可能会运行，然后我们的协同程序
//在yield（）s中运行。这意味着它放弃了线程
//JobQueue。连体衣暂停了，但准备跑了，
//因为它在
//路径查找继续。
//
//5。如果一切正常，则在JobQueue线程上运行路径查找
//并执行其延续。续贴此
//相同的连体衣！！）到作业队列。
//
//6。当jobqueue调用此coroutine时，此coroutine将恢复
//从coro->yield（）下面的行返回
//路径查找结果。
//
//有这么多运动部件，会出什么问题？
//
//就工作队列而言，在关闭时拒绝添加作业
//有两件具体的事情可能会出错。
//
//1。makeLegacyPathRequest（）排队的路径查找作业可能是
//拒绝（因为我们要关闭）。
//
//幸运的是，这个问题可以通过查看
//makeLegacyPathRequest（）的返回值。如果
//makeLegacyPathRequest（）无法获取运行路径查找的线程
//打开，然后返回空请求。
//
//2。路径查找作业可能会运行，但coro:：post（）可能是
//被作业队列拒绝（因为我们正在关闭）。
//
//我们通过恢复（而不是发布）CORO来处理这个案件。
//通过恢复CORO，我们允许CORO运行到完成
//在当前线程上，而不是要求它在
//作业队列中的新线程。
//
//这两种故障模式都很难在单元测试中重新创建。
//因为它们如此依赖于线程间的计时。然而
//故障模式可以通过同步（在
//波纹源代码）关闭应用程序。代码到
//如下所示：
//
//context.app.signalStop（）；
//而（！）context.app.getJobQueue（）.jobCounter（）.joined（））
//
//第一行启动关闭应用程序的过程。
//第二行等待，直到没有更多的作业可以添加到
//作业队列，然后让线程继续。
//
//2017年5月
        jvResult = context.app.getPathRequests().makeLegacyPathRequest (
            request,
            [&context] () {
//复制共享资源可以使协同程序保持活动状态
//通过返回。否则存储在
//当我们从
//corocopy->resume（）。这不是绝对必要的，但是
//将使维护更容易。
                std::shared_ptr<JobQueue::Coro> coroCopy {context.coro};
                if (!coroCopy->post())
                {
//post（）失败，因此我们无法让线程
//科罗完成。我们将调用coro:：resume（），因此
//科罗可以完成我们的任务。否则
//应用程序将在关闭时挂起。
                    coroCopy->resume();
                }
            },
            context.consumer, lpLedger, context.params);
        if (request)
        {
            context.coro->yield();
            jvResult = request->doStatus (context.params);
        }

        return jvResult;
    }

//调用者指定了一个分类帐
    jvResult = RPC::lookupLedger (lpLedger, context);
    if (! lpLedger)
        return jvResult;

    RPC::LegacyPathFind lpf (isUnlimited (context.role), context.app);
    if (! lpf.isOk ())
        return rpcError (rpcTOO_BUSY);

    auto result = context.app.getPathRequests().doLegacyPathRequest (
        context.consumer, lpLedger, context.params);

    for (auto &fieldName : jvResult.getMemberNames ())
        result[fieldName] = std::move (jvResult[fieldName]);

    return result;
}

} //涟漪
