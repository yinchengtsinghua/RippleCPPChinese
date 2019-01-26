
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
#ifndef ED25519_REF10_H
#define ED25519_REF10_H

int crypto_sign_pk_ref10(unsigned char *pk,unsigned char *sk);
int crypto_sign_ref10(unsigned char *sm,unsigned long long *smlen,const unsigned char *m,unsigned long long mlen,const unsigned char *sk);
int crypto_sign_open_ref10(unsigned char *m,unsigned long long *mlen,const unsigned char *sm,unsigned long long smlen,const unsigned char *pk);

/*DIF/*ED25519_ref10_h*/

