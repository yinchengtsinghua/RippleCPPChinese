
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

#include <inttypes.h>
#include <algorithm>
#include <functional>
#include <string>
#include <thread>

#include "db/db_impl.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "table/mock_table.h"
#include "util/fault_injection_test_env.h"
#include "util/random.h"
#include "util/string_util.h"
#include "util/sync_point.h"
#include "util/testharness.h"
#include "util/testutil.h"
#include "util/transaction_test_util.h"
#include "utilities/merge_operators.h"
#include "utilities/merge_operators/string_append/stringappend.h"
#include "utilities/transactions/pessimistic_transaction_db.h"

#include "port/port.h"

using std::string;

namespace rocksdb {

class TransactionTest : public ::testing::TestWithParam<
                            std::tuple<bool, bool, TxnDBWritePolicy>> {
 public:
  TransactionDB* db;
  FaultInjectionTestEnv* env;
  string dbname;
  Options options;

  TransactionDBOptions txn_db_options;

  TransactionTest() {
    options.create_if_missing = true;
    options.max_write_buffer_number = 2;
    options.write_buffer_size = 4 * 1024;
    options.level0_file_num_compaction_trigger = 2;
    options.merge_operator = MergeOperators::CreateFromStringId("stringappend");
    env = new FaultInjectionTestEnv(Env::Default());
    options.env = env;
    options.concurrent_prepare = std::get<1>(GetParam());
    dbname = test::TmpDir() + "/transaction_testdb";

    DestroyDB(dbname, options);
    txn_db_options.transaction_lock_timeout = 0;
    txn_db_options.default_lock_timeout = 0;
    txn_db_options.write_policy = std::get<2>(GetParam());
    Status s;
    if (std::get<0>(GetParam()) == false) {
      s = TransactionDB::Open(options, txn_db_options, dbname, &db);
    } else {
      s = OpenWithStackableDB();
    }
    assert(s.ok());
  }

  ~TransactionTest() {
    delete db;
    DestroyDB(dbname, options);
    delete env;
  }

  Status ReOpenNoDelete() {
    delete db;
    db = nullptr;
    env->AssertNoOpenFile();
    env->DropUnsyncedFileData();
    env->ResetState();
    Status s;
    if (std::get<0>(GetParam()) == false) {
      s = TransactionDB::Open(options, txn_db_options, dbname, &db);
    } else {
      s = OpenWithStackableDB();
    }
    return s;
  }

  Status ReOpen() {
    delete db;
    DestroyDB(dbname, options);
    Status s;
    if (std::get<0>(GetParam()) == false) {
      s = TransactionDB::Open(options, txn_db_options, dbname, &db);
    } else {
      s = OpenWithStackableDB();
    }
    return s;
  }

  Status OpenWithStackableDB() {
    std::vector<size_t> compaction_enabled_cf_indices;
    std::vector<ColumnFamilyDescriptor> column_families{ColumnFamilyDescriptor(
        kDefaultColumnFamilyName, ColumnFamilyOptions(options))};

    TransactionDB::PrepareWrap(&options, &column_families,
                               &compaction_enabled_cf_indices);
    std::vector<ColumnFamilyHandle*> handles;
    DB* root_db;
    Options options_copy(options);
    Status s =
        DB::Open(options_copy, dbname, column_families, &handles, &root_db);
    if (s.ok()) {
      assert(handles.size() == 1);
      s = TransactionDB::WrapStackableDB(
          new StackableDB(root_db), txn_db_options,
          compaction_enabled_cf_indices, handles, &db);
      delete handles[0];
    }
    return s;
  }
};

class MySQLStyleTransactionTest : public TransactionTest {};
class WritePreparedTransactionTest : public TransactionTest {};

static const TxnDBWritePolicy wc = WRITE_COMMITTED;
static const TxnDBWritePolicy wp = WRITE_PREPARED;
//todo（myabandeh）：用其他写入策略实例化测试
INSTANTIATE_TEST_CASE_P(DBAsBaseDB, TransactionTest,
                        ::testing::Values(std::make_tuple(false, false, wc)));
INSTANTIATE_TEST_CASE_P(StackableDBAsBaseDB, TransactionTest,
                        ::testing::Values(std::make_tuple(true, false, wc)));
INSTANTIATE_TEST_CASE_P(MySQLStyleTransactionTest, MySQLStyleTransactionTest,
                        ::testing::Values(std::make_tuple(false, false, wc),
                                          std::make_tuple(false, true, wc),
                                          std::make_tuple(true, false, wc),
                                          std::make_tuple(true, true, wc)));
INSTANTIATE_TEST_CASE_P(WritePreparedTransactionTest,
                        WritePreparedTransactionTest,
                        ::testing::Values(std::make_tuple(false, true, wp)));

TEST_P(TransactionTest, DoubleEmptyWrite) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = false;

  WriteBatch batch;

  ASSERT_OK(db->Write(write_options, &batch));
  ASSERT_OK(db->Write(write_options, &batch));
}

TEST_P(TransactionTest, SuccessTest) {
  ASSERT_OK(db->ResetStats());

  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, Slice("foo"), Slice("bar"));
  db->Put(write_options, Slice("foo2"), Slice("bar"));

  Transaction* txn = db->BeginTransaction(write_options, TransactionOptions());
  ASSERT_TRUE(txn);

  ASSERT_EQ(0, txn->GetNumPuts());
  ASSERT_LE(0, txn->GetID());

  s = txn->GetForUpdate(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

  s = txn->Put(Slice("foo"), Slice("bar2"));
  ASSERT_OK(s);

  ASSERT_EQ(1, txn->GetNumPuts());

  s = txn->GetForUpdate(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar2");

  delete txn;
}

TEST_P(TransactionTest, WaitingTxn) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  txn_options.lock_timeout = 1;
  s = db->Put(write_options, Slice("foo"), Slice("bar"));
  ASSERT_OK(s);

  /*创建第二个CF*/
  ColumnFamilyHandle* cfa;
  ColumnFamilyOptions cf_options;
  s = db->CreateColumnFamily(cf_options, "CFA", &cfa);
  ASSERT_OK(s);
  s = db->Put(write_options, cfa, Slice("foo"), Slice("bar"));
  ASSERT_OK(s);

  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  TransactionID id1 = txn1->GetID();
  ASSERT_TRUE(txn1);
  ASSERT_TRUE(txn2);

  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "TransactionLockMgr::AcquireWithTimeout:WaitingTxn", [&](void* arg) {
        std::string key;
        uint32_t cf_id;
        std::vector<TransactionID> wait = txn2->GetWaitingTxns(&cf_id, &key);
        ASSERT_EQ(key, "foo");
        ASSERT_EQ(wait.size(), 1);
        ASSERT_EQ(wait[0], id1);
        ASSERT_EQ(cf_id, 0);
      });

//在默认的CF中锁定键
  s = txn1->GetForUpdate(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

//CFA中的锁密钥
  s = txn1->GetForUpdate(read_options, cfa, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

  auto lock_data = db->GetLockStatusData();
//锁定的键存在于两个列族中。
  ASSERT_EQ(lock_data.size(), 2);

  auto cf_iterator = lock_data.begin();

//迭代器指向无序的多重映射
//因此，测试不能假定任何特定的顺序。

//列族是1或0（cfa）。
  if (cf_iterator->first != 1 && cf_iterator->first != 0) {
    FAIL();
  }
//锁定键为“foo”，由txn1锁定
  ASSERT_EQ(cf_iterator->second.key, "foo");
  ASSERT_EQ(cf_iterator->second.ids.size(), 1);
  ASSERT_EQ(cf_iterator->second.ids[0], txn1->GetID());

  cf_iterator++;

//列族是0（默认）或1。
  if (cf_iterator->first != 1 && cf_iterator->first != 0) {
    FAIL();
  }
//锁定键为“foo”，由txn1锁定
  ASSERT_EQ(cf_iterator->second.key, "foo");
  ASSERT_EQ(cf_iterator->second.ids.size(), 1);
  ASSERT_EQ(cf_iterator->second.ids[0], txn1->GetID());

  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

  s = txn2->GetForUpdate(read_options, "foo", &value);
  ASSERT_TRUE(s.IsTimedOut());
  ASSERT_EQ(s.ToString(), "Operation timed out: Timeout waiting to lock key");

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();

  delete cfa;
  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, SharedLocks) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  Status s;

  txn_options.lock_timeout = 1;
  s = db->Put(write_options, Slice("foo"), Slice("bar"));
  ASSERT_OK(s);

  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn3 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);
  ASSERT_TRUE(txn2);
  ASSERT_TRUE(txn3);

//测试TXN之间的共享访问
  /*txn1->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  SalpTyk OK（s）；

  s=txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/);

  ASSERT_OK(s);

  /*txn3->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  SalpTyk OK（s）；

  auto-lock_data=db->getLockStatusData（）；
  断言_eq（lock_data.size（），1）；

  auto cf_iterator=lock_data.begin（）；
  断言“eq”（cf_iterator->second.key，“foo”）；

  //我们比较锁定此密钥的txns集是否相同。做
  //这样，我们需要对两个向量进行排序，以便完成比较
  /正确。
  std:：vector<transactionid>预期的_txns=txn1->getid（），txn2->getid（），
                                              TXN3--GETId（）}；
  std:：vector<transactionid>lock_txns=cf_iterator->second.id；
  断言“eq”（预期“txns”，锁定“txns”）；
  断言_False（cf_Iterator->second.exclusive）；

  txn1->rollback（）；
  txn2->rollback（）；
  txn3->rollback（）；

  //测试txn1和txn2共享一个锁，txn3尝试获取它。
  s=txn1->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/);

  ASSERT_OK(s);

  /*txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  SalpTyk OK（s）；

  s=txn3->getforupdate（读取选项，“foo”，nullptr）；
  断言“真”（s.istimedout（））；
  assert_eq（s.toString（），“操作超时：等待锁定键超时”）；

  txn1->UndogetForUpdate（“foo”）；
  s=txn3->getforupdate（读取选项，“foo”，nullptr）；
  断言“真”（s.istimedout（））；
  assert_eq（s.toString（），“操作超时：等待锁定键超时”）；

  txn2->UndogetForUpdate（“foo”）；
  s=txn3->getforupdate（读取选项，“foo”，nullptr）；
  SalpTyk OK（s）；

  txn1->rollback（）；
  txn2->rollback（）；
  txn3->rollback（）；

  //测试txn1和txn2共享锁，txn2尝试升级锁。
  s=txn1->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/);

  ASSERT_OK(s);

  /*txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  SalpTyk OK（s）；

  s=txn2->getforupdate（读取选项，“foo”，nullptr）；
  断言“真”（s.istimedout（））；
  assert_eq（s.toString（），“操作超时：等待锁定键超时”）；

  txn1->UndogetForUpdate（“foo”）；
  s=txn2->getforupdate（读取选项，“foo”，nullptr）；
  SalpTyk OK（s）；

  txn1->rollback（）；
  txn2->rollback（）；

  //测试txn1试图降低其锁的级别。
  s=txn1->getforupdate（读取选项，“foo”，nullptr，true/*独占*/);

  ASSERT_OK(s);

  /*txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  断言“真”（s.istimedout（））；
  assert_eq（s.toString（），“操作超时：等待锁定键超时”）；

  //在“降级”后仍应失败。
  s=txn1->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/);

  ASSERT_OK(s);

  /*txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  断言“真”（s.istimedout（））；
  assert_eq（s.toString（），“操作超时：等待锁定键超时”）；

  txn1->rollback（）；
  txn2->rollback（）；

  //测试持有独占锁的txn1，txn2尝试获取共享
  //访问。
  s=txn1->getforupdate（读取选项，“foo”，nullptr）；
  SalpTyk OK（s）；

  s=txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/);

  ASSERT_TRUE(s.IsTimedOut());
  ASSERT_EQ(s.ToString(), "Operation timed out: Timeout waiting to lock key");

  txn1->UndoGetForUpdate("foo");
  /*txn2->getforupdate（read_options，“foo”，nullptr，false/*exclusive*/）；
  SalpTyk OK（s）；

  删除TXN1；
  删除TXN2；
  删除TXN3；
}

测试_P（TransactionTest，DeadlockCycleShared）
  写选项写选项；
  读取选项读取选项；
  交易选项txn_选项；

  txn_options.lock_timeout=1000000；
  txn_options.deadlock_detect=true；

  //设置如下等待链：
  / /
  //Tn＞t（n＊2）
  //t n->t（n*2+1）
  / /
  我们有：
  //T1->T2->T4…
  //>t5…
  //>t3->t6…
  //>T7…
  //到t31，然后t[16-31]->t1。
  //注意，tn将锁固定在地板上（n/2）。

  std:：vector<transaction*>txns（31）；

  对于（uint32_t i=0；i<31；i++）
    txns[i]=db->beginTransaction（写入选项，txn-u选项）；
    断言“真”（txns[i]）；
    auto s=txns[i]->getforupdate（read_选项，toString（（i+1）/2），nullptr，
                                   错误/*独占*/);

    ASSERT_OK(s);
  }

  std::atomic<uint32_t> checkpoints(0);
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "TransactionLockMgr::AcquireWithTimeout:WaitingTxn",
      [&](void* arg) { checkpoints.fetch_add(1); });
  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

