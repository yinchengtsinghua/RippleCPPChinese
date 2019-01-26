
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
#pragma once

#ifndef ROCKSDB_LITE

#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/ldb_tool.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/utilities/db_ttl.h"
#include "rocksdb/utilities/ldb_cmd_execute_result.h"

namespace rocksdb {

class LDBCommand {
 public:
//命令行参数
  static const std::string ARG_DB;
  static const std::string ARG_PATH;
  static const std::string ARG_HEX;
  static const std::string ARG_KEY_HEX;
  static const std::string ARG_VALUE_HEX;
  static const std::string ARG_CF_NAME;
  static const std::string ARG_TTL;
  static const std::string ARG_TTL_START;
  static const std::string ARG_TTL_END;
  static const std::string ARG_TIMESTAMP;
  static const std::string ARG_TRY_LOAD_OPTIONS;
  static const std::string ARG_IGNORE_UNKNOWN_OPTIONS;
  static const std::string ARG_FROM;
  static const std::string ARG_TO;
  static const std::string ARG_MAX_KEYS;
  static const std::string ARG_BLOOM_BITS;
  static const std::string ARG_FIX_PREFIX_LEN;
  static const std::string ARG_COMPRESSION_TYPE;
  static const std::string ARG_COMPRESSION_MAX_DICT_BYTES;
  static const std::string ARG_BLOCK_SIZE;
  static const std::string ARG_AUTO_COMPACTION;
  static const std::string ARG_DB_WRITE_BUFFER_SIZE;
  static const std::string ARG_WRITE_BUFFER_SIZE;
  static const std::string ARG_FILE_SIZE;
  static const std::string ARG_CREATE_IF_MISSING;
  static const std::string ARG_NO_VALUE;

  struct ParsedParams {
    std::string cmd;
    std::vector<std::string> cmd_params;
    std::map<std::string, std::string> option_map;
    std::vector<std::string> flags;
  };

  static LDBCommand* SelectCommand(const ParsedParams& parsed_parms);

  static LDBCommand* InitFromCmdLineArgs(
      const std::vector<std::string>& args, const Options& options,
      const LDBOptions& ldb_options,
      const std::vector<ColumnFamilyDescriptor>* column_families,
      const std::function<LDBCommand*(const ParsedParams&)>& selector =
          SelectCommand);

  static LDBCommand* InitFromCmdLineArgs(
      int argc, char** argv, const Options& options,
      const LDBOptions& ldb_options,
      const std::vector<ColumnFamilyDescriptor>* column_families);

  bool ValidateCmdLineOptions();

  virtual Options PrepareOptionsForOpenDB();

  virtual void SetDBOptions(Options options) { options_ = options; }

  virtual void SetColumnFamilies(
      const std::vector<ColumnFamilyDescriptor>* column_families) {
    if (column_families != nullptr) {
      column_families_ = *column_families;
    } else {
      column_families_.clear();
    }
  }

  void SetLDBOptions(const LDBOptions& ldb_options) {
    ldb_options_ = ldb_options;
  }

  virtual bool NoDBOpen() { return false; }

  virtual ~LDBCommand() { CloseDB(); }

  /*运行该命令，并返回执行结果。*/
  void Run();

  virtual void DoCommand() = 0;

  LDBCommandExecuteResult GetExecuteState() { return exec_state_; }

  void ClearPreviousRunState() { exec_state_.Reset(); }

//如果不需要
//0x前缀
  static std::string HexToString(const std::string& str);

//考虑直接使用slice:：toString（true），如果
//您不需要0x前缀
  static std::string StringToHex(const std::string& str);

  static const char* DELIM;

 protected:
  LDBCommandExecuteResult exec_state_;
  std::string db_path_;
  std::string column_family_name_;
  DB* db_;
  DBWithTTL* db_ttl_;
  std::map<std::string, ColumnFamilyHandle*> cf_handles_;

