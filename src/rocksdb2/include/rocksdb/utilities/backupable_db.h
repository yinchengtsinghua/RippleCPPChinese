
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once
#ifndef ROCKSDB_LITE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <string>
#include <map>
#include <vector>
#include <functional>

#include "rocksdb/utilities/stackable_db.h"

#include "rocksdb/env.h"
#include "rocksdb/status.h"

namespace rocksdb {

struct BackupableDBOptions {
//备份文件的存放位置。必须与dbname不同
//最好将其设置为dbname_+“/backups”
//要求的
  std::string backup_dir;

//备份env对象。它将用于备份文件I/O。
//nullptr，将使用dbs env编写备份。如果是
//非nullptr，将使用此对象执行备份的I/O。
//如果要在HDFS上备份，请在此处使用HDFS env！
//默认值：nullptr
  Env* backup_env;

//如果share_table_files==true，备份将假定表文件
//相同的名称有相同的内容。这将启用增量备份和
//避免不必要的数据复制。
//如果share_table_files==false，则每个备份都将独立进行，并且
//不与其他备份共享任何数据。
//默认值：真
  bool share_table_files;

//备份信息和错误信息将写入信息日志
//如果非nulpPTR。
//默认值：nullptr
  Logger* info_log;

//如果sync==true，我们可以保证您得到一致的备份
//在机器崩溃/重新启动时。启用同步后，备份过程较慢。
//如果sync==false，我们不保证机器重新启动时有任何问题。然而，
//一些备份可能是一致的。
//默认值：真
  bool sync;

//如果为真，则将删除已存在的所有备份
//默认：假
  bool destroy_old_data;

//如果为false，则不会备份日志文件。此选项可用于备份
//内存中的数据库，其中保留了日志文件，但表文件在其中
//记忆。
//默认值：真
  bool backup_log_files;

//备份期间每秒可传输的最大字节数。
//如果是0，请尽快
//默认值：0
  uint64_t backup_rate_limit;

//备用速率限制器。用于控制备份的传输速度。如果这是
//不为空，备份速率限制被忽略。
//默认值：nullptr
  std::shared_ptr<RateLimiter> backup_rate_limiter{nullptr};

//在还原过程中每秒可以传输的最大字节数。
//如果是0，请尽快
//默认值：0
  uint64_t restore_rate_limit;

//恢复速率限制器。用于在恢复期间控制传输速度。如果
//这不是空值，还原速率限制被忽略。
//默认值：nullptr
  std::shared_ptr<RateLimiter> restore_rate_limiter{nullptr};

//仅在share_table_files设置为true时使用。如果是真的，会考虑
//备份可以来自不同的数据库，因此SST不是唯一的
//由其名称标识，但由三个（文件名，crc32，文件长度）标识。
//默认：假
//注意：这是一个实验选项，您需要手动设置它。
//*只有当你知道自己在做什么时才打开它*
  bool share_files_with_checksum;

//
//并从备份（）中恢复
//默认值：1
  int max_background_operations;

//在备份期间，用户下次每次都可以得到回调
//正在复制的回调\触发器\间隔\大小字节。
//默认值：4194304
  uint64_t callback_trigger_interval_size;

//当调用open（）时，它将最多打开这么多最新的
//未损坏的备份。如果为0，将打开所有可用备份。
//默认值：0
  int max_valid_backups_to_open;

  void Dump(Logger* logger) const;

  explicit BackupableDBOptions(
      const std::string& _backup_dir, Env* _backup_env = nullptr,
      bool _share_table_files = true, Logger* _info_log = nullptr,
      bool _sync = true, bool _destroy_old_data = false,
      bool _backup_log_files = true, uint64_t _backup_rate_limit = 0,
      uint64_t _restore_rate_limit = 0, int _max_background_operations = 1,
      uint64_t _callback_trigger_interval_size = 4 * 1024 * 1024,
      int _max_valid_backups_to_open = 0)
      : backup_dir(_backup_dir),
        backup_env(_backup_env),
        share_table_files(_share_table_files),
        info_log(_info_log),
        sync(_sync),
        destroy_old_data(_destroy_old_data),
        backup_log_files(_backup_log_files),
        backup_rate_limit(_backup_rate_limit),
        restore_rate_limit(_restore_rate_limit),
        share_files_with_checksum(false),
        max_background_operations(_max_background_operations),
        callback_trigger_interval_size(_callback_trigger_interval_size),
        max_valid_backups_to_open(_max_valid_backups_to_open) {
    assert(share_table_files || !share_files_with_checksum);
  }
};

struct RestoreOptions {
//如果为true，则restore不会覆盖wal-dir中的现有日志文件。它将
//同时将所有日志文件从归档目录移到wal_dir。使用此选项
//与backupabledboptions结合使用：：backup_log_files=false for
//保存内存数据库。
//默认：假
  bool keep_log_files;

  explicit RestoreOptions(bool _keep_log_files = false)
      : keep_log_files(_keep_log_files) {}
};

typedef uint32_t BackupID;

struct BackupInfo {
  BackupID backup_id;
  int64_t timestamp;
  uint64_t size;

  uint32_t number_files;
  std::string app_metadata;

  BackupInfo() {}

