
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

#include "utilities/transactions/optimistic_transaction.h"

#include <string>

#include "db/column_family.h"
#include "db/db_impl.h"
#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "util/cast_util.h"
#include "util/string_util.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

struct WriteOptions;

OptimisticTransaction::OptimisticTransaction(
    OptimisticTransactionDB* txn_db, const WriteOptions& write_options,
    const OptimisticTransactionOptions& txn_options)
    : TransactionBaseImpl(txn_db->GetBaseDB(), write_options), txn_db_(txn_db) {
  Initialize(txn_options);
}

void OptimisticTransaction::Initialize(
    const OptimisticTransactionOptions& txn_options) {
  if (txn_options.set_snapshot) {
    SetSnapshot();
  }
}

void OptimisticTransaction::Reinitialize(
    OptimisticTransactionDB* txn_db, const WriteOptions& write_options,
    const OptimisticTransactionOptions& txn_options) {
  TransactionBaseImpl::Reinitialize(txn_db->GetBaseDB(), write_options);
  Initialize(txn_options);
}

OptimisticTransaction::~OptimisticTransaction() {}

void OptimisticTransaction::Clear() { TransactionBaseImpl::Clear(); }

Status OptimisticTransaction::Prepare() {
  return Status::InvalidArgument(
      "Two phase commit not supported for optimistic transactions.");
}

Status OptimisticTransaction::Commit() {
//设置将调用checkTransactionForConflicts（）的回调
//检查提交此事务是否安全。
  OptimisticTransactionCallback callback(this);

  DBImpl* db_impl = static_cast_with_check<DBImpl, DB>(db_->GetRootDB());

  Status s = db_impl->WriteWithCallback(
      write_options_, GetWriteBatch()->GetWriteBatch(), &callback);

  if (s.ok()) {
    Clear();
  }

  return s;
}

Status OptimisticTransaction::Rollback() {
  Clear();
  return Status::OK();
}

//记录这个键，以便我们可以在提交时检查它是否有冲突。
//
//“exclusive”不用于optimatictransaction。
Status OptimisticTransaction::TryLock(ColumnFamilyHandle* column_family,
                                      const Slice& key, bool read_only,
                                      bool exclusive, bool untracked) {
  if (untracked) {
    return Status::OK();
  }
  uint32_t cfh_id = GetColumnFamilyID(column_family);

  SetSnapshotIfNeeded();

  SequenceNumber seq;
  if (snapshot_) {
    seq = snapshot_->GetSequenceNumber();
  } else {
    seq = db_->GetLatestSequenceNumber();
  }

  std::string key_str = key.ToString();

  TrackKey(cfh_id, key_str, seq, read_only, exclusive);

//总是返回OK。配置检查将在提交时进行。
  return Status::OK();
}

//如果提交此事务安全，则返回OK。返回状态：：忙碌
//如果存在读或写冲突，将阻止我们提交或
//如果我们不能确定是否会有这样的冲突。
//
//只应在编写器线程上调用，以避免任何争用条件
//在检测写入冲突时。
Status OptimisticTransaction::CheckTransactionForConflicts(DB* db) {
  Status result;

  auto db_impl = static_cast_with_check<DBImpl, DB>(db);

//因为我们在写线程上，不想阻止其他的写程序，
//我们将执行仅缓存冲突检查。这会导致Tryagain
//如果没有足够的memtable历史记录可供检查，则返回
//为了冲突。
  return TransactionUtil::CheckKeysForConflicts(db_impl, GetTrackedKeys(),
                                                /*e/*仅缓存*/）；
}

状态优化事务：：setname（const transactionname&/*未使用*/) {

  return Status::InvalidArgument("Optimistic transactions cannot be named.");
}

}  //命名空间rocksdb

#endif  //摇滚乐
