
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
//该文件实现了RoSdSDB:：选项之间的Java和C++之间的“桥梁”。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <memory>
#include <vector>

#include "include/org_rocksdb_Options.h"
#include "include/org_rocksdb_DBOptions.h"
#include "include/org_rocksdb_ColumnFamilyOptions.h"
#include "include/org_rocksdb_WriteOptions.h"
#include "include/org_rocksdb_ReadOptions.h"
#include "include/org_rocksdb_ComparatorOptions.h"
#include "include/org_rocksdb_FlushOptions.h"

#include "rocksjni/comparatorjnicallback.h"
#include "rocksjni/portal.h"
#include "rocksjni/statisticsjni.h"

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/table.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/comparator.h"
#include "rocksdb/convenience.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"

/*
 *课程：org_rocksdb_选项
 *方法：新选项
 *签字：（）J
 **/

jlong Java_org_rocksdb_Options_newOptions__(JNIEnv* env, jclass jcls) {
  auto* op = new rocksdb::Options();
  return reinterpret_cast<jlong>(op);
}

/*
 *课程：org_rocksdb_选项
 *方法：新选项
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_Options_newOptions__JJ(JNIEnv* env, jclass jcls,
    jlong jdboptions, jlong jcfoptions) {
  auto* dbOpt = reinterpret_cast<const rocksdb::DBOptions*>(jdboptions);
  auto* cfOpt = reinterpret_cast<const rocksdb::ColumnFamilyOptions*>(
      jcfoptions);
  auto* op = new rocksdb::Options(*dbOpt, *cfOpt);
  return reinterpret_cast<jlong>(op);
}

/*
 *课程：org_rocksdb_选项
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_Options_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* op = reinterpret_cast<rocksdb::Options*>(handle);
  assert(op != nullptr);
  delete op;
}

/*
 *课程：org_rocksdb_选项
 *方法：setincreaseparallelism
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setIncreaseParallelism(
    JNIEnv * env, jobject jobj, jlong jhandle, jint totalThreads) {
  reinterpret_cast<rocksdb::Options*>
      (jhandle)->IncreaseParallelism(static_cast<int>(totalThreads));
}

/*
 *课程：org_rocksdb_选项
 *方法：setCreateifMissing
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setCreateIfMissing(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->create_if_missing = flag;
}

/*
 *课程：org_rocksdb_选项
 *方法：createifMissing
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_createIfMissing(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->create_if_missing;
}

/*
 *课程：org_rocksdb_选项
 *方法：setCreateMissingColumnFamilies
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setCreateMissingColumnFamilies(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  reinterpret_cast<rocksdb::Options*>
      (jhandle)->create_missing_column_families = flag;
}

/*
 *课程：org_rocksdb_选项
 *方法：创建MissingColumnFamilies
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_createMissingColumnFamilies(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>
      (jhandle)->create_missing_column_families;
}

/*
 *课程：org_rocksdb_选项
 *方法：setComparatorHandle
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setComparatorHandle__JI(
    JNIEnv* env, jobject jobj, jlong jhandle, jint builtinComparator) {
  switch (builtinComparator) {
    case 1:
      reinterpret_cast<rocksdb::Options*>(jhandle)->comparator =
          rocksdb::ReverseBytewiseComparator();
      break;
    default:
      reinterpret_cast<rocksdb::Options*>(jhandle)->comparator =
          rocksdb::BytewiseComparator();
      break;
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：setComparatorHandle
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setComparatorHandle__JJ(
    JNIEnv* env, jobject jobj, jlong jopt_handle, jlong jcomparator_handle) {
  reinterpret_cast<rocksdb::Options*>(jopt_handle)->comparator =
      reinterpret_cast<rocksdb::Comparator*>(jcomparator_handle);
}

/*
 *课程：org_rocksdb_选项
 *方法：setmergeoperatorname
 *签名：（jjjava/lang/string）v
 **/

void Java_org_rocksdb_Options_setMergeOperatorName(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jop_name) {
  const char* op_name = env->GetStringUTFChars(jop_name, nullptr);
  if(op_name == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  options->merge_operator = rocksdb::MergeOperators::CreateFromStringId(
        op_name);

  env->ReleaseStringUTFChars(jop_name, op_name);
}

/*
 *课程：org_rocksdb_选项
 *方法：setmergeoperator
 *签名：（jjjava/lang/string）v
 **/

void Java_org_rocksdb_Options_setMergeOperator(
  JNIEnv* env, jobject jobj, jlong jhandle, jlong mergeOperatorHandle) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->merge_operator =
    *(reinterpret_cast<std::shared_ptr<rocksdb::MergeOperator>*>
      (mergeOperatorHandle));
}

/*
 *课程：org_rocksdb_选项
 *方法：setWriteBufferSize
 *签字：（JJ）I
 **/

void Java_org_rocksdb_Options_setWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jwrite_buffer_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jwrite_buffer_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->write_buffer_size =
        jwrite_buffer_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：WriteBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_writeBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->write_buffer_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：setMaxWriteBufferNumber
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxWriteBufferNumber(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_write_buffer_number) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_write_buffer_number =
          jmax_write_buffer_number;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置统计
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setStatistics(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jstatistics_handle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  auto* pSptr =
      reinterpret_cast<std::shared_ptr<rocksdb::StatisticsJni>*>(
          jstatistics_handle);
  opt->statistics = *pSptr;
}

/*
 *课程：org_rocksdb_选项
 *方法：统计
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_statistics(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  std::shared_ptr<rocksdb::Statistics> sptr = opt->statistics;
  if (sptr == nullptr) {
    return 0;
  } else {
    std::shared_ptr<rocksdb::Statistics>* pSptr =
        new std::shared_ptr<rocksdb::Statistics>(sptr);
    return reinterpret_cast<jlong>(pSptr);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxWriteBufferNumber
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxWriteBufferNumber(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_write_buffer_number;
}

/*
 *课程：org_rocksdb_选项
 *方法：错误存在
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_errorIfExists(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->error_if_exists;
}

/*
 *课程：org_rocksdb_选项
 *方法：seterrorifexists
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setErrorIfExists(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean error_if_exists) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->error_if_exists =
      static_cast<bool>(error_if_exists);
}

/*
 *课程：org_rocksdb_选项
 *方法：偏执检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_paranoidChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->paranoid_checks;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置偏执检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setParanoidChecks(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean paranoid_checks) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->paranoid_checks =
      static_cast<bool>(paranoid_checks);
}

/*
 *课程：org_rocksdb_选项
 *方法：setenv
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setEnv(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jenv) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->env =
      reinterpret_cast<rocksdb::Env*>(jenv);
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxtotalwalsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxTotalWalSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_total_wal_size) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_total_wal_size =
      static_cast<jlong>(jmax_total_wal_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxTotalWalSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxTotalWalSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->
      max_total_wal_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：maxopenfiles
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxOpenFiles(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_open_files;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxopenfiles
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxOpenFiles(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max_open_files) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_open_files =
      static_cast<int>(max_open_files);
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxfileopeningthreads
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxFileOpeningThreads(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_file_opening_threads) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_file_opening_threads =
      static_cast<int>(jmax_file_opening_threads);
}

/*
 *课程：org_rocksdb_选项
 *方法：maxfileopeningthreads
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxFileOpeningThreads(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<int>(opt->max_file_opening_threads);
}

/*
 *课程：org_rocksdb_选项
 *方法：usefsync
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_useFsync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->use_fsync;
}

/*
 *课程：org_rocksdb_选项
 *方法：setusefsync
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setUseFsync(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean use_fsync) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->use_fsync =
      static_cast<bool>(use_fsync);
}

/*
 *课程：org_rocksdb_选项
 *方法：setdbpaths
 *签名：（j[ljava/lang/string；[j）v
 **/

void Java_org_rocksdb_Options_setDbPaths(
    JNIEnv* env, jobject jobj, jlong jhandle, jobjectArray jpaths,
    jlongArray jtarget_sizes) {
  std::vector<rocksdb::DbPath> db_paths;
  jlong* ptr_jtarget_size = env->GetLongArrayElements(jtarget_sizes, nullptr);
  if(ptr_jtarget_size == nullptr) {
//引发异常：OutOfMemoryError
      return;
  }

  jboolean has_exception = JNI_FALSE;
  const jsize len = env->GetArrayLength(jpaths);
  for(jsize i = 0; i < len; i++) {
    jobject jpath = reinterpret_cast<jstring>(env->
        GetObjectArrayElement(jpaths, i));
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }
    std::string path = rocksdb::JniUtil::copyString(
        env, static_cast<jstring>(jpath), &has_exception);
    env->DeleteLocalRef(jpath);

    if(has_exception == JNI_TRUE) {
        env->ReleaseLongArrayElements(
            jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
        return;
    }

    jlong jtarget_size = ptr_jtarget_size[i];

    db_paths.push_back(
        rocksdb::DbPath(path, static_cast<uint64_t>(jtarget_size)));
  }

  env->ReleaseLongArrayElements(jtarget_sizes, ptr_jtarget_size, JNI_ABORT);

  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->db_paths = db_paths;
}

/*
 *课程：org_rocksdb_选项
 *方法：dbpathslen
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_dbPathsLen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->db_paths.size());
}

/*
 *课程：org_rocksdb_选项
 *方法：dbpath
 *签名：（j[ljava/lang/string；[j）v
 **/

void Java_org_rocksdb_Options_dbPaths(
    JNIEnv* env, jobject jobj, jlong jhandle, jobjectArray jpaths,
    jlongArray jtarget_sizes) {
  jlong* ptr_jtarget_size = env->GetLongArrayElements(jtarget_sizes, nullptr);
  if(ptr_jtarget_size == nullptr) {
//引发异常：OutOfMemoryError
      return;
  }

  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  const jsize len = env->GetArrayLength(jpaths);
  for(jsize i = 0; i < len; i++) {
    rocksdb::DbPath db_path = opt->db_paths[i];

    jstring jpath = env->NewStringUTF(db_path.path.c_str());
    if(jpath == nullptr) {
//引发异常：OutOfMemoryError
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }
    env->SetObjectArrayElement(jpaths, i, jpath);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(jpath);
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }

    ptr_jtarget_size[i] = static_cast<jint>(db_path.target_size);
  }

  env->ReleaseLongArrayElements(jtarget_sizes, ptr_jtarget_size, JNI_COMMIT);
}

/*
 *课程：org_rocksdb_选项
 *方法：dblogdir
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_Options_dbLogDir(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return env->NewStringUTF(
      reinterpret_cast<rocksdb::Options*>(jhandle)->db_log_dir.c_str());
}

/*
 *课程：org_rocksdb_选项
 *方法：setdblogdir
 *签名：（jljava/lang/string）v
 **/

