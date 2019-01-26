
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <cctype>
#include <cstring>
#include <unordered_map>
#include <inttypes.h>

#include "options/options_helper.h"
#include "options/options_parser.h"
#include "options/options_sanity_check.h"
#include "rocksdb/cache.h"
#include "rocksdb/convenience.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/utilities/leveldb_options.h"
#include "util/random.h"
#include "util/stderr_logger.h"
#include "util/string_util.h"
#include "util/testharness.h"
#include "util/testutil.h"

#ifndef GFLAGS
bool FLAGS_enable_print = false;
#else
#include <gflags/gflags.h>
using GFLAGS::ParseCommandLineFlags;
DEFINE_bool(enable_print, false, "Print options generated to console.");
#endif  //GFLAGS

namespace rocksdb {

class OptionsTest : public testing::Test {};

#ifndef ROCKSDB_LITE  //RocksDB-Lite中不支持GetOptionsFromMap
TEST_F(OptionsTest, GetOptionsFromMapTest) {
  std::unordered_map<std::string, std::string> cf_options_map = {
      {"write_buffer_size", "1"},
      {"max_write_buffer_number", "2"},
      {"min_write_buffer_number_to_merge", "3"},
      {"max_write_buffer_number_to_maintain", "99"},
      {"compression", "kSnappyCompression"},
      {"compression_per_level",
       "kNoCompression:"
       "kSnappyCompression:"
       "kZlibCompression:"
       "kBZip2Compression:"
       "kLZ4Compression:"
       "kLZ4HCCompression:"
       "kXpressCompression:"
       "kZSTD:"
       "kZSTDNotFinalCompression"},
      {"bottommost_compression", "kLZ4Compression"},
      {"compression_opts", "4:5:6:7"},
      {"num_levels", "8"},
      {"level0_file_num_compaction_trigger", "8"},
      {"level0_slowdown_writes_trigger", "9"},
      {"level0_stop_writes_trigger", "10"},
      {"target_file_size_base", "12"},
      {"target_file_size_multiplier", "13"},
      {"max_bytes_for_level_base", "14"},
      {"level_compaction_dynamic_level_bytes", "true"},
      {"max_bytes_for_level_multiplier", "15.0"},
      {"max_bytes_for_level_multiplier_additional", "16:17:18"},
      {"max_compaction_bytes", "21"},
      {"soft_rate_limit", "1.1"},
      {"hard_rate_limit", "2.1"},
      {"hard_pending_compaction_bytes_limit", "211"},
      {"arena_block_size", "22"},
      {"disable_auto_compactions", "true"},
      {"compaction_style", "kCompactionStyleLevel"},
      {"compaction_pri", "kOldestSmallestSeqFirst"},
      {"verify_checksums_in_compaction", "false"},
      {"compaction_options_fifo", "23"},
      {"max_sequential_skip_in_iterations", "24"},
      {"inplace_update_support", "true"},
      {"report_bg_io_stats", "true"},
      {"compaction_measure_io_stats", "false"},
      {"inplace_update_num_locks", "25"},
      {"memtable_prefix_bloom_size_ratio", "0.26"},
      {"memtable_huge_page_size", "28"},
      {"bloom_locality", "29"},
      {"max_successive_merges", "30"},
      {"min_partial_merge_operands", "31"},
      {"prefix_extractor", "fixed:31"},
      {"optimize_filters_for_hits", "true"},
  };

  std::unordered_map<std::string, std::string> db_options_map = {
      {"create_if_missing", "false"},
      {"create_missing_column_families", "true"},
      {"error_if_exists", "false"},
      {"paranoid_checks", "true"},
      {"max_open_files", "32"},
      {"max_total_wal_size", "33"},
      {"use_fsync", "true"},
      {"db_log_dir", "/db_log_dir"},
      {"wal_dir", "/wal_dir"},
      {"delete_obsolete_files_period_micros", "34"},
      {"max_background_compactions", "35"},
      {"max_background_flushes", "36"},
      {"max_log_file_size", "37"},
      {"log_file_time_to_roll", "38"},
      {"keep_log_file_num", "39"},
      {"recycle_log_file_num", "5"},
      {"max_manifest_file_size", "40"},
      {"table_cache_numshardbits", "41"},
      {"WAL_ttl_seconds", "43"},
      {"WAL_size_limit_MB", "44"},
      {"manifest_preallocation_size", "45"},
      {"allow_mmap_reads", "true"},
      {"allow_mmap_writes", "false"},
      {"use_direct_reads", "false"},
      {"use_direct_io_for_flush_and_compaction", "false"},
      {"is_fd_close_on_exec", "true"},
      {"skip_log_error_on_recovery", "false"},
      {"stats_dump_period_sec", "46"},
      {"advise_random_on_open", "true"},
      {"use_adaptive_mutex", "false"},
      {"new_table_reader_for_compaction_inputs", "true"},
      {"compaction_readahead_size", "100"},
      {"random_access_max_buffer_size", "3145728"},
      {"writable_file_max_buffer_size", "314159"},
      {"bytes_per_sync", "47"},
      {"wal_bytes_per_sync", "48"},
  };

  ColumnFamilyOptions base_cf_opt;
  ColumnFamilyOptions new_cf_opt;
  ASSERT_OK(GetColumnFamilyOptionsFromMap(
            base_cf_opt, cf_options_map, &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 1U);
  ASSERT_EQ(new_cf_opt.max_write_buffer_number, 2);
  ASSERT_EQ(new_cf_opt.min_write_buffer_number_to_merge, 3);
  ASSERT_EQ(new_cf_opt.max_write_buffer_number_to_maintain, 99);
  ASSERT_EQ(new_cf_opt.compression, kSnappyCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level.size(), 9U);
  ASSERT_EQ(new_cf_opt.compression_per_level[0], kNoCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level[1], kSnappyCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level[2], kZlibCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level[3], kBZip2Compression);
  ASSERT_EQ(new_cf_opt.compression_per_level[4], kLZ4Compression);
  ASSERT_EQ(new_cf_opt.compression_per_level[5], kLZ4HCCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level[6], kXpressCompression);
  ASSERT_EQ(new_cf_opt.compression_per_level[7], kZSTD);
  ASSERT_EQ(new_cf_opt.compression_per_level[8], kZSTDNotFinalCompression);
  ASSERT_EQ(new_cf_opt.compression_opts.window_bits, 4);
  ASSERT_EQ(new_cf_opt.compression_opts.level, 5);
  ASSERT_EQ(new_cf_opt.compression_opts.strategy, 6);
  ASSERT_EQ(new_cf_opt.compression_opts.max_dict_bytes, 7);
  ASSERT_EQ(new_cf_opt.bottommost_compression, kLZ4Compression);
  ASSERT_EQ(new_cf_opt.num_levels, 8);
  ASSERT_EQ(new_cf_opt.level0_file_num_compaction_trigger, 8);
  ASSERT_EQ(new_cf_opt.level0_slowdown_writes_trigger, 9);
  ASSERT_EQ(new_cf_opt.level0_stop_writes_trigger, 10);
  ASSERT_EQ(new_cf_opt.target_file_size_base, static_cast<uint64_t>(12));
  ASSERT_EQ(new_cf_opt.target_file_size_multiplier, 13);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_base, 14U);
  ASSERT_EQ(new_cf_opt.level_compaction_dynamic_level_bytes, true);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_multiplier, 15.0);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_multiplier_additional.size(), 3U);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_multiplier_additional[0], 16);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_multiplier_additional[1], 17);
  ASSERT_EQ(new_cf_opt.max_bytes_for_level_multiplier_additional[2], 18);
  ASSERT_EQ(new_cf_opt.max_compaction_bytes, 21);
  ASSERT_EQ(new_cf_opt.hard_pending_compaction_bytes_limit, 211);
  ASSERT_EQ(new_cf_opt.arena_block_size, 22U);
  ASSERT_EQ(new_cf_opt.disable_auto_compactions, true);
  ASSERT_EQ(new_cf_opt.compaction_style, kCompactionStyleLevel);
  ASSERT_EQ(new_cf_opt.compaction_pri, kOldestSmallestSeqFirst);
  ASSERT_EQ(new_cf_opt.compaction_options_fifo.max_table_files_size,
            static_cast<uint64_t>(23));
  ASSERT_EQ(new_cf_opt.max_sequential_skip_in_iterations,
            static_cast<uint64_t>(24));
  ASSERT_EQ(new_cf_opt.inplace_update_support, true);
  ASSERT_EQ(new_cf_opt.inplace_update_num_locks, 25U);
  ASSERT_EQ(new_cf_opt.memtable_prefix_bloom_size_ratio, 0.26);
  ASSERT_EQ(new_cf_opt.memtable_huge_page_size, 28U);
  ASSERT_EQ(new_cf_opt.bloom_locality, 29U);
  ASSERT_EQ(new_cf_opt.max_successive_merges, 30U);
  ASSERT_TRUE(new_cf_opt.prefix_extractor != nullptr);
  ASSERT_EQ(new_cf_opt.optimize_filters_for_hits, true);
  ASSERT_EQ(std::string(new_cf_opt.prefix_extractor->Name()),
            "rocksdb.FixedPrefix.31");

  cf_options_map["write_buffer_size"] = "hello";
  ASSERT_NOK(GetColumnFamilyOptionsFromMap(
             base_cf_opt, cf_options_map, &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  cf_options_map["write_buffer_size"] = "1";
  ASSERT_OK(GetColumnFamilyOptionsFromMap(
            base_cf_opt, cf_options_map, &new_cf_opt));

  cf_options_map["unknown_option"] = "1";
  ASSERT_NOK(GetColumnFamilyOptionsFromMap(
             base_cf_opt, cf_options_map, &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  ASSERT_OK(GetColumnFamilyOptionsFromMap(base_cf_opt, cf_options_map,
                                          &new_cf_opt,
                                          /*SE，/*输入\字符串\已转义*/
                                          真/*忽略\未知\选项*/));

  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(
      /*e_-cf_-opt，new_-cf_-opt，nullptr，/*new_-opt_-map*/
      ksanitylelevellooselycompatible/*来自checkoptionscompatibility*/));

  ASSERT_NOK(RocksDBOptionsParser::VerifyCFOptions(
      /*e_-cf_-opt，new_-cf_-opt，nullptr，/*new_-opt_-map*/
      ksanitylevelexactmatch/*验证选项的默认值*/));


  DBOptions base_db_opt;
  DBOptions new_db_opt;
  ASSERT_OK(GetDBOptionsFromMap(base_db_opt, db_options_map, &new_db_opt));
  ASSERT_EQ(new_db_opt.create_if_missing, false);
  ASSERT_EQ(new_db_opt.create_missing_column_families, true);
  ASSERT_EQ(new_db_opt.error_if_exists, false);
  ASSERT_EQ(new_db_opt.paranoid_checks, true);
  ASSERT_EQ(new_db_opt.max_open_files, 32);
  ASSERT_EQ(new_db_opt.max_total_wal_size, static_cast<uint64_t>(33));
  ASSERT_EQ(new_db_opt.use_fsync, true);
  ASSERT_EQ(new_db_opt.db_log_dir, "/db_log_dir");
  ASSERT_EQ(new_db_opt.wal_dir, "/wal_dir");
  ASSERT_EQ(new_db_opt.delete_obsolete_files_period_micros,
            static_cast<uint64_t>(34));
  ASSERT_EQ(new_db_opt.max_background_compactions, 35);
  ASSERT_EQ(new_db_opt.max_background_flushes, 36);
  ASSERT_EQ(new_db_opt.max_log_file_size, 37U);
  ASSERT_EQ(new_db_opt.log_file_time_to_roll, 38U);
  ASSERT_EQ(new_db_opt.keep_log_file_num, 39U);
  ASSERT_EQ(new_db_opt.recycle_log_file_num, 5U);
  ASSERT_EQ(new_db_opt.max_manifest_file_size, static_cast<uint64_t>(40));
  ASSERT_EQ(new_db_opt.table_cache_numshardbits, 41);
  ASSERT_EQ(new_db_opt.WAL_ttl_seconds, static_cast<uint64_t>(43));
  ASSERT_EQ(new_db_opt.WAL_size_limit_MB, static_cast<uint64_t>(44));
  ASSERT_EQ(new_db_opt.manifest_preallocation_size, 45U);
  ASSERT_EQ(new_db_opt.allow_mmap_reads, true);
  ASSERT_EQ(new_db_opt.allow_mmap_writes, false);
  ASSERT_EQ(new_db_opt.use_direct_reads, false);
  ASSERT_EQ(new_db_opt.use_direct_io_for_flush_and_compaction, false);
  ASSERT_EQ(new_db_opt.is_fd_close_on_exec, true);
  ASSERT_EQ(new_db_opt.skip_log_error_on_recovery, false);
  ASSERT_EQ(new_db_opt.stats_dump_period_sec, 46U);
  ASSERT_EQ(new_db_opt.advise_random_on_open, true);
  ASSERT_EQ(new_db_opt.use_adaptive_mutex, false);
  ASSERT_EQ(new_db_opt.new_table_reader_for_compaction_inputs, true);
  ASSERT_EQ(new_db_opt.compaction_readahead_size, 100);
  ASSERT_EQ(new_db_opt.random_access_max_buffer_size, 3145728);
  ASSERT_EQ(new_db_opt.writable_file_max_buffer_size, 314159);
  ASSERT_EQ(new_db_opt.bytes_per_sync, static_cast<uint64_t>(47));
  ASSERT_EQ(new_db_opt.wal_bytes_per_sync, static_cast<uint64_t>(48));

  db_options_map["max_open_files"] = "hello";
  ASSERT_NOK(GetDBOptionsFromMap(base_db_opt, db_options_map, &new_db_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(base_db_opt, new_db_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(
      /*e_db_opt，new_db_opt，nullptr，/*new_opt_map*/
      Ksanitylevellooselycompatible/*来自checkoptionscompatibility*/));


//unknow选项解析失败，不忽略\u unknown \u options=true
  db_options_map["unknown_db_option"] = "1";
  ASSERT_NOK(GetDBOptionsFromMap(base_db_opt, db_options_map, &new_db_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(base_db_opt, new_db_opt));

  ASSERT_OK(GetDBOptionsFromMap(base_db_opt, db_options_map, &new_db_opt,
                                /*SE，/*输入\字符串\已转义*/
                                真/*忽略\未知\选项*/));

  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(
      /*e_db_opt，new_db_opt，nullptr，/*new_opt_map*/
      Ksanitylevellooselycompatible/*来自checkoptionscompatibility*/));

  ASSERT_NOK(RocksDBOptionsParser::VerifyDBOptions(
      /*e_db_opt，new_db_opt，nullptr，/*new_opt_mat*/
      ksanitylevelexactmatch/*verifydboptions的默认值*/));

}
#endif  //！摇滚乐

#ifndef ROCKSDB_LITE  //中不支持getColumnFamilyOptionsFromString
//摇滚乐
TEST_F(OptionsTest, GetColumnFamilyOptionsFromStringTest) {
  ColumnFamilyOptions base_cf_opt;
  ColumnFamilyOptions new_cf_opt;
  base_cf_opt.table_factory.reset();
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt, "", &new_cf_opt));
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=5", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 5U);
  ASSERT_TRUE(new_cf_opt.table_factory == nullptr);
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=6;", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 6U);
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "  write_buffer_size =  7  ", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 7U);
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "  write_buffer_size =  8 ; ", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 8U);
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=9;max_write_buffer_number=10", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 9U);
  ASSERT_EQ(new_cf_opt.max_write_buffer_number, 10);
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=11; max_write_buffer_number  =  12 ;",
            &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 11U);
  ASSERT_EQ(new_cf_opt.max_write_buffer_number, 12);
