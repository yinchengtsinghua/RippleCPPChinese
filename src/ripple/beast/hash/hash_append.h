
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//————————————————————————————————————————————————————————————————————————————————————————————————————————————————
/*
    此文件是Beast的一部分：https://github.com/vinniefalco/beast
    版权所有2014，howard hinnant<howard.hinnant@gmail.com>，
        vinnie falco<vinnie.falco@gmail.com

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

#ifndef BEAST_HASH_HASH_APPEND_H_INCLUDED
#define BEAST_HASH_HASH_APPEND_H_INCLUDED

#include <ripple/beast/hash/endian.h>
#include <ripple/beast/hash/meta.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <type_traits>
#include <utility>
#include <vector>
#include <boost/container/flat_set.hpp>

namespace beast {

namespace detail {

template <class T>
/*OnStEXP*/
inline
void
reverse_bytes(T& t)
{
    unsigned char* bytes = static_cast<unsigned char*>(std::memmove(std::addressof(t),
                                                                    std::addressof(t),
                                                                    sizeof(T)));
    for (unsigned i = 0; i < sizeof(T)/2; ++i)
        std::swap(bytes[i], bytes[sizeof(T)-1-i]);
}

template <class T>
/*OnStEXP*/
inline
void
maybe_reverse_bytes(T& t, std::false_type)
{
}

template <class T>
/*OnStEXP*/
inline
void
maybe_reverse_bytes(T& t, std::true_type)
{
    reverse_bytes(t);
}

template <class T, class Hasher>
/*OnStEXP*/
inline
void
maybe_reverse_bytes(T& t, Hasher&)
{
    maybe_reverse_bytes(t, std::integral_constant<bool,
        Hasher::endian != endian::native>{});
}

} //细节

//唯一代表的是

//如果两个值的所有组合
//一个类型，比如x和y，如果x==y，那么它也必须是真的。
//memcmp（addressof（x），addressof（y），sizeof（t））=0.即，如果x=＝y，
//然后x和y有相同的位模式表示。

template <class T>
struct is_uniquely_represented
    : public std::integral_constant<bool, std::is_integral<T>::value ||
                                          std::is_enum<T>::value     ||
                                          std::is_pointer<T>::value>
{
    explicit is_uniquely_represented() = default;
};

template <class T>
struct is_uniquely_represented<T const>
    : public is_uniquely_represented<T>
{
    explicit is_uniquely_represented() = default;
};

template <class T>
struct is_uniquely_represented<T volatile>
    : public is_uniquely_represented<T>
{
    explicit is_uniquely_represented() = default;
};

template <class T>
struct is_uniquely_represented<T const volatile>
    : public is_uniquely_represented<T>
{
    explicit is_uniquely_represented() = default;
};

//是否唯一地表示了“<std:：pair<t，u>>”

template <class T, class U>
struct is_uniquely_represented<std::pair<T, U>>
    : public std::integral_constant<bool, is_uniquely_represented<T>::value &&
                                          is_uniquely_represented<U>::value &&
                                          sizeof(T) + sizeof(U) == sizeof(std::pair<T, U>)>
{
    explicit is_uniquely_represented() = default;
};

//是否唯一地表示了“tuple”<std:：tuple<t…>>

template <class ...T>
struct is_uniquely_represented<std::tuple<T...>>
    : public std::integral_constant<bool,
            static_and<is_uniquely_represented<T>::value...>::value &&
            static_sum<sizeof(T)...>::value == sizeof(std::tuple<T...>)>
{
    explicit is_uniquely_represented() = default;
};

//是否唯一地表示了

template <class T, std::size_t N>
struct is_uniquely_represented<T[N]>
    : public is_uniquely_represented<T>
{
    explicit is_uniquely_represented() = default;
};

//是否唯一地表示了<std:：array<t，n>>

template <class T, std::size_t N>
struct is_uniquely_represented<std::array<T, N>>
    : public std::integral_constant<bool, is_uniquely_represented<T>::value &&
                                          sizeof(T)*N == sizeof(std::array<T, N>)>
{
    explicit is_uniquely_represented() = default;
};

/*如果类型可以在一次调用中散列，则返回“true”的元函数。

    for`is_continuous_hashable<t>：：value`为true，然后for every
    'x'和'y'中保留的't'可能值的组合，
    如果'x==y`，那么'memcmp（&x，&y，sizeof（t））`
    返回0；即“x”和“y”由相同的位模式表示。

    例如：two的补码'int'应该是连续的哈希值。
    每个位模式产生一个不等于
    任何其他位模式的值。IEEE浮点不应
    连续哈希，因为-0。0。有不同的位模式，
    尽管他们比较平等。
**/

