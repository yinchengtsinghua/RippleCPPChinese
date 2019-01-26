
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

#include "table/block_based_table_factory.h"

#include <memory>
#include <string>
#include <stdint.h>

#include "options/options_helper.h"
#include "port/port.h"
#include "rocksdb/cache.h"
#include "rocksdb/convenience.h"
#include "rocksdb/flush_block_policy.h"
#include "table/block_based_table_builder.h"
#include "table/block_based_table_reader.h"
#include "table/format.h"
#include "util/string_util.h"

namespace rocksdb {

BlockBasedTableFactory::BlockBasedTableFactory(
    const BlockBasedTableOptions& _table_options)
    : table_options_(_table_options) {
  if (table_options_.flush_block_policy_factory == nullptr) {
    table_options_.flush_block_policy_factory.reset(
        new FlushBlockBySizePolicyFactory());
  }
  if (table_options_.no_block_cache) {
    table_options_.block_cache.reset();
  } else if (table_options_.block_cache == nullptr) {
    table_options_.block_cache = NewLRUCache(8 << 20);
  }
  if (table_options_.block_size_deviation < 0 ||
      table_options_.block_size_deviation > 100) {
    table_options_.block_size_deviation = 0;
  }
  if (table_options_.block_restart_interval < 1) {
    table_options_.block_restart_interval = 1;
  }
  if (table_options_.index_block_restart_interval < 1) {
    table_options_.index_block_restart_interval = 1;
  }
  if (table_options_.partition_filters &&
      table_options_.index_type !=
          BlockBasedTableOptions::kTwoLevelIndexSearch) {
//我们不支持没有分区索引的分区过滤器
    table_options_.partition_filters = false;
  }
}

Status BlockBasedTableFactory::NewTableReader(
    const TableReaderOptions& table_reader_options,
    unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
    unique_ptr<TableReader>* table_reader,
    bool prefetch_index_and_filter_in_cache) const {
  return BlockBasedTable::Open(
      table_reader_options.ioptions, table_reader_options.env_options,
      table_options_, table_reader_options.internal_comparator, std::move(file),
      file_size, table_reader, prefetch_index_and_filter_in_cache,
      table_reader_options.skip_filters, table_reader_options.level);
}

TableBuilder* BlockBasedTableFactory::NewTableBuilder(
    const TableBuilderOptions& table_builder_options, uint32_t column_family_id,
    WritableFileWriter* file) const {
  auto table_builder = new BlockBasedTableBuilder(
      table_builder_options.ioptions, table_options_,
      table_builder_options.internal_comparator,
      table_builder_options.int_tbl_prop_collector_factories, column_family_id,
      file, table_builder_options.compression_type,
      table_builder_options.compression_opts,
      table_builder_options.compression_dict,
      table_builder_options.skip_filters,
      table_builder_options.column_family_name,
      table_builder_options.creation_time,
      table_builder_options.oldest_key_time);

  return table_builder;
}

Status BlockBasedTableFactory::SanitizeOptions(
    const DBOptions& db_opts,
    const ColumnFamilyOptions& cf_opts) const {
  if (table_options_.index_type == BlockBasedTableOptions::kHashSearch &&
      cf_opts.prefix_extractor == nullptr) {
    return Status::InvalidArgument("Hash index is specified for block-based "
        "table, but prefix_extractor is not given");
  }
  if (table_options_.cache_index_and_filter_blocks &&
      table_options_.no_block_cache) {
    return Status::InvalidArgument("Enable cache_index_and_filter_blocks, "
        ", but block cache is disabled");
  }
  if (table_options_.pin_l0_filter_and_index_blocks_in_cache &&
      table_options_.no_block_cache) {
    return Status::InvalidArgument(
        "Enable pin_l0_filter_and_index_blocks_in_cache, "
        ", but block cache is disabled");
  }
  if (!BlockBasedTableSupportedVersion(table_options_.format_version)) {
    return Status::InvalidArgument(
        "Unsupported BlockBasedTable format_version. Please check "
        "include/rocksdb/table.h for more info");
  }
  return Status::OK();
}

std::string BlockBasedTableFactory::GetPrintableTableOptions() const {
  std::string ret;
  ret.reserve(20000);
  const int kBufferSize = 200;
  char buffer[kBufferSize];

  snprintf(buffer, kBufferSize, "  flush_block_policy_factory: %s (%p)\n",
           table_options_.flush_block_policy_factory->Name(),
           static_cast<void*>(table_options_.flush_block_policy_factory.get()));
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  cache_index_and_filter_blocks: %d\n",
           table_options_.cache_index_and_filter_blocks);
  ret.append(buffer);
  snprintf(buffer, kBufferSize,
           "  cache_index_and_filter_blocks_with_high_priority: %d\n",
           table_options_.cache_index_and_filter_blocks_with_high_priority);
  ret.append(buffer);
  snprintf(buffer, kBufferSize,
           "  pin_l0_filter_and_index_blocks_in_cache: %d\n",
           table_options_.pin_l0_filter_and_index_blocks_in_cache);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  index_type: %d\n",
           table_options_.index_type);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  hash_index_allow_collision: %d\n",
           table_options_.hash_index_allow_collision);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  checksum: %d\n",
           table_options_.checksum);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  no_block_cache: %d\n",
           table_options_.no_block_cache);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  block_cache: %p\n",
           static_cast<void*>(table_options_.block_cache.get()));
  ret.append(buffer);
  if (table_options_.block_cache) {
    const char* block_cache_name = table_options_.block_cache->Name();
    if (block_cache_name != nullptr) {
      snprintf(buffer, kBufferSize, "  block_cache_name: %s\n",
               block_cache_name);
      ret.append(buffer);
    }
    ret.append("  block_cache_options:\n");
    ret.append(table_options_.block_cache->GetPrintableOptions());
  }
  snprintf(buffer, kBufferSize, "  block_cache_compressed: %p\n",
           static_cast<void*>(table_options_.block_cache_compressed.get()));
  ret.append(buffer);
  if (table_options_.block_cache_compressed) {
    const char* block_cache_compressed_name =
        table_options_.block_cache_compressed->Name();
    if (block_cache_compressed_name != nullptr) {
      snprintf(buffer, kBufferSize, "  block_cache_name: %s\n",
               block_cache_compressed_name);
      ret.append(buffer);
    }
    ret.append("  block_cache_compressed_options:\n");
    ret.append(table_options_.block_cache_compressed->GetPrintableOptions());
  }
  snprintf(buffer, kBufferSize, "  persistent_cache: %p\n",
           static_cast<void*>(table_options_.persistent_cache.get()));
  ret.append(buffer);
  if (table_options_.persistent_cache) {
    snprintf(buffer, kBufferSize, "  persistent_cache_options:\n");
    ret.append(buffer);
    ret.append(table_options_.persistent_cache->GetPrintableOptions());
  }
  snprintf(buffer, kBufferSize, "  block_size: %" ROCKSDB_PRIszt "\n",
           table_options_.block_size);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  block_size_deviation: %d\n",
           table_options_.block_size_deviation);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  block_restart_interval: %d\n",
           table_options_.block_restart_interval);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  index_block_restart_interval: %d\n",
           table_options_.index_block_restart_interval);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  filter_policy: %s\n",
           table_options_.filter_policy == nullptr ?
             "nullptr" : table_options_.filter_policy->Name());
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  whole_key_filtering: %d\n",
           table_options_.whole_key_filtering);
  ret.append(buffer);
  snprintf(buffer, kBufferSize, "  format_version: %d\n",
           table_options_.format_version);
  ret.append(buffer);
  return ret;
}