//错误名称“max_write_buffer_number”
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=13;max_write_buffer_number_=14;",
              &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//错误的键/值对
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=13;max_write_buffer_number;", &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//取整值错误
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=13;max_write_buffer_number=;", &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//缺少选项名称
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=13; =100;", &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  const int64_t kilo = 1024UL;
  const int64_t mega = 1024 * kilo;
  const int64_t giga = 1024 * mega;
  const int64_t tera = 1024 * giga;

//单位（k）
  ASSERT_OK(GetColumnFamilyOptionsFromString(
      base_cf_opt, "max_write_buffer_number=15K", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.max_write_buffer_number, 15 * kilo);
//单位（m）
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "max_write_buffer_number=16m;inplace_update_num_locks=17M",
            &new_cf_opt));
  ASSERT_EQ(new_cf_opt.max_write_buffer_number, 16 * mega);
  ASSERT_EQ(new_cf_opt.inplace_update_num_locks, 17 * mega);
//单位（g）
  ASSERT_OK(GetColumnFamilyOptionsFromString(
      base_cf_opt,
      "write_buffer_size=18g;prefix_extractor=capped:8;"
      "arena_block_size=19G",
      &new_cf_opt));

  ASSERT_EQ(new_cf_opt.write_buffer_size, 18 * giga);
  ASSERT_EQ(new_cf_opt.arena_block_size, 19 * giga);
  ASSERT_TRUE(new_cf_opt.prefix_extractor.get() != nullptr);
  std::string prefix_name(new_cf_opt.prefix_extractor->Name());
  ASSERT_EQ(prefix_name, "rocksdb.CappedPrefix.8");

