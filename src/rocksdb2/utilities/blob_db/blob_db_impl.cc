
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
#ifndef ROCKSDB_LITE

#include "utilities/blob_db/blob_db_impl.h"
#include <algorithm>
#include <cinttypes>
#include <iomanip>
#include <limits>
#include <memory>

#include "db/db_impl.h"
#include "db/write_batch_internal.h"
#include "monitoring/instrumented_mutex.h"
#include "rocksdb/convenience.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/utilities/transaction.h"
#include "table/block.h"
#include "table/block_based_table_builder.h"
#include "table/block_builder.h"
#include "table/meta_blocks.h"
#include "util/cast_util.h"
#include "util/crc32c.h"
#include "util/file_reader_writer.h"
#include "util/filename.h"
#include "util/logging.h"
#include "util/mutexlock.h"
#include "util/random.h"
#include "util/sync_point.h"
#include "util/timer_queue.h"
#include "utilities/blob_db/blob_db_iterator.h"
#include "utilities/blob_db/blob_index.h"

namespace {
int kBlockBasedTableVersionFormat = 2;
}  //结束命名空间

namespace rocksdb {
namespace blob_db {

Random blob_rgen(static_cast<uint32_t>(time(nullptr)));

void BlobDBFlushBeginListener::OnFlushBegin(DB* db, const FlushJobInfo& info) {
  if (impl_) impl_->OnFlushBeginHandler(db, info);
}

WalFilter::WalProcessingOption BlobReconcileWalFilter::LogRecordFound(
    unsigned long long log_number, const std::string& log_file_name,
    const WriteBatch& batch, WriteBatch* new_batch, bool* batch_changed) {
  return WalFilter::WalProcessingOption::kContinueProcessing;
}

bool blobf_compare_ttl::operator()(const std::shared_ptr<BlobFile>& lhs,
                                   const std::shared_ptr<BlobFile>& rhs) const {
  if (lhs->expiration_range_.first < rhs->expiration_range_.first) {
    return true;
  }
  if (lhs->expiration_range_.first > rhs->expiration_range_.first) {
    return false;
  }
  return lhs->BlobFileNumber() < rhs->BlobFileNumber();
}

void EvictAllVersionsCompactionListener::InternalListener::OnCompaction(
    int level, const Slice& key,
    CompactionEventListener::CompactionListenerValueType value_type,
    const Slice& existing_value, const SequenceNumber& sn, bool is_new) {
  assert(impl_->bdb_options_.enable_garbage_collection);
  if (!is_new &&
      value_type ==
          CompactionEventListener::CompactionListenerValueType::kValue) {
    BlobIndex blob_index;
    Status s = blob_index.DecodeFrom(existing_value);
    if (s.ok()) {
      if (impl_->debug_level_ >= 3)
        ROCKS_LOG_INFO(
            impl_->db_options_.info_log,
            "CALLBACK COMPACTED OUT KEY: %s SN: %d "
            "NEW: %d FN: %" PRIu64 " OFFSET: %" PRIu64 " SIZE: %" PRIu64,
            key.ToString().c_str(), sn, is_new, blob_index.file_number(),
            blob_index.offset(), blob_index.size());

      impl_->override_vals_q_.enqueue({blob_index.file_number(), key.size(),
                                       blob_index.offset(), blob_index.size(),
                                       sn});
    }
  } else {
    if (impl_->debug_level_ >= 3)
      ROCKS_LOG_INFO(impl_->db_options_.info_log,
                     "CALLBACK NEW KEY: %s SN: %d NEW: %d",
                     key.ToString().c_str(), sn, is_new);
  }
}

BlobDBImpl::BlobDBImpl(const std::string& dbname,
                       const BlobDBOptions& blob_db_options,
                       const DBOptions& db_options)
    : BlobDB(nullptr),
      db_impl_(nullptr),
      env_(db_options.env),
      ttl_extractor_(blob_db_options.ttl_extractor.get()),
      bdb_options_(blob_db_options),
      db_options_(db_options),
      env_options_(db_options),
      dir_change_(false),
      next_file_number_(1),
      epoch_of_(0),
      shutdown_(false),
      current_epoch_(0),
      open_file_count_(0),
      last_period_write_(0),
      last_period_ampl_(0),
      total_periods_write_(0),
      total_periods_ampl_(0),
      total_blob_space_(0),
      open_p1_done_(false),
      debug_level_(0),
      oldest_file_evicted_(false) {
  blob_dir_ = (bdb_options_.path_relative)
                  ? dbname + "/" + bdb_options_.blob_dir
                  : bdb_options_.blob_dir;
}

Status BlobDBImpl::LinkToBaseDB(DB* db) {
  assert(db_ == nullptr);
  assert(open_p1_done_);

  db_ = db;

//基本数据库本身可以是可堆叠的数据库
  db_impl_ = static_cast_with_check<DBImpl, DB>(db_->GetRootDB());

  env_ = db_->GetEnv();

  Status s = env_->CreateDirIfMissing(blob_dir_);
  if (!s.ok()) {
    ROCKS_LOG_WARN(db_options_.info_log,
                   "Failed to create blob directory: %s status: '%s'",
                   blob_dir_.c_str(), s.ToString().c_str());
  }
  s = env_->NewDirectory(blob_dir_, &dir_ent_);
  if (!s.ok()) {
    ROCKS_LOG_WARN(db_options_.info_log,
                   "Failed to open blob directory: %s status: '%s'",
                   blob_dir_.c_str(), s.ToString().c_str());
  }

  if (!bdb_options_.disable_background_tasks) {
    StartBackgroundTasks();
  }
  return s;
}

BlobDBOptions BlobDBImpl::GetBlobDBOptions() const { return bdb_options_; }

BlobDBImpl::BlobDBImpl(DB* db, const BlobDBOptions& blob_db_options)
    : BlobDB(db),
      db_impl_(static_cast_with_check<DBImpl, DB>(db)),
      bdb_options_(blob_db_options),
      db_options_(db->GetOptions()),
      env_options_(db_->GetOptions()),
      dir_change_(false),
      next_file_number_(1),
      epoch_of_(0),
      shutdown_(false),
      current_epoch_(0),
      open_file_count_(0),
      last_period_write_(0),
      last_period_ampl_(0),
      total_periods_write_(0),
      total_periods_ampl_(0),
      total_blob_space_(0),
      oldest_file_evicted_(false) {
  if (!bdb_options_.blob_dir.empty())
    blob_dir_ = (bdb_options_.path_relative)
                    ? db_->GetName() + "/" + bdb_options_.blob_dir
                    : bdb_options_.blob_dir;
}

BlobDBImpl::~BlobDBImpl() {
//取消所有背景工作（db_u，真）；

  Shutdown();
}

Status BlobDBImpl::OpenPhase1() {
  assert(db_ == nullptr);
  if (blob_dir_.empty())
    return Status::NotSupported("No blob directory in options");

  std::unique_ptr<Directory> dir_ent;
  Status s = env_->NewDirectory(blob_dir_, &dir_ent);
  if (!s.ok()) {
    ROCKS_LOG_WARN(db_options_.info_log,
                   "Failed to open blob directory: %s status: '%s'",
                   blob_dir_.c_str(), s.ToString().c_str());
    open_p1_done_ = true;
    return Status::OK();
  }

  s = OpenAllFiles();
  open_p1_done_ = true;
  return s;
}

void BlobDBImpl::StartBackgroundTasks() {
//存储对成员函数和对象的调用
  tqueue_.add(
      kReclaimOpenFilesPeriodMillisecs,
      std::bind(&BlobDBImpl::ReclaimOpenFiles, this, std::placeholders::_1));
  tqueue_.add(kGCCheckPeriodMillisecs,
              std::bind(&BlobDBImpl::RunGC, this, std::placeholders::_1));
  if (bdb_options_.enable_garbage_collection) {
    tqueue_.add(
        kDeleteCheckPeriodMillisecs,
        std::bind(&BlobDBImpl::EvictDeletions, this, std::placeholders::_1));
    tqueue_.add(
        kDeleteCheckPeriodMillisecs,
        std::bind(&BlobDBImpl::EvictCompacted, this, std::placeholders::_1));
  }
  tqueue_.add(
      kDeleteObsoleteFilesPeriodMillisecs,
      std::bind(&BlobDBImpl::DeleteObsoleteFiles, this, std::placeholders::_1));
  tqueue_.add(kSanityCheckPeriodMillisecs,
              std::bind(&BlobDBImpl::SanityCheck, this, std::placeholders::_1));
  tqueue_.add(kWriteAmplificationStatsPeriodMillisecs,
              std::bind(&BlobDBImpl::WaStats, this, std::placeholders::_1));
  tqueue_.add(kFSyncFilesPeriodMillisecs,
              std::bind(&BlobDBImpl::FsyncFiles, this, std::placeholders::_1));
  tqueue_.add(
      kCheckSeqFilesPeriodMillisecs,
      std::bind(&BlobDBImpl::CheckSeqFiles, this, std::placeholders::_1));
}

void BlobDBImpl::Shutdown() { shutdown_.store(true); }

void BlobDBImpl::OnFlushBeginHandler(DB* db, const FlushJobInfo& info) {
  if (shutdown_.load()) return;

//过早发生的回调需要忽略。
  if (!db_) return;

  FsyncFiles(false);
}

Status BlobDBImpl::GetAllLogFiles(
    std::set<std::pair<uint64_t, std::string>>* file_nums) {
  std::vector<std::string> all_files;
  Status status = env_->GetChildren(blob_dir_, &all_files);
  if (!status.ok()) {
    return status;
  }

  for (const auto& f : all_files) {
    uint64_t number;
    FileType type;
    bool psucc = ParseFileName(f, &number, &type);
    if (psucc && type == kBlobFile) {
      file_nums->insert(std::make_pair(number, f));
    } else {
      ROCKS_LOG_WARN(db_options_.info_log,
                     "Skipping file in blob directory %s parse: %d type: %d",
                     f.c_str(), psucc, ((psucc) ? type : -1));
    }
  }

  return status;
}

Status BlobDBImpl::OpenAllFiles() {
  WriteLock wl(&mutex_);

  std::set<std::pair<uint64_t, std::string>> file_nums;
  Status status = GetAllLogFiles(&file_nums);

  if (!status.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to collect files from blob dir: %s status: '%s'",
                    blob_dir_.c_str(), status.ToString().c_str());
    return status;
  }

