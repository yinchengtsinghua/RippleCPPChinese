
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#ifndef GFLAGS
#include <cstdio>
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <inttypes.h>
#include <sys/types.h>
#include <stdio.h>
#include <gflags/gflags.h>

#include "rocksdb/db.h"
#include "rocksdb/cache.h"
#include "rocksdb/env.h"
#include "port/port.h"
#include "util/mutexlock.h"
#include "util/random.h"

using GFLAGS::ParseCommandLineFlags;

static const uint32_t KB = 1024;

DEFINE_int32(threads, 16, "Number of concurrent threads to run.");
DEFINE_int64(cache_size, 8 * KB * KB,
             "Number of bytes to use as a cache of uncompressed data.");
DEFINE_int32(num_shard_bits, 4, "shard_bits.");

DEFINE_int64(max_key, 1 * KB * KB * KB, "Max number of key to place in cache");
DEFINE_uint64(ops_per_thread, 1200000, "Number of operations per thread.");

DEFINE_bool(populate_cache, false, "Populate cache before operations");
DEFINE_int32(insert_percent, 40,
             "Ratio of insert to total workload (expressed as a percentage)");
DEFINE_int32(lookup_percent, 50,
             "Ratio of lookup to total workload (expressed as a percentage)");
DEFINE_int32(erase_percent, 10,
             "Ratio of erase to total workload (expressed as a percentage)");

DEFINE_bool(use_clock_cache, false, "");

namespace rocksdb {

class CacheBench;
namespace {
void deleter(const Slice& key, void* value) {
    delete reinterpret_cast<char *>(value);
}

//由同一基准的所有并发执行共享的状态。
class SharedState {
 public:
  explicit SharedState(CacheBench* cache_bench)
      : cv_(&mu_),
        num_threads_(FLAGS_threads),
        num_initialized_(0),
        start_(false),
        num_done_(0),
        cache_bench_(cache_bench) {
  }

  ~SharedState() {}

  port::Mutex* GetMutex() {
    return &mu_;
  }

  port::CondVar* GetCondVar() {
    return &cv_;
  }

  CacheBench* GetCacheBench() const {
    return cache_bench_;
  }

  void IncInitialized() {
    num_initialized_++;
  }

  void IncDone() {
    num_done_++;
  }

  bool AllInitialized() const {
    return num_initialized_ >= num_threads_;
  }

  bool AllDone() const {
    return num_done_ >= num_threads_;
  }

  void SetStart() {
    start_ = true;
  }

  bool Started() const {
    return start_;
  }

 private:
  port::Mutex mu_;
  port::CondVar cv_;

  const uint64_t num_threads_;
  uint64_t num_initialized_;
  bool start_;
  uint64_t num_done_;

  CacheBench* cache_bench_;
};

//同一基准的并发执行的每个线程状态。
struct ThreadState {
  uint32_t tid;
  Random rnd;
  SharedState* shared;

  ThreadState(uint32_t index, SharedState* _shared)
      : tid(index), rnd(1000 + index), shared(_shared) {}
};
}  //命名空间

class CacheBench {
 public:
  CacheBench() : num_threads_(FLAGS_threads) {
    if (FLAGS_use_clock_cache) {
      cache_ = NewClockCache(FLAGS_cache_size, FLAGS_num_shard_bits);
      if (!cache_) {
        fprintf(stderr, "Clock cache not supported.\n");
        exit(1);
      }
    } else {
      cache_ = NewLRUCache(FLAGS_cache_size, FLAGS_num_shard_bits);
    }
  }

  ~CacheBench() {}

  void PopulateCache() {
    Random rnd(1);
    for (int64_t i = 0; i < FLAGS_cache_size; i++) {
      uint64_t rand_key = rnd.Next() % FLAGS_max_key;
//将uint64*强制转换为char*，数据将被复制到缓存
      Slice key(reinterpret_cast<char*>(&rand_key), 8);
//做插入
      cache_->Insert(key, new char[10], 1, &deleter);
    }
  }

