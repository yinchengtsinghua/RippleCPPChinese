
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

#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "monitoring/instrumented_mutex.h"
#include "rocksdb/utilities/transaction.h"
#include "util/autovector.h"
#include "util/hash_map.h"
#include "util/thread_local.h"
#include "utilities/transactions/pessimistic_transaction.h"

namespace rocksdb {

class ColumnFamilyHandle;
struct LockInfo;
struct LockMap;
struct LockMapStripe;

struct DeadlockInfoBuffer {
 private:
  std::vector<DeadlockPath> paths_buffer_;
  uint32_t buffer_idx_;
  std::mutex paths_buffer_mutex_;
  std::vector<DeadlockPath> Normalize();

 public:
  explicit DeadlockInfoBuffer(uint32_t n_latest_dlocks)
      : paths_buffer_(n_latest_dlocks), buffer_idx_(0) {}
  void AddNewPath(DeadlockPath path);
  void Resize(uint32_t target_size);
  std::vector<DeadlockPath> PrepareBuffer();
};

struct TrackedTrxInfo {
  autovector<TransactionID> m_neighbors;
  uint32_t m_cf_id;
  std::string m_waiting_key;
  bool m_exclusive;
};

class Slice;
class PessimisticTransactionDB;

class TransactionLockMgr {
 public:
  TransactionLockMgr(TransactionDB* txn_db, size_t default_num_stripes,
                     int64_t max_num_locks, uint32_t max_num_deadlocks,
                     std::shared_ptr<TransactionDBMutexFactory> factory);

  ~TransactionLockMgr();

//为此列族创建新的锁映射。呼叫者应保证
//此列族不存在。
  void AddColumnFamily(uint32_t column_family_id);

//删除此列族的锁映射。呼叫者应保证
//此列族不再使用。
  void RemoveColumnFamily(uint32_t column_family_id);

//尝试锁定钥匙。如果返回OK状态，则调用方负责
//用于在此密钥上调用unlock（）。
  Status TryLock(PessimisticTransaction* txn, uint32_t column_family_id,
                 const std::string& key, Env* env, bool exclusive);

//解锁被Trylock（）锁定的钥匙。txn必须与
//锁定了这把钥匙。
  void UnLock(const PessimisticTransaction* txn, const TransactionKeyMap* keys,
              Env* env);
  void UnLock(PessimisticTransaction* txn, uint32_t column_family_id,
              const std::string& key, Env* env);

  using LockStatusData = std::unordered_multimap<uint32_t, KeyLockInfo>;
  LockStatusData GetLockStatusData();
  std::vector<DeadlockPath> GetDeadlockInfoBuffer();
  void Resize(uint32_t);

 private:
  PessimisticTransactionDB* txn_db_impl_;

//每个列族的默认锁映射条数
  const size_t default_num_stripes_;

//限制每个列族锁定的键数
  const int64_t max_num_locks_;

//为了避免死锁，必须满足以下锁定顺序
//我们自己。
//-锁定\u映射\u互斥\u
//-按递增的CF ID、递增的条带顺序对互斥体进行条带处理
//-等待\txn\u映射\u互斥\u
//
//访问/修改锁定地图时必须保持。
  InstrumentedMutex lock_map_mutex_;

//ColumnFamilyID到锁定密钥信息的映射
  using LockMaps = std::unordered_map<uint32_t, std::shared_ptr<LockMap>>;
  LockMaps lock_maps_;

//在锁映射中线程本地缓存条目。这是一个优化
//避免获取互斥锁以查找锁映射
  std::unique_ptr<ThreadLocalPtr> lock_maps_cache_;

//修改wait-txn-map和rev-wait-txn-map时必须保留。
  std::mutex wait_txn_map_mutex_;

//waitee的地图->waiter的数量。
  HashMap<TransactionID, int> rev_wait_txn_map_;
//服务员的地图->服务员。
  HashMap<TransactionID, TrackedTrxInfo> wait_txn_map_;
  DeadlockInfoBuffer dlock_buffer_;

//用于分配锁定密钥时要使用的互斥锁/condvar
  std::shared_ptr<TransactionDBMutexFactory> mutex_factory_;

  bool IsLockExpired(TransactionID txn_id, const LockInfo& lock_info, Env* env,
                     uint64_t* wait_time);

  std::shared_ptr<LockMap> GetLockMap(uint32_t column_family_id);

  Status AcquireWithTimeout(PessimisticTransaction* txn, LockMap* lock_map,
                            LockMapStripe* stripe, uint32_t column_family_id,
                            const std::string& key, Env* env, int64_t timeout,
                            const LockInfo& lock_info);

  Status AcquireLocked(LockMap* lock_map, LockMapStripe* stripe,
                       const std::string& key, Env* env,
                       const LockInfo& lock_info, uint64_t* wait_time,
                       autovector<TransactionID>* txn_ids);

  void UnLockKey(const PessimisticTransaction* txn, const std::string& key,
                 LockMapStripe* stripe, LockMap* lock_map, Env* env);

  bool IncrementWaiters(const PessimisticTransaction* txn,
                        const autovector<TransactionID>& wait_ids,
                        const std::string& key, const uint32_t& cf_id,
                        const bool& exclusive);
  void DecrementWaiters(const PessimisticTransaction* txn,
                        const autovector<TransactionID>& wait_ids);
  void DecrementWaitersImpl(const PessimisticTransaction* txn,
                            const autovector<TransactionID>& wait_ids);

//不允许复制
  TransactionLockMgr(const TransactionLockMgr&);
  void operator=(const TransactionLockMgr&);
};

}  //命名空间rocksdb
#endif  //摇滚乐
