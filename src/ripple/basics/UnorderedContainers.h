
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

#ifndef RIPPLE_BASICS_UNORDEREDCONTAINERS_H_INCLUDED
#define RIPPLE_BASICS_UNORDEREDCONTAINERS_H_INCLUDED

#include <ripple/basics/hardened_hash.h>
#include <ripple/beast/hash/hash_append.h>
#include <ripple/beast/hash/uhash.h>
#include <ripple/beast/hash/xxhasher.h>
#include <unordered_map>
#include <unordered_set>

/*
*对不需要加密安全的密钥使用哈希容器
*哈希算法。
*
*对于需要安全哈希算法的密钥，请使用硬化的哈希容器。
*
*将哈希函数用作
*模板参数完全依赖于该哈希函数，而完全不依赖于
*它是什么容器。
**/


namespace ripple
{

//散列容器

template <class Key, class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hash_map = std::unordered_map <Key, Value, Hash, Pred, Allocator>;

template <class Key, class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hash_multimap = std::unordered_multimap <Key, Value, Hash, Pred, Allocator>;

template <class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hash_set = std::unordered_set <Value, Hash, Pred, Allocator>;

template <class Value, class Hash = beast::uhash<>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hash_multiset = std::unordered_multiset <Value, Hash, Pred, Allocator>;

//硬化的散列容器

using strong_hash = beast::xxhasher;

template <class Key, class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hardened_hash_map = std::unordered_map <Key, Value, Hash, Pred, Allocator>;

template <class Key, class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Key>,
          class Allocator = std::allocator<std::pair<Key const, Value>>>
using hardened_hash_multimap = std::unordered_multimap <Key, Value, Hash, Pred, Allocator>;

template <class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hardened_hash_set = std::unordered_set <Value, Hash, Pred, Allocator>;

template <class Value, class Hash = hardened_hash<strong_hash>,
          class Pred = std::equal_to<Value>,
          class Allocator = std::allocator<Value>>
using hardened_hash_multiset = std::unordered_multiset <Value, Hash, Pred, Allocator>;

} //涟漪

#endif