  ROCKS_LOG_INFO(db_options_.info_log,
                 "BlobDir files path: %s count: %d min: %" PRIu64
                 " max: %" PRIu64,
                 blob_dir_.c_str(), static_cast<int>(file_nums.size()),
                 (file_nums.empty()) ? -1 : (file_nums.begin())->first,
                 (file_nums.empty()) ? -1 : (file_nums.end())->first);

  if (!file_nums.empty())
    next_file_number_.store((file_nums.rbegin())->first + 1);

  for (auto f_iter : file_nums) {
    std::string bfpath = BlobFileName(blob_dir_, f_iter.first);
    uint64_t size_bytes;
    Status s1 = env_->GetFileSize(bfpath, &size_bytes);
    if (!s1.ok()) {
      ROCKS_LOG_WARN(
          db_options_.info_log,
          "Unable to get size of %s. File skipped from open status: '%s'",
          bfpath.c_str(), s1.ToString().c_str());
      continue;
    }

    if (debug_level_ >= 1)
      ROCKS_LOG_INFO(db_options_.info_log, "Blob File open: %s size: %" PRIu64,
                     bfpath.c_str(), size_bytes);

    std::shared_ptr<BlobFile> bfptr =
        std::make_shared<BlobFile>(this, blob_dir_, f_iter.first);
    bfptr->SetFileSize(size_bytes);

//由于此文件已存在，我们将尝试协调
//用LSM删除的计数
    bfptr->gc_once_after_open_ = true;

//读头
    std::shared_ptr<Reader> reader;
    reader = bfptr->OpenSequentialReader(env_, db_options_, env_options_);
    s1 = reader->ReadHeader(&bfptr->header_);
    if (!s1.ok()) {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Failure to read header for blob-file %s "
                      "status: '%s' size: %" PRIu64,
                      bfpath.c_str(), s1.ToString().c_str(), size_bytes);
      continue;
    }
    bfptr->SetHasTTL(bfptr->header_.has_ttl);
    bfptr->SetCompression(bfptr->header_.compression);
    bfptr->header_valid_ = true;

    std::shared_ptr<RandomAccessFileReader> ra_reader =
        GetOrOpenRandomAccessReader(bfptr, env_, env_options_);

    BlobLogFooter bf;
    s1 = bfptr->ReadFooter(&bf);

    bfptr->CloseRandomAccessLocked();
    if (s1.ok()) {
      s1 = bfptr->SetFromFooterLocked(bf);
      if (!s1.ok()) {
        ROCKS_LOG_ERROR(db_options_.info_log,
                        "Header Footer mismatch for blob-file %s "
                        "status: '%s' size: %" PRIu64,
                        bfpath.c_str(), s1.ToString().c_str(), size_bytes);
        continue;
      }
    } else {
      ROCKS_LOG_INFO(db_options_.info_log,
                     "File found incomplete (w/o footer) %s", bfpath.c_str());

//按顺序迭代文件并读取所有记录
      ExpirationRange expiration_range(std::numeric_limits<uint32_t>::max(),
                                       std::numeric_limits<uint32_t>::min());

      uint64_t blob_count = 0;
      BlobLogRecord record;
      Reader::ReadLevel shallow = Reader::kReadHeaderKey;

      uint64_t record_start = reader->GetNextByte();
//当我们发现腐败时，我们应该截短
      while (reader->ReadRecord(&record, shallow).ok()) {
        ++blob_count;
        if (bfptr->HasTTL()) {
          expiration_range.first =
              std::min(expiration_range.first, record.expiration);
          expiration_range.second =
              std::max(expiration_range.second, record.expiration);
        }
        record_start = reader->GetNextByte();
      }

      if (record_start != bfptr->GetFileSize()) {
        ROCKS_LOG_ERROR(db_options_.info_log,
                        "Blob file is corrupted or crashed during write %s"
                        " good_size: %" PRIu64 " file_size: %" PRIu64,
                        bfpath.c_str(), record_start, bfptr->GetFileSize());
      }

      if (!blob_count) {
        ROCKS_LOG_INFO(db_options_.info_log, "BlobCount = 0 in file %s",
                       bfpath.c_str());
        continue;
      }

      bfptr->SetBlobCount(blob_count);
      bfptr->SetSequenceRange({0, 0});

      ROCKS_LOG_INFO(db_options_.info_log,
                     "Blob File: %s blob_count: %" PRIu64
                     " size_bytes: %" PRIu64 " has_ttl: %d",
                     bfpath.c_str(), blob_count, size_bytes, bfptr->HasTTL());

      if (bfptr->HasTTL()) {
        expiration_range.second = std::max(
            expiration_range.second,
            expiration_range.first + (uint32_t)bdb_options_.ttl_range_secs);
        bfptr->set_expiration_range(expiration_range);

        uint64_t now = EpochNow();
        if (expiration_range.second < now) {
          Status fstatus = CreateWriterLocked(bfptr);
          if (fstatus.ok()) fstatus = bfptr->WriteFooterAndCloseLocked();
          if (!fstatus.ok()) {
            ROCKS_LOG_ERROR(
                db_options_.info_log,
                "Failed to close Blob File: %s status: '%s'. Skipped",
                bfpath.c_str(), fstatus.ToString().c_str());
            continue;
          } else {
            ROCKS_LOG_ERROR(
                db_options_.info_log,
                "Blob File Closed: %s now: %d expiration_range: (%d, %d)",
                bfpath.c_str(), now, expiration_range.first,
                expiration_range.second);
          }
        } else {
          open_ttl_files_.insert(bfptr);
        }
      }
    }

    blob_files_.insert(std::make_pair(f_iter.first, bfptr));
  }

  return status;
}

void BlobDBImpl::CloseRandomAccessLocked(
    const std::shared_ptr<BlobFile>& bfile) {
  bfile->CloseRandomAccessLocked();
  open_file_count_--;
}

std::shared_ptr<RandomAccessFileReader> BlobDBImpl::GetOrOpenRandomAccessReader(
    const std::shared_ptr<BlobFile>& bfile, Env* env,
    const EnvOptions& env_options) {
  bool fresh_open = false;
  auto rar = bfile->GetOrOpenRandomAccessReader(env, env_options, &fresh_open);
  if (fresh_open) open_file_count_++;
  return rar;
}

std::shared_ptr<BlobFile> BlobDBImpl::NewBlobFile(const std::string& reason) {
  uint64_t file_num = next_file_number_++;
  auto bfile = std::make_shared<BlobFile>(this, blob_dir_, file_num);
  ROCKS_LOG_DEBUG(db_options_.info_log, "New blob file created: %s reason='%s'",
                  bfile->PathName().c_str(), reason.c_str());
  LogFlush(db_options_.info_log);
  return bfile;
}

Status BlobDBImpl::CreateWriterLocked(const std::shared_ptr<BlobFile>& bfile) {
  std::string fpath(bfile->PathName());
  std::unique_ptr<WritableFile> wfile;

  Status s = env_->ReopenWritableFile(fpath, &wfile, env_options_);
  if (!s.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to open blob file for write: %s status: '%s'"
                    " exists: '%s'",
                    fpath.c_str(), s.ToString().c_str(),
                    env_->FileExists(fpath).ToString().c_str());
    return s;
  }

  std::unique_ptr<WritableFileWriter> fwriter;
  fwriter.reset(new WritableFileWriter(std::move(wfile), env_options_));

  uint64_t boffset = bfile->GetFileSize();
  if (debug_level_ >= 2 && boffset) {
    ROCKS_LOG_DEBUG(db_options_.info_log, "Open blob file: %s with offset: %d",
                    fpath.c_str(), boffset);
  }

  Writer::ElemType et = Writer::kEtNone;
  if (bfile->file_size_ == BlobLogHeader::kSize) {
    et = Writer::kEtFileHdr;
  } else if (bfile->file_size_ > BlobLogHeader::kSize) {
    et = Writer::kEtRecord;
  } else if (bfile->file_size_) {
    ROCKS_LOG_WARN(db_options_.info_log,
                   "Open blob file: %s with wrong size: %d", fpath.c_str(),
                   boffset);
    return Status::Corruption("Invalid blob file size");
  }

  bfile->log_writer_ = std::make_shared<Writer>(
      std::move(fwriter), bfile->file_number_, bdb_options_.bytes_per_sync,
      db_options_.use_fsync, boffset);
  bfile->log_writer_->last_elem_type_ = et;

  return s;
}

std::shared_ptr<BlobFile> BlobDBImpl::FindBlobFileLocked(
    uint64_t expiration) const {
  if (open_ttl_files_.empty()) return nullptr;

  std::shared_ptr<BlobFile> tmp = std::make_shared<BlobFile>();
  tmp->expiration_range_ = std::make_pair(expiration, 0);

  auto citr = open_ttl_files_.equal_range(tmp);
  if (citr.first == open_ttl_files_.end()) {
    assert(citr.second == open_ttl_files_.end());

    std::shared_ptr<BlobFile> check = *(open_ttl_files_.rbegin());
    return (check->expiration_range_.second < expiration) ? nullptr : check;
  }

  if (citr.first != citr.second) return *(citr.first);

  auto finditr = citr.second;
  if (finditr != open_ttl_files_.begin()) --finditr;

  bool b2 = (*finditr)->expiration_range_.second < expiration;
  bool b1 = (*finditr)->expiration_range_.first > expiration;

  return (b1 || b2) ? nullptr : (*finditr);
}

