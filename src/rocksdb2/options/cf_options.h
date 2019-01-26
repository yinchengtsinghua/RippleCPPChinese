
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

#pragma once

#include <string>
#include <vector>

#include "db/dbformat.h"
#include "options/db_options.h"
#include "rocksdb/options.h"
#include "util/compression.h"

namespace rocksdb {

//不可变的CFOptions是RockSDB Internal使用的数据结构。它包含一个
//在整个生命周期内不应更改的选项的子集
//分贝。此结构中定义的原始指针没有对数据的所有权
//他们指出。选项包含这些数据的共享指针。
struct ImmutableCFOptions {
  ImmutableCFOptions();
  explicit ImmutableCFOptions(const Options& options);

  ImmutableCFOptions(const ImmutableDBOptions& db_options,
                     const ColumnFamilyOptions& cf_options);

  CompactionStyle compaction_style;

  CompactionPri compaction_pri;

  CompactionOptionsUniversal compaction_options_universal;
  CompactionOptionsFIFO compaction_options_fifo;

  const SliceTransform* prefix_extractor;

  const Comparator* user_comparator;
  InternalKeyComparator internal_comparator;

  MergeOperator* merge_operator;

  const CompactionFilter* compaction_filter;

  CompactionFilterFactory* compaction_filter_factory;

  int min_write_buffer_number_to_merge;

  int max_write_buffer_number_to_maintain;

  bool inplace_update_support;

  UpdateStatus (*inplace_callback)(char* existing_value,
                                   uint32_t* existing_value_size,
                                   Slice delta_value,
                                   std::string* merged_value);

  Logger* info_log;

  Statistics* statistics;

  RateLimiter* rate_limiter;

  InfoLogLevel info_log_level;

  Env* env;

//允许OS到mmap文件读取sst表。默认：假
  bool allow_mmap_reads;

//允许操作系统写入mmap文件。默认：假
  bool allow_mmap_writes;

  std::vector<DbPath> db_paths;

  MemTableRepFactory* memtable_factory;

  TableFactory* table_factory;

  Options::TablePropertiesCollectorFactories
      table_properties_collector_factories;

  bool advise_random_on_open;

//PlainTableReader需要此选项。可能需要移动它
//像布卢姆一样的简单选择
  uint32_t bloom_locality;

  bool purge_redundant_kvs_while_flush;

  bool use_fsync;

  std::vector<CompressionType> compression_per_level;

  CompressionType bottommost_compression;

  CompressionOptions compression_opts;

  bool level_compaction_dynamic_level_bytes;

  Options::AccessHint access_hint_on_compaction_start;

  bool new_table_reader_for_compaction_inputs;

  size_t compaction_readahead_size;

  int num_levels;

  bool optimize_filters_for_hits;

  bool force_consistency_checks;

  bool allow_ingest_behind;

//将调用回调函数的事件侦听器的向量
//当特定的RocksDB事件发生时。
  std::vector<std::shared_ptr<EventListener>> listeners;

  std::shared_ptr<Cache> row_cache;

  uint32_t max_subcompactions;

