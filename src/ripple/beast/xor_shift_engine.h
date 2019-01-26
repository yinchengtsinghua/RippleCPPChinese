
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
    版权所有2014，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_RANDOM_XOR_SHIFT_ENGINE_H_INCLUDED
#define BEAST_RANDOM_XOR_SHIFT_ENGINE_H_INCLUDED

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace beast {

namespace detail {

template <class = void>
class xor_shift_engine
{
public:
    using result_type = std::uint64_t;

    xor_shift_engine(xor_shift_engine const&) = default;
    xor_shift_engine& operator=(xor_shift_engine const&) = default;

    explicit
    xor_shift_engine (result_type val = 1977u);

    void
    seed (result_type seed);

    result_type
    operator()();

    static
    result_type constexpr
    min()
    {
        return std::numeric_limits<result_type>::min();
    }

    static
    result_type constexpr
    max()
    {
        return std::numeric_limits<result_type>::max();
    }

private:
    result_type s_[2];

    static
    result_type
    murmurhash3 (result_type x);
};

template <class _>
xor_shift_engine<_>::xor_shift_engine (
    result_type val)
{
    seed (val);
}

template <class _>
void
xor_shift_engine<_>::seed (result_type seed)
{
    if (seed == 0)
        throw std::domain_error("invalid seed");
    s_[0] = murmurhash3 (seed);
    s_[1] = murmurhash3 (s_[0]);
}

template <class _>
auto
xor_shift_engine<_>::operator()() ->
    result_type
{
    result_type s1 = s_[0];
    result_type const s0 = s_[1];
    s_[0] = s0;
    s1 ^= s1 << 23;
    return (s_[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0;
}

template <class _>
auto
xor_shift_engine<_>::murmurhash3 (result_type x)
    -> result_type
{
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    return x ^= x >> 33;
}

} //细节

/*XOR换档发电机。

    满足统一随机数发生器的要求。

    简单快速的RNG基于：
    http://xorshift.di.unimi.it/xorshift128 plus.c
    不接受种子==0
**/

using xor_shift_engine = detail::xor_shift_engine<>;

}

#endif
