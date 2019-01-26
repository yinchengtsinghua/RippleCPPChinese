
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
#ifndef ED25519_H
#define ED25519_H

#include <stdlib.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned char ed25519_signature[64];
typedef unsigned char ed25519_public_key[32];
typedef unsigned char ed25519_secret_key[32];

typedef unsigned char curved25519_key[32];

void ed25519_publickey(const ed25519_secret_key sk, ed25519_public_key pk);
int ed25519_sign_open(const unsigned char *m, size_t mlen, const ed25519_public_key pk, const ed25519_signature RS);
void ed25519_sign(const unsigned char *m, size_t mlen, const ed25519_secret_key sk, const ed25519_public_key pk, ed25519_signature RS);

int ed25519_sign_open_batch(const unsigned char **m, size_t *mlen, const unsigned char **pk, const unsigned char **RS, size_t num, int *valid);

void ed25519_randombytes_unsafe(void *out, size_t count);

void curved25519_scalarmult_basepoint(curved25519_key pk, const curved25519_key e);

#if defined(__cplusplus)
}
#endif

#endif //ED25519YH
