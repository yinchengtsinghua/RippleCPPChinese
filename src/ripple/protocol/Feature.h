
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

#ifndef RIPPLE_PROTOCOL_FEATURE_H_INCLUDED
#define RIPPLE_PROTOCOL_FEATURE_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <boost/container/flat_map.hpp>
#include <boost/optional.hpp>
#include <array>
#include <bitset>
#include <string>

/*
 *@页面功能如何添加新功能
 *
 *向代码中添加新功能所需的步骤：
 *
 *1）将新功能名称添加到下面的FeatureNames数组中。
 *2）在该文件的底部添加该功能的uint256声明。
 *3）将特性的uint256定义添加到相应的源中。
 *文件（feature.cpp）
 *4）如果该功能将在不久的将来得到支持，请添加
 *sha512half值和名称（与此处的featurename完全匹配）与
 *feature.cpp中的supportedamendments。
 *
 **/


namespace ripple {

namespace detail {

class FeatureCollections
{
    static constexpr char const* const featureNames[] =
    {
        "MultiSign",
        "Tickets",
        "TrustSetAuth",
        "FeeEscalation",
        "OwnerPaysFee",
        "CompareFlowV1V2",
        "SHAMapV2",
        "PayChan",
        "Flow",
        "CompareTakerFlowCross",
        "FlowCross",
        "CryptoConditions",
        "TickSize",
        "fix1368",
        "Escrow",
        "CryptoConditionsSuite",
        "fix1373",
        "EnforceInvariants",
        "SortedDirectories",
        "fix1201",
        "fix1512",
        "fix1513",
        "fix1523",
        "fix1528",
        "DepositAuth",
        "Checks",
        "fix1571",
        "fix1543",
        "fix1623",
        "DepositPreauth",
        "fix1515",
        "fix1578",
        "MultiSignReserve",
        "fixTakerDryOfferRemoval"
    };

    std::vector<uint256> features;
    boost::container::flat_map<uint256, std::size_t> featureToIndex;
    boost::container::flat_map<std::string, uint256> nameToFeature;

public:
    FeatureCollections();

    static constexpr std::size_t numFeatures()
    {
        return sizeof (featureNames) / sizeof (featureNames[0]);
    }

    boost::optional<uint256>
    getRegisteredFeature(std::string const& name) const;

    std::size_t
    featureToBitsetIndex(uint256 const& f) const;

