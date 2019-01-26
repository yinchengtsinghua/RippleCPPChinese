
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
//调用C++ ROCKSDB:：BuffuEngIn和ROCKSDB:：BuffuBabdBe选项方法
//从Java端。

#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <string>
#include <vector>

#include "include/org_rocksdb_BackupableDBOptions.h"
#include "rocksjni/portal.h"
#include "rocksdb/utilities/backupable_db.h"

////////////////////////////////////////////////
//后备选项

/*
 *类：org_rocksdb_backupabledboptions
 *方法：newbackupabledboptions
 *签名：（ljava/lang/string；）j
 **/

jlong Java_org_rocksdb_BackupableDBOptions_newBackupableDBOptions(
    JNIEnv* env, jclass jcls, jstring jpath) {
  const char* cpath = env->GetStringUTFChars(jpath, nullptr);
  if(cpath == nullptr) {
//引发异常：OutOfMemoryError
    return 0;
  }
  auto* bopt = new rocksdb::BackupableDBOptions(cpath);
  env->ReleaseStringUTFChars(jpath, cpath);
  return reinterpret_cast<jlong>(bopt);
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：backupdir
 *签名：（j）ljava/lang/string；
 **/

jstring Java_org_rocksdb_BackupableDBOptions_backupDir(
    JNIEnv* env, jobject jopt, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return env->NewStringUTF(bopt->backup_dir.c_str());
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：后退环境
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setBackupEnv(
    JNIEnv* env, jobject jopt, jlong jhandle, jlong jrocks_env_handle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  auto* rocks_env = reinterpret_cast<rocksdb::Env*>(jrocks_env_handle);
  bopt->backup_env = rocks_env;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：设置共享表文件
 *签字：（JZ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setShareTableFiles(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->share_table_files = flag;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：共享表文件
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_BackupableDBOptions_shareTableFiles(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->share_table_files;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setinfolog
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setInfoLog(
  JNIEnv* env, jobject jobj, jlong jhandle, jlong jlogger_handle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  auto* sptr_logger =
      reinterpret_cast<std::shared_ptr<rocksdb::LoggerJniCallback> *>(jhandle);
  bopt->info_log = sptr_logger->get();
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setsync
 *签字：（JZ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setSync(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->sync = flag;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：同步
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_BackupableDBOptions_sync(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->sync;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setDestroyoldata
 *签字：（JZ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setDestroyOldData(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->destroy_old_data = flag;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：Destroyoldata
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_BackupableDBOptions_destroyOldData(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->destroy_old_data;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：backuplogfiles
 *签字：（JZ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setBackupLogFiles(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->backup_log_files = flag;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：backuplogfiles
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_BackupableDBOptions_backupLogFiles(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->backup_log_files;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：后退上升极限
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setBackupRateLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jbackup_rate_limit) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->backup_rate_limit = jbackup_rate_limit;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：backuprateLimit
 *签字：（j）j
 **/

jlong Java_org_rocksdb_BackupableDBOptions_backupRateLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->backup_rate_limit;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：后退限制器
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setBackupRateLimiter(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrate_limiter_handle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  auto* sptr_rate_limiter =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(jrate_limiter_handle);
  bopt->backup_rate_limiter = *sptr_rate_limiter;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setRestoreRateLimit
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setRestoreRateLimit(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrestore_rate_limit) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->restore_rate_limit = jrestore_rate_limit;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：RestoreRateLimit
 *签字：（j）j
 **/

jlong Java_org_rocksdb_BackupableDBOptions_restoreRateLimit(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->restore_rate_limit;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setrestoratelimiter
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setRestoreRateLimiter(
    JNIEnv* env, jobject jobj, jlong jhandle, jlong jrate_limiter_handle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  auto* sptr_rate_limiter =
      reinterpret_cast<std::shared_ptr<rocksdb::RateLimiter> *>(jrate_limiter_handle);
  bopt->restore_rate_limiter = *sptr_rate_limiter;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：使用校验和设置共享文件
 *签字：（JZ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setShareFilesWithChecksum(
    JNIEnv* env, jobject jobj, jlong jhandle, jboolean flag) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->share_files_with_checksum = flag;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：sharefileswithchecksum
 *签字：（j）Z
 **/

jboolean Java_org_rocksdb_BackupableDBOptions_shareFilesWithChecksum(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return bopt->share_files_with_checksum;
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setmaxbackgroundoperations
 *签名：（Ji）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setMaxBackgroundOperations(
    JNIEnv* env, jobject jobj, jlong jhandle, jint max_background_operations) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->max_background_operations =
      static_cast<int>(max_background_operations);
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：MaxBackgroundOperations
 *签字：（j）I
 **/

jint Java_org_rocksdb_BackupableDBOptions_maxBackgroundOperations(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return static_cast<jint>(bopt->max_background_operations);
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：setCallbackTriggerIntervalSize
 *签字：（JJ）V
 **/

void Java_org_rocksdb_BackupableDBOptions_setCallbackTriggerIntervalSize(
    JNIEnv* env, jobject jobj, jlong jhandle,
    jlong jcallback_trigger_interval_size) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  bopt->callback_trigger_interval_size =
      static_cast<uint64_t>(jcallback_trigger_interval_size);
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：CallbackTriggerIntervalSize
 *签字：（j）j
 **/

jlong Java_org_rocksdb_BackupableDBOptions_callbackTriggerIntervalSize(
    JNIEnv* env, jobject jobj, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  return static_cast<jlong>(bopt->callback_trigger_interval_size);
}

/*
 *类：org_rocksdb_backupabledboptions
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_BackupableDBOptions_disposeInternal(
    JNIEnv* env, jobject jopt, jlong jhandle) {
  auto* bopt = reinterpret_cast<rocksdb::BackupableDBOptions*>(jhandle);
  assert(bopt != nullptr);
  delete bopt;
}
