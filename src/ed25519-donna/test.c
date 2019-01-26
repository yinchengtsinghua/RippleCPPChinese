
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 根据官方测试向量验证ED25519实现
 网址：http://ed25519.cr.yp.to/software.html
**/


#include <stdio.h>
#include <string.h>
#include "ed25519.h"

#include "test-ticks.h"

static void
edassert(int check, int round, const char *failreason) {
	if (check)
		return;
	printf("round %d, %s\n", round, failreason);
	exit(1);
}

static void
edassert_die(const unsigned char *a, const unsigned char *b, size_t len, int round, const char *failreason) {
	size_t i;
	if (round > 0)
		printf("round %d, %s\n", round, failreason);
	else
		printf("%s\n", failreason);
	printf("want: "); for (i = 0; i < len; i++) printf("%02x,", a[i]); printf("\n");
	printf("got : "); for (i = 0; i < len; i++) printf("%02x,", b[i]); printf("\n");
	printf("diff: "); for (i = 0; i < len; i++) if (a[i] ^ b[i]) printf("%02x,", a[i] ^ b[i]); else printf("  ,"); printf("\n\n");
	exit(1);
}

static void
edassert_equal(const unsigned char *a, const unsigned char *b, size_t len, const char *failreason) {
	if (memcmp(a, b, len) == 0)
		return;
	edassert_die(a, b, len, -1, failreason);
}

static void
edassert_equal_round(const unsigned char *a, const unsigned char *b, size_t len, int round, const char *failreason) {
	if (memcmp(a, b, len) == 0)
		return;
	edassert_die(a, b, len, round, failreason);
}


/*测试数据*/
typedef struct test_data_t {
	unsigned char sk[32], pk[32], sig[64];
	const char *m;
} test_data;


test_data dataset[] = {
#include "regression.h"
};

/*曲线25519的结果scalarmult（（255*基点）*基点）…1024次*/
const curved25519_key curved25519_expected = {
	0xac,0xce,0x24,0xb1,0xd4,0xa2,0x36,0x21,
	0x15,0xe2,0x3e,0x84,0x3c,0x23,0x2b,0x5f,
	0x95,0x6c,0xc0,0x7b,0x95,0x82,0xd7,0x93,
	0xd5,0x19,0xb6,0xf1,0xfb,0x96,0xd6,0x04
};


/*来自ed25519 donna batchverify.h*/
extern unsigned char batch_point_buffer[3][32];

/*“AMD64-51-30K”最后一点的Y坐标与同一随机发生器*/
static const unsigned char batch_verify_y[32] = {
	0x51,0xe7,0x68,0xe0,0xf7,0xa1,0x88,0x45,
	0xde,0xa1,0xcb,0xd9,0x37,0xd4,0x78,0x53,
	0x1b,0x95,0xdb,0xbe,0x66,0x59,0x29,0x3b,
	0x94,0x51,0x2f,0xbc,0x0d,0x66,0xba,0x3f
};

/*
静态常量unsigned char batch_verify_y[32]=
 0X5C、0X63、0X96、0X26、0XCA、0XFE、0XFD、0XC4、
 0x2d、0x11、0xA8、0xE4、0xC4、0x46、0x42、0x97，
 0x97、0x92、0xbe、0xe0、0x3c、0xef、0x96、0x01、
 0×50,0×A1,0×CC，0×8F，0×50,0×85,0×76,0×7D
}；

将128位r scalars引入到堆\最大标量之前
适合128位改变堆的形状并产生不同的，
但仍然是中性/有效的Y/Z值。

这是在最大标量匹配时引入r标量的值
在135-256位之间。您可以用AMD64-64-24K/AMD64-51-32K制作
第一次通过时使用的随机序列

    无符号长hlen=（npoints+1）/2）1；

到

    无符号长hlen=npoints；

在ge25519_multi_scalarmult.c中

已修改ed25519 donna batchverify.h以匹配
默认AMD64-64-24K/AMD64-51-32K行为
**/




/*分批试验*/
#define test_batch_count 64
#define test_batch_rounds 96

typedef enum batch_test_t {
	batch_no_errors = 0,
	batch_wrong_message = 1,
	batch_wrong_pk = 2,
	batch_wrong_sig = 3
} batch_test;

