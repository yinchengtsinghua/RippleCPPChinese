
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

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/SField.h>
#include <cassert>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace ripple {

//这些必须放在这个文件的顶部，并且按这个顺序
//这里首选文件作用域静态，因为sfields必须
//文件范围。以下3个对象必须在
//文件范围字段。
static std::mutex SField_mutex;
static std::map<int, SField const*> knownCodeToField;
static std::map<int, std::unique_ptr<SField const>> unknownCodeToField;

//静态常量成员的存储。
SField::IsSigning const SField::notSigning;

int SField::num = 0;

using StaticScopedLockType = std::lock_guard <std::mutex>;

//仅允许此翻译单元，允许创建字段
struct SField::make
{
    explicit make() = default;

    template <class ...Args>
    static SField one(SField const* p, Args&& ...args)
    {
        SField result(std::forward<Args>(args)...);
        knownCodeToField[result.fieldCode] = p;
        return result;
    }

    template <class T, class ...Args>
    static TypedField<T> one(SField const* p, Args&& ...args)
    {
        TypedField<T> result(std::forward<Args>(args)...);
        knownCodeToField[result.fieldCode] = p;
        return result;
    }
};

using make = SField::make;

//构建所有编译时sfield，并在knowncodetofield中注册它们
//数据库：

SField const sfInvalid     = make::one(&sfInvalid, -1);
SField const sfGeneric     = make::one(&sfGeneric, 0);
SField const sfLedgerEntry = make::one(&sfLedgerEntry, STI_LEDGERENTRY, 257, "LedgerEntry");
SField const sfTransaction = make::one(&sfTransaction, STI_TRANSACTION, 257, "Transaction");
SField const sfValidation  = make::one(&sfValidation,  STI_VALIDATION,  257, "Validation");
SField const sfMetadata    = make::one(&sfMetadata,    STI_METADATA,    257, "Metadata");
SField const sfHash        = make::one(&sfHash,        STI_HASH256,     257, "hash");
SField const sfIndex       = make::one(&sfIndex,       STI_HASH256,     258, "index");

//8位整数
SF_U8 const sfCloseResolution   = make::one<SF_U8::type>(&sfCloseResolution,   STI_UINT8, 1, "CloseResolution");
SF_U8 const sfMethod            = make::one<SF_U8::type>(&sfMethod,            STI_UINT8, 2, "Method");
SF_U8 const sfTransactionResult = make::one<SF_U8::type>(&sfTransactionResult, STI_UINT8, 3, "TransactionResult");

//8位整数（不常见）
SF_U8 const sfTickSize          = make::one<SF_U8::type>(&sfTickSize,          STI_UINT8, 16, "TickSize");

//16位整数
SF_U16 const sfLedgerEntryType = make::one<SF_U16::type>(&sfLedgerEntryType, STI_UINT16, 1, "LedgerEntryType", SField::sMD_Never);
SF_U16 const sfTransactionType = make::one<SF_U16::type>(&sfTransactionType, STI_UINT16, 2, "TransactionType");
SF_U16 const sfSignerWeight    = make::one<SF_U16::type>(&sfSignerWeight,    STI_UINT16, 3, "SignerWeight");

//32位整数（公共）
SF_U32 const sfFlags             = make::one<SF_U32::type>(&sfFlags,             STI_UINT32,  2, "Flags");
SF_U32 const sfSourceTag         = make::one<SF_U32::type>(&sfSourceTag,         STI_UINT32,  3, "SourceTag");
SF_U32 const sfSequence          = make::one<SF_U32::type>(&sfSequence,          STI_UINT32,  4, "Sequence");
SF_U32 const sfPreviousTxnLgrSeq = make::one<SF_U32::type>(&sfPreviousTxnLgrSeq, STI_UINT32,  5, "PreviousTxnLgrSeq", SField::sMD_DeleteFinal);
SF_U32 const sfLedgerSequence    = make::one<SF_U32::type>(&sfLedgerSequence,    STI_UINT32,  6, "LedgerSequence");
SF_U32 const sfCloseTime         = make::one<SF_U32::type>(&sfCloseTime,         STI_UINT32,  7, "CloseTime");
SF_U32 const sfParentCloseTime   = make::one<SF_U32::type>(&sfParentCloseTime,   STI_UINT32,  8, "ParentCloseTime");
SF_U32 const sfSigningTime       = make::one<SF_U32::type>(&sfSigningTime,       STI_UINT32,  9, "SigningTime");
SF_U32 const sfExpiration        = make::one<SF_U32::type>(&sfExpiration,        STI_UINT32, 10, "Expiration");
SF_U32 const sfTransferRate      = make::one<SF_U32::type>(&sfTransferRate,      STI_UINT32, 11, "TransferRate");
SF_U32 const sfWalletSize        = make::one<SF_U32::type>(&sfWalletSize,        STI_UINT32, 12, "WalletSize");
SF_U32 const sfOwnerCount        = make::one<SF_U32::type>(&sfOwnerCount,        STI_UINT32, 13, "OwnerCount");
SF_U32 const sfDestinationTag    = make::one<SF_U32::type>(&sfDestinationTag,    STI_UINT32, 14, "DestinationTag");

