
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#include "util/threadpool_imp.h"

#include "monitoring/thread_status_util.h"
#include "port/port.h"

#ifndef OS_WIN
#  include <unistd.h>
#endif

#ifdef OS_LINUX
#  include <sys/syscall.h>
#endif

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <stdlib.h>
#include <thread>
#include <vector>

namespace rocksdb {

void ThreadPoolImpl::PthreadCall(const char* label, int result) {
  if (result != 0) {
    fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
    abort();
  }
}

struct ThreadPoolImpl::Impl {

  Impl();
  ~Impl();

  void JoinThreads(bool wait_for_jobs_to_complete);

  void SetBackgroundThreadsInternal(int num, bool allow_reduce);
  int GetBackgroundThreads();

  unsigned int GetQueueLen() const {
    return queue_len_.load(std::memory_order_relaxed);
  }

  void LowerIOPriority();

  void WakeUpAllThreads() {
    bgsignal_.notify_all();
  }

  void BGThread(size_t thread_id);

  void StartBGThreads();

  void Submit(std::function<void()>&& schedule,
    std::function<void()>&& unschedule, void* tag);

  int UnSchedule(void* arg);

  void SetHostEnv(Env* env) { env_ = env; }

  Env* GetHostEnv() const { return env_; }

  bool HasExcessiveThread() const {
    return static_cast<int>(bgthreads_.size()) > total_threads_limit_;
  }

//如果当前线程是要终止的多余线程，则返回true。
//始终终止最后添加的运行线程，即使存在
//要终止的线程不止一个。
  bool IsLastExcessiveThread(size_t thread_id) const {
    return HasExcessiveThread() && thread_id == bgthreads_.size() - 1;
  }

  bool IsExcessiveThread(size_t thread_id) const {
    return static_cast<int>(thread_id) >= total_threads_limit_;
  }

//返回线程优先级。
//这将允许其成员线程知道其优先级。
  Env::Priority GetThreadPriority() const { return priority_; }

//设置线程优先级。
  void SetThreadPriority(Env::Priority priority) { priority_ = priority; }

private:

  static void* BGThreadWrapper(void* arg);

  bool low_io_priority_;
  Env::Priority priority_;
  Env*         env_;

  int total_threads_limit_;
std::atomic_uint queue_len_;  //队列长度。用于统计报告
  bool exit_all_threads_;
  bool wait_for_jobs_to_complete_;

//每个schedule（）/submit（）调用的条目
  struct BGItem {
    void* tag = nullptr;
    std::function<void()> function;
    std::function<void()> unschedFunction;
  };

  using BGQueue = std::deque<BGItem>;
  BGQueue       queue_;