static int
test_batch_instance(batch_test type, uint64_t *ticks) {
	ed25519_secret_key sks[test_batch_count];
	ed25519_public_key pks[test_batch_count];
	ed25519_signature sigs[test_batch_count];
	unsigned char messages[test_batch_count][128];
	size_t message_lengths[test_batch_count];
	const unsigned char *message_pointers[test_batch_count];
	const unsigned char *pk_pointers[test_batch_count];
	const unsigned char *sig_pointers[test_batch_count];
	int valid[test_batch_count], ret, validret;
	size_t i;
	uint64_t t;

 /*生成密钥*/
	for (i = 0; i < test_batch_count; i++) {
		ed25519_randombytes_unsafe(sks[i], sizeof(sks[i]));
		ed25519_publickey(sks[i], pks[i]);
		pk_pointers[i] = pks[i];
	}

 /*生成消息*/
	ed25519_randombytes_unsafe(messages, sizeof(messages));
	for (i = 0; i < test_batch_count; i++) {
		message_pointers[i] = messages[i];
		message_lengths[i] = (i & 127) + 1;
	}

 /*签名消息*/
	for (i = 0; i < test_batch_count; i++) {
		ed25519_sign(message_pointers[i], message_lengths[i], sks[i], pks[i], sigs[i]);
		sig_pointers[i] = sigs[i];
	}

	validret = 0;
	if (type == batch_wrong_message) {
		message_pointers[0] = message_pointers[1];
		validret = 1|2;
	} else if (type == batch_wrong_pk) {
		pk_pointers[0] = pk_pointers[1];
		validret = 1|2;
	} else if (type == batch_wrong_sig) {
		sig_pointers[0] = sig_pointers[1];
		validret = 1|2;
	}

 /*批量验证*/
	t = get_ticks();
	ret = ed25519_sign_open_batch(message_pointers, message_lengths, pk_pointers, sig_pointers, test_batch_count, valid);
	*ticks = get_ticks() - t;
	edassert_equal((unsigned char *)&validret, (unsigned char *)&ret, sizeof(int), "batch return code");
	for (i = 0; i < test_batch_count; i++) {
		validret = ((type == batch_no_errors) || (i != 0)) ? 1 : 0;
		edassert_equal((unsigned char *)&validret, (unsigned char *)&valid[i], sizeof(int), "individual batch return code");
	}
	return ret;
}

static void
test_batch(void) {
	uint64_t dummy_ticks, ticks[test_batch_rounds], best = maxticks, sum;
	size_t i, count;

 /*检查第一遍是否达到预期结果*/
	test_batch_instance(batch_no_errors, &dummy_ticks);
	edassert_equal(batch_verify_y, batch_point_buffer[1], 32, "failed to generate expected result");

 /*确保GE25519_multi_scalarmult_vartime在整批数据错误的情况下引发错误*/
	for (i = 0; i < 4; i++) {
		test_batch_instance(batch_wrong_message, &dummy_ticks);
		test_batch_instance(batch_wrong_pk, &dummy_ticks);
		test_batch_instance(batch_wrong_sig, &dummy_ticks);
	}

 /*速度测试*/
	for (i = 0; i < test_batch_rounds; i++) {
		test_batch_instance(batch_no_errors, &ticks[i]);
		if (ticks[i] < best)
			best = ticks[i];
	}

 /*在1%的最佳时间内吃任何东西*/
	for (i = 0, sum = 0, count = 0; i < test_batch_rounds; i++) {
		if (ticks[i] < (best * 1.01)) {
			sum += ticks[i];
			count++;
		}
	}
	printf("%.0f ticks/verification\n", (double)sum / (count * test_batch_count));
}

static void
test_main(void) {
	int i, res;
	ed25519_public_key pk;
	ed25519_signature sig;
	unsigned char forge[1024] = {'x'};
	curved25519_key csk[2] = {{255}};
	uint64_t ticks, pkticks = maxticks, signticks = maxticks, openticks = maxticks, curvedticks = maxticks;

	for (i = 0; i < 1024; i++) {
		ed25519_publickey(dataset[i].sk, pk);
		edassert_equal_round(dataset[i].pk, pk, sizeof(pk), i, "public key didn't match");
		ed25519_sign((unsigned char *)dataset[i].m, i, dataset[i].sk, pk, sig);
		edassert_equal_round(dataset[i].sig, sig, sizeof(sig), i, "signature didn't match");
		edassert(!ed25519_sign_open((unsigned char *)dataset[i].m, i, pk, sig), i, "failed to open message");

		memcpy(forge, dataset[i].m, i);
		if (i)
			forge[i - 1] += 1;

		edassert(ed25519_sign_open(forge, (i) ? i : 1, pk, sig), i, "opened forged message");
	}

	for (i = 0; i < 1024; i++)
		curved25519_scalarmult_basepoint(csk[(i & 1) ^ 1], csk[i & 1]);
	edassert_equal(curved25519_expected, csk[0], sizeof(curved25519_key), "curve25519 failed to generate correct value");

	for (i = 0; i < 2048; i++) {
		timeit(ed25519_publickey(dataset[0].sk, pk), pkticks)
		edassert_equal_round(dataset[0].pk, pk, sizeof(pk), i, "public key didn't match");
		timeit(ed25519_sign((unsigned char *)dataset[0].m, 0, dataset[0].sk, pk, sig), signticks)
		edassert_equal_round(dataset[0].sig, sig, sizeof(sig), i, "signature didn't match");
		timeit(res = ed25519_sign_open((unsigned char *)dataset[0].m, 0, pk, sig), openticks)
		edassert(!res, 0, "failed to open message");
		timeit(curved25519_scalarmult_basepoint(csk[1], csk[0]), curvedticks);
	}

	printf("%.0f ticks/public key generation\n", (double)pkticks);
	printf("%.0f ticks/signature\n", (double)signticks);
	printf("%.0f ticks/signature verification\n", (double)openticks);
	printf("%.0f ticks/curve25519 basepoint scalarmult\n", (double)curvedticks);
}

int
main(void) {
	test_main();
	test_batch();
	return 0;
}

