
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

#include <ripple/protocol/Feature.h>
#include <ripple/basics/contract.h>
#include <ripple/protocol/digest.h>

#include <cstring>

namespace ripple {

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

constexpr char const* const detail::FeatureCollections::featureNames[];

detail::FeatureCollections::FeatureCollections()
{
    features.reserve(numFeatures());
    featureToIndex.reserve(numFeatures());
    nameToFeature.reserve(numFeatures());

    for (std::size_t i = 0; i < numFeatures(); ++i)
    {
        auto const name = featureNames[i];
        sha512_half_hasher h;
        h (name, std::strlen (name));
        auto const f = static_cast<uint256>(h);

        features.push_back(f);
        featureToIndex[f] = i;
        nameToFeature[name] = f;
    }
}

boost::optional<uint256>
detail::FeatureCollections::getRegisteredFeature(std::string const& name) const
{
    auto const i = nameToFeature.find(name);
    if (i == nameToFeature.end())
        return boost::none;
    return i->second;
}

size_t
detail::FeatureCollections::featureToBitsetIndex(uint256 const& f) const
{
    auto const i = featureToIndex.find(f);
    if (i == featureToIndex.end())
        LogicError("Invalid Feature ID");
    return i->second;
}

uint256 const&
detail::FeatureCollections::bitsetIndexToFeature(size_t i) const
{
    if (i >= features.size())
        LogicError("Invalid FeatureBitset index");
    return features[i];
}

static detail::FeatureCollections const featureCollections;

/*此服务器支持但默认情况下不启用的修订*/
std::vector<std::string> const&
detail::supportedAmendments ()
{
//注释掉的修改将在未来的版本中得到支持（和
//当时未注释）。
    static std::vector<std::string> const supported
    {
//“SAMAPV2”
        "MultiSign",
//“票”
        "TrustSetAuth",
"FeeEscalation", //看起来未使用，但不要删除；服务器将被修改阻止。
//“业主工资”，
        "PayChan",
        "Flow",
        "CryptoConditions",
        "TickSize",
        "fix1368",
        "Escrow",
        "CryptoConditionsSuite",
        "fix1373",
        "EnforceInvariants",
        "FlowCross",
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
//使用消耗最大报价但标记为干燥的股的流动性
        "fix1515",
        "fix1578",
        "MultiSignReserve",
        "fixTakerDryOfferRemoval"
    };
    return supported;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

boost::optional<uint256>
getRegisteredFeature (std::string const& name)
{
    return featureCollections.getRegisteredFeature(name);
}

size_t featureToBitsetIndex(uint256 const& f)
{
    return featureCollections.featureToBitsetIndex(f);
}

uint256 bitsetIndexToFeature(size_t i)
{
    return featureCollections.bitsetIndexToFeature(i);
}


uint256 const featureMultiSign = *getRegisteredFeature("MultiSign");
uint256 const featureTickets = *getRegisteredFeature("Tickets");
uint256 const featureTrustSetAuth = *getRegisteredFeature("TrustSetAuth");
uint256 const featureOwnerPaysFee = *getRegisteredFeature("OwnerPaysFee");
uint256 const featureCompareFlowV1V2 = *getRegisteredFeature("CompareFlowV1V2");
uint256 const featureSHAMapV2 = *getRegisteredFeature("SHAMapV2");
uint256 const featurePayChan = *getRegisteredFeature("PayChan");
uint256 const featureFlow = *getRegisteredFeature("Flow");
uint256 const featureCompareTakerFlowCross = *getRegisteredFeature("CompareTakerFlowCross");
uint256 const featureFlowCross = *getRegisteredFeature("FlowCross");
uint256 const featureCryptoConditions = *getRegisteredFeature("CryptoConditions");
uint256 const featureTickSize = *getRegisteredFeature("TickSize");
uint256 const fix1368 = *getRegisteredFeature("fix1368");
uint256 const featureEscrow = *getRegisteredFeature("Escrow");
uint256 const featureCryptoConditionsSuite = *getRegisteredFeature("CryptoConditionsSuite");
uint256 const fix1373 = *getRegisteredFeature("fix1373");
uint256 const featureEnforceInvariants = *getRegisteredFeature("EnforceInvariants");
uint256 const featureSortedDirectories = *getRegisteredFeature("SortedDirectories");
uint256 const fix1201 = *getRegisteredFeature("fix1201");
uint256 const fix1512 = *getRegisteredFeature("fix1512");
uint256 const fix1513 = *getRegisteredFeature("fix1513");
uint256 const fix1523 = *getRegisteredFeature("fix1523");
uint256 const fix1528 = *getRegisteredFeature("fix1528");
uint256 const featureDepositAuth = *getRegisteredFeature("DepositAuth");
uint256 const featureChecks = *getRegisteredFeature("Checks");
uint256 const fix1571 = *getRegisteredFeature("fix1571");
uint256 const fix1543 = *getRegisteredFeature("fix1543");
uint256 const fix1623 = *getRegisteredFeature("fix1623");
uint256 const featureDepositPreauth = *getRegisteredFeature("DepositPreauth");
uint256 const fix1515 = *getRegisteredFeature("fix1515");
uint256 const fix1578 = *getRegisteredFeature("fix1578");
uint256 const featureMultiSignReserve = *getRegisteredFeature("MultiSignReserve");
uint256 const fixTakerDryOfferRemoval = *getRegisteredFeature("fixTakerDryOfferRemoval");

} //涟漪
