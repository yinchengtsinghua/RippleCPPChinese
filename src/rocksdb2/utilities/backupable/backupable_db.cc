
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

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/backupable_db.h"
#include "port/port.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/transaction_log.h"
#include "util/channel.h"
#include "util/coding.h"
#include "util/crc32c.h"
#include "util/file_reader_writer.h"
#include "util/filename.h"
#include "util/logging.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "utilities/checkpoint/checkpoint_impl.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif  //_uu stdc_format_宏

#include <inttypes.h>
#include <stdlib.h>
#include <algorithm>
#include <atomic>
#include <functional>
#include <future>
#include <limits>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rocksdb {

void BackupStatistics::IncrementNumberSuccessBackup() {
  number_success_backup++;
}
void BackupStatistics::IncrementNumberFailBackup() {
  number_fail_backup++;
}

uint32_t BackupStatistics::GetNumberSuccessBackup() const {
  return number_success_backup;
}
uint32_t BackupStatistics::GetNumberFailBackup() const {
  return number_fail_backup;
}

std::string BackupStatistics::ToString() const {
  char result[50];
  snprintf(result, sizeof(result), "# success backup: %u, # fail backup: %u",
           GetNumberSuccessBackup(), GetNumberFailBackup());
  return result;
}

void BackupableDBOptions::Dump(Logger* logger) const {
  ROCKS_LOG_INFO(logger, "               Options.backup_dir: %s",
                 backup_dir.c_str());
  ROCKS_LOG_INFO(logger, "               Options.backup_env: %p", backup_env);
  ROCKS_LOG_INFO(logger, "        Options.share_table_files: %d",
                 static_cast<int>(share_table_files));
  ROCKS_LOG_INFO(logger, "                 Options.info_log: %p", info_log);
  ROCKS_LOG_INFO(logger, "                     Options.sync: %d",
                 static_cast<int>(sync));
  ROCKS_LOG_INFO(logger, "         Options.destroy_old_data: %d",
                 static_cast<int>(destroy_old_data));
  ROCKS_LOG_INFO(logger, "         Options.backup_log_files: %d",
                 static_cast<int>(backup_log_files));
  ROCKS_LOG_INFO(logger, "        Options.backup_rate_limit: %" PRIu64,
                 backup_rate_limit);
  ROCKS_LOG_INFO(logger, "       Options.restore_rate_limit: %" PRIu64,
                 restore_rate_limit);
  ROCKS_LOG_INFO(logger, "Options.max_background_operations: %d",
                 max_background_operations);
}

//--------backupengineimpl类-------
class BackupEngineImpl : public BackupEngine {
 public:
  BackupEngineImpl(Env* db_env, const BackupableDBOptions& options,
                   bool read_only = false);
  ~BackupEngineImpl();
  Status CreateNewBackupWithMetadata(DB* db, const std::string& app_metadata,
                                     bool flush_before_backup = false,
                                     std::function<void()> progress_callback =
                                         []() {}) override;
  Status PurgeOldBackups(uint32_t num_backups_to_keep) override;
  Status DeleteBackup(BackupID backup_id) override;
  void StopBackup() override {
    stop_backup_.store(true, std::memory_order_release);
  }
  Status GarbageCollect() override;

//返回的备份信息按时间顺序排列，这意味着
//最新备份排在最后。
  void GetBackupInfo(std::vector<BackupInfo>* backup_info) override;
  void GetCorruptedBackups(std::vector<BackupID>* corrupt_backup_ids) override;
  Status RestoreDBFromBackup(
      BackupID backup_id, const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) override;
  Status RestoreDBFromLatestBackup(
      const std::string& db_dir, const std::string& wal_dir,
      const RestoreOptions& restore_options = RestoreOptions()) override {
    return RestoreDBFromBackup(latest_backup_id_, db_dir, wal_dir,
                               restore_options);
  }

  virtual Status VerifyBackup(BackupID backup_id) override;

  Status Initialize();

 private:
  void DeleteChildren(const std::string& dir, uint32_t file_type_filter = 0);

//使用pathname->size映射扩展“result”映射
//“env”中的“dir”。路径名的前缀是“dir”。
  Status InsertPathnameToSizeBytes(
      const std::string& dir, Env* env,
      std::unordered_map<std::string, uint64_t>* result);

  struct FileInfo {
    FileInfo(const std::string& fname, uint64_t sz, uint32_t checksum)
      : refs(0), filename(fname), size(sz), checksum_value(checksum) {}

    FileInfo(const FileInfo&) = delete;
    FileInfo& operator=(const FileInfo&) = delete;

    int refs;
    const std::string filename;
    const uint64_t size;
    const uint32_t checksum_value;
  };

  class BackupMeta {
   public:
    BackupMeta(const std::string& meta_filename,
        std::unordered_map<std::string, std::shared_ptr<FileInfo>>* file_infos,
        Env* env)
      : timestamp_(0), size_(0), meta_filename_(meta_filename),
        file_infos_(file_infos), env_(env) {}

    BackupMeta(const BackupMeta&) = delete;
    BackupMeta& operator=(const BackupMeta&) = delete;

    ~BackupMeta() {}

    void RecordTimestamp() {
      env_->GetCurrentTime(&timestamp_);
    }
    int64_t GetTimestamp() const {
      return timestamp_;
    }
    uint64_t GetSize() const {
      return size_;
    }
    uint32_t GetNumberFiles() { return static_cast<uint32_t>(files_.size()); }
    void SetSequenceNumber(uint64_t sequence_number) {
      sequence_number_ = sequence_number;
    }
    uint64_t GetSequenceNumber() {
      return sequence_number_;
    }

    const std::string& GetAppMetadata() const { return app_metadata_; }

    void SetAppMetadata(const std::string& app_metadata) {
      app_metadata_ = app_metadata;
    }

    Status AddFile(std::shared_ptr<FileInfo> file_info);

    Status Delete(bool delete_meta = true);

    bool Empty() {
      return files_.empty();
    }

    std::shared_ptr<FileInfo> GetFile(const std::string& filename) const {
      auto it = file_infos_->find(filename);
      if (it == file_infos_->end())
        return nullptr;
      return it->second;
    }

    const std::vector<std::shared_ptr<FileInfo>>& GetFiles() {
      return files_;
    }

//@param abs_path_to_size预取文件大小（字节）。
    Status LoadFromFile(
        const std::string& backup_dir,
        const std::unordered_map<std::string, uint64_t>& abs_path_to_size);
    Status StoreToFile(bool sync);

    std::string GetInfoString() {
      std::ostringstream ss;
      ss << "Timestamp: " << timestamp_ << std::endl;
      char human_size[16];
      AppendHumanBytes(size_, human_size, sizeof(human_size));
      ss << "Size: " << human_size << std::endl;
      ss << "Files:" << std::endl;
      for (const auto& file : files_) {
        AppendHumanBytes(file->size, human_size, sizeof(human_size));
        ss << file->filename << ", size " << human_size << ", refs "
           << file->refs << std::endl;
      }
      return ss.str();
    }

   private:
    int64_t timestamp_;
//序列号仅为近似值，不应使用
//由客户
    uint64_t sequence_number_;
    uint64_t size_;
    std::string app_metadata_;
    std::string const meta_filename_;
//具有相对路径的文件（不带“/”前缀！！）
    std::vector<std::shared_ptr<FileInfo>> files_;
    std::unordered_map<std::string, std::shared_ptr<FileInfo>>* file_infos_;
    Env* env_;

static const size_t max_backup_meta_file_size_ = 10 * 1024 * 1024;  //10MB
};  //反向元

