
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

#include "utilities/transactions/pessimistic_transaction.h"

#include <map>
#include <set>
#include <string>
#include <vector>

#include "db/column_family.h"
#include "db/db_impl.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/snapshot.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"
#include "util/cast_util.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "utilities/transactions/pessimistic_transaction_db.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

struct WriteOptions;

std::atomic<TransactionID> PessimisticTransaction::txn_id_counter_(1);

TransactionID PessimisticTransaction::GenTxnID() {
  return txn_id_counter_.fetch_add(1);
}

PessimisticTransaction::PessimisticTransaction(
    TransactionDB* txn_db, const WriteOptions& write_options,
    const TransactionOptions& txn_options)
    : TransactionBaseImpl(txn_db->GetRootDB(), write_options),
      txn_db_impl_(nullptr),
      expiration_time_(0),
      txn_id_(0),
      waiting_cf_id_(0),
      waiting_key_(nullptr),
      lock_timeout_(0),
      deadlock_detect_(false),
      deadlock_detect_depth_(0) {
  txn_db_impl_ =
      static_cast_with_check<PessimisticTransactionDB, TransactionDB>(txn_db);
  db_impl_ = static_cast_with_check<DBImpl, DB>(db_);
  Initialize(txn_options);
}

void PessimisticTransaction::Initialize(const TransactionOptions& txn_options) {
  txn_id_ = GenTxnID();

  txn_state_ = STARTED;

  deadlock_detect_ = txn_options.deadlock_detect;
  deadlock_detect_depth_ = txn_options.deadlock_detect_depth;
  write_batch_.SetMaxBytes(txn_options.max_write_batch_size);

  lock_timeout_ = txn_options.lock_timeout * 1000;
  if (lock_timeout_ < 0) {
//未设置锁定超时，使用默认值
    lock_timeout_ =
        txn_db_impl_->GetTxnDBOptions().transaction_lock_timeout * 1000;
  }

  if (txn_options.expiration >= 0) {
    expiration_time_ = start_time_ + txn_options.expiration * 1000;
  } else {
    expiration_time_ = 0;
  }

  if (txn_options.set_snapshot) {
    SetSnapshot();
  }

  if (expiration_time_ > 0) {
    txn_db_impl_->InsertExpirableTransaction(txn_id_, this);
  }
}

PessimisticTransaction::~PessimisticTransaction() {
  txn_db_impl_->UnLock(this, &GetTrackedKeys());
  if (expiration_time_ > 0) {
    txn_db_impl_->RemoveExpirableTransaction(txn_id_);
  }
  if (!name_.empty() && txn_state_ != COMMITED) {
    txn_db_impl_->UnregisterTransaction(this);
  }
}

void PessimisticTransaction::Clear() {
  txn_db_impl_->UnLock(this, &GetTrackedKeys());
  TransactionBaseImpl::Clear();
}

void PessimisticTransaction::Reinitialize(
    TransactionDB* txn_db, const WriteOptions& write_options,
    const TransactionOptions& txn_options) {
  if (!name_.empty() && txn_state_ != COMMITED) {
    txn_db_impl_->UnregisterTransaction(this);
  }
  TransactionBaseImpl::Reinitialize(txn_db->GetRootDB(), write_options);
  Initialize(txn_options);
}

bool PessimisticTransaction::IsExpired() const {
  if (expiration_time_ > 0) {
    if (db_->GetEnv()->NowMicros() >= expiration_time_) {
//事务已过期。
      return true;
    }
  }

  return false;
}

WriteCommittedTxn::WriteCommittedTxn(TransactionDB* txn_db,
                                     const WriteOptions& write_options,
                                     const TransactionOptions& txn_options)
    : PessimisticTransaction(txn_db, write_options, txn_options){};