    uint256 const&
    bitsetIndexToFeature(size_t i) const;
};

/*此服务器支持但默认情况下不启用的修订*/
std::vector<std::string> const&
supportedAmendments ();

} //细节

boost::optional<uint256>
getRegisteredFeature (std::string const& name);

size_t
featureToBitsetIndex(uint256 const& f);

uint256
bitsetIndexToFeature(size_t i);

class FeatureBitset
    : private std::bitset<detail::FeatureCollections::numFeatures()>
{
    using base = std::bitset<detail::FeatureCollections::numFeatures()>;

    void initFromFeatures()
    {
    }

    template<class... Fs>
    void initFromFeatures(uint256 const& f, Fs&&... fs)
    {
        set(f);
        initFromFeatures(std::forward<Fs>(fs)...);
    }

public:
    using base::bitset;
    using base::operator==;
    using base::operator!=;

    using base::test;
    using base::all;
    using base::any;
    using base::none;
    using base::count;
    using base::size;
    using base::set;
    using base::reset;
    using base::flip;
    using base::operator[];
    using base::to_string;
    using base::to_ulong;
    using base::to_ullong;

    FeatureBitset() = default;

    explicit
    FeatureBitset(base const& b)
        : base(b)
    {
    }

    template<class... Fs>
    explicit
    FeatureBitset(uint256 const& f, Fs&&... fs)
    {
        initFromFeatures(f, std::forward<Fs>(fs)...);
    }

    template <class Col>
    explicit
    FeatureBitset(Col const& fs)
    {
        for (auto const& f : fs)
            set(featureToBitsetIndex(f));
    }

    auto operator[](uint256 const& f)
    {
        return base::operator[](featureToBitsetIndex(f));
    }

    auto operator[](uint256 const& f) const
    {
        return base::operator[](featureToBitsetIndex(f));
    }

    FeatureBitset&
    set(uint256 const& f, bool value = true)
    {
        base::set(featureToBitsetIndex(f), value);
        return *this;
    }

    FeatureBitset&
    reset(uint256 const& f)
    {
        base::reset(featureToBitsetIndex(f));
        return *this;
    }

    FeatureBitset&
    flip(uint256 const& f)
    {
        base::flip(featureToBitsetIndex(f));
        return *this;
    }

    FeatureBitset& operator&=(FeatureBitset const& rhs)
    {
        base::operator&=(rhs);
        return *this;
    }

    FeatureBitset& operator|=(FeatureBitset const& rhs)
    {
        base::operator|=(rhs);
        return *this;
    }

    FeatureBitset operator~() const
    {
        return FeatureBitset{base::operator~()};
    }

    friend
    FeatureBitset operator&(
        FeatureBitset const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{static_cast<base const&>(lhs) &
                             static_cast<base const&>(rhs)};
    }

    friend
    FeatureBitset operator&(
        FeatureBitset const& lhs,
        uint256 const& rhs)
    {
        return lhs & FeatureBitset{rhs};
    }

    friend
    FeatureBitset operator&(
        uint256 const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{lhs} & rhs;
    }

    friend
    FeatureBitset operator|(
        FeatureBitset const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{static_cast<base const&>(lhs) |
                             static_cast<base const&>(rhs)};
    }

    friend
    FeatureBitset operator|(
        FeatureBitset const& lhs,
        uint256 const& rhs)
    {
        return lhs | FeatureBitset{rhs};
    }

    friend
    FeatureBitset operator|(
        uint256 const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{lhs} | rhs;
    }

    friend
    FeatureBitset operator^(
        FeatureBitset const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{static_cast<base const&>(lhs) ^
                             static_cast<base const&>(rhs)};
    }

    friend
    FeatureBitset operator^(
        FeatureBitset const& lhs,
        uint256 const& rhs)
    {
        return lhs ^ FeatureBitset{rhs};
    }

    friend
    FeatureBitset operator^(
        uint256 const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{lhs} ^ rhs;
    }

//集差
    friend
    FeatureBitset operator-(
        FeatureBitset const& lhs,
        FeatureBitset const& rhs)
    {
        return lhs & ~rhs;
    }

    friend
    FeatureBitset operator-(
        FeatureBitset const& lhs,
        uint256 const& rhs)
    {
        return lhs - FeatureBitset{rhs};
    }

    friend
    FeatureBitset operator-(
        uint256 const& lhs,
        FeatureBitset const& rhs)
    {
        return FeatureBitset{lhs} - rhs;
    }
};

template <class F>
void
foreachFeature(FeatureBitset bs, F&& f)
{
    for (size_t i = 0; i < bs.size(); ++i)
        if (bs[i])
            f(bitsetIndexToFeature(i));
}

extern uint256 const featureMultiSign;
extern uint256 const featureTickets;
extern uint256 const featureTrustSetAuth;
extern uint256 const featureOwnerPaysFee;
extern uint256 const featureCompareFlowV1V2;
extern uint256 const featureSHAMapV2;
extern uint256 const featurePayChan;
extern uint256 const featureFlow;
extern uint256 const featureCompareTakerFlowCross;
extern uint256 const featureFlowCross;
extern uint256 const featureCryptoConditions;
extern uint256 const featureTickSize;
extern uint256 const fix1368;
extern uint256 const featureEscrow;
extern uint256 const featureCryptoConditionsSuite;
extern uint256 const fix1373;
extern uint256 const featureEnforceInvariants;
extern uint256 const featureSortedDirectories;
extern uint256 const fix1201;
extern uint256 const fix1512;
extern uint256 const fix1513;
extern uint256 const fix1523;
extern uint256 const fix1528;
extern uint256 const featureDepositAuth;
extern uint256 const featureChecks;
extern uint256 const fix1571;
extern uint256 const fix1543;
extern uint256 const fix1623;
extern uint256 const featureDepositPreauth;
extern uint256 const fix1515;
extern uint256 const fix1578;
extern uint256 const featureMultiSignReserve;
extern uint256 const fixTakerDryOfferRemoval;

} //涟漪

#endif