//我们希望叶事务阻止并阻止所有人。
  std::vector<port::Thread> threads;
  for (uint32_t i = 0; i < 15; i++) {
    std::function<void()> blocking_thread = [&, i] {
      auto s = txns[i]->GetForUpdate(read_options, ToString(i + 1), nullptr,
                                     /*e/*独占*/）；
      SalpTyk OK（s）；
      txns[i]->rollback（）；
      删除txns[i]；
    }；
    线程。放回（阻塞线程）；
  }

  //等待，直到所有线程都在彼此等待。
  同时（checkpoints.load（）！= 15）{
    /*睡眠覆盖*/

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();

//完成周期t[16-31]->t1
  for (uint32_t i = 15; i < 31; i++) {
    auto s =
        /*s[i]->getforupdate（read_options，“0”，nullptr，true/*exclusive*/）；
    断言_true（s.isreadlock（））；

    //当插入5条记录时，计算下一个缓冲区长度，5处的平台。
    const uint32_t curr_dlock_buffer_len_=
        （I-14>KinitialmaxDeadlocks）？KinitialmaxDeadlocks：（I-14）；

    auto dlock_buffer=db->getDeadlockInfoBuffer（）；
    断言eq（dlock_buffer.size（），curr_dlock_buffer_len_uu）；
    auto dlock_entry=dlock_buffer[0].路径；
    Assert_eq（dlock_entry.size（），KinitialMaxDeadLocks）；

    Int64_t curr_waiting_key=0；

    //每个txn id从共享dlock树的txn id根的偏移量。
    int64_t offset_root=dlock_entry[0].m_txn_id-1；
    //dlock路径中最后一个条目与根的txn id的偏移量。
    事务ID叶ID=
        dlock_entry[dlock_entry.size（）-1].m_txn_id-offset_root；

    对于（auto it=dlock_entry.rbegin（）；it！=dlock_entry.rend（）；it++）
      auto dl_node=*它；
      断言eq（dl_node.m_txn_id，offset_root+leaf_id）；
      断言eq（dl_node.m_cf_id，0）；
      断言eq（dl_node.m_waiting_key，to字符串（curr_waiting_key））；
      断言eq（dl_node.m_exclusive，true）；

      if（curr_waiting_key==0）
        curr_waiting_key=leaf_id；
      }
      当前等待键/=2；
      叶状ID/＝2；
    }
  }

  //回滚叶事务。
  对于（uint32_t i=15；i<31；i++）
    txns[i]->rollback（）；
    删除txns[i]；
  }

  用于（auto&t:线程）
    T.连接（）；
  }

  //缩小缓冲区的大小，并验证是否保留了3个最新的死锁。
  auto dlock_buffer_before_resize=db->getDeadlockInfoBuffer（）；
  db->setDeadlockInfoBufferSize（3）；
  auto dlock_buffer_after_resize=db->getDeadlockInfoBuffer（）；
  assert_eq（dlock_buffer_after_resize.size（），3）；

  for（uint32_t i=0；i<dlock_buffer_after_resize.size（）；i++）
    for（uint32_t j=0；j<dlock_buffer_after_resize[i].path.size（）；j++）
      在调整[i].path[j].m_txn_id之后断言_eq（dlock_buffer_，
                dlock_buffer_before_resize[i].path[j].m_txn_id）；
    }
  }

  //升迁缓冲区并验证是否保留了3个最新的释放。
  dlock_buffer_before_resize=db->getDeadlockInfoBuffer（）；
  db->setDeadlockInfoBufferSize（5）；
  dlock_buffer_after_resize=db->getDeadlockInfoBuffer（）；
  assert_eq（dlock_buffer_after_resize.size（），3）；

  for（uint32_t i=0；i<dlock_buffer_before_resize.size（）；i++）
    for（uint32_t j=0；j<dlock_buffer_before_resize[i].path.size（）；j++）
      在调整[i].path[j].m_txn_id之后断言_eq（dlock_buffer_，
                dlock_buffer_before_resize[i].path[j].m_txn_id）；
    }
  }

  //将大小缩小到0并验证大小是否一致。
  dlock_buffer_before_resize=db->getDeadlockInfoBuffer（）；
  db->setDeadlockInfoBufferSize（0）；
  dlock_buffer_after_resize=db->getDeadlockInfoBuffer（）；
  assert_eq（dlock_buffer_after_resize.size（），0）；

  //从0升迁以验证大小是否持久。
  dlock_buffer_before_resize=db->getDeadlockInfoBuffer（）；
  db->setDeadlockInfoBufferSize（3）；
  dlock_buffer_after_resize=db->getDeadlockInfoBuffer（）；
  assert_eq（dlock_buffer_after_resize.size（），0）；

  //循环大小为2的共享锁的人为事例，以验证
  //导致死锁的锁在缓冲区中正确报告为“共享”。
  std:：vector<transaction*>txns_shared（2）；

  //创建大小为2的循环。
  对于（uint32_t i=0；i<2；i++）
    txns_shared[i]=db->beginTransaction（写入选项，txn_选项）；
    断言为“真”（txns-shared[i]）；
    auto s=txns_shared[i]->getforupdate（read_options，toString（i），nullptr）；
    SalpTyk OK（s）；
  }

  std:：atomic<uint32_t>checkpoints_shared（0）；
  rocksdb:：syncpoint:：getInstance（）->setCallback（
      “TransactionLockMgr:：AcquireWithTimeout:等待xn”，
      [&]（void*arg）检查点共享。获取添加（1）；）；
  rocksdb:：syncpoint:：getInstance（）->启用处理（）；

  std:：vector<port:：thread>线程共享；
  对于（uint32_t i=0；i<1；i++）
    std:：function<void（）>blocking_thread=[&，i]
      汽车S =
          txns_shared[i]->getforupdate（read_options，toString（i+1），nullptr）；
      SalpTyk OK（s）；
      txns_shared[i]->rollback（）；
      删除txns_shared[i]；
    }；
    线程_shared.emplace_back（blocking_thread）；
  }

  //等待，直到所有线程都在彼此等待。
  同时（checkpoints_shared.load（）！= 1）{
    /*睡眠覆盖*/

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();

//使用共享锁完成周期t2->t1。
  auto s = txns_shared[1]->GetForUpdate(read_options, "0", nullptr, false);
  ASSERT_TRUE(s.IsDeadlock());

  auto dlock_buffer = db->GetDeadlockInfoBuffer();

//验证缓冲区和单个路径的大小。
  ASSERT_EQ(dlock_buffer.size(), 1);
  ASSERT_EQ(dlock_buffer[0].path.size(), 2);

//验证死锁路径中事务的独占性字段。
  ASSERT_TRUE(dlock_buffer[0].path[0].m_exclusive);
  ASSERT_FALSE(dlock_buffer[0].path[1].m_exclusive);
  txns_shared[1]->Rollback();
  delete txns_shared[1];

  for (auto& t : threads_shared) {
    t.join();
  }
}

TEST_P(TransactionTest, DeadlockCycle) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;

//从最大深度到测试边缘箱的偏移量为2
  const uint32_t kMaxCycleLength = 52;

  txn_options.lock_timeout = 1000000;
  txn_options.deadlock_detect = true;

  for (uint32_t len = 2; len < kMaxCycleLength; len++) {
//设置长等待链，如下所示：
//
//T1->T2->T3->>泰伦

    std::vector<Transaction*> txns(len);

    for (uint32_t i = 0; i < len; i++) {
      txns[i] = db->BeginTransaction(write_options, txn_options);
      ASSERT_TRUE(txns[i]);
      auto s = txns[i]->GetForUpdate(read_options, ToString(i), nullptr);
      ASSERT_OK(s);
    }

    std::atomic<uint32_t> checkpoints(0);
    rocksdb::SyncPoint::GetInstance()->SetCallBack(
        "TransactionLockMgr::AcquireWithTimeout:WaitingTxn",
        [&](void* arg) { checkpoints.fetch_add(1); });
    rocksdb::SyncPoint::GetInstance()->EnableProcessing();

//我们希望链中的最后一个事务阻止并阻止所有人
//回来。
    std::vector<port::Thread> threads;
    for (uint32_t i = 0; i < len - 1; i++) {
      std::function<void()> blocking_thread = [&, i] {
        auto s = txns[i]->GetForUpdate(read_options, ToString(i + 1), nullptr);
        ASSERT_OK(s);
        txns[i]->Rollback();
        delete txns[i];
      };
      threads.emplace_back(blocking_thread);
    }

//等待，直到所有线程都在彼此等待。
    while (checkpoints.load() != len - 1) {
      /*睡眠覆盖*/
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    rocksdb::SyncPoint::GetInstance()->DisableProcessing();
    rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();

//完成周期TLEN->T1
    auto s = txns[len - 1]->GetForUpdate(read_options, "0", nullptr);
    ASSERT_TRUE(s.IsDeadlock());

    const uint32_t dlock_buffer_size_ = (len - 1 > 5) ? 5 : (len - 1);
    uint32_t curr_waiting_key = 0;
    TransactionID curr_txn_id = txns[0]->GetID();

    auto dlock_buffer = db->GetDeadlockInfoBuffer();
    ASSERT_EQ(dlock_buffer.size(), dlock_buffer_size_);
    uint32_t check_len = len;
    bool check_limit_flag = false;

//死锁路径超过最大深度的特殊情况。
    if (len > 50) {
      check_len = 0;
      check_limit_flag = true;
    }
    auto dlock_entry = dlock_buffer[0].path;
    ASSERT_EQ(dlock_entry.size(), check_len);
    ASSERT_EQ(dlock_buffer[0].limit_exceeded, check_limit_flag);

//在路径上向后迭代，验证减少的txn_id。
    for (auto it = dlock_entry.rbegin(); it != dlock_entry.rend(); it++) {
      auto dl_node = *it;
      ASSERT_EQ(dl_node.m_txn_id, len + curr_txn_id - 1);
      ASSERT_EQ(dl_node.m_cf_id, 0);
      ASSERT_EQ(dl_node.m_waiting_key, ToString(curr_waiting_key));
      ASSERT_EQ(dl_node.m_exclusive, true);

      curr_txn_id--;
      if (curr_waiting_key == 0) {
        curr_waiting_key = len;
      }
      curr_waiting_key--;
    }

//回滚上一个事务。
    txns[len - 1]->Rollback();
    delete txns[len - 1];

    for (auto& t : threads) {
      t.join();
    }
  }
}

TEST_P(TransactionTest, DeadlockStress) {
  const uint32_t NUM_TXN_THREADS = 10;
  const uint32_t NUM_KEYS = 100;
  const uint32_t NUM_ITERS = 10000;

  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;

  txn_options.lock_timeout = 1000000;
  txn_options.deadlock_detect = true;
  std::vector<std::string> keys;

  for (uint32_t i = 0; i < NUM_KEYS; i++) {
    db->Put(write_options, Slice(ToString(i)), Slice(""));
    keys.push_back(ToString(i));
  }

  size_t tid = std::hash<std::thread::id>()(std::this_thread::get_id());
  Random rnd(static_cast<uint32_t>(tid));
  std::function<void(uint32_t)> stress_thread = [&](uint32_t seed) {
    std::default_random_engine g(seed);

    Transaction* txn;
    for (uint32_t i = 0; i < NUM_ITERS; i++) {
      txn = db->BeginTransaction(write_options, txn_options);
      auto random_keys = keys;
      std::shuffle(random_keys.begin(), random_keys.end(), g);

//按随机顺序锁定钥匙。
      for (const auto& k : random_keys) {
//锁主要用于共享访问，但只有1/4的时间是独占的。
        auto s =
            txn->GetForUpdate(read_options, k, nullptr, txn->GetID() % 4 == 0);
        if (!s.ok()) {
          ASSERT_TRUE(s.IsDeadlock());
          txn->Rollback();
          break;
        }
      }

      delete txn;
    }
  };

  std::vector<port::Thread> threads;
  for (uint32_t i = 0; i < NUM_TXN_THREADS; i++) {
    threads.emplace_back(stress_thread, rnd.Next());
  }

  for (auto& t : threads) {
    t.join();
  }
}

TEST_P(TransactionTest, CommitTimeBatchFailTest) {
  WriteOptions write_options;
  TransactionOptions txn_options;

  string value;
  Status s;

  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);

  txn1->GetCommitTimeWriteBatch()->Put("cat", "dog");

  s = txn1->Put("foo", "bar");
  ASSERT_OK(s);

//由于提交时间批处理非空而失败
  s = txn1->Commit();
  ASSERT_EQ(s, Status::InvalidArgument());

  delete txn1;
}

TEST_P(TransactionTest, SimpleTwoPhaseTransactionTest) {
  WriteOptions write_options;
  ReadOptions read_options;

  TransactionOptions txn_options;

  string value;
  Status s;

  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("xid");
  ASSERT_OK(s);

  ASSERT_EQ(db->GetTransactionByName("xid"), txn);

//交易投入
  s = txn->Put(Slice("foo"), Slice("bar"));
  ASSERT_OK(s);
  ASSERT_EQ(1, txn->GetNumPuts());

//规则数据库
  s = db->Put(write_options, Slice("foo2"), Slice("bar2"));
  ASSERT_OK(s);
  ASSERT_EQ(1, txn->GetNumPuts());

//规则数据库读取
  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "bar2");

//提交时间
  txn->GetCommitTimeWriteBatch()->Put(Slice("gtid"), Slice("dogs"));
  txn->GetCommitTimeWriteBatch()->Put(Slice("gtid2"), Slice("cats"));

//还没有准备好
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

  s = txn->Prepare();
  ASSERT_OK(s);

//数据还不是内存
  s = db->Get(read_options, Slice("foo"), &value);
  ASSERT_TRUE(s.IsNotFound());
  s = db->Get(read_options, Slice("gtid"), &value);
  ASSERT_TRUE(s.IsNotFound());

//在已准备交易记录列表中查找交易记录
  std::vector<Transaction*> prepared_trans;
  db->GetAllPreparedTransactions(&prepared_trans);
  ASSERT_EQ(prepared_trans.size(), 1);
  ASSERT_EQ(prepared_trans.front()->GetName(), "xid");

  auto log_containing_prep =
      db_impl->TEST_FindMinLogContainingOutstandingPrep();
  ASSERT_GT(log_containing_prep, 0);

//作出承诺
  s = txn->Commit();
  ASSERT_OK(s);

//值现在可用
  s = db->Get(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

  s = db->Get(read_options, "gtid", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "dogs");

  s = db->Get(read_options, "gtid2", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "cats");

//我们已经承诺了
  s = txn->Commit();
  ASSERT_EQ(s, Status::InvalidArgument());

//不再是prpared结果
  db->GetAllPreparedTransactions(&prepared_trans);
  ASSERT_EQ(prepared_trans.size(), 0);
  ASSERT_EQ(db->GetTransactionByName("xid"), nullptr);

//堆不应该再关心准备好的部分了
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

//但是现在我们的memtable应该引用准备部分
  ASSERT_EQ(log_containing_prep,
            db_impl->TEST_FindMinPrepLogReferencedByMemTable());

  db_impl->TEST_FlushMemTable(true);

//在memtable刷新之后，我们现在可以释放日志
  ASSERT_EQ(0, db_impl->TEST_FindMinPrepLogReferencedByMemTable());

  delete txn;
}

TEST_P(TransactionTest, TwoPhaseNameTest) {
  Status s;

  WriteOptions write_options;
  TransactionOptions txn_options;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn3 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn3);
  delete txn3;

//没有名字不能准备txn
  s = txn1->Prepare();
  ASSERT_EQ(s, Status::InvalidArgument());

//名称太短
  s = txn1->SetName("");
  ASSERT_EQ(s, Status::InvalidArgument());

//名称过长
  s = txn1->SetName(std::string(513, 'x'));
  ASSERT_EQ(s, Status::InvalidArgument());

