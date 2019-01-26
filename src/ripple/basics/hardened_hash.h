
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

#ifndef RIPPLE_BASICS_HARDENED_HASH_H_INCLUDED
#define RIPPLE_BASICS_HARDENED_HASH_H_INCLUDED

#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/xxhasher.h>

#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

//当设置为1时，使每个进程的种子
//每个默认构造的强化哈希实例的
//
#ifndef RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED
# ifdef __GLIBCXX__
#  define RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED 1
# else
#  define RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED 0
# endif
#endif

namespace ripple {

namespace detail {

using seed_pair = std::pair<std::uint64_t, std::uint64_t>;

template <bool = true>
seed_pair
make_seed_pair() noexcept
{
    struct state_t
    {
        std::mutex mutex;
        std::random_device rng;
        std::mt19937_64 gen;
        std::uniform_int_distribution <std::uint64_t> dist;

        state_t() : gen(rng()) {}
//state_t（state_t const&）=删除；
//state_t&operator=（state_t const&）=删除；
    };
    static state_t state;
    std::lock_guard <std::mutex> lock (state.mutex);
    return {state.dist(state.gen), state.dist(state.gen)};
}

}

template <class HashAlgorithm, bool ProcessSeeded>
class basic_hardened_hash;

/*
 *每个进程一次种子函数
**/

template <class HashAlgorithm>
class basic_hardened_hash <HashAlgorithm, true>
{
private:
    static
    detail::seed_pair const&
    init_seed_pair()
    {
        static detail::seed_pair const p = detail::make_seed_pair<>();
        return p;
    }

public:
    explicit basic_hardened_hash() = default;

    using result_type = typename HashAlgorithm::result_type;

    template <class T>
    result_type
    operator()(T const& t) const noexcept
    {
        std::uint64_t seed0;
        std::uint64_t seed1;
        std::tie(seed0, seed1) = init_seed_pair();
        HashAlgorithm h(seed0, seed1);
        hash_append(h, t);
        return static_cast<result_type>(h);
    }
};

/*
 *每次构造一次种子函数
**/

template <class HashAlgorithm>
class basic_hardened_hash<HashAlgorithm, false>
{
private:
    detail::seed_pair m_seeds;

public:
    using result_type = typename HashAlgorithm::result_type;

    basic_hardened_hash()
        : m_seeds (detail::make_seed_pair<>())
    {}

    template <class T>
    result_type
    operator()(T const& t) const noexcept
    {
        HashAlgorithm h(m_seeds.first, m_seeds.second);
        hash_append(h, t);
        return static_cast<result_type>(h);
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*一种与标准兼容的哈希适配器，可以抵抗敌对的输入。
    要使其工作，t必须在其自己的命名空间中实现：

    @代码

    template<class hasher>
    无效
    hash_append（hasher&h，t const&t）无异常
    {
        //hash_附加每个应该
        //参与哈希的形成
        使用beast：：hash_append；
        hash_append（h，static_cast<t:：base1 const&>（t））；
        hash_append（h，static_cast<t:：base2 const&>（t））；
        /…
        hash_append（h，t.member1）；
        hash_append（h，t.member2）；
        /…
    }

    @端码

    不要使用任何版本的杂音或城市灰作为散列器。
    模板参数（散列算法）。详情
    参见https://131002.net/siphash/网址
**/

#if RIPPLE_NO_HARDENED_HASH_INSTANCE_SEED
template <class HashAlgorithm = beast::xxhasher>
    using hardened_hash = basic_hardened_hash<HashAlgorithm, true>;
#else
template <class HashAlgorithm = beast::xxhasher>
    using hardened_hash = basic_hardened_hash<HashAlgorithm, false>;
#endif

} //涟漪

#endif