//32位整数（不常见）
SF_U32 const sfHighQualityIn       = make::one<SF_U32::type>(&sfHighQualityIn,       STI_UINT32, 16, "HighQualityIn");
SF_U32 const sfHighQualityOut      = make::one<SF_U32::type>(&sfHighQualityOut,      STI_UINT32, 17, "HighQualityOut");
SF_U32 const sfLowQualityIn        = make::one<SF_U32::type>(&sfLowQualityIn,        STI_UINT32, 18, "LowQualityIn");
SF_U32 const sfLowQualityOut       = make::one<SF_U32::type>(&sfLowQualityOut,       STI_UINT32, 19, "LowQualityOut");
SF_U32 const sfQualityIn           = make::one<SF_U32::type>(&sfQualityIn,           STI_UINT32, 20, "QualityIn");
SF_U32 const sfQualityOut          = make::one<SF_U32::type>(&sfQualityOut,          STI_UINT32, 21, "QualityOut");
SF_U32 const sfStampEscrow         = make::one<SF_U32::type>(&sfStampEscrow,         STI_UINT32, 22, "StampEscrow");
SF_U32 const sfBondAmount          = make::one<SF_U32::type>(&sfBondAmount,          STI_UINT32, 23, "BondAmount");
SF_U32 const sfLoadFee             = make::one<SF_U32::type>(&sfLoadFee,             STI_UINT32, 24, "LoadFee");
SF_U32 const sfOfferSequence       = make::one<SF_U32::type>(&sfOfferSequence,       STI_UINT32, 25, "OfferSequence");
SF_U32 const sfFirstLedgerSequence = make::one<SF_U32::type>(&sfFirstLedgerSequence, STI_UINT32, 26, "FirstLedgerSequence");  //已弃用：不要使用
SF_U32 const sfLastLedgerSequence  = make::one<SF_U32::type>(&sfLastLedgerSequence,  STI_UINT32, 27, "LastLedgerSequence");
SF_U32 const sfTransactionIndex    = make::one<SF_U32::type>(&sfTransactionIndex,    STI_UINT32, 28, "TransactionIndex");
SF_U32 const sfOperationLimit      = make::one<SF_U32::type>(&sfOperationLimit,      STI_UINT32, 29, "OperationLimit");
SF_U32 const sfReferenceFeeUnits   = make::one<SF_U32::type>(&sfReferenceFeeUnits,   STI_UINT32, 30, "ReferenceFeeUnits");
SF_U32 const sfReserveBase         = make::one<SF_U32::type>(&sfReserveBase,         STI_UINT32, 31, "ReserveBase");
SF_U32 const sfReserveIncrement    = make::one<SF_U32::type>(&sfReserveIncrement,    STI_UINT32, 32, "ReserveIncrement");
SF_U32 const sfSetFlag             = make::one<SF_U32::type>(&sfSetFlag,             STI_UINT32, 33, "SetFlag");
SF_U32 const sfClearFlag           = make::one<SF_U32::type>(&sfClearFlag,           STI_UINT32, 34, "ClearFlag");
SF_U32 const sfSignerQuorum        = make::one<SF_U32::type>(&sfSignerQuorum,        STI_UINT32, 35, "SignerQuorum");
SF_U32 const sfCancelAfter         = make::one<SF_U32::type>(&sfCancelAfter,         STI_UINT32, 36, "CancelAfter");
SF_U32 const sfFinishAfter         = make::one<SF_U32::type>(&sfFinishAfter,         STI_UINT32, 37, "FinishAfter");
SF_U32 const sfSignerListID        = make::one<SF_U32::type>(&sfSignerListID,        STI_UINT32, 38, "SignerListID");
SF_U32 const sfSettleDelay         = make::one<SF_U32::type>(&sfSettleDelay,         STI_UINT32, 39, "SettleDelay");