Status WriteCommittedTxn::CommitBatch(WriteBatch* batch) {
  TransactionKeyMap keys_to_unlock;
  Status s = LockBatch(batch, &keys_to_unlock);

  if (!s.ok()) {
    return s;
  }

  bool can_commit = false;

  if (IsExpired()) {
    s = Status::Expired();
  } else if (expiration_time_ > 0) {
    TransactionState expected = STARTED;
    can_commit = std::atomic_compare_exchange_strong(&txn_state_, &expected,
                                                     AWAITING_COMMIT);
  } else if (txn_state_ == STARTED) {
//偷锁不是问题
    can_commit = true;
  }

  if (can_commit) {
    txn_state_.store(AWAITING_COMMIT);
    s = db_->Write(write_options_, batch);
    if (s.ok()) {
      txn_state_.store(COMMITED);
    }
  } else if (txn_state_ == LOCKS_STOLEN) {
    s = Status::Expired();
  } else {
    s = Status::InvalidArgument("Transaction is not in state for commit.");
  }

  txn_db_impl_->UnLock(this, &keys_to_unlock);

  return s;
}

Status PessimisticTransaction::Prepare() {
  Status s;

  if (name_.empty()) {
    return Status::InvalidArgument(
        "Cannot prepare a transaction that has not been named.");
  }

  if (IsExpired()) {
    return Status::Expired();
  }

  bool can_prepare = false;

  if (expiration_time_ > 0) {
//必须关注过期和/或偷锁
//需要比较/交换BC锁可能在我们这里被盗
    TransactionState expected = STARTED;
    can_prepare = std::atomic_compare_exchange_strong(&txn_state_, &expected,
                                                      AWAITING_PREPARE);
  } else if (txn_state_ == STARTED) {
//到期和锁窃取是不可能的
    can_prepare = true;
  }

  if (can_prepare) {
    txn_state_.store(AWAITING_PREPARE);
//事务处理在准备后不能过期
    expiration_time_ = 0;
    s = PrepareInternal();
    if (s.ok()) {
      assert(log_number_ != 0);
      dbimpl_->MarkLogAsContainingPrepSection(log_number_);
      txn_state_.store(PREPARED);
    }
  } else if (txn_state_ == LOCKS_STOLEN) {
    s = Status::Expired();
  } else if (txn_state_ == PREPARED) {
    s = Status::InvalidArgument("Transaction has already been prepared.");
  } else if (txn_state_ == COMMITED) {
    s = Status::InvalidArgument("Transaction has already been committed.");
  } else if (txn_state_ == ROLLEDBACK) {
    s = Status::InvalidArgument("Transaction has already been rolledback.");
  } else {
    s = Status::InvalidArgument("Transaction is not in state for commit.");
  }

  return s;
}

Status WriteCommittedTxn::PrepareInternal() {
  WriteOptions write_options = write_options_;
  write_options.disableWAL = false;
  WriteBatchInternal::MarkEndPrepare(GetWriteBatch()->GetWriteBatch(), name_);
  Status s =
      db_impl_->WriteImpl(write_options, GetWriteBatch()->GetWriteBatch(),
                          /*allback*/nullptr，&log_number_ux，/*log ref*/0，
                          /*禁用内存表*/ true);

  return s;
}