void Java_org_rocksdb_Options_setDbLogDir(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jdb_log_dir) {
  const char* log_dir = env->GetStringUTFChars(jdb_log_dir, nullptr);
  if(log_dir == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  reinterpret_cast<rocksdb::Options*>(jhandle)->db_log_dir.assign(log_dir);
  env->ReleaseStringUTFChars(jdb_log_dir, log_dir);
}

/*
 *课程：org_rocksdb_选项
 *方法：Waldir
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_Options_walDir(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return env->NewStringUTF(
      reinterpret_cast<rocksdb::Options*>(jhandle)->wal_dir.c_str());
}

/*
 *课程：org_rocksdb_选项
 *方法：setwaldir
 *签名：（jljava/lang/string）v
 **/

void Java_org_rocksdb_Options_setWalDir(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jwal_dir) {
  const char* wal_dir = env->GetStringUTFChars(jwal_dir, nullptr);
  if(wal_dir == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  reinterpret_cast<rocksdb::Options*>(jhandle)->wal_dir.assign(wal_dir);
  env->ReleaseStringUTFChars(jwal_dir, wal_dir);
}

/*
 *课程：org_rocksdb_选项
 *方法：DeleteObsoletefilesPeriodMicros
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_deleteObsoleteFilesPeriodMicros(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->delete_obsolete_files_period_micros;
}

/*
 *课程：org_rocksdb_选项
 *方法：setDeleteObsoletefilesPeriodMicros
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setDeleteObsoleteFilesPeriodMicros(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong micros) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->delete_obsolete_files_period_micros =
          static_cast<int64_t>(micros);
}

/*
 *课程：org_rocksdb_选项
 *方法：setbasebackgroundcompactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setBaseBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->base_background_compactions = static_cast<int>(max);
}

/*
 *课程：org_rocksdb_选项
 *方法：基底压实
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_baseBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->base_background_compactions;
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxBackgroundCompactions
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_background_compactions;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxbackgroundcompactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->max_background_compactions = static_cast<int>(max);
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxsubpactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxSubcompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->max_subcompactions = static_cast<int32_t>(max);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxSubpactions
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxSubcompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_subcompactions;
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxBackgroundFlushes
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxBackgroundFlushes(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_background_flushes;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxbackgroundflushes
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxBackgroundFlushes(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max_background_flushes) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_background_flushes =
      static_cast<int>(max_background_flushes);
}

/*
 *课程：org_rocksdb_选项
 *方法：maxlogfilesize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxLogFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_log_file_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxlogfilesize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxLogFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max_log_file_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(max_log_file_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->max_log_file_size =
        max_log_file_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：logfiletimetoroll
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_logFileTimeToRoll(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->log_file_time_to_roll;
}

/*
 *课程：org_rocksdb_选项
 *方法：setlogfiletimetoroll
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setLogFileTimeToRoll(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong log_file_time_to_roll) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      log_file_time_to_roll);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->log_file_time_to_roll =
        log_file_time_to_roll;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：keeploggfilenum
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_keepLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->keep_log_file_num;
}

/*
 *课程：org_rocksdb_选项
 *方法：setkeeploggfilenum
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setKeepLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong keep_log_file_num) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(keep_log_file_num);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->keep_log_file_num =
        keep_log_file_num;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：RecycleLogFileNum
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_recycleLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->recycle_log_file_num;
}

/*
 *课程：org_rocksdb_选项
 *方法：setRecycleLogFileNum
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setRecycleLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong recycle_log_file_num) {
  rocksdb::Status s =
      rocksdb::check_if_jlong_fits_size_t(recycle_log_file_num);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->recycle_log_file_num =
        recycle_log_file_num;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：maxmanifestfilesize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxManifestFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_manifest_file_size;
}

/*
 *方法：memTableFactoryName
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_Options_memTableFactoryName(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  rocksdb::MemTableRepFactory* tf = opt->memtable_factory.get();

//不应为nullptr。
//默认的memtable工厂是skipListFactory
  assert(tf);

//暂时修复历史错误
  if (strcmp(tf->Name(), "HashLinkListRepFactory") == 0) {
    return env->NewStringUTF("HashLinkedListRepFactory");
  }

  return env->NewStringUTF(tf->Name());
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxmanifestfilesize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxManifestFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max_manifest_file_size) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_manifest_file_size =
      static_cast<int64_t>(max_manifest_file_size);
}

/*
 *方法：setmemtablefactory
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMemTableFactory(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jfactory_handle) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->memtable_factory.reset(
      reinterpret_cast<rocksdb::MemTableRepFactory*>(jfactory_handle));
}

/*
 *课程：org_rocksdb_选项
 *方法：setratelimiter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setRateLimiter(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrate_limiter_handle) {
  std::shared_ptr<rocksdb::RateLimiter> *pRateLimiter =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(
          jrate_limiter_handle);
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      rate_limiter = *pRateLimiter;
}

/*
 *课程：org_rocksdb_选项
 *方法：setlogger
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setLogger(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jlogger_handle) {
std::shared_ptr<rocksdb::LoggerJniCallback> *pLogger =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(
          jlogger_handle);
  reinterpret_cast<rocksdb::Options*>(jhandle)->info_log = *pLogger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setInfologLevel
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setInfoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jlog_level) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->info_log_level =
      static_cast<rocksdb::InfoLogLevel>(jlog_level);
}

/*
 *课程：org_rocksdb_选项
 *方法：信息日志级别
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_infoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return static_cast<jbyte>(
      reinterpret_cast<rocksdb::Options*>(jhandle)->info_log_level);
}

/*
 *课程：org_rocksdb_选项
 *方法：tablecachenumshardbits
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_tableCacheNumshardbits(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->table_cache_numshardbits;
}

/*
 *课程：org_rocksdb_选项
 *方法：可设置cachenumshardbits
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setTableCacheNumshardbits(
    JNIEnv* env, jobject jobj, jlong jhandle, jint table_cache_numshardbits) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->table_cache_numshardbits =
      static_cast<int>(table_cache_numshardbits);
}

/*
 *方法：使用FixedLengthPrefixtractor
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_useFixedLengthPrefixExtractor(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jprefix_length) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->prefix_extractor.reset(
      rocksdb::NewFixedPrefixTransform(
          static_cast<int>(jprefix_length)));
}

/*
 *方法：使用cappedPrefixtractor
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_useCappedPrefixExtractor(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jprefix_length) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->prefix_extractor.reset(
      rocksdb::NewCappedPrefixTransform(
          static_cast<int>(jprefix_length)));
}

/*
 *课程：org_rocksdb_选项
 *方法：Walttlseconds
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_walTtlSeconds(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->WAL_ttl_seconds;
}

/*
 *课程：org_rocksdb_选项
 *方法：setwalttlseconds
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWalTtlSeconds(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong WAL_ttl_seconds) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->WAL_ttl_seconds =
      static_cast<int64_t>(WAL_ttl_seconds);
}

/*
 *课程：org_rocksdb_选项
 *方法：Walttlseconds
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_walSizeLimitMB(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->WAL_size_limit_MB;
}

/*
 *课程：org_rocksdb_选项
 *方法：setwalsizelimitmb
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWalSizeLimitMB(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong WAL_size_limit_MB) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->WAL_size_limit_MB =
      static_cast<int64_t>(WAL_size_limit_MB);
}

/*
 *课程：org_rocksdb_选项
 *方法：manifestPreallocationSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_manifestPreallocationSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->manifest_preallocation_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：setManifestPreallocationSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setManifestPreallocationSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong preallocation_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(preallocation_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->manifest_preallocation_size =
        preallocation_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *方法：设置工厂
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setTableFactory(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jfactory_handle) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->table_factory.reset(
      reinterpret_cast<rocksdb::TableFactory*>(jfactory_handle));
}

/*
 *课程：org_rocksdb_选项
 *方法：allowmapreads
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_allowMmapReads(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->allow_mmap_reads;
}

/*
 *课程：org_rocksdb_选项
 *方法：setallowmapreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAllowMmapReads(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow_mmap_reads) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->allow_mmap_reads =
      static_cast<bool>(allow_mmap_reads);
}

/*
 *课程：org_rocksdb_选项
 *方法：allowmapwrites
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_allowMmapWrites(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->allow_mmap_writes;
}

/*
 *课程：org_rocksdb_选项
 *方法：setallowmapwrites
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAllowMmapWrites(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow_mmap_writes) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->allow_mmap_writes =
      static_cast<bool>(allow_mmap_writes);
}

/*
 *课程：org_rocksdb_选项
 *方法：使用直接读取
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_useDirectReads(JNIEnv* env, jobject jobj,
                                                 jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->use_direct_reads;
}

/*
 *课程：org_rocksdb_选项
 *方法：setusedirectreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setUseDirectReads(JNIEnv* env, jobject jobj,
                                                jlong jhandle,
                                                jboolean use_direct_reads) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->use_direct_reads =
      static_cast<bool>(use_direct_reads);
}

/*
 *课程：org_rocksdb_选项
 *方法：直接用于Flushandcompression
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_useDirectIoForFlushAndCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->use_direct_io_for_flush_and_compaction;
}

/*
 *课程：org_rocksdb_选项
 *方法：setusedirectioforflushandcompression
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setUseDirectIoForFlushAndCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean use_direct_io_for_flush_and_compaction) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->use_direct_io_for_flush_and_compaction =
      static_cast<bool>(use_direct_io_for_flush_and_compaction);
}

/*
 *课程：org_rocksdb_选项
 *方法：setallowflocate
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAllowFAllocate(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_fallocate) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->allow_fallocate =
      static_cast<bool>(jallow_fallocate);
}

/*
 *课程：org_rocksdb_选项
 *方法：allowflocate
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_allowFAllocate(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->allow_fallocate);
}

/*
 *课程：org_rocksdb_选项
 *方法：isfdcloseonexec
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_isFdCloseOnExec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->is_fd_close_on_exec;
}

/*
 *课程：org_rocksdb_选项
 *方法：setisfdcloseonexec
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setIsFdCloseOnExec(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean is_fd_close_on_exec) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->is_fd_close_on_exec =
      static_cast<bool>(is_fd_close_on_exec);
}

/*
 *课程：org_rocksdb_选项
 *方法：StatsDumpPeriodSec
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_statsDumpPeriodSec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->stats_dump_period_sec;
}

/*
 *课程：org_rocksdb_选项
 *方法：SetStatesDumpPeriodSec
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setStatsDumpPeriodSec(
    JNIEnv* env, jobject jobj, jlong jhandle, jint stats_dump_period_sec) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->stats_dump_period_sec =
      static_cast<int>(stats_dump_period_sec);
}

/*
 *课程：org_rocksdb_选项
 *方法：阿维司兰多芬
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_adviseRandomOnOpen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->advise_random_on_open;
}

/*
 *课程：org_rocksdb_选项
 *方法：setadviserandomonopen
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAdviseRandomOnOpen(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean advise_random_on_open) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->advise_random_on_open =
      static_cast<bool>(advise_random_on_open);
}

/*
 *课程：org_rocksdb_选项
 *方法：setdbwritebuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setDbWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jdb_write_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->db_write_buffer_size = static_cast<size_t>(jdb_write_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：dbWriteBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_dbWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->db_write_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：setAccessHintonCompactionStart
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setAccessHintOnCompactionStart(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jaccess_hint_value) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->access_hint_on_compaction_start =
      rocksdb::AccessHintJni::toCppAccessHint(jaccess_hint_value);
}

/*
 *课程：org_rocksdb_选项
 *方法：accessHintonCompactionStart
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_accessHintOnCompactionStart(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb::AccessHintJni::toJavaAccessHint(
      opt->access_hint_on_compaction_start);
}

/*
 *课程：org_rocksdb_选项
 *方法：setnewtablereaderforcompactioninputs
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setNewTableReaderForCompactionInputs(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jnew_table_reader_for_compaction_inputs) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->new_table_reader_for_compaction_inputs =
      static_cast<bool>(jnew_table_reader_for_compaction_inputs);
}

/*
 *课程：org_rocksdb_选项
 *方法：NewTableReaderForCompactionInputs
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_newTableReaderForCompactionInputs(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<bool>(opt->new_table_reader_for_compaction_inputs);
}

/*
 *课程：org_rocksdb_选项
 *方法：setcompactionreadaheadsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setCompactionReadaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jcompaction_readahead_size) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->compaction_readahead_size =
      static_cast<size_t>(jcompaction_readahead_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：压缩读取标题大小
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_compactionReadaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->compaction_readahead_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：setrandomaaccessmaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setRandomAccessMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jrandom_access_max_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->random_access_max_buffer_size =
      static_cast<size_t>(jrandom_access_max_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：RandomAccessMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_randomAccessMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->random_access_max_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：setwritablefilemaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWritableFileMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jwritable_file_max_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->writable_file_max_buffer_size =
      static_cast<size_t>(jwritable_file_max_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：WritableFileMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_writableFileMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->writable_file_max_buffer_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：useadaptivemutex
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_useAdaptiveMutex(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->use_adaptive_mutex;
}

/*
 *课程：org_rocksdb_选项
 *方法：setuseadaptivemutex
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setUseAdaptiveMutex(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean use_adaptive_mutex) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->use_adaptive_mutex =
      static_cast<bool>(use_adaptive_mutex);
}

/*
 *课程：org_rocksdb_选项
 *方法：bytespersync
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_bytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->bytes_per_sync;
}

/*
 *课程：org_rocksdb_选项
 *方法：setbytespersync
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong bytes_per_sync) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->bytes_per_sync =
      static_cast<int64_t>(bytes_per_sync);
}

/*
 *课程：org_rocksdb_选项
 *方法：setwalbytespersync
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWalBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jwal_bytes_per_sync) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->wal_bytes_per_sync =
      static_cast<int64_t>(jwal_bytes_per_sync);
}

/*
 *课程：org_rocksdb_选项
 *方法：walbytespersync
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_walBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->wal_bytes_per_sync);
}

/*
 *课程：org_rocksdb_选项
 *方法：setEnableThreadTracking
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setEnableThreadTracking(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jenable_thread_tracking) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->enable_thread_tracking = static_cast<bool>(jenable_thread_tracking);
}

/*
 *课程：org_rocksdb_选项
 *方法：启用线程跟踪
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_enableThreadTracking(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->enable_thread_tracking);
}

/*
 *课程：org_rocksdb_选项
 *方法：setDelayedWriter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setDelayedWriteRate(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jdelayed_write_rate) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->delayed_write_rate = static_cast<uint64_t>(jdelayed_write_rate);
}

/*
 *课程：org_rocksdb_选项
 *方法：延迟写入
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_delayedWriteRate(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jlong>(opt->delayed_write_rate);
}

/*
 *课程：org_rocksdb_选项
 *方法：setallowConcurrentMemTableWrite
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAllowConcurrentMemtableWrite(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      allow_concurrent_memtable_write = static_cast<bool>(allow);
}

/*
 *课程：org_rocksdb_选项
 *方法：allowConcurrentMemTableWrite
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_allowConcurrentMemtableWrite(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->
      allow_concurrent_memtable_write;
}

/*
 *课程：org_rocksdb_选项
 *方法：setEnableWriteThreadadaptiveyField
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setEnableWriteThreadAdaptiveYield(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean yield) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      enable_write_thread_adaptive_yield = static_cast<bool>(yield);
}

/*
 *课程：org_rocksdb_选项
 *方法：EnableWriteThreadadaptiveyField
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_enableWriteThreadAdaptiveYield(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->
      enable_write_thread_adaptive_yield;
}

/*
 *课程：org_rocksdb_选项
 *方法：setwritethreadmaxyieldusec
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWriteThreadMaxYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      write_thread_max_yield_usec = static_cast<int64_t>(max);
}

/*
 *课程：org_rocksdb_选项
 *方法：writethreadmaxyieldusec
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_writeThreadMaxYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->
      write_thread_max_yield_usec;
}

/*
 
 *方法：setwritethreadslowyieldusec
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setWriteThreadSlowYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong slow) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      write_thread_slow_yield_usec = static_cast<int64_t>(slow);
}

/*
 *课程：org_rocksdb_选项
 *方法：writethreadslowyieldusec
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_writeThreadSlowYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->
      write_thread_slow_yield_usec;
}

/*
 *课程：org_rocksdb_选项
 *方法：setskipstatsupdateondbopen
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setSkipStatsUpdateOnDbOpen(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jskip_stats_update_on_db_open) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->skip_stats_update_on_db_open =
      static_cast<bool>(jskip_stats_update_on_db_open);
}

/*
 *课程：org_rocksdb_选项
 *方法：skipstatsupdateondbopen
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_skipStatsUpdateOnDbOpen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->skip_stats_update_on_db_open);
}

/*
 *课程：org_rocksdb_选项
 *方法：setwalrecoverymode
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setWalRecoveryMode(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jwal_recovery_mode_value) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->wal_recovery_mode =
      rocksdb::WALRecoveryModeJni::toCppWALRecoveryMode(
          jwal_recovery_mode_value);
}

/*
 *课程：org_rocksdb_选项
 *方法：Walrecoverymode
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_walRecoveryMode(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb::WALRecoveryModeJni::toJavaWALRecoveryMode(
      opt->wal_recovery_mode);
}

/*
 *课程：org_rocksdb_选项
 *方法：setallow2pc
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAllow2pc(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_2pc) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->allow_2pc = static_cast<bool>(jallow_2pc);
}

/*
 *课程：org_rocksdb_选项
 *方法：allow2pc
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_allow2pc(JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->allow_2pc);
}

/*
 *课程：org_rocksdb_选项
 *方法：setrowcache
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setRowCache(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrow_cache_handle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  auto* row_cache = reinterpret_cast<std::shared_ptr<rocksdb::Cache>*>(jrow_cache_handle);
  opt->row_cache = *row_cache;
}

/*
 *课程：org_rocksdb_选项
 *方法：setfailifoptionsfileerror
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setFailIfOptionsFileError(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jfail_if_options_file_error) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->fail_if_options_file_error =
      static_cast<bool>(jfail_if_options_file_error);
}

/*
 *课程：org_rocksdb_选项
 *方法：failifoptionsfileerror
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_failIfOptionsFileError(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->fail_if_options_file_error);
}

/*
 *课程：org_rocksdb_选项
 *方法：setdumpmallocstats
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setDumpMallocStats(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jdump_malloc_stats) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->dump_malloc_stats = static_cast<bool>(jdump_malloc_stats);
}

/*
 *课程：org_rocksdb_选项
 *方法：dumpmallocstats
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_dumpMallocStats(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->dump_malloc_stats);
}

/*
 *课程：org_rocksdb_选项
 *方法：setavoiddflushduringrecovery
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAvoidFlushDuringRecovery(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean javoid_flush_during_recovery) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->avoid_flush_during_recovery = static_cast<bool>(javoid_flush_during_recovery);
}

/*
 *课程：org_rocksdb_选项
 *方法：避免在恢复过程中冲洗。
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_avoidFlushDuringRecovery(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->avoid_flush_during_recovery);
}

/*
 *课程：org_rocksdb_选项
 *方法：setavoidFlushDuringShutdown
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setAvoidFlushDuringShutdown(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean javoid_flush_during_shutdown) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->avoid_flush_during_shutdown = static_cast<bool>(javoid_flush_during_shutdown);
}

/*
 *课程：org_rocksdb_选项
 *方法：避免在关机时冲洗。
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_avoidFlushDuringShutdown(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<jboolean>(opt->avoid_flush_during_shutdown);
}

/*
 *方法：TableFactoryName
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_Options_tableFactoryName(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  rocksdb::TableFactory* tf = opt->table_factory.get();

//不应为nullptr。
//默认的memtable工厂是skipListFactory
  assert(tf);

  return env->NewStringUTF(tf->Name());
}


/*
 *课程：org_rocksdb_选项
 *方法：MinWriteBufferNumberToMerge
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_minWriteBufferNumberToMerge(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->min_write_buffer_number_to_merge;
}

/*
 *课程：org_rocksdb_选项
 *方法：setMinWriteBufferNumberToMerge
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMinWriteBufferNumberToMerge(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jmin_write_buffer_number_to_merge) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->min_write_buffer_number_to_merge =
          static_cast<int>(jmin_write_buffer_number_to_merge);
}
/*
 *课程：org_rocksdb_选项
 *方法：MaxWriteBufferNumberToMaintain
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_maxWriteBufferNumberToMaintain(JNIEnv* env,
                                                             jobject jobj,
                                                             jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->max_write_buffer_number_to_maintain;
}

/*
 *课程：org_rocksdb_选项
 *方法：setMaxWriteBufferNumberToMaintain
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxWriteBufferNumberToMaintain(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jmax_write_buffer_number_to_maintain) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->max_write_buffer_number_to_maintain =
      static_cast<int>(jmax_write_buffer_number_to_maintain);
}

/*
 *课程：org_rocksdb_选项
 *方法：setcompressionType
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jcompression_type_value) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  opts->compression = rocksdb::CompressionTypeJni::toCppCompressionType(
      jcompression_type_value);
}

/*
 *课程：org_rocksdb_选项
 *方法：压缩类型
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_compressionType(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb::CompressionTypeJni::toJavaCompressionType(
      opts->compression);
}

/*
 *帮助压缩压缩级别的Java字节数组的Help方法
 *到ROCKSDB的C++向量：：压缩类型
 *
 *@ PARAM Env指向Java环境的指针
 *@ PARAM-JPultStudio级别对Java字节数组的引用
 *其中每个字节表示压缩级别
 *
 *@返回一个唯一的指针到向量，或者如果发生JNI异常，返回唯一的指针（nullptr）
 **/

std::unique_ptr<std::vector<rocksdb::CompressionType>> rocksdb_compression_vector_helper(
    JNIEnv* env, jbyteArray jcompression_levels) {
  jsize len = env->GetArrayLength(jcompression_levels);
  jbyte* jcompression_level =
      env->GetByteArrayElements(jcompression_levels, nullptr);
  if(jcompression_level == nullptr) {
//引发异常：OutOfMemoryError
    return std::unique_ptr<std::vector<rocksdb::CompressionType>>();
  }

  auto* compression_levels = new std::vector<rocksdb::CompressionType>();
  std::unique_ptr<std::vector<rocksdb::CompressionType>> uptr_compression_levels(compression_levels);

  for(jsize i = 0; i < len; i++) {
    jbyte jcl = jcompression_level[i];
    compression_levels->push_back(static_cast<rocksdb::CompressionType>(jcl));
  }

  env->ReleaseByteArrayElements(jcompression_levels, jcompression_level,
      JNI_ABORT);

  return uptr_compression_levels;
}

/*
 *帮助转换ROCSDB:压缩类型的C++向量的辅助方法
 *一个压缩字节的Java字节数组
 *
 *@ PARAM Env指向Java环境的指针
 *@ PARAM-JPultStudio级别对Java字节数组的引用
 *其中每个字节表示压缩级别
 *
 *@如果发生异常，返回jbytearray或nullptr
 **/

jbyteArray rocksdb_compression_list_helper(JNIEnv* env,
    std::vector<rocksdb::CompressionType> compression_levels) {
  const size_t len = compression_levels.size();
  jbyte* jbuf = new jbyte[len];

  for (size_t i = 0; i < len; i++) {
      jbuf[i] = compression_levels[i];
  }

//在Java数组中插入
  jbyteArray jcompression_levels = env->NewByteArray(static_cast<jsize>(len));
  if(jcompression_levels == nullptr) {
//引发异常：OutOfMemoryError
      delete [] jbuf;
      return nullptr;
  }
  env->SetByteArrayRegion(jcompression_levels, 0, static_cast<jsize>(len),
      jbuf);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(jcompression_levels);
      delete [] jbuf;
      return nullptr;
  }

  delete [] jbuf;

  return jcompression_levels;
}

/*
 *课程：org_rocksdb_选项
 *方法：setcompressionPerLevel
 *签字：（J[B）V
 **/

void Java_org_rocksdb_Options_setCompressionPerLevel(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jbyteArray jcompressionLevels) {
  auto uptr_compression_levels =
      rocksdb_compression_vector_helper(env, jcompressionLevels);
  if(!uptr_compression_levels) {
//发生异常
    return;
  }
  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  options->compression_per_level = *(uptr_compression_levels.get());
}

/*
 *课程：org_rocksdb_选项
 *方法：压缩级别
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_Options_compressionPerLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb_compression_list_helper(env,
      options->compression_per_level);
}

/*
 *课程：org_rocksdb_选项
 *方法：setBottomMostCompressionType
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setBottommostCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jcompression_type_value) {
  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  options->bottommost_compression =
      rocksdb::CompressionTypeJni::toCppCompressionType(
          jcompression_type_value);
}

/*
 *课程：org_rocksdb_选项
 *方法：BottomMostCompressionType
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_bottommostCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb::CompressionTypeJni::toJavaCompressionType(
      options->bottommost_compression);
}

/*
 *课程：org_rocksdb_选项
 *方法：设置压缩选项
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setCompressionOptions(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jcompression_options_handle) {
  auto* options = reinterpret_cast<rocksdb::Options*>(jhandle);
  auto* compression_options =
      reinterpret_cast<rocksdb::CompressionOptions*>(jcompression_options_handle);
  options->compression_opts = *compression_options;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置压缩样式
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setCompactionStyle(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte compaction_style) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->compaction_style =
      static_cast<rocksdb::CompactionStyle>(compaction_style);
}

/*
 *课程：org_rocksdb_选项
 *方法：压缩样式
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_compactionStyle(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->compaction_style;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxtablefilesizefifo
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxTableFilesSizeFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jmax_table_files_size) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->compaction_options_fifo.max_table_files_size =
    static_cast<uint64_t>(jmax_table_files_size);
}

/*
 *课程：org_rocksdb_选项
 *方法：maxTableFileSizeInfo
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxTableFilesSizeFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->compaction_options_fifo.max_table_files_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：numlevels
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_numLevels(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->num_levels;
}

/*
 *课程：org_rocksdb_选项
 *方法：setNumLevels
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setNumLevels(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jnum_levels) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->num_levels =
      static_cast<int>(jnum_levels);
}

/*
 *课程：org_rocksdb_选项
 *方法：LevelZeroFileNumCompactionTrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_levelZeroFileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_file_num_compaction_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevelZeroFileNumCompactionTrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevelZeroFileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_file_num_compaction_trigger) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_file_num_compaction_trigger =
          static_cast<int>(jlevel0_file_num_compaction_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：零级减速器
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_levelZeroSlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_slowdown_writes_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setlevelslowdownwritestrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevelZeroSlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_slowdown_writes_trigger) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_slowdown_writes_trigger =
          static_cast<int>(jlevel0_slowdown_writes_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：LevelZeroStopWriteStrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_levelZeroStopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_stop_writes_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevelStopWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevelZeroStopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_stop_writes_trigger) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->level0_stop_writes_trigger =
      static_cast<int>(jlevel0_stop_writes_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：targetfilesizebase
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_targetFileSizeBase(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->target_file_size_base;
}

/*
 *课程：org_rocksdb_选项
 *方法：settargetfilesizebase
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setTargetFileSizeBase(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jtarget_file_size_base) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->target_file_size_base =
      static_cast<uint64_t>(jtarget_file_size_base);
}

/*
 *课程：org_rocksdb_选项
 *方法：TargetFileSize乘数
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_targetFileSizeMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->target_file_size_multiplier;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置目标文件大小乘数
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setTargetFileSizeMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jtarget_file_size_multiplier) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->target_file_size_multiplier =
          static_cast<int>(jtarget_file_size_multiplier);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxBytesForLevelBase
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxBytesForLevelBase(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_bytes_for_level_base;
}

/*
 *课程：org_rocksdb_选项
 *方法：setMaxBytesForLevelBase
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxBytesForLevelBase(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_bytes_for_level_base) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_bytes_for_level_base =
          static_cast<int64_t>(jmax_bytes_for_level_base);
}

/*
 *课程：org_rocksdb_选项
 *方法：LevelCompactionDynamicClevelBytes
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_levelCompactionDynamicLevelBytes(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->level_compaction_dynamic_level_bytes;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevelCompactionDynamicClevelBytes
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setLevelCompactionDynamicLevelBytes(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jenable_dynamic_level_bytes) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level_compaction_dynamic_level_bytes =
          (jenable_dynamic_level_bytes);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxBytesForLevelMultiplier
 *签名：（j）d
 **/

jdouble Java_org_rocksdb_Options_maxBytesForLevelMultiplier(JNIEnv* env,
                                                            jobject jobj,
                                                            jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_bytes_for_level_multiplier;
}

/*
 *课程：org_rocksdb_选项
 *方法：SetMaxBytesForLevelMultiplier
 *签字：（JD）V
 **/

void Java_org_rocksdb_Options_setMaxBytesForLevelMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jdouble jmax_bytes_for_level_multiplier) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_bytes_for_level_multiplier =
      static_cast<double>(jmax_bytes_for_level_multiplier);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxCompactionBytes
 *签字：（j）I
 **/

