
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 自定义哈希必须具有512位摘要并实现：

 结构ed25519_hash_context；

 作废ed25519_hash_init（ed25519_hash_context*ctx）；
 void ed25519_hash_update（ed25519_hash_context*ctx，const uint8_t*in，size_t inlen）；
 作废ed25519_hash_final（ed25519_hash_context*ctx，uint8_t*hash）；
 void ed25519_hash（uint8_t*hash，const uint8_t*in，size_t inlen）；
**/


