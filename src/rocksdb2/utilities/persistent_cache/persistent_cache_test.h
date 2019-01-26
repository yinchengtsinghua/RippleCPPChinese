
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。
#pragma once

#ifndef ROCKSDB_LITE

#include <functional>
#include <limits>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "db/db_test_util.h"
#include "rocksdb/cache.h"
#include "table/block_builder.h"
#include "port/port.h"
#include "util/arena.h"
#include "util/testharness.h"
#include "utilities/persistent_cache/volatile_tier_impl.h"

namespace rocksdb {

//
//用于测试PersistentCacheTier的单元测试
//
class PersistentCacheTierTest : public testing::Test {
 public:
  PersistentCacheTierTest();
  virtual ~PersistentCacheTierTest() {
    if (cache_) {
      Status s = cache_->Close();
      assert(s.ok());
    }
  }

 protected:
//刷新缓存
  void Flush() {
    if (cache_) {
      cache_->TEST_Flush();
    }
  }

//创建线程工作负荷
  template <class T>
  std::list<port::Thread> SpawnThreads(const size_t n, const T& fn) {
    std::list<port::Thread> threads;
    for (size_t i = 0; i < n; i++) {
      port::Thread th(fn);
      threads.push_back(std::move(th));
    }
    return threads;
  }

//等待线程加入
  void Join(std::list<port::Thread>&& threads) {
    for (auto& th : threads) {
      th.join();
    }
    threads.clear();
  }

//在线程中运行插入工作负荷
  void Insert(const size_t nthreads, const size_t max_keys) {
    key_ = 0;
    max_keys_ = max_keys;
//产卵螺纹
    auto fn = std::bind(&PersistentCacheTierTest::InsertImpl, this);
    auto threads = SpawnThreads(nthreads, fn);
//用螺纹连接
    Join(std::move(threads));
//刷新缓存
    Flush();
  }

//在缓存上运行验证
  void Verify(const size_t nthreads = 1, const bool eviction_enabled = false) {
    stats_verify_hits_ = 0;
    stats_verify_missed_ = 0;
    key_ = 0;
//产卵螺纹
    auto fn =
        std::bind(&PersistentCacheTierTest::VerifyImpl, this, eviction_enabled);
    auto threads = SpawnThreads(nthreads, fn);
//用螺纹连接
    Join(std::move(threads));
  }

//将0填充到数字
  std::string PaddedNumber(const size_t data, const size_t pad_size) {
    assert(pad_size);
    char* ret = new char[pad_size];
    int pos = static_cast<int>(pad_size) - 1;
    size_t count = 0;
    size_t t = data;
//拷贝数
    while (t) {
      count++;
      ret[pos--] = '0' + t % 10;
      t = t / 10;
    }
//拷贝0
    while (pos >= 0) {
      ret[pos--] = '0';
    }
//后置条件
    assert(count <= pad_size);
    assert(pos == -1);
    std::string result(ret, pad_size);
    delete[] ret;
    return result;
  }

//插入工作负荷实现
  void InsertImpl() {
    const std::string prefix = "key_prefix_";

    while (true) {
      size_t i = key_++;
      if (i >= max_keys_) {
        break;
      }

      char data[4 * 1024];
      memset(data, '0' + (i % 10), sizeof(data));
      /*o k=前缀+paddedNumber（i，/*计数=*/8）；
      切片键（K）；
      当（真）{
        status status=cache_u->insert（key，data，sizeof（data））；
        if（status.ok（））
          断裂；
        }
        断言“真”（status.istrigaain（））；
        env:：default（）->sleepFormicroseconds（1*1000*1000）；
      }
    }
  }

  //验证实现
  void verifyimpl（const bool evaction_enabled=false）
    const std:：string prefix=“key_prefix_u”；
    当（真）{
      尺寸_t i=键
      如果（i>=max_keys_
        断裂；
      }