/*@ {*/
template <class T, class HashAlgorithm>
struct is_contiguously_hashable
    : public std::integral_constant<bool, is_uniquely_represented<T>::value &&
                                      (sizeof(T) == 1 ||
                                       HashAlgorithm::endian == endian::native)>
{
    explicit is_contiguously_hashable() = default;
};

template <class T, std::size_t N, class HashAlgorithm>
struct is_contiguously_hashable<T[N], HashAlgorithm>
    : public std::integral_constant<bool, is_uniquely_represented<T[N]>::value &&
                                      (sizeof(T) == 1 ||
                                       HashAlgorithm::endian == endian::native)>
{
    explicit is_contiguously_hashable() = default;
};
/*@ }*/

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*逻辑地将输入数据连接到“hasher”。

    哈希要求：

        'x'是'hasher'类型
        'h'是'x'类型的值
        “p”是一个可转换为“void const”的值
        'n'是'std:：size_t'类型的值，大于零

        表达式：
            `H.APPEND（P，N）；`
        投掷：
            从未
        效果：
            将输入数据添加到哈希状态。

        表达式：
            `static_cast<std:：size_t>（j）`
        投掷：
            从未
        效果：
            返回所有输入数据的重新筛选哈希。
**/

/*@ {*/

//标量

template <class Hasher, class T>
inline
std::enable_if_t
<
    is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, T const& t) noexcept
{
    h(std::addressof(t), sizeof(t));
}

template <class Hasher, class T>
inline
std::enable_if_t
<
    !is_contiguously_hashable<T, Hasher>::value &&
    (std::is_integral<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value)
>
hash_append(Hasher& h, T t) noexcept
{
    detail::reverse_bytes(t);
    h(std::addressof(t), sizeof(t));
}

template <class Hasher, class T>
inline
std::enable_if_t
<
    std::is_floating_point<T>::value
>
hash_append(Hasher& h, T t) noexcept
{
    if (t == 0)
        t = 0;
    detail::maybe_reverse_bytes(t, h);
    h(&t, sizeof(t));
}

template <class Hasher>
inline
void
hash_append(Hasher& h, std::nullptr_t) noexcept
{
    void const* p = nullptr;
    detail::maybe_reverse_bytes(p, h);
    h(&p, sizeof(p));
}

//ADL用途的转发声明

template <class Hasher, class T, std::size_t N>
std::enable_if_t
<
    !is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, T (&a)[N]) noexcept;

template <class Hasher, class CharT, class Traits, class Alloc>
std::enable_if_t
<
    !is_contiguously_hashable<CharT, Hasher>::value
>
hash_append(Hasher& h, std::basic_string<CharT, Traits, Alloc> const& s) noexcept;

template <class Hasher, class CharT, class Traits, class Alloc>
std::enable_if_t
<
    is_contiguously_hashable<CharT, Hasher>::value
>
hash_append(Hasher& h, std::basic_string<CharT, Traits, Alloc> const& s) noexcept;

template <class Hasher, class T, class U>
std::enable_if_t
<
    !is_contiguously_hashable<std::pair<T, U>, Hasher>::value
>
hash_append (Hasher& h, std::pair<T, U> const& p) noexcept;

template <class Hasher, class T, class Alloc>
std::enable_if_t
<
    !is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, std::vector<T, Alloc> const& v) noexcept;

template <class Hasher, class T, class Alloc>
std::enable_if_t
<
    is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, std::vector<T, Alloc> const& v) noexcept;

template <class Hasher, class T, std::size_t N>
std::enable_if_t
<
    !is_contiguously_hashable<std::array<T, N>, Hasher>::value
>
hash_append(Hasher& h, std::array<T, N> const& a) noexcept;

template <class Hasher, class ...T>
std::enable_if_t
<
    !is_contiguously_hashable<std::tuple<T...>, Hasher>::value
>
hash_append(Hasher& h, std::tuple<T...> const& t) noexcept;

template <class Hasher, class Key, class T, class Hash, class Pred, class Alloc>
void
hash_append(Hasher& h, std::unordered_map<Key, T, Hash, Pred, Alloc> const& m);

template <class Hasher, class Key, class Hash, class Pred, class Alloc>
void
hash_append(Hasher& h, std::unordered_set<Key, Hash, Pred, Alloc> const& s);

template <class Hasher, class Key, class Compare, class Alloc>
std::enable_if_t
<
    !is_contiguously_hashable<Key, Hasher>::value
>
hash_append(Hasher& h, boost::container::flat_set<Key, Compare, Alloc> const& v) noexcept;
template <class Hasher, class Key, class Compare, class Alloc>
std::enable_if_t
<
    is_contiguously_hashable<Key, Hasher>::value
>
hash_append(Hasher& h, boost::container::flat_set<Key, Compare, Alloc> const& v) noexcept;
template <class Hasher, class T0, class T1, class ...T>
void
hash_append (Hasher& h, T0 const& t0, T1 const& t1, T const& ...t) noexcept;

