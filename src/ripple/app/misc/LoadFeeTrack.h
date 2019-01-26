
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

#ifndef RIPPLE_CORE_LOADFEETRACK_H_INCLUDED
#define RIPPLE_CORE_LOADFEETRACK_H_INCLUDED

#include <ripple/json/json_value.h>
#include <ripple/beast/utility/Journal.h>
#include <algorithm>
#include <cstdint>
#include <mutex>

namespace ripple {

struct Fees;

/*管理当前费用计划。

    “基本”费用是指在无负载情况下发送参考交易的成本，
    以百万分之一XRP表示。

    “加载”费用是本地服务器当前为发送
    参考交易。此费用根据
    服务器。
**/

class LoadFeeTrack final
{
public:
    explicit LoadFeeTrack (beast::Journal journal =
        beast::Journal(beast::Journal::getNullSink()))
        : j_ (journal)
        , localTxnLoadFee_ (lftNormalFee)
        , remoteTxnLoadFee_ (lftNormalFee)
        , clusterTxnLoadFee_ (lftNormalFee)
        , raiseCount_ (0)
    {
    }

    ~LoadFeeTrack() = default;

    void setRemoteFee (std::uint32_t f)
    {
        std::lock_guard <std::mutex> sl (lock_);
        remoteTxnLoadFee_ = f;
    }

    std::uint32_t getRemoteFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return remoteTxnLoadFee_;
    }

    std::uint32_t getLocalFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return localTxnLoadFee_;
    }

    std::uint32_t getClusterFee () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return clusterTxnLoadFee_;
    }

    std::uint32_t getLoadBase () const
    {
        return lftNormalFee;
    }

    std::uint32_t getLoadFactor () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return std::max({ clusterTxnLoadFee_, localTxnLoadFee_, remoteTxnLoadFee_ });
    }

    std::pair<std::uint32_t, std::uint32_t>
    getScalingFactors() const
    {
        std::lock_guard<std::mutex> sl(lock_);

        return std::make_pair(
            std::max(localTxnLoadFee_, remoteTxnLoadFee_),
            std::max(remoteTxnLoadFee_, clusterTxnLoadFee_));
    }


    void setClusterFee (std::uint32_t fee)
    {
        std::lock_guard <std::mutex> sl (lock_);
        clusterTxnLoadFee_ = fee;
    }

    bool raiseLocalFee ();
    bool lowerLocalFee ();

    bool isLoadedLocal () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return (raiseCount_ != 0) || (localTxnLoadFee_ != lftNormalFee);
    }

    bool isLoadedCluster () const
    {
        std::lock_guard <std::mutex> sl (lock_);
        return (raiseCount_ != 0) || (localTxnLoadFee_ != lftNormalFee) ||
            (clusterTxnLoadFee_ != lftNormalFee);
    }

private:
static std::uint32_t constexpr lftNormalFee = 256;        //256是最小/正常负载系数
static std::uint32_t constexpr lftFeeIncFraction = 4;     //费用增加1/4
static std::uint32_t constexpr lftFeeDecFraction = 4;     //费用减少1/4
    static std::uint32_t constexpr lftFeeMax = lftNormalFee * 1000000;

    beast::Journal j_;
    std::mutex mutable lock_;

std::uint32_t localTxnLoadFee_;        //比例系数，lftnormalfee=正常费用
std::uint32_t remoteTxnLoadFee_;       //比例系数，lftnormalfee=正常费用
std::uint32_t clusterTxnLoadFee_;      //比例系数，lftnormalfee=正常费用
    std::uint32_t raiseCount_;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//使用负荷和基本速率缩放
std::uint64_t scaleFeeLoad(std::uint64_t fee, LoadFeeTrack const& feeTrack,
    Fees const& fees, bool bUnlimited);

} //涟漪

#endif