//64位整数
SF_U64 const sfIndexNext        = make::one<SF_U64::type>(&sfIndexNext,        STI_UINT64, 1, "IndexNext");
SF_U64 const sfIndexPrevious    = make::one<SF_U64::type>(&sfIndexPrevious,    STI_UINT64, 2, "IndexPrevious");
SF_U64 const sfBookNode         = make::one<SF_U64::type>(&sfBookNode,         STI_UINT64, 3, "BookNode");
SF_U64 const sfOwnerNode        = make::one<SF_U64::type>(&sfOwnerNode,        STI_UINT64, 4, "OwnerNode");
SF_U64 const sfBaseFee          = make::one<SF_U64::type>(&sfBaseFee,          STI_UINT64, 5, "BaseFee");
SF_U64 const sfExchangeRate     = make::one<SF_U64::type>(&sfExchangeRate,     STI_UINT64, 6, "ExchangeRate");
SF_U64 const sfLowNode          = make::one<SF_U64::type>(&sfLowNode,          STI_UINT64, 7, "LowNode");
SF_U64 const sfHighNode         = make::one<SF_U64::type>(&sfHighNode,         STI_UINT64, 8, "HighNode");
SF_U64 const sfDestinationNode  = make::one<SF_U64::type>(&sfDestinationNode,  STI_UINT64, 9, "DestinationNode");
SF_U64 const sfCookie           = make::one<SF_U64::type>(&sfCookie,           STI_UINT64, 10,"Cookie");


//128位
SF_U128 const sfEmailHash = make::one<SF_U128::type>(&sfEmailHash, STI_HASH128, 1, "EmailHash");

//160位（公共）
SF_U160 const sfTakerPaysCurrency = make::one<SF_U160::type>(&sfTakerPaysCurrency, STI_HASH160, 1, "TakerPaysCurrency");
SF_U160 const sfTakerPaysIssuer   = make::one<SF_U160::type>(&sfTakerPaysIssuer,   STI_HASH160, 2, "TakerPaysIssuer");
SF_U160 const sfTakerGetsCurrency = make::one<SF_U160::type>(&sfTakerGetsCurrency, STI_HASH160, 3, "TakerGetsCurrency");
SF_U160 const sfTakerGetsIssuer   = make::one<SF_U160::type>(&sfTakerGetsIssuer,   STI_HASH160, 4, "TakerGetsIssuer");

//256位（公共）
SF_U256 const sfLedgerHash      = make::one<SF_U256::type>(&sfLedgerHash,      STI_HASH256, 1, "LedgerHash");
SF_U256 const sfParentHash      = make::one<SF_U256::type>(&sfParentHash,      STI_HASH256, 2, "ParentHash");
SF_U256 const sfTransactionHash = make::one<SF_U256::type>(&sfTransactionHash, STI_HASH256, 3, "TransactionHash");
SF_U256 const sfAccountHash     = make::one<SF_U256::type>(&sfAccountHash,     STI_HASH256, 4, "AccountHash");
SF_U256 const sfPreviousTxnID   = make::one<SF_U256::type>(&sfPreviousTxnID,   STI_HASH256, 5, "PreviousTxnID", SField::sMD_DeleteFinal);
SF_U256 const sfLedgerIndex     = make::one<SF_U256::type>(&sfLedgerIndex,     STI_HASH256, 6, "LedgerIndex");
SF_U256 const sfWalletLocator   = make::one<SF_U256::type>(&sfWalletLocator,   STI_HASH256, 7, "WalletLocator");
SF_U256 const sfRootIndex       = make::one<SF_U256::type>(&sfRootIndex,       STI_HASH256, 8, "RootIndex", SField::sMD_Always);
SF_U256 const sfAccountTxnID    = make::one<SF_U256::type>(&sfAccountTxnID,    STI_HASH256, 9, "AccountTxnID");

