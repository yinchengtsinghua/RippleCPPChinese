
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
    版权所有（c）2014 Ripple Labs Inc.

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

#include <ripple/protocol/Serializer.h>
#include <ripple/protocol/SystemParameters.h>
#include <ripple/protocol/UintTypes.h>
#include <ripple/beast/utility/Zero.h>

namespace ripple {

std::string to_string(Currency const& currency)
{
//我们愿意在ASCII表示中允许的字符
//三个字母的货币代码。
    static std::string const allowed_characters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "<>(){}[]|?!@#$%^&*";

    if (currency == beast::zero)
        return systemCurrencyCode();

    if (currency == noCurrency())
        return "1";

    static Currency const sIsoBits (
        from_hex_text<Currency>("FFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFF"));

    if ((currency & sIsoBits).isZero ())
    {
//货币描述符中3个字符的ISO代码的偏移量
        int const isoOffset = 12;

        std::string const iso(
            currency.data () + isoOffset,
            currency.data () + isoOffset + 3);

//使用ISO样式表示指定系统货币代码
//是不允许的。
        if ((iso != systemCurrencyCode()) &&
            (iso.find_first_not_of (allowed_characters) == std::string::npos))
        {
            return iso;
        }
    }

    return strHex (currency);
}

bool to_currency(Currency& currency, std::string const& code)
{
    if (code.empty () || !code.compare (systemCurrencyCode()))
    {
        currency = beast::zero;
        return true;
    }

    static const int CURRENCY_CODE_LENGTH = 3;
    if (code.size () == CURRENCY_CODE_LENGTH)
    {
        Blob codeBlob (CURRENCY_CODE_LENGTH);

        std::transform (code.begin (), code.end (), codeBlob.begin (),
                        [](auto c)
                        {
                            return ::toupper(static_cast<unsigned char>(c));
                        });

        Serializer  s;

        s.addZeros (96 / 8);
        s.addRaw (codeBlob);
        s.addZeros (16 / 8);
        s.addZeros (24 / 8);

        s.get160 (currency, 0);
        return true;
    }

    if (40 == code.size ())
        return currency.SetHex (code);

    return false;
}

Currency to_currency(std::string const& code)
{
    Currency currency;
    if (!to_currency(currency, code))
        currency = noCurrency();
    return currency;
}

Currency const& xrpCurrency()
{
    static Currency const currency(0);
    return currency;
}

Currency const& noCurrency()
{
    static Currency const currency(1);
    return currency;
}

Currency const& badCurrency()
{
    static Currency const currency(0x5852500000000000);
    return currency;
}

} //涟漪