jlong Java_org_rocksdb_Options_maxCompactionBytes(JNIEnv* env, jobject jobj,
                                                  jlong jhandle) {
  return static_cast<jlong>(
      reinterpret_cast<rocksdb::Options*>(jhandle)->max_compaction_bytes);
}

/*
 *课程：org_rocksdb_选项
 *方法：setMaxCompactionBytes
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMaxCompactionBytes(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jmax_compaction_bytes) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->max_compaction_bytes =
      static_cast<uint64_t>(jmax_compaction_bytes);
}

/*
 *课程：org_rocksdb_选项
 *方法：arenablocksize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_arenaBlockSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->arena_block_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：setarenablocksize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setArenaBlockSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jarena_block_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jarena_block_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->arena_block_size =
        jarena_block_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：禁用自动压缩
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_disableAutoCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->disable_auto_compactions;
}

/*
 *课程：org_rocksdb_选项
 *方法：setDisableAutoCompactions
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setDisableAutoCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jdisable_auto_compactions) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->disable_auto_compactions =
          static_cast<bool>(jdisable_auto_compactions);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxSequentialsKipinIterations
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxSequentialSkipInIterations(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_sequential_skip_in_iterations;
}

/*
 *课程：org_rocksdb_选项
 *方法：setMaxSequentialsKipinIterations
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxSequentialSkipInIterations(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_sequential_skip_in_iterations) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->max_sequential_skip_in_iterations =
          static_cast<int64_t>(jmax_sequential_skip_in_iterations);
}

/*
 *课程：org_rocksdb_选项
 *方法：inplaceUpdateSupport
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_inplaceUpdateSupport(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->inplace_update_support;
}

/*
 *课程：org_rocksdb_选项
 *方法：setinplaceupdatesupport
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setInplaceUpdateSupport(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jinplace_update_support) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->inplace_update_support =
          static_cast<bool>(jinplace_update_support);
}

/*
 *课程：org_rocksdb_选项
 *方法：inplaceUpdateNumLocks
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_inplaceUpdateNumLocks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->inplace_update_num_locks;
}

/*
 *课程：org_rocksdb_选项
 *方法：setInplaceUpdateNumLocks
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setInplaceUpdateNumLocks(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jinplace_update_num_locks) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jinplace_update_num_locks);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->inplace_update_num_locks =
        jinplace_update_num_locks;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：MEMTablePrefixBloomSizeratio
 *签字：（j）I
 **/