//单位（t）
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=20t;arena_block_size=21T", &new_cf_opt));
  ASSERT_EQ(new_cf_opt.write_buffer_size, 20 * tera);
  ASSERT_EQ(new_cf_opt.arena_block_size, 21 * tera);

//基于块的嵌套表选项
//空的
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "block_based_table_factory={};arena_block_size=1024",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.table_factory != nullptr);
//非空的
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "block_based_table_factory={block_cache=1M;block_size=4;};"
            "arena_block_size=1024",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.table_factory != nullptr);
//最后一个
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "block_based_table_factory={block_cache=1M;block_size=4;}",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.table_factory != nullptr);
//大括号不匹配
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=10;max_write_buffer_number=16;"
             "block_based_table_factory={{{block_size=4;};"
             "arena_block_size=1024",
             &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//关闭大括号后出现意外字符
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=10;max_write_buffer_number=16;"
             "block_based_table_factory={block_size=4;}};"
             "arena_block_size=1024",
             &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=10;max_write_buffer_number=16;"
             "block_based_table_factory={block_size=4;}xdfa;"
             "arena_block_size=1024",
             &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=10;max_write_buffer_number=16;"
             "block_based_table_factory={block_size=4;}xdfa",
             &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//基于块的表选项无效
  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
             "write_buffer_size=10;max_write_buffer_number=16;"
             "block_based_table_factory={xx_block_size=4;}",
             &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
           "optimize_filters_for_hits=true",
           &new_cf_opt));
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "optimize_filters_for_hits=false",
            &new_cf_opt));

  ASSERT_NOK(GetColumnFamilyOptionsFromString(base_cf_opt,
              "optimize_filters_for_hits=junk",
              &new_cf_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_cf_opt, new_cf_opt));

//嵌套的普通表选项
//空的
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "plain_table_factory={};arena_block_size=1024",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.table_factory != nullptr);
  ASSERT_EQ(std::string(new_cf_opt.table_factory->Name()), "PlainTable");
//非空的
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "plain_table_factory={user_key_len=66;bloom_bits_per_key=20;};"
            "arena_block_size=1024",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.table_factory != nullptr);
  ASSERT_EQ(std::string(new_cf_opt.table_factory->Name()), "PlainTable");

//Memtable工厂
  ASSERT_OK(GetColumnFamilyOptionsFromString(base_cf_opt,
            "write_buffer_size=10;max_write_buffer_number=16;"
            "memtable=skip_list:10;arena_block_size=1024",
            &new_cf_opt));
  ASSERT_TRUE(new_cf_opt.memtable_factory != nullptr);
  ASSERT_EQ(std::string(new_cf_opt.memtable_factory->Name()), "SkipListFactory");
}
#endif  //！摇滚乐

