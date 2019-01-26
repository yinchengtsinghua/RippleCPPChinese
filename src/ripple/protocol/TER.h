
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
    版权所有（c）2012-2018 Ripple Labs Inc.

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

#ifndef RIPPLE_PROTOCOL_TER_H_INCLUDED
#define RIPPLE_PROTOCOL_TER_H_INCLUDED

#include <ripple/basics/safe_cast.h>
#include <ripple/json/json_value.h>

#include <boost/optional.hpp>
#include <ostream>
#include <string>

namespace ripple {

//请参阅https://ripple.com/wiki/transaction\u错误
//
//“事务引擎结果”
//或事务错误。
//
using TERUnderlyingType = int;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TELcodes : TERUnderlyingType
{
//注：范围稳定。确切的数字是不稳定的。使用令牌。

//- 399。-300:L本地错误（交易费用不足，超过本地限额）
//仅在非共识处理期间有效。
//启示：
//-未转发
//-不收费检查
    telLOCAL_ERROR  = -399,
    telBAD_DOMAIN,
    telBAD_PATH_COUNT,
    telBAD_PUBLIC_KEY,
    telFAILED_PROCESSING,
    telINSUF_FEE_P,
    telNO_DST_PARTIAL,
    telCAN_NOT_QUEUE,
    telCAN_NOT_QUEUE_BALANCE,
    telCAN_NOT_QUEUE_BLOCKS,
    telCAN_NOT_QUEUE_BLOCKED,
    telCAN_NOT_QUEUE_FEE,
    telCAN_NOT_QUEUE_FULL
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TEMcodes : TERUnderlyingType
{
//注：范围稳定。确切的数字是不稳定的。使用令牌。

//- 299。-200:m格式错误（签名错误）
//原因：
//-事务已损坏。
//启示：
//-未应用
//-未转发
//-拒绝
//-无法在任何想象的分类帐中成功。
    temMALFORMED    = -299,

    temBAD_AMOUNT,
    temBAD_CURRENCY,
    temBAD_EXPIRATION,
    temBAD_FEE,
    temBAD_ISSUER,
    temBAD_LIMIT,
    temBAD_OFFER,
    temBAD_PATH,
    temBAD_PATH_LOOP,
    temBAD_SEND_XRP_LIMIT,
    temBAD_SEND_XRP_MAX,
    temBAD_SEND_XRP_NO_DIRECT,
    temBAD_SEND_XRP_PARTIAL,
    temBAD_SEND_XRP_PATHS,
    temBAD_SEQUENCE,
    temBAD_SIGNATURE,
    temBAD_SRC_ACCOUNT,
    temBAD_TRANSFER_RATE,
    temDST_IS_SRC,
    temDST_NEEDED,
    temINVALID,
    temINVALID_FLAG,
    temREDUNDANT,
    temRIPPLE_EMPTY,
    temDISABLED,
    temBAD_SIGNER,
    temBAD_QUORUM,
    temBAD_WEIGHT,
    temBAD_TICK_SIZE,
    temINVALID_ACCOUNT_ID,
    temCANNOT_PREAUTH_SELF,

//不应返回内部使用的中间结果。
    temUNCERTAIN,
    temUNKNOWN
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TEFcodes : TERUnderlyingType
{
//注：范围稳定。确切的数字是不稳定的。使用令牌。

//- 199。- 100：F
//故障（之前使用的序列号）
//
//原因：
//-由于分类帐状态，交易无法成功。
//-意外的分类帐状态。
//C++异常。
//
//启示：
//-未应用
//-未转发
//-可以在想象的账本中成功。
    tefFAILURE      = -199,
    tefALREADY,
    tefBAD_ADD_AUTH,
    tefBAD_AUTH,
    tefBAD_LEDGER,
    tefCREATED,
    tefEXCEPTION,
    tefINTERNAL,
tefNO_AUTH_REQUIRED,    //如果不需要身份验证，则无法设置身份验证。
    tefPAST_SEQ,
    tefWRONG_PRIOR,
    tefMASTER_DISABLED,
    tefMAX_LEDGER,
    tefBAD_SIGNATURE,
    tefBAD_QUORUM,
    tefNOT_MULTI_SIGNING,
    tefBAD_AUTH_MASTER,
    tefINVARIANT_FAILED,
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TERcodes : TERUnderlyingType
{
//注：范围稳定。确切的数字是不稳定的。使用令牌。

//- 99。- 1：R重试
//序列太高，没有用于txn费用的资金，发起-帐户
//不存在的
//
//原因：
//先前应用另一个可能不存在的交易可以
//允许此事务成功。
//
//启示：
//-未应用
//-可转发
//-指示txn已转发的结果：terqueued
//-未转发所有其他邮件。
//-以后可能成功
//-保持
//-按顺序钻孔，堵塞交易。
    terRETRY        = -99,
terFUNDS_SPENT,      //这是一个自由的事务，所以不要给网络带来负担。
terINSUF_FEE_B,      //不能交费，所以不负担网络。
terNO_ACCOUNT,       //不能交费，所以不负担网络。
terNO_AUTH,          //未授权持有借据。
terNO_LINE,          //内部标志。
terOWNERS,           //拥有者计数非零时无法成功。
terPRE_SEQ,          //不能支付费用，没有转发的意义，所以不要
//负担网络。
terLAST,             //在所有其他交易之后处理
terNO_RIPPLE,        //不允许有波纹
terQUEUED            //交易在TXQ中保持，直到费用下降。
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TEScodes : TERUnderlyingType
{
//注意：确切数字必须保持稳定。此代码按值存储
//在历史事务的元数据中。

//0:s成功（成功）
//原因：
//-成功。
//启示：
//-应用
//-转发
    tesSUCCESS      = 0
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

enum TECcodes : TERUnderlyingType
{
//注：准确数字必须保持稳定。这些代码存储在
//历史事务的元数据中的值。

//100…159 C
//仅收取索赔费（无良好路径的Ripple交易，支付至
//不存在的帐户，没有路径）
//
//原因：
//-成功，但没有达到最佳效果。
//-交易无效或无效，但使用该序列的索赔费
//号码。
//
//启示：
//-应用
//-转发
//
//仅允许作为appliedTransaction的返回代码！磁带录音机
//否则，视为大地。
//
//不要更改这些数字：它们出现在分类帐元数据中。
    tecCLAIM                    = 100,
    tecPATH_PARTIAL             = 101,
    tecUNFUNDED_ADD             = 102,
    tecUNFUNDED_OFFER           = 103,
    tecUNFUNDED_PAYMENT         = 104,
    tecFAILED_PROCESSING        = 105,
    tecDIR_FULL                 = 121,
    tecINSUF_RESERVE_LINE       = 122,
    tecINSUF_RESERVE_OFFER      = 123,
    tecNO_DST                   = 124,
    tecNO_DST_INSUF_XRP         = 125,
    tecNO_LINE_INSUF_RESERVE    = 126,
    tecNO_LINE_REDUNDANT        = 127,
    tecPATH_DRY                 = 128,
tecUNFUNDED                 = 129,  //弃用，老暧昧无着落。
    tecNO_ALTERNATIVE_KEY       = 130,
    tecNO_REGULAR_KEY           = 131,
    tecOWNERS                   = 132,
    tecNO_ISSUER                = 133,
    tecNO_AUTH                  = 134,
    tecNO_LINE                  = 135,
    tecINSUFF_FEE               = 136,
    tecFROZEN                   = 137,
    tecNO_TARGET                = 138,
    tecNO_PERMISSION            = 139,
    tecNO_ENTRY                 = 140,
    tecINSUFFICIENT_RESERVE     = 141,
    tecNEED_MASTER_KEY          = 142,
    tecDST_TAG_NEEDED           = 143,
    tecINTERNAL                 = 144,
    tecOVERSIZE                 = 145,
    tecCRYPTOCONDITION_ERROR    = 146,
    tecINVARIANT_FAILED         = 147,
    tecEXPIRED                  = 148,
    tecDUPLICATE                = 149,
    tecKILLED                   = 150
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//出于一般目的，返回TE*代码值的自由函数。
constexpr TERUnderlyingType TERtoInt (TELcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEMcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEFcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TERcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TEScodes v)
{ return safe_cast<TERUnderlyingType>(v); }

constexpr TERUnderlyingType TERtoInt (TECcodes v)
{ return safe_cast<TERUnderlyingType>(v); }

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//特定于所选错误代码范围的模板类。这个
//trait告诉std:：enable_如果允许哪些范围。
template <template<typename> class Trait>
class TERSubset
{
    TERUnderlyingType code_;

public:
//构造函数
    constexpr TERSubset() : code_ (tesSUCCESS) { }
    constexpr TERSubset (TERSubset const& rhs) = default;
    constexpr TERSubset (TERSubset&& rhs) = default;
private:
    constexpr explicit TERSubset (int rhs) : code_ (rhs) { }
public:
    static constexpr TERSubset fromInt (int from)
    {
        return TERSubset (from);
    }

//如果允许构造哪些类型，则特征告诉启用。
    template <typename T, typename = std::enable_if_t<Trait<T>::value>>
    constexpr TERSubset (T rhs)
    : code_ (TERtoInt (rhs))
    { }

//转让
    constexpr TERSubset& operator=(TERSubset const& rhs) = default;
    constexpr TERSubset& operator=(TERSubset&& rhs) = default;

//如果允许分配哪些类型，则特征告诉启用。
    template <typename T>
    constexpr auto
    operator= (T rhs) -> std::enable_if_t<Trait<T>::value, TERSubset&>
    {
        code_ = TERtoInt (rhs);
        return *this;
    }

//转换为bool。
    explicit operator bool() const
    {
        return code_ != tesSUCCESS;
    }

//转换为json:：value允许分配给json:：objects
//没有铸造。
    operator Json::Value() const
    {
        return Json::Value {code_};
    }

//流媒体运营商。
    friend std::ostream& operator<< (std::ostream& os, TERSubset const& rhs)
    {
        return os << rhs.code_;
    }

//返回基础值。不是一个类似的自由成员
//函数可以对枚举执行相同的操作。
//
//值得注意的是，我们考虑了显式转换运算符
//被拒绝了。考虑这个案例，从状态开始。
//
//类状态{
//int码；
//公众：
//状态（ter ter ter）
//：代码
//}
//
//如果ter具有显式的
//（未命名）转换为int。以避免这样的静默转换
//我们只提供命名转换。
    friend constexpr TERUnderlyingType TERtoInt (TERSubset v)
    {
        return v.code_;
    }
};

//比较运算符。
//只有当两个参数都返回int（如果用它们调用了tertiint）时才启用。
template <typename L, typename R>
constexpr auto
operator== (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) == TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator!= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) != TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator< (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) < TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator<= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) <= TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator> (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) > TERtoInt(rhs);
}

template <typename L, typename R>
constexpr auto
operator>= (L const& lhs, R const& rhs) -> std::enable_if_t<
    std::is_same<decltype (TERtoInt(lhs)), int>::value &&
    std::is_same<decltype (TERtoInt(rhs)), int>::value, bool>
{
    return TERtoInt(lhs) >= TERtoInt(rhs);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//使用traits构建一个tersubset，它可以从任何te*代码转换
//Enums*除外*技术代码：NotTec

//注：nottec对于事务处理程序中由预检返回的代码很有用。
//飞行前检查发生在签名检查之前。如果飞行前返回
//一个tec代码，那么恶意用户可以提交一个交易
//一笔很大的费用，不用这笔钱就可以从一个账户中扣除。
//帐户的有效签名。
template <typename FROM> class CanCvtToNotTEC           : public std::false_type {};
template <>              class CanCvtToNotTEC<TELcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEMcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEFcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TERcodes> : public std::true_type {};
template <>              class CanCvtToNotTEC<TEScodes> : public std::true_type {};

using NotTEC = TERSubset<CanCvtToNotTEC>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//使用traits构建一个tersubset，它可以从任何te*代码转换
//Enums和Nottec。
template <typename FROM> class CanCvtToTER           : public std::false_type {};
template <>              class CanCvtToTER<TELcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEMcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEFcodes> : public std::true_type {};
template <>              class CanCvtToTER<TERcodes> : public std::true_type {};
template <>              class CanCvtToTER<TEScodes> : public std::true_type {};
template <>              class CanCvtToTER<TECcodes> : public std::true_type {};
template <>              class CanCvtToTER<NotTEC>   : public std::true_type {};

//TER允许所有子集。
using TER = TERSubset<CanCvtToTER>;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool isTelLocal(TER x)
{
    return ((x) >= telLOCAL_ERROR && (x) < temMALFORMED);
}

inline bool isTemMalformed(TER x)
{
    return ((x) >= temMALFORMED && (x) < tefFAILURE);
}

inline bool isTefFailure(TER x)
{
    return ((x) >= tefFAILURE && (x) < terRETRY);
}

inline bool isTerRetry(TER x)
{
    return ((x) >= terRETRY && (x) < tesSUCCESS);
}

inline bool isTesSuccess(TER x)
{
    return ((x) == tesSUCCESS);
}

inline bool isTecClaim(TER x)
{
    return ((x) >= tecCLAIM);
}

bool
transResultInfo (TER code, std::string& token, std::string& text);

std::string
transToken (TER code);

std::string
transHuman (TER code);

boost::optional<TER>
transCode(std::string const& token);

} //涟漪

#endif
