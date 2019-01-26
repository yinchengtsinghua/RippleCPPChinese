
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
//从Java端调用C++ROCSDB:：WreEffTealPosits方法。
#include <memory>

#include "db/memtable.h"
#include "db/write_batch_internal.h"
#include "include/org_rocksdb_WriteBatch.h"
#include "include/org_rocksdb_WriteBatch_Handler.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/status.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_buffer_manager.h"
#include "rocksjni/portal.h"
#include "rocksjni/writebatchhandlerjnicallback.h"
#include "table/scoped_arena_iterator.h"
#include "util/logging.h"

/*
 *类别：org_rocksdb_writebatch
 *方法：newwritebatch
 *签名：（i）J
 **/

jlong Java_org_rocksdb_WriteBatch_newWriteBatch(
    JNIEnv* env, jclass jcls, jint jreserved_bytes) {
  auto* wb = new rocksdb::WriteBatch(static_cast<size_t>(jreserved_bytes));
  return reinterpret_cast<jlong>(wb);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：count0
 *签字：（j）I
 **/

jint Java_org_rocksdb_WriteBatch_count0(JNIEnv* env, jobject jobj,
    jlong jwb_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  return static_cast<jint>(wb->Count());
}

/*
 *类别：org_rocksdb_writebatch
 *方法：Clear0
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatch_clear0(JNIEnv* env, jobject jobj,
    jlong jwb_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  wb->Clear();
}

/*
 *类别：org_rocksdb_writebatch
 *方法：设置保存点0
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatch_setSavePoint0(
    JNIEnv* env, jobject jobj, jlong jwb_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  wb->SetSavePoint();
}

/*
 *类别：org_rocksdb_writebatch
 *方法：RollbackToSavePoint0
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatch_rollbackToSavePoint0(
    JNIEnv* env, jobject jobj, jlong jwb_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  auto s = wb->RollbackToSavePoint();

  if (s.ok()) {
    return;
  }
  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：投入
 *签字：（J[BI[BI）V
 **/

void Java_org_rocksdb_WriteBatch_put__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len,
    jbyteArray jentry_value, jint jentry_value_len) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto put = [&wb] (rocksdb::Slice key, rocksdb::Slice value) {
    wb->Put(key, value);
  };
  rocksdb::JniUtil::kv_op(put, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：投入
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatch_put__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len,
    jbyteArray jentry_value, jint jentry_value_len, jlong jcf_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto put = [&wb, &cf_handle] (rocksdb::Slice key, rocksdb::Slice value) {
    wb->Put(cf_handle, key, value);
  };
  rocksdb::JniUtil::kv_op(put, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：合并
 *签字：（J[BI[BI）V
 **/

void Java_org_rocksdb_WriteBatch_merge__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len,
    jbyteArray jentry_value, jint jentry_value_len) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto merge = [&wb] (rocksdb::Slice key, rocksdb::Slice value) {
    wb->Merge(key, value);
  };
  rocksdb::JniUtil::kv_op(merge, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：合并
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatch_merge__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len,
    jbyteArray jentry_value, jint jentry_value_len, jlong jcf_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto merge = [&wb, &cf_handle] (rocksdb::Slice key, rocksdb::Slice value) {
    wb->Merge(cf_handle, key, value);
  };
  rocksdb::JniUtil::kv_op(merge, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：删除
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_WriteBatch_remove__J_3BI(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto remove = [&wb] (rocksdb::Slice key) {
    wb->Delete(key);
  };
  rocksdb::JniUtil::k_op(remove, env, jobj, jkey, jkey_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：删除
 *签字：（J[BIJ）V
 **/

void Java_org_rocksdb_WriteBatch_remove__J_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwb_handle,
    jbyteArray jkey, jint jkey_len, jlong jcf_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto remove = [&wb, &cf_handle] (rocksdb::Slice key) {
    wb->Delete(cf_handle, key);
  };
  rocksdb::JniUtil::k_op(remove, env, jobj, jkey, jkey_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：删除范围
 *签字：（J[BI[BI）V
 **/

JNIEXPORT void JNICALL Java_org_rocksdb_WriteBatch_deleteRange__J_3BI_3BI(
    JNIEnv*, jobject, jlong, jbyteArray, jint, jbyteArray, jint);

void Java_org_rocksdb_WriteBatch_deleteRange__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwb_handle, jbyteArray jbegin_key,
    jint jbegin_key_len, jbyteArray jend_key, jint jend_key_len) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto deleteRange = [&wb](rocksdb::Slice beginKey, rocksdb::Slice endKey) {
    wb->DeleteRange(beginKey, endKey);
  };
  rocksdb::JniUtil::kv_op(deleteRange, env, jobj, jbegin_key, jbegin_key_len,
                          jend_key, jend_key_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：删除范围
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatch_deleteRange__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwb_handle, jbyteArray jbegin_key,
    jint jbegin_key_len, jbyteArray jend_key, jint jend_key_len,
    jlong jcf_handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto deleteRange = [&wb, &cf_handle](rocksdb::Slice beginKey,
                                       rocksdb::Slice endKey) {
    wb->DeleteRange(cf_handle, beginKey, endKey);
  };
  rocksdb::JniUtil::kv_op(deleteRange, env, jobj, jbegin_key, jbegin_key_len,
                          jend_key, jend_key_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：putlogdata
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_WriteBatch_putLogData(
    JNIEnv* env, jobject jobj, jlong jwb_handle, jbyteArray jblob,
    jint jblob_len) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);
  auto putLogData = [&wb] (rocksdb::Slice blob) {
    wb->PutLogData(blob);
  };
  rocksdb::JniUtil::k_op(putLogData, env, jobj, jblob, jblob_len);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：迭代
 *签字：（JJ）V
 **/

void Java_org_rocksdb_WriteBatch_iterate(
    JNIEnv* env , jobject jobj, jlong jwb_handle, jlong handlerHandle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);
  assert(wb != nullptr);

  rocksdb::Status s = wb->Iterate(
    reinterpret_cast<rocksdb::WriteBatchHandlerJniCallback*>(handlerHandle));

  if (s.ok()) {
    return;
  }
  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *类别：org_rocksdb_writebatch
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatch_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(handle);
  assert(wb != nullptr);
  delete wb;
}

/*
 *类：org-rocksdb-writebatch-handler
 *方法：createnewhandler0
 *签字：（）J
 **/

jlong Java_org_rocksdb_WriteBatch_00024Handler_createNewHandler0(
    JNIEnv* env, jobject jobj) {
  auto* wbjnic = new rocksdb::WriteBatchHandlerJniCallback(env, jobj);
  return reinterpret_cast<jlong>(wbjnic);
}

/*
 *类：org-rocksdb-writebatch-handler
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatch_00024Handler_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* wbjnic =
      reinterpret_cast<rocksdb::WriteBatchHandlerJniCallback*>(handle);
  assert(wbjnic != nullptr);
  delete wbjnic;
}
