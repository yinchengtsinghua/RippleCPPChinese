
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

#include "rocksdb/utilities/sim_cache.h"
#include <cstdlib>
#include "db/db_test_util.h"
#include "port/stack_trace.h"

namespace rocksdb {

class SimCacheTest : public DBTestBase {
 private:
  size_t miss_count_ = 0;
  size_t hit_count_ = 0;
  size_t insert_count_ = 0;
  size_t failure_count_ = 0;

 public:
  const size_t kNumBlocks = 5;
  const size_t kValueSize = 1000;

  SimCacheTest() : DBTestBase("/sim_cache_test") {}

  BlockBasedTableOptions GetTableOptions() {
    BlockBasedTableOptions table_options;
//设置足够小的块大小，以便每个键值都有自己的块。
    table_options.block_size = 1;
    return table_options;
  }

  Options GetOptions(const BlockBasedTableOptions& table_options) {
    Options options = CurrentOptions();
    options.create_if_missing = true;
//options.compression=knocompression；
    options.statistics = rocksdb::CreateDBStatistics();
    options.table_factory.reset(new BlockBasedTableFactory(table_options));
    return options;
  }

  void InitTable(const Options& options) {
    std::string value(kValueSize, 'a');
    for (size_t i = 0; i < kNumBlocks * 2; i++) {
      ASSERT_OK(Put(ToString(i), value.c_str()));
    }
  }

  void RecordCacheCounters(const Options& options) {
    miss_count_ = TestGetTickerCount(options, BLOCK_CACHE_MISS);
    hit_count_ = TestGetTickerCount(options, BLOCK_CACHE_HIT);
    insert_count_ = TestGetTickerCount(options, BLOCK_CACHE_ADD);
    failure_count_ = TestGetTickerCount(options, BLOCK_CACHE_ADD_FAILURES);
  }

