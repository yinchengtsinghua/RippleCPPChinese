
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

#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/utilities/transaction_db.h"
#include "utilities/transactions/pessimistic_transaction.h"
#include "utilities/transactions/transaction_lock_mgr.h"
#include "utilities/transactions/write_prepared_txn.h"

namespace rocksdb {

class PessimisticTransactionDB : public TransactionDB {
 public:
  explicit PessimisticTransactionDB(DB* db,
                                    const TransactionDBOptions& txn_db_options);

  explicit PessimisticTransactionDB(StackableDB* db,
                                    const TransactionDBOptions& txn_db_options);

  virtual ~PessimisticTransactionDB();

  Status Initialize(const std::vector<size_t>& compaction_enabled_cf_indices,
                    const std::vector<ColumnFamilyHandle*>& handles);

  Transaction* BeginTransaction(const WriteOptions& write_options,
                                const TransactionOptions& txn_options,
                                Transaction* old_txn) override = 0;

  using StackableDB::Put;
  virtual Status Put(const WriteOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& val) override;

  using StackableDB::Delete;
  virtual Status Delete(const WriteOptions& wopts,
                        ColumnFamilyHandle* column_family,
                        const Slice& key) override;

  using StackableDB::Merge;
  virtual Status Merge(const WriteOptions& options,
                       ColumnFamilyHandle* column_family, const Slice& key,
                       const Slice& value) override;

  using StackableDB::Write;
  virtual Status Write(const WriteOptions& opts, WriteBatch* updates) override;

  using StackableDB::CreateColumnFamily;
  virtual Status CreateColumnFamily(const ColumnFamilyOptions& options,
                                    const std::string& column_family_name,
                                    ColumnFamilyHandle** handle) override;

  using StackableDB::DropColumnFamily;
  virtual Status DropColumnFamily(ColumnFamilyHandle* column_family) override;

  Status TryLock(PessimisticTransaction* txn, uint32_t cfh_id,
                 const std::string& key, bool exclusive);

  void UnLock(PessimisticTransaction* txn, const TransactionKeyMap* keys);
  void UnLock(PessimisticTransaction* txn, uint32_t cfh_id,
              const std::string& key);

  void AddColumnFamily(const ColumnFamilyHandle* handle);

  static TransactionDBOptions ValidateTxnDBOptions(
      const TransactionDBOptions& txn_db_options);

  const TransactionDBOptions& GetTxnDBOptions() const {
    return txn_db_options_;
  }

  void InsertExpirableTransaction(TransactionID tx_id,
                                  PessimisticTransaction* tx);
  void RemoveExpirableTransaction(TransactionID tx_id);

//如果事务不再可用，锁可能被盗
//如果事务可用，请尝试直接从事务窃取锁
//调用者有责任确保
//是可过期的（getExpirationTime（）>0），并且已过期。
  bool TryStealingExpiredTransactionLocks(TransactionID tx_id);

  Transaction* GetTransactionByName(const TransactionName& name) override;

  void RegisterTransaction(Transaction* txn);
  void UnregisterTransaction(Transaction* txn);

//不是线程安全的。当前用例在恢复期间（单线程）
  void GetAllPreparedTransactions(std::vector<Transaction*>* trans) override;

  TransactionLockMgr::LockStatusData GetLockStatusData() override;

  std::vector<DeadlockPath> GetDeadlockInfoBuffer() override;
  void SetDeadlockInfoBufferSize(uint32_t target_size) override;

  struct CommitEntry {
    uint64_t prep_seq;
    uint64_t commit_seq;
    CommitEntry() : prep_seq(0), commit_seq(0) {}
    CommitEntry(uint64_t ps, uint64_t cs) : prep_seq(ps), commit_seq(cs) {}
  };

 protected:
  void ReinitializeTransaction(
      Transaction* txn, const WriteOptions& write_options,
      const TransactionOptions& txn_options = TransactionOptions());
  DBImpl* db_impl_;
  std::shared_ptr<Logger> info_log_;