  std::mutex               mu_;
  std::condition_variable  bgsignal_;
  std::vector<port::Thread> bgthreads_;
};


inline
ThreadPoolImpl::Impl::Impl()
    :
      low_io_priority_(false),
      priority_(Env::LOW),
      env_(nullptr),
      total_threads_limit_(0),
      queue_len_(),
      exit_all_threads_(false),
      wait_for_jobs_to_complete_(false),
      queue_(),
      mu_(),
      bgsignal_(),
      bgthreads_() {
}

inline
ThreadPoolImpl::Impl::~Impl() { assert(bgthreads_.size() == 0U); }

void ThreadPoolImpl::Impl::JoinThreads(bool wait_for_jobs_to_complete) {

  std::unique_lock<std::mutex> lock(mu_);
  assert(!exit_all_threads_);

  wait_for_jobs_to_complete_ = wait_for_jobs_to_complete;
  exit_all_threads_ = true;

  lock.unlock();

  bgsignal_.notify_all();

  for (auto& th : bgthreads_) {
    th.join();
  }

  bgthreads_.clear();

  exit_all_threads_ = false;
  wait_for_jobs_to_complete_ = false;
}

inline
void ThreadPoolImpl::Impl::LowerIOPriority() {
  std::lock_guard<std::mutex> lock(mu_);
  low_io_priority_ = true;
}


void ThreadPoolImpl::Impl::BGThread(size_t thread_id) {
  bool low_io_priority = false;
  while (true) {
//等待直到有一个项目可以运行
    std::unique_lock<std::mutex> lock(mu_);
//如果线程需要工作或需要终止，请停止等待。
    while (!exit_all_threads_ && !IsLastExcessiveThread(thread_id) &&
           (queue_.empty() || IsExcessiveThread(thread_id))) {
      bgsignal_.wait(lock);
    }

if (exit_all_threads_) {  //允许BG线程安全退出的机制

      if(!wait_for_jobs_to_complete_ ||
          queue_.empty()) {
        break;
       }
    }

    if (IsLastExcessiveThread(thread_id)) {
//当前线程是最后一个生成的线程，它太多了。
//我们总是以相反的顺序终止多余的线程
//生成时间。
      auto& terminating_thread = bgthreads_.back();
      terminating_thread.detach();
      bgthreads_.pop_back();

      if (HasExcessiveThread()) {
//至少还有更多的线程需要终止。
        WakeUpAllThreads();
      }
      break;
    }

    auto func = std::move(queue_.front().function);
    queue_.pop_front();

    queue_len_.store(static_cast<unsigned int>(queue_.size()),
                     std::memory_order_relaxed);

    bool decrease_io_priority = (low_io_priority != low_io_priority_);
    lock.unlock();

#ifdef OS_LINUX
    if (decrease_io_priority) {
#define IOPRIO_CLASS_SHIFT (13)
#define IOPRIO_PRIO_VALUE(class, data) (((class) << IOPRIO_CLASS_SHIFT) | data)
//将计划放入ioprio_class_idle class（最低）
//这些系统调用仅在与一起使用时有效
//使用支持I/O优先级的I/O调度程序。如AT
//内核2.6.17唯一这样的调度程序是
//公平排队（CFQ）I/O调度程序。
//要更改计划程序：
//echo cfq>/sys/block/<device\name>/queue/schedule
//要考虑的可调参数：
///sys/block/<device_name>/queue/slice_idle
///sys/block/<device\u name>/queue/slice\u sync
syscall(SYS_ioprio_set, 1,  //ioprio_谁处理
0,                  //当前线程
              IOPRIO_PRIO_VALUE(3, 0));
      low_io_priority = true;
    }
#else
(void)decrease_io_priority;  //避免“未使用的变量”错误
#endif
    func();
  }
}

//用于在创建线程时传递参数的helper结构。
struct BGThreadMetadata {
  ThreadPoolImpl::Impl* thread_pool_;
size_t thread_id_;  //线程中的线程数。
  BGThreadMetadata(ThreadPoolImpl::Impl* thread_pool, size_t thread_id)
      : thread_pool_(thread_pool), thread_id_(thread_id) {}
};

void* ThreadPoolImpl::Impl::BGThreadWrapper(void* arg) {
  BGThreadMetadata* meta = reinterpret_cast<BGThreadMetadata*>(arg);
  size_t thread_id = meta->thread_id_;
  ThreadPoolImpl::Impl* tp = meta->thread_pool_;
#ifdef ROCKSDB_USING_THREAD_STATUS
//线程状态
  ThreadStatusUtil::RegisterThread(
      tp->GetHostEnv(), (tp->GetThreadPriority() == Env::Priority::HIGH
                             ? ThreadStatus::HIGH_PRIORITY
                             : ThreadStatus::LOW_PRIORITY));
#endif
  delete meta;
  tp->BGThread(thread_id);
#ifdef ROCKSDB_USING_THREAD_STATUS
  ThreadStatusUtil::UnregisterThread();
#endif
  return nullptr;
}

void ThreadPoolImpl::Impl::SetBackgroundThreadsInternal(int num,
  bool allow_reduce) {
  std::unique_lock<std::mutex> lock(mu_);
  if (exit_all_threads_) {
    lock.unlock();
    return;
  }
  if (num > total_threads_limit_ ||
      (num < total_threads_limit_ && allow_reduce)) {
    total_threads_limit_ = std::max(0, num);
    WakeUpAllThreads();
    StartBGThreads();
  }
}

int ThreadPoolImpl::Impl::GetBackgroundThreads() {
  std::unique_lock<std::mutex> lock(mu_);
  return total_threads_limit_;
}

void ThreadPoolImpl::Impl::StartBGThreads() {
//必要时启动后台线程
  while ((int)bgthreads_.size() < total_threads_limit_) {

    port::Thread p_t(&BGThreadWrapper,
      new BGThreadMetadata(this, bgthreads_.size()));

//设置线程名称以帮助调试
#if defined(_GNU_SOURCE) && defined(__GLIBC_PREREQ)
#if __GLIBC_PREREQ(2, 12)
    auto th_handle = p_t.native_handle();
    char name_buf[16];
    snprintf(name_buf, sizeof name_buf, "rocksdb:bg%" ROCKSDB_PRIszt,
             bgthreads_.size());
    name_buf[sizeof name_buf - 1] = '\0';
    pthread_setname_np(th_handle, name_buf);
#endif
#endif
    bgthreads_.push_back(std::move(p_t));
  }
}

void ThreadPoolImpl::Impl::Submit(std::function<void()>&& schedule,
  std::function<void()>&& unschedule, void* tag) {

  std::lock_guard<std::mutex> lock(mu_);

  if (exit_all_threads_) {
    return;
  }

  StartBGThreads();

//添加到优先级队列
  queue_.push_back(BGItem());

  auto& item = queue_.back();
  item.tag = tag;
  item.function = std::move(schedule);
  item.unschedFunction = std::move(unschedule);

  queue_len_.store(static_cast<unsigned int>(queue_.size()),
    std::memory_order_relaxed);

  if (!HasExcessiveThread()) {
//至少唤醒一个等待线程。
    bgsignal_.notify_one();
  } else {
//需要唤醒所有线程以确保一个唤醒
//“向上”不是要终止的。
    WakeUpAllThreads();
  }
}

int ThreadPoolImpl::Impl::UnSchedule(void* arg) {
  int count = 0;

  std::vector<std::function<void()>> candidates;
  {
    std::lock_guard<std::mutex> lock(mu_);

//从优先级队列中删除
    BGQueue::iterator it = queue_.begin();
    while (it != queue_.end()) {
      if (arg == (*it).tag) {
        if (it->unschedFunction) {
          candidates.push_back(std::move(it->unschedFunction));
        }
        it = queue_.erase(it);
        count++;
      } else {
        ++it;
      }
    }
    queue_len_.store(static_cast<unsigned int>(queue_.size()),
      std::memory_order_relaxed);
  }


//在互斥体之外运行非计划函数
  for (auto& f : candidates) {
    f();
  }

  return count;
}

ThreadPoolImpl::ThreadPoolImpl() :
  impl_(new Impl()) {
}


ThreadPoolImpl::~ThreadPoolImpl() {
}

void ThreadPoolImpl::JoinAllThreads() {
  impl_->JoinThreads(false);
}

void ThreadPoolImpl::SetBackgroundThreads(int num) {
  impl_->SetBackgroundThreadsInternal(num, true);
}

int ThreadPoolImpl::GetBackgroundThreads() {
  return impl_->GetBackgroundThreads();
}

unsigned int ThreadPoolImpl::GetQueueLen() const {
  return impl_->GetQueueLen();
}

void ThreadPoolImpl::WaitForJobsAndJoinAllThreads() {
  impl_->JoinThreads(true);
}

void ThreadPoolImpl::LowerIOPriority() {
  impl_->LowerIOPriority();
}

void ThreadPoolImpl::IncBackgroundThreadsIfNeeded(int num) {
  impl_->SetBackgroundThreadsInternal(num, false);
}

void ThreadPoolImpl::SubmitJob(const std::function<void()>& job) {
  auto copy(job);
  impl_->Submit(std::move(copy), std::function<void()>(), nullptr);
}


void ThreadPoolImpl::SubmitJob(std::function<void()>&& job) {
  impl_->Submit(std::move(job), std::function<void()>(), nullptr);
}

void ThreadPoolImpl::Schedule(void(*function)(void* arg1), void* arg,
  void* tag, void(*unschedFunction)(void* arg)) {

  std::function<void()> fn = [arg, function] { function(arg); };

  std::function<void()> unfn;
  if (unschedFunction != nullptr) {
    auto uf = [arg, unschedFunction] { unschedFunction(arg); };
    unfn = std::move(uf);
  }

  impl_->Submit(std::move(fn), std::move(unfn), tag);
}

int ThreadPoolImpl::UnSchedule(void* arg) {
  return impl_->UnSchedule(arg);
}

void ThreadPoolImpl::SetHostEnv(Env* env) { impl_->SetHostEnv(env); }

Env* ThreadPoolImpl::GetHostEnv() const { return impl_->GetHostEnv(); }

//返回线程优先级。
//这将允许其成员线程知道其优先级。
Env::Priority ThreadPoolImpl::GetThreadPriority() const {
  return impl_->GetThreadPriority();
}

//设置线程优先级。
void ThreadPoolImpl::SetThreadPriority(Env::Priority priority) {
  impl_->SetThreadPriority(priority);
}

ThreadPool* NewThreadPool(int num_threads) {
  ThreadPoolImpl* thread_pool = new ThreadPoolImpl();
  thread_pool->SetBackgroundThreads(num_threads);
  return thread_pool;
}

}  //命名空间rocksdb
