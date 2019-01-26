
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
//
//这个工具的目标是做一个简单的压力测试，重点是捕捉：
//*压缩/冲洗过程中的错误，尤其是导致
//断言错误
//*代码中删除过时文件的错误
//
//测试分为两部分：
//*写入压力，写入数据库的二进制文件
//*write_stress_runner.py，一个调用并消除write_stress的脚本
//
//以下是一些有趣的写作压力部分：
//*以非常高的压缩和刷新并发性运行（32个线程）
//总计）并尝试创建大量小文件
//*写入数据库的密钥分布不均匀——有
//一个3个字符的前缀，偶尔发生变异（在前缀变异线程中），在
//使第一个字符的变异比第二个字符的变异慢
//比第三个字符慢。这样，压实应力测试
//有趣的压缩特性，比如琐碎的移动和最底层
//计算
//*有一个线程创建迭代器，将其保持几秒钟。
//然后遍历所有键。这是为了测试Rocksdb的能力
//在引用文件时保持文件的活动状态。
//*有些写操作会触发wal-sync。这是对我们的wal-sync代码的压力测试。
//*在运行结束时，我们确保没有泄漏任何SST。
//文件夹
//
//write-stress.py改变了我们运行write-stress的模式，并且
//杀死并重新启动。有一些有趣的特点：
//*在开始时，我们将整个测试运行时划分为较小的部分--
//更短的运行时间（几秒）和更长的运行时间（100、1000秒）
//*我们第一次运行write_stress时，会销毁旧数据库。每隔一次
//在测试期间，我们使用相同的数据库。
//*我们可以在kill模式或clean restart模式下运行。杀死模式杀死
//写得很重。
//*我们可以在删除“过时的”文件的模式下运行，其中“完全扫描”为真或
//假
//*我们可以在打开或关闭“低打开”文件模式的情况下运行。打开后，
//我们将表缓存配置为只保存几个文件——这样我们就需要
//每次访问文件时都重新打开。
//
//另一个目标是创建一个没有很多参数的压力测试。所以
//tools/write_stress_runner.py应该只接受一个参数--运行时秒
//它应该自己解决其他问题。

#include <cstdio>

#ifndef GFLAGS
int main() {
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
  return 1;
}
#else

#include <gflags/gflags.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <atomic>
#include <random>
#include <set>
#include <string>
#include <thread>

#include "port/port.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "util/filename.h"

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::RegisterFlagValidator;
using GFLAGS::SetUsageMessage;

DEFINE_int32(key_size, 10, "Key size");
DEFINE_int32(value_size, 100, "Value size");
DEFINE_string(db, "", "Use the db with the following name.");
DEFINE_bool(destroy_db, true,
            "Destroy the existing DB before running the test");

DEFINE_int32(runtime_sec, 10 * 60, "How long are we running for, in seconds");
DEFINE_int32(seed, 139, "Random seed");

DEFINE_double(prefix_mutate_period_sec, 1.0,
              "How often are we going to mutate the prefix");
DEFINE_double(first_char_mutate_probability, 0.1,
              "How likely are we to mutate the first char every period");
DEFINE_double(second_char_mutate_probability, 0.2,
              "How likely are we to mutate the second char every period");
DEFINE_double(third_char_mutate_probability, 0.5,
              "How likely are we to mutate the third char every period");

DEFINE_int32(iterator_hold_sec, 5,
             "How long will the iterator hold files before it gets destroyed");

DEFINE_double(sync_probability, 0.01, "How often are we syncing writes");
DEFINE_bool(delete_obsolete_files_with_fullscan, false,
            "If true, we delete obsolete files after each compaction/flush "
            "using GetChildren() API");
DEFINE_bool(low_open_files_mode, false,
            "If true, we set max_open_files to 20, so that every file access "
            "needs to reopen it");

namespace rocksdb {

static const int kPrefixSize = 3;

class WriteStress {
 public:
  WriteStress() : stop_(false) {
//初始化密钥前缀
    for (int i = 0; i < kPrefixSize; ++i) {
      key_prefix_[i].store('a');
    }

//选择测试数据库的位置，如果没有给定--db=<path>
    if (FLAGS_db.empty()) {
      std::string default_db_path;
      Env::Default()->GetTestDirectory(&default_db_path);
      default_db_path += "/write_stress";
      FLAGS_db = default_db_path;
    }

    Options options;
    if (FLAGS_destroy_db) {
DestroyDB(FLAGS_db, options);  //忽视
    }

//使LSM树变深，以便我们有许多并发刷新和
//压实
    options.create_if_missing = true;
options.write_buffer_size = 256 * 1024;              //256K
options.max_bytes_for_level_base = 1 * 1024 * 1024;  //1MB
options.target_file_size_base = 100 * 1024;          //100K
    options.max_write_buffer_number = 16;
    options.max_background_compactions = 16;
    options.max_background_flushes = 16;
    options.max_open_files = FLAGS_low_open_files_mode ? 20 : -1;
    if (FLAGS_delete_obsolete_files_with_fullscan) {
      options.delete_obsolete_files_period_micros = 0;
    }

//开放数据库
    DB* db;
    Status s = DB::Open(options, FLAGS_db, &db);
    if (!s.ok()) {
      fprintf(stderr, "Can't open database: %s\n", s.ToString().c_str());
      std::abort();
    }
    db_.reset(db);
  }

