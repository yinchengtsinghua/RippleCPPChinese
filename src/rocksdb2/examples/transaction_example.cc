
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

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"

using namespace rocksdb;

std::string kDBPath = "/tmp/rocksdb_transaction_example";

int main() {
//开放数据库
  Options options;
  TransactionDBOptions txn_db_options;
  options.create_if_missing = true;
  TransactionDB* txn_db;

  Status s = TransactionDB::Open(options, txn_db_options, kDBPath, &txn_db);
  assert(s.ok());

  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  std::string value;

///////////////////////////////////////
//
//简单事务示例（“read committed”）
//
///////////////////////////////////////

//启动事务
  Transaction* txn = txn_db->BeginTransaction(write_options);
  assert(txn);

//读取此事务中的密钥
  s = txn->Get(read_options, "abc", &value);
  assert(s.IsNotFound());

//在此事务中写入密钥
  s = txn->Put("abc", "def");
  assert(s.ok());

//读取此事务外部的密钥。不影响TXN。
  s = txn_db->Get(read_options, "abc", &value);

//在此事务之外写入密钥。
//不会影响txn，因为这是一个不相关的密钥。如果我们写了“ABC”键
//在这里，事务将无法提交。
  s = txn_db->Put(write_options, "xyz", "zzz");

//提交事务
  s = txn->Commit();
  assert(s.ok());
  delete txn;

///////////////////////////////////////
//
//“可重复读取”（快照隔离）示例
//--使用单个快照
//
///////////////////////////////////////

//通过设置set_snapshot=true在事务开始时设置快照
  txn_options.set_snapshot = true;
  txn = txn_db->BeginTransaction(write_options, txn_options);

  const Snapshot* snapshot = txn->GetSnapshot();

//在事务外写入密钥
  s = txn_db->Put(write_options, "abc", "xyz");
  assert(s.ok());

//尝试使用快照读取密钥。这将失败，因为
//此txn之外的上一次写入与此读取冲突。
  read_options.snapshot = snapshot;
  s = txn->GetForUpdate(read_options, "abc", &value);
  assert(s.IsBusy());

  txn->Rollback();

  delete txn;
//从读取选项中清除快照，因为它不再有效
  read_options.snapshot = nullptr;
  snapshot = nullptr;

///////////////////////////////////////
//
//“read committed”（单调原子视图）示例
//--使用多个快照
//
///////////////////////////////////////

//在本例中，我们多次设置快照。这可能是
//只有当你有非常严格的隔离要求
//实施。

//在事务开始时设置快照
  txn_options.set_snapshot = true;
  txn = txn_db->BeginTransaction(write_options, txn_options);

//对键“X”进行读写操作
  read_options.snapshot = txn_db->GetSnapshot();
  s = txn->Get(read_options, "x", &value);
  txn->Put("x", "x");

//在事务外部写入键“Y”
  s = txn_db->Put(write_options, "y", "y");

//在事务中设置新快照
  txn->SetSnapshot();
  txn->SetSavePoint();
  read_options.snapshot = txn_db->GetSnapshot();

//对“Y”键进行读写操作
//由于快照是高级的，因此在
//事务不冲突。
  s = txn->GetForUpdate(read_options, "y", &value);
  txn->Put("y", "y");

//决定我们要从此事务中恢复最后一次写入。
  txn->RollbackToSavePoint();

//承诺。
  s = txn->Commit();
  assert(s.ok());
  delete txn;
//从读取选项中清除快照，因为它不再有效
  read_options.snapshot = nullptr;

//清理
  delete txn_db;
  DestroyDB(kDBPath, options);
  return 0;
}

#endif  //摇滚乐
