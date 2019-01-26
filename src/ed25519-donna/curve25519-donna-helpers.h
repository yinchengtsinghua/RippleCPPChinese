
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 公共领域作者：Andrew M.<liquidsun@gmail.com>
 参见：https://github.com/flowlyberry/curve25519-donna

 curve25519实现不可知帮助程序
**/


/*
 *英寸：B=2^5-2^0
 *输出：b=2^250-2^0
 **/

static void
curve25519_pow_two5mtwo0_two250mtwo0(bignum25519 b) {
	bignum25519 ALIGN(16) t0,c;

 /*2^5-2^0*/*b*/
 /*2^10-2^5*/曲线25519_平方_次（t0，b，5）；
 /* 2 ^ 10 - 2 ^ 0*/ curve25519_mul_noinline(b, t0, b);

 /*2^20-2^10*/曲线25519_平方_次（t0，b，10）；
 /* 2 ^ 20 - 2 ^ 0*/ curve25519_mul_noinline(c, t0, b);

 /*2^40-2^20*/曲线25519_平方_次（t0，c，20）；
 /* 2 ^ 40 - 2 ^ 0*/ curve25519_mul_noinline(t0, t0, c);

 /*2^50-2^10*/曲线25519_平方_次（t0，t0，10）；
 /* 2 ^ 50 - 2 ^ 0*/ curve25519_mul_noinline(b, t0, b);

 /*2^100-2^50*/曲线25519_平方_次（t0，b，50）；
 /* 2 ^ 100 - 2 ^ 0*/ curve25519_mul_noinline(c, t0, b);

 /*2^200-2^100*/曲线25519_平方_次（t0，c，100）；
 /* 2 ^ 200 - 2 ^ 0*/ curve25519_mul_noinline(t0, t0, c);

 /*2^250-2^50*/曲线25519_平方_次（t0，t0，50）；
 /* 2 ^ 250 - 2 ^ 0*/ curve25519_mul_noinline(b, t0, b);

}

/*
 *Z^（P-2）=Z（2^255-21）
 **/

static void
curve25519_recip(bignum25519 out, const bignum25519 z) {
	bignum25519 ALIGN(16) a,t0,b;

 /*2*/curve25519_square_times（a，z，1）；/*a=2*/
 /*8*/曲线25519_平方_次（t0，a，2）；
 /* 9*/ curve25519_mul_noinline(b, t0, z); /* b = 9 */

 /*11*/curve25519_mul_noinline（a，b，a）；/*a=11*/
 /*22*/曲线25519_平方_次（t0，a，1）；
 /*2^5-2^0=31*/ curve25519_mul_noinline(b, t0, b);

 /*2^250-2^0*/曲线25519_-pow_-two5mtwo0_-two250mtwo0（b）；
 /* 2 ^ 255 - 2 ^ 5*/ curve25519_square_times(b, b, 5);

 /*2^255-21*/curve25519_mul_noinline（输出，B，A）；
}

/*
 *Z^（（P-5）/8）=Z^（2^252-3）
 **/

static void
curve25519_pow_two252m3(bignum25519 two252m3, const bignum25519 z) {
	bignum25519 ALIGN(16) b,c,t0;

 /*2*/curve25519_square_times（c，z，1）；/*c=2*/
 /*8*/curve25519_square_times（t0，c，2）；/*t0=8*/
 /*9*/curve25519_mul_noinline（b，t0，z）；/*b=9*/
 /*11*/curve25519_mul_noinline（c，b，c）；/*c=11*/
 /*22*/曲线25519_平方_次（t0，c，1）；
 /*2^5-2^0=31*/ curve25519_mul_noinline(b, t0, b);

 /*2^250-2^0*/曲线25519_-pow_-two5mtwo0_-two250mtwo0（b）；
 /* 2 ^ 252 - 2 ^ 2*/ curve25519_square_times(b, b, 2);

 /*2^252-3*/曲线25519_-mul-noinline（two252m3，b，z）；
}