  inline std::string GetAbsolutePath(
      const std::string &relative_path = "") const {
    assert(relative_path.size() == 0 || relative_path[0] != '/');
    return options_.backup_dir + "/" + relative_path;
  }
  inline std::string GetPrivateDirRel() const {
    return "private";
  }
  inline std::string GetSharedChecksumDirRel() const {
    return "shared_checksum";
  }
  inline std::string GetPrivateFileRel(BackupID backup_id,
                                       bool tmp = false,
                                       const std::string& file = "") const {
    assert(file.size() == 0 || file[0] != '/');
    return GetPrivateDirRel() + "/" + rocksdb::ToString(backup_id) +
           (tmp ? ".tmp" : "") + "/" + file;
  }
  inline std::string GetSharedFileRel(const std::string& file = "",
                                      bool tmp = false) const {
    assert(file.size() == 0 || file[0] != '/');
    return "shared/" + file + (tmp ? ".tmp" : "");
  }
  inline std::string GetSharedFileWithChecksumRel(const std::string& file = "",
                                                  bool tmp = false) const {
    assert(file.size() == 0 || file[0] != '/');
    return GetSharedChecksumDirRel() + "/" + file + (tmp ? ".tmp" : "");
  }
  inline std::string GetSharedFileWithChecksum(const std::string& file,
                                               const uint32_t checksum_value,
                                               const uint64_t file_size) const {
    assert(file.size() == 0 || file[0] != '/');
    std::string file_copy = file;
    return file_copy.insert(file_copy.find_last_of('.'),
                            "_" + rocksdb::ToString(checksum_value) + "_" +
                                rocksdb::ToString(file_size));
  }
  inline std::string GetFileFromChecksumFile(const std::string& file) const {
    assert(file.size() == 0 || file[0] != '/');
    std::string file_copy = file;
    size_t first_underscore = file_copy.find_first_of('_');
    return file_copy.erase(first_underscore,
                           file_copy.find_last_of('.') - first_underscore);
  }
  inline std::string GetBackupMetaDir() const {
    return GetAbsolutePath("meta");
  }
  inline std::string GetBackupMetaFile(BackupID backup_id) const {
    return GetBackupMetaDir() + "/" + rocksdb::ToString(backup_id);
  }

//如果大小限制=0，则没有大小限制，请复制所有内容。
//
//只有一个src和contents必须是非空的。
//
//@param src如果非空，则从该路径名复制该文件。
//@param contents如果非空，文件将用这些内容创建。
  Status CopyOrCreateFile(const std::string& src, const std::string& dst,
                          const std::string& contents, Env* src_env,
                          Env* dst_env, bool sync, RateLimiter* rate_limiter,
                          uint64_t* size = nullptr,
                          uint32_t* checksum_value = nullptr,
                          uint64_t size_limit = 0,
                          std::function<void()> progress_callback = []() {});

  Status CalculateChecksum(const std::string& src,
                           Env* src_env,
                           uint64_t size_limit,
                           uint32_t* checksum_value);

  struct CopyOrCreateResult {
    uint64_t size;
    uint32_t checksum_value;
    Status status;
  };

//只有一个src_路径和内容必须是非空的。如果SRCYPATH是
//非空，文件将从此路径名复制。否则，如果内容是
//非空文件将在dst_路径创建，其中包含这些内容。
  struct CopyOrCreateWorkItem {
    std::string src_path;
    std::string dst_path;
    std::string contents;
    Env* src_env;
    Env* dst_env;
    bool sync;
    RateLimiter* rate_limiter;
    uint64_t size_limit;
    std::promise<CopyOrCreateResult> result;
    std::function<void()> progress_callback;

    CopyOrCreateWorkItem() {}
    CopyOrCreateWorkItem(const CopyOrCreateWorkItem&) = delete;
    CopyOrCreateWorkItem& operator=(const CopyOrCreateWorkItem&) = delete;

    CopyOrCreateWorkItem(CopyOrCreateWorkItem&& o) ROCKSDB_NOEXCEPT {
      *this = std::move(o);
    }

    CopyOrCreateWorkItem& operator=(CopyOrCreateWorkItem&& o) ROCKSDB_NOEXCEPT {
      src_path = std::move(o.src_path);
      dst_path = std::move(o.dst_path);
      contents = std::move(o.contents);
      src_env = o.src_env;
      dst_env = o.dst_env;
      sync = o.sync;
      rate_limiter = o.rate_limiter;
      size_limit = o.size_limit;
      result = std::move(o.result);
      progress_callback = std::move(o.progress_callback);
      return *this;
    }

    CopyOrCreateWorkItem(std::string _src_path, std::string _dst_path,
                         std::string _contents, Env* _src_env, Env* _dst_env,
                         bool _sync, RateLimiter* _rate_limiter,
                         uint64_t _size_limit,
                         std::function<void()> _progress_callback = []() {})
        : src_path(std::move(_src_path)),
          dst_path(std::move(_dst_path)),
          contents(std::move(_contents)),
          src_env(_src_env),
          dst_env(_dst_env),
          sync(_sync),
          rate_limiter(_rate_limiter),
          size_limit(_size_limit),
          progress_callback(_progress_callback) {}
  };

  struct BackupAfterCopyOrCreateWorkItem {
    std::future<CopyOrCreateResult> result;
    bool shared;
    bool needed_to_copy;
    Env* backup_env;
    std::string dst_path_tmp;
    std::string dst_path;
    std::string dst_relative;
    BackupAfterCopyOrCreateWorkItem() {}

    BackupAfterCopyOrCreateWorkItem(BackupAfterCopyOrCreateWorkItem&& o)
        ROCKSDB_NOEXCEPT {
      *this = std::move(o);
    }

    BackupAfterCopyOrCreateWorkItem& operator=(
        BackupAfterCopyOrCreateWorkItem&& o) ROCKSDB_NOEXCEPT {
      result = std::move(o.result);
      shared = o.shared;
      needed_to_copy = o.needed_to_copy;
      backup_env = o.backup_env;
      dst_path_tmp = std::move(o.dst_path_tmp);
      dst_path = std::move(o.dst_path);
      dst_relative = std::move(o.dst_relative);
      return *this;
    }

    BackupAfterCopyOrCreateWorkItem(std::future<CopyOrCreateResult>&& _result,
                                    bool _shared, bool _needed_to_copy,
                                    Env* _backup_env, std::string _dst_path_tmp,
                                    std::string _dst_path,
                                    std::string _dst_relative)
        : result(std::move(_result)),
          shared(_shared),
          needed_to_copy(_needed_to_copy),
          backup_env(_backup_env),
          dst_path_tmp(std::move(_dst_path_tmp)),
          dst_path(std::move(_dst_path)),
          dst_relative(std::move(_dst_relative)) {}
  };

  struct RestoreAfterCopyOrCreateWorkItem {
    std::future<CopyOrCreateResult> result;
    uint32_t checksum_value;
    RestoreAfterCopyOrCreateWorkItem() {}
    RestoreAfterCopyOrCreateWorkItem(std::future<CopyOrCreateResult>&& _result,
                                     uint32_t _checksum_value)
        : result(std::move(_result)), checksum_value(_checksum_value) {}
    RestoreAfterCopyOrCreateWorkItem(RestoreAfterCopyOrCreateWorkItem&& o)
        ROCKSDB_NOEXCEPT {
      *this = std::move(o);
    }

    RestoreAfterCopyOrCreateWorkItem& operator=(
        RestoreAfterCopyOrCreateWorkItem&& o) ROCKSDB_NOEXCEPT {
      result = std::move(o.result);
      checksum_value = o.checksum_value;
      return *this;
    }
  };

  bool initialized_;
  std::mutex byte_report_mutex_;
  channel<CopyOrCreateWorkItem> files_to_copy_or_create_;
  std::vector<port::Thread> threads_;

//将文件添加到要复制或创建的备份工作队列（如果不复制或创建）
//已经存在。
//
//只有一个src_dir和contents必须是非空的。
//
//@param src_dir如果不为空，则此目录中名为fname的文件将
//复制。
//@param fname目标文件的名称，如果是副本，则为源文件。
//@param contents如果非空，文件将用这些内容创建。
  Status AddBackupFileWorkItem(
      std::unordered_set<std::string>& live_dst_paths,
      std::vector<BackupAfterCopyOrCreateWorkItem>& backup_items_to_finish,
      BackupID backup_id, bool shared, const std::string& src_dir,
const std::string& fname,  //以“/”开始
      RateLimiter* rate_limiter, uint64_t size_bytes, uint64_t size_limit = 0,
      bool shared_checksum = false,
      std::function<void()> progress_callback = []() {},
      const std::string& contents = std::string());

//备份状态数据
  BackupID latest_backup_id_;
  std::map<BackupID, unique_ptr<BackupMeta>> backups_;
  std::map<BackupID,
           std::pair<Status, unique_ptr<BackupMeta>>> corrupt_backups_;
  std::unordered_map<std::string,
                     std::shared_ptr<FileInfo>> backuped_file_infos_;
  std::atomic<bool> stop_backup_;

//选项数据
  BackupableDBOptions options_;
  Env* db_env_;
  Env* backup_env_;

//目录
  unique_ptr<Directory> backup_directory_;
  unique_ptr<Directory> shared_directory_;
  unique_ptr<Directory> meta_directory_;
  unique_ptr<Directory> private_directory_;

static const size_t kDefaultCopyFileBufferSize = 5 * 1024 * 1024LL;  //5MB
  size_t copy_file_buffer_size_;
  bool read_only_;
  BackupStatistics backup_statistics_;
static const size_t kMaxAppMetaSize = 1024 * 1024;  //1MB
};

