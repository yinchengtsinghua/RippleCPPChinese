
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

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "options/cf_options.h"
#include "options/db_options.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"

namespace rocksdb {

DBOptions BuildDBOptions(const ImmutableDBOptions& immutable_db_options,
                         const MutableDBOptions& mutable_db_options);

ColumnFamilyOptions BuildColumnFamilyOptions(
    const ColumnFamilyOptions& ioptions,
    const MutableCFOptions& mutable_cf_options);

static std::map<CompactionStyle, std::string> compaction_style_to_string = {
    {kCompactionStyleLevel, "kCompactionStyleLevel"},
    {kCompactionStyleUniversal, "kCompactionStyleUniversal"},
    {kCompactionStyleFIFO, "kCompactionStyleFIFO"},
    {kCompactionStyleNone, "kCompactionStyleNone"}};

static std::map<CompactionPri, std::string> compaction_pri_to_string = {
    {kByCompensatedSize, "kByCompensatedSize"},
    {kOldestLargestSeqFirst, "kOldestLargestSeqFirst"},
    {kOldestSmallestSeqFirst, "kOldestSmallestSeqFirst"},
    {kMinOverlappingRatio, "kMinOverlappingRatio"}};

static std::map<CompactionStopStyle, std::string>
    compaction_stop_style_to_string = {
        {kCompactionStopStyleSimilarSize, "kCompactionStopStyleSimilarSize"},
        {kCompactionStopStyleTotalSize, "kCompactionStopStyleTotalSize"}};

static std::unordered_map<std::string, ChecksumType> checksum_type_string_map =
    {{"kNoChecksum", kNoChecksum}, {"kCRC32c", kCRC32c}, {"kxxHash", kxxHash}};

#ifndef ROCKSDB_LITE

Status GetMutableOptionsFromStrings(
    const MutableCFOptions& base_options,
    const std::unordered_map<std::string, std::string>& options_map,
    MutableCFOptions* new_options);

Status GetMutableDBOptionsFromStrings(
    const MutableDBOptions& base_options,
    const std::unordered_map<std::string, std::string>& options_map,
    MutableDBOptions* new_options);

Status GetTableFactoryFromMap(
    const std::string& factory_name,
    const std::unordered_map<std::string, std::string>& opt_map,
    std::shared_ptr<TableFactory>* table_factory,
    bool ignore_unknown_options = false);

enum class OptionType {
  kBoolean,
  kInt,
  kVectorInt,
  kUInt,
  kUInt32T,
  kUInt64T,
  kSizeT,
  kString,
  kDouble,
  kCompactionStyle,
  kCompactionPri,
  kSliceTransform,
  kCompressionType,
  kVectorCompressionType,
  kTableFactory,
  kComparator,
  kCompactionFilter,
  kCompactionFilterFactory,
  kMergeOperator,
  kMemTableRepFactory,
  kBlockBasedTableIndexType,
  kFilterPolicy,
  kFlushBlockPolicyFactory,
  kChecksumType,
  kEncodingType,
  kWALRecoveryMode,
  kAccessHint,
  kInfoLogLevel,
  kUnknown
};

enum class OptionVerificationType {
  kNormal,
kByName,           //选项是指针类型的，因此我们只能验证
//基于它的名字。
kByNameAllowNull,  //与Kbyname相同，但它也允许
//其中一个是nullptr。
kDeprecated        //该选项不再用于RocksDB。摇滚乐
//如果这个选项
//恰好存在于某些选项文件中。然而，
//解析器不会将其包含在序列化和
//验证过程。
};

//用于存储常量选项信息（如选项名称）的结构，
//选项类型和偏移量。
struct OptionTypeInfo {
  int offset;
  OptionType type;
  OptionVerificationType verification;
  bool is_mutable;
  int mutable_offset;
};

//将“opt_address”转换为std:：string的helper函数
//基于指定的选项类型。
bool SerializeSingleOptionHelper(const char* opt_address,
                                 const OptionType opt_type, std::string* value);

//除了在rocksdb/convenience.h中定义的公共版本之外，
//这进一步采用了可选的输出向量“不支持的\选项\名称”，
//存储“opts-map”中指定的所有不支持选项的名称。
Status GetDBOptionsFromMapInternal(
    const DBOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    DBOptions* new_options, bool input_strings_escaped,
    std::vector<std::string>* unsupported_options_names = nullptr,
    bool ignore_unknown_options = false);

//除了在rocksdb/convenience.h中定义的公共版本之外，
//这进一步采用了可选的输出向量“不支持的\选项\名称”，
//存储“opts-map”中指定的所有不支持选项的名称。
Status GetColumnFamilyOptionsFromMapInternal(
    const ColumnFamilyOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    ColumnFamilyOptions* new_options, bool input_strings_escaped,
    std::vector<std::string>* unsupported_options_names = nullptr,
    bool ignore_unknown_options = false);

static std::unordered_map<std::string, OptionTypeInfo> db_options_type_info = {
    /*
     //尚不支持
      Env＊Env；
      std:：shared_ptr<cache>row_cache；
      std:：shared_ptr<deleteScheduler>delete_scheduler；
      std:：shared_ptr<logger>info_log；
      std:：shared_ptr<rate limiter>速率限制器；
      std:：shared_ptr<statistics>statistics；
      std:：vector<dbpath>db_path；
      std:：vector<std:：shared_ptr<eventListener>>监听器；
     **/

    {"advise_random_on_open",
     {offsetof(struct DBOptions, advise_random_on_open), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"allow_mmap_reads",
     {offsetof(struct DBOptions, allow_mmap_reads), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"allow_fallocate",
     {offsetof(struct DBOptions, allow_fallocate), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"allow_mmap_writes",
     {offsetof(struct DBOptions, allow_mmap_writes), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"use_direct_reads",
     {offsetof(struct DBOptions, use_direct_reads), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"use_direct_writes",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, false, 0}},
    {"use_direct_io_for_flush_and_compaction",
     {offsetof(struct DBOptions, use_direct_io_for_flush_and_compaction),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"allow_2pc",
     {offsetof(struct DBOptions, allow_2pc), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"allow_os_buffer",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, true, 0}},
    {"create_if_missing",
     {offsetof(struct DBOptions, create_if_missing), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"create_missing_column_families",
     {offsetof(struct DBOptions, create_missing_column_families),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"disableDataSync",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, false, 0}},
{"disable_data_sync",  //为了兼容性
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, false, 0}},
    {"enable_thread_tracking",
     {offsetof(struct DBOptions, enable_thread_tracking), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"error_if_exists",
     {offsetof(struct DBOptions, error_if_exists), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"is_fd_close_on_exec",
     {offsetof(struct DBOptions, is_fd_close_on_exec), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"paranoid_checks",
     {offsetof(struct DBOptions, paranoid_checks), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"skip_log_error_on_recovery",
     {offsetof(struct DBOptions, skip_log_error_on_recovery),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"skip_stats_update_on_db_open",
     {offsetof(struct DBOptions, skip_stats_update_on_db_open),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"new_table_reader_for_compaction_inputs",
     {offsetof(struct DBOptions, new_table_reader_for_compaction_inputs),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"compaction_readahead_size",
     {offsetof(struct DBOptions, compaction_readahead_size), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"random_access_max_buffer_size",
     {offsetof(struct DBOptions, random_access_max_buffer_size),
      OptionType::kSizeT, OptionVerificationType::kNormal, false, 0}},
    {"writable_file_max_buffer_size",
     {offsetof(struct DBOptions, writable_file_max_buffer_size),
      OptionType::kSizeT, OptionVerificationType::kNormal, false, 0}},
    {"use_adaptive_mutex",
     {offsetof(struct DBOptions, use_adaptive_mutex), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"use_fsync",
     {offsetof(struct DBOptions, use_fsync), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"max_background_jobs",
     {offsetof(struct DBOptions, max_background_jobs), OptionType::kInt,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, max_background_jobs)}},
    {"max_background_compactions",
     {offsetof(struct DBOptions, max_background_compactions), OptionType::kInt,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, max_background_compactions)}},
    {"base_background_compactions",
     {offsetof(struct DBOptions, base_background_compactions), OptionType::kInt,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, base_background_compactions)}},
    {"max_background_flushes",
     {offsetof(struct DBOptions, max_background_flushes), OptionType::kInt,
      OptionVerificationType::kNormal, false, 0}},
    {"max_file_opening_threads",
     {offsetof(struct DBOptions, max_file_opening_threads), OptionType::kInt,
      OptionVerificationType::kNormal, false, 0}},
    {"max_open_files",
     {offsetof(struct DBOptions, max_open_files), OptionType::kInt,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, max_open_files)}},
    {"table_cache_numshardbits",
     {offsetof(struct DBOptions, table_cache_numshardbits), OptionType::kInt,
      OptionVerificationType::kNormal, false, 0}},
    {"db_write_buffer_size",
     {offsetof(struct DBOptions, db_write_buffer_size), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"keep_log_file_num",
     {offsetof(struct DBOptions, keep_log_file_num), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"recycle_log_file_num",
     {offsetof(struct DBOptions, recycle_log_file_num), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"log_file_time_to_roll",
     {offsetof(struct DBOptions, log_file_time_to_roll), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"manifest_preallocation_size",
     {offsetof(struct DBOptions, manifest_preallocation_size),
      OptionType::kSizeT, OptionVerificationType::kNormal, false, 0}},
    {"max_log_file_size",
     {offsetof(struct DBOptions, max_log_file_size), OptionType::kSizeT,
      OptionVerificationType::kNormal, false, 0}},
    {"db_log_dir",
     {offsetof(struct DBOptions, db_log_dir), OptionType::kString,
      OptionVerificationType::kNormal, false, 0}},
    {"wal_dir",
     {offsetof(struct DBOptions, wal_dir), OptionType::kString,
      OptionVerificationType::kNormal, false, 0}},
    {"max_subcompactions",
     {offsetof(struct DBOptions, max_subcompactions), OptionType::kUInt32T,
      OptionVerificationType::kNormal, false, 0}},
    {"WAL_size_limit_MB",
     {offsetof(struct DBOptions, WAL_size_limit_MB), OptionType::kUInt64T,
      OptionVerificationType::kNormal, false, 0}},
    {"WAL_ttl_seconds",
     {offsetof(struct DBOptions, WAL_ttl_seconds), OptionType::kUInt64T,
      OptionVerificationType::kNormal, false, 0}},
    {"bytes_per_sync",
     {offsetof(struct DBOptions, bytes_per_sync), OptionType::kUInt64T,
      OptionVerificationType::kNormal, false, 0}},
    {"delayed_write_rate",
     {offsetof(struct DBOptions, delayed_write_rate), OptionType::kUInt64T,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, delayed_write_rate)}},
    {"delete_obsolete_files_period_micros",
     {offsetof(struct DBOptions, delete_obsolete_files_period_micros),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, delete_obsolete_files_period_micros)}},
    {"max_manifest_file_size",
     {offsetof(struct DBOptions, max_manifest_file_size), OptionType::kUInt64T,
      OptionVerificationType::kNormal, false, 0}},
    {"max_total_wal_size",
     {offsetof(struct DBOptions, max_total_wal_size), OptionType::kUInt64T,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, max_total_wal_size)}},
    {"wal_bytes_per_sync",
     {offsetof(struct DBOptions, wal_bytes_per_sync), OptionType::kUInt64T,
      OptionVerificationType::kNormal, false, 0}},
    {"stats_dump_period_sec",
     {offsetof(struct DBOptions, stats_dump_period_sec), OptionType::kUInt,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, stats_dump_period_sec)}},
    {"fail_if_options_file_error",
     {offsetof(struct DBOptions, fail_if_options_file_error),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"enable_pipelined_write",
     {offsetof(struct DBOptions, enable_pipelined_write), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"allow_concurrent_memtable_write",
     {offsetof(struct DBOptions, allow_concurrent_memtable_write),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"wal_recovery_mode",
     {offsetof(struct DBOptions, wal_recovery_mode),
      OptionType::kWALRecoveryMode, OptionVerificationType::kNormal, false, 0}},
    {"enable_write_thread_adaptive_yield",
     {offsetof(struct DBOptions, enable_write_thread_adaptive_yield),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"write_thread_slow_yield_usec",
     {offsetof(struct DBOptions, write_thread_slow_yield_usec),
      OptionType::kUInt64T, OptionVerificationType::kNormal, false, 0}},
    {"write_thread_max_yield_usec",
     {offsetof(struct DBOptions, write_thread_max_yield_usec),
      OptionType::kUInt64T, OptionVerificationType::kNormal, false, 0}},
    {"access_hint_on_compaction_start",
     {offsetof(struct DBOptions, access_hint_on_compaction_start),
      OptionType::kAccessHint, OptionVerificationType::kNormal, false, 0}},
    {"info_log_level",
     {offsetof(struct DBOptions, info_log_level), OptionType::kInfoLogLevel,
      OptionVerificationType::kNormal, false, 0}},
    {"dump_malloc_stats",
     {offsetof(struct DBOptions, dump_malloc_stats), OptionType::kBoolean,
      OptionVerificationType::kNormal, false, 0}},
    {"avoid_flush_during_recovery",
     {offsetof(struct DBOptions, avoid_flush_during_recovery),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"avoid_flush_during_shutdown",
     {offsetof(struct DBOptions, avoid_flush_during_shutdown),
      OptionType::kBoolean, OptionVerificationType::kNormal, true,
      offsetof(struct MutableDBOptions, avoid_flush_during_shutdown)}},
    {"allow_ingest_behind",
     {offsetof(struct DBOptions, allow_ingest_behind), OptionType::kBoolean,
      OptionVerificationType::kNormal, false,
      offsetof(struct ImmutableDBOptions, allow_ingest_behind)}},
    {"concurrent_prepare",
     {offsetof(struct DBOptions, concurrent_prepare), OptionType::kBoolean,
      OptionVerificationType::kNormal, false,
      offsetof(struct ImmutableDBOptions, concurrent_prepare)}},
    {"manual_wal_flush",
     {offsetof(struct DBOptions, manual_wal_flush), OptionType::kBoolean,
      OptionVerificationType::kNormal, false,
      offsetof(struct ImmutableDBOptions, manual_wal_flush)}}};

//offset_of用于获取类数据成员的偏移量
//例如：偏移量（&columnFamilyOptions:：num_levels）
//此调用将返回ColumnFamilyOptions类中num_级别的偏移量。
//
//这与offsetof（）相同，但允许我们使用非标准布局
//类别和结构
//参考文献：
//http://en.cppreference.com/w/cpp/concept/standardlayouttype/标准布局类型
//https://gist.github.com/graphitemaster/494f21190bb2c63c5516
template <typename T1, typename T2>
inline int offset_of(T1 T2::*member) {
  static T2 obj;
  return int(size_t(&(obj.*member)) - size_t(&obj));
}

static std::unordered_map<std::string, OptionTypeInfo> cf_options_type_info = {
    /*尚不支持
    压实选项FIFO压实选项FIFO；
    压实选项通用压实选项通用；
    压缩选项压缩选项；
    表属性CollectorFactories表属性Collector工厂；
    typedef std:：vector<std:：shared_ptr<tablePropertiesCollectorFactory>>
        表属性集合面；
    updateStatus（*inplace_callback）（char*现有值，
                                     uint34_t*现有值大小，
                                     切片delta_值，
                                     std:：string*合并后的_值）；
     **/

    {"report_bg_io_stats",
     {offset_of(&ColumnFamilyOptions::report_bg_io_stats), OptionType::kBoolean,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, report_bg_io_stats)}},
    {"compaction_measure_io_stats",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, false, 0}},
    {"disable_auto_compactions",
     {offset_of(&ColumnFamilyOptions::disable_auto_compactions),
      OptionType::kBoolean, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, disable_auto_compactions)}},
    {"filter_deletes",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, true, 0}},
    {"inplace_update_support",
     {offset_of(&ColumnFamilyOptions::inplace_update_support),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"level_compaction_dynamic_level_bytes",
     {offset_of(&ColumnFamilyOptions::level_compaction_dynamic_level_bytes),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"optimize_filters_for_hits",
     {offset_of(&ColumnFamilyOptions::optimize_filters_for_hits),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"paranoid_file_checks",
     {offset_of(&ColumnFamilyOptions::paranoid_file_checks),
      OptionType::kBoolean, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, paranoid_file_checks)}},
    {"force_consistency_checks",
     {offset_of(&ColumnFamilyOptions::force_consistency_checks),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"purge_redundant_kvs_while_flush",
     {offset_of(&ColumnFamilyOptions::purge_redundant_kvs_while_flush),
      OptionType::kBoolean, OptionVerificationType::kNormal, false, 0}},
    {"verify_checksums_in_compaction",
     {0, OptionType::kBoolean, OptionVerificationType::kDeprecated, true, 0}},
    {"soft_pending_compaction_bytes_limit",
     {offset_of(&ColumnFamilyOptions::soft_pending_compaction_bytes_limit),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, soft_pending_compaction_bytes_limit)}},
    {"hard_pending_compaction_bytes_limit",
     {offset_of(&ColumnFamilyOptions::hard_pending_compaction_bytes_limit),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, hard_pending_compaction_bytes_limit)}},
    {"hard_rate_limit",
     {0, OptionType::kDouble, OptionVerificationType::kDeprecated, true, 0}},
    {"soft_rate_limit",
     {0, OptionType::kDouble, OptionVerificationType::kDeprecated, true, 0}},
    {"max_compaction_bytes",
     {offset_of(&ColumnFamilyOptions::max_compaction_bytes),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_compaction_bytes)}},
    {"expanded_compaction_factor",
     {0, OptionType::kInt, OptionVerificationType::kDeprecated, true, 0}},
    {"level0_file_num_compaction_trigger",
     {offset_of(&ColumnFamilyOptions::level0_file_num_compaction_trigger),
      OptionType::kInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, level0_file_num_compaction_trigger)}},
    {"level0_slowdown_writes_trigger",
     {offset_of(&ColumnFamilyOptions::level0_slowdown_writes_trigger),
      OptionType::kInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, level0_slowdown_writes_trigger)}},
    {"level0_stop_writes_trigger",
     {offset_of(&ColumnFamilyOptions::level0_stop_writes_trigger),
      OptionType::kInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, level0_stop_writes_trigger)}},
    {"max_grandparent_overlap_factor",
     {0, OptionType::kInt, OptionVerificationType::kDeprecated, true, 0}},
    {"max_mem_compaction_level",
     {0, OptionType::kInt, OptionVerificationType::kDeprecated, false, 0}},
    {"max_write_buffer_number",
     {offset_of(&ColumnFamilyOptions::max_write_buffer_number),
      OptionType::kInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_write_buffer_number)}},
    {"max_write_buffer_number_to_maintain",
     {offset_of(&ColumnFamilyOptions::max_write_buffer_number_to_maintain),
      OptionType::kInt, OptionVerificationType::kNormal, false, 0}},
    {"min_write_buffer_number_to_merge",
     {offset_of(&ColumnFamilyOptions::min_write_buffer_number_to_merge),
      OptionType::kInt, OptionVerificationType::kNormal, false, 0}},
    {"num_levels",
     {offset_of(&ColumnFamilyOptions::num_levels), OptionType::kInt,
      OptionVerificationType::kNormal, false, 0}},
    {"source_compaction_factor",
     {0, OptionType::kInt, OptionVerificationType::kDeprecated, true, 0}},
    {"target_file_size_multiplier",
     {offset_of(&ColumnFamilyOptions::target_file_size_multiplier),
      OptionType::kInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, target_file_size_multiplier)}},
    {"arena_block_size",
     {offset_of(&ColumnFamilyOptions::arena_block_size), OptionType::kSizeT,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, arena_block_size)}},
    {"inplace_update_num_locks",
     {offset_of(&ColumnFamilyOptions::inplace_update_num_locks),
      OptionType::kSizeT, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, inplace_update_num_locks)}},
    {"max_successive_merges",
     {offset_of(&ColumnFamilyOptions::max_successive_merges),
      OptionType::kSizeT, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_successive_merges)}},
    {"memtable_huge_page_size",
     {offset_of(&ColumnFamilyOptions::memtable_huge_page_size),
      OptionType::kSizeT, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, memtable_huge_page_size)}},
    {"memtable_prefix_bloom_huge_page_tlb_size",
     {0, OptionType::kSizeT, OptionVerificationType::kDeprecated, true, 0}},
    {"write_buffer_size",
     {offset_of(&ColumnFamilyOptions::write_buffer_size), OptionType::kSizeT,
      OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, write_buffer_size)}},
    {"bloom_locality",
     {offset_of(&ColumnFamilyOptions::bloom_locality), OptionType::kUInt32T,
      OptionVerificationType::kNormal, false, 0}},
    {"memtable_prefix_bloom_bits",
     {0, OptionType::kUInt32T, OptionVerificationType::kDeprecated, true, 0}},
    {"memtable_prefix_bloom_size_ratio",
     {offset_of(&ColumnFamilyOptions::memtable_prefix_bloom_size_ratio),
      OptionType::kDouble, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, memtable_prefix_bloom_size_ratio)}},
    {"memtable_prefix_bloom_probes",
     {0, OptionType::kUInt32T, OptionVerificationType::kDeprecated, true, 0}},
    {"min_partial_merge_operands",
     {0, OptionType::kUInt32T, OptionVerificationType::kDeprecated, true, 0}},
    {"max_bytes_for_level_base",
     {offset_of(&ColumnFamilyOptions::max_bytes_for_level_base),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_bytes_for_level_base)}},
    {"max_bytes_for_level_multiplier",
     {offset_of(&ColumnFamilyOptions::max_bytes_for_level_multiplier),
      OptionType::kDouble, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_bytes_for_level_multiplier)}},
    {"max_bytes_for_level_multiplier_additional",
     {offset_of(
          &ColumnFamilyOptions::max_bytes_for_level_multiplier_additional),
      OptionType::kVectorInt, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions,
               max_bytes_for_level_multiplier_additional)}},
    {"max_sequential_skip_in_iterations",
     {offset_of(&ColumnFamilyOptions::max_sequential_skip_in_iterations),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, max_sequential_skip_in_iterations)}},
    {"target_file_size_base",
     {offset_of(&ColumnFamilyOptions::target_file_size_base),
      OptionType::kUInt64T, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, target_file_size_base)}},
    {"rate_limit_delay_max_milliseconds",
     {0, OptionType::kUInt, OptionVerificationType::kDeprecated, false, 0}},
    {"compression",
     {offset_of(&ColumnFamilyOptions::compression),
      OptionType::kCompressionType, OptionVerificationType::kNormal, true,
      offsetof(struct MutableCFOptions, compression)}},
    {"compression_per_level",
     {offset_of(&ColumnFamilyOptions::compression_per_level),
      OptionType::kVectorCompressionType, OptionVerificationType::kNormal,
      false, 0}},
    {"bottommost_compression",
     {offset_of(&ColumnFamilyOptions::bottommost_compression),
      OptionType::kCompressionType, OptionVerificationType::kNormal, false, 0}},
    {"comparator",
     {offset_of(&ColumnFamilyOptions::comparator), OptionType::kComparator,
      OptionVerificationType::kByName, false, 0}},
    {"prefix_extractor",
     {offset_of(&ColumnFamilyOptions::prefix_extractor),
      OptionType::kSliceTransform, OptionVerificationType::kByNameAllowNull,
      false, 0}},
    {"memtable_insert_with_hint_prefix_extractor",
     {offset_of(
          &ColumnFamilyOptions::memtable_insert_with_hint_prefix_extractor),
      OptionType::kSliceTransform, OptionVerificationType::kByNameAllowNull,
      false, 0}},
    {"memtable_factory",
     {offset_of(&ColumnFamilyOptions::memtable_factory),
      OptionType::kMemTableRepFactory, OptionVerificationType::kByName, false,
      0}},
    {"table_factory",
     {offset_of(&ColumnFamilyOptions::table_factory), OptionType::kTableFactory,
      OptionVerificationType::kByName, false, 0}},
    {"compaction_filter",
     {offset_of(&ColumnFamilyOptions::compaction_filter),
      OptionType::kCompactionFilter, OptionVerificationType::kByName, false,
      0}},
    {"compaction_filter_factory",
     {offset_of(&ColumnFamilyOptions::compaction_filter_factory),
      OptionType::kCompactionFilterFactory, OptionVerificationType::kByName,
      false, 0}},
    {"merge_operator",
     {offset_of(&ColumnFamilyOptions::merge_operator),
      OptionType::kMergeOperator, OptionVerificationType::kByName, false, 0}},
    {"compaction_style",
     {offset_of(&ColumnFamilyOptions::compaction_style),
      OptionType::kCompactionStyle, OptionVerificationType::kNormal, false, 0}},
    {"compaction_pri",
     {offset_of(&ColumnFamilyOptions::compaction_pri),
      OptionType::kCompactionPri, OptionVerificationType::kNormal, false, 0}}};