//256位（不常见）
SF_U256 const sfBookDirectory = make::one<SF_U256::type>(&sfBookDirectory, STI_HASH256, 16, "BookDirectory");
SF_U256 const sfInvoiceID     = make::one<SF_U256::type>(&sfInvoiceID,     STI_HASH256, 17, "InvoiceID");
SF_U256 const sfNickname      = make::one<SF_U256::type>(&sfNickname,      STI_HASH256, 18, "Nickname");
SF_U256 const sfAmendment     = make::one<SF_U256::type>(&sfAmendment,     STI_HASH256, 19, "Amendment");
SF_U256 const sfTicketID      = make::one<SF_U256::type>(&sfTicketID,      STI_HASH256, 20, "TicketID");
SF_U256 const sfDigest        = make::one<SF_U256::type>(&sfDigest,        STI_HASH256, 21, "Digest");
SF_U256 const sfPayChannel    = make::one<SF_U256::type>(&sfPayChannel,    STI_HASH256, 22, "Channel");
SF_U256 const sfConsensusHash = make::one<SF_U256::type>(&sfConsensusHash, STI_HASH256, 23, "ConsensusHash");
SF_U256 const sfCheckID       = make::one<SF_U256::type>(&sfCheckID,       STI_HASH256, 24, "CheckID");

//货币金额（普通）
SF_Amount const sfAmount      = make::one<SF_Amount::type>(&sfAmount,      STI_AMOUNT,  1, "Amount");
SF_Amount const sfBalance     = make::one<SF_Amount::type>(&sfBalance,     STI_AMOUNT,  2, "Balance");
SF_Amount const sfLimitAmount = make::one<SF_Amount::type>(&sfLimitAmount, STI_AMOUNT,  3, "LimitAmount");
SF_Amount const sfTakerPays   = make::one<SF_Amount::type>(&sfTakerPays,   STI_AMOUNT,  4, "TakerPays");
SF_Amount const sfTakerGets   = make::one<SF_Amount::type>(&sfTakerGets,   STI_AMOUNT,  5, "TakerGets");
SF_Amount const sfLowLimit    = make::one<SF_Amount::type>(&sfLowLimit,    STI_AMOUNT,  6, "LowLimit");
SF_Amount const sfHighLimit   = make::one<SF_Amount::type>(&sfHighLimit,   STI_AMOUNT,  7, "HighLimit");
SF_Amount const sfFee         = make::one<SF_Amount::type>(&sfFee,         STI_AMOUNT,  8, "Fee");
SF_Amount const sfSendMax     = make::one<SF_Amount::type>(&sfSendMax,     STI_AMOUNT,  9, "SendMax");
SF_Amount const sfDeliverMin  = make::one<SF_Amount::type>(&sfDeliverMin,  STI_AMOUNT, 10, "DeliverMin");

//货币金额（不常见）
SF_Amount const sfMinimumOffer    = make::one<SF_Amount::type>(&sfMinimumOffer,    STI_AMOUNT, 16, "MinimumOffer");
SF_Amount const sfRippleEscrow    = make::one<SF_Amount::type>(&sfRippleEscrow,    STI_AMOUNT, 17, "RippleEscrow");
SF_Amount const sfDeliveredAmount = make::one<SF_Amount::type>(&sfDeliveredAmount, STI_AMOUNT, 18, "DeliveredAmount");

//可变长度（公共）
SF_Blob const sfPublicKey       = make::one<SF_Blob::type>(&sfPublicKey,     STI_VL,  1, "PublicKey");
SF_Blob const sfSigningPubKey   = make::one<SF_Blob::type>(&sfSigningPubKey, STI_VL,  3, "SigningPubKey");
SF_Blob const sfSignature       = make::one<SF_Blob::type>(&sfSignature,     STI_VL,  6, "Signature", SField::sMD_Default, SField::notSigning);
SF_Blob const sfMessageKey      = make::one<SF_Blob::type>(&sfMessageKey,    STI_VL,  2, "MessageKey");
SF_Blob const sfTxnSignature    = make::one<SF_Blob::type>(&sfTxnSignature,  STI_VL,  4, "TxnSignature", SField::sMD_Default, SField::notSigning);
SF_Blob const sfDomain          = make::one<SF_Blob::type>(&sfDomain,        STI_VL,  7, "Domain");
SF_Blob const sfFundCode        = make::one<SF_Blob::type>(&sfFundCode,      STI_VL,  8, "FundCode");
SF_Blob const sfRemoveCode      = make::one<SF_Blob::type>(&sfRemoveCode,    STI_VL,  9, "RemoveCode");
SF_Blob const sfExpireCode      = make::one<SF_Blob::type>(&sfExpireCode,    STI_VL, 10, "ExpireCode");
SF_Blob const sfCreateCode      = make::one<SF_Blob::type>(&sfCreateCode,    STI_VL, 11, "CreateCode");
SF_Blob const sfMemoType        = make::one<SF_Blob::type>(&sfMemoType,      STI_VL, 12, "MemoType");
SF_Blob const sfMemoData        = make::one<SF_Blob::type>(&sfMemoData,      STI_VL, 13, "MemoData");
SF_Blob const sfMemoFormat      = make::one<SF_Blob::type>(&sfMemoFormat,    STI_VL, 14, "MemoFormat");


