
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

#include <ripple/app/misc/LoadFeeTrack.h>
#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/ReadView.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/JsonFields.h>
#include <cstdint>
#include <type_traits>

namespace ripple {

bool
LoadFeeTrack::raiseLocalFee ()
{
    std::lock_guard <std::mutex> sl (lock_);

    if (++raiseCount_ < 2)
        return false;

    std::uint32_t origFee = localTxnLoadFee_;

//确保这个费用生效
    if (localTxnLoadFee_ < remoteTxnLoadFee_)
        localTxnLoadFee_ = remoteTxnLoadFee_;

//缓慢增长
    localTxnLoadFee_ += (localTxnLoadFee_ / lftFeeIncFraction);

    if (localTxnLoadFee_ > lftFeeMax)
        localTxnLoadFee_ = lftFeeMax;

    if (origFee == localTxnLoadFee_)
        return false;

    JLOG(j_.debug()) << "Local load fee raised from " <<
        origFee << " to " << localTxnLoadFee_;
    return true;
}

bool
LoadFeeTrack::lowerLocalFee ()
{
    std::lock_guard <std::mutex> sl (lock_);
    std::uint32_t origFee = localTxnLoadFee_;
    raiseCount_ = 0;

//缓慢减少
    localTxnLoadFee_ -= (localTxnLoadFee_ / lftFeeDecFraction );

    if (localTxnLoadFee_ < lftNormalFee)
        localTxnLoadFee_ = lftNormalFee;

    if (origFee == localTxnLoadFee_)
        return false;

    JLOG(j_.debug()) << "Local load fee lowered from " <<
        origFee << " to " << localTxnLoadFee_;
    return true;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//NIKB todo:一旦我们获得了C++ 17，我们就可以替换低级词汇了。
//用这个：
//
//模板<T1类，T2类，
//class=std：：启用
//std：：为积分
//std:：is_integral_v<t2>>
//>
//无效的最低条款（T1&A、T2&B）
//{
//如果（auto const gcd=std:：gcd（a，b））
//{
//A/GCD；
//B/GCD；
//}
//}

template <class T1, class T2,
    class = std::enable_if_t <
        std::is_integral<T1>::value &&
        std::is_unsigned<T1>::value &&
        sizeof(T1) <= sizeof(std::uint64_t) >,
    class = std::enable_if_t <
        std::is_integral<T2>::value &&
        std::is_unsigned<T2>::value &&
        sizeof(T2) <= sizeof(std::uint64_t) >
>
void lowestTerms(T1& a,  T2& b)
{
    if (a == 0 && b == 0)
        return;

    std::uint64_t x = a, y = b;
    while (y != 0)
    {
        auto t = x % y;
        x = y;
        y = t;
    }
    a /= x;
    b /= x;
}

//使用负荷和基本速率缩放
std::uint64_t
scaleFeeLoad(std::uint64_t fee, LoadFeeTrack const& feeTrack,
    Fees const& fees, bool bUnlimited)
{
    if (fee == 0)
        return fee;
    std::uint32_t feeFactor;
    std::uint32_t uRemFee;
    {
//收取费率
        std::tie(feeFactor, uRemFee) = feeTrack.getScalingFactors();
    }
//允许特权用户支付正常费用，直到
//本地负载超过远程负载的四倍。
    if (bUnlimited && (feeFactor > uRemFee) && (feeFactor < (4 * uRemFee)))
        feeFactor = uRemFee;

    auto baseFee = fees.base;
//计算：
//费用=费用*基本费用*费系数/（费用.单位*费系数）
//无溢出，且尽可能准确

//我们要计算的分数的分母。
//fees.units和lftnormalfee都是32位，
//所以乘法不能溢出。
    auto den = safe_cast<std::uint64_t>(fees.units)
        * safe_cast<std::uint64_t>(feeTrack.getLoadBase());
//减少费用*基本费用*费系数/（费用.单位*费）
//到最低期限。
    lowestTerms(fee, den);
    lowestTerms(baseFee, den);
    lowestTerms(feeFactor, den);

//fee和basefee为64位，feefactor为32位
//订单费用和基本费用最大优先
    if (fee < baseFee)
        std::swap(fee, baseFee);
//如果basefee*feefactor溢出，则最终结果将溢出
    const auto max = std::numeric_limits<std::uint64_t>::max();
    if (baseFee > max / feeFactor)
        Throw<std::overflow_error> ("scaleFeeLoad");
    baseFee *= feeFactor;
//再订购费和基本费用
    if (fee < baseFee)
        std::swap(fee, baseFee);
//如果fee*basefee/den可能溢出…
    if (fee > max / baseFee)
    {
//先做除法，费用和基本费用取较大者
        fee /= den;
        if (fee > max / baseFee)
            Throw<std::overflow_error> ("scaleFeeLoad");
        fee *= baseFee;
    }
    else
    {
//否则费用*基本费用不会溢出，
//在分队之前也要这样做。
        fee *= baseFee;
        fee /= den;
    }
    return fee;
}

} //涟漪