#ifndef ROCKSDB_LITE
namespace {
bool SerializeSingleBlockBasedTableOption(
    std::string* opt_string, const BlockBasedTableOptions& bbt_options,
    const std::string& name, const std::string& delimiter) {
  auto iter = block_based_table_type_info.find(name);
  if (iter == block_based_table_type_info.end()) {
    return false;
  }
  auto& opt_info = iter->second;
  const char* opt_address =
      reinterpret_cast<const char*>(&bbt_options) + opt_info.offset;
  std::string value;
  bool result = SerializeSingleOptionHelper(opt_address, opt_info.type, &value);
  if (result) {
    *opt_string = name + "=" + value + delimiter;
  }
  return result;
}
}  //命名空间

Status BlockBasedTableFactory::GetOptionString(
    std::string* opt_string, const std::string& delimiter) const {
  assert(opt_string);
  opt_string->clear();
  for (auto iter = block_based_table_type_info.begin();
       iter != block_based_table_type_info.end(); ++iter) {
    if (iter->second.verification == OptionVerificationType::kDeprecated) {
//如果rocksdb中不再使用该选项并标记为已弃用，
//我们在序列化中跳过它。
      continue;
    }
    std::string single_output;
    bool result = SerializeSingleBlockBasedTableOption(
        &single_output, table_options_, iter->first, delimiter);
    assert(result);
    if (result) {
      opt_string->append(single_output);
    }
  }
  return Status::OK();
}
#else
Status BlockBasedTableFactory::GetOptionString(
    std::string* opt_string, const std::string& delimiter) const {
  return Status::OK();
}
#endif  //！摇滚乐