Status BackupEngine::Open(Env* env, const BackupableDBOptions& options,
                          BackupEngine** backup_engine_ptr) {
  std::unique_ptr<BackupEngineImpl> backup_engine(
      new BackupEngineImpl(env, options));
  auto s = backup_engine->Initialize();
  if (!s.ok()) {
    *backup_engine_ptr = nullptr;
    return s;
  }
  *backup_engine_ptr = backup_engine.release();
  return Status::OK();
}

BackupEngineImpl::BackupEngineImpl(Env* db_env,
                                   const BackupableDBOptions& options,
                                   bool read_only)
    : initialized_(false),
      stop_backup_(false),
      options_(options),
      db_env_(db_env),
      backup_env_(options.backup_env != nullptr ? options.backup_env : db_env_),
      copy_file_buffer_size_(kDefaultCopyFileBufferSize),
      read_only_(read_only) {
  if (options_.backup_rate_limiter == nullptr &&
      options_.backup_rate_limit > 0) {
    options_.backup_rate_limiter.reset(
        NewGenericRateLimiter(options_.backup_rate_limit));
  }
  if (options_.restore_rate_limiter == nullptr &&
      options_.restore_rate_limit > 0) {
    options_.restore_rate_limiter.reset(
        NewGenericRateLimiter(options_.restore_rate_limit));
  }
}

BackupEngineImpl::~BackupEngineImpl() {
  files_to_copy_or_create_.sendEof();
  for (auto& t : threads_) {
    t.join();
  }
  LogFlush(options_.info_log);
}

Status BackupEngineImpl::Initialize() {
  assert(!initialized_);
  initialized_ = true;
  if (read_only_) {
    ROCKS_LOG_INFO(options_.info_log, "Starting read_only backup engine");
  }
  options_.Dump(options_.info_log);

  if (!read_only_) {
//收集我们需要创建的目录列表
    std::vector<std::pair<std::string, std::unique_ptr<Directory>*>>
        directories;
    directories.emplace_back(GetAbsolutePath(), &backup_directory_);
    if (options_.share_table_files) {
      if (options_.share_files_with_checksum) {
        directories.emplace_back(
            GetAbsolutePath(GetSharedFileWithChecksumRel()),
            &shared_directory_);
      } else {
        directories.emplace_back(GetAbsolutePath(GetSharedFileRel()),
                                 &shared_directory_);
      }
    }
    directories.emplace_back(GetAbsolutePath(GetPrivateDirRel()),
                             &private_directory_);
    directories.emplace_back(GetBackupMetaDir(), &meta_directory_);
//创造我们所需要的一切
    for (const auto& d : directories) {
      auto s = backup_env_->CreateDirIfMissing(d.first);
      if (s.ok()) {
        s = backup_env_->NewDirectory(d.first, d.second);
      }
      if (!s.ok()) {
        return s;
      }
    }
  }

  std::vector<std::string> backup_meta_files;
  {
    auto s = backup_env_->GetChildren(GetBackupMetaDir(), &backup_meta_files);
    if (s.IsNotFound()) {
      return Status::NotFound(GetBackupMetaDir() + " is missing");
    } else if (!s.ok()) {
      return s;
    }
  }
//创建备份结构
  for (auto& file : backup_meta_files) {
    if (file == "." || file == "..") {
      continue;
    }
    ROCKS_LOG_INFO(options_.info_log, "Detected backup %s", file.c_str());
    BackupID backup_id = 0;
    sscanf(file.c_str(), "%u", &backup_id);
    if (backup_id == 0 || file != rocksdb::ToString(backup_id)) {
      if (!read_only_) {
//文件名无效，请删除
        auto s = backup_env_->DeleteFile(GetBackupMetaDir() + "/" + file);
        ROCKS_LOG_INFO(options_.info_log,
                       "Unrecognized meta file %s, deleting -- %s",
                       file.c_str(), s.ToString().c_str());
      }
      continue;
    }
    assert(backups_.find(backup_id) == backups_.end());
    backups_.insert(
        std::make_pair(backup_id, unique_ptr<BackupMeta>(new BackupMeta(
                                      GetBackupMetaFile(backup_id),
                                      &backuped_file_infos_, backup_env_))));
  }

  latest_backup_id_ = 0;
if (options_.destroy_old_data) {  //销毁旧数据
    assert(!read_only_);
    ROCKS_LOG_INFO(
        options_.info_log,
        "Backup Engine started with destroy_old_data == true, deleting all "
        "backups");
    auto s = PurgeOldBackups(0);
    if (s.ok()) {
      s = GarbageCollect();
    }
    if (!s.ok()) {
      return s;
    }
} else {  //从存储中加载数据
    std::unordered_map<std::string, uint64_t> abs_path_to_size;
    for (const auto& rel_dir :
         {GetSharedFileRel(), GetSharedFileWithChecksumRel()}) {
      const auto abs_dir = GetAbsolutePath(rel_dir);
      InsertPathnameToSizeBytes(abs_dir, backup_env_, &abs_path_to_size);
    }
//加载备份（如果有），直到有效的备份打开
//已成功打开未损坏的备份。
    int valid_backups_to_open;
    if (options_.max_valid_backups_to_open == 0) {
      valid_backups_to_open = INT_MAX;
    } else {
      valid_backups_to_open = options_.max_valid_backups_to_open;
    }
    for (auto backup_iter = backups_.rbegin();
         backup_iter != backups_.rend() && valid_backups_to_open > 0;
         ++backup_iter) {
      InsertPathnameToSizeBytes(
          GetAbsolutePath(GetPrivateFileRel(backup_iter->first)), backup_env_,
          &abs_path_to_size);
      Status s = backup_iter->second->LoadFromFile(options_.backup_dir,
                                                   abs_path_to_size);
      if (s.IsCorruption()) {
        ROCKS_LOG_INFO(options_.info_log, "Backup %u corrupted -- %s",
                       backup_iter->first, s.ToString().c_str());
        corrupt_backups_.insert(
            std::make_pair(backup_iter->first,
                           std::make_pair(s, std::move(backup_iter->second))));
      } else if (!s.ok()) {
//区分损坏错误和备份环境中的错误。
//备份环境（即此代码路径）中的错误将导致open（）
//失败，而损坏错误不会导致open（）失败。
        return s;
      } else {
        ROCKS_LOG_INFO(options_.info_log, "Loading backup %" PRIu32 " OK:\n%s",
                       backup_iter->first,
                       backup_iter->second->GetInfoString().c_str());
        latest_backup_id_ = std::max(latest_backup_id_, backup_iter->first);
        --valid_backups_to_open;
      }
    }

    for (const auto& corrupt : corrupt_backups_) {
      backups_.erase(backups_.find(corrupt.first));
    }
//在最大有效备份打开之前清除备份
    int num_unopened_backups;
    if (options_.max_valid_backups_to_open == 0) {
      num_unopened_backups = 0;
    } else {
      num_unopened_backups =
          std::max(0, static_cast<int>(backups_.size()) -
                          options_.max_valid_backups_to_open);
    }
    for (int i = 0; i < num_unopened_backups; ++i) {
      assert(backups_.begin()->second->Empty());
      backups_.erase(backups_.begin());
    }
  }

  ROCKS_LOG_INFO(options_.info_log, "Latest backup is %u", latest_backup_id_);

//设置线程在
//背景
  for (int t = 0; t < options_.max_background_operations; t++) {
    threads_.emplace_back([this]() {
#if defined(_GNU_SOURCE) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
      pthread_setname_np(pthread_self(), "backup_engine");
#endif
#endif
      CopyOrCreateWorkItem work_item;
      while (files_to_copy_or_create_.read(work_item)) {
        CopyOrCreateResult result;
        result.status = CopyOrCreateFile(
            work_item.src_path, work_item.dst_path, work_item.contents,
            work_item.src_env, work_item.dst_env, work_item.sync,
            work_item.rate_limiter, &result.size, &result.checksum_value,
            work_item.size_limit, work_item.progress_callback);
        work_item.result.set_value(std::move(result));
      }
    });
  }
  ROCKS_LOG_INFO(options_.info_log, "Initialized BackupEngine");

  return Status::OK();
}

