
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


#include <ripple/basics/contract.h>
#include <ripple/basics/Log.h>
#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/protocol/STAmount.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/beast/core/LexicalCast.h>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <iterator>
#include <memory>
#include <iostream>

namespace ripple {

LocalValue<bool> stAmountCalcSwitchover(true);
LocalValue<bool> stAmountCalcSwitchover2(true);

using namespace std::chrono_literals;
const NetClock::time_point STAmountSO::soTime{504640800s};

//2016年2月26日星期五太平洋标准时间下午9:00:00
const NetClock::time_point STAmountSO::soTime2{509864400s};

static const std::uint64_t tenTo14 = 100000000000000ull;
static const std::uint64_t tenTo14m1 = tenTo14 - 1;
static const std::uint64_t tenTo17 = tenTo14 * 1000;

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static
std::int64_t
getSNValue (STAmount const& amount)
{
    if (!amount.native ())
        Throw<std::runtime_error> ("amount is not native!");

    auto ret = static_cast<std::int64_t>(amount.mantissa ());

    assert (static_cast<std::uint64_t>(ret) == amount.mantissa ());

    if (amount.negative ())
        ret = -ret;

    return ret;
}

static
bool
areComparable (STAmount const& v1, STAmount const& v2)
{
    return v1.native() == v2.native() &&
        v1.issue().currency == v2.issue().currency;
}

STAmount::STAmount(SerialIter& sit, SField const& name)
    : STBase(name)
{
    std::uint64_t value = sit.get64 ();

//本地的
    if ((value & cNotNative) == 0)
    {
//积极的
        if ((value & cPosNative) != 0)
        {
            mValue = value & ~cPosNative;
            mOffset = 0;
            mIsNative = true;
            mIsNegative = false;
            return;
        }

//消极的
        if (value == 0)
            Throw<std::runtime_error> ("negative zero is not canonical");

        mValue = value;
        mOffset = 0;
        mIsNative = true;
        mIsNegative = true;
        return;
    }

    Issue issue;
    issue.currency.copyFrom (sit.get160 ());

    if (isXRP (issue.currency))
        Throw<std::runtime_error> ("invalid native currency");

    issue.account.copyFrom (sit.get160 ());

    if (isXRP (issue.account))
        Throw<std::runtime_error> ("invalid native account");

//10位用于偏移、符号和“非本机”标志
    int offset = static_cast<int>(value >> (64 - 10));

    value &= ~ (1023ull << (64 - 10));

    if (value)
    {
        bool isNegative = (offset & 256) == 0;
offset = (offset & 255) - 97; //使范围居中

        if (value < cMinValue ||
            value > cMaxValue ||
            offset < cMinOffset ||
            offset > cMaxOffset)
        {
            Throw<std::runtime_error> ("invalid currency value");
        }

        mIssue = issue;
        mValue = value;
        mOffset = offset;
        mIsNegative = isNegative;
        canonicalize();
        return;
    }

    if (offset != 512)
        Throw<std::runtime_error> ("invalid currency value");

    mIssue = issue;
    mValue = 0;
    mOffset = 0;
    mIsNegative = false;
    canonicalize();
}

STAmount::STAmount (SField const& name, Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative, unchecked)
    : STBase (name)
    , mIssue (issue)
    , mValue (mantissa)
    , mOffset (exponent)
    , mIsNative (native)
    , mIsNegative (negative)
{
}

STAmount::STAmount (Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative, unchecked)
    : mIssue (issue)
    , mValue (mantissa)
    , mOffset (exponent)
    , mIsNative (native)
    , mIsNegative (negative)
{
}


STAmount::STAmount (SField const& name, Issue const& issue,
        mantissa_type mantissa, exponent_type exponent,
            bool native, bool negative)
    : STBase (name)
    , mIssue (issue)
    , mValue (mantissa)
    , mOffset (exponent)
    , mIsNative (native)
    , mIsNegative (negative)
{
    canonicalize();
}

STAmount::STAmount (SField const& name, std::int64_t mantissa)
    : STBase (name)
    , mOffset (0)
    , mIsNative (true)
{
    set (mantissa);
}

STAmount::STAmount (SField const& name,
        std::uint64_t mantissa, bool negative)
    : STBase (name)
    , mValue (mantissa)
    , mOffset (0)
    , mIsNative (true)
    , mIsNegative (negative)
{
}

STAmount::STAmount (SField const& name, Issue const& issue,
        std::uint64_t mantissa, int exponent, bool negative)
    : STBase (name)
    , mIssue (issue)
    , mValue (mantissa)
    , mOffset (exponent)
    , mIsNegative (negative)
{
    canonicalize ();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STAmount::STAmount (std::uint64_t mantissa, bool negative)
    : mValue (mantissa)
    , mOffset (0)
    , mIsNative (true)
    , mIsNegative (mantissa != 0 && negative)
{
}

STAmount::STAmount (Issue const& issue,
    std::uint64_t mantissa, int exponent, bool negative)
    : mIssue (issue)
    , mValue (mantissa)
    , mOffset (exponent)
    , mIsNegative (negative)
{
    canonicalize ();
}

STAmount::STAmount (Issue const& issue,
        std::int64_t mantissa, int exponent)
    : mIssue (issue)
    , mOffset (exponent)
{
    set (mantissa);
    canonicalize ();
}

STAmount::STAmount (Issue const& issue,
        std::uint32_t mantissa, int exponent, bool negative)
    : STAmount (issue, safe_cast<std::uint64_t>(mantissa), exponent, negative)
{
}

STAmount::STAmount (Issue const& issue,
        int mantissa, int exponent)
    : STAmount (issue, safe_cast<std::int64_t>(mantissa), exponent)
{
}

//对新型金额的传统支持
STAmount::STAmount (IOUAmount const& amount, Issue const& issue)
    : mIssue (issue)
    , mOffset (amount.exponent ())
    , mIsNative (false)
    , mIsNegative (amount < beast::zero)
{
    if (mIsNegative)
        mValue = static_cast<std::uint64_t> (-amount.mantissa ());
    else
        mValue = static_cast<std::uint64_t> (amount.mantissa ());

    canonicalize ();
}

STAmount::STAmount (XRPAmount const& amount)
    : mOffset (0)
    , mIsNative (true)
    , mIsNegative (amount < beast::zero)
{
    if (mIsNegative)
        mValue = static_cast<std::uint64_t> (-amount.drops ());
    else
        mValue = static_cast<std::uint64_t> (amount.drops ());

    canonicalize ();
}

std::unique_ptr<STAmount>
STAmount::construct (SerialIter& sit, SField const& name)
{
    return std::make_unique<STAmount>(sit, name);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//转换
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
XRPAmount STAmount::xrp () const
{
    if (!mIsNative)
        Throw<std::logic_error> ("Cannot return non-native STAmount as XRPAmount");

    auto drops = static_cast<std::int64_t> (mValue);

    if (mIsNegative)
        drops = -drops;

    return { drops };
}

IOUAmount STAmount::iou () const
{
    if (mIsNative)
        Throw<std::logic_error> ("Cannot return native STAmount as IOUAmount");

    auto mantissa = static_cast<std::int64_t> (mValue);
    auto exponent = mOffset;

    if (mIsNegative)
        mantissa = -mantissa;

    return { mantissa, exponent };
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//算子
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STAmount& STAmount::operator+= (STAmount const& a)
{
    *this = *this + a;
    return *this;
}

STAmount& STAmount::operator-= (STAmount const& a)
{
    *this = *this - a;
    return *this;
}

STAmount operator+ (STAmount const& v1, STAmount const& v2)
{
    if (!areComparable (v1, v2))
        Throw<std::runtime_error> ("Can't add amounts that are't comparable!");

    if (v2 == beast::zero)
        return v1;

    if (v1 == beast::zero)
    {
//结果必须以v1货币和发行人为单位。
        return { v1.getFName (), v1.issue (),
            v2.mantissa (), v2.exponent (), v2.negative () };
    }

    if (v1.native ())
        return { v1.getFName (), getSNValue (v1) + getSNValue (v2) };

    int ov1 = v1.exponent (), ov2 = v2.exponent ();
    std::int64_t vv1 = static_cast<std::int64_t>(v1.mantissa ());
    std::int64_t vv2 = static_cast<std::int64_t>(v2.mantissa ());

    if (v1.negative ())
        vv1 = -vv1;

    if (v2.negative ())
        vv2 = -vv2;

    while (ov1 < ov2)
    {
        vv1 /= 10;
        ++ov1;
    }

    while (ov2 < ov1)
    {
        vv2 /= 10;
        ++ov2;
    }

//此添加不能溢出std:：int64_t。它可以溢出
//stamount和构造函数将抛出。

    std::int64_t fv = vv1 + vv2;

    if ((fv >= -10) && (fv <= 10))
        return { v1.getFName (), v1.issue () };

    if (fv >= 0)
        return STAmount { v1.getFName (), v1.issue (),
            static_cast<std::uint64_t>(fv), ov1, false };

    return STAmount { v1.getFName (), v1.issue (),
        static_cast<std::uint64_t>(-fv), ov1, true };
}

STAmount operator- (STAmount const& v1, STAmount const& v2)
{
    return v1 + (-v2);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::uint64_t const STAmount::uRateOne = getRate (STAmount (1), STAmount (1));

void
STAmount::setIssue (Issue const& issue)
{
    mIssue = std::move(issue);
    mIsNative = isXRP (*this);
}

//将报价转换为指数金额，以便按比率排序。
//一个接受者将以最好的、最低的、最优先的价格。
//（例如，一个接受者更喜欢用1比3来支付1比2。
//发盘人：接受者得到：发盘人卖给接受者的金额。
//-->offerin:接受方支付：接受方收到的报价金额。
//<--尿酸盐：正常化（报盘/报盘）
//对接受订单的人来说，较低的价格更好。
//以较低的价格，接受者得到更多的回报。
//如果报价毫无价值，则返回零。
std::uint64_t
getRate (STAmount const& offerOut, STAmount const& offerIn)
{
    if (offerOut == beast::zero)
        return 0;
    try
    {
        STAmount r = divide (offerIn, offerOut, noIssue());
if (r == beast::zero) //报价太好了
            return 0;
        assert ((r.exponent() >= -100) && (r.exponent() <= 155));
        std::uint64_t ret = r.exponent() + 100;
        return (ret << (64 - 8)) | r.mantissa();
    }
    catch (std::exception const&)
    {
    }

//溢出——非常糟糕的报价
    return 0;
}

void STAmount::setJson (Json::Value& elem) const
{
    elem = Json::objectValue;

    if (!mIsNative)
    {
//指定货币或发行人无效是错误的。
//杰森。
        elem[jss::value]      = getText ();
        elem[jss::currency]   = to_string (mIssue.currency);
        elem[jss::issuer]     = to_string (mIssue.account);
    }
    else
    {
        elem = getText ();
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//斯塔基
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

std::string
STAmount::getFullText () const
{
    std::string ret;

    ret.reserve(64);
    ret = getText () + "/" + to_string (mIssue.currency);

    if (!mIsNative)
    {
        ret += "/";

        if (isXRP (*this))
            ret += "0";
        else if (mIssue.account == noAccount())
            ret += "1";
        else
            ret += to_string (mIssue.account);
    }

    return ret;
}

std::string
STAmount::getText () const
{
//保持完整的内部准确度，但如果可以的话，让人更友好
    if (*this == beast::zero)
        return "0";

    std::string const raw_value (std::to_string (mValue));
    std::string ret;

    if (mIsNegative)
        ret.append (1, '-');

    bool const scientific ((mOffset != 0) && ((mOffset < -25) || (mOffset > -5)));

    if (mIsNative || scientific)
    {
        ret.append (raw_value);

        if (scientific)
        {
            ret.append (1, 'e');
            ret.append (std::to_string (mOffset));
        }

        return ret;
    }

    assert (mOffset + 43 > 0);

    size_t const pad_prefix = 27;
    size_t const pad_suffix = 23;

    std::string val;
    val.reserve (raw_value.length () + pad_prefix + pad_suffix);
    val.append (pad_prefix, '0');
    val.append (raw_value);
    val.append (pad_suffix, '0');

    size_t const offset (mOffset + 43);

    auto pre_from (val.begin ());
    auto const pre_to (val.begin () + offset);

    auto const post_from (val.begin () + offset);
    auto post_to (val.end ());

//裁剪前导零。利用这样一个事实
//固定数量的前导零并跳过它们。
    if (std::distance (pre_from, pre_to) > pad_prefix)
        pre_from += pad_prefix;

    assert (post_to >= post_from);

    pre_from = std::find_if (pre_from, pre_to,
        [](char c)
        {
            return c != '0';
        });

//裁剪尾随零。利用这样一个事实
//固定数量的尾随零并跳过它们。
    if (std::distance (post_from, post_to) > pad_suffix)
        post_to -= pad_suffix;

    assert (post_to >= post_from);

    post_to = std::find_if(
        std::make_reverse_iterator (post_to),
        std::make_reverse_iterator (post_from),
        [](char c)
        {
            return c != '0';
        }).base();

//组装输出：
    if (pre_from == pre_to)
        ret.append (1, '0');
    else
        ret.append(pre_from, pre_to);

    if (post_to != post_from)
    {
        ret.append (1, '.');
        ret.append (post_from, post_to);
    }

    return ret;
}

Json::Value
STAmount::getJson (int) const
{
    Json::Value elem;
    setJson (elem);
    return elem;
}

void
STAmount::add (Serializer& s) const
{
    if (mIsNative)
    {
        assert (mOffset == 0);

        if (!mIsNegative)
            s.add64 (mValue | cPosNative);
        else
            s.add64 (mValue);
    }
    else
    {
        if (*this == beast::zero)
            s.add64 (cNotNative);
else if (mIsNegative) //512=非本地
            s.add64 (mValue | (static_cast<std::uint64_t>(mOffset + 512 + 97) << (64 - 10)));
else //256＝正
            s.add64 (mValue | (static_cast<std::uint64_t>(mOffset + 512 + 256 + 97) << (64 - 10)));

        s.add160 (mIssue.currency);
        s.add160 (mIssue.account);
    }
}

bool
STAmount::isEquivalent (const STBase& t) const
{
    const STAmount* v = dynamic_cast<const STAmount*> (&t);
    return v && (*v == *this);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//金额=mValue*[10^moffset]
//表示范围为10^80-10^（-80）。
//
//在电线上：
//-xrp的高位为0，发行货币的高位为1。
//-下一位为1表示正，0表示负（除了0发行货币外，
//是0x80000000000000的特殊情况
//-对于已发行货币，接下来的8位是（moffset+97）。
//+97使该值始终为正值。
//-其余位为有效数字（尾数）
//发行货币是54位，本地货币是62位
//（但对于10^17滴的最大值，xrp只需要57位）
//
//如果金额为零，则mValue为零，否则它在范围内。
//10^15至（10^16-1）（含10^16-1）。
//moffset在-96到+80范围内。
void STAmount::canonicalize ()
{
    if (isXRP (*this))
    {
//本国货币金额的偏移量应始终为零。
        mIsNative = true;

        if (mValue == 0)
        {
            mOffset = 0;
            mIsNegative = false;
            return;
        }

        while (mOffset < 0)
        {
            mValue /= 10;
            ++mOffset;
        }

        while (mOffset > 0)
        {
            mValue *= 10;
            --mOffset;
        }

        if (mValue > cMaxNativeN)
            Throw<std::runtime_error> ("Native currency amount out of range");

        return;
    }

    mIsNative = false;

    if (mValue == 0)
    {
        mOffset = -100;
        mIsNegative = false;
        return;
    }

    while ((mValue < cMinValue) && (mOffset > cMinOffset))
    {
        mValue *= 10;
        --mOffset;
    }

    while (mValue > cMaxValue)
    {
        if (mOffset >= cMaxOffset)
            Throw<std::runtime_error> ("value overflow");

        mValue /= 10;
        ++mOffset;
    }

    if ((mOffset < cMinOffset) || (mValue < cMinValue))
    {
        mValue = 0;
        mIsNegative = false;
        mOffset = -100;
        return;
    }

    if (mOffset > cMaxOffset)
        Throw<std::runtime_error> ("value overflow");

    assert ((mValue == 0) || ((mValue >= cMinValue) && (mValue <= cMaxValue)));
    assert ((mValue == 0) || ((mOffset >= cMinOffset) && (mOffset <= cMaxOffset)));
    assert ((mValue != 0) || (mOffset != -100));
}

void STAmount::set (std::int64_t v)
{
    if (v < 0)
    {
        mIsNegative = true;
        mValue = static_cast<std::uint64_t>(-v);
    }
    else
    {
        mIsNegative = false;
        mValue = static_cast<std::uint64_t>(v);
    }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

STAmount
amountFromQuality (std::uint64_t rate)
{
    if (rate == 0)
        return STAmount (noIssue());

    std::uint64_t mantissa = rate & ~ (255ull << (64 - 8));
    int exponent = static_cast<int>(rate >> (64 - 8)) - 100;

    return STAmount (noIssue(), mantissa, exponent);
}

STAmount
amountFromString (Issue const& issue, std::string const& amount)
{
    static boost::regex const reNumber (
"^"                       //字符串的开头
"([-+]?)"                 //（可选）+或-字符
"(0|[1-9][0-9]*)"         //一个数字（没有前导零，除非0）
"(\\.([0-9]+))?"          //（可选）句点后接任意数字
"([eE]([+-]?)([0-9]+))?"  //（可选）e，可选+或-任何数字
        "$",
        boost::regex_constants::optimize);

    boost::smatch match;

    if (!boost::regex_match (amount, match, reNumber))
        Throw<std::runtime_error> ("Number '" + amount + "' is not valid");

//匹配字段：
//0 =整体输入
//1 =符号
//2=整数部分
//3=整分数（带''）
//4=分数（不带“.”）
//5=整指数（带‘e’）
//6=指数符号
//7=指数

//为什么是32？这不是16岁吗？
    if ((match[2].length () + match[4].length ()) > 32)
        Throw<std::runtime_error> ("Number '" + amount + "' is overlong");

    bool negative = (match[1].matched && (match[1] == "-"));

//无法使用分数表示指定xrp
    if (isXRP(issue) && match[3].matched)
        Throw<std::runtime_error> ("XRP must be specified in integral drops.");

    std::uint64_t mantissa;
    int exponent;

if (!match[4].matched) //仅整数
    {
        mantissa = beast::lexicalCastThrow <std::uint64_t> (std::string (match[2]));
        exponent = 0;
    }
    else
    {
//整数和分数
        mantissa = beast::lexicalCastThrow <std::uint64_t> (match[2] + match[4]);
        exponent = -(match[4].length ());
    }

    if (match[5].matched)
    {
//我们有一个指数
        if (match[6].matched && (match[6] == "-"))
            exponent -= beast::lexicalCastThrow <int> (std::string (match[7]));
        else
            exponent += beast::lexicalCastThrow <int> (std::string (match[7]));
    }

    return { issue, mantissa, exponent, negative };
}

STAmount
amountFromJson (SField const& name, Json::Value const& v)
{
    STAmount::mantissa_type mantissa = 0;
    STAmount::exponent_type exponent = 0;
    bool negative = false;
    Issue issue;

    Json::Value value;
    Json::Value currency;
    Json::Value issuer;

    if (v.isNull())
    {
        Throw<std::runtime_error> ("XRP may not be specified with a null Json value");
    }
    else if (v.isObject())
    {
        value       = v[jss::value];
        currency    = v[jss::currency];
        issuer      = v[jss::issuer];
    }
    else if (v.isArray())
    {
        value = v.get (Json::UInt (0), 0);
        currency = v.get (Json::UInt (1), Json::nullValue);
        issuer = v.get (Json::UInt (2), Json::nullValue);
    }
    else if (v.isString ())
    {
        std::string val = v.asString ();
        std::vector<std::string> elements;
        boost::split (elements, val, boost::is_any_of ("\t\n\r ,/"));

        if (elements.size () > 3)
            Throw<std::runtime_error> ("invalid amount string");

        value = elements[0];

        if (elements.size () > 1)
            currency = elements[1];

        if (elements.size () > 2)
            issuer = elements[2];
    }
    else
    {
        value = v;
    }

    bool const native = ! currency.isString () ||
        currency.asString ().empty () ||
        (currency.asString () == systemCurrencyCode());

    if (native)
    {
        if (v.isObjectOrNull ())
            Throw<std::runtime_error> ("XRP may not be specified as an object");
        issue = xrpIssue ();
    }
    else
    {
//非XRP
        if (! to_currency (issue.currency, currency.asString ()))
            Throw<std::runtime_error> ("invalid currency");

        if (! issuer.isString ()
                || !to_issuer (issue.account, issuer.asString ()))
            Throw<std::runtime_error> ("invalid issuer");

        if (isXRP (issue.currency))
            Throw<std::runtime_error> ("invalid issuer");
    }

    if (value.isInt ())
    {
        if (value.asInt () >= 0)
        {
            mantissa = value.asInt ();
        }
        else
        {
            mantissa = -value.asInt ();
            negative = true;
        }
    }
    else if (value.isUInt ())
    {
        mantissa = v.asUInt ();
    }
    else if (value.isString ())
    {
        auto const ret = amountFromString (issue, value.asString ());

        mantissa = ret.mantissa ();
        exponent = ret.exponent ();
        negative = ret.negative ();
    }
    else
    {
        Throw<std::runtime_error> ("invalid amount type");
    }

    return { name, issue, mantissa, exponent, native, negative };
}

bool
amountFromJsonNoThrow (STAmount& result, Json::Value const& jvSource)
{
    try
    {
        result = amountFromJson (sfGeneric, jvSource);
        return true;
    }
    catch (const std::exception& e)
    {
        JLOG (debugLog().warn()) <<
            "amountFromJsonNoThrow: caught: " << e.what ();
    }
    return false;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//算子
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

bool
operator== (STAmount const& lhs, STAmount const& rhs)
{
    return areComparable (lhs, rhs) &&
        lhs.negative() == rhs.negative() &&
        lhs.exponent() == rhs.exponent() &&
        lhs.mantissa() == rhs.mantissa();
}

bool
operator< (STAmount const& lhs, STAmount const& rhs)
{
    if (!areComparable (lhs, rhs))
        Throw<std::runtime_error> ("Can't compare amounts that are't comparable!");

    if (lhs.negative() != rhs.negative())
        return lhs.negative();

    if (lhs.mantissa() == 0)
    {
        if (rhs.negative())
            return false;
        return rhs.mantissa() != 0;
    }

//我们知道lhs是非零的，两边都有相同的符号。自从
//rhs为零（因此不是负），因此lhs必须严格
//大于零。因此，如果rhs为零，则比较结果必须为假。
    if (rhs.mantissa() == 0) return false;

    if (lhs.exponent() > rhs.exponent()) return lhs.negative();
    if (lhs.exponent() < rhs.exponent()) return ! lhs.negative();
    if (lhs.mantissa() > rhs.mantissa()) return lhs.negative();
    if (lhs.mantissa() < rhs.mantissa()) return ! lhs.negative();

    return false;
}

STAmount
operator- (STAmount const& value)
{
    if (value.mantissa() == 0)
        return value;
    return STAmount (value.getFName (),
        value.issue(), value.mantissa(), value.exponent(),
            value.native(), ! value.negative(), STAmount::unchecked{});
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
//
//算术
//
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//当三个值都是64位时计算（A*B）/C
//无精度损失：
static
std::uint64_t
muldiv(
    std::uint64_t multiplier,
    std::uint64_t multiplicand,
    std::uint64_t divisor)
{
    boost::multiprecision::uint128_t ret;

    boost::multiprecision::multiply(ret, multiplier, multiplicand);
    ret /= divisor;

    if (ret > std::numeric_limits<std::uint64_t>::max())
    {
        Throw<std::overflow_error> ("overflow: (" +
            std::to_string (multiplier) + " * " +
            std::to_string (multiplicand) + ") / " +
            std::to_string (divisor));
    }

    return static_cast<uint64_t>(ret);
}

static
std::uint64_t
muldiv_round(
    std::uint64_t multiplier,
    std::uint64_t multiplicand,
    std::uint64_t divisor,
    std::uint64_t rounding)
{
    boost::multiprecision::uint128_t ret;

    boost::multiprecision::multiply(ret, multiplier, multiplicand);
    ret += rounding;
    ret /= divisor;

    if (ret > std::numeric_limits<std::uint64_t>::max())
    {
        Throw<std::overflow_error> ("overflow: ((" +
            std::to_string (multiplier) + " * " +
            std::to_string (multiplicand) + ") + " +
            std::to_string (rounding) + ") / " +
            std::to_string (divisor));
    }

    return static_cast<uint64_t>(ret);
}

STAmount
divide (STAmount const& num, STAmount const& den, Issue const& issue)
{
    if (den == beast::zero)
        Throw<std::runtime_error> ("division by zero");

    if (num == beast::zero)
        return {issue};

    std::uint64_t numVal = num.mantissa();
    std::uint64_t denVal = den.mantissa();
    int numOffset = num.exponent();
    int denOffset = den.exponent();

    if (num.native())
    {
        while (numVal < STAmount::cMinValue)
        {
//需要进入范围
            numVal *= 10;
            --numOffset;
        }
    }

    if (den.native())
    {
        while (denVal < STAmount::cMinValue)
        {
            denVal *= 10;
            --denOffset;
        }
    }

//我们将两个尾数分开（每个尾数在10^15之间
//10 ^ 16）。为了保持精度，我们将
//分子乘以10^17（产品在
//10^32到10^33），然后是除法，所以结果
//在10^16到10^15之间。
    return STAmount (issue,
        muldiv(numVal, tenTo17, denVal) + 5,
        numOffset - denOffset - 17,
        num.negative() != den.negative());
}

STAmount
multiply (STAmount const& v1, STAmount const& v2, Issue const& issue)
{
    if (v1 == beast::zero || v2 == beast::zero)
        return STAmount (issue);

    if (v1.native() && v2.native() && isXRP (issue))
    {
        std::uint64_t const minV = getSNValue (v1) < getSNValue (v2)
                ? getSNValue (v1) : getSNValue (v2);
        std::uint64_t const maxV = getSNValue (v1) < getSNValue (v2)
                ? getSNValue (v2) : getSNValue (v1);

if (minV > 3000000000ull) //sqrt（CMaxNative）
            Throw<std::runtime_error> ("Native value overflow");

if (((maxV >> 32) * minV) > 2095475792ull) //Cmax本机/2^32
            Throw<std::runtime_error> ("Native value overflow");

        return STAmount (v1.getFName (), minV * maxV);
    }

    std::uint64_t value1 = v1.mantissa();
    std::uint64_t value2 = v2.mantissa();
    int offset1 = v1.exponent();
    int offset2 = v2.exponent();

    if (v1.native())
    {
        while (value1 < STAmount::cMinValue)
        {
            value1 *= 10;
            --offset1;
        }
    }

    if (v2.native())
    {
        while (value2 < STAmount::cMinValue)
        {
            value2 *= 10;
            --offset2;
        }
    }

//我们将两个尾数相乘（每个尾数在10^15之间
//和10^16），所以他们的产品在10^30到10^32之间
//范围。将他们的产品除以10^14保持
//精度，将结果缩放到10^16到10^18。
    return STAmount (issue,
        muldiv(value1, value2, tenTo14) + 7,
        offset1 + offset2 + 14,
        v1.negative() != v2.negative());
}

static
void
canonicalizeRound (bool native, std::uint64_t& value, int& offset)
{
    if (native)
    {
        if (offset < 0)
        {
            int loops = 0;

            while (offset < -1)
            {
                value /= 10;
                ++offset;
                ++loops;
            }

value += (loops >= 2) ? 9 : 10; //在最后一个除法之前添加
            value /= 10;
            ++offset;
        }
    }
    else if (value > STAmount::cMaxValue)
    {
        while (value > (10 * STAmount::cMaxValue))
        {
            value /= 10;
            ++offset;
        }

value += 9;     //在最后一个除法之前添加
        value /= 10;
        ++offset;
    }
}

STAmount
mulRound (STAmount const& v1, STAmount const& v2, Issue const& issue,
    bool roundUp)
{
    if (v1 == beast::zero || v2 == beast::zero)
        return {issue};

    bool const xrp = isXRP (issue);

    if (v1.native() && v2.native() && xrp)
    {
        std::uint64_t minV = (getSNValue (v1) < getSNValue (v2)) ?
                getSNValue (v1) : getSNValue (v2);
        std::uint64_t maxV = (getSNValue (v1) < getSNValue (v2)) ?
                getSNValue (v2) : getSNValue (v1);

if (minV > 3000000000ull) //sqrt（CMaxNative）
            Throw<std::runtime_error> ("Native value overflow");

if (((maxV >> 32) * minV) > 2095475792ull) //Cmax本机/2^32
            Throw<std::runtime_error> ("Native value overflow");

        return STAmount (v1.getFName (), minV * maxV);
    }

    std::uint64_t value1 = v1.mantissa(), value2 = v2.mantissa();
    int offset1 = v1.exponent(), offset2 = v2.exponent();

    if (v1.native())
    {
        while (value1 < STAmount::cMinValue)
        {
            value1 *= 10;
            --offset1;
        }
    }

    if (v2.native())
    {
        while (value2 < STAmount::cMinValue)
        {
            value2 *= 10;
            --offset2;
        }
    }

    bool const resultNegative = v1.negative() != v2.negative();

//我们将两个尾数相乘（每个尾数在10^15之间
//和10^16），所以他们的产品在10^30到10^32之间
//范围。将他们的产品除以10^14保持
//精度，将结果缩放到10^16到10^18。
//
//如果我们要围捕，我们要围捕
//从零开始，如果我们取整，则截断
//是隐含的。
    std::uint64_t amount = muldiv_round (
        value1, value2, tenTo14,
        (resultNegative != roundUp) ? tenTo14m1 : 0);

    int offset = offset1 + offset2 + 14;
    if (resultNegative != roundUp)
        canonicalizeRound (xrp, amount, offset);
    STAmount result (issue, amount, offset, resultNegative);

//控制何时启用需要切换日期的错误修复
    if (roundUp && !resultNegative && !result && *stAmountCalcSwitchover)
    {
        if (xrp && *stAmountCalcSwitchover2)
        {
//返回零以上的最小值
            amount = 1;
            offset = 0;
        }
        else
        {
//返回零以上的最小值
            amount = STAmount::cMinValue;
            offset = STAmount::cMinOffset;
        }
        return STAmount(issue, amount, offset, resultNegative);
    }
    return result;
}

STAmount
divRound (STAmount const& num, STAmount const& den,
    Issue const& issue, bool roundUp)
{
    if (den == beast::zero)
        Throw<std::runtime_error> ("division by zero");

    if (num == beast::zero)
        return {issue};

    std::uint64_t numVal = num.mantissa(), denVal = den.mantissa();
    int numOffset = num.exponent(), denOffset = den.exponent();

    if (num.native())
    {
        while (numVal < STAmount::cMinValue)
        {
            numVal *= 10;
            --numOffset;
        }
    }

    if (den.native())
    {
        while (denVal < STAmount::cMinValue)
        {
            denVal *= 10;
            --denOffset;
        }
    }

    bool const resultNegative =
        (num.negative() != den.negative());

//我们将两个尾数分开（每个尾数在10^15之间
//10 ^ 16）。为了保持精度，我们将
//分子乘以10^17（产品在
//10^32到10^33），然后是除法，所以结果
//在10^16到10^15之间。
//
//如果我们要四舍五入或
//如果要舍入，则截断。
    std::uint64_t amount = muldiv_round (
        numVal, tenTo17, denVal,
        (resultNegative != roundUp) ? denVal - 1 : 0);

    int offset = numOffset - denOffset - 17;

    if (resultNegative != roundUp)
        canonicalizeRound (isXRP (issue), amount, offset);

    STAmount result (issue, amount, offset, resultNegative);
//控制何时启用需要切换日期的错误修复
    if (roundUp && !resultNegative && !result && *stAmountCalcSwitchover)
    {
        if (isXRP(issue) && *stAmountCalcSwitchover2)
        {
//返回零以上的最小值
            amount = 1;
            offset = 0;
        }
        else
        {
//返回零以上的最小值
            amount = STAmount::cMinValue;
            offset = STAmount::cMinOffset;
        }
        return STAmount(issue, amount, offset, resultNegative);
    }
    return result;
}

} //涟漪
