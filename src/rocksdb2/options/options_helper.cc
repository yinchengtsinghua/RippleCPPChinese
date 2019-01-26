
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
#include "options/options_helper.h"

#include <cassert>
#include <cctype>
#include <cstdlib>
#include <unordered_set>
#include <vector>
#include "rocksdb/cache.h"
#include "rocksdb/compaction_filter.h"
#include "rocksdb/convenience.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/options.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "table/block_based_table_factory.h"
#include "table/plain_table_factory.h"
#include "util/cast_util.h"
#include "util/string_util.h"

namespace rocksdb {

DBOptions BuildDBOptions(const ImmutableDBOptions& immutable_db_options,
                         const MutableDBOptions& mutable_db_options) {
  DBOptions options;

  options.create_if_missing = immutable_db_options.create_if_missing;
  options.create_missing_column_families =
      immutable_db_options.create_missing_column_families;
  options.error_if_exists = immutable_db_options.error_if_exists;
  options.paranoid_checks = immutable_db_options.paranoid_checks;
  options.env = immutable_db_options.env;
  options.rate_limiter = immutable_db_options.rate_limiter;
  options.sst_file_manager = immutable_db_options.sst_file_manager;
  options.info_log = immutable_db_options.info_log;
  options.info_log_level = immutable_db_options.info_log_level;
  options.max_open_files = mutable_db_options.max_open_files;
  options.max_file_opening_threads =
      immutable_db_options.max_file_opening_threads;
  options.max_total_wal_size = mutable_db_options.max_total_wal_size;
  options.statistics = immutable_db_options.statistics;
  options.use_fsync = immutable_db_options.use_fsync;
  options.db_paths = immutable_db_options.db_paths;
  options.db_log_dir = immutable_db_options.db_log_dir;
  options.wal_dir = immutable_db_options.wal_dir;
  options.delete_obsolete_files_period_micros =
      mutable_db_options.delete_obsolete_files_period_micros;
  options.max_background_jobs = mutable_db_options.max_background_jobs;
  options.base_background_compactions =
      mutable_db_options.base_background_compactions;
  options.max_background_compactions =
      mutable_db_options.max_background_compactions;
  options.max_subcompactions = immutable_db_options.max_subcompactions;
  options.max_background_flushes = immutable_db_options.max_background_flushes;
  options.max_log_file_size = immutable_db_options.max_log_file_size;
  options.log_file_time_to_roll = immutable_db_options.log_file_time_to_roll;
  options.keep_log_file_num = immutable_db_options.keep_log_file_num;
  options.recycle_log_file_num = immutable_db_options.recycle_log_file_num;
  options.max_manifest_file_size = immutable_db_options.max_manifest_file_size;
  options.table_cache_numshardbits =
      immutable_db_options.table_cache_numshardbits;
  options.WAL_ttl_seconds = immutable_db_options.wal_ttl_seconds;
  options.WAL_size_limit_MB = immutable_db_options.wal_size_limit_mb;
  options.manifest_preallocation_size =
      immutable_db_options.manifest_preallocation_size;
  options.allow_mmap_reads = immutable_db_options.allow_mmap_reads;
  options.allow_mmap_writes = immutable_db_options.allow_mmap_writes;
  options.use_direct_reads = immutable_db_options.use_direct_reads;
  options.use_direct_io_for_flush_and_compaction =
      immutable_db_options.use_direct_io_for_flush_and_compaction;
  options.allow_fallocate = immutable_db_options.allow_fallocate;
  options.is_fd_close_on_exec = immutable_db_options.is_fd_close_on_exec;
  options.stats_dump_period_sec = mutable_db_options.stats_dump_period_sec;
  options.advise_random_on_open = immutable_db_options.advise_random_on_open;
  options.db_write_buffer_size = immutable_db_options.db_write_buffer_size;
  options.write_buffer_manager = immutable_db_options.write_buffer_manager;
  options.access_hint_on_compaction_start =
      immutable_db_options.access_hint_on_compaction_start;
  options.new_table_reader_for_compaction_inputs =
      immutable_db_options.new_table_reader_for_compaction_inputs;
  options.compaction_readahead_size =
      immutable_db_options.compaction_readahead_size;
  options.random_access_max_buffer_size =
      immutable_db_options.random_access_max_buffer_size;
  options.writable_file_max_buffer_size =
      immutable_db_options.writable_file_max_buffer_size;
  options.use_adaptive_mutex = immutable_db_options.use_adaptive_mutex;
  options.bytes_per_sync = immutable_db_options.bytes_per_sync;
  options.wal_bytes_per_sync = immutable_db_options.wal_bytes_per_sync;
  options.listeners = immutable_db_options.listeners;
  options.enable_thread_tracking = immutable_db_options.enable_thread_tracking;
  options.delayed_write_rate = mutable_db_options.delayed_write_rate;
  options.allow_concurrent_memtable_write =
      immutable_db_options.allow_concurrent_memtable_write;
  options.enable_write_thread_adaptive_yield =
      immutable_db_options.enable_write_thread_adaptive_yield;
  options.write_thread_max_yield_usec =
      immutable_db_options.write_thread_max_yield_usec;
  options.write_thread_slow_yield_usec =
      immutable_db_options.write_thread_slow_yield_usec;
  options.skip_stats_update_on_db_open =
      immutable_db_options.skip_stats_update_on_db_open;
  options.wal_recovery_mode = immutable_db_options.wal_recovery_mode;
  options.allow_2pc = immutable_db_options.allow_2pc;
  options.row_cache = immutable_db_options.row_cache;
#ifndef ROCKSDB_LITE
  options.wal_filter = immutable_db_options.wal_filter;
#endif  //摇滚乐
  options.fail_if_options_file_error =
      immutable_db_options.fail_if_options_file_error;
  options.dump_malloc_stats = immutable_db_options.dump_malloc_stats;
  options.avoid_flush_during_recovery =
      immutable_db_options.avoid_flush_during_recovery;
  options.avoid_flush_during_shutdown =
      mutable_db_options.avoid_flush_during_shutdown;
  options.allow_ingest_behind =
      immutable_db_options.allow_ingest_behind;

  return options;
}

ColumnFamilyOptions BuildColumnFamilyOptions(
    const ColumnFamilyOptions& options,
    const MutableCFOptions& mutable_cf_options) {
  ColumnFamilyOptions cf_opts(options);

//与memtable相关的选项
  cf_opts.write_buffer_size = mutable_cf_options.write_buffer_size;
  cf_opts.max_write_buffer_number = mutable_cf_options.max_write_buffer_number;
  cf_opts.arena_block_size = mutable_cf_options.arena_block_size;
  cf_opts.memtable_prefix_bloom_size_ratio =
      mutable_cf_options.memtable_prefix_bloom_size_ratio;
  cf_opts.memtable_huge_page_size = mutable_cf_options.memtable_huge_page_size;
  cf_opts.max_successive_merges = mutable_cf_options.max_successive_merges;
  cf_opts.inplace_update_num_locks =
      mutable_cf_options.inplace_update_num_locks;

//压缩相关选项
  cf_opts.disable_auto_compactions =
      mutable_cf_options.disable_auto_compactions;
  cf_opts.level0_file_num_compaction_trigger =
      mutable_cf_options.level0_file_num_compaction_trigger;
  cf_opts.level0_slowdown_writes_trigger =
      mutable_cf_options.level0_slowdown_writes_trigger;
  cf_opts.level0_stop_writes_trigger =
      mutable_cf_options.level0_stop_writes_trigger;
  cf_opts.max_compaction_bytes = mutable_cf_options.max_compaction_bytes;
  cf_opts.target_file_size_base = mutable_cf_options.target_file_size_base;
  cf_opts.target_file_size_multiplier =
      mutable_cf_options.target_file_size_multiplier;
  cf_opts.max_bytes_for_level_base =
      mutable_cf_options.max_bytes_for_level_base;
  cf_opts.max_bytes_for_level_multiplier =
      mutable_cf_options.max_bytes_for_level_multiplier;

  cf_opts.max_bytes_for_level_multiplier_additional.clear();
  for (auto value :
       mutable_cf_options.max_bytes_for_level_multiplier_additional) {
    cf_opts.max_bytes_for_level_multiplier_additional.emplace_back(value);
  }

//MISC选项
  cf_opts.max_sequential_skip_in_iterations =
      mutable_cf_options.max_sequential_skip_in_iterations;
  cf_opts.paranoid_file_checks = mutable_cf_options.paranoid_file_checks;
  cf_opts.report_bg_io_stats = mutable_cf_options.report_bg_io_stats;
  cf_opts.compression = mutable_cf_options.compression;

  cf_opts.table_factory = options.table_factory;
//todo（yhching）：找到处理以下派生选项的方法
//*Max文件大小

  return cf_opts;
}

#ifndef ROCKSDB_LITE

namespace {
template <typename T>
bool ParseEnum(const std::unordered_map<std::string, T>& type_map,
               const std::string& type, T* value) {
  auto iter = type_map.find(type);
  if (iter != type_map.end()) {
    *value = iter->second;
    return true;
  }
  return false;
}

template <typename T>
bool SerializeEnum(const std::unordered_map<std::string, T>& type_map,
                   const T& type, std::string* value) {
  for (const auto& pair : type_map) {
    if (pair.second == type) {
      *value = pair.first;
      return true;
    }
  }
  return false;
}

bool SerializeVectorCompressionType(const std::vector<CompressionType>& types,
                                    std::string* value) {
  std::stringstream ss;
  bool result;
  for (size_t i = 0; i < types.size(); ++i) {
    if (i > 0) {
      ss << ':';
    }
    std::string string_type;
    result = SerializeEnum<CompressionType>(compression_type_string_map,
                                            types[i], &string_type);
    if (result == false) {
      return result;
    }
    ss << string_type;
  }
  *value = ss.str();
  return true;
}

bool ParseVectorCompressionType(
    const std::string& value,
    std::vector<CompressionType>* compression_per_level) {
  compression_per_level->clear();
  size_t start = 0;
  while (start < value.size()) {
    size_t end = value.find(':', start);
    bool is_ok;
    CompressionType type;
    if (end == std::string::npos) {
      is_ok = ParseEnum<CompressionType>(compression_type_string_map,
                                         value.substr(start), &type);
      if (!is_ok) {
        return false;
      }
      compression_per_level->emplace_back(type);
      break;
    } else {
      is_ok = ParseEnum<CompressionType>(
          compression_type_string_map, value.substr(start, end - start), &type);
      if (!is_ok) {
        return false;
      }
      compression_per_level->emplace_back(type);
      start = end + 1;
    }
  }
  return true;
}

bool ParseSliceTransformHelper(
    const std::string& kFixedPrefixName, const std::string& kCappedPrefixName,
    const std::string& value,
    std::shared_ptr<const SliceTransform>* slice_transform) {

  auto& pe_value = value;
  if (pe_value.size() > kFixedPrefixName.size() &&
      pe_value.compare(0, kFixedPrefixName.size(), kFixedPrefixName) == 0) {
    int prefix_length = ParseInt(trim(value.substr(kFixedPrefixName.size())));
    slice_transform->reset(NewFixedPrefixTransform(prefix_length));
  } else if (pe_value.size() > kCappedPrefixName.size() &&
             pe_value.compare(0, kCappedPrefixName.size(), kCappedPrefixName) ==
                 0) {
    int prefix_length =
        ParseInt(trim(pe_value.substr(kCappedPrefixName.size())));
    slice_transform->reset(NewCappedPrefixTransform(prefix_length));
  } else if (value == kNullptrString) {
    slice_transform->reset();
  } else {
    return false;
  }

  return true;
}

bool ParseSliceTransform(
    const std::string& value,
    std::shared_ptr<const SliceTransform>* slice_transform) {
//虽然我们通常不转换
//在它的实例中使用指针类型的选项，这里我们这样做是为了向后
//兼容性，因为我们允许在setOption（）中执行此操作。

//TODO（YCHIAN）：这些系列化的一个可能更好的地方/
//反序列化在指针类型的类定义内
//选项本身，但这需要对公共API进行更大的更改。
  bool result =
      ParseSliceTransformHelper("fixed:", "capped:", value, slice_transform);
  if (result) {
    return result;
  }
  result = ParseSliceTransformHelper(
      "rocksdb.FixedPrefix.", "rocksdb.CappedPrefix.", value, slice_transform);
  if (result) {
    return result;
  }
//托多：我们可以进一步支持其他违约行为
//这里是切片转换。
  return false;
}
}  //匿名命名空间

bool ParseOptionHelper(char* opt_address, const OptionType& opt_type,
                       const std::string& value) {
  switch (opt_type) {
    case OptionType::kBoolean:
      *reinterpret_cast<bool*>(opt_address) = ParseBoolean("", value);
      break;
    case OptionType::kInt:
      *reinterpret_cast<int*>(opt_address) = ParseInt(value);
      break;
    case OptionType::kVectorInt:
      *reinterpret_cast<std::vector<int>*>(opt_address) = ParseVectorInt(value);
      break;
    case OptionType::kUInt:
      *reinterpret_cast<unsigned int*>(opt_address) = ParseUint32(value);
      break;
    case OptionType::kUInt32T:
      *reinterpret_cast<uint32_t*>(opt_address) = ParseUint32(value);
      break;
    case OptionType::kUInt64T:
      PutUnaligned(reinterpret_cast<uint64_t*>(opt_address), ParseUint64(value));
      break;
    case OptionType::kSizeT:
      PutUnaligned(reinterpret_cast<size_t*>(opt_address), ParseSizeT(value));
      break;
    case OptionType::kString:
      *reinterpret_cast<std::string*>(opt_address) = value;
      break;
    case OptionType::kDouble:
      *reinterpret_cast<double*>(opt_address) = ParseDouble(value);
      break;
    case OptionType::kCompactionStyle:
      return ParseEnum<CompactionStyle>(
          compaction_style_string_map, value,
          reinterpret_cast<CompactionStyle*>(opt_address));
    case OptionType::kCompactionPri:
      return ParseEnum<CompactionPri>(
          compaction_pri_string_map, value,
          reinterpret_cast<CompactionPri*>(opt_address));
    case OptionType::kCompressionType:
      return ParseEnum<CompressionType>(
          compression_type_string_map, value,
          reinterpret_cast<CompressionType*>(opt_address));
    case OptionType::kVectorCompressionType:
      return ParseVectorCompressionType(
          value, reinterpret_cast<std::vector<CompressionType>*>(opt_address));
    case OptionType::kSliceTransform:
      return ParseSliceTransform(
          value, reinterpret_cast<std::shared_ptr<const SliceTransform>*>(
                     opt_address));
    case OptionType::kChecksumType:
      return ParseEnum<ChecksumType>(
          checksum_type_string_map, value,
          reinterpret_cast<ChecksumType*>(opt_address));
    case OptionType::kBlockBasedTableIndexType:
      return ParseEnum<BlockBasedTableOptions::IndexType>(
          block_base_table_index_type_string_map, value,
          reinterpret_cast<BlockBasedTableOptions::IndexType*>(opt_address));
    case OptionType::kEncodingType:
      return ParseEnum<EncodingType>(
          encoding_type_string_map, value,
          reinterpret_cast<EncodingType*>(opt_address));
    case OptionType::kWALRecoveryMode:
      return ParseEnum<WALRecoveryMode>(
          wal_recovery_mode_string_map, value,
          reinterpret_cast<WALRecoveryMode*>(opt_address));
    case OptionType::kAccessHint:
      return ParseEnum<DBOptions::AccessHint>(
          access_hint_string_map, value,
          reinterpret_cast<DBOptions::AccessHint*>(opt_address));
    case OptionType::kInfoLogLevel:
      return ParseEnum<InfoLogLevel>(
          info_log_level_string_map, value,
          reinterpret_cast<InfoLogLevel*>(opt_address));
    default:
      return false;
  }
  return true;
}

bool SerializeSingleOptionHelper(const char* opt_address,
                                 const OptionType opt_type,
                                 std::string* value) {

  assert(value);
  switch (opt_type) {
    case OptionType::kBoolean:
      *value = *(reinterpret_cast<const bool*>(opt_address)) ? "true" : "false";
      break;
    case OptionType::kInt:
      *value = ToString(*(reinterpret_cast<const int*>(opt_address)));
      break;
    case OptionType::kVectorInt:
      return SerializeIntVector(
          *reinterpret_cast<const std::vector<int>*>(opt_address), value);
    case OptionType::kUInt:
      *value = ToString(*(reinterpret_cast<const unsigned int*>(opt_address)));
      break;
    case OptionType::kUInt32T:
      *value = ToString(*(reinterpret_cast<const uint32_t*>(opt_address)));
      break;
    case OptionType::kUInt64T:
      {
        uint64_t v;
        GetUnaligned(reinterpret_cast<const uint64_t*>(opt_address), &v);
        *value = ToString(v);
      }
      break;
    case OptionType::kSizeT:
      {
        size_t v;
        GetUnaligned(reinterpret_cast<const size_t*>(opt_address), &v);
        *value = ToString(v);
      }
      break;
    case OptionType::kDouble:
      *value = ToString(*(reinterpret_cast<const double*>(opt_address)));
      break;
    case OptionType::kString:
      *value = EscapeOptionString(
          *(reinterpret_cast<const std::string*>(opt_address)));
      break;
    case OptionType::kCompactionStyle:
      return SerializeEnum<CompactionStyle>(
          compaction_style_string_map,
          *(reinterpret_cast<const CompactionStyle*>(opt_address)), value);
    case OptionType::kCompactionPri:
      return SerializeEnum<CompactionPri>(
          compaction_pri_string_map,
          *(reinterpret_cast<const CompactionPri*>(opt_address)), value);
    case OptionType::kCompressionType:
      return SerializeEnum<CompressionType>(
          compression_type_string_map,
          *(reinterpret_cast<const CompressionType*>(opt_address)), value);
    case OptionType::kVectorCompressionType:
      return SerializeVectorCompressionType(
          *(reinterpret_cast<const std::vector<CompressionType>*>(opt_address)),
          value);
      break;
    case OptionType::kSliceTransform: {
      const auto* slice_transform_ptr =
          reinterpret_cast<const std::shared_ptr<const SliceTransform>*>(
              opt_address);
      *value = slice_transform_ptr->get() ? slice_transform_ptr->get()->Name()
                                          : kNullptrString;
      break;
    }
    case OptionType::kTableFactory: {
      const auto* table_factory_ptr =
          reinterpret_cast<const std::shared_ptr<const TableFactory>*>(
              opt_address);
      *value = table_factory_ptr->get() ? table_factory_ptr->get()->Name()
                                        : kNullptrString;
      break;
    }
    case OptionType::kComparator: {
//它是常量比较器的常量指针*
      const auto* ptr = reinterpret_cast<const Comparator* const*>(opt_address);
//因为用户指定的比较器将由
//InternalKeyComparator，我们应该保留用户指定的
//而不是InternalKeyComparator。
      if (*ptr == nullptr) {
        *value = kNullptrString;
      } else {
        const Comparator* root_comp = (*ptr)->GetRootComparator();
        if (root_comp == nullptr) {
          root_comp = (*ptr);
        }
        *value = root_comp->Name();
      }
      break;
    }
    case OptionType::kCompactionFilter: {
//它是常量compactionfilter*的常量指针
      const auto* ptr =
          reinterpret_cast<const CompactionFilter* const*>(opt_address);
      *value = *ptr ? (*ptr)->Name() : kNullptrString;
      break;
    }
    case OptionType::kCompactionFilterFactory: {
      const auto* ptr =
          reinterpret_cast<const std::shared_ptr<CompactionFilterFactory>*>(
              opt_address);
      *value = ptr->get() ? ptr->get()->Name() : kNullptrString;
      break;
    }
    case OptionType::kMemTableRepFactory: {
      const auto* ptr =
          reinterpret_cast<const std::shared_ptr<MemTableRepFactory>*>(
              opt_address);
      *value = ptr->get() ? ptr->get()->Name() : kNullptrString;
      break;
    }
    case OptionType::kMergeOperator: {
      const auto* ptr =
          reinterpret_cast<const std::shared_ptr<MergeOperator>*>(opt_address);
      *value = ptr->get() ? ptr->get()->Name() : kNullptrString;
      break;
    }
    case OptionType::kFilterPolicy: {
      const auto* ptr =
          reinterpret_cast<const std::shared_ptr<FilterPolicy>*>(opt_address);
      *value = ptr->get() ? ptr->get()->Name() : kNullptrString;
      break;
    }
    case OptionType::kChecksumType:
      return SerializeEnum<ChecksumType>(
          checksum_type_string_map,
          *reinterpret_cast<const ChecksumType*>(opt_address), value);
    case OptionType::kBlockBasedTableIndexType:
      return SerializeEnum<BlockBasedTableOptions::IndexType>(
          block_base_table_index_type_string_map,
          *reinterpret_cast<const BlockBasedTableOptions::IndexType*>(
              opt_address),
          value);
    case OptionType::kFlushBlockPolicyFactory: {
      const auto* ptr =
          reinterpret_cast<const std::shared_ptr<FlushBlockPolicyFactory>*>(
              opt_address);
      *value = ptr->get() ? ptr->get()->Name() : kNullptrString;
      break;
    }
    case OptionType::kEncodingType:
      return SerializeEnum<EncodingType>(
          encoding_type_string_map,
          *reinterpret_cast<const EncodingType*>(opt_address), value);
    case OptionType::kWALRecoveryMode:
      return SerializeEnum<WALRecoveryMode>(
          wal_recovery_mode_string_map,
          *reinterpret_cast<const WALRecoveryMode*>(opt_address), value);
    case OptionType::kAccessHint:
      return SerializeEnum<DBOptions::AccessHint>(
          access_hint_string_map,
          *reinterpret_cast<const DBOptions::AccessHint*>(opt_address), value);
    case OptionType::kInfoLogLevel:
      return SerializeEnum<InfoLogLevel>(
          info_log_level_string_map,
          *reinterpret_cast<const InfoLogLevel*>(opt_address), value);
    default:
      return false;
  }
  return true;
}

Status GetMutableOptionsFromStrings(
    const MutableCFOptions& base_options,
    const std::unordered_map<std::string, std::string>& options_map,
    MutableCFOptions* new_options) {
  assert(new_options);
  *new_options = base_options;
  for (const auto& o : options_map) {
    try {
      auto iter = cf_options_type_info.find(o.first);
      if (iter == cf_options_type_info.end()) {
        return Status::InvalidArgument("Unrecognized option: " + o.first);
      }
      const auto& opt_info = iter->second;
      if (!opt_info.is_mutable) {
        return Status::InvalidArgument("Option not changeable: " + o.first);
      }
      bool is_ok = ParseOptionHelper(
          reinterpret_cast<char*>(new_options) + opt_info.mutable_offset,
          opt_info.type, o.second);
      if (!is_ok) {
        return Status::InvalidArgument("Error parsing " + o.first);
      }
    } catch (std::exception& e) {
      return Status::InvalidArgument("Error parsing " + o.first + ":" +
                                     std::string(e.what()));
    }
  }
  return Status::OK();
}

Status GetMutableDBOptionsFromStrings(
    const MutableDBOptions& base_options,
    const std::unordered_map<std::string, std::string>& options_map,
    MutableDBOptions* new_options) {
  assert(new_options);
  *new_options = base_options;
  for (const auto& o : options_map) {
    try {
      auto iter = db_options_type_info.find(o.first);
      if (iter == db_options_type_info.end()) {
        return Status::InvalidArgument("Unrecognized option: " + o.first);
      }
      const auto& opt_info = iter->second;
      if (!opt_info.is_mutable) {
        return Status::InvalidArgument("Option not changeable: " + o.first);
      }
      bool is_ok = ParseOptionHelper(
          reinterpret_cast<char*>(new_options) + opt_info.mutable_offset,
          opt_info.type, o.second);
      if (!is_ok) {
        return Status::InvalidArgument("Error parsing " + o.first);
      }
    } catch (std::exception& e) {
      return Status::InvalidArgument("Error parsing " + o.first + ":" +
                                     std::string(e.what()));
    }
  }
  return Status::OK();
}

Status StringToMap(const std::string& opts_str,
                   std::unordered_map<std::string, std::string>* opts_map) {
  assert(opts_map);
//例子：
//opts_str=“write_buffer_size=1024；max_write_buffer_number=2；”
//“嵌套的_opt=opt1=1；opt2=2 max_bytes_for_level_base=100”
  size_t pos = 0;
  std::string opts = trim(opts_str);
  while (pos < opts.size()) {
    size_t eq_pos = opts.find('=', pos);
    if (eq_pos == std::string::npos) {
      return Status::InvalidArgument("Mismatched key value pair, '=' expected");
    }
    std::string key = trim(opts.substr(pos, eq_pos - pos));
    if (key.empty()) {
      return Status::InvalidArgument("Empty key found");
    }

//跳过“=”后面的空格，查找“”以查找可能的嵌套选项
    pos = eq_pos + 1;
    while (pos < opts.size() && isspace(opts[pos])) {
      ++pos;
    }
//末尾为空值
    if (pos >= opts.size()) {
      (*opts_map)[key] = "";
      break;
    }
    if (opts[pos] == '{') {
      int count = 1;
      size_t brace_pos = pos + 1;
      while (brace_pos < opts.size()) {
        if (opts[brace_pos] == '{') {
          ++count;
        } else if (opts[brace_pos] == '}') {
          --count;
          if (count == 0) {
            break;
          }
        }
        ++brace_pos;
      }
//找到匹配的右大括号
      if (count == 0) {
        (*opts_map)[key] = trim(opts.substr(pos + 1, brace_pos - pos - 1));
//跳过所有空白并移到下一个“；”
//括号“”指向匹配后的下一个位置
        pos = brace_pos + 1;
        while (pos < opts.size() && isspace(opts[pos])) {
          ++pos;
        }
        if (pos < opts.size() && opts[pos] != ';') {
          return Status::InvalidArgument(
              "Unexpected chars after nested options");
        }
        ++pos;
      } else {
        return Status::InvalidArgument(
            "Mismatched curly braces for nested options");
      }
    } else {
      size_t sc_pos = opts.find(';', pos);
      if (sc_pos == std::string::npos) {
        (*opts_map)[key] = trim(opts.substr(pos));
//它要么以尾随的分号结束，要么以最后一个键值对结束。
        break;
      } else {
        (*opts_map)[key] = trim(opts.substr(pos, sc_pos - pos));
      }
      pos = sc_pos + 1;
    }
  }

  return Status::OK();
}

Status ParseColumnFamilyOption(const std::string& name,
                               const std::string& org_value,
                               ColumnFamilyOptions* new_options,
                               bool input_strings_escaped = false) {
  const std::string& value =
      input_strings_escaped ? UnescapeOptionString(org_value) : org_value;
  try {
    if (name == "block_based_table_factory") {
//嵌套选项
      BlockBasedTableOptions table_opt, base_table_options;
      BlockBasedTableFactory* block_based_table_factory =
          static_cast_with_check<BlockBasedTableFactory, TableFactory>(
              new_options->table_factory.get());
      if (block_based_table_factory != nullptr) {
        base_table_options = block_based_table_factory->table_options();
      }
      Status table_opt_s = GetBlockBasedTableOptionsFromString(
          base_table_options, value, &table_opt);
      if (!table_opt_s.ok()) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      new_options->table_factory.reset(NewBlockBasedTableFactory(table_opt));
    } else if (name == "plain_table_factory") {
//嵌套选项
      PlainTableOptions table_opt, base_table_options;
      PlainTableFactory* plain_table_factory =
          static_cast_with_check<PlainTableFactory, TableFactory>(
              new_options->table_factory.get());
      if (plain_table_factory != nullptr) {
        base_table_options = plain_table_factory->table_options();
      }
      Status table_opt_s = GetPlainTableOptionsFromString(
          base_table_options, value, &table_opt);
      if (!table_opt_s.ok()) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      new_options->table_factory.reset(NewPlainTableFactory(table_opt));
    } else if (name == "memtable") {
      std::unique_ptr<MemTableRepFactory> new_mem_factory;
      Status mem_factory_s =
          GetMemTableRepFactoryFromString(value, &new_mem_factory);
      if (!mem_factory_s.ok()) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      new_options->memtable_factory.reset(new_mem_factory.release());
    } else if (name == "compression_opts") {
      size_t start = 0;
      size_t end = value.find(':');
      if (end == std::string::npos) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      new_options->compression_opts.window_bits =
          ParseInt(value.substr(start, end - start));
      start = end + 1;
      end = value.find(':', start);
      if (end == std::string::npos) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      new_options->compression_opts.level =
          ParseInt(value.substr(start, end - start));
      start = end + 1;
      if (start >= value.size()) {
        return Status::InvalidArgument(
            "unable to parse the specified CF option " + name);
      }
      end = value.find(':', start);
      new_options->compression_opts.strategy =
          ParseInt(value.substr(start, value.size() - start));
//为了向后兼容，max_dict_字节是可选的
      if (end != std::string::npos) {
        start = end + 1;
        if (start >= value.size()) {
          return Status::InvalidArgument(
              "unable to parse the specified CF option " + name);
        }
        new_options->compression_opts.max_dict_bytes =
            ParseInt(value.substr(start, value.size() - start));
      }
    } else if (name == "compaction_options_fifo") {
      new_options->compaction_options_fifo.max_table_files_size =
          ParseUint64(value);
    } else {
      auto iter = cf_options_type_info.find(name);
      if (iter == cf_options_type_info.end()) {
        return Status::InvalidArgument(
            "Unable to parse the specified CF option " + name);
      }
      const auto& opt_info = iter->second;
      if (opt_info.verification != OptionVerificationType::kDeprecated &&
          ParseOptionHelper(
              reinterpret_cast<char*>(new_options) + opt_info.offset,
              opt_info.type, value)) {
        return Status::OK();
      }
      switch (opt_info.verification) {
        case OptionVerificationType::kByName:
        case OptionVerificationType::kByNameAllowNull:
          return Status::NotSupported(
              "Deserializing the specified CF option " + name +
                  " is not supported");
        case OptionVerificationType::kDeprecated:
          return Status::OK();
        default:
          return Status::InvalidArgument(
              "Unable to parse the specified CF option " + name);
      }
    }
  } catch (const std::exception&) {
    return Status::InvalidArgument(
        "unable to parse the specified option " + name);
  }
  return Status::OK();
}

bool SerializeSingleDBOption(std::string* opt_string,
                             const DBOptions& db_options,
                             const std::string& name,
                             const std::string& delimiter) {
  auto iter = db_options_type_info.find(name);
  if (iter == db_options_type_info.end()) {
    return false;
  }
  auto& opt_info = iter->second;
  const char* opt_address =
      reinterpret_cast<const char*>(&db_options) + opt_info.offset;
  std::string value;
  bool result = SerializeSingleOptionHelper(opt_address, opt_info.type, &value);
  if (result) {
    *opt_string = name + "=" + value + delimiter;
  }
  return result;
}

Status GetStringFromDBOptions(std::string* opt_string,
                              const DBOptions& db_options,
                              const std::string& delimiter) {
  assert(opt_string);
  opt_string->clear();
  for (auto iter = db_options_type_info.begin();
       iter != db_options_type_info.end(); ++iter) {
    if (iter->second.verification == OptionVerificationType::kDeprecated) {
//如果rocksdb中不再使用该选项并标记为已弃用，
//我们在序列化中跳过它。
      continue;
    }
    std::string single_output;
    bool result = SerializeSingleDBOption(&single_output, db_options,
                                          iter->first, delimiter);
    assert(result);
    if (result) {
      opt_string->append(single_output);
    }
  }
  return Status::OK();
}

bool SerializeSingleColumnFamilyOption(std::string* opt_string,
                                       const ColumnFamilyOptions& cf_options,
                                       const std::string& name,
                                       const std::string& delimiter) {
  auto iter = cf_options_type_info.find(name);
  if (iter == cf_options_type_info.end()) {
    return false;
  }
  auto& opt_info = iter->second;
  const char* opt_address =
      reinterpret_cast<const char*>(&cf_options) + opt_info.offset;
  std::string value;
  bool result = SerializeSingleOptionHelper(opt_address, opt_info.type, &value);
  if (result) {
    *opt_string = name + "=" + value + delimiter;
  }
  return result;
}

Status GetStringFromColumnFamilyOptions(std::string* opt_string,
                                        const ColumnFamilyOptions& cf_options,
                                        const std::string& delimiter) {
  assert(opt_string);
  opt_string->clear();
  for (auto iter = cf_options_type_info.begin();
       iter != cf_options_type_info.end(); ++iter) {
    if (iter->second.verification == OptionVerificationType::kDeprecated) {
//如果rocksdb中不再使用该选项并标记为已弃用，
//我们在序列化中跳过它。
      continue;
    }
    std::string single_output;
    bool result = SerializeSingleColumnFamilyOption(&single_output, cf_options,
                                                    iter->first, delimiter);
    if (result) {
      opt_string->append(single_output);
    } else {
      return Status::InvalidArgument("failed to serialize %s\n",
                                     iter->first.c_str());
    }
    assert(result);
  }
  return Status::OK();
}

Status GetStringFromCompressionType(std::string* compression_str,
                                    CompressionType compression_type) {
  bool ok = SerializeEnum<CompressionType>(compression_type_string_map,
                                           compression_type, compression_str);
  if (ok) {
    return Status::OK();
  } else {
    return Status::InvalidArgument("Invalid compression types");
  }
}

std::vector<CompressionType> GetSupportedCompressions() {
  std::vector<CompressionType> supported_compressions;
  for (const auto& comp_to_name : compression_type_string_map) {
    CompressionType t = comp_to_name.second;
    if (t != kDisableCompressionOption && CompressionTypeSupported(t)) {
      supported_compressions.push_back(t);
    }
  }
  return supported_compressions;
}

Status ParseDBOption(const std::string& name,
                     const std::string& org_value,
                     DBOptions* new_options,
                     bool input_strings_escaped = false) {
  const std::string& value =
      input_strings_escaped ? UnescapeOptionString(org_value) : org_value;
  try {
    if (name == "rate_limiter_bytes_per_sec") {
      new_options->rate_limiter.reset(
          NewGenericRateLimiter(static_cast<int64_t>(ParseUint64(value))));
    } else {
      auto iter = db_options_type_info.find(name);
      if (iter == db_options_type_info.end()) {
        return Status::InvalidArgument("Unrecognized option DBOptions:", name);
      }
      const auto& opt_info = iter->second;
      if (opt_info.verification != OptionVerificationType::kDeprecated &&
          ParseOptionHelper(
              reinterpret_cast<char*>(new_options) + opt_info.offset,
              opt_info.type, value)) {
        return Status::OK();
      }
      switch (opt_info.verification) {
        case OptionVerificationType::kByName:
        case OptionVerificationType::kByNameAllowNull:
          return Status::NotSupported(
              "Deserializing the specified DB option " + name +
                  " is not supported");
        case OptionVerificationType::kDeprecated:
          return Status::OK();
        default:
          return Status::InvalidArgument(
              "Unable to parse the specified DB option " + name);
      }
    }
  } catch (const std::exception&) {
    return Status::InvalidArgument("Unable to parse DBOptions:", name);
  }
  return Status::OK();
}

Status GetColumnFamilyOptionsFromMap(
    const ColumnFamilyOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    ColumnFamilyOptions* new_options, bool input_strings_escaped,
    bool ignore_unknown_options) {
  return GetColumnFamilyOptionsFromMapInternal(
      base_options, opts_map, new_options, input_strings_escaped, nullptr,
      ignore_unknown_options);
}

Status GetColumnFamilyOptionsFromMapInternal(
    const ColumnFamilyOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    ColumnFamilyOptions* new_options, bool input_strings_escaped,
    std::vector<std::string>* unsupported_options_names,
    bool ignore_unknown_options) {
  assert(new_options);
  *new_options = base_options;
  if (unsupported_options_names) {
    unsupported_options_names->clear();
  }
  for (const auto& o : opts_map) {
    auto s = ParseColumnFamilyOption(o.first, o.second, new_options,
                                 input_strings_escaped);
    if (!s.ok()) {
      if (s.IsNotSupported()) {
//如果不支持指定选项的反序列化
//并提供不支持的输出向量，然后
//我们记录不支持的选项的名称并继续。
        if (unsupported_options_names != nullptr) {
          unsupported_options_names->push_back(o.first);
        }
//注意，在这种情况下，我们仍然返回状态：：OK以维护
//中定义的旧公共API中的向后兼容性
//RocksDB/便利.H
      } else if (s.IsInvalidArgument() && ignore_unknown_options) {
        continue;
      } else {
//将“新选项”还原为默认的“基本选项”。
        *new_options = base_options;
        return s;
      }
    }
  }
  return Status::OK();
}

Status GetColumnFamilyOptionsFromString(
    const ColumnFamilyOptions& base_options,
    const std::string& opts_str,
    ColumnFamilyOptions* new_options) {
  std::unordered_map<std::string, std::string> opts_map;
  Status s = StringToMap(opts_str, &opts_map);
  if (!s.ok()) {
    *new_options = base_options;
    return s;
  }
  return GetColumnFamilyOptionsFromMap(base_options, opts_map, new_options);
}

Status GetDBOptionsFromMap(
    const DBOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    DBOptions* new_options, bool input_strings_escaped,
    bool ignore_unknown_options) {
  return GetDBOptionsFromMapInternal(base_options, opts_map, new_options,
                                     input_strings_escaped, nullptr,
                                     ignore_unknown_options);
}

Status GetDBOptionsFromMapInternal(
    const DBOptions& base_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    DBOptions* new_options, bool input_strings_escaped,
    std::vector<std::string>* unsupported_options_names,
    bool ignore_unknown_options) {
  assert(new_options);
  *new_options = base_options;
  if (unsupported_options_names) {
    unsupported_options_names->clear();
  }
  for (const auto& o : opts_map) {
    auto s = ParseDBOption(o.first, o.second,
                           new_options, input_strings_escaped);
    if (!s.ok()) {
      if (s.IsNotSupported()) {
//如果不支持指定选项的反序列化
//并提供不支持的输出向量，然后
//我们记录不支持的选项的名称并继续。
        if (unsupported_options_names != nullptr) {
          unsupported_options_names->push_back(o.first);
        }
//注意，在这种情况下，我们仍然返回状态：：OK以维护
//中定义的旧公共API中的向后兼容性
//RocksDB/便利.H
      } else if (s.IsInvalidArgument() && ignore_unknown_options) {
        continue;
      } else {
//将“新选项”还原为默认的“基本选项”。
        *new_options = base_options;
        return s;
      }
    }
  }
  return Status::OK();
}

Status GetDBOptionsFromString(
    const DBOptions& base_options,
    const std::string& opts_str,
    DBOptions* new_options) {
  std::unordered_map<std::string, std::string> opts_map;
  Status s = StringToMap(opts_str, &opts_map);
  if (!s.ok()) {
    *new_options = base_options;
    return s;
  }
  return GetDBOptionsFromMap(base_options, opts_map, new_options);
}

Status GetOptionsFromString(const Options& base_options,
                            const std::string& opts_str, Options* new_options) {
  std::unordered_map<std::string, std::string> opts_map;
  Status s = StringToMap(opts_str, &opts_map);
  if (!s.ok()) {
    return s;
  }
  DBOptions new_db_options(base_options);
  ColumnFamilyOptions new_cf_options(base_options);
  for (const auto& o : opts_map) {
    if (ParseDBOption(o.first, o.second, &new_db_options).ok()) {
    } else if (ParseColumnFamilyOption(
        o.first, o.second, &new_cf_options).ok()) {
    } else {
      return Status::InvalidArgument("Can't parse option " + o.first);
    }
  }
  *new_options = Options(new_db_options, new_cf_options);
  return Status::OK();
}

Status GetTableFactoryFromMap(
    const std::string& factory_name,
    const std::unordered_map<std::string, std::string>& opt_map,
    std::shared_ptr<TableFactory>* table_factory, bool ignore_unknown_options) {
  Status s;
  if (factory_name == BlockBasedTableFactory().Name()) {
    BlockBasedTableOptions bbt_opt;
    s = GetBlockBasedTableOptionsFromMap(BlockBasedTableOptions(), opt_map,
                                         &bbt_opt,
                                         /*e，/*输入\字符串\已转义*/
                                         忽略“未知”选项）；
    如果（！）S.O.（））{
      返回S；
    }
    表“工厂->重置（新的BlockBasedTableFactory（bbt_opt））”
    返回状态：：OK（）；
  else if（factory_name==plainTableFactory（）.name（））
    普通选项pt_opt；
    S=GetPlainTableOptionsFromMap（PlainTableOptions（），opt_map，&pt_opt，
                                    是，/*输入\字符串\已转义*/

                                    ignore_unknown_options);
    if (!s.ok()) {
      return s;
    }
    table_factory->reset(new PlainTableFactory(pt_opt));
    return Status::OK();
  }
//对于不支持的表工厂，返回OK作为TableFactory
//反序列化是可选的。
  table_factory->reset();
  return Status::OK();
}

#endif  //！摇滚乐

}  //命名空间rocksdb
