
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

//版权所有（c）2009-2010 Satoshi Nakamoto
//版权所有（c）2011比特币开发商
//在MIT/X11软件许可证下分发，请参见随附的
//文件license.txt或http://www.opensource.org/licenses/mit-license.php。

#ifndef RIPPLE_BASICS_STRHEX_H_INCLUDED
#define RIPPLE_BASICS_STRHEX_H_INCLUDED

#include <cassert>
#include <string>

#include <boost/algorithm/hex.hpp>
#include <boost/endian/conversion.hpp>

namespace ripple {

/*将整数转换为相应的十六进制数字
    @param idigit 0-15（含0-15）
    @返回“0”-“9”或“a”-“f”中的字符。
**/

inline
char
charHex (unsigned int digit)
{
    static
    char const xtab[] = "0123456789ABCDEF";

    assert (digit < 16);

    return xtab[digit];
}

/*@ {*/
/*将十六进制数字转换为相应的整数
    @param cdigit“0”-“9”、“a”-“f”或“a”-“f”之一
    @成功时返回0到15的整数；失败时返回-1。
**/

int
charUnHex (unsigned char c);

inline
int
charUnHex (char c)
{
    return charUnHex (static_cast<unsigned char>(c));
}
/*@ }*/

template <class FwdIt>
std::string
strHex(FwdIt begin, FwdIt end)
{
    static_assert(
        std::is_convertible<
            typename std::iterator_traits<FwdIt>::iterator_category,
            std::forward_iterator_tag>::value,
        "FwdIt must be a forward iterator");
    std::string result;
    result.reserve(2 * std::distance(begin, end));
    boost::algorithm::hex(begin, end, std::back_inserter(result));
    return result;
}

template <class T, class = decltype(std::declval<T>().begin())>
std::string strHex(T const& from)
{
    return strHex(from.begin(), from.end());
}

inline std::string strHex (const std::uint64_t uiHost)
{
    uint64_t uBig    = boost::endian::native_to_big (uiHost);

    auto const begin = (unsigned char*) &uBig;
    auto const end   = begin + sizeof(uBig);
    return strHex(begin, end);
}
}

#endif
