
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

#include <ripple/protocol/STValidation.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/json/to_string.h>

namespace ripple {

STValidation::STValidation(
    uint256 const& ledgerHash,
    std::uint32_t ledgerSeq,
    uint256 const& consensusHash,
    NetClock::time_point signTime,
    PublicKey const& publicKey,
    SecretKey const& secretKey,
    NodeID const& nodeID,
    bool isFull,
    FeeSettings const& fees,
    std::vector<uint256> const& amendments)
    : STObject(getFormat(), sfValidation), mNodeID(nodeID), mSeen(signTime)
{
//这是我们自己的公钥，应该始终有效。
    if (!publicKeyType(publicKey))
        LogicError("Invalid validation public key");
    assert(mNodeID.isNonZero());
    setFieldH256(sfLedgerHash, ledgerHash);
    setFieldH256(sfConsensusHash, consensusHash);
    setFieldU32(sfSigningTime, signTime.time_since_epoch().count());

    setFieldVL(sfSigningPubKey, publicKey.slice());
    if (isFull)
        setFlag(kFullFlag);

    setFieldU32(sfLedgerSequence, ledgerSeq);

    if (fees.loadFee)
        setFieldU32(sfLoadFee, *fees.loadFee);

    if (fees.baseFee)
        setFieldU64(sfBaseFee, *fees.baseFee);

    if (fees.reserveBase)
        setFieldU32(sfReserveBase, *fees.reserveBase);

    if (fees.reserveIncrement)
        setFieldU32(sfReserveIncrement, *fees.reserveIncrement);

    if (!amendments.empty())
        setFieldV256(sfAmendments, STVector256(sfAmendments, amendments));

    setFlag(vfFullyCanonicalSig);

    auto const signingHash = getSigningHash();
    setFieldVL(
        sfSignature, signDigest(getSignerPublic(), secretKey, signingHash));

    setTrusted();
}

uint256 STValidation::getSigningHash () const
{
    return STObject::getSigningHash (HashPrefix::validation);
}

uint256 STValidation::getLedgerHash () const
{
    return getFieldH256 (sfLedgerHash);
}

uint256 STValidation::getConsensusHash () const
{
    return getFieldH256 (sfConsensusHash);
}

NetClock::time_point
STValidation::getSignTime () const
{
    return NetClock::time_point{NetClock::duration{getFieldU32(sfSigningTime)}};
}

NetClock::time_point STValidation::getSeenTime () const
{
    return mSeen;
}

bool STValidation::isValid () const
{
    return isValid (getSigningHash ());
}

bool STValidation::isValid (uint256 const& signingHash) const
{
    try
    {
        if (publicKeyType(getSignerPublic()) != KeyType::secp256k1)
            return false;

        return verifyDigest (getSignerPublic(),
            signingHash,
            makeSlice(getFieldVL (sfSignature)),
            getFlags () & vfFullyCanonicalSig);
    }
    catch (std::exception const&)
    {
        JLOG (debugLog().error())
            << "Exception validating validation";
        return false;
    }
}

PublicKey STValidation::getSignerPublic () const
{
    return PublicKey(makeSlice (getFieldVL (sfSigningPubKey)));
}

bool STValidation::isFull () const
{
    return (getFlags () & kFullFlag) != 0;
}

Blob STValidation::getSignature () const
{
    return getFieldVL (sfSignature);
}

Blob STValidation::getSerialized () const
{
    Serializer s;
    add (s);
    return s.peekData ();
}

SOTemplate const& STValidation::getFormat ()
{
    struct FormatHolder
    {
        SOTemplate format;

        FormatHolder ()
        {
            format.push_back (SOElement (sfFlags,           SOE_REQUIRED));
            format.push_back (SOElement (sfLedgerHash,      SOE_REQUIRED));
            format.push_back (SOElement (sfLedgerSequence,  SOE_OPTIONAL));
            format.push_back (SOElement (sfCloseTime,       SOE_OPTIONAL));
            format.push_back (SOElement (sfLoadFee,         SOE_OPTIONAL));
            format.push_back (SOElement (sfAmendments,      SOE_OPTIONAL));
            format.push_back (SOElement (sfBaseFee,         SOE_OPTIONAL));
            format.push_back (SOElement (sfReserveBase,     SOE_OPTIONAL));
            format.push_back (SOElement (sfReserveIncrement, SOE_OPTIONAL));
            format.push_back (SOElement (sfSigningTime,     SOE_REQUIRED));
            format.push_back (SOElement (sfSigningPubKey,   SOE_REQUIRED));
            format.push_back (SOElement (sfSignature,       SOE_OPTIONAL));
            format.push_back (SOElement (sfConsensusHash,   SOE_OPTIONAL));
            format.push_back (SOElement (sfCookie,          SOE_OPTIONAL));

        }
    };

    static const FormatHolder holder;

    return holder.format;
}

} //涟漪