 private:
  friend class WritePreparedTxnDB;
  const TransactionDBOptions txn_db_options_;
  TransactionLockMgr lock_mgr_;

//添加/删除列族时必须保留。
  InstrumentedMutex column_family_mutex_;
  Transaction* BeginInternalTransaction(const WriteOptions& options);

//用于确保没有从可过期事务中窃取锁
//已经开始提交。仅限具有到期时间的事务
//应该在这张地图上。
  std::mutex map_mutex_;
  std::unordered_map<TransactionID, PessimisticTransaction*>
      expirable_transactions_map_;

//从名称映射到两阶段事务实例
  std::mutex name_map_mutex_;
  std::unordered_map<TransactionName, Transaction*> transactions_;
};

//提交后将数据写入数据库的悲观事务数据库。
//这样，数据库只包含提交的数据。
class WriteCommittedTxnDB : public PessimisticTransactionDB {
 public:
  explicit WriteCommittedTxnDB(DB* db,
                               const TransactionDBOptions& txn_db_options)
      : PessimisticTransactionDB(db, txn_db_options) {}

  explicit WriteCommittedTxnDB(StackableDB* db,
                               const TransactionDBOptions& txn_db_options)
      : PessimisticTransactionDB(db, txn_db_options) {}

  virtual ~WriteCommittedTxnDB() {}

  Transaction* BeginTransaction(const WriteOptions& write_options,
                                const TransactionOptions& txn_options,
                                Transaction* old_txn) override;
};

//悲观事务数据库，在2PC准备阶段后将数据写入数据库。
//这样，数据库中的某些数据可能无法提交。数据库提供
//将这些数据与提交的数据区分开来的机制。
class WritePreparedTxnDB : public PessimisticTransactionDB {
 public:
  explicit WritePreparedTxnDB(DB* db,
                              const TransactionDBOptions& txn_db_options)
      : PessimisticTransactionDB(db, txn_db_options),
        SNAPSHOT_CACHE_SIZE(DEF_SNAPSHOT_CACHE_SIZE),
        COMMIT_CACHE_SIZE(DEF_COMMIT_CACHE_SIZE) {
    init(txn_db_options);
  }

  explicit WritePreparedTxnDB(StackableDB* db,
                              const TransactionDBOptions& txn_db_options)
      : PessimisticTransactionDB(db, txn_db_options),
        SNAPSHOT_CACHE_SIZE(DEF_SNAPSHOT_CACHE_SIZE),
        COMMIT_CACHE_SIZE(DEF_COMMIT_CACHE_SIZE) {
    init(txn_db_options);
  }

  virtual ~WritePreparedTxnDB() {}

  Transaction* BeginTransaction(const WriteOptions& write_options,
                                const TransactionOptions& txn_options,
                                Transaction* old_txn) override;

//检查是否使用seqeunce number seq写入值的事务
//对序列号为snapshot_seq的快照可见
  bool IsInSnapshot(uint64_t seq, uint64_t snapshot_seq);
//将带有Prepare Sequence Seq的事务添加到准备列表中
  void AddPrepared(uint64_t seq);
//添加带有准备序列的事务准备序列和提交序列
//提交到提交映射
  void AddCommitted(uint64_t prepare_seq, uint64_t commit_seq);

 private:
  friend class WritePreparedTransactionTest_IsInSnapshotTest_Test;

  /*D init（const transactiondboptions&/*unused*/）
    快照缓存唯一指针
        new std:：atomic<sequencenumber>[快照缓存大小]）；
    CopyScCase=
        唯一的_ptr<commentry[]>（new commentry[commit_cache_size]）；
  }

  //具有用于擦除的摊余O（1）复杂性的堆。它使用一个额外的堆
  //跟踪尚未位于主堆顶部的已擦除项。
  类准备堆
    std:：priority_queue<uint64_t，std:：vector<uint64_t>，std:：greater<uint64_t>>
        希帕斯；
    std:：priority_queue<uint64_t，std:：vector<uint64_t>，std:：greater<uint64_t>>
        EraseDeHeaPig；