  BackupInfo(BackupID _backup_id, int64_t _timestamp, uint64_t _size,
             uint32_t _number_files, const std::string& _app_metadata)
      : backup_id(_backup_id),
        timestamp(_timestamp),
        size(_size),
        number_files(_number_files),
        app_metadata(_app_metadata) {}
};

class BackupStatistics {
 public:
  BackupStatistics() {
    number_success_backup = 0;
    number_fail_backup = 0;
  }

  BackupStatistics(uint32_t _number_success_backup,
                   uint32_t _number_fail_backup)
      : number_success_backup(_number_success_backup),
        number_fail_backup(_number_fail_backup) {}

  ~BackupStatistics() {}

  void IncrementNumberSuccessBackup();
  void IncrementNumberFailBackup();

  uint32_t GetNumberSuccessBackup() const;
  uint32_t GetNumberFailBackup() const;

  std::string ToString() const;

 private:
  uint32_t number_success_backup;
  uint32_t number_fail_backup;
};

//用于访问有关备份和从中恢复的信息的备份引擎
//他们。
class BackupEngineReadOnly {
 public:
  virtual ~BackupEngineReadOnly() {}

  static Status Open(Env* db_env, const BackupableDBOptions& options,
                     BackupEngineReadOnly** backup_engine_ptr);

//在备份信息中返回有关备份的信息
//即使其他备份引擎正在执行，您也可以安全地获取备份信息。
//在同一目录上备份
  virtual void GetBackupInfo(std::vector<BackupInfo>* backup_info) = 0;

//返回有关损坏的备份中损坏的备份的信息
  virtual void GetCorruptedBackups(
      std::vector<BackupID>* corrupt_backup_ids) = 0;

//当存在另一个备份引擎时，从备份还原数据库不安全
//运行它可能调用deletebackup（）或purgeoldbackups（）。这是来电者的
//同步操作的责任，即不要删除备份
//当你从中恢复的时候
//另请参见backupengine中的相应文档
  virtual Status RestoreDBFromBackup(
      BackupID backup_id, const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) = 0;

//参见backupengine中的相应文档
  virtual Status RestoreDBFromLatestBackup(
      const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) = 0;

//检查每个文件是否存在以及文件大小是否与
//期望。它不检查文件校验和。
//
//如果此备份引擎创建了备份，则会比较文件的当前
//根据创建期间写入它们的字节数调整大小。
//否则，当
//备份引擎已打开。
//
//如果所有检查都正常，则返回status:：ok（）。
  virtual Status VerifyBackup(BackupID backup_id) = 0;
};

//用于创建新备份的备份引擎。
class BackupEngine {
 public:
  virtual ~BackupEngine() {}

//backupabledboptions必须与前面使用的选项相同
//同一备份目录的备份引擎。
  static Status Open(Env* db_env,
                     const BackupableDBOptions& options,
                     BackupEngine** backup_engine_ptr);

//与CreateNewBackup相同，但存储额外的应用程序元数据
//如果启用2PC，则始终触发刷新。
  virtual Status CreateNewBackupWithMetadata(
      DB* db, const std::string& app_metadata, bool flush_before_backup = false,
      std::function<void()> progress_callback = []() {}) = 0;

//捕获最新备份中数据库的状态
//不是线程安全调用
//如果启用2PC，则始终触发刷新。
  virtual Status CreateNewBackup(DB* db, bool flush_before_backup = false,
                                 std::function<void()> progress_callback =
                                     []() {}) {
    return CreateNewBackupWithMetadata(db, "", flush_before_backup,
                                       progress_callback);
  }

//删除旧备份，保留最新的num备份
  virtual Status PurgeOldBackups(uint32_t num_backups_to_keep) = 0;

//删除特定备份
  virtual Status DeleteBackup(BackupID backup_id) = 0;

//如果要停止备份，请从其他线程调用此函数
//这是目前正在发生的。它马上就会回来，威尔
//不等待备份停止。
//备份将尽快停止，调用CreateNewbackup将
//返回状态：：不完整（）。它不会自己清理，但是
//国家将保持一致。状态将被清除
//下次创建backupabledb或restorebackupabledb时。
  virtual void StopBackup() = 0;

//在备份信息中返回有关备份的信息
  virtual void GetBackupInfo(std::vector<BackupInfo>* backup_info) = 0;

//返回有关损坏的备份中损坏的备份的信息
  virtual void GetCorruptedBackups(
      std::vector<BackupID>* corrupt_backup_ids) = 0;

//使用备份ID从备份还原
//重要信息—如果选项uu.share_table_files==true，
//选项_u share_files_with_checksum==false，您可以从
//备份不是最新的，并且您开始从
//新的数据库，他们可能会失败。
//
//示例：假设您有备份1、2、3、4、5，并且恢复了3。
//如果向数据库添加新数据并尝试立即创建新备份，则
//数据库将偏离备份4和5，新备份将失败。
//如果要创建新备份，首先必须删除备份4
//5。
  virtual Status RestoreDBFromBackup(
      BackupID backup_id, const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) = 0;

//从最新备份还原
  virtual Status RestoreDBFromLatestBackup(
      const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) = 0;

//检查每个文件是否存在以及文件大小是否与
//期望。它不检查文件校验和。
//如果所有检查都正常，则返回status:：ok（）。
  virtual Status VerifyBackup(BackupID backup_id) = 0;

//将删除我们不再需要的所有文件
//它将对文件/目录进行完全扫描，并删除所有
//未引用的文件。
  virtual Status GarbageCollect() = 0;
};

}  //命名空间rocksdb
#endif  //摇滚乐
