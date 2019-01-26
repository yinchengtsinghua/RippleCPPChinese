
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

#include <ripple/basics/Log.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/STInteger.h>
#include <ripple/protocol/TxFormats.h>
#include <ripple/protocol/TER.h>
#include <ripple/beast/core/LexicalCast.h>

namespace ripple {

template<>
STInteger<unsigned char>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get8())
{
}

template <>
SerializedTypeID
STUInt8::getSType () const
{
    return STI_UINT8;
}

template <>
std::string
STUInt8::getText () const
{
    if (getFName () == sfTransactionResult)
    {
        std::string token, human;

        if (transResultInfo (TER::fromInt (value_), token, human))
            return human;

        JLOG (debugLog().error())
            << "Unknown result code in metadata: " << value_;
    }

    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt8::getJson (int) const
{
    if (getFName () == sfTransactionResult)
    {
        std::string token, human;

        if (transResultInfo (TER::fromInt (value_), token, human))
            return token;

        JLOG (debugLog().error())
            << "Unknown result code in metadata: " << value_;
    }

    return value_;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<>
STInteger<std::uint16_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get16())
{
}

template <>
SerializedTypeID
STUInt16::getSType () const
{
    return STI_UINT16;
}

template <>
std::string
STUInt16::getText () const
{
    if (getFName () == sfLedgerEntryType)
    {
        auto item = LedgerFormats::getInstance ().findByType (
            safe_cast<LedgerEntryType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    if (getFName () == sfTransactionType)
    {
        auto item =TxFormats::getInstance().findByType (
            safe_cast<TxType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt16::getJson (int) const
{
    if (getFName () == sfLedgerEntryType)
    {
        auto item = LedgerFormats::getInstance ().findByType (
            safe_cast<LedgerEntryType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    if (getFName () == sfTransactionType)
    {
        auto item = TxFormats::getInstance().findByType (
            safe_cast<TxType> (value_));

        if (item != nullptr)
            return item->getName ();
    }

    return value_;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<>
STInteger<std::uint32_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get32())
{
}

template <>
SerializedTypeID
STUInt32::getSType () const
{
    return STI_UINT32;
}

template <>
std::string
STUInt32::getText () const
{
    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt32::getJson (int) const
{
    return value_;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template<>
STInteger<std::uint64_t>::STInteger(SerialIter& sit, SField const& name)
    : STInteger(name, sit.get64())
{
}

template <>
SerializedTypeID
STUInt64::getSType () const
{
    return STI_UINT64;
}

template <>
std::string
STUInt64::getText () const
{
    return beast::lexicalCastThrow <std::string> (value_);
}

template <>
Json::Value
STUInt64::getJson (int) const
{
    return strHex (value_);
}

} //涟漪
