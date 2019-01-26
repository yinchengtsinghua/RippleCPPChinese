
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
#include <stdint.h>

typedef int32_t crypto_int32;
typedef int64_t crypto_int64;
typedef uint64_t crypto_uint64;

typedef crypto_int32 fe[10];

/*
H＝0
**/


void fe_0(fe h)
{
  h[0] = 0;
  h[1] = 0;
  h[2] = 0;
  h[3] = 0;
  h[4] = 0;
  h[5] = 0;
  h[6] = 0;
  h[7] = 0;
  h[8] = 0;
  h[9] = 0;
}

/*
H＝1
**/


void fe_1(fe h)
{
  h[0] = 1;
  h[1] = 0;
  h[2] = 0;
  h[3] = 0;
  h[4] = 0;
  h[5] = 0;
  h[6] = 0;
  h[7] = 0;
  h[8] = 0;
  h[9] = 0;
}

/*
H+F+G
可以将h与f或g重叠。

先决条件：
   f以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。
   _g以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。

Postconditions：
   h以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。
**/


void fe_add(fe h,fe f,fe g)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int32 g0 = g[0];
  crypto_int32 g1 = g[1];
  crypto_int32 g2 = g[2];
  crypto_int32 g3 = g[3];
  crypto_int32 g4 = g[4];
  crypto_int32 g5 = g[5];
  crypto_int32 g6 = g[6];
  crypto_int32 g7 = g[7];
  crypto_int32 g8 = g[8];
  crypto_int32 g9 = g[9];
  crypto_int32 h0 = f0 + g0;
  crypto_int32 h1 = f1 + g1;
  crypto_int32 h2 = f2 + g2;
  crypto_int32 h3 = f3 + g3;
  crypto_int32 h4 = f4 + g4;
  crypto_int32 h5 = f5 + g5;
  crypto_int32 h6 = f6 + g6;
  crypto_int32 h7 = f7 + g7;
  crypto_int32 h8 = f8 + g8;
  crypto_int32 h9 = f9 + g9;
  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
}

/*
H＝f
**/


void fe_copy(fe h,fe f)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  h[0] = f0;
  h[1] = f1;
  h[2] = f2;
  h[3] = f3;
  h[4] = f4;
  h[5] = f5;
  h[6] = f6;
  h[7] = f7;
  h[8] = f8;
  h[9] = f9;
}


/*
如果b=1，用（g，f）替换（f，g）；
如果b==0，用（f，g）替换（f，g）。

前提条件：b in 0,1。
**/


void fe_cswap(fe f,fe g,unsigned int b)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int32 g0 = g[0];
  crypto_int32 g1 = g[1];
  crypto_int32 g2 = g[2];
  crypto_int32 g3 = g[3];
  crypto_int32 g4 = g[4];
  crypto_int32 g5 = g[5];
  crypto_int32 g6 = g[6];
  crypto_int32 g7 = g[7];
  crypto_int32 g8 = g[8];
  crypto_int32 g9 = g[9];
  crypto_int32 x0 = f0 ^ g0;
  crypto_int32 x1 = f1 ^ g1;
  crypto_int32 x2 = f2 ^ g2;
  crypto_int32 x3 = f3 ^ g3;
  crypto_int32 x4 = f4 ^ g4;
  crypto_int32 x5 = f5 ^ g5;
  crypto_int32 x6 = f6 ^ g6;
  crypto_int32 x7 = f7 ^ g7;
  crypto_int32 x8 = f8 ^ g8;
  crypto_int32 x9 = f9 ^ g9;
  b = -b;
  x0 &= b;
  x1 &= b;
  x2 &= b;
  x3 &= b;
  x4 &= b;
  x5 &= b;
  x6 &= b;
  x7 &= b;
  x8 &= b;
  x9 &= b;
  f[0] = f0 ^ x0;
  f[1] = f1 ^ x1;
  f[2] = f2 ^ x2;
  f[3] = f3 ^ x3;
  f[4] = f4 ^ x4;
  f[5] = f5 ^ x5;
  f[6] = f6 ^ x6;
  f[7] = f7 ^ x7;
  f[8] = f8 ^ x8;
  f[9] = f9 ^ x9;
  g[0] = g0 ^ x0;
  g[1] = g1 ^ x1;
  g[2] = g2 ^ x2;
  g[3] = g3 ^ x3;
  g[4] = g4 ^ x4;
  g[5] = g5 ^ x5;
  g[6] = g6 ^ x6;
  g[7] = g7 ^ x7;
  g[8] = g8 ^ x8;
  g[9] = g9 ^ x9;
}

static crypto_uint64 load_3(const unsigned char *in)
{
  crypto_uint64 result;
  result = (crypto_uint64) in[0];
  result |= ((crypto_uint64) in[1]) << 8;
  result |= ((crypto_uint64) in[2]) << 16;
  return result;
}

static crypto_uint64 load_4(const unsigned char *in)
{
  crypto_uint64 result;
  result = (crypto_uint64) in[0];
  result |= ((crypto_uint64) in[1]) << 8;
  result |= ((crypto_uint64) in[2]) << 16;
  result |= ((crypto_uint64) in[3]) << 24;
  return result;
}