const BlockBasedTableOptions& BlockBasedTableFactory::table_options() const {
  return table_options_;
}

#ifndef ROCKSDB_LITE
namespace {
std::string ParseBlockBasedTableOption(const std::string& name,
                                       const std::string& org_value,
                                       BlockBasedTableOptions* new_options,
                                       bool input_strings_escaped = false,
                                       bool ignore_unknown_options = false) {
  const std::string& value =
      input_strings_escaped ? UnescapeOptionString(org_value) : org_value;
  if (!input_strings_escaped) {
//如果输入字符串没有转义，则表示此函数是
//从setoptions调用，它采用旧格式。
    if (name == "block_cache") {
      new_options->block_cache = NewLRUCache(ParseSizeT(value));
      return "";
    } else if (name == "block_cache_compressed") {
      new_options->block_cache_compressed = NewLRUCache(ParseSizeT(value));
      return "";
    } else if (name == "filter_policy") {
//需要以下格式
//布卢姆菲尔特：int:bool
      const std::string kName = "bloomfilter:";
      if (value.compare(0, kName.size(), kName) != 0) {
        return "Invalid filter policy name";
      }
      size_t pos = value.find(':', kName.size());
      if (pos == std::string::npos) {
        return "Invalid filter policy config, missing bits_per_key";
      }
      int bits_per_key =
          ParseInt(trim(value.substr(kName.size(), pos - kName.size())));
      bool use_block_based_builder =
          ParseBoolean("use_block_based_builder", trim(value.substr(pos + 1)));
      new_options->filter_policy.reset(
          NewBloomFilterPolicy(bits_per_key, use_block_based_builder));
      return "";
    }
  }
  const auto iter = block_based_table_type_info.find(name);
  if (iter == block_based_table_type_info.end()) {
    if (ignore_unknown_options) {
      return "";
    } else {
      return "Unrecognized option";
    }
  }
  const auto& opt_info = iter->second;
  if (opt_info.verification != OptionVerificationType::kDeprecated &&
      !ParseOptionHelper(reinterpret_cast<char*>(new_options) + opt_info.offset,
                         opt_info.type, value)) {
    return "Invalid value";
  }
  return "";
}
}  //命名空间