   公众：
    bool empty（）返回heap_.empty（）；
    uint64_t top（）返回堆_u.top（）；
    空推（uint64_t v）堆推（v）；
    空（）
      HeAP.Pop.（）；
      而（！）堆空（）&！删除的堆空（）&&
             heap_.top（）==删除的_heap_.top（））
        HeAP.Pop.（）；
        删除了堆pop（）；
      }
    }
    无效擦除（uint64_t seq）
      如果（！）堆_.empty（））
        if（seq<heap_.top（））
          //已经弹出，忽略它。
        else if（heap_.top（）==seq）
          HeAP.Pop.（）；
        else//（heap_.top（）>序列）
          //在堆中，记住稍后弹出
          清除堆推送（seq）；
        }
      }
    }
  }；

  //从提交表中获取索引为“seq”的提交项。它
  //如果存在这样的条目，则返回true。
  bool getcommitentry（uint64_t indexed_seq，commitentry*条目）；
  //在提交表中用索引“seq”重写该条目，
  //commit entry<prep_seq，commit_seq>。如果重写结果被逐出，
  //设置收回的_项并返回true。
  bool addcommentry（uint64_t indexed_seq，commentry&new_entry，
                      承诺*驱逐出境）；
  //在提交表中用索引“seq”重写该条目，
  //仅当现有项与
  //需要_项。否则返回false。
  bool exchangecommentry（uint64_t indexed_seq，commentry&expected_entry，
                           新入职）；

  //如果prep seq<=snapshot seq<
  //PrimeSeq。如果不需要检查下一个快照，则返回false。
  //如果条目已经添加到旧的提交映射或没有
  //下一个快照可以满足条件。下一个更大：下一个
  //快照将是一个较大的值
  Bool MaybeUpdateOldCommitmap（const uint64_t&Prep_seq，
                               const uint64_t&commit序列，
                               const uint64_t&snapshot_seq，
                               const bool next_更大）；

  //上一次Max退出“高级”时的实时快照列表。
  //存储在两个数据结构中的列表：快照缓存中，即
  //对于并发读取和快照（如果数据不适合）
  //进入快照缓存。两个列表中快照的总数
  std:：atomic<size_t>snapshots_total_
  //列表按升序排序。为写入提供线程安全性
  //使用快照时，由于std:：atomic for，mutex和并发读取是安全的。
  /每个条目。在x8664体系结构中，这种读取被编译为简单读取
  //指令。128条目
  //TODO（myabandeh）：避免使用非常量静态变量
  静态大小定义快照缓存大小；
  常量大小快照缓存大小；
  唯一的快照缓存
  //存储快照的第二个列表。列表按升序排序。
  //thread safety提供快照\u mutex\。
  std:：vector<sequencenumber>snapshots；
  //最新快照列表的版本。这可以用来避免
  //重写与较新版本同时更新的列表。
  序列号快照_version_uu=0；

  //一堆准备好的事务。提供螺纹安全
  //准备好了“mutex”。
  PreparedHeap准备好了
  //TODO（myabandeh）：避免使用非常量静态变量
  静态大小定义提交缓存大小；
  常量大小提交缓存大小；
  //提交缓存必须初始化为零才能区分空索引
  //一个填充的。提交缓存mutex提供线程安全。
  唯一的提交缓存
  //从提交缓存中退出的最大*commit*序列号
  std:：atomic<uint64_t>max_eviced_seq_
  //必须保留的从提交缓存中收回的项的映射
  //为旧快照提供服务。正常情况下应为空。
  //旧的_commit_map_mutex_u提供线程安全。
  std:：map<uint64_t，uint64_t>old_commit_map_；
  //一组长时间运行的准备事务，这些事务不是由
  //time max_eviced_seq_u推进其序列号。这应该是
  //正常为空。准备好的_mutex_u提供线程安全性。
  std:：set<uint64_t>delayed_prepared_uuu；
  //在延迟的“准备就绪”更改时更新。预期为真
  /正常情况下。
  std:：atomic<bool>delayed_prepared_empty_true_
  //当旧的_commit_map_u.empty（）更改时更新。正常情况下应为真。
  std:：atomic<bool>old_commit_map_empty_true_
  端口：：rwmutex准备好了\u mutex；
  端口：：rwmutex old_commit_map_mutex；
  端口：：rwmutex commit_cache_mutex；
  端口：：rwmutex快照\u mutex；
}；

//命名空间rocksdb
endif//rocksdb_lite
