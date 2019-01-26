
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
//
//threadstatus的实现。
//
//请注意，我们让get和set访问threadstatusdata无锁。
//因此，threadstatusdata作为一个整体不是原子的。然而，
//我们保证在任何时候
//用户调用GetThreadList（）。此一致性保证已完成
//通过在内部实现中具有以下约束
//集合和获取顺序：
//
//1。当重置threadstatusdata中的任何信息时，始终从
//首先清除低级信息。
//2。在threadstatusdata中设置任何信息时，始终从
//设置更高级别的信息。
//三。将threadstatusdata返回给用户时，字段从
//从高到低。此外，如果存在nullptr
//在一个字段中，则所有级别低于该字段的字段
//应该被忽略。
//
//从高到低的信息将是：
//线程类型>数据库>操作>状态
//
//这意味着用户可能不会总是获得完整的信息，但无论何时
//getthreadlist（）返回的值保证一致。
#pragma once
#include <atomic>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rocksdb/status.h"
#include "rocksdb/thread_status.h"
#include "port/port.h"
#include "util/thread_operation.h"

namespace rocksdb {

class ColumnFamilyHandle;

//保持列族信息不变的结构。
struct ConstantColumnFamilyInfo {
#ifdef ROCKSDB_USING_THREAD_STATUS
 public:
  ConstantColumnFamilyInfo(
      const void* _db_key,
      const std::string& _db_name,
      const std::string& _cf_name) :
      db_key(_db_key), db_name(_db_name), cf_name(_cf_name) {}
  const void* db_key;
  const std::string db_name;
  const std::string cf_name;
#endif  //使用线程状态的rocksdb_
};

//用于反映当前数据的内部数据结构
//使用一组原子指针的线程的状态。
struct ThreadStatusData {
#ifdef ROCKSDB_USING_THREAD_STATUS
  explicit ThreadStatusData() : enable_tracking(false) {
    thread_id.store(0);
    thread_type.store(ThreadStatus::USER);
    cf_key.store(nullptr);
    operation_type.store(ThreadStatus::OP_UNKNOWN);
    op_start_time.store(0);
    state_type.store(ThreadStatus::STATE_UNKNOWN);
  }

//指示是否启用线程跟踪的标志
//在当前线程中。此值将根据是否
//关联选项：：启用线程跟踪设置为true
//在threadstatusUtil:：setColumnFamily（）中。
//
//如果设置为false，则设置hreadooperation和setthreadstate
//将不是OP。
  bool enable_tracking;

  std::atomic<uint64_t> thread_id;
  std::atomic<ThreadStatus::ThreadType> thread_type;
  std::atomic<void*> cf_key;
  std::atomic<ThreadStatus::OperationType> operation_type;
  std::atomic<uint64_t> op_start_time;
  std::atomic<ThreadStatus::OperationStage> operation_stage;
  std::atomic<uint64_t> op_properties[ThreadStatus::kNumOperationProperties];
  std::atomic<ThreadStatus::StateType> state_type;
#endif  //使用线程状态的rocksdb_
};

//存储和更新当前线程状态的类
//使用线程本地线程状态数据。
//
//在大多数情况下，应该使用threadstatusUtil更新
//当前线程的状态，而不是使用ThreadStatusUpdater
//直接。
//
//@见threadstatusutil
class ThreadStatusUpdater {
 public:
  ThreadStatusUpdater() {}

//释放所有活动线程的所有线程状态数据。
  virtual ~ThreadStatusUpdater() {}

//注销当前线程。
  void UnregisterThread();

//重置当前线程的状态。这包括重置
//ColumnFamilyInfoKey、ThreadOperation和ThreadState。
  void ResetThreadStatus();

//设置当前线程的ID。
  void SetThreadID(uint64_t thread_id);

//注册当前线程进行跟踪。
  void RegisterThread(ThreadStatus::ThreadType ttype, uint64_t thread_id);

//通过设置更新当前线程的列族信息
//它的线程本地指针threadstateinfo指向正确的条目。
  void SetColumnFamilyInfoKey(const void* cf_key);

//返回列族信息键。
  const void* GetColumnFamilyInfoKey();

//更新当前线程的线程操作。
  void SetThreadOperation(const ThreadStatus::OperationType type);

//当前线程操作的开始时间。格式是这样的
//从某个固定时间点开始的微秒。
  void SetOperationStartTime(const uint64_t start_time);

//设置当前操作的“i”th属性。
//
//注意：我们这里的做法是设置所有线程操作属性
//设置线程操作和线程操作之前的阶段
//将在std:：memory_order_release中设置。这是为了确保
//每当线程操作不是opu unknown时，我们将始终
//对其属性有一致的信息。
  void SetThreadOperationProperty(
      int i, uint64_t value);

//增加当前操作的“i”th属性
//指定的增量。
  void IncreaseThreadOperationProperty(
      int i, uint64_t delta);

//更新当前线程的线程操作阶段。
  ThreadStatus::OperationStage SetThreadOperationStage(
      const ThreadStatus::OperationStage stage);

//清除当前线程的线程操作。
  void ClearThreadOperation();

//将所有线程操作属性重置为0。
  void ClearThreadOperationProperties();

//更新当前线程的线程状态。
  void SetThreadState(const ThreadStatus::StateType type);

//清除当前线程的线程状态。
  void ClearThreadState();

//获取所有活动注册线程的状态。
  Status GetThreadList(
      std::vector<ThreadStatus>* thread_list);

//在Global ColumnFamilyInfo表中为
//指定的列族。只应调用此函数
//当当前线程不保持db_mutex时。
  void NewColumnFamilyInfo(
      const void* db_key, const std::string& db_name,
      const void* cf_key, const std::string& cf_name);

//删除与
//指定的数据库实例。只有当
//当前线程不包含db_mutex。
  void EraseDatabaseInfo(const void* db_key);

//删除与
//指定的ColumnFamilyData。只应调用此函数
//当当前线程不保持db_mutex时。
  void EraseColumnFamilyInfo(const void* cf_key);

//验证输入列FamilyHandles是否匹配
//存储在当前cf_info_映射中的信息。
  void TEST_VerifyColumnFamilyInfoMap(
      const std::vector<ColumnFamilyHandle*>& handles,
      bool check_exist);

 protected:
#ifdef ROCKSDB_USING_THREAD_STATUS
//用于存储线程状态的线程局部变量。
  static __thread ThreadStatusData* thread_status_data_;

//仅当
//线程状态数据不为空，并且启用了\u tracking==true。
  ThreadStatusData* GetLocalThreadStatus();

//直接返回指向线程状态数据的指针
//正在检查启用跟踪是否正确。
  ThreadStatusData* Get() {
    return thread_status_data_;
  }

//保护cf_info_map和db_key_map的互斥体。
  std::mutex thread_list_mutex_;

//所有活动线程的当前状态数据。
  std::unordered_set<ThreadStatusData*> thread_data_set_;

//保留列族信息的全局映射。它被储存起来
//全局而不是内部数据库是为了避免数据库
//当getthreadlist函数已获取指向其
//CopInstantColumnFamilyInfo。
  std::unordered_map<
      const void*, std::unique_ptr<ConstantColumnFamilyInfo>> cf_info_map_;

//一个DB-U键到CF-U键映射，允许删除CF-U信息映射中的元素。
//更快地关联到同一个db_密钥。
  std::unordered_map<
      const void*, std::unordered_set<const void*>> db_key_map_;

#else
  static ThreadStatusData* thread_status_data_;
#endif  //使用线程状态的rocksdb_
};

}  //命名空间rocksdb