std::shared_ptr<Writer> BlobDBImpl::CheckOrCreateWriterLocked(
    const std::shared_ptr<BlobFile>& bfile) {
  std::shared_ptr<Writer> writer = bfile->GetWriter();
  if (writer) return writer;

  Status s = CreateWriterLocked(bfile);
  if (!s.ok()) return nullptr;

  writer = bfile->GetWriter();
  return writer;
}

std::shared_ptr<BlobFile> BlobDBImpl::SelectBlobFile() {
  {
    ReadLock rl(&mutex_);
    if (open_non_ttl_file_ != nullptr) {
      return open_non_ttl_file_;
    }
  }

//再次检查
  WriteLock wl(&mutex_);
  if (open_non_ttl_file_ != nullptr) {
    return open_non_ttl_file_;
  }

  std::shared_ptr<BlobFile> bfile = NewBlobFile("SelectBlobFile");
  assert(bfile);

//文件不可见，因此没有锁定
  std::shared_ptr<Writer> writer = CheckOrCreateWriterLocked(bfile);
  if (!writer) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to get writer from blob file: %s",
                    bfile->PathName().c_str());
    return nullptr;
  }

  bfile->file_size_ = BlobLogHeader::kSize;
  bfile->header_.compression = bdb_options_.compression;
  bfile->header_.has_ttl = false;
  bfile->header_.column_family_id =
      reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily())->GetID();
  bfile->header_valid_ = true;
  bfile->SetHasTTL(false);
  bfile->SetCompression(bdb_options_.compression);

  Status s = writer->WriteHeader(bfile->header_);
  if (!s.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to write header to new blob file: %s"
                    " status: '%s'",
                    bfile->PathName().c_str(), s.ToString().c_str());
    return nullptr;
  }

  dir_change_.store(true);
  blob_files_.insert(std::make_pair(bfile->BlobFileNumber(), bfile));
  open_non_ttl_file_ = bfile;
  return bfile;
}

std::shared_ptr<BlobFile> BlobDBImpl::SelectBlobFileTTL(uint64_t expiration) {
  assert(expiration != kNoExpiration);
  uint64_t epoch_read = 0;
  std::shared_ptr<BlobFile> bfile;
  {
    ReadLock rl(&mutex_);
    bfile = FindBlobFileLocked(expiration);
    epoch_read = epoch_of_.load();
  }

  if (bfile) {
    assert(!bfile->Immutable());
    return bfile;
  }

  uint64_t exp_low =
      (expiration / bdb_options_.ttl_range_secs) * bdb_options_.ttl_range_secs;
  uint64_t exp_high = exp_low + bdb_options_.ttl_range_secs;
  ExpirationRange expiration_range = std::make_pair(exp_low, exp_high);

  bfile = NewBlobFile("SelectBlobFileTTL");
  assert(bfile);

  ROCKS_LOG_INFO(db_options_.info_log, "New blob file TTL range: %s %d %d",
                 bfile->PathName().c_str(), exp_low, exp_high);
  LogFlush(db_options_.info_log);

//我们不需要使用锁，因为其他线程还没有看到bfile
  std::shared_ptr<Writer> writer = CheckOrCreateWriterLocked(bfile);
  if (!writer) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to get writer from blob file with TTL: %s",
                    bfile->PathName().c_str());
    return nullptr;
  }

  bfile->header_.expiration_range = expiration_range;
  bfile->header_.compression = bdb_options_.compression;
  bfile->header_.has_ttl = true;
  bfile->header_.column_family_id =
      reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily())->GetID();
  ;
  bfile->header_valid_ = true;
  bfile->SetHasTTL(true);
  bfile->SetCompression(bdb_options_.compression);
  bfile->file_size_ = BlobLogHeader::kSize;

//设置范围的第一个值，因为这是
//此时混凝土。还需要添加以打开“ttl”文件
  bfile->expiration_range_ = expiration_range;

  WriteLock wl(&mutex_);
//如果在这段时间内时代发生了变化，则检查
//再次检查状况-应该很少。
  if (epoch_of_.load() != epoch_read) {
    auto bfile2 = FindBlobFileLocked(expiration);
    if (bfile2) return bfile2;
  }

  Status s = writer->WriteHeader(bfile->header_);
  if (!s.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failed to write header to new blob file: %s"
                    " status: '%s'",
                    bfile->PathName().c_str(), s.ToString().c_str());
    return nullptr;
  }

  dir_change_.store(true);
  blob_files_.insert(std::make_pair(bfile->BlobFileNumber(), bfile));
  open_ttl_files_.insert(bfile);
  epoch_of_++;

  return bfile;
}

Status BlobDBImpl::Delete(const WriteOptions& options, const Slice& key) {
  SequenceNumber lsn = db_impl_->GetLatestSequenceNumber();
  Status s = db_->Delete(options, key);

  if (bdb_options_.enable_garbage_collection) {
//将已删除的密钥添加到已删除用于记账的密钥列表中
    delete_keys_q_.enqueue({DefaultColumnFamily(), key.ToString(), lsn});
  }
  return s;
}

class BlobDBImpl::BlobInserter : public WriteBatch::Handler {
 private:
  const WriteOptions& options_;
  BlobDBImpl* blob_db_impl_;
  uint32_t default_cf_id_;
  SequenceNumber sequence_;
  WriteBatch batch_;

 public:
  BlobInserter(const WriteOptions& options, BlobDBImpl* blob_db_impl,
               uint32_t default_cf_id, SequenceNumber seq)
      : options_(options),
        blob_db_impl_(blob_db_impl),
        default_cf_id_(default_cf_id),
        sequence_(seq) {}

  SequenceNumber sequence() { return sequence_; }

  WriteBatch* batch() { return &batch_; }

  virtual Status PutCF(uint32_t column_family_id, const Slice& key,
                       const Slice& value) override {
    if (column_family_id != default_cf_id_) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    std::string new_value;
    Slice value_slice;
    uint64_t expiration =
        blob_db_impl_->ExtractExpiration(key, value, &value_slice, &new_value);
    Status s = blob_db_impl_->PutBlobValue(options_, key, value_slice,
                                           expiration, sequence_, &batch_);
    sequence_++;
    return s;
  }

  virtual Status DeleteCF(uint32_t column_family_id,
                          const Slice& key) override {
    if (column_family_id != default_cf_id_) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    Status s = WriteBatchInternal::Delete(&batch_, column_family_id, key);
    sequence_++;
    return s;
  }

  virtual Status DeleteRange(uint32_t column_family_id, const Slice& begin_key,
                             const Slice& end_key) {
    if (column_family_id != default_cf_id_) {
      return Status::NotSupported(
          "Blob DB doesn't support non-default column family.");
    }
    Status s = WriteBatchInternal::DeleteRange(&batch_, column_family_id,
                                               begin_key, end_key);
    sequence_++;
    return s;
  }

  /*实际状态单个删除cf（uint32_t/*列_family_id*/，
                                常量切片&/*ke*/) override {

    return Status::NotSupported("Not supported operation in blob db.");
  }

  /*实际状态合并cf（uint32_t/*column_family_id*/，const slice&/*key*/，
                         常量切片和/值*/) override {

    return Status::NotSupported("Not supported operation in blob db.");
  }

  virtual void LogData(const Slice& blob) override { batch_.PutLogData(blob); }
};

Status BlobDBImpl::Write(const WriteOptions& options, WriteBatch* updates) {

  uint32_t default_cf_id =
      reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily())->GetID();
//托多（义乌）：如果有多个作者，最新的序列将
//不是我们写的顺序。需要得到序列
//从写入批处理到数据库写入。
  SequenceNumber current_seq = GetLatestSequenceNumber() + 1;
  Status s;
  BlobInserter blob_inserter(options, this, default_cf_id, current_seq);
  {
//在db write之前释放write-mutex，以避免与
//flush begin listener，它还需要写入mutex来同步
//BLB文件。
    MutexLock l(&write_mutex_);
    s = updates->Iterate(&blob_inserter);
  }
  if (!s.ok()) {
    return s;
  }
  s = db_->Write(options, blob_inserter.batch());
  if (!s.ok()) {
    return s;
  }

//将已删除的密钥添加到已删除用于记账的密钥列表中
  class DeleteBookkeeper : public WriteBatch::Handler {
   public:
    explicit DeleteBookkeeper(BlobDBImpl* impl, const SequenceNumber& seq)
        : impl_(impl), sequence_(seq) {}

    /*tual status putcf（uint32_t/*column_family_id*/，const slice&/*key*/，
                         常量切片和/值*/) override {

      sequence_++;
      return Status::OK();
    }

    virtual Status DeleteCF(uint32_t column_family_id,
                            const Slice& key) override {
      ColumnFamilyHandle* cfh =
          impl_->db_impl_->GetColumnFamilyHandleUnlocked(column_family_id);

      impl_->delete_keys_q_.enqueue({cfh, key.ToString(), sequence_});
      sequence_++;
      return Status::OK();
    }

   private:
    BlobDBImpl* impl_;
    SequenceNumber sequence_;
  };

  if (bdb_options_.enable_garbage_collection) {
//将已删除的密钥添加到已删除用于记账的密钥列表中
    DeleteBookkeeper delete_bookkeeper(this, current_seq);
    s = updates->Iterate(&delete_bookkeeper);
  }

  return s;
}

Status BlobDBImpl::GetLiveFiles(std::vector<std::string>& ret,
                                uint64_t* manifest_file_size,
                                bool flush_memtable) {
//在开始时保持锁定，以避免在调用期间更新基数据库
  ReadLock rl(&mutex_);
  Status s = db_->GetLiveFiles(ret, manifest_file_size, flush_memtable);
  if (!s.ok()) {
    return s;
  }
  ret.reserve(ret.size() + blob_files_.size());
  for (auto bfile_pair : blob_files_) {
    auto blob_file = bfile_pair.second;
    ret.emplace_back(blob_file->PathName());
  }
  return Status::OK();
}