#ifndef ROCKSDB_LITE  //不支持GetBlockBasedTableOptionsFromsString
TEST_F(OptionsTest, GetBlockBasedTableOptionsFromString) {
  BlockBasedTableOptions table_opt;
  BlockBasedTableOptions new_opt;
//确保默认值被其他内容覆盖
  ASSERT_OK(GetBlockBasedTableOptionsFromString(table_opt,
            "cache_index_and_filter_blocks=1;index_type=kHashSearch;"
            "checksum=kxxHash;hash_index_allow_collision=1;no_block_cache=1;"
            "block_cache=1M;block_cache_compressed=1k;block_size=1024;"
            "block_size_deviation=8;block_restart_interval=4;"
            "filter_policy=bloomfilter:4:true;whole_key_filtering=1;",
            &new_opt));
  ASSERT_TRUE(new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(new_opt.index_type, BlockBasedTableOptions::kHashSearch);
  ASSERT_EQ(new_opt.checksum, ChecksumType::kxxHash);
  ASSERT_TRUE(new_opt.hash_index_allow_collision);
  ASSERT_TRUE(new_opt.no_block_cache);
  ASSERT_TRUE(new_opt.block_cache != nullptr);
  ASSERT_EQ(new_opt.block_cache->GetCapacity(), 1024UL*1024UL);
  ASSERT_TRUE(new_opt.block_cache_compressed != nullptr);
  ASSERT_EQ(new_opt.block_cache_compressed->GetCapacity(), 1024UL);
  ASSERT_EQ(new_opt.block_size, 1024UL);
  ASSERT_EQ(new_opt.block_size_deviation, 8);
  ASSERT_EQ(new_opt.block_restart_interval, 4);
  ASSERT_TRUE(new_opt.filter_policy != nullptr);

//未知选项
  ASSERT_NOK(GetBlockBasedTableOptionsFromString(table_opt,
             "cache_index_and_filter_blocks=1;index_type=kBinarySearch;"
             "bad_option=1",
             &new_opt));
  ASSERT_EQ(table_opt.cache_index_and_filter_blocks,
            new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(table_opt.index_type, new_opt.index_type);

//无法识别的索引类型
  ASSERT_NOK(GetBlockBasedTableOptionsFromString(table_opt,
             "cache_index_and_filter_blocks=1;index_type=kBinarySearchXX",
             &new_opt));
  ASSERT_EQ(table_opt.cache_index_and_filter_blocks,
            new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(table_opt.index_type, new_opt.index_type);

//无法识别的校验和类型
  ASSERT_NOK(GetBlockBasedTableOptionsFromString(table_opt,
             "cache_index_and_filter_blocks=1;checksum=kxxHashXX",
             &new_opt));
  ASSERT_EQ(table_opt.cache_index_and_filter_blocks,
            new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(table_opt.index_type, new_opt.index_type);

//无法识别的筛选器策略名称
  ASSERT_NOK(GetBlockBasedTableOptionsFromString(table_opt,
             "cache_index_and_filter_blocks=1;"
             "filter_policy=bloomfilterxx:4:true",
             &new_opt));
  ASSERT_EQ(table_opt.cache_index_and_filter_blocks,
            new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(table_opt.filter_policy, new_opt.filter_policy);

//无法识别的筛选器策略配置
  ASSERT_NOK(GetBlockBasedTableOptionsFromString(table_opt,
             "cache_index_and_filter_blocks=1;"
             "filter_policy=bloomfilter:4",
             &new_opt));
  ASSERT_EQ(table_opt.cache_index_and_filter_blocks,
            new_opt.cache_index_and_filter_blocks);
  ASSERT_EQ(table_opt.filter_policy, new_opt.filter_policy);
}
#endif  //！摇滚乐


#ifndef ROCKSDB_LITE  //不支持GetPlainTableOptionsFromString
TEST_F(OptionsTest, GetPlainTableOptionsFromString) {
  PlainTableOptions table_opt;
  PlainTableOptions new_opt;
//确保默认值被其他内容覆盖
  ASSERT_OK(GetPlainTableOptionsFromString(table_opt,
            "user_key_len=66;bloom_bits_per_key=20;hash_table_ratio=0.5;"
            "index_sparseness=8;huge_page_tlb_size=4;encoding_type=kPrefix;"
            "full_scan_mode=true;store_index_in_file=true",
            &new_opt));
  ASSERT_EQ(new_opt.user_key_len, 66);
  ASSERT_EQ(new_opt.bloom_bits_per_key, 20);
  ASSERT_EQ(new_opt.hash_table_ratio, 0.5);
  ASSERT_EQ(new_opt.index_sparseness, 8);
  ASSERT_EQ(new_opt.huge_page_tlb_size, 4);
  ASSERT_EQ(new_opt.encoding_type, EncodingType::kPrefix);
  ASSERT_TRUE(new_opt.full_scan_mode);
  ASSERT_TRUE(new_opt.store_index_in_file);

//未知选项
  ASSERT_NOK(GetPlainTableOptionsFromString(table_opt,
             "user_key_len=66;bloom_bits_per_key=20;hash_table_ratio=0.5;"
             "bad_option=1",
             &new_opt));

//无法识别的编码类型
  ASSERT_NOK(GetPlainTableOptionsFromString(table_opt,
             "user_key_len=66;bloom_bits_per_key=20;hash_table_ratio=0.5;"
             "encoding_type=kPrefixXX",
             &new_opt));
}
#endif  //！摇滚乐

#ifndef ROCKSDB_LITE  //不支持GetMemTableRepFactoryFromString
TEST_F(OptionsTest, GetMemTableRepFactoryFromString) {
  std::unique_ptr<MemTableRepFactory> new_mem_factory = nullptr;

  ASSERT_OK(GetMemTableRepFactoryFromString("skip_list", &new_mem_factory));
  ASSERT_OK(GetMemTableRepFactoryFromString("skip_list:16", &new_mem_factory));
  ASSERT_EQ(std::string(new_mem_factory->Name()), "SkipListFactory");
  ASSERT_NOK(GetMemTableRepFactoryFromString("skip_list:16:invalid_opt",
                                             &new_mem_factory));

  ASSERT_OK(GetMemTableRepFactoryFromString("prefix_hash", &new_mem_factory));
  ASSERT_OK(GetMemTableRepFactoryFromString("prefix_hash:1000",
                                            &new_mem_factory));
  ASSERT_EQ(std::string(new_mem_factory->Name()), "HashSkipListRepFactory");
  ASSERT_NOK(GetMemTableRepFactoryFromString("prefix_hash:1000:invalid_opt",
                                             &new_mem_factory));

  ASSERT_OK(GetMemTableRepFactoryFromString("hash_linkedlist",
                                            &new_mem_factory));
  ASSERT_OK(GetMemTableRepFactoryFromString("hash_linkedlist:1000",
                                            &new_mem_factory));
  ASSERT_EQ(std::string(new_mem_factory->Name()), "HashLinkListRepFactory");
  ASSERT_NOK(GetMemTableRepFactoryFromString("hash_linkedlist:1000:invalid_opt",
                                             &new_mem_factory));

  ASSERT_OK(GetMemTableRepFactoryFromString("vector", &new_mem_factory));
  ASSERT_OK(GetMemTableRepFactoryFromString("vector:1024", &new_mem_factory));
  ASSERT_EQ(std::string(new_mem_factory->Name()), "VectorRepFactory");
  ASSERT_NOK(GetMemTableRepFactoryFromString("vector:1024:invalid_opt",
                                             &new_mem_factory));

  ASSERT_NOK(GetMemTableRepFactoryFromString("cuckoo", &new_mem_factory));
  ASSERT_OK(GetMemTableRepFactoryFromString("cuckoo:1024", &new_mem_factory));
  ASSERT_EQ(std::string(new_mem_factory->Name()), "HashCuckooRepFactory");

  ASSERT_NOK(GetMemTableRepFactoryFromString("bad_factory", &new_mem_factory));
}
#endif  //！摇滚乐

#ifndef ROCKSDB_LITE  //RocksDB Lite不支持GetOptionsFromsString
TEST_F(OptionsTest, GetOptionsFromStringTest) {
  Options base_options, new_options;
  base_options.write_buffer_size = 20;
  base_options.min_write_buffer_number_to_merge = 15;
  BlockBasedTableOptions block_based_table_options;
  block_based_table_options.cache_index_and_filter_blocks = true;
  base_options.table_factory.reset(
      NewBlockBasedTableFactory(block_based_table_options));
  ASSERT_OK(GetOptionsFromString(
      base_options,
      "write_buffer_size=10;max_write_buffer_number=16;"
      "block_based_table_factory={block_cache=1M;block_size=4;};"
      "compression_opts=4:5:6;create_if_missing=true;max_open_files=1;"
      "rate_limiter_bytes_per_sec=1024",
      &new_options));

  ASSERT_EQ(new_options.compression_opts.window_bits, 4);
  ASSERT_EQ(new_options.compression_opts.level, 5);
  ASSERT_EQ(new_options.compression_opts.strategy, 6);
  ASSERT_EQ(new_options.compression_opts.max_dict_bytes, 0);
  ASSERT_EQ(new_options.bottommost_compression, kDisableCompressionOption);
  ASSERT_EQ(new_options.write_buffer_size, 10U);
  ASSERT_EQ(new_options.max_write_buffer_number, 16);
  BlockBasedTableOptions new_block_based_table_options =
      dynamic_cast<BlockBasedTableFactory*>(new_options.table_factory.get())
          ->table_options();
  ASSERT_EQ(new_block_based_table_options.block_cache->GetCapacity(), 1U << 20);
  ASSERT_EQ(new_block_based_table_options.block_size, 4U);
//不覆盖基于块的表选项
  ASSERT_TRUE(new_block_based_table_options.cache_index_and_filter_blocks);

  ASSERT_EQ(new_options.create_if_missing, true);
  ASSERT_EQ(new_options.max_open_files, 1);
  ASSERT_TRUE(new_options.rate_limiter.get() != nullptr);
}

TEST_F(OptionsTest, DBOptionsSerialization) {
  Options base_options, new_options;
  Random rnd(301);

//第一阶段：基础选项发生重大变化
  test::RandomInitDBOptions(&base_options, &rnd);

//第2阶段：从基本选项获取字符串
  std::string base_options_file_content;
  ASSERT_OK(GetStringFromDBOptions(&base_options_file_content, base_options));

//阶段3：从派生字符串中设置新的\选项并预期
//new_options==base_选项
  ASSERT_OK(GetDBOptionsFromString(DBOptions(), base_options_file_content,
                                   &new_options));
  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(base_options, new_options));
}

TEST_F(OptionsTest, ColumnFamilyOptionsSerialization) {
  ColumnFamilyOptions base_opt, new_opt;
  Random rnd(302);
//阶段1：随机分配基础选项
//自定义类型选项
  test::RandomInitCFOptions(&base_opt, &rnd);

//第2阶段：从基本选项获取字符串
  std::string base_options_file_content;
  ASSERT_OK(
      GetStringFromColumnFamilyOptions(&base_options_file_content, base_opt));

//第3阶段：从派生字符串中设置新选项，并预期
//new_opt==基本_opt
  ASSERT_OK(GetColumnFamilyOptionsFromString(
      ColumnFamilyOptions(), base_options_file_content, &new_opt));
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(base_opt, new_opt));
  if (base_opt.compaction_filter) {
    delete base_opt.compaction_filter;
  }
}

#endif  //！摇滚乐

Status StringToMap(
    const std::string& opts_str,
    std::unordered_map<std::string, std::string>* opts_map);

#ifndef ROCKSDB_LITE  //RocksDB-Lite中不支持StringTomap
TEST_F(OptionsTest, StringToMapTest) {
  std::unordered_map<std::string, std::string> opts_map;
//正则期权
  ASSERT_OK(StringToMap("k1=v1;k2=v2;k3=v3", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "v2");
  ASSERT_EQ(opts_map["k3"], "v3");
//用“=”值
  opts_map.clear();
  ASSERT_OK(StringToMap("k1==v1;k2=v2=;", &opts_map));
  ASSERT_EQ(opts_map["k1"], "=v1");
  ASSERT_EQ(opts_map["k2"], "v2=");
//越权期权
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k1=v2;k3=v3", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v2");
  ASSERT_EQ(opts_map["k3"], "v3");
//空值
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2=;k3=v3;k4=", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_TRUE(opts_map.find("k2") != opts_map.end());
  ASSERT_EQ(opts_map["k2"], "");
  ASSERT_EQ(opts_map["k3"], "v3");
  ASSERT_TRUE(opts_map.find("k4") != opts_map.end());
  ASSERT_EQ(opts_map["k4"], "");
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2=;k3=v3;k4=   ", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_TRUE(opts_map.find("k2") != opts_map.end());
  ASSERT_EQ(opts_map["k2"], "");
  ASSERT_EQ(opts_map["k3"], "v3");
  ASSERT_TRUE(opts_map.find("k4") != opts_map.end());
  ASSERT_EQ(opts_map["k4"], "");
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2=;k3=", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_TRUE(opts_map.find("k2") != opts_map.end());
  ASSERT_EQ(opts_map["k2"], "");
  ASSERT_TRUE(opts_map.find("k3") != opts_map.end());
  ASSERT_EQ(opts_map["k3"], "");
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2=;k3=;", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_TRUE(opts_map.find("k2") != opts_map.end());
  ASSERT_EQ(opts_map["k2"], "");
  ASSERT_TRUE(opts_map.find("k3") != opts_map.end());
  ASSERT_EQ(opts_map["k3"], "");
//常规嵌套选项
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2={nk1=nv1;nk2=nv2};k3=v3", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "nk1=nv1;nk2=nv2");
  ASSERT_EQ(opts_map["k3"], "v3");
//多层嵌套选项
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2={nk1=nv1;nk2={nnk1=nnk2}};"
                        "k3={nk1={nnk1={nnnk1=nnnv1;nnnk2;nnnv2}}};k4=v4",
                        &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "nk1=nv1;nk2={nnk1=nnk2}");
  ASSERT_EQ(opts_map["k3"], "nk1={nnk1={nnnk1=nnnv1;nnnk2;nnnv2}}");
  ASSERT_EQ(opts_map["k4"], "v4");
//大括号内有垃圾
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2={dfad=};k3={=};k4=v4",
                        &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "dfad=");
  ASSERT_EQ(opts_map["k3"], "=");
  ASSERT_EQ(opts_map["k4"], "v4");
//空嵌套选项
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2={};", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "");
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2={{{{}}}{}{}};", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "{{{}}}{}{}");
//带有随机空格
  opts_map.clear();
  ASSERT_OK(StringToMap("  k1 =  v1 ; k2= {nk1=nv1; nk2={nnk1=nnk2}}  ; "
                        "k3={  {   } }; k4= v4  ",
                        &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "nk1=nv1; nk2={nnk1=nnk2}");
  ASSERT_EQ(opts_map["k3"], "{   }");
  ASSERT_EQ(opts_map["k4"], "v4");

//空键
  ASSERT_NOK(StringToMap("k1=v1;k2=v2;=", &opts_map));
  ASSERT_NOK(StringToMap("=v1;k2=v2", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2v2;", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2=v2;fadfa", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2=v2;;", &opts_map));
//大括号不匹配
  ASSERT_NOK(StringToMap("k1=v1;k2={;k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{};k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={}};k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{}{}}};k3=v3", &opts_map));
//但是这是有效的！
  opts_map.clear();
  ASSERT_OK(StringToMap("k1=v1;k2=};k3=v3", &opts_map));
  ASSERT_EQ(opts_map["k1"], "v1");
  ASSERT_EQ(opts_map["k2"], "}");
  ASSERT_EQ(opts_map["k3"], "v3");

//关闭大括号后字符无效
  ASSERT_NOK(StringToMap("k1=v1;k2={{}}{};k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{}}cfda;k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{}}  cfda;k3=v3", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{}}  cfda", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{}}{}", &opts_map));
  ASSERT_NOK(StringToMap("k1=v1;k2={{dfdl}adfa}{}", &opts_map));
}
#endif  //摇滚乐

#ifndef ROCKSDB_LITE  //RocksDB-Lite中不支持StringTomap
TEST_F(OptionsTest, StringToMapRandomTest) {
  std::unordered_map<std::string, std::string> opts_map;
//确保segfault不被半随机字符串击中

  std::vector<std::string> bases = {
      "a={aa={};tt={xxx={}}};c=defff",
      "a={aa={};tt={xxx={}}};c=defff;d={{}yxx{}3{xx}}",
      "abc={{}{}{}{{{}}}{{}{}{}{}{}{}{}"};

  for (std::string base : bases) {
    for (int rand_seed = 301; rand_seed < 401; rand_seed++) {
      Random rnd(rand_seed);
      for (int attempt = 0; attempt < 10; attempt++) {
        std::string str = base;
//将随机位置替换为空格
        size_t pos = static_cast<size_t>(
            rnd.Uniform(static_cast<int>(base.size())));
        str[pos] = ' ';
        Status s = StringToMap(str, &opts_map);
        ASSERT_TRUE(s.ok() || s.IsInvalidArgument());
        opts_map.clear();
      }
    }
  }

//随机构造字符串
  std::vector<char> chars = {'{', '}', ' ', '=', ';', 'c'};
  for (int rand_seed = 301; rand_seed < 1301; rand_seed++) {
    Random rnd(rand_seed);
    int len = rnd.Uniform(30);
    std::string str = "";
    for (int attempt = 0; attempt < len; attempt++) {
//添加随机字符
      size_t pos = static_cast<size_t>(
          rnd.Uniform(static_cast<int>(chars.size())));
      str.append(1, chars[pos]);
    }
    Status s = StringToMap(str, &opts_map);
    ASSERT_TRUE(s.ok() || s.IsInvalidArgument());
    s = StringToMap("name=" + str, &opts_map);
    ASSERT_TRUE(s.ok() || s.IsInvalidArgument());
    opts_map.clear();
  }
}

TEST_F(OptionsTest, GetStringFromCompressionType) {
  std::string res;

  ASSERT_OK(GetStringFromCompressionType(&res, kNoCompression));
  ASSERT_EQ(res, "kNoCompression");

  ASSERT_OK(GetStringFromCompressionType(&res, kSnappyCompression));
  ASSERT_EQ(res, "kSnappyCompression");

  ASSERT_OK(GetStringFromCompressionType(&res, kDisableCompressionOption));
  ASSERT_EQ(res, "kDisableCompressionOption");

  ASSERT_OK(GetStringFromCompressionType(&res, kLZ4Compression));
  ASSERT_EQ(res, "kLZ4Compression");

  ASSERT_OK(GetStringFromCompressionType(&res, kZlibCompression));
  ASSERT_EQ(res, "kZlibCompression");

  ASSERT_NOK(
      GetStringFromCompressionType(&res, static_cast<CompressionType>(-10)));
}
#endif  //！摇滚乐

TEST_F(OptionsTest, ConvertOptionsTest) {
  LevelDBOptions leveldb_opt;
  Options converted_opt = ConvertOptions(leveldb_opt);

  ASSERT_EQ(converted_opt.create_if_missing, leveldb_opt.create_if_missing);
  ASSERT_EQ(converted_opt.error_if_exists, leveldb_opt.error_if_exists);
  ASSERT_EQ(converted_opt.paranoid_checks, leveldb_opt.paranoid_checks);
  ASSERT_EQ(converted_opt.env, leveldb_opt.env);
  ASSERT_EQ(converted_opt.info_log.get(), leveldb_opt.info_log);
  ASSERT_EQ(converted_opt.write_buffer_size, leveldb_opt.write_buffer_size);
  ASSERT_EQ(converted_opt.max_open_files, leveldb_opt.max_open_files);
  ASSERT_EQ(converted_opt.compression, leveldb_opt.compression);

  std::shared_ptr<TableFactory> tb_guard = converted_opt.table_factory;
  BlockBasedTableFactory* table_factory =
      dynamic_cast<BlockBasedTableFactory*>(converted_opt.table_factory.get());

  ASSERT_TRUE(table_factory != nullptr);

  const BlockBasedTableOptions table_opt = table_factory->table_options();

  ASSERT_EQ(table_opt.block_cache->GetCapacity(), 8UL << 20);
  ASSERT_EQ(table_opt.block_size, leveldb_opt.block_size);
  ASSERT_EQ(table_opt.block_restart_interval,
            leveldb_opt.block_restart_interval);
  ASSERT_EQ(table_opt.filter_policy.get(), leveldb_opt.filter_policy);
}

#ifndef ROCKSDB_LITE
class OptionsParserTest : public testing::Test {
 public:
  OptionsParserTest() { env_.reset(new test::StringEnv(Env::Default())); }

 protected:
  std::unique_ptr<test::StringEnv> env_;
};

TEST_F(OptionsParserTest, Comment) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[ DBOptions ]\n"
      "  # note that we don't support space around \"=\"\n"
      "  max_open_files=12345;\n"
      "  max_background_flushes=301  # comment after a statement is fine\n"
      "  # max_background_flushes=1000  # this line would be ignored\n"
      "  # max_background_compactions=2000 # so does this one\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "[CFOptions   \"default\"]  # column family must be specified\n"
      "                     # in the correct order\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_OK(parser.Parse(kTestFileName, env_.get()));

  ASSERT_OK(RocksDBOptionsParser::VerifyDBOptions(*parser.db_opt(), db_opt));
  ASSERT_EQ(parser.NumColumnFamilies(), 1U);
  ASSERT_OK(RocksDBOptionsParser::VerifyCFOptions(
      *parser.GetCFOptions("default"), cf_opt));
}

TEST_F(OptionsParserTest, ExtraSpace) {
  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[      Version   ]\n"
      "  rocksdb_version     = 3.14.0      \n"
      "  options_file_version=1   # some comment\n"
      "[DBOptions  ]  # some comment\n"
      "max_open_files=12345   \n"
      "    max_background_flushes   =    301   \n"
      " max_total_wal_size     =   1024  # keep_log_file_num=1000\n"
      "        [CFOptions      \"default\"     ]\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_OK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, MissingDBOptions) {
  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[CFOptions \"default\"]\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, DoubleDBOptions) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[DBOptions]\n"
      "  max_open_files=12345\n"
      "  max_background_flushes=301\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "[DBOptions]\n"
      "[CFOptions \"default\"]\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, NoDefaultCFOptions) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[DBOptions]\n"
      "  max_open_files=12345\n"
      "  max_background_flushes=301\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "[CFOptions \"something_else\"]\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, DefaultCFOptionsMustBeTheFirst) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[DBOptions]\n"
      "  max_open_files=12345\n"
      "  max_background_flushes=301\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "[CFOptions \"something_else\"]\n"
      "  # if a section is blank, we will use the default\n"
      "[CFOptions \"default\"]\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, DuplicateCFOptions) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[DBOptions]\n"
      "  max_open_files=12345\n"
      "  max_background_flushes=301\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "[CFOptions \"default\"]\n"
      "[CFOptions \"something_else\"]\n"
      "[CFOptions \"something_else\"]\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
}

TEST_F(OptionsParserTest, IgnoreUnknownOptions) {
  DBOptions db_opt;
  db_opt.max_open_files = 12345;
  db_opt.max_background_flushes = 301;
  db_opt.max_total_wal_size = 1024;
  ColumnFamilyOptions cf_opt;

  std::string options_file_content =
      "# This is a testing option string.\n"
      "# Currently we only support \"#\" styled comment.\n"
      "\n"
      "[Version]\n"
      "  rocksdb_version=3.14.0\n"
      "  options_file_version=1\n"
      "[DBOptions]\n"
      "  max_open_files=12345\n"
      "  max_background_flushes=301\n"
      "  max_total_wal_size=1024  # keep_log_file_num=1000\n"
      "  unknown_db_option1=321\n"
      "  unknown_db_option2=false\n"
      "[CFOptions \"default\"]\n"
      "  unknown_cf_option1=hello\n"
      "[CFOptions \"something_else\"]\n"
      "  unknown_cf_option2=world\n"
      "  # if a section is blank, we will use the default\n";

  const std::string kTestFileName = "test-rocksdb-options.ini";
  env_->WriteToNewFile(kTestFileName, options_file_content);
  RocksDBOptionsParser parser;
  ASSERT_NOK(parser.Parse(kTestFileName, env_.get()));
  ASSERT_OK(parser.Parse(kTestFileName, env_.get(),
                         /*e/*忽略_未知_选项*/）；
}

测试_f（OptionsPerTest，ParseVersion）
  数据库选项数据库选项；
  db_opt.max_open_files=12345；
  db_opt.max_background_flushes=301；
  db_opt.max_total_wal_size=1024；
  列系列选项cf_opt；

  std：：字符串文件_模板=
      “这是一个测试选项字符串。\n”
      “目前我们只支持\”“\”“样式的评论。\n”
      “\n”
      [版本] \n
      “rocksdb_版本=3.13.1\n”
      “选项\u文件\u版本=%s\n”
      “[dBoope] \n”
      “[cOptions\”默认值\“]\n”；
  常量int klength=1000；
  字符缓冲区[klength]；
  RocksDBOptions解析器解析器；

  const std:：vector<std:：string>无效的_versions=
      “A.B.C”，“3.2.2b”，“3.-12”，“3.1“，//只允许数字和点
      “1.2.3.4”
      “1.2.3”//最多只能包含一个点。
      “0”，//选项\文件\版本必须至少为一个
      “3…2”，
      “”，“.1.2”，//每个点之前必须至少有一个数字。
      “1.2.”，“1.”，“2.34.”//每个点后必须至少有一个数字
  对于（自动四：无效的_版本）
    snprintf（buffer，klength-1，file_template.c_str（），iv.c_str（））；

    语法分析器。
    env->writetonewfile（iv，缓冲区）；
    assert_nok（parser.parse（iv，env_u.get（））；
  }

  const std:：vector<std:：string>valid_versions=
      “1.232”、“100”、“3.12”、“1”、“12.3”、“1.25”；
  对于（自动VV：有效版本）
    snprintf（buffer，klength-1，file_template.c_str（），vv.c_str（））；
    语法分析器。
    env->writetonewfile（vv，buffer）；
    断言ou ok（parser.parse（vv，env_u.get（））；
  }
}

void verifycfpointertypedoptions（
    column系列选项*基本选项，const列系列选项*新选项，
    const std:：unordered_map<std:：string，std:：string>*new_cf_opt_map）
  std：：字符串名称_buffer；
  断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                  新的_-cf_-opt_-map））；

  //前后更改合并运算符的名称
  {
    auto*merge_operator=dynamic_cast<test:：changlingmergeoperator*>（）
        base_cf_opt->merge_operator.get（））；
    如果（合并操作员！= null pTr）{
      name_buffer=merge_operator->name（）；
      //更改名称并期望非OK状态
      merge_operator->setname（“其他名称”）；
      断言“否”（RocksDBOptionsParser:：VerifyCFOptions（
          *基础选项，*新选项，新选项地图）；
      //将名称更改回原来的状态，并期望OK状态
      merge_operator->setname（name_buffer）；
      断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                      新的_-cf_-opt_-map））；
    }
  }

  //前后更改压缩过滤器工厂的名称
  {
    自动压实过滤器工厂=
        动态铸模<测试：：ChanglingCompactionFilterFactory/>
            base_cf_opt->compaction_filter_factory.get（））；
    如果（压缩过滤器工厂！= null pTr）{
      name_buffer=compaction_filter_factory->name（）；
      //更改名称并期望非OK状态
      compaction_filter_factory->setname（“其他名称”）；
      断言“否”（RocksDBOptionsParser:：VerifyCFOptions（
          *基础选项，*新选项，新选项地图）；
      //将名称更改回原来的状态，并期望OK状态
      compaction_filter_factory->setname（name_buffer）；
      断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                      新的_-cf_-opt_-map））；
    }
  }

  //通过将compaction_filter设置为nullptr进行测试
  {
    auto*tmp_compaction_filter=base_cf_opt->compaction_filter；
    如果（tmp_compaction_filter！= null pTr）{
      基_cf_opt->compaction_filter=nullptr；
      //将compaction_filter设置为nullptr并期望非OK状态
      断言“否”（RocksDBOptionsParser:：VerifyCFOptions（
          *基础选项，*新选项，新选项地图）；
      //将值设置回“Expect OK”状态
      基_cf_opt->compaction_filter=tmp_compaction_filter；
      断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                      新的_-cf_-opt_-map））；
    }
  }

  //通过将表工厂设置为nullptr进行测试
  {
    auto tmp_table_factory=base_cf_opt->table_factory；
    如果（TMP表格工厂！= null pTr）{
      base_cf_opt->table_factory.reset（）；
      //将表工厂设置为nullptr并期望非OK状态
      断言“否”（RocksDBOptionsParser:：VerifyCFOptions（
          *基础选项，*新选项，新选项地图）；
      //将值设置回“Expect OK”状态
      base_cf_opt->table_factory=tmp_table_factory；
      断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                      新的_-cf_-opt_-map））；
    }
  }

  //通过将memtable_factory设置为nullptr进行测试
  {
    auto tmp_memtable_factory=base_cf_opt->memtable_factory；
    如果（tmp-memtable-factory！= null pTr）{
      base_cf_opt->memtable_factory.reset（）；
      //将memtable_factory设置为nullptr并期望非OK状态
      断言“否”（RocksDBOptionsParser:：VerifyCFOptions（
          *基础选项，*新选项，新选项地图）；
      //将值设置回“Expect OK”状态
      base_cf_opt->memtable_factory=tmp_memtable_factory；
      断言“确定”（RocksDBoption Parser:：VerifycOptions（*基本选项，*新选项，*
                                                      新的_-cf_-opt_-map））；
    }
  }
}

测试_f（选项sparsertest、dumpandparse）
  DBOPTIONS基本选项
  std:：vector<columnFamilyOptions>base_cf_opts；
  std：：vector<std：：string>cf name=“default”，“cf1”，“cf2”，“cf3”，
                                       C:F:4:4:4
                                       “P\\I\\K\\A\\CHU\\
                                       “Rocksdb 1-测试cf 2”
  const int num_cf=static_cast<int>（cf_names.size（））；
  随机RND（302）；
  测试：：随机化选项（&base_db_opt，&rnd）；
  Base_-db_-opt.db_-log_-dir+=“/奇数但可能会发生路径/\ \ \ \ \ omg”；

  BlockBasedTableOptions专用_btto；
  特殊缓存索引块和过滤块=真；
  特殊块大小=999999；

  对于（int c=0；c<num_cf；+c）
    列系列选项cf_opt；
    随机对照（0XFB+C）；
    测试：：随机化选项（&cf_opt，&cf_rnd）；
    如果（c＜4）{
      cfou opt.prefixou提取器.reset（test:：randomslicetransform（&rnd，c））；
    }
    如果（c＜3）{
      cf_opt.table_factory.reset（测试：：randomtablefactory（&rnd，c））；
    否则，如果（c==4）
      cf_opt.table_factory.reset（newblockbasedTableFactory（special_bbto））；
    }
    基地选择。重新安置（选择）；
  }

  const std:：string koptionsFileName=“测试持久化选项.ini”；
  断言“确定”（PersisterRocksDBOptions（Base_-DB_-Opt，CF_-Names，Base_-CF_-Opts，
                                  koptionsFileName，env_.get（））；

  RocksDBOptions解析器解析器；
  断言ou ok（parser.parse（koptionsFileName，env_u.get（））；

  //确保正确反序列化了基于块的表工厂选项
  std:：shared_ptr<table factory>ttf=（*parser.cf_opts（））[4].表_factory；
  断言eq（blockBasedTableFactory:：kname，std:：string（ttf->name（））；
  const blockbasedTableOptions&parsed_bto=
      static_cast<blockbasedTableFactory*>（ttf.get（））->table_options（）；
  断言eq（特殊块大小，解析块大小）；
  断言eq（特殊缓存索引块和过滤器块，
            解析的缓存索引块和过滤块）；

  断言“确定”（RocksDBOptionsParser:：VerifyRocksDBOptionsFromFile（
      base_db_opt，cf_name，base_cf_opts，koptionsFileName，env_u.get（））；

  断言
      rocksdboptionsParser:：verifyDBOptions（*parser.db_opt（），base_db_opt））；
  对于（int c=0；c<num_cf；+c）
    const auto*cf_opt=parser.getcfOptions（cf_name[c]）；
    断言“ne”（cf_opt，nullptr）；
    断言“确定”（RocksDBOptionsParser:：VerifyCFOptions（
        base_cf_opts[c]，*cf_opt，&（parser.cf_opt_maps（）->at（c）））；
  }

  //进一步验证指针类型的选项
  对于（int c=0；c<num_cf；+c）
    const auto*cf_opt=parser.getcfOptions（cf_name[c]）；
    断言“ne”（cf_opt，nullptr）；
    验证cfpointertypedoptions（&base_cf_opts[c]，cf_opt，
                                &（parser.cf_opt_maps（）->at（c））；
  }

  断言eq（parser.getcfoptions（“不存在”），nullptr）；

  base_db_opt.max_open_files++；
  断言_k（rocksdboptionsParser:：verifyRocksdboptionsFromFile（
      base_db_opt，cf_name，base_cf_opts，koptionsFileName，env_u.get（））；

  对于（int c=0；c<num_cf；+c）
    if（基础过滤器）
      删除基础选项[c].压缩过滤器；
    }
  }
}