//有效集名
  s = txn1->SetName("name1");
  ASSERT_OK(s);

//不能有重复的名称
  s = txn2->SetName("name1");
  ASSERT_EQ(s, Status::InvalidArgument());

//不应该准备
  s = txn2->Prepare();
  ASSERT_EQ(s, Status::InvalidArgument());

//有效名称集
  s = txn2->SetName("name2");
  ASSERT_OK(s);

//无法重置名称
  s = txn2->SetName("name3");
  ASSERT_EQ(s, Status::InvalidArgument());

  ASSERT_EQ(txn1->GetName(), "name1");
  ASSERT_EQ(txn2->GetName(), "name2");

  s = txn1->Prepare();
  ASSERT_OK(s);

//准备后无法重命名
  s = txn1->SetName("name4");
  ASSERT_EQ(s, Status::InvalidArgument());

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, TwoPhaseEmptyWriteTest) {
  Status s;
  std::string value;

  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

  s = txn1->SetName("joe");
  ASSERT_OK(s);

  s = txn2->SetName("bob");
  ASSERT_OK(s);

  s = txn1->Prepare();
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  delete txn1;

  txn2->GetCommitTimeWriteBatch()->Put(Slice("foo"), Slice("bar"));

  s = txn2->Prepare();
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

  delete txn2;
}

TEST_P(TransactionTest, TwoPhaseExpirationTest) {
  Status s;

  WriteOptions write_options;
  TransactionOptions txn_options;
txn_options.expiration = 500;  //500毫秒
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);
  ASSERT_TRUE(txn1);

  s = txn1->SetName("joe");
  ASSERT_OK(s);
  s = txn2->SetName("bob");
  ASSERT_OK(s);

  s = txn1->Prepare();
  ASSERT_OK(s);

  /*睡眠覆盖*/
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Prepare();
  ASSERT_EQ(s, Status::Expired());

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, TwoPhaseRollbackTest) {
  WriteOptions write_options;
  ReadOptions read_options;

  TransactionOptions txn_options;

  string value;
  Status s;

  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());
  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("xid");
  ASSERT_OK(s);

//交易投入
  s = txn->Put(Slice("tfoo"), Slice("tbar"));
  ASSERT_OK(s);

//值的可读形式为txn
  s = txn->Get(read_options, Slice("tfoo"), &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "tbar");

//问题回滚
  s = txn->Rollback();
  ASSERT_OK(s);

//值不再可读
  s = txn->Get(read_options, Slice("tfoo"), &value);
  ASSERT_TRUE(s.IsNotFound());
  ASSERT_EQ(txn->GetNumPuts(), 0);

//放入新的txn值
  s = txn->Put(Slice("tfoo2"), Slice("tbar2"));
  ASSERT_OK(s);

//从txn读取新值
  s = txn->Get(read_options, Slice("tfoo2"), &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "tbar2");

  s = txn->Prepare();
  ASSERT_OK(s);

//与下一个墙齐平
  s = db->Put(write_options, Slice("foo"), Slice("bar"));
  ASSERT_OK(s);
  db_impl->TEST_FlushMemTable(true);

//发布回滚（标记写入Wal）
  s = txn->Rollback();
  ASSERT_OK(s);

//值不再可读
  s = txn->Get(read_options, Slice("tfoo2"), &value);
  ASSERT_TRUE(s.IsNotFound());
  ASSERT_EQ(txn->GetNumPuts(), 0);

//作出承诺
  s = txn->Commit();
  ASSERT_EQ(s, Status::InvalidArgument());

//再次尝试回滚
  s = txn->Rollback();
  ASSERT_EQ(s, Status::InvalidArgument());

  delete txn;
}

TEST_P(TransactionTest, PersistentTwoPhaseTransactionTest) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = false;
  ReadOptions read_options;

  TransactionOptions txn_options;

  string value;
  Status s;

  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("xid");
  ASSERT_OK(s);

  ASSERT_EQ(db->GetTransactionByName("xid"), txn);

//交易投入
  s = txn->Put(Slice("foo"), Slice("bar"));
  ASSERT_OK(s);
  ASSERT_EQ(1, txn->GetNumPuts());

//TXN读
  s = txn->Get(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

//规则数据库
  s = db->Put(write_options, Slice("foo2"), Slice("bar2"));
  ASSERT_OK(s);
  ASSERT_EQ(1, txn->GetNumPuts());

  db_impl->TEST_FlushMemTable(true);

//规则数据库读取
  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "bar2");

//还没有准备好
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

//准备
  s = txn->Prepare();
  ASSERT_OK(s);

//数据库仍不可用
  s = db->Get(read_options, Slice("foo"), &value);
  ASSERT_TRUE(s.IsNotFound());

  db->FlushWAL(false);
  delete txn;
//杀戮与重开
  s = ReOpenNoDelete();
  ASSERT_OK(s);
  db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

//在已准备交易记录列表中查找交易记录
  std::vector<Transaction*> prepared_trans;
  db->GetAllPreparedTransactions(&prepared_trans);
  ASSERT_EQ(prepared_trans.size(), 1);

  txn = prepared_trans.front();
  ASSERT_TRUE(txn);
  ASSERT_EQ(txn->GetName(), "xid");
  ASSERT_EQ(db->GetTransactionByName("xid"), txn);

//日志已标记
  auto log_containing_prep =
      db_impl->TEST_FindMinLogContainingOutstandingPrep();
  ASSERT_GT(log_containing_prep, 0);

//值可从txn读取
  s = txn->Get(read_options, "foo", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "bar");

//作出承诺
  s = txn->Commit();
  ASSERT_OK(s);

//值现在可用
  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

//我们已经承诺了
  s = txn->Commit();
  ASSERT_EQ(s, Status::InvalidArgument());

//不再是prpared结果
  prepared_trans.clear();
  db->GetAllPreparedTransactions(&prepared_trans);
  ASSERT_EQ(prepared_trans.size(), 0);

//事务不应再可见
  ASSERT_EQ(db->GetTransactionByName("xid"), nullptr);

//堆不应该再关心准备好的部分了
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

//但是现在我们的memtable应该引用准备部分
  ASSERT_EQ(log_containing_prep,
            db_impl->TEST_FindMinPrepLogReferencedByMemTable());

  db_impl->TEST_FlushMemTable(true);

//在memtable刷新之后，我们现在可以释放日志
  ASSERT_EQ(0, db_impl->TEST_FindMinPrepLogReferencedByMemTable());

  delete txn;

//删除事务应注销事务
  ASSERT_EQ(db->GetTransactionByName("xid"), nullptr);
}

//TODO需要用串行提交更新此测试
TEST_P(TransactionTest, DISABLED_TwoPhaseMultiThreadTest) {
//混合事务写入和常规写入
  const uint32_t NUM_TXN_THREADS = 50;
  std::atomic<uint32_t> txn_thread_num(0);

  std::function<void()> txn_write_thread = [&]() {
    uint32_t id = txn_thread_num.fetch_add(1);

    WriteOptions write_options;
    write_options.sync = true;
    write_options.disableWAL = false;
    TransactionOptions txn_options;
    txn_options.lock_timeout = 1000000;
    if (id % 2 == 0) {
      txn_options.expiration = 1000000;
    }
    TransactionName name("xid_" + std::string(1, 'A' + id));
    Transaction* txn = db->BeginTransaction(write_options, txn_options);
    ASSERT_OK(txn->SetName(name));
    for (int i = 0; i < 10; i++) {
      std::string key(name + "_" + std::string(1, 'A' + i));
      ASSERT_OK(txn->Put(key, "val"));
    }
    ASSERT_OK(txn->Prepare());
    ASSERT_OK(txn->Commit());
    delete txn;
  };

//确保所有线程都在同一个写入组中
  std::atomic<uint32_t> t_wait_on_prepare(0);
  std::atomic<uint32_t> t_wait_on_commit(0);

  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "WriteThread::JoinBatchGroup:Wait", [&](void* arg) {
        auto* writer = reinterpret_cast<WriteThread::Writer*>(arg);

        if (writer->ShouldWriteToWAL()) {
          t_wait_on_prepare.fetch_add(1);
//等待朋友
          while (t_wait_on_prepare.load() < NUM_TXN_THREADS) {
            env->SleepForMicroseconds(10);
          }
        } else if (writer->ShouldWriteToMemtable()) {
          t_wait_on_commit.fetch_add(1);
//等待朋友
          while (t_wait_on_commit.load() < NUM_TXN_THREADS) {
            env->SleepForMicroseconds(10);
          }
        } else {
          FAIL();
        }
      });

  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

//做所有的写作
  std::vector<port::Thread> threads;
  for (uint32_t i = 0; i < NUM_TXN_THREADS; i++) {
    threads.emplace_back(txn_write_thread);
  }
  for (auto& t : threads) {
    t.join();
  }

  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
  rocksdb::SyncPoint::GetInstance()->ClearAllCallBacks();

  ReadOptions read_options;
  std::string value;
  Status s;
  for (uint32_t t = 0; t < NUM_TXN_THREADS; t++) {
    TransactionName name("xid_" + std::string(1, 'A' + t));
    for (int i = 0; i < 10; i++) {
      std::string key(name + "_" + std::string(1, 'A' + i));
      s = db->Get(read_options, key, &value);
      ASSERT_OK(s);
      ASSERT_EQ(value, "val");
    }
  }
}

TEST_P(TransactionTest, TwoPhaseLongPrepareTest) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = false;
  ReadOptions read_options;
  TransactionOptions txn_options;

  std::string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("bob");
  ASSERT_OK(s);

//交易投入
  s = txn->Put(Slice("foo"), Slice("bar"));
  ASSERT_OK(s);

//准备
  s = txn->Prepare();
  ASSERT_OK(s);

  delete txn;

  for (int i = 0; i < 1000; i++) {
    std::string key(i, 'k');
    std::string val(1000, 'v');
    s = db->Put(write_options, key, val);
    ASSERT_OK(s);

    if (i % 29 == 0) {
//崩溃
      env->SetFilesystemActive(false);
      ReOpenNoDelete();
    } else if (i % 37 == 0) {
//关闭
      ReOpenNoDelete();
    }
  }

//提交旧的TXN
  txn = db->GetTransactionByName("bob");
  ASSERT_TRUE(txn);
  s = txn->Commit();
  ASSERT_OK(s);

//验证数据txn数据
  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(s, Status::OK());
  ASSERT_EQ(value, "bar");

//验证非txn数据
  for (int i = 0; i < 1000; i++) {
    std::string key(i, 'k');
    std::string val(1000, 'v');
    s = db->Get(read_options, key, &value);
    ASSERT_EQ(s, Status::OK());
    ASSERT_EQ(value, val);
  }

  delete txn;
}

TEST_P(TransactionTest, TwoPhaseSequenceTest) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = false;
  ReadOptions read_options;

  TransactionOptions txn_options;

  std::string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("xid");
  ASSERT_OK(s);

//交易投入
  s = txn->Put(Slice("foo"), Slice("bar"));
  ASSERT_OK(s);
  s = txn->Put(Slice("foo2"), Slice("bar2"));
  ASSERT_OK(s);
  s = txn->Put(Slice("foo3"), Slice("bar3"));
  ASSERT_OK(s);
  s = txn->Put(Slice("foo4"), Slice("bar4"));
  ASSERT_OK(s);

//准备
  s = txn->Prepare();
  ASSERT_OK(s);

//作出承诺
  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;

//杀戮与重开
  env->SetFilesystemActive(false);
  ReOpenNoDelete();

//值现在可用
  s = db->Get(read_options, "foo4", &value);
  ASSERT_EQ(s, Status::OK());
  ASSERT_EQ(value, "bar4");
}

TEST_P(TransactionTest, TwoPhaseDoubleRecoveryTest) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = false;
  ReadOptions read_options;

  TransactionOptions txn_options;

  std::string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("a");
  ASSERT_OK(s);

//交易投入
  s = txn->Put(Slice("foo"), Slice("bar"));
  ASSERT_OK(s);

//准备
  s = txn->Prepare();
  ASSERT_OK(s);

  delete txn;

//杀戮与重开
  env->SetFilesystemActive(false);
  ReOpenNoDelete();

//提交旧的TXN
  txn = db->GetTransactionByName("a");
  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(s, Status::OK());
  ASSERT_EQ(value, "bar");

  delete txn;

  txn = db->BeginTransaction(write_options, txn_options);
  s = txn->SetName("b");
  ASSERT_OK(s);

  s = txn->Put(Slice("foo2"), Slice("bar2"));
  ASSERT_OK(s);

  s = txn->Prepare();
  ASSERT_OK(s);

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;

//杀戮与重开
  env->SetFilesystemActive(false);
  ReOpenNoDelete();

//值现在可用
  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(s, Status::OK());
  ASSERT_EQ(value, "bar");

  s = db->Get(read_options, "foo2", &value);
  ASSERT_EQ(s, Status::OK());
  ASSERT_EQ(value, "bar2");
}

TEST_P(TransactionTest, TwoPhaseLogRollingTest) {
  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

  Status s;
  string v;
  ColumnFamilyHandle *cfa, *cfb;

//创建2个新的柱族
  ColumnFamilyOptions cf_options;
  s = db->CreateColumnFamily(cf_options, "CFA", &cfa);
  ASSERT_OK(s);
  s = db->CreateColumnFamily(cf_options, "CFB", &cfb);
  ASSERT_OK(s);

  WriteOptions wopts;
  wopts.disableWAL = false;
  wopts.sync = true;

  TransactionOptions topts1;
  Transaction* txn1 = db->BeginTransaction(wopts, topts1);
  s = txn1->SetName("xid1");
  ASSERT_OK(s);

  TransactionOptions topts2;
  Transaction* txn2 = db->BeginTransaction(wopts, topts2);
  s = txn2->SetName("xid2");
  ASSERT_OK(s);

//事务放入两列族
  s = txn1->Put(cfa, "ka1", "va1");
  ASSERT_OK(s);

//事务放入两列族
  s = txn2->Put(cfa, "ka2", "va2");
  ASSERT_OK(s);
  s = txn2->Put(cfb, "kb2", "vb2");
  ASSERT_OK(s);

//将准备部分写入Wal
  s = txn1->Prepare();
  ASSERT_OK(s);

//我们的日志应该在堆中
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(),
            txn1->GetLogNumber());
  ASSERT_EQ(db_impl->TEST_LogfileNumber(), txn1->GetLogNumber());

//刷新默认CF以装入新日志
  s = db->Put(wopts, "foo", "bar");
  ASSERT_OK(s);
  s = db_impl->TEST_FlushMemTable(true);
  ASSERT_OK(s);

