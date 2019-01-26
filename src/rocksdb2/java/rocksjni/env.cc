
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
//该文件实现Java和C++之间的“桥接”并启用
//从Java端调用C++ ROCSDB:：Env方法。

#include "include/org_rocksdb_Env.h"
#include "include/org_rocksdb_RocksEnv.h"
#include "include/org_rocksdb_RocksMemEnv.h"
#include "rocksdb/env.h"

/*
 *课程：Org_Rocksdb_env
 *方法：GetDefaultEnvInternal
 *签字：（）J
 **/

jlong Java_org_rocksdb_Env_getDefaultEnvInternal(
    JNIEnv* env, jclass jclazz) {
  return reinterpret_cast<jlong>(rocksdb::Env::Default());
}

/*
 *课程：Org_Rocksdb_env
 *方法：退刀螺纹
 *签字：（JII）V
 **/

void Java_org_rocksdb_Env_setBackgroundThreads(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint num, jint priority) {
  auto* rocks_env = reinterpret_cast<rocksdb::Env*>(jhandle);
  switch (priority) {
    case org_rocksdb_Env_FLUSH_POOL:
      rocks_env->SetBackgroundThreads(num, rocksdb::Env::Priority::LOW);
      break;
    case org_rocksdb_Env_COMPACTION_POOL:
      rocks_env->SetBackgroundThreads(num, rocksdb::Env::Priority::HIGH);
      break;
  }
}

/*
 *课程：Org_Rocksdb_Senv
 *方法：getthreadpoolqueuelen
 *签名：（Ji）I
 **/

jint Java_org_rocksdb_Env_getThreadPoolQueueLen(
    JNIEnv* env, jobject jobj, jlong jhandle, jint pool_id) {
  auto* rocks_env = reinterpret_cast<rocksdb::Env*>(jhandle);
  switch (pool_id) {
    case org_rocksdb_RocksEnv_FLUSH_POOL:
      return rocks_env->GetThreadPoolQueueLen(rocksdb::Env::Priority::LOW);
    case org_rocksdb_RocksEnv_COMPACTION_POOL:
      return rocks_env->GetThreadPoolQueueLen(rocksdb::Env::Priority::HIGH);
  }
  return 0;
}

/*
 *课程：Org_Rocksdb_Rocksmemenv
 *方法：CreateMemenv
 *签字：（）J
 **/

jlong Java_org_rocksdb_RocksMemEnv_createMemEnv(
    JNIEnv* env, jclass jclazz) {
  return reinterpret_cast<jlong>(rocksdb::NewMemEnv(
      rocksdb::Env::Default()));
}

/*
 *课程：Org_Rocksdb_Rocksmemenv
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksMemEnv_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* e = reinterpret_cast<rocksdb::Env*>(jhandle);
  assert(e != nullptr);
  delete e;
}
