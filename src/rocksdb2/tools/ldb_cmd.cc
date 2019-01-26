
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
#ifndef ROCKSDB_LITE
#include "rocksdb/utilities/ldb_cmd.h"

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "db/db_impl.h"
#include "db/dbformat.h"
#include "db/log_reader.h"
#include "db/write_batch_internal.h"
#include "port/dirent.h"
#include "rocksdb/cache.h"
#include "rocksdb/table_properties.h"
#include "rocksdb/utilities/backupable_db.h"
#include "rocksdb/utilities/checkpoint.h"
#include "rocksdb/utilities/debug.h"
#include "rocksdb/utilities/object_registry.h"
#include "rocksdb/utilities/options_util.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/write_buffer_manager.h"
#include "table/scoped_arena_iterator.h"
#include "tools/ldb_cmd_impl.h"
#include "tools/sst_dump_tool_imp.h"
#include "util/cast_util.h"
#include "util/coding.h"
#include "util/filename.h"
#include "util/stderr_logger.h"
#include "util/string_util.h"
#include "utilities/ttl/db_ttl_impl.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace rocksdb {

const std::string LDBCommand::ARG_DB = "db";
const std::string LDBCommand::ARG_PATH = "path";
const std::string LDBCommand::ARG_HEX = "hex";
const std::string LDBCommand::ARG_KEY_HEX = "key_hex";
const std::string LDBCommand::ARG_VALUE_HEX = "value_hex";
const std::string LDBCommand::ARG_CF_NAME = "column_family";
const std::string LDBCommand::ARG_TTL = "ttl";
const std::string LDBCommand::ARG_TTL_START = "start_time";
const std::string LDBCommand::ARG_TTL_END = "end_time";
const std::string LDBCommand::ARG_TIMESTAMP = "timestamp";
const std::string LDBCommand::ARG_TRY_LOAD_OPTIONS = "try_load_options";
const std::string LDBCommand::ARG_IGNORE_UNKNOWN_OPTIONS =
    "ignore_unknown_options";
const std::string LDBCommand::ARG_FROM = "from";
const std::string LDBCommand::ARG_TO = "to";
const std::string LDBCommand::ARG_MAX_KEYS = "max_keys";
const std::string LDBCommand::ARG_BLOOM_BITS = "bloom_bits";
const std::string LDBCommand::ARG_FIX_PREFIX_LEN = "fix_prefix_len";
const std::string LDBCommand::ARG_COMPRESSION_TYPE = "compression_type";
const std::string LDBCommand::ARG_COMPRESSION_MAX_DICT_BYTES =
    "compression_max_dict_bytes";
const std::string LDBCommand::ARG_BLOCK_SIZE = "block_size";
const std::string LDBCommand::ARG_AUTO_COMPACTION = "auto_compaction";
const std::string LDBCommand::ARG_DB_WRITE_BUFFER_SIZE = "db_write_buffer_size";
const std::string LDBCommand::ARG_WRITE_BUFFER_SIZE = "write_buffer_size";
const std::string LDBCommand::ARG_FILE_SIZE = "file_size";
const std::string LDBCommand::ARG_CREATE_IF_MISSING = "create_if_missing";
const std::string LDBCommand::ARG_NO_VALUE = "no_value";

const char* LDBCommand::DELIM = " ==> ";

namespace {

void DumpWalFile(std::string wal_file, bool print_header, bool print_values,
                 LDBCommandExecuteResult* exec_state);

void DumpSstFile(std::string filename, bool output_hex, bool show_properties);
};

LDBCommand* LDBCommand::InitFromCmdLineArgs(
    int argc, char** argv, const Options& options,
    const LDBOptions& ldb_options,
    const std::vector<ColumnFamilyDescriptor>* column_families) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; i++) {
    args.push_back(argv[i]);
  }
  return InitFromCmdLineArgs(args, options, ldb_options, column_families,
                             SelectCommand);
}

/*
 *分析命令行参数并创建适当的ldbcommand2
 *实例。
 *命令行参数必须采用以下格式：
 *./ldb--db=path_to_db[--commoopt1=commoopt1val]。
 *命令<param1><param2>…[-CmdSpecificOpT1=CmdSpecificOpt1Val]。
 *这类似于hbaseclienttool使用的命令行格式。
 *命令名不包含在参数中。
 *如果无法分析命令行，则返回nullptr。
 **/

LDBCommand* LDBCommand::InitFromCmdLineArgs(
    const std::vector<std::string>& args, const Options& options,
    const LDBOptions& ldb_options,
    const std::vector<ColumnFamilyDescriptor>* column_families,
    const std::function<LDBCommand*(const ParsedParams&)>& selector) {
//--x=y命令行参数作为x->y映射项添加到
//已分析的_params.option_map。
//
//格式为hex的命令行参数在该数组中以hex-to结尾
//已分析的_参数标志
  ParsedParams parsed_params;

//除选项“地图和标志”以外的所有内容。表示命令
//以及它们的参数。例如：将key1值1放入这个向量。
  std::vector<std::string> cmdTokens;

  const std::string OPTION_PREFIX = "--";

  for (const auto& arg : args) {
    if (arg[0] == '-' && arg[1] == '-'){
      std::vector<std::string> splits = StringSplit(arg, '=');
      if (splits.size() == 2) {
        std::string optionKey = splits[0].substr(OPTION_PREFIX.size());
        parsed_params.option_map[optionKey] = splits[1];
      } else {
        std::string optionKey = splits[0].substr(OPTION_PREFIX.size());
        parsed_params.flags.push_back(optionKey);
      }
    } else {
      cmdTokens.push_back(arg);
    }
  }

  if (cmdTokens.size() < 1) {
    fprintf(stderr, "Command not specified!");
    return nullptr;
  }

  parsed_params.cmd = cmdTokens[0];
  parsed_params.cmd_params.assign(cmdTokens.begin() + 1, cmdTokens.end());

  LDBCommand* command = selector(parsed_params);

  if (command) {
    command->SetDBOptions(options);
    command->SetLDBOptions(ldb_options);
  }
  return command;
}

LDBCommand* LDBCommand::SelectCommand(const ParsedParams& parsed_params) {
  if (parsed_params.cmd == GetCommand::Name()) {
    return new GetCommand(parsed_params.cmd_params, parsed_params.option_map,
                          parsed_params.flags);
  } else if (parsed_params.cmd == PutCommand::Name()) {
    return new PutCommand(parsed_params.cmd_params, parsed_params.option_map,
                          parsed_params.flags);
  } else if (parsed_params.cmd == BatchPutCommand::Name()) {
    return new BatchPutCommand(parsed_params.cmd_params,
                               parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == ScanCommand::Name()) {
    return new ScanCommand(parsed_params.cmd_params, parsed_params.option_map,
                           parsed_params.flags);
  } else if (parsed_params.cmd == DeleteCommand::Name()) {
    return new DeleteCommand(parsed_params.cmd_params, parsed_params.option_map,
                             parsed_params.flags);
  } else if (parsed_params.cmd == DeleteRangeCommand::Name()) {
    return new DeleteRangeCommand(parsed_params.cmd_params,
                                  parsed_params.option_map,
                                  parsed_params.flags);
  } else if (parsed_params.cmd == ApproxSizeCommand::Name()) {
    return new ApproxSizeCommand(parsed_params.cmd_params,
                                 parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == DBQuerierCommand::Name()) {
    return new DBQuerierCommand(parsed_params.cmd_params,
                                parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == CompactorCommand::Name()) {
    return new CompactorCommand(parsed_params.cmd_params,
                                parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == WALDumperCommand::Name()) {
    return new WALDumperCommand(parsed_params.cmd_params,
                                parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == ReduceDBLevelsCommand::Name()) {
    return new ReduceDBLevelsCommand(parsed_params.cmd_params,
                                     parsed_params.option_map,
                                     parsed_params.flags);
  } else if (parsed_params.cmd == ChangeCompactionStyleCommand::Name()) {
    return new ChangeCompactionStyleCommand(parsed_params.cmd_params,
                                            parsed_params.option_map,
                                            parsed_params.flags);
  } else if (parsed_params.cmd == DBDumperCommand::Name()) {
    return new DBDumperCommand(parsed_params.cmd_params,
                               parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == DBLoaderCommand::Name()) {
    return new DBLoaderCommand(parsed_params.cmd_params,
                               parsed_params.option_map, parsed_params.flags);
  } else if (parsed_params.cmd == ManifestDumpCommand::Name()) {
    return new ManifestDumpCommand(parsed_params.cmd_params,
                                   parsed_params.option_map,
                                   parsed_params.flags);
  } else if (parsed_params.cmd == ListColumnFamiliesCommand::Name()) {
    return new ListColumnFamiliesCommand(parsed_params.cmd_params,
                                         parsed_params.option_map,
                                         parsed_params.flags);
  } else if (parsed_params.cmd == CreateColumnFamilyCommand::Name()) {
    return new CreateColumnFamilyCommand(parsed_params.cmd_params,
                                         parsed_params.option_map,
                                         parsed_params.flags);
  } else if (parsed_params.cmd == DBFileDumperCommand::Name()) {
    return new DBFileDumperCommand(parsed_params.cmd_params,
                                   parsed_params.option_map,
                                   parsed_params.flags);
  } else if (parsed_params.cmd == InternalDumpCommand::Name()) {
    return new InternalDumpCommand(parsed_params.cmd_params,
                                   parsed_params.option_map,
                                   parsed_params.flags);
  } else if (parsed_params.cmd == CheckConsistencyCommand::Name()) {
    return new CheckConsistencyCommand(parsed_params.cmd_params,
                                       parsed_params.option_map,
                                       parsed_params.flags);
  } else if (parsed_params.cmd == CheckPointCommand::Name()) {
    return new CheckPointCommand(parsed_params.cmd_params,
                                 parsed_params.option_map,
                                 parsed_params.flags);
  } else if (parsed_params.cmd == RepairCommand::Name()) {
    return new RepairCommand(parsed_params.cmd_params, parsed_params.option_map,
                             parsed_params.flags);
  } else if (parsed_params.cmd == BackupCommand::Name()) {
    return new BackupCommand(parsed_params.cmd_params, parsed_params.option_map,
                             parsed_params.flags);
  } else if (parsed_params.cmd == RestoreCommand::Name()) {
    return new RestoreCommand(parsed_params.cmd_params,
                              parsed_params.option_map, parsed_params.flags);
  }
  return nullptr;
}

/*运行该命令，并返回执行结果。*/
void LDBCommand::Run() {
  if (!exec_state_.IsNotStarted()) {
    return;
  }

  if (db_ == nullptr && !NoDBOpen()) {
    OpenDB();
    if (exec_state_.IsFailed() && try_load_options_) {
//如果因为wal文件或
//清单文件可以提供给“dump”命令，因此我们应该继续。
//--在这些情况下，尝试加载选项无效。
      return;
    }
  }

//即使由于用户原因无法打开数据库，我们也会故意继续。
//还可以指定文件名，而不仅仅是目录。
  DoCommand();

  if (exec_state_.IsNotStarted()) {
    exec_state_ = LDBCommandExecuteResult::Succeed("");
  }

  if (db_ != nullptr) {
    CloseDB();
  }
}

LDBCommand::LDBCommand(const std::map<std::string, std::string>& options,
                       const std::vector<std::string>& flags, bool is_read_only,
                       const std::vector<std::string>& valid_cmd_line_options)
    : db_(nullptr),
      is_read_only_(is_read_only),
      is_key_hex_(false),
      is_value_hex_(false),
      is_db_ttl_(false),
      timestamp_(false),
      try_load_options_(false),
      ignore_unknown_options_(false),
      create_if_missing_(false),
      option_map_(options),
      flags_(flags),
      valid_cmd_line_options_(valid_cmd_line_options) {
  std::map<std::string, std::string>::const_iterator itr = options.find(ARG_DB);
  if (itr != options.end()) {
    db_path_ = itr->second;
  }

  itr = options.find(ARG_CF_NAME);
  if (itr != options.end()) {
    column_family_name_ = itr->second;
  } else {
    column_family_name_ = kDefaultColumnFamilyName;
  }

  is_key_hex_ = IsKeyHex(options, flags);
  is_value_hex_ = IsValueHex(options, flags);
  is_db_ttl_ = IsFlagPresent(flags, ARG_TTL);
  timestamp_ = IsFlagPresent(flags, ARG_TIMESTAMP);
  try_load_options_ = IsFlagPresent(flags, ARG_TRY_LOAD_OPTIONS);
  ignore_unknown_options_ = IsFlagPresent(flags, ARG_IGNORE_UNKNOWN_OPTIONS);
}

void LDBCommand::OpenDB() {
  Options opt;
  bool opt_set = false;
  if (!create_if_missing_ && try_load_options_) {
    Status s = LoadLatestOptions(db_path_, Env::Default(), &opt,
                                 &column_families_, ignore_unknown_options_);
    if (s.ok()) {
      opt_set = true;
    } else if (!s.IsNotFound()) {
//选项文件存在，但加载选项文件时出错。
      std::string msg = s.ToString();
      exec_state_ = LDBCommandExecuteResult::Failed(msg);
      db_ = nullptr;
      return;
    }
  }
  if (!opt_set) {
    opt = PrepareOptionsForOpenDB();
  }
  if (!exec_state_.IsNotStarted()) {
    return;
  }
//打开数据库。
  Status st;
  std::vector<ColumnFamilyHandle*> handles_opened;
  if (is_db_ttl_) {
//LDB还不支持具有多个列族的TTL DB
    if (!column_family_name_.empty() || !column_families_.empty()) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "ldb doesn't support TTL DB with multiple column families");
    }
    if (is_read_only_) {
      st = DBWithTTL::Open(opt, db_path_, &db_ttl_, 0, true);
    } else {
      st = DBWithTTL::Open(opt, db_path_, &db_ttl_);
    }
    db_ = db_ttl_;
  } else {
    if (!opt_set && column_families_.empty()) {
//尝试找出列族列表
      std::vector<std::string> cf_list;
      st = DB::ListColumnFamilies(DBOptions(), db_path_, &cf_list);
//数据库可能还不存在，因为“如果不存在则创建”
//“现有案例”。这里忽略了失败。我们依赖db:：open（）。
//为打开问题提供正确的错误消息
//现有数据库。
      if (st.ok() && cf_list.size() > 1) {
//忽略单列族数据库。
        for (auto cf_name : cf_list) {
          column_families_.emplace_back(cf_name, opt);
        }
      }
    }
    if (is_read_only_) {
      if (column_families_.empty()) {
        st = DB::OpenForReadOnly(opt, db_path_, &db_);
      } else {
        st = DB::OpenForReadOnly(opt, db_path_, column_families_,
                                 &handles_opened, &db_);
      }
    } else {
      if (column_families_.empty()) {
        st = DB::Open(opt, db_path_, &db_);
      } else {
        st = DB::Open(opt, db_path_, column_families_, &handles_opened, &db_);
      }
    }
  }
  if (!st.ok()) {
    std::string msg = st.ToString();
    exec_state_ = LDBCommandExecuteResult::Failed(msg);
  } else if (!handles_opened.empty()) {
    assert(handles_opened.size() == column_families_.size());
    bool found_cf_name = false;
    for (size_t i = 0; i < handles_opened.size(); i++) {
      cf_handles_[column_families_[i].name] = handles_opened[i];
      if (column_family_name_ == column_families_[i].name) {
        found_cf_name = true;
      }
    }
    if (!found_cf_name) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "Non-existing column family " + column_family_name_);
      CloseDB();
    }
  } else {
//我们成功地在单列家庭模式下打开了数据库。
    assert(column_families_.empty());
    if (column_family_name_ != kDefaultColumnFamilyName) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "Non-existing column family " + column_family_name_);
      CloseDB();
    }
  }

  options_ = opt;
}

