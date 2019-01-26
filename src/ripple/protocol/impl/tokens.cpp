
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

#include <ripple/basics/safe_cast.h>
#include <ripple/protocol/tokens.h>
#include <ripple/protocol/digest.h>
#include <boost/container/small_vector.hpp>
#include <cassert>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ripple {

static char rippleAlphabet[] =
    "rpshnaf39wBUDNEGHJKLM4PQRST7VWXYZ2bcdeCg65jkm8oFqi1tuvAxyz";

static char bitcoinAlphabet[] =
    "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

template <class Hasher>
static
typename Hasher::result_type
digest (void const* data, std::size_t size) noexcept
{
    Hasher h;
    h(data, size);
    return static_cast<
        typename Hasher::result_type>(h);
}

template <class Hasher, class T, std::size_t N,
    class = std::enable_if_t<sizeof(T) == 1>>
static
typename Hasher::result_type
digest (std::array<T, N> const& v)
{
    return digest<Hasher>(v.data(), v.size());
}

//计算双摘要（例如摘要的摘要）
template <class Hasher, class... Args>
static
typename Hasher::result_type
digest2 (Args const&... args)
{
    return digest<Hasher>(
        digest<Hasher>(args...));
}

/*计算数据的4字节校验和

    校验和按前4个字节计算。
    消息的sha256摘要。这是增加的
    以58为基数对要检测的标识符进行编码
    数据输入中的用户错误。

    @注意，这个校验和算法是客户端API的一部分。
**/