Status GetBlockBasedTableOptionsFromString(
    const BlockBasedTableOptions& table_options, const std::string& opts_str,
    BlockBasedTableOptions* new_table_options) {
  std::unordered_map<std::string, std::string> opts_map;
  Status s = StringToMap(opts_str, &opts_map);
  if (!s.ok()) {
    return s;
  }

  return GetBlockBasedTableOptionsFromMap(table_options, opts_map,
                                          new_table_options);
}

Status GetBlockBasedTableOptionsFromMap(
    const BlockBasedTableOptions& table_options,
    const std::unordered_map<std::string, std::string>& opts_map,
    BlockBasedTableOptions* new_table_options, bool input_strings_escaped,
    bool ignore_unknown_options) {
  assert(new_table_options);
  *new_table_options = table_options;
  for (const auto& o : opts_map) {
    auto error_message = ParseBlockBasedTableOption(
        o.first, o.second, new_table_options, input_strings_escaped,
        ignore_unknown_options);
    if (error_message != "") {
      const auto iter = block_based_table_type_info.find(o.first);
      if (iter == block_based_table_type_info.end() ||
!input_strings_escaped ||  //！输入字符串转义表示
//旧的API，一切都在那里
//可解析的
          (iter->second.verification != OptionVerificationType::kByName &&
           iter->second.verification !=
               OptionVerificationType::kByNameAllowNull &&
           iter->second.verification != OptionVerificationType::kDeprecated)) {
//将“新选项”还原为默认的“基本选项”。
        *new_table_options = table_options;
        return Status::InvalidArgument("Can't parse BlockBasedTableOptions:",
                                       o.first + " " + error_message);
      }
    }
  }
  return Status::OK();
}

Status VerifyBlockBasedTableFactory(
    const BlockBasedTableFactory* base_tf,
    const BlockBasedTableFactory* file_tf,
    OptionsSanityCheckLevel sanity_check_level) {
  if ((base_tf != nullptr) != (file_tf != nullptr) &&
      sanity_check_level > kSanityLevelNone) {
    return Status::Corruption(
        "[RocksDBOptionsParser]: Inconsistent TableFactory class type");
  }
  if (base_tf == nullptr) {
    return Status::OK();
  }
  assert(file_tf != nullptr);

  const auto& base_opt = base_tf->table_options();
  const auto& file_opt = file_tf->table_options();

  for (auto& pair : block_based_table_type_info) {
    if (pair.second.verification == OptionVerificationType::kDeprecated) {
//我们跳过检查不推荐使用的变量，因为它们可能
//包含随机值，因为它们可能未初始化
      continue;
    }
    if (BBTOptionSanityCheckLevel(pair.first) <= sanity_check_level) {
      if (!AreEqualOptions(reinterpret_cast<const char*>(&base_opt),
                           reinterpret_cast<const char*>(&file_opt),
                           pair.second, pair.first, nullptr)) {
        return Status::Corruption(
            "[RocksDBOptionsParser]: "
            "failed the verification on BlockBasedTableOptions::",
            pair.first);
      }
    }
  }
  return Status::OK();
}
#endif  //！摇滚乐

TableFactory* NewBlockBasedTableFactory(
    const BlockBasedTableOptions& _table_options) {
  return new BlockBasedTableFactory(_table_options);
}

const std::string BlockBasedTableFactory::kName = "BlockBasedTable";
const std::string BlockBasedTablePropertyNames::kIndexType =
    "rocksdb.block.based.table.index.type";
const std::string BlockBasedTablePropertyNames::kWholeKeyFiltering =
    "rocksdb.block.based.table.whole.key.filtering";
const std::string BlockBasedTablePropertyNames::kPrefixFiltering =
    "rocksdb.block.based.table.prefix.filtering";
const std::string kHashIndexPrefixesBlock = "rocksdb.hashindex.prefixes";
const std::string kHashIndexPrefixesMetadataBlock =
    "rocksdb.hashindex.metadata";
const std::string kPropTrue = "1";
const std::string kPropFalse = "0";

}  //命名空间rocksdb