//可变长度（不常见）
SF_Blob const sfFulfillment     = make::one<SF_Blob::type>(&sfFulfillment,     STI_VL, 16, "Fulfillment");
SF_Blob const sfCondition       = make::one<SF_Blob::type>(&sfCondition,       STI_VL, 17, "Condition");
SF_Blob const sfMasterSignature = make::one<SF_Blob::type>(&sfMasterSignature, STI_VL, 18, "MasterSignature", SField::sMD_Default, SField::notSigning);


//解释
SF_Account const sfAccount     = make::one<SF_Account::type>(&sfAccount,     STI_ACCOUNT, 1, "Account");
SF_Account const sfOwner       = make::one<SF_Account::type>(&sfOwner,       STI_ACCOUNT, 2, "Owner");
SF_Account const sfDestination = make::one<SF_Account::type>(&sfDestination, STI_ACCOUNT, 3, "Destination");
SF_Account const sfIssuer      = make::one<SF_Account::type>(&sfIssuer,      STI_ACCOUNT, 4, "Issuer");
SF_Account const sfAuthorize   = make::one<SF_Account::type>(&sfAuthorize,   STI_ACCOUNT, 5, "Authorize");
SF_Account const sfUnauthorize = make::one<SF_Account::type>(&sfUnauthorize, STI_ACCOUNT, 6, "Unauthorize");
SF_Account const sfTarget      = make::one<SF_Account::type>(&sfTarget,      STI_ACCOUNT, 7, "Target");
SF_Account const sfRegularKey  = make::one<SF_Account::type>(&sfRegularKey,  STI_ACCOUNT, 8, "RegularKey");

//路径集
SField const sfPaths = make::one(&sfPaths, STI_PATHSET, 1, "Paths");

//256位矢量
SF_Vec256 const sfIndexes    = make::one<SF_Vec256::type>(&sfIndexes,    STI_VECTOR256, 1, "Indexes", SField::sMD_Never);
SF_Vec256 const sfHashes     = make::one<SF_Vec256::type>(&sfHashes,     STI_VECTOR256, 2, "Hashes");
SF_Vec256 const sfAmendments = make::one<SF_Vec256::type>(&sfAmendments, STI_VECTOR256, 3, "Amendments");

//内部对象
//对象/1是为对象结尾保留的
SField const sfTransactionMetaData = make::one(&sfTransactionMetaData, STI_OBJECT,  2, "TransactionMetaData");
SField const sfCreatedNode         = make::one(&sfCreatedNode,         STI_OBJECT,  3, "CreatedNode");
SField const sfDeletedNode         = make::one(&sfDeletedNode,         STI_OBJECT,  4, "DeletedNode");
SField const sfModifiedNode        = make::one(&sfModifiedNode,        STI_OBJECT,  5, "ModifiedNode");
SField const sfPreviousFields      = make::one(&sfPreviousFields,      STI_OBJECT,  6, "PreviousFields");
SField const sfFinalFields         = make::one(&sfFinalFields,         STI_OBJECT,  7, "FinalFields");
SField const sfNewFields           = make::one(&sfNewFields,           STI_OBJECT,  8, "NewFields");
SField const sfTemplateEntry       = make::one(&sfTemplateEntry,       STI_OBJECT,  9, "TemplateEntry");
SField const sfMemo                = make::one(&sfMemo,                STI_OBJECT, 10, "Memo");
SField const sfSignerEntry         = make::one(&sfSignerEntry,         STI_OBJECT, 11, "SignerEntry");

//内部对象（不常见）
SField const sfSigner              = make::one(&sfSigner,              STI_OBJECT, 16, "Signer");
//17还没有使用…
SField const sfMajority            = make::one(&sfMajority,            STI_OBJECT, 18, "Majority");

