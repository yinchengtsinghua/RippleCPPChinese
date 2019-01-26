
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

#ifndef RIPPLE_RPC_TRANSACTIONSIGN_H_INCLUDED
#define RIPPLE_RPC_TRANSACTIONSIGN_H_INCLUDED

#include <ripple/app/misc/NetworkOPs.h>
#include <ripple/rpc/Role.h>
#include <ripple/ledger/ApplyView.h>

namespace ripple {

//向前声明
class Application;
class LoadFeeTrack;
class Transaction;
class TxQ;

namespace RPC {

/*代表客户填写费用。
    当客户未明确指定费用时，将调用此函数。
    客户也可以对费用金额设定上限。这个天花板
    表示为基于当前分类帐费用计划的乘数。

    JSON域

    “费用”交易支付的费用。当客户
            希望填写费用。

    “费用乘数”应用于当前分类账交易的乘数。
                    限制服务器应自动填充的最大费用的费用。
                    如果未指定此可选字段，则默认
                    使用乘数。
    “Fee_Div_Max”应用于当前分类账交易的分隔符。
                    限制服务器应自动填充的最大费用的费用。
                    如果未指定此可选字段，则默认
                    使用分隔器（1）。fee_mult_max”和“fee_div_max”
                    都是这样使用的
                    以整数形式表示的'base*fee_mult_max/fee_div_max'。

    @param tx要填写的事务对应的JSON。
    @param ledger用于检索当前费用计划的分类帐。
    @param roll标识是否由管理终结点调用。

    @返回包含错误结果的JSON对象（如果有）
**/

Json::Value checkFee (
    Json::Value& request,
    Role const role,
    bool doAutoFill,
    Config const& config,
    LoadFeeTrack const& feeTrack,
    TxQ const& txQ,
    std::shared_ptr<OpenView const> const& ledger);

//返回一个调用networkops:：processTransaction的std:：function<>
using ProcessTransactionFn =
    std::function<void (std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, NetworkOPs::FailHard failType)>;

inline ProcessTransactionFn getProcessTxnFn (NetworkOPs& netOPs)
{
    return [&netOPs](std::shared_ptr<Transaction>& transaction,
        bool bUnlimited, bool bLocal, NetworkOPs::FailHard failType)
    {
        netOPs.processTransaction(transaction, bUnlimited, bLocal, failType);
    };
}

/*返回json:：objectValue。*/
Json::Value transactionSign (
Json::Value params,  //通过值传递，以便在本地修改它。
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app);

/*返回json:：objectValue。*/
Json::Value transactionSubmit (
Json::Value params,  //通过值传递，以便在本地修改它。
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction);

/*返回json:：objectValue。*/
Json::Value transactionSignFor (
Json::Value params,  //通过值传递，以便在本地修改它。
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app);

/*返回json:：objectValue。*/
Json::Value transactionSubmitMultiSigned (
Json::Value params,  //通过值传递，以便在本地修改它。
    NetworkOPs::FailHard failType,
    Role role,
    std::chrono::seconds validatedLedgerAge,
    Application& app,
    ProcessTransactionFn const& processTransaction);

} //RPC
} //涟漪

#endif
