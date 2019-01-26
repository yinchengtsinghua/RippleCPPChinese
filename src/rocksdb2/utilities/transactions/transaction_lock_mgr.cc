
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

#include "utilities/transactions/transaction_lock_mgr.h"

#include <inttypes.h>

#include <algorithm>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

#include "rocksdb/slice.h"
#include "rocksdb/utilities/transaction_db_mutex.h"
#include "util/cast_util.h"
#include "util/murmurhash.h"
#include "util/sync_point.h"
#include "util/thread_local.h"
#include "utilities/transactions/pessimistic_transaction_db.h"

namespace rocksdb {

struct LockInfo {
  bool exclusive;
  autovector<TransactionID> txn_ids;

//在此之后，事务锁在美国无效
  uint64_t expiration_time;

  LockInfo(TransactionID id, uint64_t time, bool ex)
      : exclusive(ex), expiration_time(time) {
    txn_ids.push_back(id);
  }
  LockInfo(const LockInfo& lock_info)
      : exclusive(lock_info.exclusive),
        txn_ids(lock_info.txn_ids),
        expiration_time(lock_info.expiration_time) {}
};

struct LockMapStripe {
  explicit LockMapStripe(std::shared_ptr<TransactionDBMutexFactory> factory) {
    stripe_mutex = factory->AllocateMutex();
    stripe_cv = factory->AllocateCondVar();
    assert(stripe_mutex);
    assert(stripe_cv);
  }

//修改键映射之前必须保持互斥体
  std::shared_ptr<TransactionDBMutex> stripe_mutex;

//每个条带等待锁的条件变量
  std::shared_ptr<TransactionDBCondVar> stripe_cv;

//锁定的键映射到锁定它们的事务的信息。
//TODO（Agiardullo）：探索其他数据结构的性能。
  std::unordered_map<std::string, LockInfo> keys;
};

//num_条纹的地图LockMapStripes
struct LockMap {
  explicit LockMap(size_t num_stripes,
                   std::shared_ptr<TransactionDBMutexFactory> factory)
      : num_stripes_(num_stripes) {
    lock_map_stripes_.reserve(num_stripes);
    for (size_t i = 0; i < num_stripes; i++) {
      LockMapStripe* stripe = new LockMapStripe(factory);
      lock_map_stripes_.push_back(stripe);
    }
  }

  ~LockMap() {
    for (auto stripe : lock_map_stripes_) {
      delete stripe;
    }
  }

//要创建的单独的LockMapStripes数，每个都有自己的互斥体
  const size_t num_stripes_;

//当前锁定在此列族中的键的计数。
//（仅在TransactionLockMgr:：Max_Num_Locks_u为正数时维护。）
  std::atomic<int64_t> lock_cnt{0};

  std::vector<LockMapStripe*> lock_map_stripes_;

