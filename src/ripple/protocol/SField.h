
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

#ifndef RIPPLE_PROTOCOL_SFIELD_H_INCLUDED
#define RIPPLE_PROTOCOL_SFIELD_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/json/json_value.h>
#include <cstdint>
#include <utility>

namespace ripple {

/*

有些字段的含义不同
    默认值与不存在。
        例子：
            信任线上的质量

**/


//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//前锋
class STAccount;
class STAmount;
class STBlob;
template <std::size_t>
class STBitString;
template <class>
class STInteger;
class STVector256;

enum SerializedTypeID
{
//特殊类型
    STI_UNKNOWN     = -2,
    STI_DONE        = -1,
    STI_NOTPRESENT  = 0,

////类型（公共）
    STI_UINT16 = 1,
    STI_UINT32 = 2,
    STI_UINT64 = 3,
    STI_HASH128 = 4,
    STI_HASH256 = 5,
    STI_AMOUNT = 6,
    STI_VL = 7,
    STI_ACCOUNT = 8,
//9-13保留
    STI_OBJECT = 14,
    STI_ARRAY = 15,

//类型（不常见）
    STI_UINT8 = 16,
    STI_HASH160 = 17,
    STI_PATHSET = 18,
    STI_VECTOR256 = 19,

//高级类型
//无法在其他类型内序列化
    STI_TRANSACTION = 10001,
    STI_LEDGERENTRY = 10002,
    STI_VALIDATION  = 10003,
    STI_METADATA    = 10004,
};

//常量表达式
inline
int
field_code(SerializedTypeID id, int index)
{
    return (safe_cast<int>(id) << 16) | index;
}

//常量表达式
inline
int
field_code(int id, int index)
{
    return (id << 16) | index;
}

/*标识字段。

    字段是标记已签名事务中的数据所必需的，以便
    事务的二进制格式可以规范化。

    这些字段有两类：

    1。编译时创建的。
    2。在运行时创建的。

    两者都是常量。只能在fieldname.cpp中创建类别1。
    这在编译时强制执行。类别2只能由
    使用尚未使用的FieldType和FieldValue（或
    等效字段代码）。

    每个sfield一旦构建，就一直存在到程序终止，并且
    每个fieldtype/fieldvalue对只有一个实例，它服务于整个
    应用。
**/

class SField
{
public:
    enum
    {
        sMD_Never          = 0x00,
sMD_ChangeOrig     = 0x01, //原值变化时
sMD_ChangeNew      = 0x02, //变化时的新值
sMD_DeleteFinal    = 0x04, //删除时的最终值
sMD_Create         = 0x08, //创建时的值
sMD_Always         = 0x10, //当包含它的节点完全受影响时的值
        sMD_Default        = sMD_ChangeOrig | sMD_ChangeNew | sMD_DeleteFinal | sMD_Create
    };

    enum class IsSigning : unsigned char
    {
        no,
        yes
    };
    static IsSigning const notSigning = IsSigning::no;

int const               fieldCode;      //（类型<<16）索引
SerializedTypeID const  fieldType;      //斯蒂伊*
int const               fieldValue;     //协议代码
    std::string             fieldName;
    int                     fieldMeta;
    int                     fieldNum;
    IsSigning const         signingField;
    std::string             jsonName;

    SField(SField const&) = delete;
    SField& operator=(SField const&) = delete;
    SField(SField&&) = default;

protected:
//只能从fieldname.cpp调用这些构造函数
    SField (SerializedTypeID tid, int fv, const char* fn,
            int meta = sMD_Default, IsSigning signing = IsSigning::yes);
    explicit SField (int fc);
    SField (SerializedTypeID id, int val);

public:
//如果需要，getfield将动态构造一个新的sfield
    static const SField& getField (int fieldCode);
    static const SField& getField (std::string const& fieldName);
    static const SField& getField (int type, int value)
    {
        return getField (field_code (type, value));
    }
    static const SField& getField (SerializedTypeID type, int value)
    {
        return getField (field_code (type, value));
    }

    std::string getName () const;
    bool hasName () const
    {
        return !fieldName.empty ();
    }

    std::string const& getJsonName () const
    {
        return jsonName;
    }

    bool isGeneric () const
    {
        return fieldCode == 0;
    }
    bool isInvalid () const
    {
        return fieldCode == -1;
    }
    bool isUseful () const
    {
        return fieldCode > 0;
    }
    bool isKnown () const
    {
        return fieldType != STI_UNKNOWN;
    }
    bool isBinary () const
    {
        return fieldValue < 256;
    }

//可丢弃字段是无法序列化的字段，并且
//应在序列化期间丢弃，如“hash”。
//无法序列化该对象内的对象哈希，
//但是您可以在JSON表示中使用它。
    bool isDiscardable () const
    {
        return fieldValue > 256;
    }