//确保我们在新的日志上
  ASSERT_GT(db_impl->TEST_LogfileNumber(), txn1->GetLogNumber());

//在此日志中放置txn2准备部分
  s = txn2->Prepare();
  ASSERT_OK(s);
  ASSERT_EQ(db_impl->TEST_LogfileNumber(), txn2->GetLogNumber());

//堆仍应看到第一个日志
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(),
            txn1->GetLogNumber());

//提交TXN1
  s = txn1->Commit();
  ASSERT_OK(s);

//堆现在应该显示txn2s日志
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(),
            txn2->GetLogNumber());

//我们应该看到memtables引用的txn1s日志
  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(),
            txn1->GetLogNumber());

//刷新默认CF以装入新日志
  s = db->Put(wopts, "foo", "bar2");
  ASSERT_OK(s);
  s = db_impl->TEST_FlushMemTable(true);
  ASSERT_OK(s);

//确保我们在新的日志上
  ASSERT_GT(db_impl->TEST_LogfileNumber(), txn2->GetLogNumber());

//提交TXN2
  s = txn2->Commit();
  ASSERT_OK(s);

//堆不应显示任何日志
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

//应该显示第一个txn日志
  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(),
            txn1->GetLogNumber());

//仅冲洗CFA MEMTABLE
  s = db_impl->TEST_FlushMemTable(true, cfa);
  ASSERT_OK(s);

//应该显示第一个txn日志
  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(),
            txn2->GetLogNumber());

//仅冲洗CFB MEMTABLE
  s = db_impl->TEST_FlushMemTable(true, cfb);
  ASSERT_OK(s);

//不应显示对日志的依赖性
  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(), 0);
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(), 0);

  delete txn1;
  delete txn2;
  delete cfa;
  delete cfb;
}

TEST_P(TransactionTest, TwoPhaseLogRollingTest2) {
  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

  Status s;
  ColumnFamilyHandle *cfa, *cfb;

  ColumnFamilyOptions cf_options;
  s = db->CreateColumnFamily(cf_options, "CFA", &cfa);
  ASSERT_OK(s);
  s = db->CreateColumnFamily(cf_options, "CFB", &cfb);
  ASSERT_OK(s);

  WriteOptions wopts;
  wopts.disableWAL = false;
  wopts.sync = true;

  auto cfh_a = reinterpret_cast<ColumnFamilyHandleImpl*>(cfa);
  auto cfh_b = reinterpret_cast<ColumnFamilyHandleImpl*>(cfb);

  TransactionOptions topts1;
  Transaction* txn1 = db->BeginTransaction(wopts, topts1);
  s = txn1->SetName("xid1");
  ASSERT_OK(s);
  s = txn1->Put(cfa, "boys", "girls1");
  ASSERT_OK(s);

  Transaction* txn2 = db->BeginTransaction(wopts, topts1);
  s = txn2->SetName("xid2");
  ASSERT_OK(s);
  s = txn2->Put(cfb, "up", "down1");
  ASSERT_OK(s);

//在日志A中预先处理事务
  s = txn1->Prepare();
  ASSERT_OK(s);

//在日志A中预先处理事务
  s = txn2->Prepare();
  ASSERT_OK(s);

//常规放置，以便实际冲洗MEM表进行日志滚动
  s = db->Put(wopts, "cats", "dogs1");
  ASSERT_OK(s);

  auto prepare_log_no = txn1->GetLogNumber();

//滚动到日志B
  s = db_impl->TEST_FlushMemTable(true);
  ASSERT_OK(s);

//现在我们暂停后台工作，以便
//在检查imm（）的状态之前，不会刷新它们
  s = db_impl->PauseBackgroundWork();
  ASSERT_OK(s);

  ASSERT_GT(db_impl->TEST_LogfileNumber(), prepare_log_no);
  ASSERT_GT(cfh_a->cfd()->GetLogNumber(), prepare_log_no);
  ASSERT_EQ(cfh_a->cfd()->GetLogNumber(), db_impl->TEST_LogfileNumber());
  ASSERT_EQ(db_impl->TEST_FindMinLogContainingOutstandingPrep(),
            prepare_log_no);
  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(), 0);

//在日志B中提交
  s = txn1->Commit();
  ASSERT_OK(s);

  ASSERT_EQ(db_impl->TEST_FindMinPrepLogReferencedByMemTable(), prepare_log_no);

  ASSERT_TRUE(!db_impl->TEST_UnableToFlushOldestLog());

//请求对所有列族进行刷新，以便
//活动日志文件可以被删除
  db_impl->TEST_HandleWALFull();
//无法刷新日志，因为txn2尚未提交
  ASSERT_TRUE(!db_impl->TEST_IsLogGettingFlushed());
  ASSERT_TRUE(db_impl->TEST_UnableToFlushOldestLog());

//断言CFA已请求刷新
  ASSERT_TRUE(cfh_a->cfd()->imm()->HasFlushRequested());

//CFB不应该被刷新，因为它没有来自日志A的数据
  ASSERT_TRUE(!cfh_b->cfd()->imm()->HasFlushRequested());

//CFB现在有来自日志A的数据
  s = txn2->Commit();
  ASSERT_OK(s);

  db_impl->TEST_HandleWALFull();
  ASSERT_TRUE(!db_impl->TEST_UnableToFlushOldestLog());

//我们应该看到CFB现在有一个冲洗请求
  ASSERT_TRUE(cfh_b->cfd()->imm()->HasFlushRequested());

//日志A中的所有数据都驻留在一个memtable中，
//要求冲洗
  ASSERT_TRUE(db_impl->TEST_IsLogGettingFlushed());

  delete txn1;
  delete txn2;
  delete cfa;
  delete cfb;
}
/*
 *1）使用“准备”保留第一个日志，以确定启动顺序。
 *在恢复过程中。
 *2）插入多个值，跳过wal，增加seqid。
 *3）在Wal中插入最终值
 *4）恢复并查看最终值是否正确恢复-未恢复
 *隐藏在不正确求和的序列ID后面
 **/

TEST_P(TransactionTest, TwoPhaseOutOfOrderDelete) {
  DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());
  WriteOptions wal_on, wal_off;
  wal_on.sync = true;
  wal_on.disableWAL = false;
  wal_off.disableWAL = true;
  ReadOptions read_options;
  TransactionOptions txn_options;

  std::string value;
  Status s;

  Transaction* txn1 = db->BeginTransaction(wal_on, txn_options);

  s = txn1->SetName("1");
  ASSERT_OK(s);

  s = db->Put(wal_on, "first", "first");
  ASSERT_OK(s);

  s = txn1->Put(Slice("dummy"), Slice("dummy"));
  ASSERT_OK(s);
  s = txn1->Prepare();
  ASSERT_OK(s);

  s = db->Put(wal_off, "cats", "dogs1");
  ASSERT_OK(s);
  s = db->Put(wal_off, "cats", "dogs2");
  ASSERT_OK(s);
  s = db->Put(wal_off, "cats", "dogs3");
  ASSERT_OK(s);

  s = db_impl->TEST_FlushMemTable(true);
  ASSERT_OK(s);

  s = db->Put(wal_on, "cats", "dogs4");
  ASSERT_OK(s);

  db->FlushWAL(false);

//杀戮与重开
  env->SetFilesystemActive(false);
  ReOpenNoDelete();

  s = db->Get(read_options, "first", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "first");

  s = db->Get(read_options, "cats", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "dogs4");
}

TEST_P(TransactionTest, FirstWriteTest) {
  WriteOptions write_options;

//根据对数据库的第一次写入测试冲突检查。
//事务的快照将具有seq 1和以下写入
//将有序列1。
  Status s = db->Put(write_options, "A", "a");

  Transaction* txn = db->BeginTransaction(write_options);
  txn->SetSnapshot();

  ASSERT_OK(s);

  s = txn->Put("A", "b");
  ASSERT_OK(s);

  delete txn;
}

TEST_P(TransactionTest, FirstWriteTest2) {
  WriteOptions write_options;

  Transaction* txn = db->BeginTransaction(write_options);
  txn->SetSnapshot();

//根据对数据库的第一次写入测试冲突检查。
//事务的快照为seq 0，而以下写入
//将有序列1。
  Status s = db->Put(write_options, "A", "a");
  ASSERT_OK(s);

  s = txn->Put("A", "b");
  ASSERT_TRUE(s.IsBusy());

  delete txn;
}

TEST_P(TransactionTest, WriteOptionsTest) {
  WriteOptions write_options;
  write_options.sync = true;
  write_options.disableWAL = true;

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  ASSERT_TRUE(txn->GetWriteOptions()->sync);

  write_options.sync = false;
  txn->SetWriteOptions(write_options);
  ASSERT_FALSE(txn->GetWriteOptions()->sync);
  ASSERT_TRUE(txn->GetWriteOptions()->disableWAL);

  delete txn;
}

TEST_P(TransactionTest, WriteConflictTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "A");
  db->Put(write_options, "foo2", "B");

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->Put("foo", "A2");
  ASSERT_OK(s);

  s = txn->Put("foo2", "B2");
  ASSERT_OK(s);

//此放在事务之外的操作将与上一次写入操作冲突
  s = db->Put(write_options, "foo", "xxx");
  ASSERT_TRUE(s.IsTimedOut());

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "A");

  s = txn->Commit();
  ASSERT_OK(s);

  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "A2");
  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "B2");

  delete txn;
}

TEST_P(TransactionTest, WriteConflictTest2) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "bar");

  txn_options.set_snapshot = true;
  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn);

//此放在事务之外的操作将与以后的写入操作冲突
  s = db->Put(write_options, "foo", "barz");
  ASSERT_OK(s);

  s = txn->Put("foo2", "X");
  ASSERT_OK(s);

  s = txn->Put("foo",
"bar2");  //与快照拍摄后完成的写入冲突
  ASSERT_TRUE(s.IsBusy());

  s = txn->Put("foo3", "Y");
  ASSERT_OK(s);

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");

  ASSERT_EQ(2, txn->GetNumKeys());

  s = txn->Commit();
ASSERT_OK(s);  //txn应该提交，但只写foo2和foo3

//验证事务是否写入foo2和foo3，而不是foo
  db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "barz");

  db->Get(read_options, "foo2", &value);
  ASSERT_EQ(value, "X");

  db->Get(read_options, "foo3", &value);
  ASSERT_EQ(value, "Y");

  delete txn;
}

TEST_P(TransactionTest, ReadConflictTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  db->Put(write_options, "foo", "bar");
  db->Put(write_options, "foo2", "bar");

  txn_options.set_snapshot = true;
  Transaction* txn = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn);

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

//此放在事务之外的操作将与以前的读取操作冲突
  s = db->Put(write_options, "foo", "barz");
  ASSERT_TRUE(s.IsTimedOut());

  s = db->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  s = txn->Get(read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;
}

TEST_P(TransactionTest, TxnOnlyTest) {
//测试以确保事务在
//空数据库。

  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->Put("x", "y");
  ASSERT_OK(s);

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;
}