Status BackupEngineImpl::CreateNewBackupWithMetadata(
    DB* db, const std::string& app_metadata, bool flush_before_backup,
    std::function<void()> progress_callback) {
  assert(initialized_);
  assert(!read_only_);
  if (app_metadata.size() > kMaxAppMetaSize) {
    return Status::InvalidArgument("App metadata too large");
  }

  BackupID new_backup_id = latest_backup_id_ + 1;

  assert(backups_.find(new_backup_id) == backups_.end());
  auto ret = backups_.insert(
      std::make_pair(new_backup_id, unique_ptr<BackupMeta>(new BackupMeta(
                                        GetBackupMetaFile(new_backup_id),
                                        &backuped_file_infos_, backup_env_))));
  assert(ret.second == true);
  auto& new_backup = ret.first->second;
  new_backup->RecordTimestamp();
  new_backup->SetAppMetadata(app_metadata);

  auto start_backup = backup_env_-> NowMicros();

  ROCKS_LOG_INFO(options_.info_log,
                 "Started the backup process -- creating backup %u",
                 new_backup_id);

  auto private_tmp_dir = GetAbsolutePath(GetPrivateFileRel(new_backup_id, true));
  Status s = backup_env_->FileExists(private_tmp_dir);
  if (s.ok()) {
//可能最后一次备份失败并留下部分状态，清理它
    s = GarbageCollect();
  } else if (s.IsNotFound()) {
//正常情况下，新备份的专用目录还不存在
    s = Status::OK();
  }
  if (s.ok()) {
    s = backup_env_->CreateDir(private_tmp_dir);
  }

  RateLimiter* rate_limiter = options_.backup_rate_limiter.get();
  if (rate_limiter) {
    copy_file_buffer_size_ = rate_limiter->GetSingleBurstBytes();
  }

//我们将在其中插入为Live计算的DST_路径的集合
//文件和实时Wal文件。
//用于检查活动文件是否与其他文件共享DST路径
//现场文件。
  std::unordered_set<std::string> live_dst_paths;

  std::vector<BackupAfterCopyOrCreateWorkItem> backup_items_to_finish;
//为每个实时文件向频道添加副本或创建工作项
  db->DisableFileDeletions();
  if (s.ok()) {
    CheckpointImpl checkpoint(db);
    uint64_t sequence_number = 0;
    s = checkpoint.CreateCustomCheckpoint(
        db->GetDBOptions(),
        [&](const std::string& src_dirname, const std::string& fname,
            FileType) {
//当自定义检查点看到
//不支持从链接文件返回。
          return Status::NotSupported();
        /**链接文件\u cb*/，
        [&]（const std:：string&src_dirname，const std:：string&fname，
            uint64_t size_limit_bytes，filetype type）
          如果（type==klogfile&&！选项备份日志文件）
            返回状态：：OK（）；
          }
          日志（选项“info”log，“为备份%s添加文件”，fname.c_str（））；
          uint64_t size_bytes=0；
          状态ST；
          if（type==ktablefile）
            st=db_env_u->getfilesize（src_dirname+fname，&size_bytes）；
          }
          如果（圣奥克（））{
            st=addbackupfileworkitem（
                实时路径，备份项目到完成，新备份ID，
                选项.share_table_files&&type==ktablefile，src_dirname，
                fname，速率限制器，大小字节，大小限制字节，
                选项.share_files_with_checksum&&type==ktablefile，
                进度回电）；
          }
          返回ST；
        /*复制_文件_cb*/,

        [&](const std::string& fname, const std::string& contents, FileType) {
          Log(options_.info_log, "add file for backup %s", fname.c_str());
          return AddBackupFileWorkItem(
              live_dst_paths, backup_items_to_finish, new_backup_id,
              /*se/*shared*/，“”/*src_dir*/，fname，速率限制器，
              contents.size（），0/*大小限制*/, false /* shared_checksum */,

              progress_callback, contents);
        /**创建\u文件\u cb*/，
        序列号，在备份之前刷新？0：端口：：kmaxuint64）；
    如果（S.O.（））{
      新的_backup->setsequencenumber（序列号）；
    }
  }
  rocks_log_info（选项_u.info_log，“添加备份文件完成，等待完成”）；
  状态项\状态；
  用于（自动和项目：备份项目到完成）
    item.result.wait（）；
    自动结果=item.result.get（）；
    项目状态=结果状态；
    if（item_status.ok（）&&item.shared&&item.needed_to_copy）
      item_status=item.backup_env->renamefile（item.dst_path_tmp，
                                                项目dSTYPATH；
    }
    if（item_status.ok（））
      项目_status=new_backup.get（）->addfile（
              std：：使_shared<fileinfo>（item.dst_relative，
                                         结果：大小，
                                         result.checksum_value））；
    }
    如果（！）项目_status.ok（））
      S=项目状态；
    }
  }

  //我们复制了所有文件，启用文件删除
  db->EnableFileDeletions（错误）；

  如果（S.O.（））{
    //将tmp private backup移到real backup文件夹
    罗克斯洛格洛夫
        选项信息日志，
        “正在将tmp备份目录移动到实际目录：%s->%s\n”，
        getabsolutePath（getprivatefilerel（new_backup_id，true））.c_str（），
        getabsolutePath（getprivatefilerel（new_backup_id，false））.c_str（））；
    S=备份环境->重命名文件（
        getabsolutePath（getprivatefilerel（new_backup_id，true）），//tmp
        getabsolutePath（getprivatefilerel（new_backup_id，false））；
  }

  auto backup_time=backup_env_u->nowmicros（）-启动备份；

  如果（S.O.（））{
    //在磁盘上保留备份元数据
    s=新的_backup->storetofile（选项_u.sync）；
  }
  if（s.ok（）&&options_.sync）
    unique_ptr<directory>backup_private_directory；
    备份目录->newdirectory（
        getabsolutePath（getprivatefilerel（new_backup_id，false）），
        &backup_private_目录）；
    如果（备份私有目录！= null pTr）{
      backup_private_directory->fsync（）；
    }
    如果（私有目录）= null pTr）{
      private_directory_u->fsync（）；
    }
    如果（meta_目录）= null pTr）{
      meta_directory_u->fsync（）；
    }
    如果（共享目录）= null pTr）{
      共享目录
    }
    如果（备份目录）= null pTr）{
      备份目录
    }
  }

  如果（S.O.（））{
    backup_statistics_u.incrementNumberSuccessbackup（）；
  }
  如果（！）S.O.（））{
    backup_statistics_u.incrementNumberFailbackup（）；
    //清除我们可能创建的所有文件
    rocks_log_info（选项_u.info_log，“备份失败--%s”，
                   s.toString（）.c_str（））；
    rocks_log_info（选项_u.info_log，“备份统计信息%s\n”，
                   backup_statistics_.toString（）.c_str（））；
    //删除可能已经写入的文件
    删除备份（新的备份ID）；
    garbageCollect（）；
    返回S；
  }

  //这里我们知道我们成功安装了新的备份
  //在最新的备份文件中
  最新的备份
  rocks“日志”信息（选项“信息”日志，“备份完成”。“一切都好”；

  //备份速度以字节/秒为单位
  双备份\u speed=new_backup->getsize（）/（1.048576*备份时间）；
  rocks_log_info（options_u.info_log，“备份文件数：%u”，
                 new_backup->getNumberFiles（））；
  人字形字符[16]；
  appendHumanBytes（new_backup->getSize（），human_size，sizeof（human_size））；
  rocks_log_info（选项_u.info_log，“备份大小：%s”，人员大小）；
  rocks_log_info（选项_uu info_log，“备份时间：”%“priu64”微秒“，
                 备份时间；
  rocks_log_info（选项_u.info_log，“备份速度：%.3f mb/s”，备份速度）；
  rocks_log_info（选项_u.info_log，“备份统计信息%s”，
                 backup_statistics_.toString（）.c_str（））；
  返回S；
}