Status PessimisticTransaction::Commit() {
  Status s;
  bool commit_without_prepare = false;
  bool commit_prepared = false;

  if (IsExpired()) {
    return Status::Expired();
  }

  if (expiration_time_ > 0) {
//我们必须原子地比较和交换这里的状态，因为
//此状态在事务中，另一个线程是可能的
//甚至在我们到期和拥有
//我们的锁被偷了。在这种情况下，唯一有效的状态是启动的，因为
//准备状态将具有清除的到期时间。
    TransactionState expected = STARTED;
    commit_without_prepare = std::atomic_compare_exchange_strong(
        &txn_state_, &expected, AWAITING_COMMIT);
    TEST_SYNC_POINT("TransactionTest::ExpirableTransactionDataRace:1");
  } else if (txn_state_ == PREPARED) {
//过期和锁窃取不是问题
    commit_prepared = true;
  } else if (txn_state_ == STARTED) {
//过期和锁窃取不是问题
    commit_without_prepare = true;
//托多（Myabandeh）：如果用户错误地忘记了准备怎么办？我们应该
//添加一个选项，以便用户明确表达
//跳过准备阶段。
  }

  if (commit_without_prepare) {
    assert(!commit_prepared);
    if (WriteBatchInternal::Count(GetCommitTimeWriteBatch()) > 0) {
      s = Status::InvalidArgument(
          "Commit-time batch contains values that will not be committed.");
    } else {
      txn_state_.store(AWAITING_COMMIT);
      s = CommitWithoutPrepareInternal();
      Clear();
      if (s.ok()) {
        txn_state_.store(COMMITED);
      }
    }
  } else if (commit_prepared) {
    txn_state_.store(AWAITING_COMMIT);

    s = CommitInternal();

    if (!s.ok()) {
      ROCKS_LOG_WARN(db_impl_->immutable_db_options().info_log,
                     "Commit write failed");
      return s;
    }

//findobsoletefiles现在必须查找memtables
//为了确定必须保留哪些准备日志，
//不是准备部分堆。
    assert(log_number_ > 0);
    dbimpl_->MarkLogAsHavingPrepSectionFlushed(log_number_);
    txn_db_impl_->UnregisterTransaction(this);

    Clear();
    txn_state_.store(COMMITED);
  } else if (txn_state_ == LOCKS_STOLEN) {
    s = Status::Expired();
  } else if (txn_state_ == COMMITED) {
    s = Status::InvalidArgument("Transaction has already been committed.");
  } else if (txn_state_ == ROLLEDBACK) {
    s = Status::InvalidArgument("Transaction has already been rolledback.");
  } else {
    s = Status::InvalidArgument("Transaction is not in state for commit.");
  }

  return s;
}

Status WriteCommittedTxn::CommitWithoutPrepareInternal() {
  Status s = db_->Write(write_options_, GetWriteBatch()->GetWriteBatch());
  return s;
}

Status WriteCommittedTxn::CommitInternal() {
//我们采用提交时间批处理并附加提交标记。
//在非恢复模式下，memtable将忽略提交标记
  WriteBatch* working_batch = GetCommitTimeWriteBatch();
  WriteBatchInternal::MarkCommit(working_batch, name_);

//任何附加到此工作批次的操作都将被Wal忽略。
  working_batch->MarkWalTerminationPoint();

//将准备好的批插入memtable，只跳过wal。
//memtable将忽略BeginPrepare/EndPrepare标记
//在非恢复模式下，只需插入值
  WriteBatchInternal::Append(working_batch, GetWriteBatch()->GetWriteBatch());

  auto s = db_impl_->WriteImpl(write_options_, working_batch, nullptr, nullptr,
                               log_number_);
  return s;
}

Status WriteCommittedTxn::Rollback() {
  Status s;
  if (txn_state_ == PREPARED) {
    WriteBatch rollback_marker;
    WriteBatchInternal::MarkRollback(&rollback_marker, name_);
    txn_state_.store(AWAITING_ROLLBACK);
    s = db_impl_->WriteImpl(write_options_, &rollback_marker);
    if (s.ok()) {
//我们不需要保留我们准备好的部分
      assert(log_number_ > 0);
      dbimpl_->MarkLogAsHavingPrepSectionFlushed(log_number_);
      Clear();
      txn_state_.store(ROLLEDBACK);
    }
  } else if (txn_state_ == STARTED) {
//准备不可能发生
    Clear();
  } else if (txn_state_ == COMMITED) {
    s = Status::InvalidArgument("This transaction has already been committed.");
  } else {
    s = Status::InvalidArgument(
        "Two phase transaction is not in state for rollback.");
  }

  return s;
}

Status PessimisticTransaction::RollbackToSavePoint() {
  if (txn_state_ != STARTED) {
    return Status::InvalidArgument("Transaction is beyond state for rollback.");
  }

//解锁自上次交易以来锁定的所有密钥
  const std::unique_ptr<TransactionKeyMap>& keys =
      GetTrackedKeysSinceSavePoint();

  if (keys) {
    txn_db_impl_->UnLock(this, keys.get());
  }

  return TransactionBaseImpl::RollbackToSavePoint();
}

