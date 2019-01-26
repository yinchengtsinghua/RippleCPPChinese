
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

#include <string>
#include <utility>
#include <vector>

#include "rocksdb/comparator.h"
#include "rocksdb/db.h"
#include "rocksdb/utilities/stackable_db.h"
#include "rocksdb/utilities/transaction.h"

//具有事务支持的数据库。
//
//参见transaction.h和examples/transaction\u example.cc

namespace rocksdb {

class TransactionDBMutexFactory;

enum TxnDBWritePolicy {
WRITE_COMMITTED = 0,  //只写提交的数据
//TODO（Myabandeh）：尚未实施
WRITE_PREPARED,  //2PC准备阶段后写入数据
//TODO（Myabandeh）：尚未实施
WRITE_UNPREPARED  //在2PC准备阶段之前写入数据
};

const uint32_t kInitialMaxDeadlocks = 5;

struct TransactionDBOptions {
//指定可以同时锁定的最大键数
//每个柱族。
//如果锁定的密钥数大于max_num_locks，则事务
//写入（或GetForUpdate）将返回错误。
//如果该值不是正数，则不会强制执行任何限制。
  int64_t max_num_locks = -1;

//存储要跟踪的最新死锁数
  uint32_t max_num_deadlocks = kInitialMaxDeadlocks;

//增加这个值将通过划分锁来增加并发性。
//将表（每个列族）分成多个子表，每个子表都有自己的子表
//分离
//互斥
  size_t num_stripes = 16;

//如果为正数，则指定默认的等待超时（以毫秒为单位）
//如果未由指定，则事务将尝试锁定密钥。
//TransactionOptions：：锁定超时。
//
//如果为0，则在无法立即获取锁时不等待。
//如果为负，则没有超时。不建议使用超时
//因为它会导致僵局。目前，没有死锁检测到
//恢复
//从僵局中恢复过来。
int64_t transaction_lock_timeout = 1000;  //1秒

//如果为正数，则指定写入密钥时的等待超时（毫秒）
//在事务外部（即通过调用db:：put（）、merge（）、delete（）、write（））
//直接）。
//如果为0，则在无法立即获取锁时不等待。
//如果为负，则没有超时，在获取时将无限期阻塞
//一把锁。
//
//不使用超时可能导致死锁。目前，那里
//不是要从死锁中恢复的死锁检测。当DB写入时
//不能与其他DB写入死锁，它们可以与事务死锁。
//只有当所有事务都具有较小的
//到期集。
int64_t default_lock_timeout = 1000;  //1秒

//如果设置，TransactionDB将使用互斥的这种实现，并且
//所有事务锁定的条件变量，而不是默认值
//mutex/condvar实现。
  std::shared_ptr<TransactionDBMutexFactory> custom_mutex_factory;

//何时将数据写入数据库的策略。默认策略是
//只写已提交的数据（写已提交）。数据可以写入
//在提交阶段之前。然后，数据库需要提供以下机制：
//区分提交数据和未提交数据。
  TxnDBWritePolicy write_policy = TxnDBWritePolicy::WRITE_COMMITTED;
};

struct TransactionOptions {
//设置set_snapshot=true与调用相同
//事务：：SetSnapshot（）。
  bool set_snapshot = false;

//设置为true意味着在获取锁之前，此事务将
//检查这样做是否会导致死锁。如果是这样，它将返回
//状态：忙。用户应重试其事务。
  bool deadlock_detect = false;

//TODO（Agiardullo）：TransactionDB尚不支持允许
//两个不相等的键相等。也就是说，CMP->Compare（A，B）应该只
//返回0如果
//A.Compare（B）返回0。


//如果为正数，则指定等待超时时间（毫秒）
//事务试图锁定密钥。
//
//如果为0，则在无法立即获取锁时不等待。
//如果为负，则将使用TransactionDBOptions:：Transaction_Lock_Timeout。
  int64_t lock_timeout = -1;

//过期持续时间（毫秒）。如果为非负数，则为
//持续时间超过此毫秒将无法提交。如果没有设置，
//从未提交、回滚或删除的已忘记事务
//将永远不会放弃它持有的任何锁。这可以防止钥匙
//被其他作家写的。
  int64_t expiration = -1;

//死锁检测期间要进行的遍历数。
  int64_t deadlock_detect_depth = 50;

//用于写入批处理的最大字节数。0表示无限制。
  size_t max_write_batch_size = 0;
};

struct KeyLockInfo {
  std::string key;
  std::vector<TransactionID> ids;
  bool exclusive;
};

struct DeadlockInfo {
  TransactionID m_txn_id;
  uint32_t m_cf_id;
  std::string m_waiting_key;
  bool m_exclusive;
};

struct DeadlockPath {
  std::vector<DeadlockInfo> path;
  bool limit_exceeded;