状态备份引擎impl:：purgeoldbackups（uint32_t num_backups_to_keep）
  断言（已初始化）；
  断言（！）读数）；
  rocks_log_info（选项_u.info_log，“清除旧备份，保留%u”，
                 num_backup_to_keep）；
  std:：vector<backupid>to_delete；
  auto itr=备份
  while（（backups.size（）-to_delete.size（））>num_backups_to_keep）
    要删除，请向后推（itr->first）；
    ITR++；
  }
  for（自动备份：to_delete）
    auto s=删除备份（backup_id）；
    如果（！）S.O.（））{
      返回S；
    }
  }
  返回状态：：OK（）；
}

状态backupengineimpl：：删除备份（backup id backup_id）
  断言（已初始化）；
  断言（！）读数）；
  rocks_log_info（选项_u.info_log，“删除备份%u”，备份ID）；
  自动备份=备份查找（备份ID）；
  如果（备份）！=备份.end（））
    auto s=backup->second->delete（）；
    如果（！）S.O.（））{
      返回S；
    }
    备份擦除（备份）；
  }否则{
    auto corrupt=损坏的备份查找（backup_id）；
    if（corrupt==corrupt_backups_.end（））
      返回状态：：未找到（“未找到备份”）；
    }
    auto s=corrupt->second.second->delete（）；
    如果（！）S.O.（））{
      返回S；
    }
    损坏的备份擦除（损坏）；
  }

  std:：vector<std:：string>to_delete；
  用于（auto&itr:backuped_file_infos_
    if（itr.second->refs==0）
      状态S=backup_env_u->deletefile（getabsolutepath（itr.first））；
      rocks_log_info（options_u info_log，“删除%s--%s”，itr.first.c_str（），
                     s.toString（）.c_str（））；
      要删除，请向后推（itr.first）；
    }
  }
  for（auto&td:to_delete）
    备份的文件信息擦除（td）；
  }

  //处理私有目录--garbageCollect（）将处理它们
  //如果不是空的
  std：：string private_dir=getprivatefilerel（backup_id）；
  状态S=backup_env_u->deletedir（getabsolutepath（private_dir））；
  rocks_log_info（选项_u info_log，“删除专用目录%s--%s”，
                 private_dir.c_str（），s.toString（）.c_str（））；
  返回状态：：OK（）；
}

void backupengineimpl:：getbackupinfo（std:：vector<backup info>*backup_info）
  断言（已初始化）；
  backup_info->reserve（backups_.size（））；
  用于（自动和备份：备份）
    如果（！）backup.second->empty（））
      backup_info->push_back（备份信息（
          backup.first，backup.second->getTimeStamp（），backup.second->getSize（），
          backup.second->getNumberFiles（），backup.second->getAppMetadata（））；
    }
  }
}

无效
backupengineimpl:：getcorruptedbackups（
    std:：vector<backupid>*损坏的_backupid）
  断言（已初始化）；
  损坏的_backup_id->reserve（损坏的_backup_.size（））；
  用于（自动备份和备份：损坏的备份）
    损坏的_backup_id->push_back（backup.first）；
  }
}

状态备份引擎impl:：restoredbfrombackup（
    backup id backup_id，const std:：string&db_dir，const std:：string&wal_dir，
    const restore options&restore_
  断言（已初始化）；
  auto corrupt_itr=corrupt_backups_u.find（backup_id）；
  如果（腐败）=损坏的_备份_.end（））
    返回corrupt-itr->second.first；
  }
  auto backup_itr=备份查找（backup_id）；
  if（backup_itr==backups_.end（））
    返回状态：：未找到（“未找到备份”）；
  }
  auto&backup=backup-itr->second；
  if（backup->empty（））
    返回状态：：未找到（“未找到备份”）；
  }

  rocks_log_info（选项_u.info_log，“还原备份ID%u\n”，备份ID）；
  rocks_log_info（选项_u.info_log，“保留日志文件：%d\n”，
                 static_cast<int>（restore_options.keep_log_files））；

  //以防万一。忽略错误
  db_env_u->createdirifmissing（db_dir）；
  数据库env->createdirifmissing（wal_dir）；

  if（还原_选项。保留_日志_文件）
    //删除db_dir中的文件，但保留所有日志文件
    删除子项（db_dir，1<<klogfile）；
    //将所有文件从archive dir移到wal_dir
    std:：string archive_dir=archivaldirectory（wal_dir）；
    std:：vector<std:：string>存档文件；
    db_env_->getchildren（archive_dir，&archive_files）；//忽略错误
    for（const auto&f:存档文件）
      uint64_t编号；
      文件类型；
      bool ok=parseFileName（f，&number，&type）；
      if（确定&&type==klogfile）
        岩石日志信息（选项日志，
                       “将日志文件从存档/移动到wal目录：%s”，
                       F.CyS-（））；
        状态S＝
            数据库环境文件（archive_dir+“/”+f，wal_dir+“/”+f）；
        如果（！）S.O.（））{
          //如果无法将日志文件从archive_dir移到wal_dir，
          //我们应该失败，因为这可能意味着数据丢失
          返回S；
        }
      }
    }
  }否则{
    删除子项（wal_dir）；
    删除子项（archivaldirectory（wal_dir））；
    删除子项（db_dir）；
  }

  rate limiter*rate_limiter=选项_u.restore_rate_limiter.get（）；
  如果（速率限制器）
    copy_file_buffer_size_u=rate_limiter->getsingleburstbytes（）；
  }
  状态S；
  std:：vector<restoreaftercopy或reateworksitem>将项目还原到完成；
  for（const auto&file_info:backup->getfiles（））
    const std:：string&file=file_info->filename；
    std：：字符串dst；
    / / 1。提取文件名
    size_t slash=file.find_last_of（'/'）；
    //文件将被共享/<file>，共享的_checksum/<file_crc32 _size>
    //或private/<number>/<file>
    断言（斜线！=std：：字符串：：npos）；
    dst=file.substr（斜线+1）；

    //如果文件在共享校验和中，则提取真正的文件名
    //在这种情况下，文件是<number>校验和>大小><type>
    if（file.substr（0，斜线）==getsharedchecksumdirrel（））
      dst=getfilefromchecksumfile（dst）；
    }

    / / 2。查找文件类型
    uint64_t编号；
    文件类型；
    bool ok=parseFileName（dst，&number，&type）；
    如果（！）好的）{
      返回状态：：损坏（“备份损坏”）；
    }
    / / 3。构建最终路径
    //klogfile在wal_dir中，其余的都在db_dir中
    dst=（type==klogfile）？wal_dir:db_dir）+
      “/”+DST；

    rocks_log_info（选项_u.info_log，“将%s还原为%s\n”，file.c_str（），
                   DST.
    复制或创建工作项复制或创建工作项（
        GetAbsolutePath（文件），DST，“”/*内容*/, backup_env_, db_env_,

        /*SE，速率限制器，0/*大小限制*/）；
    在“复制”或“创建”工作项（
        复制或创建工作项。result.get\u future（），
        文件_info->checksum_value）；
    文件“复制”或“创建”写入（std:：move（copy_或“创建”工作项））；
    将项目恢复到完成状态。向后推（
        std：：移动（在“复制”或“创建”工作项之后）；
  }
  状态项\状态；
  for（auto&item:将_items_还原为_finish）
    item.result.wait（）；
    自动结果=item.result.get（）；
    项目状态=结果状态；
    //注意：以下两种不良状态都有可能发生
    //复制时。但是，我们只返回一个状态。
    如果（！）项目_status.ok（））
      S=项目状态；
      断裂；
    否则，如果（item.checksum_value！=结果.校验和值）
      S=状态：：损坏（“校验和检查失败”）；
      断裂；
    }
  }

  rocks_log_info（选项_u.info_log，“还原完成--%s\n”，
                 s.toString（）.c_str（））；
  返回S；
}