  size_t GetStripe(const std::string& key) const;
};

void DeadlockInfoBuffer::AddNewPath(DeadlockPath path) {
  std::lock_guard<std::mutex> lock(paths_buffer_mutex_);

  if (paths_buffer_.empty()) {
    return;
  }

  paths_buffer_[buffer_idx_] = path;
  buffer_idx_ = (buffer_idx_ + 1) % paths_buffer_.size();
}

void DeadlockInfoBuffer::Resize(uint32_t target_size) {
  std::lock_guard<std::mutex> lock(paths_buffer_mutex_);

  paths_buffer_ = Normalize();

//在正常化之后，放下不再需要的死锁
  if (target_size < paths_buffer_.size()) {
    paths_buffer_.erase(
        paths_buffer_.begin(),
        paths_buffer_.begin() + (paths_buffer_.size() - target_size));
    buffer_idx_ = 0;
  }
//将缓冲区大小调整为目标大小并还原缓冲区的IDX
  else {
    auto prev_size = paths_buffer_.size();
    paths_buffer_.resize(target_size);
    buffer_idx_ = (uint32_t)prev_size;
  }
}

std::vector<DeadlockPath> DeadlockInfoBuffer::Normalize() {
  auto working = paths_buffer_;

  if (working.empty()) {
    return working;
  }

//下一次写入发生在不存在的路径槽中
  if (paths_buffer_[buffer_idx_].empty()) {
    working.resize(buffer_idx_);
  } else {
    std::rotate(working.begin(), working.begin() + buffer_idx_, working.end());
  }

  return working;
}

std::vector<DeadlockPath> DeadlockInfoBuffer::PrepareBuffer() {
  std::lock_guard<std::mutex> lock(paths_buffer_mutex_);

//反转归一化向量首先返回最新的死锁
  auto working = Normalize();
  std::reverse(working.begin(), working.end());

  return working;
}

namespace {
void UnrefLockMapsCache(void* ptr) {
//当线程退出或threadlocalptr被销毁时调用。
  auto lock_maps_cache =
      static_cast<std::unordered_map<uint32_t, std::shared_ptr<LockMap>>*>(ptr);
  delete lock_maps_cache;
}
}  //匿名命名空间

TransactionLockMgr::TransactionLockMgr(
    TransactionDB* txn_db, size_t default_num_stripes, int64_t max_num_locks,
    uint32_t max_num_deadlocks,
    std::shared_ptr<TransactionDBMutexFactory> mutex_factory)
    : txn_db_impl_(nullptr),
      default_num_stripes_(default_num_stripes),
      max_num_locks_(max_num_locks),
      lock_maps_cache_(new ThreadLocalPtr(&UnrefLockMapsCache)),
      dlock_buffer_(max_num_deadlocks),
      mutex_factory_(mutex_factory) {
  assert(txn_db);
  txn_db_impl_ =
      static_cast_with_check<PessimisticTransactionDB, TransactionDB>(txn_db);
}

TransactionLockMgr::~TransactionLockMgr() {}

size_t LockMap::GetStripe(const std::string& key) const {
  assert(num_stripes_ > 0);
  static murmur_hash hash;
  size_t stripe = hash(key) % num_stripes_;
  return stripe;
}

void TransactionLockMgr::AddColumnFamily(uint32_t column_family_id) {
  InstrumentedMutexLock l(&lock_map_mutex_);

  if (lock_maps_.find(column_family_id) == lock_maps_.end()) {
    lock_maps_.emplace(column_family_id,
                       std::shared_ptr<LockMap>(
                           new LockMap(default_num_stripes_, mutex_factory_)));
  } else {
//锁映射中已存在列\族
    assert(false);
  }
}

void TransactionLockMgr::RemoveColumnFamily(uint32_t column_family_id) {
//删除此列族的锁定图。因为锁映射已存储
//作为共享的ptr，并发事务仍然可以继续使用它
//直到他们发布了对它的引用。
  {
    InstrumentedMutexLock l(&lock_map_mutex_);

    auto lock_maps_iter = lock_maps_.find(column_family_id);
    assert(lock_maps_iter != lock_maps_.end());

    lock_maps_.erase(lock_maps_iter);
}  //洛克

//清除所有线程本地缓存
  autovector<void*> local_caches;
  lock_maps_cache_->Scrape(&local_caches, nullptr);
  for (auto cache : local_caches) {
    delete static_cast<LockMaps*>(cache);
  }
}

//查找给定列的锁映射共享指针。
//注意：只有当呼叫方仍在保持状态时，锁映射才有效。
//返回的共享指针。
std::shared_ptr<LockMap> TransactionLockMgr::GetLockMap(
    uint32_t column_family_id) {
//首先检查线程本地缓存
  if (lock_maps_cache_->Get() == nullptr) {
    lock_maps_cache_->Reset(new LockMaps());
  }

  auto lock_maps_cache = static_cast<LockMaps*>(lock_maps_cache_->Get());

  auto lock_map_iter = lock_maps_cache->find(column_family_id);
  if (lock_map_iter != lock_maps_cache->end()) {
//找到了此列族的锁映射。
    return lock_map_iter->second;
  }

//在本地缓存中找不到，获取mutex并检查共享锁映射
  InstrumentedMutexLock l(&lock_map_mutex_);

  lock_map_iter = lock_maps_.find(column_family_id);
  if (lock_map_iter == lock_maps_.end()) {
    return std::shared_ptr<LockMap>(nullptr);
  } else {
//找到了锁映射。存储在线程本地缓存中并返回。
    std::shared_ptr<LockMap>& lock_map = lock_map_iter->second;
    lock_maps_cache->insert({column_family_id, lock_map});

    return lock_map;
  }
}

//如果此锁已过期，并且可以由其他人获取，则返回true
//交易。
//如果为false，则根据
//env->getmicros（）或0（如果没有过期）。
bool TransactionLockMgr::IsLockExpired(TransactionID txn_id,
                                       const LockInfo& lock_info, Env* env,
                                       uint64_t* expire_time) {
  auto now = env->NowMicros();

  bool expired =
      (lock_info.expiration_time > 0 && lock_info.expiration_time <= now);

  if (!expired && lock_info.expiration_time > 0) {
//返回锁过期前的毫秒数
    *expire_time = lock_info.expiration_time;
  } else {
    for (auto id : lock_info.txn_ids) {
      if (txn_id == id) {
        continue;
      }

      bool success = txn_db_impl_->TryStealingExpiredTransactionLocks(id);
      if (!success) {
        expired = false;
        break;
      }
      *expire_time = 0;
    }
  }

  return expired;
}

Status TransactionLockMgr::TryLock(PessimisticTransaction* txn,
                                   uint32_t column_family_id,
                                   const std::string& key, Env* env,
                                   bool exclusive) {
//此列族ID的查找锁定映射
  std::shared_ptr<LockMap> lock_map_ptr = GetLockMap(column_family_id);
  LockMap* lock_map = lock_map_ptr.get();
  if (lock_map == nullptr) {
    char msg[255];
    snprintf(msg, sizeof(msg), "Column family id not found: %" PRIu32,
             column_family_id);

    return Status::InvalidArgument(msg);
  }

//需要为该密钥哈希到的条带锁定互斥体
  size_t stripe_num = lock_map->GetStripe(key);
  assert(lock_map->lock_map_stripes_.size() > stripe_num);
  LockMapStripe* stripe = lock_map->lock_map_stripes_.at(stripe_num);

  LockInfo lock_info(txn->GetID(), txn->GetExpirationTime(), exclusive);
  int64_t timeout = txn->GetLockTimeout();

  return AcquireWithTimeout(txn, lock_map, stripe, column_family_id, key, env,
                            timeout, lock_info);
}

//trylock（）的helper函数。
Status TransactionLockMgr::AcquireWithTimeout(
    PessimisticTransaction* txn, LockMap* lock_map, LockMapStripe* stripe,
    uint32_t column_family_id, const std::string& key, Env* env,
    int64_t timeout, const LockInfo& lock_info) {
  Status result;
  uint64_t start_time = 0;
  uint64_t end_time = 0;

  if (timeout > 0) {
    start_time = env->NowMicros();
    end_time = start_time + timeout;
  }

  if (timeout < 0) {
//如果超时为负，我们将无限期地等待获取锁。
    result = stripe->stripe_mutex->Lock();
  } else {
    result = stripe->stripe_mutex->TryLockFor(timeout);
  }

  if (!result.ok()) {
//无法获取互斥体
    return result;
  }

//如果我们能够
  uint64_t expire_time_hint = 0;
  autovector<TransactionID> wait_ids;
  result = AcquireLocked(lock_map, stripe, key, env, lock_info,
                         &expire_time_hint, &wait_ids);

  if (!result.ok() && timeout != 0) {
//如果我们无法获得锁，我们将继续重试。
//在超时允许的情况下。
    bool timed_out = false;
    do {
//决定等待多长时间
      int64_t cv_end_time = -1;

//检查保持锁的过期时间是否早于超时时间
      if (expire_time_hint > 0 &&
          (timeout < 0 || (timeout > 0 && expire_time_hint < end_time))) {
//过期时间比超时时间早
        cv_end_time = expire_time_hint;
      } else if (timeout >= 0) {
        cv_end_time = end_time;
      }

      assert(result.IsBusy() || wait_ids.size() != 0);

//我们依赖一个事务来完成，所以执行死锁
//检测。
      if (wait_ids.size() != 0) {
        if (txn->IsDeadlockDetect()) {
          if (IncrementWaiters(txn, wait_ids, key, column_family_id,
                               lock_info.exclusive)) {
            result = Status::Busy(Status::SubCode::kDeadlock);
            stripe->stripe_mutex->UnLock();
            return result;
          }
        }
        txn->SetWaitingTxn(wait_ids, column_family_id, &key);
      }

      TEST_SYNC_POINT("TransactionLockMgr::AcquireWithTimeout:WaitingTxn");
      if (cv_end_time < 0) {
//无限期等待
        result = stripe->stripe_cv->Wait(stripe->stripe_mutex);
      } else {
        uint64_t now = env->NowMicros();
        if (static_cast<uint64_t>(cv_end_time) > now) {
          result = stripe->stripe_cv->WaitFor(stripe->stripe_mutex,
                                              cv_end_time - now);
        }
      }

      if (wait_ids.size() != 0) {
        txn->ClearWaitingTxn();
        if (txn->IsDeadlockDetect()) {
          DecrementWaiters(txn, wait_ids);
        }
      }

      if (result.IsTimedOut()) {
          timed_out = true;
//即使我们超时了，我们仍将再次尝试
//获取下面的锁（可能锁已过期，我们
//从未发出过信号）。
      }

      if (result.ok() || result.IsTimedOut()) {
        result = AcquireLocked(lock_map, stripe, key, env, lock_info,
                               &expire_time_hint, &wait_ids);
      }
    } while (!result.ok() && !timed_out);
  }

  stripe->stripe_mutex->UnLock();

  return result;
}

void TransactionLockMgr::DecrementWaiters(
    const PessimisticTransaction* txn,
    const autovector<TransactionID>& wait_ids) {
  std::lock_guard<std::mutex> lock(wait_txn_map_mutex_);
  DecrementWaitersImpl(txn, wait_ids);
}

void TransactionLockMgr::DecrementWaitersImpl(
    const PessimisticTransaction* txn,
    const autovector<TransactionID>& wait_ids) {
  auto id = txn->GetID();
  assert(wait_txn_map_.Contains(id));
  wait_txn_map_.Delete(id);

  for (auto wait_id : wait_ids) {
    rev_wait_txn_map_.Get(wait_id)--;
    if (rev_wait_txn_map_.Get(wait_id) == 0) {
      rev_wait_txn_map_.Delete(wait_id);
    }
  }
}

bool TransactionLockMgr::IncrementWaiters(
    const PessimisticTransaction* txn,
    const autovector<TransactionID>& wait_ids, const std::string& key,
    const uint32_t& cf_id, const bool& exclusive) {
  auto id = txn->GetID();
  std::vector<int> queue_parents(txn->GetDeadlockDetectDepth());
  std::vector<TransactionID> queue_values(txn->GetDeadlockDetectDepth());
  std::lock_guard<std::mutex> lock(wait_txn_map_mutex_);
  assert(!wait_txn_map_.Contains(id));

  wait_txn_map_.Insert(id, {wait_ids, cf_id, key, exclusive});

  for (auto wait_id : wait_ids) {
    if (rev_wait_txn_map_.Contains(wait_id)) {
      rev_wait_txn_map_.Get(wait_id)++;
    } else {
      rev_wait_txn_map_.Insert(wait_id, 1);
    }
  }

//如果没有人在等自己，就没有僵局。
  if (!rev_wait_txn_map_.Contains(id)) {
    return false;
  }

  const auto* next_ids = &wait_ids;
  int parent = -1;
  for (int tail = 0, head = 0; head < txn->GetDeadlockDetectDepth(); head++) {
    int i = 0;
    if (next_ids) {
      for (; i < static_cast<int>(next_ids->size()) &&
             tail + i < txn->GetDeadlockDetectDepth();
           i++) {
        queue_values[tail + i] = (*next_ids)[i];
        queue_parents[tail + i] = parent;
      }
      tail += i;
    }

//列表中没有其他项，表示没有死锁。
    if (tail == head) {
      return false;
    }

    auto next = queue_values[head];
    if (next == id) {
      std::vector<DeadlockInfo> path;
      while (head != -1) {
        assert(wait_txn_map_.Contains(queue_values[head]));

        auto extracted_info = wait_txn_map_.Get(queue_values[head]);
        path.push_back({queue_values[head], extracted_info.m_cf_id,
                        extracted_info.m_waiting_key,
                        extracted_info.m_exclusive});
        head = queue_parents[head];
      }
      std::reverse(path.begin(), path.end());
      dlock_buffer_.AddNewPath(DeadlockPath(path));
      DecrementWaitersImpl(txn, wait_ids);
      return true;
    } else if (!wait_txn_map_.Contains(next)) {
      next_ids = nullptr;
      continue;
    } else {
      parent = head;
      next_ids = &(wait_txn_map_.Get(next).m_neighbors);
    }
  }

//等待周期太大，只需假设死锁。
  dlock_buffer_.AddNewPath(DeadlockPath(true));
  DecrementWaitersImpl(txn, wait_ids);
  return true;
}

//获取互斥体后，尝试锁定此密钥。
//将*过期时间设置为以微秒为单位的过期时间
//如果没有过期，则为0。
//必需：必须保持条带互斥。
Status TransactionLockMgr::AcquireLocked(LockMap* lock_map,
                                         LockMapStripe* stripe,
                                         const std::string& key, Env* env,
                                         const LockInfo& txn_lock_info,
                                         uint64_t* expire_time,
                                         autovector<TransactionID>* txn_ids) {
  assert(txn_lock_info.txn_ids.size() == 1);

  Status result;
//检查此钥匙是否已锁定
  if (stripe->keys.find(key) != stripe->keys.end()) {
//锁定已保持
    LockInfo& lock_info = stripe->keys.at(key);
    assert(lock_info.txn_ids.size() == 1 || !lock_info.exclusive);

    if (lock_info.exclusive || txn_lock_info.exclusive) {
      if (lock_info.txn_ids.size() == 1 &&
          lock_info.txn_ids[0] == txn_lock_info.txn_ids[0]) {
//列表中包含一个txn，我们就是它，所以就拿着它。
        lock_info.exclusive = txn_lock_info.exclusive;
        lock_info.expiration_time = txn_lock_info.expiration_time;
      } else {
//检查是否过期。跳过txn_lock_info.txn_ids[0]以防
//它是为一个共享锁与多个持有人，而不是
//在第一个案件中被捕。
        if (IsLockExpired(txn_lock_info.txn_ids[0], lock_info, env,
                          expire_time)) {
//锁过期了，可以偷了
          lock_info.txn_ids = txn_lock_info.txn_ids;
          lock_info.exclusive = txn_lock_info.exclusive;
          lock_info.expiration_time = txn_lock_info.expiration_time;
//锁定不改变
        } else {
          result = Status::TimedOut(Status::SubCode::kLockTimeout);
          *txn_ids = lock_info.txn_ids;
        }
      }
    } else {
//我们请求共享对共享锁的访问，所以只需授予它。
      lock_info.txn_ids.push_back(txn_lock_info.txn_ids[0]);
//使用std:：max意味着即使在
//事务将从列表中删除。正确的解决办法是
//跟踪每个事务的到期时间，但这也适用于
//现在。
      lock_info.expiration_time =
          std::max(lock_info.expiration_time, txn_lock_info.expiration_time);
    }
} else {  //锁未被保存。
//检查锁定限制
    if (max_num_locks_ > 0 &&
        lock_map->lock_cnt.load(std::memory_order_acquire) >= max_num_locks_) {
      result = Status::Busy(Status::SubCode::kLockLimit);
    } else {
//获取锁定
      stripe->keys.insert({key, txn_lock_info});

//如果锁的数量有限制，则保持锁计数
      if (max_num_locks_) {
        lock_map->lock_cnt++;
      }
    }
  }

  return result;
}

void TransactionLockMgr::UnLockKey(const PessimisticTransaction* txn,
                                   const std::string& key,
                                   LockMapStripe* stripe, LockMap* lock_map,
                                   Env* env) {
  TransactionID txn_id = txn->GetID();

  auto stripe_iter = stripe->keys.find(key);
  if (stripe_iter != stripe->keys.end()) {
    auto& txns = stripe_iter->second.txn_ids;
    auto txn_it = std::find(txns.begin(), txns.end(), txn_id);
//找到了我们锁的钥匙。解锁它。
    if (txn_it != txns.end()) {
      if (txns.size() == 1) {
        stripe->keys.erase(stripe_iter);
      } else {
        auto last_it = txns.end() - 1;
        if (txn_it != last_it) {
          *txn_it = *last_it;
        }
        txns.pop_back();
      }

      if (max_num_locks_ > 0) {
//如果锁的数量有限制，请保持锁的数量。
        assert(lock_map->lock_cnt.load(std::memory_order_relaxed) > 0);
        lock_map->lock_cnt--;
      }
    }
  } else {
//这把钥匙不是被别人锁着就是被别人锁着。这应该
//仅在解锁事务已过期时发生。
    assert(txn->GetExpirationTime() > 0 &&
           txn->GetExpirationTime() < env->NowMicros());
  }
}

void TransactionLockMgr::UnLock(PessimisticTransaction* txn,
                                uint32_t column_family_id,
                                const std::string& key, Env* env) {
  std::shared_ptr<LockMap> lock_map_ptr = GetLockMap(column_family_id);
  LockMap* lock_map = lock_map_ptr.get();
  if (lock_map == nullptr) {
//列族必须已删除。
    return;
  }

//锁定此密钥哈希到的条带的互斥体
  size_t stripe_num = lock_map->GetStripe(key);
  assert(lock_map->lock_map_stripes_.size() > stripe_num);
  LockMapStripe* stripe = lock_map->lock_map_stripes_.at(stripe_num);

  stripe->stripe_mutex->Lock();
  UnLockKey(txn, key, stripe, lock_map, env);
  stripe->stripe_mutex->UnLock();

//发出等待线程重试锁定的信号
  stripe->stripe_cv->NotifyAll();
}

void TransactionLockMgr::UnLock(const PessimisticTransaction* txn,
                                const TransactionKeyMap* key_map, Env* env) {
  for (auto& key_map_iter : *key_map) {
    uint32_t column_family_id = key_map_iter.first;
    auto& keys = key_map_iter.second;

    std::shared_ptr<LockMap> lock_map_ptr = GetLockMap(column_family_id);
    LockMap* lock_map = lock_map_ptr.get();

    if (lock_map == nullptr) {
//列族必须已删除。
      return;
    }

//按锁定条显示的bucket键
    std::unordered_map<size_t, std::vector<const std::string*>> keys_by_stripe(
        std::max(keys.size(), lock_map->num_stripes_));

    for (auto& key_iter : keys) {
      const std::string& key = key_iter.first;

      size_t stripe_num = lock_map->GetStripe(key);
      keys_by_stripe[stripe_num].push_back(&key);
    }

//对于每个条带，获取条带互斥并解锁此条带中的所有密钥
    for (auto& stripe_iter : keys_by_stripe) {
      size_t stripe_num = stripe_iter.first;
      auto& stripe_keys = stripe_iter.second;

      assert(lock_map->lock_map_stripes_.size() > stripe_num);
      LockMapStripe* stripe = lock_map->lock_map_stripes_.at(stripe_num);

      stripe->stripe_mutex->Lock();

      for (const std::string* key : stripe_keys) {
        UnLockKey(txn, *key, stripe, lock_map, env);
      }

      stripe->stripe_mutex->UnLock();

//发出等待线程重试锁定的信号
      stripe->stripe_cv->NotifyAll();
    }
  }
}

TransactionLockMgr::LockStatusData TransactionLockMgr::GetLockStatusData() {
  LockStatusData data;
//这里的锁定顺序很重要。正确的顺序是锁定\u映射\u互斥，然后
//对于每个列族ID，按升序锁定
//升序。
  InstrumentedMutexLock l(&lock_map_mutex_);

  std::vector<uint32_t> cf_ids;
  for (const auto& map : lock_maps_) {
    cf_ids.push_back(map.first);
  }
  std::sort(cf_ids.begin(), cf_ids.end());

  for (auto i : cf_ids) {
    const auto& stripes = lock_maps_[i]->lock_map_stripes_;
//按升序迭代并锁定所有条带。
    for (const auto& j : stripes) {
      j->stripe_mutex->Lock();
      for (const auto& it : j->keys) {
        struct KeyLockInfo info;
        info.exclusive = it.second.exclusive;
        info.key = it.first;
        for (const auto& id : it.second.txn_ids) {
          info.ids.push_back(id);
        }
        data.insert({i, info});
      }
    }
  }

//解锁所有东西。解锁顺序并不重要。
  for (auto i : cf_ids) {
    const auto& stripes = lock_maps_[i]->lock_map_stripes_;
    for (const auto& j : stripes) {
      j->stripe_mutex->UnLock();
    }
  }

  return data;
}
std::vector<DeadlockPath> TransactionLockMgr::GetDeadlockInfoBuffer() {
  return dlock_buffer_.PrepareBuffer();
}

void TransactionLockMgr::Resize(uint32_t target_size) {
  dlock_buffer_.Resize(target_size);
}

}  //命名空间rocksdb
#endif  //摇滚乐