static std::unordered_map<std::string, CompressionType>
    compression_type_string_map = {
        {"kNoCompression", kNoCompression},
        {"kSnappyCompression", kSnappyCompression},
        {"kZlibCompression", kZlibCompression},
        {"kBZip2Compression", kBZip2Compression},
        {"kLZ4Compression", kLZ4Compression},
        {"kLZ4HCCompression", kLZ4HCCompression},
        {"kXpressCompression", kXpressCompression},
        {"kZSTD", kZSTD},
        {"kZSTDNotFinalCompression", kZSTDNotFinalCompression},
        {"kDisableCompressionOption", kDisableCompressionOption}};

static std::unordered_map<std::string, BlockBasedTableOptions::IndexType>
    block_base_table_index_type_string_map = {
        {"kBinarySearch", BlockBasedTableOptions::IndexType::kBinarySearch},
        {"kHashSearch", BlockBasedTableOptions::IndexType::kHashSearch},
        {"kTwoLevelIndexSearch",
         BlockBasedTableOptions::IndexType::kTwoLevelIndexSearch}};

static std::unordered_map<std::string, EncodingType> encoding_type_string_map =
    {{"kPlain", kPlain}, {"kPrefix", kPrefix}};

static std::unordered_map<std::string, CompactionStyle>
    compaction_style_string_map = {
        {"kCompactionStyleLevel", kCompactionStyleLevel},
        {"kCompactionStyleUniversal", kCompactionStyleUniversal},
        {"kCompactionStyleFIFO", kCompactionStyleFIFO},
        {"kCompactionStyleNone", kCompactionStyleNone}};

