
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
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/json/to_string.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/STLedgerEntry.h>
#include <boost/format.hpp>

namespace ripple {

STLedgerEntry::STLedgerEntry (Keylet const& k)
    :  STObject(sfLedgerEntry)
    , key_ (k.key)
    , type_ (k.type)
{
    if (!(0u <= type_ &&
        type_ <= std::min<unsigned>(std::numeric_limits<std::uint16_t>::max(),
        std::numeric_limits<std::underlying_type_t<LedgerEntryType>>::max())))
            Throw<std::runtime_error> ("invalid ledger entry type: out of range");
    auto const format =
        LedgerFormats::getInstance().findByType (type_);

    if (format == nullptr)
        Throw<std::runtime_error> ("invalid ledger entry type");

    set (format->elements);

    setFieldU16 (sfLedgerEntryType,
        static_cast <std::uint16_t> (type_));
}

STLedgerEntry::STLedgerEntry (
        SerialIter& sit,
        uint256 const& index)
    : STObject (sfLedgerEntry)
    , key_ (index)
{
    set (sit);
    setSLEType ();
}

STLedgerEntry::STLedgerEntry (
        STObject const& object,
        uint256 const& index)
    : STObject (object)
    , key_ (index)
{
    setSLEType ();
}

void STLedgerEntry::setSLEType ()
{
    auto format = LedgerFormats::getInstance().findByType (
        safe_cast <LedgerEntryType> (
            getFieldU16 (sfLedgerEntryType)));

    if (format == nullptr)
        Throw<std::runtime_error> ("invalid ledger entry type");

    type_ = format->getType ();
applyTemplate (format->elements);  //可能扔
}

std::string STLedgerEntry::getFullText () const
{
    auto const format =
        LedgerFormats::getInstance().findByType (type_);

    if (format == nullptr)
        Throw<std::runtime_error> ("invalid ledger entry type");

    std::string ret = "\"";
    ret += to_string (key_);
    ret += "\" = { ";
    ret += format->getName ();
    ret += ", ";
    ret += STObject::getFullText ();
    ret += "}";
    return ret;
}

std::string STLedgerEntry::getText () const
{
    return str (boost::format ("{ %s, %s }")
                % to_string (key_)
                % STObject::getText ());
}

Json::Value STLedgerEntry::getJson (int options) const
{
    Json::Value ret (STObject::getJson (options));

    ret[jss::index] = to_string (key_);

    return ret;
}

bool STLedgerEntry::isThreadedType () const
{
    return getFieldIndex (sfPreviousTxnID) != -1;
}

bool STLedgerEntry::thread (
    uint256 const& txID,
    std::uint32_t ledgerSeq,
    uint256& prevTxID,
    std::uint32_t& prevLedgerID)
{
    uint256 oldPrevTxID = getFieldH256 (sfPreviousTxnID);

    JLOG (debugLog().info())
        << "Thread Tx:" << txID << " prev:" << oldPrevTxID;

    if (oldPrevTxID == txID)
    {
//此事务已被线程化
        assert (getFieldU32 (sfPreviousTxnLgrSeq) == ledgerSeq);
        return false;
    }

    prevTxID = oldPrevTxID;
    prevLedgerID = getFieldU32 (sfPreviousTxnLgrSeq);
    setFieldH256 (sfPreviousTxnID, txID);
    setFieldU32 (sfPreviousTxnLgrSeq, ledgerSeq);
    return true;
}

} //涟漪
