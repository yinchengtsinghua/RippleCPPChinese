
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

#ifndef RIPPLE_PROTOCOL_TXFLAGS_H_INCLUDED
#define RIPPLE_PROTOCOL_TXFLAGS_H_INCLUDED

#include <cstdint>

namespace ripple {

//
//事务标志。
//

/*事务标志。

    这些标志修改操作的行为。

    @注意更改这些将创建一个硬分叉
    @ingroup协议
**/

class TxFlag
{
public:
    explicit TxFlag() = default;

    static std::uint32_t const requireDestTag = 0x00010000;
};
//vfalc todo在完成一些研究后将所有标志移到这个容器中。

//通用事务标志：
const std::uint32_t tfFullyCanonicalSig    = 0x80000000;
const std::uint32_t tfUniversal            = tfFullyCanonicalSig;
const std::uint32_t tfUniversalMask        = ~ tfUniversal;

//帐户集标志：
//vfalc todo javadoc注释这些常量中的每一个
//const std:：uint32_t txflag:：requiredEsttag=0x00010000；
const std::uint32_t tfOptionalDestTag      = 0x00020000;
const std::uint32_t tfRequireAuth          = 0x00040000;
const std::uint32_t tfOptionalAuth         = 0x00080000;
const std::uint32_t tfDisallowXRP          = 0x00100000;
const std::uint32_t tfAllowXRP             = 0x00200000;
const std::uint32_t tfAccountSetMask       = ~ (tfUniversal | TxFlag::requireDestTag | tfOptionalDestTag
                                             | tfRequireAuth | tfOptionalAuth
                                             | tfDisallowXRP | tfAllowXRP);

//accountset setflag/clearfag值
const std::uint32_t asfRequireDest         = 1;
const std::uint32_t asfRequireAuth         = 2;
const std::uint32_t asfDisallowXRP         = 3;
const std::uint32_t asfDisableMaster       = 4;
const std::uint32_t asfAccountTxnID        = 5;
const std::uint32_t asfNoFreeze            = 6;
const std::uint32_t asfGlobalFreeze        = 7;
const std::uint32_t asfDefaultRipple       = 8;
const std::uint32_t asfDepositAuth         = 9;

//offerCreate标志：
const std::uint32_t tfPassive              = 0x00010000;
const std::uint32_t tfImmediateOrCancel    = 0x00020000;
const std::uint32_t tfFillOrKill           = 0x00040000;
const std::uint32_t tfSell                 = 0x00080000;
const std::uint32_t tfOfferCreateMask      = ~ (tfUniversal | tfPassive | tfImmediateOrCancel | tfFillOrKill | tfSell);

//付款标志：
const std::uint32_t tfNoRippleDirect       = 0x00010000;
const std::uint32_t tfPartialPayment       = 0x00020000;
const std::uint32_t tfLimitQuality         = 0x00040000;
const std::uint32_t tfPaymentMask          = ~ (tfUniversal | tfPartialPayment | tfLimitQuality | tfNoRippleDirect);

//信任集标志：
const std::uint32_t tfSetfAuth             = 0x00010000;
const std::uint32_t tfSetNoRipple          = 0x00020000;
const std::uint32_t tfClearNoRipple        = 0x00040000;
const std::uint32_t tfSetFreeze            = 0x00100000;
const std::uint32_t tfClearFreeze          = 0x00200000;
const std::uint32_t tfTrustSetMask         = ~ (tfUniversal | tfSetfAuth | tfSetNoRipple | tfClearNoRipple
                                             | tfSetFreeze | tfClearFreeze);

//启用修正标志：
const std::uint32_t tfGotMajority          = 0x00010000;
const std::uint32_t tfLostMajority         = 0x00020000;

//PaymentChannelClaim标志：
const std::uint32_t tfRenew                = 0x00010000;
const std::uint32_t tfClose                = 0x00020000;
const std::uint32_t tfPayChanClaimMask     = ~ (tfUniversal | tfRenew | tfClose);

} //涟漪

#endif
