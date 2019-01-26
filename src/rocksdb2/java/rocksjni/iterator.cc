
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
//调用C++ ROCKSDB:：Java端的迭代器方法。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>

#include "include/org_rocksdb_RocksIterator.h"
#include "rocksjni/portal.h"
#include "rocksdb/iterator.h"

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::Iterator*>(handle);
  assert(it != nullptr);
  delete it;
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：isvalid0
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_RocksIterator_isValid0(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<rocksdb::Iterator*>(handle)->Valid();
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：seektofirst0
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_seekToFirst0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::Iterator*>(handle)->SeekToFirst();
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：seektolast0
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_seekToLast0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::Iterator*>(handle)->SeekToLast();
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：next0
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_next0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::Iterator*>(handle)->Next();
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：prev0
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_prev0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::Iterator*>(handle)->Prev();
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：SEEK0
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_RocksIterator_seek0(
    JNIEnv* env, jobject jobj, jlong handle,
    jbyteArray jtarget, jint jtarget_len) {
  jbyte* target = env->GetByteArrayElements(jtarget, nullptr);
  if(target == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  rocksdb::Slice target_slice(
      reinterpret_cast<char*>(target), jtarget_len);

  auto* it = reinterpret_cast<rocksdb::Iterator*>(handle);
  it->Seek(target_slice);

  env->ReleaseByteArrayElements(jtarget, target, JNI_ABORT);
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：状态0
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksIterator_status0(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::Iterator*>(handle);
  rocksdb::Status s = it->status();

  if (s.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：键0
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_RocksIterator_key0(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::Iterator*>(handle);
  rocksdb::Slice key_slice = it->key();

  jbyteArray jkey = env->NewByteArray(static_cast<jsize>(key_slice.size()));
  if(jkey == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  env->SetByteArrayRegion(jkey, 0, static_cast<jsize>(key_slice.size()),
                          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(key_slice.data())));
  return jkey;
}

/*
 *课程：Org_RocksDB_RockSiterator
 *方法：值0
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_RocksIterator_value0(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::Iterator*>(handle);
  rocksdb::Slice value_slice = it->value();

  jbyteArray jkeyValue =
      env->NewByteArray(static_cast<jsize>(value_slice.size()));
  if(jkeyValue == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  env->SetByteArrayRegion(jkeyValue, 0, static_cast<jsize>(value_slice.size()),
                          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(value_slice.data())));
  return jkeyValue;
}
