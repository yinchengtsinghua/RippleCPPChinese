
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2013，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。
//
#ifndef ROCKSDB_LITE

#ifndef GFLAGS
#include <cstdio>
int main() { fprintf(stderr, "Please install gflags to run tools\n"); }
#else
#include <gflags/gflags.h>
#include <atomic>
#include <functional>
#include <memory>
#include <sstream>
#include <unordered_map>

#include "rocksdb/env.h"

#include "utilities/persistent_cache/block_cache_tier.h"
#include "utilities/persistent_cache/persistent_cache_tier.h"
#include "utilities/persistent_cache/volatile_tier_impl.h"

#include "monitoring/histogram.h"
#include "port/port.h"
#include "table/block_builder.h"
#include "util/mutexlock.h"
#include "util/stop_watch.h"

DEFINE_int32(nsec, 10, "nsec");
DEFINE_int32(nthread_write, 1, "Insert threads");
DEFINE_int32(nthread_read, 1, "Lookup threads");
DEFINE_string(path, "/tmp/microbench/blkcache", "Path for cachefile");
DEFINE_string(log_path, "/tmp/log", "Path for the log file");
DEFINE_uint64(cache_size, std::numeric_limits<uint64_t>::max(), "Cache size");
DEFINE_int32(iosize, 4 * 1024, "Read IO size");
DEFINE_int32(writer_iosize, 4 * 1024, "File writer IO size");
DEFINE_int32(writer_qdepth, 1, "File writer qdepth");
DEFINE_bool(enable_pipelined_writes, false, "Enable async writes");
DEFINE_string(cache_type, "block_cache",
              "Cache type. (block_cache, volatile, tiered)");
DEFINE_bool(benchmark, false, "Benchmark mode");
DEFINE_int32(volatile_cache_pct, 10, "Percentage of cache in memory tier.");

namespace rocksdb {

std::unique_ptr<PersistentCacheTier> NewVolatileCache() {
  assert(FLAGS_cache_size != std::numeric_limits<uint64_t>::max());
  std::unique_ptr<PersistentCacheTier> pcache(
      new VolatileCacheTier(FLAGS_cache_size));
  return pcache;
}

std::unique_ptr<PersistentCacheTier> NewBlockCache() {
  std::shared_ptr<Logger> log;
  if (!Env::Default()->NewLogger(FLAGS_log_path, &log).ok()) {
    fprintf(stderr, "Error creating log %s \n", FLAGS_log_path.c_str());
    return nullptr;
  }

  PersistentCacheConfig opt(Env::Default(), FLAGS_path, FLAGS_cache_size, log);
  opt.writer_dispatch_size = FLAGS_writer_iosize;
  opt.writer_qdepth = FLAGS_writer_qdepth;
  opt.pipeline_writes = FLAGS_enable_pipelined_writes;
  opt.max_write_pipeline_backlog_size = std::numeric_limits<uint64_t>::max();
  std::unique_ptr<PersistentCacheTier> cache(new BlockCacheTier(opt));
  Status status = cache->Open();
  return cache;
}

//创建新的缓存层
//构建分层RAM+块缓存
std::unique_ptr<PersistentTieredCache> NewTieredCache(
    const size_t mem_size, const PersistentCacheConfig& opt) {
  std::unique_ptr<PersistentTieredCache> tcache(new PersistentTieredCache());
//创建主要层
  assert(mem_size);
  auto pcache =
      std::shared_ptr<PersistentCacheTier>(new VolatileCacheTier(mem_size));
  tcache->AddTier(pcache);
//创建辅助层
  auto scache = std::shared_ptr<PersistentCacheTier>(new BlockCacheTier(opt));
  tcache->AddTier(scache);

  Status s = tcache->Open();
  assert(s.ok());
  return tcache;
}

std::unique_ptr<PersistentTieredCache> NewTieredCache() {
  std::shared_ptr<Logger> log;
  if (!Env::Default()->NewLogger(FLAGS_log_path, &log).ok()) {
    fprintf(stderr, "Error creating log %s \n", FLAGS_log_path.c_str());
    abort();
  }

  auto pct = FLAGS_volatile_cache_pct / static_cast<double>(100);
  PersistentCacheConfig opt(Env::Default(), FLAGS_path,
                            (1 - pct) * FLAGS_cache_size, log);
  opt.writer_dispatch_size = FLAGS_writer_iosize;
  opt.writer_qdepth = FLAGS_writer_qdepth;
  opt.pipeline_writes = FLAGS_enable_pipelined_writes;
  opt.max_write_pipeline_backlog_size = std::numeric_limits<uint64_t>::max();
  return NewTieredCache(FLAGS_cache_size * pct, opt);
}

//
//基准驱动因素
//
class CacheTierBenchmark {
 public:
  explicit CacheTierBenchmark(std::shared_ptr<PersistentCacheTier>&& cache)
      : cache_(cache) {
    if (FLAGS_nthread_read) {
      fprintf(stdout, "Pre-populating\n");
      Prepop();
      fprintf(stdout, "Pre-population completed\n");
    }

    stats_.Clear();

//启动IO线程
    std::list<port::Thread> threads;
    Spawn(FLAGS_nthread_write, &threads,
          std::bind(&CacheTierBenchmark::Write, this));
    Spawn(FLAGS_nthread_read, &threads,
          std::bind(&CacheTierBenchmark::Read, this));

//等待直到标记“NSEC”，然后发出退出信号
    /*pWatchnano t（env:：default（），/*自动启动=*/true）；
    尺寸_t sec=t.elapsednanos（）/1000000000ull；
    而（！）{ }）
      sec=t.elapsednanos（）/1000000000ull；
      退出“秒>大小”（标记“nsec”）；
      /*睡眠覆盖*/ sleep(1);

    }

//等待线程退出
    Join(&threads);
//打印统计
    PrintStats(sec);
//关闭缓存
    cache_->TEST_Flush();
    cache_->Close();
  }

