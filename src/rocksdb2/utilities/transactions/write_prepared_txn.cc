
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

#include "utilities/transactions/write_prepared_txn.h"

#include <map>

#include "db/column_family.h"
#include "db/db_impl.h"
#include "rocksdb/db.h"
#include "rocksdb/status.h"
#include "rocksdb/utilities/transaction_db.h"
#include "utilities/transactions/pessimistic_transaction.h"
#include "utilities/transactions/pessimistic_transaction_db.h"

namespace rocksdb {

struct WriteOptions;

WritePreparedTxn::WritePreparedTxn(WritePreparedTxnDB* txn_db,
                                   const WriteOptions& write_options,
                                   const TransactionOptions& txn_options)
    : PessimisticTransaction(txn_db, write_options, txn_options),
      wpt_db_(txn_db) {
  PessimisticTransaction::Initialize(txn_options);
}

/*tus writePreparedTxn:：commitbatch（writebatch*/*未使用*/）
  //todo（myabandeh）实现此
  throw std:：runtime_error（“commitbatch not implemented”）；
  返回状态：：OK（）；
}

状态writePreparedTxn:：PrepareInternal（）
  writeOptions write_options=write_options_；
  写入\options.disablewal=false；
  writeBatchInternal:：markendPrepare（getWriteBatch（）->getWriteBatch（），name_u）；
  const bool disable_memtable=true；
  使用的uint64序列号；
  状态S＝
      db_impl->write impl（write_options，getwritebatch（）->getwritebatch（），
                          /Calbac*/ nullptr, &log_number_, /*log ref*/ 0,

                          !disable_memtable, &seq_used);
  prepare_seq_ = seq_used;
  wpt_db_->AddPrepared(prepare_seq_);
  return s;
}

Status WritePreparedTxn::CommitWithoutPrepareInternal() {
//Todo（Myabandeh）实现
  throw std::runtime_error("Commit not Implemented");
  return Status::OK();
}

Status WritePreparedTxn::CommitInternal() {
//我们采用提交时间批处理并附加提交标记。
//在非恢复模式下，memtable将忽略提交标记
  WriteBatch* working_batch = GetCommitTimeWriteBatch();
//TODO（myabandeh）：防止用户在准备后写入txn
//阶段
  assert(working_batch->Count() == 0);
  WriteBatchInternal::MarkCommit(working_batch, name_);

//任何附加到此工作批次的操作都将被Wal忽略。
  working_batch->MarkWalTerminationPoint();

  const bool disable_memtable = true;
  uint64_t seq_used;
  auto s = db_impl_->WriteImpl(write_options_, working_batch, nullptr, nullptr,
                               log_number_, disable_memtable, &seq_used);
  uint64_t& commit_seq = seq_used;
  wpt_db_->AddCommitted(prepare_seq_, commit_seq);
  return s;
}

Status WritePreparedTxn::Rollback() {
//Todo（Myabandeh）实现
  throw std::runtime_error("Rollback not Implemented");
  return Status::OK();
}

}  //命名空间rocksdb

#endif  //摇滚乐
