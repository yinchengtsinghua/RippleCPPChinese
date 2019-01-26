
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

#include "monitoring/thread_status_updater.h"
#include <memory>
#include "port/likely.h"
#include "rocksdb/env.h"
#include "util/mutexlock.h"

namespace rocksdb {

#ifdef ROCKSDB_USING_THREAD_STATUS

__thread ThreadStatusData* ThreadStatusUpdater::thread_status_data_ = nullptr;

void ThreadStatusUpdater::RegisterThread(
    ThreadStatus::ThreadType ttype, uint64_t thread_id) {
  if (UNLIKELY(thread_status_data_ == nullptr)) {
    thread_status_data_ = new ThreadStatusData();
    thread_status_data_->thread_type = ttype;
    thread_status_data_->thread_id = thread_id;
    std::lock_guard<std::mutex> lck(thread_list_mutex_);
    thread_data_set_.insert(thread_status_data_);
  }

  ClearThreadOperationProperties();
}

void ThreadStatusUpdater::UnregisterThread() {
  if (thread_status_data_ != nullptr) {
    std::lock_guard<std::mutex> lck(thread_list_mutex_);
    thread_data_set_.erase(thread_status_data_);
    delete thread_status_data_;
    thread_status_data_ = nullptr;
  }
}

void ThreadStatusUpdater::ResetThreadStatus() {
  ClearThreadState();
  ClearThreadOperation();
  SetColumnFamilyInfoKey(nullptr);
}

void ThreadStatusUpdater::SetColumnFamilyInfoKey(
    const void* cf_key) {
  auto* data = Get();
  if (data == nullptr) {
    return;
  }
//根据是否为非空设置跟踪标志。
//如果启用线程跟踪设置为假，则输入cf_键
//将为nullptr。
  data->enable_tracking = (cf_key != nullptr);
  data->cf_key.store(const_cast<void*>(cf_key), std::memory_order_relaxed);
}

const void* ThreadStatusUpdater::GetColumnFamilyInfoKey() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return nullptr;
  }
  return data->cf_key.load(std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetThreadOperation(
    const ThreadStatus::OperationType type) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
//注意：我们这里的做法是设置所有线程操作属性
//设置线程操作和线程操作之前的阶段
//将在std:：memory_order_release中设置。这是为了确保
//每当线程操作不是opu unknown时，我们将始终
//对其属性有一致的信息。
  data->operation_type.store(type, std::memory_order_release);
  if (type == ThreadStatus::OP_UNKNOWN) {
    data->operation_stage.store(ThreadStatus::STAGE_UNKNOWN,
        std::memory_order_relaxed);
    ClearThreadOperationProperties();
  }
}

void ThreadStatusUpdater::SetThreadOperationProperty(
    int i, uint64_t value) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_properties[i].store(value, std::memory_order_relaxed);
}

void ThreadStatusUpdater::IncreaseThreadOperationProperty(
    int i, uint64_t delta) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_properties[i].fetch_add(delta, std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetOperationStartTime(const uint64_t start_time) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->op_start_time.store(start_time, std::memory_order_relaxed);
}

void ThreadStatusUpdater::ClearThreadOperation() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->operation_stage.store(ThreadStatus::STAGE_UNKNOWN,
      std::memory_order_relaxed);
  data->operation_type.store(
      ThreadStatus::OP_UNKNOWN, std::memory_order_relaxed);
  ClearThreadOperationProperties();
}

void ThreadStatusUpdater::ClearThreadOperationProperties() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  for (int i = 0; i < ThreadStatus::kNumOperationProperties; ++i) {
    data->op_properties[i].store(0, std::memory_order_relaxed);
  }
}

ThreadStatus::OperationStage ThreadStatusUpdater::SetThreadOperationStage(
    ThreadStatus::OperationStage stage) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return ThreadStatus::STAGE_UNKNOWN;
  }
  return data->operation_stage.exchange(
      stage, std::memory_order_relaxed);
}

void ThreadStatusUpdater::SetThreadState(
    const ThreadStatus::StateType type) {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->state_type.store(type, std::memory_order_relaxed);
}

void ThreadStatusUpdater::ClearThreadState() {
  auto* data = GetLocalThreadStatus();
  if (data == nullptr) {
    return;
  }
  data->state_type.store(
      ThreadStatus::STATE_UNKNOWN, std::memory_order_relaxed);
}