TEST_P(TransactionTest, FlushTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  db->Put(write_options, Slice("foo"), Slice("bar"));
  db->Put(write_options, Slice("foo2"), Slice("bar"));

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn->GetForUpdate(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "bar");

  s = txn->Put(Slice("foo"), Slice("bar2"));
  ASSERT_OK(s);

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

TEST_P(TransactionTest, FlushTest2) {
  const size_t num_tests = 3;

  for (size_t n = 0; n < num_tests; n++) {
//测试不同的表工厂
    switch (n) {
      case 0:
        break;
      case 1:
        options.table_factory.reset(new mock::MockTableFactory());
        break;
      case 2: {
        PlainTableOptions pt_opts;
        pt_opts.hash_table_ratio = 0;
        options.table_factory.reset(NewPlainTableFactory(pt_opts));
        break;
      }
    }

    Status s = ReOpen();
    ASSERT_OK(s);

    WriteOptions write_options;
    ReadOptions read_options, snapshot_read_options;
    TransactionOptions txn_options;
    string value;

    DBImpl* db_impl = reinterpret_cast<DBImpl*>(db->GetRootDB());

    db->Put(write_options, Slice("foo"), Slice("bar"));
    db->Put(write_options, Slice("foo2"), Slice("bar2"));
    db->Put(write_options, Slice("foo3"), Slice("bar3"));

    txn_options.set_snapshot = true;
    Transaction* txn = db->BeginTransaction(write_options, txn_options);
    ASSERT_TRUE(txn);

    snapshot_read_options.snapshot = txn->GetSnapshot();

    txn->GetForUpdate(snapshot_read_options, "foo", &value);
    ASSERT_EQ(value, "bar");

    s = txn->Put(Slice("foo"), Slice("bar2"));
    ASSERT_OK(s);

    txn->GetForUpdate(snapshot_read_options, "foo", &value);
    ASSERT_EQ(value, "bar2");
//确认foo被txn锁定
    s = db->Delete(write_options, "foo");
    ASSERT_TRUE(s.IsTimedOut());

    s = db->Put(write_options, "Z", "z");
    ASSERT_OK(s);
    s = db->Put(write_options, "dummy", "dummy");
    ASSERT_OK(s);

    s = db->Put(write_options, "S", "s");
    ASSERT_OK(s);
    s = db->SingleDelete(write_options, "S");
    ASSERT_OK(s);

    s = txn->Delete("S");
//在遇到写入memtable中的s时失败
    ASSERT_TRUE(s.IsBusy());

//强制Memtable刷新
    s = db_impl->TEST_FlushMemTable(true);
    ASSERT_OK(s);

//放一个随机键，这样我们就有了一个可刷新的memtable
    s = db->Put(write_options, "dummy", "dummy2");
    ASSERT_OK(s);

//强制Memtable刷新
    ASSERT_OK(db_impl->TEST_FlushMemTable(true));

    s = db->Put(write_options, "dummy", "dummy3");
    ASSERT_OK(s);

//强制Memtable刷新
//由于测试数据库的max_write_buffer_number=2，因此此刷新将导致
//从MemTableList历史记录中清除的第一个MemTable。
    ASSERT_OK(db_impl->TEST_FlushMemTable(true));

    s = txn->Put("X", "Y");
//在确认sst文件中没有对x的写操作之后应该成功
    ASSERT_OK(s);

    s = txn->Put("Z", "zz");
//在sst文件中遇到对z的写操作后应失败
    ASSERT_TRUE(s.IsBusy());

    s = txn->GetForUpdate(read_options, "foo2", &value);
//应该成功，因为密钥是在txn启动之前写入的
    ASSERT_OK(s);
//确认foo2被txn锁定
    s = db->Delete(write_options, "foo2");
    ASSERT_TRUE(s.IsTimedOut());

    s = txn->Delete("S");
//在sst文件中遇到对s的写入后应失败
    ASSERT_TRUE(s.IsBusy());

//向数据库写入一组键以强制进行压缩
    Random rnd(47);
    for (int i = 0; i < 1000; i++) {
      s = db->Put(write_options, std::to_string(i),
                  test::CompressibleString(&rnd, 0.8, 100, &value));
      ASSERT_OK(s);
    }

    s = txn->Put("X", "yy");
//在确认sst文件中没有对x的写操作之后应该成功
    ASSERT_OK(s);

    s = txn->Put("Z", "zzz");
//在sst文件中遇到对z的写操作后应失败
    ASSERT_TRUE(s.IsBusy());

    s = txn->Delete("S");
//在sst文件中遇到对s的写入后应失败
    ASSERT_TRUE(s.IsBusy());

    s = txn->GetForUpdate(read_options, "foo3", &value);
//应该成功，因为密钥是在txn启动之前写入的
    ASSERT_OK(s);
//确认foo3被txn锁定
    s = db->Delete(write_options, "foo3");
    ASSERT_TRUE(s.IsTimedOut());

    db_impl->TEST_WaitForCompact();

    s = txn->Commit();
    ASSERT_OK(s);

//事务只应写入成功的密钥。
    s = db->Get(read_options, "foo", &value);
    ASSERT_EQ(value, "bar2");

    s = db->Get(read_options, "X", &value);
    ASSERT_OK(s);
    ASSERT_EQ("yy", value);

    s = db->Get(read_options, "Z", &value);
    ASSERT_OK(s);
    ASSERT_EQ("z", value);

  delete txn;
  }
}

TEST_P(TransactionTest, NoSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  db->Put(write_options, "AAA", "bar");

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

//事务启动后修改密钥
  db->Put(write_options, "AAA", "bar1");

//读写不需要快速
  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar1");
  s = txn->Put("AAA", "bar2");
  ASSERT_OK(s);

//应该提交，因为在数据更改后完成了读/写操作
  s = txn->Commit();
  ASSERT_OK(s);

  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar2");

  delete txn;
}

TEST_P(TransactionTest, MultipleSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  db->Put(write_options, "AAA", "bar");
  db->Put(write_options, "BBB", "bar");
  db->Put(write_options, "CCC", "bar");

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  db->Put(write_options, "AAA", "bar1");

//无快照读写
  txn->GetForUpdate(read_options, "AAA", &value);
  ASSERT_EQ(value, "bar1");
  s = txn->Put("AAA", "bar2");
  ASSERT_OK(s);

//在拍摄快照之前修改bbb
  db->Put(write_options, "BBB", "bar1");

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

//用快照读写
  txn->GetForUpdate(snapshot_read_options, "BBB", &value);
  ASSERT_EQ(value, "bar1");
  s = txn->Put("BBB", "bar2");
  ASSERT_OK(s);

  db->Put(write_options, "CCC", "bar1");

//设置新快照
  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

//用快照读写
  txn->GetForUpdate(snapshot_read_options, "CCC", &value);
  ASSERT_EQ(value, "bar1");
  s = txn->Put("CCC", "bar2");
  ASSERT_OK(s);

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
  txn = db->BeginTransaction(write_options);

//可能发生冲突的写入
  db->Put(write_options, "ZZZ", "zzz");
  db->Put(write_options, "XXX", "xxx");

  txn->SetSnapshot();

  TransactionOptions txn_options;
  txn_options.set_snapshot = true;
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  txn2->SetSnapshot();

//这不应在txn中冲突，因为快照晚于
//上一次写入（扰流器警报：稍后将与txn2冲突）。
  s = txn->Put("ZZZ", "zzzz");
  ASSERT_OK(s);

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;

//这将发生冲突，因为快照早于对zzz的另一次写入
  s = txn2->Put("ZZZ", "xxxxx");
  ASSERT_TRUE(s.IsBusy());

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "ZZZ", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "zzzz");

  delete txn2;
}

TEST_P(TransactionTest, ColumnFamiliesTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
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
  delete db;
  db = nullptr;

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

  s = TransactionDB::Open(options, txn_db_options, dbname, column_families,
                          &handles, &db);
  assert(db != nullptr);
  ASSERT_OK(s);

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn_options.set_snapshot = true;
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

//将一些数据写入数据库
  WriteBatch batch;
  batch.Put("foo", "foo");
  batch.Put(handles[1], "AAA", "bar");
  batch.Put(handles[1], "AAAZZZ", "bar");
  s = db->Write(write_options, &batch);
  ASSERT_OK(s);
  db->Delete(write_options, handles[1], "AAAZZZ");

//这些密钥不与现有写入冲突，因为它们位于
//不同的柱族
  s = txn->Delete("AAA");
  ASSERT_OK(s);
  s = txn->GetForUpdate(snapshot_read_options, handles[1], "foo", &value);
  ASSERT_TRUE(s.IsNotFound());
  Slice key_slice("AAAZZZ");
  Slice value_slices[2] = {Slice("bar"), Slice("bar")};
  s = txn->Put(handles[2], SliceParts(&key_slice, 1),
               SliceParts(value_slices, 2));
  ASSERT_OK(s);
  ASSERT_EQ(3, txn->GetNumKeys());

  s = txn->Commit();
  ASSERT_OK(s);
  s = db->Get(read_options, "AAA", &value);
  ASSERT_TRUE(s.IsNotFound());
  s = db->Get(read_options, handles[2], "AAAZZZ", &value);
  ASSERT_EQ(value, "barbar");

  Slice key_slices[3] = {Slice("AAA"), Slice("ZZ"), Slice("Z")};
  Slice value_slice("barbarbar");

  s = txn2->Delete(handles[2], "XXX");
  ASSERT_OK(s);
  s = txn2->Delete(handles[1], "XXX");
  ASSERT_OK(s);

//此写入将导致与早期批写入冲突
  s = txn2->Put(handles[1], SliceParts(key_slices, 3),
                SliceParts(&value_slice, 1));
  ASSERT_TRUE(s.IsBusy());

  s = txn2->Commit();
  ASSERT_OK(s);
//在上面，句柄[1]中对aaazzz的最新更改是删除。
  s = db->Get(read_options, handles[1], "AAAZZZ", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
  delete txn2;

  txn = db->BeginTransaction(write_options, txn_options);
  snapshot_read_options.snapshot = txn->GetSnapshot();

  txn2 = db->BeginTransaction(write_options, txn_options);
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

  s = txn->SingleDelete(handles[2], "ZZZ");
  ASSERT_OK(s);
  s = txn->Put(handles[2], "ZZZ", "YYY");
  ASSERT_OK(s);
  s = txn->Put(handles[2], "ZZZ", "YYYY");
  ASSERT_OK(s);
  s = txn->Delete(handles[2], "ZZZ");
  ASSERT_OK(s);
  s = txn->Put(handles[2], "AAAZZZ", "barbarbar");
  ASSERT_OK(s);

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
//由于发生冲突，所有结果都应该失败
  ASSERT_TRUE(results[0].IsBusy());
  ASSERT_TRUE(results[1].IsBusy());
  ASSERT_TRUE(results[2].IsBusy());
  ASSERT_TRUE(results[3].IsBusy());

  s = db->Get(read_options, handles[2], "foo", &value);
  ASSERT_EQ(value, "000");

  s = txn2->Commit();
  ASSERT_OK(s);

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

TEST_P(TransactionTest, ColumnFamiliesTest2) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  ColumnFamilyHandle *one, *two;
  ColumnFamilyOptions cf_options;

//创建2个新的柱族
  s = db->CreateColumnFamily(cf_options, "ONE", &one);
  ASSERT_OK(s);
  s = db->CreateColumnFamily(cf_options, "TWO", &two);
  ASSERT_OK(s);

  Transaction* txn1 = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn1);
  Transaction* txn2 = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn2);

  s = txn1->Put(one, "X", "1");
  ASSERT_OK(s);
  s = txn1->Put(two, "X", "2");
  ASSERT_OK(s);
  s = txn1->Put("X", "0");
  ASSERT_OK(s);

  s = txn2->Put(one, "X", "11");
  ASSERT_TRUE(s.IsTimedOut());

  s = txn1->Commit();
  ASSERT_OK(s);

//删除第一个列族
  s = db->DropColumnFamily(one);
  ASSERT_OK(s);

//应失败，因为列族已被删除。
  s = txn2->Commit();
  ASSERT_OK(s);

  delete txn1;
  txn1 = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn1);

//应失败，因为列族已被删除
  s = txn1->Put(one, "X", "111");
  ASSERT_TRUE(s.IsInvalidArgument());

  s = txn1->Put(two, "X", "222");
  ASSERT_OK(s);

  s = txn1->Put("X", "000");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, two, "X", &value);
  ASSERT_OK(s);
  ASSERT_EQ("222", value);

  s = db->Get(read_options, "X", &value);
  ASSERT_OK(s);
  ASSERT_EQ("000", value);

  s = db->DropColumnFamily(two);
  ASSERT_OK(s);

  delete txn1;
  delete txn2;

  delete one;
  delete two;
}

TEST_P(TransactionTest, EmptyTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  s = db->Put(write_options, "aaa", "aaa");
  ASSERT_OK(s);

  Transaction* txn = db->BeginTransaction(write_options);
  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  txn = db->BeginTransaction(write_options);
  txn->Rollback();
  delete txn;

  txn = db->BeginTransaction(write_options);
  s = txn->GetForUpdate(read_options, "aaa", &value);
  ASSERT_EQ(value, "aaa");

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  txn = db->BeginTransaction(write_options);
  txn->SetSnapshot();

  s = txn->GetForUpdate(read_options, "aaa", &value);
  ASSERT_EQ(value, "aaa");

//与以前的GetForUpdate冲突
  s = db->Put(write_options, "aaa", "xxx");
  ASSERT_TRUE(s.IsTimedOut());

//交易已过期！
  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;
}

TEST_P(TransactionTest, PredicateManyPreceders) {
  WriteOptions write_options;
  ReadOptions read_options1, read_options2;
  TransactionOptions txn_options;
  string value;
  Status s;

  txn_options.set_snapshot = true;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  Transaction* txn2 = db->BeginTransaction(write_options);
  txn2->SetSnapshot();
  read_options2.snapshot = txn2->GetSnapshot();

  std::vector<Slice> multiget_keys = {"1", "2", "3"};
  std::vector<std::string> multiget_values;

  std::vector<Status> results =
      txn1->MultiGetForUpdate(read_options1, multiget_keys, &multiget_values);
  ASSERT_TRUE(results[1].IsNotFound());

s = txn2->Put("2", "x");  //与txn1的multigetforupdate冲突
  ASSERT_TRUE(s.IsTimedOut());

  txn2->Rollback();

  multiget_values.clear();
  results =
      txn1->MultiGetForUpdate(read_options1, multiget_keys, &multiget_values);
  ASSERT_TRUE(results[1].IsNotFound());

  s = txn1->Commit();
  ASSERT_OK(s);

  delete txn1;
  delete txn2;

  txn1 = db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  s = txn1->Put("4", "x");
  ASSERT_OK(s);

s = txn2->Delete("4");  //冲突
  ASSERT_TRUE(s.IsTimedOut());

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->GetForUpdate(read_options2, "4", &value);
  ASSERT_TRUE(s.IsBusy());

  txn2->Rollback();

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, LostUpdate) {
  WriteOptions write_options;
  ReadOptions read_options, read_options1, read_options2;
  TransactionOptions txn_options;
  string value;
  Status s;

//测试2个事务在多个订单中写入同一个密钥，以及
//有/无快照

  Transaction* txn1 = db->BeginTransaction(write_options);
  Transaction* txn2 = db->BeginTransaction(write_options);

  s = txn1->Put("1", "1");
  ASSERT_OK(s);

s = txn2->Put("1", "2");  //冲突
  ASSERT_TRUE(s.IsTimedOut());

  s = txn2->Commit();
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ("1", value);

  delete txn1;
  delete txn2;

  txn_options.set_snapshot = true;
  txn1 = db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  s = txn1->Put("1", "3");
  ASSERT_OK(s);
s = txn2->Put("1", "4");  //冲突
  ASSERT_TRUE(s.IsTimedOut());

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ("3", value);

  delete txn1;
  delete txn2;

  txn1 = db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  s = txn1->Put("1", "5");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Put("1", "6");
  ASSERT_TRUE(s.IsBusy());
  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

  delete txn1;
  delete txn2;

  txn1 = db->BeginTransaction(write_options, txn_options);
  read_options1.snapshot = txn1->GetSnapshot();

  txn2 = db->BeginTransaction(write_options, txn_options);
  read_options2.snapshot = txn2->GetSnapshot();

  s = txn1->Put("1", "7");
  ASSERT_OK(s);
  s = txn1->Commit();
  ASSERT_OK(s);

  txn2->SetSnapshot();
  s = txn2->Put("1", "8");
  ASSERT_OK(s);
  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ("8", value);

  delete txn1;
  delete txn2;

  txn1 = db->BeginTransaction(write_options);
  txn2 = db->BeginTransaction(write_options);

  s = txn1->Put("1", "9");
  ASSERT_OK(s);
  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Put("1", "10");
  ASSERT_OK(s);
  s = txn2->Commit();
  ASSERT_OK(s);

  delete txn1;
  delete txn2;

  s = db->Get(read_options, "1", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "10");
}

TEST_P(TransactionTest, UntrackedWrites) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

//验证事务回滚是否适用于未跟踪的密钥。
  Transaction* txn = db->BeginTransaction(write_options);
  txn->SetSnapshot();

  s = txn->PutUntracked("untracked", "0");
  ASSERT_OK(s);
  txn->Rollback();
  s = db->Get(read_options, "untracked", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
  txn = db->BeginTransaction(write_options);
  txn->SetSnapshot();

  s = db->Put(write_options, "untracked", "x");
  ASSERT_OK(s);

//即使密钥是在快照后写入的，未跟踪的写入也应成功。
  s = txn->PutUntracked("untracked", "1");
  ASSERT_OK(s);
  s = txn->MergeUntracked("untracked", "2");
  ASSERT_OK(s);
  s = txn->DeleteUntracked("untracked");
  ASSERT_OK(s);

//冲突
  s = txn->Put("untracked", "3");
  ASSERT_TRUE(s.IsBusy());

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "untracked", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
}

TEST_P(TransactionTest, ExpiredTransaction) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

