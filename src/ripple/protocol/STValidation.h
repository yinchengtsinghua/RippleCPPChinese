
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

#ifndef RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED
#define RIPPLE_PROTOCOL_STVALIDATION_H_INCLUDED

#include <ripple/basics/Log.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/STObject.h>
#include <ripple/protocol/SecretKey.h>
#include <cstdint>
#include <functional>
#include <memory>

namespace ripple {

//验证标志
const std::uint32_t vfFullyCanonicalSig =
0x80000000;  //签名完全规范

class STValidation final : public STObject, public CountedObject<STValidation>
{
public:
    static char const*
    getCountedObjectName()
    {
        return "STValidation";
    }

    using pointer = std::shared_ptr<STValidation>;
    using ref = const std::shared_ptr<STValidation>&;

    enum { kFullFlag = 0x1 };

    /*从对等机构造一个stvalidation。

        从以前由
        同龄人。

        @param sit迭代器覆盖序列化数据
        带签名的@param lookupnodeid invocable
                               nodeid（公钥常量&）
                            用于根据公钥查找节点ID
                            签署了验证。基于清单
                            验证器，这应该是主节点的nodeid
                            公钥。
        @param checksignature是否验证数据签名是否正确

        @note如果对象无效则抛出
    **/

    template <class LookupNodeID>
    STValidation(
        SerialIter& sit,
        LookupNodeID&& lookupNodeID,
        bool checkSignature)
        : STObject(getFormat(), sit, sfValidation)
    {
        auto const spk = getFieldVL(sfSigningPubKey);

        if (publicKeyType(makeSlice(spk)) != KeyType::secp256k1)
        {
            JLOG (debugLog().error())
                << "Invalid public key in validation" << getJson (0);
            Throw<std::runtime_error> ("Invalid public key in validation");
        }

        if  (checkSignature && !isValid ())
        {
            JLOG (debugLog().error())
                << "Invalid signature in validation" << getJson (0);
            Throw<std::runtime_error> ("Invalid signature in validation");
        }

        mNodeID = lookupNodeID(PublicKey(makeSlice(spk)));
        assert(mNodeID.isNonZero());
    }

    /*发布新验证时要设置的费用。

        发布新验证时要包括的可选费用
    **/

    struct FeeSettings
    {
        boost::optional<std::uint32_t> loadFee;
        boost::optional<std::uint64_t> baseFee;
        boost::optional<std::uint32_t> reserveBase;
        boost::optional<std::uint32_t> reserveIncrement;
    };

    /*构造、签名并信任新的stvalidation

        构造、签名和信任此节点发出的新stvalidation。

        @param ledger hash已验证分类帐的哈希
        @param ledgerseq分类帐的序列号（索引）
        @param consensus hash共识事务集的哈希
        验证签名时的@param signtime
        @param public key当前正在签名的公钥
        @param secret key当前签名密钥
        对应于节点公钥的@param node id id
        无论验证是完整的还是部分的，@param都是完整的
        要包括在验证中的@param feesettings
        @param modifications如果不为空，则包含在此验证中的修正

        @注意，只有在不设置boost：：none时才设置费用和修改设置。
              通常，修改和费用是为标志的验证设置的。
              分类帐只。
    **/

    STValidation(
        uint256 const& ledgerHash,
        std::uint32_t ledgerSeq,
        uint256 const& consensusHash,
        NetClock::time_point signTime,
        PublicKey const& publicKey,
        SecretKey const& secretKey,
        NodeID const& nodeID,
        bool isFull,
        FeeSettings const& fees,
        std::vector<uint256> const& amendments);

    STBase*
    copy(std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move(std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

//已验证分类帐的哈希
    uint256
    getLedgerHash() const;

//用于生成分类帐的一致性事务集的哈希
    uint256
    getConsensusHash() const;

    NetClock::time_point
    getSignTime() const;

    NetClock::time_point
    getSeenTime() const;

    PublicKey
    getSignerPublic() const;

    NodeID
    getNodeID() const
    {
        return mNodeID;
    }

    bool
    isValid() const;

    bool
    isFull() const;

    bool
    isTrusted() const
    {
        return mTrusted;
    }

    uint256
    getSigningHash() const;

    bool
    isValid(uint256 const&) const;

    void
    setTrusted()
    {
        mTrusted = true;
    }

    void
    setUntrusted()
    {
        mTrusted = false;
    }

    void
    setSeen(NetClock::time_point s)
    {
        mSeen = s;
    }

    Blob
    getSerialized() const;

    Blob
    getSignature() const;

private:

    static SOTemplate const&
    getFormat();

    NodeID mNodeID;
    bool mTrusted = false;
    NetClock::time_point mSeen = {};
};

} //涟漪

#endif
