
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

#include <functional>
#include <string>
#include <thread>

#include "rocksdb/db.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/transaction.h"
#include "util/crc32c.h"
#include "util/logging.h"
#include "util/random.h"
#include "util/testharness.h"
#include "util/transaction_test_util.h"
#include "port/port.h"

using std::string;

namespace rocksdb {

class OptimisticTransactionTest : public testing::Test {
 public:
  OptimisticTransactionDB* txn_db;
  DB* db;
  string dbname;
  Options options;

  OptimisticTransactionTest() {
    options.create_if_missing = true;
    options.max_write_buffer_number = 2;
    dbname = test::TmpDir() + "/optimistic_transaction_testdb";

    DestroyDB(dbname, options);
    Open();
  }
  ~OptimisticTransactionTest() {
    delete txn_db;
    DestroyDB(dbname, options);
  }

  void Reopen() {
    delete txn_db;
    txn_db = nullptr;
    Open();
  }

private:
  void Open() {
    Status s = OptimisticTransactionDB::Open(options, dbname, &txn_db);
    assert(s.ok());
    assert(txn_db != nullptr);
    db = txn_db->GetBaseDB();
  }
};

TEST_F(OptimisticTransactionTest, SuccessTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, Slice("foo"), Slice("bar"));
  db->Put(write_options, Slice("foo2"), Slice("bar"));

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  txn->GetForUpdate(read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  txn->Put(Slice("foo"), Slice("bar2"));

  txn->GetForUpdate(read_options, "foo", &value);
  ASSERT_EQ(value, "bar2");

  s = txn->Commit();
  ASSERT_OK(s);

  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar2");

  delete txn;
}

TEST_F(OptimisticTransactionTest, WriteConflictTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "bar");
  db->Put(write_options, "foo2", "bar");

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  txn->Put("foo", "bar2");

//此放在事务之外的操作将与上一次写入操作冲突
  s = db->Put(write_options, "foo", "barz");
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");
  ASSERT_EQ(1, txn->GetNumKeys());

  s = txn->Commit();
ASSERT_TRUE(s.IsBusy());  //txn不应提交

//验证事务没有写入任何内容
  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");
  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "bar");

  delete txn;
}

TEST_F(OptimisticTransactionTest, WriteConflictTest2) {
  WriteOptions write_options;
  ReadOptions read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "bar");
  db->Put(write_options, "foo2", "bar");

  txn_options.set_snapshot = true;
  Transaction* txn = txn_db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn);

//此放在事务之外的操作将与以后的写入操作冲突
  s = db->Put(write_options, "foo", "barz");
  ASSERT_OK(s);

txn->Put("foo", "bar2");  //与快照拍摄后完成的写入冲突

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");

  s = txn->Commit();
ASSERT_TRUE(s.IsBusy());  //txn不应提交

//验证事务没有写入任何内容
  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");
  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "bar");

  delete txn;
}

TEST_F(OptimisticTransactionTest, ReadConflictTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "bar");
  db->Put(write_options, "foo2", "bar");

  txn_options.set_snapshot = true;
  Transaction* txn = txn_db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn);

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

//此放在事务之外的操作将与以前的读取操作冲突
  s = db->Put(write_options, "foo", "barz");
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");

  s = txn->Commit();
ASSERT_TRUE(s.IsBusy());  //txn不应提交

//验证事务没有写入任何内容
  txn->GetForUpdate(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");
  txn->GetForUpdate(read_options, "foo2", &value);
  ASSERT_EQ(value, "bar");

  delete txn;
}

TEST_F(OptimisticTransactionTest, TxnOnlyTest) {
//测试以确保事务在
//空数据库。

  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  txn->Put("x", "y");

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;
}

TEST_F(OptimisticTransactionTest, FlushTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  db->Put(write_options, Slice("foo"), Slice("bar"));
  db->Put(write_options, Slice("foo2"), Slice("bar"));

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  txn->Put(Slice("foo"), Slice("bar2"));

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar2");

//放一个随机键，这样我们就有了一个可刷新的memtable
  s = db->Put(write_options, "dummy", "dummy");
  ASSERT_OK(s);

//强制Memtable刷新
  FlushOptions flush_ops;
  db->Flush(flush_ops);

  s = txn->Commit();
//由于刷新的表仍在memtablelist历史记录中，因此txn应提交。
  ASSERT_OK(s);

  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar2");

  delete txn;
}