//将txn过期超时设置为0微秒（立即过期）
  txn_options.expiration = 0;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);

  s = txn1->Put("X", "1");
  ASSERT_OK(s);

  s = txn1->Put("Y", "1");
  ASSERT_OK(s);

  Transaction* txn2 = db->BeginTransaction(write_options);

//txn2应该能够写入x，因为txn1已经过期
  s = txn2->Put("X", "2");
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);
  s = db->Get(read_options, "X", &value);
  ASSERT_OK(s);
  ASSERT_EQ("2", value);

  s = txn1->Put("Z", "1");
  ASSERT_OK(s);

//txn1过期后应无法提交
  s = txn1->Commit();
  ASSERT_TRUE(s.IsExpired());

  s = db->Get(read_options, "Y", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = db->Get(read_options, "Z", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, ReinitializeTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

//将txn过期超时设置为0微秒（立即过期）
  txn_options.expiration = 0;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);

//重新初始化事务，使其不长时间过期
  txn_options.expiration = -1;
  txn1 = db->BeginTransaction(write_options, txn_options, txn1);

  s = txn1->Put("Z", "z");
  ASSERT_OK(s);

//应该提交，因为没有过期
  s = txn1->Commit();
  ASSERT_OK(s);

  txn1 = db->BeginTransaction(write_options, txn_options, txn1);

  s = txn1->Put("Z", "zz");
  ASSERT_OK(s);

//重新初始化txn1并确认Z已解锁
  txn1 = db->BeginTransaction(write_options, txn_options, txn1);

  Transaction* txn2 = db->BeginTransaction(write_options, txn_options, nullptr);
  s = txn2->Put("Z", "zzz");
  ASSERT_OK(s);
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = db->Get(read_options, "Z", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "zzz");

//验证快照是否已正确重新初始化
  txn1->SetSnapshot();
  s = txn1->Put("Z", "zzzz");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "Z", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "zzzz");

  txn1 = db->BeginTransaction(write_options, txn_options, txn1);
  const Snapshot* snapshot = txn1->GetSnapshot();
  ASSERT_FALSE(snapshot);

  txn_options.set_snapshot = true;
  txn1 = db->BeginTransaction(write_options, txn_options, txn1);
  snapshot = txn1->GetSnapshot();
  ASSERT_TRUE(snapshot);

  s = txn1->Put("Z", "a");
  ASSERT_OK(s);

  txn1->Rollback();

  s = txn1->Put("Y", "y");
  ASSERT_OK(s);

  txn_options.set_snapshot = false;
  txn1 = db->BeginTransaction(write_options, txn_options, txn1);
  snapshot = txn1->GetSnapshot();
  ASSERT_FALSE(snapshot);

  s = txn1->Put("X", "x");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "Z", &value);
  ASSERT_OK(s);
  ASSERT_EQ(value, "zzzz");

  s = db->Get(read_options, "Y", &value);
  ASSERT_TRUE(s.IsNotFound());

  txn1 = db->BeginTransaction(write_options, txn_options, txn1);

  s = txn1->SetName("name");
  ASSERT_OK(s);

  s = txn1->Prepare();
  ASSERT_OK(s);
  s = txn1->Commit();
  ASSERT_OK(s);

  txn1 = db->BeginTransaction(write_options, txn_options, txn1);

  s = txn1->SetName("name");
  ASSERT_OK(s);

  delete txn1;
}

TEST_P(TransactionTest, Rollback) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);

  ASSERT_OK(s);

  s = txn1->Put("X", "1");
  ASSERT_OK(s);

  Transaction* txn2 = db->BeginTransaction(write_options);

//由于txn1已锁定，txn2不能写入x
  s = txn2->Put("X", "2");
  ASSERT_TRUE(s.IsTimedOut());

  txn1->Rollback();
  delete txn1;

//txn2现在应该可以写入x
  s = txn2->Put("X", "3");
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "X", &value);
  ASSERT_OK(s);
  ASSERT_EQ("3", value);

  delete txn2;
}

TEST_P(TransactionTest, LockLimitTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  delete db;
  db = nullptr;

//打开DB，锁定限制为3
  txn_db_options.max_num_locks = 3;
  s = TransactionDB::Open(options, txn_db_options, dbname, &db);
  assert(db != nullptr);
  ASSERT_OK(s);

//创建一个txn并验证我们最多只能锁定3个键
  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->Put("X", "x");
  ASSERT_OK(s);

  s = txn->Put("Y", "y");
  ASSERT_OK(s);

  s = txn->Put("Z", "z");
  ASSERT_OK(s);

//达到锁定限制
  s = txn->Put("W", "w");
  ASSERT_TRUE(s.IsBusy());

//重新锁定同一把钥匙不应该让我们超过限制
  s = txn->Put("X", "xx");
  ASSERT_OK(s);

  s = txn->GetForUpdate(read_options, "W", &value);
  ASSERT_TRUE(s.IsBusy());
  s = txn->GetForUpdate(read_options, "V", &value);
  ASSERT_TRUE(s.IsBusy());

//重新锁定同一把钥匙不应该让我们超过限制
  s = txn->GetForUpdate(read_options, "Y", &value);
  ASSERT_OK(s);
  ASSERT_EQ("y", value);

  s = txn->Get(read_options, "W", &value);
  ASSERT_TRUE(s.IsNotFound());

  Transaction* txn2 = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn2);

//“X”当前锁定
  s = txn2->Put("X", "x");
  ASSERT_TRUE(s.IsTimedOut());

//达到锁定限制
  s = txn2->Put("M", "m");
  ASSERT_TRUE(s.IsBusy());

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "X", &value);
  ASSERT_OK(s);
  ASSERT_EQ("xx", value);

  s = db->Get(read_options, "W", &value);
  ASSERT_TRUE(s.IsNotFound());

//提交txn应释放其锁并允许txn2继续
  s = txn2->Put("X", "x2");
  ASSERT_OK(s);

  s = txn2->Delete("X");
  ASSERT_OK(s);

  s = txn2->Put("M", "m");
  ASSERT_OK(s);

  s = txn2->Put("Z", "z2");
  ASSERT_OK(s);

//达到锁定限制
  s = txn2->Delete("Y");
  ASSERT_TRUE(s.IsBusy());

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "Z", &value);
  ASSERT_OK(s);
  ASSERT_EQ("z2", value);

  s = db->Get(read_options, "Y", &value);
  ASSERT_OK(s);
  ASSERT_EQ("y", value);

  s = db->Get(read_options, "X", &value);
  ASSERT_TRUE(s.IsNotFound());

  delete txn;
  delete txn2;
}

TEST_P(TransactionTest, IteratorTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
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

  Transaction* txn = db->BeginTransaction(write_options);
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
    if (i == 2) {
//在txn的快照之后修改了“c”
      ASSERT_TRUE(s.IsBusy());
    } else {
      ASSERT_OK(s);
    }

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

  s = txn->Commit();
  ASSERT_OK(s);

  delete iter;
  delete txn;
}

TEST_P(TransactionTest, DisableIndexingTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  txn->DisableIndexing();

  s = txn->Put("B", "b");
  ASSERT_OK(s);

  s = txn->Get(read_options, "B", &value);
  ASSERT_TRUE(s.IsNotFound());

  Iterator* iter = txn->GetIterator(read_options);
  ASSERT_OK(iter->status());

  iter->Seek("B");
  ASSERT_OK(iter->status());
  ASSERT_FALSE(iter->Valid());

  s = txn->Delete("A");

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  txn->EnableIndexing();

  s = txn->Put("B", "bb");
  ASSERT_OK(s);

  iter->Seek("B");
  ASSERT_OK(iter->status());
  ASSERT_TRUE(iter->Valid());
  ASSERT_EQ("bb", iter->value().ToString());

  s = txn->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("bb", value);

  s = txn->Put("A", "aa");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("aa", value);

  delete iter;
  delete txn;
}

TEST_P(TransactionTest, SavepointTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  ASSERT_EQ(0, txn->GetNumPuts());

  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());

txn->SetSavePoint();  //一

ASSERT_OK(txn->RollbackToSavePoint());  //回滚到txn的开头
  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Put("B", "b");
  ASSERT_OK(s);

  ASSERT_EQ(1, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

  s = txn->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("b", value);

  delete txn;
  txn = db->BeginTransaction(write_options);
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

  ASSERT_EQ(5, txn->GetNumPuts());
  ASSERT_EQ(1, txn->GetNumDeletes());

ASSERT_OK(txn->RollbackToSavePoint());  //回滚至2

  ASSERT_EQ(3, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

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

  ASSERT_EQ(5, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

//回滚到txn的开头
  s = txn->RollbackToSavePoint();
  ASSERT_TRUE(s.IsNotFound());
  txn->Rollback();

  ASSERT_EQ(0, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

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

  ASSERT_EQ(2, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

txn->SetSavePoint();  //三
txn->SetSavePoint();  //四

  s = txn->Put("G", "g");
  ASSERT_OK(s);

  s = txn->SingleDelete("F");
  ASSERT_OK(s);

  s = txn->Delete("B");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("aa", value);

  s = txn->Get(read_options, "F", &value);
//根据db.h，对
//覆盖将具有未定义的行为。所以还不清楚
//提取“f”的结果应为。当前的实施将
//在这种情况下，返回NotFound。
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Get(read_options, "B", &value);
  ASSERT_TRUE(s.IsNotFound());

  ASSERT_EQ(3, txn->GetNumPuts());
  ASSERT_EQ(2, txn->GetNumDeletes());

ASSERT_OK(txn->RollbackToSavePoint());  //回滚至3

  ASSERT_EQ(2, txn->GetNumPuts());
  ASSERT_EQ(0, txn->GetNumDeletes());

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

TEST_P(TransactionTest, SavepointTest2) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  Status s;

txn_options.lock_timeout = 1;  //1毫秒
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);

  s = txn1->Put("A", "");
  ASSERT_OK(s);

txn1->SetSavePoint();  //一

  s = txn1->Put("A", "a");
  ASSERT_OK(s);

  s = txn1->Put("C", "c");
  ASSERT_OK(s);

txn1->SetSavePoint();  //二

  s = txn1->Put("A", "a");
  ASSERT_OK(s);
  s = txn1->Put("B", "b");
  ASSERT_OK(s);

ASSERT_OK(txn1->RollbackToSavePoint());  //回滚至2

//确认“A”和“C”仍处于锁定状态，而“B”未处于锁定状态
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

  s = txn2->Put("A", "a2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b2");
  ASSERT_OK(s);

  s = txn1->Put("A", "aa");
  ASSERT_OK(s);
  s = txn1->Put("B", "bb");
  ASSERT_TRUE(s.IsTimedOut());

  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = txn1->Put("A", "aaa");
  ASSERT_OK(s);
  s = txn1->Put("B", "bbb");
  ASSERT_OK(s);
  s = txn1->Put("C", "ccc");
  ASSERT_OK(s);

txn1->SetSavePoint();                    //三
ASSERT_OK(txn1->RollbackToSavePoint());  //回滚至3

//确认“A”、“B”、“C”仍然锁定
  txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

  s = txn2->Put("A", "a2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c2");
  ASSERT_TRUE(s.IsTimedOut());

ASSERT_OK(txn1->RollbackToSavePoint());  //回滚至1

//确认只有“A”被锁定
  s = txn2->Put("A", "a3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b3");
  ASSERT_OK(s);
  s = txn2->Put("C", "c3po");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;

//确认“A”“C”“B”不再锁定
  s = txn2->Put("A", "a4");
  ASSERT_OK(s);
  s = txn2->Put("B", "b4");
  ASSERT_OK(s);
  s = txn2->Put("C", "c4");
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;
}

TEST_P(TransactionTest, UndoGetForUpdateTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

txn_options.lock_timeout = 1;  //1毫秒
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);

  txn1->UndoGetForUpdate("A");

  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;

  txn1 = db->BeginTransaction(write_options, txn_options);

  txn1->UndoGetForUpdate("A");
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

//确认A已锁定
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  s = txn2->Put("A", "a");
  ASSERT_TRUE(s.IsTimedOut());

  txn1->UndoGetForUpdate("A");

//确认A现在已解锁
  s = txn2->Put("A", "a2");
  ASSERT_OK(s);
  txn2->Commit();
  delete txn2;
  s = db->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a2", value);

  s = txn1->Delete("A");
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn1->Put("B", "b3");
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "B", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");

//确认A和B仍然锁定
  txn2 = db->BeginTransaction(write_options, txn_options);
  s = txn2->Put("A", "a4");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b4");
  ASSERT_TRUE(s.IsTimedOut());

  txn1->Rollback();
  delete txn1;

//确认A和B不再锁定
  s = txn2->Put("A", "a5");
  ASSERT_OK(s);
  s = txn2->Put("B", "b5");
  ASSERT_OK(s);
  s = txn2->Commit();
  delete txn2;
  ASSERT_OK(s);

  txn1 = db->BeginTransaction(write_options, txn_options);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "C", &value);
  ASSERT_TRUE(s.IsNotFound());
  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "C", &value);
  ASSERT_TRUE(s.IsNotFound());
  s = txn1->GetForUpdate(read_options, "B", &value);
  ASSERT_OK(s);
  s = txn1->Put("B", "b5");
  s = txn1->GetForUpdate(read_options, "B", &value);
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("X");

//确认A、B、C已锁定
  txn2 = db->BeginTransaction(write_options, txn_options);
  s = txn2->Put("A", "a6");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Delete("B");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c6");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("X", "x6");
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("X");

//确认A、B已锁定，C未锁定
  s = txn2->Put("A", "a6");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Delete("B");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c6");
  ASSERT_OK(s);
  s = txn2->Put("X", "x6");
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("X");

//确认B已锁定，A和C未锁定
  s = txn2->Put("A", "a7");
  ASSERT_OK(s);
  s = txn2->Delete("B");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c7");
  ASSERT_OK(s);
  s = txn2->Put("X", "x7");
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;
}

TEST_P(TransactionTest, UndoGetForUpdateTest2) {
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  string value;
  Status s;

  s = db->Put(write_options, "A", "");
  ASSERT_OK(s);

txn_options.lock_timeout = 1;  //1毫秒
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn1);

  s = txn1->GetForUpdate(read_options, "A", &value);
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "B", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn1->Put("F", "f");
  ASSERT_OK(s);