  explicit DeadlockPath(std::vector<DeadlockInfo> path_entry)
      : path(path_entry), limit_exceeded(false) {}

//空路径，超出限制的构造函数和默认构造函数
  explicit DeadlockPath(bool limit = false) : path(0), limit_exceeded(limit) {}

  bool empty() { return path.empty() && !limit_exceeded; }
};

class TransactionDB : public StackableDB {
 public:
//打开类似于db:：open（）的TransactionDB。
//在内部调用PrepareWrap（）和WrapDB（）。
  static Status Open(const Options& options,
                     const TransactionDBOptions& txn_db_options,
                     const std::string& dbname, TransactionDB** dbptr);

  static Status Open(const DBOptions& db_options,
                     const TransactionDBOptions& txn_db_options,
                     const std::string& dbname,
                     const std::vector<ColumnFamilyDescriptor>& column_families,
                     std::vector<ColumnFamilyHandle*>* handles,
                     TransactionDB** dbptr);
//以下函数用于在内部使用
//打开的数据库或可堆叠的数据库。
//1。调用PrepareWrap（），将空std:：vector<size_t>传递到
//压缩启用的索引。
//2。打开db或可堆叠db，并将db_选项和column_系列传递给
//准备RAP（）
//注意：PrepareWrap（）可能会更改参数，在
//必要时调用。
//三。在步骤1中使用启用压缩的索引调用wrap*db（）并处理
//步骤2中打开的db/stackabledb
  static void PrepareWrap(DBOptions* db_options,
                          std::vector<ColumnFamilyDescriptor>* column_families,
                          std::vector<size_t>* compaction_enabled_cf_indices);
  static Status WrapDB(DB* db, const TransactionDBOptions& txn_db_options,
                       const std::vector<size_t>& compaction_enabled_cf_indices,
                       const std::vector<ColumnFamilyHandle*>& handles,
                       TransactionDB** dbptr);
  static Status WrapStackableDB(
      StackableDB* db, const TransactionDBOptions& txn_db_options,
      const std::vector<size_t>& compaction_enabled_cf_indices,
      const std::vector<ColumnFamilyHandle*>& handles, TransactionDB** dbptr);
  ~TransactionDB() override {}

//启动新事务。
//
//调用方负责删除返回的事务，如果没有
//需要更长的时间。
//
//如果旧的_Txn不为空，BeginTransaction将重用此事务
//处理而不是分配新的。这是要避免的优化
//重复创建事务时的额外分配。
  virtual Transaction* BeginTransaction(
      const WriteOptions& write_options,
      const TransactionOptions& txn_options = TransactionOptions(),
      Transaction* old_txn = nullptr) = 0;

  virtual Transaction* GetTransactionByName(const TransactionName& name) = 0;
  virtual void GetAllPreparedTransactions(std::vector<Transaction*>* trans) = 0;

//返回保留的所有锁的集合。
//
//映射为列族ID->keylockInfo
  virtual std::unordered_multimap<uint32_t, KeyLockInfo>
  GetLockStatusData() = 0;
  virtual std::vector<DeadlockPath> GetDeadlockInfoBuffer() = 0;
  virtual void SetDeadlockInfoBufferSize(uint32_t target_size) = 0;

 protected:
//若要创建TransactionDB，请调用open（）。
  explicit TransactionDB(DB* db) : StackableDB(db) {}

 private:
//不允许复制
  TransactionDB(const TransactionDB&);
  void operator=(const TransactionDB&);
};

}  //命名空间rocksdb

#endif  //摇滚乐
