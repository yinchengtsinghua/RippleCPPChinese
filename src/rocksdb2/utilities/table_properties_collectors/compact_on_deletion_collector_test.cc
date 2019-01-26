
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

#include <stdio.h>

#ifndef ROCKSDB_LITE
#include <algorithm>
#include <cmath>
#include <vector>

#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/utilities/table_properties_collectors.h"
#include "util/random.h"
#include "utilities/table_properties_collectors/compact_on_deletion_collector.h"

int main(int argc, char** argv) {
  const int kWindowSizes[] =
      {1000, 10000, 10000, 127, 128, 129, 255, 256, 257, 2, 10000};
  const int kDeletionTriggers[] =
      {500, 9500, 4323, 47, 61, 128, 250, 250, 250, 2, 2};
  rocksdb::TablePropertiesCollectorFactory::Context context;
  context.column_family_id =
      rocksdb::TablePropertiesCollectorFactory::Context::kUnknownColumnFamily;

  std::vector<int> window_sizes;
  std::vector<int> deletion_triggers;
//确定性测试
  for (int test = 0; test < 9; ++test) {
    window_sizes.emplace_back(kWindowSizes[test]);
    deletion_triggers.emplace_back(kDeletionTriggers[test]);
  }

//随机化试验
  rocksdb::Random rnd(301);
  const int kMaxTestSize = 100000l;
  for (int random_test = 0; random_test < 100; random_test++) {
    int window_size = rnd.Uniform(kMaxTestSize) + 1;
    int deletion_trigger = rnd.Uniform(window_size);
    window_sizes.emplace_back(window_size);
    deletion_triggers.emplace_back(deletion_trigger);
  }

  assert(window_sizes.size() == deletion_triggers.size());

  for (size_t test = 0; test < window_sizes.size(); ++test) {
    const int kBucketSize = 128;
    const int kWindowSize = window_sizes[test];
    const int kPaddedWindowSize =
        kBucketSize * ((window_sizes[test] + kBucketSize - 1) / kBucketSize);
    const int kNumDeletionTrigger = deletion_triggers[test];
    const int kBias = (kNumDeletionTrigger + kBucketSize - 1) / kBucketSize;
//简单测试
    {
      std::unique_ptr<rocksdb::TablePropertiesCollector> collector;
      auto factory = rocksdb::NewCompactOnDeletionCollectorFactory(
          kWindowSize, kNumDeletionTrigger);
      collector.reset(factory->CreateTablePropertiesCollector(context));
      const int kSample = 10;
      for (int delete_rate = 0; delete_rate <= kSample; ++delete_rate) {
        int deletions = 0;
        for (int i = 0; i < kPaddedWindowSize; ++i) {
          if (i % kSample < delete_rate) {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryDelete, 0, 0);
            deletions++;
          } else {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryPut, 0, 0);
          }
        }
        if (collector->NeedCompact() !=
            (deletions >= kNumDeletionTrigger) &&
            std::abs(deletions - kNumDeletionTrigger) > kBias) {
          fprintf(stderr, "[Error] collector->NeedCompact() != (%d >= %d)"
                  " with kWindowSize = %d and kNumDeletionTrigger = %d\n",
                  deletions, kNumDeletionTrigger,
                  kWindowSize, kNumDeletionTrigger);
          assert(false);
        }
        collector->Finish(nullptr);
      }
    }

//文件中只有一个部分满足压缩触发器
    {
      std::unique_ptr<rocksdb::TablePropertiesCollector> collector;
      auto factory = rocksdb::NewCompactOnDeletionCollectorFactory(
          kWindowSize, kNumDeletionTrigger);
      collector.reset(factory->CreateTablePropertiesCollector(context));
      const int kSample = 10;
      for (int delete_rate = 0; delete_rate <= kSample; ++delete_rate) {
        int deletions = 0;
        for (int section = 0; section < 5; ++section) {
          int initial_entries = rnd.Uniform(kWindowSize) + kWindowSize;
          for (int i = 0; i < initial_entries; ++i) {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryPut, 0, 0);
          }
        }
        for (int i = 0; i < kPaddedWindowSize; ++i) {
          if (i % kSample < delete_rate) {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryDelete, 0, 0);
            deletions++;
          } else {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryPut, 0, 0);
          }
        }
        for (int section = 0; section < 5; ++section) {
          int ending_entries = rnd.Uniform(kWindowSize) + kWindowSize;
          for (int i = 0; i < ending_entries; ++i) {
            collector->AddUserKey("hello", "rocksdb",
                                  rocksdb::kEntryPut, 0, 0);
          }
        }
        if (collector->NeedCompact() != (deletions >= kNumDeletionTrigger) &&
            std::abs(deletions - kNumDeletionTrigger) > kBias) {
          fprintf(stderr, "[Error] collector->NeedCompact() %d != (%d >= %d)"
                  " with kWindowSize = %d, kNumDeletionTrigger = %d\n",
                  collector->NeedCompact(),
                  deletions, kNumDeletionTrigger, kWindowSize,
                  kNumDeletionTrigger);
          assert(false);
        }
        collector->Finish(nullptr);
      }
    }

//测试3：发布大量删除，但它们的密度不是
//足够高以触发压缩。
    {
      std::unique_ptr<rocksdb::TablePropertiesCollector> collector;
      auto factory = rocksdb::NewCompactOnDeletionCollectorFactory(
          kWindowSize, kNumDeletionTrigger);
      collector.reset(factory->CreateTablePropertiesCollector(context));
      assert(collector->NeedCompact() == false);
//插入“kNumDeletionTrigger*0.95”删除内容
//“Kwindowsize”并验证不需要压缩。
      const int kDeletionsPerSection = kNumDeletionTrigger * 95 / 100;
      if (kDeletionsPerSection >= 0) {
        for (int section = 0; section < 200; ++section) {
          for (int i = 0; i < kPaddedWindowSize; ++i) {
            if (i < kDeletionsPerSection) {
              collector->AddUserKey("hello", "rocksdb",
                                    rocksdb::kEntryDelete, 0, 0);
            } else {
              collector->AddUserKey("hello", "rocksdb",
                                    rocksdb::kEntryPut, 0, 0);
            }
          }
        }
        if (collector->NeedCompact() &&
            std::abs(kDeletionsPerSection - kNumDeletionTrigger) > kBias) {
          fprintf(stderr, "[Error] collector->NeedCompact() != false"
                  " with kWindowSize = %d and kNumDeletionTrigger = %d\n",
                  kWindowSize, kNumDeletionTrigger);
          assert(false);
        }
        collector->Finish(nullptr);
      }
    }
  }
  fprintf(stderr, "PASSED\n");
}
#else
int main(int argc, char** argv) {
  fprintf(stderr, "SKIPPED as RocksDBLite does not include utilities.\n");
  return 0;
}
#endif  //！摇滚乐