void LDBCommand::CloseDB() {
  if (db_ != nullptr) {
    for (auto& pair : cf_handles_) {
      delete pair.second;
    }
    delete db_;
    db_ = nullptr;
  }
}

ColumnFamilyHandle* LDBCommand::GetCfHandle() {
  if (!cf_handles_.empty()) {
    auto it = cf_handles_.find(column_family_name_);
    if (it == cf_handles_.end()) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "Cannot find column family " + column_family_name_);
    } else {
      return it->second;
    }
  }
  return db_->DefaultColumnFamily();
}

std::vector<std::string> LDBCommand::BuildCmdLineOptions(
    std::vector<std::string> options) {
  std::vector<std::string> ret = {ARG_DB,
                                  ARG_BLOOM_BITS,
                                  ARG_BLOCK_SIZE,
                                  ARG_AUTO_COMPACTION,
                                  ARG_COMPRESSION_TYPE,
                                  ARG_COMPRESSION_MAX_DICT_BYTES,
                                  ARG_WRITE_BUFFER_SIZE,
                                  ARG_FILE_SIZE,
                                  ARG_FIX_PREFIX_LEN,
                                  ARG_TRY_LOAD_OPTIONS,
                                  ARG_IGNORE_UNKNOWN_OPTIONS,
                                  ARG_CF_NAME};
  ret.insert(ret.end(), options.begin(), options.end());
  return ret;
}

/*
 *解析特定的整数选项并填充该值。
 *如果找到选项，则返回true。
 *如果找不到该选项或分析
 *值。如果出现错误，则指定的执行状态也为
 *更新。
 **/

bool LDBCommand::ParseIntOption(
    const std::map<std::string, std::string>& options,
    const std::string& option, int& value,
    LDBCommandExecuteResult& exec_state) {
  std::map<std::string, std::string>::const_iterator itr =
      option_map_.find(option);
  if (itr != option_map_.end()) {
    try {
#if defined(CYGWIN)
      value = strtol(itr->second.c_str(), 0, 10);
#else
      value = std::stoi(itr->second);
#endif
      return true;
    } catch (const std::invalid_argument&) {
      exec_state =
          LDBCommandExecuteResult::Failed(option + " has an invalid value.");
    } catch (const std::out_of_range&) {
      exec_state = LDBCommandExecuteResult::Failed(
          option + " has a value out-of-range.");
    }
  }
  return false;
}

/*
 *解析指定的选项并填充值。
 *如果找到选项，则返回true。
 *否则返回false。
 **/

bool LDBCommand::ParseStringOption(
    const std::map<std::string, std::string>& options,
    const std::string& option, std::string* value) {
  auto itr = option_map_.find(option);
  if (itr != option_map_.end()) {
    *value = itr->second;
    return true;
  }
  return false;
}

Options LDBCommand::PrepareOptionsForOpenDB() {

  Options opt = options_;
  opt.create_if_missing = false;

  std::map<std::string, std::string>::const_iterator itr;

  BlockBasedTableOptions table_options;
  bool use_table_options = false;
  int bits;
  if (ParseIntOption(option_map_, ARG_BLOOM_BITS, bits, exec_state_)) {
    if (bits > 0) {
      use_table_options = true;
      table_options.filter_policy.reset(NewBloomFilterPolicy(bits));
    } else {
      exec_state_ =
          LDBCommandExecuteResult::Failed(ARG_BLOOM_BITS + " must be > 0.");
    }
  }

  int block_size;
  if (ParseIntOption(option_map_, ARG_BLOCK_SIZE, block_size, exec_state_)) {
    if (block_size > 0) {
      use_table_options = true;
      table_options.block_size = block_size;
    } else {
      exec_state_ =
          LDBCommandExecuteResult::Failed(ARG_BLOCK_SIZE + " must be > 0.");
    }
  }

  if (use_table_options) {
    opt.table_factory.reset(NewBlockBasedTableFactory(table_options));
  }

  itr = option_map_.find(ARG_AUTO_COMPACTION);
  if (itr != option_map_.end()) {
    opt.disable_auto_compactions = ! StringToBool(itr->second);
  }

  itr = option_map_.find(ARG_COMPRESSION_TYPE);
  if (itr != option_map_.end()) {
    std::string comp = itr->second;
    if (comp == "no") {
      opt.compression = kNoCompression;
    } else if (comp == "snappy") {
      opt.compression = kSnappyCompression;
    } else if (comp == "zlib") {
      opt.compression = kZlibCompression;
    } else if (comp == "bzip2") {
      opt.compression = kBZip2Compression;
    } else if (comp == "lz4") {
      opt.compression = kLZ4Compression;
    } else if (comp == "lz4hc") {
      opt.compression = kLZ4HCCompression;
    } else if (comp == "xpress") {
      opt.compression = kXpressCompression;
    } else if (comp == "zstd") {
      opt.compression = kZSTD;
    } else {
//未知压缩。
      exec_state_ =
          LDBCommandExecuteResult::Failed("Unknown compression level: " + comp);
    }
  }

  int compression_max_dict_bytes;
  if (ParseIntOption(option_map_, ARG_COMPRESSION_MAX_DICT_BYTES,
                     compression_max_dict_bytes, exec_state_)) {
    if (compression_max_dict_bytes >= 0) {
      opt.compression_opts.max_dict_bytes = compression_max_dict_bytes;
    } else {
      exec_state_ = LDBCommandExecuteResult::Failed(
          ARG_COMPRESSION_MAX_DICT_BYTES + " must be >= 0.");
    }
  }

  int db_write_buffer_size;
  if (ParseIntOption(option_map_, ARG_DB_WRITE_BUFFER_SIZE,
        db_write_buffer_size, exec_state_)) {
    if (db_write_buffer_size >= 0) {
      opt.db_write_buffer_size = db_write_buffer_size;
    } else {
      exec_state_ = LDBCommandExecuteResult::Failed(ARG_DB_WRITE_BUFFER_SIZE +
                                                    " must be >= 0.");
    }
  }

  int write_buffer_size;
  if (ParseIntOption(option_map_, ARG_WRITE_BUFFER_SIZE, write_buffer_size,
        exec_state_)) {
    if (write_buffer_size > 0) {
      opt.write_buffer_size = write_buffer_size;
    } else {
      exec_state_ = LDBCommandExecuteResult::Failed(ARG_WRITE_BUFFER_SIZE +
                                                    " must be > 0.");
    }
  }

  int file_size;
  if (ParseIntOption(option_map_, ARG_FILE_SIZE, file_size, exec_state_)) {
    if (file_size > 0) {
      opt.target_file_size_base = file_size;
    } else {
      exec_state_ =
          LDBCommandExecuteResult::Failed(ARG_FILE_SIZE + " must be > 0.");
    }
  }

  if (opt.db_paths.size() == 0) {
    opt.db_paths.emplace_back(db_path_, std::numeric_limits<uint64_t>::max());
  }

  int fix_prefix_len;
  if (ParseIntOption(option_map_, ARG_FIX_PREFIX_LEN, fix_prefix_len,
                     exec_state_)) {
    if (fix_prefix_len > 0) {
      opt.prefix_extractor.reset(
          NewFixedPrefixTransform(static_cast<size_t>(fix_prefix_len)));
    } else {
      exec_state_ =
          LDBCommandExecuteResult::Failed(ARG_FIX_PREFIX_LEN + " must be > 0.");
    }
  }

  return opt;
}

bool LDBCommand::ParseKeyValue(const std::string& line, std::string* key,
                               std::string* value, bool is_key_hex,
                               bool is_value_hex) {
  size_t pos = line.find(DELIM);
  if (pos != std::string::npos) {
    *key = line.substr(0, pos);
    *value = line.substr(pos + strlen(DELIM));
    if (is_key_hex) {
      *key = HexToString(*key);
    }
    if (is_value_hex) {
      *value = HexToString(*value);
    }
    return true;
  } else {
    return false;
  }
}

