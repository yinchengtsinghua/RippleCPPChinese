
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

#include "port/port_posix.h"

#include <assert.h>
#if defined(__i386__) || defined(__x86_64__)
#include <cpuid.h>
#endif
#include <errno.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <cstdlib>
#include "util/logging.h"

namespace rocksdb {
namespace port {

static int PthreadCall(const char* label, int result) {
  if (result != 0 && result != ETIMEDOUT) {
    fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
    abort();
  }
  return result;
}

Mutex::Mutex(bool adaptive) {
#ifdef ROCKSDB_PTHREAD_ADAPTIVE_MUTEX
  if (!adaptive) {
    PthreadCall("init mutex", pthread_mutex_init(&mu_, nullptr));
  } else {
    pthread_mutexattr_t mutex_attr;
    PthreadCall("init mutex attr", pthread_mutexattr_init(&mutex_attr));
    PthreadCall("set mutex attr",
                pthread_mutexattr_settype(&mutex_attr,
                                          PTHREAD_MUTEX_ADAPTIVE_NP));
    PthreadCall("init mutex", pthread_mutex_init(&mu_, &mutex_attr));
    PthreadCall("destroy mutex attr",
                pthread_mutexattr_destroy(&mutex_attr));
  }
#else
  PthreadCall("init mutex", pthread_mutex_init(&mu_, nullptr));
#endif //RocksDBpthread_自适应静音
}

Mutex::~Mutex() { PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_)); }

void Mutex::Lock() {
  PthreadCall("lock", pthread_mutex_lock(&mu_));
#ifndef NDEBUG
  locked_ = true;
#endif
}

void Mutex::Unlock() {
#ifndef NDEBUG
  locked_ = false;
#endif
  PthreadCall("unlock", pthread_mutex_unlock(&mu_));
}

void Mutex::AssertHeld() {
#ifndef NDEBUG
  assert(locked_);
#endif
}

CondVar::CondVar(Mutex* mu)
    : mu_(mu) {
    PthreadCall("init cv", pthread_cond_init(&cv_, nullptr));
}

CondVar::~CondVar() { PthreadCall("destroy cv", pthread_cond_destroy(&cv_)); }

void CondVar::Wait() {
#ifndef NDEBUG
  mu_->locked_ = false;
#endif
  PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
#ifndef NDEBUG
  mu_->locked_ = true;
#endif
}

bool CondVar::TimedWait(uint64_t abs_time_us) {
  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(abs_time_us / 1000000);
  ts.tv_nsec = static_cast<suseconds_t>((abs_time_us % 1000000) * 1000);

#ifndef NDEBUG
  mu_->locked_ = false;
#endif
  int err = pthread_cond_timedwait(&cv_, &mu_->mu_, &ts);
#ifndef NDEBUG
  mu_->locked_ = true;
#endif
  if (err == ETIMEDOUT) {
    return true;
  }
  if (err != 0) {
    PthreadCall("timedwait", err);
  }
  return false;
}

void CondVar::Signal() {
  PthreadCall("signal", pthread_cond_signal(&cv_));
}

void CondVar::SignalAll() {
  PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

RWMutex::RWMutex() {
  PthreadCall("init mutex", pthread_rwlock_init(&mu_, nullptr));
}

RWMutex::~RWMutex() { PthreadCall("destroy mutex", pthread_rwlock_destroy(&mu_)); }

void RWMutex::ReadLock() { PthreadCall("read lock", pthread_rwlock_rdlock(&mu_)); }

void RWMutex::WriteLock() { PthreadCall("write lock", pthread_rwlock_wrlock(&mu_)); }

void RWMutex::ReadUnlock() { PthreadCall("read unlock", pthread_rwlock_unlock(&mu_)); }

void RWMutex::WriteUnlock() { PthreadCall("write unlock", pthread_rwlock_unlock(&mu_)); }

int PhysicalCoreID() {
#if defined(ROCKSDB_SCHED_GETCPU_PRESENT) && defined(__x86_64__) && \
    (__GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 22))
//sched_getcpu从2.22开始使用vdso getcpu（）syscall。我相信Linux提供了vdso
//仅支持x86_64。如果可行，这是最快/首选的方法。
  int cpuno = sched_getcpu();
  if (cpuno < 0) {
    return -1;
  }
  return cpuno;
#elif defined(__x86_64__) || defined(__i386__)
//clang/gcc都提供cpuid.h，它为x86_64和i386定义了uu get_cpuid（）。
  unsigned eax, ebx = 0, ecx, edx;
  if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx)) {
    return -1;
  }
  return ebx >> 24;
#else
//放弃，呼叫者可以生成一个随机数或其他东西。
  return -1;
#endif
}

void InitOnce(OnceType* once, void (*initializer)()) {
  PthreadCall("once", pthread_once(once, initializer));
}

void Crash(const std::string& srcfile, int srcline) {
  fprintf(stdout, "Crashing at %s:%d\n", srcfile.c_str(), srcline);
  fflush(stdout);
  kill(getpid(), SIGTERM);
}

int GetMaxOpenFiles() {
#if defined(RLIMIT_NOFILE)
  struct rlimit no_files_limit;
  if (getrlimit(RLIMIT_NOFILE, &no_files_limit) != 0) {
    return -1;
  }
//防止溢出
  if (no_files_limit.rlim_cur >= std::numeric_limits<int>::max()) {
    return std::numeric_limits<int>::max();
  }
  return static_cast<int>(no_files_limit.rlim_cur);
#endif
  return -1;
}

void *cacheline_aligned_alloc(size_t size) {
#if __GNUC__ < 5 && defined(__SANITIZE_ADDRESS__)
  return malloc(size);
#elif defined(_ISOC11_SOURCE)
  return aligned_alloc(CACHE_LINE_SIZE, size);
#elif ( _POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600 || defined(__APPLE__))
  void *m;
  errno = posix_memalign(&m, CACHE_LINE_SIZE, size);
  return errno ? NULL : m;
#else
  return malloc(size);
#endif
}

void cacheline_aligned_free(void *memblock) {
  free(memblock);
}


}  //命名空间端口
}  //命名空间rocksdb