txn1->SetSavePoint();  //一

  txn1->UndoGetForUpdate("A");

  s = txn1->GetForUpdate(read_options, "C", &value);
  ASSERT_TRUE(s.IsNotFound());
  s = txn1->GetForUpdate(read_options, "D", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn1->Put("E", "e");
  ASSERT_OK(s);
  s = txn1->GetForUpdate(read_options, "E", &value);
  ASSERT_OK(s);

  s = txn1->GetForUpdate(read_options, "F", &value);
  ASSERT_OK(s);

//验证A、B、C、D、E、F是否仍然锁定
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  s = txn2->Put("A", "a1");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b1");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c1");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("D", "d1");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("E", "e1");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f1");
  ASSERT_TRUE(s.IsTimedOut());

  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("E");

//验证A、B、D、E、F是否仍然锁定，C是否锁定。
  s = txn2->Put("A", "a2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("D", "d2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("E", "e2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f2");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c2");
  ASSERT_OK(s);

txn1->SetSavePoint();  //二

  s = txn1->Put("H", "h");
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("D");
  txn1->UndoGetForUpdate("E");
  txn1->UndoGetForUpdate("F");
  txn1->UndoGetForUpdate("G");
  txn1->UndoGetForUpdate("H");

//验证A、B、D、E、F、H是否仍然锁定，C、G是否锁定。
  s = txn2->Put("A", "a3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("D", "d3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("E", "e3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("H", "h3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c3");
  ASSERT_OK(s);
  s = txn2->Put("G", "g3");
  ASSERT_OK(s);

txn1->RollbackToSavePoint();  //回滚至2

//验证A、B、D、E、F是否仍然锁定，C、G、H是否锁定。
  s = txn2->Put("A", "a3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("D", "d3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("E", "e3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c3");
  ASSERT_OK(s);
  s = txn2->Put("G", "g3");
  ASSERT_OK(s);
  s = txn2->Put("H", "h3");
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("D");
  txn1->UndoGetForUpdate("E");
  txn1->UndoGetForUpdate("F");
  txn1->UndoGetForUpdate("G");
  txn1->UndoGetForUpdate("H");

//验证A、B、E、F是否仍然锁定，C、D、G、H是否锁定。
  s = txn2->Put("A", "a3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("E", "e3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c3");
  ASSERT_OK(s);
  s = txn2->Put("D", "d3");
  ASSERT_OK(s);
  s = txn2->Put("G", "g3");
  ASSERT_OK(s);
  s = txn2->Put("H", "h3");
  ASSERT_OK(s);

txn1->RollbackToSavePoint();  //回滚至1

//验证A、B、F是否仍然锁定，C、D、E、G、H是否锁定。
  s = txn2->Put("A", "a3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("B", "b3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("F", "f3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("C", "c3");
  ASSERT_OK(s);
  s = txn2->Put("D", "d3");
  ASSERT_OK(s);
  s = txn2->Put("E", "e3");
  ASSERT_OK(s);
  s = txn2->Put("G", "g3");
  ASSERT_OK(s);
  s = txn2->Put("H", "h3");
  ASSERT_OK(s);

  txn1->UndoGetForUpdate("A");
  txn1->UndoGetForUpdate("B");
  txn1->UndoGetForUpdate("C");
  txn1->UndoGetForUpdate("D");
  txn1->UndoGetForUpdate("E");
  txn1->UndoGetForUpdate("F");
  txn1->UndoGetForUpdate("G");
  txn1->UndoGetForUpdate("H");

//验证F是否仍然锁定，A、B、C、D、E、G、H是否锁定。
  s = txn2->Put("F", "f3");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Put("A", "a3");
  ASSERT_OK(s);
  s = txn2->Put("B", "b3");
  ASSERT_OK(s);
  s = txn2->Put("C", "c3");
  ASSERT_OK(s);
  s = txn2->Put("D", "d3");
  ASSERT_OK(s);
  s = txn2->Put("E", "e3");
  ASSERT_OK(s);
  s = txn2->Put("G", "g3");
  ASSERT_OK(s);
  s = txn2->Put("H", "h3");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);
  s = txn2->Commit();
  ASSERT_OK(s);

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, TimeoutTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  delete db;
  db = nullptr;

//事务写入具有无限超时，
//但是我们会在启动TXN时覆盖这个
//DB写入有无限超时
  txn_db_options.transaction_lock_timeout = -1;
  txn_db_options.default_lock_timeout = -1;

  s = TransactionDB::Open(options, txn_db_options, dbname, &db);
  assert(db != nullptr);
  ASSERT_OK(s);

  s = db->Put(write_options, "aaa", "aaa");
  ASSERT_OK(s);

  TransactionOptions txn_options0;
txn_options0.expiration = 100;  //100毫秒
txn_options0.lock_timeout = 50;  //txn超时不再无限
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options0);

  s = txn1->GetForUpdate(read_options, "aaa", nullptr);
  ASSERT_OK(s);

//与以前的GetForUpdate冲突。
//由于db写入没有超时，因此当
//交易到期。
  s = db->Put(write_options, "aaa", "xxx");
  ASSERT_OK(s);

  ASSERT_GE(txn1->GetElapsedTime(),
            static_cast<uint64_t>(txn_options0.expiration));

  s = txn1->Commit();
ASSERT_TRUE(s.IsExpired());  //期满！

  s = db->Get(read_options, "aaa", &value);
  ASSERT_OK(s);
  ASSERT_EQ("xxx", value);

  delete txn1;
  delete db;

//事务写入有10毫秒超时，
//DB写入有无限超时
  txn_db_options.transaction_lock_timeout = 50;
  txn_db_options.default_lock_timeout = -1;

  s = TransactionDB::Open(options, txn_db_options, dbname, &db);
  ASSERT_OK(s);

  s = db->Put(write_options, "aaa", "aaa");
  ASSERT_OK(s);

  TransactionOptions txn_options;
txn_options.expiration = 100;  //100毫秒
  txn1 = db->BeginTransaction(write_options, txn_options);

  s = txn1->GetForUpdate(read_options, "aaa", nullptr);
  ASSERT_OK(s);

//与以前的GetForUpdate冲突。
//由于db写入没有超时，因此当
//交易到期。
  s = db->Put(write_options, "aaa", "xxx");
  ASSERT_OK(s);

  s = txn1->Commit();
ASSERT_NOK(s);  //期满！

  s = db->Get(read_options, "aaa", &value);
  ASSERT_OK(s);
  ASSERT_EQ("xxx", value);

  delete txn1;
txn_options.expiration = 6000000;  //100分钟
txn_options.lock_timeout = 1;      //千分之一秒
  txn1 = db->BeginTransaction(write_options, txn_options);
  txn1->SetLockTimeout(100);

  TransactionOptions txn_options2;
txn_options2.expiration = 10;  //10ms
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options2);
  ASSERT_OK(s);

  s = txn2->Put("a", "2");
  ASSERT_OK(s);

//txn1的锁定超时比txn2的到期时间长，因此它将获胜
  s = txn1->Delete("a");
  ASSERT_OK(s);

  s = txn1->Commit();
  ASSERT_OK(s);

//txn2应该过期，因为txn1正在等待超时。
  s = txn2->Commit();
  ASSERT_TRUE(s.IsExpired());

  delete txn1;
  delete txn2;
txn_options.expiration = 6000000;  //100分钟
  txn1 = db->BeginTransaction(write_options, txn_options);
  txn_options2.expiration = 100000000;
  txn2 = db->BeginTransaction(write_options, txn_options2);

  s = txn1->Delete("asdf");
  ASSERT_OK(s);

//txn2的锁超时比txn1的到期时间小，因此它将超时
  s = txn2->Delete("asdf");
  ASSERT_TRUE(s.IsTimedOut());
  ASSERT_EQ(s.ToString(), "Operation timed out: Timeout waiting to lock key");

  s = txn1->Commit();
  ASSERT_OK(s);

  s = txn2->Put("asdf", "asdf");
  ASSERT_OK(s);

  s = txn2->Commit();
  ASSERT_OK(s);

  s = db->Get(read_options, "asdf", &value);
  ASSERT_OK(s);
  ASSERT_EQ("asdf", value);

  delete txn1;
  delete txn2;
}

TEST_P(TransactionTest, SingleDeleteTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = txn->SingleDelete("A");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  txn = db->BeginTransaction(write_options);

  s = txn->SingleDelete("A");
  ASSERT_OK(s);

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  s = db->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  txn = db->BeginTransaction(write_options);

  s = txn->SingleDelete("A");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  s = db->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  txn = db->BeginTransaction(write_options);
  Transaction* txn2 = db->BeginTransaction(write_options);
  txn2->SetSnapshot();

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Put("A", "a2");
  ASSERT_OK(s);

  s = txn->SingleDelete("A");
  ASSERT_OK(s);

  s = txn->SingleDelete("B");
  ASSERT_OK(s);

//根据db.h，对
//覆盖将具有未定义的行为。所以还不清楚
//获取“A”的结果应该是。当前的实施将
//在这种情况下，返回NotFound。
  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = txn2->Put("B", "b");
  ASSERT_TRUE(s.IsTimedOut());
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

//根据db.h，对
//覆盖将具有未定义的行为。所以还不清楚
//获取“A”的结果应该是。当前的实施将
//在这种情况下，返回NotFound。
  s = db->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsNotFound());

  s = db->Get(read_options, "B", &value);
  ASSERT_TRUE(s.IsNotFound());
}

TEST_P(TransactionTest, MergeTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(write_options, TransactionOptions());
  ASSERT_TRUE(txn);

  s = db->Put(write_options, "A", "a0");
  ASSERT_OK(s);

  s = txn->Merge("A", "1");
  ASSERT_OK(s);

  s = txn->Merge("A", "2");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsMergeInProgress());

  s = txn->Put("A", "a");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a", value);

  s = txn->Merge("A", "3");
  ASSERT_OK(s);

  s = txn->Get(read_options, "A", &value);
  ASSERT_TRUE(s.IsMergeInProgress());

  TransactionOptions txn_options;
txn_options.lock_timeout = 1;  //1毫秒
  Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
  ASSERT_TRUE(txn2);

//确认TXN已锁定“A”
  s = txn2->Merge("A", "4");
  ASSERT_TRUE(s.IsTimedOut());

  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = txn->Commit();
  ASSERT_OK(s);
  delete txn;

  s = db->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a,3", value);
}

TEST_P(TransactionTest, DeferSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;
  Status s;

  s = db->Put(write_options, "A", "a0");
  ASSERT_OK(s);

  Transaction* txn1 = db->BeginTransaction(write_options);
  Transaction* txn2 = db->BeginTransaction(write_options);

  txn1->SetSnapshotOnNextOperation();
  auto snapshot = txn1->GetSnapshot();
  ASSERT_FALSE(snapshot);

  s = txn2->Put("A", "a2");
  ASSERT_OK(s);
  s = txn2->Commit();
  ASSERT_OK(s);
  delete txn2;

  s = txn1->GetForUpdate(read_options, "A", &value);
//不应与txn2冲突，因为快照在之前未设置
//调用了GetForUpdate。
  ASSERT_OK(s);
  ASSERT_EQ("a2", value);

  s = txn1->Put("A", "a1");
  ASSERT_OK(s);

  s = db->Put(write_options, "B", "b0");
  ASSERT_OK(s);

//无法锁定B，因为它是在设置快照后写入的
  s = txn1->Put("B", "b1");
  ASSERT_TRUE(s.IsBusy());

  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;

  s = db->Get(read_options, "A", &value);
  ASSERT_OK(s);
  ASSERT_EQ("a1", value);

  s = db->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_EQ("b0", value);
}

TEST_P(TransactionTest, DeferSnapshotTest2) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  Transaction* txn1 = db->BeginTransaction(write_options);

  txn1->SetSnapshot();

  s = txn1->Put("A", "a1");
  ASSERT_OK(s);

  s = db->Put(write_options, "C", "c0");
  ASSERT_OK(s);
  s = db->Put(write_options, "D", "d0");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();

  txn1->SetSnapshotOnNextOperation();

  s = txn1->Get(snapshot_read_options, "C", &value);
//快照是在写入C之前设置的
  ASSERT_TRUE(s.IsNotFound());
  s = txn1->Get(snapshot_read_options, "D", &value);
//快照是在写入d之前设置的
  ASSERT_TRUE(s.IsNotFound());

//快照尚未更改。
  snapshot_read_options.snapshot = txn1->GetSnapshot();

  s = txn1->Get(snapshot_read_options, "C", &value);
//快照是在写入C之前设置的
  ASSERT_TRUE(s.IsNotFound());
  s = txn1->Get(snapshot_read_options, "D", &value);
//快照是在写入d之前设置的
  ASSERT_TRUE(s.IsNotFound());

  s = txn1->GetForUpdate(read_options, "C", &value);
  ASSERT_OK(s);
  ASSERT_EQ("c0", value);

  s = db->Put(write_options, "D", "d00");
  ASSERT_OK(s);

//现在已设置快照
  snapshot_read_options.snapshot = txn1->GetSnapshot();
  s = txn1->Get(snapshot_read_options, "D", &value);
  ASSERT_OK(s);
  ASSERT_EQ("d0", value);

  s = txn1->Commit();
  ASSERT_OK(s);
  delete txn1;
}

