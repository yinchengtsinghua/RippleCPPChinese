
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
//调用C++ ROCKSDB:：TTLDB方法。
//从Java端。

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>

#include "include/org_rocksdb_TtlDB.h"
#include "rocksdb/utilities/db_ttl.h"
#include "rocksjni/portal.h"

/*
 *课程：Org_Rocksdb_ttldb
 *方法：打开
 *签名：（jljava/lang/string；iz）j
 **/

jlong Java_org_rocksdb_TtlDB_open(JNIEnv* env,
    jclass jcls, jlong joptions_handle, jstring jdb_path,
    jint jttl, jboolean jread_only) {
  const char* db_path = env->GetStringUTFChars(jdb_path, nullptr);
  if(db_path == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  auto* opt = reinterpret_cast<rocksdb::Options*>(joptions_handle);
  rocksdb::DBWithTTL* db = nullptr;
  rocksdb::Status s = rocksdb::DBWithTTL::Open(*opt, db_path, &db,
      jttl, jread_only);
  env->ReleaseStringUTFChars(jdb_path, db_path);

//由于TTLDB在Java端扩展了ROCKSDB，所以我们可以重用
//这里是Rocksdb门户。
  if (s.ok()) {
    return reinterpret_cast<jlong>(db);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
    return 0;
  }
}

/*
 *课程：Org_Rocksdb_ttldb
 *方法：opencf
 *签名：（jljava/lang/string；[[b[j[iz）[j
 **/

jlongArray
    Java_org_rocksdb_TtlDB_openCF(
    JNIEnv* env, jclass jcls, jlong jopt_handle, jstring jdb_path,
    jobjectArray jcolumn_names, jlongArray jcolumn_options,
    jintArray jttls, jboolean jread_only) {
  const char* db_path = env->GetStringUTFChars(jdb_path, nullptr);
  if(db_path == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  const jsize len_cols = env->GetArrayLength(jcolumn_names);
  jlong* jco = env->GetLongArrayElements(jcolumn_options, nullptr);
  if(jco == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseStringUTFChars(jdb_path, db_path);
    return nullptr;
  }

  std::vector<rocksdb::ColumnFamilyDescriptor> column_families;
  jboolean has_exception = JNI_FALSE;
  rocksdb::JniUtil::byteStrings<std::string>(
    env,
    jcolumn_names,
    [](const char* str_data, const size_t str_len) {
      return std::string(str_data, str_len);
    },
    [&jco, &column_families](size_t idx, std::string cf_name) {
      rocksdb::ColumnFamilyOptions* cf_options =
          reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jco[idx]);
      column_families.push_back(
          rocksdb::ColumnFamilyDescriptor(cf_name, *cf_options));
    },
    &has_exception);

  env->ReleaseLongArrayElements(jcolumn_options, jco, JNI_ABORT);

  if(has_exception == JNI_TRUE) {
//发生异常
    env->ReleaseStringUTFChars(jdb_path, db_path);
    return nullptr;
  }

  std::vector<int32_t> ttl_values;
  jint* jttlv = env->GetIntArrayElements(jttls, nullptr);
  if(jttlv == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseStringUTFChars(jdb_path, db_path);
    return nullptr;
  }
  const jsize len_ttls = env->GetArrayLength(jttls);
  for(jsize i = 0; i < len_ttls; i++) {
    ttl_values.push_back(jttlv[i]);
  }
  env->ReleaseIntArrayElements(jttls, jttlv, JNI_ABORT);

  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jopt_handle);
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DBWithTTL* db = nullptr;
  rocksdb::Status s = rocksdb::DBWithTTL::Open(*opt, db_path, column_families,
      &handles, &db, ttl_values, jread_only);

//我们现在已经完成了db_路径
  env->ReleaseStringUTFChars(jdb_path, db_path);

//检查打开操作是否成功
  if (s.ok()) {
const jsize resultsLen = 1 + len_cols; //DB句柄+列族句柄
    std::unique_ptr<jlong[]> results =
        std::unique_ptr<jlong[]>(new jlong[resultsLen]);
    results[0] = reinterpret_cast<jlong>(db);
    for(int i = 1; i <= len_cols; i++) {
      results[i] = reinterpret_cast<jlong>(handles[i - 1]);
    }

    jlongArray jresults = env->NewLongArray(resultsLen);
    if(jresults == nullptr) {
//引发异常：OutOfMemoryError
      return nullptr;
    }

    env->SetLongArrayRegion(jresults, 0, resultsLen, results.get());
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(jresults);
      return nullptr;
    }

    return jresults;
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
    return NULL;
  }
}

/*
 *课程：Org_Rocksdb_ttldb
 *方法：CreateColumnFamilyWithTTL
 *签名：（jlorg/rocksdb/columnfamilyDescriptor；[bji）j；
 **/

jlong Java_org_rocksdb_TtlDB_createColumnFamilyWithTtl(
    JNIEnv* env, jobject jobj, jlong jdb_handle,
    jbyteArray jcolumn_name, jlong jcolumn_options, jint jttl) {

  jbyte* cfname = env->GetByteArrayElements(jcolumn_name, nullptr);
  if(cfname == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }
  const jsize len = env->GetArrayLength(jcolumn_name);

  auto* cfOptions =
      reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jcolumn_options);

  auto* db_handle = reinterpret_cast<rocksdb::DBWithTTL*>(jdb_handle);
  rocksdb::ColumnFamilyHandle* handle;
  rocksdb::Status s = db_handle->CreateColumnFamilyWithTtl(
      *cfOptions, std::string(reinterpret_cast<char *>(cfname),
          len), &handle, jttl);

  env->ReleaseByteArrayElements(jcolumn_name, cfname, 0);

  if (s.ok()) {
    return reinterpret_cast<jlong>(handle);
  }
  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return 0;
}
