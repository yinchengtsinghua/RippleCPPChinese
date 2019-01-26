
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
//调用C++ ROCKSDB:：SSTFILE写入方法
//从Java端。

#include <jni.h>
#include <string>

#include "include/org_rocksdb_SstFileWriter.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/sst_file_writer.h"
#include "rocksjni/portal.h"

/*
 *课程：组织rocksdb文件编写器
 *方法：newsstfilewriter
 *签字：（JJJ）J
 **/

jlong Java_org_rocksdb_SstFileWriter_newSstFileWriter__JJJ(JNIEnv *env, jclass jcls,
                                                      jlong jenvoptions,
                                                      jlong joptions,
                                                      jlong jcomparator) {
  auto *env_options =
      reinterpret_cast<const rocksdb::EnvOptions *>(jenvoptions);
  auto *options = reinterpret_cast<const rocksdb::Options *>(joptions);
  auto *comparator = reinterpret_cast<const rocksdb::Comparator *>(jcomparator);
  rocksdb::SstFileWriter *sst_file_writer =
      new rocksdb::SstFileWriter(*env_options, *options, comparator);
  return reinterpret_cast<jlong>(sst_file_writer);
}

/*
 *课程：组织rocksdb文件编写器
 *方法：newsstfilewriter
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_SstFileWriter_newSstFileWriter__JJ(JNIEnv *env, jclass jcls,
                                                      jlong jenvoptions,
                                                      jlong joptions) {
  auto *env_options =
      reinterpret_cast<const rocksdb::EnvOptions *>(jenvoptions);
  auto *options = reinterpret_cast<const rocksdb::Options *>(joptions);
  rocksdb::SstFileWriter *sst_file_writer =
      new rocksdb::SstFileWriter(*env_options, *options);
  return reinterpret_cast<jlong>(sst_file_writer);
}

/*
 *课程：组织rocksdb文件编写器
 *方法：打开
 *签名：（jljava/lang/string；）v
 **/

void Java_org_rocksdb_SstFileWriter_open(JNIEnv *env, jobject jobj,
                                         jlong jhandle, jstring jfile_path) {
  const char *file_path = env->GetStringUTFChars(jfile_path, nullptr);
  if(file_path == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  rocksdb::Status s =
      reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Open(file_path);
  env->ReleaseStringUTFChars(jfile_path, file_path);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：投入
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_SstFileWriter_put__JJJ(JNIEnv *env, jobject jobj,
                                             jlong jhandle, jlong jkey_handle,
                                             jlong jvalue_handle) {
  auto *key_slice = reinterpret_cast<rocksdb::Slice *>(jkey_handle);
  auto *value_slice = reinterpret_cast<rocksdb::Slice *>(jvalue_handle);
  rocksdb::Status s =
    reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Put(*key_slice,
                                                             *value_slice);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：投入
 *签字：（JJJ）V
 **/

 void Java_org_rocksdb_SstFileWriter_put__J_3B_3B(JNIEnv *env, jobject jobj,
                                                  jlong jhandle, jbyteArray jkey,
                                                  jbyteArray jval) {
  jbyte* key = env->GetByteArrayElements(jkey, nullptr);
  if(key == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  rocksdb::Slice key_slice(
      reinterpret_cast<char*>(key),  env->GetArrayLength(jkey));

  jbyte* value = env->GetByteArrayElements(jval, nullptr);
  if(value == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
    return;
  }
  rocksdb::Slice value_slice(
      reinterpret_cast<char*>(value),  env->GetArrayLength(jval));

  rocksdb::Status s =
  reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Put(key_slice,
                                                           value_slice);

  env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
  env->ReleaseByteArrayElements(jval, value, JNI_ABORT);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：合并
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_SstFileWriter_merge__JJJ(JNIEnv *env, jobject jobj,
                                               jlong jhandle, jlong jkey_handle,
                                               jlong jvalue_handle) {
  auto *key_slice = reinterpret_cast<rocksdb::Slice *>(jkey_handle);
  auto *value_slice = reinterpret_cast<rocksdb::Slice *>(jvalue_handle);
  rocksdb::Status s =
    reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Merge(*key_slice,
                                                               *value_slice);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：合并
 *签字：（J[B[B）V
 **/

void Java_org_rocksdb_SstFileWriter_merge__J_3B_3B(JNIEnv *env, jobject jobj,
                                                   jlong jhandle, jbyteArray jkey,
                                                   jbyteArray jval) {

  jbyte* key = env->GetByteArrayElements(jkey, nullptr);
  if(key == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  rocksdb::Slice key_slice(
      reinterpret_cast<char*>(key),  env->GetArrayLength(jkey));

  jbyte* value = env->GetByteArrayElements(jval, nullptr);
  if(value == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
    return;
  }
  rocksdb::Slice value_slice(
      reinterpret_cast<char*>(value),  env->GetArrayLength(jval));

  rocksdb::Status s =
    reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Merge(key_slice,
                                                               value_slice);

  env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);
  env->ReleaseByteArrayElements(jval, value, JNI_ABORT);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：删除
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_SstFileWriter_delete__J_3B(JNIEnv *env, jobject jobj,
                                               jlong jhandle, jbyteArray jkey) {
  jbyte* key = env->GetByteArrayElements(jkey, nullptr);
  if(key == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  rocksdb::Slice key_slice(
      reinterpret_cast<char*>(key),  env->GetArrayLength(jkey));

  rocksdb::Status s =
    reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Delete(key_slice);

  env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：删除
 *签字：（JJJ）V
 **/

 void Java_org_rocksdb_SstFileWriter_delete__JJ(JNIEnv *env, jobject jobj,
  jlong jhandle, jlong jkey_handle) {
  auto *key_slice = reinterpret_cast<rocksdb::Slice *>(jkey_handle);
  rocksdb::Status s =
    reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Delete(*key_slice);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：完成
 *签名：（j）v
 **/

void Java_org_rocksdb_SstFileWriter_finish(JNIEnv *env, jobject jobj,
                                           jlong jhandle) {
  rocksdb::Status s =
      reinterpret_cast<rocksdb::SstFileWriter *>(jhandle)->Finish();
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：组织rocksdb文件编写器
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_SstFileWriter_disposeInternal(JNIEnv *env, jobject jobj,
                                                    jlong jhandle) {
  delete reinterpret_cast<rocksdb::SstFileWriter *>(jhandle);
}