TEST_F(OptimisticTransactionTest, FlushTest2) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  db->Put(write_options, Slice("foo"), Slice("bar"));
  db->Put(write_options, Slice("foo2"), Slice("bar"));

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  txn->Put(Slice("foo"), Slice("bar2"));

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar2");

//放一个随机键，这样我们就有了一个可刷新的memtable
  s = db->Put(write_options, "dummy", "dummy");
  ASSERT_OK(s);

//强制Memtable刷新
  FlushOptions flush_ops;
  db->Flush(flush_ops);

//放一个随机键，这样我们就有了一个可刷新的memtable
  s = db->Put(write_options, "dummy", "dummy2");
  ASSERT_OK(s);

//强制Memtable刷新
  db->Flush(flush_ops);

  s = db->Put(write_options, "dummy", "dummy3");
  ASSERT_OK(s);

//强制Memtable刷新
//由于测试数据库的max_write_buffer_number=2，因此此刷新将导致
//从MemTableList历史记录中清除的第一个MemTable。
  db->Flush(flush_ops);

  s = txn->Commit();
//由于memtablelist历史记录不够大，txn不应提交
  ASSERT_TRUE(s.IsTryAgain());

  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  delete txn;
}

TEST_F(OptimisticTransactionTest, NoSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, "AAA", "bar");

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

//事务启动后修改密钥
  db->Put(write_options, "AAA", "bar1");

//无快照读写
  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar1");
  txn->Put("AAA", "bar2");

//应该提交，因为在数据更改后完成了读/写操作
  s = txn->Commit();
  ASSERT_OK(s);

  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar2");

  delete txn;
}

