
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
#include "utilities/transactions/pessimistic_transaction.h"
#include "utilities/transactions/pessimistic_transaction_db.h"
#include "utilities/transactions/transaction_base.h"
#include "utilities/transactions/transaction_util.h"

namespace rocksdb {

class WritePreparedTxnDB;

//这个impl可以向db写入未经审计的数据，然后将其分开
//来自未提交数据的已提交数据。未提交的数据可能在
//在2pc（writePreparedTxn）或之前准备阶段
//（写入UnparedTxNimpl）。
class WritePreparedTxn : public PessimisticTransaction {
 public:
  WritePreparedTxn(WritePreparedTxnDB* db, const WriteOptions& write_options,
                   const TransactionOptions& txn_options);

  virtual ~WritePreparedTxn() {}

  Status CommitBatch(WriteBatch* batch) override;

  Status Rollback() override;

 private:
  Status PrepareInternal() override;

  Status CommitWithoutPrepareInternal() override;

  Status CommitInternal() override;

//TODO（myabandeh）：验证当前impl是否使用值为
//也写有准备序列号。
//状态验证快照（ColumnFamilyHandle*Column_Family，Const Slice&
//钥匙，
//序列号前一个序列号，序列号*
//NexySeqNO；

//不允许复制
  WritePreparedTxn(const WritePreparedTxn&);
  void operator=(const WritePreparedTxn&);

  WritePreparedTxnDB* wpt_db_;
  uint64_t prepare_seq_;
};

}  //命名空间rocksdb

#endif  //摇滚乐
