
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 公共领域作者：Andrew M.<liquidsun@gmail.com>
 修改自AMD64-51-30K实施
  丹尼尔·伯恩斯坦
  尼尔斯杜夫
  坦贾兰格
  施瓦布
  杨伯银
**/



#include "ed25519-donna-portable.h"

#if defined(ED25519_SSE2)
#else
	#if defined(HAVE_UINT128) && !defined(ED25519_FORCE_32BIT)
		#define ED25519_64BIT
	#else
		#define ED25519_32BIT
	#endif
#endif

#if !defined(ED25519_NO_INLINE_ASM)
 /*首先检测额外功能，以便在整个过程中禁用不需要的功能*/
	#if defined(ED25519_SSE2)
		#if defined(COMPILER_GCC) && defined(CPU_X86)
			#define ED25519_GCC_32BIT_SSE_CHOOSE
		#elif defined(COMPILER_GCC) && defined(CPU_X86_64)
			#define ED25519_GCC_64BIT_SSE_CHOOSE
		#endif
	#else
		#if defined(CPU_X86_64)
			#if defined(COMPILER_GCC) 
				#if defined(ED25519_64BIT)
					#define ED25519_GCC_64BIT_X86_CHOOSE
				#else
					#define ED25519_GCC_64BIT_32BIT_CHOOSE
				#endif
			#endif
		#endif
	#endif
#endif

#if defined(ED25519_SSE2)
	#include "curve25519-donna-sse2.h"
#elif defined(ED25519_64BIT)
	#include "curve25519-donna-64bit.h"
#else
	#include "curve25519-donna-32bit.h"
#endif

#include "curve25519-donna-helpers.h"

/*单独检查64位SSE2的uint128*/
#if defined(HAVE_UINT128) && !defined(ED25519_FORCE_32BIT)
	#include "modm-donna-64bit.h"
#else
	#include "modm-donna-32bit.h"
#endif

typedef unsigned char hash_512bits[64];

/*
 定时安全存储器比较
**/

static int
ed25519_verify(const unsigned char *x, const unsigned char *y, size_t len) {
	size_t differentbits = 0;
	while (len--)
		differentbits |= (*x++ ^ *y++);
	return (int) (1 & ((differentbits - 1) >> 8));
}


/*
 *扭曲爱德华兹曲线的算术-x^2+y^2=1+dx^2y^2
 *D=-（121665/121666）=37095705934669439343138083508754565189542113879843219016388785533085940283555
 *基点：（1512221349354007725011514095885315145401269304185720604611328394984776220246316835694926478169428339400347516314130793866256256225615783033603165251855960）；
 **/


typedef struct ge25519_t {
	bignum25519 x, y, z, t;
} ge25519;

typedef struct ge25519_p1p1_t {
	bignum25519 x, y, z, t;
} ge25519_p1p1;

typedef struct ge25519_niels_t {
	bignum25519 ysubx, xaddy, t2d;
} ge25519_niels;

typedef struct ge25519_pniels_t {
	bignum25519 ysubx, xaddy, z, t2d;
} ge25519_pniels;

#include "ed25519-donna-basepoint-table.h"

#if defined(ED25519_64BIT)
	#include "ed25519-donna-64bit-tables.h"
	#include "ed25519-donna-64bit-x86.h"
#else
	#include "ed25519-donna-32bit-tables.h"
	#include "ed25519-donna-64bit-x86-32bit.h"
#endif


#if defined(ED25519_SSE2)
	#include "ed25519-donna-32bit-sse2.h"
	#include "ed25519-donna-64bit-sse2.h"
	#include "ed25519-donna-impl-sse2.h"
#else
	#include "ed25519-donna-impl-base.h"
#endif

