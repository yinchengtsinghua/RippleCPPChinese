
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

#include <ripple/protocol/TxFormats.h>

namespace ripple {

TxFormats::TxFormats ()
{
    add ("AccountSet", ttACCOUNT_SET)
        << SOElement (sfEmailHash,           SOE_OPTIONAL)
        << SOElement (sfWalletLocator,       SOE_OPTIONAL)
        << SOElement (sfWalletSize,          SOE_OPTIONAL)
        << SOElement (sfMessageKey,          SOE_OPTIONAL)
        << SOElement (sfDomain,              SOE_OPTIONAL)
        << SOElement (sfTransferRate,        SOE_OPTIONAL)
        << SOElement (sfSetFlag,             SOE_OPTIONAL)
        << SOElement (sfClearFlag,           SOE_OPTIONAL)
        << SOElement (sfTickSize,            SOE_OPTIONAL)
        ;

    add ("TrustSet", ttTRUST_SET)
        << SOElement (sfLimitAmount,         SOE_OPTIONAL)
        << SOElement (sfQualityIn,           SOE_OPTIONAL)
        << SOElement (sfQualityOut,          SOE_OPTIONAL)
        ;

    add ("OfferCreate", ttOFFER_CREATE)
        << SOElement (sfTakerPays,           SOE_REQUIRED)
        << SOElement (sfTakerGets,           SOE_REQUIRED)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        << SOElement (sfOfferSequence,       SOE_OPTIONAL)
        ;

    add ("OfferCancel", ttOFFER_CANCEL)
        << SOElement (sfOfferSequence,       SOE_REQUIRED)
        ;

    add ("SetRegularKey", ttREGULAR_KEY_SET)
        << SOElement (sfRegularKey,          SOE_OPTIONAL)
        ;

    add ("Payment", ttPAYMENT)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfSendMax,             SOE_OPTIONAL)
        << SOElement (sfPaths,               SOE_DEFAULT)
        << SOElement (sfInvoiceID,           SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        << SOElement (sfDeliverMin,          SOE_OPTIONAL)
        ;

    add ("EscrowCreate", ttESCROW_CREATE)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfCondition,           SOE_OPTIONAL)
        << SOElement (sfCancelAfter,         SOE_OPTIONAL)
        << SOElement (sfFinishAfter,         SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        ;

    add ("EscrowFinish", ttESCROW_FINISH)
        << SOElement (sfOwner,               SOE_REQUIRED)
        << SOElement (sfOfferSequence,       SOE_REQUIRED)
        << SOElement (sfFulfillment,         SOE_OPTIONAL)
        << SOElement (sfCondition,           SOE_OPTIONAL)
        ;

    add ("EscrowCancel", ttESCROW_CANCEL)
        << SOElement (sfOwner,               SOE_REQUIRED)
        << SOElement (sfOfferSequence,       SOE_REQUIRED)
        ;

    add ("EnableAmendment", ttAMENDMENT)
        << SOElement (sfLedgerSequence,      SOE_REQUIRED)
        << SOElement (sfAmendment,           SOE_REQUIRED)
        ;

    add ("SetFee", ttFEE)
        << SOElement (sfLedgerSequence,      SOE_OPTIONAL)
        << SOElement (sfBaseFee,             SOE_REQUIRED)
        << SOElement (sfReferenceFeeUnits,   SOE_REQUIRED)
        << SOElement (sfReserveBase,         SOE_REQUIRED)
        << SOElement (sfReserveIncrement,    SOE_REQUIRED)
        ;

    add ("TicketCreate", ttTICKET_CREATE)
        << SOElement (sfTarget,              SOE_OPTIONAL)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        ;

    add ("TicketCancel", ttTICKET_CANCEL)
        << SOElement (sfTicketID,            SOE_REQUIRED)
        ;

//签名程序是可选的，因为签名程序被删除
//将SignerQuarum设置为零并省略SignerEntries。
    add ("SignerListSet", ttSIGNER_LIST_SET)
        << SOElement (sfSignerQuorum,        SOE_REQUIRED)
        << SOElement (sfSignerEntries,       SOE_OPTIONAL)
        ;

    add ("PaymentChannelCreate", ttPAYCHAN_CREATE)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfSettleDelay,         SOE_REQUIRED)
        << SOElement (sfPublicKey,           SOE_REQUIRED)
        << SOElement (sfCancelAfter,         SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        ;

    add ("PaymentChannelFund", ttPAYCHAN_FUND)
        << SOElement (sfPayChannel,          SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_REQUIRED)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        ;

    add ("PaymentChannelClaim", ttPAYCHAN_CLAIM)
        << SOElement (sfPayChannel,          SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_OPTIONAL)
        << SOElement (sfBalance,             SOE_OPTIONAL)
        << SOElement (sfSignature,           SOE_OPTIONAL)
        << SOElement (sfPublicKey,           SOE_OPTIONAL)
        ;

    add ("CheckCreate", ttCHECK_CREATE)
        << SOElement (sfDestination,         SOE_REQUIRED)
        << SOElement (sfSendMax,             SOE_REQUIRED)
        << SOElement (sfExpiration,          SOE_OPTIONAL)
        << SOElement (sfDestinationTag,      SOE_OPTIONAL)
        << SOElement (sfInvoiceID,           SOE_OPTIONAL)
        ;

    add ("CheckCash", ttCHECK_CASH)
        << SOElement (sfCheckID,             SOE_REQUIRED)
        << SOElement (sfAmount,              SOE_OPTIONAL)
        << SOElement (sfDeliverMin,          SOE_OPTIONAL)
        ;

    add ("CheckCancel", ttCHECK_CANCEL)
        << SOElement (sfCheckID,             SOE_REQUIRED)
        ;

    add ("DepositPreauth", ttDEPOSIT_PREAUTH)
        << SOElement (sfAuthorize,           SOE_OPTIONAL)
        << SOElement (sfUnauthorize,         SOE_OPTIONAL)
        ;
}

void TxFormats::addCommonFields (Item& item)
{
    item
        << SOElement(sfTransactionType,      SOE_REQUIRED)
        << SOElement(sfFlags,                SOE_OPTIONAL)
        << SOElement(sfSourceTag,            SOE_OPTIONAL)
        << SOElement(sfAccount,              SOE_REQUIRED)
        << SOElement(sfSequence,             SOE_REQUIRED)
<< SOElement(sfPreviousTxnID,        SOE_OPTIONAL) //仿真027
        << SOElement(sfLastLedgerSequence,   SOE_OPTIONAL)
        << SOElement(sfAccountTxnID,         SOE_OPTIONAL)
        << SOElement(sfFee,                  SOE_REQUIRED)
        << SOElement(sfOperationLimit,       SOE_OPTIONAL)
        << SOElement(sfMemos,                SOE_OPTIONAL)
        << SOElement(sfSigningPubKey,        SOE_REQUIRED)
        << SOElement(sfTxnSignature,         SOE_OPTIONAL)
<< SOElement(sfSigners,              SOE_OPTIONAL) //提交多签名
        ;
}

TxFormats const&
TxFormats::getInstance ()
{
    static TxFormats const instance;
    return instance;
}

} //涟漪