void BlobDBImpl::GetLiveFilesMetaData(std::vector<LiveFileMetaData>* metadata) {
//在开始时保持锁定，以避免在调用期间更新基数据库
  ReadLock rl(&mutex_);
  db_->GetLiveFilesMetaData(metadata);
  for (auto bfile_pair : blob_files_) {
    auto blob_file = bfile_pair.second;
    LiveFileMetaData filemetadata;
    filemetadata.size = blob_file->GetFileSize();
    filemetadata.name = blob_file->PathName();
    auto cfh =
        reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily());
    filemetadata.column_family_name = cfh->GetName();
    metadata->emplace_back(filemetadata);
  }
}

Status BlobDBImpl::Put(const WriteOptions& options, const Slice& key,
                       const Slice& value) {
  std::string new_value;
  Slice value_slice;
  uint64_t expiration = ExtractExpiration(key, value, &value_slice, &new_value);
  return PutUntil(options, key, value_slice, expiration);
}

Status BlobDBImpl::PutWithTTL(const WriteOptions& options,
                              const Slice& key, const Slice& value,
                              uint64_t ttl) {
  uint64_t now = EpochNow();
  uint64_t expiration = kNoExpiration - now > ttl ? now + ttl : kNoExpiration;
  return PutUntil(options, key, value, expiration);
}

Status BlobDBImpl::PutUntil(const WriteOptions& options, const Slice& key,
                            const Slice& value, uint64_t expiration) {
  TEST_SYNC_POINT("BlobDBImpl::PutUntil:Start");
  Status s;
  WriteBatch batch;
  {
//在db write之前释放write-mutex，以避免与
//flush begin listener，它还需要写入mutex来同步
//BLB文件。
    MutexLock l(&write_mutex_);
//托多（义乌）：如果有多个作者，最新的序列将
//不是我们写的顺序。需要得到序列
//从写入批处理到数据库写入。
    SequenceNumber sequence = GetLatestSequenceNumber() + 1;
    s = PutBlobValue(options, key, value, expiration, sequence, &batch);
  }
  if (s.ok()) {
    s = db_->Write(options, &batch);
  }
  TEST_SYNC_POINT("BlobDBImpl::PutUntil:Finish");
  return s;
}

Status BlobDBImpl::PutBlobValue(const WriteOptions& options, const Slice& key,
                                const Slice& value, uint64_t expiration,
                                SequenceNumber sequence, WriteBatch* batch) {
  Status s;
  std::string index_entry;
  uint32_t column_family_id =
      reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily())->GetID();
  if (value.size() < bdb_options_.min_blob_size) {
    if (expiration == kNoExpiration) {
//作为正常值
      s = batch->Put(key, value);
    } else {
//与TTL内联
      BlobIndex::EncodeInlinedTTL(&index_entry, expiration, value);
      s = WriteBatchInternal::PutBlobIndex(batch, column_family_id, key,
                                           index_entry);
    }
  } else {
    std::shared_ptr<BlobFile> bfile = (expiration != kNoExpiration)
                                          ? SelectBlobFileTTL(expiration)
                                          : SelectBlobFile();
    if (!bfile) {
      return Status::NotFound("Blob file not found");
    }

    assert(bfile->compression() == bdb_options_.compression);
    std::string compression_output;
    Slice value_compressed = GetCompressedSlice(value, &compression_output);

    std::string headerbuf;
    Writer::ConstructBlobHeader(&headerbuf, key, value_compressed, expiration);

    s = AppendBlob(bfile, headerbuf, key, value_compressed, expiration,
                   &index_entry);

    if (s.ok()) {
      bfile->ExtendSequenceRange(sequence);
      if (expiration != kNoExpiration) {
        bfile->ExtendExpirationRange(expiration);
      }
      s = CloseBlobFileIfNeeded(bfile);
      if (s.ok()) {
        s = WriteBatchInternal::PutBlobIndex(batch, column_family_id, key,
                                             index_entry);
      }
    } else {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Failed to append blob to FILE: %s: KEY: %s VALSZ: %d"
                      " status: '%s' blob_file: '%s'",
                      bfile->PathName().c_str(), key.ToString().c_str(),
                      value.size(), s.ToString().c_str(),
                      bfile->DumpState().c_str());
    }
  }

  return s;
}

Slice BlobDBImpl::GetCompressedSlice(const Slice& raw,
                                     std::string* compression_output) const {
  if (bdb_options_.compression == kNoCompression) {
    return raw;
  }
  CompressionType ct = bdb_options_.compression;
  CompressionOptions compression_opts;
  CompressBlock(raw, compression_opts, &ct, kBlockBasedTableVersionFormat,
                Slice(), compression_output);
  return *compression_output;
}

uint64_t BlobDBImpl::ExtractExpiration(const Slice& key, const Slice& value,
                                       Slice* value_slice,
                                       std::string* new_value) {
  uint64_t expiration = kNoExpiration;
  bool has_expiration = false;
  bool value_changed = false;
  if (ttl_extractor_ != nullptr) {
    has_expiration = ttl_extractor_->ExtractExpiration(
        key, value, EpochNow(), &expiration, new_value, &value_changed);
  }
  *value_slice = value_changed ? Slice(*new_value) : value;
  return has_expiration ? expiration : kNoExpiration;
}

std::shared_ptr<BlobFile> BlobDBImpl::GetOldestBlobFile() {
  std::vector<std::shared_ptr<BlobFile>> blob_files;
  CopyBlobFiles(&blob_files, [](const std::shared_ptr<BlobFile>& f) {
    return !f->Obsolete() && f->Immutable();
  });
  blobf_compare_ttl compare;
  return *std::min_element(blob_files.begin(), blob_files.end(), compare);
}

bool BlobDBImpl::EvictOldestBlobFile() {
  auto oldest_file = GetOldestBlobFile();
  if (oldest_file == nullptr) {
    return false;
  }

  WriteLock wl(&mutex_);
//再次检查文件是否被其他人废弃
  if (oldest_file_evicted_ == false && !oldest_file->Obsolete()) {
    auto expiration_range = oldest_file->GetExpirationRange();
    ROCKS_LOG_INFO(db_options_.info_log,
                   "Evict oldest blob file since DB out of space. Current "
                   "space used: %" PRIu64 ", blob dir size: %" PRIu64
                   ", evicted blob file #%" PRIu64
                   " with expiration range (%" PRIu64 ", %" PRIu64 ").",
                   total_blob_space_.load(), bdb_options_.blob_dir_size,
                   oldest_file->BlobFileNumber(), expiration_range.first,
                   expiration_range.second);
    oldest_file->MarkObsolete(oldest_file->GetSequenceRange().second);
    obsolete_files_.push_back(oldest_file);
    oldest_file_evicted_.store(true);
    return true;
  }

  return false;
}

Status BlobDBImpl::CheckSize(size_t blob_size) {
  uint64_t new_space_util = total_blob_space_.load() + blob_size;
  if (bdb_options_.blob_dir_size > 0) {
    if (!bdb_options_.is_fifo &&
        (new_space_util > bdb_options_.blob_dir_size)) {
      return Status::NoSpace(
          "Write failed, as writing it would exceed blob_dir_size limit.");
    }
    if (bdb_options_.is_fifo && !oldest_file_evicted_.load() &&
        (new_space_util >
         kEvictOldestFileAtSize * bdb_options_.blob_dir_size)) {
      EvictOldestBlobFile();
    }
  }

  return Status::OK();
}

Status BlobDBImpl::AppendBlob(const std::shared_ptr<BlobFile>& bfile,
                              const std::string& headerbuf, const Slice& key,
                              const Slice& value, uint64_t expiration,
                              std::string* index_entry) {
  auto size_put = BlobLogRecord::kHeaderSize + key.size() + value.size();
  Status s = CheckSize(size_put);
  if (!s.ok()) {
    return s;
  }

  uint64_t blob_offset = 0;
  uint64_t key_offset = 0;
  {
    WriteLock lockbfile_w(&bfile->mutex_);
    std::shared_ptr<Writer> writer = CheckOrCreateWriterLocked(bfile);
    if (!writer) return Status::IOError("Failed to create blob writer");

//将blob写入blob日志。
    s = writer->EmitPhysicalRecord(headerbuf, key, value, &key_offset,
                                   &blob_offset);
  }

  if (!s.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Invalid status in AppendBlob: %s status: '%s'",
                    bfile->PathName().c_str(), s.ToString().c_str());
    return s;
  }

//增量blob计数
  bfile->blob_count_++;

  bfile->file_size_ += size_put;
  last_period_write_ += size_put;
  total_blob_space_ += size_put;

  if (expiration == kNoExpiration) {
    BlobIndex::EncodeBlob(index_entry, bfile->BlobFileNumber(), blob_offset,
                          value.size(), bdb_options_.compression);
  } else {
    BlobIndex::EncodeBlobTTL(index_entry, expiration, bfile->BlobFileNumber(),
                             blob_offset, value.size(),
                             bdb_options_.compression);
  }

  return s;
}

std::vector<Status> BlobDBImpl::MultiGet(
    const ReadOptions& read_options,
    const std::vector<Slice>& keys, std::vector<std::string>* values) {
//获取快照以避免在我们之间删除blob文件
//从文件中获取并索引条目和读取。
  ReadOptions ro(read_options);
  bool snapshot_created = SetSnapshotIfNeeded(&ro);

  std::vector<Status> statuses;
  statuses.reserve(keys.size());
  values->clear();
  values->reserve(keys.size());
  PinnableSlice value;
  for (size_t i = 0; i < keys.size(); i++) {
    statuses.push_back(Get(ro, DefaultColumnFamily(), keys[i], &value));
    values->push_back(value.ToString());
    value.Reset();
  }
  if (snapshot_created) {
    db_->ReleaseSnapshot(ro.snapshot);
  }
  return statuses;
}

bool BlobDBImpl::SetSnapshotIfNeeded(ReadOptions* read_options) {
  assert(read_options != nullptr);
  if (read_options->snapshot != nullptr) {
    return false;
  }
  read_options->snapshot = db_->GetSnapshot();
  return true;
}