TEST_P(TransactionTest, DeferSnapshotSavePointTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  Transaction* txn1 = db->BeginTransaction(write_options);

txn1->SetSavePoint();  //一

  s = db->Put(write_options, "T", "1");
  ASSERT_OK(s);

  txn1->SetSnapshotOnNextOperation();

  s = db->Put(write_options, "T", "2");
  ASSERT_OK(s);

txn1->SetSavePoint();  //二

  s = db->Put(write_options, "T", "3");
  ASSERT_OK(s);

  s = txn1->Put("A", "a");
  ASSERT_OK(s);

txn1->SetSavePoint();  //三

  s = db->Put(write_options, "T", "4");
  ASSERT_OK(s);

  txn1->SetSnapshot();
  txn1->SetSnapshotOnNextOperation();

txn1->SetSavePoint();  //四

  s = db->Put(write_options, "T", "5");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("4", value);

  s = txn1->Put("A", "a1");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

s = txn1->RollbackToSavePoint();  //回滚至4
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("4", value);

s = txn1->RollbackToSavePoint();  //回滚至3
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("3", value);

  s = txn1->Get(read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

s = txn1->RollbackToSavePoint();  //回滚至2
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  ASSERT_FALSE(snapshot_read_options.snapshot);
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

  s = txn1->Delete("A");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  ASSERT_TRUE(snapshot_read_options.snapshot);
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

s = txn1->RollbackToSavePoint();  //回滚至1
  ASSERT_OK(s);

  s = txn1->Delete("A");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn1->GetSnapshot();
  ASSERT_FALSE(snapshot_read_options.snapshot);
  s = txn1->Get(snapshot_read_options, "T", &value);
  ASSERT_OK(s);
  ASSERT_EQ("5", value);

  s = txn1->Commit();
  ASSERT_OK(s);

  delete txn1;
}

TEST_P(TransactionTest, SetSnapshotOnNextOperationWithNotification) {
  WriteOptions write_options;
  ReadOptions read_options;
  string value;

  class Notifier : public TransactionNotifier {
   private:
    const Snapshot** snapshot_ptr_;

   public:
    explicit Notifier(const Snapshot** snapshot_ptr)
        : snapshot_ptr_(snapshot_ptr) {}

    void SnapshotCreated(const Snapshot* newSnapshot) {
      *snapshot_ptr_ = newSnapshot;
    }
  };

  std::shared_ptr<Notifier> notifier =
      std::make_shared<Notifier>(&read_options.snapshot);
  Status s;

  s = db->Put(write_options, "B", "0");
  ASSERT_OK(s);

  Transaction* txn1 = db->BeginTransaction(write_options);

  txn1->SetSnapshotOnNextOperation(notifier);
  ASSERT_FALSE(read_options.snapshot);

  s = db->Put(write_options, "B", "1");
  ASSERT_OK(s);

//get不生成快照
  s = txn1->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_FALSE(read_options.snapshot);
  ASSERT_EQ(value, "1");

//任何其他操作都可以
  s = txn1->Put("A", "0");
  ASSERT_OK(s);

//现在更改“B”。
  s = db->Put(write_options, "B", "2");
  ASSERT_OK(s);

//仍应读取原始值
  s = txn1->Get(read_options, "B", &value);
  ASSERT_OK(s);
  ASSERT_TRUE(read_options.snapshot);
  ASSERT_EQ(value, "1");

  s = txn1->Commit();
  ASSERT_OK(s);

  delete txn1;
}

TEST_P(TransactionTest, ClearSnapshotTest) {
  WriteOptions write_options;
  ReadOptions read_options, snapshot_read_options;
  string value;
  Status s;

  s = db->Put(write_options, "foo", "0");
  ASSERT_OK(s);

  Transaction* txn = db->BeginTransaction(write_options);
  ASSERT_TRUE(txn);

  s = db->Put(write_options, "foo", "1");
  ASSERT_OK(s);

  snapshot_read_options.snapshot = txn->GetSnapshot();
  ASSERT_FALSE(snapshot_read_options.snapshot);

//尚未创建快照
  s = txn->Get(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "1");

  txn->SetSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();
  ASSERT_TRUE(snapshot_read_options.snapshot);

  s = db->Put(write_options, "foo", "2");
  ASSERT_OK(s);

//快照是在更改为“2”之前创建的
  s = txn->Get(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "1");

  txn->ClearSnapshot();
  snapshot_read_options.snapshot = txn->GetSnapshot();
  ASSERT_FALSE(snapshot_read_options.snapshot);

//快照现在已清除
  s = txn->Get(snapshot_read_options, "foo", &value);
  ASSERT_EQ(value, "2");

  s = txn->Commit();
  ASSERT_OK(s);

  delete txn;
}

TEST_P(TransactionTest, ToggleAutoCompactionTest) {
  Status s;

  TransactionOptions txn_options;
  ColumnFamilyHandle *cfa, *cfb;
  ColumnFamilyOptions cf_options;

//创建2个新的柱族
  s = db->CreateColumnFamily(cf_options, "CFA", &cfa);
  ASSERT_OK(s);
  s = db->CreateColumnFamily(cf_options, "CFB", &cfb);
  ASSERT_OK(s);

  delete cfa;
  delete cfb;
  delete db;

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

  ColumnFamilyOptions* cf_opt_default = &column_families[0].options;
  ColumnFamilyOptions* cf_opt_cfa = &column_families[1].options;
  ColumnFamilyOptions* cf_opt_cfb = &column_families[2].options;
  cf_opt_default->disable_auto_compactions = false;
  cf_opt_cfa->disable_auto_compactions = true;
  cf_opt_cfb->disable_auto_compactions = false;

  std::vector<ColumnFamilyHandle*> handles;

  s = TransactionDB::Open(options, txn_db_options, dbname, column_families,
                          &handles, &db);
  ASSERT_OK(s);

  auto cfh_default = reinterpret_cast<ColumnFamilyHandleImpl*>(handles[0]);
  auto opt_default = *cfh_default->cfd()->GetLatestMutableCFOptions();

  auto cfh_a = reinterpret_cast<ColumnFamilyHandleImpl*>(handles[1]);
  auto opt_a = *cfh_a->cfd()->GetLatestMutableCFOptions();

  auto cfh_b = reinterpret_cast<ColumnFamilyHandleImpl*>(handles[2]);
  auto opt_b = *cfh_b->cfd()->GetLatestMutableCFOptions();

  ASSERT_EQ(opt_default.disable_auto_compactions, false);
  ASSERT_EQ(opt_a.disable_auto_compactions, true);
  ASSERT_EQ(opt_b.disable_auto_compactions, false);

  for (auto handle : handles) {
    delete handle;
  }
}

TEST_P(TransactionTest, ExpiredTransactionDataRace1) {
//在这个测试中，txn1应该成功提交，
//在txn1开始提交之后调用回调。
  rocksdb::SyncPoint::GetInstance()->LoadDependency(
      {{"TransactionTest::ExpirableTransactionDataRace:1"}});
  rocksdb::SyncPoint::GetInstance()->SetCallBack(
      "TransactionTest::ExpirableTransactionDataRace:1", [&](void* arg) {
        WriteOptions write_options;
        TransactionOptions txn_options;

//强制txn1过期
        /*睡眠覆盖*/
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        Transaction* txn2 = db->BeginTransaction(write_options, txn_options);
        Status s;
        s = txn2->Put("X", "2");
        ASSERT_TRUE(s.IsTimedOut());
        s = txn2->Commit();
        ASSERT_OK(s);
        delete txn2;
      });

  rocksdb::SyncPoint::GetInstance()->EnableProcessing();

  WriteOptions write_options;
  TransactionOptions txn_options;

  txn_options.expiration = 100;
  Transaction* txn1 = db->BeginTransaction(write_options, txn_options);

  Status s;
  s = txn1->Put("X", "1");
  ASSERT_OK(s);
  s = txn1->Commit();
  ASSERT_OK(s);

  ReadOptions read_options;
  string value;
  s = db->Get(read_options, "X", &value);
  ASSERT_EQ("1", value);

  delete txn1;
  rocksdb::SyncPoint::GetInstance()->DisableProcessing();
}

namespace {
Status TransactionStressTestInserter(TransactionDB* db,
                                     const size_t num_transactions,
                                     const size_t num_sets,
                                     const size_t num_keys_per_set) {
  size_t seed = std::hash<std::thread::id>()(std::this_thread::get_id());
  Random64 _rand(seed);
  WriteOptions write_options;
  ReadOptions read_options;
  TransactionOptions txn_options;
  txn_options.set_snapshot = true;

  RandomTransactionInserter inserter(&_rand, write_options, read_options,
                                     num_keys_per_set,
                                     static_cast<uint16_t>(num_sets));

  for (size_t t = 0; t < num_transactions; t++) {
    bool success = inserter.TransactionDBInsert(db, txn_options);
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

TEST_P(MySQLStyleTransactionTest, TransactionStressTest) {
  const size_t num_threads = 4;
  const size_t num_transactions_per_thread = 10000;
  const size_t num_sets = 3;
  const size_t num_keys_per_set = 100;
//将密钥空间设置为100个密钥将导致足够的写入冲突
//使这个测试有趣。

  std::vector<port::Thread> threads;

  std::function<void()> call_inserter = [&] {
    ASSERT_OK(TransactionStressTestInserter(db, num_transactions_per_thread,
                                            num_sets, num_keys_per_set));
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

TEST_P(TransactionTest, MemoryLimitTest) {
  TransactionOptions txn_options;
//数据头（12字节）+noop（1字节）+2*8字节。
  txn_options.max_write_batch_size = 29;
  string value;
  Status s;

  Transaction* txn = db->BeginTransaction(WriteOptions(), txn_options);
  ASSERT_TRUE(txn);

  ASSERT_EQ(0, txn->GetNumPuts());
  ASSERT_LE(0, txn->GetID());

  s = txn->Put(Slice("a"), Slice("...."));
  ASSERT_OK(s);
  ASSERT_EQ(1, txn->GetNumPuts());

  s = txn->Put(Slice("b"), Slice("...."));
  ASSERT_OK(s);
  ASSERT_EQ(2, txn->GetNumPuts());

  s = txn->Put(Slice("b"), Slice("...."));
  ASSERT_TRUE(s.IsMemoryLimit());
  ASSERT_EQ(2, txn->GetNumPuts());

  txn->Rollback();
  delete txn;
}

//针对不同的
//Snapshot、Max_Committed、Prepared和Commit条目。
TEST_P(WritePreparedTransactionTest, IsInSnapshotTest) {
  WriteOptions wo;
//使用小的提交缓存触发大量的逐出和快速前进
//最大排出量
//重新打开后生效
  WritePreparedTxnDB::DEF_COMMIT_CACHE_SIZE = 8;
//快照缓存大小相同
  WritePreparedTxnDB::DEF_SNAPSHOT_CACHE_SIZE = 5;

//先拍一些初步的快照。这是为了强调数据结构
//它保存了旧的快照，因为当
//只有几张快照低于“最大移出顺序”。
  for (int max_snapshots = 1; max_snapshots < 20; max_snapshots++) {
//在初步快照和最终快照之间留出一些间隙
//我们检查一下。这应该测试不同重叠的scnerios
//在最后一个快照和提交之间。
    for (int max_gap = 1; max_gap < 10; max_gap++) {
//因为我们实际上不向数据库写入数据，所以我们模拟seq。
//由数据库增加。唯一的例外是我们需要db seq to
//为我们的快照前进。我们每次都会用到一个假人
//增加我们对seq的模拟。
      uint64_t seq = 0;
//在每个步骤中，我们准备一个TXN，然后在下一个TXN中提交它。
//这将模拟写入同一密钥的连续事务。
      uint64_t cur_txn = 0;
//迄今拍摄的快照数
      int num_snapshots = 0;
      std::vector<const Snapshot*> to_be_released;
//迄今应用的差距数
      int gap_cnt = 0;
//我们将检查的最终快照
      uint64_t snapshot = 0;
      bool found_committed = false;
//在每个周期强调维护准备好的txn的数据结构
//我们添加了一个新的Prepare TXN。这些并不意味着承诺
//快速检查。
      std::set<uint64_t> prepared;
//在我们进行最后一次快照之前，我们会保留TXN的列表。
//这些应该是将在快照中找到的唯一序列号
      std::set<uint64_t> committed_before;
ReOpen();  //重新启动数据库
      WritePreparedTxnDB* wp_db = dynamic_cast<WritePreparedTxnDB*>(db);
      assert(wp_db);
      assert(wp_db->db_impl_);
//我们继续，直到马克斯超过快照一点。
      while (!snapshot || wp_db->max_evicted_seq_ < snapshot + 100) {
//为交易做准备
wp_db->db_impl_->Put(wo, "key", "value");  //虚拟输入到inc db seq
        seq++;
        ASSERT_EQ(wp_db->db_impl_->GetLatestSequenceNumber(), seq);
        wp_db->AddPrepared(seq);
        prepared.insert(seq);

//如果没有启动cur-txn，请做好准备。
        if (!cur_txn) {
wp_db->db_impl_->Put(wo, "key", "value");  //虚拟输入到inc db seq
          seq++;
          ASSERT_EQ(wp_db->db_impl_->GetLatestSequenceNumber(), seq);
          cur_txn = seq;
          wp_db->AddPrepared(cur_txn);
} else {                                     //否则提交它
wp_db->db_impl_->Put(wo, "key", "value");  //虚拟输入到inc db seq
          seq++;
          ASSERT_EQ(wp_db->db_impl_->GetLatestSequenceNumber(), seq);
          wp_db->AddCommitted(cur_txn, seq);
          if (!snapshot) {
            committed_before.insert(cur_txn);
          }
          cur_txn = 0;
        }

        if (num_snapshots < max_snapshots - 1) {
//拍摄初步快照
          auto tmp_snapshot = db->GetSnapshot();
          to_be_released.push_back(tmp_snapshot);
          num_snapshots++;
        } else if (gap_cnt < max_gap) {
//在拍摄最终快照之前等待一些间隙
          gap_cnt++;
        } else if (!snapshot) {
//如果尚未拍摄，则拍摄最终快照
          auto tmp_snapshot = db->GetSnapshot();
          to_be_released.push_back(tmp_snapshot);
          snapshot = tmp_snapshot->GetSequenceNumber();
//我们通过一个虚拟的Put人为地增加了db seq。检查这个
//技术是有效的，DB-Seq和我们的一样。
          ASSERT_EQ(snapshot, seq);
          num_snapshots++;
        }

//如果拍摄了快照，请验证其可见的序列号。我们重做
//在每个循环中，当
//Max收回了预付款。
        if (snapshot) {
          for (uint64_t s = 0; s <= seq; s++) {
            bool was_committed =
                (committed_before.find(s) != committed_before.end());
            bool is_in_snapshot = wp_db->IsInSnapshot(s, snapshot);
            if (was_committed != is_in_snapshot) {
              printf("max_snapshots %d max_gap %d seq %" PRIu64 " max %" PRIu64
                     " snapshot %" PRIu64
                     " gap_cnt %d num_snapshots %d s %" PRIu64 "\n",
                     max_snapshots, max_gap, seq,
                     wp_db->max_evicted_seq_.load(), snapshot, gap_cnt,
                     num_snapshots, s);
            }
            ASSERT_EQ(was_committed, is_in_snapshot);
            found_committed = found_committed || is_in_snapshot;
          }
        }
      }
//安全检查以确保测试实际运行
      ASSERT_TRUE(found_committed);
//作为一个额外的检查，检查准备好的设备在
//他们是坚定的。
      if (cur_txn) {
        wp_db->AddCommitted(cur_txn, seq);
      }
      for (auto p : prepared) {
        wp_db->AddCommitted(p, seq);
      }
      ASSERT_TRUE(wp_db->delayed_prepared_.empty());
      ASSERT_TRUE(wp_db->prepared_txns_.empty());
      for (auto s : to_be_released) {
        db->ReleaseSnapshot(s);
      }
    }
  }
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#else
#include <stdio.h>

int main(int argc, char** argv) {
  fprintf(stderr,
          "SKIPPED as Transactions are not supported in ROCKSDB_LITE\n");
  return 0;
}

#endif  //摇滚乐
