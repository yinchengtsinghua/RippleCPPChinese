
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
    版权所有2013，vinnie falco<vinnie.falco@gmail.com>

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

#ifndef BEAST_CRYPTO_SHA512_CONTEXT_H_INCLUDED
#define BEAST_CRYPTO_SHA512_CONTEXT_H_INCLUDED

#include <cstdint>
#include <cstring>

namespace beast {
namespace detail {

//基于https://github.com/ogay/sha2
//此实现已从
//原创。它已经被更新为C++ 11。

/*
 *更新到C++，ZeWoRo.com 2012
 *基于Olivier Gay的版本
 *请参见下面修改的BSD许可证：
 *
 *FIPS 180-2 SHA-224/256/384/512实施
 *发行日期：2005年4月30日
 *网址：http://www.ouah.org/ogay/sha2/
 *
 *版权所有（c）2005，2007 Olivier Gay<Olivier.gay@a3.epfl.ch>
 *保留所有权利。
 *
 *以源和二进制形式重新分配和使用，有或无
 *允许修改，前提是满足以下条件
 *满足：
 * 1。源代码的再分配必须保留上述版权
 *注意，此条件列表和以下免责声明。
 * 2。二进制形式的再分配必须复制上述版权
 *注意，此条件列表和以下免责声明
 *分发时提供的文件和/或其他材料。
 * 3。既不是项目的名称，也不是项目贡献者的名称
 *可用于支持或推广从本软件衍生的产品。
 *未经事先明确书面许可。
 *
 *本软件由项目和贡献者“原样”提供，并且
 *任何明示或暗示的保证，包括但不限于
 *对适销性和特定用途适用性的默示保证
 *不予承认。在任何情况下，项目或出资人都不承担任何责任。
 *任何直接、间接、附带、特殊、示范性或结果性的
 *损害赔偿（包括但不限于采购替代货物）
 *或服务；使用、数据或利润损失；或业务中断）
 *无论是何种原因，根据任何责任理论，无论是在合同中，严格
 *以任何方式产生的责任或侵权（包括疏忽或其他）
 *不使用本软件，即使已告知
 *这种损害。
 **/


struct sha256_context
{
    explicit sha256_context() = default;

    static unsigned int const block_size = 64;
    static unsigned int const digest_size = 32;

    unsigned int tot_len;
    unsigned int len;
    unsigned char block[2 * block_size];
    std::uint32_t h[8];
};

struct sha512_context
{
    explicit sha512_context() = default;

    static unsigned int const block_size = 128;
    static unsigned int const digest_size = 64;

