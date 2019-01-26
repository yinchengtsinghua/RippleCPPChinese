
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

#ifndef ROCKSDB_LITE

#include "db/db_impl.h"
#include "rocksdb/cache.h"
#include "rocksdb/table.h"
#include "rocksdb/utilities/memory_util.h"
#include "rocksdb/utilities/stackable_db.h"
#include "table/block_based_table_factory.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

namespace rocksdb {

class MemoryTest : public testing::Test {
 public:
  MemoryTest() : kDbDir(test::TmpDir() + "/memory_test"), rnd_(301) {
    assert(Env::Default()->CreateDirIfMissing(kDbDir).ok());
  }

  std::string GetDBName(int id) { return kDbDir + "db_" + ToString(id); }

  std::string RandomString(int len) {
    std::string r;
    test::RandomString(&rnd_, len, &r);
    return r;
  }

  void UpdateUsagesHistory(const std::vector<DB*>& dbs) {
    std::map<MemoryUtil::UsageType, uint64_t> usage_by_type;
    ASSERT_OK(GetApproximateMemoryUsageByType(dbs, &usage_by_type));
    for (int i = 0; i < MemoryUtil::kNumUsageTypes; ++i) {
      usage_history_[i].push_back(
          usage_by_type[static_cast<MemoryUtil::UsageType>(i)]);
    }
  }

  void GetCachePointersFromTableFactory(
      const TableFactory* factory,
      std::unordered_set<const Cache*>* cache_set) {
    const BlockBasedTableFactory* bbtf =
        dynamic_cast<const BlockBasedTableFactory*>(factory);
    if (bbtf != nullptr) {
      const auto bbt_opts = bbtf->table_options();
      cache_set->insert(bbt_opts.block_cache.get());
      cache_set->insert(bbt_opts.block_cache_compressed.get());
    }
  }

  void GetCachePointers(const std::vector<DB*>& dbs,
                        std::unordered_set<const Cache*>* cache_set) {
    cache_set->clear();

    for (auto* db : dbs) {
//从dbimpl缓存
      StackableDB* sdb = dynamic_cast<StackableDB*>(db);
      DBImpl* db_impl = dynamic_cast<DBImpl*>(sdb ? sdb->GetBaseDB() : db);
      if (db_impl != nullptr) {
        cache_set->insert(db_impl->TEST_table_cache());
      }

//从dboptions缓存
      cache_set->insert(db->GetDBOptions().row_cache.get());

//从表工厂缓存
      std::unordered_map<std::string, const ImmutableCFOptions*> iopts_map;
      if (db_impl != nullptr) {
        ASSERT_OK(db_impl->TEST_GetAllImmutableCFOptions(&iopts_map));
      }
      for (auto pair : iopts_map) {
        GetCachePointersFromTableFactory(pair.second->table_factory, cache_set);
      }
    }
  }

  Status GetApproximateMemoryUsageByType(
      const std::vector<DB*>& dbs,
      std::map<MemoryUtil::UsageType, uint64_t>* usage_by_type) {
    std::unordered_set<const Cache*> cache_set;
    GetCachePointers(dbs, &cache_set);

    return MemoryUtil::GetApproximateMemoryUsageByType(dbs, cache_set,
                                                       usage_by_type);
  }

