
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

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/basics/Log.h>
#include <ripple/net/RPCErr.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>
#include <ripple/rpc/Role.h>

namespace ripple {

Json::Value doUnsubscribe (RPC::Context& context)
{

    InfoSub::pointer ispSub;
    Json::Value jvResult (Json::objectValue);
    bool removeUrl {false};

    if (! context.infoSub && ! context.params.isMember(jss::url))
    {
//必须是JSON-RPC调用。
        return rpcError(rpcINVALID_PARAMS);
    }

    if (context.params.isMember(jss::url))
    {
        if (context.role != Role::ADMIN)
            return rpcError(rpcNO_PERMISSION);

        std::string strUrl = context.params[jss::url].asString ();
        ispSub = context.netOps.findRpcSub (strUrl);
        if (! ispSub)
            return jvResult;
        removeUrl = true;
    }
    else
    {
        ispSub = context.infoSub;
    }

    if (context.params.isMember (jss::streams))
    {
        if (! context.params[jss::streams].isArray())
            return rpcError (rpcINVALID_PARAMS);

        for (auto& it: context.params[jss::streams])
        {
            if (! it.isString())
                return rpcError(rpcSTREAM_MALFORMED);

            std::string streamName = it.asString ();
            if (streamName == "server")
            {
                context.netOps.unsubServer (ispSub->getSeq ());
            }
            else if (streamName == "ledger")
            {
                context.netOps.unsubLedger (ispSub->getSeq ());
            }
            else if (streamName == "manifests")
            {
                context.netOps.unsubManifests (ispSub->getSeq ());
            }
            else if (streamName == "transactions")
            {
                context.netOps.unsubTransactions (ispSub->getSeq ());
            }
            else if (streamName == "transactions_proposed"
|| streamName == "rt_transactions") //贬低
            {
                context.netOps.unsubRTTransactions (ispSub->getSeq ());
            }
            else if (streamName == "validations")
            {
                context.netOps.unsubValidations (ispSub->getSeq ());
            }
            else if (streamName == "peer_status")
            {
                context.netOps.unsubPeerStatus (ispSub->getSeq ());
            }
            else
            {
                return rpcError(rpcSTREAM_MALFORMED);
            }
        }
    }

    auto accountsProposed = context.params.isMember(jss::accounts_proposed)
? jss::accounts_proposed : jss::rt_accounts;  //贬低
    if (context.params.isMember(accountsProposed))
    {
        if (! context.params[accountsProposed].isArray())
            return rpcError(rpcINVALID_PARAMS);

        auto ids = RPC::parseAccountIds(context.params[accountsProposed]);
        if (ids.empty())
            return rpcError(rpcACT_MALFORMED);
        context.netOps.unsubAccount(ispSub, ids, true);
    }

    if (context.params.isMember(jss::accounts))
    {
        if (! context.params[jss::accounts].isArray())
            return rpcError(rpcINVALID_PARAMS);

        auto ids = RPC::parseAccountIds(context.params[jss::accounts]);
        if (ids.empty())
            return rpcError(rpcACT_MALFORMED);
        context.netOps.unsubAccount(ispSub, ids, false);
    }

    if (context.params.isMember(jss::books))
    {
        if (! context.params[jss::books].isArray())
            return rpcError(rpcINVALID_PARAMS);

        for (auto& jv: context.params[jss::books])
        {
            if (! jv.isObject() ||
                ! jv.isMember(jss::taker_pays) ||
                ! jv.isMember(jss::taker_gets) ||
                ! jv[jss::taker_pays].isObjectOrNull() ||
                ! jv[jss::taker_gets].isObjectOrNull())
            {
                return rpcError(rpcINVALID_PARAMS);
            }

            Json::Value taker_pays = jv[jss::taker_pays];
            Json::Value taker_gets = jv[jss::taker_gets];

            Book book;

//分析强制货币。
            if (!taker_pays.isMember (jss::currency)
                || !to_currency (
                    book.in.currency, taker_pays[jss::currency].asString ()))
            {
                JLOG (context.j.info()) << "Bad taker_pays currency.";
                return rpcError (rpcSRC_CUR_MALFORMED);
            }
//分析可选颁发者。
            else if (((taker_pays.isMember (jss::issuer))
                      && (!taker_pays[jss::issuer].isString ()
                          || !to_issuer (
                              book.in.account, taker_pays[jss::issuer].asString ())))
//不允许非法发行人。
                     || !isConsistent (book.in)
                     || noAccount() == book.in.account)
            {
                JLOG (context.j.info()) << "Bad taker_pays issuer.";

                return rpcError (rpcSRC_ISR_MALFORMED);
            }

//分析强制货币。
            if (!taker_gets.isMember (jss::currency)
                    || !to_currency (book.out.currency,
                                     taker_gets[jss::currency].asString ()))
            {
                JLOG (context.j.info()) << "Bad taker_gets currency.";

                return rpcError (rpcDST_AMT_MALFORMED);
            }
//分析可选颁发者。
            else if (((taker_gets.isMember (jss::issuer))
                      && (!taker_gets[jss::issuer].isString ()
                          || !to_issuer (book.out.account,
                                         taker_gets[jss::issuer].asString ())))
//不允许非法发行人。
                     || !isConsistent (book.out)
                     || noAccount() == book.out.account)
            {
                JLOG (context.j.info()) << "Bad taker_gets issuer.";

                return rpcError (rpcDST_ISR_MALFORMED);
            }

            if (book.in == book.out)
            {
                JLOG (context.j.info())
                    << "taker_gets same as taker_pays.";
                return rpcError (rpcBAD_MARKET);
            }

            context.netOps.unsubBook (ispSub->getSeq (), book);

//双方都不赞成。
            if ((jv.isMember(jss::both) && jv[jss::both].asBool()) ||
                (jv.isMember(jss::both_sides) && jv[jss::both_sides].asBool()))
            {
                context.netOps.unsubBook(ispSub->getSeq(), reversed(book));
            }
        }
    }

    if (removeUrl)
    {
        context.netOps.tryRemoveRpcSub(context.params[jss::url].asString ());
    }

    return jvResult;
}

} //涟漪
