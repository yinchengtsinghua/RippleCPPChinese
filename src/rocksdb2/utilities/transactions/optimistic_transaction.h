
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
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/write_batch_with_index.h"
#include "utilities/transactions/transaction_base.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

class OptimisticTransaction : public TransactionBaseImpl {
 public:
  OptimisticTransaction(OptimisticTransactionDB* db,
                        const WriteOptions& write_options,
                        const OptimisticTransactionOptions& txn_options);

  virtual ~OptimisticTransaction();

  void Reinitialize(OptimisticTransactionDB* txn_db,
                    const WriteOptions& write_options,
                    const OptimisticTransactionOptions& txn_options);

  Status Prepare() override;

  Status Commit() override;

  Status Rollback() override;

  Status SetName(const TransactionName& name) override;

 protected:
  Status TryLock(ColumnFamilyHandle* column_family, const Slice& key,
                 bool read_only, bool exclusive,
                 bool untracked = false) override;

 private:
  OptimisticTransactionDB* const txn_db_;

  friend class OptimisticTransactionCallback;

  void Initialize(const OptimisticTransactionOptions& txn_options);

//如果提交此事务安全，则返回OK。返回状态：：忙碌
//如果存在读或写冲突，将阻止我们提交
//或者如果我们不能确定是否会有这样的冲突。
//
//只应在编写器线程上调用。
  Status CheckTransactionForConflicts(DB* db);

  void Clear() override;

  /*d unlocketforupdate（columnfamilyhandle*/*未使用*/，
                          常量切片&/*未使用*/) override {

//无需解锁。
  }

//不允许复制
  OptimisticTransaction(const OptimisticTransaction&);
  void operator=(const OptimisticTransaction&);
};

//在提交时用于触发事务验证
class OptimisticTransactionCallback : public WriteCallback {
 public:
  explicit OptimisticTransactionCallback(OptimisticTransaction* txn)
      : txn_(txn) {}

  Status Callback(DB* db) override {
    return txn_->CheckTransactionForConflicts(db);
  }

  bool AllowWriteBatching() override { return false; }

 private:
  OptimisticTransaction* txn_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
