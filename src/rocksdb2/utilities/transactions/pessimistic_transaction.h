
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

#pragma once

#ifndef ROCKSDB_LITE

#include <algorithm>
#include <atomic>
#include <mutex>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "db/write_callback.h"
#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/snapshot.h"
#include "rocksdb/status.h"
#include "rocksdb/types.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "util/autovector.h"
#include "utilities/transactions/transaction_base.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

class PessimisticTransactionDB;

//处于悲观并发控制下的事务。此类实现
//锁定API和与锁管理器以及
//悲观事务数据库。
class PessimisticTransaction : public TransactionBaseImpl {
 public:
  PessimisticTransaction(TransactionDB* db, const WriteOptions& write_options,
                         const TransactionOptions& txn_options);

  virtual ~PessimisticTransaction();

  void Reinitialize(TransactionDB* txn_db, const WriteOptions& write_options,
                    const TransactionOptions& txn_options);

  Status Prepare() override;

  Status Commit() override;

  virtual Status CommitBatch(WriteBatch* batch) = 0;

  Status Rollback() override = 0;

  Status RollbackToSavePoint() override;

  Status SetName(const TransactionName& name) override;

//生成新的唯一事务标识符
  static TransactionID GenTxnID();

  TransactionID GetID() const override { return txn_id_; }

  std::vector<TransactionID> GetWaitingTxns(uint32_t* column_family_id,
                                            std::string* key) const override {
    std::lock_guard<std::mutex> lock(wait_mutex_);
    std::vector<TransactionID> ids(waiting_txn_ids_.size());
    if (key) *key = waiting_key_ ? *waiting_key_ : "";
    if (column_family_id) *column_family_id = waiting_cf_id_;
    std::copy(waiting_txn_ids_.begin(), waiting_txn_ids_.end(), ids.begin());
    return ids;
  }

  void SetWaitingTxn(autovector<TransactionID> ids, uint32_t column_family_id,
                     const std::string* key) {
    std::lock_guard<std::mutex> lock(wait_mutex_);
    waiting_txn_ids_ = ids;
    waiting_cf_id_ = column_family_id;
    waiting_key_ = key;
  }

  void ClearWaitingTxn() {
    std::lock_guard<std::mutex> lock(wait_mutex_);
    waiting_txn_ids_.clear();
    waiting_cf_id_ = 0;
    waiting_key_ = nullptr;
  }

//返回时间（以微秒为单位，根据env->getmicros（））
//此交易将过期。如果此事务执行此操作，则返回0
//不过期。
  uint64_t GetExpirationTime() const { return expiration_time_; }

//如果此事务有过期时间且已过期，则返回true。
  bool IsExpired() const;

//返回事务在获取
//如果没有超时，则锁定或-1。
  int64_t GetLockTimeout() const { return lock_timeout_; }
  void SetLockTimeout(int64_t timeout) override {
    lock_timeout_ = timeout * 1000;
  }

//如果成功窃取锁，则返回true，否则返回false。
  bool TryStealingLocks();

  bool IsDeadlockDetect() const override { return deadlock_detect_; }

  int64_t GetDeadlockDetectDepth() const { return deadlock_detect_depth_; }

 protected:
  virtual Status PrepareInternal() = 0;

  virtual Status CommitWithoutPrepareInternal() = 0;

  virtual Status CommitInternal() = 0;

  void Initialize(const TransactionOptions& txn_options);

  Status LockBatch(WriteBatch* batch, TransactionKeyMap* keys_to_unlock);

  Status TryLock(ColumnFamilyHandle* column_family, const Slice& key,
                 bool read_only, bool exclusive,
                 bool untracked = false) override;

  void Clear() override;

  PessimisticTransactionDB* txn_db_impl_;
  DBImpl* db_impl_;

//如果非零，则不应在此时间之后提交此事务（在
//根据env->nowmicros（）的微秒数）
  uint64_t expiration_time_;

 private:
//用于为事务创建唯一ID。
  static std::atomic<TransactionID> txn_id_counter_;

//此交易记录的唯一ID
  TransactionID txn_id_;

//阻止当前事务的事务的ID。
//
//如果当前事务没有等待，则为空。
  autovector<TransactionID> waiting_txn_ids_;

//以下两个表示事务正在等待的（CF，key）
//在。
//
//如果等待键不为空，则指针应始终指向
//有效的字符串对象。原因是只有当
//事务在TransactionLockMgr:：AcquireWithTimeout中被阻止
//功能。此时，key string对象是函数之一
//参数。
  uint32_t waiting_cf_id_;
  const std::string* waiting_key_;

//互斥保护等待、等待和等待键。
  mutable std::mutex wait_mutex_;

//锁定密钥时超时（以微秒计），如果没有超时，则为-1。
  int64_t lock_timeout_;

//是否执行死锁检测。
  bool deadlock_detect_;

//是否执行死锁检测。
  int64_t deadlock_detect_depth_;

  Status ValidateSnapshot(ColumnFamilyHandle* column_family, const Slice& key,
                          SequenceNumber prev_seqno, SequenceNumber* new_seqno);

  void UnlockGetForUpdate(ColumnFamilyHandle* column_family,
                          const Slice& key) override;

//不允许复制
  PessimisticTransaction(const PessimisticTransaction&);
  void operator=(const PessimisticTransaction&);
};

class WriteCommittedTxn : public PessimisticTransaction {
 public:
  WriteCommittedTxn(TransactionDB* db, const WriteOptions& write_options,
                    const TransactionOptions& txn_options);

  virtual ~WriteCommittedTxn() {}

  Status CommitBatch(WriteBatch* batch) override;

  Status Rollback() override;

 private:
  Status PrepareInternal() override;

  Status CommitWithoutPrepareInternal() override;

  Status CommitInternal() override;

  Status ValidateSnapshot(ColumnFamilyHandle* column_family, const Slice& key,
                          SequenceNumber prev_seqno, SequenceNumber* new_seqno);

//不允许复制
  WriteCommittedTxn(const WriteCommittedTxn&);
  void operator=(const WriteCommittedTxn&);
};

//在提交时用于检查事务是否在其之前提交
//到期时间。
class TransactionCallback : public WriteCallback {
 public:
  explicit TransactionCallback(PessimisticTransaction* txn) : txn_(txn) {}

  /*tus回调（db*/*未使用*/）重写
    if（txn_u->isexpired（））
      返回状态：：expired（）；
    }否则{
      返回状态：：OK（）；
    }
  }

  bool allowritebatching（）重写返回true；

 私人：
  悲观交易*txn_uu；
}；

//命名空间rocksdb

endif//rocksdb_lite