    int getCode () const
    {
        return fieldCode;
    }
    int getNum () const
    {
        return fieldNum;
    }
    static int getNumFields ()
    {
        return num;
    }

    bool isSigningField () const
    {
        return signingField == IsSigning::yes;
    }
    bool shouldMeta (int c) const
    {
        return (fieldMeta & c) != 0;
    }
    void setMeta (int c)
    {
        fieldMeta = c;
    }

    bool shouldInclude (bool withSigningField) const
    {
        return (fieldValue < 256) &&
            (withSigningField || (signingField == IsSigning::yes));
    }

    bool operator== (const SField& f) const
    {
        return fieldCode == f.fieldCode;
    }

    bool operator!= (const SField& f) const
    {
        return fieldCode != f.fieldCode;
    }

    static int compare (const SField& f1, const SField& f2);

struct make;  //公开，但仍然是一个实现细节

private:
    static int num;
};

/*编译时已知类型的字段。*/
template <class T>
struct TypedField : SField
{
    using type = T;

    template <class... Args>
    explicit
    TypedField (Args&&... args)
        : SField(std::forward<Args>(args)...)
    {
    }

    TypedField (TypedField&& u)
        : SField(std::move(u))
    {
    }
};

/*指示boost：：可选字段语义。*/
template <class T>
struct OptionaledField
{
    TypedField<T> const* f;

