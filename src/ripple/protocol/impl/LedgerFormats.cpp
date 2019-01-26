
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

#include <ripple/protocol/LedgerFormats.h>
#include <ripple/protocol/ErrorCodes.h>
#include <ripple/protocol/JsonFields.h>
#include <algorithm>
#include <array>
#include <utility>

namespace ripple {

LedgerFormats::LedgerFormats ()
{
    add ("AccountRoot", ltACCOUNT_ROOT)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfSequence,            SOE_REQUIRED)
            << SOElement (sfBalance,             SOE_REQUIRED)
            << SOElement (sfOwnerCount,          SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            << SOElement (sfAccountTxnID,        SOE_OPTIONAL)
            << SOElement (sfRegularKey,          SOE_OPTIONAL)
            << SOElement (sfEmailHash,           SOE_OPTIONAL)
            << SOElement (sfWalletLocator,       SOE_OPTIONAL)
            << SOElement (sfWalletSize,          SOE_OPTIONAL)
            << SOElement (sfMessageKey,          SOE_OPTIONAL)
            << SOElement (sfTransferRate,        SOE_OPTIONAL)
            << SOElement (sfDomain,              SOE_OPTIONAL)
            << SOElement (sfTickSize,            SOE_OPTIONAL)
            ;

    add ("DirectoryNode", ltDIR_NODE)
<< SOElement (sfOwner,               SOE_OPTIONAL)  //对于所有者目录
<< SOElement (sfTakerPaysCurrency,   SOE_OPTIONAL)  //订单簿目录
<< SOElement (sfTakerPaysIssuer,     SOE_OPTIONAL)  //订单簿目录
<< SOElement (sfTakerGetsCurrency,   SOE_OPTIONAL)  //订单簿目录
<< SOElement (sfTakerGetsIssuer,     SOE_OPTIONAL)  //订单簿目录
<< SOElement (sfExchangeRate,        SOE_OPTIONAL)  //订单簿目录
            << SOElement (sfIndexes,             SOE_REQUIRED)
            << SOElement (sfRootIndex,           SOE_REQUIRED)
            << SOElement (sfIndexNext,           SOE_OPTIONAL)
            << SOElement (sfIndexPrevious,       SOE_OPTIONAL)
            ;

    add ("Offer", ltOFFER)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfSequence,            SOE_REQUIRED)
            << SOElement (sfTakerPays,           SOE_REQUIRED)
            << SOElement (sfTakerGets,           SOE_REQUIRED)
            << SOElement (sfBookDirectory,       SOE_REQUIRED)
            << SOElement (sfBookNode,            SOE_REQUIRED)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            << SOElement (sfExpiration,          SOE_OPTIONAL)
            ;

    add ("RippleState", ltRIPPLE_STATE)
            << SOElement (sfBalance,             SOE_REQUIRED)
            << SOElement (sfLowLimit,            SOE_REQUIRED)
            << SOElement (sfHighLimit,           SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            << SOElement (sfLowNode,             SOE_OPTIONAL)
            << SOElement (sfLowQualityIn,        SOE_OPTIONAL)
            << SOElement (sfLowQualityOut,       SOE_OPTIONAL)
            << SOElement (sfHighNode,            SOE_OPTIONAL)
            << SOElement (sfHighQualityIn,       SOE_OPTIONAL)
            << SOElement (sfHighQualityOut,      SOE_OPTIONAL)
            ;

    add ("Escrow", ltESCROW)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfDestination,         SOE_REQUIRED)
            << SOElement (sfAmount,              SOE_REQUIRED)
            << SOElement (sfCondition,           SOE_OPTIONAL)
            << SOElement (sfCancelAfter,         SOE_OPTIONAL)
            << SOElement (sfFinishAfter,         SOE_OPTIONAL)
            << SOElement (sfSourceTag,           SOE_OPTIONAL)
            << SOElement (sfDestinationTag,      SOE_OPTIONAL)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            << SOElement (sfDestinationNode,     SOE_OPTIONAL)
            ;

    add ("LedgerHashes", ltLEDGER_HASHES)
<< SOElement (sfFirstLedgerSequence, SOE_OPTIONAL) //如果重新启动分类帐，则删除
            << SOElement (sfLastLedgerSequence,  SOE_OPTIONAL)
            << SOElement (sfHashes,              SOE_REQUIRED)
            ;

    add ("Amendments", ltAMENDMENTS)
<< SOElement (sfAmendments,          SOE_OPTIONAL) //启用
            << SOElement (sfMajorities,          SOE_OPTIONAL)
            ;

    add ("FeeSettings", ltFEE_SETTINGS)
            << SOElement (sfBaseFee,             SOE_REQUIRED)
            << SOElement (sfReferenceFeeUnits,   SOE_REQUIRED)
            << SOElement (sfReserveBase,         SOE_REQUIRED)
            << SOElement (sfReserveIncrement,    SOE_REQUIRED)
            ;

    add ("Ticket", ltTICKET)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfSequence,            SOE_REQUIRED)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfTarget,              SOE_OPTIONAL)
            << SOElement (sfExpiration,          SOE_OPTIONAL)
            ;

//所有字段都需要SOE_，因为
//签字人。如果没有签名，则删除节点。
    add ("SignerList", ltSIGNER_LIST)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfSignerQuorum,        SOE_REQUIRED)
            << SOElement (sfSignerEntries,       SOE_REQUIRED)
            << SOElement (sfSignerListID,        SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            ;

    add ("PayChannel", ltPAYCHAN)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfDestination,         SOE_REQUIRED)
            << SOElement (sfAmount,              SOE_REQUIRED)
            << SOElement (sfBalance,             SOE_REQUIRED)
            << SOElement (sfPublicKey,           SOE_REQUIRED)
            << SOElement (sfSettleDelay,         SOE_REQUIRED)
            << SOElement (sfExpiration,          SOE_OPTIONAL)
            << SOElement (sfCancelAfter,         SOE_OPTIONAL)
            << SOElement (sfSourceTag,           SOE_OPTIONAL)
            << SOElement (sfDestinationTag,      SOE_OPTIONAL)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            ;

    add ("Check", ltCHECK)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfDestination,         SOE_REQUIRED)
            << SOElement (sfSendMax,             SOE_REQUIRED)
            << SOElement (sfSequence,            SOE_REQUIRED)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfDestinationNode,     SOE_REQUIRED)
            << SOElement (sfExpiration,          SOE_OPTIONAL)
            << SOElement (sfInvoiceID,           SOE_OPTIONAL)
            << SOElement (sfSourceTag,           SOE_OPTIONAL)
            << SOElement (sfDestinationTag,      SOE_OPTIONAL)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            ;

    add ("DepositPreauth", ltDEPOSIT_PREAUTH)
            << SOElement (sfAccount,             SOE_REQUIRED)
            << SOElement (sfAuthorize,           SOE_REQUIRED)
            << SOElement (sfOwnerNode,           SOE_REQUIRED)
            << SOElement (sfPreviousTxnID,       SOE_REQUIRED)
            << SOElement (sfPreviousTxnLgrSeq,   SOE_REQUIRED)
            ;
}

void LedgerFormats::addCommonFields (Item& item)
{
    item
        << SOElement(sfLedgerIndex,             SOE_OPTIONAL)
        << SOElement(sfLedgerEntryType,         SOE_REQUIRED)
        << SOElement(sfFlags,                   SOE_REQUIRED)
        ;
}

LedgerFormats const&
LedgerFormats::getInstance ()
{
    static LedgerFormats instance;
    return instance;
}

} //涟漪
