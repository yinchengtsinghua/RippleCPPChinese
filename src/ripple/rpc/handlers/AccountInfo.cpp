
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


#include <ripple/app/main/Application.h>
#include <ripple/app/misc/TxQ.h>
#include <ripple/json/json_value.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/rpc/Context.h>
#include <ripple/rpc/impl/RPCHelpers.h>

namespace ripple {

//{
//帐户：<ident>，
//严格：<bool>>/可选（默认为false）
////如果为true，则只允许公钥和地址。
//分类帐哈希：<ledger>
//分类帐索引：<ledger索引>
//签名者列表：<bool>>/可选（默认为false）
////如果为真，则返回signerList。
//队列：<bool>>/可选（默认为false）
////如果为真，则返回有关事务的信息
////在当前txq中，仅当
////分类帐已打开。否则，如果为真，则返回
///错误。
//}

//托多（汤姆）：什么是“违约”？
Json::Value doAccountInfo (RPC::Context& context)
{
    auto& params = context.params;

    std::string strIdent;
    if (params.isMember(jss::account))
        strIdent = params[jss::account].asString();
    else if (params.isMember(jss::ident))
        strIdent = params[jss::ident].asString();
    else
        return RPC::missing_field_error (jss::account);

    std::shared_ptr<ReadView const> ledger;
    auto result = RPC::lookupLedger (ledger, context);

    if (!ledger)
        return result;

    bool bStrict = params.isMember (jss::strict) && params[jss::strict].asBool ();
    AccountID accountID;

//获取有关帐户的信息。

    auto jvAccepted = RPC::accountFromString (accountID, strIdent, bStrict);

    if (jvAccepted)
        return jvAccepted;

    auto const sleAccepted = ledger->read(keylet::account(accountID));
    if (sleAccepted)
    {
        auto const queue = params.isMember(jss::queue) &&
            params[jss::queue].asBool();

        if (queue && !ledger->open())
        {
//请求队列是没有意义的
//任何已关闭或已验证的分类帐。
            RPC::inject_error(rpcINVALID_PARAMS, result);
            return result;
        }

        RPC::injectSLE(jvAccepted, *sleAccepted);
        result[jss::account_data] = jvAccepted;

//如果请求，则返回签名者列表。
        if (params.isMember (jss::signer_lists) &&
            params[jss::signer_lists].asBool ())
        {
//由于预期的
//将来当我们在一个帐户上支持多个签名者列表时。
            Json::Value jvSignerList = Json::arrayValue;

//如果将来我们支持，则需要重新访问此代码
//一个帐户上有多个签名者。
            auto const sleSigners = ledger->read (keylet::signers (accountID));
            if (sleSigners)
                jvSignerList.append (sleSigners->getJson (0));

            result[jss::account_data][jss::signer_lists] =
                std::move(jvSignerList);
        }
//如果请求，则返回队列信息
        if (queue)
        {
            Json::Value jvQueueData = Json::objectValue;

            auto const txs = context.app.getTxQ().getAccountTxs(
                accountID, *ledger);
            if (!txs.empty())
            {
                jvQueueData[jss::txn_count] = static_cast<Json::UInt>(txs.size());
                jvQueueData[jss::lowest_sequence] = txs.begin()->first;
                jvQueueData[jss::highest_sequence] = txs.rbegin()->first;

                auto& jvQueueTx = jvQueueData[jss::transactions];
                jvQueueTx = Json::arrayValue;

                boost::optional<bool> anyAuthChanged(false);
                boost::optional<XRPAmount> totalSpend(0);

                for (auto const& tx : txs)
                {
                    Json::Value jvTx = Json::objectValue;

                    jvTx[jss::seq] = tx.first;
                    jvTx[jss::fee_level] = to_string(tx.second.feeLevel);
                    if (tx.second.lastValid)
                        jvTx[jss::LastLedgerSequence] = *tx.second.lastValid;
                    if (tx.second.consequences)
                    {
                        jvTx[jss::fee] = to_string(
                            tx.second.consequences->fee);
                        auto spend = tx.second.consequences->potentialSpend +
                            tx.second.consequences->fee;
                        jvTx[jss::max_spend_drops] = to_string(spend);
                        if (totalSpend)
                            *totalSpend += spend;
                        auto authChanged = tx.second.consequences->category ==
                            TxConsequences::blocker;
                        if (authChanged)
                            anyAuthChanged.emplace(authChanged);
                        jvTx[jss::auth_change] = authChanged;
                    }
                    else
                    {
                        if (anyAuthChanged && !*anyAuthChanged)
                            anyAuthChanged.reset();
                        totalSpend.reset();
                    }

                    jvQueueTx.append(std::move(jvTx));
                }

                if (anyAuthChanged)
                    jvQueueData[jss::auth_change_queued] =
                        *anyAuthChanged;
                if (totalSpend)
                    jvQueueData[jss::max_spend_drops_total] =
                        to_string(*totalSpend);
            }
            else
                jvQueueData[jss::txn_count] = 0u;

            result[jss::queue_data] = std::move(jvQueueData);
        }
    }
    else
    {
        result[jss::account] = context.app.accountIDCache().toBase58 (accountID);
        RPC::inject_error (rpcACT_NOT_FOUND, result);
    }

    return result;
}

} //涟漪
