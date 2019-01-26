
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
    版权所有（c）2012-2015 Ripple Labs Inc.

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
#include <ripple/app/misc/TxQ.h>
#include <ripple/basics/base_uint.h>
#include <ripple/basics/date.h>

namespace ripple {

namespace {

bool isFull(LedgerFill const& fill)
{
    return fill.options & LedgerFill::full;
}

bool isExpanded(LedgerFill const& fill)
{
    return isFull(fill) || (fill.options & LedgerFill::expand);
}

bool isBinary(LedgerFill const& fill)
{
    return fill.options & LedgerFill::binary;
}

template <class Object>
void fillJson(Object& json, bool closed, LedgerInfo const& info, bool bFull)
{
    json[jss::parent_hash]  = to_string (info.parentHash);
    json[jss::ledger_index] = to_string (info.seq);
json[jss::seqNum]       = to_string (info.seq);      //贬低

    if (closed)
    {
        json[jss::closed] = true;
    }
    else if (! bFull)
    {
        json[jss::closed] = false;
        return;
    }

    json[jss::ledger_hash] = to_string (info.hash);
    json[jss::transaction_hash] = to_string (info.txHash);
    json[jss::account_hash] = to_string (info.accountHash);
    json[jss::total_coins] = to_string (info.drops);

//接下来的三个被否决。
    json[jss::hash] = to_string (info.hash);
    json[jss::totalCoins] = to_string (info.drops);
    json[jss::accepted] = closed;
    json[jss::close_flags] = info.closeFlags;

//始终显示构成分类帐哈希的字段
    json[jss::parent_close_time] = info.parentCloseTime.time_since_epoch().count();
    json[jss::close_time] = info.closeTime.time_since_epoch().count();
    json[jss::close_time_resolution] = info.closeTimeResolution.count();

    if (info.closeTime != NetClock::time_point{})
    {
        json[jss::close_time_human] = to_string(info.closeTime);
        if (! getCloseAgree(info))
            json[jss::close_time_estimated] = true;
    }
}

template <class Object>
void fillJsonBinary(Object& json, bool closed, LedgerInfo const& info)
{
    if (! closed)
        json[jss::closed] = false;
    else
    {
        json[jss::closed] = true;

        Serializer s;
        addRaw (info, s);
        json[jss::ledger_data] = strHex (s.peekData ());
    }
}

Json::Value fillJsonTx (LedgerFill const& fill,
    bool bBinary, bool bExpanded,
        std::pair<std::shared_ptr<STTx const>,
            std::shared_ptr<STObject const>> const i)
{
    if (! bExpanded)
    {
        return to_string(i.first->getTransactionID());
    }
    else
    {
        Json::Value txJson{ Json::objectValue };
        if (bBinary)
        {
            txJson[jss::tx_blob] = serializeHex(*i.first);
            if (i.second)
                txJson[jss::meta] = serializeHex(*i.second);
        }
        else
        {
            copyFrom(txJson, i.first->getJson(0));
            if (i.second)
                txJson[jss::metaData] = i.second->getJson(0);
        }

        if ((fill.options & LedgerFill::ownerFunds) &&
            i.first->getTxnType() == ttOFFER_CREATE)
        {
            auto const account = i.first->getAccountID(sfAccount);
            auto const amount = i.first->getFieldAmount(sfTakerGets);

//如果创建的报价不是自筹资金，则添加
//业主平衡
            if (account != amount.getIssuer())
            {
                auto const ownerFunds = accountFunds(fill.ledger,
                    account, amount, fhIGNORE_FREEZE,
                    beast::Journal {beast::Journal::getNullSink()});
                txJson[jss::owner_funds] = ownerFunds.getText ();
            }
        }
        return txJson;
    }
}

template <class Object>
void fillJsonTx (Object& json, LedgerFill const& fill)
{
    auto&& txns = setArray (json, jss::transactions);
    auto bBinary = isBinary(fill);
    auto bExpanded = isExpanded(fill);

    try
    {
        for (auto& i: fill.ledger.txs)
        {
            txns.append(fillJsonTx(fill, bBinary, bExpanded, i));
        }
    }
    catch (std::exception const&)
    {
//用户对此无能为力。
    }
}

template <class Object>
void fillJsonState(Object& json, LedgerFill const& fill)
{
    auto& ledger = fill.ledger;
    auto&& array = Json::setArray (json, jss::accountState);
    auto expanded = isExpanded(fill);
    auto binary = isBinary(fill);

    for(auto const& sle : ledger.sles)
    {
        if (fill.type == ltINVALID || sle->getType () == fill.type)
        {
            if (binary)
            {
                auto&& obj = appendObject(array);
                obj[jss::hash] = to_string(sle->key());
                obj[jss::tx_blob] = serializeHex(*sle);
            }
            else if (expanded)
                array.append(sle->getJson(0));
            else
                array.append(to_string(sle->key()));
        }
    }
}

template <class Object>
void fillJsonQueue(Object& json, LedgerFill const& fill)
{
    auto&& queueData = Json::setArray(json, jss::queue_data);
    auto bBinary = isBinary(fill);
    auto bExpanded = isExpanded(fill);

    for (auto const& tx : fill.txQueue)
    {
        auto&& txJson = appendObject(queueData);
        txJson[jss::fee_level] = to_string(tx.feeLevel);
        if (tx.lastValid)
            txJson[jss::LastLedgerSequence] = *tx.lastValid;
        if (tx.consequences)
        {
            txJson[jss::fee] = to_string(
                tx.consequences->fee);
            auto spend = tx.consequences->potentialSpend +
                tx.consequences->fee;
            txJson[jss::max_spend_drops] = to_string(spend);
            auto authChanged = tx.consequences->category ==
                TxConsequences::blocker;
            txJson[jss::auth_change] = authChanged;
        }

        txJson[jss::account] = to_string(tx.account);
        txJson["retries_remaining"] = tx.retriesRemaining;
        txJson["preflight_result"] = transToken(tx.preflightResult);
        if (tx.lastResult)
            txJson["last_result"] = transToken(*tx.lastResult);

        txJson[jss::tx] = fillJsonTx(fill, bBinary, bExpanded,
            std::make_pair(tx.txn, nullptr));
    }
}

template <class Object>
void fillJson (Object& json, LedgerFill const& fill)
{
//托多：如果B二进制和B抽取都设置好了会发生什么？
//有没有办法把这个报告回去？
    auto bFull = isFull(fill);
    if (isBinary(fill))
        fillJsonBinary(json, ! fill.ledger.open(), fill.ledger.info());
    else
        fillJson(json, ! fill.ledger.open(), fill.ledger.info(), bFull);

    if (bFull || fill.options & LedgerFill::dumpTxrp)
        fillJsonTx(json, fill);

    if (bFull || fill.options & LedgerFill::dumpState)
        fillJsonState(json, fill);
}

} //命名空间

void addJson (Json::Value& json, LedgerFill const& fill)
{
    auto&& object = Json::addObject (json, jss::ledger);
    fillJson (object, fill);

    if ((fill.options & LedgerFill::dumpQueue) && !fill.txQueue.empty())
        fillJsonQueue(json, fill);
}

Json::Value getJson (LedgerFill const& fill)
{
    Json::Value json;
    fillJson (json, fill);
    return json;
}

} //涟漪