测试_f（OptionsBarest，DifferentDefault）
  const std:：string koptionsFileName=“测试持久化选项.ini”；

  列系列选项“级别”选项；
  cf_level_opts.optimizelStyleCompaction（）；

  列家庭选项；
  cfou univou opts.optimizeUniversalStyleCompaction（）；

  断言_OK（persisterRocksDBOptions（dbOptions（），“default”，“universal”，
                                  _cf_u级opts，cf_u大学opts，
                                  koptionsFileName，env_.get（））；

  RocksDBOptions解析器解析器；
  断言ou ok（parser.parse（koptionsFileName，env_u.get（））；

  {
    旧选项默认选项；
    old_default_opts.olddefaults（）；
    断言_eq（10*1048576，旧的_-default_-opts.max_-bytes_用于_-level_-base）；
    断言eq（5000，旧的opts.max打开文件）；
    断言eq（2*1024u*1024u，旧的默认值opts.delayed写入速率）；
    断言eq（walrecoverymode:：ktoleratecorruptedTailRecords，
              旧的“默认”模式；
  }
  {
    旧选项默认选项；
    old_default_opts.olddefaults（4，6）；
    断言_eq（10*1048576，旧的_-default_-opts.max_-bytes_用于_-level_-base）；
    断言eq（5000，旧的opts.max打开文件）；
  }
  {
    旧选项默认选项；
    old_default_opts.olddefaults（4，7）；
    断言“ne”（10*1048576，旧的“默认值”opts.max“字节”表示“级别”库）；
    断言“ne”（4，旧的“default”opts.table“cache”numshardbits）；
    断言eq（5000，旧的opts.max打开文件）；
    断言eq（2*1024u*1024u，旧的默认值opts.delayed写入速率）；
  }
  {
    columnfamilyOptions旧的默认选项；
    old_default_cf_opts.olddefaults（）；
    断言eq（2*1048576，old_-default_-cf_-opts.target_-file_-size_-base）；
    断言eq（4<<20，old_-default_-cf_-opts.write_-buffer_-size）；
    断言eq（2*1048576，old_-default_-cf_-opts.target_-file_-size_-base）；
    断言_Eq（0，旧的_-default_-cf-opts.soft_-pending_-compaction_-bytes_-limit）；
    断言“eq”（0，旧的“default”选项。硬的“pending”压缩“bytes”限制）；
    断言eq（compactionpri:：kbycompensedSize，
              旧的_-default_-cf_-opts.compaction_-pri）；
  }
  {
    columnfamilyOptions旧的默认选项；
    “旧默认值”选项。旧默认值（4，6）；
    断言eq（2*1048576，old_-default_-cf_-opts.target_-file_-size_-base）；
    断言eq（compactionpri:：kbycompensedSize，
              旧的_-default_-cf_-opts.compaction_-pri）；
  }
  {
    columnfamilyOptions旧的默认选项；
    “旧默认值”选项。旧默认值（4，7）；
    断言“ne”（2*1048576，旧的“默认值”opts.target“文件大小”base）；
    断言eq（compactionpri:：kbycompensedSize，
              旧的_-default_-cf_-opts.compaction_-pri）；
  }
  {
    旧选项默认选项；
    old_default_opts.olddefaults（5，1）；
    断言eq（2*1024u*1024u，旧的默认值opts.delayed写入速率）；
  }
  {
    旧选项默认选项；
    old_default_opts.olddefaults（5，2）；
    断言eq（16*1024u*1024u，旧的默认值opts.delayed写入速率）；
  }

  小选择；
  small_opts.optimizeformaldb（）；
  断言eq（2<<20，small-opts.write-buffer-size）；
  断言eq（5000，small-opts.max-open-files）；
}

