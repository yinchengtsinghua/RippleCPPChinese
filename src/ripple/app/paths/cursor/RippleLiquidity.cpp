
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

#include <ripple/protocol/Quality.h>
#include <ripple/app/paths/cursor/RippleLiquidity.h>
#include <ripple/basics/Log.h>

namespace ripple {
namespace path {

//计算传递的节点的可能流动量。实际上不是
//调整余额。
//
//uQualityIn->uQualityOut
//saprvreq->sacurreq
//sqprvact->sacuract
//
//这是一个最小化的例程：反向移动会传播发送限制
//向发送方，向前移动它会将实际发送传播到
//接收器。
//
//当这个程序向后工作时，sacurreq是驱动变量：it
//根据以前的信用额度和当前的需求计算以前的需求。
//
//当这个例程向前工作时，saprvreq是驱动变量：it
//根据以前的交货限制和当前交货计算当前交货
//欲望。
//
//对于一个过程中的一个节点，这个例程被调用一到两次。如果叫一次，
//它会起作用，并设定费率。如果再次调用，新工作不得恶化
//以前的价格。

void rippleLiquidity (
    RippleCalc& rippleCalc,
    Rate const& qualityIn,
    Rate const& qualityOut,
STAmount const& saPrvReq,   //-->in limit including fees，<0=无限制
STAmount const& saCurReq,   //-->外极限
STAmount& saPrvAct,  //包括目前为止实现的限制：<--<=-->
STAmount& saCurAct,  //<->Out limit including achieved so far:<-<=->
    std::uint64_t& uRateMax)
{
    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity>"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;

//sacuract曾经是生产服务器中的Beast:：Zero。
    assert (saCurReq != beast::zero);
    assert (saCurReq > beast::zero);

    assert (saPrvReq.getCurrency () == saCurReq.getCurrency ());
    assert (saPrvReq.getCurrency () == saPrvAct.getCurrency ());
    assert (saPrvReq.getIssuer () == saPrvAct.getIssuer ());

const bool bPrvUnlimited = (saPrvReq < beast::zero);  //-1表示无限。

//无限保持无限-不要做计算。

//前一个节点可能有多少流量？
    const STAmount saPrv = bPrvUnlimited ? saPrvReq : saPrvReq - saPrvAct;

//有多少可能流过当前节点？
    const STAmount  saCur = saCurReq - saCurAct;

    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity: "
        << " bPrvUnlimited=" << bPrvUnlimited
        << " saPrv=" << saPrv
        << " saCur=" << saCur;

//如果没有什么可以流动，我们最好不要做任何工作。
    if (saPrv == beast::zero || saCur == beast::zero)
        return;

    if (qualityIn >= qualityOut)
    {
//你的质量比你要求的好，所以不收费。
        JLOG (rippleCalc.j_.trace()) << "rippleLiquidity: No fees";

//仅当当前比率1:1不比前一比率差时才处理
//速率，乌拉特马-否则没有流量。
        if (!uRateMax || STAmount::uRateOne <= uRateMax)
        {
//如果需要，限制要转移的金额-金额的最小值为
//支付和所需金额。
            STAmount saTransfer = bPrvUnlimited ? saCur
                : std::min (saPrv, saCur);

//相反，我们希望将有限的cur传播到prv并设置
//实际曲线。
//
//在转发中，我们希望将有限的prv传播到cur并设置
//实际PRV。
//
//这是实际流量。
            saPrvAct += saTransfer;
            saCurAct += saTransfer;

//如果没有利率限制，则设置利率限制以避免与
//速度更慢的东西。
            if (uRateMax == 0)
                uRateMax = STAmount::uRateOne;
        }
    }
    else
    {
//如果质量比以前差
        JLOG (rippleCalc.j_.trace()) << "rippleLiquidity: Fee";

        std::uint64_t const uRate = getRate (
            STAmount (qualityOut.value),
            STAmount (qualityIn.value));

//如果下一个速率至少与当前速率相同，则处理。
        if (!uRateMax || uRate <= uRateMax)
        {
//当前实际值=当前请求*（质量输出/质量输入）。
            auto numerator = multiplyRound (saCur, qualityOut, true);
//正确的意思是“聚集”以获得最佳流。

            STAmount saCurIn = divideRound (numerator, qualityIn, true);

            JLOG (rippleCalc.j_.trace())
                << "rippleLiquidity:"
                << " bPrvUnlimited=" << bPrvUnlimited
                << " saPrv=" << saPrv
                << " saCurIn=" << saCurIn;

            if (bPrvUnlimited || saCurIn <= saPrv)
            {
//所有的电流。以前的一些数量。
                saCurAct += saCur;
                saPrvAct += saCurIn;
                JLOG (rippleCalc.j_.trace())
                    << "rippleLiquidity:3c:"
                    << " saCurReq=" << saCurReq
                    << " saPrvAct=" << saPrvAct;
            }
            else
            {
//没有足够的钱开始-所以考虑到
//投入有限，我们能提供多少？
//当前实际值=上一个请求
//*（质量输入/质量输出）。
//与上面的代码相比，这是颠倒的，因为我们
//走另一条路
                auto numerator = multiplyRound (saPrv,
                    qualityIn, saCur.issue(), true);
//电流的一部分。以前所有的。（cur是司机
//变量）
                STAmount saCurOut = divideRound (numerator,
                    qualityOut, saCur.issue(), true);

                JLOG (rippleCalc.j_.trace())
                    << "rippleLiquidity:4: saCurReq=" << saCurReq;

                saCurAct += saCurOut;
                saPrvAct = saPrvReq;
            }
            if (!uRateMax)
                uRateMax = uRate;
        }
    }

    JLOG (rippleCalc.j_.trace())
        << "rippleLiquidity<"
        << " qualityIn=" << qualityIn
        << " qualityOut=" << qualityOut
        << " saPrvReq=" << saPrvReq
        << " saCurReq=" << saCurReq
        << " saPrvAct=" << saPrvAct
        << " saCurAct=" << saCurAct;
}

static
Rate
rippleQuality (
    ReadView const& view,
    AccountID const& destination,
    AccountID const& source,
    Currency const& currency,
    SField const& sfLow,
    SField const& sfHigh)
{
    if (destination == source)
        return parityRate;

    auto const& sfField = destination < source ? sfLow : sfHigh;

    auto const sle = view.read(
        keylet::line(destination, source, currency));

    if (!sle || !sle->isFieldPresent (sfField))
        return parityRate;

    auto quality = sle->getFieldU32 (sfField);

//避免被零除。如果我们
//现在允许零质量，那么我们不应该。
    if (quality == 0)
        quality = 1;

    return Rate{ quality };
}

Rate
quality_in (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return rippleQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityIn, sfHighQualityIn);
}

Rate
quality_out (
    ReadView const& view,
    AccountID const& uToAccountID,
    AccountID const& uFromAccountID,
    Currency const& currency)
{
    return rippleQuality (view, uToAccountID, uFromAccountID, currency,
        sfLowQualityOut, sfHighQualityOut);
}

} //路径
} //涟漪