      char edata[4*1024]；
      memset（edata，'0'+（i%10），sizeof（edata））；
      auto k=前缀+paddedNumber（i，/*计数*/8);

      Slice key(k);
      unique_ptr<char[]> block;
      size_t block_size;

      if (eviction_enabled) {
        if (!cache_->Lookup(key, &block, &block_size).ok()) {
//假设密钥已被收回
          stats_verify_missed_++;
          continue;
        }
      }

      ASSERT_OK(cache_->Lookup(key, &block, &block_size));
      ASSERT_EQ(block_size, sizeof(edata));
      ASSERT_EQ(memcmp(edata, block.get(), sizeof(edata)), 0);
      stats_verify_hits_++;
    }
  }

//插入测试模板
  void RunInsertTest(const size_t nthreads, const size_t max_keys) {
    Insert(nthreads, max_keys);
    Verify(nthreads);
    ASSERT_EQ(stats_verify_hits_, max_keys);
    ASSERT_EQ(stats_verify_missed_, 0);

    cache_->Close();
    cache_.reset();
  }

//阴性插入测试模板
  void RunNegativeInsertTest(const size_t nthreads, const size_t max_keys) {
    Insert(nthreads, max_keys);
    /*ify（nthreads，/*退出已启用=*/true）；
    断言“lt”（统计值“验证”击数“max”键）；
    断言gt（统计verify Missed uu0）；

    缓存_->close（）；
    CaseE.ReSET（）；
  }

  //用于插入和逐出测试的模板
  void runinserttestwithevaction（const size_t nthreads，const size_t max_keys）
    插入（n个读数，最大值键）；
    验证（n次读取，/*已启用逐出*/true);

    ASSERT_EQ(stats_verify_hits_ + stats_verify_missed_, max_keys);
    ASSERT_GT(stats_verify_hits_, 0);
    ASSERT_GT(stats_verify_missed_, 0);

    cache_->Close();
    cache_.reset();
  }

  const std::string path_;
  shared_ptr<Logger> log_;
  std::shared_ptr<PersistentCacheTier> cache_;
  std::atomic<size_t> key_{0};
  size_t max_keys_ = 0;
  std::atomic<size_t> stats_verify_hits_{0};
  std::atomic<size_t> stats_verify_missed_{0};
};

//
//ROCKSDB试验
//
class PersistentCacheDBTest : public DBTestBase {
 public:
  PersistentCacheDBTest();

  static uint64_t TestGetTickerCount(const Options& options,
                                     Tickers ticker_type) {
    return static_cast<uint32_t>(
        options.statistics->getTickerCount(ticker_type));
  }

//将数据插入表
  void Insert(const Options& options,
              const BlockBasedTableOptions& table_options, const int num_iter,
              std::vector<std::string>* values) {
    CreateAndReopenWithCF({"pikachu"}, options);
//默认列族没有块缓存
    Options no_block_cache_opts;
    no_block_cache_opts.statistics = options.statistics;
    no_block_cache_opts = CurrentOptions(no_block_cache_opts);
    BlockBasedTableOptions table_options_no_bc;
    table_options_no_bc.no_block_cache = true;
    no_block_cache_opts.table_factory.reset(
        NewBlockBasedTableFactory(table_options_no_bc));
    ReopenWithColumnFamilies(
        {"default", "pikachu"},
        std::vector<Options>({no_block_cache_opts, options}));

    Random rnd(301);

//写入8MB（80个值，每个100K）
    ASSERT_EQ(NumTableFilesAtLevel(0, 1), 0);
    std::string str;
    for (int i = 0; i < num_iter; i++) {
if (i % 4 == 0) {  //高压缩比
        str = RandomString(&rnd, 1000);
      }
      values->push_back(str);
      ASSERT_OK(Put(1, Key(i), (*values)[i]));
    }

//从memtable中刷新所有数据，以便读取来自块缓存
    ASSERT_OK(Flush(1));
  }

//验证数据
  void Verify(const int num_iter, const std::vector<std::string>& values) {
    for (int j = 0; j < 2; ++j) {
      for (int i = 0; i < num_iter; i++) {
        ASSERT_EQ(Get(1, Key(i)), values[i]);
      }
    }
  }

//测试模板
  void RunTest(const std::function<std::shared_ptr<PersistentCacheTier>(bool)>&
                   new_pcache,
               const size_t max_keys, const size_t max_usecase);
};

}  //命名空间rocksdb

#endif