状态backupengineimpl:：verifybackup（backup id backup_id）
  断言（已初始化）；
  auto corrupt_itr=corrupt_backups_u.find（backup_id）；
  如果（腐败）=损坏的_备份_.end（））
    返回corrupt-itr->second.first；
  }

  auto backup_itr=备份查找（backup_id）；
  if（backup_itr==backups_.end（））
    返回状态：：NotFound（）；
  }

  auto&backup=backup-itr->second；
  if（backup->empty（））
    返回状态：：NotFound（）；
  }

  rocks_log_info（选项_u.info_log，“验证备份ID%u\n”，备份ID）；

  std:：unordered_map<std:：string，uint64_t>curr_abs_path_to_size；
  对于（const auto&rel_dir:getprivatefilerel（backup_id），getsharedfilerel（），
                              getsharedfilewithchecksumrel（））
    const auto abs_dir=getabsolutepath（rel_dir）；
    insertpathnametosizebytes（abs_dir，backup_env_u，&curr_abs_path_to_size）；
  }

  for（const auto&file_info:backup->getfiles（））
    const auto abs_path=getabsolutepath（file_info->filename）；
    if（curr_abs_path_to_size.find（abs_path）==curr_abs_path_to_size.end（））
      返回状态：：NotFound（“文件丢失：”+abs_path）；
    }
    如果（文件信息->大小！=curr_abs_path_to_size[abs_path]）；
      返回状态：：损坏（“文件损坏：”+abs_path）；
    }
  }
  返回状态：：OK（）；
}

状态备份引擎模板：：复制或创建文件（
    const std:：string&src，const std:：string&dst，const std:：string&contents，
    env*src_env，env*dst_env，bool sync，rate limiter*速率限制器，
    uint64_t*大小，uint32_t*校验和值，uint64_t大小_限制，
    std:：function<void（）>progress_callback）
  断言（src.empty（）！=contents.empty（））；
  状态S；
  唯一的_ptr<writablefile>dst_file；
  唯一的_ptr<sequentialfile>src_file；
  环境选项环境选项；
  env？options.use？mmap？writes=false；
  //TODO:（gzh）如果可能，可以在此处使用直接读/写
  如果（大小）！= null pTr）{
    *尺寸＝0；
  }
  如果（校验和值！= null pTr）{
    *校验和值=0；
  }

  //检查是否设置了大小限制。如果不是，设置为非常大的数字
  如果（尺寸限制==0）
    大小_limit=std:：numeric_limits<uint64_t>：：max（）；
  }

  s=dst_env->newwritablefile（dst，&dst_file，env_选项）；
  如果（s）（和）& &！SRC.EMPTY（））{
    s=src_env->newsequentialfile（src，&src_file，env_options）；
  }
  如果（！）S.O.（））{
    返回S；
  }

  unique_ptr<writablefilewriter>dest_writer（
      新的可写文件编写器（std:：move（dst_file），env_options））；
  唯一读卡器<SequentialFileReader>SRC读卡器；
  唯一指针<char[]>buf；
  如果（！）SRC.EMPTY（））{
    src_reader.reset（new sequentialfilereader（std:：move（src_file））；
    buf.reset（新字符[复制缓冲区大小]）
  }

  切片数据；
  uint64_t处理过的_buffer_size=0；
  做{
    if（停止备份加载（std:：memory_order_acquire））
      返回状态：：未完成（“备份已停止”）；
    }
    如果（！）SRC.EMPTY（））{
      大小缓冲区到读取的大小=（复制文件缓冲区大小限制）
                                  ？复制文件缓冲区大小
                                  尺寸限制；
      s=src_reader->read（buffer_to_read，&data，buf.get（））；
      已处理的缓冲区大小+=缓冲区读取；
    }否则{
      数据=内容；
    }
    大小_limit-=data.size（）；

    如果（！）S.O.（））{
      返回S；
    }

    如果（大小）！= null pTr）{
      *size+=data.size（）；
    }
    如果（校验和值！= null pTr）{
      *校验和值=
          crc32c:：extend（*checksum_value，data.data（），data.size（））；
    }
    s=dest_writer->append（数据）；
    如果（速率限制器！= null pTr）{
      速率限制器->请求（data.size（），env:：io_low，nullptr/*stats*/,

                            RateLimiter::OpType::kWrite);
    }
    if (processed_buffer_size > options_.callback_trigger_interval_size) {
      processed_buffer_size -= options_.callback_trigger_interval_size;
      std::lock_guard<std::mutex> lock(byte_report_mutex_);
      progress_callback();
    }
  } while (s.ok() && contents.empty() && data.size() > 0 && size_limit > 0);

  if (s.ok() && sync) {
    s = dest_writer->Sync(false);
  }
  if (s.ok()) {
    s = dest_writer->Close();
  }
  return s;
}

//fname将始终以“/”开头
Status BackupEngineImpl::AddBackupFileWorkItem(
    std::unordered_set<std::string>& live_dst_paths,
    std::vector<BackupAfterCopyOrCreateWorkItem>& backup_items_to_finish,
    BackupID backup_id, bool shared, const std::string& src_dir,
    const std::string& fname, RateLimiter* rate_limiter, uint64_t size_bytes,
    uint64_t size_limit, bool shared_checksum,
    std::function<void()> progress_callback, const std::string& contents) {
  assert(!fname.empty() && fname[0] == '/');
  assert(contents.empty() != src_dir.empty());

  std::string dst_relative = fname.substr(1);
  std::string dst_relative_tmp;
  Status s;
  uint32_t checksum_value = 0;

  if (shared && shared_checksum) {
//将校验和和和文件长度添加到文件名中
    s = CalculateChecksum(src_dir + fname, db_env_, size_limit,
                          &checksum_value);
    if (!s.ok()) {
      return s;
    }
    if (size_bytes == port::kMaxUint64) {
      return Status::NotFound("File missing: " + src_dir + fname);
    }
    dst_relative =
        GetSharedFileWithChecksum(dst_relative, checksum_value, size_bytes);
    dst_relative_tmp = GetSharedFileWithChecksumRel(dst_relative, true);
    dst_relative = GetSharedFileWithChecksumRel(dst_relative, false);
  } else if (shared) {
    dst_relative_tmp = GetSharedFileRel(dst_relative, true);
    dst_relative = GetSharedFileRel(dst_relative, false);
  } else {
    dst_relative_tmp = GetPrivateFileRel(backup_id, true, dst_relative);
    dst_relative = GetPrivateFileRel(backup_id, false, dst_relative);
  }
  std::string dst_path = GetAbsolutePath(dst_relative);
  std::string dst_path_tmp = GetAbsolutePath(dst_relative_tmp);

//如果它是共享的，我们还需要检查它是否存在——如果存在，就不需要
//再复制一次。
  bool need_to_copy = true;
//如果dst_路径与另一个活动文件的路径相同，则为true
  const bool same_path =
      live_dst_paths.find(dst_path) != live_dst_paths.end();

  bool file_exists = false;
  if (shared && !same_path) {
    Status exist = backup_env_->FileExists(dst_path);
    if (exist.ok()) {
      file_exists = true;
    } else if (exist.IsNotFound()) {
      file_exists = false;
    } else {
      assert(s.IsIOError());
      return exist;
    }
  }

  if (!contents.empty()) {
    need_to_copy = false;
  } else if (shared && (same_path || file_exists)) {
    need_to_copy = false;
    if (shared_checksum) {
      ROCKS_LOG_INFO(options_.info_log,
                     "%s already present, with checksum %u and size %" PRIu64,
                     fname.c_str(), checksum_value, size_bytes);
    } else if (backuped_file_infos_.find(dst_relative) ==
               backuped_file_infos_.end() && !same_path) {
//文件已经存在，但没有被任何备份引用。重写
//文件
      ROCKS_LOG_INFO(
          options_.info_log,
          "%s already present, but not referenced by any backup. We will "
          "overwrite the file.",
          fname.c_str());
      need_to_copy = true;
      backup_env_->DeleteFile(dst_path);
    } else {
//文件存在并由备份引用
      ROCKS_LOG_INFO(options_.info_log,
                     "%s already present, calculate checksum", fname.c_str());
      s = CalculateChecksum(src_dir + fname, db_env_, size_limit,
                            &checksum_value);
    }
  }
  live_dst_paths.insert(dst_path);

  if (!contents.empty() || need_to_copy) {
    ROCKS_LOG_INFO(options_.info_log, "Copying %s to %s", fname.c_str(),
                   dst_path_tmp.c_str());
    CopyOrCreateWorkItem copy_or_create_work_item(
        src_dir.empty() ? "" : src_dir + fname, dst_path_tmp, contents, db_env_,
        backup_env_, options_.sync, rate_limiter, size_limit,
        progress_callback);
    BackupAfterCopyOrCreateWorkItem after_copy_or_create_work_item(
        copy_or_create_work_item.result.get_future(), shared, need_to_copy,
        backup_env_, dst_path_tmp, dst_path, dst_relative);
    files_to_copy_or_create_.write(std::move(copy_or_create_work_item));
    backup_items_to_finish.push_back(std::move(after_copy_or_create_work_item));
  } else {
    std::promise<CopyOrCreateResult> promise_result;
    BackupAfterCopyOrCreateWorkItem after_copy_or_create_work_item(
        promise_result.get_future(), shared, need_to_copy, backup_env_,
        dst_path_tmp, dst_path, dst_relative);
    backup_items_to_finish.push_back(std::move(after_copy_or_create_work_item));
    CopyOrCreateResult result;
    result.status = s;
    result.size = size_bytes;
    result.checksum_value = checksum_value;
    promise_result.set_value(std::move(result));
  }
  return s;
}