  /*
   *true表示如果数据库以只读方式打开，则此命令可以工作。
   *模式。
   **/

  bool is_read_only_;

  /*如果为真，则键在get/put/scan/delete等中作为十六进制输入/输出。*/
  bool is_key_hex_;

  /*如果为真，则该值在GET/PUT/SCAN/DELETE等中作为十六进制输入/输出。*/
  bool is_value_hex_;

  /*如果为true，则将该值视为后缀为timestamp的值。*/
  bool is_db_ttl_;

//如果为真，则在TTL数据库中输出带有插入/修改时间戳的KVS。
  bool timestamp_;

//如果为true，则尝试从db的选项文件构造选项。
  bool try_load_options_;

  bool ignore_unknown_options_;

  bool create_if_missing_;

  /*
   *在命令行上传递的选项映射。
   **/

  const std::map<std::string, std::string> option_map_;

  /*
   *在命令行上传递的标志。
   **/

  const std::vector<std::string> flags_;

  /*对该命令有效的命令行选项列表*/
  const std::vector<std::string> valid_cmd_line_options_;

  bool ParseKeyValue(const std::string& line, std::string* key,
                     std::string* value, bool is_key_hex, bool is_value_hex);

  LDBCommand(const std::map<std::string, std::string>& options,
             const std::vector<std::string>& flags, bool is_read_only,
             const std::vector<std::string>& valid_cmd_line_options);

  void OpenDB();

  void CloseDB();

  ColumnFamilyHandle* GetCfHandle();

  static std::string PrintKeyValue(const std::string& key,
                                   const std::string& value, bool is_key_hex,
                                   bool is_value_hex);

  static std::string PrintKeyValue(const std::string& key,
                                   const std::string& value, bool is_hex);

  /*
   *如果指定标志矢量中存在指定标志，则返回true。
   **/

  static bool IsFlagPresent(const std::vector<std::string>& flags,
                            const std::string& flag) {
    return (std::find(flags.begin(), flags.end(), flag) != flags.end());
  }

  static std::string HelpRangeCmdArgs();

  /*
   *返回命令行选项列表的helper函数
   *用于此命令。它包括常用选项和
   *通过。
   **/

  static std::vector<std::string> BuildCmdLineOptions(
      std::vector<std::string> options);

  bool ParseIntOption(const std::map<std::string, std::string>& options,
                      const std::string& option, int& value,
                      LDBCommandExecuteResult& exec_state);

  bool ParseStringOption(const std::map<std::string, std::string>& options,
                         const std::string& option, std::string* value);

  Options options_;
  std::vector<ColumnFamilyDescriptor> column_families_;
  LDBOptions ldb_options_;

 private:
  /*
   *解释命令行选项和标志以确定键
   *应为十六进制输入/输出。
   **/

  bool IsKeyHex(const std::map<std::string, std::string>& options,
                const std::vector<std::string>& flags);

  /*
   *解释命令行选项和标志以确定值
   *应为十六进制输入/输出。
   **/

  bool IsValueHex(const std::map<std::string, std::string>& options,
                  const std::vector<std::string>& flags);

  /*
   *以布尔值的形式返回指定选项的值。
   *如果在选项中找不到该选项，则使用默认值。
   *如果选项的值不是
   *“真”或“假”（不区分大小写）。
   **/

  bool ParseBooleanOption(const std::map<std::string, std::string>& options,
                          const std::string& option, bool default_val);

  /*
   *将val转换为布尔值。
   *VAL必须为真或假（不区分大小写）。
   *否则会引发异常。
   **/

  bool StringToBool(std::string val);
};

class LDBCommandRunner {
 public:
  static void PrintHelp(const LDBOptions& ldb_options, const char* exec_name);

  static void RunCommand(
      int argc, char** argv, Options options, const LDBOptions& ldb_options,
      const std::vector<ColumnFamilyDescriptor>* column_families);
};

}  //命名空间rocksdb

#endif  //摇滚乐
