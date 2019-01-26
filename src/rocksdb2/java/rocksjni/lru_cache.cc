
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
//该文件实现了Java和C++之间的“桥梁”。
//RocksDB：：lRucache。

#include <jni.h>

#include "cache/lru_cache.h"
#include "include/org_rocksdb_LRUCache.h"

/*
 *课程：Org_Rocksdb_Lrucache
 *方法：NewlRucache
 *签字：（Jizd）J
 **/

jlong Java_org_rocksdb_LRUCache_newLRUCache(
    JNIEnv* env, jclass jcls, jlong jcapacity, jint jnum_shard_bits,
    jboolean jstrict_capacity_limit, jdouble jhigh_pri_pool_ratio) {
  auto* sptr_lru_cache =
      new std::shared_ptr<rocksdb::Cache>(rocksdb::NewLRUCache(
          static_cast<size_t>(jcapacity),
          static_cast<int>(jnum_shard_bits),
          static_cast<bool>(jstrict_capacity_limit),
          static_cast<double>(jhigh_pri_pool_ratio)));
  return reinterpret_cast<jlong>(sptr_lru_cache);
}

/*
 *课程：Org_Rocksdb_Lrucache
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_LRUCache_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* sptr_lru_cache =
      reinterpret_cast<std::shared_ptr<rocksdb::Cache> *>(jhandle);
delete sptr_lru_cache;  //删除std:：shared_ptr
}
