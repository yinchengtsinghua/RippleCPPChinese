
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
#include <ripple/app/ledger/AcceptedLedgerTx.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/UintTypes.h>

namespace ripple {

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    std::shared_ptr<STObject const> const& met,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mMeta (std::make_shared<TxMeta> (
        txn->getTransactionID(), ledger->seq(), *met, logs.journal ("View")))
    , mAffected (mMeta->getAffectedAccounts ())
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (! ledger->open());

    mResult = mMeta->getResultTER ();

    Serializer s;
    met->add(s);
    mRawMeta = std::move (s.modData());

    buildJson ();
}

AcceptedLedgerTx::AcceptedLedgerTx (
    std::shared_ptr<ReadView const> const& ledger,
    std::shared_ptr<STTx const> const& txn,
    TER result,
    AccountIDCache const& accountCache,
    Logs& logs)
    : mLedger (ledger)
    , mTxn (txn)
    , mResult (result)
    , mAffected (txn->getMentionedAccounts ())
    , accountCache_ (accountCache)
    , logs_ (logs)
{
    assert (ledger->open());
    buildJson ();
}

std::string AcceptedLedgerTx::getEscMeta () const
{
    assert (!mRawMeta.empty ());
    return sqlEscape (mRawMeta);
}

void AcceptedLedgerTx::buildJson ()
{
    mJson = Json::objectValue;
    mJson[jss::transaction] = mTxn->getJson (0);

    if (mMeta)
    {
        mJson[jss::meta] = mMeta->getJson (0);
        mJson[jss::raw_meta] = strHex (mRawMeta);
    }

    mJson[jss::result] = transHuman (mResult);

    if (! mAffected.empty ())
    {
        Json::Value& affected = (mJson[jss::affected] = Json::arrayValue);
        for (auto const& account: mAffected)
            affected.append (accountCache_.toBase58(account));
    }

    if (mTxn->getTxnType () == ttOFFER_CREATE)
    {
        auto const& account = mTxn->getAccountID(sfAccount);
        auto const amount = mTxn->getFieldAmount (sfTakerGets);

//如果创建的报价不是自筹资金，则添加所有者余额
        if (account != amount.issue ().account)
        {
            auto const ownerFunds = accountFunds(*mLedger,
                account, amount, fhIGNORE_FREEZE, logs_.journal ("View"));
            mJson[jss::transaction][jss::owner_funds] = ownerFunds.getText ();
        }
    }
}

} //涟漪
