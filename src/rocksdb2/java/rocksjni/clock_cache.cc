
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
//rocksdb：：时钟缓存。

#include <jni.h>

#include "cache/clock_cache.h"
#include "include/org_rocksdb_ClockCache.h"

/*
 *类别：Org_RocksDB_ClockCache
 *方法：newclockcache
 *签字：（Jiz）J
 **/

jlong Java_org_rocksdb_ClockCache_newClockCache(
    JNIEnv* env, jclass jcls, jlong jcapacity, jint jnum_shard_bits,
    jboolean jstrict_capacity_limit) {
  auto* sptr_clock_cache =
      new std::shared_ptr<rocksdb::Cache>(rocksdb::NewClockCache(
          static_cast<size_t>(jcapacity),
          static_cast<int>(jnum_shard_bits),
          static_cast<bool>(jstrict_capacity_limit)));
  return reinterpret_cast<jlong>(sptr_clock_cache);
}

/*
 *类别：Org_RocksDB_ClockCache
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_ClockCache_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* sptr_clock_cache =
      reinterpret_cast<std::shared_ptr<rocksdb::Cache> *>(jhandle);
delete sptr_clock_cache;  //删除std:：shared_ptr
}
