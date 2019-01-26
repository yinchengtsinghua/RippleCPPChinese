
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
//该文件实现了RateLimiter与C++之间的“桥梁”。

#include "rocksjni/portal.h"
#include "include/org_rocksdb_RateLimiter.h"
#include "rocksdb/rate_limiter.h"

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：Newratelimiterhandle
 *签字：（JJI）J
 **/

jlong Java_org_rocksdb_RateLimiter_newRateLimiterHandle(
    JNIEnv* env, jclass jclazz, jlong jrate_bytes_per_second,
    jlong jrefill_period_micros, jint jfairness) {
  auto * sptr_rate_limiter =
      new std::shared_ptr<rocksdb::RateLimiter>(rocksdb::NewGenericRateLimiter(
          static_cast<int64_t>(jrate_bytes_per_second),
          static_cast<int64_t>(jrefill_period_micros),
          static_cast<int32_t>(jfairness)));

  return reinterpret_cast<jlong>(sptr_rate_limiter);
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_RateLimiter_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* handle =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(jhandle);
delete handle;  //删除std:：shared_ptr
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：setbytespersecond
 *签字：（JJ）V
 **/

void Java_org_rocksdb_RateLimiter_setBytesPerSecond(
    JNIEnv* env, jobject jobj, jlong handle,
    jlong jbytes_per_second) {
  reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(handle)->get()->
      SetBytesPerSecond(jbytes_per_second);
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：请求
 *签字：（JJ）V
 **/

void Java_org_rocksdb_RateLimiter_request(
    JNIEnv* env, jobject jobj, jlong handle,
    jlong jbytes) {
  reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(handle)->get()->
      Request(jbytes, rocksdb::Env::IO_TOTAL);
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：GetSingleBurstBytes
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RateLimiter_getSingleBurstBytes(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(handle)->
      get()->GetSingleBurstBytes();
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：gettotalbytesthrough
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RateLimiter_getTotalBytesThrough(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(handle)->
      get()->GetTotalBytesThrough();
}

/*
 *课程：Org_RocksDB_Ratelimiter
 *方法：GetTotalRequests
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RateLimiter_getTotalRequests(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(handle)->
      get()->GetTotalRequests();
}
