
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
//从Java端调用C++ ROCSDB:：DB方法。

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "include/org_rocksdb_RocksDB.h"
#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/types.h"
#include "rocksjni/portal.h"

#ifdef min
#undef min
#endif

//////////////////////////////////////////////////
//RocksDB:：DB：：打开
jlong rocksdb_open_helper(JNIEnv* env, jlong jopt_handle, jstring jdb_path,
    std::function<rocksdb::Status(
      const rocksdb::Options&, const std::string&, rocksdb::DB**)> open_fn
    ) {
  const char* db_path = env->GetStringUTFChars(jdb_path, nullptr);
  if(db_path == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  auto* opt = reinterpret_cast<rocksdb::Options*>(jopt_handle);
  rocksdb::DB* db = nullptr;
  rocksdb::Status s = open_fn(*opt, db_path, &db);

  env->ReleaseStringUTFChars(jdb_path, db_path);

  if (s.ok()) {
    return reinterpret_cast<jlong>(db);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
    return 0;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：打开
 *签名：（jljava/lang/string；）j
 **/

jlong Java_org_rocksdb_RocksDB_open__JLjava_lang_String_2(
    JNIEnv* env, jclass jcls, jlong jopt_handle, jstring jdb_path) {
  return rocksdb_open_helper(env, jopt_handle, jdb_path,
    (rocksdb::Status(*)
      (const rocksdb::Options&, const std::string&, rocksdb::DB**)
    )&rocksdb::DB::Open
  );
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：开放式
 *签名：（jljava/lang/string；）j
 **/

jlong Java_org_rocksdb_RocksDB_openROnly__JLjava_lang_String_2(
    JNIEnv* env, jclass jcls, jlong jopt_handle, jstring jdb_path) {
  return rocksdb_open_helper(env, jopt_handle, jdb_path, [](
      const rocksdb::Options& options,
      const std::string& db_path, rocksdb::DB** db) {
    return rocksdb::DB::OpenForReadOnly(options, db_path, db);
  });
}

jlongArray rocksdb_open_helper(JNIEnv* env, jlong jopt_handle,
    jstring jdb_path, jobjectArray jcolumn_names, jlongArray jcolumn_options,
    std::function<rocksdb::Status(
      const rocksdb::DBOptions&, const std::string&,
      const std::vector<rocksdb::ColumnFamilyDescriptor>&,
      std::vector<rocksdb::ColumnFamilyHandle*>*,
      rocksdb::DB**)> open_fn
    ) {
  const char* db_path = env->GetStringUTFChars(jdb_path, nullptr);
  if(db_path == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
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

  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jopt_handle);
  std::vector<rocksdb::ColumnFamilyHandle*> handles;
  rocksdb::DB* db = nullptr;
  rocksdb::Status s = open_fn(*opt, db_path, column_families,
      &handles, &db);

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
    return nullptr;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：开放式
 *签名：（jljava/lang/string；[[b[j）[j
 **/

jlongArray Java_org_rocksdb_RocksDB_openROnly__JLjava_lang_String_2_3_3B_3J(
    JNIEnv* env, jclass jcls, jlong jopt_handle, jstring jdb_path,
    jobjectArray jcolumn_names, jlongArray jcolumn_options) {
  return rocksdb_open_helper(env, jopt_handle, jdb_path, jcolumn_names,
    jcolumn_options, [](
        const rocksdb::DBOptions& options, const std::string& db_path,
        const std::vector<rocksdb::ColumnFamilyDescriptor>& column_families,
        std::vector<rocksdb::ColumnFamilyHandle*>* handles, rocksdb::DB** db) {
      return rocksdb::DB::OpenForReadOnly(options, db_path, column_families,
        handles, db);
  });
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：打开
 *签名：（jljava/lang/string；[[b[j）[j
 **/

jlongArray Java_org_rocksdb_RocksDB_open__JLjava_lang_String_2_3_3B_3J(
    JNIEnv* env, jclass jcls, jlong jopt_handle, jstring jdb_path,
    jobjectArray jcolumn_names, jlongArray jcolumn_options) {
  return rocksdb_open_helper(env, jopt_handle, jdb_path, jcolumn_names,
    jcolumn_options, (rocksdb::Status(*)
      (const rocksdb::DBOptions&, const std::string&,
        const std::vector<rocksdb::ColumnFamilyDescriptor>&,
        std::vector<rocksdb::ColumnFamilyHandle*>*, rocksdb::DB**)
      )&rocksdb::DB::Open
    );
}

//////////////////////////////////////////////////
//RocksDB:：DB:：ListColumnFamilies系列

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：ListColumnFamilies
 *签名：（jljava/lang/string；）[[b
 **/

jobjectArray Java_org_rocksdb_RocksDB_listColumnFamilies(
    JNIEnv* env, jclass jclazz, jlong jopt_handle, jstring jdb_path) {
  std::vector<std::string> column_family_names;
  const char* db_path = env->GetStringUTFChars(jdb_path, nullptr);
  if(db_path == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }

  auto* opt = reinterpret_cast<rocksdb::Options*>(jopt_handle);
  rocksdb::Status s = rocksdb::DB::ListColumnFamilies(*opt, db_path,
      &column_family_names);

  env->ReleaseStringUTFChars(jdb_path, db_path);

  jobjectArray jcolumn_family_names =
      rocksdb::JniUtil::stringsBytes(env, column_family_names);

  return jcolumn_family_names;
}

//////////////////////////////////////////////////
//RocksDB：：数据库：：放置

/*
 *如果TRUE成功，则返回true，如果抛出Java异常，则返回false
 **/

bool rocksdb_put_helper(JNIEnv* env, rocksdb::DB* db,
                        const rocksdb::WriteOptions& write_options,
                        rocksdb::ColumnFamilyHandle* cf_handle, jbyteArray jkey,
                        jint jkey_off, jint jkey_len, jbyteArray jval,
                        jint jval_off, jint jval_len) {
  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] key;
    return false;
  }

  jbyte* value = new jbyte[jval_len];
  env->GetByteArrayRegion(jval, jval_off, jval_len, value);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] value;
    delete [] key;
    return false;
  }

  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);
  rocksdb::Slice value_slice(reinterpret_cast<char*>(value), jval_len);

  rocksdb::Status s;
  if (cf_handle != nullptr) {
    s = db->Put(write_options, cf_handle, key_slice, value_slice);
  } else {
//向后兼容性
    s = db->Put(write_options, key_slice, value_slice);
  }

//清理
  delete [] value;
  delete [] key;

  if (s.ok()) {
    return true;
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
    return false;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：投入
 *签字：（J[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_put__J_3BII_3BII(JNIEnv* env, jobject jdb,
                                               jlong jdb_handle,
                                               jbyteArray jkey, jint jkey_off,
                                               jint jkey_len, jbyteArray jval,
                                               jint jval_off, jint jval_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();

  rocksdb_put_helper(env, db, default_write_options, nullptr, jkey, jkey_off,
                     jkey_len, jval, jval_off, jval_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：投入
 *签字：（J[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_put__J_3BII_3BIIJ(JNIEnv* env, jobject jdb,
                                                jlong jdb_handle,
                                                jbyteArray jkey, jint jkey_off,
                                                jint jkey_len, jbyteArray jval,
                                                jint jval_off, jint jval_len,
                                                jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_put_helper(env, db, default_write_options, cf_handle, jkey,
                       jkey_off, jkey_len, jval, jval_off, jval_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：投入
 *签字：（JJ[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_put__JJ_3BII_3BII(JNIEnv* env, jobject jdb,
                                                jlong jdb_handle,
                                                jlong jwrite_options_handle,
                                                jbyteArray jkey, jint jkey_off,
                                                jint jkey_len, jbyteArray jval,
                                                jint jval_off, jint jval_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options = reinterpret_cast<rocksdb::WriteOptions*>(
      jwrite_options_handle);

  rocksdb_put_helper(env, db, *write_options, nullptr, jkey, jkey_off, jkey_len,
                     jval, jval_off, jval_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：投入
 *签字：（JJ[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_put__JJ_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jwrite_options_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jbyteArray jval,
    jint jval_off, jint jval_len, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options = reinterpret_cast<rocksdb::WriteOptions*>(
      jwrite_options_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_put_helper(env, db, *write_options, cf_handle, jkey, jkey_off,
                       jkey_len, jval, jval_off, jval_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

//////////////////////////////////////////////////
//rocksdb:：db：：写入
/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：写0
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_RocksDB_write0(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options_handle, jlong jwb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options = reinterpret_cast<rocksdb::WriteOptions*>(
      jwrite_options_handle);
  auto* wb = reinterpret_cast<rocksdb::WriteBatch*>(jwb_handle);

  rocksdb::Status s = db->Write(*write_options, wb);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：写1
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_RocksDB_write1(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options_handle, jlong jwbwi_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options = reinterpret_cast<rocksdb::WriteOptions*>(
      jwrite_options_handle);
  auto* wbwi = reinterpret_cast<rocksdb::WriteBatchWithIndex*>(jwbwi_handle);
  auto* wb = wbwi->GetWriteBatch();

  rocksdb::Status s = db->Write(*write_options, wb);

  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：keymayexist
jboolean key_may_exist_helper(JNIEnv* env, rocksdb::DB* db,
    const rocksdb::ReadOptions& read_opt,
    rocksdb::ColumnFamilyHandle* cf_handle, jbyteArray jkey, jint jkey_off,
    jint jkey_len, jobject jstring_builder, bool* has_exception) {

  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] key;
    *has_exception = true;
    return false;
  }

  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

  std::string value;
  bool value_found = false;
  bool keyMayExist;
  if (cf_handle != nullptr) {
    keyMayExist = db->KeyMayExist(read_opt, cf_handle, key_slice,
        &value, &value_found);
  } else {
    keyMayExist = db->KeyMayExist(read_opt, key_slice,
        &value, &value_found);
  }

//清理
  delete [] key;

//提取价值
  if (value_found && !value.empty()) {
    jobject jresult_string_builder =
        rocksdb::StringBuilderJni::append(env, jstring_builder,
            value.c_str());
    if(jresult_string_builder == nullptr) {
      *has_exception = true;
      return false;
    }
  }

  *has_exception = false;
  return static_cast<jboolean>(keyMayExist);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：keymayexist
 *签名：（j[biiljava/lang/stringbuilder；）z
 **/

jboolean Java_org_rocksdb_RocksDB_keyMayExist__J_3BIILjava_lang_StringBuilder_2(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jkey, jint jkey_off,
    jint jkey_len, jobject jstring_builder) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  bool has_exception = false;
  return key_may_exist_helper(env, db, rocksdb::ReadOptions(),
      nullptr, jkey, jkey_off, jkey_len, jstring_builder, &has_exception);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：keymayexist
 *签名：（j[biijljava/lang/StringBuilder；）z
 **/

jboolean Java_org_rocksdb_RocksDB_keyMayExist__J_3BIIJLjava_lang_StringBuilder_2(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jkey, jint jkey_off,
    jint jkey_len, jlong jcf_handle, jobject jstring_builder) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(
      jcf_handle);
  if (cf_handle != nullptr) {
    bool has_exception = false;
    return key_may_exist_helper(env, db, rocksdb::ReadOptions(),
        cf_handle, jkey, jkey_off, jkey_len, jstring_builder, &has_exception);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
    return true;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：keymayexist
 *签名：（jj[biiljava/lang/StringBuilder；）Z
 **/

jboolean Java_org_rocksdb_RocksDB_keyMayExist__JJ_3BIILjava_lang_StringBuilder_2(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jread_options_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jobject jstring_builder) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto& read_options = *reinterpret_cast<rocksdb::ReadOptions*>(
      jread_options_handle);
  bool has_exception = false;
  return key_may_exist_helper(env, db, read_options,
      nullptr, jkey, jkey_off, jkey_len, jstring_builder, &has_exception);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：keymayexist
 *签名：（jj[biijljava/lang/StringBuilder；）Z
 **/

jboolean Java_org_rocksdb_RocksDB_keyMayExist__JJ_3BIIJLjava_lang_StringBuilder_2(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jread_options_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jlong jcf_handle,
    jobject jstring_builder) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto& read_options = *reinterpret_cast<rocksdb::ReadOptions*>(
      jread_options_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(
      jcf_handle);
  if (cf_handle != nullptr) {
    bool has_exception = false;
    return key_may_exist_helper(env, db, read_options, cf_handle,
        jkey, jkey_off, jkey_len, jstring_builder, &has_exception);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
    return true;
  }
}

//////////////////////////////////////////////////
//rocksdb:：db：：获取

jbyteArray rocksdb_get_helper(
    JNIEnv* env, rocksdb::DB* db, const rocksdb::ReadOptions& read_opt,
    rocksdb::ColumnFamilyHandle* column_family_handle, jbyteArray jkey,
    jint jkey_off, jint jkey_len) {

  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] key;
    return nullptr;
  }

  rocksdb::Slice key_slice(
      reinterpret_cast<char*>(key), jkey_len);

  std::string value;
  rocksdb::Status s;
  if (column_family_handle != nullptr) {
    s = db->Get(read_opt, column_family_handle, key_slice, &value);
  } else {
//向后兼容性
    s = db->Get(read_opt, key_slice, &value);
  }

//清理
  delete [] key;

  if (s.IsNotFound()) {
    return nullptr;
  }

  if (s.ok()) {
    jbyteArray jret_value = rocksdb::JniUtil::copyBytes(env, value);
    if(jret_value == nullptr) {
//发生异常
      return nullptr;
    }
    return jret_value;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return nullptr;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（J[BII）[B]
 **/

jbyteArray Java_org_rocksdb_RocksDB_get__J_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len) {
  return rocksdb_get_helper(env,
      reinterpret_cast<rocksdb::DB*>(jdb_handle),
      rocksdb::ReadOptions(), nullptr,
      jkey, jkey_off, jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（J[Biij）[B]
 **/

jbyteArray Java_org_rocksdb_RocksDB_get__J_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jlong jcf_handle) {
  auto db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    return rocksdb_get_helper(env, db_handle, rocksdb::ReadOptions(),
        cf_handle, jkey, jkey_off, jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
    return nullptr;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（JJ[BII）[B]
 **/

jbyteArray Java_org_rocksdb_RocksDB_get__JJ_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jropt_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len) {
  return rocksdb_get_helper(env,
      reinterpret_cast<rocksdb::DB*>(jdb_handle),
      *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle), nullptr,
      jkey, jkey_off, jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（JJ[BIIJ）[B]
 **/

jbyteArray Java_org_rocksdb_RocksDB_get__JJ_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jropt_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jlong jcf_handle) {
  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto& ro_opt = *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    return rocksdb_get_helper(env, db_handle, ro_opt, cf_handle,
        jkey, jkey_off, jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
    return nullptr;
  }
}

jint rocksdb_get_helper(JNIEnv* env, rocksdb::DB* db,
                        const rocksdb::ReadOptions& read_options,
                        rocksdb::ColumnFamilyHandle* column_family_handle,
                        jbyteArray jkey, jint jkey_off, jint jkey_len,
                        jbyteArray jval, jint jval_off, jint jval_len,
                        bool* has_exception) {
  static const int kNotFound = -1;
  static const int kStatusError = -2;

  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
    delete [] key;
    *has_exception = true;
    return kStatusError;
  }
  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

//todo（yzhang）：我们可以通过添加
//一个将预分配的jbyte*作为输入的db:：get（）函数。
  std::string cvalue;
  rocksdb::Status s;
  if (column_family_handle != nullptr) {
    s = db->Get(read_options, column_family_handle, key_slice, &cvalue);
  } else {
//向后兼容性
    s = db->Get(read_options, key_slice, &cvalue);
  }

//清理
  delete [] key;

  if (s.IsNotFound()) {
    *has_exception = false;
    return kNotFound;
  } else if (!s.ok()) {
    *has_exception = true;
//这里，因为我们正在从C++侧抛出Java异常。
//因此，C++不知道事实上将调用这个函数。
//引发异常。因此，执行流将
//不要停在这里，这次投掷后的代码仍然是
//执行。
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);

//返回一个虚拟常量值以避免编译错误，尽管
//Java端可能没有获得返回值的机会：
    return kStatusError;
  }

  const jint cvalue_len = static_cast<jint>(cvalue.size());
  const jint length = std::min(jval_len, cvalue_len);

  env->SetByteArrayRegion(jval, jval_off, length,
                          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(cvalue.c_str())));
  if(env->ExceptionCheck()) {
//引发异常：OutOfMemoryError
    *has_exception = true;
    return kStatusError;
  }

  *has_exception = false;
  return cvalue_len;
}

inline void multi_get_helper_release_keys(JNIEnv* env,
    std::vector<std::pair<jbyte*, jobject>> &keys_to_free) {
  auto end = keys_to_free.end();
  for (auto it = keys_to_free.begin(); it != end; ++it) {
    delete [] it->first;
    env->DeleteLocalRef(it->second);
  }
  keys_to_free.clear();
}

/*
 ＊CF多获取
 *
 *@返回值的byte[]或nullptr（如果发生异常）
 **/

jobjectArray multi_get_helper(JNIEnv* env, jobject jdb, rocksdb::DB* db,
    const rocksdb::ReadOptions& rOpt, jobjectArray jkeys,
    jintArray jkey_offs, jintArray jkey_lens,
    jlongArray jcolumn_family_handles) {
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  if (jcolumn_family_handles != nullptr) {
    const jsize len_cols = env->GetArrayLength(jcolumn_family_handles);

    jlong* jcfh = env->GetLongArrayElements(jcolumn_family_handles, nullptr);
    if(jcfh == nullptr) {
//引发异常：OutOfMemoryError
      return nullptr;
    }

    for (jsize i = 0; i < len_cols; i++) {
      auto* cf_handle =
          reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcfh[i]);
      cf_handles.push_back(cf_handle);
    }
    env->ReleaseLongArrayElements(jcolumn_family_handles, jcfh, JNI_ABORT);
  }

  const jsize len_keys = env->GetArrayLength(jkeys);
  if (env->EnsureLocalCapacity(len_keys) != 0) {
//引发异常：OutOfMemoryError
    return nullptr;
  }

  jint* jkey_off = env->GetIntArrayElements(jkey_offs, nullptr);
  if(jkey_off == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }

  jint* jkey_len = env->GetIntArrayElements(jkey_lens, nullptr);
  if(jkey_len == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseIntArrayElements(jkey_offs, jkey_off, JNI_ABORT);
    return nullptr;
  }

  std::vector<rocksdb::Slice> keys;
  std::vector<std::pair<jbyte*, jobject>> keys_to_free;
  for (jsize i = 0; i < len_keys; i++) {
    jobject jkey = env->GetObjectArrayElement(jkeys, i);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->ReleaseIntArrayElements(jkey_lens, jkey_len, JNI_ABORT);
      env->ReleaseIntArrayElements(jkey_offs, jkey_off, JNI_ABORT);
      multi_get_helper_release_keys(env, keys_to_free);
      return nullptr;
    }

    jbyteArray jkey_ba = reinterpret_cast<jbyteArray>(jkey);

    const jint len_key = jkey_len[i];
    jbyte* key = new jbyte[len_key];
    env->GetByteArrayRegion(jkey_ba, jkey_off[i], len_key, key);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      delete [] key;
      env->DeleteLocalRef(jkey);
      env->ReleaseIntArrayElements(jkey_lens, jkey_len, JNI_ABORT);
      env->ReleaseIntArrayElements(jkey_offs, jkey_off, JNI_ABORT);
      multi_get_helper_release_keys(env, keys_to_free);
      return nullptr;
    }

    rocksdb::Slice key_slice(reinterpret_cast<char*>(key), len_key);
    keys.push_back(key_slice);

    keys_to_free.push_back(std::pair<jbyte*, jobject>(key, jkey));
  }

//清理jkey-off和jken-len
  env->ReleaseIntArrayElements(jkey_lens, jkey_len, JNI_ABORT);
  env->ReleaseIntArrayElements(jkey_offs, jkey_off, JNI_ABORT);

  std::vector<std::string> values;
  std::vector<rocksdb::Status> s;
  if (cf_handles.size() == 0) {
    s = db->MultiGet(rOpt, keys, &values);
  } else {
    s = db->MultiGet(rOpt, cf_handles, keys, &values);
  }

//释放分配的字节数组
  multi_get_helper_release_keys(env, keys_to_free);

//准备结果
  jobjectArray jresults =
      rocksdb::ByteJni::new2dByteArray(env, static_cast<jsize>(s.size()));
  if(jresults == nullptr) {
//发生异常
    return nullptr;
  }

//todo（ar）我不清楚为什么
//使用env->deleteLocalRef（jentry_value）清理引用时循环；
  if (env->EnsureLocalCapacity(static_cast<jint>(s.size())) != 0) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
//添加到jresults
  for (std::vector<rocksdb::Status>::size_type i = 0; i != s.size(); i++) {
    if (s[i].ok()) {
      std::string* value = &values[i];
      const jsize jvalue_len = static_cast<jsize>(value->size());
      jbyteArray jentry_value = env->NewByteArray(jvalue_len);
      if(jentry_value == nullptr) {
//引发异常：OutOfMemoryError
        return nullptr;
      }

      env->SetByteArrayRegion(jentry_value, 0, static_cast<jsize>(jvalue_len),
          const_cast<jbyte*>(reinterpret_cast<const jbyte*>(value->c_str())));
      if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
        env->DeleteLocalRef(jentry_value);
        return nullptr;
      }

      env->SetObjectArrayElement(jresults, static_cast<jsize>(i), jentry_value);
      if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
        env->DeleteLocalRef(jentry_value);
        return nullptr;
      }

      env->DeleteLocalRef(jentry_value);
    }
  }

  return jresults;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：multiget
 *签名：（J[[B[I[I]）[B
 **/

jobjectArray Java_org_rocksdb_RocksDB_multiGet__J_3_3B_3I_3I(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jobjectArray jkeys,
    jintArray jkey_offs, jintArray jkey_lens) {
  return multi_get_helper(env, jdb, reinterpret_cast<rocksdb::DB*>(jdb_handle),
      rocksdb::ReadOptions(), jkeys, jkey_offs, jkey_lens, nullptr);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：multiget
 *签名：（J[[B[I[I[J]）[B
 **/

jobjectArray Java_org_rocksdb_RocksDB_multiGet__J_3_3B_3I_3I_3J(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jobjectArray jkeys,
    jintArray jkey_offs, jintArray jkey_lens,
    jlongArray jcolumn_family_handles) {
  return multi_get_helper(env, jdb, reinterpret_cast<rocksdb::DB*>(jdb_handle),
      rocksdb::ReadOptions(), jkeys, jkey_offs, jkey_lens,
      jcolumn_family_handles);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：multiget
 *签名：（JJ[[B[I[I）[[B
 **/

jobjectArray Java_org_rocksdb_RocksDB_multiGet__JJ_3_3B_3I_3I(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jropt_handle,
    jobjectArray jkeys, jintArray jkey_offs, jintArray jkey_lens) {
  return multi_get_helper(env, jdb, reinterpret_cast<rocksdb::DB*>(jdb_handle),
      *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle), jkeys, jkey_offs,
      jkey_lens, nullptr);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：multiget
 *签名：（JJ[[B[I[I[J]）[B
 **/

jobjectArray Java_org_rocksdb_RocksDB_multiGet__JJ_3_3B_3I_3I_3J(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jropt_handle,
    jobjectArray jkeys, jintArray jkey_offs, jintArray jkey_lens,
    jlongArray jcolumn_family_handles) {
  return multi_get_helper(env, jdb, reinterpret_cast<rocksdb::DB*>(jdb_handle),
      *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle), jkeys, jkey_offs,
      jkey_lens, jcolumn_family_handles);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（J[BII[BII）I
 **/

jint Java_org_rocksdb_RocksDB_get__J_3BII_3BII(JNIEnv* env, jobject jdb,
                                               jlong jdb_handle,
                                               jbyteArray jkey, jint jkey_off,
                                               jint jkey_len, jbyteArray jval,
                                               jint jval_off, jint jval_len) {
  bool has_exception = false;
  return rocksdb_get_helper(env, reinterpret_cast<rocksdb::DB*>(jdb_handle),
                            rocksdb::ReadOptions(), nullptr, jkey, jkey_off,
                            jkey_len, jval, jval_off, jval_len,
                            &has_exception);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（J[BII[BIIJ）I
 **/

jint Java_org_rocksdb_RocksDB_get__J_3BII_3BIIJ(JNIEnv* env, jobject jdb,
                                                jlong jdb_handle,
                                                jbyteArray jkey, jint jkey_off,
                                                jint jkey_len, jbyteArray jval,
                                                jint jval_off, jint jval_len,
                                                jlong jcf_handle) {
  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    bool has_exception = false;
    return rocksdb_get_helper(env, db_handle, rocksdb::ReadOptions(), cf_handle,
                              jkey, jkey_off, jkey_len, jval, jval_off,
                              jval_len, &has_exception);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
//永远不会被评估
    return 0;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（JJ[BII[BII）I
 **/

jint Java_org_rocksdb_RocksDB_get__JJ_3BII_3BII(JNIEnv* env, jobject jdb,
                                                jlong jdb_handle,
                                                jlong jropt_handle,
                                                jbyteArray jkey, jint jkey_off,
                                                jint jkey_len, jbyteArray jval,
                                                jint jval_off, jint jval_len) {
  bool has_exception = false;
  return rocksdb_get_helper(
      env, reinterpret_cast<rocksdb::DB*>(jdb_handle),
      *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle), nullptr, jkey,
      jkey_off, jkey_len, jval, jval_off, jval_len, &has_exception);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：获取
 *签字：（JJ[BII[BIIJ）I
 **/

jint Java_org_rocksdb_RocksDB_get__JJ_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jropt_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jbyteArray jval,
    jint jval_off, jint jval_len, jlong jcf_handle) {
  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto& ro_opt = *reinterpret_cast<rocksdb::ReadOptions*>(jropt_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    bool has_exception = false;
    return rocksdb_get_helper(env, db_handle, ro_opt, cf_handle, jkey, jkey_off,
                              jkey_len, jval, jval_off, jval_len,
                              &has_exception);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
//永远不会被评估
    return 0;
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：delete（）。

/*
 *如果删除成功，则返回true，如果抛出Java异常，则返回false
 **/

bool rocksdb_delete_helper(
    JNIEnv* env, rocksdb::DB* db, const rocksdb::WriteOptions& write_options,
    rocksdb::ColumnFamilyHandle* cf_handle, jbyteArray jkey, jint jkey_off,
    jint jkey_len) {
  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] key;
    return false;
  }
  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

  rocksdb::Status s;
  if (cf_handle != nullptr) {
    s = db->Delete(write_options, cf_handle, key_slice);
  } else {
//向后兼容性
    s = db->Delete(write_options, key_slice);
  }

//清理
  delete [] key;

  if (s.ok()) {
    return true;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return false;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除
 *签字：（J[BII）V
 **/

void Java_org_rocksdb_RocksDB_delete__J_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  rocksdb_delete_helper(env, db, default_write_options, nullptr,
      jkey, jkey_off, jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除
 *签名：（J[Biij）V
 **/

void Java_org_rocksdb_RocksDB_delete__J_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_delete_helper(env, db, default_write_options, cf_handle,
        jkey, jkey_off, jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除
 *签字：（JJ[BII）V
 **/

void Java_org_rocksdb_RocksDB_delete__JJ_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options, jbyteArray jkey, jint jkey_off, jint jkey_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  rocksdb_delete_helper(env, db, *write_options, nullptr, jkey, jkey_off,
      jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除
 *签字：（JJ[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_delete__JJ_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options, jbyteArray jkey, jint jkey_off, jint jkey_len,
    jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_delete_helper(env, db, *write_options, cf_handle, jkey, jkey_off,
        jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：singledelete（）。
/*
 *如果单个删除成功，则返回true，如果Java异常，则返回false
 *被抛出
 **/

bool rocksdb_single_delete_helper(
    JNIEnv* env, rocksdb::DB* db, const rocksdb::WriteOptions& write_options,
    rocksdb::ColumnFamilyHandle* cf_handle, jbyteArray jkey, jint jkey_len) {
  jbyte* key = env->GetByteArrayElements(jkey, nullptr);
  if(key == nullptr) {
//引发异常：OutOfMemoryError
    return false;
  }
  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

  rocksdb::Status s;
  if (cf_handle != nullptr) {
    s = db->SingleDelete(write_options, cf_handle, key_slice);
  } else {
//向后兼容性
    s = db->SingleDelete(write_options, key_slice);
  }

//触发JavaunRF键和值。
//通过传递jniou abort，它将简单地释放引用而不
//将结果复制回Java字节数组。
  env->ReleaseByteArrayElements(jkey, key, JNI_ABORT);

  if (s.ok()) {
    return true;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return false;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：单删除
 *签字：（J[BI）V
 **/

void Java_org_rocksdb_RocksDB_singleDelete__J_3BI(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  rocksdb_single_delete_helper(env, db, default_write_options, nullptr,
      jkey, jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：单删除
 *签字：（J[BIJ）V
 **/

void Java_org_rocksdb_RocksDB_singleDelete__J_3BIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jkey, jint jkey_len, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_single_delete_helper(env, db, default_write_options, cf_handle,
        jkey, jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：单删除
 *签字：（JJ[BIJ）V
 **/

void Java_org_rocksdb_RocksDB_singleDelete__JJ_3BI(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options, jbyteArray jkey, jint jkey_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  rocksdb_single_delete_helper(env, db, *write_options, nullptr, jkey,
      jkey_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：单删除
 *签字：（JJ[BIJ）V
 **/

void Java_org_rocksdb_RocksDB_singleDelete__JJ_3BIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jwrite_options, jbyteArray jkey, jint jkey_len,
    jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_single_delete_helper(env, db, *write_options, cf_handle, jkey,
        jkey_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

//////////////////////////////////////////////////
//RocksDB：：数据库：：删除范围（）。
/*
 *如果删除范围成功，则返回true；如果Java异常，则返回false
 *被抛出
 **/

bool rocksdb_delete_range_helper(JNIEnv* env, rocksdb::DB* db,
                                 const rocksdb::WriteOptions& write_options,
                                 rocksdb::ColumnFamilyHandle* cf_handle,
                                 jbyteArray jbegin_key, jint jbegin_key_off,
                                 jint jbegin_key_len, jbyteArray jend_key,
                                 jint jend_key_off, jint jend_key_len) {
  jbyte* begin_key = new jbyte[jbegin_key_len];
  env->GetByteArrayRegion(jbegin_key, jbegin_key_off, jbegin_key_len,
                          begin_key);
  if (env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete[] begin_key;
    return false;
  }
  rocksdb::Slice begin_key_slice(reinterpret_cast<char*>(begin_key),
                                 jbegin_key_len);

  jbyte* end_key = new jbyte[jend_key_len];
  env->GetByteArrayRegion(jend_key, jend_key_off, jend_key_len, end_key);
  if (env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete[] begin_key;
    delete[] end_key;
    return false;
  }
  rocksdb::Slice end_key_slice(reinterpret_cast<char*>(end_key), jend_key_len);

  rocksdb::Status s =
      db->DeleteRange(write_options, cf_handle, begin_key_slice, end_key_slice);

//清理
  delete[] begin_key;
  delete[] end_key;

  if (s.ok()) {
    return true;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return false;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除范围
 *签字：（J[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_deleteRange__J_3BII_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jbegin_key,
    jint jbegin_key_off, jint jbegin_key_len, jbyteArray jend_key,
    jint jend_key_off, jint jend_key_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  rocksdb_delete_range_helper(env, db, default_write_options, nullptr,
                              jbegin_key, jbegin_key_off, jbegin_key_len,
                              jend_key, jend_key_off, jend_key_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除范围
 *签字：（J[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_deleteRange__J_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jbegin_key,
    jint jbegin_key_off, jint jbegin_key_len, jbyteArray jend_key,
    jint jend_key_off, jint jend_key_len, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_delete_range_helper(env, db, default_write_options, cf_handle,
                                jbegin_key, jbegin_key_off, jbegin_key_len,
                                jend_key, jend_key_off, jend_key_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(
        env, rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除范围
 *签字：（JJ[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_deleteRange__JJ_3BII_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jwrite_options,
    jbyteArray jbegin_key, jint jbegin_key_off, jint jbegin_key_len,
    jbyteArray jend_key, jint jend_key_off, jint jend_key_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  rocksdb_delete_range_helper(env, db, *write_options, nullptr, jbegin_key,
                              jbegin_key_off, jbegin_key_len, jend_key,
                              jend_key_off, jend_key_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：删除范围
 *签字：（JJ[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_deleteRange__JJ_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jwrite_options,
    jbyteArray jbegin_key, jint jbegin_key_off, jint jbegin_key_len,
    jbyteArray jend_key, jint jend_key_off, jint jend_key_len,
    jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_delete_range_helper(env, db, *write_options, cf_handle, jbegin_key,
                                jbegin_key_off, jbegin_key_len, jend_key,
                                jend_key_off, jend_key_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(
        env, rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：合并

/*
 *如果合并成功，则返回true，如果抛出Java异常，则返回false
 **/

bool rocksdb_merge_helper(JNIEnv* env, rocksdb::DB* db,
                          const rocksdb::WriteOptions& write_options,
                          rocksdb::ColumnFamilyHandle* cf_handle,
                          jbyteArray jkey, jint jkey_off, jint jkey_len,
                          jbyteArray jval, jint jval_off, jint jval_len) {
  jbyte* key = new jbyte[jkey_len];
  env->GetByteArrayRegion(jkey, jkey_off, jkey_len, key);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] key;
    return false;
  }
  rocksdb::Slice key_slice(reinterpret_cast<char*>(key), jkey_len);

  jbyte* value = new jbyte[jval_len];
  env->GetByteArrayRegion(jval, jval_off, jval_len, value);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
    delete [] value;
    delete [] key;
    return false;
  }
  rocksdb::Slice value_slice(reinterpret_cast<char*>(value), jval_len);

  rocksdb::Status s;
  if (cf_handle != nullptr) {
    s = db->Merge(write_options, cf_handle, key_slice, value_slice);
  } else {
    s = db->Merge(write_options, key_slice, value_slice);
  }

//清理
  delete [] value;
  delete [] key;

  if (s.ok()) {
    return true;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return false;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：合并
 *签字：（J[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_merge__J_3BII_3BII(JNIEnv* env, jobject jdb,
                                                 jlong jdb_handle,
                                                 jbyteArray jkey, jint jkey_off,
                                                 jint jkey_len, jbyteArray jval,
                                                 jint jval_off, jint jval_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();

  rocksdb_merge_helper(env, db, default_write_options, nullptr, jkey, jkey_off,
                       jkey_len, jval, jval_off, jval_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：合并
 *签字：（J[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_merge__J_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jkey, jint jkey_off,
    jint jkey_len, jbyteArray jval, jint jval_off, jint jval_len,
    jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  static const rocksdb::WriteOptions default_write_options =
      rocksdb::WriteOptions();
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_merge_helper(env, db, default_write_options, cf_handle, jkey,
                         jkey_off, jkey_len, jval, jval_off, jval_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：合并
 *签字：（JJ[BII[BII）V
 **/

void Java_org_rocksdb_RocksDB_merge__JJ_3BII_3BII(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jwrite_options_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jbyteArray jval,
    jint jval_off, jint jval_len) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options_handle);

  rocksdb_merge_helper(env, db, *write_options, nullptr, jkey, jkey_off,
                       jkey_len, jval, jval_off, jval_len);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：合并
 *签字：（JJ[BII[BIIJ）V
 **/

void Java_org_rocksdb_RocksDB_merge__JJ_3BII_3BIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jwrite_options_handle,
    jbyteArray jkey, jint jkey_off, jint jkey_len, jbyteArray jval,
    jint jval_off, jint jval_len, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* write_options =
      reinterpret_cast<rocksdb::WriteOptions*>(jwrite_options_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  if (cf_handle != nullptr) {
    rocksdb_merge_helper(env, db, *write_options, cf_handle, jkey, jkey_off,
                         jkey_len, jval, jval_off, jval_len);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env,
        rocksdb::Status::InvalidArgument("Invalid ColumnFamilyHandle."));
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：~db（）。

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksDB_disposeInternal(
    JNIEnv* env, jobject java_db, jlong jhandle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jhandle);
  assert(db != nullptr);
  delete db;
}

jlong rocksdb_iterator_helper(
    rocksdb::DB* db, rocksdb::ReadOptions read_options,
    rocksdb::ColumnFamilyHandle* cf_handle) {
  rocksdb::Iterator* iterator = nullptr;
  if (cf_handle != nullptr) {
    iterator = db->NewIterator(read_options, cf_handle);
  } else {
    iterator = db->NewIterator(read_options);
  }
  return reinterpret_cast<jlong>(iterator);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：迭代器
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RocksDB_iterator__J(
    JNIEnv* env, jobject jdb, jlong db_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  return rocksdb_iterator_helper(db, rocksdb::ReadOptions(),
      nullptr);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：迭代器
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_RocksDB_iterator__JJ(
    JNIEnv* env, jobject jdb, jlong db_handle,
    jlong jread_options_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto& read_options = *reinterpret_cast<rocksdb::ReadOptions*>(
      jread_options_handle);
  return rocksdb_iterator_helper(db, read_options,
      nullptr);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：迭代器
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_RocksDB_iteratorCF__JJ(
    JNIEnv* env, jobject jdb, jlong db_handle, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  return rocksdb_iterator_helper(db, rocksdb::ReadOptions(),
        cf_handle);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：迭代器
 *签字：（JJJ）J
 **/

jlong Java_org_rocksdb_RocksDB_iteratorCF__JJJ(
    JNIEnv* env, jobject jdb, jlong db_handle, jlong jcf_handle,
    jlong jread_options_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  auto& read_options = *reinterpret_cast<rocksdb::ReadOptions*>(
      jread_options_handle);
  return rocksdb_iterator_helper(db, read_options,
        cf_handle);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：迭代器
 *签字：（J[JJ）[J
 **/

jlongArray Java_org_rocksdb_RocksDB_iterators(
    JNIEnv* env, jobject jdb, jlong db_handle,
    jlongArray jcolumn_family_handles, jlong jread_options_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto& read_options = *reinterpret_cast<rocksdb::ReadOptions*>(
        jread_options_handle);

  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  if (jcolumn_family_handles != nullptr) {
    const jsize len_cols = env->GetArrayLength(jcolumn_family_handles);
    jlong* jcfh = env->GetLongArrayElements(jcolumn_family_handles, nullptr);
    if(jcfh == nullptr) {
//引发异常：OutOfMemoryError
      return nullptr;
    }

    for (jsize i = 0; i < len_cols; i++) {
      auto* cf_handle =
          reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcfh[i]);
      cf_handles.push_back(cf_handle);
    }

    env->ReleaseLongArrayElements(jcolumn_family_handles, jcfh, JNI_ABORT);
  }

  std::vector<rocksdb::Iterator*> iterators;
  rocksdb::Status s = db->NewIterators(read_options,
      cf_handles, &iterators);
  if (s.ok()) {
    jlongArray jLongArray =
        env->NewLongArray(static_cast<jsize>(iterators.size()));
    if(jLongArray == nullptr) {
//引发异常：OutOfMemoryError
      return nullptr;
    }

    for (std::vector<rocksdb::Iterator*>::size_type i = 0;
        i < iterators.size(); i++) {
      env->SetLongArrayRegion(jLongArray, static_cast<jsize>(i), 1,
                              const_cast<jlong*>(reinterpret_cast<const jlong*>(&iterators[i])));
      if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
        env->DeleteLocalRef(jLongArray);
        return nullptr;
      }
    }

    return jLongArray;
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
    return nullptr;
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetDefaultColumnFamily
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RocksDB_getDefaultColumnFamily(
    JNIEnv* env, jobject jobj, jlong jdb_handle) {
  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = db_handle->DefaultColumnFamily();
  return reinterpret_cast<jlong>(cf_handle);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：CreateColumnFamily
 *签字：（J[BJ）J
 **/

jlong Java_org_rocksdb_RocksDB_createColumnFamily(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jbyteArray jcolumn_name, jlong jcolumn_options) {
  rocksdb::ColumnFamilyHandle* handle;
  jboolean has_exception = JNI_FALSE;
  std::string column_name = rocksdb::JniUtil::byteString<std::string>(env,
    jcolumn_name,
    [](const char* str, const size_t len) { return std::string(str, len); },
    &has_exception);
  if(has_exception == JNI_TRUE) {
//发生异常
    return 0;
  }

  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cfOptions =
      reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jcolumn_options);

  rocksdb::Status s = db_handle->CreateColumnFamily(
      *cfOptions, column_name, &handle);

  if (s.ok()) {
    return reinterpret_cast<jlong>(handle);
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return 0;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：DropColumnFamily
 *签字：（JJ）V；
 **/

void Java_org_rocksdb_RocksDB_dropColumnFamily(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jcf_handle) {
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  auto* db_handle = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb::Status s = db_handle->DropColumnFamily(cf_handle);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *方法：GetSnapshot
 *签字：（j）j
 **/

jlong Java_org_rocksdb_RocksDB_getSnapshot(
    JNIEnv* env, jobject jdb, jlong db_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  const rocksdb::Snapshot* snapshot = db->GetSnapshot();
  return reinterpret_cast<jlong>(snapshot);
}

/*
 *方法：releaseSnapshot
 *签字：（JJ）V
 **/

void Java_org_rocksdb_RocksDB_releaseSnapshot(
    JNIEnv* env, jobject jdb, jlong db_handle, jlong snapshot_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* snapshot = reinterpret_cast<rocksdb::Snapshot*>(snapshot_handle);
  db->ReleaseSnapshot(snapshot);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetProperty0
 *签名：（jljava/lang/string；i）ljava/lang/string；
 **/

jstring Java_org_rocksdb_RocksDB_getProperty0__JLjava_lang_String_2I(
    JNIEnv* env, jobject jdb, jlong db_handle, jstring jproperty,
    jint jproperty_len) {
  const char* property = env->GetStringUTFChars(jproperty, nullptr);
  if(property == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  rocksdb::Slice property_slice(property, jproperty_len);

  auto *db = reinterpret_cast<rocksdb::DB*>(db_handle);
  std::string property_value;
  bool retCode = db->GetProperty(property_slice, &property_value);
  env->ReleaseStringUTFChars(jproperty, property);

  if (retCode) {
    return env->NewStringUTF(property_value.c_str());
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, rocksdb::Status::NotFound());
  return nullptr;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetProperty0
 *签名：（jjljava/lang/string；i）ljava/lang/string；
 **/

jstring Java_org_rocksdb_RocksDB_getProperty0__JJLjava_lang_String_2I(
    JNIEnv* env, jobject jdb, jlong db_handle, jlong jcf_handle,
    jstring jproperty, jint jproperty_len) {
  const char* property = env->GetStringUTFChars(jproperty, nullptr);
  if(property == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  rocksdb::Slice property_slice(property, jproperty_len);

  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  std::string property_value;
  bool retCode = db->GetProperty(cf_handle, property_slice, &property_value);
  env->ReleaseStringUTFChars(jproperty, property);

  if (retCode) {
    return env->NewStringUTF(property_value.c_str());
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, rocksdb::Status::NotFound());
  return nullptr;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetLongProperty
 *签名：（jljava/lang/string；i）l；
 **/

jlong Java_org_rocksdb_RocksDB_getLongProperty__JLjava_lang_String_2I(
    JNIEnv* env, jobject jdb, jlong db_handle, jstring jproperty,
    jint jproperty_len) {
  const char* property = env->GetStringUTFChars(jproperty, nullptr);
  if(property == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }
  rocksdb::Slice property_slice(property, jproperty_len);

  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  uint64_t property_value = 0;
  bool retCode = db->GetIntProperty(property_slice, &property_value);
  env->ReleaseStringUTFChars(jproperty, property);

  if (retCode) {
    return property_value;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, rocksdb::Status::NotFound());
  return 0;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetLongProperty
 *签名：（jjljava/lang/string；i）l；
 **/

jlong Java_org_rocksdb_RocksDB_getLongProperty__JJLjava_lang_String_2I(
    JNIEnv* env, jobject jdb, jlong db_handle, jlong jcf_handle,
    jstring jproperty, jint jproperty_len) {
  const char* property = env->GetStringUTFChars(jproperty, nullptr);
  if(property == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }
  rocksdb::Slice property_slice(property, jproperty_len);

  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  uint64_t property_value;
  bool retCode = db->GetIntProperty(cf_handle, property_slice, &property_value);
  env->ReleaseStringUTFChars(jproperty, property);

  if (retCode) {
    return property_value;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, rocksdb::Status::NotFound());
  return 0;
}

//////////////////////////////////////////////////
//rocksdb:：db:：flush（刷新）

void rocksdb_flush_helper(
    JNIEnv* env, rocksdb::DB* db, const rocksdb::FlushOptions& flush_options,
  rocksdb::ColumnFamilyHandle* column_family_handle) {
  rocksdb::Status s;
  if (column_family_handle != nullptr) {
    s = db->Flush(flush_options, column_family_handle);
  } else {
    s = db->Flush(flush_options);
  }
  if (!s.ok()) {
      rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：冲洗
 *签字：（JJ）V
 **/

void Java_org_rocksdb_RocksDB_flush__JJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jflush_options) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* flush_options =
      reinterpret_cast<rocksdb::FlushOptions*>(jflush_options);
  rocksdb_flush_helper(env, db, *flush_options, nullptr);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：冲洗
 *签字：（JJJ）V
 **/

void Java_org_rocksdb_RocksDB_flush__JJJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
    jlong jflush_options, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* flush_options =
      reinterpret_cast<rocksdb::FlushOptions*>(jflush_options);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  rocksdb_flush_helper(env, db, *flush_options, cf_handle);
}

//////////////////////////////////////////////////
//rocksdb:：db:：compactrange-满

void rocksdb_compactrange_helper(JNIEnv* env, rocksdb::DB* db,
    rocksdb::ColumnFamilyHandle* cf_handle, jboolean jreduce_level,
    jint jtarget_level, jint jtarget_path_id) {

  rocksdb::Status s;
  rocksdb::CompactRangeOptions compact_options;
  compact_options.change_level = jreduce_level;
  compact_options.target_level = jtarget_level;
  compact_options.target_path_id = static_cast<uint32_t>(jtarget_path_id);
  if (cf_handle != nullptr) {
    s = db->CompactRange(compact_options, cf_handle, nullptr, nullptr);
  } else {
//向后兼容性
    s = db->CompactRange(compact_options, nullptr, nullptr);
  }

  if (s.ok()) {
    return;
  }
  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：CompactRange0
 *签字：（JZII）V
 **/

void Java_org_rocksdb_RocksDB_compactRange0__JZII(JNIEnv* env,
    jobject jdb, jlong jdb_handle, jboolean jreduce_level,
    jint jtarget_level, jint jtarget_path_id) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb_compactrange_helper(env, db, nullptr, jreduce_level,
      jtarget_level, jtarget_path_id);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：压实范围
 *签字：（JZIIJ）V
 **/

void Java_org_rocksdb_RocksDB_compactRange__JZIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle,
     jboolean jreduce_level, jint jtarget_level,
     jint jtarget_path_id, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  rocksdb_compactrange_helper(env, db, cf_handle, jreduce_level,
      jtarget_level, jtarget_path_id);
}

//////////////////////////////////////////////////
//rocksdb:：db:：compactrange-范围

/*
 *如果压缩范围成功，则返回true；如果Java异常，则返回false
 *被抛出
 **/

bool rocksdb_compactrange_helper(JNIEnv* env, rocksdb::DB* db,
    rocksdb::ColumnFamilyHandle* cf_handle, jbyteArray jbegin, jint jbegin_len,
    jbyteArray jend, jint jend_len, jboolean jreduce_level, jint jtarget_level,
    jint jtarget_path_id) {

  jbyte* begin = env->GetByteArrayElements(jbegin, nullptr);
  if(begin == nullptr) {
//引发异常：OutOfMemoryError
    return false;
  }

  jbyte* end = env->GetByteArrayElements(jend, nullptr);
  if(end == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseByteArrayElements(jbegin, begin, JNI_ABORT);
    return false;
  }

  const rocksdb::Slice begin_slice(reinterpret_cast<char*>(begin), jbegin_len);
  const rocksdb::Slice end_slice(reinterpret_cast<char*>(end), jend_len);

  rocksdb::Status s;
  rocksdb::CompactRangeOptions compact_options;
  compact_options.change_level = jreduce_level;
  compact_options.target_level = jtarget_level;
  compact_options.target_path_id = static_cast<uint32_t>(jtarget_path_id);
  if (cf_handle != nullptr) {
    s = db->CompactRange(compact_options, cf_handle, &begin_slice, &end_slice);
  } else {
//向后兼容性
    s = db->CompactRange(compact_options, &begin_slice, &end_slice);
  }

  env->ReleaseByteArrayElements(jend, end, JNI_ABORT);
  env->ReleaseByteArrayElements(jbegin, begin, JNI_ABORT);

  if (s.ok()) {
    return true;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return false;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：CompactRange0
 *签字：（J[BI[Bizii）V
 **/

void Java_org_rocksdb_RocksDB_compactRange0__J_3BI_3BIZII(JNIEnv* env,
    jobject jdb, jlong jdb_handle, jbyteArray jbegin, jint jbegin_len,
    jbyteArray jend, jint jend_len, jboolean jreduce_level,
    jint jtarget_level, jint jtarget_path_id) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb_compactrange_helper(env, db, nullptr, jbegin, jbegin_len,
      jend, jend_len, jreduce_level, jtarget_level, jtarget_path_id);
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：压实范围
 *签字：（JJ[BI[Bizii）V
 **/

void Java_org_rocksdb_RocksDB_compactRange__J_3BI_3BIZIIJ(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jbyteArray jbegin,
    jint jbegin_len, jbyteArray jend, jint jend_len,
    jboolean jreduce_level, jint jtarget_level,
    jint jtarget_path_id, jlong jcf_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  rocksdb_compactrange_helper(env, db, cf_handle, jbegin, jbegin_len,
      jend, jend_len, jreduce_level, jtarget_level, jtarget_path_id);
}

//////////////////////////////////////////////////
//rocksdb:：db:：pausebackgroundwork

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：Pausebackgroundwork
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksDB_pauseBackgroundWork(
    JNIEnv* env, jobject jobj, jlong jdb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto s = db->PauseBackgroundWork();
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

//////////////////////////////////////////////////
//RocksDB:：DB:：ContinueBackgroundWork

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：连续土方工程
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksDB_continueBackgroundWork(
    JNIEnv* env, jobject jobj, jlong jdb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto s = db->ContinueBackgroundWork();
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：getLatestSequenceNumber（最新序列号）

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetLatestSequenceNumber
 *签名：（j）v
 **/

jlong Java_org_rocksdb_RocksDB_getLatestSequenceNumber(JNIEnv* env,
    jobject jdb, jlong jdb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  return db->GetLatestSequenceNumber();
}

//////////////////////////////////////////////////
//rocksdb:：db启用/禁用文件删除

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：启用文件删除
 *签名：（j）v
 **/

void Java_org_rocksdb_RocksDB_disableFileDeletions(JNIEnv* env,
    jobject jdb, jlong jdb_handle) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb::Status s = db->DisableFileDeletions();
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：启用文件删除
 *签字：（JZ）V
 **/

void Java_org_rocksdb_RocksDB_enableFileDeletions(JNIEnv* env,
    jobject jdb, jlong jdb_handle, jboolean jforce) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb::Status s = db->EnableFileDeletions(jforce);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}

//////////////////////////////////////////////////
//rocksdb:：db:：getUpdateScience

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：GetUpdateScience
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_RocksDB_getUpdatesSince(JNIEnv* env,
    jobject jdb, jlong jdb_handle, jlong jsequence_number) {
  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  rocksdb::SequenceNumber sequence_number =
      static_cast<rocksdb::SequenceNumber>(jsequence_number);
  std::unique_ptr<rocksdb::TransactionLogIterator> iter;
  rocksdb::Status s = db->GetUpdatesSince(sequence_number, &iter);
  if (s.ok()) {
    return reinterpret_cast<jlong>(iter.release());
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  return 0;
}

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：设置选项
 *签名：（jj[ljava/lang/string；[ljava/lang/string；）v
 **/

void Java_org_rocksdb_RocksDB_setOptions(JNIEnv* env, jobject jdb,
    jlong jdb_handle, jlong jcf_handle, jobjectArray jkeys,
    jobjectArray jvalues) {
  const jsize len = env->GetArrayLength(jkeys);
  assert(len == env->GetArrayLength(jvalues));

  std::unordered_map<std::string, std::string> options_map;
  for (jsize i = 0; i < len; i++) {
    jobject jobj_key = env->GetObjectArrayElement(jkeys, i);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      return;
    }

    jobject jobj_value = env->GetObjectArrayElement(jvalues, i);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(jobj_key);
      return;
    }

    jstring jkey = reinterpret_cast<jstring>(jobj_key);
    jstring jval = reinterpret_cast<jstring>(jobj_value);

    const char* key = env->GetStringUTFChars(jkey, nullptr);
    if(key == nullptr) {
//引发异常：OutOfMemoryError
      env->DeleteLocalRef(jobj_value);
      env->DeleteLocalRef(jobj_key);
      return;
    }

    const char* value = env->GetStringUTFChars(jval, nullptr);
    if(value == nullptr) {
//引发异常：OutOfMemoryError
      env->ReleaseStringUTFChars(jkey, key);
      env->DeleteLocalRef(jobj_value);
      env->DeleteLocalRef(jobj_key);
      return;
    }

    std::string s_key(key);
    std::string s_value(value);
    options_map[s_key] = s_value;

    env->ReleaseStringUTFChars(jkey, key);
    env->ReleaseStringUTFChars(jval, value);
    env->DeleteLocalRef(jobj_key);
    env->DeleteLocalRef(jobj_value);
  }

  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* cf_handle = reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  db->SetOptions(cf_handle, options_map);
}

//////////////////////////////////////////////////
//rocksdb：：db：：摄取外部文件

/*
 *课程：Org_Rocksdb_Rocksdb
 *方法：摄取外部文件
 *签名：（jj[ljava/lang/string；ij）v
 **/

void Java_org_rocksdb_RocksDB_ingestExternalFile(
    JNIEnv* env, jobject jdb, jlong jdb_handle, jlong jcf_handle,
    jobjectArray jfile_path_list, jint jfile_path_list_len,
    jlong jingest_external_file_options_handle) {
  jboolean has_exception = JNI_FALSE;
  std::vector<std::string> file_path_list =
      rocksdb::JniUtil::copyStrings(env, jfile_path_list, jfile_path_list_len,
          &has_exception);
  if(has_exception == JNI_TRUE) {
//发生异常
    return;
  }

  auto* db = reinterpret_cast<rocksdb::DB*>(jdb_handle);
  auto* column_family =
      reinterpret_cast<rocksdb::ColumnFamilyHandle*>(jcf_handle);
  auto* ifo =
      reinterpret_cast<rocksdb::IngestExternalFileOptions*>(
          jingest_external_file_options_handle);
  rocksdb::Status s =
      db->IngestExternalFile(column_family, file_path_list, *ifo);
  if (!s.ok()) {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, s);
  }
}