Status BlobDBImpl::GetBlobValue(const Slice& key, const Slice& index_entry,
                                PinnableSlice* value) {
  assert(value != nullptr);
  BlobIndex blob_index;
  Status s = blob_index.DecodeFrom(index_entry);
  if (!s.ok()) {
    return s;
  }
  if (blob_index.HasTTL() && blob_index.expiration() <= EpochNow()) {
    return Status::NotFound("Key expired");
  }
  if (blob_index.IsInlined()) {
//TODO（义乌）：如果索引项是可固定切片，我们也可以固定相同的切片。
//内存缓冲区以避免额外的复制。
    value->PinSelf(blob_index.value());
    return Status::OK();
  }
  if (blob_index.size() == 0) {
    value->PinSelf("");
    return Status::OK();
  }

//偏移必须有一定的最小值，因为我们将读取CRC
//后面的blob头，它还需要是
//有效偏移量。
  if (blob_index.offset() <
      (BlobLogHeader::kSize + BlobLogRecord::kHeaderSize + key.size())) {
    if (debug_level_ >= 2) {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Invalid blob index file_number: %" PRIu64
                      " blob_offset: %" PRIu64 " blob_size: %" PRIu64
                      " key: %s",
                      blob_index.file_number(), blob_index.offset(),
                      blob_index.size(), key.data());
    }
    return Status::NotFound("Invalid blob offset");
  }

  std::shared_ptr<BlobFile> bfile;
  {
    ReadLock rl(&mutex_);
    auto hitr = blob_files_.find(blob_index.file_number());

//文件已删除
    if (hitr == blob_files_.end()) {
      return Status::NotFound("Blob Not Found as blob file missing");
    }

    bfile = hitr->second;
  }

  if (blob_index.size() == 0 && value != nullptr) {
    value->PinSelf("");
    return Status::OK();
  }

//调用时获取锁
  std::shared_ptr<RandomAccessFileReader> reader =
      GetOrOpenRandomAccessReader(bfile, env_, env_options_);

  std::string* valueptr = value->GetSelf();
  std::string value_c;
  if (bdb_options_.compression != kNoCompression) {
    valueptr = &value_c;
  }

//分配缓冲区。这在C++ 11中是安全的。
//注意，std:：string:：reserved（）不起作用，因为前面的值
//缓冲区的大小可以大于blob_index.size（）。
  valueptr->resize(blob_index.size());
  char* buffer = &(*valueptr)[0];

  Slice blob_value;
  s = reader->Read(blob_index.offset(), blob_index.size(), &blob_value, buffer);
  if (!s.ok() || blob_value.size() != blob_index.size()) {
    if (debug_level_ >= 2) {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Failed to read blob from file: %s blob_offset: %" PRIu64
                      " blob_size: %" PRIu64 " read: %d key: %s status: '%s'",
                      bfile->PathName().c_str(), blob_index.offset(),
                      blob_index.size(), static_cast<int>(blob_value.size()),
                      key.data(), s.ToString().c_str());
    }
    return Status::NotFound("Blob Not Found as couldnt retrieve Blob");
  }

//TODO（义乌）：添加跳过CRC检查的选项。
  Slice crc_slice;
  uint32_t crc_exp;
  std::string crc_str;
  crc_str.resize(sizeof(uint32_t));
  char* crc_buffer = &(crc_str[0]);
  s = reader->Read(blob_index.offset() - (key.size() + sizeof(uint32_t)),
                   sizeof(uint32_t), &crc_slice, crc_buffer);
  if (!s.ok() || !GetFixed32(&crc_slice, &crc_exp)) {
    if (debug_level_ >= 2) {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Failed to fetch blob crc file: %s blob_offset: %" PRIu64
                      " blob_size: %" PRIu64 " key: %s status: '%s'",
                      bfile->PathName().c_str(), blob_index.offset(),
                      blob_index.size(), key.data(), s.ToString().c_str());
    }
    return Status::NotFound("Blob Not Found as couldnt retrieve CRC");
  }

  uint32_t crc = crc32c::Value(key.data(), key.size());
  crc = crc32c::Extend(crc, blob_value.data(), blob_value.size());
crc = crc32c::Mask(crc);  //调整存储
  if (crc != crc_exp) {
    if (debug_level_ >= 2) {
      ROCKS_LOG_ERROR(db_options_.info_log,
                      "Blob crc mismatch file: %s blob_offset: %" PRIu64
                      " blob_size: %" PRIu64 " key: %s status: '%s'",
                      bfile->PathName().c_str(), blob_index.offset(),
                      blob_index.size(), key.data(), s.ToString().c_str());
    }
    return Status::Corruption("Corruption. Blob CRC mismatch");
  }

  if (bfile->compression() != kNoCompression) {
    BlockContents contents;
    auto cfh = reinterpret_cast<ColumnFamilyHandleImpl*>(DefaultColumnFamily());
    s = UncompressBlockContentsForCompressionType(
        blob_value.data(), blob_value.size(), &contents,
        kBlockBasedTableVersionFormat, Slice(), bfile->compression(),
        *(cfh->cfd()->ioptions()));
    *(value->GetSelf()) = contents.data.ToString();
  }

  value->PinSelf();

  return s;
}

Status BlobDBImpl::Get(const ReadOptions& read_options,
                       ColumnFamilyHandle* column_family, const Slice& key,
                       PinnableSlice* value) {
  if (column_family != DefaultColumnFamily()) {
    return Status::NotSupported(
        "Blob DB doesn't support non-default column family.");
  }
//获取快照以避免在我们之间删除blob文件
//从文件中获取并索引条目和读取。
//TODO（义乌）：对于get（），如果找不到文件，则重试是一种更简单的策略。
  ReadOptions ro(read_options);
  bool snapshot_created = SetSnapshotIfNeeded(&ro);

  Status s;
  bool is_blob_index = false;
  s = db_impl_->GetImpl(ro, column_family, key, value,
                        /*lptr/*value_found*/，&is_blob_index）；
  测试同步点（“blobdbimpl:：get:afterindexentryget:1”）；
  测试同步点（“blobdbimpl:：get:afterindexentryget:2”）；
  如果（s.ok（）&&is_blob_index）
    std:：string index_entry=value->toString（）；
    值>重新设置（）；
    s=getBlobValue（key，index_entry，value）；
  }
  如果（创建快照）
    db_uu->releasesnapshot（ro.snapshot）；
  }
  返回S；
}

std:：pair<bool，int64_t>blobdbimpl:：sanitycheck（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  rocks_log_info（db_options_u.info_log，“启动健全性检查”）；

  rocks_log_info（db_options_u.info_log，“number of files%”priu64，
                 blob_文件_.size（））；

  rocks日志信息（db选项日志，“打开文件数%”priu64，
                 打开_ttl_files_u.size（））；

  用于（自动文件：打开“ttl”文件）
    断言（！）bfile->immutable（））；
  }

  uint64_t epoch_now=epoch now（）；

  对于（自动文件对：blob文件对）
    auto bfile=bfile_pair.second；
    罗克斯洛格洛夫
        数据库选项信息日志，
        “blob文件%s%”priu64“%”priu64“%”priu64“%”priu64“%”priu64“%”priu64，
        bfile->pathname（）.c_str（），bfile->getfilesize（），bfile->blobcount（），
        bfile->deleted_count_u，bfile->deleted_size_u，
        （bfile->expiration_range_uu2-epoch_now））；
  }

  / /重新安排
  返回std:：make_pair（true，-1）；
}

状态blobdbimpl:：closeblobfile（std:：shared_ptr<blobfile>bfile）
  断言（b锉！= null pTr）；
  状态S；
  rocks_log_info（db_options_uu info_log，“close blob file%”priu64，
                 bfile->blobfilenumber（））；
  {
    writelock wl（&mutex_uu）；

    if（bfile->hastl（））
      大小“删除”属性（“未使用”）；
      erased=打开“ttl”文件“erase”（b文件）；
      断言（擦除==1）；
    }否则{
      断言（bfile=open_non_ttl_file）；
      打开_non_ttl_file_uu=nullptr；
    }
  }

  如果（！）bfile->closed_.load（））
    writelock lockbfile_w（&bfile->mutex_）；
    s=bfile->writefooterandcloselocked（）；
  }

  如果（！）S.O.（））{
    岩石记录错误（数据库选项记录，
                    “无法关闭blob文件%”priu64“，错误为：%s”，
                    bfile->blobfilenumber（），s.toString（）.c_str（））；
  }

  返回S；
}

状态blobdbimpl:：closeblobfileif needed（std:：shared_ptr<blobfile>&bfile）
  //原子读
  if（bfile->getfilesize（）<bdb_options_u.blob_file_size）
    返回状态：：OK（）；
  }
  返回closeblobfile（bfile）；
}

bool blobdbimpl:：visibletoactivesnapshot（）。
    const std:：shared&ptr<blobfile>&bfile）
  断言（bfile->obsolete（））；
  sequenceNumber first_sequence=bfile->getSequenceRange（）。first；
  SequenceNumber Obsolete_Sequence=bFile->GetObsoleteSequence（）；
  返回db impl->hasActiveSnapshotinRange（第一个序列，过时的序列）；
}

bool blobdbimpl:：findfileandevicetablob（uint64_t file_number，uint64_t key_size，
                                       uint64_t blob_偏移，
                                       uint64_t blob_大小）
  断言（bdb_选项启用垃圾收集）；
  （无效）斑点偏移；
  std:：shared_ptr<blobfile>bfile；
  {
    readlock rl（&mutex_u）；
    auto hitr=blob_files_u.find（文件编号）；

    //文件已删除
    if（hitr==blob_files_.end（））
      返回错误；
    }

    bfile=hitr->second；
  }

  writelock lockbfile_w（&bfile->mutex_）；

  bFile->Deleted_Count_++；
  bfile->deleted_size_+=key_size+blob_size+blobloblogrecord:：kheadersize；
  回归真实；
}