  const std::string kDbDir;
  Random rnd_;
  std::vector<uint64_t> usage_history_[MemoryUtil::kNumUsageTypes];
};

TEST_F(MemoryTest, SharedBlockCacheTotal) {
  std::vector<DB*> dbs;
  std::vector<uint64_t> usage_by_type;
  const int kNumDBs = 10;
  const int kKeySize = 100;
  const int kValueSize = 500;
  Options opt;
  opt.create_if_missing = true;
  opt.write_buffer_size = kKeySize + kValueSize;
  opt.max_write_buffer_number = 10;
  opt.min_write_buffer_number_to_merge = 10;
  opt.disable_auto_compactions = true;
  BlockBasedTableOptions bbt_opts;
  bbt_opts.block_cache = NewLRUCache(4096 * 1000 * 10);
  for (int i = 0; i < kNumDBs; ++i) {
    DestroyDB(GetDBName(i), opt);
    DB* db = nullptr;
    ASSERT_OK(DB::Open(opt, GetDBName(i), &db));
    dbs.push_back(db);
  }

  std::vector<std::string> keys_by_db[kNumDBs];

//每次放入一个memtable以使memtable使用更多内存。
  for (int p = 0; p < opt.min_write_buffer_number_to_merge / 2; ++p) {
    for (int i = 0; i < kNumDBs; ++i) {
      for (int j = 0; j < 100; ++j) {
        keys_by_db[i].emplace_back(RandomString(kKeySize));
        dbs[i]->Put(WriteOptions(), keys_by_db[i].back(),
                    RandomString(kValueSize));
      }
      dbs[i]->Flush(FlushOptions());
    }
  }
  for (int i = 0; i < kNumDBs; ++i) {
    for (auto& key : keys_by_db[i]) {
      std::string value;
      dbs[i]->Get(ReadOptions(), key, &value);
    }
    UpdateUsagesHistory(dbs);
  }
  for (size_t i = 1; i < usage_history_[MemoryUtil::kMemTableTotal].size();
       ++i) {
//因为我们没有刷新更多的内存表，所以需要eq。
    ASSERT_EQ(usage_history_[MemoryUtil::kTableReadersTotal][i],
              usage_history_[MemoryUtil::kTableReadersTotal][i - 1]);
  }
  for (int i = 0; i < kNumDBs; ++i) {
    delete dbs[i];
  }
}

TEST_F(MemoryTest, MemTableAndTableReadersTotal) {
  std::vector<DB*> dbs;
  std::vector<uint64_t> usage_by_type;
  std::vector<std::vector<ColumnFamilyHandle*>> vec_handles;
  const int kNumDBs = 10;
  const int kKeySize = 100;
  const int kValueSize = 500;
  Options opt;
  opt.create_if_missing = true;
  opt.create_missing_column_families = true;
  opt.write_buffer_size = kKeySize + kValueSize;
  opt.max_write_buffer_number = 10;
  opt.min_write_buffer_number_to_merge = 10;
  opt.disable_auto_compactions = true;

  std::vector<ColumnFamilyDescriptor> cf_descs = {
      {kDefaultColumnFamilyName, ColumnFamilyOptions(opt)},
      {"one", ColumnFamilyOptions(opt)},
      {"two", ColumnFamilyOptions(opt)},
  };

  for (int i = 0; i < kNumDBs; ++i) {
    DestroyDB(GetDBName(i), opt);
    std::vector<ColumnFamilyHandle*> handles;
    dbs.emplace_back();
    vec_handles.emplace_back();
    ASSERT_OK(DB::Open(DBOptions(opt), GetDBName(i), cf_descs,
                       &vec_handles.back(), &dbs.back()));
  }

//每次放入一个memtable以使memtable使用更多内存。
  for (int p = 0; p < opt.min_write_buffer_number_to_merge / 2; ++p) {
    for (int i = 0; i < kNumDBs; ++i) {
      for (auto* handle : vec_handles[i]) {
        dbs[i]->Put(WriteOptions(), handle, RandomString(kKeySize),
                    RandomString(kValueSize));
        UpdateUsagesHistory(dbs);
      }
    }
  }
//预期使用历史是单调增加的
  for (size_t i = 1; i < usage_history_[MemoryUtil::kMemTableTotal].size();
       ++i) {
    ASSERT_GT(usage_history_[MemoryUtil::kMemTableTotal][i],
              usage_history_[MemoryUtil::kMemTableTotal][i - 1]);
    ASSERT_GT(usage_history_[MemoryUtil::kMemTableUnFlushed][i],
              usage_history_[MemoryUtil::kMemTableUnFlushed][i - 1]);
    ASSERT_EQ(usage_history_[MemoryUtil::kTableReadersTotal][i],
              usage_history_[MemoryUtil::kTableReadersTotal][i - 1]);
  }

  size_t usage_check_point = usage_history_[MemoryUtil::kMemTableTotal].size();
  std::vector<Iterator*> iters;

//创建迭代器并刷新每个数据库的所有memtable
  for (int i = 0; i < kNumDBs; ++i) {
    iters.push_back(dbs[i]->NewIterator(ReadOptions()));
    dbs[i]->Flush(FlushOptions());

    for (int j = 0; j < 100; ++j) {
      std::string value;
      dbs[i]->Get(ReadOptions(), RandomString(kKeySize), &value);
    }

    UpdateUsagesHistory(dbs);
  }
  for (size_t i = usage_check_point;
       i < usage_history_[MemoryUtil::kMemTableTotal].size(); ++i) {
//因为memtables是由迭代器固定的，所以我们不期望
//所有memtable的内存使用率都会随着固定而降低。
//通过迭代器。
    ASSERT_GE(usage_history_[MemoryUtil::kMemTableTotal][i],
              usage_history_[MemoryUtil::kMemTableTotal][i - 1]);
//预计“使用衰减点”的使用历史是
//单调递减。
    ASSERT_LT(usage_history_[MemoryUtil::kMemTableUnFlushed][i],
              usage_history_[MemoryUtil::kMemTableUnFlushed][i - 1]);
//希望表读取器的使用历史记录增加
//当我们冲洗桌子时。
    ASSERT_GT(usage_history_[MemoryUtil::kTableReadersTotal][i],
              usage_history_[MemoryUtil::kTableReadersTotal][i - 1]);
    ASSERT_GT(usage_history_[MemoryUtil::kCacheTotal][i],
              usage_history_[MemoryUtil::kCacheTotal][i - 1]);
  }
  usage_check_point = usage_history_[MemoryUtil::kMemTableTotal].size();
  for (int i = 0; i < kNumDBs; ++i) {
    delete iters[i];
    UpdateUsagesHistory(dbs);
  }
  for (size_t i = usage_check_point;
       i < usage_history_[MemoryUtil::kMemTableTotal].size(); ++i) {
//当我们删除迭代器时，所有memtable的使用都会减少。
    ASSERT_LT(usage_history_[MemoryUtil::kMemTableTotal][i],
              usage_history_[MemoryUtil::kMemTableTotal][i - 1]);
//因为未刷新的memtables的内存使用只受影响
//通过put和flush，我们期望eq出现在这里，因为我们只删除迭代器。
    ASSERT_EQ(usage_history_[MemoryUtil::kMemTableUnFlushed][i],
              usage_history_[MemoryUtil::kMemTableUnFlushed][i - 1]);
//因为我们没有刷新更多的内存表，所以需要eq。
    ASSERT_EQ(usage_history_[MemoryUtil::kTableReadersTotal][i],
              usage_history_[MemoryUtil::kTableReadersTotal][i - 1]);
  }

  for (int i = 0; i < kNumDBs; ++i) {
    for (auto* handle : vec_handles[i]) {
      delete handle;
    }
    delete dbs[i];
  }
}
}  //命名空间rocksdb

int main(int argc, char** argv) {
#if !(defined NDEBUG) || !defined(OS_WIN)
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
#else
  return 0;
#endif
}

#else
#include <cstdio>

int main(int argc, char** argv) {
  printf("Skipped in RocksDBLite as utilities are not supported.\n");
  return 0;
}
#endif  //！摇滚乐