jdouble Java_org_rocksdb_Options_memtablePrefixBloomSizeRatio(JNIEnv* env,
                                                              jobject jobj,
                                                              jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)
      ->memtable_prefix_bloom_size_ratio;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmemtableprefixbloomsizeratio
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setMemtablePrefixBloomSizeRatio(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jdouble jmemtable_prefix_bloom_size_ratio) {
  reinterpret_cast<rocksdb::Options*>(jhandle)
      ->memtable_prefix_bloom_size_ratio =
      static_cast<double>(jmemtable_prefix_bloom_size_ratio);
}

/*
 *课程：org_rocksdb_选项
 *方法：BloomLocality
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_bloomLocality(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->bloom_locality;
}

/*
 *课程：org_rocksdb_选项
 *方法：setBloomLocality
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setBloomLocality(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jbloom_locality) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->bloom_locality =
      static_cast<int32_t>(jbloom_locality);
}

/*
 *课程：org_rocksdb_选项
 *方法：MaxSuccessiveMerges
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_maxSuccessiveMerges(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(jhandle)->max_successive_merges;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmaxsuccessivemerges
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMaxSuccessiveMerges(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_successive_merges) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jmax_successive_merges);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(jhandle)->max_successive_merges =
        jmax_successive_merges;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：优化过滤器
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_optimizeFiltersForHits(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->optimize_filters_for_hits;
}

/*
 *课程：org_rocksdb_选项
 *方法：setoptimizefiltersforhits
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setOptimizeFiltersForHits(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean joptimize_filters_for_hits) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->optimize_filters_for_hits =
          static_cast<bool>(joptimize_filters_for_hits);
}

/*
 *课程：org_rocksdb_选项
 *方法：优化malldb
 *签名：（j）v
 **/

void Java_org_rocksdb_Options_optimizeForSmallDb(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->OptimizeForSmallDb();
}

/*
 *课程：org_rocksdb_选项
 *方法：优化点查找
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_optimizeForPointLookup(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong block_cache_size_mb) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      OptimizeForPointLookup(block_cache_size_mb);
}

/*
 *课程：org_rocksdb_选项
 *方法：优化级别样式比较
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_optimizeLevelStyleCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong memtable_memory_budget) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      OptimizeLevelStyleCompaction(memtable_memory_budget);
}

/*
 *课程：org_rocksdb_选项
 *方法：优化通用样式压缩
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_optimizeUniversalStyleCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong memtable_memory_budget) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      OptimizeUniversalStyleCompaction(memtable_memory_budget);
}

/*
 *课程：org_rocksdb_选项
 *方法：准备散装货物
 *签名：（j）v
 **/

void Java_org_rocksdb_Options_prepareForBulkLoad(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  reinterpret_cast<rocksdb::Options*>(jhandle)->
      PrepareForBulkLoad();
}

/*
 *课程：org_rocksdb_选项
 *方法：memtablehugepageSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_memtableHugePageSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->memtable_huge_page_size;
}

/*
 *课程：org_rocksdb_选项
 *方法：setmemtablehugepageSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setMemtableHugePageSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmemtable_huge_page_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jmemtable_huge_page_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::Options*>(
        jhandle)->memtable_huge_page_size =
            jmemtable_huge_page_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *课程：org_rocksdb_选项
 *方法：SoftPendingCompactionBytesLimit
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_softPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->soft_pending_compaction_bytes_limit;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置PendingCompactionBytesLimit
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setSoftPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jsoft_pending_compaction_bytes_limit) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->soft_pending_compaction_bytes_limit =
          static_cast<int64_t>(jsoft_pending_compaction_bytes_limit);
}

/*
 *课程：org_rocksdb_选项
 *方法：软硬件压缩字节限制
 *签字：（j）j
 **/

jlong Java_org_rocksdb_Options_hardPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->hard_pending_compaction_bytes_limit;
}

/*
 *课程：org_rocksdb_选项
 *方法：sethardpendingcompactionbyteslimit
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setHardPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jhard_pending_compaction_bytes_limit) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->hard_pending_compaction_bytes_limit =
          static_cast<int64_t>(jhard_pending_compaction_bytes_limit);
}

/*
 *课程：org_rocksdb_选项
 *方法：Level0FileNumCompactionTrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_level0FileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
    jhandle)->level0_file_num_compaction_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevel0fileNumCompactionTrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevel0FileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_file_num_compaction_trigger) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_file_num_compaction_trigger =
          static_cast<int32_t>(jlevel0_file_num_compaction_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：0级减速器
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_level0SlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
    jhandle)->level0_slowdown_writes_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevel0 SlowDownWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevel0SlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_slowdown_writes_trigger) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_slowdown_writes_trigger =
          static_cast<int32_t>(jlevel0_slowdown_writes_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：Level0StopWriteStrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_Options_level0StopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
    jhandle)->level0_stop_writes_trigger;
}

/*
 *课程：org_rocksdb_选项
 *方法：setLevel0stopWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_Options_setLevel0StopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_stop_writes_trigger) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->level0_stop_writes_trigger =
          static_cast<int32_t>(jlevel0_stop_writes_trigger);
}

/*
 *课程：org_rocksdb_选项
 *方法：最大字节数用于电平多路附加
 *签字：（j）[i
 **/

jintArray Java_org_rocksdb_Options_maxBytesForLevelMultiplierAdditional(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto mbflma =
      reinterpret_cast<rocksdb::Options*>(jhandle)->
          max_bytes_for_level_multiplier_additional;

  const size_t size = mbflma.size();

  jint* additionals = new jint[size];
  for (size_t i = 0; i < size; i++) {
    additionals[i] = static_cast<jint>(mbflma[i]);
  }

  jsize jlen = static_cast<jsize>(size);
  jintArray result = env->NewIntArray(jlen);
  if(result == nullptr) {
//引发异常：OutOfMemoryError
      delete [] additionals;
      return nullptr;
  }

  env->SetIntArrayRegion(result, 0, jlen, additionals);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(result);
      delete [] additionals;
      return nullptr;
  }

  delete [] additionals;

  return result;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置最大字节数，用于电平多路附加
 *签字：（J[I）V
 **/

void Java_org_rocksdb_Options_setMaxBytesForLevelMultiplierAdditional(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jintArray jmax_bytes_for_level_multiplier_additional) {
  jsize len = env->GetArrayLength(jmax_bytes_for_level_multiplier_additional);
  jint *additionals =
      env->GetIntArrayElements(jmax_bytes_for_level_multiplier_additional, nullptr);
  if(additionals == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  auto* opt = reinterpret_cast<rocksdb::Options*>(jhandle);
  opt->max_bytes_for_level_multiplier_additional.clear();
  for (jsize i = 0; i < len; i++) {
    opt->max_bytes_for_level_multiplier_additional.push_back(static_cast<int32_t>(additionals[i]));
  }

  env->ReleaseIntArrayElements(jmax_bytes_for_level_multiplier_additional,
      additionals, JNI_ABORT);
}

/*
 *课程：org_rocksdb_选项
 *方法：偏执文件检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_paranoidFileChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::Options*>(
      jhandle)->paranoid_file_checks;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置偏执文件检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setParanoidFileChecks(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jparanoid_file_checks) {
  reinterpret_cast<rocksdb::Options*>(
      jhandle)->paranoid_file_checks =
          static_cast<bool>(jparanoid_file_checks);
}

/*
 *课程：org_rocksdb_选项
 *方法：setCompactionPriority
 *签字：（JB）V
 **/

void Java_org_rocksdb_Options_setCompactionPriority(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jbyte jcompaction_priority_value) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  opts->compaction_pri =
      rocksdb::CompactionPriorityJni::toCppCompactionPriority(jcompaction_priority_value);
}

/*
 *课程：org_rocksdb_选项
 *方法：压缩优先级
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_Options_compactionPriority(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  return rocksdb::CompactionPriorityJni::toJavaCompactionPriority(
      opts->compaction_pri);
}

/*
 *课程：org_rocksdb_选项
 *方法：setreportbgiostats
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setReportBgIoStats(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jreport_bg_io_stats) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  opts->report_bg_io_stats = static_cast<bool>(jreport_bg_io_stats);
}

/*
 *课程：org_rocksdb_选项
 *方法：reportbgiostats
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_reportBgIoStats(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<bool>(opts->report_bg_io_stats);
}

/*
 *课程：org_rocksdb_选项
 *方法：设置压缩选项通用
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setCompactionOptionsUniversal(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jcompaction_options_universal_handle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  auto* opts_uni =
      reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(
          jcompaction_options_universal_handle);
  opts->compaction_options_universal = *opts_uni;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置压缩选项FIFO
 *签字：（JJ）V
 **/

void Java_org_rocksdb_Options_setCompactionOptionsFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jcompaction_options_fifo_handle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  auto* opts_fifo =
      reinterpret_cast<rocksdb::CompactionOptionsFIFO*>(
          jcompaction_options_fifo_handle);
  opts->compaction_options_fifo = *opts_fifo;
}

/*
 *课程：org_rocksdb_选项
 *方法：设置强制一致性检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_Options_setForceConsistencyChecks(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jforce_consistency_checks) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  opts->force_consistency_checks = static_cast<bool>(jforce_consistency_checks);
}

/*
 *课程：org_rocksdb_选项
 *方法：强制一致性检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_Options_forceConsistencyChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opts = reinterpret_cast<rocksdb::Options*>(jhandle);
  return static_cast<bool>(opts->force_consistency_checks);
}

//////////////////////////////////////////////////
//RocksDB：：列系列选项

/*
 *类别：组织rocksdb列系列选项
 *方法：NewColumnFamilyOptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_newColumnFamilyOptions(
    JNIEnv* env, jclass jcls) {
  auto* op = new rocksdb::ColumnFamilyOptions();
  return reinterpret_cast<jlong>(op);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：从Props获取列族选项
 *签名：（ljava/util/string；）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_getColumnFamilyOptionsFromProps(
    JNIEnv* env, jclass jclazz, jstring jopt_string) {
  const char* opt_string = env->GetStringUTFChars(jopt_string, nullptr);
  if(opt_string == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  auto* cf_options = new rocksdb::ColumnFamilyOptions();
  rocksdb::Status status = rocksdb::GetColumnFamilyOptionsFromString(
      rocksdb::ColumnFamilyOptions(), opt_string, cf_options);

  env->ReleaseStringUTFChars(jopt_string, opt_string);

//检查是否可以创建ColumnFamilyOptions。
  jlong ret_value = 0;
  if (status.ok()) {
    ret_value = reinterpret_cast<jlong>(cf_options);
  } else {
//如果操作失败，则需要删除ColumnFamilyOptions
//再次防止内存泄漏。
    delete cf_options;
  }
  return ret_value;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_ColumnFamilyOptions_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* cfo = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(handle);
  assert(cfo != nullptr);
  delete cfo;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：优化malldb
 *签名：（j）v
 **/