/*
 *确保只有命令行选项和标志
 *命令在命令行上指定。外来的选择通常是
 *用户错误的结果。
 *如果所有支票都通过，则返回“真”。否则返回false，并打印
 *适当的错误消息到stderr。
 **/

bool LDBCommand::ValidateCmdLineOptions() {
  for (std::map<std::string, std::string>::const_iterator itr =
           option_map_.begin();
       itr != option_map_.end(); ++itr) {
    if (std::find(valid_cmd_line_options_.begin(),
                  valid_cmd_line_options_.end(),
                  itr->first) == valid_cmd_line_options_.end()) {
      fprintf(stderr, "Invalid command-line option %s\n", itr->first.c_str());
      return false;
    }
  }

  for (std::vector<std::string>::const_iterator itr = flags_.begin();
       itr != flags_.end(); ++itr) {
    if (std::find(valid_cmd_line_options_.begin(),
                  valid_cmd_line_options_.end(),
                  *itr) == valid_cmd_line_options_.end()) {
      fprintf(stderr, "Invalid command-line flag %s\n", itr->c_str());
      return false;
    }
  }

  if (!NoDBOpen() && option_map_.find(ARG_DB) == option_map_.end() &&
      option_map_.find(ARG_PATH) == option_map_.end()) {
    fprintf(stderr, "Either %s or %s must be specified.\n", ARG_DB.c_str(),
            ARG_PATH.c_str());
    return false;
  }

  return true;
}

std::string LDBCommand::HexToString(const std::string& str) {
  std::string result;
  std::string::size_type len = str.length();
  if (len < 2 || str[0] != '0' || str[1] != 'x') {
    fprintf(stderr, "Invalid hex input %s.  Must start with 0x\n", str.c_str());
    throw "Invalid hex input";
  }
  if (!Slice(str.data() + 2, len - 2).DecodeHex(&result)) {
    throw "Invalid hex input";
  }
  return result;
}

std::string LDBCommand::StringToHex(const std::string& str) {
  std::string result("0x");
  result.append(Slice(str).ToString(true));
  return result;
}

std::string LDBCommand::PrintKeyValue(const std::string& key,
                                      const std::string& value, bool is_key_hex,
                                      bool is_value_hex) {
  std::string result;
  result.append(is_key_hex ? StringToHex(key) : key);
  result.append(DELIM);
  result.append(is_value_hex ? StringToHex(value) : value);
  return result;
}

std::string LDBCommand::PrintKeyValue(const std::string& key,
                                      const std::string& value, bool is_hex) {
  return PrintKeyValue(key, value, is_hex, is_hex);
}

std::string LDBCommand::HelpRangeCmdArgs() {
  std::ostringstream str_stream;
  str_stream << " ";
  str_stream << "[--" << ARG_FROM << "] ";
  str_stream << "[--" << ARG_TO << "] ";
  return str_stream.str();
}

bool LDBCommand::IsKeyHex(const std::map<std::string, std::string>& options,
                          const std::vector<std::string>& flags) {
  return (IsFlagPresent(flags, ARG_HEX) || IsFlagPresent(flags, ARG_KEY_HEX) ||
          ParseBooleanOption(options, ARG_HEX, false) ||
          ParseBooleanOption(options, ARG_KEY_HEX, false));
}

bool LDBCommand::IsValueHex(const std::map<std::string, std::string>& options,
                            const std::vector<std::string>& flags) {
  return (IsFlagPresent(flags, ARG_HEX) ||
          IsFlagPresent(flags, ARG_VALUE_HEX) ||
          ParseBooleanOption(options, ARG_HEX, false) ||
          ParseBooleanOption(options, ARG_VALUE_HEX, false));
}

bool LDBCommand::ParseBooleanOption(
    const std::map<std::string, std::string>& options,
    const std::string& option, bool default_val) {
  std::map<std::string, std::string>::const_iterator itr = options.find(option);
  if (itr != options.end()) {
    std::string option_val = itr->second;
    return StringToBool(itr->second);
  }
  return default_val;
}

bool LDBCommand::StringToBool(std::string val) {
  std::transform(val.begin(), val.end(), val.begin(),
                 [](char ch) -> char { return (char)::tolower(ch); });

  if (val == "true") {
    return true;
  } else if (val == "false") {
    return false;
  } else {
    throw "Invalid value for boolean argument";
  }
}

CompactorCommand::CompactorCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_FROM, ARG_TO, ARG_HEX, ARG_KEY_HEX,
                                      ARG_VALUE_HEX, ARG_TTL})),
      null_from_(true),
      null_to_(true) {
  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_FROM);
  if (itr != options.end()) {
    null_from_ = false;
    from_ = itr->second;
  }

  itr = options.find(ARG_TO);
  if (itr != options.end()) {
    null_to_ = false;
    to_ = itr->second;
  }

  if (is_key_hex_) {
    if (!null_from_) {
      from_ = HexToString(from_);
    }
    if (!null_to_) {
      to_ = HexToString(to_);
    }
  }
}

void CompactorCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(CompactorCommand::Name());
  ret.append(HelpRangeCmdArgs());
  ret.append("\n");
}

void CompactorCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }

  Slice* begin = nullptr;
  Slice* end = nullptr;
  if (!null_from_) {
    begin = new Slice(from_);
  }
  if (!null_to_) {
    end = new Slice(to_);
  }

  CompactRangeOptions cro;
  cro.bottommost_level_compaction = BottommostLevelCompaction::kForce;

  db_->CompactRange(cro, GetCfHandle(), begin, end);
  exec_state_ = LDBCommandExecuteResult::Succeed("");

  delete begin;
  delete end;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

const std::string DBLoaderCommand::ARG_DISABLE_WAL = "disable_wal";
const std::string DBLoaderCommand::ARG_BULK_LOAD = "bulk_load";
const std::string DBLoaderCommand::ARG_COMPACT = "compact";

DBLoaderCommand::DBLoaderCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, false,
          BuildCmdLineOptions({ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX, ARG_FROM,
                               ARG_TO, ARG_CREATE_IF_MISSING, ARG_DISABLE_WAL,
                               ARG_BULK_LOAD, ARG_COMPACT})),
      disable_wal_(false),
      bulk_load_(false),
      compact_(false) {
  create_if_missing_ = IsFlagPresent(flags, ARG_CREATE_IF_MISSING);
  disable_wal_ = IsFlagPresent(flags, ARG_DISABLE_WAL);
  bulk_load_ = IsFlagPresent(flags, ARG_BULK_LOAD);
  compact_ = IsFlagPresent(flags, ARG_COMPACT);
}

void DBLoaderCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DBLoaderCommand::Name());
  ret.append(" [--" + ARG_CREATE_IF_MISSING + "]");
  ret.append(" [--" + ARG_DISABLE_WAL + "]");
  ret.append(" [--" + ARG_BULK_LOAD + "]");
  ret.append(" [--" + ARG_COMPACT + "]");
  ret.append("\n");
}

Options DBLoaderCommand::PrepareOptionsForOpenDB() {
  Options opt = LDBCommand::PrepareOptionsForOpenDB();
  opt.create_if_missing = create_if_missing_;
  if (bulk_load_) {
    opt.PrepareForBulkLoad();
  }
  return opt;
}

void DBLoaderCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }

  WriteOptions write_options;
  if (disable_wal_) {
    write_options.disableWAL = true;
  }

  int bad_lines = 0;
  std::string line;
//首选ifstream getline性能与std:：cin istream的性能
  std::ifstream ifs_stdin("/dev/stdin");
  std::istream* istream_p = ifs_stdin.is_open() ? &ifs_stdin : &std::cin;
  while (getline(*istream_p, line, '\n')) {
    std::string key;
    std::string value;
    if (ParseKeyValue(line, &key, &value, is_key_hex_, is_value_hex_)) {
      db_->Put(write_options, GetCfHandle(), Slice(key), Slice(value));
    } else if (0 == line.find("Keys in range:")) {
//忽略此行
    } else if (0 == line.find("Created bg thread 0x")) {
//忽略此行
    } else {
      bad_lines ++;
    }
  }

  if (bad_lines > 0) {
    std::cout << "Warning: " << bad_lines << " bad lines ignored." << std::endl;
  }
  if (compact_) {
    db_->CompactRange(CompactRangeOptions(), GetCfHandle(), nullptr, nullptr);
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace {

void DumpManifestFile(std::string file, bool verbose, bool hex, bool json) {
  Options options;
  EnvOptions sopt;
  std::string dbname("dummy");
  std::shared_ptr<Cache> tc(NewLRUCache(options.max_open_files - 10,
                                        options.table_cache_numshardbits));
//注意，我们使用的是默认选项，而不是通过sanitizeOptions（），
//如果versionset:：dumpmanifest（）依赖于
//sanitizeOptions（），我们需要手动初始化它。
  options.db_paths.emplace_back("dummy", 0);
  options.num_levels = 64;
  WriteController wc(options.delayed_write_rate);
  WriteBufferManager wb(options.db_write_buffer_size);
  ImmutableDBOptions immutable_db_options(options);
  VersionSet versions(dbname, &immutable_db_options, sopt, tc.get(), &wb, &wc);
  Status s = versions.DumpManifest(options, file, verbose, hex, json);
  if (!s.ok()) {
    printf("Error in processing file %s %s\n", file.c_str(),
           s.ToString().c_str());
  }
}

}  //命名空间

const std::string ManifestDumpCommand::ARG_VERBOSE = "verbose";
const std::string ManifestDumpCommand::ARG_JSON = "json";
const std::string ManifestDumpCommand::ARG_PATH = "path";

void ManifestDumpCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ManifestDumpCommand::Name());
  ret.append(" [--" + ARG_VERBOSE + "]");
  ret.append(" [--" + ARG_JSON + "]");
  ret.append(" [--" + ARG_PATH + "=<path_to_manifest_file>]");
  ret.append("\n");
}

ManifestDumpCommand::ManifestDumpCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, false,
          BuildCmdLineOptions({ARG_VERBOSE, ARG_PATH, ARG_HEX, ARG_JSON})),
      verbose_(false),
      json_(false),
      path_("") {
  verbose_ = IsFlagPresent(flags, ARG_VERBOSE);
  json_ = IsFlagPresent(flags, ARG_JSON);

  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_PATH);
  if (itr != options.end()) {
    path_ = itr->second;
    if (path_.empty()) {
      exec_state_ = LDBCommandExecuteResult::Failed("--path: missing pathname");
    }
  }
}

void ManifestDumpCommand::DoCommand() {

  std::string manifestfile;

  if (!path_.empty()) {
    manifestfile = path_;
  } else {
    bool found = false;
//我们需要通过搜索目录找到清单文件
//包含表单清单文件的db

    auto CloseDir = [](DIR* p) { closedir(p); };
    std::unique_ptr<DIR, decltype(CloseDir)> d(opendir(db_path_.c_str()),
                                               CloseDir);

    if (d == nullptr) {
      exec_state_ =
          LDBCommandExecuteResult::Failed(db_path_ + " is not a directory");
      return;
    }
    struct dirent* entry;
    while ((entry = readdir(d.get())) != nullptr) {
      unsigned int match;
      uint64_t num;
      if (sscanf(entry->d_name, "MANIFEST-%" PRIu64 "%n", &num, &match) &&
          match == strlen(entry->d_name)) {
        if (!found) {
          manifestfile = db_path_ + "/" + std::string(entry->d_name);
          found = true;
        } else {
          exec_state_ = LDBCommandExecuteResult::Failed(
              "Multiple MANIFEST files found; use --path to select one");
          return;
        }
      }
    }
  }

  if (verbose_) {
    printf("Processing Manifest file %s\n", manifestfile.c_str());
  }

  DumpManifestFile(manifestfile, verbose_, is_key_hex_, json_);

  if (verbose_) {
    printf("Processing Manifest file %s done\n", manifestfile.c_str());
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

void ListColumnFamiliesCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ListColumnFamiliesCommand::Name());
  ret.append(" full_path_to_db_directory ");
  ret.append("\n");
}

