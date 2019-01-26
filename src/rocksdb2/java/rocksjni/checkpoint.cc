
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
//调用C++ ROCKSDB:：Java端的检查点方法。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>

#include "include/org_rocksdb_Checkpoint.h"
#include "rocksjni/portal.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/checkpoint.h"
/*
 *课程：组织rocksdb检查点
 *方法：NewCheckpoint
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Checkpoint_newCheckpoint(JNIEnv* env,
    jclass jclazz, jlong jdb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb::Checkpoint* checkpoint;
  rocksdb::Checkpoint::Create(db, &checkpoint);
  return reinterpret_cast<jlong>(checkpoint);
}

/*
 *课程：组织rocksdb检查点
 *方法：处置
 *签名：（j）v
 **/

void Java_org_rocksdb_Checkpoint_disposeInternal(JNIEnv* env, jobject jobj,
    jlong jhandle) {
  auto* checkpoint = reinterpret_cast<rocksdb::Checkpoint*>(jhandle);
  assert(checkpoint != nullptr);
  delete checkpoint;
}

/*
 *课程：组织rocksdb检查点
 *方法：创建检查点
 *签名：（jljava/lang/string；）v
 **/

void Java_org_rocksdb_Checkpoint_createCheckpoint(
    JNIEnv* env, jobject jobj, jlong jcheckpoint_handle,
    jstring jcheckpoint_path) {
  const char* checkpoint_path = env->GetStringUTFChars(
      jcheckpoint_path, 0);
  if(checkpoint_path == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  auto* checkpoint = reinterpret_cast<rocksdb::Checkpoint*>(
      jcheckpoint_handle);
  rocksdb::Status s = checkpoint->CreateCheckpoint(
      checkpoint_path);
  
  env->ReleaseStringUTFChars(jcheckpoint_path, checkpoint_path);
  
  if (!s.ok()) {
      rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}
