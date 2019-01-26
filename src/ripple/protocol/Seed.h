
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

#ifndef RIPPLE_PROTOCOL_SEED_H_INCLUDED
#define RIPPLE_PROTOCOL_SEED_H_INCLUDED

#include <ripple/basics/base_uint.h>
#include <ripple/basics/Slice.h>
#include <ripple/protocol/tokens.h>
#include <boost/optional.hpp>
#include <array>

namespace ripple {

/*种子用于生成确定性密钥。*/
class Seed
{
private:
    std::array<uint8_t, 16> buf_;

public:
    using const_iterator = std::array<uint8_t, 16>::const_iterator;

    Seed() = delete;

    Seed (Seed const&) = default;
    Seed& operator= (Seed const&) = default;

    /*摧毁种子。
        缓冲区将首先被安全擦除。
    **/

    ~Seed();

    /*构建一个种子*/
    /*@ {*/
    explicit Seed (Slice const& slice);
    explicit Seed (uint128 const& seed);
    /*@ }*/

    std::uint8_t const*
    data() const
    {
        return buf_.data();
    }

    std::size_t
    size() const
    {
        return buf_.size();
    }

    const_iterator
    begin() const noexcept
    {
        return buf_.begin();
    }

    const_iterator
    cbegin() const noexcept
    {
        return buf_.cbegin();
    }

    const_iterator
    end() const noexcept
    {
        return buf_.end();
    }

    const_iterator
    cend() const noexcept
    {
        return buf_.cend();
    }
};

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

/*使用安全随机数创建种子。*/
Seed
randomSeed();

/*确定地生成种子。

    该算法特定于Ripple：

        种子计算为前128位
        sha512字符串文本的一半，不包括
        任何终止的空值。

    @注意，这不会试图确定
          字符串（例如十六进制或base58）。
**/

Seed
generateSeed (std::string const& passPhrase);

/*将base58编码的字符串解析为种子*/
template <>
boost::optional<Seed>
parseBase58 (std::string const& s);

/*尝试将字符串作为种子进行分析*/
boost::optional<Seed>
parseGenericSeed (std::string const& str);

/*以rfc1751格式编码种子*/
std::string
seedAs1751 (Seed const& seed);

/*将种子格式化为base58字符串*/
inline
std::string
toBase58 (Seed const& seed)
{
    return base58EncodeToken(TokenType::FamilySeed, seed.data(), seed.size());
}

}

#endif