//锁定此批中的所有密钥。
//成功后，呼叫者应解锁钥匙
Status PessimisticTransaction::LockBatch(WriteBatch* batch,
                                         TransactionKeyMap* keys_to_unlock) {
  class Handler : public WriteBatch::Handler {
   public:
//已将列_family_id的映射排序到已排序的键集。
//由于lockbatch（）总是按排序顺序锁定键，因此它不能死锁
//与它自己。我们这里不使用比较器，因为它无关紧要
//只要排序是一致的。
    std::map<uint32_t, std::set<std::string>> keys_;

    Handler() {}

    void RecordKey(uint32_t column_family_id, const Slice& key) {
      std::string key_str = key.ToString();

      auto iter = (keys_)[column_family_id].find(key_str);
      if (iter == (keys_)[column_family_id].end()) {
//钥匙还没看到，把它存起来。
        (keys_)[column_family_id].insert({std::move(key_str)});
      }
    }

    virtual Status PutCF(uint32_t column_family_id, const Slice& key,
                         /*st slice&/*unused*/）override
      recordkey（column_family_id，key）；
      返回状态：：OK（）；
    }
    虚拟状态合并cf（uint32_t column_family_id，const slice&key，
                           常量切片&/*未使用*/) override {

      RecordKey(column_family_id, key);
      return Status::OK();
    }
    virtual Status DeleteCF(uint32_t column_family_id,
                            const Slice& key) override {
      RecordKey(column_family_id, key);
      return Status::OK();
    }
  };

//迭代此处理程序会将此批处理中的所有键添加到键中
  Handler handler;
  batch->Iterate(&handler);

  Status s;

//尝试锁定所有密钥
  for (const auto& cf_iter : handler.keys_) {
    uint32_t cfh_id = cf_iter.first;
    auto& cfh_keys = cf_iter.second;

    for (const auto& key_iter : cfh_keys) {
      const std::string& key = key_iter;

      /*txn_db_impl_u->trylock（this，cfh_id，key，true/*exclusive*/）；
      如果（！）S.O.（））{
        断裂；
      }
      trackkey（key_to_unlock，cfh_id，std:：move（key），kmaxSequenceNumber，
               假，真/*独占*/);

    }

    if (!s.ok()) {
      break;
    }
  }

  if (!s.ok()) {
    txn_db_impl_->UnLock(this, keys_to_unlock);
  }

  return s;
}

