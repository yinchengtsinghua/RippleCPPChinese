
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

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/org_rocksdb_TransactionLogIterator.h"
#include "rocksdb/transaction_log.h"
#include "rocksjni/portal.h"

/*
 *类别：Org_RocksDB_TransactionLogiterator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_TransactionLogIterator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  delete reinterpret_cast<rocksdb::TransactionLogIterator*>(handle);
}

/*
 *类别：Org_RocksDB_TransactionLogiterator
 *方法：有效
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_TransactionLogIterator_isValid(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<rocksdb::TransactionLogIterator*>(handle)->Valid();
}

/*
 *类别：Org_RocksDB_TransactionLogiterator
 *方法：下一步
 *签名：（j）v
 **/

void Java_org_rocksdb_TransactionLogIterator_next(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::TransactionLogIterator*>(handle)->Next();
}

/*
 *类别：Org_RocksDB_TransactionLogiterator
 *方法：状态
 *签名：（j）v
 **/

void Java_org_rocksdb_TransactionLogIterator_status(
    JNIEnv* env, jobject jobj, jlong handle) {
  rocksdb::Status s = reinterpret_cast<
      rocksdb::TransactionLogIterator*>(handle)->status();
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：Org_RocksDB_TransactionLogiterator
 *方法：getbatch
 *签名：（j）lorg/rocksdb/transactionlogiterator$batchresult
 **/

jobject Java_org_rocksdb_TransactionLogIterator_getBatch(
    JNIEnv* env, jobject jobj, jlong handle) {
  rocksdb::BatchResult batch_result =
      reinterpret_cast<rocksdb::TransactionLogIterator*>(handle)->GetBatch();
  return rocksdb::BatchResultJni::construct(env, batch_result);
}
