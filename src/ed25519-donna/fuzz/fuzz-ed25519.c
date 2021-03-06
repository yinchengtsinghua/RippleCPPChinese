
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
#if defined(_WIN32)
	#include <windows.h>
	#include <wincrypt.h>
	typedef unsigned int uint32_t;
#else
	#include <stdint.h>
#endif

#include <string.h>
#include <stdio.h>

#include "ed25519-donna.h"
#include "ed25519-ref10.h"

static void
print_diff(const char *desc, const unsigned char *a, const unsigned char *b, size_t len) {
	size_t p = 0;
	unsigned char diff;
	printf("%s diff:\n", desc);
	while (len--) {
		diff = *a++ ^ *b++;
		if (!diff)
			printf("____,");
		else
			printf("0x%02x,", diff);
		if ((++p & 15) == 0)
			printf("\n");
	}
	printf("\n");
}

static void
print_bytes(const char *desc, const unsigned char *bytes, size_t len) {
	size_t p = 0;
	printf("%s:\n", desc);
	while (len--) {
		printf("0x%02x,", *bytes++);
		if ((++p & 15) == 0)
			printf("\n");
	}
	printf("\n");
}


/*查查20/12 prng*/
void
prng(unsigned char *out, size_t bytes) {
	static uint32_t state[16];
	static int init = 0;
	uint32_t x[16], t;
	size_t i;

	if (!init) {
	#if defined(_WIN32)
		HCRYPTPROV csp = NULL;
		if (!CryptAcquireContext(&csp, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
			printf("CryptAcquireContext failed\n");
			exit(1);
		}
		if (!CryptGenRandom(csp, (DWORD)sizeof(state), (BYTE*)state)) {
			printf("CryptGenRandom failed\n");
			exit(1);
		}
		CryptReleaseContext(csp, 0);
	#else
		FILE *f = NULL;
		f = fopen("/dev/urandom", "rb");
		if (!f) {
			printf("failed to open /dev/urandom\n");
			exit(1);
		}
		if (fread(state, sizeof(state), 1, f) != 1) {
			printf("read error on /dev/urandom\n");
			exit(1);
		}
	#endif
		init = 1;
	}

	while (bytes) {
		for (i = 0; i < 16; i++) x[i] = state[i];

		#define rotl32(x,k) ((x << k) | (x >> (32 - k)))
		#define quarter(a,b,c,d) \
			x[a] += x[b]; t = x[d]^x[a]; x[d] = rotl32(t,16); \
			x[c] += x[d]; t = x[b]^x[c]; x[b] = rotl32(t,12); \
			x[a] += x[b]; t = x[d]^x[a]; x[d] = rotl32(t, 8); \
			x[c] += x[d]; t = x[b]^x[c]; x[b] = rotl32(t, 7);

		for (i = 0; i < 12; i += 2) {
			quarter( 0, 4, 8,12)
			quarter( 1, 5, 9,13)
			quarter( 2, 6,10,14)
			quarter( 3, 7,11,15)
			quarter( 0, 5,10,15)
			quarter( 1, 6,11,12)
			quarter( 2, 7, 8,13)
			quarter( 3, 4, 9,14)
		};

		if (bytes <= 64) {
			memcpy(out, x, bytes);
			bytes = 0;
		} else {
			memcpy(out, x, 64);
			bytes -= 64;
			out += 64;
		}

  /*不需要现在，所以最后4个字是计数器。2^136字节可生成*/
		if (!++state[12]) if (!++state[13]) if (!++state[14]) ++state[15];
	}
}

typedef struct random_data_t {
	unsigned char sk[32];
	unsigned char m[128];
} random_data;

typedef struct generated_data_t {
	unsigned char pk[32];
	unsigned char sig[64];
	int valid;
} generated_data;

static void
print_generated(const char *desc, generated_data *g) {
	printf("%s:\n", desc);
	print_bytes("pk", g->pk, 32);
	print_bytes("sig", g->sig, 64);
	printf("valid: %s\n\n", g->valid ? "no" : "yes");
}

static void
print_generated_diff(const char *desc, const generated_data *base, generated_data *g) {
	printf("%s:\n", desc);
	print_diff("pk", base->pk, g->pk, 32);
	print_diff("sig", base->sig, g->sig, 64);
	printf("valid: %s\n\n", (base->valid == g->valid) ? "___" : (g->valid ? "no" : "yes"));
}

int main() {
	const size_t rndmax = 128;
	static random_data rnd[128];
	static generated_data gen[3];
	random_data *r;
	generated_data *g;
	unsigned long long dummylen;
	unsigned char dummysk[64];
	unsigned char dummymsg[2][128+64];
	size_t rndi, geni, i, j;
	uint64_t ctr;

	printf("fuzzing: ");
	printf(" ref10");
	printf(" ed25519-donna");
#if defined(ED25519_SSE2)
	printf(" ed25519-donna-sse2");
#endif
	printf("\n\n");

	for (ctr = 0, rndi = rndmax;;ctr++) {
		if (rndi == rndmax) {
			prng((unsigned char *)rnd, sizeof(rnd));
			rndi = 0;
		}
		r = &rnd[rndi++];

  /*参考文献10，很多可怕的体操围绕着不稳定的API工作*/
		geni = 0;
		g = &gen[geni++];
  /*cpy（dummysk，r->sk，32）；/*pk附加到sk，需要将sk复制到更大的缓冲区*/
  加密标记（dummysk+32，dummysk）；
  memcpy（g->pk，dummysk+32，32）；
  crypto_sign_ref10（dummymmsg[0]，&dummylen，r->m，128，dummysk）；
  memcpy（g->sig，dummymmsg[0]，64）；/*sig放在签名消息前面*/

		g->valid = crypto_sign_open_ref10(dummymsg[1], &dummylen, dummymsg[0], 128 + 64, g->pk);

  /*ED25519*/
		g = &gen[geni++];
		ed25519_publickey(r->sk, g->pk);
		ed25519_sign(r->m, 128, r->sk, g->pk, g->sig);
		g->valid = ed25519_sign_open(r->m, 128, g->pk, g->sig);

		#if defined(ED25519_SSE2)
   /*ED25519-唐娜-SSE2*/
			g = &gen[geni++];
			ed25519_publickey_sse2(r->sk, g->pk);
			ed25519_sign_sse2(r->m, 128, r->sk, g->pk, g->sig);
			g->valid = ed25519_sign_open_sse2(r->m, 128, g->pk, g->sig);
		#endif

  /*将实现1.geni与引用进行比较*/
		for (i = 1; i < geni; i++) {
			if (memcmp(&gen[0], &gen[i], sizeof(generated_data)) != 0) {
				printf("\n\n");
				print_bytes("sk", r->sk, 32);
				print_bytes("m", r->m, 128);
				print_generated("ref10", &gen[0]);
				print_generated_diff("ed25519-donna", &gen[0], &gen[1]);
				#if defined(ED25519_SSE2)
					print_generated_diff("ed25519-donna-sse2", &gen[0], &gen[2]);
				#endif
				exit(1);
			}
		}

  /*打印输出状态*/
		if (ctr && (ctr % 0x1000 == 0)) {
			printf(".");
			if ((ctr % 0x20000) == 0) {
				printf(" [");
				for (i = 0; i < 8; i++)
					printf("%02x", (unsigned char)(ctr >> ((7 - i) * 8)));
				printf("]\n");
			}
		}
	}
}