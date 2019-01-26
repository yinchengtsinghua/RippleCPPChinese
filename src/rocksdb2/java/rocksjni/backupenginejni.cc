
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
//从Java端调用C++ROCSDB:：BuffEngEngress方法。

#include <jni.h>
#include <vector>

#include "include/org_rocksdb_BackupEngine.h"
#include "rocksdb/utilities/backupable_db.h"
#include "rocksjni/portal.h"

/*
 *类：org_rocksdb_backupengine
 *方法：打开
 *签字：（JJ）J
 **/

jlong Java_org_rocksdb_BackupEngine_open(
    JNIEnv* env, jclass jcls, jlong env_handle,
    jlong backupable_db_options_handle) {
  auto* rocks_env = reinterpret_cast<rocksdb::Env*>(env_handle);
  auto* backupable_db_options =
      reinterpret_cast<rocksdb::BackupableDBOptions*>(
      backupable_db_options_handle);
  rocksdb::BackupEngine* backup_engine;
  auto status = rocksdb::BackupEngine::Open(rocks_env,
      *backupable_db_options, &backup_engine);

  if (status.ok()) {
    return reinterpret_cast<jlong>(backup_engine);
  } else {
    rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
    return 0;
  }
}

/*
 *类：org_rocksdb_backupengine
 *方法：创建新备份
 *签字：（JJZ）V
 **/

void Java_org_rocksdb_BackupEngine_createNewBackup(
    JNIEnv* env, jobject jbe, jlong jbe_handle, jlong db_handle,
    jboolean jflush_before_backup) {
  auto* db = reinterpret_cast<rocksdb::DB*>(db_handle);
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  auto status = backup_engine->CreateNewBackup(db,
      static_cast<bool>(jflush_before_backup));

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：getbackupinfo
 *签名：（j）ljava/util/list；
 **/

jobject Java_org_rocksdb_BackupEngine_getBackupInfo(
    JNIEnv* env, jobject jbe, jlong jbe_handle) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  std::vector<rocksdb::BackupInfo> backup_infos;
  backup_engine->GetBackupInfo(&backup_infos);
  return rocksdb::BackupInfoListJni::getBackupInfo(env, backup_infos);
}

/*
 *类：org_rocksdb_backupengine
 *方法：getcorruptedbackup
 *签字：（j）[i
 **/

jintArray Java_org_rocksdb_BackupEngine_getCorruptedBackups(
    JNIEnv* env, jobject jbe, jlong jbe_handle) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  std::vector<rocksdb::BackupID> backup_ids;
  backup_engine->GetCorruptedBackups(&backup_ids);
//在int数组中存储backupid
  std::vector<jint> int_backup_ids(backup_ids.begin(), backup_ids.end());
  
//用Java数组存储int
//这里可以放宽精度（64->32）
  jsize ret_backup_ids_size = static_cast<jsize>(backup_ids.size());
  jintArray ret_backup_ids = env->NewIntArray(ret_backup_ids_size);
  if(ret_backup_ids == nullptr) {
//引发异常：OutOfMemoryError
    return nullptr;
  }
  env->SetIntArrayRegion(ret_backup_ids, 0, ret_backup_ids_size,
      int_backup_ids.data());
  return ret_backup_ids;
}

/*
 *类：org_rocksdb_backupengine
 *方法：垃圾收集
 *签名：（j）v
 **/

void Java_org_rocksdb_BackupEngine_garbageCollect(
    JNIEnv* env, jobject jbe, jlong jbe_handle) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  auto status = backup_engine->GarbageCollect();

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：purgeoldbackups
 *签名：（Ji）V
 **/

void Java_org_rocksdb_BackupEngine_purgeOldBackups(
    JNIEnv* env, jobject jbe, jlong jbe_handle, jint jnum_backups_to_keep) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  auto status =
      backup_engine->
          PurgeOldBackups(static_cast<uint32_t>(jnum_backups_to_keep));

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：删除备份
 *签名：（Ji）V
 **/

void Java_org_rocksdb_BackupEngine_deleteBackup(
    JNIEnv* env, jobject jbe, jlong jbe_handle, jint jbackup_id) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  auto status =
      backup_engine->DeleteBackup(static_cast<rocksdb::BackupID>(jbackup_id));

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：从备份中恢复
 *签名：（jiljava/lang/string；ljava/lang/string；j）v
 **/

void Java_org_rocksdb_BackupEngine_restoreDbFromBackup(
    JNIEnv* env, jobject jbe, jlong jbe_handle, jint jbackup_id,
    jstring jdb_dir, jstring jwal_dir, jlong jrestore_options_handle) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  const char* db_dir = env->GetStringUTFChars(jdb_dir, nullptr);
  if(db_dir == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  const char* wal_dir = env->GetStringUTFChars(jwal_dir, nullptr);
  if(wal_dir == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseStringUTFChars(jdb_dir, db_dir);
    return;
  }
  auto* restore_options =
      reinterpret_cast<rocksdb::RestoreOptions*>(jrestore_options_handle);
  auto status =
      backup_engine->RestoreDBFromBackup(
          static_cast<rocksdb::BackupID>(jbackup_id), db_dir, wal_dir,
          *restore_options);

  env->ReleaseStringUTFChars(jwal_dir, wal_dir);
  env->ReleaseStringUTFChars(jdb_dir, db_dir);

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：从最新备份中恢复
 *签名：（j ljava/lang/string；ljava/lang/string；j）v
 **/

void Java_org_rocksdb_BackupEngine_restoreDbFromLatestBackup(
    JNIEnv* env, jobject jbe, jlong jbe_handle, jstring jdb_dir,
    jstring jwal_dir, jlong jrestore_options_handle) {
  auto* backup_engine = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  const char* db_dir = env->GetStringUTFChars(jdb_dir, nullptr);
  if(db_dir == nullptr) {
//引发异常：OutOfMemoryError
    return;
  }
  const char* wal_dir = env->GetStringUTFChars(jwal_dir, nullptr);
  if(wal_dir == nullptr) {
//引发异常：OutOfMemoryError
    env->ReleaseStringUTFChars(jdb_dir, db_dir);
    return;
  }
  auto* restore_options =
      reinterpret_cast<rocksdb::RestoreOptions*>(jrestore_options_handle);
  auto status =
      backup_engine->RestoreDBFromLatestBackup(db_dir, wal_dir,
          *restore_options);

  env->ReleaseStringUTFChars(jwal_dir, wal_dir);
  env->ReleaseStringUTFChars(jdb_dir, db_dir);

  if (status.ok()) {
    return;
  }

  rocksdb::RocksDBExceptionJni::ThrowNew(env, status);
}

/*
 *类：org_rocksdb_backupengine
 *方法：外用
 *签名：（j）v
 **/

void Java_org_rocksdb_BackupEngine_disposeInternal(
    JNIEnv* env, jobject jbe, jlong jbe_handle) {
  auto* be = reinterpret_cast<rocksdb::BackupEngine*>(jbe_handle);
  assert(be != nullptr);
  delete be;
}