  void CheckCacheCounters(const Options& options, size_t expected_misses,
                          size_t expected_hits, size_t expected_inserts,
                          size_t expected_failures) {
    size_t new_miss_count = TestGetTickerCount(options, BLOCK_CACHE_MISS);
    size_t new_hit_count = TestGetTickerCount(options, BLOCK_CACHE_HIT);
    size_t new_insert_count = TestGetTickerCount(options, BLOCK_CACHE_ADD);
    size_t new_failure_count =
        TestGetTickerCount(options, BLOCK_CACHE_ADD_FAILURES);
    ASSERT_EQ(miss_count_ + expected_misses, new_miss_count);
    ASSERT_EQ(hit_count_ + expected_hits, new_hit_count);
    ASSERT_EQ(insert_count_ + expected_inserts, new_insert_count);
    ASSERT_EQ(failure_count_ + expected_failures, new_failure_count);
    miss_count_ = new_miss_count;
    hit_count_ = new_hit_count;
    insert_count_ = new_insert_count;
    failure_count_ = new_failure_count;
  }
};

TEST_F(SimCacheTest, SimCache) {
  ReadOptions read_options;
  auto table_options = GetTableOptions();
  auto options = GetOptions(table_options);
  InitTable(options);
  std::shared_ptr<SimCache> simCache =
      NewSimCache(NewLRUCache(0, 0, false), 20000, 0);
  table_options.block_cache = simCache;
  options.table_factory.reset(new BlockBasedTableFactory(table_options));
  Reopen(options);
  RecordCacheCounters(options);

  std::vector<std::unique_ptr<Iterator>> iterators(kNumBlocks);
  Iterator* iter = nullptr;

//将块加载到缓存中。
  for (size_t i = 0; i < kNumBlocks; i++) {
    iter = db_->NewIterator(read_options);
    iter->Seek(ToString(i));
    ASSERT_OK(iter->status());
    CheckCacheCounters(options, 1, 0, 1, 0);
    iterators[i].reset(iter);
  }
  ASSERT_EQ(kNumBlocks,
            simCache->get_hit_counter() + simCache->get_miss_counter());
  ASSERT_EQ(0, simCache->get_hit_counter());
  size_t usage = simCache->GetUsage();
  ASSERT_LT(0, usage);
  ASSERT_EQ(usage, simCache->GetSimUsage());
  simCache->SetCapacity(usage);
  ASSERT_EQ(usage, simCache->GetPinnedUsage());

//以严格的容量限制进行测试。
  simCache->SetStrictCapacityLimit(true);
  iter = db_->NewIterator(read_options);
  iter->Seek(ToString(kNumBlocks * 2 - 1));
  ASSERT_TRUE(iter->status().IsIncomplete());
  CheckCacheCounters(options, 1, 0, 0, 1);
  delete iter;
  iter = nullptr;

//再次释放迭代器并访问缓存。
  for (size_t i = 0; i < kNumBlocks; i++) {
    iterators[i].reset();
    CheckCacheCounters(options, 0, 0, 0, 0);
  }
//再次添加滚花锁
  for (size_t i = 0; i < kNumBlocks; i++) {
    std::unique_ptr<Iterator> it(db_->NewIterator(read_options));
    it->Seek(ToString(i));
    ASSERT_OK(it->status());
    CheckCacheCounters(options, 0, 1, 0, 0);
  }
  ASSERT_EQ(5, simCache->get_hit_counter());
  for (size_t i = kNumBlocks; i < kNumBlocks * 2; i++) {
    std::unique_ptr<Iterator> it(db_->NewIterator(read_options));
    it->Seek(ToString(i));
    ASSERT_OK(it->status());
    CheckCacheCounters(options, 1, 0, 1, 0);
  }
  ASSERT_EQ(0, simCache->GetPinnedUsage());
  ASSERT_EQ(3 * kNumBlocks + 1,
            simCache->get_hit_counter() + simCache->get_miss_counter());
  ASSERT_EQ(6, simCache->get_hit_counter());
}

TEST_F(SimCacheTest, SimCacheLogging) {
  auto table_options = GetTableOptions();
  auto options = GetOptions(table_options);
  options.disable_auto_compactions = true;
  std::shared_ptr<SimCache> sim_cache =
      NewSimCache(NewLRUCache(1024 * 1024), 20000, 0);
  table_options.block_cache = sim_cache;
  options.table_factory.reset(new BlockBasedTableFactory(table_options));
  Reopen(options);

  int num_block_entries = 20;
  for (int i = 0; i < num_block_entries; i++) {
    Put(Key(i), "val");
    Flush();
  }

  std::string log_file = test::TmpDir(env_) + "/cache_log.txt";
  ASSERT_OK(sim_cache->StartActivityLogging(log_file, env_));
  for (int i = 0; i < num_block_entries; i++) {
    ASSERT_EQ(Get(Key(i)), "val");
  }
  for (int i = 0; i < num_block_entries; i++) {
    ASSERT_EQ(Get(Key(i)), "val");
  }
  sim_cache->StopActivityLogging();
  ASSERT_OK(sim_cache->GetActivityLoggingStatus());

  std::string file_contents = "";
  ReadFileToString(env_, log_file, &file_contents);

  int lookup_num = 0;
  int add_num = 0;
  std::string::size_type pos;

//计数查找数
  pos = 0;
  while ((pos = file_contents.find("LOOKUP -", pos)) != std::string::npos) {
    ++lookup_num;
    pos += 1;
  }

//计数添加数
  pos = 0;
  while ((pos = file_contents.find("ADD -", pos)) != std::string::npos) {
    ++add_num;
    pos += 1;
  }

//我们要求每个街区两次
  ASSERT_EQ(lookup_num, num_block_entries * 2);

//我们只添加了一次每个块，因为缓存可以容纳所有块
  ASSERT_EQ(add_num, num_block_entries);

//再次记录，但达到512字节后自动停止记录
	int max_size = 512;
  ASSERT_OK(sim_cache->StartActivityLogging(log_file, env_, max_size));
  for (int it = 0; it < 10; it++) {
    for (int i = 0; i < num_block_entries; i++) {
      ASSERT_EQ(Get(Key(i)), "val");
    }
  }
  ASSERT_OK(sim_cache->GetActivityLoggingStatus());

  uint64_t fsize = 0;
  ASSERT_OK(env_->GetFileSize(log_file, &fsize));
//100字节的误差范围
  ASSERT_LT(fsize, max_size + 100);
	ASSERT_GT(fsize, max_size - 100);
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  rocksdb::port::InstallStackTraceHandler();
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
