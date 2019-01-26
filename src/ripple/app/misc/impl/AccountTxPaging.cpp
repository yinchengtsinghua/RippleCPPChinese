
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

#include <ripple/app/ledger/LedgerToJson.h>
#include <ripple/app/ledger/LedgerMaster.h>
#include <ripple/app/main/Application.h>
#include <ripple/app/misc/Transaction.h>
#include <ripple/app/misc/impl/AccountTxPaging.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/UintTypes.h>
#include <boost/format.hpp>
#include <memory>

namespace ripple {

void
convertBlobsToTxResult (
    NetworkOPs::AccountTxs& to,
    std::uint32_t ledger_index,
    std::string const& status,
    Blob const& rawTxn,
    Blob const& rawMeta,
    Application& app)
{
    SerialIter it (makeSlice(rawTxn));
    auto txn = std::make_shared<STTx const> (it);
    std::string reason;

    auto tr = std::make_shared<Transaction> (txn, reason, app);

    tr->setStatus (Transaction::sqlTransactionStatus(status));
    tr->setLedger (ledger_index);

    auto metaset = std::make_shared<TxMeta> (
        tr->getID (), tr->getLedger (), rawMeta, app.journal ("TxMeta"));

    to.emplace_back(std::move(tr), metaset);
};

void
saveLedgerAsync (Application& app, std::uint32_t seq)
{
    if (auto l = app.getLedgerMaster().getLedgerBySeq(seq))
        pendSaveValidated(app, l, false, false);
}

void
accountTxPage (
    DatabaseCon& connection,
    AccountIDCache const& idCache,
    std::function<void (std::uint32_t)> const& onUnsavedLedger,
    std::function<void (std::uint32_t,
                        std::string const&,
                        Blob const&,
                        Blob const&)> const& onTransaction,
    AccountID const& account,
    std::int32_t minLedger,
    std::int32_t maxLedger,
    bool forward,
    Json::Value& token,
    int limit,
    bool bAdmin,
    std::uint32_t page_length)
{
    bool lookingForMarker = token.isObject();

    std::uint32_t numberOfResults;

    if (limit <= 0 || (limit > page_length && !bAdmin))
        numberOfResults = page_length;
    else
        numberOfResults = limit;

//因为一个账户可以有成千上万的交易，所以有一个限制
//放置在返回的事务量上。如果达到极限
//在结果集用完之前（我们总是再查询一个
//然后我们返回一个不透明标记，可以在
//后续查询。
    std::uint32_t queryLimit = numberOfResults + 1;
    std::uint32_t findLedger = 0, findSeq = 0;

    if (lookingForMarker)
    {
        try
        {
            if (!token.isMember(jss::ledger) || !token.isMember(jss::seq))
                return;
            findLedger = token[jss::ledger].asInt();
            findSeq = token[jss::seq].asInt();
        }
        catch (std::exception const&)
        {
            return;
        }
    }

//我们使用令牌引用来传递输入和输出，所以
//我们需要在两者之间清除它。
    token = Json::nullValue;

    static std::string const prefix (
        R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
          Status,RawTxn,TxnMeta
          FROM AccountTransactions INNER JOIN Transactions
          ON Transactions.TransID = AccountTransactions.TransID
          AND AccountTransactions.Account = '%s' WHERE
          )");

    std::string sql;

//SQL之间使用闭合间隔（[a，b]）

    if (forward && (findLedger == 0))
    {
        sql = boost::str (boost::format(
            prefix +
            (R"(AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u'
             ORDER BY AccountTransactions.LedgerSeq ASC,
             AccountTransactions.TxnSeq ASC
             LIMIT %u;)"))
            % idCache.toBase58(account)
            % minLedger
            % maxLedger
            % queryLimit);
    }
    else if (forward && (findLedger != 0))
    {
        auto b58acct = idCache.toBase58(account);
        sql = boost::str (boost::format(
            (R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
            Status,RawTxn,TxnMeta
            FROM AccountTransactions, Transactions WHERE
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u')
            OR
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq = '%u' AND
            AccountTransactions.TxnSeq >= '%u')
            ORDER BY AccountTransactions.LedgerSeq ASC,
            AccountTransactions.TxnSeq ASC
            LIMIT %u;
            )"))
        % b58acct
        % (findLedger + 1)
        % maxLedger
        % b58acct
        % findLedger
        % findSeq
        % queryLimit);
    }
    else if (!forward && (findLedger == 0))
    {
        sql = boost::str (boost::format(
            prefix +
            (R"(AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u'
             ORDER BY AccountTransactions.LedgerSeq DESC,
             AccountTransactions.TxnSeq DESC
             LIMIT %u;)"))
            % idCache.toBase58(account)
            % minLedger
            % maxLedger
            % queryLimit);
    }
    else if (!forward && (findLedger != 0))
    {
        auto b58acct = idCache.toBase58(account);
        sql = boost::str (boost::format(
            (R"(SELECT AccountTransactions.LedgerSeq,AccountTransactions.TxnSeq,
            Status,RawTxn,TxnMeta
            FROM AccountTransactions, Transactions WHERE
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq BETWEEN '%u' AND '%u')
            OR
            (AccountTransactions.TransID = Transactions.TransID AND
            AccountTransactions.Account = '%s' AND
            AccountTransactions.LedgerSeq = '%u' AND
            AccountTransactions.TxnSeq <= '%u')
            ORDER BY AccountTransactions.LedgerSeq DESC,
            AccountTransactions.TxnSeq DESC
            LIMIT %u;
            )"))
            % b58acct
            % minLedger
            % (findLedger - 1)
            % b58acct
            % findLedger
            % findSeq
            % queryLimit);
    }
    else
    {
        assert (false);
//SQL是空的
        return;
    }

    {
        auto db (connection.checkoutDb());

        Blob rawData;
        Blob rawMeta;

        boost::optional<std::uint64_t> ledgerSeq;
        boost::optional<std::uint32_t> txnSeq;
        boost::optional<std::string> status;
        soci::blob txnData (*db);
        soci::blob txnMeta (*db);
        soci::indicator dataPresent, metaPresent;

        soci::statement st = (db->prepare << sql,
            soci::into (ledgerSeq),
            soci::into (txnSeq),
            soci::into (status),
            soci::into (txnData, dataPresent),
            soci::into (txnMeta, metaPresent));

        st.execute ();

        while (st.fetch ())
        {
            if (lookingForMarker)
            {
                if (findLedger == ledgerSeq.value_or (0) &&
                    findSeq == txnSeq.value_or (0))
                {
                    lookingForMarker = false;
                }
            }
            else if (numberOfResults == 0)
            {
                token = Json::objectValue;
                token[jss::ledger] = rangeCheckedCast<std::uint32_t>(ledgerSeq.value_or (0));
                token[jss::seq] = txnSeq.value_or (0);
                break;
            }

            if (!lookingForMarker)
            {
                if (dataPresent == soci::i_ok)
                    convert (txnData, rawData);
                else
                    rawData.clear ();

                if (metaPresent == soci::i_ok)
                    convert (txnMeta, rawMeta);
                else
                    rawMeta.clear ();

//解决可能导致元数据丢失的错误
                if (rawMeta.size() == 0)
                    onUnsavedLedger(ledgerSeq.value_or (0));

                onTransaction(rangeCheckedCast<std::uint32_t>(ledgerSeq.value_or (0)),
                    *status, rawData, rawMeta);
                --numberOfResults;
            }
        }
    }

    return;
}

}