static std::unordered_map<std::string, CompactionPri>
    compaction_pri_string_map = {
        {"kByCompensatedSize", kByCompensatedSize},
        {"kOldestLargestSeqFirst", kOldestLargestSeqFirst},
        {"kOldestSmallestSeqFirst", kOldestSmallestSeqFirst},
        {"kMinOverlappingRatio", kMinOverlappingRatio}};

static std::unordered_map<std::string,
                          WALRecoveryMode> wal_recovery_mode_string_map = {
    {"kTolerateCorruptedTailRecords",
     WALRecoveryMode::kTolerateCorruptedTailRecords},
    {"kAbsoluteConsistency", WALRecoveryMode::kAbsoluteConsistency},
    {"kPointInTimeRecovery", WALRecoveryMode::kPointInTimeRecovery},
    {"kSkipAnyCorruptedRecords", WALRecoveryMode::kSkipAnyCorruptedRecords}};

static std::unordered_map<std::string, DBOptions::AccessHint>
    access_hint_string_map = {{"NONE", DBOptions::AccessHint::NONE},
                              {"NORMAL", DBOptions::AccessHint::NORMAL},
                              {"SEQUENTIAL", DBOptions::AccessHint::SEQUENTIAL},
                              {"WILLNEED", DBOptions::AccessHint::WILLNEED}};

static std::unordered_map<std::string, InfoLogLevel> info_log_level_string_map =
    {{"DEBUG_LEVEL", InfoLogLevel::DEBUG_LEVEL},
     {"INFO_LEVEL", InfoLogLevel::INFO_LEVEL},
     {"WARN_LEVEL", InfoLogLevel::WARN_LEVEL},
     {"ERROR_LEVEL", InfoLogLevel::ERROR_LEVEL},
     {"FATAL_LEVEL", InfoLogLevel::FATAL_LEVEL},
     {"HEADER_LEVEL", InfoLogLevel::HEADER_LEVEL}};

extern Status StringToMap(
    const std::string& opts_str,
    std::unordered_map<std::string, std::string>* opts_map);

extern bool ParseOptionHelper(char* opt_address, const OptionType& opt_type,
                              const std::string& value);
#endif  //！摇滚乐

}  //命名空间rocksdb
