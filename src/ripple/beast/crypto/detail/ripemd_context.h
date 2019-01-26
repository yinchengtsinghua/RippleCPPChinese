
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

#ifndef BEAST_CRYPTO_RIPEMD_CONTEXT_H_INCLUDED
#define BEAST_CRYPTO_RIPEMD_CONTEXT_H_INCLUDED

#include <array>
#include <cstdint>
#include <cstring>

namespace beast {
namespace detail {

//基于
//https://code.google.com/p/blockback/source/browse/trunk/ripemd160.cpp
/*
    版权所有（c）Katholieke Universiteit Leuven
    1996版权所有

    特此免费向任何获得副本的人授予许可。
    本软件及相关文档文件（以下简称“软件”），以处理
    在软件中不受限制，包括但不限于权利
    使用、复制、修改、合并、发布、分发、再许可和/或销售
    软件的副本，并允许软件所属人员
    按照以下条件提供：

    上述版权声明和本许可声明应包含在
    软件的所有副本或实质部分。

    本软件按“原样”提供，不提供任何形式的保证、明示或
    隐含的，包括但不限于对适销性的保证，
    适用于特定目的和非侵害。在任何情况下，
    作者或版权所有者对任何索赔、损害或其他
    在合同诉讼、侵权诉讼或其他诉讼中产生的责任，
    与软件或使用或其他交易有关的
    软件。
**/

//此实现已从
//原创。它已经被更新为C++ 11。

struct ripemd160_context
{
    explicit ripemd160_context() = default;

    static unsigned int const block_size = 64;
    static unsigned int const digest_size = 20;