  const SliceTransform* memtable_insert_with_hint_prefix_extractor;
};

struct MutableCFOptions {
  explicit MutableCFOptions(const ColumnFamilyOptions& options)
      : write_buffer_size(options.write_buffer_size),
        max_write_buffer_number(options.max_write_buffer_number),
        arena_block_size(options.arena_block_size),
        memtable_prefix_bloom_size_ratio(
            options.memtable_prefix_bloom_size_ratio),
        memtable_huge_page_size(options.memtable_huge_page_size),
        max_successive_merges(options.max_successive_merges),
        inplace_update_num_locks(options.inplace_update_num_locks),
        disable_auto_compactions(options.disable_auto_compactions),
        soft_pending_compaction_bytes_limit(
            options.soft_pending_compaction_bytes_limit),
        hard_pending_compaction_bytes_limit(
            options.hard_pending_compaction_bytes_limit),
        level0_file_num_compaction_trigger(
            options.level0_file_num_compaction_trigger),
        level0_slowdown_writes_trigger(options.level0_slowdown_writes_trigger),
        level0_stop_writes_trigger(options.level0_stop_writes_trigger),
        max_compaction_bytes(options.max_compaction_bytes),
        target_file_size_base(options.target_file_size_base),
        target_file_size_multiplier(options.target_file_size_multiplier),
        max_bytes_for_level_base(options.max_bytes_for_level_base),
        max_bytes_for_level_multiplier(options.max_bytes_for_level_multiplier),
        max_bytes_for_level_multiplier_additional(
            options.max_bytes_for_level_multiplier_additional),
        max_sequential_skip_in_iterations(
            options.max_sequential_skip_in_iterations),
        paranoid_file_checks(options.paranoid_file_checks),
        report_bg_io_stats(options.report_bg_io_stats),
        compression(options.compression) {
    RefreshDerivedOptions(options.num_levels, options.compaction_style);
  }

  MutableCFOptions()
      : write_buffer_size(0),
        max_write_buffer_number(0),
        arena_block_size(0),
        memtable_prefix_bloom_size_ratio(0),
        memtable_huge_page_size(0),
        max_successive_merges(0),
        inplace_update_num_locks(0),
        disable_auto_compactions(false),
        soft_pending_compaction_bytes_limit(0),
        hard_pending_compaction_bytes_limit(0),
        level0_file_num_compaction_trigger(0),
        level0_slowdown_writes_trigger(0),
        level0_stop_writes_trigger(0),
        max_compaction_bytes(0),
        target_file_size_base(0),
        target_file_size_multiplier(0),
        max_bytes_for_level_base(0),
        max_bytes_for_level_multiplier(0),
        max_sequential_skip_in_iterations(0),
        paranoid_file_checks(false),
        report_bg_io_stats(false),
        compression(Snappy_Supported() ? kSnappyCompression : kNoCompression) {}

//必须在对mutablecfOptions进行任何更改后调用
  void RefreshDerivedOptions(int num_levels, CompactionStyle compaction_style);

  void RefreshDerivedOptions(const ImmutableCFOptions& ioptions) {
    RefreshDerivedOptions(ioptions.num_levels, ioptions.compaction_style);
  }

//获取给定级别中的最大文件大小。
  uint64_t MaxFileSizeForLevel(int level) const;
  int MaxBytesMultiplerAdditional(int level) const {
    if (level >=
        static_cast<int>(max_bytes_for_level_multiplier_additional.size())) {
      return 1;
    }
    return max_bytes_for_level_multiplier_additional[level];
  }

  void Dump(Logger* log) const;

//与memtable相关的选项
  size_t write_buffer_size;
  int max_write_buffer_number;
  size_t arena_block_size;
  double memtable_prefix_bloom_size_ratio;
  size_t memtable_huge_page_size;
  size_t max_successive_merges;
  size_t inplace_update_num_locks;

//压缩相关选项
  bool disable_auto_compactions;
  uint64_t soft_pending_compaction_bytes_limit;
  uint64_t hard_pending_compaction_bytes_limit;
  int level0_file_num_compaction_trigger;
  int level0_slowdown_writes_trigger;
  int level0_stop_writes_trigger;
  uint64_t max_compaction_bytes;
  uint64_t target_file_size_base;
  int target_file_size_multiplier;
  uint64_t max_bytes_for_level_base;
  double max_bytes_for_level_multiplier;
  std::vector<int> max_bytes_for_level_multiplier_additional;

//MISC选项
  uint64_t max_sequential_skip_in_iterations;
  bool paranoid_file_checks;
  bool report_bg_io_stats;
  CompressionType compression;

//派生选项
//每级目标文件大小。
  std::vector<uint64_t> max_file_size;
};

uint64_t MultiplyCheckOverflow(uint64_t op1, double op2);

}  //命名空间rocksdb
