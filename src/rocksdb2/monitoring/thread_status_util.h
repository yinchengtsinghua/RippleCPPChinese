
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

#include <string>

#include "monitoring/thread_status_updater.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/thread_status.h"

namespace rocksdb {

class ColumnFamilyData;

//用于更新线程本地状态的静态实用程序类。
//
//通过线程本地缓存更新线程本地状态
//指针线程更新程序本地缓存。在每次函数调用期间，
//当threadstatusUtil发现thread_updater_local_cache_u时
//未初始化（由线程_updater_initialized_uu确定），
//它将尝试使用返回值初始化
//env:：getthreadstatusupdater（）。当线程更新程序本地缓存时
//由非空指针初始化，每个函数调用将
//然后更新当前线程的状态。否则，
//对threadstatusUtil的所有函数调用都将是no-op。
class ThreadStatusUtil {
 public:
//注册当前线程进行跟踪。
  static void RegisterThread(
      const Env* env, ThreadStatus::ThreadType thread_type);

//注销当前线程。
  static void UnregisterThread();

//在Global ColumnFamilyInfo表中为
//指定的列族。只应调用此函数
//当当前线程不保持db_mutex时。
  static void NewColumnFamilyInfo(const DB* db, const ColumnFamilyData* cfd,
                                  const std::string& cf_name, const Env* env);

//删除与
//指定的ColumnFamilyData。只应调用此函数
//当当前线程不保持db_mutex时。
  static void EraseColumnFamilyInfo(const ColumnFamilyData* cfd);

//删除与
//指定的数据库实例。只有当
//当前线程不包含db_mutex。
  static void EraseDatabaseInfo(const DB* db);

//更新线程状态以指示当前线程正在执行的操作
//与指定的列族相关的内容。
  static void SetColumnFamily(const ColumnFamilyData* cfd, const Env* env,
                              bool enable_thread_tracking);

  static void SetThreadOperation(ThreadStatus::OperationType type);

  static ThreadStatus::OperationStage SetThreadOperationStage(
      ThreadStatus::OperationStage stage);

  static void SetThreadOperationProperty(
      int code, uint64_t value);

  static void IncreaseThreadOperationProperty(
      int code, uint64_t delta);

  static void SetThreadState(ThreadStatus::StateType type);

  static void ResetThreadStatus();

#ifndef NDEBUG
  static void TEST_SetStateDelay(
      const ThreadStatus::StateType state, int micro);
  static void TEST_StateDelay(const ThreadStatus::StateType state);
#endif

 protected:
//当线程找到本地threadstatusupdater时初始化该线程
//缓存值为nullptr。如果已缓存，则返回true
//非空指针。
  static bool MaybeInitThreadLocalUpdater(const Env* env);

#ifdef ROCKSDB_USING_THREAD_STATUS
//一个布尔标志，指示线程\u更新程序\u本地\u缓存\u
//已初始化。当env使用任何
//threadstatusUtil函数使用当前线程其他
//而不是unregisterThread（）。当
//调用UnregisterThread（）。
//
//当此变量设置为true时，线程\u更新程序\u本地\u缓存\u
//在该变量再次设置为false之前不会更新
//在UnregisterThread（）中。
  static  __thread bool thread_updater_initialized_;

//用于缓存
//使用任何threadstatusUtil的第一个env的thread_status_updater_uu
//函数而不是unregisterThread（）。这个变量将
//调用UnregisterThread（）时清除。
//
//当此变量设置为非空指针时，则状态
//当函数
//调用threadstatusUtil。否则，所有功能
//threadstatusutil将不可用。
//
//当线程更新程序初始化为真时，这个变量
//在这个线程的更新程序初始化之前不会被更新
//在unregisterThread（）中再次设置为false。
  static __thread ThreadStatusUpdater* thread_updater_local_cache_;
#else
  static bool thread_updater_initialized_;
  static ThreadStatusUpdater* thread_updater_local_cache_;
#endif
};

//用于更新线程状态的帮助程序类。它将设置
//线程状态根据其构造函数中的输入参数
//并在其析构函数中将线程状态设置为前一状态。
class AutoThreadOperationStageUpdater {
 public:
  explicit AutoThreadOperationStageUpdater(
      ThreadStatus::OperationStage stage);
  ~AutoThreadOperationStageUpdater();

#ifdef ROCKSDB_USING_THREAD_STATUS
 private:
  ThreadStatus::OperationStage prev_stage_;
#endif
};

}  //命名空间rocksdb
