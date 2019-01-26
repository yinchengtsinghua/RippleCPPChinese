
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

#ifndef RIPPLE_BASICS_RANDOM_H_INCLUDED
#define RIPPLE_BASICS_RANDOM_H_INCLUDED

#include <ripple/basics/win32_workaround.h>
#include <ripple/beast/xor_shift_engine.h>
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/thread/tss.hpp>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <limits>
#include <type_traits>

namespace ripple {

#ifndef __INTELLISENSE__
static_assert (
    std::is_integral <beast::xor_shift_engine::result_type>::value &&
    std::is_unsigned <beast::xor_shift_engine::result_type>::value,
        "The Ripple default PRNG engine must return an unsigned integral type.");

static_assert (
    std::numeric_limits<beast::xor_shift_engine::result_type>::max() >=
    std::numeric_limits<std::uint64_t>::max(),
        "The Ripple default PRNG engine return must be at least 64 bits wide.");
#endif

namespace detail {


//确定类型是否可以像引擎一样调用
template <class Engine, class Result = typename Engine::result_type>
using is_engine =
    boost::beast::detail::is_invocable<Engine, Result()>;
}

/*返回默认的随机引擎。

    这个引擎是确定的，但是
    默认值将随机设定。它不是密码学上的
    安全，不得用于产生随机性
    将用于密钥、安全cookie、ivs、填充等。

    每个线程都有自己的引擎实例，
    将随机播种。
**/

inline
beast::xor_shift_engine&
default_prng ()
{
    static
    boost::thread_specific_ptr<beast::xor_shift_engine> engine;

    if (!engine.get())
    {
        std::random_device rng;

        std::uint64_t seed = rng();

        for (int i = 0; i < 6; ++i)
        {
            if (seed == 0)
                seed = rng();

            seed ^= (seed << (7 - i)) * rng();
        }

        engine.reset (new beast::xor_shift_engine (seed));
    }

    return *engine;
}

/*返回一个均匀分布的随机整数。

    @param min返回的最小值。如果未指定
               该值默认为0。
    @param max返回的最大值。如果未指定
               该值默认为
               可以表示。

    随机性由指定的引擎生成（或
    默认引擎（如果未指定）。结果
    只有当prng引擎
    密码安全。

    @注意，范围始终是一个封闭的间隔，因此调用
          rand_int（-5，15）可以返回
          关闭间隔[-5，15]；类似地，调用
          rand_int（7）可以返回关闭的
          间隔[0，7]。
**/

/*@ {*/
template <class Engine, class Integral>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine,
    Integral min,
    Integral max)
{
    assert (max > min);

//这不应该有状态，构造它应该
//非常便宜。如果事实并非如此
//它可以手动优化。
    return std::uniform_int_distribution<Integral>(min, max)(engine);
}

template <class Integral>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int (
    Integral min,
    Integral max)
{
    return rand_int (default_prng(), min, max);
}

template <class Engine, class Integral>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine,
    Integral max)
{
    return rand_int (engine, Integral(0), max);
}

template <class Integral>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int (Integral max)
{
    return rand_int (default_prng(), max);
}

template <class Integral, class Engine>
std::enable_if_t<
    std::is_integral<Integral>::value &&
    detail::is_engine<Engine>::value,
Integral>
rand_int (
    Engine& engine)
{
    return rand_int (
        engine,
        std::numeric_limits<Integral>::max());
}

template <class Integral = int>
std::enable_if_t<std::is_integral<Integral>::value, Integral>
rand_int ()
{
    return rand_int (
        default_prng(),
        std::numeric_limits<Integral>::max());
}
/*@ }*/

/*返回随机布尔值*/
/*@ {*/
template <class Engine>
inline
bool
rand_bool (Engine& engine)
{
    return rand_int (engine, 1) == 1;
}

inline
bool
rand_bool ()
{
    return rand_bool (default_prng());
}
/*@ }*/

} //涟漪

#endif //Ripple_基础\随机\包括
