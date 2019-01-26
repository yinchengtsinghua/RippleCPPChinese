
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

#include "rocksdb/options.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <limits>

#include "monitoring/statistics.h"
#include "options/db_options.h"
#include "options/options_helper.h"
#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/sst_file_manager.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/wal_filter.h"
#include "table/block_based_table_factory.h"
#include "util/compression.h"

namespace rocksdb {

AdvancedColumnFamilyOptions::AdvancedColumnFamilyOptions() {
  assert(memtable_factory.get() != nullptr);
}

AdvancedColumnFamilyOptions::AdvancedColumnFamilyOptions(const Options& options)
    : max_write_buffer_number(options.max_write_buffer_number),
      min_write_buffer_number_to_merge(
          options.min_write_buffer_number_to_merge),
      max_write_buffer_number_to_maintain(
          options.max_write_buffer_number_to_maintain),
      inplace_update_support(options.inplace_update_support),
      inplace_update_num_locks(options.inplace_update_num_locks),
      inplace_callback(options.inplace_callback),
      memtable_prefix_bloom_size_ratio(
          options.memtable_prefix_bloom_size_ratio),
      memtable_huge_page_size(options.memtable_huge_page_size),
      memtable_insert_with_hint_prefix_extractor(
          options.memtable_insert_with_hint_prefix_extractor),
      bloom_locality(options.bloom_locality),
      arena_block_size(options.arena_block_size),
      compression_per_level(options.compression_per_level),
      num_levels(options.num_levels),
      level0_slowdown_writes_trigger(options.level0_slowdown_writes_trigger),
      level0_stop_writes_trigger(options.level0_stop_writes_trigger),
      target_file_size_base(options.target_file_size_base),
      target_file_size_multiplier(options.target_file_size_multiplier),
      level_compaction_dynamic_level_bytes(
          options.level_compaction_dynamic_level_bytes),
      max_bytes_for_level_multiplier(options.max_bytes_for_level_multiplier),
      max_bytes_for_level_multiplier_additional(
          options.max_bytes_for_level_multiplier_additional),
      max_compaction_bytes(options.max_compaction_bytes),
      soft_pending_compaction_bytes_limit(
          options.soft_pending_compaction_bytes_limit),
      hard_pending_compaction_bytes_limit(
          options.hard_pending_compaction_bytes_limit),
      compaction_style(options.compaction_style),
      compaction_pri(options.compaction_pri),
      compaction_options_universal(options.compaction_options_universal),
      compaction_options_fifo(options.compaction_options_fifo),
      max_sequential_skip_in_iterations(
          options.max_sequential_skip_in_iterations),
      memtable_factory(options.memtable_factory),
      table_properties_collector_factories(
          options.table_properties_collector_factories),
      max_successive_merges(options.max_successive_merges),
      optimize_filters_for_hits(options.optimize_filters_for_hits),
      paranoid_file_checks(options.paranoid_file_checks),
      force_consistency_checks(options.force_consistency_checks),
      report_bg_io_stats(options.report_bg_io_stats) {
  assert(memtable_factory.get() != nullptr);
  if (max_bytes_for_level_multiplier_additional.size() <
      static_cast<unsigned int>(num_levels)) {
    max_bytes_for_level_multiplier_additional.resize(num_levels, 1);
  }
}

ColumnFamilyOptions::ColumnFamilyOptions()
    : compression(Snappy_Supported() ? kSnappyCompression : kNoCompression),
      table_factory(
          std::shared_ptr<TableFactory>(new BlockBasedTableFactory())) {}

ColumnFamilyOptions::ColumnFamilyOptions(const Options& options)
    : AdvancedColumnFamilyOptions(options),
      comparator(options.comparator),
      merge_operator(options.merge_operator),
      compaction_filter(options.compaction_filter),
      compaction_filter_factory(options.compaction_filter_factory),
      write_buffer_size(options.write_buffer_size),
      compression(options.compression),
      bottommost_compression(options.bottommost_compression),
      compression_opts(options.compression_opts),
      level0_file_num_compaction_trigger(
          options.level0_file_num_compaction_trigger),
      prefix_extractor(options.prefix_extractor),
      max_bytes_for_level_base(options.max_bytes_for_level_base),
      disable_auto_compactions(options.disable_auto_compactions),
      table_factory(options.table_factory) {}

DBOptions::DBOptions() {}

DBOptions::DBOptions(const Options& options)
    : create_if_missing(options.create_if_missing),
      create_missing_column_families(options.create_missing_column_families),
      error_if_exists(options.error_if_exists),
      paranoid_checks(options.paranoid_checks),
      env(options.env),
      rate_limiter(options.rate_limiter),
      sst_file_manager(options.sst_file_manager),
      info_log(options.info_log),
      info_log_level(options.info_log_level),
      max_open_files(options.max_open_files),
      max_file_opening_threads(options.max_file_opening_threads),
      max_total_wal_size(options.max_total_wal_size),
      statistics(options.statistics),
      use_fsync(options.use_fsync),
      db_paths(options.db_paths),
      db_log_dir(options.db_log_dir),
      wal_dir(options.wal_dir),
      delete_obsolete_files_period_micros(
          options.delete_obsolete_files_period_micros),
      max_background_jobs(options.max_background_jobs),
      base_background_compactions(options.base_background_compactions),
      max_background_compactions(options.max_background_compactions),
      max_subcompactions(options.max_subcompactions),
      max_background_flushes(options.max_background_flushes),
      max_log_file_size(options.max_log_file_size),
      log_file_time_to_roll(options.log_file_time_to_roll),
      keep_log_file_num(options.keep_log_file_num),
      recycle_log_file_num(options.recycle_log_file_num),
      max_manifest_file_size(options.max_manifest_file_size),
      table_cache_numshardbits(options.table_cache_numshardbits),
      WAL_ttl_seconds(options.WAL_ttl_seconds),
      WAL_size_limit_MB(options.WAL_size_limit_MB),
      manifest_preallocation_size(options.manifest_preallocation_size),
      allow_mmap_reads(options.allow_mmap_reads),
      allow_mmap_writes(options.allow_mmap_writes),
      use_direct_reads(options.use_direct_reads),
      use_direct_io_for_flush_and_compaction(
          options.use_direct_io_for_flush_and_compaction),
      allow_fallocate(options.allow_fallocate),
      is_fd_close_on_exec(options.is_fd_close_on_exec),
      skip_log_error_on_recovery(options.skip_log_error_on_recovery),
      stats_dump_period_sec(options.stats_dump_period_sec),
      advise_random_on_open(options.advise_random_on_open),
      db_write_buffer_size(options.db_write_buffer_size),
      write_buffer_manager(options.write_buffer_manager),
      access_hint_on_compaction_start(options.access_hint_on_compaction_start),
      new_table_reader_for_compaction_inputs(
          options.new_table_reader_for_compaction_inputs),
      compaction_readahead_size(options.compaction_readahead_size),
      random_access_max_buffer_size(options.random_access_max_buffer_size),
      writable_file_max_buffer_size(options.writable_file_max_buffer_size),
      use_adaptive_mutex(options.use_adaptive_mutex),
      bytes_per_sync(options.bytes_per_sync),
      wal_bytes_per_sync(options.wal_bytes_per_sync),
      listeners(options.listeners),
      enable_thread_tracking(options.enable_thread_tracking),
      delayed_write_rate(options.delayed_write_rate),
      enable_pipelined_write(options.enable_pipelined_write),
      allow_concurrent_memtable_write(options.allow_concurrent_memtable_write),
      enable_write_thread_adaptive_yield(
          options.enable_write_thread_adaptive_yield),
      write_thread_max_yield_usec(options.write_thread_max_yield_usec),
      write_thread_slow_yield_usec(options.write_thread_slow_yield_usec),
      skip_stats_update_on_db_open(options.skip_stats_update_on_db_open),
      wal_recovery_mode(options.wal_recovery_mode),
      row_cache(options.row_cache),
#ifndef ROCKSDB_LITE
      wal_filter(options.wal_filter),
#endif  //摇滚乐
      fail_if_options_file_error(options.fail_if_options_file_error),
      dump_malloc_stats(options.dump_malloc_stats),
      avoid_flush_during_recovery(options.avoid_flush_during_recovery),
      avoid_flush_during_shutdown(options.avoid_flush_during_shutdown),
      allow_ingest_behind(options.allow_ingest_behind) {
}

void DBOptions::Dump(Logger* log) const {
    ImmutableDBOptions(*this).Dump(log);
    MutableDBOptions(*this).Dump(log);
}  //DBOPTIOS:：转储

void ColumnFamilyOptions::Dump(Logger* log) const {
  ROCKS_LOG_HEADER(log, "              Options.comparator: %s",
                   comparator->Name());
  ROCKS_LOG_HEADER(log, "          Options.merge_operator: %s",
                   merge_operator ? merge_operator->Name() : "None");
  ROCKS_LOG_HEADER(log, "       Options.compaction_filter: %s",
                   compaction_filter ? compaction_filter->Name() : "None");
  ROCKS_LOG_HEADER(
      log, "       Options.compaction_filter_factory: %s",
      compaction_filter_factory ? compaction_filter_factory->Name() : "None");
  ROCKS_LOG_HEADER(log, "        Options.memtable_factory: %s",
                   memtable_factory->Name());
  ROCKS_LOG_HEADER(log, "           Options.table_factory: %s",
                   table_factory->Name());
  ROCKS_LOG_HEADER(log, "           table_factory options: %s",
                   table_factory->GetPrintableTableOptions().c_str());
  ROCKS_LOG_HEADER(log, "       Options.write_buffer_size: %" ROCKSDB_PRIszt,
                   write_buffer_size);
  ROCKS_LOG_HEADER(log, " Options.max_write_buffer_number: %d",
                   max_write_buffer_number);
  if (!compression_per_level.empty()) {
    for (unsigned int i = 0; i < compression_per_level.size(); i++) {
      ROCKS_LOG_HEADER(
          log, "       Options.compression[%d]: %s", i,
          CompressionTypeToString(compression_per_level[i]).c_str());
    }
    } else {
      ROCKS_LOG_HEADER(log, "         Options.compression: %s",
                       CompressionTypeToString(compression).c_str());
    }
    ROCKS_LOG_HEADER(
        log, "                 Options.bottommost_compression: %s",
        bottommost_compression == kDisableCompressionOption
            ? "Disabled"
            : CompressionTypeToString(bottommost_compression).c_str());
    ROCKS_LOG_HEADER(
        log, "      Options.prefix_extractor: %s",
        prefix_extractor == nullptr ? "nullptr" : prefix_extractor->Name());
    ROCKS_LOG_HEADER(log,
                     "  Options.memtable_insert_with_hint_prefix_extractor: %s",
                     memtable_insert_with_hint_prefix_extractor == nullptr
                         ? "nullptr"
                         : memtable_insert_with_hint_prefix_extractor->Name());
    ROCKS_LOG_HEADER(log, "            Options.num_levels: %d", num_levels);
    ROCKS_LOG_HEADER(log, "       Options.min_write_buffer_number_to_merge: %d",
                     min_write_buffer_number_to_merge);
    ROCKS_LOG_HEADER(log, "    Options.max_write_buffer_number_to_maintain: %d",
                     max_write_buffer_number_to_maintain);
    ROCKS_LOG_HEADER(log, "           Options.compression_opts.window_bits: %d",
                     compression_opts.window_bits);
    ROCKS_LOG_HEADER(log, "                 Options.compression_opts.level: %d",
                     compression_opts.level);
    ROCKS_LOG_HEADER(log, "              Options.compression_opts.strategy: %d",
                     compression_opts.strategy);
    ROCKS_LOG_HEADER(
        log,
        "        Options.compression_opts.max_dict_bytes: %" ROCKSDB_PRIszt,
        compression_opts.max_dict_bytes);
    ROCKS_LOG_HEADER(log, "     Options.level0_file_num_compaction_trigger: %d",
                     level0_file_num_compaction_trigger);
    ROCKS_LOG_HEADER(log, "         Options.level0_slowdown_writes_trigger: %d",
                     level0_slowdown_writes_trigger);
    ROCKS_LOG_HEADER(log, "             Options.level0_stop_writes_trigger: %d",
                     level0_stop_writes_trigger);
    ROCKS_LOG_HEADER(
        log, "                  Options.target_file_size_base: %" PRIu64,
        target_file_size_base);
    ROCKS_LOG_HEADER(log, "            Options.target_file_size_multiplier: %d",
                     target_file_size_multiplier);
    ROCKS_LOG_HEADER(
        log, "               Options.max_bytes_for_level_base: %" PRIu64,
        max_bytes_for_level_base);
    ROCKS_LOG_HEADER(log, "Options.level_compaction_dynamic_level_bytes: %d",
                     level_compaction_dynamic_level_bytes);
    ROCKS_LOG_HEADER(log, "         Options.max_bytes_for_level_multiplier: %f",
                     max_bytes_for_level_multiplier);
    for (size_t i = 0; i < max_bytes_for_level_multiplier_additional.size();
         i++) {
      ROCKS_LOG_HEADER(
          log, "Options.max_bytes_for_level_multiplier_addtl[%" ROCKSDB_PRIszt
               "]: %d",
          i, max_bytes_for_level_multiplier_additional[i]);
    }
    ROCKS_LOG_HEADER(
        log, "      Options.max_sequential_skip_in_iterations: %" PRIu64,
        max_sequential_skip_in_iterations);
    ROCKS_LOG_HEADER(
        log, "                   Options.max_compaction_bytes: %" PRIu64,
        max_compaction_bytes);
    ROCKS_LOG_HEADER(
        log,
        "                       Options.arena_block_size: %" ROCKSDB_PRIszt,
        arena_block_size);
    ROCKS_LOG_HEADER(log,
                     "  Options.soft_pending_compaction_bytes_limit: %" PRIu64,
                     soft_pending_compaction_bytes_limit);
    ROCKS_LOG_HEADER(log,
                     "  Options.hard_pending_compaction_bytes_limit: %" PRIu64,
                     hard_pending_compaction_bytes_limit);
    ROCKS_LOG_HEADER(log, "      Options.rate_limit_delay_max_milliseconds: %u",
                     rate_limit_delay_max_milliseconds);
    ROCKS_LOG_HEADER(log, "               Options.disable_auto_compactions: %d",
                     disable_auto_compactions);

    const auto& it_compaction_style =
        compaction_style_to_string.find(compaction_style);
    std::string str_compaction_style;
    if (it_compaction_style == compaction_style_to_string.end()) {
      assert(false);
      str_compaction_style = "unknown_" + std::to_string(compaction_style);
    } else {
      str_compaction_style = it_compaction_style->second;
    }
    ROCKS_LOG_HEADER(log,
                     "                       Options.compaction_style: %s",
                     str_compaction_style.c_str());

    const auto& it_compaction_pri =
        compaction_pri_to_string.find(compaction_pri);
    std::string str_compaction_pri;
    if (it_compaction_pri == compaction_pri_to_string.end()) {
      assert(false);
      str_compaction_pri = "unknown_" + std::to_string(compaction_pri);
    } else {
      str_compaction_pri = it_compaction_pri->second;
    }
    ROCKS_LOG_HEADER(log,
                     "                         Options.compaction_pri: %s",
                     str_compaction_pri.c_str());
    ROCKS_LOG_HEADER(log,
                     "Options.compaction_options_universal.size_ratio: %u",
                     compaction_options_universal.size_ratio);
    ROCKS_LOG_HEADER(log,
                     "Options.compaction_options_universal.min_merge_width: %u",
                     compaction_options_universal.min_merge_width);
    ROCKS_LOG_HEADER(log,
                     "Options.compaction_options_universal.max_merge_width: %u",
                     compaction_options_universal.max_merge_width);
    ROCKS_LOG_HEADER(
        log,
        "Options.compaction_options_universal."
        "max_size_amplification_percent: %u",
        compaction_options_universal.max_size_amplification_percent);
    ROCKS_LOG_HEADER(
        log,
        "Options.compaction_options_universal.compression_size_percent: %d",
        compaction_options_universal.compression_size_percent);
    const auto& it_compaction_stop_style = compaction_stop_style_to_string.find(
        compaction_options_universal.stop_style);
    std::string str_compaction_stop_style;
    if (it_compaction_stop_style == compaction_stop_style_to_string.end()) {
      assert(false);
      str_compaction_stop_style =
          "unknown_" + std::to_string(compaction_options_universal.stop_style);
    } else {
      str_compaction_stop_style = it_compaction_stop_style->second;
    }
    ROCKS_LOG_HEADER(log,
                     "Options.compaction_options_universal.stop_style: %s",
                     str_compaction_stop_style.c_str());
    ROCKS_LOG_HEADER(
        log, "Options.compaction_options_fifo.max_table_files_size: %" PRIu64,
        compaction_options_fifo.max_table_files_size);
    ROCKS_LOG_HEADER(log,
                     "Options.compaction_options_fifo.allow_compaction: %d",
                     compaction_options_fifo.allow_compaction);
    ROCKS_LOG_HEADER(log, "Options.compaction_options_fifo.ttl: %" PRIu64,
                     compaction_options_fifo.ttl);
    std::string collector_names;
    for (const auto& collector_factory : table_properties_collector_factories) {
      collector_names.append(collector_factory->Name());
      collector_names.append("; ");
    }
    ROCKS_LOG_HEADER(
        log, "                  Options.table_properties_collectors: %s",
        collector_names.c_str());
    ROCKS_LOG_HEADER(log,
                     "                  Options.inplace_update_support: %d",
                     inplace_update_support);
    ROCKS_LOG_HEADER(
        log,
        "                Options.inplace_update_num_locks: %" ROCKSDB_PRIszt,
        inplace_update_num_locks);
//TODO:Bloom的简单配置（可能基于平均键/值大小）
    ROCKS_LOG_HEADER(
        log, "              Options.memtable_prefix_bloom_size_ratio: %f",
        memtable_prefix_bloom_size_ratio);

    ROCKS_LOG_HEADER(log, "  Options.memtable_huge_page_size: %" ROCKSDB_PRIszt,
                     memtable_huge_page_size);
    ROCKS_LOG_HEADER(log,
                     "                          Options.bloom_locality: %d",
                     bloom_locality);

    ROCKS_LOG_HEADER(
        log,
        "                   Options.max_successive_merges: %" ROCKSDB_PRIszt,
        max_successive_merges);
    ROCKS_LOG_HEADER(log,
                     "               Options.optimize_filters_for_hits: %d",
                     optimize_filters_for_hits);
    ROCKS_LOG_HEADER(log, "               Options.paranoid_file_checks: %d",
                     paranoid_file_checks);
    ROCKS_LOG_HEADER(log, "               Options.force_consistency_checks: %d",
                     force_consistency_checks);
    ROCKS_LOG_HEADER(log, "               Options.report_bg_io_stats: %d",
                     report_bg_io_stats);
}  //列系列选项：：转储

void Options::Dump(Logger* log) const {
  DBOptions::Dump(log);
  ColumnFamilyOptions::Dump(log);
}   //选项：：转储

void Options::DumpCFOptions(Logger* log) const {
  ColumnFamilyOptions::Dump(log);
}  //选项：：dumpcOptions

//
//此方法的目标是创建一个配置
//允许应用程序将所有文件写入l0和
//然后进行一次压缩，将所有文件输出到l1。
Options*
Options::PrepareForBulkLoad()
{
//绝不能放慢进食速度。
  level0_file_num_compaction_trigger = (1<<30);
  level0_slowdown_writes_trigger = (1<<30);
  level0_stop_writes_trigger = (1<<30);
  soft_pending_compaction_bytes_limit = 0;
  hard_pending_compaction_bytes_limit = 0;

//请不要自动压缩。应用程序应发出
//所有数据加载到l0后手动压缩。
  disable_auto_compactions = true;
//手动压缩运行应选择l0中的所有文件
//一次压实。
  max_compaction_bytes = (static_cast<uint64_t>(1) << 60);

//最好只有两个级别，否则一个手册
//压实会在每一个可能的水平上压实，因此
//增加压实所需的总时间。
  num_levels = 2;

//需要允许更多的写缓冲区以允许更多的Parallism
//潮红的
  max_write_buffer_number = 6;
  min_write_buffer_number_to_merge = 1;

//当禁用压缩时，更多的并行刷新线程可以
//帮助提高写吞吐量。
  max_background_flushes = 4;

//阻止memtable刷新以自动升级文件
//到L1。这有助于
//手动压实的输入均为l0。
  max_background_compactions = 2;

//压缩将在l1中创建大型文件。
  target_file_size_base = 256 * 1024 * 1024;
  return this;
}

Options* Options::OptimizeForSmallDb() {
  ColumnFamilyOptions::OptimizeForSmallDb();
  DBOptions::OptimizeForSmallDb();
  return this;
}

Options* Options::OldDefaults(int rocksdb_major_version,
                              int rocksdb_minor_version) {
  ColumnFamilyOptions::OldDefaults(rocksdb_major_version,
                                   rocksdb_minor_version);
  DBOptions::OldDefaults(rocksdb_major_version, rocksdb_minor_version);
  return this;
}

DBOptions* DBOptions::OldDefaults(int rocksdb_major_version,
                                  int rocksdb_minor_version) {
  if (rocksdb_major_version < 4 ||
      (rocksdb_major_version == 4 && rocksdb_minor_version < 7)) {
    max_file_opening_threads = 1;
    table_cache_numshardbits = 4;
  }
  if (rocksdb_major_version < 5 ||
      (rocksdb_major_version == 5 && rocksdb_minor_version < 2)) {
    delayed_write_rate = 2 * 1024U * 1024U;
  } else if (rocksdb_major_version < 5 ||
             (rocksdb_major_version == 5 && rocksdb_minor_version < 6)) {
    delayed_write_rate = 16 * 1024U * 1024U;
  }
  max_open_files = 5000;
  wal_recovery_mode = WALRecoveryMode::kTolerateCorruptedTailRecords;
  return this;
}

ColumnFamilyOptions* ColumnFamilyOptions::OldDefaults(
    int rocksdb_major_version, int rocksdb_minor_version) {
  if (rocksdb_major_version < 4 ||
      (rocksdb_major_version == 4 && rocksdb_minor_version < 7)) {
    write_buffer_size = 4 << 20;
    target_file_size_base = 2 * 1048576;
    max_bytes_for_level_base = 10 * 1048576;
    soft_pending_compaction_bytes_limit = 0;
    hard_pending_compaction_bytes_limit = 0;
  }
  if (rocksdb_major_version < 5) {
    level0_stop_writes_trigger = 24;
  } else if (rocksdb_major_version == 5 && rocksdb_minor_version < 2) {
    level0_stop_writes_trigger = 30;
  }
  compaction_pri = CompactionPri::kByCompensatedSize;

  return this;
}

//优化函数
DBOptions* DBOptions::OptimizeForSmallDb() {
  max_file_opening_threads = 1;
  max_open_files = 5000;
  return this;
}

ColumnFamilyOptions* ColumnFamilyOptions::OptimizeForSmallDb() {
  write_buffer_size = 2 << 20;
  target_file_size_base = 2 * 1048576;
  max_bytes_for_level_base = 10 * 1048576;
  soft_pending_compaction_bytes_limit = 256 * 1048576;
  hard_pending_compaction_bytes_limit = 1073741824ul;
  return this;
}

#ifndef ROCKSDB_LITE
ColumnFamilyOptions* ColumnFamilyOptions::OptimizeForPointLookup(
    uint64_t block_cache_size_mb) {
  prefix_extractor.reset(NewNoopTransform());
  BlockBasedTableOptions block_based_options;
  block_based_options.index_type = BlockBasedTableOptions::kHashSearch;
  block_based_options.filter_policy.reset(NewBloomFilterPolicy(10));
  block_based_options.block_cache =
      NewLRUCache(static_cast<size_t>(block_cache_size_mb * 1024 * 1024));
  table_factory.reset(new BlockBasedTableFactory(block_based_options));
  memtable_prefix_bloom_size_ratio = 0.02;
  return this;
}

ColumnFamilyOptions* ColumnFamilyOptions::OptimizeLevelStyleCompaction(
    uint64_t memtable_memory_budget) {
  write_buffer_size = static_cast<size_t>(memtable_memory_budget / 4);
//刷新到l0时合并两个memtable
  min_write_buffer_number_to_merge = 2;
//这意味着在最坏的情况下我们将使用50%的额外内存，但会减少
//写摊位。
  max_write_buffer_number = 6;
//尽快开始冲洗L0->L1。级别0上的每个文件都是
//（memtable_memory_budget/2）。当0级大于
//内存预算。
  level0_file_num_compaction_trigger = 2;
//不太重要，但我们不想创建太多文件
  target_file_size_base = memtable_memory_budget / 8;
//使1级尺寸等于0级尺寸，以便l0->l1的压实速度快
  max_bytes_for_level_base = memtable_memory_budget;

//水平式压实
  compaction_style = kCompactionStyleLevel;

//仅压缩级别>=2
  compression_per_level.resize(num_levels);
  for (int i = 0; i < num_levels; ++i) {
    if (i < 2) {
      compression_per_level[i] = kNoCompression;
    } else {
      compression_per_level[i] = kSnappyCompression;
    }
  }
  return this;
}

ColumnFamilyOptions* ColumnFamilyOptions::OptimizeUniversalStyleCompaction(
    uint64_t memtable_memory_budget) {
  write_buffer_size = static_cast<size_t>(memtable_memory_budget / 4);
//刷新到l0时合并两个memtable
  min_write_buffer_number_to_merge = 2;
//这意味着在最坏的情况下我们将使用50%的额外内存，但会减少
//写摊位。
  max_write_buffer_number = 6;
//万能式压实
  compaction_style = kCompactionStyleUniversal;
  compaction_options_universal.compression_size_percent = 80;
  return this;
}

DBOptions* DBOptions::IncreaseParallelism(int total_threads) {
  max_background_compactions = total_threads - 1;
  max_background_flushes = 1;
  env->SetBackgroundThreads(total_threads, Env::LOW);
  env->SetBackgroundThreads(1, Env::HIGH);
  return this;
}

#endif  //！摇滚乐

ReadOptions::ReadOptions()
    : snapshot(nullptr),
      iterate_upper_bound(nullptr),
      readahead_size(0),
      max_skippable_internal_keys(0),
      read_tier(kReadAllTier),
      verify_checksums(true),
      fill_cache(true),
      tailing(false),
      managed(false),
      total_order_seek(false),
      prefix_same_as_start(false),
      pin_data(false),
      background_purge_on_iterator_cleanup(false),
      ignore_range_deletions(false) {}

ReadOptions::ReadOptions(bool cksum, bool cache)
    : snapshot(nullptr),
      iterate_upper_bound(nullptr),
      readahead_size(0),
      max_skippable_internal_keys(0),
      read_tier(kReadAllTier),
      verify_checksums(cksum),
      fill_cache(cache),
      tailing(false),
      managed(false),
      total_order_seek(false),
      prefix_same_as_start(false),
      pin_data(false),
      background_purge_on_iterator_cleanup(false),
      ignore_range_deletions(false) {}

}  //命名空间rocksdb