void Java_org_rocksdb_ColumnFamilyOptions_optimizeForSmallDb(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      OptimizeForSmallDb();
}

/*
 *类别：组织rocksdb列系列选项
 *方法：优化点查找
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_optimizeForPointLookup(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong block_cache_size_mb) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      OptimizeForPointLookup(block_cache_size_mb);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：优化级别样式比较
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_optimizeLevelStyleCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong memtable_memory_budget) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      OptimizeLevelStyleCompaction(memtable_memory_budget);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：优化通用样式压缩
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_optimizeUniversalStyleCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong memtable_memory_budget) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      OptimizeUniversalStyleCompaction(memtable_memory_budget);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setComparatorHandle
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setComparatorHandle__JI(
    JNIEnv* env, jobject jobj, jlong jhandle, jint builtinComparator) {
  switch (builtinComparator) {
    case 1:
      reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->comparator =
          rocksdb::ReverseBytewiseComparator();
      break;
    default:
      reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->comparator =
          rocksdb::BytewiseComparator();
      break;
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setComparatorHandle
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setComparatorHandle__JJ(
    JNIEnv* env, jobject jobj, jlong jopt_handle, jlong jcomparator_handle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jopt_handle)->comparator =
      reinterpret_cast<rocksdb::Comparator*>(jcomparator_handle);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmergeoperatorname
 *签名：（jjjava/lang/string）v
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMergeOperatorName(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jop_name) {
  auto* options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  const char* op_name = env->GetStringUTFChars(jop_name, nullptr);
  if(op_name == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  options->merge_operator =
      rocksdb::MergeOperators::CreateFromStringId(op_name);
  env->ReleaseStringUTFChars(jop_name, op_name);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmergeoperator
 *签名：（jjjava/lang/string）v
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMergeOperator(
  JNIEnv* env, jobject jobj, jlong jhandle, jlong mergeOperatorHandle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->merge_operator =
    *(reinterpret_cast<std::shared_ptr<rocksdb::MergeOperator>*>
      (mergeOperatorHandle));
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setcompactionfilterhandle
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompactionFilterHandle(
    JNIEnv* env, jobject jobj, jlong jopt_handle,
    jlong jcompactionfilter_handle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jopt_handle)->
      compaction_filter = reinterpret_cast<rocksdb::CompactionFilter*>
        (jcompactionfilter_handle);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setWriteBufferSize
 *签字：（JJ）I
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jwrite_buffer_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jwrite_buffer_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
        write_buffer_size = jwrite_buffer_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：WriteBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_writeBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      write_buffer_size;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMaxWriteBufferNumber
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxWriteBufferNumber(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_write_buffer_number) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      max_write_buffer_number = jmax_write_buffer_number;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxWriteBufferNumber
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_maxWriteBufferNumber(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      max_write_buffer_number;
}

/*
 *方法：setmemtablefactory
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMemTableFactory(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jfactory_handle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      memtable_factory.reset(
      reinterpret_cast<rocksdb::MemTableRepFactory*>(jfactory_handle));
}

/*
 *类别：组织rocksdb列系列选项
 *方法：memTableFactoryName
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_ColumnFamilyOptions_memTableFactoryName(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  rocksdb::MemTableRepFactory* tf = opt->memtable_factory.get();

//不应为nullptr。
//默认的memtable工厂是skipListFactory
  assert(tf);

//暂时修复历史错误
  if (strcmp(tf->Name(), "HashLinkListRepFactory") == 0) {
    return env->NewStringUTF("HashLinkedListRepFactory");
  }

  return env->NewStringUTF(tf->Name());
}

/*
 *方法：使用FixedLengthPrefixtractor
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_useFixedLengthPrefixExtractor(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jprefix_length) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      prefix_extractor.reset(rocksdb::NewFixedPrefixTransform(
          static_cast<int>(jprefix_length)));
}

/*
 *方法：使用cappedPrefixtractor
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_useCappedPrefixExtractor(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jprefix_length) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      prefix_extractor.reset(rocksdb::NewCappedPrefixTransform(
          static_cast<int>(jprefix_length)));
}

/*
 *方法：设置工厂
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setTableFactory(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jfactory_handle) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      table_factory.reset(reinterpret_cast<rocksdb::TableFactory*>(
      jfactory_handle));
}

/*
 *方法：TableFactoryName
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_ColumnFamilyOptions_tableFactoryName(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  rocksdb::TableFactory* tf = opt->table_factory.get();

//不应为nullptr。
//默认的memtable工厂是skipListFactory
  assert(tf);

  return env->NewStringUTF(tf->Name());
}


/*
 *类别：组织rocksdb列系列选项
 *方法：MinWriteBufferNumberToMerge
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_minWriteBufferNumberToMerge(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->min_write_buffer_number_to_merge;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMinWriteBufferNumberToMerge
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMinWriteBufferNumberToMerge(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jmin_write_buffer_number_to_merge) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->min_write_buffer_number_to_merge =
          static_cast<int>(jmin_write_buffer_number_to_merge);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxWriteBufferNumberToMaintain
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_maxWriteBufferNumberToMaintain(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->max_write_buffer_number_to_maintain;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMaxWriteBufferNumberToMaintain
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxWriteBufferNumberToMaintain(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jmax_write_buffer_number_to_maintain) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->max_write_buffer_number_to_maintain =
      static_cast<int>(jmax_write_buffer_number_to_maintain);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setcompressionType
 *签字：（JB）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jcompression_type_value) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_opts->compression = rocksdb::CompressionTypeJni::toCppCompressionType(
      jcompression_type_value);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：压缩类型
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_ColumnFamilyOptions_compressionType(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return rocksdb::CompressionTypeJni::toJavaCompressionType(
      cf_opts->compression);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setcompressionPerLevel
 *签字：（J[B）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompressionPerLevel(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jbyteArray jcompressionLevels) {
  auto* options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  auto uptr_compression_levels =
      rocksdb_compression_vector_helper(env, jcompressionLevels);
  if(!uptr_compression_levels) {
//发生异常
      return;
  }
  options->compression_per_level = *(uptr_compression_levels.get());
}

/*
 *类别：组织rocksdb列系列选项
 *方法：压缩级别
 *签字：（j）[b
 **/

