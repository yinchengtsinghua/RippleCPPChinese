
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 公共领域作者：Andrew M.<liquidsun@gmail.com>

 使用ed25519 donna的ed25519引用实现
**/



/*定义ed25519_后缀，使其附加到每个公共函数的末尾*/
#if !defined(ED25519_SUFFIX)
#define ED25519_SUFFIX 
#endif

#define ED25519_FN3(fn,suffix) fn##suffix
#define ED25519_FN2(fn,suffix) ED25519_FN3(fn,suffix)
#define ED25519_FN(fn)         ED25519_FN2(fn,ED25519_SUFFIX)

#include "ed25519-donna.h"
#include "ed25519.h"
#include "ed25519-randombytes.h"
#include "ed25519-hash.h"

/*
 生成（extsk[0..31]）和AExt（extsk[32..63]）
**/


DONNA_INLINE static void
ed25519_extsk(hash_512bits extsk, const ed25519_secret_key sk) {
	ed25519_hash(extsk, sk, 32);
	extsk[0] &= 248;
	extsk[31] &= 127;
	extsk[31] |= 64;
}

static void
ed25519_hram(hash_512bits hram, const ed25519_signature RS, const ed25519_public_key pk, const unsigned char *m, size_t mlen) {
	ed25519_hash_context ctx;
	ed25519_hash_init(&ctx);
	ed25519_hash_update(&ctx, RS, 32);
	ed25519_hash_update(&ctx, pk, 32);
	ed25519_hash_update(&ctx, m, mlen);
	ed25519_hash_final(&ctx, hram);
}

void
ED25519_FN(ed25519_publickey) (const ed25519_secret_key sk, ed25519_public_key pk) {
	bignum256modm a;
	ge25519 ALIGN(16) A;
	hash_512bits extsk;

 /*A=*/
	ed25519_extsk(extsk, sk);
	expand256_modm(a, extsk, 32);
	ge25519_scalarmult_base_niels(&A, ge25519_niels_base_multiples, a);
	ge25519_pack(pk, &A);
}


void
ED25519_FN(ed25519_sign) (const unsigned char *m, size_t mlen, const ed25519_secret_key sk, const ed25519_public_key pk, ed25519_signature RS) {
	ed25519_hash_context ctx;
	bignum256modm r, S, a;
	ge25519 ALIGN(16) R;
	hash_512bits extsk, hashr, hram;

	ed25519_extsk(extsk, sk);

 /*r=h（aext[32..64]，米）*/
	ed25519_hash_init(&ctx);
	ed25519_hash_update(&ctx, extsk + 32, 32);
	ed25519_hash_update(&ctx, m, mlen);
	ed25519_hash_final(&ctx, hashr);
	expand256_modm(r, hashr, 64);

 /*R= RB*/
	ge25519_scalarmult_base_niels(&R, ge25519_niels_base_multiples, r);
	ge25519_pack(RS, &R);

 /*S＝H（r，a，m）。*/
	ed25519_hram(hram, RS, pk, m, mlen);
	expand256_modm(S, hram, 64);

 /*S= H（R，A，M）A*/
	expand256_modm(a, extsk, 32);
	mul256_modm(S, S, a);

 /*S=（R+H（R，A，M）A）*/
	add256_modm(S, S, r);

 /*S=（R+H（R，A，M）A）模型L*/
	contract256_modm(RS + 32, S);
}

int
ED25519_FN(ed25519_sign_open) (const unsigned char *m, size_t mlen, const ed25519_public_key pk, const ed25519_signature RS) {
	ge25519 ALIGN(16) R, A;
	hash_512bits hash;
	bignum256modm hram, S;
	unsigned char checkR[32];

	if ((RS[63] & 224) || !ge25519_unpack_negative_vartime(&A, pk))
		return -1;

 /*HRAM＝h（r，a，m）*/
	ed25519_hram(hash, RS, pk, m, mlen);
	expand256_modm(hram, hash, 64);

 /*S*/
	expand256_modm(S, RS + 32, 32);

 /*SB—H（R，A，M）A*/
	ge25519_double_scalarmult_vartime(&R, &A, hram, S);
	ge25519_pack(checkR, &R);

 /*检查r=sb-h（r，a，m）a*/
	return ed25519_verify(RS, checkR, 32) ? 0 : -1;
}

#include "ed25519-donna-batchverify.h"

/*
 快速曲线25519基点标量乘法
**/


void
ED25519_FN(curved25519_scalarmult_basepoint) (curved25519_key pk, const curved25519_key e) {
	curved25519_key ec;
	bignum256modm s;
	bignum25519 ALIGN(16) yplusz, zminusy;
	ge25519 ALIGN(16) p;
	size_t i;

 /*夹紧*/
	for (i = 0; i < 32; i++) ec[i] = e[i];
	ec[0] &= 248;
	ec[31] &= 127;
	ec[31] |= 64;

	expand_raw256_modm(s, ec);

 /*标量*基点*/
	ge25519_scalarmult_base_niels(&p, ge25519_niels_base_multiples, s);

 /*u=（y+z）/（z-y）*/
	curve25519_add(yplusz, p.y, p.z);
	curve25519_sub(zminusy, p.z, p.y);
	curve25519_recip(zminusy, zminusy);
	curve25519_mul(yplusz, yplusz, zminusy);
	curve25519_contract(pk, yplusz);
}