ListColumnFamiliesCommand::ListColumnFamiliesCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false, {}) {
  if (params.size() != 1) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "dbname must be specified for the list_column_families command");
  } else {
    dbname_ = params[0];
  }
}

void ListColumnFamiliesCommand::DoCommand() {
  std::vector<std::string> column_families;
  Status s = DB::ListColumnFamilies(DBOptions(), dbname_, &column_families);
  if (!s.ok()) {
    printf("Error in processing db %s %s\n", dbname_.c_str(),
           s.ToString().c_str());
  } else {
    printf("Column families in %s: \n{", dbname_.c_str());
    bool first = true;
    for (auto cf : column_families) {
      if (!first) {
        printf(", ");
      }
      first = false;
      printf("%s", cf.c_str());
    }
    printf("}\n");
  }
}

void CreateColumnFamilyCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(CreateColumnFamilyCommand::Name());
  ret.append(" --db=<db_path> <new_column_family_name>");
  ret.append("\n");
}

CreateColumnFamilyCommand::CreateColumnFamilyCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, true, {ARG_DB}) {
  if (params.size() != 1) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "new column family name must be specified");
  } else {
    new_cf_name_ = params[0];
  }
}

void CreateColumnFamilyCommand::DoCommand() {
  ColumnFamilyHandle* new_cf_handle = nullptr;
  Status st = db_->CreateColumnFamily(options_, new_cf_name_, &new_cf_handle);
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Fail to create new column family: " + st.ToString());
  }
  delete new_cf_handle;
  CloseDB();
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace {

std::string ReadableTime(int unixtime) {
  char time_buffer [80];
  time_t rawtime = unixtime;
  struct tm tInfo;
  struct tm* timeinfo = localtime_r(&rawtime, &tInfo);
  assert(timeinfo == &tInfo);
  strftime(time_buffer, 80, "%c", timeinfo);
  return std::string(time_buffer);
}

//此函数仅在时间范围内大于1个存储桶的正常情况下调用
//也只有在提供的timekv介于ttl-start和ttl-end之间时才调用
void IncBucketCounts(std::vector<uint64_t>& bucket_counts, int ttl_start,
                     int time_range, int bucket_size, int timekv,
                     int num_buckets) {
  assert(time_range > 0 && timekv >= ttl_start && bucket_size > 0 &&
    timekv < (ttl_start + time_range) && num_buckets > 1);
  int bucket = (timekv - ttl_start) / bucket_size;
  bucket_counts[bucket]++;
}

void PrintBucketCounts(const std::vector<uint64_t>& bucket_counts,
                       int ttl_start, int ttl_end, int bucket_size,
                       int num_buckets) {
  int time_point = ttl_start;
  for(int i = 0; i < num_buckets - 1; i++, time_point += bucket_size) {
    fprintf(stdout, "Keys in range %s to %s : %lu\n",
            ReadableTime(time_point).c_str(),
            ReadableTime(time_point + bucket_size).c_str(),
            (unsigned long)bucket_counts[i]);
  }
  fprintf(stdout, "Keys in range %s to %s : %lu\n",
          ReadableTime(time_point).c_str(),
          ReadableTime(ttl_end).c_str(),
          (unsigned long)bucket_counts[num_buckets - 1]);
}

}  //命名空间

const std::string InternalDumpCommand::ARG_COUNT_ONLY = "count_only";
const std::string InternalDumpCommand::ARG_COUNT_DELIM = "count_delim";
const std::string InternalDumpCommand::ARG_STATS = "stats";
const std::string InternalDumpCommand::ARG_INPUT_KEY_HEX = "input_key_hex";

InternalDumpCommand::InternalDumpCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, true,
          BuildCmdLineOptions({ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX, ARG_FROM,
                               ARG_TO, ARG_MAX_KEYS, ARG_COUNT_ONLY,
                               ARG_COUNT_DELIM, ARG_STATS, ARG_INPUT_KEY_HEX})),
      has_from_(false),
      has_to_(false),
      max_keys_(-1),
      delim_("."),
      count_only_(false),
      count_delim_(false),
      print_stats_(false),
      is_input_key_hex_(false) {
  has_from_ = ParseStringOption(options, ARG_FROM, &from_);
  has_to_ = ParseStringOption(options, ARG_TO, &to_);

  ParseIntOption(options, ARG_MAX_KEYS, max_keys_, exec_state_);
  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_COUNT_DELIM);
  if (itr != options.end()) {
    delim_ = itr->second;
    count_delim_ = true;
//fprintf（stdout，“delim=%c\n”，delim_[0]）；
  } else {
    count_delim_ = IsFlagPresent(flags, ARG_COUNT_DELIM);
    delim_=".";
  }

  print_stats_ = IsFlagPresent(flags, ARG_STATS);
  count_only_ = IsFlagPresent(flags, ARG_COUNT_ONLY);
  is_input_key_hex_ = IsFlagPresent(flags, ARG_INPUT_KEY_HEX);

  if (is_input_key_hex_) {
    if (has_from_) {
      from_ = HexToString(from_);
    }
    if (has_to_) {
      to_ = HexToString(to_);
    }
  }
}

void InternalDumpCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(InternalDumpCommand::Name());
  ret.append(HelpRangeCmdArgs());
  ret.append(" [--" + ARG_INPUT_KEY_HEX + "]");
  ret.append(" [--" + ARG_MAX_KEYS + "=<N>]");
  ret.append(" [--" + ARG_COUNT_ONLY + "]");
  ret.append(" [--" + ARG_COUNT_DELIM + "=<char>]");
  ret.append(" [--" + ARG_STATS + "]");
  ret.append("\n");
}

void InternalDumpCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }

  if (print_stats_) {
    std::string stats;
    if (db_->GetProperty(GetCfHandle(), "rocksdb.stats", &stats)) {
      fprintf(stdout, "%s\n", stats.c_str());
    }
  }

//强制转换为dbimpl以获取内部迭代器
  std::vector<KeyVersion> key_versions;
  Status st = GetAllKeyVersions(db_, from_, to_, &key_versions);
  if (!st.ok()) {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
    return;
  }
  std::string rtype1, rtype2, row, val;
  rtype2 = "";
  uint64_t c=0;
  uint64_t s1=0,s2=0;

  long long count = 0;
  for (auto& key_version : key_versions) {
    InternalKey ikey(key_version.user_key, key_version.sequence,
                     static_cast<ValueType>(key_version.type));
    if (has_to_ && ikey.user_key() == to_) {
//getAllKeyVersions（）包含用户键为“to”的键，但idump具有
//传统上不包括这类钥匙。
      break;
    }
    ++count;
    int k;
    if (count_delim_) {
      rtype1 = "";
      s1=0;
      row = ikey.Encode().ToString();
      val = key_version.value;
      for(k=0;row[k]!='\x01' && row[k]!='\0';k++)
        s1++;
      for(k=0;val[k]!='\x01' && val[k]!='\0';k++)
        s1++;
      for(int j=0;row[j]!=delim_[0] && row[j]!='\0' && row[j]!='\x01';j++)
        rtype1+=row[j];
      if(rtype2.compare("") && rtype2.compare(rtype1)!=0) {
        fprintf(stdout,"%s => count:%lld\tsize:%lld\n",rtype2.c_str(),
            (long long)c,(long long)s2);
        c=1;
        s2=s1;
        rtype2 = rtype1;
      } else {
        c++;
        s2+=s1;
        rtype2=rtype1;
      }
    }

    if (!count_only_ && !count_delim_) {
      std::string key = ikey.DebugString(is_key_hex_);
      std::string value = Slice(key_version.value).ToString(is_value_hex_);
      std::cout << key << " => " << value << "\n";
    }

//如果转储了最大键数，则终止
    if (max_keys_ > 0 && count >= max_keys_) break;
  }
  if(count_delim_) {
    fprintf(stdout,"%s => count:%lld\tsize:%lld\n", rtype2.c_str(),
        (long long)c,(long long)s2);
  } else
  fprintf(stdout, "Internal keys in range: %lld\n", (long long) count);
}

const std::string DBDumperCommand::ARG_COUNT_ONLY = "count_only";
const std::string DBDumperCommand::ARG_COUNT_DELIM = "count_delim";
const std::string DBDumperCommand::ARG_STATS = "stats";
const std::string DBDumperCommand::ARG_TTL_BUCKET = "bucket";

DBDumperCommand::DBDumperCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, true,
                 BuildCmdLineOptions(
                     {ARG_TTL, ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX, ARG_FROM,
                      ARG_TO, ARG_MAX_KEYS, ARG_COUNT_ONLY, ARG_COUNT_DELIM,
                      ARG_STATS, ARG_TTL_START, ARG_TTL_END, ARG_TTL_BUCKET,
                      ARG_TIMESTAMP, ARG_PATH})),
      null_from_(true),
      null_to_(true),
      max_keys_(-1),
      count_only_(false),
      count_delim_(false),
      print_stats_(false) {
  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_FROM);
  if (itr != options.end()) {
    null_from_ = false;
    from_ = itr->second;
  }

  itr = options.find(ARG_TO);
  if (itr != options.end()) {
    null_to_ = false;
    to_ = itr->second;
  }

  itr = options.find(ARG_MAX_KEYS);
  if (itr != options.end()) {
    try {
#if defined(CYGWIN)
      max_keys_ = strtol(itr->second.c_str(), 0, 10);
#else
      max_keys_ = std::stoi(itr->second);
#endif
    } catch (const std::invalid_argument&) {
      exec_state_ = LDBCommandExecuteResult::Failed(ARG_MAX_KEYS +
                                                    " has an invalid value");
    } catch (const std::out_of_range&) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          ARG_MAX_KEYS + " has a value out-of-range");
    }
  }
  itr = options.find(ARG_COUNT_DELIM);
  if (itr != options.end()) {
    delim_ = itr->second;
    count_delim_ = true;
  } else {
    count_delim_ = IsFlagPresent(flags, ARG_COUNT_DELIM);
    delim_=".";
  }

  print_stats_ = IsFlagPresent(flags, ARG_STATS);
  count_only_ = IsFlagPresent(flags, ARG_COUNT_ONLY);

  if (is_key_hex_) {
    if (!null_from_) {
      from_ = HexToString(from_);
    }
    if (!null_to_) {
      to_ = HexToString(to_);
    }
  }

  itr = options.find(ARG_PATH);
  if (itr != options.end()) {
    path_ = itr->second;
  }
}

void DBDumperCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DBDumperCommand::Name());
  ret.append(HelpRangeCmdArgs());
  ret.append(" [--" + ARG_TTL + "]");
  ret.append(" [--" + ARG_MAX_KEYS + "=<N>]");
  ret.append(" [--" + ARG_TIMESTAMP + "]");
  ret.append(" [--" + ARG_COUNT_ONLY + "]");
  ret.append(" [--" + ARG_COUNT_DELIM + "=<char>]");
  ret.append(" [--" + ARG_STATS + "]");
  ret.append(" [--" + ARG_TTL_BUCKET + "=<N>]");
  ret.append(" [--" + ARG_TTL_START + "=<N>:- is inclusive]");
  ret.append(" [--" + ARG_TTL_END + "=<N>:- is exclusive]");
  ret.append(" [--" + ARG_PATH + "=<path_to_a_file>]");
  ret.append("\n");
}

/*
 *处理两个单独的案例：
 *
 *1）--指定了db-只需转储数据库。
 *
 *2）--指定路径-根据文件扩展名确定转储内容
 *要调用的函数。请注意，我们有意使用扩展名
 *并避免在重命名的假设下探测文件内容。
 *文件不是受支持的方案。
 *
 **/

