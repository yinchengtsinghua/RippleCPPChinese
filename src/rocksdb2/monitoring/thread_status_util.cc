
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

#include "monitoring/thread_status_util.h"

#include "monitoring/thread_status_updater.h"
#include "rocksdb/env.h"

namespace rocksdb {


#ifdef ROCKSDB_USING_THREAD_STATUS
__thread ThreadStatusUpdater*
    ThreadStatusUtil::thread_updater_local_cache_ = nullptr;
__thread bool ThreadStatusUtil::thread_updater_initialized_ = false;

void ThreadStatusUtil::RegisterThread(
    const Env* env, ThreadStatus::ThreadType thread_type) {
  if (!MaybeInitThreadLocalUpdater(env)) {
    return;
  }
  assert(thread_updater_local_cache_);
  thread_updater_local_cache_->RegisterThread(
      thread_type, env->GetThreadID());
}

void ThreadStatusUtil::UnregisterThread() {
  thread_updater_initialized_ = false;
  if (thread_updater_local_cache_ != nullptr) {
    thread_updater_local_cache_->UnregisterThread();
    thread_updater_local_cache_ = nullptr;
  }
}

void ThreadStatusUtil::SetColumnFamily(const ColumnFamilyData* cfd,
                                       const Env* env,
                                       bool enable_thread_tracking) {
  if (!MaybeInitThreadLocalUpdater(env)) {
    return;
  }
  assert(thread_updater_local_cache_);
  if (cfd != nullptr && enable_thread_tracking) {
    thread_updater_local_cache_->SetColumnFamilyInfoKey(cfd);
  } else {
//当cfd==nullptr或启用线程跟踪==false时，我们设置
//columnFamilyInfoKey到nullptr，这使得setthreadooperation
//而setthreadstate变成了no-op。
    thread_updater_local_cache_->SetColumnFamilyInfoKey(nullptr);
  }
}

void ThreadStatusUtil::SetThreadOperation(ThreadStatus::OperationType op) {
  if (thread_updater_local_cache_ == nullptr) {
//必须在setcolumnfamily中设置线程更新程序本地缓存
//或其他threadstatusUtil函数。
    return;
  }

  if (op != ThreadStatus::OP_UNKNOWN) {
    uint64_t current_time = Env::Default()->NowMicros();
    thread_updater_local_cache_->SetOperationStartTime(current_time);
  } else {
//tdoo（yzhang）：我们可以报告我们将行动的时间
//一旦完成整个仪器，就不知道了。
    thread_updater_local_cache_->SetOperationStartTime(0);
  }
  thread_updater_local_cache_->SetThreadOperation(op);
}

ThreadStatus::OperationStage ThreadStatusUtil::SetThreadOperationStage(
    ThreadStatus::OperationStage stage) {
  if (thread_updater_local_cache_ == nullptr) {
//必须在setcolumnfamily中设置线程更新程序本地缓存
//或其他threadstatusUtil函数。
    return ThreadStatus::STAGE_UNKNOWN;
  }

  return thread_updater_local_cache_->SetThreadOperationStage(stage);
}

void ThreadStatusUtil::SetThreadOperationProperty(
    int code, uint64_t value) {
  if (thread_updater_local_cache_ == nullptr) {
//必须在setcolumnfamily中设置线程更新程序本地缓存
//或其他threadstatusUtil函数。
    return;
  }

  thread_updater_local_cache_->SetThreadOperationProperty(
      code, value);
}

void ThreadStatusUtil::IncreaseThreadOperationProperty(
    int code, uint64_t delta) {
  if (thread_updater_local_cache_ == nullptr) {
//必须在setcolumnfamily中设置线程更新程序本地缓存
//或其他threadstatusUtil函数。
    return;
  }

  thread_updater_local_cache_->IncreaseThreadOperationProperty(
      code, delta);
}

void ThreadStatusUtil::SetThreadState(ThreadStatus::StateType state) {
  if (thread_updater_local_cache_ == nullptr) {
//必须在setcolumnfamily中设置线程更新程序本地缓存
//或其他threadstatusUtil函数。
    return;
  }

  thread_updater_local_cache_->SetThreadState(state);
}

void ThreadStatusUtil::ResetThreadStatus() {
  if (thread_updater_local_cache_ == nullptr) {
    return;
  }
  thread_updater_local_cache_->ResetThreadStatus();
}

void ThreadStatusUtil::NewColumnFamilyInfo(const DB* db,
                                           const ColumnFamilyData* cfd,
                                           const std::string& cf_name,
                                           const Env* env) {
  if (!MaybeInitThreadLocalUpdater(env)) {
    return;
  }
  assert(thread_updater_local_cache_);
  if (thread_updater_local_cache_) {
    thread_updater_local_cache_->NewColumnFamilyInfo(db, db->GetName(), cfd,
                                                     cf_name);
  }
}

void ThreadStatusUtil::EraseColumnFamilyInfo(
    const ColumnFamilyData* cfd) {
  if (thread_updater_local_cache_ == nullptr) {
    return;
  }
  thread_updater_local_cache_->EraseColumnFamilyInfo(cfd);
}

void ThreadStatusUtil::EraseDatabaseInfo(const DB* db) {
  ThreadStatusUpdater* thread_updater = db->GetEnv()->GetThreadStatusUpdater();
  if (thread_updater == nullptr) {
    return;
  }
  thread_updater->EraseDatabaseInfo(db);
}

bool ThreadStatusUtil::MaybeInitThreadLocalUpdater(const Env* env) {
  if (!thread_updater_initialized_ && env != nullptr) {
    thread_updater_initialized_ = true;
    thread_updater_local_cache_ = env->GetThreadStatusUpdater();
  }
  return (thread_updater_local_cache_ != nullptr);
}

AutoThreadOperationStageUpdater::AutoThreadOperationStageUpdater(
    ThreadStatus::OperationStage stage) {
  prev_stage_ = ThreadStatusUtil::SetThreadOperationStage(stage);
}

AutoThreadOperationStageUpdater::~AutoThreadOperationStageUpdater() {
  ThreadStatusUtil::SetThreadOperationStage(prev_stage_);
}

#else

ThreadStatusUpdater* ThreadStatusUtil::thread_updater_local_cache_ = nullptr;
bool ThreadStatusUtil::thread_updater_initialized_ = false;

bool ThreadStatusUtil::MaybeInitThreadLocalUpdater(const Env* env) {
  return false;
}

void ThreadStatusUtil::SetColumnFamily(const ColumnFamilyData* cfd,
                                       const Env* env,
                                       bool enable_thread_tracking) {}

void ThreadStatusUtil::SetThreadOperation(ThreadStatus::OperationType op) {
}

void ThreadStatusUtil::SetThreadOperationProperty(
    int code, uint64_t value) {
}

void ThreadStatusUtil::IncreaseThreadOperationProperty(
    int code, uint64_t delta) {
}

void ThreadStatusUtil::SetThreadState(ThreadStatus::StateType state) {
}

void ThreadStatusUtil::NewColumnFamilyInfo(const DB* db,
                                           const ColumnFamilyData* cfd,
                                           const std::string& cf_name,
                                           const Env* env) {}

void ThreadStatusUtil::EraseColumnFamilyInfo(
    const ColumnFamilyData* cfd) {
}

void ThreadStatusUtil::EraseDatabaseInfo(const DB* db) {
}

void ThreadStatusUtil::ResetThreadStatus() {
}

AutoThreadOperationStageUpdater::AutoThreadOperationStageUpdater(
    ThreadStatus::OperationStage stage) {
}

AutoThreadOperationStageUpdater::~AutoThreadOperationStageUpdater() {
}

#endif  //使用线程状态的rocksdb_

}  //命名空间rocksdb
