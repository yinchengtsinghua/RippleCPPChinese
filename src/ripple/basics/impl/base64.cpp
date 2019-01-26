
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

//
//版权所有（c）2013-2017 Vinnie Falco（Gmail.com上的Vinnie Dot Falco）
//
//在Boost软件许可证1.0版下分发。（见附件
//文件许可证_1_0.txt或复制到http://www.boost.org/license_1_0.txt）
//

/*
   部分来自http://www.adp-gmbH.ch/cpp/common/base64.html
   版权声明：

   base64.cpp和base64.h

   版权所有（c）2004-2008 Ren_nyffenegger

   此源代码按原样提供，没有任何明示或暗示
   担保。在任何情况下，作者都不承担任何损害赔偿责任。
   因使用本软件而产生。

   允许任何人将本软件用于任何目的，
   包括商业应用程序，并对其进行更改和重新分发
   自由，受以下限制：

   1。不得歪曲此源代码的来源；不得
      声明您编写了原始源代码。如果使用此源代码
      在产品中，产品文档中的确认将是
      感谢但不是必需的。

   2。更改的源版本必须清楚地标记为这样，并且不能
      误传为原始源代码。

   三。不得从任何源分发中删除或更改此通知。

   Ren_nyffeegger rene.nyffeegger@adp-gmbH.ch

**/


#include <ripple/basics/base64.h>

#include <cctype>
#include <utility>

namespace ripple {

namespace base64 {

inline
char const*
get_alphabet()
{
    static char constexpr tab[] = {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    };
    return &tab[0];
}

inline
signed char const*
get_inverse()
{
    static signed char constexpr tab[] = {
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //0－15
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //16-31
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, //32-47
52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, //43-63
-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, //64-79
15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, //80-95
-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, //96－111
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, //112～127
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //128～143
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //144-159
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //160—175
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //176—191
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //192-207
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //208—223
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //224～23
-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1  //240－255
    };
    return &tab[0];
}


///返回对base64字符串进行编码所需的最大字符数。
inline
std::size_t constexpr
encoded_size(std::size_t n)
{
    return 4 * ((n + 2) / 3);
}

///返回解码base64字符串所需的最大字节数。
inline
std::size_t constexpr
decoded_size(std::size_t n)
{
    return ((n / 4) * 3) + 2;
}

/*将一系列八位字节编码为加垫的base64字符串。

    结果字符串不会以空结尾。

    按标准要求

    “out”指向的内存指向有效内存
    至少包含'encoded_size（len）'个字节。

    @返回写入“out”的字符数。这个
    将排除任何无效终止。
**/

std::size_t
encode(void* dest, void const* src, std::size_t len)
{
    char*      out = static_cast<char*>(dest);
    char const* in = static_cast<char const*>(src);
    auto const tab = base64::get_alphabet();

    for(auto n = len / 3; n--;)
    {
        *out++ = tab[ (in[0] & 0xfc) >> 2];
        *out++ = tab[((in[0] & 0x03) << 4) + ((in[1] & 0xf0) >> 4)];
        *out++ = tab[((in[2] & 0xc0) >> 6) + ((in[1] & 0x0f) << 2)];
        *out++ = tab[  in[2] & 0x3f];
        in += 3;
    }

    switch(len % 3)
    {
    case 2:
        *out++ = tab[ (in[0] & 0xfc) >> 2];
        *out++ = tab[((in[0] & 0x03) << 4) + ((in[1] & 0xf0) >> 4)];
        *out++ = tab[                         (in[1] & 0x0f) << 2];
        *out++ = '=';
        break;

    case 1:
        *out++ = tab[ (in[0] & 0xfc) >> 2];
        *out++ = tab[((in[0] & 0x03) << 4)];
        *out++ = '=';
        *out++ = '=';
        break;

    case 0:
        break;
    }

    return out - static_cast<char*>(dest);
}

/*将填充的base64字符串解码为一系列八位字节。

    按标准要求

    “out”指向的内存指向有效内存
    至少有'decoded\u size（len）'个字节。

    @返回写入“out”的八位字节数，以及
    从输入字符串中读取的字符数，
    表示为一对。
**/

std::pair<std::size_t, std::size_t>
decode(void* dest, char const* src, std::size_t len)
{
    char* out = static_cast<char*>(dest);
    auto in = reinterpret_cast<unsigned char const*>(src);
    unsigned char c3[3], c4[4];
    int i = 0;
    int j = 0;

    auto const inverse = base64::get_inverse();

    while(len-- && *in != '=')
    {
        auto const v = inverse[*in];
        if(v == -1)
            break;
        ++in;
        c4[i] = v;
        if(++i == 4)
        {
            c3[0] =  (c4[0]        << 2) + ((c4[1] & 0x30) >> 4);
            c3[1] = ((c4[1] & 0xf) << 4) + ((c4[2] & 0x3c) >> 2);
            c3[2] = ((c4[2] & 0x3) << 6) +   c4[3];

            for(i = 0; i < 3; i++)
                *out++ = c3[i];
            i = 0;
        }
    }

    if(i)
    {
        c3[0] = ( c4[0]        << 2) + ((c4[1] & 0x30) >> 4);
        c3[1] = ((c4[1] & 0xf) << 4) + ((c4[2] & 0x3c) >> 2);
        c3[2] = ((c4[2] & 0x3) << 6) +   c4[3];

        for(j = 0; j < i - 1; j++)
            *out++ = c3[j];
    }

    return {out - static_cast<char*>(dest),
        in - reinterpret_cast<unsigned char const*>(src)};
}

} //Base64

std::string
base64_encode (std::uint8_t const* data,
    std::size_t len)
{
    std::string dest;
    dest.resize(base64::encoded_size(len));
    dest.resize(base64::encode(&dest[0], data, len));
    return dest;
}

std::string
base64_decode(std::string const& data)
{
    std::string dest;
    dest.resize(base64::decoded_size(data.size()));
    auto const result = base64::decode(
        &dest[0], data.data(), data.size());
    dest.resize(result.first);
    return dest;
}

} //涟漪