void DBDumperCommand::DoCommand() {
  if (!db_) {
    assert(!path_.empty());
    std::string fileName = GetFileNameFromPath(path_);
    uint64_t number;
    FileType type;

    exec_state_ = LDBCommandExecuteResult::Succeed("");

    if (!ParseFileName(fileName, &number, &type)) {
      exec_state_ =
          LDBCommandExecuteResult::Failed("Can't parse file type: " + path_);
      return;
    }

    switch (type) {
      case kLogFile:
        /*pwalfile（路径x，/*打印头x/true，/*打印值x/true，
                    & Excel
        断裂；
      案例k表格文件：
        dumpsstfile（path_u，is_key_hex_u，/*显示_属性*/ true);

        break;
      case kDescriptorFile:
        /*pmanifestfile（path_ux，/*verbose_x/false，is_key_hex_ux，
                         /JSON1*/ false);

        break;
      default:
        exec_state_ = LDBCommandExecuteResult::Failed(
            "File type not supported: " + path_);
        break;
    }

  } else {
    DoDumpCommand();
  }
}

void DBDumperCommand::DoDumpCommand() {
  assert(nullptr != db_);
  assert(path_.empty());

//分析命令行参数
  uint64_t count = 0;
  if (print_stats_) {
    std::string stats;
    if (db_->GetProperty("rocksdb.stats", &stats)) {
      fprintf(stdout, "%s\n", stats.c_str());
    }
  }

//设置密钥迭代器
  Iterator* iter = db_->NewIterator(ReadOptions(), GetCfHandle());
  Status st = iter->status();
  if (!st.ok()) {
    exec_state_ =
        LDBCommandExecuteResult::Failed("Iterator error." + st.ToString());
  }

  if (!null_from_) {
    iter->Seek(from_);
  } else {
    iter->SeekToFirst();
  }

  int max_keys = max_keys_;
  int ttl_start;
  if (!ParseIntOption(option_map_, ARG_TTL_START, ttl_start, exec_state_)) {
ttl_start = DBWithTTLImpl::kMinTimestamp;  //TTL引入时间
  }
  int ttl_end;
  if (!ParseIntOption(option_map_, ARG_TTL_END, ttl_end, exec_state_)) {
ttl_end = DBWithTTLImpl::kMaxTimestamp;  //TTL功能允许的最大时间
  }
  if (ttl_end < ttl_start) {
    fprintf(stderr, "Error: End time can't be less than start time\n");
    delete iter;
    return;
  }
  int time_range = ttl_end - ttl_start;
  int bucket_size;
  if (!ParseIntOption(option_map_, ARG_TTL_BUCKET, bucket_size, exec_state_) ||
      bucket_size <= 0) {
bucket_size = time_range; //默认只有一个桶
  }
//为每种类型的行计数创建变量
  std::string rtype1, rtype2, row, val;
  rtype2 = "";
  uint64_t c=0;
  uint64_t s1=0,s2=0;

//此时，bucket_size=0=>time_range=0
  int num_buckets = (bucket_size >= time_range)
                        ? 1
                        : ((time_range + bucket_size - 1) / bucket_size);
  std::vector<uint64_t> bucket_counts(num_buckets, 0);
  if (is_db_ttl_ && !count_only_ && timestamp_ && !count_delim_) {
    fprintf(stdout, "Dumping key-values from %s to %s\n",
            ReadableTime(ttl_start).c_str(), ReadableTime(ttl_end).c_str());
  }

  for (; iter->Valid(); iter->Next()) {
    int rawtime = 0;
//如果指定了结束标记，我们将在它之前停止
    if (!null_to_ && (iter->key().ToString() >= to_))
      break;
//如果转储了最大键数，则终止
    if (max_keys == 0)
      break;
    if (is_db_ttl_) {
      TtlIterator* it_ttl = static_cast_with_check<TtlIterator, Iterator>(iter);
      rawtime = it_ttl->timestamp();
      if (rawtime < ttl_start || rawtime >= ttl_end) {
        continue;
      }
    }
    if (max_keys > 0) {
      --max_keys;
    }
    if (is_db_ttl_ && num_buckets > 1) {
      IncBucketCounts(bucket_counts, ttl_start, time_range, bucket_size,
                      rawtime, num_buckets);
    }
    ++count;
    if (count_delim_) {
      rtype1 = "";
      row = iter->key().ToString();
      val = iter->value().ToString();
      s1 = row.size()+val.size();
      for(int j=0;row[j]!=delim_[0] && row[j]!='\0';j++)
        rtype1+=row[j];
      if(rtype2.compare("") && rtype2.compare(rtype1)!=0) {
        fprintf(stdout,"%s => count:%lld\tsize:%lld\n",rtype2.c_str(),
            (long long )c,(long long)s2);
        c=1;
        s2=s1;
        rtype2 = rtype1;
      } else {
          c++;
          s2+=s1;
          rtype2=rtype1;
      }

    }



    if (!count_only_ && !count_delim_) {
      if (is_db_ttl_ && timestamp_) {
        fprintf(stdout, "%s ", ReadableTime(rawtime).c_str());
      }
      std::string str =
          PrintKeyValue(iter->key().ToString(), iter->value().ToString(),
                        is_key_hex_, is_value_hex_);
      fprintf(stdout, "%s\n", str.c_str());
    }
  }

  if (num_buckets > 1 && is_db_ttl_) {
    PrintBucketCounts(bucket_counts, ttl_start, ttl_end, bucket_size,
                      num_buckets);
  } else if(count_delim_) {
    fprintf(stdout,"%s => count:%lld\tsize:%lld\n",rtype2.c_str(),
        (long long )c,(long long)s2);
  } else {
    fprintf(stdout, "Keys in range: %lld\n", (long long) count);
  }
//清理
  delete iter;
}

const std::string ReduceDBLevelsCommand::ARG_NEW_LEVELS = "new_levels";
const std::string ReduceDBLevelsCommand::ARG_PRINT_OLD_LEVELS =
    "print_old_levels";

ReduceDBLevelsCommand::ReduceDBLevelsCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_NEW_LEVELS, ARG_PRINT_OLD_LEVELS})),
      old_levels_(1 << 7),
      new_levels_(-1),
      print_old_levels_(false) {
  ParseIntOption(option_map_, ARG_NEW_LEVELS, new_levels_, exec_state_);
  print_old_levels_ = IsFlagPresent(flags, ARG_PRINT_OLD_LEVELS);

  if(new_levels_ <= 0) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        " Use --" + ARG_NEW_LEVELS + " to specify a new level number\n");
  }
}

std::vector<std::string> ReduceDBLevelsCommand::PrepareArgs(
    const std::string& db_path, int new_levels, bool print_old_level) {
  std::vector<std::string> ret;
  ret.push_back("reduce_levels");
  ret.push_back("--" + ARG_DB + "=" + db_path);
  ret.push_back("--" + ARG_NEW_LEVELS + "=" + rocksdb::ToString(new_levels));
  if(print_old_level) {
    ret.push_back("--" + ARG_PRINT_OLD_LEVELS);
  }
  return ret;
}

void ReduceDBLevelsCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ReduceDBLevelsCommand::Name());
  ret.append(" --" + ARG_NEW_LEVELS + "=<New number of levels>");
  ret.append(" [--" + ARG_PRINT_OLD_LEVELS + "]");
  ret.append("\n");
}

Options ReduceDBLevelsCommand::PrepareOptionsForOpenDB() {
  Options opt = LDBCommand::PrepareOptionsForOpenDB();
  opt.num_levels = old_levels_;
  opt.max_bytes_for_level_multiplier_additional.resize(opt.num_levels, 1);
//禁用大小压缩
  opt.max_bytes_for_level_base = 1ULL << 50;
  opt.max_bytes_for_level_multiplier = 1;
  return opt;
}

Status ReduceDBLevelsCommand::GetOldNumOfLevels(Options& opt,
    int* levels) {
  ImmutableDBOptions db_options(opt);
  EnvOptions soptions;
  std::shared_ptr<Cache> tc(
      NewLRUCache(opt.max_open_files - 10, opt.table_cache_numshardbits));
  const InternalKeyComparator cmp(opt.comparator);
  WriteController wc(opt.delayed_write_rate);
  WriteBufferManager wb(opt.db_write_buffer_size);
  VersionSet versions(db_path_, &db_options, soptions, tc.get(), &wb, &wc);
  std::vector<ColumnFamilyDescriptor> dummy;
  ColumnFamilyDescriptor dummy_descriptor(kDefaultColumnFamilyName,
                                          ColumnFamilyOptions(opt));
  dummy.push_back(dummy_descriptor);
//我们依靠versionset:：recover来告诉我们内部数据结构
//在数据库中。recover（）不应该做任何更改
//（如日志和应用程序）到清单文件。
  Status st = versions.Recover(dummy);
  if (!st.ok()) {
    return st;
  }
  int max = -1;
  auto default_cfd = versions.GetColumnFamilySet()->GetDefault();
  for (int i = 0; i < default_cfd->NumberLevels(); i++) {
    if (default_cfd->current()->storage_info()->NumLevelFiles(i)) {
      max = i;
    }
  }

  *levels = max + 1;
  return st;
}

void ReduceDBLevelsCommand::DoCommand() {
  if (new_levels_ <= 1) {
    exec_state_ =
        LDBCommandExecuteResult::Failed("Invalid number of levels.\n");
    return;
  }

  Status st;
  Options opt = PrepareOptionsForOpenDB();
  int old_level_num = -1;
  st = GetOldNumOfLevels(opt, &old_level_num);
  if (!st.ok()) {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
    return;
  }

  if (print_old_levels_) {
    fprintf(stdout, "The old number of levels in use is %d\n", old_level_num);
  }

  if (old_level_num <= new_levels_) {
    return;
  }

  old_levels_ = old_level_num;

  OpenDB();
  if (exec_state_.IsFailed()) {
    return;
  }
//压缩整个数据库以将所有文件放到最高级别。
  fprintf(stdout, "Compacting the db...\n");
  db_->CompactRange(CompactRangeOptions(), GetCfHandle(), nullptr, nullptr);
  CloseDB();

  EnvOptions soptions;
  st = VersionSet::ReduceNumberOfLevels(db_path_, &opt, soptions, new_levels_);
  if (!st.ok()) {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
    return;
  }
}

const std::string ChangeCompactionStyleCommand::ARG_OLD_COMPACTION_STYLE =
    "old_compaction_style";
const std::string ChangeCompactionStyleCommand::ARG_NEW_COMPACTION_STYLE =
    "new_compaction_style";

ChangeCompactionStyleCommand::ChangeCompactionStyleCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions(
                     {ARG_OLD_COMPACTION_STYLE, ARG_NEW_COMPACTION_STYLE})),
      old_compaction_style_(-1),
      new_compaction_style_(-1) {
  ParseIntOption(option_map_, ARG_OLD_COMPACTION_STYLE, old_compaction_style_,
    exec_state_);
  if (old_compaction_style_ != kCompactionStyleLevel &&
     old_compaction_style_ != kCompactionStyleUniversal) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Use --" + ARG_OLD_COMPACTION_STYLE + " to specify old compaction " +
        "style. Check ldb help for proper compaction style value.\n");
    return;
  }

  ParseIntOption(option_map_, ARG_NEW_COMPACTION_STYLE, new_compaction_style_,
    exec_state_);
  if (new_compaction_style_ != kCompactionStyleLevel &&
     new_compaction_style_ != kCompactionStyleUniversal) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Use --" + ARG_NEW_COMPACTION_STYLE + " to specify new compaction " +
        "style. Check ldb help for proper compaction style value.\n");
    return;
  }

  if (new_compaction_style_ == old_compaction_style_) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Old compaction style is the same as new compaction style. "
        "Nothing to do.\n");
    return;
  }

  if (old_compaction_style_ == kCompactionStyleUniversal &&
      new_compaction_style_ == kCompactionStyleLevel) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Convert from universal compaction to level compaction. "
        "Nothing to do.\n");
    return;
  }
}

void ChangeCompactionStyleCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ChangeCompactionStyleCommand::Name());
  ret.append(" --" + ARG_OLD_COMPACTION_STYLE + "=<Old compaction style: 0 " +
             "for level compaction, 1 for universal compaction>");
  ret.append(" --" + ARG_NEW_COMPACTION_STYLE + "=<New compaction style: 0 " +
             "for level compaction, 1 for universal compaction>");
  ret.append("\n");
}

Options ChangeCompactionStyleCommand::PrepareOptionsForOpenDB() {
  Options opt = LDBCommand::PrepareOptionsForOpenDB();

  if (old_compaction_style_ == kCompactionStyleLevel &&
      new_compaction_style_ == kCompactionStyleUniversal) {
//为了将水平压实转换为通用压实，我们
//需要将所有数据压缩到一个文件中并将其移动到级别0。
    opt.disable_auto_compactions = true;
    opt.target_file_size_base = INT_MAX;
    opt.target_file_size_multiplier = 1;
    opt.max_bytes_for_level_base = INT_MAX;
    opt.max_bytes_for_level_multiplier = 1;
  }

  return opt;
}

void ChangeCompactionStyleCommand::DoCommand() {
//在进行任何更改之前打印数据库统计信息
  std::string property;
  std::string files_per_level;
  for (int i = 0; i < db_->NumberLevels(GetCfHandle()); i++) {
    db_->GetProperty(GetCfHandle(),
                     "rocksdb.num-files-at-level" + NumberToString(i),
                     &property);

//设置打印字符串格式
    char buf[100];
    snprintf(buf, sizeof(buf), "%s%s", (i ? "," : ""), property.c_str());
    files_per_level += buf;
  }
  fprintf(stdout, "files per level before compaction: %s\n",
          files_per_level.c_str());

//手动压缩为单个文件并将文件移动到0级
  CompactRangeOptions compact_options;
  compact_options.change_level = true;
  compact_options.target_level = 0;
  db_->CompactRange(compact_options, GetCfHandle(), nullptr, nullptr);

//验证压实结果
  files_per_level = "";
  int num_files = 0;
  for (int i = 0; i < db_->NumberLevels(GetCfHandle()); i++) {
    db_->GetProperty(GetCfHandle(),
                     "rocksdb.num-files-at-level" + NumberToString(i),
                     &property);

//设置打印字符串格式
    char buf[100];
    snprintf(buf, sizeof(buf), "%s%s", (i ? "," : ""), property.c_str());
    files_per_level += buf;

    num_files = atoi(property.c_str());

//级别0应该只有1个文件
    if (i == 0 && num_files != 1) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "Number of db files at "
          "level 0 after compaction is " +
          ToString(num_files) + ", not 1.\n");
      return;
    }
//其他级别应该没有文件
    if (i > 0 && num_files != 0) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          "Number of db files at "
          "level " +
          ToString(i) + " after compaction is " + ToString(num_files) +
          ", not 0.\n");
      return;
    }
  }

  fprintf(stdout, "files per level after compaction: %s\n",
          files_per_level.c_str());
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace {

struct StdErrReporter : public log::Reader::Reporter {
  virtual void Corruption(size_t bytes, const Status& s) override {
    std::cerr << "Corruption detected in log file " << s.ToString() << "\n";
  }
};

class InMemoryHandler : public WriteBatch::Handler {
 public:
  InMemoryHandler(std::stringstream& row, bool print_values)
      : Handler(), row_(row) {
    print_values_ = print_values;
  }

  void commonPutMerge(const Slice& key, const Slice& value) {
    std::string k = LDBCommand::StringToHex(key.ToString());
    if (print_values_) {
      std::string v = LDBCommand::StringToHex(value.ToString());
      row_ << k << " : ";
      row_ << v << " ";
    } else {
      row_ << k << " ";
    }
  }

  virtual Status PutCF(uint32_t cf, const Slice& key,
                       const Slice& value) override {
    row_ << "PUT(" << cf << ") : ";
    commonPutMerge(key, value);
    return Status::OK();
  }

  virtual Status MergeCF(uint32_t cf, const Slice& key,
                         const Slice& value) override {
    row_ << "MERGE(" << cf << ") : ";
    commonPutMerge(key, value);
    return Status::OK();
  }

  virtual Status DeleteCF(uint32_t cf, const Slice& key) override {
    row_ << "DELETE(" << cf << ") : ";
    row_ << LDBCommand::StringToHex(key.ToString()) << " ";
    return Status::OK();
  }

  virtual Status SingleDeleteCF(uint32_t cf, const Slice& key) override {
    row_ << "SINGLE_DELETE(" << cf << ") : ";
    row_ << LDBCommand::StringToHex(key.ToString()) << " ";
    return Status::OK();
  }

  virtual Status DeleteRangeCF(uint32_t cf, const Slice& begin_key,
                               const Slice& end_key) override {
    row_ << "DELETE_RANGE(" << cf << ") : ";
    row_ << LDBCommand::StringToHex(begin_key.ToString()) << " ";
    row_ << LDBCommand::StringToHex(end_key.ToString()) << " ";
    return Status::OK();
  }

  virtual Status MarkBeginPrepare() override {
    row_ << "BEGIN_PREARE ";
    return Status::OK();
  }

  virtual Status MarkEndPrepare(const Slice& xid) override {
    row_ << "END_PREPARE(";
    row_ << LDBCommand::StringToHex(xid.ToString()) << ") ";
    return Status::OK();
  }

  virtual Status MarkRollback(const Slice& xid) override {
    row_ << "ROLLBACK(";
    row_ << LDBCommand::StringToHex(xid.ToString()) << ") ";
    return Status::OK();
  }

  virtual Status MarkCommit(const Slice& xid) override {
    row_ << "COMMIT(";
    row_ << LDBCommand::StringToHex(xid.ToString()) << ") ";
    return Status::OK();
  }

  virtual ~InMemoryHandler() {}

 private:
  std::stringstream& row_;
  bool print_values_;
};

void DumpWalFile(std::string wal_file, bool print_header, bool print_values,
                 LDBCommandExecuteResult* exec_state) {
  Env* env_ = Env::Default();
  EnvOptions soptions;
  unique_ptr<SequentialFileReader> wal_file_reader;

  Status status;
  {
    unique_ptr<SequentialFile> file;
    status = env_->NewSequentialFile(wal_file, &file, soptions);
    if (status.ok()) {
      wal_file_reader.reset(new SequentialFileReader(std::move(file)));
    }
  }
  if (!status.ok()) {
    if (exec_state) {
      *exec_state = LDBCommandExecuteResult::Failed("Failed to open WAL file " +
                                                    status.ToString());
    } else {
      std::cerr << "Error: Failed to open WAL file " << status.ToString()
                << std::endl;
    }
  } else {
    StdErrReporter reporter;
    uint64_t log_number;
    FileType type;

//我们需要日志号，但parseFileName需要dbname/nnn.log。
    std::string sanitized = wal_file;
    size_t lastslash = sanitized.rfind('/');
    if (lastslash != std::string::npos)
      sanitized = sanitized.substr(lastslash + 1);
    if (!ParseFileName(sanitized, &log_number, &type)) {
//虚假输入，尽我们所能继续下去
      log_number = 0;
    }
    DBOptions db_options;
    log::Reader reader(db_options.info_log, std::move(wal_file_reader),
                       &reporter, true, 0, log_number);
    std::string scratch;
    WriteBatch batch;
    Slice record;
    std::stringstream row;
    if (print_header) {
      std::cout << "Sequence,Count,ByteSize,Physical Offset,Key(s)";
      if (print_values) {
        std::cout << " : value ";
      }
      std::cout << "\n";
    }
    while (reader.ReadRecord(&record, &scratch)) {
      row.str("");
      if (record.size() < WriteBatchInternal::kHeader) {
        reporter.Corruption(record.size(),
                            Status::Corruption("log record too small"));
      } else {
        WriteBatchInternal::SetContents(&batch, record);
        row << WriteBatchInternal::Sequence(&batch) << ",";
        row << WriteBatchInternal::Count(&batch) << ",";
        row << WriteBatchInternal::ByteSize(&batch) << ",";
        row << reader.LastRecordOffset() << ",";
        InMemoryHandler handler(row, print_values);
        batch.Iterate(&handler);
        row << "\n";
      }
      std::cout << row.str();
    }
  }
}

}  //命名空间

const std::string WALDumperCommand::ARG_WAL_FILE = "walfile";
const std::string WALDumperCommand::ARG_PRINT_VALUE = "print_value";
const std::string WALDumperCommand::ARG_PRINT_HEADER = "header";

WALDumperCommand::WALDumperCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, true,
                 BuildCmdLineOptions(
                     {ARG_WAL_FILE, ARG_PRINT_HEADER, ARG_PRINT_VALUE})),
      print_header_(false),
      print_values_(false) {
  wal_file_.clear();

  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_WAL_FILE);
  if (itr != options.end()) {
    wal_file_ = itr->second;
  }


  print_header_ = IsFlagPresent(flags, ARG_PRINT_HEADER);
  print_values_ = IsFlagPresent(flags, ARG_PRINT_VALUE);
  if (wal_file_.empty()) {
    exec_state_ = LDBCommandExecuteResult::Failed("Argument " + ARG_WAL_FILE +
                                                  " must be specified.");
  }
}

void WALDumperCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(WALDumperCommand::Name());
  ret.append(" --" + ARG_WAL_FILE + "=<write_ahead_log_file_path>");
  ret.append(" [--" + ARG_PRINT_HEADER + "] ");
  ret.append(" [--" + ARG_PRINT_VALUE + "] ");
  ret.append("\n");
}

void WALDumperCommand::DoCommand() {
  DumpWalFile(wal_file_, print_header_, print_values_, &exec_state_);
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

GetCommand::GetCommand(const std::vector<std::string>& params,
                       const std::map<std::string, std::string>& options,
                       const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, true,
          BuildCmdLineOptions({ARG_TTL, ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX})) {
  if (params.size() != 1) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "<key> must be specified for the get command");
  } else {
    key_ = params.at(0);
  }

  if (is_key_hex_) {
    key_ = HexToString(key_);
  }
}

void GetCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(GetCommand::Name());
  ret.append(" <key>");
  ret.append(" [--" + ARG_TTL + "]");
  ret.append("\n");
}

void GetCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  std::string value;
  Status st = db_->Get(ReadOptions(), GetCfHandle(), key_, &value);
  if (st.ok()) {
    fprintf(stdout, "%s\n",
              (is_value_hex_ ? StringToHex(value) : value).c_str());
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

ApproxSizeCommand::ApproxSizeCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, true,
                 BuildCmdLineOptions(
                     {ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX, ARG_FROM, ARG_TO})) {
  if (options.find(ARG_FROM) != options.end()) {
    start_key_ = options.find(ARG_FROM)->second;
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(
        ARG_FROM + " must be specified for approxsize command");
    return;
  }

  if (options.find(ARG_TO) != options.end()) {
    end_key_ = options.find(ARG_TO)->second;
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(
        ARG_TO + " must be specified for approxsize command");
    return;
  }

  if (is_key_hex_) {
    start_key_ = HexToString(start_key_);
    end_key_ = HexToString(end_key_);
  }
}

void ApproxSizeCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ApproxSizeCommand::Name());
  ret.append(HelpRangeCmdArgs());
  ret.append("\n");
}

void ApproxSizeCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  Range ranges[1];
  ranges[0] = Range(start_key_, end_key_);
  uint64_t sizes[1];
  db_->GetApproximateSizes(GetCfHandle(), ranges, 1, sizes);
  fprintf(stdout, "%lu\n", (unsigned long)sizes[0]);
  /*奇怪的是getapproxidesizes（）返回了void，尽管文档
   *表示返回状态对象。
  如果（！）圣奥克（））{
    exec_state_u=ldbcommandexecuteresult:：failed（st.toString（））；
  }
  **/

}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