    unsigned int tot_len;
    unsigned int len;
    unsigned char block[256];
    std::uint32_t h[5];
};

//rol（x，n）循环地将x绕n位向左旋转
//x必须是无符号32位类型，0<=n<32。
#define BEAST_RIPEMD_ROL(x, n)  (((x) << (n)) | ((x) >> (32-(n))))

//五个基本函数f（）、g（）和h（）。
#define BEAST_RIPEMD_F(x, y, z)  ((x) ^  (y) ^ (z))
#define BEAST_RIPEMD_G(x, y, z) (((x) &  (y)) | (~(x) & (z)))
#define BEAST_RIPEMD_H(x, y, z) (((x) | ~(y)) ^ (z))
#define BEAST_RIPEMD_I(x, y, z) (((x) &  (z)) | ((y) & ~(z)))
#define BEAST_RIPEMD_J(x, y, z)  ((x) ^  ((y) | ~(z)))

//从ff（）到iii（）的十个基本操作
#define BEAST_RIPEMD_FF(a, b, c, d, e, x, s) {      \
      (a) += BEAST_RIPEMD_F((b), (c), (d)) + (x);   \
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_GG(a, b, c, d, e, x, s) {      \
      (a) += BEAST_RIPEMD_G((b), (c), (d)) + (x) + 0x5a827999UL; \
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_HH(a, b, c, d, e, x, s) {      \
      (a) += BEAST_RIPEMD_H((b), (c), (d)) + (x) + 0x6ed9eba1UL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_II(a, b, c, d, e, x, s) {      \
      (a) += BEAST_RIPEMD_I((b), (c), (d)) + (x) + 0x8f1bbcdcUL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_JJ(a, b, c, d, e, x, s) {      \
      (a) += BEAST_RIPEMD_J((b), (c), (d)) + (x) + 0xa953fd4eUL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_FFF(a, b, c, d, e, x, s) {     \
      (a) += BEAST_RIPEMD_F((b), (c), (d)) + (x);   \
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_GGG(a, b, c, d, e, x, s) {     \
      (a) += BEAST_RIPEMD_G((b), (c), (d)) + (x) + 0x7a6d76e9UL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_HHH(a, b, c, d, e, x, s) {     \
      (a) += BEAST_RIPEMD_H((b), (c), (d)) + (x) + 0x6d703ef3UL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }
#define BEAST_RIPEMD_III(a, b, c, d, e, x, s) {     \
      (a) += BEAST_RIPEMD_I((b), (c), (d)) + (x) + 0x5c4dd124UL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10);  }
#define BEAST_RIPEMD_JJJ(a, b, c, d, e, x, s) {     \
      (a) += BEAST_RIPEMD_J((b), (c), (d)) + (x) + 0x50a28be6UL;\
      (a) = BEAST_RIPEMD_ROL((a), (s)) + (e);       \
      (c) = BEAST_RIPEMD_ROL((c), 10); }

template <class = void>
void ripemd_load (std::array<std::uint32_t, 16>& X,
    unsigned char const* p)
{
    for(int i = 0; i < 16; ++i)
    {
        X[i] =
            ((std::uint32_t) *((p)+3) << 24) |
            ((std::uint32_t) *((p)+2) << 16) |
            ((std::uint32_t) *((p)+1) <<  8) |
            ((std::uint32_t) * (p));
        p += 4;
    }
}

template <class = void>
void ripemd_compress (ripemd160_context& ctx,
        std::array<std::uint32_t, 16>& X) noexcept
{
    std::uint32_t aa  = ctx.h[0];
    std::uint32_t bb  = ctx.h[1];
    std::uint32_t cc  = ctx.h[2];
    std::uint32_t dd  = ctx.h[3];
    std::uint32_t ee  = ctx.h[4];
    std::uint32_t aaa = ctx.h[0];
    std::uint32_t bbb = ctx.h[1];
    std::uint32_t ccc = ctx.h[2];
    std::uint32_t ddd = ctx.h[3];
    std::uint32_t eee = ctx.h[4];

//第1轮
    BEAST_RIPEMD_FF(aa, bb, cc, dd, ee, X[ 0], 11);
    BEAST_RIPEMD_FF(ee, aa, bb, cc, dd, X[ 1], 14);
    BEAST_RIPEMD_FF(dd, ee, aa, bb, cc, X[ 2], 15);
    BEAST_RIPEMD_FF(cc, dd, ee, aa, bb, X[ 3], 12);
    BEAST_RIPEMD_FF(bb, cc, dd, ee, aa, X[ 4],  5);
    BEAST_RIPEMD_FF(aa, bb, cc, dd, ee, X[ 5],  8);
    BEAST_RIPEMD_FF(ee, aa, bb, cc, dd, X[ 6],  7);
    BEAST_RIPEMD_FF(dd, ee, aa, bb, cc, X[ 7],  9);
    BEAST_RIPEMD_FF(cc, dd, ee, aa, bb, X[ 8], 11);
    BEAST_RIPEMD_FF(bb, cc, dd, ee, aa, X[ 9], 13);
    BEAST_RIPEMD_FF(aa, bb, cc, dd, ee, X[10], 14);
    BEAST_RIPEMD_FF(ee, aa, bb, cc, dd, X[11], 15);
    BEAST_RIPEMD_FF(dd, ee, aa, bb, cc, X[12],  6);
    BEAST_RIPEMD_FF(cc, dd, ee, aa, bb, X[13],  7);
    BEAST_RIPEMD_FF(bb, cc, dd, ee, aa, X[14],  9);
    BEAST_RIPEMD_FF(aa, bb, cc, dd, ee, X[15],  8);

//第2轮
    BEAST_RIPEMD_GG(ee, aa, bb, cc, dd, X[ 7],  7);
    BEAST_RIPEMD_GG(dd, ee, aa, bb, cc, X[ 4],  6);
    BEAST_RIPEMD_GG(cc, dd, ee, aa, bb, X[13],  8);
    BEAST_RIPEMD_GG(bb, cc, dd, ee, aa, X[ 1], 13);
    BEAST_RIPEMD_GG(aa, bb, cc, dd, ee, X[10], 11);
    BEAST_RIPEMD_GG(ee, aa, bb, cc, dd, X[ 6],  9);
    BEAST_RIPEMD_GG(dd, ee, aa, bb, cc, X[15],  7);
    BEAST_RIPEMD_GG(cc, dd, ee, aa, bb, X[ 3], 15);
    BEAST_RIPEMD_GG(bb, cc, dd, ee, aa, X[12],  7);
    BEAST_RIPEMD_GG(aa, bb, cc, dd, ee, X[ 0], 12);
    BEAST_RIPEMD_GG(ee, aa, bb, cc, dd, X[ 9], 15);
    BEAST_RIPEMD_GG(dd, ee, aa, bb, cc, X[ 5],  9);
    BEAST_RIPEMD_GG(cc, dd, ee, aa, bb, X[ 2], 11);
    BEAST_RIPEMD_GG(bb, cc, dd, ee, aa, X[14],  7);
    BEAST_RIPEMD_GG(aa, bb, cc, dd, ee, X[11], 13);
    BEAST_RIPEMD_GG(ee, aa, bb, cc, dd, X[ 8], 12);

//第3轮
    BEAST_RIPEMD_HH(dd, ee, aa, bb, cc, X[ 3], 11);
    BEAST_RIPEMD_HH(cc, dd, ee, aa, bb, X[10], 13);
    BEAST_RIPEMD_HH(bb, cc, dd, ee, aa, X[14],  6);
    BEAST_RIPEMD_HH(aa, bb, cc, dd, ee, X[ 4],  7);
    BEAST_RIPEMD_HH(ee, aa, bb, cc, dd, X[ 9], 14);
    BEAST_RIPEMD_HH(dd, ee, aa, bb, cc, X[15],  9);
    BEAST_RIPEMD_HH(cc, dd, ee, aa, bb, X[ 8], 13);
    BEAST_RIPEMD_HH(bb, cc, dd, ee, aa, X[ 1], 15);
    BEAST_RIPEMD_HH(aa, bb, cc, dd, ee, X[ 2], 14);
    BEAST_RIPEMD_HH(ee, aa, bb, cc, dd, X[ 7],  8);
    BEAST_RIPEMD_HH(dd, ee, aa, bb, cc, X[ 0], 13);
    BEAST_RIPEMD_HH(cc, dd, ee, aa, bb, X[ 6],  6);
    BEAST_RIPEMD_HH(bb, cc, dd, ee, aa, X[13],  5);
    BEAST_RIPEMD_HH(aa, bb, cc, dd, ee, X[11], 12);
    BEAST_RIPEMD_HH(ee, aa, bb, cc, dd, X[ 5],  7);
    BEAST_RIPEMD_HH(dd, ee, aa, bb, cc, X[12],  5);

//第4轮
    BEAST_RIPEMD_II(cc, dd, ee, aa, bb, X[ 1], 11);
    BEAST_RIPEMD_II(bb, cc, dd, ee, aa, X[ 9], 12);
    BEAST_RIPEMD_II(aa, bb, cc, dd, ee, X[11], 14);
    BEAST_RIPEMD_II(ee, aa, bb, cc, dd, X[10], 15);
    BEAST_RIPEMD_II(dd, ee, aa, bb, cc, X[ 0], 14);
    BEAST_RIPEMD_II(cc, dd, ee, aa, bb, X[ 8], 15);
    BEAST_RIPEMD_II(bb, cc, dd, ee, aa, X[12],  9);
    BEAST_RIPEMD_II(aa, bb, cc, dd, ee, X[ 4],  8);
    BEAST_RIPEMD_II(ee, aa, bb, cc, dd, X[13],  9);
    BEAST_RIPEMD_II(dd, ee, aa, bb, cc, X[ 3], 14);
    BEAST_RIPEMD_II(cc, dd, ee, aa, bb, X[ 7],  5);
    BEAST_RIPEMD_II(bb, cc, dd, ee, aa, X[15],  6);
    BEAST_RIPEMD_II(aa, bb, cc, dd, ee, X[14],  8);
    BEAST_RIPEMD_II(ee, aa, bb, cc, dd, X[ 5],  6);
    BEAST_RIPEMD_II(dd, ee, aa, bb, cc, X[ 6],  5);
    BEAST_RIPEMD_II(cc, dd, ee, aa, bb, X[ 2], 12);

//第5轮
    BEAST_RIPEMD_JJ(bb, cc, dd, ee, aa, X[ 4],  9);
    BEAST_RIPEMD_JJ(aa, bb, cc, dd, ee, X[ 0], 15);
    BEAST_RIPEMD_JJ(ee, aa, bb, cc, dd, X[ 5],  5);
    BEAST_RIPEMD_JJ(dd, ee, aa, bb, cc, X[ 9], 11);
    BEAST_RIPEMD_JJ(cc, dd, ee, aa, bb, X[ 7],  6);
    BEAST_RIPEMD_JJ(bb, cc, dd, ee, aa, X[12],  8);
    BEAST_RIPEMD_JJ(aa, bb, cc, dd, ee, X[ 2], 13);
    BEAST_RIPEMD_JJ(ee, aa, bb, cc, dd, X[10], 12);
    BEAST_RIPEMD_JJ(dd, ee, aa, bb, cc, X[14],  5);
    BEAST_RIPEMD_JJ(cc, dd, ee, aa, bb, X[ 1], 12);
    BEAST_RIPEMD_JJ(bb, cc, dd, ee, aa, X[ 3], 13);
    BEAST_RIPEMD_JJ(aa, bb, cc, dd, ee, X[ 8], 14);
    BEAST_RIPEMD_JJ(ee, aa, bb, cc, dd, X[11], 11);
    BEAST_RIPEMD_JJ(dd, ee, aa, bb, cc, X[ 6],  8);
    BEAST_RIPEMD_JJ(cc, dd, ee, aa, bb, X[15],  5);
    BEAST_RIPEMD_JJ(bb, cc, dd, ee, aa, X[13],  6);

//平行圆1
    BEAST_RIPEMD_JJJ(aaa, bbb, ccc, ddd, eee, X[ 5],  8);
    BEAST_RIPEMD_JJJ(eee, aaa, bbb, ccc, ddd, X[14],  9);
    BEAST_RIPEMD_JJJ(ddd, eee, aaa, bbb, ccc, X[ 7],  9);
    BEAST_RIPEMD_JJJ(ccc, ddd, eee, aaa, bbb, X[ 0], 11);
    BEAST_RIPEMD_JJJ(bbb, ccc, ddd, eee, aaa, X[ 9], 13);
    BEAST_RIPEMD_JJJ(aaa, bbb, ccc, ddd, eee, X[ 2], 15);
    BEAST_RIPEMD_JJJ(eee, aaa, bbb, ccc, ddd, X[11], 15);
    BEAST_RIPEMD_JJJ(ddd, eee, aaa, bbb, ccc, X[ 4],  5);
    BEAST_RIPEMD_JJJ(ccc, ddd, eee, aaa, bbb, X[13],  7);
    BEAST_RIPEMD_JJJ(bbb, ccc, ddd, eee, aaa, X[ 6],  7);
    BEAST_RIPEMD_JJJ(aaa, bbb, ccc, ddd, eee, X[15],  8);
    BEAST_RIPEMD_JJJ(eee, aaa, bbb, ccc, ddd, X[ 8], 11);
    BEAST_RIPEMD_JJJ(ddd, eee, aaa, bbb, ccc, X[ 1], 14);
    BEAST_RIPEMD_JJJ(ccc, ddd, eee, aaa, bbb, X[10], 14);
    BEAST_RIPEMD_JJJ(bbb, ccc, ddd, eee, aaa, X[ 3], 12);
    BEAST_RIPEMD_JJJ(aaa, bbb, ccc, ddd, eee, X[12],  6);

//平行圆2
    BEAST_RIPEMD_III(eee, aaa, bbb, ccc, ddd, X[ 6],  9);
    BEAST_RIPEMD_III(ddd, eee, aaa, bbb, ccc, X[11], 13);
    BEAST_RIPEMD_III(ccc, ddd, eee, aaa, bbb, X[ 3], 15);
    BEAST_RIPEMD_III(bbb, ccc, ddd, eee, aaa, X[ 7],  7);
    BEAST_RIPEMD_III(aaa, bbb, ccc, ddd, eee, X[ 0], 12);
    BEAST_RIPEMD_III(eee, aaa, bbb, ccc, ddd, X[13],  8);
    BEAST_RIPEMD_III(ddd, eee, aaa, bbb, ccc, X[ 5],  9);
    BEAST_RIPEMD_III(ccc, ddd, eee, aaa, bbb, X[10], 11);
    BEAST_RIPEMD_III(bbb, ccc, ddd, eee, aaa, X[14],  7);
    BEAST_RIPEMD_III(aaa, bbb, ccc, ddd, eee, X[15],  7);
    BEAST_RIPEMD_III(eee, aaa, bbb, ccc, ddd, X[ 8], 12);
    BEAST_RIPEMD_III(ddd, eee, aaa, bbb, ccc, X[12],  7);
    BEAST_RIPEMD_III(ccc, ddd, eee, aaa, bbb, X[ 4],  6);
    BEAST_RIPEMD_III(bbb, ccc, ddd, eee, aaa, X[ 9], 15);
    BEAST_RIPEMD_III(aaa, bbb, ccc, ddd, eee, X[ 1], 13);
    BEAST_RIPEMD_III(eee, aaa, bbb, ccc, ddd, X[ 2], 11);

//平行圆3
    BEAST_RIPEMD_HHH(ddd, eee, aaa, bbb, ccc, X[15],  9);
    BEAST_RIPEMD_HHH(ccc, ddd, eee, aaa, bbb, X[ 5],  7);
    BEAST_RIPEMD_HHH(bbb, ccc, ddd, eee, aaa, X[ 1], 15);
    BEAST_RIPEMD_HHH(aaa, bbb, ccc, ddd, eee, X[ 3], 11);
    BEAST_RIPEMD_HHH(eee, aaa, bbb, ccc, ddd, X[ 7],  8);
    BEAST_RIPEMD_HHH(ddd, eee, aaa, bbb, ccc, X[14],  6);
    BEAST_RIPEMD_HHH(ccc, ddd, eee, aaa, bbb, X[ 6],  6);
    BEAST_RIPEMD_HHH(bbb, ccc, ddd, eee, aaa, X[ 9], 14);
    BEAST_RIPEMD_HHH(aaa, bbb, ccc, ddd, eee, X[11], 12);
    BEAST_RIPEMD_HHH(eee, aaa, bbb, ccc, ddd, X[ 8], 13);
    BEAST_RIPEMD_HHH(ddd, eee, aaa, bbb, ccc, X[12],  5);
    BEAST_RIPEMD_HHH(ccc, ddd, eee, aaa, bbb, X[ 2], 14);
    BEAST_RIPEMD_HHH(bbb, ccc, ddd, eee, aaa, X[10], 13);
    BEAST_RIPEMD_HHH(aaa, bbb, ccc, ddd, eee, X[ 0], 13);
    BEAST_RIPEMD_HHH(eee, aaa, bbb, ccc, ddd, X[ 4],  7);
    BEAST_RIPEMD_HHH(ddd, eee, aaa, bbb, ccc, X[13],  5);

//平行圆4
    BEAST_RIPEMD_GGG(ccc, ddd, eee, aaa, bbb, X[ 8], 15);
    BEAST_RIPEMD_GGG(bbb, ccc, ddd, eee, aaa, X[ 6],  5);
    BEAST_RIPEMD_GGG(aaa, bbb, ccc, ddd, eee, X[ 4],  8);
    BEAST_RIPEMD_GGG(eee, aaa, bbb, ccc, ddd, X[ 1], 11);
    BEAST_RIPEMD_GGG(ddd, eee, aaa, bbb, ccc, X[ 3], 14);
    BEAST_RIPEMD_GGG(ccc, ddd, eee, aaa, bbb, X[11], 14);
    BEAST_RIPEMD_GGG(bbb, ccc, ddd, eee, aaa, X[15],  6);
    BEAST_RIPEMD_GGG(aaa, bbb, ccc, ddd, eee, X[ 0], 14);
    BEAST_RIPEMD_GGG(eee, aaa, bbb, ccc, ddd, X[ 5],  6);
    BEAST_RIPEMD_GGG(ddd, eee, aaa, bbb, ccc, X[12],  9);
    BEAST_RIPEMD_GGG(ccc, ddd, eee, aaa, bbb, X[ 2], 12);
    BEAST_RIPEMD_GGG(bbb, ccc, ddd, eee, aaa, X[13],  9);
    BEAST_RIPEMD_GGG(aaa, bbb, ccc, ddd, eee, X[ 9], 12);
    BEAST_RIPEMD_GGG(eee, aaa, bbb, ccc, ddd, X[ 7],  5);
    BEAST_RIPEMD_GGG(ddd, eee, aaa, bbb, ccc, X[10], 15);
    BEAST_RIPEMD_GGG(ccc, ddd, eee, aaa, bbb, X[14],  8);

//平行圆5
    BEAST_RIPEMD_FFF(bbb, ccc, ddd, eee, aaa, X[12] ,  8);
    BEAST_RIPEMD_FFF(aaa, bbb, ccc, ddd, eee, X[15] ,  5);
    BEAST_RIPEMD_FFF(eee, aaa, bbb, ccc, ddd, X[10] , 12);
    BEAST_RIPEMD_FFF(ddd, eee, aaa, bbb, ccc, X[ 4] ,  9);
    BEAST_RIPEMD_FFF(ccc, ddd, eee, aaa, bbb, X[ 1] , 12);
    BEAST_RIPEMD_FFF(bbb, ccc, ddd, eee, aaa, X[ 5] ,  5);
    BEAST_RIPEMD_FFF(aaa, bbb, ccc, ddd, eee, X[ 8] , 14);
    BEAST_RIPEMD_FFF(eee, aaa, bbb, ccc, ddd, X[ 7] ,  6);
    BEAST_RIPEMD_FFF(ddd, eee, aaa, bbb, ccc, X[ 6] ,  8);
    BEAST_RIPEMD_FFF(ccc, ddd, eee, aaa, bbb, X[ 2] , 13);
    BEAST_RIPEMD_FFF(bbb, ccc, ddd, eee, aaa, X[13] ,  6);
    BEAST_RIPEMD_FFF(aaa, bbb, ccc, ddd, eee, X[14] ,  5);
    BEAST_RIPEMD_FFF(eee, aaa, bbb, ccc, ddd, X[ 0] , 15);
    BEAST_RIPEMD_FFF(ddd, eee, aaa, bbb, ccc, X[ 3] , 13);
    BEAST_RIPEMD_FFF(ccc, ddd, eee, aaa, bbb, X[ 9] , 11);
    BEAST_RIPEMD_FFF(bbb, ccc, ddd, eee, aaa, X[11] , 11);

//合并结果
ddd += cc + ctx.h[1]; //h[0]的最终结果
    ctx.h[1] = ctx.h[2] + dd + eee;
    ctx.h[2] = ctx.h[3] + ee + aaa;
    ctx.h[3] = ctx.h[4] + aa + bbb;
    ctx.h[4] = ctx.h[0] + bb + ccc;
    ctx.h[0] = ddd;
}

template <class = void>
void init (ripemd160_context& ctx) noexcept
{
    ctx.len = 0;
    ctx.tot_len = 0;
    ctx.h[0] = 0x67452301UL;
    ctx.h[1] = 0xefcdab89UL;
    ctx.h[2] = 0x98badcfeUL;
    ctx.h[3] = 0x10325476UL;
    ctx.h[4] = 0xc3d2e1f0UL;
}

template <class = void>
void update (ripemd160_context& ctx,
    void const* message, std::size_t size) noexcept
{
    auto const pm = reinterpret_cast<
        unsigned char const*>(message);
    unsigned int block_nb;
    unsigned int new_len, rem_len, tmp_len;
    const unsigned char *shifted_message;
    tmp_len = ripemd160_context::block_size - ctx.len;
    rem_len = size < tmp_len ? size : tmp_len;
    std::memcpy(&ctx.block[ctx.len], pm, rem_len);
    if (ctx.len + size < ripemd160_context::block_size) {
        ctx.len += size;
        return;
    }
    new_len = size - rem_len;
    block_nb = new_len / ripemd160_context::block_size;
    shifted_message = pm + rem_len;
    std::array<std::uint32_t, 16> X;
    ripemd_load(X, ctx.block);
    ripemd_compress(ctx, X);
    for (int i = 0; i < block_nb; ++i)
    {
        ripemd_load(X, shifted_message +
            i * ripemd160_context::block_size);
        ripemd_compress(ctx, X);
    }
    rem_len = new_len % ripemd160_context::block_size;
    std::memcpy(ctx.block, &shifted_message[
        block_nb * ripemd160_context::block_size],
            rem_len);
    ctx.len = rem_len;
    ctx.tot_len += (block_nb + 1) *
        ripemd160_context::block_size;
}

template <class = void>
void finish (ripemd160_context& ctx,
    void* digest) noexcept
{
    std::array<std::uint32_t, 16> X;
    X.fill(0);
//把剩菜放到X里
    auto p = &ctx.block[0];
//uint8_t i在位置8*处进入单词x[i div 4]（i mod 4）
    for (int i = 0; i < ctx.len; ++i)
      X[i >> 2] ^= (std::uint32_t) *p++ << (8 * (i & 3));
    ctx.tot_len += ctx.len;
//附加位m_n==1
    X[(ctx.tot_len>>2)&15] ^=
        (uint32_t)1 << (8*(ctx.tot_len&3) + 7);
//长度到下一个块？
    if ((ctx.tot_len & 63) > 55)
    {
        ripemd_compress(ctx, X);
        X.fill(0);
    }
//以位为单位附加长度*/
    X[14] = ctx.tot_len << 3;
    X[15] = (ctx.tot_len >> 29) | (0 << 3);
    ripemd_compress(ctx, X);

    std::uint8_t* pd = reinterpret_cast<
        std::uint8_t*>(digest);
    for (std::uint32_t i = 0; i < 20; i += 4)
    {
pd[i]   = (std::uint8_t)(ctx.h[i>>2]);          //隐式强制转换为uint8
pd[i+1] = (std::uint8_t)(ctx.h[i>>2] >>  8);    //提取最少8个
pd[i+2] = (std::uint8_t)(ctx.h[i>>2] >> 16);    //有效位。
        pd[i+3] = (std::uint8_t)(ctx.h[i>>2] >> 24);
    }
}

} //细节
} //野兽

#endif