Status ThreadStatusUpdater::GetThreadList(
    std::vector<ThreadStatus>* thread_list) {
  thread_list->clear();
  std::vector<std::shared_ptr<ThreadStatusData>> valid_list;
  uint64_t now_micros = Env::Default()->NowMicros();

  std::lock_guard<std::mutex> lck(thread_list_mutex_);
  for (auto* thread_data : thread_data_set_) {
    assert(thread_data);
    auto thread_id = thread_data->thread_id.load(
        std::memory_order_relaxed);
    auto thread_type = thread_data->thread_type.load(
        std::memory_order_relaxed);
//由于对cf_info_map的任何更改都需要线程_list_mutex，
//它目前由getthreadlist（）持有，在这里我们可以安全地
//使用“记忆顺序”来加载cf_键。
    auto cf_key = thread_data->cf_key.load(
        std::memory_order_relaxed);
    auto iter = cf_info_map_.find(cf_key);
    auto* cf_info = iter != cf_info_map_.end() ?
        iter->second.get() : nullptr;
    const std::string* db_name = nullptr;
    const std::string* cf_name = nullptr;
    ThreadStatus::OperationType op_type = ThreadStatus::OP_UNKNOWN;
    ThreadStatus::OperationStage op_stage = ThreadStatus::STAGE_UNKNOWN;
    ThreadStatus::StateType state_type = ThreadStatus::STATE_UNKNOWN;
    uint64_t op_elapsed_micros = 0;
    uint64_t op_props[ThreadStatus::kNumOperationProperties] = {0};
    if (cf_info != nullptr) {
      db_name = &cf_info->db_name;
      cf_name = &cf_info->cf_name;
      op_type = thread_data->operation_type.load(
          std::memory_order_acquire);
//只有在高级信息可用时才显示低级信息。
      if (op_type != ThreadStatus::OP_UNKNOWN) {
        op_elapsed_micros = now_micros - thread_data->op_start_time.load(
            std::memory_order_relaxed);
        op_stage = thread_data->operation_stage.load(
            std::memory_order_relaxed);
        state_type = thread_data->state_type.load(
            std::memory_order_relaxed);
        for (int i = 0; i < ThreadStatus::kNumOperationProperties; ++i) {
          op_props[i] = thread_data->op_properties[i].load(
              std::memory_order_relaxed);
        }
      }
    }
    thread_list->emplace_back(
        thread_id, thread_type,
        db_name ? *db_name : "",
        cf_name ? *cf_name : "",
        op_type, op_elapsed_micros, op_stage, op_props,
        state_type);
  }

  return Status::OK();
}

ThreadStatusData* ThreadStatusUpdater::GetLocalThreadStatus() {
  if (thread_status_data_ == nullptr) {
    return nullptr;
  }
  if (!thread_status_data_->enable_tracking) {
    assert(thread_status_data_->cf_key.load(
        std::memory_order_relaxed) == nullptr);
    return nullptr;
  }
  return thread_status_data_;
}

void ThreadStatusUpdater::NewColumnFamilyInfo(
    const void* db_key, const std::string& db_name,
    const void* cf_key, const std::string& cf_name) {
//获取与getthreadlist（）相同的锁以保证
//全局列族表的一致视图（cf_info_map）。
  std::lock_guard<std::mutex> lck(thread_list_mutex_);

  cf_info_map_[cf_key].reset(
      new ConstantColumnFamilyInfo(db_key, db_name, cf_name));
  db_key_map_[db_key].insert(cf_key);
}

void ThreadStatusUpdater::EraseColumnFamilyInfo(const void* cf_key) {
//获取与getthreadlist（）相同的锁以保证
//全局列族表的一致视图（cf_info_map）。
  std::lock_guard<std::mutex> lck(thread_list_mutex_);
  auto cf_pair = cf_info_map_.find(cf_key);
  if (cf_pair == cf_info_map_.end()) {
    return;
  }

  auto* cf_info = cf_pair->second.get();
  assert(cf_info);

//通过以下步骤从db_key_map_u中删除其条目：
//1。获取db_key_map_u中包含cf_key的条目
//2。从集合中移除它。
  auto db_pair = db_key_map_.find(cf_info->db_key);
  assert(db_pair != db_key_map_.end());
  size_t result __attribute__((unused)) = db_pair->second.erase(cf_key);
  assert(result);

  cf_pair->second.reset();
  result = cf_info_map_.erase(cf_key);
  assert(result);
}

void ThreadStatusUpdater::EraseDatabaseInfo(const void* db_key) {
//获取与getthreadlist（）相同的锁以保证
//全局列族表的一致视图（cf_info_map）。
  std::lock_guard<std::mutex> lck(thread_list_mutex_);
  auto db_pair = db_key_map_.find(db_key);
  if (UNLIKELY(db_pair == db_key_map_.end())) {
//在某些偶然情况下，如db:：open失败，我们不会
//为数据库注册ColumnFamilyInfo。
    return;
  }

  size_t result __attribute__((unused)) = 0;
  for (auto cf_key : db_pair->second) {
    auto cf_pair = cf_info_map_.find(cf_key);
    if (cf_pair == cf_info_map_.end()) {
      continue;
    }
    cf_pair->second.reset();
    result = cf_info_map_.erase(cf_key);
    assert(result);
  }
  db_key_map_.erase(db_key);
}

#else

void ThreadStatusUpdater::RegisterThread(
    ThreadStatus::ThreadType ttype, uint64_t thread_id) {
}

void ThreadStatusUpdater::UnregisterThread() {
}

void ThreadStatusUpdater::ResetThreadStatus() {
}

void ThreadStatusUpdater::SetColumnFamilyInfoKey(
    const void* cf_key) {
}

void ThreadStatusUpdater::SetThreadOperation(
    const ThreadStatus::OperationType type) {
}

void ThreadStatusUpdater::ClearThreadOperation() {
}

void ThreadStatusUpdater::SetThreadState(
    const ThreadStatus::StateType type) {
}

void ThreadStatusUpdater::ClearThreadState() {
}

Status ThreadStatusUpdater::GetThreadList(
    std::vector<ThreadStatus>* thread_list) {
  return Status::NotSupported(
      "GetThreadList is not supported in the current running environment.");
}

void ThreadStatusUpdater::NewColumnFamilyInfo(
    const void* db_key, const std::string& db_name,
    const void* cf_key, const std::string& cf_name) {
}

void ThreadStatusUpdater::EraseColumnFamilyInfo(const void* cf_key) {
}

void ThreadStatusUpdater::EraseDatabaseInfo(const void* db_key) {
}

void ThreadStatusUpdater::SetThreadOperationProperty(
    int i, uint64_t value) {
}

void ThreadStatusUpdater::IncreaseThreadOperationProperty(
    int i, uint64_t delta) {
}

#endif  //使用线程状态的rocksdb_
}  //命名空间rocksdb
