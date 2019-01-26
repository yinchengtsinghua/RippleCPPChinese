
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
//从Java端调用C++ROCKSDB::WrreButCuffDe指标方法。

#include "include/org_rocksdb_WBWIRocksIterator.h"
#include "include/org_rocksdb_WriteBatchWithIndex.h"
#include "rocksdb/comparator.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "rocksjni/portal.h"

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：newwritebatchwithindex
 *签字：（）J
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_newWriteBatchWithIndex__(
    JNIEnv* env, jclass jcls) {
  auto* wbwi = new rocksdb::WriteBatchWithIndex();
  return reinterpret_cast<jlong>(wbwi);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：newwritebatchwithindex
 *签字：（Z）J
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_newWriteBatchWithIndex__Z(
    JNIEnv* env, jclass jcls, jboolean joverwrite_key) {
  auto* wbwi =
      new rocksdb::WriteBatchWithIndex(rocksdb::BytewiseComparator(), 0,
          static_cast<bool>(joverwrite_key));
  return reinterpret_cast<jlong>(wbwi);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：newwritebatchwithindex
 *签字：（Jiz）J
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_newWriteBatchWithIndex__JIZ(
    JNIEnv* env, jclass jcls, jlong jfallback_index_comparator_handle,
    jint jreserved_bytes, jboolean joverwrite_key) {
  auto* wbwi =
      new rocksdb::WriteBatchWithIndex(
          reinterpret_cast<rocksdb::Comparator*>(jfallback_index_comparator_handle),
          static_cast<size_t>(jreserved_bytes), static_cast<bool>(joverwrite_key));
  return reinterpret_cast<jlong>(wbwi);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：count0
 *签字：（j）I
 **/

jint Java_org_rocksdb_WriteBatchWithIndex_count0(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);

  return static_cast<jint>(wbwi->GetWriteBatch()->Count());
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：投入
 *签字：（J[BI[BI）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_put__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len, jbyteArray jentry_value, jint jentry_value_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto put = [&wbwi] (rocksdb::Slice key, rocksdb::Slice value) {
    wbwi->Put(key, value);
  };
  rocksdb::JniUtil::kv_op(put, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：投入
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_put__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len, jbyteArray jentry_value, jint jentry_value_len,
    jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto put = [&wbwi, &cf_handle] (rocksdb::Slice key, rocksdb::Slice value) {
    wbwi->Put(cf_handle, key, value);
  };
  rocksdb::JniUtil::kv_op(put, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：合并
 *签字：（J[BI[BI）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_merge__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len, jbyteArray jentry_value, jint jentry_value_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto merge = [&wbwi] (rocksdb::Slice key, rocksdb::Slice value) {
    wbwi->Merge(key, value);
  };
  rocksdb::JniUtil::kv_op(merge, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：合并
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_merge__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len, jbyteArray jentry_value, jint jentry_value_len,
    jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto merge = [&wbwi, &cf_handle] (rocksdb::Slice key, rocksdb::Slice value) {
    wbwi->Merge(cf_handle, key, value);
  };
  rocksdb::JniUtil::kv_op(merge, env, jobj, jkey, jkey_len, jentry_value,
      jentry_value_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：删除
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_remove__J_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto remove = [&wbwi] (rocksdb::Slice key) {
    wbwi->Delete(key);
  };
  rocksdb::JniUtil::k_op(remove, env, jobj, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：删除
 *签字：（J[BIJ）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_remove__J_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jkey,
    jint jkey_len, jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto remove = [&wbwi, &cf_handle] (rocksdb::Slice key) {
    wbwi->Delete(cf_handle, key);
  };
  rocksdb::JniUtil::k_op(remove, env, jobj, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：删除范围
 *签字：（J[BI[BI）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_deleteRange__J_3BI_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jbegin_key,
    jint jbegin_key_len, jbyteArray jend_key, jint jend_key_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto deleteRange = [&wbwi](rocksdb::Slice beginKey, rocksdb::Slice endKey) {
    wbwi->DeleteRange(beginKey, endKey);
  };
  rocksdb::JniUtil::kv_op(deleteRange, env, jobj, jbegin_key, jbegin_key_len,
                          jend_key, jend_key_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：删除范围
 *签字：（J[BI[BIJ）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_deleteRange__J_3BI_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jbegin_key,
    jint jbegin_key_len, jbyteArray jend_key, jint jend_key_len,
    jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  assert(cf_handle != nullptr);
  auto deleteRange = [&wbwi, &cf_handle](rocksdb::Slice beginKey,
                                         rocksdb::Slice endKey) {
    wbwi->DeleteRange(cf_handle, beginKey, endKey);
  };
  rocksdb::JniUtil::kv_op(deleteRange, env, jobj, jbegin_key, jbegin_key_len,
                          jend_key, jend_key_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：putlogdata
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_WriteBatchWithIndex_putLogData(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jbyteArray jblob,
    jint jblob_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);
  auto putLogData = [&wbwi] (rocksdb::Slice blob) {
    wbwi->PutLogData(blob);
  };
  rocksdb::JniUtil::k_op(putLogData, env, jobj, jblob, jblob_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：清除
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatchWithIndex_clear0(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);

  wbwi->Clear();
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：设置保存点0
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatchWithIndex_setSavePoint0(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);

  wbwi->SetSavePoint();
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：RollbackToSavePoint0
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatchWithIndex_rollbackToSavePoint0(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  assert(wbwi != nullptr);

  auto s = wbwi->RollbackToSavePoint();

  if (s.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：迭代器0
 *签字：（j）j
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_iterator0(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* wbwi_iterator = wbwi->NewIterator();
  return reinterpret_cast<jlong>(wbwi_iterator);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：迭代器1
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_iterator1(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  auto* wbwi_iterator = wbwi->NewIterator(cf_handle);
  return reinterpret_cast<jlong>(wbwi_iterator);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：IteratorWithBase
 *签字：（JJJ）J
 **/

jlong Java_org_rocksdb_WriteBatchWithIndex_iteratorWithBase(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jcf_handle,
    jlong jbi_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  auto* base_iterator = reinterpret_cast<rocksdb::Iterator*>(jbi_handle);
  auto* iterator = wbwi->NewIteratorWithBase(cf_handle, base_iterator);
  return reinterpret_cast<jlong>(iterator);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：GetFromBatch
 *签字：（JJ[BI）[B
 **/

jbyteArray JNICALL Java_org_rocksdb_WriteBatchWithIndex_getFromBatch__JJ_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jdbopt_handle,
    jbyteArray jkey, jint jkey_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* dbopt = reinterpret_cast<rocksdb::DBOptions*>(jdbopt_handle);

  auto getter = [&wbwi, &dbopt](const rocksdb::Slice& key, std::string* value) {
    return wbwi->GetFromBatch(*dbopt, key, value);
  };

  return rocksdb::JniUtil::v_op(getter, env, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：GetFromBatch
 *签字：（JJ[BIJ）[B]
 **/

jbyteArray Java_org_rocksdb_WriteBatchWithIndex_getFromBatch__JJ_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jdbopt_handle,
    jbyteArray jkey, jint jkey_len, jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* dbopt = reinterpret_cast<rocksdb::DBOptions*>(jdbopt_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);

  auto getter =
      [&wbwi, &cf_handle, &dbopt](const rocksdb::Slice& key,
                                  std::string* value) {
        return wbwi->GetFromBatch(cf_handle, *dbopt, key, value);
      };

  return rocksdb::JniUtil::v_op(getter, env, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：GetFromBatchAndDB
 *签字：（JJJ[BI）[B]
 **/

jbyteArray Java_org_rocksdb_WriteBatchWithIndex_getFromBatchAndDB__JJJ_3BI(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jdb_handle,
    jlong jreadopt_handle, jbyteArray jkey, jint jkey_len) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* readopt = reinterpret_cast<rocksdb::ReadOptions*>(jreadopt_handle);

  auto getter =
      [&wbwi, &db, &readopt](const rocksdb::Slice& key, std::string* value) {
        return wbwi->GetFromBatchAndDB(db, *readopt, key, value);
      };

  return rocksdb::JniUtil::v_op(getter, env, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：GetFromBatchAndDB
 *签字：（JJJ[BIJ）[B]
 **/

jbyteArray Java_org_rocksdb_WriteBatchWithIndex_getFromBatchAndDB__JJJ_3BIJ(
    JNIEnv* env, jobject jobj, jlong jwbwi_handle, jlong jdb_handle,
    jlong jreadopt_handle, jbyteArray jkey, jint jkey_len, jlong jcf_handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* readopt = reinterpret_cast<rocksdb::ReadOptions*>(jreadopt_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);

  auto getter =
      [&wbwi, &db, &cf_handle, &readopt](const rocksdb::Slice& key,
                                         std::string* value) {
        return wbwi->GetFromBatchAndDB(db, *readopt, cf_handle, key, value);
      };

  return rocksdb::JniUtil::v_op(getter, env, jkey, jkey_len);
}

/*
 *类：org_rocksdb_writebatchwithindex
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_WriteBatchWithIndex_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(handle);
  assert(wbwi != nullptr);
  delete wbwi;
}

/*WBWIRockSiteror下方*/

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::WBWIIterator*>(handle);
  assert(it != nullptr);
  delete it;
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：isvalid0
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_WBWIRocksIterator_isValid0(
    JNIEnv* env, jobject jobj, jlong handle) {
  return reinterpret_cast<rocksdb::WBWIIterator*>(handle)->Valid();
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：seektofirst0
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_seekToFirst0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::WBWIIterator*>(handle)->SeekToFirst();
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：seektolast0
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_seekToLast0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::WBWIIterator*>(handle)->SeekToLast();
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：next0
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_next0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::WBWIIterator*>(handle)->Next();
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：prev0
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_prev0(
    JNIEnv* env, jobject jobj, jlong handle) {
  reinterpret_cast<rocksdb::WBWIIterator*>(handle)->Prev();
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：SEEK0
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_WBWIRocksIterator_seek0(
    JNIEnv* env, jobject jobj, jlong handle, jbyteArray jtarget,
    jint jtarget_len) {
  auto* it = reinterpret_cast<rocksdb::WBWIIterator*>(handle);
  jbyte* target = env->GetByteArrayElements(jtarget, nullptr);
  if(target == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  rocksdb::Slice target_slice(
      reinterpret_cast<char*>(target), jtarget_len);

  it->Seek(target_slice);

  env->ReleaseByteArrayElements(jtarget, target, JNI_ABORT);
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：状态0
 *签名：（j）v
 **/

void Java_org_rocksdb_WBWIRocksIterator_status0(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::WBWIIterator*>(handle);
  rocksdb::Status s = it->status();

  if (s.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *课程：Org_RocksDB_WbwiRockSiterator
 *方法：输入1
 *签字：（j）[j
 **/

jlongArray Java_org_rocksdb_WBWIRocksIterator_entry1(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* it = reinterpret_cast<rocksdb::WBWIIterator*>(handle);
  const rocksdb::WriteEntry& we = it->Entry();

  jlong results[3];

//设置写入项的类型
  switch (we.type) {
    case rocksdb::kPutRecord:
      results[0] = 0x1;
      break;

    case rocksdb::kMergeRecord:
      results[0] = 0x2;
      break;

    case rocksdb::kDeleteRecord:
      results[0] = 0x4;
      break;

    case rocksdb::kLogDataRecord:
      results[0] = 0x8;
      break;

    default:
      results[0] = 0x0;
  }

//关键字切片和值切片将由org.rocksdb.directslice close释放

  auto* key_slice = new rocksdb::Slice(we.key.data(), we.key.size());
  results[1] = reinterpret_cast<jlong>(key_slice);
  if (we.type == rocksdb::kDeleteRecord
      || we.type == rocksdb::kLogDataRecord) {
//如果没有可用的值，则将值切片的本机句柄设置为空。
    results[2] = 0;
  } else {
    auto* value_slice = new rocksdb::Slice(we.value.data(), we.value.size());
    results[2] = reinterpret_cast<jlong>(value_slice);
  }

  jlongArray jresults = env->NewLongArray(3);
  if(jresults == nullptr) {
//引发异常：OutOfMemoryError
    if(results[2] != 0) {
      auto* value_slice = reinterpret_cast<rocksdb::Slice*>(results[2]);
      delete value_slice;
    }
    delete key_slice;
    return nullptr;
  }

  env->SetLongArrayRegion(jresults, 0, 3, results);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    env->DeleteLocalRef(jresults);
    if(results[2] != 0) {
      auto* value_slice = reinterpret_cast<rocksdb::Slice*>(results[2]);
      delete value_slice;
    }
    delete key_slice;
    return nullptr;
  }

  return jresults;
}