//对象数组
//array/1是为数组结尾保留的
//sfield const sfsigningaccounts=make:：one（&sfsigningaccounts，sti_array，2，“signingaccounts”）；//从未使用过。
SField const sfSigners         = make::one(&sfSigners,         STI_ARRAY, 3, "Signers", SField::sMD_Default, SField::notSigning);
SField const sfSignerEntries   = make::one(&sfSignerEntries,   STI_ARRAY, 4, "SignerEntries");
SField const sfTemplate        = make::one(&sfTemplate,        STI_ARRAY, 5, "Template");
SField const sfNecessary       = make::one(&sfNecessary,       STI_ARRAY, 6, "Necessary");
SField const sfSufficient      = make::one(&sfSufficient,      STI_ARRAY, 7, "Sufficient");
SField const sfAffectedNodes   = make::one(&sfAffectedNodes,   STI_ARRAY, 8, "AffectedNodes");
SField const sfMemos           = make::one(&sfMemos,           STI_ARRAY, 9, "Memos");

//对象数组（不常见）
SField const sfMajorities      = make::one(&sfMajorities,      STI_ARRAY, 16, "Majorities");

SField::SField (SerializedTypeID tid, int fv, const char* fn,
                int meta, IsSigning signing)
    : fieldCode (field_code (tid, fv))
    , fieldType (tid)
    , fieldValue (fv)
    , fieldName (fn)
    , fieldMeta (meta)
    , fieldNum (++num)
    , signingField (signing)
    , jsonName (getName ())
{
}

SField::SField (int fc)
    : fieldCode (fc)
    , fieldType (STI_UNKNOWN)
    , fieldValue (0)
    , fieldMeta (sMD_Never)
    , fieldNum (++num)
    , signingField (IsSigning::yes)
    , jsonName (getName ())
{
}

//使用map mutex调用以保护num。
//这是自然完成的，没有额外的费用
//来自GetField（int代码）。
SField::SField (SerializedTypeID tid, int fv)
        : fieldCode (field_code (tid, fv))
        , fieldType (tid)
        , fieldValue (fv)
        , fieldMeta (sMD_Default)
        , fieldNum (++num)
        , signingField (IsSigning::yes)
{
    fieldName = std::to_string (tid) + '/' + std::to_string (fv);
    jsonName = getName ();
    assert ((fv != 1) || ((tid != STI_ARRAY) && (tid != STI_OBJECT)));
}

SField const&
SField::getField (int code)
{
    auto it = knownCodeToField.find (code);

    if (it != knownCodeToField.end ())
    {
//99%以上的时间，它将是一个有效的已知字段
        return * (it->second);
    }

    int type = code >> 16;
    int field = code & 0xffff;

//不要动态扩展没有二进制编码的类型。
    if ((field > 255) || (code < 0))
        return sfInvalid;

    switch (type)
    {
//我们愿意动态扩展的类型
//类型（通用）
    case STI_UINT16:
    case STI_UINT32:
    case STI_UINT64:
    case STI_HASH128:
    case STI_HASH256:
    case STI_AMOUNT:
    case STI_VL:
    case STI_ACCOUNT:
    case STI_OBJECT:
    case STI_ARRAY:
//类型（不常见）
    case STI_UINT8:
    case STI_HASH160:
    case STI_PATHSET:
    case STI_VECTOR256:
        break;

    default:
        return sfInvalid;
    }

    {
//在运行时数据库中查找，如果不查找则创建
//但仍然存在。
        StaticScopedLockType sl (SField_mutex);

        auto it = unknownCodeToField.find (code);

        if (it != unknownCodeToField.end ())
            return * (it->second);
        return *(unknownCodeToField[code] = std::unique_ptr<SField const>(
                       new SField(safe_cast<SerializedTypeID>(type), field)));
    }
}

int SField::compare (SField const& f1, SField const& f2)
{
//-1=f1在f2之前，0=非法组合，1=f1在f2之后
    if ((f1.fieldCode <= 0) || (f2.fieldCode <= 0))
        return 0;

    if (f1.fieldCode < f2.fieldCode)
        return -1;

    if (f2.fieldCode < f1.fieldCode)
        return 1;

    return 0;
}

std::string SField::getName () const
{
    if (!fieldName.empty ())
        return fieldName;

    if (fieldValue == 0)
        return "";

    return std::to_string(safe_cast<int> (fieldType)) + "/" +
            std::to_string(fieldValue);
}

SField const&
SField::getField (std::string const& fieldName)
{
    for (auto const & fieldPair : knownCodeToField)
    {
        if (fieldPair.second->fieldName == fieldName)
            return * (fieldPair.second);
    }
    {
        StaticScopedLockType sl (SField_mutex);

        for (auto const & fieldPair : unknownCodeToField)
        {
            if (fieldPair.second->fieldName == fieldName)
                return * (fieldPair.second);
        }
    }
    return sfInvalid;
}

} //涟漪