 private:
  void PrintStats(const size_t sec) {
    std::ostringstream msg;
    msg << "Test stats" << std::endl
        << "* Elapsed: " << sec << " s" << std::endl
        << "* Write Latency:" << std::endl
        << stats_.write_latency_.ToString() << std::endl
        << "* Read Latency:" << std::endl
        << stats_.read_latency_.ToString() << std::endl
        << "* Bytes written:" << std::endl
        << stats_.bytes_written_.ToString() << std::endl
        << "* Bytes read:" << std::endl
        << stats_.bytes_read_.ToString() << std::endl
        << "Cache stats:" << std::endl
        << cache_->PrintStats() << std::endl;
    fprintf(stderr, "%s\n", msg.str().c_str());
  }

//
//插入实现和相应的助手函数
//
  void Prepop() {
    for (uint64_t i = 0; i < 1024 * 1024; ++i) {
      InsertKey(i);
      insert_key_limit_++;
      read_key_limit_++;
    }

//等待数据刷新
    cache_->TEST_Flush();
//预热缓存
    for (uint64_t i = 0; i < 1024 * 1024; ReadKey(i++)) {
    }
  }

  void Write() {
    while (!quit_) {
      InsertKey(insert_key_limit_++);
    }
  }

  void InsertKey(const uint64_t key) {
//构造键
    uint64_t k[3];
    Slice block_key = FillKey(k, key);

//建构价值
    auto block = NewBlock(key);

//插入
    /*pWatchnano计时器（env:：default（），/*自动启动=*/true）；
    当（真）{
      status status=cache_->insert（block_key，block.get（），flags_iosize）；
      if（status.ok（））
        断裂；
      }

      //如果在没有管道的情况下运行，则可能出现瞬时错误
      断言（！）标记“启用”管道“写入”；
    }

    /调整统计数据
    const size_t elapsed_micro=timer.elapsednanos（）/1000；
    stats_u.write_latency_u.add（elapsed_micro）；
    stats_u.bytes_u written_u.add（flags_o iosize）；
  }

  / /
  //读取实现
  / /
  空读取（）
    而（！）{ }）
      read key（random（）%read_key_limit_u）；
    }
  }

  void readkey（const uint64_t val）
    //构造键
    UIT64 64 K K〔3〕；
    切片键=填充键（k，val）；

    //在缓存中查找
    StopWatchNano计时器（env:：default（），/*自动启动*/true);

    std::unique_ptr<char[]> block;
    size_t size;
    Status status = cache_->Lookup(key, &block, &size);
    if (!status.ok()) {
      fprintf(stderr, "%s\n", status.ToString().c_str());
    }
    assert(status.ok());
    assert(size == (size_t) FLAGS_iosize);