TEST_F(OptimisticTransactionTest, MultipleSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  db->Put(write_options, "AAA", "bar");
  db->Put(write_options, "BBB", "bar");
  db->Put(write_options, "CCC", "bar");

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  db->Put(write_options, "AAA", "bar1");

//无快照读写
  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar1");
  txn->Put("AAA", "bar2");

//在拍摄快照之前修改bbb
  db->Put(write_options, "BBB", "bar1");

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

//用快照读写
  txn->GetForUpdate(snapshot_read_options, "BBB", &value);
  ASSERT_EQ(value, "bar1");
  txn->Put("BBB", "bar2");

  db->Put(write_options, "CCC", "bar1");

//设置新快照
  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

//用快照读写
  txn->GetForUpdate(snapshot_read_options, "CCC", &value);
  ASSERT_EQ(value, "bar1");
  txn->Put("CCC", "bar2");

  s = txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");
  s = txn->GetForUpdate(read_options, "BBB", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");
  s = txn->GetForUpdate(read_options, "CCC", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");

  s = db->Get(read_options, "AAA", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar1");
  s = db->Get(read_options, "BBB", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar1");
  s = db->Get(read_options, "CCC", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar1");

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "AAA", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");
  s = db->Get(read_options, "BBB", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");
  s = db->Get(read_options, "CCC", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");

//验证是否在不同快照中跟踪对同一密钥的多个写入操作
  delete txn;
  txn = txn_db->BeginTransaction(write_options);

//可能发生冲突的写入
  db->Put(write_options, "ZZZ", "zzz");
  db->Put(write_options, "XXX", "xxx");

  txn->SetSnapshot();

  OptimisticTransactionOptions txn_options;
  txn_options.set_snapshot = true;
  Transaction* txn2 = txn_db->BeginTransaction(write_options, txn_options);
  txn2->SetSnapshot();

//这不应在txn中冲突，因为快照晚于
//上一次写入（扰流器警报：稍后将与txn2冲突）。
  txn->Put("ZZZ", "zzzz");
  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;

//这将发生冲突，因为快照早于对zzz的另一次写入
  txn2->Put("ZZZ", "xxxxx");

  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn2;
}

TEST_F(OptimisticTransactionTest, ColumnFamiliesTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  ColumnFamilyHandle *cfa, *cfb;
  ColumnFamilyOptions cf_options;

//创建2个新的柱族
  s = db->CreateColumnFamily(cf_options, "CFA", &cfa);
  ASSERT_OK(s);
  s = db->CreateColumnFamily(cf_options, "CFB", &cfb);
  ASSERT_OK(s);

  delete cfa;
  delete cfb;
  delete txn_db;
  txn_db = nullptr;

//使用三列族打开数据库
  std::vector<ColumnFamilyDescriptor> column_families;
//必须打开默认列族
  column_families.push_back(
      ColumnFamilyDescriptor(kDefaultColumnFamilyName, ColumnFamilyOptions()));
//打开新的柱族
  column_families.push_back(
      ColumnFamilyDescriptor("CFA", ColumnFamilyOptions()));
  column_families.push_back(
      ColumnFamilyDescriptor("CFB", ColumnFamilyOptions()));
  std::vector<ColumnFamilyHandle*> handles;
  s = OptimisticTransactionDB::Open(options, dbname, column_families, &handles,
                                    &txn_db);
  ASSERT_OK(s);
  assert(txn_db != nullptr);
  db = txn_db->GetBaseDB();

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn_options.set_snapshot = true;
  Transaction* txn2 = txn_db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

//将一些数据写入数据库
  WriteBatch batch;
  batch.Put("foo", "foo");
  batch.Put(handles[1], "AAA", "bar");
  batch.Put(handles[1], "AAAZZZ", "bar");
  s = db->Write(write_options, &batch);
  ASSERT_OK(s);
  db->Delete(write_options, handles[1], "AAAZZZ");

//这些密钥与现有的写入没有冲突，因为它们位于
//不同的柱族
  txn->Delete("AAA");
  txn->GetForUpdate(snapshot_read_options, handles[1], "foo", &value);
  Slice key_slice("AAAZZZ");
  Slice value_slices[2] = {Slice("bar"), Slice("bar")};
  txn->Put(handles[2], SliceParts(&key_slice, 1), SliceParts(value_slices, 2));

  ASSERT_EQ(3, txn->GetNumKeys());

//TXN应该提交
  s = txn->Commit();
  ASSERT_OK(s);
  s = db->Get(read_options, "AAA", &value);
  ASSERT_TRUE(s.IsNotFound());
  s = db->Get(read_options, handles[2], "AAAZZZ", &value);
  ASSERT_EQ(value, "barbar");

  Slice key_slices[3] = {Slice("AAA"), Slice("ZZ"), Slice("Z")};
  Slice value_slice("barbarbar");
//此写入将导致与早期批写入冲突
  txn2->Put(handles[1], SliceParts(key_slices, 3), SliceParts(&value_slice, 1));

  txn2->Delete(handles[2], "XXX");
  txn2->Delete(handles[1], "XXX");
  s = txn2->GetForUpdate(snapshot_read_options, handles[1], "AAA", &value);
  ASSERT_TRUE(s.IsNotFound());

//验证txn未提交
  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());
  s = db->Get(read_options, handles[1], "AAAZZZ", &value);
  ASSERT_EQ(value, "barbar");

  delete txn;
  delete txn2;

  txn = txn_db->BeginTransaction(write_options, txn_options);
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn2 = txn_db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn);

  std::vector<ColumnFamilyHandle*> multiget_cfh = {handles[1], handles[2],
                                                   handles[0], handles[2]};
  std::vector<Slice> multiget_keys = {"AAA", "AAAZZZ", "foo", "foo"};
  std::vector<std::string> values(4);

  std::vector<Status> results = txn->MultiGetForUpdate(
      snapshot_read_options, multiget_cfh, multiget_keys, &values);
  ASSERT_OK(results[0]);
  ASSERT_OK(results[1]);
  ASSERT_OK(results[2]);
  ASSERT_TRUE(results[3].IsNotFound());
  ASSERT_EQ(values[0], "bar");
  ASSERT_EQ(values[1], "barbar");
  ASSERT_EQ(values[2], "foo");

  txn->Delete(handles[2], "ZZZ");
  txn->Put(handles[2], "ZZZ", "YYY");
  txn->Put(handles[2], "ZZZ", "YYYY");
  txn->Delete(handles[2], "ZZZ");
  txn->Put(handles[2], "AAAZZZ", "barbarbar");

  ASSERT_EQ(5, txn->GetNumKeys());

//TXN应该提交
  s = txn->Commit();
  ASSERT_OK(s);
  s = db->Get(read_options, handles[2], "ZZZ", &value);
  ASSERT_TRUE(s.IsNotFound());

//使用上一个快照放置一个与下一个txn冲突的键
  db->Put(write_options, handles[2], "foo", "000");

  results = txn2->MultiGetForUpdate(snapshot_read_options, multiget_cfh,
                                    multiget_keys, &values);
  ASSERT_OK(results[0]);
  ASSERT_OK(results[1]);
  ASSERT_OK(results[2]);
  ASSERT_TRUE(results[3].IsNotFound());
  ASSERT_EQ(values[0], "bar");
  ASSERT_EQ(values[1], "barbar");
  ASSERT_EQ(values[2], "foo");

//验证txn未提交
  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  s = db->DropColumnFamily(handles[1]);
  ASSERT_OK(s);
  s = db->DropColumnFamily(handles[2]);
  ASSERT_OK(s);

  delete txn;
  delete txn2;

  for (auto handle : handles) {
    delete handle;
  }
}

TEST_F(OptimisticTransactionTest, EmptyTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  s = db->Put(write_options, "aaa", "aaa");
  ASSERT_OK(s);

  Transaction* txn = txn_db->BeginTransaction(write_options);
  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  txn = txn_db->BeginTransaction(write_options);
  txn->Rollback();
  delete txn;

  txn = txn_db->BeginTransaction(write_options);
  s = txn->GetForUpdate(read_options, "aaa", &value);
  ASSERT_EQ(value, "aaa");

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  txn = txn_db->BeginTransaction(write_options);
  txn->SetSnapshot();
  s = txn->GetForUpdate(read_options, "aaa", &value);
  ASSERT_EQ(value, "aaa");

  s = db->Put(write_options, "aaa", "xxx");
  s = txn->Commit();
  ASSERT_TRUE(s.IsBusy());
  delete txn;
}

TEST_F(OptimisticTransactionTest, PredicateManyPreceders) {
  WriteOptions write_options;
  ReadOptions read_options1, read_options2;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  txn_options.set_snapshot = true;
  Transaction* txn1 = txn_db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  Transaction* txn2 = txn_db->BeginTransaction(write_options);
  txn2->SetSnapshot();
  read_options2.snapshot = txn2->GetSnapshot();

  std::vector<Slice> multiget_keys = {"1", "2", "3"};
  std::vector<std::string> multiget_values;

  std::vector<Status> results =
      txn1->MultiGetForUpdate(read_options1, multiget_keys, &multiget_values);
  ASSERT_TRUE(results[1].IsNotFound());

  txn2->Put("2", "x");

  s = txn2->Commit();
  ASSERT_OK(s);

  multiget_values.clear();
  results =
      txn1->MultiGetForUpdate(read_options1, multiget_keys, &multiget_values);
  ASSERT_TRUE(results[1].IsNotFound());

//不应提交，因为txn2写入了txn已读取的密钥
  s = txn1->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn1;
  delete txn2;

  txn1 = txn_db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = txn_db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  txn1->Put("4", "x");

  txn2->Delete("4");

//txn1可以提交，因为txn2的删除尚未发生（它只是批处理的）
  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->GetForUpdate(read_options2, "4", &value);
  ASSERT_TRUE(s.IsNotFound());

//由于txn1更改了“4”，txn2无法提交
  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn1;
  delete txn2;
}

TEST_F(OptimisticTransactionTest, LostUpdate) {
  WriteOptions write_options;
  ReadOptions read_options, read_options1, read_options2;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

//测试2个事务在多个订单中写入同一个密钥，以及
//有/无快照

  Transaction* txn1 = txn_db->BeginTransaction(write_options);
  Transaction* txn2 = txn_db->BeginTransaction(write_options);

  txn1->Put("1", "1");
  txn2->Put("1", "2");

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn1;
  delete txn2;

  txn_options.set_snapshot = true;
  txn1 = txn_db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = txn_db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  txn1->Put("1", "3");
  txn2->Put("1", "4");

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn1;
  delete txn2;

  txn1 = txn_db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = txn_db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  txn1->Put("1", "5");
  s = txn1->Commit();
  ASSERT_OK(s);

  txn2->Put("1", "6");
  s = txn2->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete txn1;
  delete txn2;

  txn1 = txn_db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = txn_db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  txn1->Put("1", "5");
  s = txn1->Commit();
  ASSERT_OK(s);

  txn2->SetSnapshot();
  txn2->Put("1", "6");
  s = txn2->Commit();
  ASSERT_OK(s);

  delete txn1;
  delete txn2;

  txn1 = txn_db->BeginTransaction(write_options);
  txn2 = txn_db->BeginTransaction(write_options);

  txn1->Put("1", "7");
  s = txn1->Commit();
  ASSERT_OK(s);

  txn2->Put("1", "8");
  s = txn2->Commit();
  ASSERT_OK(s);

  delete txn1;
  delete txn2;

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "8");
}

TEST_F(OptimisticTransactionTest, UntrackedWrites) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

//验证事务回滚是否适用于未跟踪的密钥。
  Transaction* txn = txn_db->BeginTransaction(write_options);
  txn->PutUntracked("untracked", "0");
  txn->Rollback();
  s = db->Get(read_options, "untracked", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
  txn = txn_db->BeginTransaction(write_options);

  txn->Put("tracked", "1");
  txn->PutUntracked("untracked", "1");
  txn->MergeUntracked("untracked", "2");
  txn->DeleteUntracked("untracked");

//写入事务外部的未跟踪密钥并验证
//它不会阻止事务提交。
  s = db->Put(write_options, "untracked", "x");
  ASSERT_OK(s);

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "untracked", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
  txn = txn_db->BeginTransaction(write_options);

  txn->Put("tracked", "10");
  txn->PutUntracked("untracked", "A");

//写入事务外部的跟踪密钥并验证
//提交失败时不会写入未跟踪的密钥。
  s = db->Delete(write_options, "tracked");

  s = txn->Commit();
  ASSERT_TRUE(s.IsBusy());

  s = db->Get(read_options, "untracked", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
}

TEST_F(OptimisticTransactionTest, IteratorTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

//给数据库写一些键
  s = db->Put(write_options, "A", "a");
  ASSERT_OK(s);

  s = db->Put(write_options, "G", "g");
  ASSERT_OK(s);

  s = db->Put(write_options, "F", "f");
  ASSERT_OK(s);

  s = db->Put(write_options, "C", "c");
  ASSERT_OK(s);

  s = db->Put(write_options, "D", "d");
  ASSERT_OK(s);

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

//在txn中写一些键
  s = txn->Put("B", "b");
  ASSERT_OK(s);

  s = txn->Put("H", "h");
  ASSERT_OK(s);

  s = txn->Delete("D");
  ASSERT_OK(s);

  s = txn->Put("E", "e");
  ASSERT_OK(s);

  txn->SetSnapshot();
  const Snapshot* snapshot = txn->GetSnapshot();

//在快照后向数据库写入一些键
  s = db->Put(write_options, "BB", "xx");
  ASSERT_OK(s);

  s = db->Put(write_options, "C", "xx");
  ASSERT_OK(s);

  read_options.snapshot = snapshot;
  Iterator* iter = txn->GetIterator(read_options);
  ASSERT_OK(iter->status());
  iter->SeekToFirst();

//通过ITER读取所有密钥并将其全部锁定
  std::string results[] = {"a", "b", "c", "e", "f", "g", "h"};
  for (int i = 0; i < 7; i++) {
    ASSERT_OK(iter->status());
    ASSERT_TRUE(iter->Valid());
    ASSERT_EQ(results[i], iter->value().ToString());

    s = txn->GetForUpdate(read_options, iter->key(), nullptr);
    ASSERT_OK(s);

    iter->Next();
  }
  ASSERT_FALSE(iter->Valid());

  iter->Seek("G");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("g", iter->value().ToString());

  iter->Prev();
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("f", iter->value().ToString());

  iter->Seek("D");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("e", iter->value().ToString());

  iter->Seek("C");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("c", iter->value().ToString());

  iter->Next();
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("e", iter->value().ToString());

  iter->Seek("");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("a", iter->value().ToString());

  iter->Seek("X");
  ASSERT_OK(iter->status());
  ASSERT_FALSE(iter->Valid());

  iter->SeekToLast();
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("h", iter->value().ToString());

//在txn的快照之后，在数据库中修改了键“c”。TXN将不提交。
  s = txn->Commit();
  ASSERT_TRUE(s.IsBusy());

  delete iter;
  delete txn;
}

TEST_F(OptimisticTransactionTest, SavepointTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  Transaction* txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());

txn->SetSavePoint();  //一

ASSERT_OK(txn->RollbackToSavePoint());  //回滚到txn的开头
  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Put("B", "b");
  ASSERT_OK(s);

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("b", value);

  delete txn;
  txn = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Put("B", "bb");
  ASSERT_OK(s);

  s = txn->Put("C", "c");
  ASSERT_OK(s);

txn->SetSavePoint();  //二

  s = txn->Delete("B");
  ASSERT_OK(s);

  s = txn->Put("C", "cc");
  ASSERT_OK(s);

  s = txn->Put("D", "d");
  ASSERT_OK(s);

ASSERT_OK(txn->RollbackToSavePoint());  //回滚至2

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  s = txn->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("bb", value);

  s = txn->Get(read_options, "C", &value);
  ASSERT_OK(s);
  ASSERT_EQ("c", value);

  s = txn->Get(read_options, "D", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Put("E", "e");
  ASSERT_OK(s);

//回滚到txn的开头
  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());
  txn->Rollback();

  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("b", value);

  s = txn->Get(read_options, "D", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Get(read_options, "D", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Get(read_options, "E", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Put("A", "aa");
  ASSERT_OK(s);

  s = txn->Put("F", "f");
  ASSERT_OK(s);

txn->SetSavePoint();  //三
txn->SetSavePoint();  //四

  s = txn->Put("G", "g");
  ASSERT_OK(s);

  s = txn->Delete("F");
  ASSERT_OK(s);

  s = txn->Delete("B");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("aa", value);

  s = txn->Get(read_options, "F", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Get(read_options, "B", &value);
  ASSERT_TRUE(s.IsNotFound());

ASSERT_OK(txn->RollbackToSavePoint());  //回滚至3

  s = txn->Get(read_options, "F", &value);
  ASSERT_OK(s);
  ASSERT_EQ("f", value);

  s = txn->Get(read_options, "G", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "F", &value);
  ASSERT_OK(s);
  ASSERT_EQ("f", value);

  s = db->Get(read_options, "G", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = db->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("aa", value);

  s = db->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("b", value);

  s = db->Get(read_options, "C", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = db->Get(read_options, "D", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = db->Get(read_options, "E", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
}

TEST_F(OptimisticTransactionTest, UndoGetForUpdateTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  OptimisticTransactionOptions txn_options;
  string value;
  Status s;

  db->Put(write_options, "A", "");

  Transaction* txn1 = txn_db->BeginTransaction(write_options);
  ASSERT_TRUE(txn1);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");

  Transaction* txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1是否可以提交，因为未检查冲突
  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);
  txn1->Put("A", "a");

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1无法提交，因为仍将检查冲突
  s = txn1->Commit();
  ASSERT_TRUE(s.IsBusy());
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1无法提交，因为仍将检查冲突
  s = txn1->Commit();
  ASSERT_TRUE(s.IsBusy());
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1是否可以提交，因为未检查冲突
  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->SetSavePoint();
  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1无法提交，因为仍将检查冲突
  s = txn1->Commit();
  ASSERT_TRUE(s.IsBusy());
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->SetSavePoint();
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1无法提交，因为仍将检查冲突
  s = txn1->Commit();
  ASSERT_TRUE(s.IsBusy());
  delete txn1;

  txn1 = txn_db->BeginTransaction(write_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);

  txn1->SetSavePoint();
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  txn1->UndoGetForUpdate("A");

  txn1->RollbackToSavePoint();
  txn1->UndoGetForUpdate("A");

  txn2 = txn_db->BeginTransaction(write_options);
  txn2->Put("A", "x");
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

//验证Txn1是否可以提交，因为未检查冲突
  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;
}

namespace {
Status OptimisticTransactionStressTestInserter(OptimisticTransactionDB* db,
                                               const size_t num_transactions,
                                               const size_t num_sets,
                                               const size_t num_keys_per_set) {
  size_t seed = std::hash<std::thread::id>()(std::this_thread::get_id());
  Random64 _rand(seed);
  WriteOptions write_options;
  ReadOptions read_options;
  OptimisticTransactionOptions txn_options;
  txn_options.set_snapshot = true;

  RandomTransactionInserter inserter(&_rand, write_options, read_options,
                                     num_keys_per_set,
                                     static_cast<uint16_t>(num_sets));

  for (size_t t = 0; t < num_transactions; t++) {
    bool success = inserter.OptimisticTransactionDBInsert(db, txn_options);
    if (!success) {
//意外失败
      return inserter.GetLastStatus();
    }
  }

//确保至少部分事务成功。如果可以的话
//有些由于写入冲突而失败。
  if (inserter.GetFailureCount() > num_transactions / 2) {
    return Status::TryAgain("Too many transactions failed! " +
                            std::to_string(inserter.GetFailureCount()) + " / " +
                            std::to_string(num_transactions));
  }

  return Status::OK();
}
}  //命名空间

TEST_F(OptimisticTransactionTest, OptimisticTransactionStressTest) {
  const size_t num_threads = 4;
  const size_t num_transactions_per_thread = 10000;
  const size_t num_sets = 3;
  const size_t num_keys_per_set = 100;
//将密钥空间设置为100个密钥将导致足够的写入冲突
//使这个测试有趣。

  std::vector<port::Thread> threads;

  std::function<void()> call_inserter = [&] {
    ASSERT_OK(OptimisticTransactionStressTestInserter(
        txn_db, num_transactions_per_thread, num_sets, num_keys_per_set));
  };

//创建n个使用RandomTransactionInsert写入的线程
//许多交易。
  for (uint32_t i = 0; i < num_threads; i++) {
    threads.emplace_back(call_inserter);
  }

//等待所有线程运行
  for (auto& t : threads) {
    t.join();
  }

//验证数据是否一致
  Status s = RandomTransactionInserter::Verify(db, num_sets);
  ASSERT_OK(s);
}

TEST_F(OptimisticTransactionTest, SequenceNumberAfterRecoverTest) {
  WriteOptions write_options;
  OptimisticTransactionOptions transaction_options;

  Transaction* transaction(txn_db->BeginTransaction(write_options, transaction_options));
  Status s = transaction->Put("foo", "val");
  ASSERT_OK(s);
  s = transaction->Put("foo2", "val");
  ASSERT_OK(s);
  s = transaction->Put("foo3", "val");
  ASSERT_OK(s);
  s = transaction->Commit();
  ASSERT_OK(s);
  delete transaction;

  Reopen();
  transaction = txn_db->BeginTransaction(write_options, transaction_options);
  s = transaction->Put("bar", "val");
  ASSERT_OK(s);
  s = transaction->Put("bar2", "val");
  ASSERT_OK(s);
  s = transaction->Commit();
  ASSERT_OK(s);

  delete transaction;
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(
      stderr,
      "SKIPPED as optimistic_transaction is not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //！摇滚乐