Status BackupEngineImpl::CalculateChecksum(const std::string& src, Env* src_env,
                                           uint64_t size_limit,
                                           uint32_t* checksum_value) {
  *checksum_value = 0;
  if (size_limit == 0) {
    size_limit = std::numeric_limits<uint64_t>::max();
  }

  EnvOptions env_options;
  env_options.use_mmap_writes = false;
  env_options.use_direct_reads = false;

  std::unique_ptr<SequentialFile> src_file;
  Status s = src_env->NewSequentialFile(src, &src_file, env_options);
  if (!s.ok()) {
    return s;
  }

  unique_ptr<SequentialFileReader> src_reader(
      new SequentialFileReader(std::move(src_file)));
  std::unique_ptr<char[]> buf(new char[copy_file_buffer_size_]);
  Slice data;

  do {
    if (stop_backup_.load(std::memory_order_acquire)) {
      return Status::Incomplete("Backup stopped");
    }
    size_t buffer_to_read = (copy_file_buffer_size_ < size_limit) ?
      copy_file_buffer_size_ : size_limit;
    s = src_reader->Read(buffer_to_read, &data, buf.get());

    if (!s.ok()) {
      return s;
    }

    size_limit -= data.size();
    *checksum_value = crc32c::Extend(*checksum_value, data.data(), data.size());
  } while (data.size() > 0 && size_limit > 0);

  return s;
}

void BackupEngineImpl::DeleteChildren(const std::string& dir,
                                      uint32_t file_type_filter) {
  std::vector<std::string> children;
db_env_->GetChildren(dir, &children);  //忽略错误

  for (const auto& f : children) {
    uint64_t number;
    FileType type;
    bool ok = ParseFileName(f, &number, &type);
    if (ok && (file_type_filter & (1 << type))) {
//不删除此文件
      continue;
    }
db_env_->DeleteFile(dir + "/" + f);  //忽略错误
  }
}

Status BackupEngineImpl::InsertPathnameToSizeBytes(
    const std::string& dir, Env* env,
    std::unordered_map<std::string, uint64_t>* result) {
  assert(result != nullptr);
  std::vector<Env::FileAttributes> files_attrs;
  Status status = env->FileExists(dir);
  if (status.ok()) {
    status = env->GetChildrenFileAttributes(dir, &files_attrs);
  } else if (status.IsNotFound()) {
//插入任何条目都不能视为成功
    status = Status::OK();
  }
  const bool slash_needed = dir.empty() || dir.back() != '/';
  for (const auto& file_attrs : files_attrs) {
    result->emplace(dir + (slash_needed ? "/" : "") + file_attrs.name,
                    file_attrs.size_bytes);
  }
  return status;
}

Status BackupEngineImpl::GarbageCollect() {
  assert(!read_only_);
  ROCKS_LOG_INFO(options_.info_log, "Starting garbage collection");

  if (options_.share_table_files) {
//删除过时的共享文件
    std::vector<std::string> shared_children;
    {
      std::string shared_path;
      if (options_.share_files_with_checksum) {
        shared_path = GetAbsolutePath(GetSharedFileWithChecksumRel());
      } else {
        shared_path = GetAbsolutePath(GetSharedFileRel());
      }
      auto s = backup_env_->FileExists(shared_path);
      if (s.ok()) {
        s = backup_env_->GetChildren(shared_path, &shared_children);
      } else if (s.IsNotFound()) {
        s = Status::OK();
      }
      if (!s.ok()) {
        return s;
      }
    }
    for (auto& child : shared_children) {
      std::string rel_fname;
      if (options_.share_files_with_checksum) {
        rel_fname = GetSharedFileWithChecksumRel(child);
      } else {
        rel_fname = GetSharedFileRel(child);
      }
      auto child_itr = backuped_file_infos_.find(rel_fname);
//如果不重新计数，请删除它
      if (child_itr == backuped_file_infos_.end() ||
          child_itr->second->refs == 0) {
//这可能是一个目录，但deletefile将在该目录中失败
//案例，所以我们很好
        Status s = backup_env_->DeleteFile(GetAbsolutePath(rel_fname));
        ROCKS_LOG_INFO(options_.info_log, "Deleting %s -- %s",
                       rel_fname.c_str(), s.ToString().c_str());
        backuped_file_infos_.erase(rel_fname);
      }
    }
  }

//删除过时的私人文件
  std::vector<std::string> private_children;
  {
    auto s = backup_env_->GetChildren(GetAbsolutePath(GetPrivateDirRel()),
                                      &private_children);
    if (!s.ok()) {
      return s;
    }
  }
  for (auto& child : private_children) {
    BackupID backup_id = 0;
    bool tmp_dir = child.find(".tmp") != std::string::npos;
    sscanf(child.c_str(), "%u", &backup_id);
if (!tmp_dir &&  //如果是tmp_dir，删除它
        (backup_id == 0 || backups_.find(backup_id) != backups_.end())) {
//要么不是数字，要么还活着。持续
      continue;
    }
//这里我们必须删除dir及其所有子目录
    std::string full_private_path =
        GetAbsolutePath(GetPrivateFileRel(backup_id, tmp_dir));
    std::vector<std::string> subchildren;
    backup_env_->GetChildren(full_private_path, &subchildren);
    for (auto& subchild : subchildren) {
      Status s = backup_env_->DeleteFile(full_private_path + subchild);
      ROCKS_LOG_INFO(options_.info_log, "Deleting %s -- %s",
                     (full_private_path + subchild).c_str(),
                     s.ToString().c_str());
    }
//最后删除private dir
    Status s = backup_env_->DeleteDir(full_private_path);
    ROCKS_LOG_INFO(options_.info_log, "Deleting dir %s -- %s",
                   full_private_path.c_str(), s.ToString().c_str());
  }

  return Status::OK();
}

//-------备份元类--------

Status BackupEngineImpl::BackupMeta::AddFile(
    std::shared_ptr<FileInfo> file_info) {
  auto itr = file_infos_->find(file_info->filename);
  if (itr == file_infos_->end()) {
    auto ret = file_infos_->insert({file_info->filename, file_info});
    if (ret.second) {
      itr = ret.first;
      itr->second->refs = 1;
    } else {
//如果发生这种情况，就会有严重的问题
      return Status::Corruption("In memory metadata insertion error");
    }
  } else {
    if (itr->second->checksum_value != file_info->checksum_value) {
      return Status::Corruption(
          "Checksum mismatch for existing backup file. Delete old backups and "
          "try again.");
    }
++itr->second->refs;  //如果已经存在，则增加refcount
  }

  size_ += file_info->size;
  files_.push_back(itr->second);

  return Status::OK();
}

Status BackupEngineImpl::BackupMeta::Delete(bool delete_meta) {
  Status s;
  for (const auto& file : files_) {
--file->refs;  //减少引用计数
  }
  files_.clear();
//删除元文件
  if (delete_meta) {
    s = env_->FileExists(meta_filename_);
    if (s.ok()) {
      s = env_->DeleteFile(meta_filename_);
    } else if (s.IsNotFound()) {
s = Status::OK();  //没有要删除的内容
    }
  }
  timestamp_ = 0;
  return s;
}

Slice kMetaDataPrefix("metadata ");