//调整统计
    const size_t elapsed_micro = timer.ElapsedNanos() / 1000;
    stats_.read_latency_.Add(elapsed_micro);
    stats_.bytes_read_.Add(FLAGS_iosize);

//验证内容
    if (!FLAGS_benchmark) {
      auto expected_block = NewBlock(val);
      assert(memcmp(block.get(), expected_block.get(), FLAGS_iosize) == 0);
    }
  }

//通过填充特定模式为键创建数据
  std::unique_ptr<char[]> NewBlock(const uint64_t val) {
    unique_ptr<char[]> data(new char[FLAGS_iosize]);
    memset(data.get(), val % 255, FLAGS_iosize);
    return data;
  }

//产卵螺纹
  void Spawn(const size_t n, std::list<port::Thread>* threads,
             const std::function<void()>& fn) {
    for (size_t i = 0; i < n; ++i) {
      threads->emplace_back(fn);
    }
  }

//连接线程
  void Join(std::list<port::Thread>* threads) {
    for (auto& th : *threads) {
      th.join();
    }
  }

//构造键
  Slice FillKey(uint64_t (&k)[3], const uint64_t val) {
    k[0] = k[1] = 0;
    k[2] = val;
    void* p = static_cast<void*>(&k);
    return Slice(static_cast<char*>(p), sizeof(k));
  }

//基准统计
  struct Stats {
    void Clear() {
      bytes_written_.Clear();
      bytes_read_.Clear();
      read_latency_.Clear();
      write_latency_.Clear();
    }

    HistogramImpl bytes_written_;
    HistogramImpl bytes_read_;
    HistogramImpl read_latency_;
    HistogramImpl write_latency_;
  };

std::shared_ptr<PersistentCacheTier> cache_;  //缓存实现
std::atomic<uint64_t> insert_key_limit_{0};   //数据插入到
std::atomic<uint64_t> read_key_limit_{0};     //数据可以安全读取到
bool quit_ = false;                           //退出线程？
mutable Stats stats_;                         //统计数据
};

}  //命名空间rocksdb

//
//主要的
//
int main(int argc, char** argv) {
  GFLAGS::SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
                          " [OPTIONS]...");
  GFLAGS::ParseCommandLineFlags(&argc, &argv, false);

  std::ostringstream msg;
  msg << "Config" << std::endl
      << "======" << std::endl
      << "* nsec=" << FLAGS_nsec << std::endl
      << "* nthread_write=" << FLAGS_nthread_write << std::endl
      << "* path=" << FLAGS_path << std::endl
      << "* cache_size=" << FLAGS_cache_size << std::endl
      << "* iosize=" << FLAGS_iosize << std::endl
      << "* writer_iosize=" << FLAGS_writer_iosize << std::endl
      << "* writer_qdepth=" << FLAGS_writer_qdepth << std::endl
      << "* enable_pipelined_writes=" << FLAGS_enable_pipelined_writes
      << std::endl
      << "* cache_type=" << FLAGS_cache_type << std::endl
      << "* benchmark=" << FLAGS_benchmark << std::endl
      << "* volatile_cache_pct=" << FLAGS_volatile_cache_pct << std::endl;

  fprintf(stderr, "%s\n", msg.str().c_str());

  std::shared_ptr<rocksdb::PersistentCacheTier> cache;
  if (FLAGS_cache_type == "block_cache") {
    fprintf(stderr, "Using block cache implementation\n");
    cache = rocksdb::NewBlockCache();
  } else if (FLAGS_cache_type == "volatile") {
    fprintf(stderr, "Using volatile cache implementation\n");
    cache = rocksdb::NewVolatileCache();
  } else if (FLAGS_cache_type == "tiered") {
    fprintf(stderr, "Using tiered cache implementation\n");
    cache = rocksdb::NewTieredCache();
  } else {
    fprintf(stderr, "Unknown option for cache\n");
  }

  assert(cache);
  if (!cache) {
    fprintf(stderr, "Error creating cache\n");
    abort();
  }

  std::unique_ptr<rocksdb::CacheTierBenchmark> benchmark(
      new rocksdb::CacheTierBenchmark(std::move(cache)));

  return 0;
}
#endif  //γIFNDEF GFLAGS
#else
int main(int, char**) { return 0; }
#endif
