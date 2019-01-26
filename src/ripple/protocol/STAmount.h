
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

#ifndef RIPPLE_PROTOCOL_STAMOUNT_H_INCLUDED
#define RIPPLE_PROTOCOL_STAMOUNT_H_INCLUDED

#include <ripple/basics/chrono.h>
#include <ripple/basics/LocalValue.h>
#include <ripple/protocol/SField.h>
#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/STBase.h>
#include <ripple/protocol/Issue.h>
#include <ripple/protocol/IOUAmount.h>
#include <ripple/protocol/XRPAmount.h>
#include <memory>

namespace ripple {

//内部形式：
//1：如果金额为零，则值为零，偏移量为-100。
//2：否则：
//法定抵销范围为-96至+80（含）
//值范围为10^15到（10^16-1）（含10^16-1）
//金额=值*[10^偏移量]

//导线形式：
//高8位是（偏移量+142），合法范围是，80到22包括在内
//低56位是值，合法范围是10^15到（10^16-1），包括
class STAmount
    : public STBase
{
public:
    using mantissa_type = std::uint64_t;
    using exponent_type = int;
    using rep = std::pair <mantissa_type, exponent_type>;

private:
    Issue mIssue;
    mantissa_type mValue;
    exponent_type mOffset;
bool mIsNative;      //ISXRP（Misue）的简写。
    bool mIsNegative;

public:
    using value_type = STAmount;

    static const int cMinOffset = -96;
    static const int cMaxOffset = 80;

//代码支持的最大本机值
    static const std::uint64_t cMinValue   = 1000000000000000ull;
    static const std::uint64_t cMaxValue   = 9999999999999999ull;
    static const std::uint64_t cMaxNative  = 9000000000000000000ull;

//网络上的最大本机值。
    static const std::uint64_t cMaxNativeN = 100000000000000000ull;
    static const std::uint64_t cNotNative  = 0x8000000000000000ull;
    static const std::uint64_t cPosNative  = 0x4000000000000000ull;

    static std::uint64_t const uRateOne;

//——————————————————————————————————————————————————————————————
    STAmount(SerialIter& sit, SField const& name);

    struct unchecked
    {
        explicit unchecked() = default;
    };

//不要称之为规范化
    STAmount (SField const& name, Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative, unchecked);

    STAmount (Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative, unchecked);

//调用规范化
    STAmount (SField const& name, Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative);

    STAmount (SField const& name, std::int64_t mantissa);

    STAmount (SField const& name,
        std::uint64_t mantissa = 0, bool negative = false);

    STAmount (SField const& name, Issue const& issue,
        std::uint64_t mantissa = 0, int exponent = 0, bool negative = false);

    STAmount (std::uint64_t mantissa = 0, bool negative = false);

    STAmount (Issue const& issue, std::uint64_t mantissa = 0, int exponent = 0,
        bool negative = false);

//vfalc我们有上一个签名时需要这样做吗？
    STAmount (Issue const& issue, std::uint32_t mantissa, int exponent = 0,
        bool negative = false);

    STAmount (Issue const& issue, std::int64_t mantissa, int exponent = 0);

    STAmount (Issue const& issue, int mantissa, int exponent = 0);

//对新型金额的传统支持
    STAmount (IOUAmount const& amount, Issue const& issue);
    STAmount (XRPAmount const& amount);

    STBase*
    copy (std::size_t n, void* buf) const override
    {
        return emplace(n, buf, *this);
    }

    STBase*
    move (std::size_t n, void* buf) override
    {
        return emplace(n, buf, std::move(*this));
    }

//——————————————————————————————————————————————————————————————

private:
    static
    std::unique_ptr<STAmount>
    construct (SerialIter&, SField const& name);

    void set (std::int64_t v);
    void canonicalize();

public:
//——————————————————————————————————————————————————————————————
//
//观察员
//
//——————————————————————————————————————————————————————————————

    int exponent() const noexcept { return mOffset; }
    bool native() const noexcept { return mIsNative; }
    bool negative() const noexcept { return mIsNegative; }
    std::uint64_t mantissa() const noexcept { return mValue; }
    Issue const& issue() const { return mIssue; }

//这三个被否决了
    Currency const& getCurrency() const { return mIssue.currency; }
    AccountID const& getIssuer() const { return mIssue.account; }

    int
    signum() const noexcept
    {
        return mValue ? (mIsNegative ? -1 : 1) : 0;
    }

    /*返回具有相同颁发者和货币的零值。*/
    STAmount
    zeroed() const
    {
        return STAmount (mIssue);
    }

    void
    setJson (Json::Value&) const;

    STAmount const&
    value() const noexcept
    {
        return *this;
    }

//——————————————————————————————————————————————————————————————
//
//算子
//
//——————————————————————————————————————————————————————————————

    explicit operator bool() const noexcept
    {
        return *this != beast::zero;
    }

    STAmount& operator+= (STAmount const&);
    STAmount& operator-= (STAmount const&);

    STAmount& operator= (beast::Zero)
    {
        clear();
        return *this;
    }

    STAmount& operator= (XRPAmount const& amount)
    {
        *this = STAmount (amount);
        return *this;
    }

//——————————————————————————————————————————————————————————————
//
//修改
//
//——————————————————————————————————————————————————————————————

    void negate()
    {
        if (*this != beast::zero)
            mIsNegative = !mIsNegative;
    }

    void clear()
    {
//-100用于允许0对小于小正值的值进行排序
//指数为负。
        mOffset = mIsNative ? 0 : -100;
        mValue = 0;
        mIsNegative = false;
    }

//复制货币和发行人时为零。
    void clear (STAmount const& saTmpl)
    {
        clear (saTmpl.mIssue);
    }

    void clear (Issue const& issue)
    {
        setIssue(issue);
        clear();
    }

    void setIssuer (AccountID const& uIssuer)
    {
        mIssue.account = uIssuer;
        setIssue(mIssue);
    }

    /*为此金额设置问题并更新misnive。*/
    void setIssue (Issue const& issue);

//——————————————————————————————————————————————————————————————
//
//斯塔基
//
//——————————————————————————————————————————————————————————————

    SerializedTypeID
    getSType() const override
    {
        return STI_AMOUNT;
    }

    std::string
    getFullText() const override;

    std::string
    getText() const override;

    Json::Value
    getJson (int) const override;

    void
    add (Serializer& s) const override;

    bool
    isEquivalent (const STBase& t) const override;

    bool
    isDefault() const override
    {
        return (mValue == 0) && mIsNative;
    }

    XRPAmount xrp () const;
    IOUAmount iou () const;
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//创造
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//vfalc todo参数类型应为质量而不是uint64
STAmount
amountFromQuality (std::uint64_t rate);

STAmount
amountFromString (Issue const& issue, std::string const& amount);

STAmount
amountFromJson (SField const& name, Json::Value const& v);

bool
amountFromJsonNoThrow (STAmount& result, Json::Value const& jvSource);

//iouamount和xrpamount定义为tamount，定义此
//这里的简单转换使编写通用代码更加容易
inline
STAmount const&
toSTAmount (STAmount const& a)
{
    return a;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//观察员
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline
bool
isLegalNet (STAmount const& value)
{
    return ! value.native() || (value.mantissa() <= STAmount::cMaxNativeN);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//算子
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

bool operator== (STAmount const& lhs, STAmount const& rhs);
bool operator<  (STAmount const& lhs, STAmount const& rhs);

inline
bool
operator!= (STAmount const& lhs, STAmount const& rhs)
{
    return !(lhs == rhs);
}

inline
bool
operator> (STAmount const& lhs, STAmount const& rhs)
{
    return rhs < lhs;
}

inline
bool
operator<= (STAmount const& lhs, STAmount const& rhs)
{
    return !(rhs < lhs);
}

inline
bool
operator>= (STAmount const& lhs, STAmount const& rhs)
{
    return !(lhs < rhs);
}

STAmount operator- (STAmount const& value);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//算术
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STAmount operator+ (STAmount const& v1, STAmount const& v2);
STAmount operator- (STAmount const& v1, STAmount const& v2);

STAmount
divide (STAmount const& v1, STAmount const& v2, Issue const& issue);

STAmount
multiply (STAmount const& v1, STAmount const& v2, Issue const& issue);

//按指定方向乘或除舍入结果
STAmount
mulRound (STAmount const& v1, STAmount const& v2,
    Issue const& issue, bool roundUp);

STAmount
divRound (STAmount const& v1, STAmount const& v2,
    Issue const& issue, bool roundUp);

//有人给你X，价格是多少？
//费率：越小越好，接受者越想要：进/出
//vfalc todo返回质量对象
std::uint64_t
getRate (STAmount const& offerOut, STAmount const& offerIn);

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

inline bool isXRP(STAmount const& amount)
{
    return isXRP (amount.issue().currency);
}

extern LocalValue<bool> stAmountCalcSwitchover;
extern LocalValue<bool> stAmountCalcSwitchover2;

/*RAII类设置和恢复Stamount Calc切换*/
class STAmountSO
{
public:
    explicit STAmountSO(NetClock::time_point const closeTime)
        : saved_(*stAmountCalcSwitchover)
        , saved2_(*stAmountCalcSwitchover2)
    {
        *stAmountCalcSwitchover = closeTime > soTime;
        *stAmountCalcSwitchover2 = closeTime > soTime2;
    }

    ~STAmountSO()
    {
        *stAmountCalcSwitchover = saved_;
        *stAmountCalcSwitchover2 = saved2_;
    }

//2015年12月28日星期一上午10:00:00（太平洋标准时间）
    static NetClock::time_point const soTime;

//2015年3月28日星期一上午10:00:00（太平洋标准时间）
    static NetClock::time_point const soTime2;

private:
    bool saved_;
    bool saved2_;
};

} //涟漪

#endif