bool blobdbimpl:：markblobdeleted（const slice&key，const slice&indexentry）
  断言（bdb_选项启用垃圾收集）；
  blob index blob_索引；
  状态S=blob_index.decodefrom（index_entry）；
  如果（！）S.O.（））{
    岩石日志信息（数据库选项日志，
                   “无法分析markblobdeleted%s中的lsm val”，
                   index_entry.toString（）.c_str（））；
    返回错误；
  }
  bool suc=findfileandevicetablob（blob_index.file_number（），key.size（），
                                    blob_index.offset（），blob_index.size（））；
  返回；
}

std:：pair<bool，int64_t>blobdbimpl:：evictcompressed（bool中止）
  断言（bdb_选项启用垃圾收集）；
  如果（中止）返回std：：make_pair（false，-1）；

  覆盖数据包；
  尺寸总值=0；
  尺寸标记收回=0；
  while（覆盖数据包）
    bool成功=
        findfileandevicetablob（packet.file_number_u，packet.key_size_u，
                              packet.blob偏移量，packet.blob大小；
    总计+ +；
    如果（成功）
      马可驱逐+++；
    }
  }
  岩石日志信息（数据库选项日志，
                 “Mark%”Rocksdb_Priszt
                 “要排出的值，超出%”Rocksdb_Priszt
                 “压缩值。”，
                 Mark_Eviced，总数）；
  返回std:：make_pair（true，-1）；
}

std:：pair<bool，int64_t>blobdbimpl:：evictDeletions（bool已中止）
  断言（bdb_选项启用垃圾收集）；
  如果（中止）返回std：：make_pair（false，-1）；

  columnFamilyhandle*last_cfh=nullptr；
  最后选项

  竞技场竞技场；
  标定器ITER；

  //我们将对所有CF使用相同的RangeDelaggregator。
  //本质上我们现在不支持范围删除
  std:：unique_ptr<rangedelaggregator>range_del_agg；
  删除数据包；
  while（删除_-keys_q_u.dequeue（&dpacket））
    如果（LASTHYCFH）！=dpacket.cfh_123;
      如果（！）范围
        auto cfhi=reinterpret_cast<columnfamilyhandleimpl*>（dpacket.cfh_uuu）；
        auto cfd=cfhi->cfd（）；
        范围重置（新的RangeDelaggregator（CFD->Internal_Comparator（），
                                                   k最大序列号）；
      }

      //这可能很贵
      最后一个CFH=dpacket.cfh_u；
      last_op=db_impl_u->getoptions（last_cfh）；
      iter.set（db_impl_u->newInternalIterator（&arena，range_del_agg.get（），
                                             DPACK.CFHHY）；
      //这对多个CF不起作用。
    }

    切片用户密钥（dpacket.key）；
    InternalKey目标（用户密钥、dpacket.dsn、ktypeValue）；

    slice eslice=target.encode（）；
    iter->seek（eslice）；

    如果（！）iter->status（）.ok（））
      rocks_log_info（db_options_u.info_log，“无效的迭代器查找%s”，
                     dpacket.key_u.c_str（））；
      继续；
    }

    const comparator*bwc=bytewiseComparator（）；
    while（iter->valid（））
      如果（！）bwc->equal（extractUserKey（iter->key（）），extractUserKey（eslice）））
        断裂；

      parsedinteralkey ikey（slice（），0，ktypeValue）；
      如果（！）parseInternalKey（iter->key（），&ikey））
        继续；
      }

      //一旦点击删除，假设下面的键
      //以前处理过
      if（ikey.type==ktypedelection ikey.type==ktypedsingledeletion）break；

      slice val=iter->value（）；
      markblobdeleted（ikey.user_key，val）；

      ITER > NEXT（）；
    }
  }
  返回std:：make_pair（true，-1）；
}

std:：pair<bool，int64_t>blobdbimpl:：checkseqfiles（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  std:：vector<std:：shared_ptr<blobfile>>进程文件；
  {
    uint64_t epoch_now=epoch now（）；

    readlock rl（&mutex_u）；
    用于（自动文件：打开“ttl”文件）
      {
        readlock lockbfile_r（&bfile->mutex_）；

        如果（bfile->expiration\u range\u2>epoch\u now）继续；
        处理文件。向后推（bfile）；
      }
    }
  }

  用于（自动文件：进程文件）
    关闭文件（bfile）；
  }

  返回std:：make_pair（true，-1）；
}

std:：pair<bool，int64_t>blobdbimpl:：fsyncfiles（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  mutexlock l（&write_mutex_u）；

  std:：vector<std:：shared_ptr<blobfile>>进程文件；
  {
    readlock rl（&mutex_u）；
    对于（auto-fitr:open_ttl_files_
      if（fitr->needsfsync（true，bdb_options_u.bytes_per_sync））。
        处理文件。向后推（fitr）；
    }

    如果（打开非TTL文件）= NulLPTR & &
        打开_non_ttl_file_u->needsfsync（true，bdb_options_u.bytes_per_sync））
      处理文件。向后推（打开非TTL文件）；
    }
  }

  对于（auto-fitr:process_files）
    if（fitr->needsfsync（true，bdb_options_u.bytes_per_sync））fitr->fsync（）；
  }

  bool expected=true；
  if（dir_change_uu.compare_exchange_weak（expected，false））dir_u ent_uuu->fsync（）；

  返回std:：make_pair（true，-1）；
}

std:：pair<bool，int64_t>blobdbimpl:：recrealOpenfiles（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  if（open_file_count_u.load（）<kopenfilesTrigger）
    返回std:：make_pair（true，-1）；
  }

  //在将来，我们应该按最后一次访问进行排序
  //而不是关闭每个文件
  readlock rl（&mutex_u）；
  对于（auto-const&ent:blob_files_）
    自动bfile=ent.second；
    如果（bfile->last_access_u.load（）==-1）继续；

    writelock lockbfile_w（&bfile->mutex_）；
    CloseRandoAccessLocked（b文件）；
  }

  返回std:：make_pair（true，-1）；
}

//todo（义乌）：更正统计并公开。
std:：pair<bool，int64_t>blobdbimpl:：wastats（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  writelock wl（&mutex_uu）；

  if（所有_periods_write_.size（）>=kWriteamplificationStatsPeriods）
    总的_periods_write_-=（*所有_periods_write_.begin（））；
    _periods_ampl_u=（*所有_periods_ampl_u.begin（））；

    所有_periods_write_u.pop_front（）；
    所有_periods_ampl_u.pop_front（）；
  }

  uint64_t val1=最后一个_period_write_.load（）；
  uint64_t val2=最后一个_period_ampl_u.load（）；

  所有时间段写回（val1）；
  所有时间段Ampl_uuU。向后推U（Val2）；

  最后一个时期写为0；
  最后一个周期Ampl_uu0；

  总时间段
  总周期数

  返回std:：make_pair（true，-1）；
}

//写入垃圾收集的回调以检查密钥是否已更新
//自上次读取以来。类似于optimatictransaction的工作方式。见内联
//gcfileandupdatelsm（）中的注释。
类blobdbimpl:：garbageCollectionWriteCallback:公共WriteCallback
 公众：
  GarbageCollectionWriteCallback（columnFamilyData*CFD，const slice&key，
                                 序列号上限）
      ：cfd_uu（cfd），key_u（key），upper_bound_u（upper_bound）

  虚拟状态回调（db*db）重写
    auto*db_impl=reinterpret_cast<db impl*>（db）；
    auto*sv=db_impl->getandrefuscorrusion（cfd_uu）；
    序列号最新序列号=0；
    bool找到\u record\u for \u key=false；
    bool is_blob_index=false；
    状态s=db_impl->getlatestSequenceForkey（
        sv，key_u，false/*缓存\u onl*/, &latest_seq, &found_record_for_key,

        &is_blob_index);
    db_impl->ReturnAndCleanupSuperVersion(cfd_, sv);
    if (!s.ok() && !s.IsNotFound()) {
//错误。
      assert(!s.IsBusy());
      return s;
    }
    if (s.IsNotFound()) {
      assert(!found_record_for_key);
      return Status::Busy("Key deleted");
    }
    assert(found_record_for_key);
    assert(is_blob_index);
    if (latest_seq > upper_bound_) {
      return Status::Busy("Key overwritten");
    }
    return s;
  }

  virtual bool AllowWriteBatching() override { return false; }

 private:
  ColumnFamilyData* cfd_;
//检查密钥
  Slice key_;
//要继续的序列号上限。
  SequenceNumber upper_bound_;
};

//按顺序迭代blob并检查blob序列号
//是最新的。如果是最新的，保留它，否则删除它
//如果是基于TTL的，且TTL已过期，则
//如果密钥仍然是最新的，或者密钥不是最新的，我们就可以破坏实体。
//建立
//如果该键已被重写，会发生什么情况？然后我们就可以把那块东西掉了
//如果最早的快照不是
//指序列号，即大于序列号
//新钥匙的钥匙
//
//如果它不是基于TTL的，那么如果密钥
//在LSM中删除
Status BlobDBImpl::GCFileAndUpdateLSM(const std::shared_ptr<BlobFile>& bfptr,
                                      GCStats* gc_stats) {
  uint64_t now = EpochNow();

  std::shared_ptr<Reader> reader =
      bfptr->OpenSequentialReader(env_, db_options_, env_options_);
  if (!reader) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "File sequential reader could not be opened",
                    bfptr->PathName().c_str());
    return Status::IOError("failed to create sequential reader");
  }

  BlobLogHeader header;
  Status s = reader->ReadHeader(&header);
  if (!s.ok()) {
    ROCKS_LOG_ERROR(db_options_.info_log,
                    "Failure to read header for blob-file %s",
                    bfptr->PathName().c_str());
    return s;
  }

  bool first_gc = bfptr->gc_once_after_open_;

  auto* cfh =
      db_impl_->GetColumnFamilyHandleUnlocked(bfptr->column_family_id());
  auto* cfd = reinterpret_cast<ColumnFamilyHandleImpl*>(cfh)->cfd();
  auto column_family_id = cfd->GetID();
  bool has_ttl = header.has_ttl;