class optionssanitychecktest:公共选项sparsertest
 公众：
  选项sanitychecktest（）

 受保护的：
  状态sanitycheckcfOptions（const columnFamilyOptions和cf_Opts，
                              选项snitychecklevel level）
    返回rocksdboptionsParser:：verifyRocksdboptionsFromFile（）。
        dboptions（），“默认”，cf_opts，koptionsFileName，env_u.get（），
        水平）；
  }

  状态持久选项（const列系列选项&cf_opts）
    状态s=env->deletefile（koptionsFileName）；
    如果（！）S.O.（））{
      返回S；
    }
    返回PersisterRocksDBOptions（dbOptions（），“默认”，cf_opts，
                                 koptionsFileName，env_.get（））；
  }

  const std:：string koptionsFileName=“选项”；
}；

测试_f（选项sanitycheck test，sanitycheck）
  列系列选项选项；
  随机RND（301）；

  //默认列FamilyOptions
  {
    断言“确定”（PersistecOptions（opts））；
    断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；
  }

  //前缀提取程序
  {
    //可以将前缀提取程序表单nullptr更改为非nullptr
    断言_eq（opts.prefix_extractor.get（），nullptr）；
    opts.prefix提取程序.reset（newcappedprifixtransform（10））；
    断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
    断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

    //坚持更改
    断言“确定”（PersistecOptions（opts））；
    断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；

    //使用相同的前缀提取程序，但参数不同
    opts.prefix提取程序.reset（newcappedprifixtransform（15））；
    //expect pass only in ksanitylevelloose兼容
    断言“否”（sanitycheckcfoptions（opts，ksanitylevelexactmatch））；
    断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
    断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

    //使用FixedPrefixTransform重复测试
    opts.prefix提取程序.reset（newfixedprifixtransform（10））；
    断言“否”（sanitycheckcfoptions（opts，ksanitylevelexactmatch））；
    断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
    断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

    //保留前缀提取程序的更改
    断言“确定”（PersistecOptions（opts））；
    断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；

    //使用相同的前缀提取程序，但参数不同
    opts.prefix提取程序.reset（newfixedprifixtransform（15））；
    //expect pass only in ksanitylevelloose兼容
    断言“否”（sanitycheckcfoptions（opts，ksanitylevelexactmatch））；
    断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
    断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

    //将前缀提取程序从非nullptr更改为nullptr
    opts.prefix_extractor.reset（）；
    //期望通过，因为更改前缀提取程序是安全的
    //从非空到空
    断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
    断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；
  }
  //坚持更改
  断言“确定”（PersistecOptions（opts））；
  断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；

  //表工厂
  {
    对于（int tb=0；tb<=2；++tb）
      //更改表工厂
      opts.table_factory.reset（测试：：randomtablefactory（&rnd，tb））；
      断言_k（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
      断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

      //坚持更改
      断言“确定”（PersistecOptions（opts））；
      断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；
    }
  }

  //合并运算符
  {
    对于（int test=0；test<5；++test）
      //更改合并运算符
      opts.merge_operator.reset（测试：：randomergeoperator（&rnd））；
      断言_k（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；
      断言“确定”（sanitycheckcfoptions（opts，ksanitylevelnone））；

      //坚持更改
      断言“确定”（PersistecOptions（opts））；
      断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；
    }
  }

  //压缩过滤器
  {
    对于（int test=0；test<5；++test）
      //更改压缩过滤器
      opts.compaction_filter=test:：randomccompactionfilter（&rnd）；
      断言“否”（sanitycheckcfoptions（opts，ksanitylevelexactmatch））；
      断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；

      //坚持更改
      断言“确定”（PersistecOptions（opts））；
      断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；
      删除opts.compaction_filter；
      opts.compaction_filter=nullptr；
    }
  }

  //压缩过滤器工厂
  {
    对于（int test=0；test<5；++test）
      //更改压缩过滤器工厂
      opts.compaction_filter_factory.reset（
          测试：：RandomCompactionFilterFactory（&rnd））；
      断言“否”（sanitycheckcfoptions（opts，ksanitylevelexactmatch））；
      断言“正常”（sanitycheckcfoptions（opts，ksanitylevellooselycompatible））；

      //坚持更改
      断言“确定”（PersistecOptions（opts））；
      断言“确定”（sanitycheckcOptions（opts，ksanitylevelexactmatch））；
    }
  }
}