//C阵列

template <class Hasher, class T, std::size_t N>
std::enable_if_t
<
    !is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, T (&a)[N]) noexcept
{
    for (auto const& t : a)
        hash_append(h, t);
}

//基本字符串

template <class Hasher, class CharT, class Traits, class Alloc>
inline
std::enable_if_t
<
    !is_contiguously_hashable<CharT, Hasher>::value
>
hash_append(Hasher& h, std::basic_string<CharT, Traits, Alloc> const& s) noexcept
{
    for (auto c : s)
        hash_append(h, c);
    hash_append(h, s.size());
}

template <class Hasher, class CharT, class Traits, class Alloc>
inline
std::enable_if_t
<
    is_contiguously_hashable<CharT, Hasher>::value
>
hash_append(Hasher& h, std::basic_string<CharT, Traits, Alloc> const& s) noexcept
{
    h(s.data(), s.size()*sizeof(CharT));
    hash_append(h, s.size());
}

//一对

template <class Hasher, class T, class U>
inline
std::enable_if_t
<
    !is_contiguously_hashable<std::pair<T, U>, Hasher>::value
>
hash_append (Hasher& h, std::pair<T, U> const& p) noexcept
{
    hash_append (h, p.first, p.second);
}

//矢量

template <class Hasher, class T, class Alloc>
inline
std::enable_if_t
<
    !is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, std::vector<T, Alloc> const& v) noexcept
{
    for (auto const& t : v)
        hash_append(h, t);
    hash_append(h, v.size());
}

template <class Hasher, class T, class Alloc>
inline
std::enable_if_t
<
    is_contiguously_hashable<T, Hasher>::value
>
hash_append(Hasher& h, std::vector<T, Alloc> const& v) noexcept
{
    h(v.data(), v.size()*sizeof(T));
    hash_append(h, v.size());
}

//数组

template <class Hasher, class T, std::size_t N>
std::enable_if_t
<
    !is_contiguously_hashable<std::array<T, N>, Hasher>::value
>
hash_append(Hasher& h, std::array<T, N> const& a) noexcept
{
    for (auto const& t : a)
        hash_append(h, t);
}

template <class Hasher, class Key, class Compare, class Alloc>
std::enable_if_t
<
    !is_contiguously_hashable<Key, Hasher>::value
>
hash_append(Hasher& h, boost::container::flat_set<Key, Compare, Alloc> const& v) noexcept
{
    for (auto const& t : v)
        hash_append(h, t);
}
template <class Hasher, class Key, class Compare, class Alloc>
std::enable_if_t
<
    is_contiguously_hashable<Key, Hasher>::value
>
hash_append(Hasher& h, boost::container::flat_set<Key, Compare, Alloc> const& v) noexcept
{
    h(&(v.begin()), v.size()*sizeof(Key));
}
//元组

namespace detail
{

inline
void
for_each_item(...) noexcept
{
}

template <class Hasher, class T>
inline
int
hash_one(Hasher& h, T const& t) noexcept
{
    hash_append(h, t);
    return 0;
}

template <class Hasher, class ...T, std::size_t ...I>
inline
void
tuple_hash(Hasher& h, std::tuple<T...> const& t, std::index_sequence<I...>) noexcept
{
    for_each_item(hash_one(h, std::get<I>(t))...);
}

}  //细节

template <class Hasher, class ...T>
inline
std::enable_if_t
<
    !is_contiguously_hashable<std::tuple<T...>, Hasher>::value
>
hash_append(Hasher& h, std::tuple<T...> const& t) noexcept
{
    detail::tuple_hash(h, t, std::index_sequence_for<T...>{});
}

//SelddPPTR

template <class Hasher, class T>
inline
void
hash_append (Hasher& h, std::shared_ptr<T> const& p) noexcept
{
    hash_append(h, p.get());
}

//年代

template <class Hasher, class Rep, class Period>
inline
void
hash_append (Hasher& h, std::chrono::duration<Rep, Period> const& d) noexcept
{
    hash_append(h, d.count());
}

template <class Hasher, class Clock, class Duration>
inline
void
hash_append (Hasher& h, std::chrono::time_point<Clock, Duration> const& tp) noexcept
{
    hash_append(h, tp.time_since_epoch());
}

//多变的

template <class Hasher, class T0, class T1, class ...T>
inline
void
hash_append (Hasher& h, T0 const& t0, T1 const& t1, T const& ...t) noexcept
{
    hash_append(h, t0);
    hash_append(h, t1, t...);
}

//错误代码

template <class HashAlgorithm>
inline
void
hash_append(HashAlgorithm& h, std::error_code const& ec)
{
    hash_append(h, ec.value(), &ec.category());
}

} //野兽

#endif