//这读取密钥，但跳过了blob
  Reader::ReadLevel shallow = Reader::kReadHeaderKey;

  bool no_relocation_ttl =
      (has_ttl && now >= bfptr->GetExpirationRange().second);

  bool no_relocation_lsmdel = false;
  {
    ReadLock lockbfile_r(&bfptr->mutex_);
    no_relocation_lsmdel =
        (bfptr->GetFileSize() ==
         (BlobLogHeader::kSize + bfptr->deleted_size_ + BlobLogFooter::kSize));
  }

  bool no_relocation = no_relocation_ttl || no_relocation_lsmdel;
  if (!no_relocation) {
//读取blob，因为必须将其写回新文件
    shallow = Reader::kReadHeaderKeyBlob;
  }

  BlobLogRecord record;
  std::shared_ptr<BlobFile> newfile;
  std::shared_ptr<Writer> new_writer;
  uint64_t blob_offset = 0;

  while (true) {
    assert(s.ok());

//读取下一个blob记录。
    Status read_record_status =
        reader->ReadRecord(&record, shallow, &blob_offset);
//如果到达blob文件的结尾，则退出。
//TODO（义乌）：正确处理readrecord错误。
    if (!read_record_status.ok()) {
      break;
    }
    gc_stats->blob_count++;

//与optimatictransaction类似，我们从
//基本db，保证不小于
//当前密钥。我们在写入时使用writeCallback来检查密钥序列
//关于写作。如果键序列大于最新的序列，我们知道
//插入新版本，旧的blob可能会被丢弃。
//
//我们不能使用optimatictransaction，因为我们需要通过
//是getimpl的_blob_index标志。
    SequenceNumber latest_seq = GetLatestSequenceNumber();
    bool is_blob_index = false;
    PinnableSlice index_entry;
    Status get_status = db_impl_->GetImpl(
        /*doptions（），cfh，record.key，&index_entry，nullptr/*找到值\u*/，
        &isou blobou index）；
    测试同步点（“blobdbimpl:：gcfileandupdatelsm:AfterGetFromBaseDB”）；
    如果（！）获取_status.ok（）&！获取_status.isNotFound（））
      /误差
      S＝GETX状态；
      岩石记录错误（数据库选项记录，
                      “获取索引项时出错：%s”，
                      s.toString（）.c_str（））；
      断裂；
    }
    如果（get_status.isNotFound（）！是blob_index）
      //要么删除该键，要么用更新的版本更新该键，
      //在LSM中内联。
      继续；
    }

    blob index blob_索引；
    s=blob_index.decodefrom（index_entry）；
    如果（！）S.O.（））{
      岩石记录错误（数据库选项记录，
                      “解码索引项时出错：%s”，
                      s.toString（）.c_str（））；
      断裂；
    }
    如果（blob_index.file_number（）！=bftr->blobfilenumber（）
        blob_index.offset（）！=斑点偏移量）
      //键已被覆盖。删除blob记录。
      继续；
    }

    garbageCollectionWriteCallback回调（cfd，record.key，最新序列）；

    //如果密钥已过期，请将其从基数据库中删除。
    //TODO（义乌）：Blob索引将由BlobindExCompactionFilter删除。
    //我们可以直接删除blob记录。
    if（no_relocation_ttl（has_ttl&&now>=record.expiration））
      gc_stats->num_deletes++；
      gc_stats->deleted_size+=record.value_size；
      测试同步点（“blobdbimpl:：gcfileandupdatelsm:beforeelete”）；
      writebatch删除\u batch；
      status delete_status=delete_batch.delete（record.key）；
      if（delete_status.ok（））
        delete_status=db_impl_u->writeWithCallback（writeOptions（），
                                                    删除&delete_batch，&callback）；
      }
      if（delete_status.ok（））
        gc_stats->delete_succeeded++；
      else if（delete_status.isbusy（））
        //同时覆盖密钥。删除blob记录。
        gc_stats->overwrited_while_delete++；
      }否则{
        //我们出错了。
        S=删除状态；
        岩石记录错误（数据库选项记录，
                        “删除过期密钥时出错：%s”，
                        s.toString（）.c_str（））；
        断裂；
      }
      //继续下一个blob记录或重试。
      继续；
    }

    如果（FiSTYGGC）{
      //不要为初始GC重新定位blob记录。
      继续；
    }

    //将blob记录重新定位到新文件。
    如果（！）新文件）{
      //新文件
      std：：字符串原因（“gc of”）；
      原因：+=bftr->pathname（）；
      newfile=newblobfile（原因）；
      gc_stats->newfile=newfile；

      new_writer=checkorcreatewriterlocked（newfile）；
      newfile->header_=std:：move（header）；
      //不能在此点之外使用header
      newfile->header_valid_u=true；
      newfile->file_-size_=blobloblogheader:：ksize；
      s=新_-writer->writeheader（newfile->header_u）；

      如果（！）S.O.（））{
        岩石记录错误（数据库选项记录，
                        “文件：%s-头写入失败”，
                        newfile->pathname（）.c_str（））；
        断裂；
      }

      writelock wl（&mutex_uu）；

      目录更改存储（真）；
      blob_files_u.insert（std:：make_pair（newfile->blobfilenumber（），newfile））；
    }

    gc_stats->num_relocate++；
    std：：字符串new_index_entry；

    uint64_t new_blob_offset=0；
    uint64_t new_key_offset=0；
    //将blob写入blob日志。
    s=new_writer->addrecord（record.key，record.value，record.expiration，
                              &new_key_offset，&new_blob_offset）；

    blobindex:：encodeblob（&new_index_entry，newfile->blobfilenumber（），
                          新的“blob”偏移量，record.value.size（），
                          bdb_选项压缩）；

    newfile->blob计数
    newfile->文件大小
        bloblogrecord:：kheadersize+record.key.size（）+record.value.size（）；

    测试同步点（“blobdbimpl:：gcfileandupdatelsm:beforerelocate”）；
    writebatch重写\u batch；
    状态重写\u status=WriteBatchInternal:：PutBloBindex（
        重写&rewrite_batch，column_family_id，record.key，new_index_entry）；
    if（rewrite_status.ok（））
      重写_status=db_impl_u->writeWithCallback（writeOptions（），
                                                   重写&rewrite_batch，&callback）；
    }
    if（rewrite_status.ok（））
      newfile->extendsequencerange（
          writebatchInternal：：序列（&rewrite_batch））；
      gc_stats->relocate_succeeded++；
    else if（rewrite_status.isbusy（））
      //同时覆盖密钥。删除blob记录。
      gc_stats->overwrited_while_relocate++；
    }否则{
      //我们出错了。
      S=重写状态；
      rocks_log_error（db_options_u.info_log，“重新定位密钥时出错：%s”，
                      s.toString（）.c_str（））；
      断裂；
    }
  //readrecord循环结束

  如果（S.O.（））{
    SequenceNumber过时的序列=
        newfile==nullptr？bftr->GetSequenceRange（）。秒+1
                           ：newfile->getSequenceRange（）.second；
    bftr->markobsolete（过时的序列）；
    如果（！）FiSTYGGC）{
      writelock wl（&mutex_uu）；
      过时的_u文件u.push_back（bftr）；
    }
  }

  罗克斯洛格洛夫
      数据库选项信息日志，
      “%s blob file%”priu64
      “。BLOB记录总数：百分比“priu64”，删除百分比“priu64”/%“priu64”
      “成功，重新定位：%”priu64“/%”priu64“成功。”，
      “？”垃圾收集成功”：“垃圾收集失败”，
      bftr->blobfilenumber（），gc_stats->blob_count，gc_stats->delete_succeeded，
      gc_stats->num_deletes，gc_stats->relocate_succeeded，
      gc_stats->num_relocate）；
  如果（新文件！= null pTr）{
    总空间大小
    rocks_log_info（db_options_u info_log，“new blob file%”priu64“.”，
                   newfile->blobfilenumber（））；
  }
  返回S；
}

//理想情况下，我们应该在整个函数期间保持锁，
//但在asusm选项下，只有在
//文件是不可变的，我们可以减少关键部分
bool blobdbimpl:：shouldgcfile（std:：shared_ptr<blobfile>bfile，uint64，现在，
                              bool是最古老的非ttl文件，
                              std:：string*原因）
  if（bfile->hastl（））
    expirationRange expiration_range=bfile->getExpirationRange（）；
    如果（现在>到期时间范围.秒）
      *reason=“整个文件ttl过期”；
      回归真实；
    }

    如果（！）bfile->file_size_.load（））
      rocks_log_error（db_options_u.info_log，“无效文件大小=0%s”，
                      bfile->pathname（）.c_str（））；
      *reason=“文件为空”；
      返回错误；
    }

    if（bfile->gc_once_after_open_u.load（））
      回归真实；
    }

    if（bdb_options_u.ttl_range_secs<kpartializationgcrangesecs）
      *reason=“有TTL但部分过期未打开”；
      返回错误；
    }

    readlock lockbfile_r（&bfile->mutex_）；
    bool ret=（bfile->deleted_size_*100.0/bfile->file_size_.load（））>
                KPartialPirationPercentage）；
    如果（RET）{
      *reason=“删除的斑点超过阈值”；
    }否则{
      *reason=“已删除低于阈值的斑点”；
    }
    返回RET；
  }

  //发生崩溃时，会丢失已删除blob的内存帐户。
  //因此，我们必须执行一个GC以确保删除记帐
  /可以
  if（bfile->gc_once_after_open_u.load（））
    回归真实；
  }

  readlock lockbfile_r（&bfile->mutex_）；

  if（bdb_选项启用_垃圾收集）
    if（（bfile->deleted_size_*100.0/bfile->file_size_.load（））>
        KPartialPirationPercentage）
      *reason=“删除的简单斑点超过阈值”；
      回归真实；
    }
  }

  //如果没有达到磁盘空间限制，不要删除
  if（bdb_options_u.blob_dir_size==0_
      total_blob_space_.load（）<bdb_options_u.blob_dir_size）
    *reason=“未超过磁盘空间”；
    返回错误；
  }

  if（is_oldest_non_ttl_file）
    *reason=“空间不足，是最古老的简单blob文件”；
    回归真实；
  }
  *reason=“空间不足，但不是最旧的简单blob文件”；
  返回错误；
}