void
checksum (void* out,
    void const* message,
        std::size_t size)
{
    auto const h =
        digest2<sha256_hasher>(message, size);
    std::memcpy(out, h.data(), 4);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//比特币代码：https://github.com/bitcoin/bitcoin
//版权所有（c）2014比特币核心开发者
//根据MIT软件许可证分发，请参见随附的
//文件复制或http://www.opensource.org/licenses/mit-license.php。
//
//从原始版本修改
//
//警告不要直接调用此函数，请使用
//编码base58令牌，因为它
//计算所需缓冲区的大小。
static
std::string
encodeBase58(
    void const* message, std::size_t size,
        void *temp, std::size_t temp_size,
            char const* const alphabet)
{
    auto pbegin = reinterpret_cast<unsigned char const*>(message);
    auto const pend = pbegin + size;

//跳过并计数前导零。
    int zeroes = 0;
    while (pbegin != pend && *pbegin == 0)
    {
        pbegin++;
        zeroes++;
    }

    auto const b58begin = reinterpret_cast<unsigned char*>(temp);
    auto const b58end = b58begin + temp_size;

    std::fill(b58begin, b58end, 0);

    while (pbegin != pend)
    {
        int carry = *pbegin;
//应用“b58=b58*256+ch”。
        for (auto iter = b58end; iter != b58begin; --iter)
        {
            carry += 256 * (iter[-1]);
            iter[-1] = carry % 58;
            carry /= 58;
        }
        assert(carry == 0);
        pbegin++;
    }

//跳过base58结果中的前导零。
    auto iter = b58begin;
    while (iter != b58end && *iter == 0)
        ++iter;

//将结果转换为字符串。
    std::string str;
    str.reserve(zeroes + (b58end - iter));
    str.assign(zeroes, alphabet[0]);
    while (iter != b58end)
        str += alphabet[*(iter++)];
    return str;
}

static
std::string
encodeToken (TokenType type,
    void const* token, std::size_t size, char const* const alphabet)
{
//扩展的令牌包括类型+4字节校验和
    auto const expanded = 1 + size + 4;

//我们需要扩展+扩展*（log（256）/log（58）），即
//以扩展+扩展*（138/100+1）为界，有效
//向外扩展*3：
    auto const bufsize = expanded * 3;

    boost::container::small_vector<std::uint8_t, 1024> buf (bufsize);

//数据布局为
//<type><token><checksum>
    buf[0] = safe_cast<std::underlying_type_t <TokenType>>(type);
    if (size)
        std::memcpy(buf.data() + 1, token, size);
    checksum(buf.data() + 1 + size, buf.data(), 1 + size);

    return encodeBase58(
        buf.data(), expanded,
        buf.data() + expanded, bufsize - expanded,
        alphabet);
}

std::string
base58EncodeToken (TokenType type,
    void const* token, std::size_t size)
{
    return encodeToken(type, token, size, rippleAlphabet);
}

std::string
base58EncodeTokenBitcoin (TokenType type,
    void const* token, std::size_t size)
{
    return encodeToken(type, token, size, bitcoinAlphabet);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//比特币代码：https://github.com/bitcoin/bitcoin
//版权所有（c）2014比特币核心开发者
//根据MIT软件许可证分发，请参见随附的
//文件复制或http://www.opensource.org/licenses/mit-license.php。
//
//从原始版本修改
template <class InverseArray>
static
std::string
decodeBase58 (std::string const& s,
    InverseArray const& inv)
{
    auto psz = s.c_str();
    auto remain = s.size();
//跳过并计数前导零
    int zeroes = 0;
    while (remain > 0 && inv[*psz] == 0)
    {
        ++zeroes;
        ++psz;
        --remain;
    }
//在big-endian base256表示中分配足够的空间。
//对数（58）/对数（256），向上取整。
    std::vector<unsigned char> b256(
        remain * 733 / 1000 + 1);
    while (remain > 0)
    {
        auto carry = inv[*psz];
        if (carry == -1)
            return {};
//应用“B256=B256*58+进位”。
        for (auto iter = b256.rbegin();
            iter != b256.rend(); ++iter)
        {
            carry += 58 * *iter;
            *iter = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        ++psz;
        --remain;
    }
//跳过B256中的前导零。
    auto iter = std::find_if(
        b256.begin(), b256.end(),[](unsigned char c)
            { return c != 0; });
    std::string result;
    result.reserve (zeroes + (b256.end() - iter));
    result.assign (zeroes, 0x00);
    while (iter != b256.end())
        result.push_back(*(iter++));
    return result;
}

/*base58解码纹波令牌

    检查类型和校验和
    从返回的结果中移除。
**/

template <class InverseArray>
static
std::string
decodeBase58Token (std::string const& s,
    TokenType type, InverseArray const& inv)
{
    std::string const ret = decodeBase58(s, inv);

//拒绝零长度标记
    if (ret.size() < 6)
        return {};

//类型必须匹配。
    if (type != safe_cast<TokenType>(static_cast<std::uint8_t>(ret[0])))
        return {};

//校验和也必须。
    std::array<char, 4> guard;
    checksum(guard.data(), ret.data(), ret.size() - guard.size());
    if (!std::equal (guard.rbegin(), guard.rend(), ret.rbegin()))
        return {};

//跳过前导类型字节和尾随校验和。
    return ret.substr(1, ret.size() - 1 - guard.size());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//将字符映射到其基数58位
class InverseAlphabet
{
private:
    std::array<int, 256> map_;

public:
    explicit
    InverseAlphabet(std::string const& digits)
    {
        map_.fill(-1);
        int i = 0;
        for(auto const c : digits)
            map_[static_cast<
                unsigned char>(c)] = i++;
    }

    int
    operator[](char c) const
    {
        return map_[static_cast<
            unsigned char>(c)];
    }
};

static InverseAlphabet rippleInverse(rippleAlphabet);

static InverseAlphabet bitcoinInverse(bitcoinAlphabet);

std::string
decodeBase58Token(
    std::string const& s, TokenType type)
{
    return decodeBase58Token(s, type, rippleInverse);
}

std::string
decodeBase58TokenBitcoin(
    std::string const& s, TokenType type)
{
    return decodeBase58Token(s, type, bitcoinInverse);
}

} //涟漪