    unsigned int tot_len;
    unsigned int len;
    unsigned char block[2 * block_size];
    std::uint64_t h[8];
};

#define BEAST_SHA2_SHFR(x, n)    (x >> n)
#define BEAST_SHA2_ROTR(x, n)   ((x >> n) | (x << ((sizeof(x) << 3) - n)))
#define BEAST_SHA2_ROTL(x, n)   ((x << n) | (x >> ((sizeof(x) << 3) - n)))
#define BEAST_SHA2_CH(x, y, z)  ((x & y) ^ (~x & z))
#define BEAST_SHA2_MAJ(x, y, z) ((x & y) ^ (x & z) ^ (y & z))
#define BEAST_SHA256_F1(x) (BEAST_SHA2_ROTR(x,  2) ^ BEAST_SHA2_ROTR(x, 13) ^ BEAST_SHA2_ROTR(x, 22))
#define BEAST_SHA256_F2(x) (BEAST_SHA2_ROTR(x,  6) ^ BEAST_SHA2_ROTR(x, 11) ^ BEAST_SHA2_ROTR(x, 25))
#define BEAST_SHA256_F3(x) (BEAST_SHA2_ROTR(x,  7) ^ BEAST_SHA2_ROTR(x, 18) ^ BEAST_SHA2_SHFR(x,  3))
#define BEAST_SHA256_F4(x) (BEAST_SHA2_ROTR(x, 17) ^ BEAST_SHA2_ROTR(x, 19) ^ BEAST_SHA2_SHFR(x, 10))
#define BEAST_SHA512_F1(x) (BEAST_SHA2_ROTR(x, 28) ^ BEAST_SHA2_ROTR(x, 34) ^ BEAST_SHA2_ROTR(x, 39))
#define BEAST_SHA512_F2(x) (BEAST_SHA2_ROTR(x, 14) ^ BEAST_SHA2_ROTR(x, 18) ^ BEAST_SHA2_ROTR(x, 41))
#define BEAST_SHA512_F3(x) (BEAST_SHA2_ROTR(x,  1) ^ BEAST_SHA2_ROTR(x,  8) ^ BEAST_SHA2_SHFR(x,  7))
#define BEAST_SHA512_F4(x) (BEAST_SHA2_ROTR(x, 19) ^ BEAST_SHA2_ROTR(x, 61) ^ BEAST_SHA2_SHFR(x,  6))
#define BEAST_SHA2_PACK32(str, x)               \
{                                               \
    *(x) =                                      \
        ((std::uint32_t) *((str) + 3)      )    \
      | ((std::uint32_t) *((str) + 2) <<  8)    \
      | ((std::uint32_t) *((str) + 1) << 16)    \
      | ((std::uint32_t) *((str) + 0) << 24);   \
}
#define BEAST_SHA2_UNPACK32(x, str)             \
{                                               \
    *((str) + 3) = (std::uint8_t) ((x)      );  \
    *((str) + 2) = (std::uint8_t) ((x) >>  8);  \
    *((str) + 1) = (std::uint8_t) ((x) >> 16);  \
    *((str) + 0) = (std::uint8_t) ((x) >> 24);  \
}
#define BEAST_SHA2_PACK64(str, x)               \
{                                               \
    *(x) =                                      \
          ((std::uint64_t) *((str) + 7)      )  \
        | ((std::uint64_t) *((str) + 6) <<  8)  \
        | ((std::uint64_t) *((str) + 5) << 16)  \
        | ((std::uint64_t) *((str) + 4) << 24)  \
        | ((std::uint64_t) *((str) + 3) << 32)  \
        | ((std::uint64_t) *((str) + 2) << 40)  \
        | ((std::uint64_t) *((str) + 1) << 48)  \
        | ((std::uint64_t) *((str) + 0) << 56); \
}
#define BEAST_SHA2_UNPACK64(x, str)             \
{                                               \
    *((str) + 7) = (std::uint8_t) ((x)      );  \
    *((str) + 6) = (std::uint8_t) ((x) >>  8);  \
    *((str) + 5) = (std::uint8_t) ((x) >> 16);  \
    *((str) + 4) = (std::uint8_t) ((x) >> 24);  \
    *((str) + 3) = (std::uint8_t) ((x) >> 32);  \
    *((str) + 2) = (std::uint8_t) ((x) >> 40);  \
    *((str) + 1) = (std::uint8_t) ((x) >> 48);  \
    *((str) + 0) = (std::uint8_t) ((x) >> 56);  \
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//沙256

template <class = void>
void sha256_transform (sha256_context& ctx,
    unsigned char const* message,
        unsigned int block_nb) noexcept
{
    static unsigned long long const K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };
    std::uint32_t w[64];
    std::uint32_t wv[8];
    std::uint32_t t1, t2;
    unsigned char const* sub_block;
    int i, j;
    for (i = 0; i < (int) block_nb; i++) {
        sub_block = message + (i << 6);
        for (j = 0; j < 16; j++)
            BEAST_SHA2_PACK32(&sub_block[j << 2], &w[j]);
        for (j = 16; j < 64; j++)
            w[j] =  BEAST_SHA256_F4(
                w[j -  2]) + w[j -  7] +
                    BEAST_SHA256_F3(w[j - 15]) +
                        w[j - 16];
        for (j = 0; j < 8; j++)
            wv[j] = ctx.h[j];
        for (j = 0; j < 64; j++) {
            t1 = wv[7] + BEAST_SHA256_F2(wv[4]) +
                BEAST_SHA2_CH(wv[4], wv[5], wv[6]) +
                    K[j] + w[j];
            t2 = BEAST_SHA256_F1(wv[0]) +
                BEAST_SHA2_MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }
        for (j = 0; j < 8; j++)
            ctx.h[j] += wv[j];
    }
}

template <class = void>
void init (sha256_context& ctx) noexcept
{
    ctx.len = 0;
    ctx.tot_len = 0;
    ctx.h[0] = 0x6a09e667;
    ctx.h[1] = 0xbb67ae85;
    ctx.h[2] = 0x3c6ef372;
    ctx.h[3] = 0xa54ff53a;
    ctx.h[4] = 0x510e527f;
    ctx.h[5] = 0x9b05688c;
    ctx.h[6] = 0x1f83d9ab;
    ctx.h[7] = 0x5be0cd19;
}

template <class = void>
void update (sha256_context& ctx,
    void const* message, std::size_t size) noexcept
{
    auto const pm = reinterpret_cast<
        unsigned char const*>(message);
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;
    tmp_len = sha256_context::block_size - ctx.len;
    rem_len = size < tmp_len ? size : tmp_len;
    std::memcpy(&ctx.block[ctx.len], pm, rem_len);
    if (ctx.len + size < sha256_context::block_size) {
        ctx.len += size;
        return;
    }
    new_len = size - rem_len;
    block_nb = new_len / sha256_context::block_size;
    shifted_message = pm + rem_len;
    sha256_transform(ctx, ctx.block, 1);
    sha256_transform(ctx, shifted_message, block_nb);
    rem_len = new_len % sha256_context::block_size;
    std::memcpy(ctx.block, &shifted_message[
        block_nb << 6], rem_len);
    ctx.len = rem_len;
    ctx.tot_len += (block_nb + 1) << 6;
}

template <class = void>
void finish (sha256_context& ctx,
    void* digest) noexcept
{
    auto const pd = reinterpret_cast<
        unsigned char*>(digest);
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;
    int i;
    block_nb = (1 + ((sha256_context::block_size - 9) <
        (ctx.len % sha256_context::block_size)));
    len_b = (ctx.tot_len + ctx.len) << 3;
    pm_len = block_nb << 6;
    std::memset(ctx.block + ctx.len, 0, pm_len - ctx.len);
    ctx.block[ctx.len] = 0x80;
    BEAST_SHA2_UNPACK32(len_b, ctx.block + pm_len - 4);
    sha256_transform(ctx, ctx.block, block_nb);
    for (i = 0 ; i < 8; i++)
        BEAST_SHA2_UNPACK32(ctx.h[i], &pd[i << 2]);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

//沙512

template <class = void>
void sha512_transform (sha512_context& ctx,
    unsigned char const* message,
        unsigned int block_nb) noexcept
{
    static unsigned long long const K[80] = {
        0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL,
        0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
        0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL,
        0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
        0xd807aa98a3030242ULL, 0x12835b0145706fbeULL,
        0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
        0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL,
        0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
        0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL,
        0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
        0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL,
        0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
        0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL,
        0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
        0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL,
        0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
        0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL,
        0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
        0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL,
        0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
        0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL,
        0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
        0xd192e819d6ef5218ULL, 0xd69906245565a910ULL,
        0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
        0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL,
        0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
        0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL,
        0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
        0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL,
        0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
        0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL,
        0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
        0xca273eceea26619cULL, 0xd186b8c721c0c207ULL,
        0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
        0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL,
        0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
        0x28db77f523047d84ULL, 0x32caab7b40c72493ULL,
        0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
        0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL,
        0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL};

    std::uint64_t w[80];
    std::uint64_t wv[8];
    std::uint64_t t1, t2;
    unsigned char const* sub_block;
    int i, j;
    for (i = 0; i < (int) block_nb; i++)
    {
        sub_block = message + (i << 7);
        for (j = 0; j < 16; j++)
            BEAST_SHA2_PACK64(&sub_block[j << 3], &w[j]);
        for (j = 16; j < 80; j++)
            w[j] =  BEAST_SHA512_F4(
                w[j -  2]) + w[j -  7] +
                    BEAST_SHA512_F3(w[j - 15]) +
                        w[j - 16];
        for (j = 0; j < 8; j++)
            wv[j] = ctx.h[j];
        for (j = 0; j < 80; j++) {
            t1 = wv[7] + BEAST_SHA512_F2(wv[4]) +
                BEAST_SHA2_CH(wv[4], wv[5], wv[6]) +
                    K[j] + w[j];
            t2 = BEAST_SHA512_F1(wv[0]) +
                BEAST_SHA2_MAJ(wv[0], wv[1], wv[2]);
            wv[7] = wv[6];
            wv[6] = wv[5];
            wv[5] = wv[4];
            wv[4] = wv[3] + t1;
            wv[3] = wv[2];
            wv[2] = wv[1];
            wv[1] = wv[0];
            wv[0] = t1 + t2;
        }
        for (j = 0; j < 8; j++)
            ctx.h[j] += wv[j];
    }
}

template <class = void>
void init (sha512_context& ctx) noexcept
{
    ctx.len = 0;
    ctx.tot_len = 0;
    ctx.h[0] = 0x6a09e667f3bcc908ULL;
    ctx.h[1] = 0xbb67ae8584caa73bULL;
    ctx.h[2] = 0x3c6ef372fe94f82bULL;
    ctx.h[3] = 0xa54ff53a5f1d36f1ULL;
    ctx.h[4] = 0x510e527fade682d1ULL;
    ctx.h[5] = 0x9b05688c2b3e6c1fULL;
    ctx.h[6] = 0x1f83d9abfb41bd6bULL;
    ctx.h[7] = 0x5be0cd19137e2179ULL;
}

template <class = void>
void update (sha512_context& ctx,
    void const* message, std::size_t size) noexcept
{
    auto const pm = reinterpret_cast<
        unsigned char const*>(message);
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;
    tmp_len = sha512_context::block_size - ctx.len;
    rem_len = size < tmp_len ? size : tmp_len;
    std::memcpy(&ctx.block[ctx.len], pm, rem_len);
    if (ctx.len + size < sha512_context::block_size) {
        ctx.len += size;
        return;
    }
    new_len = size - rem_len;
    block_nb = new_len / sha512_context::block_size;
    shifted_message = pm + rem_len;
    sha512_transform(ctx, ctx.block, 1);
    sha512_transform(ctx, shifted_message, block_nb);
    rem_len = new_len % sha512_context::block_size;
    std::memcpy(ctx.block, &shifted_message[
        block_nb << 7], rem_len);
    ctx.len = rem_len;
    ctx.tot_len += (block_nb + 1) << 7;
}

template <class = void>
void finish (sha512_context& ctx,
    void* digest) noexcept
{
    auto const pd = reinterpret_cast<
        unsigned char*>(digest);
    unsigned int block_nb;
    unsigned int pm_len;
    unsigned int len_b;
    int i;
    block_nb = 1 + ((sha512_context::block_size - 17) <
        (ctx.len % sha512_context::block_size));
    len_b = (ctx.tot_len + ctx.len) << 3;
    pm_len = block_nb << 7;
    std::memset(ctx.block + ctx.len, 0, pm_len - ctx.len);
    ctx.block[ctx.len] = 0x80;
    BEAST_SHA2_UNPACK32(len_b, ctx.block + pm_len - 4);
    sha512_transform(ctx, ctx.block, block_nb);
    for (i = 0 ; i < 8; i++)
        BEAST_SHA2_UNPACK64(ctx.h[i], &pd[i << 3]);
}

} //细节
} //野兽

#endif