//每个备份元文件的格式为：
//<时间戳>
//< SEQ号>
//<metadata（literal string）><metadata>（可选）
//<文件数>
//<file1><crc32（literal string）><crc32_value>
//<file2><crc32（literal string）><crc32_value>
//…
Status BackupEngineImpl::BackupMeta::LoadFromFile(
    const std::string& backup_dir,
    const std::unordered_map<std::string, uint64_t>& abs_path_to_size) {
  assert(Empty());
  Status s;
  unique_ptr<SequentialFile> backup_meta_file;
  s = env_->NewSequentialFile(meta_filename_, &backup_meta_file, EnvOptions());
  if (!s.ok()) {
    return s;
  }

  unique_ptr<SequentialFileReader> backup_meta_reader(
      new SequentialFileReader(std::move(backup_meta_file)));
  unique_ptr<char[]> buf(new char[max_backup_meta_file_size_ + 1]);
  Slice data;
  s = backup_meta_reader->Read(max_backup_meta_file_size_, &data, buf.get());

  if (!s.ok() || data.size() == max_backup_meta_file_size_) {
    return s.ok() ? Status::Corruption("File size too big") : s;
  }
  buf[data.size()] = 0;

  uint32_t num_files = 0;
  char *next;
  timestamp_ = strtoull(data.data(), &next, 10);
data.remove_prefix(next - data.data() + 1); //+n为1
  sequence_number_ = strtoull(data.data(), &next, 10);
data.remove_prefix(next - data.data() + 1); //+n为1

  if (data.starts_with(kMetaDataPrefix)) {
//存在应用程序元数据
    data.remove_prefix(kMetaDataPrefix.size());
    Slice hex_encoded_metadata = GetSliceUntil(&data, '\n');
    bool decode_success = hex_encoded_metadata.DecodeHex(&app_metadata_);
    if (!decode_success) {
      return Status::Corruption(
          "Failed to decode stored hex encoded app metadata");
    }
  }

  num_files = static_cast<uint32_t>(strtoul(data.data(), &next, 10));
data.remove_prefix(next - data.data() + 1); //+n为1

  std::vector<std::shared_ptr<FileInfo>> files;

  Slice checksum_prefix("crc32 ");

  for (uint32_t i = 0; s.ok() && i < num_files; ++i) {
    auto line = GetSliceUntil(&data, '\n');
    std::string filename = GetSliceUntil(&line, ' ').ToString();

    uint64_t size;
    const std::shared_ptr<FileInfo> file_info = GetFile(filename);
    if (file_info) {
      size = file_info->size;
    } else {
      std::string abs_path = backup_dir + "/" + filename;
      try {
        size = abs_path_to_size.at(abs_path);
      } catch (std::out_of_range&) {
        return Status::Corruption("Size missing for pathname: " + abs_path);
      }
    }

    if (line.empty()) {
      return Status::Corruption("File checksum is missing for " + filename +
                                " in " + meta_filename_);
    }

    uint32_t checksum_value = 0;
    if (line.starts_with(checksum_prefix)) {
      line.remove_prefix(checksum_prefix.size());
      checksum_value = static_cast<uint32_t>(
          strtoul(line.data(), nullptr, 10));
      if (line != rocksdb::ToString(checksum_value)) {
        return Status::Corruption("Invalid checksum value for " + filename +
                                  " in " + meta_filename_);
      }
    } else {
      return Status::Corruption("Unknown checksum type for " + filename +
                                " in " + meta_filename_);
    }

    files.emplace_back(new FileInfo(filename, size, checksum_value));
  }

  if (s.ok() && data.size() > 0) {
//必须完全读取文件。如果不是，我们将其视为腐败。
    s = Status::Corruption("Tailing data in backup meta file in " +
                           meta_filename_);
  }

  if (s.ok()) {
    files_.reserve(files.size());
    for (const auto& file_info : files) {
      s = AddFile(file_info);
      if (!s.ok()) {
        break;
      }
    }
  }

  return s;
}

Status BackupEngineImpl::BackupMeta::StoreToFile(bool sync) {
  Status s;
  unique_ptr<WritableFile> backup_meta_file;
  EnvOptions env_options;
  env_options.use_mmap_writes = false;
  env_options.use_direct_writes = false;
  s = env_->NewWritableFile(meta_filename_ + ".tmp", &backup_meta_file,
                            env_options);
  if (!s.ok()) {
    return s;
  }

  unique_ptr<char[]> buf(new char[max_backup_meta_file_size_]);
  size_t len = 0, buf_size = max_backup_meta_file_size_;
  len += snprintf(buf.get(), buf_size, "%" PRId64 "\n", timestamp_);
  len += snprintf(buf.get() + len, buf_size - len, "%" PRIu64 "\n",
                  sequence_number_);
  if (!app_metadata_.empty()) {
    std::string hex_encoded_metadata =
        /*ce（app_metadata_u）.toString（/*hex*/true）；
    if（hex_encoded_metadata.size（）+kmetadataprefix.size（）+1>
        buf_尺寸-长度）
      返回状态：：损坏（“缓冲区太小，无法容纳备份元数据”）；
    }
    memcpy（buf.get（）+len，kmetadaprefix.data（），kmetadaprefix.size（））；
    len+=kmetataPrefix.size（）；
    memcpy（buf.get（）+len，十六进制编码的元数据.data（），
           十六进制编码的元数据.size（））；
    len+=hex_encoded_metadata.size（）；
    buf[len++]='\n'；
  }
  len+=snprintf（buf.get（）+len，buf_-size-len，“%”rocksdb_-priszt“\n”，
                  文件大小（））；
  用于（const auto&file:files_
    //现在使用crc32，如果需要，请切换到其他类型
    len+=snprintf（buf.get（）+len，buf_size-len，“%s crc32%u\n”，
                    file->filename.c_str（），file->checksum_value）；
  }

  s=backup_meta_file->append（slice（buf.get（），len））；
  if（s.ok（）&&sync）
    s=backup_meta_file->sync（）；
  }
  如果（S.O.（））{
    s=backup_meta_file->close（）；
  }
  如果（S.O.（））{
    s=env_u->renamefile（meta_filename_+“.tmp”，meta_filename_uu）；
  }
  返回S；
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
类backupengineradonlyimpl:公共backupengineradonly
 公众：
  backupengineradonlyimpl（env*db_env，const backupabledboptions&options）
      ：备份引擎（新备份引擎模板（db_env，options，true））

  虚拟~backupEngineeradOnlyImpl（）

  //返回的backupinfos按时间顺序排列，这意味着
  //最新备份排在最后。
  virtual void getbackupinfo（std:：vector<backup info>>*backup_info）override_
    backup_engine_u->getbackupinfo（backup_info）；
  }

  虚拟void getcorruptedbackup（
      std:：vector<backupid>*损坏的_backup_id）override_
    backup_engine_u->getcorruptedbackup（损坏的_backup_id）；
  }

  从备份中恢复的虚拟状态（
      backup id backup_id，const std:：string&db_dir，const std:：string&wal_dir，
      const restoreOptions&restore_options=restoreOptions（））override_
    返回backup_engine_u->restoredbfrombackup（backup_id，db_dir，wal_dir，
                                               恢复选项）；
  }

  从最新备份中恢复的虚拟状态（
      const std:：string&db_dir，const std:：string&wal_dir，
      const restoreOptions&restore_options=restoreOptions（））override_
    返回backup_engine_u->restoredbfromlatestbackup（db_dir，wal_dir，
                                                     恢复选项）；
  }

  虚拟状态验证备份（backup id backup_id）覆盖
    返回backup_engine_uu->verifybackup（backup_id）；
  }

  status initialize（）返回backup_engine_u->initialize（）；

 私人：
  std:：unique_ptr<backupengineimpl>backup_engine_u；
}；

状态备份引擎只读：：打开（env*env，const backupabledboptions&options，
                                  备份引擎只读**备份引擎指针）
  如果（选项，销毁旧数据）
    返回状态：：invalidArgument（
        “无法用只读备份引擎销毁旧数据”）；
  }
  std:：unique_ptr<backupengineradonlyimpl>backup_engine（
      新建backupengineradonlyimpl（env，options））；
  auto s=backup_engine->initialize（）；
  如果（！）S.O.（））{
    *backup_engine_ptr=nullptr；
    返回S；
  }
  *backup_engine_ptr=备份_engine.release（）；
  返回状态：：OK（）；
}

//命名空间rocksdb

endif//rocksdb_lite