jbyteArray Java_org_rocksdb_ColumnFamilyOptions_compressionPerLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return rocksdb_compression_list_helper(env,
      cf_options->compression_per_level);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setBottomMostCompressionType
 *签字：（JB）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setBottommostCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jcompression_type_value) {
  auto* cf_options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_options->bottommost_compression =
      rocksdb::CompressionTypeJni::toCppCompressionType(
          jcompression_type_value);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：BottomMostCompressionType
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_ColumnFamilyOptions_bottommostCompressionType(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return rocksdb::CompressionTypeJni::toJavaCompressionType(
      cf_options->bottommost_compression);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置压缩选项
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompressionOptions(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jcompression_options_handle) {
  auto* cf_options = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  auto* compression_options =
    reinterpret_cast<rocksdb::CompressionOptions*>(jcompression_options_handle);
  cf_options->compression_opts = *compression_options;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置压缩样式
 *签字：（JB）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompactionStyle(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte compaction_style) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->compaction_style =
      static_cast<rocksdb::CompactionStyle>(compaction_style);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：压缩样式
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_ColumnFamilyOptions_compactionStyle(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>
      (jhandle)->compaction_style;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmaxtablefilesizefifo
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxTableFilesSizeFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jmax_table_files_size) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->compaction_options_fifo.max_table_files_size =
    static_cast<uint64_t>(jmax_table_files_size);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：maxTableFileSizeInfo
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_maxTableFilesSizeFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->compaction_options_fifo.max_table_files_size;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：numlevels
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_numLevels(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->num_levels;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setNumLevels
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setNumLevels(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jnum_levels) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->num_levels =
      static_cast<int>(jnum_levels);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：LevelZeroFileNumCompactionTrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_levelZeroFileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_file_num_compaction_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevelZeroFileNumCompactionTrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevelZeroFileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_file_num_compaction_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_file_num_compaction_trigger =
          static_cast<int>(jlevel0_file_num_compaction_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：零级减速器
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_levelZeroSlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_slowdown_writes_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setlevelslowdownwritestrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevelZeroSlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_slowdown_writes_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_slowdown_writes_trigger =
          static_cast<int>(jlevel0_slowdown_writes_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：LevelZeroStopWriteStrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_levelZeroStopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_stop_writes_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevelStopWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevelZeroStopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_stop_writes_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      level0_stop_writes_trigger = static_cast<int>(
      jlevel0_stop_writes_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：targetfilesizebase
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_targetFileSizeBase(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      target_file_size_base;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：settargetfilesizebase
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setTargetFileSizeBase(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jtarget_file_size_base) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      target_file_size_base = static_cast<uint64_t>(jtarget_file_size_base);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：TargetFileSize乘数
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_targetFileSizeMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->target_file_size_multiplier;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置目标文件大小乘数
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setTargetFileSizeMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jtarget_file_size_multiplier) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->target_file_size_multiplier =
          static_cast<int>(jtarget_file_size_multiplier);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxBytesForLevelBase
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_maxBytesForLevelBase(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_bytes_for_level_base;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMaxBytesForLevelBase
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxBytesForLevelBase(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_bytes_for_level_base) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_bytes_for_level_base =
          static_cast<int64_t>(jmax_bytes_for_level_base);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：LevelCompactionDynamicClevelBytes
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_levelCompactionDynamicLevelBytes(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level_compaction_dynamic_level_bytes;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevelCompactionDynamicClevelBytes
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevelCompactionDynamicLevelBytes(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jenable_dynamic_level_bytes) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level_compaction_dynamic_level_bytes =
          (jenable_dynamic_level_bytes);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxBytesForLevelMultiplier
 *签名：（j）d
 **/

jdouble Java_org_rocksdb_ColumnFamilyOptions_maxBytesForLevelMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_bytes_for_level_multiplier;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：SetMaxBytesForLevelMultiplier
 *签字：（JD）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxBytesForLevelMultiplier(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jdouble jmax_bytes_for_level_multiplier) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->max_bytes_for_level_multiplier =
      static_cast<double>(jmax_bytes_for_level_multiplier);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxCompactionBytes
 *签字：（j）I
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_maxCompactionBytes(JNIEnv* env,
                                                              jobject jobj,
                                                              jlong jhandle) {
  return static_cast<jlong>(
      reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
          ->max_compaction_bytes);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMaxCompactionBytes
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxCompactionBytes(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jmax_compaction_bytes) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->max_compaction_bytes = static_cast<uint64_t>(jmax_compaction_bytes);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：arenablocksize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_arenaBlockSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      arena_block_size;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setarenablocksize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setArenaBlockSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jarena_block_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(jarena_block_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
        arena_block_size = jarena_block_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：禁用自动压缩
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_disableAutoCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->disable_auto_compactions;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setDisableAutoCompactions
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setDisableAutoCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jdisable_auto_compactions) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->disable_auto_compactions =
          static_cast<bool>(jdisable_auto_compactions);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxSequentialsKipinIterations
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_maxSequentialSkipInIterations(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_sequential_skip_in_iterations;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setMaxSequentialsKipinIterations
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxSequentialSkipInIterations(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_sequential_skip_in_iterations) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_sequential_skip_in_iterations =
          static_cast<int64_t>(jmax_sequential_skip_in_iterations);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：inplaceUpdateSupport
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_inplaceUpdateSupport(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->inplace_update_support;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setinplaceupdatesupport
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setInplaceUpdateSupport(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jinplace_update_support) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->inplace_update_support =
          static_cast<bool>(jinplace_update_support);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：inplaceUpdateNumLocks
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_inplaceUpdateNumLocks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->inplace_update_num_locks;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setInplaceUpdateNumLocks
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setInplaceUpdateNumLocks(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jinplace_update_num_locks) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jinplace_update_num_locks);
  if (s.ok()) {
    reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
        inplace_update_num_locks = jinplace_update_num_locks;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MEMTablePrefixBloomSizeratio
 *签字：（j）I
 **/

jdouble Java_org_rocksdb_ColumnFamilyOptions_memtablePrefixBloomSizeRatio(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->memtable_prefix_bloom_size_ratio;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmemtableprefixbloomsizeratio
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMemtablePrefixBloomSizeRatio(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jdouble jmemtable_prefix_bloom_size_ratio) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)
      ->memtable_prefix_bloom_size_ratio =
      static_cast<double>(jmemtable_prefix_bloom_size_ratio);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：BloomLocality
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_bloomLocality(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      bloom_locality;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setBloomLocality
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setBloomLocality(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jbloom_locality) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->bloom_locality =
      static_cast<int32_t>(jbloom_locality);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：MaxSuccessiveMerges
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_maxSuccessiveMerges(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
      max_successive_merges;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmaxsuccessivemerges
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxSuccessiveMerges(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_successive_merges) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jmax_successive_merges);
  if (s.ok()) {
    reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle)->
        max_successive_merges = jmax_successive_merges;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：优化过滤器
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_optimizeFiltersForHits(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->optimize_filters_for_hits;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setoptimizefiltersforhits
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setOptimizeFiltersForHits(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean joptimize_filters_for_hits) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->optimize_filters_for_hits =
          static_cast<bool>(joptimize_filters_for_hits);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：memtablehugepageSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_memtableHugePageSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->memtable_huge_page_size;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setmemtablehugepageSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMemtableHugePageSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmemtable_huge_page_size) {

  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      jmemtable_huge_page_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
        jhandle)->memtable_huge_page_size =
            jmemtable_huge_page_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：组织rocksdb列系列选项
 *方法：SoftPendingCompactionBytesLimit
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_softPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->soft_pending_compaction_bytes_limit;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置PendingCompactionBytesLimit
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setSoftPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jsoft_pending_compaction_bytes_limit) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->soft_pending_compaction_bytes_limit =
          static_cast<int64_t>(jsoft_pending_compaction_bytes_limit);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：软硬件压缩字节限制
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ColumnFamilyOptions_hardPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->hard_pending_compaction_bytes_limit;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：sethardpendingcompactionbyteslimit
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setHardPendingCompactionBytesLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jhard_pending_compaction_bytes_limit) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->hard_pending_compaction_bytes_limit =
          static_cast<int64_t>(jhard_pending_compaction_bytes_limit);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：Level0FileNumCompactionTrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_level0FileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
    jhandle)->level0_file_num_compaction_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevel0fileNumCompactionTrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevel0FileNumCompactionTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_file_num_compaction_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_file_num_compaction_trigger =
          static_cast<int32_t>(jlevel0_file_num_compaction_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：0级减速器
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_level0SlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
    jhandle)->level0_slowdown_writes_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevel0 SlowDownWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevel0SlowdownWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_slowdown_writes_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_slowdown_writes_trigger =
          static_cast<int32_t>(jlevel0_slowdown_writes_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：Level0StopWriteStrigger
 *签字：（j）I
 **/

jint Java_org_rocksdb_ColumnFamilyOptions_level0StopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
    jhandle)->level0_stop_writes_trigger;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setLevel0stopWriteStrigger
 *签名：（Ji）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setLevel0StopWritesTrigger(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jint jlevel0_stop_writes_trigger) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->level0_stop_writes_trigger =
          static_cast<int32_t>(jlevel0_stop_writes_trigger);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：最大字节数用于电平多路附加
 *签字：（j）[i
 **/

jintArray Java_org_rocksdb_ColumnFamilyOptions_maxBytesForLevelMultiplierAdditional(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto mbflma = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->max_bytes_for_level_multiplier_additional;

  const size_t size = mbflma.size();

  jint* additionals = new jint[size];
  for (size_t i = 0; i < size; i++) {
    additionals[i] = static_cast<jint>(mbflma[i]);
  }

  jsize jlen = static_cast<jsize>(size);
  jintArray result = env->NewIntArray(jlen);
  if(result == nullptr) {
//引发异常：OutOfMemoryError
    delete [] additionals;
    return nullptr;
  }
  env->SetIntArrayRegion(result, 0, jlen, additionals);
  if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(result);
      delete [] additionals;
      return nullptr;
  }

  delete [] additionals;

  return result;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置最大字节数，用于电平多路附加
 *签字：（J[I）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setMaxBytesForLevelMultiplierAdditional(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jintArray jmax_bytes_for_level_multiplier_additional) {
  jsize len = env->GetArrayLength(jmax_bytes_for_level_multiplier_additional);
  jint *additionals =
      env->GetIntArrayElements(jmax_bytes_for_level_multiplier_additional, 0);
  if(additionals == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  auto* cf_opt = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_opt->max_bytes_for_level_multiplier_additional.clear();
  for (jsize i = 0; i < len; i++) {
    cf_opt->max_bytes_for_level_multiplier_additional.push_back(static_cast<int32_t>(additionals[i]));
  }

  env->ReleaseIntArrayElements(jmax_bytes_for_level_multiplier_additional,
      additionals, JNI_ABORT);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：偏执文件检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_paranoidFileChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->paranoid_file_checks;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置偏执文件检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setParanoidFileChecks(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jparanoid_file_checks) {
  reinterpret_cast<rocksdb::ColumnFamilyOptions*>(
      jhandle)->paranoid_file_checks =
          static_cast<bool>(jparanoid_file_checks);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setCompactionPriority
 *签字：（JB）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompactionPriority(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jbyte jcompaction_priority_value) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_opts->compaction_pri =
      rocksdb::CompactionPriorityJni::toCppCompactionPriority(jcompaction_priority_value);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：压缩优先级
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_ColumnFamilyOptions_compactionPriority(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return rocksdb::CompactionPriorityJni::toJavaCompactionPriority(
      cf_opts->compaction_pri);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：setreportbgiostats
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setReportBgIoStats(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jreport_bg_io_stats) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_opts->report_bg_io_stats = static_cast<bool>(jreport_bg_io_stats);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：reportbgiostats
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_reportBgIoStats(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return static_cast<bool>(cf_opts->report_bg_io_stats);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置压缩选项通用
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompactionOptionsUniversal(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jcompaction_options_universal_handle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  auto* opts_uni =
      reinterpret_cast<rocksdb::CompactionOptionsUniversal*>(
          jcompaction_options_universal_handle);
  cf_opts->compaction_options_universal = *opts_uni;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置压缩选项FIFO
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setCompactionOptionsFIFO(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jcompaction_options_fifo_handle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  auto* opts_fifo =
      reinterpret_cast<rocksdb::CompactionOptionsFIFO*>(
          jcompaction_options_fifo_handle);
  cf_opts->compaction_options_fifo = *opts_fifo;
}

/*
 *类别：组织rocksdb列系列选项
 *方法：设置强制一致性检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ColumnFamilyOptions_setForceConsistencyChecks(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jforce_consistency_checks) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  cf_opts->force_consistency_checks = static_cast<bool>(jforce_consistency_checks);
}

/*
 *类别：组织rocksdb列系列选项
 *方法：强制一致性检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ColumnFamilyOptions_forceConsistencyChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* cf_opts = reinterpret_cast<rocksdb::ColumnFamilyOptions*>(jhandle);
  return static_cast<bool>(cf_opts->force_consistency_checks);
}

////////////////////////////////////////////////
//rocksdb:：dboptions（选项）

/*
 *类别：org_rocksdb_dboptions
 *方法：newdboptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_DBOptions_newDBOptions(JNIEnv* env,
    jclass jcls) {
  auto* dbop = new rocksdb::DBOptions();
  return reinterpret_cast<jlong>(dbop);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：GetDBOptionsFromProps
 *签名：（ljava/util/string；）j
 **/

jlong Java_org_rocksdb_DBOptions_getDBOptionsFromProps(
    JNIEnv* env, jclass jclazz, jstring jopt_string) {
  const char* opt_string = env->GetStringUTFChars(jopt_string, nullptr);
  if(opt_string == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }

  auto* db_options = new rocksdb::DBOptions();
  rocksdb::Status status = rocksdb::GetDBOptionsFromString(
      rocksdb::DBOptions(), opt_string, db_options);

  env->ReleaseStringUTFChars(jopt_string, opt_string);

//检查是否可以创建dboptions。
  jlong ret_value = 0;
  if (status.ok()) {
    ret_value = reinterpret_cast<jlong>(db_options);
  } else {
//如果操作失败，则需要删除dboptions
//再次防止内存泄漏。
    delete db_options;
  }
  return ret_value;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_DBOptions_disposeInternal(
    JNIEnv* env, jobject jobj, jlong handle) {
  auto* dbo = reinterpret_cast<rocksdb::DBOptions*>(handle);
  assert(dbo != nullptr);
  delete dbo;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：优化malldb
 *签名：（j）v
 **/

void Java_org_rocksdb_DBOptions_optimizeForSmallDb(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->OptimizeForSmallDb();
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setenv
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setEnv(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jenv_handle) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->env =
      reinterpret_cast<rocksdb::Env*>(jenv_handle);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setincreaseparallelism
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setIncreaseParallelism(
    JNIEnv * env, jobject jobj, jlong jhandle, jint totalThreads) {
  reinterpret_cast<rocksdb::DBOptions*>
      (jhandle)->IncreaseParallelism(static_cast<int>(totalThreads));
}


/*
 *类别：org_rocksdb_dboptions
 *方法：setCreateifMissing
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setCreateIfMissing(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      create_if_missing = flag;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：createifMissing
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_createIfMissing(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->create_if_missing;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setCreateMissingColumnFamilies
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setCreateMissingColumnFamilies(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  reinterpret_cast<rocksdb::DBOptions*>
      (jhandle)->create_missing_column_families = flag;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：创建MissingColumnFamilies
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_createMissingColumnFamilies(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>
      (jhandle)->create_missing_column_families;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：seterrorifexists
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setErrorIfExists(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean error_if_exists) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->error_if_exists =
      static_cast<bool>(error_if_exists);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：错误存在
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_errorIfExists(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->error_if_exists;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：设置偏执检查
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setParanoidChecks(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean paranoid_checks) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->paranoid_checks =
      static_cast<bool>(paranoid_checks);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：偏执检查
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_paranoidChecks(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->paranoid_checks;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setratelimiter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setRateLimiter(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrate_limiter_handle) {
  std::shared_ptr<rocksdb::RateLimiter> *pRateLimiter =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(
          jrate_limiter_handle);
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->rate_limiter = *pRateLimiter;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setlogger
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setLogger(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jlogger_handle) {
  std::shared_ptr<rocksdb::LoggerJniCallback> *pLogger =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(
          jlogger_handle);
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->info_log = *pLogger;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setInfologLevel
 *签字：（JB）V
 **/

void Java_org_rocksdb_DBOptions_setInfoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jlog_level) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->info_log_level =
    static_cast<rocksdb::InfoLogLevel>(jlog_level);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：信息日志级别
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_DBOptions_infoLogLevel(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return static_cast<jbyte>(
      reinterpret_cast<rocksdb::DBOptions*>(jhandle)->info_log_level);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxtotalwalsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setMaxTotalWalSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jmax_total_wal_size) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_total_wal_size =
      static_cast<jlong>(jmax_total_wal_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：MaxTotalWalSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_maxTotalWalSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      max_total_wal_size;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxopenfiles
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setMaxOpenFiles(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max_open_files) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_open_files =
      static_cast<int>(max_open_files);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：maxopenfiles
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_maxOpenFiles(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_open_files;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxfileopeningthreads
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setMaxFileOpeningThreads(
    JNIEnv* env, jobject jobj, jlong jhandle, jint jmax_file_opening_threads) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_file_opening_threads =
      static_cast<int>(jmax_file_opening_threads);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：maxfileopeningthreads
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_maxFileOpeningThreads(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<int>(opt->max_file_opening_threads);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：设置统计
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setStatistics(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jstatistics_handle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  auto* pSptr =
      reinterpret_cast<std::shared_ptr<rocksdb::StatisticsJni>*>(
          jstatistics_handle);
  opt->statistics = *pSptr;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：统计
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_statistics(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  std::shared_ptr<rocksdb::Statistics> sptr = opt->statistics;
  if (sptr == nullptr) {
    return 0;
  } else {
    std::shared_ptr<rocksdb::Statistics>* pSptr =
        new std::shared_ptr<rocksdb::Statistics>(sptr);
    return reinterpret_cast<jlong>(pSptr);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setusefsync
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setUseFsync(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean use_fsync) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_fsync =
      static_cast<bool>(use_fsync);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：usefsync
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_useFsync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_fsync;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setdbpaths
 *签名：（j[ljava/lang/string；[j）v
 **/

void Java_org_rocksdb_DBOptions_setDbPaths(
    JNIEnv* env, jobject jobj, jlong jhandle, jobjectArray jpaths,
    jlongArray jtarget_sizes) {
  std::vector<rocksdb::DbPath> db_paths;
  jlong* ptr_jtarget_size = env->GetLongArrayElements(jtarget_sizes, nullptr);
  if(ptr_jtarget_size == nullptr) {
//引发异常：OutOfMemoryError
      return;
  }

  jboolean has_exception = JNI_FALSE;
  const jsize len = env->GetArrayLength(jpaths);
  for(jsize i = 0; i < len; i++) {
    jobject jpath = reinterpret_cast<jstring>(env->
        GetObjectArrayElement(jpaths, i));
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }
    std::string path = rocksdb::JniUtil::copyString(
        env, static_cast<jstring>(jpath), &has_exception);
    env->DeleteLocalRef(jpath);

    if(has_exception == JNI_TRUE) {
        env->ReleaseLongArrayElements(
            jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
        return;
    }

    jlong jtarget_size = ptr_jtarget_size[i];

    db_paths.push_back(
        rocksdb::DbPath(path, static_cast<uint64_t>(jtarget_size)));
  }

  env->ReleaseLongArrayElements(jtarget_sizes, ptr_jtarget_size, JNI_ABORT);

  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->db_paths = db_paths;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：dbpathslen
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_dbPathsLen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->db_paths.size());
}

/*
 *类别：org_rocksdb_dboptions
 *方法：dbpath
 *签名：（j[ljava/lang/string；[j）v
 **/

void Java_org_rocksdb_DBOptions_dbPaths(
    JNIEnv* env, jobject jobj, jlong jhandle, jobjectArray jpaths,
    jlongArray jtarget_sizes) {
  jlong* ptr_jtarget_size = env->GetLongArrayElements(jtarget_sizes, nullptr);
  if(ptr_jtarget_size == nullptr) {
//引发异常：OutOfMemoryError
      return;
  }

  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  const jsize len = env->GetArrayLength(jpaths);
  for(jsize i = 0; i < len; i++) {
    rocksdb::DbPath db_path = opt->db_paths[i];

    jstring jpath = env->NewStringUTF(db_path.path.c_str());
    if(jpath == nullptr) {
//引发异常：OutOfMemoryError
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }
    env->SetObjectArrayElement(jpaths, i, jpath);
    if(env->ExceptionCheck()) {
//引发异常：arrayindexoutofboundsException
      env->DeleteLocalRef(jpath);
      env->ReleaseLongArrayElements(
          jtarget_sizes, ptr_jtarget_size, JNI_ABORT);
      return;
    }

    ptr_jtarget_size[i] = static_cast<jint>(db_path.target_size);
  }

  env->ReleaseLongArrayElements(jtarget_sizes, ptr_jtarget_size, JNI_COMMIT);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setdblogdir
 *签名：（jljava/lang/string）v
 **/

void Java_org_rocksdb_DBOptions_setDbLogDir(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jdb_log_dir) {
  const char* log_dir = env->GetStringUTFChars(jdb_log_dir, nullptr);
  if(log_dir == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }

  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->db_log_dir.assign(log_dir);
  env->ReleaseStringUTFChars(jdb_log_dir, log_dir);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：dblogdir
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_DBOptions_dbLogDir(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return env->NewStringUTF(
      reinterpret_cast<rocksdb::DBOptions*>(jhandle)->db_log_dir.c_str());
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwaldir
 *签名：（jljava/lang/string）v
 **/

void Java_org_rocksdb_DBOptions_setWalDir(
    JNIEnv* env, jobject jobj, jlong jhandle, jstring jwal_dir) {
  const char* wal_dir = env->GetStringUTFChars(jwal_dir, 0);
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->wal_dir.assign(wal_dir);
  env->ReleaseStringUTFChars(jwal_dir, wal_dir);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：Waldir
 *签名：（j）ljava/lang/string
 **/

jstring Java_org_rocksdb_DBOptions_walDir(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return env->NewStringUTF(
      reinterpret_cast<rocksdb::DBOptions*>(jhandle)->wal_dir.c_str());
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setDeleteObsoletefilesPeriodMicros
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setDeleteObsoleteFilesPeriodMicros(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong micros) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->delete_obsolete_files_period_micros =
          static_cast<int64_t>(micros);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：DeleteObsoletefilesPeriodMicros
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_deleteObsoleteFilesPeriodMicros(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->delete_obsolete_files_period_micros;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setbasebackgroundcompactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setBaseBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->base_background_compactions = static_cast<int>(max);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：基底压实
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_baseBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->base_background_compactions;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxbackgroundcompactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setMaxBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->max_background_compactions = static_cast<int>(max);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：MaxBackgroundCompactions
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_maxBackgroundCompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(
      jhandle)->max_background_compactions;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxsubpactions
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setMaxSubcompactions(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->max_subcompactions = static_cast<int32_t>(max);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：MaxSubpactions
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_maxSubcompactions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->max_subcompactions;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxbackgroundflushes
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setMaxBackgroundFlushes(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max_background_flushes) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_background_flushes =
      static_cast<int>(max_background_flushes);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：MaxBackgroundFlushes
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_maxBackgroundFlushes(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      max_background_flushes;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxlogfilesize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setMaxLogFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max_log_file_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(max_log_file_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_log_file_size =
        max_log_file_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：maxlogfilesize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_maxLogFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_log_file_size;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setlogfiletimetoroll
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setLogFileTimeToRoll(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong log_file_time_to_roll) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(
      log_file_time_to_roll);
  if (s.ok()) {
    reinterpret_cast<rocksdb::DBOptions*>(jhandle)->log_file_time_to_roll =
        log_file_time_to_roll;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：logfiletimetoroll
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_logFileTimeToRoll(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->log_file_time_to_roll;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setkeeploggfilenum
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setKeepLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong keep_log_file_num) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(keep_log_file_num);
  if (s.ok()) {
    reinterpret_cast<rocksdb::DBOptions*>(jhandle)->keep_log_file_num =
        keep_log_file_num;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：keeploggfilenum
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_keepLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->keep_log_file_num;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setRecycleLogFileNum
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setRecycleLogFileNum(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong recycle_log_file_num) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(recycle_log_file_num);
  if (s.ok()) {
    reinterpret_cast<rocksdb::DBOptions*>(jhandle)->recycle_log_file_num =
        recycle_log_file_num;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：RecycleLogFileNum
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_recycleLogFileNum(JNIEnv* env, jobject jobj,
                                                   jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->recycle_log_file_num;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setmaxmanifestfilesize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setMaxManifestFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max_manifest_file_size) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->max_manifest_file_size =
      static_cast<int64_t>(max_manifest_file_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：maxmanifestfilesize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_maxManifestFileSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      max_manifest_file_size;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：可设置cachenumshardbits
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setTableCacheNumshardbits(
    JNIEnv* env, jobject jobj, jlong jhandle, jint table_cache_numshardbits) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->table_cache_numshardbits =
      static_cast<int>(table_cache_numshardbits);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：tablecachenumshardbits
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_tableCacheNumshardbits(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      table_cache_numshardbits;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwalttlseconds
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWalTtlSeconds(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong WAL_ttl_seconds) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->WAL_ttl_seconds =
      static_cast<int64_t>(WAL_ttl_seconds);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：Walttlseconds
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_walTtlSeconds(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->WAL_ttl_seconds;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwalsizelimitmb
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWalSizeLimitMB(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong WAL_size_limit_MB) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->WAL_size_limit_MB =
      static_cast<int64_t>(WAL_size_limit_MB);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：Walttlseconds
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_walSizeLimitMB(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->WAL_size_limit_MB;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setManifestPreallocationSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setManifestPreallocationSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong preallocation_size) {
  rocksdb::Status s = rocksdb::check_if_jlong_fits_size_t(preallocation_size);
  if (s.ok()) {
    reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
        manifest_preallocation_size = preallocation_size;
  } else {
    rocksdb::IllegalArgumentExceptionJni::ThrowNew(env, s);
  }
}

/*
 *类别：org_rocksdb_dboptions
 *方法：manifestPreallocationSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_manifestPreallocationSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->manifest_preallocation_size;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：使用直接读取
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_useDirectReads(JNIEnv* env, jobject jobj,
                                                   jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_direct_reads;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setusedirectreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setUseDirectReads(JNIEnv* env, jobject jobj,
                                                  jlong jhandle,
                                                  jboolean use_direct_reads) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_direct_reads =
      static_cast<bool>(use_direct_reads);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：直接用于Flushandcompression
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_useDirectIoForFlushAndCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->use_direct_io_for_flush_and_compaction;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setusedirectreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setUseDirectIoForFlushAndCompaction(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean use_direct_io_for_flush_and_compaction) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)
      ->use_direct_io_for_flush_and_compaction =
      static_cast<bool>(use_direct_io_for_flush_and_compaction);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setallowflocate
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAllowFAllocate(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_fallocate) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->allow_fallocate =
      static_cast<bool>(jallow_fallocate);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：allowflocate
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_allowFAllocate(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->allow_fallocate);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setallowmapreads
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAllowMmapReads(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow_mmap_reads) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->allow_mmap_reads =
      static_cast<bool>(allow_mmap_reads);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：allowmapreads
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_allowMmapReads(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->allow_mmap_reads;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setallowmapwrites
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAllowMmapWrites(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow_mmap_writes) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->allow_mmap_writes =
      static_cast<bool>(allow_mmap_writes);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：allowmapwrites
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_allowMmapWrites(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->allow_mmap_writes;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setisfdcloseonexec
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setIsFdCloseOnExec(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean is_fd_close_on_exec) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->is_fd_close_on_exec =
      static_cast<bool>(is_fd_close_on_exec);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：isfdcloseonexec
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_isFdCloseOnExec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->is_fd_close_on_exec;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：SetStatesDumpPeriodSec
 *签名：（Ji）V
 **/

void Java_org_rocksdb_DBOptions_setStatsDumpPeriodSec(
    JNIEnv* env, jobject jobj, jlong jhandle, jint stats_dump_period_sec) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->stats_dump_period_sec =
      static_cast<int>(stats_dump_period_sec);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：StatsDumpPeriodSec
 *签字：（j）I
 **/

jint Java_org_rocksdb_DBOptions_statsDumpPeriodSec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->stats_dump_period_sec;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setadviserandomonopen
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAdviseRandomOnOpen(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean advise_random_on_open) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->advise_random_on_open =
      static_cast<bool>(advise_random_on_open);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：阿维司兰多芬
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_adviseRandomOnOpen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->advise_random_on_open;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setdbwritebuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setDbWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jdb_write_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->db_write_buffer_size = static_cast<size_t>(jdb_write_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：dbWriteBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_dbWriteBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->db_write_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setAccessHintonCompactionStart
 *签字：（JB）V
 **/

void Java_org_rocksdb_DBOptions_setAccessHintOnCompactionStart(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jaccess_hint_value) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->access_hint_on_compaction_start =
      rocksdb::AccessHintJni::toCppAccessHint(jaccess_hint_value);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：accessHintonCompactionStart
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_DBOptions_accessHintOnCompactionStart(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return rocksdb::AccessHintJni::toJavaAccessHint(
      opt->access_hint_on_compaction_start);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setnewtablereaderforcompactioninputs
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setNewTableReaderForCompactionInputs(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jnew_table_reader_for_compaction_inputs) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->new_table_reader_for_compaction_inputs =
      static_cast<bool>(jnew_table_reader_for_compaction_inputs);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：NewTableReaderForCompactionInputs
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_newTableReaderForCompactionInputs(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<bool>(opt->new_table_reader_for_compaction_inputs);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setcompactionreadaheadsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setCompactionReadaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jcompaction_readahead_size) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->compaction_readahead_size =
      static_cast<size_t>(jcompaction_readahead_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：压缩读取标题大小
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_compactionReadaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->compaction_readahead_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setrandomaaccessmaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setRandomAccessMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jrandom_access_max_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->random_access_max_buffer_size =
      static_cast<size_t>(jrandom_access_max_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：RandomAccessMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_randomAccessMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->random_access_max_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwritablefilemaxbuffersize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWritableFileMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jwritable_file_max_buffer_size) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->writable_file_max_buffer_size =
      static_cast<size_t>(jwritable_file_max_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：WritableFileMaxBufferSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_writableFileMaxBufferSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->writable_file_max_buffer_size);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setuseadaptivemutex
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setUseAdaptiveMutex(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean use_adaptive_mutex) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_adaptive_mutex =
      static_cast<bool>(use_adaptive_mutex);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：useadaptivemutex
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_useAdaptiveMutex(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->use_adaptive_mutex;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setbytespersync
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong bytes_per_sync) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->bytes_per_sync =
      static_cast<int64_t>(bytes_per_sync);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：bytespersync
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_bytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->bytes_per_sync;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwalbytespersync
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWalBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jwal_bytes_per_sync) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->wal_bytes_per_sync =
      static_cast<int64_t>(jwal_bytes_per_sync);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：walbytespersync
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_walBytesPerSync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->wal_bytes_per_sync);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setEnableThreadTracking
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setEnableThreadTracking(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jenable_thread_tracking) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->enable_thread_tracking = static_cast<bool>(jenable_thread_tracking);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：启用线程跟踪
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_enableThreadTracking(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->enable_thread_tracking);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setDelayedWriter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setDelayedWriteRate(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jdelayed_write_rate) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->delayed_write_rate = static_cast<uint64_t>(jdelayed_write_rate);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：延迟写入
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_delayedWriteRate(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jlong>(opt->delayed_write_rate);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setallowConcurrentMemTableWrite
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAllowConcurrentMemtableWrite(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean allow) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      allow_concurrent_memtable_write = static_cast<bool>(allow);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：allowConcurrentMemTableWrite
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_allowConcurrentMemtableWrite(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      allow_concurrent_memtable_write;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setEnableWriteThreadadaptiveyField
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setEnableWriteThreadAdaptiveYield(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean yield) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      enable_write_thread_adaptive_yield = static_cast<bool>(yield);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：EnableWriteThreadadaptiveyField
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_enableWriteThreadAdaptiveYield(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      enable_write_thread_adaptive_yield;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwritethreadmaxyieldusec
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWriteThreadMaxYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong max) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      write_thread_max_yield_usec = static_cast<int64_t>(max);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：writethreadmaxyieldusec
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_writeThreadMaxYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      write_thread_max_yield_usec;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwritethreadslowyieldusec
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setWriteThreadSlowYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong slow) {
  reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      write_thread_slow_yield_usec = static_cast<int64_t>(slow);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：writethreadslowyieldusec
 *签字：（j）j
 **/

jlong Java_org_rocksdb_DBOptions_writeThreadSlowYieldUsec(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::DBOptions*>(jhandle)->
      write_thread_slow_yield_usec;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setskipstatsupdateondbopen
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setSkipStatsUpdateOnDbOpen(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jskip_stats_update_on_db_open) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->skip_stats_update_on_db_open =
      static_cast<bool>(jskip_stats_update_on_db_open);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：skipstatsupdateondbopen
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_skipStatsUpdateOnDbOpen(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->skip_stats_update_on_db_open);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setwalrecoverymode
 *签字：（JB）V
 **/

void Java_org_rocksdb_DBOptions_setWalRecoveryMode(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jwal_recovery_mode_value) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->wal_recovery_mode =
      rocksdb::WALRecoveryModeJni::toCppWALRecoveryMode(
          jwal_recovery_mode_value);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：Walrecoverymode
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_DBOptions_walRecoveryMode(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return rocksdb::WALRecoveryModeJni::toJavaWALRecoveryMode(
      opt->wal_recovery_mode);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setallow2pc
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAllow2pc(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jallow_2pc) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->allow_2pc = static_cast<bool>(jallow_2pc);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：allow2pc
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_allow2pc(JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->allow_2pc);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setrowcache
 *签字：（JJ）V
 **/

void Java_org_rocksdb_DBOptions_setRowCache(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrow_cache_handle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  auto* row_cache = reinterpret_cast<std::shared_ptr<rocksdb::Cache>*>(jrow_cache_handle);
  opt->row_cache = *row_cache;
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setfailifoptionsfileerror
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setFailIfOptionsFileError(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jfail_if_options_file_error) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->fail_if_options_file_error =
      static_cast<bool>(jfail_if_options_file_error);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：failifoptionsfileerror
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_failIfOptionsFileError(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->fail_if_options_file_error);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setdumpmallocstats
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setDumpMallocStats(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jdump_malloc_stats) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->dump_malloc_stats = static_cast<bool>(jdump_malloc_stats);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：dumpmallocstats
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_dumpMallocStats(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->dump_malloc_stats);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setavoiddflushduringrecovery
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAvoidFlushDuringRecovery(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean javoid_flush_during_recovery) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->avoid_flush_during_recovery = static_cast<bool>(javoid_flush_during_recovery);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：避免在恢复过程中冲洗。
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_avoidFlushDuringRecovery(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->avoid_flush_during_recovery);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：setavoidFlushDuringShutdown
 *签字：（JZ）V
 **/

void Java_org_rocksdb_DBOptions_setAvoidFlushDuringShutdown(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean javoid_flush_during_shutdown) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  opt->avoid_flush_during_shutdown = static_cast<bool>(javoid_flush_during_shutdown);
}

/*
 *类别：org_rocksdb_dboptions
 *方法：避免在关机时冲洗。
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_DBOptions_avoidFlushDuringShutdown(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::DBOptions*>(jhandle);
  return static_cast<jboolean>(opt->avoid_flush_during_shutdown);
}

//////////////////////////////////////////////////
//rocksdb：：写入选项

/*
 *类：org-rocksdb-writeoptions
 *方法：NewWriteOptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_WriteOptions_newWriteOptions(
    JNIEnv* env, jclass jcls) {
  auto* op = new rocksdb::WriteOptions();
  return reinterpret_cast<jlong>(op);
}

/*
 *类：org-rocksdb-writeoptions
 *方法：外用
 *签名：（）V
 **/

void Java_org_rocksdb_WriteOptions_disposeInternal(
    JNIEnv* env, jobject jwrite_options, jlong jhandle) {
  auto* write_options = reinterpret_cast<rocksdb::WriteOptions*>(jhandle);
  assert(write_options != nullptr);
  delete write_options;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：setsync
 *签字：（JZ）V
 **/

void Java_org_rocksdb_WriteOptions_setSync(
  JNIEnv* env, jobject jwrite_options, jlong jhandle, jboolean jflag) {
  reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->sync = jflag;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：同步
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_WriteOptions_sync(
    JNIEnv* env, jobject jwrite_options, jlong jhandle) {
  return reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->sync;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：setdisablewal
 *签字：（JZ）V
 **/

void Java_org_rocksdb_WriteOptions_setDisableWAL(
    JNIEnv* env, jobject jwrite_options, jlong jhandle, jboolean jflag) {
  reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->disableWAL = jflag;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：禁用
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_WriteOptions_disableWAL(
    JNIEnv* env, jobject jwrite_options, jlong jhandle) {
  return reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->disableWAL;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：setignoremissingColumnFamilies
 *签字：（JZ）V
 **/

void Java_org_rocksdb_WriteOptions_setIgnoreMissingColumnFamilies(
    JNIEnv* env, jobject jwrite_options, jlong jhandle,
    jboolean jignore_missing_column_families) {
  reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->
      ignore_missing_column_families =
          static_cast<bool>(jignore_missing_column_families);
}

/*
 *类：org-rocksdb-writeoptions
 *方法：IgnoreMissingColumnFamilies
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_WriteOptions_ignoreMissingColumnFamilies(
    JNIEnv* env, jobject jwrite_options, jlong jhandle) {
  return reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->
      ignore_missing_column_families;
}

/*
 *类：org-rocksdb-writeoptions
 *方法：setnoslowdown
 *签字：（JZ）V
 **/

void Java_org_rocksdb_WriteOptions_setNoSlowdown(
    JNIEnv* env, jobject jwrite_options, jlong jhandle, jboolean jno_slowdown) {
  reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->no_slowdown =
      static_cast<bool>(jno_slowdown);
}

/*
 *类：org-rocksdb-writeoptions
 *方法：不向下搜索
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_WriteOptions_noSlowdown(
    JNIEnv* env, jobject jwrite_options, jlong jhandle) {
  return reinterpret_cast<rocksdb::WriteOptions*>(jhandle)->no_slowdown;
}

////////////////////////////////////////////////
//RocksDB：：读取选项

/*
 *课程：组织rocksdb_readoptions
 *方法：newreadoptions
 *签字：（）J
 **/

jlong Java_org_rocksdb_ReadOptions_newReadOptions(
    JNIEnv* env, jclass jcls) {
  auto* read_options = new rocksdb::ReadOptions();
  return reinterpret_cast<jlong>(read_options);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_ReadOptions_disposeInternal(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* read_options = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  assert(read_options != nullptr);
  delete read_options;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setverifychecksums
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setVerifyChecksums(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jverify_checksums) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->verify_checksums =
      static_cast<bool>(jverify_checksums);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：验证校验和
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_verifyChecksums(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(
      jhandle)->verify_checksums;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setFillCache
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setFillCache(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jfill_cache) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->fill_cache =
      static_cast<bool>(jfill_cache);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：FillCache
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_fillCache(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->fill_cache;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：设置
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setTailing(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jtailing) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->tailing =
      static_cast<bool>(jtailing);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：尾随
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_tailing(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->tailing;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：管理
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_managed(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->managed;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setmanaged
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setManaged(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jmanaged) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->managed =
      static_cast<bool>(jmanaged);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：TotalOrderSeek
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_totalOrderSeek(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->total_order_seek;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：settotaLoderSeek
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setTotalOrderSeek(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jtotal_order_seek) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->total_order_seek =
      static_cast<bool>(jtotal_order_seek);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：PrefixSameassStart
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_prefixSameAsStart(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->prefix_same_as_start;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setPrefixSameassStart
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setPrefixSameAsStart(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jprefix_same_as_start) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->prefix_same_as_start =
      static_cast<bool>(jprefix_same_as_start);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：pindata
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_pinData(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->pin_data;
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setpindata
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setPinData(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean jpin_data) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->pin_data =
      static_cast<bool>(jpin_data);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：BackgroundPurgeOnIteratorCleanup
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_backgroundPurgeOnIteratorCleanup(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  return static_cast<jboolean>(opt->background_purge_on_iterator_cleanup);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：BackgroundPurgeOnIteratorCleanup
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setBackgroundPurgeOnIteratorCleanup(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jbackground_purge_on_iterator_cleanup) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  opt->background_purge_on_iterator_cleanup =
      static_cast<bool>(jbackground_purge_on_iterator_cleanup);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：readaheadsize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ReadOptions_readaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  return static_cast<jlong>(opt->readahead_size);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setreadaheadsize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ReadOptions_setReadaheadSize(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jreadahead_size) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  opt->readahead_size = static_cast<size_t>(jreadahead_size);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：忽略删除
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ReadOptions_ignoreRangeDeletions(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  return static_cast<jboolean>(opt->ignore_range_deletions);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setignorangedelections
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ReadOptions_setIgnoreRangeDeletions(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jboolean jignore_range_deletions) {
  auto* opt = reinterpret_cast<rocksdb::ReadOptions*>(jhandle);
  opt->ignore_range_deletions = static_cast<bool>(jignore_range_deletions);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：设置快照
 *签字：（JJ）V
 **/

void Java_org_rocksdb_ReadOptions_setSnapshot(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jsnapshot) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->snapshot =
      reinterpret_cast<rocksdb::Snapshot*>(jsnapshot);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：快照
 *签字：（j）j
 **/

jlong Java_org_rocksdb_ReadOptions_snapshot(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto& snapshot =
      reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->snapshot;
  return reinterpret_cast<jlong>(snapshot);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：readtier
 *签名：（j）b
 **/

jbyte Java_org_rocksdb_ReadOptions_readTier(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  return static_cast<jbyte>(
      reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->read_tier);
}

/*
 *课程：组织rocksdb_readoptions
 *方法：setreadtier
 *签字：（JB）V
 **/

void Java_org_rocksdb_ReadOptions_setReadTier(
    JNIEnv* env, jobject jobj, jlong jhandle, jbyte jread_tier) {
  reinterpret_cast<rocksdb::ReadOptions*>(jhandle)->read_tier =
      static_cast<rocksdb::ReadTier>(jread_tier);
}

////////////////////////////////////////////////
//RocksDB:：ComparatorOptions（比较选项）

/*
 *类别：Org_RocksDB_ComparatorOptions
 *方法：新公司选项
 *签字：（）J
 **/

jlong Java_org_rocksdb_ComparatorOptions_newComparatorOptions(
    JNIEnv* env, jclass jcls) {
  auto* comparator_opt = new rocksdb::ComparatorJniCallbackOptions();
  return reinterpret_cast<jlong>(comparator_opt);
}

/*
 *类别：Org_RocksDB_ComparatorOptions
 *方法：useadaptivemutex
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_ComparatorOptions_useAdaptiveMutex(
    JNIEnv * env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::ComparatorJniCallbackOptions*>(jhandle)
    ->use_adaptive_mutex;
}

/*
 *类别：Org_RocksDB_ComparatorOptions
 *方法：setuseadaptivemutex
 *签字：（JZ）V
 **/

void Java_org_rocksdb_ComparatorOptions_setUseAdaptiveMutex(
    JNIEnv * env, jobject jobj, jlong jhandle, jboolean juse_adaptive_mutex) {
  reinterpret_cast<rocksdb::ComparatorJniCallbackOptions*>(jhandle)
    ->use_adaptive_mutex = static_cast<bool>(juse_adaptive_mutex);
}

/*
 *类别：Org_RocksDB_ComparatorOptions
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_ComparatorOptions_disposeInternal(
    JNIEnv * env, jobject jobj, jlong jhandle) {
  auto* comparator_opt =
      reinterpret_cast<rocksdb::ComparatorJniCallbackOptions*>(jhandle);
  assert(comparator_opt != nullptr);
  delete comparator_opt;
}

////////////////////////////////////////////////
//RocksDB:Flushoppings公司

/*
 *课程：Org_Rocksdb_Flushoppings
 *方法：NewFlushoppings
 *签字：（）J
 **/

jlong Java_org_rocksdb_FlushOptions_newFlushOptions(
    JNIEnv* env, jclass jcls) {
  auto* flush_opt = new rocksdb::FlushOptions();
  return reinterpret_cast<jlong>(flush_opt);
}

/*
 *课程：Org_Rocksdb_Flushoppings
 *方法：setwaitforflush
 *签字：（JZ）V
 **/

void Java_org_rocksdb_FlushOptions_setWaitForFlush(
    JNIEnv * env, jobject jobj, jlong jhandle, jboolean jwait) {
  reinterpret_cast<rocksdb::FlushOptions*>(jhandle)
    ->wait = static_cast<bool>(jwait);
}

/*
 *课程：Org_Rocksdb_Flushoppings
 *方法：waitforflush
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_FlushOptions_waitForFlush(
    JNIEnv * env, jobject jobj, jlong jhandle) {
  return reinterpret_cast<rocksdb::FlushOptions*>(jhandle)
    ->wait;
}

/*
 *课程：Org_Rocksdb_Flushoppings
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_FlushOptions_disposeInternal(
    JNIEnv * env, jobject jobj, jlong jhandle) {
  auto* flush_opt = reinterpret_cast<rocksdb::FlushOptions*>(jhandle);
  assert(flush_opt != nullptr);
  delete flush_opt;
}