    explicit
    OptionaledField (TypedField<T> const& f_)
        : f (&f_)
    {
    }
};

template <class T>
inline
OptionaledField<T>
operator~(TypedField<T> const& f)
{
    return OptionaledField<T>(f);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

using SF_U8 = TypedField<STInteger<std::uint8_t>>;
using SF_U16 = TypedField<STInteger<std::uint16_t>>;
using SF_U32 = TypedField<STInteger<std::uint32_t>>;
using SF_U64 = TypedField<STInteger<std::uint64_t>>;
using SF_U128 = TypedField<STBitString<128>>;
using SF_U160 = TypedField<STBitString<160>>;
using SF_U256 = TypedField<STBitString<256>>;
using SF_Account = TypedField<STAccount>;
using SF_Amount = TypedField<STAmount>;
using SF_Blob = TypedField<STBlob>;
using SF_Vec256 = TypedField<STVector256>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

extern SField const sfInvalid;
extern SField const sfGeneric;
extern SField const sfLedgerEntry;
extern SField const sfTransaction;
extern SField const sfValidation;
extern SField const sfMetadata;

//8位整数
extern SF_U8 const sfCloseResolution;
extern SF_U8 const sfMethod;
extern SF_U8 const sfTransactionResult;
extern SF_U8 const sfTickSize;

//16位整数
extern SF_U16 const sfLedgerEntryType;
extern SF_U16 const sfTransactionType;
extern SF_U16 const sfSignerWeight;

//32位整数（公共）
extern SF_U32 const sfFlags;
extern SF_U32 const sfSourceTag;
extern SF_U32 const sfSequence;
extern SF_U32 const sfPreviousTxnLgrSeq;
extern SF_U32 const sfLedgerSequence;
extern SF_U32 const sfCloseTime;
extern SF_U32 const sfParentCloseTime;
extern SF_U32 const sfSigningTime;
extern SF_U32 const sfExpiration;
extern SF_U32 const sfTransferRate;
extern SF_U32 const sfWalletSize;
extern SF_U32 const sfOwnerCount;
extern SF_U32 const sfDestinationTag;

//32位整数（不常见）
extern SF_U32 const sfHighQualityIn;
extern SF_U32 const sfHighQualityOut;
extern SF_U32 const sfLowQualityIn;
extern SF_U32 const sfLowQualityOut;
extern SF_U32 const sfQualityIn;
extern SF_U32 const sfQualityOut;
extern SF_U32 const sfStampEscrow;
extern SF_U32 const sfBondAmount;
extern SF_U32 const sfLoadFee;
extern SF_U32 const sfOfferSequence;
extern SF_U32 const sfFirstLedgerSequence;  //已弃用：不要使用
extern SF_U32 const sfLastLedgerSequence;
extern SF_U32 const sfTransactionIndex;
extern SF_U32 const sfOperationLimit;
extern SF_U32 const sfReferenceFeeUnits;
extern SF_U32 const sfReserveBase;
extern SF_U32 const sfReserveIncrement;
extern SF_U32 const sfSetFlag;
extern SF_U32 const sfClearFlag;
extern SF_U32 const sfSignerQuorum;
extern SF_U32 const sfCancelAfter;
extern SF_U32 const sfFinishAfter;
extern SF_U32 const sfSignerListID;
extern SF_U32 const sfSettleDelay;

//64位整数
extern SF_U64 const sfIndexNext;
extern SF_U64 const sfIndexPrevious;
extern SF_U64 const sfBookNode;
extern SF_U64 const sfOwnerNode;
extern SF_U64 const sfBaseFee;
extern SF_U64 const sfExchangeRate;
extern SF_U64 const sfLowNode;
extern SF_U64 const sfHighNode;
extern SF_U64 const sfDestinationNode;
extern SF_U64 const sfCookie;


//128位
extern SF_U128 const sfEmailHash;

//160位（公共）
extern SF_U160 const sfTakerPaysCurrency;
extern SF_U160 const sfTakerPaysIssuer;
extern SF_U160 const sfTakerGetsCurrency;
extern SF_U160 const sfTakerGetsIssuer;

//256位（公共）
extern SF_U256 const sfLedgerHash;
extern SF_U256 const sfParentHash;
extern SF_U256 const sfTransactionHash;
extern SF_U256 const sfAccountHash;
extern SF_U256 const sfPreviousTxnID;
extern SF_U256 const sfLedgerIndex;
extern SF_U256 const sfWalletLocator;
extern SF_U256 const sfRootIndex;
extern SF_U256 const sfAccountTxnID;

//256位（不常见）
extern SF_U256 const sfBookDirectory;
extern SF_U256 const sfInvoiceID;
extern SF_U256 const sfNickname;
extern SF_U256 const sfAmendment;
extern SF_U256 const sfTicketID;
extern SF_U256 const sfDigest;
extern SF_U256 const sfPayChannel;
extern SF_U256 const sfConsensusHash;
extern SF_U256 const sfCheckID;

//货币金额（普通）
extern SF_Amount const sfAmount;
extern SF_Amount const sfBalance;
extern SF_Amount const sfLimitAmount;
extern SF_Amount const sfTakerPays;
extern SF_Amount const sfTakerGets;
extern SF_Amount const sfLowLimit;
extern SF_Amount const sfHighLimit;
extern SF_Amount const sfFee;
extern SF_Amount const sfSendMax;
extern SF_Amount const sfDeliverMin;

//货币金额（不常见）
extern SF_Amount const sfMinimumOffer;
extern SF_Amount const sfRippleEscrow;
extern SF_Amount const sfDeliveredAmount;

//可变长度（公共）
extern SF_Blob const sfPublicKey;
extern SF_Blob const sfMessageKey;
extern SF_Blob const sfSigningPubKey;
extern SF_Blob const sfTxnSignature;
extern SF_Blob const sfSignature;
extern SF_Blob const sfDomain;
extern SF_Blob const sfFundCode;
extern SF_Blob const sfRemoveCode;
extern SF_Blob const sfExpireCode;
extern SF_Blob const sfCreateCode;
extern SF_Blob const sfMemoType;
extern SF_Blob const sfMemoData;
extern SF_Blob const sfMemoFormat;

//可变长度（不常见）
extern SF_Blob const sfFulfillment;
extern SF_Blob const sfCondition;
extern SF_Blob const sfMasterSignature;

//解释
extern SF_Account const sfAccount;
extern SF_Account const sfOwner;
extern SF_Account const sfDestination;
extern SF_Account const sfIssuer;
extern SF_Account const sfAuthorize;
extern SF_Account const sfUnauthorize;
extern SF_Account const sfTarget;
extern SF_Account const sfRegularKey;

//路径集
extern SField const sfPaths;

//256位矢量
extern SF_Vec256 const sfIndexes;
extern SF_Vec256 const sfHashes;
extern SF_Vec256 const sfAmendments;

//内部对象
//对象/1是为对象结尾保留的
extern SField const sfTransactionMetaData;
extern SField const sfCreatedNode;
extern SField const sfDeletedNode;
extern SField const sfModifiedNode;
extern SField const sfPreviousFields;
extern SField const sfFinalFields;
extern SField const sfNewFields;
extern SField const sfTemplateEntry;
extern SField const sfMemo;
extern SField const sfSignerEntry;
extern SField const sfSigner;
extern SField const sfMajority;

//对象数组
//array/1是为数组结尾保留的
//extern sfield const sfsigningaccounts；//从未使用过。
extern SField const sfSigners;
extern SField const sfSignerEntries;
extern SField const sfTemplate;
extern SField const sfNecessary;
extern SField const sfSufficient;
extern SField const sfAffectedNodes;
extern SField const sfMemos;
extern SField const sfMajorities;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

} //涟漪

#endif
