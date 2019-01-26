
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
    版权所有（c）2018 Ripple Labs Inc.

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

#ifndef RIPPLE_BASICS_SAFE_CAST_H_INCLUDED
#define RIPPLE_BASICS_SAFE_CAST_H_INCLUDED

#include <type_traits>

namespace ripple {

//安全强制转换向静态强制转换添加编译时检查，以确保
//目标可以保存源的所有值。这是特别的
//当源或目标是枚举类型时很方便。

template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_integral<Dest>::value && std::is_integral<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    static_assert(std::is_signed<Dest>::value || std::is_unsigned<Src>::value,
        "Cannot cast signed to unsigned");
    constexpr unsigned not_same = std::is_signed<Dest>::value !=
                                  std::is_signed<Src>::value;
    static_assert(sizeof(Dest) >= sizeof(Src) + not_same,
        "Destination is too small to hold all values of source");
    return static_cast<Dest>(s);
}

template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_enum<Dest>::value && std::is_integral<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    return static_cast<Dest>(safe_cast<std::underlying_type_t<Dest>>(s));
}

template <class Dest, class Src>
inline
constexpr
std::enable_if_t
<
    std::is_integral<Dest>::value && std::is_enum<Src>::value,
    Dest
>
safe_cast(Src s) noexcept
{
    return safe_cast<Dest>(static_cast<std::underlying_type_t<Src>>(s));
}

} //涟漪

#endif