BatchPutCommand::BatchPutCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_TTL, ARG_HEX, ARG_KEY_HEX,
                                      ARG_VALUE_HEX, ARG_CREATE_IF_MISSING})) {
  if (params.size() < 2) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "At least one <key> <value> pair must be specified batchput.");
  } else if (params.size() % 2 != 0) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "Equal number of <key>s and <value>s must be specified for batchput.");
  } else {
    for (size_t i = 0; i < params.size(); i += 2) {
      std::string key = params.at(i);
      std::string value = params.at(i + 1);
      key_values_.push_back(std::pair<std::string, std::string>(
          is_key_hex_ ? HexToString(key) : key,
          is_value_hex_ ? HexToString(value) : value));
    }
  }
  create_if_missing_ = IsFlagPresent(flags_, ARG_CREATE_IF_MISSING);
}

void BatchPutCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(BatchPutCommand::Name());
  ret.append(" <key> <value> [<key> <value>] [..]");
  ret.append(" [--" + ARG_TTL + "]");
  ret.append("\n");
}

void BatchPutCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  WriteBatch batch;

  for (std::vector<std::pair<std::string, std::string>>::const_iterator itr =
           key_values_.begin();
       itr != key_values_.end(); ++itr) {
    batch.Put(GetCfHandle(), itr->first, itr->second);
  }
  Status st = db_->Write(WriteOptions(), &batch);
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

Options BatchPutCommand::PrepareOptionsForOpenDB() {
  Options opt = LDBCommand::PrepareOptionsForOpenDB();
  opt.create_if_missing = create_if_missing_;
  return opt;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

ScanCommand::ScanCommand(const std::vector<std::string>& params,
                         const std::map<std::string, std::string>& options,
                         const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, true,
          BuildCmdLineOptions({ARG_TTL, ARG_NO_VALUE, ARG_HEX, ARG_KEY_HEX,
                               ARG_TO, ARG_VALUE_HEX, ARG_FROM, ARG_TIMESTAMP,
                               ARG_MAX_KEYS, ARG_TTL_START, ARG_TTL_END})),
      start_key_specified_(false),
      end_key_specified_(false),
      max_keys_scanned_(-1),
      no_value_(false) {
  std::map<std::string, std::string>::const_iterator itr =
      options.find(ARG_FROM);
  if (itr != options.end()) {
    start_key_ = itr->second;
    if (is_key_hex_) {
      start_key_ = HexToString(start_key_);
    }
    start_key_specified_ = true;
  }
  itr = options.find(ARG_TO);
  if (itr != options.end()) {
    end_key_ = itr->second;
    if (is_key_hex_) {
      end_key_ = HexToString(end_key_);
    }
    end_key_specified_ = true;
  }

  std::vector<std::string>::const_iterator vitr =
      std::find(flags.begin(), flags.end(), ARG_NO_VALUE);
  if (vitr != flags.end()) {
    no_value_ = true;
  }

  itr = options.find(ARG_MAX_KEYS);
  if (itr != options.end()) {
    try {
#if defined(CYGWIN)
      max_keys_scanned_ = strtol(itr->second.c_str(), 0, 10);
#else
      max_keys_scanned_ = std::stoi(itr->second);
#endif
    } catch (const std::invalid_argument&) {
      exec_state_ = LDBCommandExecuteResult::Failed(ARG_MAX_KEYS +
                                                    " has an invalid value");
    } catch (const std::out_of_range&) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          ARG_MAX_KEYS + " has a value out-of-range");
    }
  }
}

void ScanCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(ScanCommand::Name());
  ret.append(HelpRangeCmdArgs());
  ret.append(" [--" + ARG_TTL + "]");
  ret.append(" [--" + ARG_TIMESTAMP + "]");
  ret.append(" [--" + ARG_MAX_KEYS + "=<N>q] ");
  ret.append(" [--" + ARG_TTL_START + "=<N>:- is inclusive]");
  ret.append(" [--" + ARG_TTL_END + "=<N>:- is exclusive]");
  ret.append(" [--" + ARG_NO_VALUE + "]");
  ret.append("\n");
}

void ScanCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }

  int num_keys_scanned = 0;
  Iterator* it = db_->NewIterator(ReadOptions(), GetCfHandle());
  if (start_key_specified_) {
    it->Seek(start_key_);
  } else {
    it->SeekToFirst();
  }
  int ttl_start;
  if (!ParseIntOption(option_map_, ARG_TTL_START, ttl_start, exec_state_)) {
ttl_start = DBWithTTLImpl::kMinTimestamp;  //TTL引入时间
  }
  int ttl_end;
  if (!ParseIntOption(option_map_, ARG_TTL_END, ttl_end, exec_state_)) {
ttl_end = DBWithTTLImpl::kMaxTimestamp;  //TTL功能允许的最大时间
  }
  if (ttl_end < ttl_start) {
    fprintf(stderr, "Error: End time can't be less than start time\n");
    delete it;
    return;
  }
  if (is_db_ttl_ && timestamp_) {
    fprintf(stdout, "Scanning key-values from %s to %s\n",
            ReadableTime(ttl_start).c_str(), ReadableTime(ttl_end).c_str());
  }
  for ( ;
        it->Valid() && (!end_key_specified_ || it->key().ToString() < end_key_);
        it->Next()) {
    if (is_db_ttl_) {
      TtlIterator* it_ttl = static_cast_with_check<TtlIterator, Iterator>(it);
      int rawtime = it_ttl->timestamp();
      if (rawtime < ttl_start || rawtime >= ttl_end) {
        continue;
      }
      if (timestamp_) {
        fprintf(stdout, "%s ", ReadableTime(rawtime).c_str());
      }
    }

    Slice key_slice = it->key();

    std::string formatted_key;
    if (is_key_hex_) {
      /*matted_key=“0x”+key_slice.to字符串（true/*hex*/）；
      key_slice=格式化的_key；
    其他if（ldb_选项_u.key_格式化程序）
      格式化的_key=ldb_options_u.key_formatter->format（key_slice）；
      key_slice=格式化的_key；
    }

    如果（无价值）
      fprintf（stdout，“%*s\n”，static_cast<int>（key_slice.size（）），
              key_slice.data（））；
    }否则{
      slice val_slice=it->value（）；
      std：：字符串格式的_值；
      if（is_value_hex_
        格式化的_value=“0x”+val_slice.toString（true/*hex*/);

        val_slice = formatted_value;
      }
      fprintf(stdout, "%.*s : %.*s\n", static_cast<int>(key_slice.size()),
              key_slice.data(), static_cast<int>(val_slice.size()),
              val_slice.data());
    }

    num_keys_scanned++;
    if (max_keys_scanned_ >= 0 && num_keys_scanned >= max_keys_scanned_) {
      break;
    }
  }
if (!it->status().ok()) {  //检查扫描过程中是否发现任何错误
    exec_state_ = LDBCommandExecuteResult::Failed(it->status().ToString());
  }
  delete it;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

DeleteCommand::DeleteCommand(const std::vector<std::string>& params,
                             const std::map<std::string, std::string>& options,
                             const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX})) {
  if (params.size() != 1) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "KEY must be specified for the delete command");
  } else {
    key_ = params.at(0);
    if (is_key_hex_) {
      key_ = HexToString(key_);
    }
  }
}

void DeleteCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DeleteCommand::Name() + " <key>");
  ret.append("\n");
}

void DeleteCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  Status st = db_->Delete(WriteOptions(), GetCfHandle(), key_);
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

DeleteRangeCommand::DeleteRangeCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX})) {
  if (params.size() != 2) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "begin and end keys must be specified for the delete command");
  } else {
    begin_key_ = params.at(0);
    end_key_ = params.at(1);
    if (is_key_hex_) {
      begin_key_ = HexToString(begin_key_);
      end_key_ = HexToString(end_key_);
    }
  }
}

void DeleteRangeCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DeleteRangeCommand::Name() + " <begin key> <end key>");
  ret.append("\n");
}

void DeleteRangeCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  Status st =
      db_->DeleteRange(WriteOptions(), GetCfHandle(), begin_key_, end_key_);
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

PutCommand::PutCommand(const std::vector<std::string>& params,
                       const std::map<std::string, std::string>& options,
                       const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false,
                 BuildCmdLineOptions({ARG_TTL, ARG_HEX, ARG_KEY_HEX,
                                      ARG_VALUE_HEX, ARG_CREATE_IF_MISSING})) {
  if (params.size() != 2) {
    exec_state_ = LDBCommandExecuteResult::Failed(
        "<key> and <value> must be specified for the put command");
  } else {
    key_ = params.at(0);
    value_ = params.at(1);
  }

  if (is_key_hex_) {
    key_ = HexToString(key_);
  }

  if (is_value_hex_) {
    value_ = HexToString(value_);
  }
  create_if_missing_ = IsFlagPresent(flags_, ARG_CREATE_IF_MISSING);
}

void PutCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(PutCommand::Name());
  ret.append(" <key> <value> ");
  ret.append(" [--" + ARG_TTL + "]");
  ret.append("\n");
}

void PutCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  Status st = db_->Put(WriteOptions(), GetCfHandle(), key_, value_);
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

Options PutCommand::PrepareOptionsForOpenDB() {
  Options opt = LDBCommand::PrepareOptionsForOpenDB();
  opt.create_if_missing = create_if_missing_;
  return opt;
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

const char* DBQuerierCommand::HELP_CMD = "help";
const char* DBQuerierCommand::GET_CMD = "get";
const char* DBQuerierCommand::PUT_CMD = "put";
const char* DBQuerierCommand::DELETE_CMD = "delete";

DBQuerierCommand::DBQuerierCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(
          options, flags, false,
          BuildCmdLineOptions({ARG_TTL, ARG_HEX, ARG_KEY_HEX, ARG_VALUE_HEX})) {

}

void DBQuerierCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DBQuerierCommand::Name());
  ret.append(" [--" + ARG_TTL + "]");
  ret.append("\n");
  ret.append("    Starts a REPL shell.  Type help for list of available "
             "commands.");
  ret.append("\n");
}

void DBQuerierCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }

  ReadOptions read_options;
  WriteOptions write_options;

  std::string line;
  std::string key;
  std::string value;
  while (getline(std::cin, line, '\n')) {
//将行解析为std:：vector<std:：string>
    std::vector<std::string> tokens;
    size_t pos = 0;
    while (true) {
      size_t pos2 = line.find(' ', pos);
      if (pos2 == std::string::npos) {
        break;
      }
      tokens.push_back(line.substr(pos, pos2-pos));
      pos = pos2 + 1;
    }
    tokens.push_back(line.substr(pos));

    const std::string& cmd = tokens[0];

    if (cmd == HELP_CMD) {
      fprintf(stdout,
              "get <key>\n"
              "put <key> <value>\n"
              "delete <key>\n");
    } else if (cmd == DELETE_CMD && tokens.size() == 2) {
      key = (is_key_hex_ ? HexToString(tokens[1]) : tokens[1]);
      db_->Delete(write_options, GetCfHandle(), Slice(key));
      fprintf(stdout, "Successfully deleted %s\n", tokens[1].c_str());
    } else if (cmd == PUT_CMD && tokens.size() == 3) {
      key = (is_key_hex_ ? HexToString(tokens[1]) : tokens[1]);
      value = (is_value_hex_ ? HexToString(tokens[2]) : tokens[2]);
      db_->Put(write_options, GetCfHandle(), Slice(key), Slice(value));
      fprintf(stdout, "Successfully put %s %s\n",
              tokens[1].c_str(), tokens[2].c_str());
    } else if (cmd == GET_CMD && tokens.size() == 2) {
      key = (is_key_hex_ ? HexToString(tokens[1]) : tokens[1]);
      if (db_->Get(read_options, GetCfHandle(), Slice(key), &value).ok()) {
        fprintf(stdout, "%s\n", PrintKeyValue(key, value,
              is_key_hex_, is_value_hex_).c_str());
      } else {
        fprintf(stdout, "Not found %s\n", tokens[1].c_str());
      }
    } else {
      fprintf(stdout, "Unknown command %s\n", line.c_str());
    }
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

CheckConsistencyCommand::CheckConsistencyCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, false, BuildCmdLineOptions({})) {}

void CheckConsistencyCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(CheckConsistencyCommand::Name());
  ret.append("\n");
}

void CheckConsistencyCommand::DoCommand() {
  Options opt = PrepareOptionsForOpenDB();
  opt.paranoid_checks = true;
  if (!exec_state_.IsNotStarted()) {
    return;
  }
  DB* db;
  Status st = DB::OpenForReadOnly(opt, db_path_, &db, false);
  delete db;
  if (st.ok()) {
    fprintf(stdout, "OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(st.ToString());
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

const std::string CheckPointCommand::ARG_CHECKPOINT_DIR = "checkpoint_dir";

CheckPointCommand::CheckPointCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    /*dbcommand（选项、标志、false/*为只读*/，
                 buildcmdlineoptions（arg_checkpoint_dir））
  auto itr=options.find（arg_checkpoint_dir）；
  if（itr==options.end（））
    exec_state_u=ldbcommandexecuteresult:：failed（
        “--”+arg_checkpoint_dir+“：缺少检查点目录”）；
  }否则{
    checkpoint_dir_=itr->second；
  }
}

void checkpointcommand:：help（std:：string&ret）
  ret.append（“”）；
  ret.append（checkpointCommand:：name（））；
  ret.append（“[--”+arg_checkpoint_dir+“]”）；
  ret.append（“\n”）；
}

void checkpointcommand:：docommand（）
  如果（！）dBi）{
    断言（getExecuteState（）.isFailed（））；
    返回；
  }
  检查点*检查点；
  状态状态=检查点：：创建（db_u，&checkpoint）；
  状态=检查点->创建检查点（检查点目录）；
  if（status.ok（））
    PROTF（“OK \ N”）；
  }否则{
    exec_state_u=ldbcommandexecuteresult:：failed（status.toString（））；
  }
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————

repaircommand:：repaircommand（const std:：vector<std:：string>和params，
                             const std:：map<std:：string，std:：string>选项（&O）
                             const std:：vector<std:：string>和flags）
    ：ldbcommand（选项、标志、假、buildcmdlineoptions（））

void repaircommand:：help（std:：string&ret）
  ret.append（“”）；
  ret.append（repairCommand:：name（））；
  ret.append（“\n”）；
}

void repairCommand:：docommand（）
  选项选项=PrepareOptionsForPropendB（）；
  options.info_log.reset（新建stderlogger（info log level:：warn_level））；
  status status=repairdb（db_path_uu，选项）；
  if（status.ok（））
    PROTF（“OK \ N”）；
  }否则{
    exec_state_u=ldbcommandexecuteresult:：failed（status.toString（））；
  }
}

//——————————————————————————————————————————————————————————————————————————————————————————————————————————————————

const std:：string backupablecommand:：arg_num_threads=“num_threads”；
const std:：string backupablecommand:：arg_backup_env_uri=“backup_env_uri”；
const std:：string backupablecommand:：arg_backup_dir=“backup_dir”；
const std:：string backupablecommand:：arg_stderr_log_level=“stderr_log_level”；

backupablecommand：：backupablecommand（）。
    常量std:：vector<std:：string>和参数，
    const std:：map<std:：string，std:：string>选项（&O）
    const std:：vector<std:：string>和flags）
    ：ldbcommand（选项、标志、假/*为只读*/,

                 BuildCmdLineOptions({ARG_BACKUP_ENV_URI, ARG_BACKUP_DIR,
                                      ARG_NUM_THREADS, ARG_STDERR_LOG_LEVEL})),
      num_threads_(1) {
  auto itr = options.find(ARG_NUM_THREADS);
  if (itr != options.end()) {
    num_threads_ = std::stoi(itr->second);
  }
  itr = options.find(ARG_BACKUP_ENV_URI);
  if (itr != options.end()) {
    backup_env_uri_ = itr->second;
  }
  itr = options.find(ARG_BACKUP_DIR);
  if (itr == options.end()) {
    exec_state_ = LDBCommandExecuteResult::Failed("--" + ARG_BACKUP_DIR +
                                                  ": missing backup directory");
  } else {
    backup_dir_ = itr->second;
  }

  itr = options.find(ARG_STDERR_LOG_LEVEL);
  if (itr != options.end()) {
    int stderr_log_level = std::stoi(itr->second);
    if (stderr_log_level < 0 ||
        stderr_log_level >= InfoLogLevel::NUM_INFO_LOG_LEVELS) {
      exec_state_ = LDBCommandExecuteResult::Failed(
          ARG_STDERR_LOG_LEVEL + " must be >= 0 and < " +
          std::to_string(InfoLogLevel::NUM_INFO_LOG_LEVELS) + ".");
    } else {
      logger_.reset(
          new StderrLogger(static_cast<InfoLogLevel>(stderr_log_level)));
    }
  }
}

void BackupableCommand::Help(const std::string& name, std::string& ret) {
  ret.append("  ");
  ret.append(name);
  ret.append(" [--" + ARG_BACKUP_ENV_URI + "] ");
  ret.append(" [--" + ARG_BACKUP_DIR + "] ");
  ret.append(" [--" + ARG_NUM_THREADS + "] ");
  ret.append(" [--" + ARG_STDERR_LOG_LEVEL + "=<int (InfoLogLevel)>] ");
  ret.append("\n");
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

BackupCommand::BackupCommand(const std::vector<std::string>& params,
                             const std::map<std::string, std::string>& options,
                             const std::vector<std::string>& flags)
    : BackupableCommand(params, options, flags) {}

void BackupCommand::Help(std::string& ret) {
  BackupableCommand::Help(Name(), ret);
}

void BackupCommand::DoCommand() {
  BackupEngine* backup_engine;
  Status status;
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  printf("open db OK\n");
  std::unique_ptr<Env> custom_env_guard;
  Env* custom_env = NewCustomObject<Env>(backup_env_uri_, &custom_env_guard);
  BackupableDBOptions backup_options =
      BackupableDBOptions(backup_dir_, custom_env);
  backup_options.info_log = logger_.get();
  backup_options.max_background_operations = num_threads_;
  status = BackupEngine::Open(Env::Default(), backup_options, &backup_engine);
  if (status.ok()) {
    printf("open backup engine OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(status.ToString());
    return;
  }
  status = backup_engine->CreateNewBackup(db_);
  if (status.ok()) {
    printf("create new backup OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(status.ToString());
    return;
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

RestoreCommand::RestoreCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : BackupableCommand(params, options, flags) {}

void RestoreCommand::Help(std::string& ret) {
  BackupableCommand::Help(Name(), ret);
}

void RestoreCommand::DoCommand() {
  std::unique_ptr<Env> custom_env_guard;
  Env* custom_env = NewCustomObject<Env>(backup_env_uri_, &custom_env_guard);
  std::unique_ptr<BackupEngineReadOnly> restore_engine;
  Status status;
  {
    BackupableDBOptions opts(backup_dir_, custom_env);
    opts.info_log = logger_.get();
    opts.max_background_operations = num_threads_;
    BackupEngineReadOnly* raw_restore_engine_ptr;
    status = BackupEngineReadOnly::Open(Env::Default(), opts,
                                        &raw_restore_engine_ptr);
    if (status.ok()) {
      restore_engine.reset(raw_restore_engine_ptr);
    }
  }
  if (status.ok()) {
    printf("open restore engine OK\n");
    status = restore_engine->RestoreDBFromLatestBackup(db_path_, db_path_);
  }
  if (status.ok()) {
    printf("restore from backup OK\n");
  } else {
    exec_state_ = LDBCommandExecuteResult::Failed(status.ToString());
  }
}

//————————————————————————————————————————————————————————————————————————————————————————————————————————————————

namespace {

void DumpSstFile(std::string filename, bool output_hex, bool show_properties) {
  std::string from_key;
  std::string to_key;
  if (filename.length() <= 4 ||
      filename.rfind(".sst") != filename.length() - 4) {
    std::cout << "Invalid sst file name." << std::endl;
    return;
  }
//不验证
  rocksdb::SstFileReader reader(filename, false, output_hex);
Status st = reader.ReadSequential(true, -1, false,  //哈斯卡
from_key, false,  //哈斯托
                                    to_key);
  if (!st.ok()) {
    std::cerr << "Error in reading SST file " << filename << st.ToString()
              << std::endl;
    return;
  }

  if (show_properties) {
    const rocksdb::TableProperties* table_properties;

    std::shared_ptr<const rocksdb::TableProperties>
        table_properties_from_reader;
    st = reader.ReadTableProperties(&table_properties_from_reader);
    if (!st.ok()) {
      std::cerr << filename << ": " << st.ToString()
                << ". Try to use initial table properties" << std::endl;
      table_properties = reader.GetInitTableProperties();
    } else {
      table_properties = table_properties_from_reader.get();
    }
    if (table_properties != nullptr) {
      std::cout << std::endl << "Table Properties:" << std::endl;
      std::cout << table_properties->ToString("\n") << std::endl;
      std::cout << "# deleted keys: "
                << rocksdb::GetDeletedKeys(
                       table_properties->user_collected_properties)
                << std::endl;
    }
  }
}

}  //命名空间

DBFileDumperCommand::DBFileDumperCommand(
    const std::vector<std::string>& params,
    const std::map<std::string, std::string>& options,
    const std::vector<std::string>& flags)
    : LDBCommand(options, flags, true, BuildCmdLineOptions({})) {}

void DBFileDumperCommand::Help(std::string& ret) {
  ret.append("  ");
  ret.append(DBFileDumperCommand::Name());
  ret.append("\n");
}

void DBFileDumperCommand::DoCommand() {
  if (!db_) {
    assert(GetExecuteState().IsFailed());
    return;
  }
  Status s;

  std::cout << "Manifest File" << std::endl;
  std::cout << "==============================" << std::endl;
  std::string manifest_filename;
  s = ReadFileToString(db_->GetEnv(), CurrentFileName(db_->GetName()),
                       &manifest_filename);
  if (!s.ok() || manifest_filename.empty() ||
      manifest_filename.back() != '\n') {
    std::cerr << "Error when reading CURRENT file "
              << CurrentFileName(db_->GetName()) << std::endl;
  }
//删除尾随的'\n'
  manifest_filename.resize(manifest_filename.size() - 1);
  std::string manifest_filepath = db_->GetName() + "/" + manifest_filename;
  std::cout << manifest_filepath << std::endl;
  DumpManifestFile(manifest_filepath, false, false, false);
  std::cout << std::endl;

  std::cout << "SST Files" << std::endl;
  std::cout << "==============================" << std::endl;
  std::vector<LiveFileMetaData> metadata;
  db_->GetLiveFilesMetaData(&metadata);
  for (auto& fileMetadata : metadata) {
    std::string filename = fileMetadata.db_path + fileMetadata.name;
    std::cout << filename << " level:" << fileMetadata.level << std::endl;
    std::cout << "------------------------------" << std::endl;
    DumpSstFile(filename, false, true);
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::cout << "Write Ahead Log Files" << std::endl;
  std::cout << "==============================" << std::endl;
  rocksdb::VectorLogPtr wal_files;
  s = db_->GetSortedWalFiles(wal_files);
  if (!s.ok()) {
    std::cerr << "Error when getting WAL files" << std::endl;
  } else {
    for (auto& wal : wal_files) {
//TODO（qyang）：应将option.wal_dir传递到ldb命令中
      std::string filename = db_->GetOptions().wal_dir + wal->PathName();
      std::cout << filename << std::endl;
      DumpWalFile(filename, true, true, &exec_state_);
    }
  }
}

}   //命名空间rocksdb
#endif  //摇滚乐