void fe_frombytes(fe h,const unsigned char *s)
{
  crypto_int64 h0 = load_4(s);
  crypto_int64 h1 = load_3(s + 4) << 6;
  crypto_int64 h2 = load_3(s + 7) << 5;
  crypto_int64 h3 = load_3(s + 10) << 3;
  crypto_int64 h4 = load_3(s + 13) << 2;
  crypto_int64 h5 = load_4(s + 16);
  crypto_int64 h6 = load_3(s + 20) << 7;
  crypto_int64 h7 = load_3(s + 23) << 5;
  crypto_int64 h8 = load_3(s + 26) << 4;
  crypto_int64 h9 = load_3(s + 29) << 2;
  crypto_int64 carry0;
  crypto_int64 carry1;
  crypto_int64 carry2;
  crypto_int64 carry3;
  crypto_int64 carry4;
  crypto_int64 carry5;
  crypto_int64 carry6;
  crypto_int64 carry7;
  crypto_int64 carry8;
  crypto_int64 carry9;

  carry9 = (h9 + (crypto_int64) (1<<24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  carry1 = (h1 + (crypto_int64) (1<<24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry3 = (h3 + (crypto_int64) (1<<24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry5 = (h5 + (crypto_int64) (1<<24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry7 = (h7 + (crypto_int64) (1<<24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

  carry0 = (h0 + (crypto_int64) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry2 = (h2 + (crypto_int64) (1<<25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry4 = (h4 + (crypto_int64) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry6 = (h6 + (crypto_int64) (1<<25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry8 = (h8 + (crypto_int64) (1<<25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
}


/*
H= F*G
可以将h与f或g重叠。

先决条件：
   f以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。
   _g以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。

Postconditions：
   h以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。
**/


/*
实施战略说明：

使用课本乘法。
在一些成本模型中，Karatsuba可以节省一点。

大多数乘2和乘19是32位预计算；
比64位后置运算便宜。

在进位链中还有一个剩余的乘19；
一个*19的预计算可以合并到这个中，
但由此产生的数据流却不那么干净。

下面有12个进位。
其中10个是双向可并行和可向量的。
可以通过11个进位逃脱，但数据流要深得多。

如果对输入有更严格的限制，可以将进位压缩到Int32。
**/


void fe_mul(fe h,fe f,fe g)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int32 g0 = g[0];
  crypto_int32 g1 = g[1];
  crypto_int32 g2 = g[2];
  crypto_int32 g3 = g[3];
  crypto_int32 g4 = g[4];
  crypto_int32 g5 = g[5];
  crypto_int32 g6 = g[6];
  crypto_int32 g7 = g[7];
  crypto_int32 g8 = g[8];
  crypto_int32 g9 = g[9];
  /*动力输出轴Int32 g1_19=19*g1；/*1.4*2^29*/
  crypto_int32 g2_19=19*g2；/*1.4*2^30；仍然正常*/

  crypto_int32 g3_19 = 19 * g3;
  crypto_int32 g4_19 = 19 * g4;
  crypto_int32 g5_19 = 19 * g5;
  crypto_int32 g6_19 = 19 * g6;
  crypto_int32 g7_19 = 19 * g7;
  crypto_int32 g8_19 = 19 * g8;
  crypto_int32 g9_19 = 19 * g9;
  crypto_int32 f1_2 = 2 * f1;
  crypto_int32 f3_2 = 2 * f3;
  crypto_int32 f5_2 = 2 * f5;
  crypto_int32 f7_2 = 2 * f7;
  crypto_int32 f9_2 = 2 * f9;
  crypto_int64 f0g0    = f0   * (crypto_int64) g0;
  crypto_int64 f0g1    = f0   * (crypto_int64) g1;
  crypto_int64 f0g2    = f0   * (crypto_int64) g2;
  crypto_int64 f0g3    = f0   * (crypto_int64) g3;
  crypto_int64 f0g4    = f0   * (crypto_int64) g4;
  crypto_int64 f0g5    = f0   * (crypto_int64) g5;
  crypto_int64 f0g6    = f0   * (crypto_int64) g6;
  crypto_int64 f0g7    = f0   * (crypto_int64) g7;
  crypto_int64 f0g8    = f0   * (crypto_int64) g8;
  crypto_int64 f0g9    = f0   * (crypto_int64) g9;
  crypto_int64 f1g0    = f1   * (crypto_int64) g0;
  crypto_int64 f1g1_2  = f1_2 * (crypto_int64) g1;
  crypto_int64 f1g2    = f1   * (crypto_int64) g2;
  crypto_int64 f1g3_2  = f1_2 * (crypto_int64) g3;
  crypto_int64 f1g4    = f1   * (crypto_int64) g4;
  crypto_int64 f1g5_2  = f1_2 * (crypto_int64) g5;
  crypto_int64 f1g6    = f1   * (crypto_int64) g6;
  crypto_int64 f1g7_2  = f1_2 * (crypto_int64) g7;
  crypto_int64 f1g8    = f1   * (crypto_int64) g8;
  crypto_int64 f1g9_38 = f1_2 * (crypto_int64) g9_19;
  crypto_int64 f2g0    = f2   * (crypto_int64) g0;
  crypto_int64 f2g1    = f2   * (crypto_int64) g1;
  crypto_int64 f2g2    = f2   * (crypto_int64) g2;
  crypto_int64 f2g3    = f2   * (crypto_int64) g3;
  crypto_int64 f2g4    = f2   * (crypto_int64) g4;
  crypto_int64 f2g5    = f2   * (crypto_int64) g5;
  crypto_int64 f2g6    = f2   * (crypto_int64) g6;
  crypto_int64 f2g7    = f2   * (crypto_int64) g7;
  crypto_int64 f2g8_19 = f2   * (crypto_int64) g8_19;
  crypto_int64 f2g9_19 = f2   * (crypto_int64) g9_19;
  crypto_int64 f3g0    = f3   * (crypto_int64) g0;
  crypto_int64 f3g1_2  = f3_2 * (crypto_int64) g1;
  crypto_int64 f3g2    = f3   * (crypto_int64) g2;
  crypto_int64 f3g3_2  = f3_2 * (crypto_int64) g3;
  crypto_int64 f3g4    = f3   * (crypto_int64) g4;
  crypto_int64 f3g5_2  = f3_2 * (crypto_int64) g5;
  crypto_int64 f3g6    = f3   * (crypto_int64) g6;
  crypto_int64 f3g7_38 = f3_2 * (crypto_int64) g7_19;
  crypto_int64 f3g8_19 = f3   * (crypto_int64) g8_19;
  crypto_int64 f3g9_38 = f3_2 * (crypto_int64) g9_19;
  crypto_int64 f4g0    = f4   * (crypto_int64) g0;
  crypto_int64 f4g1    = f4   * (crypto_int64) g1;
  crypto_int64 f4g2    = f4   * (crypto_int64) g2;
  crypto_int64 f4g3    = f4   * (crypto_int64) g3;
  crypto_int64 f4g4    = f4   * (crypto_int64) g4;
  crypto_int64 f4g5    = f4   * (crypto_int64) g5;
  crypto_int64 f4g6_19 = f4   * (crypto_int64) g6_19;
  crypto_int64 f4g7_19 = f4   * (crypto_int64) g7_19;
  crypto_int64 f4g8_19 = f4   * (crypto_int64) g8_19;
  crypto_int64 f4g9_19 = f4   * (crypto_int64) g9_19;
  crypto_int64 f5g0    = f5   * (crypto_int64) g0;
  crypto_int64 f5g1_2  = f5_2 * (crypto_int64) g1;
  crypto_int64 f5g2    = f5   * (crypto_int64) g2;
  crypto_int64 f5g3_2  = f5_2 * (crypto_int64) g3;
  crypto_int64 f5g4    = f5   * (crypto_int64) g4;
  crypto_int64 f5g5_38 = f5_2 * (crypto_int64) g5_19;
  crypto_int64 f5g6_19 = f5   * (crypto_int64) g6_19;
  crypto_int64 f5g7_38 = f5_2 * (crypto_int64) g7_19;
  crypto_int64 f5g8_19 = f5   * (crypto_int64) g8_19;
  crypto_int64 f5g9_38 = f5_2 * (crypto_int64) g9_19;
  crypto_int64 f6g0    = f6   * (crypto_int64) g0;
  crypto_int64 f6g1    = f6   * (crypto_int64) g1;
  crypto_int64 f6g2    = f6   * (crypto_int64) g2;
  crypto_int64 f6g3    = f6   * (crypto_int64) g3;
  crypto_int64 f6g4_19 = f6   * (crypto_int64) g4_19;
  crypto_int64 f6g5_19 = f6   * (crypto_int64) g5_19;
  crypto_int64 f6g6_19 = f6   * (crypto_int64) g6_19;
  crypto_int64 f6g7_19 = f6   * (crypto_int64) g7_19;
  crypto_int64 f6g8_19 = f6   * (crypto_int64) g8_19;
  crypto_int64 f6g9_19 = f6   * (crypto_int64) g9_19;
  crypto_int64 f7g0    = f7   * (crypto_int64) g0;
  crypto_int64 f7g1_2  = f7_2 * (crypto_int64) g1;
  crypto_int64 f7g2    = f7   * (crypto_int64) g2;
  crypto_int64 f7g3_38 = f7_2 * (crypto_int64) g3_19;
  crypto_int64 f7g4_19 = f7   * (crypto_int64) g4_19;
  crypto_int64 f7g5_38 = f7_2 * (crypto_int64) g5_19;
  crypto_int64 f7g6_19 = f7   * (crypto_int64) g6_19;
  crypto_int64 f7g7_38 = f7_2 * (crypto_int64) g7_19;
  crypto_int64 f7g8_19 = f7   * (crypto_int64) g8_19;
  crypto_int64 f7g9_38 = f7_2 * (crypto_int64) g9_19;
  crypto_int64 f8g0    = f8   * (crypto_int64) g0;
  crypto_int64 f8g1    = f8   * (crypto_int64) g1;
  crypto_int64 f8g2_19 = f8   * (crypto_int64) g2_19;
  crypto_int64 f8g3_19 = f8   * (crypto_int64) g3_19;
  crypto_int64 f8g4_19 = f8   * (crypto_int64) g4_19;
  crypto_int64 f8g5_19 = f8   * (crypto_int64) g5_19;
  crypto_int64 f8g6_19 = f8   * (crypto_int64) g6_19;
  crypto_int64 f8g7_19 = f8   * (crypto_int64) g7_19;
  crypto_int64 f8g8_19 = f8   * (crypto_int64) g8_19;
  crypto_int64 f8g9_19 = f8   * (crypto_int64) g9_19;
  crypto_int64 f9g0    = f9   * (crypto_int64) g0;
  crypto_int64 f9g1_38 = f9_2 * (crypto_int64) g1_19;
  crypto_int64 f9g2_19 = f9   * (crypto_int64) g2_19;
  crypto_int64 f9g3_38 = f9_2 * (crypto_int64) g3_19;
  crypto_int64 f9g4_19 = f9   * (crypto_int64) g4_19;
  crypto_int64 f9g5_38 = f9_2 * (crypto_int64) g5_19;
  crypto_int64 f9g6_19 = f9   * (crypto_int64) g6_19;
  crypto_int64 f9g7_38 = f9_2 * (crypto_int64) g7_19;
  crypto_int64 f9g8_19 = f9   * (crypto_int64) g8_19;
  crypto_int64 f9g9_38 = f9_2 * (crypto_int64) g9_19;
  crypto_int64 h0 = f0g0+f1g9_38+f2g8_19+f3g7_38+f4g6_19+f5g5_38+f6g4_19+f7g3_38+f8g2_19+f9g1_38;
  crypto_int64 h1 = f0g1+f1g0   +f2g9_19+f3g8_19+f4g7_19+f5g6_19+f6g5_19+f7g4_19+f8g3_19+f9g2_19;
  crypto_int64 h2 = f0g2+f1g1_2 +f2g0   +f3g9_38+f4g8_19+f5g7_38+f6g6_19+f7g5_38+f8g4_19+f9g3_38;
  crypto_int64 h3 = f0g3+f1g2   +f2g1   +f3g0   +f4g9_19+f5g8_19+f6g7_19+f7g6_19+f8g5_19+f9g4_19;
  crypto_int64 h4 = f0g4+f1g3_2 +f2g2   +f3g1_2 +f4g0   +f5g9_38+f6g8_19+f7g7_38+f8g6_19+f9g5_38;
  crypto_int64 h5 = f0g5+f1g4   +f2g3   +f3g2   +f4g1   +f5g0   +f6g9_19+f7g8_19+f8g7_19+f9g6_19;
  crypto_int64 h6 = f0g6+f1g5_2 +f2g4   +f3g3_2 +f4g2   +f5g1_2 +f6g0   +f7g9_38+f8g8_19+f9g7_38;
  crypto_int64 h7 = f0g7+f1g6   +f2g5   +f3g4   +f4g3   +f5g2   +f6g1   +f7g0   +f8g9_19+f9g8_19;
  crypto_int64 h8 = f0g8+f1g7_2 +f2g6   +f3g5_2 +f4g4   +f5g3_2 +f6g2   +f7g1_2 +f8g0   +f9g9_38;
  crypto_int64 h9 = f0g9+f1g8   +f2g7   +f3g6   +f4g5   +f5g4   +f6g3   +f7g2   +f8g1   +f9g0   ;
  crypto_int64 carry0;
  crypto_int64 carry1;
  crypto_int64 carry2;
  crypto_int64 carry3;
  crypto_int64 carry4;
  crypto_int64 carry5;
  crypto_int64 carry6;
  crypto_int64 carry7;
  crypto_int64 carry8;
  crypto_int64 carry9;

  /*
  h0<=（1.1*1.1*2^52*（1+19+19+19+19）+1.1*1.1*2^50*（38+38+38+38+38）
    即h0<=1.2*2^59；h2、h4、h6、h8的范围更窄
  h1<=（1.1*1.1*2^51*（1+1+19+19+19+19+19+19+19+19+19+19）
    即h1<=1.5*2^58；h3、h5、h7、h9的范围更窄
  **/


  carry0 = (h0 + (crypto_int64) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry4 = (h4 + (crypto_int64) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  /*H0＜2＝25*/
  /*H4≤2＝25*/
  /*h1<=1.51*2^58*/
  /*_h5<=1.51*2^58*/

  carry1 = (h1 + (crypto_int64) (1<<24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry5 = (h5 + (crypto_int64) (1<<24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  /*h1<=2^24；从现在开始适合Int32*/
  /*h5<=2^24；从现在开始，适合Int32*/
  /*h2<=1.21*2^59*/
  /*H6<=1.21*2^59*/

  carry2 = (h2 + (crypto_int64) (1<<25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry6 = (h6 + (crypto_int64) (1<<25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  /*h2<=2^25；从现在起，适应Int32不变*/
  /*H6<=2^25；从现在起，适应Int32不变*/
  /*H3<=1.51*2^58*/
  /*H7<=1.51*2^58*/

  carry3 = (h3 + (crypto_int64) (1<<24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry7 = (h7 + (crypto_int64) (1<<24)) >> 25; h8 += carry7; h7 -= carry7 << 25;
  /*H3<=2^24；从现在起，适应Int32不变*/
  /*H7<=2^24；从现在起，适应Int32不变*/
  /*h4<=1.52*2^33*/
  /*H8<=1.52*2^33*/

  carry4 = (h4 + (crypto_int64) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry8 = (h8 + (crypto_int64) (1<<25)) >> 26; h9 += carry8; h8 -= carry8 << 26;
  /*h4<=2^25；从现在起，适应Int32不变*/
  /*H8<=2^25；从现在起，适应Int32不变*/
  /*_h5<=1.01*2^24*/
  /*H9<=1.51*2^58*/

  carry9 = (h9 + (crypto_int64) (1<<24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  /*H9<=2^24；从现在起，适应Int32不变*/
  /*h0<=1.8*2^37*/

  carry0 = (h0 + (crypto_int64) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  /*h0<=2^25；从现在起，适应Int32不变*/
  /*h1<=1.01*2^24*/

  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
}

/*
H＝F＊121666
可以将h与f重叠。

先决条件：
   f以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。

Postconditions：
   h以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。
**/


void fe_mul121666(fe h,fe f)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int64 h0 = f0 * (crypto_int64) 121666;
  crypto_int64 h1 = f1 * (crypto_int64) 121666;
  crypto_int64 h2 = f2 * (crypto_int64) 121666;
  crypto_int64 h3 = f3 * (crypto_int64) 121666;
  crypto_int64 h4 = f4 * (crypto_int64) 121666;
  crypto_int64 h5 = f5 * (crypto_int64) 121666;
  crypto_int64 h6 = f6 * (crypto_int64) 121666;
  crypto_int64 h7 = f7 * (crypto_int64) 121666;
  crypto_int64 h8 = f8 * (crypto_int64) 121666;
  crypto_int64 h9 = f9 * (crypto_int64) 121666;
  crypto_int64 carry0;
  crypto_int64 carry1;
  crypto_int64 carry2;
  crypto_int64 carry3;
  crypto_int64 carry4;
  crypto_int64 carry5;
  crypto_int64 carry6;
  crypto_int64 carry7;
  crypto_int64 carry8;
  crypto_int64 carry9;

  carry9 = (h9 + (crypto_int64) (1<<24)) >> 25; h0 += carry9 * 19; h9 -= carry9 << 25;
  carry1 = (h1 + (crypto_int64) (1<<24)) >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry3 = (h3 + (crypto_int64) (1<<24)) >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry5 = (h5 + (crypto_int64) (1<<24)) >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry7 = (h7 + (crypto_int64) (1<<24)) >> 25; h8 += carry7; h7 -= carry7 << 25;

  carry0 = (h0 + (crypto_int64) (1<<25)) >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry2 = (h2 + (crypto_int64) (1<<25)) >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry4 = (h4 + (crypto_int64) (1<<25)) >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry6 = (h6 + (crypto_int64) (1<<25)) >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry8 = (h8 + (crypto_int64) (1<<25)) >> 26; h9 += carry8; h8 -= carry8 << 26;

  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
}

/*
H＝F＊f
可以将h与f重叠。

先决条件：
   f以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。

Postconditions：
   h以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。
**/


/*
有关实施策略的讨论，请参阅fe_mul.c。
**/


void fe_sq(fe h,fe f)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int32 f0_2 = 2 * f0;
  crypto_int32 f1_2 = 2 * f1;
  crypto_int32 f2_2 = 2 * f2;
  crypto_int32 f3_2 = 2 * f3;
  crypto_int32 f4_2 = 2 * f4;
  crypto_int32 f5_2 = 2 * f5;
  crypto_int32 f6_2 = 2 * f6;
  crypto_int32 f7_2 = 2 * f7;
  /*动力输出轴INT32 F5U 38=38*F5；/*1.31*2^30*/
  crypto_int32 f6_19=19*f6；/*1.31*2^30*/

  /*动力输出轴INT32 F7U 38=38*F7；/*1.31*2^30*/
  crypto_int32 f8_19=19*f8；/*1.31*2^30*/

  /*动力输出轴Int32 F9_38=38*F9；/*1.31*2^30*/
  crypto_int64 f0 f0=f0*（crypto_int64）f0；
  crypto_int64 f0 f1_2=f0_2*（crypto_int64）f1；
  crypto_int64 f0 f2_2=f0_2*（crypto_int64）f2；
  crypto_int64 f0 f3_2=f0_2*（crypto_int64）f3；
  crypto_int64 f0 f4_2=f0_2*（crypto_int64）f4；
  crypto_int64 f0 f5_2=f0_2*（crypto_int64）f5；
  crypto_int64 f0 f6_2=f0_2*（crypto_int64）f6；
  crypto_int64 f0 f7_2=f0_2*（crypto_int64）f7；
  crypto_int64 f0 f8_2=f0_2*（crypto_int64）f8；
  crypto_int64 f0 f9_2=f0_2*（crypto_int64）f9；
  crypto_int64 f1 f1_2=f1_2*（crypto_int64）f1；
  crypto_int64 f1 f2_2=f1_2*（crypto_int64）f2；
  crypto_int64 f1 f3_4=f1_2*（crypto_int64）f3_2；
  crypto_int64 f1 f4_2=f1_2*（crypto_int64）f4；
  crypto_int64 f1 f5_4=f1_2*（crypto_int64）f5_2；
  crypto_int64 f1 f6_2=f1_2*（crypto_int64）f6；
  crypto_int64 f1 f7_4=f1_2*（crypto_int64）f7_2；
  crypto_int64 f1 f8_2=f1_2*（crypto_int64）f8；
  crypto_int64 f1 f9_76=f1_2*（crypto_int64）f9_38；
  crypto_int64 f2 f2=f2*（crypto_int64）f2；
  crypto_int64 f2 f3_2=f2_2*（crypto_int64）f3；
  crypto_int64 f2 f4_2=f2_2*（crypto_int64）f4；
  crypto_int64 f2 f5_2=f2_2*（crypto_int64）f5；
  crypto_int64 f2 f6_2=f2_2*（crypto_int64）f6；
  crypto_int64 f2 f7_2=f2_2*（crypto_int64）f7；
  crypto_int64 f2 f8_38=f2_2*（crypto_int64）f8_19；
  crypto_int64 f2 f9_38=f2*（crypto_int64）f9_38；
  crypto_int64 f3 f3_2=f3_2*（crypto_int64）f3；
  crypto_int64 f3 f4_2=f3_2*（crypto_int64）f4；
  crypto_int64 f3 f5_4=f3_2*（crypto_int64）f5_2；
  crypto_int64 f3 f6_2=f3_2*（crypto_int64）f6；
  crypto_int64 f3 f7_76=f3_2*（crypto_int64）f7_38；
  crypto_int64 f3 f8_38=f3_2*（crypto_int64）f8_19；
  crypto_int64 f3 f9_76=f3_2*（crypto_int64）f9_38；
  crypto_int64 f4 f4=f4*（crypto_int64）f4；
  crypto_int64 f4 f5_2=f4_2*（crypto_int64）f5；
  crypto_int64 f4 f6_38=f4_2*（crypto_int64）f6_19；
  crypto_int64 f4 f7_38=f4*（crypto_int64）f7_38；
  crypto_int64 f4 f8_38=f4_2*（crypto_int64）f8_19；
  crypto_int64 f4 f9_38=f4*（crypto_int64）f9_38；
  crypto_int64 f5 f5_38=f5*（crypto_int64）f5_38；
  crypto_int64 f5 f6_38=f5_2*（crypto_int64）f6_19；
  crypto_int64 f5 f7_76=f5_2*（crypto_int64）f7_38；
  crypto_int64 f5 f8_38=f5_2*（crypto_int64）f8_19；
  crypto_int64 f5 f9_76=f5_2*（crypto_int64）f9_38；
  crypto_int64 f6 f6_19=f6*（crypto_int64）f6_19；
  crypto_int64 f6 f7_38=f6*（crypto_int64）f7_38；
  crypto_int64 f6 f8_38=f6_2*（crypto_int64）f8_19；
  crypto_int64 f6 f9_38=f6*（crypto_int64）f9_38；
  crypto_int64 f7 f7_38=f7*（crypto_int64）f7_38；
  crypto_int64 f7 f8_38=f7_2*（crypto_int64）f8_19；
  crypto_int64 f7 f9_76=f7_2*（crypto_int64）f9_38；
  crypto_int64 f8 f8_19=f8*（crypto_int64）f8_19；
  crypto_int64 f8 f9_38=f8*（crypto_int64）f9_38；
  crypto_int64 f9 f9_38=f9*（crypto_int64）f9_38；
  crypto_int64 h0=f0f0+f1f9_76+f2f8_38+f3f7_76+f4f6_38+f5f5_38；
  crypto_int64 h1=f0f1_2+f2f9_38+f3f8_38+f4f7_38+f5f6_38；
  crypto_int64 h2=f0f2_2+f1f1_2+f3f9_76+f4f8_38+f5f7_76+f6f6_19；
  crypto_int64 h3=f0f3_2+f1f2_2+f4f9_38+f5f8_38+f6f7_38；
  crypto_int64 h4=f0f4_2+f1f3_4+f2f2+f5f9_76+f6f8_38+f7f7_38；
  crypto_int64 h5=f0f5_2+f1f4_2+f2f3_2+f6f9_38+f7f8_38；
  crypto_int64 h6=f0f6_2+f1f5_4+f2f4_2+f3f3_2+f7f9_76+f8f8_19；
  crypto_int64 h7=f0f7_2+f1f6_2+f2f5_2+f3f4_2+f8f9_38；
  crypto_int64 h8=f0f8_2+f1f7_4+f2f6_2+f3f5_4+f4f4+f9f9_38；
  crypto_int64 h9=f0f9_2+f1f8_2+f2f7_2+f3f6_2+f4f5_2；
  加密输入64 carry0；
  加密输入64 carry1；
  加密输入64 carry2；
  加密输入64 carry3；
  加密输入64 carry4；
  加密输入64 carry5；
  加密输入64 carry6；
  加密输入64 carry7；
  加密输入64 carry8；
  加密输入64 carry9；

  carry0=（h0+（crypto_int64）（1<<25））>>26；h1+=carry0；h0-=carry0<<26；
  carry4=（h4+（crypto_int64）（1<<25））>>26；h5+=carry4；h4-=carry4<<26；

  carry1=（h1+（crypto_int64）（1<<24））>>25；h2+=carry1；h1-=carry1<<25；
  carry5=（h5+（crypto_int64）（1<<24））>>25；h6+=carry5；h5-=carry5<<25；

  carry2=（h2+（crypto_int64）（1<<25））>>26；h3+=carry2；h2-=carry2<<26；
  carry6=（h6+（crypto_int64）（1<<25））>>26；h7+=carry6；h6-=carry6<<26；

  carry3=（h3+（crypto_int64）（1<<24））>>25；h4+=carry3；h3-=carry3<<25；
  carry7=（h7+（crypto_int64）（1<<24））>>25；h8+=carry7；h7-=carry7<<25；

  carry4=（h4+（crypto_int64）（1<<25））>>26；h5+=carry4；h4-=carry4<<26；
  carry8=（h8+（crypto_int64）（1<<25））>>26；h9+=carry8；h8-=carry8<<26；

  carry9=（h9+（crypto_int64）（1<<24））>>25；h0+=carry9*19；h9-=carry9<<25；

  carry0=（h0+（crypto_int64）（1<<25））>>26；h1+=carry0；h0-=carry0<<26；

  H〔0〕＝H0；
  H〔1〕＝H1；
  H〔2〕＝H2；
  H〔3〕＝H3；
  H〔4〕＝H4；
  H〔5〕＝H5；
  H〔6〕＝H6；
  H〔7〕＝H7；
  H〔8〕＝H8；
  H〔9〕＝H9；
}

/*
H＝F—G
可以将h与f或g重叠。

先决条件：
   f以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。
   _g以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。

Postconditions：
   h以1.1*2^26、1.1*2^25、1.1*2^26、1.1*2^25等为界。
**/


void fe_sub(fe h,fe f,fe g)
{
  crypto_int32 f0 = f[0];
  crypto_int32 f1 = f[1];
  crypto_int32 f2 = f[2];
  crypto_int32 f3 = f[3];
  crypto_int32 f4 = f[4];
  crypto_int32 f5 = f[5];
  crypto_int32 f6 = f[6];
  crypto_int32 f7 = f[7];
  crypto_int32 f8 = f[8];
  crypto_int32 f9 = f[9];
  crypto_int32 g0 = g[0];
  crypto_int32 g1 = g[1];
  crypto_int32 g2 = g[2];
  crypto_int32 g3 = g[3];
  crypto_int32 g4 = g[4];
  crypto_int32 g5 = g[5];
  crypto_int32 g6 = g[6];
  crypto_int32 g7 = g[7];
  crypto_int32 g8 = g[8];
  crypto_int32 g9 = g[9];
  crypto_int32 h0 = f0 - g0;
  crypto_int32 h1 = f1 - g1;
  crypto_int32 h2 = f2 - g2;
  crypto_int32 h3 = f3 - g3;
  crypto_int32 h4 = f4 - g4;
  crypto_int32 h5 = f5 - g5;
  crypto_int32 h6 = f6 - g6;
  crypto_int32 h7 = f7 - g7;
  crypto_int32 h8 = f8 - g8;
  crypto_int32 h9 = f9 - g9;
  h[0] = h0;
  h[1] = h1;
  h[2] = h2;
  h[3] = h3;
  h[4] = h4;
  h[5] = h5;
  h[6] = h6;
  h[7] = h7;
  h[8] = h8;
  h[9] = h9;
}

/*
先决条件：
  h以1.1*2^25、1.1*2^24、1.1*2^25、1.1*2^24等为界。

写下p=2^255-19；q=floor（h/p）。
基本权利要求：Q=地板（2^（-255）（H+19 2^（-25）H9+2^（-1））。

证明：
  有h<=p so q<=1 so 19^2^（-255）q<1/4。
  也有H-2^230 H9<2^230所以19 2^（-255）（H-2^230 H9）<1/4。

  写y=2^（-1）-19^2 2^（-255）q-19 2^（-255）（h-2^230 h9）。
  0＜y＜1。

  写出R＝H-PQ。
  0<=R<=P-1=2^255-20。
  因此，0<=r+19（2^-255）r<r+19（2^-255）2^255<=2^255-1。

  写X=R+19（2^-255）R+Y。
  然后0<x<2^255，那么floor（2^（-255）x）=0，那么floor（q+2^（-255）x）=q。

  使q+2^（-255）x=2^（-255）（h+19 2^（-25）h9+2^（-1））
  所以地板（2^（-255）（H+19 2^（-25）H9+2^（-1））=Q。
**/


void fe_tobytes(unsigned char *s,fe h)
{
  crypto_int32 h0 = h[0];
  crypto_int32 h1 = h[1];
  crypto_int32 h2 = h[2];
  crypto_int32 h3 = h[3];
  crypto_int32 h4 = h[4];
  crypto_int32 h5 = h[5];
  crypto_int32 h6 = h[6];
  crypto_int32 h7 = h[7];
  crypto_int32 h8 = h[8];
  crypto_int32 h9 = h[9];
  crypto_int32 q;
  crypto_int32 carry0;
  crypto_int32 carry1;
  crypto_int32 carry2;
  crypto_int32 carry3;
  crypto_int32 carry4;
  crypto_int32 carry5;
  crypto_int32 carry6;
  crypto_int32 carry7;
  crypto_int32 carry8;
  crypto_int32 carry9;

  q = (19 * h9 + (((crypto_int32) 1) << 24)) >> 25;
  q = (h0 + q) >> 26;
  q = (h1 + q) >> 25;
  q = (h2 + q) >> 26;
  q = (h3 + q) >> 25;
  q = (h4 + q) >> 26;
  q = (h5 + q) >> 25;
  q = (h6 + q) >> 26;
  q = (h7 + q) >> 25;
  q = (h8 + q) >> 26;
  q = (h9 + q) >> 25;

  /*目标：输出h-（2^255-19）q，介于0和2^255-20之间。*/
  h0 += 19 * q;
  /*目标：输出H-2^255 Q，介于0和2^255-20之间。*/

  carry0 = h0 >> 26; h1 += carry0; h0 -= carry0 << 26;
  carry1 = h1 >> 25; h2 += carry1; h1 -= carry1 << 25;
  carry2 = h2 >> 26; h3 += carry2; h2 -= carry2 << 26;
  carry3 = h3 >> 25; h4 += carry3; h3 -= carry3 << 25;
  carry4 = h4 >> 26; h5 += carry4; h4 -= carry4 << 26;
  carry5 = h5 >> 25; h6 += carry5; h5 -= carry5 << 25;
  carry6 = h6 >> 26; h7 += carry6; h6 -= carry6 << 26;
  carry7 = h7 >> 25; h8 += carry7; h7 -= carry7 << 25;
  carry8 = h8 >> 26; h9 += carry8; h8 -= carry8 << 26;
  carry9 = h9 >> 25;               h9 -= carry9 << 25;
                  /*H10＝CARY9*/

  /*
  目标：输出h0+…+2^255 h10-2^255 q，介于0和2^255-20之间。
  H0+…+2^230 H9在0和2^255-1之间；
  显然2^255 h10-2^255 q=0。
  目标：输出H0+…+2^230 H9。
  **/


  s[0] = h0 >> 0;
  s[1] = h0 >> 8;
  s[2] = h0 >> 16;
  s[3] = (h0 >> 24) | (h1 << 2);
  s[4] = h1 >> 6;
  s[5] = h1 >> 14;
  s[6] = (h1 >> 22) | (h2 << 3);
  s[7] = h2 >> 5;
  s[8] = h2 >> 13;
  s[9] = (h2 >> 21) | (h3 << 5);
  s[10] = h3 >> 3;
  s[11] = h3 >> 11;
  s[12] = (h3 >> 19) | (h4 << 6);
  s[13] = h4 >> 2;
  s[14] = h4 >> 10;
  s[15] = h4 >> 18;
  s[16] = h5 >> 0;
  s[17] = h5 >> 8;
  s[18] = h5 >> 16;
  s[19] = (h5 >> 24) | (h6 << 1);
  s[20] = h6 >> 7;
  s[21] = h6 >> 15;
  s[22] = (h6 >> 23) | (h7 << 3);
  s[23] = h7 >> 5;
  s[24] = h7 >> 13;
  s[25] = (h7 >> 21) | (h8 << 4);
  s[26] = h8 >> 4;
  s[27] = h8 >> 12;
  s[28] = (h8 >> 20) | (h9 << 6);
  s[29] = h9 >> 2;
  s[30] = h9 >> 10;
  s[31] = h9 >> 18;
}

void fe_invert(fe out,fe z)
{
  fe t0;
  fe t1;
  fe t2;
  fe t3;
  int i;


/*QHASM:Fe Z1*/

/*QHASM:Fe Z2*/

/*QHASM:FE-Z8*/

/*QHASM:FE-Z9*/

/*QHASM:Fe Z11*/

/*QHASM:FE-Z22*/

/*QZAS:FEZZ5Y0*/

/*质量安全管理体系：铁Z_10_5*/

/*质量安全管理体系：铁Z_10_0*/

/*质量安全管理体系：铁Z_20_10*/

/*质量安全管理体系：铁Z_20_0*/

/*质量安全管理体系：铁Z_40_20*/

/*质量安全管理体系：铁Z_40_0*/

/*质量安全管理体系：铁Z_50_10*/

/*质量安全管理体系：铁Z_50_0*/

/*质量安全管理体系：铁Z_100_50*/

/*质量安全管理体系：铁Z_100_0*/

/*质量安全管理体系：铁Z_200_100*/

/*质量安全管理体系：铁Z_200_0*/

/*质量安全管理体系：铁Z_250_50*/

/*质量安全管理体系：铁Z_250_0*/

/*质量安全管理体系：铁Z_255_5*/

/*质量安全管理体系：铁Z_255_21*/

/*qasm：输入pow225521*/

/*qasm:z2=z1^2^1*/
/*asm 1:fe_sq（>z2=fe 1，<z1=fe 11）；对于（i=1；i<1；++i）fe_sq（>z2=fe 1，>z2=fe 1）；*/
/*asm 2:fe_sq（>z2=t0，<z1=z）；对于（i=1；i<1；+i）fe_sq（>z2=t0，>z2=t0）；*/
fe_sq(t0,z); for (i = 1;i < 1;++i) fe_sq(t0,t0);

/*qasm:z8=z2^2^2*/
/*asm 1:fe_sq（>z8=fe 2，<z2=fe 1）；对于（i=1；i<2；++i）fe_sq（>z8=fe 2，>z8=fe 2）；*/
/*asm 2:fe_sq（>z8=t1，<z2=t0）；对于（i=1；i<2；+i）fe_sq（>z8=t1，>z8=t1）；*/
fe_sq(t1,t0); for (i = 1;i < 2;++i) fe_sq(t1,t1);

/*质量安全管理体系：Z9=Z1*Z8*/
/*asm 1:fe_mul（>z9=fe 2，<z1=fe 11，<z8=fe 2）；*/
/*asm 2:fe_mul（>z9=t1，<z1=z，<z8=t1）；*/
fe_mul(t1,z,t1);

/*质量安全管理体系：Z11=Z2*Z9*/
/*asm 1:fe_mul（>z11=fe 1，<z2=fe 1，<z9=fe 2）；*/
/*asm 2:fe_mul（>z11=t0，<z2=t0，<z9=t1）；*/
fe_mul(t0,t0,t1);

/*质量安全管理体系：Z22=Z11^2^1*/
/*asm 1:fe_sq（>z22=fe 3，<z11=fe 1）；对于（i=1；i<1；++i）fe_sq（>z22=fe 3，>z22=fe 3）；*/
/*asm 2:fe_sq（>z22=t2，<z11=t0）；对于（i=1；i<1；++i）fe_sq（>z22=t2，>z22=t2）；*/
fe_sq(t2,t0); for (i = 1;i < 1;++i) fe_sq(t2,t2);

/*Qhasm:z_5_0=z9*z22*/
/*asm 1:fe_mul（>z_5_0=fe_2，<z9=fe_2，<z22=fe_3）；*/
/*asm 2:fe_mul（>z_5_0=t1，<z9=t1，<z22=t2）；*/
fe_mul(t1,t1,t2);

/*Qhasm:z_10_5=z_5_0^2^5*/
/*asm 1:fe_sq（>z_10_5=fe_3，<z_5_0=fe_2）；对于（i=1；i<5；+i）fe_sq（>z_10_5=fe_3，>z_10_5=fe_3）；*/
/*asm 2:fe_sq（>z_10_5=t2，<z_5_0=t1）；对于（i=1；i<5；+i）fe_sq（>z_10_5=t2，>z_10_5=t2）；*/
fe_sq(t2,t1); for (i = 1;i < 5;++i) fe_sq(t2,t2);

/*Qhasm:z_10_0=z_10_5*z_5_0*/
/*asm 1:fe_mul（>z_10_0=fe_2，<z_10_5=fe_3，<z_5_0=fe_2）；*/
/*asm 2:fe_mul（>z_10_0=t1，<z_10_5=t2，<z_5_0=t1）；*/
fe_mul(t1,t2,t1);

/*qasm:z_20_10=z_10_0^2^10*/
/*asm 1:fe_sq（>z_20_10=fe_3，<z_10_0=fe_2）；对于（i=1；i<10；+i）fe_sq（>z_20_10=fe_3，>z_20_10=fe_3）；*/
/*asm 2:fe_sq（>z_20_10=t2，<z_10_0=t1）；对于（i=1；i<10；+i）fe_sq（>z_20_10=t2，>z_20_10=t2）；*/
fe_sq(t2,t1); for (i = 1;i < 10;++i) fe_sq(t2,t2);

/*Qhasm:z_20_0=z_20_10*z_10_0*/
/*asm 1:fe_mul（>z_20_0=fe_3，<z_20_10=fe_3，<z_10_0=fe_2）；*/
/*asm 2:fe_mul（>z_20_0=t2，<z_20_10=t2，<z_10_0=t1）；*/
fe_mul(t2,t2,t1);

/*Qhasm:z_40_20=z_20_0^2^20*/
/*asm 1:fe_sq（>z_40_20=fe_4，<z_20_0=fe_3）；对于（i=1；i<20；+i）fe_sq（>z_40_20=fe_4，>z_40_20=fe_4）；*/
/*asm 2:fe_sq（>z_40_20=t3，<z_20_0=t2）；对于（i=1；i<20；+i）fe_sq（>z_40_20=t3，>z_40_20=t3）；*/
fe_sq(t3,t2); for (i = 1;i < 20;++i) fe_sq(t3,t3);

/*Qhasm:Z_40_0=Z_40_20*Z_20_0*/
/*asm 1:fe_mul（>z_40_0=fe_3，<z_40_20=fe_4，<z_20_0=fe_3）；*/
/*asm 2:fe_mul（>z_40_0=t2，<z_40_20=t3，<z_20_0=t2）；*/
fe_mul(t2,t3,t2);

/*qasm:z_50_10=z_40_0^2^10*/
/*asm 1:fe_sq（>z_50_10=fe_3，<z_40_0=fe_3）；对于（i=1；i<10；+i）fe_sq（>z_50_10=fe_3，>z_50_10=fe_3）；*/
/*asm 2:fe_sq（>z_50_10=t2，<z_40_0=t2）；对于（i=1；i<10；+i）fe_sq（>z_50_10=t2，>z_50_10=t2）；*/
fe_sq(t2,t2); for (i = 1;i < 10;++i) fe_sq(t2,t2);

/*Qhasm:z_50_0=z_50_10*z_10_0*/
/*asm 1:fe_mul（>z_50_0=fe_2，<z_50_10=fe_3，<z_10_0=fe_2）；*/
/*asm 2:fe_mul（>z_50_0=t1，<z_50_10=t2，<z_10_0=t1）；*/
fe_mul(t1,t2,t1);

/*qasm:z_100_50=z_50_0^2^50*/
/*asm 1:fe_sq（>z_100_50=fe_3，<z_50_0=fe_2）；对于（i=1；i<50；+i）fe_sq（>z_100_50=fe_3，>z_100_50=fe_3）；*/
/*asm 2:fe_sq（>z_100_50=t2，<z_50_0=t1）；对于（i=1；i<50；+i）fe_sq（>z_100_50=t2，>z_100_50=t2）；*/
fe_sq(t2,t1); for (i = 1;i < 50;++i) fe_sq(t2,t2);

/*Qhasm:Z_100_0=Z_100_50*Z_50_0*/
/*asm 1:fe_mul（>z_100_0=fe_3，<z_100_50=fe_3，<z_50_0=fe_2）；*/
/*asm 2:fe_mul（>z_100_0=t2，<z_100_50=t2，<z_50_0=t1）；*/
fe_mul(t2,t2,t1);

/*Qhasm:Z200_100=Z_100_0^2^100*/
/*asm 1:fe_sq（>z_200_100=fe_4，<z_100_0=fe_3）；对于（i=1；i<100；+i）fe_sq（>z_200_100=fe_4，>z_200_100=fe_4）；*/
/*asm 2:fe_sq（>z_200_100=t3，<z_100_0=t2）；对于（i=1；i<100；+i）fe_sq（>z_200_100=t3，>z_200_100=t3）；*/
fe_sq(t3,t2); for (i = 1;i < 100;++i) fe_sq(t3,t3);

/*qasm:z200_0=z_200_100*z_100_0*/
/*asm 1:fe_mul（>z_200_0=fe_3，<z_200_100=fe_4，<z_100_0=fe_3）；*/
/*asm 2:fe_mul（>z_200_0=t2，<z_200_100=t3，<z_100_0=t2）；*/
fe_mul(t2,t3,t2);

/*Qhasm:z250_50=z200_0^2^50*/
/*asm 1:fe_sq（>z_250_50=fe_3，<z_200_0=fe_3）；对于（i=1；i<50；+i）fe_sq（>z_250_50=fe_3，>z_250_50=fe_3）；*/
/*asm 2:fe_sq（>z_250_50=t2，<z_200_0=t2）；对于（i=1；i<50；+i）fe_sq（>z_250_50=t2，>z_250_50=t2）；*/
fe_sq(t2,t2); for (i = 1;i < 50;++i) fe_sq(t2,t2);

/*Qhasm:z250_0=z250_50*z_50_0*/
/*asm 1:fe_mul（>z_250_0=fe_2，<z_250_50=fe_3，<z_50_0=fe_2）；*/
/*asm 2:fe_mul（>z_250_0=t1，<z_250_50=t2，<z_50_0=t1）；*/
fe_mul(t1,t2,t1);

/*质量安全管理体系：Z_255_5=Z_250_0^2^5*/
/*asm 1:fe_sq（>z_255_5=fe_2，<z_250_0=fe_2）；对于（i=1；i<5；+i）fe_sq（>z_255_5=fe_2，>z_255_5=fe_2）；*/
/*asm 2:fe_sq（>z_255_5=T1，<z_250_0=T1）；对于（i=1；i<5；+i）fe_sq（>z_255_5=T1，>z_255_5=T1）；*/
fe_sq(t1,t1); for (i = 1;i < 5;++i) fe_sq(t1,t1);

/*质量安全管理体系：Z_255_21=Z_255_5*Z11*/
/*asm 1:fe_mul（>z_255_21=fe_12，<z_255_5=fe_2，<z11=fe_1）；*/
/*asm 2:fe_mul（>z_255_21=out，<z_255_5=t1，<z11=t0）；*/
fe_mul(out,t1,t0);

/*QHASM：返回*/

  return;
}


int crypto_scalarmult_ref10(unsigned char *q,
  const unsigned char *n,
  const unsigned char *p)
{
  unsigned char e[32];
  unsigned int i;
  fe x1;
  fe x2;
  fe z2;
  fe x3;
  fe z3;
  fe tmp0;
  fe tmp1;
  int pos;
  unsigned int swap;
  unsigned int b;

  for (i = 0;i < 32;++i) e[i] = n[i];
  e[0] &= 248;
  e[31] &= 127;
  e[31] |= 64;
  fe_frombytes(x1,p);
  fe_1(x2);
  fe_0(z2);
  fe_copy(x3,x1);
  fe_1(z3);

  swap = 0;
  for (pos = 254;pos >= 0;--pos) {
    b = e[pos / 8] >> (pos & 7);
    b &= 1;
    swap ^= b;
    fe_cswap(x2,x3,swap);
    fe_cswap(z2,z3,swap);
    swap = b;
/*QHASM:Fe x2*/

/*QHASM:Fe Z2*/

/*QHASM:Fe x3*/

/*QHASM:Fe Z3*/

/*QHASM:Fe x4*/

/*QHASM:Fe Z4*/

/*QHASM:Fe x5*/

/*QHASM:Fe Z5*/

/*QHASM：FEA*/

/*QHASM：Fe B*/

/*QHASM:FE-C*/

/*QHASM：FED*/

/*QHASM：FE*/

/*QHASM:FE-AA*/

/*QHASM:FE-BB*/

/*QHASM:FE-DA*/

/*QHASM:Fe CB*/

/*QHASM:Fe T0*/

/*QHASM:Fe T1*/

/*QHASM:Fe T2*/

/*QHASM:Fe T3*/

/*QHASM:Fe T4*/

/*Qhasm：进入梯子*/

/*Qhasm:d=x3-z3*/
/*asm 1:fe_sub（>d=fe 5，<x3=fe 3，<z3=fe 4）；*/
/*asm 2:fe_sub（>d=tmp0，<x3=x3，<z3=z3）；*/
fe_sub(tmp0,x3,z3);

/*Qhasm:b=x2-z2*/
/*asm 1:fe_sub（>b=fe 6，<x2=fe 1，<z2=fe 2）；*/
/*asm 2:fe_sub（>b=tmp1，<x2=x2，<z2=z2）；*/
fe_sub(tmp1,x2,z2);

/*Qhasm:a=x2+z2*/
/*asm 1:fe_添加（>a=fe 1，<x2=fe 1，<z2=fe 2）；*/
/*asm 2:fe_-add（>a=x2，<x2=x2，<z2=z2）；*/
fe_add(x2,x2,z2);

/*Qhasm:c=x3+z3*/
/*asm 1:fe_添加（>c=fe 2，<x3=fe 3，<z3=fe 4）；*/
/*asm 2:fe_-add（>c=z2，<x3=x3，<z3=z3）；*/
fe_add(z2,x3,z3);

/*QHASM:DA＝D＊A*/
/*asm 1:fe_mul（>d a=fe_4，<d=fe 5，<a=fe_1）；*/
/*asm 2:fe_mul（>d a=z3，<d=tmp0，<a=x2）；*/
fe_mul(z3,tmp0,x2);

/*QHASM:Cb= C*B*/
/*asm 1:fe_mul（>c b=fe 2，<c=fe 2，<b=fe 6）；*/
/*asm 2:fe_mul（>c b=z2，<c=z2，<b=tmp1）；*/
fe_mul(z2,z2,tmp1);

/*QHASM：BB= B^ 2*/
/*asm 1:fe_sq（>b b=fe 5，<b=fe 6）；*/
/*asm 2:fe_sq（>b b=tmp0，<b=tmp1）；*/
fe_sq(tmp0,tmp1);

/*QHASM：AA= a^ 2*/
/*asm 1:fe_sq（>a a=fe_6，<a=fe_1）；*/
/*asm 2:fe_sq（>a a=tmp1，<a=x2）；*/
fe_sq(tmp1,x2);

/*Qhasm:t0=da+cb*/
/*asm 1:fe_添加（>t0=fe 3，<da=fe 4，<cb=fe 2）；*/
/*asm 2:fe_-add（>t0=x3，<da=z3，<cb=z2）；*/
fe_add(x3,z3,z2);

/*qasm：将x3分配给t0*/

/*质量安全管理体系：T1=DA-CB*/
/*asm 1:fe_sub（>t1=fe 2，<da=fe 4，<cb=fe 2）；*/
/*asm 2:fe_sub（>t1=z2，<da=z3，<cb=z2）；*/
fe_sub(z2,z3,z2);

/*Qhasm:x4=aa*bb*/
/*asm 1:fe_mul（>x4=fe 1，<aa=fe 6，<bb=fe 5）；*/
/*asm 2:fe_mul（>x4=x2，<aa=tmp1，<bb=tmp0）；*/
fe_mul(x2,tmp1,tmp0);

/*质量健康管理体系：E=AA-BB*/
/*asm 1:fe_sub（>e=fe 6，<aa=fe 6，<bb=fe 5）；*/
/*asm 2:fe_sub（>e=tmp1，<aa=tmp1，<bb=tmp0）；*/
fe_sub(tmp1,tmp1,tmp0);

/*qasm:t2=t1^2*/
/*asm 1:fe_sq（>t2=fe 2，<t1=fe 2）；*/
/*asm 2:fe_sq（>t2=z2，<t1=z2）；*/
fe_sq(z2,z2);

/*质量安全管理体系：t3=A24*E*/
/*asm 1:fe_mul121666（>t3=fe 4，<e=fe 6）；*/
/*asm 2:fe_mul121666（>t3=z3，<e=tmp1）；*/
fe_mul121666(z3,tmp1);

/*qasm:x5=t0^2*/
/*asm 1:fe_sq（>x5=fe 3，<t0=fe 3）；*/
/*asm 2:fe_sq（>x5=x3，<t0=x3）；*/
fe_sq(x3,x3);

/*Qhasm:t4=bb+t3*/
/*asm 1:fe_添加（>t4=fe 5，<bb=fe 5，<t3=fe 4）；*/
/*asm 2:fe_添加（>t4=tmp0，<bb=tmp0，<t3=z3）；*/
fe_add(tmp0,tmp0,z3);

/*Qhasm:Z5=x1*t2*/
/*asm 1:fe_mul（>z5=fe 4，x1，<t2=fe 2）；*/
/*asm 2:fe_mul（>z5=z3，x1，<t2=z2）；*/
fe_mul(z3,x1,z2);

/*质量健康管理体系：Z4=E*T4*/
/*asm 1:fe_mul（>z4=fe 2，<e=fe 6，<t4=fe 5）；*/
/*asm 2:fe_mul（>z4=z2，<e=tmp1，<t4=tmp0）；*/
fe_mul(z2,tmp1,tmp0);

/*QHASM：返回*/
  }
  fe_cswap(x2,x3,swap);
  fe_cswap(z2,z3,swap);

  fe_invert(z2,z2);
  fe_mul(x2,x2,z2);
  fe_tobytes(q,x2);
  return 0;
}

static const unsigned char basepoint[32] = {9};

int crypto_scalarmult_base_ref10(unsigned char *q,const unsigned char *n)
{
  return crypto_scalarmult_ref10(q,n,basepoint);
}