//尝试锁定此密钥。
//如果密钥已成功锁定，则返回OK。不好，否则。
//如果check-shapshot为真，并且此事务具有快照集，
//只有在此后没有写入此密钥的情况下，才会锁定此密钥。
//快照时间。
Status PessimisticTransaction::TryLock(ColumnFamilyHandle* column_family,
                                       const Slice& key, bool read_only,
                                       bool exclusive, bool untracked) {
  uint32_t cfh_id = GetColumnFamilyID(column_family);
  std::string key_str = key.ToString();
  bool previously_locked;
  bool lock_upgrade = false;
  Status s;

//如果此事务尚未锁定此密钥，请将其锁定
  SequenceNumber current_seqno = kMaxSequenceNumber;
  SequenceNumber new_seqno = kMaxSequenceNumber;

  const auto& tracked_keys = GetTrackedKeys();
  const auto tracked_keys_cf = tracked_keys.find(cfh_id);
  if (tracked_keys_cf == tracked_keys.end()) {
    previously_locked = false;
  } else {
    auto iter = tracked_keys_cf->second.find(key_str);
    if (iter == tracked_keys_cf->second.end()) {
      previously_locked = false;
    } else {
      if (!iter->second.exclusive && exclusive) {
        lock_upgrade = true;
      }
      previously_locked = true;
      current_seqno = iter->second.seq;
    }
  }

//如果此交易尚未锁定或我们需要，请锁定此密钥
//升级。
  if (!previously_locked || lock_upgrade) {
    s = txn_db_impl_->TryLock(this, cfh_id, key_str, exclusive);
  }

  SetSnapshotIfNeeded();

//即使我们不关心对这篇文章进行冲突检查，
//我们仍然需要锁上以确保不会与
//另写一些。但是，我们不需要检查
//此事务的快照之后的任何写入操作。
//todo（agiardullo）：可以通过在
//未来
  if (untracked || snapshot_ == nullptr) {
//需要记住我们知道的最早的序列号
//之后未修改密钥。如果相同，这是有用的
//交易
//稍后尝试再次锁定此密钥。
    if (current_seqno == kMaxSequenceNumber) {
//因为我们没有检查快照，我们只知道这个密钥没有
//在我们锁定后被修改。
      new_seqno = db_->GetLatestSequenceNumber();
    } else {
      new_seqno = current_seqno;
    }
  } else {
//如果设置了快照，我们需要确保密钥没有被修改。
//从快照开始。这必须在我们锁上钥匙后完成。
    if (s.ok()) {
      s = ValidateSnapshot(column_family, key, current_seqno, &new_seqno);

      if (!s.ok()) {
//无法验证密钥
        if (!previously_locked) {
//解锁钥匙我们刚刚锁定
          if (lock_upgrade) {
            s = txn_db_impl_->TryLock(this, cfh_id, key_str,
                                      /*SE/*独占*/）；
            断言（S.O.（））；
          }否则{
            txn_db_impl_u->unlock（this，cfh_id，key.toString（））；
          }
        }
      }
    }
  }

  如果（S.O.（））{
    //让基类知道我们检查了该键的冲突。
    trackkey（cfh_id，key_str，new_seqno，read_only，exclusive）；
  }

  返回S；
}

//如果此键最近没有被修改过，则返回OK（）。
//事务快照。
状态悲观事务：：验证快照（
    columnfamilyhandle*column_family，const slice&key，
    序列号前一个序列号，序列号*新序列号）
  断言（快照）；

  sequenceNumber seq=snapshot_u->getSequenceNumber（）；
  如果（上一个<=seq）
    //如果之前在序列号处验证过该键
    //而不是当前快照的序列号，我们已经知道它没有
    //已修改。
    返回状态：：OK（）；
  }

  *新编号=序号；

  柱状簇柄*cfh=
      专栏“家庭”？column_family:db_impl_u->defaultColumnFamily（）；

  返回transactionUtil:：checkKeyForConflicts（db_impl_uuh，cfh，key.toString（），
                                               快照->GetSequenceNumber（），
                                               假/*仅缓存*/);

}

bool PessimisticTransaction::TryStealingLocks() {
  assert(IsExpired());
  TransactionState expected = STARTED;
  return std::atomic_compare_exchange_strong(&txn_state_, &expected,
                                             LOCKS_STOLEN);
}

void PessimisticTransaction::UnlockGetForUpdate(
    ColumnFamilyHandle* column_family, const Slice& key) {
  txn_db_impl_->UnLock(this, GetColumnFamilyID(column_family), key.ToString());
}

Status PessimisticTransaction::SetName(const TransactionName& name) {
  Status s;
  if (txn_state_ == STARTED) {
    if (name_.length()) {
      s = Status::InvalidArgument("Transaction has already been named.");
    } else if (txn_db_impl_->GetTransactionByName(name) != nullptr) {
      s = Status::InvalidArgument("Transaction name must be unique.");
    } else if (name.length() < 1 || name.length() > 512) {
      s = Status::InvalidArgument(
          "Transaction name length must be between 1 and 512 chars.");
    } else {
      name_ = name;
      txn_db_impl_->RegisterTransaction(this);
    }
  } else {
    s = Status::InvalidArgument("Transaction is beyond state for naming.");
  }
  return s;
}

}  //命名空间rocksdb

#endif  //摇滚乐