std:：pair<bool，int64_t>blobdbimpl:：deleteobosoletefiles（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  {
    readlock rl（&mutex_u）；
    if（obsoleted_files_.empty（））返回std：：make_pair（true，-1）；
  }

  std:：list<std:：shared_ptr<blobfile>>tobsolete；
  {
    writelock wl（&mutex_uu）；
    tobsolete.swap（过时的文件）；
  }

  bool file_deleted=false；
  对于（auto iter=tobsolete.begin（）；iter！=tobsolete.end（）；）
    auto bfile=*iter；
    {
      readlock lockbfile_r（&bfile->mutex_）；
      if（visibletoactivesnapshot（bfile））
        岩石日志信息（数据库选项日志，
                       “由于快照失败，无法删除文件%s”，
                       bfile->pathname（）.c_str（））；
        +ITER；
        继续；
      }
    }
    岩石日志信息（数据库选项日志，
                   “将删除文件，因为快照成功%s”，
                   bfile->pathname（）.c_str（））；

    blob文件擦除（bfile->blobfilenumber（））；
    状态s=env_u->deletefile（bfile->pathname（））；
    如果（！）S.O.（））{
      岩石记录错误（数据库选项记录，
                      “由于过时的%s，无法删除文件”，
                      bfile->pathname（）.c_str（））；
      +ITER；
      继续；
    }

    文件删除=真；
    总空间大小
    岩石日志信息（数据库选项日志，
                   “已从blob目录%s中删除过时的文件”，
                   bfile->pathname（）.c_str（））；

    iter=tobsolete.erase（iter）；
  }

  //目录更改。FSYNC
  如果（文件已删除）
    dir_uent->fsync（）；

    //重置最旧的_文件_收回标志
    最旧的_文件_已移出_u.store（false）；
  }

  //如果由于某种原因删除失败，则将文件放回废弃状态
  如果（！）tobsolete.empty（））
    writelock wl（&mutex_uu）；
    用于（自动文件：Tobsolete）
      过时的“推”文件（bfile）；
    }
  }

  返回std:：make_pair（！流产，-1）；
}

void blobdbimpl:：copyblobfiles（
    std:：vector<std:：shared_ptr<blobfile>>*bfiles_copy，
    std:：function<bool（const std:：shared_ptr<blobfile>&）>谓词）
  readlock rl（&mutex_u）；

  对于（auto const&p:blob_files_
    bool pred_值=真；
    if（谓词）
      pred_值=谓词（p.second）；
    }
    if（pred_值）
      bfiles_copy->push_back（p.second）；
    }
  }
}

void blobdbimpl:：filtersubsetoffiles（）。
    const std:：vector<std:：shared_ptr<blobfile>>&blob_文件，
    std:：vector<std:：shared_ptr<blobfile>>*to_process，uint64_t epoch，
    调整文件大小
  //100.0/15.0=7
  uint64_t next_epoch_increment=static_cast<uint64_t>
      std:：ceil（100/static_cast<double>（kgcfilePercentage））；
  uint64_t now=epochnow（）；

  处理的文件大小=0；
  bool non_ttl_file_found=false；
  对于（auto-bfile:blob_文件）
    if（files_processed>=files_to_collect）中断；
    //如果这是第一次处理文件
    //即gc_epoch=-1，处理它。
    //否则，如果文件的处理epoch匹配，则处理该文件
    //当前时代。典型的时代应该是
    /5-10左右
    如果（bfile->gc_epoch！=-1&（uint64_t）bfile->gc_epoch_uu！{历元）{
      继续；
    }

    文件_processed++；
    //重置epoch
    bfile->gc_epoch_u=epoch+next_epoch_increment；

    //文件已被gc'd或仍在为append打开中，
    //那么它不应该是gc'd
    如果（bfile->obsolete（）！bfile->immutable（））继续；

    bool是\u oldest \u non \u ttl \u file=false；
    如果（！）找到非TTL文件&！bfile->hasttl（））
      是“最旧的”非“TTL”文件=真；
      非TTL文件找到=真；
    }

    std：：字符串原因；
    bool shouldgc=shouldgc file（bfile，现在，是“最旧的”非“ttl”文件，&reason）；
    如果（！）{应} GC）{
      rocks日志调试（db选项日志，
                      “已跳过gc ttl%s%”priu64“%”priu64“的文件
                      “原因＝%s”，
                      bfile->pathname（）.c_str（），现在，
                      bfile->getExpirationRange（）.second，reason.c_str（））；
      继续；
    }

    岩石日志信息（数据库选项日志，
                   “已为GC TTL%s%选择文件”priu64“%”priu64
                   “原因＝%s”，
                   bfile->pathname（）.c_str（），现在，
                   bfile->getExpirationRange（）.second，reason.c_str（））；
    到“进程”--向后推（bfile）；
  }
}

std:：pair<bool，int64_t>blobdbimpl:：rungc（bool已中止）
  如果（中止）返回std：：make_pair（false，-1）；

  当前“时代”+；

  std:：vector<std:：shared_ptr<blobfile>>blob_文件；
  copyblobfiles（&blob_文件）；

  如果（！）blob_files.size（））返回std：：make_pair（true，-1）；

  //每次调用收集15%的文件以释放IO和CPU空间
  / /消耗。
  size_t files_to_collect=（kgcfilePercentage*blob_files.size（））/100；

  std:：vector<std:：shared_ptr<blobfile>>to_process；
  filtersubsetoffiles（blob_files，&to_process，current_epoch_x，）
                      文件收集）；

  for（auto bfile:to_process）
    gc stats gc_状态；
    状态s=gcfileandupdatelsm（bfile，&gc_stats）；
    如果（！）S.O.（））{
      继续；
    }

    if（bfile->gc_once_after_open_u.load（））
      writelock lockbfile_w（&bfile->mutex_）；

      bfile->deleted_size_=gc_stats.deleted_size；
      bfile->deleted_count_u=gc_stats.num_删除；
      bfile->gc_once_after_open_uu=false；
    }
  }

  / /重新安排
  返回std:：make_pair（true，-1）；
}

迭代器*blobdbimpl:：new迭代器（const read options&read_options）
  自动* CFD＝
      reinterpret_cast<columnFamilyhandleImpl*>（defaultColumnFamily（））->cfd（）；
  //获取快照以避免在我们之间删除blob文件
  //从文件中获取和索引条目以及读取。
  managedSnapshot*自己的快照=nullptr；
  const snapshot*snapshot=read_options.snapshot；
  if（快照==nullptr）
    own_snapshot=新建ManagedSnapshot（db_uu）；
    snapshot=own_snapshot->snapshot（）；
  }
  auto*iter=db_impl_u->newIteratorImpl（
      读取“选项”，cfd，snapshot->getSequenceNumber（），
      真/*允许-blo*/);

  return new BlobDBIterator(own_snapshot, iter, this);
}

Status DestroyBlobDB(const std::string& dbname, const Options& options,
                     const BlobDBOptions& bdb_options) {
  const ImmutableDBOptions soptions(SanitizeOptions(dbname, options));
  Env* env = soptions.env;

  Status status;
  std::string blobdir;
  blobdir = (bdb_options.path_relative) ? dbname + "/" + bdb_options.blob_dir
                                        : bdb_options.blob_dir;

  std::vector<std::string> filenames;
  env->GetChildren(blobdir, &filenames);

  for (const auto& f : filenames) {
    uint64_t number;
    FileType type;
    if (ParseFileName(f, &number, &type) && type == kBlobFile) {
      Status del = env->DeleteFile(blobdir + "/" + f);
      if (status.ok() && !del.ok()) {
        status = del;
      }
    }
  }
  env->DeleteDir(blobdir);

  Status destroy = DestroyDB(dbname, options);
  if (status.ok() && !destroy.ok()) {
    status = destroy;
  }

  return status;
}

#ifndef NDEBUG
Status BlobDBImpl::TEST_GetBlobValue(const Slice& key, const Slice& index_entry,
                                     PinnableSlice* value) {
  return GetBlobValue(key, index_entry, value);
}

std::vector<std::shared_ptr<BlobFile>> BlobDBImpl::TEST_GetBlobFiles() const {
  ReadLock l(&mutex_);
  std::vector<std::shared_ptr<BlobFile>> blob_files;
  for (auto& p : blob_files_) {
    blob_files.emplace_back(p.second);
  }
  return blob_files;
}

std::vector<std::shared_ptr<BlobFile>> BlobDBImpl::TEST_GetObsoleteFiles()
    const {
  ReadLock l(&mutex_);
  std::vector<std::shared_ptr<BlobFile>> obsolete_files;
  for (auto& bfile : obsolete_files_) {
    obsolete_files.emplace_back(bfile);
  }
  return obsolete_files;
}

void BlobDBImpl::TEST_DeleteObsoleteFiles() {
  /*eteobsoletefiles（错误/*中止*/）；
}

状态blobdbimpl:：test_closeblobfile（std:：shared_ptr<blobfile>&bfile）
  返回closeblobfile（bfile）；
}

状态blobdbimpl:：test_gcfileandupdatelsm（std:：shared_ptr<blobfile>>&bfile，
                                           gc stats*gc_状态）
  返回gcfileandupdatelsm（bfile，gc_stats）；
}

void blobdbimpl:：test_rungc（）rungc（false/*abor*/); }

#endif  //！调试程序

}  //命名空间blob_db
}  //命名空间rocksdb
#endif  //摇滚乐
