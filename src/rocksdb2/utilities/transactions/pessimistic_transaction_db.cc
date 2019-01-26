
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include "utilities/transactions/pessimistic_transaction_db.h"

#include <inttypes.h>
#include <string>
#include <unordered_set>
#include <vector>

#include "db/db_impl.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/utilities/transaction_db.h"
#include "util/cast_util.h"
#include "util/mutexlock.h"
#include "utilities/transactions/pessimistic_transaction.h"
#include "utilities/transactions/transaction_db_mutex_impl.h"

namespace rocksdb {

PessimisticTransactionDB::PessimisticTransactionDB(
    DB* db, const TransactionDBOptions& txn_db_options)
    : TransactionDB(db),
      db_impl_(static_cast_with_check<DBImpl, DB>(db)),
      txn_db_options_(txn_db_options),
      lock_mgr_(this, txn_db_options_.num_stripes, txn_db_options.max_num_locks,
                txn_db_options_.max_num_deadlocks,
                txn_db_options_.custom_mutex_factory
                    ? txn_db_options_.custom_mutex_factory
                    : std::shared_ptr<TransactionDBMutexFactory>(
                          new TransactionDBMutexFactoryImpl())) {
  assert(db_impl_ != nullptr);
  info_log_ = db_impl_->GetDBOptions().info_log;
}

//支持从可堆叠数据库初始化悲观事务数据库
//
//悲观交易数据库
//^ ^
//γ
//+
//斯塔克贝莱德
//^ ^
//γ
//++
//溴化十六烷基吡啶
//^
//（继承）
//+
//分贝
//
PessimisticTransactionDB::PessimisticTransactionDB(
    StackableDB* db, const TransactionDBOptions& txn_db_options)
    : TransactionDB(db),
      db_impl_(static_cast_with_check<DBImpl, DB>(db->GetRootDB())),
      txn_db_options_(txn_db_options),
      lock_mgr_(this, txn_db_options_.num_stripes, txn_db_options.max_num_locks,
                txn_db_options_.max_num_deadlocks,
                txn_db_options_.custom_mutex_factory
                    ? txn_db_options_.custom_mutex_factory
                    : std::shared_ptr<TransactionDBMutexFactory>(
                          new TransactionDBMutexFactoryImpl())) {
  assert(db_impl_ != nullptr);
}

PessimisticTransactionDB::~PessimisticTransactionDB() {
  while (!transactions_.empty()) {
    delete transactions_.begin()->second;
  }
}

Status PessimisticTransactionDB::Initialize(
    const std::vector<size_t>& compaction_enabled_cf_indices,
    const std::vector<ColumnFamilyHandle*>& handles) {
  for (auto cf_ptr : handles) {
    AddColumnFamily(cf_ptr);
  }
//重新启用最初具有的柱族的压缩
//压缩已启用。
  std::vector<ColumnFamilyHandle*> compaction_enabled_cf_handles;
  compaction_enabled_cf_handles.reserve(compaction_enabled_cf_indices.size());
  for (auto index : compaction_enabled_cf_indices) {
    compaction_enabled_cf_handles.push_back(handles[index]);
  }

  Status s = EnableAutoCompaction(compaction_enabled_cf_handles);

//从恢复的shell事务创建“真实”事务
  auto dbimpl = reinterpret_cast<DBImpl*>(GetRootDB());
  assert(dbimpl != nullptr);
  auto rtrxs = dbimpl->recovered_transactions();

  for (auto it = rtrxs.begin(); it != rtrxs.end(); it++) {
    auto recovered_trx = it->second;
    assert(recovered_trx);
    assert(recovered_trx->log_number_);
    assert(recovered_trx->name_.length());

    WriteOptions w_options;
    w_options.sync = true;
    TransactionOptions t_options;

    Transaction* real_trx = BeginTransaction(w_options, t_options, nullptr);
    assert(real_trx);
    real_trx->SetLogNumber(recovered_trx->log_number_);

    s = real_trx->SetName(recovered_trx->name_);
    if (!s.ok()) {
      break;
    }

    s = real_trx->RebuildFromWriteBatch(recovered_trx->batch_);
    real_trx->SetState(Transaction::PREPARED);
    if (!s.ok()) {
      break;
    }
  }
  if (s.ok()) {
    dbimpl->DeleteAllRecoveredTransactions();
  }
  return s;
}

Transaction* WriteCommittedTxnDB::BeginTransaction(
    const WriteOptions& write_options, const TransactionOptions& txn_options,
    Transaction* old_txn) {
  if (old_txn != nullptr) {
    ReinitializeTransaction(old_txn, write_options, txn_options);
    return old_txn;
  } else {
    return new WriteCommittedTxn(this, write_options, txn_options);
  }
}

Transaction* WritePreparedTxnDB::BeginTransaction(
    const WriteOptions& write_options, const TransactionOptions& txn_options,
    Transaction* old_txn) {
  if (old_txn != nullptr) {
    ReinitializeTransaction(old_txn, write_options, txn_options);
    return old_txn;
  } else {
    return new WritePreparedTxn(this, write_options, txn_options);
  }
}

TransactionDBOptions PessimisticTransactionDB::ValidateTxnDBOptions(
    const TransactionDBOptions& txn_db_options) {
  TransactionDBOptions validated = txn_db_options;

  if (txn_db_options.num_stripes == 0) {
    validated.num_stripes = 1;
  }

  return validated;
}

Status TransactionDB::Open(const Options& options,
                           const TransactionDBOptions& txn_db_options,
                           const std::string& dbname, TransactionDB** dbptr) {
  DBOptions db_options(options);
  ColumnFamilyOptions cf_options(options);
  std::vector<ColumnFamilyDescriptor> column_families;
  column_families.push_back(
      ColumnFamilyDescriptor(kDefaultColumnFamilyName, cf_options));
  std::vector<ColumnFamilyHandle*> handles;
  Status s = TransactionDB::Open(db_options, txn_db_options, dbname,
                                 column_families, &handles, dbptr);
  if (s.ok()) {
    assert(handles.size() == 1);
//我可以删除句柄，因为dbimpl总是保存对
//默认列族
    delete handles[0];
  }

  return s;
}

Status TransactionDB::Open(
    const DBOptions& db_options, const TransactionDBOptions& txn_db_options,
    const std::string& dbname,
    const std::vector<ColumnFamilyDescriptor>& column_families,
    std::vector<ColumnFamilyHandle*>* handles, TransactionDB** dbptr) {
  Status s;
  DB* db;

  std::vector<ColumnFamilyDescriptor> column_families_copy = column_families;
  std::vector<size_t> compaction_enabled_cf_indices;
  DBOptions db_options_2pc = db_options;
  PrepareWrap(&db_options_2pc, &column_families_copy,
              &compaction_enabled_cf_indices);
  s = DB::Open(db_options_2pc, dbname, column_families_copy, handles, &db);
  if (s.ok()) {
    s = WrapDB(db, txn_db_options, compaction_enabled_cf_indices, *handles,
               dbptr);
  }
  return s;
}

void TransactionDB::PrepareWrap(
    DBOptions* db_options, std::vector<ColumnFamilyDescriptor>* column_families,
    std::vector<size_t>* compaction_enabled_cf_indices) {
  compaction_enabled_cf_indices->clear();

//启用memtable历史记录（如果尚未启用）
  for (size_t i = 0; i < column_families->size(); i++) {
    ColumnFamilyOptions* cf_options = &(*column_families)[i].options;

    if (cf_options->max_write_buffer_number_to_maintain == 0) {
//设置为-1会将历史大小设置为max_write_buffer_number。
      cf_options->max_write_buffer_number_to_maintain = -1;
    }
    if (!cf_options->disable_auto_compactions) {
//立即禁用压缩以防止与db:：open争用
      cf_options->disable_auto_compactions = true;
      compaction_enabled_cf_indices->push_back(i);
    }
  }
  db_options->allow_2pc = true;
}

Status TransactionDB::WrapDB(
//确保此数据库已在启用memtable历史的情况下打开，
//已禁用自动压缩并启用2阶段提交
    DB* db, const TransactionDBOptions& txn_db_options,
    const std::vector<size_t>& compaction_enabled_cf_indices,
    const std::vector<ColumnFamilyHandle*>& handles, TransactionDB** dbptr) {
  PessimisticTransactionDB* txn_db;
  switch (txn_db_options.write_policy) {
    case WRITE_UNPREPARED:
      return Status::NotSupported("WRITE_UNPREPARED is not implemented yet");
    case WRITE_PREPARED:
      txn_db = new WritePreparedTxnDB(
          db, PessimisticTransactionDB::ValidateTxnDBOptions(txn_db_options));
      break;
    case WRITE_COMMITTED:
    default:
      txn_db = new WriteCommittedTxnDB(
          db, PessimisticTransactionDB::ValidateTxnDBOptions(txn_db_options));
  }
  *dbptr = txn_db;
  Status s = txn_db->Initialize(compaction_enabled_cf_indices, handles);
  return s;
}

Status TransactionDB::WrapStackableDB(
//确保此可堆叠数据库已用memtable history打开
//启用，
//已禁用自动压缩并启用2阶段提交
    StackableDB* db, const TransactionDBOptions& txn_db_options,
    const std::vector<size_t>& compaction_enabled_cf_indices,
    const std::vector<ColumnFamilyHandle*>& handles, TransactionDB** dbptr) {
  PessimisticTransactionDB* txn_db;
  switch (txn_db_options.write_policy) {
    case WRITE_UNPREPARED:
      return Status::NotSupported("WRITE_UNPREPARED is not implemented yet");
    case WRITE_PREPARED:
      txn_db = new WritePreparedTxnDB(
          db, PessimisticTransactionDB::ValidateTxnDBOptions(txn_db_options));
      break;
    case WRITE_COMMITTED:
    default:
      txn_db = new WriteCommittedTxnDB(
          db, PessimisticTransactionDB::ValidateTxnDBOptions(txn_db_options));
  }
  *dbptr = txn_db;
  Status s = txn_db->Initialize(compaction_enabled_cf_indices, handles);
  return s;
}

//让TransactionLockMgr知道此列族存在，以便它可以
//为它分配一个锁映射。
void PessimisticTransactionDB::AddColumnFamily(
    const ColumnFamilyHandle* handle) {
  lock_mgr_.AddColumnFamily(handle->GetID());
}

Status PessimisticTransactionDB::CreateColumnFamily(
    const ColumnFamilyOptions& options, const std::string& column_family_name,
    ColumnFamilyHandle** handle) {
  InstrumentedMutexLock l(&column_family_mutex_);

  Status s = db_->CreateColumnFamily(options, column_family_name, handle);
  if (s.ok()) {
    lock_mgr_.AddColumnFamily((*handle)->GetID());
  }

  return s;
}

//让TransactionLockMgr知道它可以为此解除锁定映射
//列族。
Status PessimisticTransactionDB::DropColumnFamily(
    ColumnFamilyHandle* column_family) {
  InstrumentedMutexLock l(&column_family_mutex_);

  Status s = db_->DropColumnFamily(column_family);
  if (s.ok()) {
    lock_mgr_.RemoveColumnFamily(column_family->GetID());
  }

  return s;
}

Status PessimisticTransactionDB::TryLock(PessimisticTransaction* txn,
                                         uint32_t cfh_id,
                                         const std::string& key,
                                         bool exclusive) {
  return lock_mgr_.TryLock(txn, cfh_id, key, GetEnv(), exclusive);
}

void PessimisticTransactionDB::UnLock(PessimisticTransaction* txn,
                                      const TransactionKeyMap* keys) {
  lock_mgr_.UnLock(txn, keys, GetEnv());
}

void PessimisticTransactionDB::UnLock(PessimisticTransaction* txn,
                                      uint32_t cfh_id, const std::string& key) {
  lock_mgr_.UnLock(txn, cfh_id, key, GetEnv());
}

//在事务中包装数据库写入操作时使用
Transaction* PessimisticTransactionDB::BeginInternalTransaction(
    const WriteOptions& options) {
  TransactionOptions txn_options;
  Transaction* txn = BeginTransaction(options, txn_options, nullptr);

//对非事务性写入使用默认超时
  txn->SetLockTimeout(txn_db_options_.default_lock_timeout);
  return txn;
}

//必须拦截所有用户的Put、Merge、Delete和Write请求，才能
//确保它们锁定正在写入的所有密钥，以避免引起冲突
//任何并发交易。最简单的方法是把所有
//在事务中写入操作。
//
//Put（）、Merge（）和Delete（）每次调用只锁定一个键。Wrar（）
//把钥匙分类后再锁上。这保证了TransactionDB写入
//方法不能与其他方法死锁（但仍然可以使用
//事务处理）。
Status PessimisticTransactionDB::Put(const WriteOptions& options,
                                     ColumnFamilyHandle* column_family,
                                     const Slice& key, const Slice& val) {
  Status s;

  Transaction* txn = BeginInternalTransaction(options);
  txn->DisableIndexing();

//因为客户机没有创建事务，所以他们不关心
//此写入的冲突检查。所以我们只需要做putunttracked（）。
  s = txn->PutUntracked(column_family, key, val);

  if (s.ok()) {
    s = txn->Commit();
  }

  delete txn;

  return s;
}

Status PessimisticTransactionDB::Delete(const WriteOptions& wopts,
                                        ColumnFamilyHandle* column_family,
                                        const Slice& key) {
  Status s;

  Transaction* txn = BeginInternalTransaction(wopts);
  txn->DisableIndexing();

//因为客户机没有创建事务，所以他们不关心
//此写入的冲突检查。所以我们只需要做
//删除未跟踪（）。
  s = txn->DeleteUntracked(column_family, key);

  if (s.ok()) {
    s = txn->Commit();
  }

  delete txn;

  return s;
}

Status PessimisticTransactionDB::Merge(const WriteOptions& options,
                                       ColumnFamilyHandle* column_family,
                                       const Slice& key, const Slice& value) {
  Status s;

  Transaction* txn = BeginInternalTransaction(options);
  txn->DisableIndexing();

//因为客户机没有创建事务，所以他们不关心
//此写入的冲突检查。所以我们只需要做
//mergeuntracked（）。
  s = txn->MergeUntracked(column_family, key, value);

  if (s.ok()) {
    s = txn->Commit();
  }

  delete txn;

  return s;
}

Status PessimisticTransactionDB::Write(const WriteOptions& opts,
                                       WriteBatch* updates) {
//需要锁定此批处理中的所有密钥，以防止与
//同时进行的交易。
  Transaction* txn = BeginInternalTransaction(opts);
  txn->DisableIndexing();

  auto txn_impl =
      static_cast_with_check<PessimisticTransaction, Transaction>(txn);

//因为commitbatch在锁定之前对密钥进行排序，所以并发写入（）。
//操作不会导致死锁。
//为了避免并发事务出现死锁，事务
//应该使用锁定超时。
  Status s = txn_impl->CommitBatch(updates);

  delete txn;

  return s;
}

void PessimisticTransactionDB::InsertExpirableTransaction(
    TransactionID tx_id, PessimisticTransaction* tx) {
  assert(tx->GetExpirationTime() > 0);
  std::lock_guard<std::mutex> lock(map_mutex_);
  expirable_transactions_map_.insert({tx_id, tx});
}

void PessimisticTransactionDB::RemoveExpirableTransaction(TransactionID tx_id) {
  std::lock_guard<std::mutex> lock(map_mutex_);
  expirable_transactions_map_.erase(tx_id);
}

bool PessimisticTransactionDB::TryStealingExpiredTransactionLocks(
    TransactionID tx_id) {
  std::lock_guard<std::mutex> lock(map_mutex_);

  auto tx_it = expirable_transactions_map_.find(tx_id);
  if (tx_it == expirable_transactions_map_.end()) {
    return true;
  }
  PessimisticTransaction& tx = *(tx_it->second);
  return tx.TryStealingLocks();
}

void PessimisticTransactionDB::ReinitializeTransaction(
    Transaction* txn, const WriteOptions& write_options,
    const TransactionOptions& txn_options) {
  auto txn_impl =
      static_cast_with_check<PessimisticTransaction, Transaction>(txn);

  txn_impl->Reinitialize(this, write_options, txn_options);
}

Transaction* PessimisticTransactionDB::GetTransactionByName(
    const TransactionName& name) {
  std::lock_guard<std::mutex> lock(name_map_mutex_);
  auto it = transactions_.find(name);
  if (it == transactions_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

void PessimisticTransactionDB::GetAllPreparedTransactions(
    std::vector<Transaction*>* transv) {
  assert(transv);
  transv->clear();
  std::lock_guard<std::mutex> lock(name_map_mutex_);
  for (auto it = transactions_.begin(); it != transactions_.end(); it++) {
    if (it->second->GetState() == Transaction::PREPARED) {
      transv->push_back(it->second);
    }
  }
}

TransactionLockMgr::LockStatusData
PessimisticTransactionDB::GetLockStatusData() {
  return lock_mgr_.GetLockStatusData();
}

std::vector<DeadlockPath> PessimisticTransactionDB::GetDeadlockInfoBuffer() {
  return lock_mgr_.GetDeadlockInfoBuffer();
}

void PessimisticTransactionDB::SetDeadlockInfoBufferSize(uint32_t target_size) {
  lock_mgr_.Resize(target_size);
}

void PessimisticTransactionDB::RegisterTransaction(Transaction* txn) {
  assert(txn);
  assert(txn->GetName().length() > 0);
  assert(GetTransactionByName(txn->GetName()) == nullptr);
  assert(txn->GetState() == Transaction::STARTED);
  std::lock_guard<std::mutex> lock(name_map_mutex_);
  transactions_[txn->GetName()] = txn;
}

void PessimisticTransactionDB::UnregisterTransaction(Transaction* txn) {
  assert(txn);
  std::lock_guard<std::mutex> lock(name_map_mutex_);
  auto it = transactions_.find(txn->GetName());
  assert(it != transactions_.end());
  transactions_.erase(it);
}

//如果commit_seq<=snapshot_seq，则返回true
bool WritePreparedTxnDB::IsInSnapshot(uint64_t prep_seq,
                                      uint64_t snapshot_seq) {
//在这里，我们尝试推断返回值而不查看准备列表。
//这将有助于避免在共享映射上同步。
//托多：读你自己的文章
//托多：优化这个。这一检查顺序必须正确，但
//不需要效率
  if (snapshot_seq < prep_seq) {
//快照顺序<准备顺序<=提交顺序=>快照顺序<提交顺序
    return false;
  }
  if (!delayed_prepared_empty_.load(std::memory_order_acquire)) {
//我们通常不应该到达这里
    ReadLock rl(&prepared_mutex_);
    if (delayed_prepared_.find(prep_seq) != delayed_prepared_.end()) {
//那么它还没有承诺
      return false;
    }
  }
  auto indexed_seq = prep_seq % COMMIT_CACHE_SIZE;
  CommitEntry cached;
  bool exist = GetCommitEntry(indexed_seq, &cached);
  if (!exist) {
//它没有承诺，所以必须做好准备
    return false;
  }
  if (prep_seq == cached.prep_seq) {
//它已提交，也未从提交缓存中收回
    return cached.commit_seq <= snapshot_seq;
  }
//在这一点上，我们不知道这是承诺还是准备好了
  auto max_evicted_seq = max_evicted_seq_.load(std::memory_order_acquire);
  if (max_evicted_seq < prep_seq) {
//未从缓存中移出也不存在，因此必须继续准备
    return false;
  }
//当前进时，马克斯退出了，我们将旧的实体从准备移动到
//延迟准备。此外，我们还将收回的条目从提交缓存移动到
//旧的提交映射如果与任何快照重叠。自准备工作开始
//Max_驱逐了_Seq_u，我们有三个案例：i）延迟的_Prepared_u，i i）在
//旧的提交与任何快照没有冲突（i）
//上面检查了延迟准备
if (max_evicted_seq < snapshot_seq) {  //那么（ii）就不是这样了
//只有（iii）是这样的：承诺
//提交顺序<=max
//快照
    return true;
  }
//否则（ii）可能是这样：检查为此快照保存的提交数据。
//如果没有重叠的提交项，则使用
//提交低于任何实时快照的序列，包括快照序列。
  if (old_commit_map_empty_.load(std::memory_order_acquire)) {
    return true;
  }
  {
//我们通常不应该到达这里
    ReadLock rl(&old_commit_map_mutex_);
    auto old_commit_entry = old_commit_map_.find(prep_seq);
    if (old_commit_entry == old_commit_map_.end() ||
        old_commit_entry->second <= snapshot_seq) {
      return true;
    }
  }
//（ii）情况：已提交，但在快照后
  return false;
}

void WritePreparedTxnDB::AddPrepared(uint64_t seq) {
  ROCKS_LOG_DEBUG(info_log_, "Txn %" PRIu64 " Prepareing", seq);
  WriteLock wl(&prepared_mutex_);
  prepared_txns_.push(seq);
}

void WritePreparedTxnDB::AddCommitted(uint64_t prepare_seq,
                                      uint64_t commit_seq) {
  ROCKS_LOG_DEBUG(info_log_, "Txn %" PRIu64 " Committing with %" PRIu64,
                  prepare_seq, commit_seq);
  auto indexed_seq = prepare_seq % COMMIT_CACHE_SIZE;
  CommitEntry evicted;
  bool to_be_evicted = GetCommitEntry(indexed_seq, &evicted);
  if (to_be_evicted) {
    auto prev_max = max_evicted_seq_.load(std::memory_order_acquire);
    if (prev_max < evicted.commit_seq) {
//todo（myabandeh）inc max以更大的步骤避免频繁更新
      auto max_evicted_seq = evicted.commit_seq;
//当max_逐出_seq_uu advances时，从准备好的_txns_中移动较旧的条目
//推迟准备。这保证了如果seq小于max，
//那么它就不在准备之中了，省去了昂贵的，同步的
//从共享集进行查找。延迟的“准备就绪”预计在
//正常病例。
      {
        WriteLock wl(&prepared_mutex_);
        while (!prepared_txns_.empty() &&
               prepared_txns_.top() <= max_evicted_seq) {
          auto to_be_popped = prepared_txns_.top();
          delayed_prepared_.insert(to_be_popped);
          prepared_txns_.pop();
          delayed_prepared_empty_.store(false, std::memory_order_release);
        }
      }

//每次更改为max时，都会获取其后面的实时快照
      SequenceNumber curr_seq;
      std::vector<SequenceNumber> all_snapshots;
      bool update_snapshots = false;
      {
        InstrumentedMutex(db_impl_->mutex());
//我们使用这个来确定快照列表的新鲜度。既然这样
//自动完成获取快照列表，其中
//较大的seq更新鲜。如果seq等于完整快照
//列表可能不同，因为拍摄快照不会增加
//DB SEQ。但是，因为我们只关心快照
//新的max，此类最近的快照将不包括在
//不管怎么说。
        curr_seq = db_impl_->GetLatestSequenceNumber();
        if (curr_seq > snapshots_version_) {
//这是为了避免更新快照，如果快照已经更新
//通过当前线程使用更新的vesion
          update_snapshots = true;
//我们只关心快照低于max
          all_snapshots =
              db_impl_->snapshots().GetAll(nullptr, max_evicted_seq);
        }
      }
      if (update_snapshots) {
        WriteLock wl(&snapshots_mutex_);
        snapshots_version_ = curr_seq;
//我们与读者同时更新列表。
//对新列表和旧列表进行排序，新列表是
//上一个列表加上一些新项目。因此，如果快照在
//新列表和旧列表都将显示在新列表的上方。所以如果
//如果一个覆盖项
//在新列表中仍然有效，或者写入到相同的位置
//数组或它在被
//被另一项覆盖。这保证了一个阅读
//列表自下而上将始终看到在
//在被写入程序覆盖之前进行更新，或者
//之后。
        size_t i = 0;
        auto it = all_snapshots.begin();
        for (; it != all_snapshots.end() && i < SNAPSHOT_CACHE_SIZE;
             it++, i++) {
          snapshot_cache_[i].store(*it, std::memory_order_release);
        }
        snapshots_.clear();
        for (; it != all_snapshots.end(); it++) {
//将它们插入到访问效率较低的向量中
//同时地
          snapshots_.push_back(*it);
        }
//更新末尾的大小。否则并行阅读器可能会读取
//尚未设置的项。
        snapshots_total_.store(all_snapshots.size(), std::memory_order_release);
      }
      while (prev_max < max_evicted_seq &&
             !max_evicted_seq_.compare_exchange_weak(
                 prev_max, max_evicted_seq, std::memory_order_release,
                 std::memory_order_acquire)) {
      };
    }
//每次从提交缓存中逐出后，检查提交项是否应
//保持在周围，因为它与实时快照重叠。
//首先检查对并发访问有效的快照缓存
    auto cnt = snapshots_total_.load(std::memory_order_acquire);
//当我们从列表中读取时，该列表可能会同时更新。这个
//读卡器应该能够读取所有仍然有效的快照。
//更新后。因为幸存的快照是用更高的
//在被覆盖之前放置自下而上读取的读卡器将
//终于看到了。
    const bool next_is_larger = true;
    SequenceNumber snapshot_seq = kMaxSequenceNumber;
    size_t ip1 = std::min(cnt, SNAPSHOT_CACHE_SIZE);
    for (; 0 < ip1; ip1--) {
      snapshot_seq = snapshot_cache_[ip1 - 1].load(std::memory_order_acquire);
      if (!MaybeUpdateOldCommitMap(evicted.prep_seq, evicted.commit_seq,
                                   snapshot_seq, !next_is_larger)) {
        break;
      }
    }
    if (UNLIKELY(SNAPSHOT_CACHE_SIZE < cnt && ip1 == SNAPSHOT_CACHE_SIZE &&
                 snapshot_seq < evicted.prep_seq)) {
//然后访问效率较低的快照列表
      ReadLock rl(&snapshots_mutex_);
//项目可能已经从快照移动到快照缓存
//打开门锁。为了确保我们不会错过有效的快照，
//持有锁时再次读取快照缓存。
      for (size_t i = 0; i < SNAPSHOT_CACHE_SIZE; i++) {
        snapshot_seq = snapshot_cache_[i].load(std::memory_order_acquire);
        if (!MaybeUpdateOldCommitMap(evicted.prep_seq, evicted.commit_seq,
                                     snapshot_seq, next_is_larger)) {
          break;
        }
      }
      for (auto snapshot_seq_2 : snapshots_) {
        if (!MaybeUpdateOldCommitMap(evicted.prep_seq, evicted.commit_seq,
                                     snapshot_seq_2, next_is_larger)) {
          break;
        }
      }
    }
  }
  bool succ =
      ExchangeCommitEntry(indexed_seq, evicted, {prepare_seq, commit_seq});
  if (!succ) {
//一个非常罕见的事件，在这个事件中，提交项在我们之前被更新。
//这里我们应用一个非常简单的重试解决方案。
//TODO（Myabandeh）：采取预防措施检测导致无限循环的错误
    AddCommitted(prepare_seq, commit_seq);
    return;
  }
  {
    WriteLock wl(&prepared_mutex_);
    prepared_txns_.erase(prepare_seq);
    bool was_empty = delayed_prepared_.empty();
    if (!was_empty) {
      delayed_prepared_.erase(prepare_seq);
      bool is_empty = delayed_prepared_.empty();
      if (was_empty != is_empty) {
        delayed_prepared_empty_.store(is_empty, std::memory_order_release);
      }
    }
  }
}

bool WritePreparedTxnDB::GetCommitEntry(uint64_t indexed_seq,
                                        CommitEntry* entry) {
//TODO（myabandeh）：实现无锁提交缓存
  ReadLock rl(&commit_cache_mutex_);
  *entry = commit_cache_[indexed_seq];
return (entry->commit_seq != 0);  //初始化
}

bool WritePreparedTxnDB::AddCommitEntry(uint64_t indexed_seq,
                                        CommitEntry& new_entry,
                                        CommitEntry* evicted_entry) {
//TODO（myabandeh）：实现无锁提交缓存
  WriteLock wl(&commit_cache_mutex_);
  *evicted_entry = commit_cache_[indexed_seq];
  commit_cache_[indexed_seq] = new_entry;
return (evicted_entry->commit_seq != 0);  //初始化
}

bool WritePreparedTxnDB::ExchangeCommitEntry(uint64_t indexed_seq,
                                             CommitEntry& expected_entry,
                                             CommitEntry new_entry) {
//TODO（myabandeh）：实现无锁提交缓存
  WriteLock wl(&commit_cache_mutex_);
  auto& evicted_entry = commit_cache_[indexed_seq];
  if (evicted_entry.prep_seq != expected_entry.prep_seq) {
    return false;
  }
  commit_cache_[indexed_seq] = new_entry;
  return true;
}

//10M入口，80MB尺寸
size_t WritePreparedTxnDB::DEF_COMMIT_CACHE_SIZE = static_cast<size_t>(1 << 21);
size_t WritePreparedTxnDB::DEF_SNAPSHOT_CACHE_SIZE =
    static_cast<size_t>(1 << 7);

bool WritePreparedTxnDB::MaybeUpdateOldCommitMap(
    const uint64_t& prep_seq, const uint64_t& commit_seq,
    const uint64_t& snapshot_seq, const bool next_is_larger = true) {
//如果我们不在旧的提交映射中存储一个条目，我们假设它是在
//所有快照。如果commit_seq<=snapshot_seq，则认为它已经在
//快照，因此我们不需要保留此快照的条目。
  if (commit_seq <= snapshot_seq) {
//如果下一个快照可能小于commit序列，则继续搜索
    return !next_is_larger;
  }
//然后快照
if (prep_seq <= snapshot_seq) {  //重叠范围
    WriteLock wl(&old_commit_map_mutex_);
    old_commit_map_empty_.store(false, std::memory_order_release);
    old_commit_map_[prep_seq] = commit_seq;
//储存一次就足够了。无需检查其他快照。
    return false;
  }
//如果下一个快照可能大于Prep序列，则继续搜索
  return next_is_larger;
}

}  //命名空间rocksdb
#endif  //摇滚乐