  void WriteThread() {
    std::mt19937 rng(static_cast<unsigned int>(FLAGS_seed));
    std::uniform_real_distribution<double> dist(0, 1);

    auto random_string = [](std::mt19937& r, int len) {
      std::uniform_int_distribution<int> char_dist('a', 'z');
      std::string ret;
      for (int i = 0; i < len; ++i) {
        ret += char_dist(r);
      }
      return ret;
    };

    while (!stop_.load(std::memory_order_relaxed)) {
      std::string prefix;
      prefix.resize(kPrefixSize);
      for (int i = 0; i < kPrefixSize; ++i) {
        prefix[i] = key_prefix_[i].load(std::memory_order_relaxed);
      }
      auto key = prefix + random_string(rng, FLAGS_key_size - kPrefixSize);
      auto value = random_string(rng, FLAGS_value_size);
      WriteOptions woptions;
      woptions.sync = dist(rng) < FLAGS_sync_probability;
      auto s = db_->Put(woptions, key, value);
      if (!s.ok()) {
        fprintf(stderr, "Write to DB failed: %s\n", s.ToString().c_str());
        std::abort();
      }
    }
  }

  void IteratorHoldThread() {
    while (!stop_.load(std::memory_order_relaxed)) {
      std::unique_ptr<Iterator> iterator(db_->NewIterator(ReadOptions()));
      Env::Default()->SleepForMicroseconds(FLAGS_iterator_hold_sec * 1000 *
                                           1000LL);
      for (iterator->SeekToFirst(); iterator->Valid(); iterator->Next()) {
      }
      if (!iterator->status().ok()) {
        fprintf(stderr, "Iterator statuts not OK: %s\n",
                iterator->status().ToString().c_str());
        std::abort();
      }
    }
  }

  void PrefixMutatorThread() {
    std::mt19937 rng(static_cast<unsigned int>(FLAGS_seed));
    std::uniform_real_distribution<double> dist(0, 1);
    std::uniform_int_distribution<int> char_dist('a', 'z');
    while (!stop_.load(std::memory_order_relaxed)) {
      Env::Default()->SleepForMicroseconds(static_cast<int>(
                                           FLAGS_prefix_mutate_period_sec *
                                           1000 * 1000LL));
      if (dist(rng) < FLAGS_first_char_mutate_probability) {
        key_prefix_[0].store(char_dist(rng), std::memory_order_relaxed);
      }
      if (dist(rng) < FLAGS_second_char_mutate_probability) {
        key_prefix_[1].store(char_dist(rng), std::memory_order_relaxed);
      }
      if (dist(rng) < FLAGS_third_char_mutate_probability) {
        key_prefix_[2].store(char_dist(rng), std::memory_order_relaxed);
      }
    }
  }

  int Run() {
    threads_.emplace_back([&]() { WriteThread(); });
    threads_.emplace_back([&]() { PrefixMutatorThread(); });
    threads_.emplace_back([&]() { IteratorHoldThread(); });

    if (FLAGS_runtime_sec == -1) {
//无限的运行时间，直到我们被杀
      while (true) {
        Env::Default()->SleepForMicroseconds(1000 * 1000);
      }
    }

    Env::Default()->SleepForMicroseconds(FLAGS_runtime_sec * 1000 * 1000);

    stop_.store(true, std::memory_order_relaxed);
    for (auto& t : threads_) {
      t.join();
    }
    threads_.clear();

//跳过检查RocksDB-Lite中的泄漏文件，因为我们无法访问
//函数getlivefilesmetadata
#ifndef ROCKSDB_LITE
//让我们看看是否泄露了一些文件
    db_->PauseBackgroundWork();
    std::vector<LiveFileMetaData> metadata;
    db_->GetLiveFilesMetaData(&metadata);
    std::set<uint64_t> sst_file_numbers;
    for (const auto& file : metadata) {
      uint64_t number;
      FileType type;
      if (!ParseFileName(file.name, &number, "LOG", &type)) {
        continue;
      }
      if (type == kTableFile) {
        sst_file_numbers.insert(number);
      }
    }

    std::vector<std::string> children;
    Env::Default()->GetChildren(FLAGS_db, &children);
    for (const auto& child : children) {
      uint64_t number;
      FileType type;
      if (!ParseFileName(child, &number, "LOG", &type)) {
        continue;
      }
      if (type == kTableFile) {
        if (sst_file_numbers.find(number) == sst_file_numbers.end()) {
          fprintf(stderr,
                  "Found a table file in DB path that should have been "
                  "deleted: %s\n",
                  child.c_str());
          std::abort();
        }
      }
    }
    db_->ContinueBackgroundWork();
#endif  //！摇滚乐

    return 0;
  }

 private:
//每个键前面都有这个前缀。我们偶尔会改变它。第三的
//字母的更改频率比秒高，后者的更改频率更高
//经常比第一个。
  std::atomic<char> key_prefix_[kPrefixSize];
  std::atomic<bool> stop_;
  std::vector<port::Thread> threads_;
  std::unique_ptr<DB> db_;
};

}  //命名空间rocksdb

int main(int argc, char** argv) {
  SetUsageMessage(std::string("\nUSAGE:\n") + std::string(argv[0]) +
                  " [OPTIONS]...");
  ParseCommandLineFlags(&argc, &argv, true);
  rocksdb::WriteStress write_stress;
  return write_stress.Run();
}

#endif  //GFLAGS
