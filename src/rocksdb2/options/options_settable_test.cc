
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

#include <cstring>

#include "options/options_parser.h"
#include "rocksdb/convenience.h"
#include "util/testharness.h"

#ifndef GFLAGS
bool FLAGS_enable_print = false;
#else
#include <gflags/gflags.h>
using GFLAGS::ParseCommandLineFlags;
DEFINE_bool(enable_print, false, "Print options generated to console.");
#endif  //GFLAGS

namespace rocksdb {

//验证可从选项字符串设置选项。
//我们采用的方法依赖于复制构造函数的编译器行为
//不会碰到隐式填充字节，所以测试是脆弱的。
//因此，我们只运行测试来验证选项中的新字段
//可通过字符串在有限平台上设置，因为它取决于
//编译器。
#ifndef ROCKSDB_LITE
#if defined OS_LINUX || defined OS_WIN
#ifndef __clang__

class OptionsSettableTest : public testing::Test {
 public:
  OptionsSettableTest() {}
};

const char kSpecialChar = 'z';
typedef std::vector<std::pair<size_t, size_t>> OffsetGap;

void FillWithSpecialChar(char* start_ptr, size_t total_size,
                         const OffsetGap& blacklist) {
  size_t offset = 0;
  for (auto& pair : blacklist) {
    std::memset(start_ptr + offset, kSpecialChar, pair.first - offset);
    offset = pair.first + pair.second;
  }
  std::memset(start_ptr + offset, kSpecialChar, total_size - offset);
}

int NumUnsetBytes(char* start_ptr, size_t total_size,
                  const OffsetGap& blacklist) {
  int total_unset_bytes_base = 0;
  size_t offset = 0;
  for (auto& pair : blacklist) {
    for (char* ptr = start_ptr + offset; ptr < start_ptr + pair.first; ptr++) {
      if (*ptr == kSpecialChar) {
        total_unset_bytes_base++;
      }
    }
    offset = pair.first + pair.second;
  }
  for (char* ptr = start_ptr + offset; ptr < start_ptr + total_size; ptr++) {
    if (*ptr == kSpecialChar) {
      total_unset_bytes_base++;
    }
  }
  return total_unset_bytes_base;
}

//如果测试失败，可能会向BlockBasedTableOptions添加一个新选项。
//但不能通过getBlockBasedTableOptionsFromsString（）或
//测试没有相应更新。
//添加选项后，我们需要确保可通过
//getBlockBasedTableOptionsFromString（）并将该选项添加到输入字符串
//传递给此测试中的GetBlockBasedTableOptionsFromsString（）。
//如果是复杂类型，还需要将字段添加到
//KBtbtOblogList，并可能为其添加自定义验证。
TEST_F(OptionsSettableTest, BlockBasedTableOptionsAllFieldsSettable) {
//项目的形式为<offset，size>。需要按升序排列
//不重叠。如果添加了新的指针选项，则需要更新。
  const OffsetGap kBbtoBlacklist = {
      {offsetof(struct BlockBasedTableOptions, flush_block_policy_factory),
       sizeof(std::shared_ptr<FlushBlockPolicyFactory>)},
      {offsetof(struct BlockBasedTableOptions, block_cache),
       sizeof(std::shared_ptr<Cache>)},
      {offsetof(struct BlockBasedTableOptions, persistent_cache),
       sizeof(std::shared_ptr<PersistentCache>)},
      {offsetof(struct BlockBasedTableOptions, block_cache_compressed),
       sizeof(std::shared_ptr<Cache>)},
      {offsetof(struct BlockBasedTableOptions, filter_policy),
       sizeof(std::shared_ptr<const FilterPolicy>)},
  };

//在这个测试中，我们捕获一个新的BlockBasedTableOptions选项，它不是
//可通过getBlockBasedTableOptionsFromsString（）设置。
//我们计算选项结构的填充字节数，并断言它是相同的
//作为由初始化的选项结构的未设置字节
//GetBlockBasedTableOptionsFromString（）。

  char* bbto_ptr = new char[sizeof(BlockBasedTableOptions)];

//通过将内存中的所有字节设置为特殊字符来计算填充字节数，
//将构造良好的结构复制到此内存，并查看有多少个特殊的
//字节保留。
  BlockBasedTableOptions* bbto = new (bbto_ptr) BlockBasedTableOptions();
  FillWithSpecialChar(bbto_ptr, sizeof(BlockBasedTableOptions), kBbtoBlacklist);
//根据编译器的行为，填充字节不会更改
//复制结构时。当编译器行为时，它很容易失败
//变化。我们验证是否有未设置的字节来检测情况。
  *bbto = BlockBasedTableOptions();
  int unset_bytes_base =
      NumUnsetBytes(bbto_ptr, sizeof(BlockBasedTableOptions), kBbtoBlacklist);
  ASSERT_GT(unset_bytes_base, 0);
  bbto->~BlockBasedTableOptions();

//构造传递给
//GetBlockBasedTableOptionsFromString（）。
  bbto = new (bbto_ptr) BlockBasedTableOptions();
  FillWithSpecialChar(bbto_ptr, sizeof(BlockBasedTableOptions), kBbtoBlacklist);
//此选项不可设置：
  bbto->use_delta_encoding = true;

  char* new_bbto_ptr = new char[sizeof(BlockBasedTableOptions)];
  BlockBasedTableOptions* new_bbto =
      new (new_bbto_ptr) BlockBasedTableOptions();
  FillWithSpecialChar(new_bbto_ptr, sizeof(BlockBasedTableOptions),
                      kBbtoBlacklist);

//如果添加了新选项，则需要更新选项字符串。
  ASSERT_OK(GetBlockBasedTableOptionsFromString(
      *bbto,
      "cache_index_and_filter_blocks=1;"
      "cache_index_and_filter_blocks_with_high_priority=true;"
      "pin_l0_filter_and_index_blocks_in_cache=1;"
      "index_type=kHashSearch;"
      "checksum=kxxHash;hash_index_allow_collision=1;no_block_cache=1;"
      "block_cache=1M;block_cache_compressed=1k;block_size=1024;"
      "block_size_deviation=8;block_restart_interval=4; "
      "metadata_block_size=1024;"
      "partition_filters=false;"
      "index_block_restart_interval=4;"
      "filter_policy=bloomfilter:4:true;whole_key_filtering=1;"
      "format_version=1;"
      "hash_index_allow_collision=false;"
      "verify_compression=true;read_amp_bytes_per_bit=0",
      new_bbto));

  ASSERT_EQ(unset_bytes_base,
            NumUnsetBytes(new_bbto_ptr, sizeof(BlockBasedTableOptions),
                          kBbtoBlacklist));

  ASSERT_TRUE(new_bbto->block_cache.get() != nullptr);
  ASSERT_TRUE(new_bbto->block_cache_compressed.get() != nullptr);
  ASSERT_TRUE(new_bbto->filter_policy.get() != nullptr);

  bbto->~BlockBasedTableOptions();
  new_bbto->~BlockBasedTableOptions();

  delete[] bbto_ptr;
  delete[] new_bbto_ptr;
}

//如果测试失败，可能会向dboptions添加一个新选项。
//但不能通过getDBOptionsFromsString（）设置，或者测试不是
//相应更新。
//添加选项后，我们需要确保可通过
//getDBOptionsFromsString（）并将该选项添加到传递给
//此测试中的dboptionsFromstring（）。
//如果是复杂类型，还需要将字段添加到
//kdbopitionsblacklist，并可能为其添加自定义验证。
TEST_F(OptionsSettableTest, DBOptionsAllFieldsSettable) {
  const OffsetGap kDBOptionsBlacklist = {
      {offsetof(struct DBOptions, env), sizeof(Env*)},
      {offsetof(struct DBOptions, rate_limiter),
       sizeof(std::shared_ptr<RateLimiter>)},
      {offsetof(struct DBOptions, sst_file_manager),
       sizeof(std::shared_ptr<SstFileManager>)},
      {offsetof(struct DBOptions, info_log), sizeof(std::shared_ptr<Logger>)},
      {offsetof(struct DBOptions, statistics),
       sizeof(std::shared_ptr<Statistics>)},
      {offsetof(struct DBOptions, db_paths), sizeof(std::vector<DbPath>)},
      {offsetof(struct DBOptions, db_log_dir), sizeof(std::string)},
      {offsetof(struct DBOptions, wal_dir), sizeof(std::string)},
      {offsetof(struct DBOptions, write_buffer_manager),
       sizeof(std::shared_ptr<WriteBufferManager>)},
      {offsetof(struct DBOptions, listeners),
       sizeof(std::vector<std::shared_ptr<EventListener>>)},
      {offsetof(struct DBOptions, row_cache), sizeof(std::shared_ptr<Cache>)},
      {offsetof(struct DBOptions, wal_filter), sizeof(const WalFilter*)},
  };

  char* options_ptr = new char[sizeof(DBOptions)];

//通过将内存中的所有字节设置为特殊字符来计算填充字节数，
//将构造良好的结构复制到此内存，并查看有多少个特殊的
//字节保留。
  DBOptions* options = new (options_ptr) DBOptions();
  FillWithSpecialChar(options_ptr, sizeof(DBOptions), kDBOptionsBlacklist);
//根据编译器的行为，填充字节不会更改
//复制结构时。当编译器行为时，它很容易失败
//变化。我们验证是否有未设置的字节来检测情况。
  *options = DBOptions();
  int unset_bytes_base =
      NumUnsetBytes(options_ptr, sizeof(DBOptions), kDBOptionsBlacklist);
  ASSERT_GT(unset_bytes_base, 0);
  options->~DBOptions();

  options = new (options_ptr) DBOptions();
  FillWithSpecialChar(options_ptr, sizeof(DBOptions), kDBOptionsBlacklist);

  char* new_options_ptr = new char[sizeof(DBOptions)];
  DBOptions* new_options = new (new_options_ptr) DBOptions();
  FillWithSpecialChar(new_options_ptr, sizeof(DBOptions), kDBOptionsBlacklist);

//如果添加了新选项，则需要更新选项字符串。
  ASSERT_OK(
      GetDBOptionsFromString(*options,
                             "wal_bytes_per_sync=4295048118;"
                             "delete_obsolete_files_period_micros=4294967758;"
                             "WAL_ttl_seconds=4295008036;"
                             "WAL_size_limit_MB=4295036161;"
                             "wal_dir=path/to/wal_dir;"
                             "db_write_buffer_size=2587;"
                             "max_subcompactions=64330;"
                             "table_cache_numshardbits=28;"
                             "max_open_files=72;"
                             "max_file_opening_threads=35;"
                             "max_background_jobs=8;"
                             "base_background_compactions=3;"
                             "max_background_compactions=33;"
                             "use_fsync=true;"
                             "use_adaptive_mutex=false;"
                             "max_total_wal_size=4295005604;"
                             "compaction_readahead_size=0;"
                             "new_table_reader_for_compaction_inputs=false;"
                             "keep_log_file_num=4890;"
                             "skip_stats_update_on_db_open=false;"
                             "max_manifest_file_size=4295009941;"
                             "db_log_dir=path/to/db_log_dir;"
                             "skip_log_error_on_recovery=true;"
                             "writable_file_max_buffer_size=1048576;"
                             "paranoid_checks=true;"
                             "is_fd_close_on_exec=false;"
                             "bytes_per_sync=4295013613;"
                             "enable_thread_tracking=false;"
                             "recycle_log_file_num=0;"
                             "create_missing_column_families=true;"
                             "log_file_time_to_roll=3097;"
                             "max_background_flushes=35;"
                             "create_if_missing=false;"
                             "error_if_exists=true;"
                             "delayed_write_rate=4294976214;"
                             "manifest_preallocation_size=1222;"
                             "allow_mmap_writes=false;"
                             "stats_dump_period_sec=70127;"
                             "allow_fallocate=true;"
                             "allow_mmap_reads=false;"
                             "use_direct_reads=false;"
                             "use_direct_io_for_flush_and_compaction=false;"
                             "max_log_file_size=4607;"
                             "random_access_max_buffer_size=1048576;"
                             "advise_random_on_open=true;"
                             "fail_if_options_file_error=false;"
                             "enable_pipelined_write=false;"
                             "allow_concurrent_memtable_write=true;"
                             "wal_recovery_mode=kPointInTimeRecovery;"
                             "enable_write_thread_adaptive_yield=true;"
                             "write_thread_slow_yield_usec=5;"
                             "write_thread_max_yield_usec=1000;"
                             "access_hint_on_compaction_start=NONE;"
                             "info_log_level=DEBUG_LEVEL;"
                             "dump_malloc_stats=false;"
                             "allow_2pc=false;"
                             "avoid_flush_during_recovery=false;"
                             "avoid_flush_during_shutdown=false;"
                             "allow_ingest_behind=false;"
                             "concurrent_prepare=false;"
                             "manual_wal_flush=false;",
                             new_options));

  ASSERT_EQ(unset_bytes_base, NumUnsetBytes(new_options_ptr, sizeof(DBOptions),
                                            kDBOptionsBlacklist));

  options->~DBOptions();
  new_options->~DBOptions();

  delete[] options_ptr;
  delete[] new_options_ptr;
}

//如果测试失败，可能会向ColumnFamilyOptions添加一个新选项。
//但不能通过getColumnFamilyOptionsFromString（）或
//测试没有相应更新。
//添加选项后，我们需要确保可通过
//getColumnFamilyOptionsFromString（）并将选项添加到输入中
//在此测试中传递给getColumnFamilyOptionsFromString（）的字符串。
//如果是复杂类型，还需要将字段添加到
//kColumnFamilyOptions黑名单，并可能添加自定义验证
//为了它。
TEST_F(OptionsSettableTest, ColumnFamilyOptionsAllFieldsSettable) {
//黑名单中的选项需要按与中相同的顺序显示
//列系列选项。
  const OffsetGap kColumnFamilyOptionsBlacklist = {
      {offset_of(&ColumnFamilyOptions::inplace_callback),
       sizeof(UpdateStatus(*)(char*, uint32_t*, Slice, std::string*))},
      {offset_of(
           &ColumnFamilyOptions::memtable_insert_with_hint_prefix_extractor),
       sizeof(std::shared_ptr<const SliceTransform>)},
      {offset_of(&ColumnFamilyOptions::compression_per_level),
       sizeof(std::vector<CompressionType>)},
      {offset_of(
           &ColumnFamilyOptions::max_bytes_for_level_multiplier_additional),
       sizeof(std::vector<int>)},
      {offset_of(&ColumnFamilyOptions::memtable_factory),
       sizeof(std::shared_ptr<MemTableRepFactory>)},
      {offset_of(&ColumnFamilyOptions::table_properties_collector_factories),
       sizeof(ColumnFamilyOptions::TablePropertiesCollectorFactories)},
      {offset_of(&ColumnFamilyOptions::comparator), sizeof(Comparator*)},
      {offset_of(&ColumnFamilyOptions::merge_operator),
       sizeof(std::shared_ptr<MergeOperator>)},
      {offset_of(&ColumnFamilyOptions::compaction_filter),
       sizeof(const CompactionFilter*)},
      {offset_of(&ColumnFamilyOptions::compaction_filter_factory),
       sizeof(std::shared_ptr<CompactionFilterFactory>)},
      {offset_of(&ColumnFamilyOptions::prefix_extractor),
       sizeof(std::shared_ptr<const SliceTransform>)},
      {offset_of(&ColumnFamilyOptions::table_factory),
       sizeof(std::shared_ptr<TableFactory>)},
  };

  char* options_ptr = new char[sizeof(ColumnFamilyOptions)];

//通过将内存中的所有字节设置为特殊字符来计算填充字节数，
//将构造良好的结构复制到此内存，并查看有多少个特殊的
//字节保留。
  ColumnFamilyOptions* options = new (options_ptr) ColumnFamilyOptions();
  FillWithSpecialChar(options_ptr, sizeof(ColumnFamilyOptions),
                      kColumnFamilyOptionsBlacklist);
//根据编译器的行为，填充字节不会更改
//复制结构时。当编译器行为时，它很容易失败
//变化。我们验证是否有未设置的字节来检测情况。
  *options = ColumnFamilyOptions();

//未初始化的deprecatt选项。需要设置以避免
//Valgrind误差
  options->max_mem_compaction_level = 0;

  int unset_bytes_base = NumUnsetBytes(options_ptr, sizeof(ColumnFamilyOptions),
                                       kColumnFamilyOptionsBlacklist);
  ASSERT_GT(unset_bytes_base, 0);
  options->~ColumnFamilyOptions();

  options = new (options_ptr) ColumnFamilyOptions();
  FillWithSpecialChar(options_ptr, sizeof(ColumnFamilyOptions),
                      kColumnFamilyOptionsBlacklist);

//以下选项无法通过设置
//getColumnFamilyOptions从字符串（）开始：
  options->rate_limit_delay_max_milliseconds = 33;
  options->compaction_options_universal = CompactionOptionsUniversal();
  options->compression_opts = CompressionOptions();
  options->hard_rate_limit = 0;
  options->soft_rate_limit = 0;
  options->compaction_options_fifo = CompactionOptionsFIFO();
  options->max_mem_compaction_level = 0;

  char* new_options_ptr = new char[sizeof(ColumnFamilyOptions)];
  ColumnFamilyOptions* new_options =
      new (new_options_ptr) ColumnFamilyOptions();
  FillWithSpecialChar(new_options_ptr, sizeof(ColumnFamilyOptions),
                      kColumnFamilyOptionsBlacklist);

//如果添加了新选项，则需要更新选项字符串。
  ASSERT_OK(GetColumnFamilyOptionsFromString(
      *options,
      "compaction_filter_factory=mpudlojcujCompactionFilterFactory;"
      "table_factory=PlainTable;"
      "prefix_extractor=rocksdb.CappedPrefix.13;"
      "comparator=leveldb.BytewiseComparator;"
      "compression_per_level=kBZip2Compression:kBZip2Compression:"
      "kBZip2Compression:kNoCompression:kZlibCompression:kBZip2Compression:"
      "kSnappyCompression;"
      "max_bytes_for_level_base=986;"
      "bloom_locality=8016;"
      "target_file_size_base=4294976376;"
      "memtable_huge_page_size=2557;"
      "max_successive_merges=5497;"
      "max_sequential_skip_in_iterations=4294971408;"
      "arena_block_size=1893;"
      "target_file_size_multiplier=35;"
      "min_write_buffer_number_to_merge=9;"
      "max_write_buffer_number=84;"
      "write_buffer_size=1653;"
      "max_compaction_bytes=64;"
      "max_bytes_for_level_multiplier=60;"
      "memtable_factory=SkipListFactory;"
      "compression=kNoCompression;"
      "bottommost_compression=kDisableCompressionOption;"
      "level0_stop_writes_trigger=33;"
      "num_levels=99;"
      "level0_slowdown_writes_trigger=22;"
      "level0_file_num_compaction_trigger=14;"
      "compaction_filter=urxcqstuwnCompactionFilter;"
      "soft_rate_limit=530.615385;"
      "soft_pending_compaction_bytes_limit=0;"
      "max_write_buffer_number_to_maintain=84;"
      "merge_operator=aabcxehazrMergeOperator;"
      "memtable_prefix_bloom_size_ratio=0.4642;"
      "memtable_insert_with_hint_prefix_extractor=rocksdb.CappedPrefix.13;"
      "paranoid_file_checks=true;"
      "force_consistency_checks=true;"
      "inplace_update_num_locks=7429;"
      "optimize_filters_for_hits=false;"
      "level_compaction_dynamic_level_bytes=false;"
      "inplace_update_support=false;"
      "compaction_style=kCompactionStyleFIFO;"
      "compaction_pri=kMinOverlappingRatio;"
      "purge_redundant_kvs_while_flush=true;"
      "hard_pending_compaction_bytes_limit=0;"
      "disable_auto_compactions=false;"
      "report_bg_io_stats=true;",
      new_options));

  ASSERT_EQ(unset_bytes_base,
            NumUnsetBytes(new_options_ptr, sizeof(ColumnFamilyOptions),
                          kColumnFamilyOptionsBlacklist));

  options->~ColumnFamilyOptions();
  new_options->~ColumnFamilyOptions();

  delete[] options_ptr;
  delete[] new_options_ptr;
}
#endif  //！阿克克兰格
#endif  //操作系统Linux
#endif  //！摇滚乐

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
#ifdef GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif  //GFLAGS
  return RUN_ALL_TESTS();
}
