
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

#include <ripple/basics/contract.h>
#include <ripple/ledger/TxMeta.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/STAccount.h>
#include <string>

namespace ripple {

//vfalc todo将类重命名为TransactionMeta

template<class T>
TxMeta::TxMeta (uint256 const& txid,
    std::uint32_t ledger, T const& data, beast::Journal j, CtorHelper)
    : mTransactionID (txid)
    , mLedger (ledger)
    , mNodes (sfAffectedNodes, 32)
    , j_ (j)
{
    SerialIter sit (makeSlice(data));

    STObject obj(sit, sfMetadata);
    mResult = obj.getFieldU8 (sfTransactionResult);
    mIndex = obj.getFieldU32 (sfTransactionIndex);
    mNodes = * dynamic_cast<STArray*> (&obj.getField (sfAffectedNodes));

    if (obj.isFieldPresent (sfDeliveredAmount))
        setDeliveredAmount (obj.getFieldAmount (sfDeliveredAmount));
}

TxMeta::TxMeta (uint256 const& txid, std::uint32_t ledger, STObject const& obj,
    beast::Journal j)
    : mTransactionID (txid)
    , mLedger (ledger)
    , mNodes (obj.getFieldArray (sfAffectedNodes))
    , j_ (j)
{
    mResult = obj.getFieldU8 (sfTransactionResult);
    mIndex = obj.getFieldU32 (sfTransactionIndex);

    auto affectedNodes = dynamic_cast <STArray const*>
        (obj.peekAtPField (sfAffectedNodes));
    assert (affectedNodes);
    if (affectedNodes)
        mNodes = *affectedNodes;

    if (obj.isFieldPresent (sfDeliveredAmount))
        setDeliveredAmount (obj.getFieldAmount (sfDeliveredAmount));
}

TxMeta::TxMeta (uint256 const& txid,
    std::uint32_t ledger,
    Blob const& vec,
    beast::Journal j)
    : TxMeta (txid, ledger, vec, j, CtorHelper ())
{
}

TxMeta::TxMeta (uint256 const& txid,
    std::uint32_t ledger,
    std::string const& data,
    beast::Journal j)
    : TxMeta (txid, ledger, data, j, CtorHelper ())
{
}

bool TxMeta::isNodeAffected (uint256 const& node) const
{
    for (auto const& n : mNodes)
    {
        if (n.getFieldH256 (sfLedgerIndex) == node)
            return true;
    }

    return false;
}

void TxMeta::setAffectedNode (uint256 const& node, SField const& type,
                                          std::uint16_t nodeType)
{
//确保节点存在并强制其类型
    for (auto& n : mNodes)
    {
        if (n.getFieldH256 (sfLedgerIndex) == node)
        {
            n.setFName (type);
            n.setFieldU16 (sfLedgerEntryType, nodeType);
            return;
        }
    }

    mNodes.push_back (STObject (type));
    STObject& obj = mNodes.back ();

    assert (obj.getFName () == type);
    obj.setFieldH256 (sfLedgerIndex, node);
    obj.setFieldU16 (sfLedgerEntryType, nodeType);
}

boost::container::flat_set<AccountID>
TxMeta::getAffectedAccounts() const
{
    boost::container::flat_set<AccountID> list;
    list.reserve (10);

//此代码应与JS方法的行为匹配：
//元影响计数
    for (auto const& it : mNodes)
    {
        int index = it.getFieldIndex ((it.getFName () == sfCreatedNode) ? sfNewFields : sfFinalFields);

        if (index != -1)
        {
            const STObject* inner = dynamic_cast<const STObject*> (&it.peekAtIndex (index));
            assert(inner);
            if (inner)
            {
                for (auto const& field : *inner)
                {
                    if (auto sa = dynamic_cast<STAccount const*> (&field))
                    {
                        assert (! sa->isDefault());
                        if (! sa->isDefault())
                            list.insert(sa->value());
                    }
                    else if ((field.getFName () == sfLowLimit) || (field.getFName () == sfHighLimit) ||
                             (field.getFName () == sfTakerPays) || (field.getFName () == sfTakerGets))
                    {
                        const STAmount* lim = dynamic_cast<const STAmount*> (&field);

                        if (lim != nullptr)
                        {
                            auto issuer = lim->getIssuer ();

                            if (issuer.isNonZero ())
                                list.insert(issuer);
                        }
                        else
                        {
                            JLOG (j_.fatal()) << "limit is not amount " << field.getJson (0);
                        }
                    }
                }
            }
        }
    }

    return list;
}

STObject& TxMeta::getAffectedNode (SLE::ref node, SField const& type)
{
    uint256 index = node->key();
    for (auto& n : mNodes)
    {
        if (n.getFieldH256 (sfLedgerIndex) == index)
            return n;
    }
    mNodes.push_back (STObject (type));
    STObject& obj = mNodes.back ();

    assert (obj.getFName () == type);
    obj.setFieldH256 (sfLedgerIndex, index);
    obj.setFieldU16 (sfLedgerEntryType, node->getFieldU16 (sfLedgerEntryType));

    return obj;
}

STObject& TxMeta::getAffectedNode (uint256 const& node)
{
    for (auto& n : mNodes)
    {
        if (n.getFieldH256 (sfLedgerIndex) == node)
            return n;
    }
    assert (false);
    Throw<std::runtime_error> ("Affected node not found");
return *(mNodes.begin()); //静默编译器警告。
}

const STObject& TxMeta::peekAffectedNode (uint256 const& node) const
{
    for (auto const& n : mNodes)
    {
        if (n.getFieldH256 (sfLedgerIndex) == node)
            return n;
    }

    Throw<std::runtime_error> ("Affected node not found");
return *(mNodes.begin()); //静默编译器警告。
}

void TxMeta::init (uint256 const& id, std::uint32_t ledger)
{
    mTransactionID = id;
    mLedger = ledger;
    mNodes = STArray (sfAffectedNodes, 32);
    mDelivered = boost::optional <STAmount> ();
}

void TxMeta::swap (TxMeta& s) noexcept
{
    assert ((mTransactionID == s.mTransactionID) && (mLedger == s.mLedger));
    mNodes.swap (s.mNodes);
}

bool TxMeta::thread (STObject& node, uint256 const& prevTxID, std::uint32_t prevLgrID)
{
    if (node.getFieldIndex (sfPreviousTxnID) == -1)
    {
        assert (node.getFieldIndex (sfPreviousTxnLgrSeq) == -1);
        node.setFieldH256 (sfPreviousTxnID, prevTxID);
        node.setFieldU32 (sfPreviousTxnLgrSeq, prevLgrID);
        return true;
    }

    assert (node.getFieldH256 (sfPreviousTxnID) == prevTxID);
    assert (node.getFieldU32 (sfPreviousTxnLgrSeq) == prevLgrID);
    return false;
}

static bool compare (const STObject& o1, const STObject& o2)
{
    return o1.getFieldH256 (sfLedgerIndex) < o2.getFieldH256 (sfLedgerIndex);
}

STObject TxMeta::getAsObject () const
{
    STObject metaData (sfTransactionMetaData);
    assert (mResult != 255);
    metaData.setFieldU8 (sfTransactionResult, mResult);
    metaData.setFieldU32 (sfTransactionIndex, mIndex);
    metaData.emplace_back (mNodes);
    if (hasDeliveredAmount ())
        metaData.setFieldAmount (sfDeliveredAmount, getDeliveredAmount ());
    return metaData;
}

void TxMeta::addRaw (Serializer& s, TER result, std::uint32_t index)
{
    mResult = TERtoInt (result);
    mIndex = index;
    assert ((mResult == 0) || ((mResult > 100) && (mResult <= 255)));

    mNodes.sort (compare);

    getAsObject ().add (s);
}

} //涟漪