命名空间{
bool isescapedString（const std:：string&str）
  对于（size_t i=0；i<str.size（）；++i）
    if（str[i]='\\'）
      //因为我们已经在
      //下一个if-then分支，结尾出现任何'\'
      //在这种情况下，转义字符串的无效。
      如果（i==str.size（）-1）
        返回错误；
      }
      if（str[i+1]='\\'）
        //如果有两个连续的'\'s，则跳过第二个'\'s。
        I++；
        继续；
      }
      开关（str[i+1]）；
        案例：：
        案例“：”
        案例“α”：
          继续；
        违约：
          //如果为真，“与str[i+1]一起使用”不是有效的转义。
          if（unescapechar（str[i+1]）==str[i+1]）；
            返回错误；
          }
      }
    否则，如果（IsSpecialChar（str[i]）&&（i==0 str[i-1]！=“\\”）{
      返回错误；
    }
  }
  回归真实；
}
} / /命名空间

测试F（OptionsPerTest，EscapeOptionString）
  断言eq（unescapeoptionString（
                “这是一个测试字符串，包含\ \ \ \ \：和\ \ \转义字符。”），
            “这是一个带有：和\\转义字符的测试字符串。”）；

  断言
      escapeOptionString（“这是一个带有：和\\ escape字符的测试字符串。”），
      “这是一个带有\ \ \35; \：和\ \ \转义字符的测试字符串。”）；

  std：：字符串可读字符=
      “像这样的字符串\”1234567890-=（*&^%$@！）尔图伊奥普[]波尤”
      “Ytrewqasdfghjkl；'：lkjhgfdsazxcvbnm，”>
      “<mnbvcxz\\”应该可以\ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \ \
      “被序列化和反序列化”；

  std:：string转义的_string=escapeOptionString（可读的_字符）；
  断言“真”（ISescapedString（转义的“字符串”）；
  //应取消这两个转换，并应输出
  //原始输入。
  断言eq（unescapeoptionString（转义的_-string），可读的_-chars）；

  std：：字符串所有字符；
  for（无符号字符c=0；；++c）
    阿尔沙克斯+C＝C；
    如果（c＝255）{
      断裂；
    }
  }
  转义的_string=escapeOptionString（所有_字符）；
  断言“真”（ISescapedString（转义的“字符串”）；
  断言eq（unescapeoptionString（转义的字符串），所有字符）；

  断言eq（rocksdboptionParser:：TrimAnderMoveComment（
                “带有注释的简单语句。像这样：），
            “带有注释的简单语句。”）；

  断言eq（rocksdboptionParser:：TrimAnderMoveComment（
                “一起转义和评论。”），
            “逃离\ \ \和”）；
}
我很喜欢你！摇滚乐
//命名空间rocksdb

int main（int argc，char**argv）
  ：：测试：：initgoogletest（&argc，argv）；
γIFIFF GFLAGS
  parseCommandLineFlags（&argc，&argv，true）；
endif//g标志
  返回run_all_tests（）；
}