  bool Run() {
    rocksdb::Env* env = rocksdb::Env::Default();

    PrintEnv();
    SharedState shared(this);
    std::vector<ThreadState*> threads(num_threads_);
    for (uint32_t i = 0; i < num_threads_; i++) {
      threads[i] = new ThreadState(i, &shared);
      env->StartThread(ThreadBody, threads[i]);
    }
    {
      MutexLock l(shared.GetMutex());
      while (!shared.AllInitialized()) {
        shared.GetCondVar()->Wait();
      }
//记录开始时间
      uint64_t start_time = env->NowMicros();

//启动所有线程
      shared.SetStart();
      shared.GetCondVar()->SignalAll();

//等待线程完成
      while (!shared.AllDone()) {
        shared.GetCondVar()->Wait();
      }

//记录结束时间
      uint64_t end_time = env->NowMicros();
      double elapsed = static_cast<double>(end_time - start_time) * 1e-6;
      uint32_t qps = static_cast<uint32_t>(
          static_cast<double>(FLAGS_threads * FLAGS_ops_per_thread) / elapsed);
      fprintf(stdout, "Complete in %.3f s; QPS = %u\n", elapsed, qps);
    }
    return true;
  }

 private:
  std::shared_ptr<Cache> cache_;
  uint32_t num_threads_;

  static void ThreadBody(void* v) {
    ThreadState* thread = reinterpret_cast<ThreadState*>(v);
    SharedState* shared = thread->shared;

    {
      MutexLock l(shared->GetMutex());
      shared->IncInitialized();
      if (shared->AllInitialized()) {
        shared->GetCondVar()->SignalAll();
      }
      while (!shared->Started()) {
        shared->GetCondVar()->Wait();
      }
    }
    thread->shared->GetCacheBench()->OperateCache(thread);

    {
      MutexLock l(shared->GetMutex());
      shared->IncDone();
      if (shared->AllDone()) {
        shared->GetCondVar()->SignalAll();
      }
    }
  }

  void OperateCache(ThreadState* thread) {
    for (uint64_t i = 0; i < FLAGS_ops_per_thread; i++) {
      uint64_t rand_key = thread->rnd.Next() % FLAGS_max_key;
//将uint64*强制转换为char*，数据将被复制到缓存
      Slice key(reinterpret_cast<char*>(&rand_key), 8);
      int32_t prob_op = thread->rnd.Uniform(100);
      if (prob_op >= 0 && prob_op < FLAGS_insert_percent) {
//做插入
        cache_->Insert(key, new char[10], 1, &deleter);
      } else if (prob_op -= FLAGS_insert_percent &&
                 prob_op < FLAGS_lookup_percent) {
//查找查找
        auto handle = cache_->Lookup(key);
        if (handle) {
          cache_->Release(handle);
        }
      } else if (prob_op -= FLAGS_lookup_percent &&
                 prob_op < FLAGS_erase_percent) {
//擦除
        cache_->Erase(key);
      }
    }
  }

  void PrintEnv() const {
    printf("RocksDB version     : %d.%d\n", kMajorVersion, kMinorVersion);
    printf("Number of threads   : %d\n", FLAGS_threads);
    printf("Ops per thread      : %" PRIu64 "\n", FLAGS_ops_per_thread);
    printf("Cache size          : %" PRIu64 "\n", FLAGS_cache_size);
    printf("Num shard bits      : %d\n", FLAGS_num_shard_bits);
    printf("Max key             : %" PRIu64 "\n", FLAGS_max_key);
    printf("Populate cache      : %d\n", FLAGS_populate_cache);
    printf("Insert percentage   : %d%%\n", FLAGS_insert_percent);
    printf("Lookup percentage   : %d%%\n", FLAGS_lookup_percent);
    printf("Erase percentage    : %d%%\n", FLAGS_erase_percent);
    printf("----------------------------\n");
  }
};
}  //命名空间rocksdb

int main(int argc, char** argv) {
  ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_threads <= 0) {
    fprintf(stderr, "threads number <= 0\n");
    exit(1);
  }

  rocksdb::CacheBench bench;
  if (FLAGS_populate_cache) {
    bench.PopulateCache();
  }
  if (bench.Run()) {
    return 0;
  } else {
    return 1;
  }
}

#endif  //GFLAGS
