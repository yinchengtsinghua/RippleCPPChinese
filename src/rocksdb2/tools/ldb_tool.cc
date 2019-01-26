
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
#include "rocksdb/ldb_tool.h"
#include "rocksdb/utilities/ldb_cmd.h"
#include "tools/ldb_cmd_impl.h"

namespace rocksdb {

LDBOptions::LDBOptions() {}

void LDBCommandRunner::PrintHelp(const LDBOptions& ldb_options,
                                 const char* exec_name) {
  std::string ret;

  ret.append(ldb_options.print_help_header);
  ret.append("\n\n");
  ret.append("commands MUST specify --" + LDBCommand::ARG_DB +
             "=<full_path_to_db_directory> when necessary\n");
  ret.append("\n");
  ret.append(
      "The following optional parameters control if keys/values are "
      "input/output as hex or as plain strings:\n");
  ret.append("  --" + LDBCommand::ARG_KEY_HEX +
             " : Keys are input/output as hex\n");
  ret.append("  --" + LDBCommand::ARG_VALUE_HEX +
             " : Values are input/output as hex\n");
  ret.append("  --" + LDBCommand::ARG_HEX +
             " : Both keys and values are input/output as hex\n");
  ret.append("\n");

  ret.append(
      "The following optional parameters control the database "
      "internals:\n");
  ret.append(
      "  --" + LDBCommand::ARG_CF_NAME +
      "=<string> : name of the column family to operate on. default: default "
      "column family\n");
  ret.append("  --" + LDBCommand::ARG_TTL +
             " with 'put','get','scan','dump','query','batchput'"
             " : DB supports ttl and value is internally timestamp-suffixed\n");
  ret.append("  --" + LDBCommand::ARG_TRY_LOAD_OPTIONS +
             " : Try to load option file from DB.\n");
  ret.append("  --" + LDBCommand::ARG_IGNORE_UNKNOWN_OPTIONS +
             " : Ignore unknown options when loading option file.\n");
  ret.append("  --" + LDBCommand::ARG_BLOOM_BITS + "=<int,e.g.:14>\n");
  ret.append("  --" + LDBCommand::ARG_FIX_PREFIX_LEN + "=<int,e.g.:14>\n");
  ret.append("  --" + LDBCommand::ARG_COMPRESSION_TYPE +
             "=<no|snappy|zlib|bzip2|lz4|lz4hc|xpress|zstd>\n");
  ret.append("  --" + LDBCommand::ARG_COMPRESSION_MAX_DICT_BYTES +
             "=<int,e.g.:16384>\n");
  ret.append("  --" + LDBCommand::ARG_BLOCK_SIZE + "=<block_size_in_bytes>\n");
  ret.append("  --" + LDBCommand::ARG_AUTO_COMPACTION + "=<true|false>\n");
  ret.append("  --" + LDBCommand::ARG_DB_WRITE_BUFFER_SIZE +
             "=<int,e.g.:16777216>\n");
  ret.append("  --" + LDBCommand::ARG_WRITE_BUFFER_SIZE +
             "=<int,e.g.:4194304>\n");
  ret.append("  --" + LDBCommand::ARG_FILE_SIZE + "=<int,e.g.:2097152>\n");

  ret.append("\n\n");
  ret.append("Data Access Commands:\n");
  PutCommand::Help(ret);
  GetCommand::Help(ret);
  BatchPutCommand::Help(ret);
  ScanCommand::Help(ret);
  DeleteCommand::Help(ret);
  DeleteRangeCommand::Help(ret);
  DBQuerierCommand::Help(ret);
  ApproxSizeCommand::Help(ret);
  CheckConsistencyCommand::Help(ret);

  ret.append("\n\n");
  ret.append("Admin Commands:\n");
  WALDumperCommand::Help(ret);
  CompactorCommand::Help(ret);
  ReduceDBLevelsCommand::Help(ret);
  ChangeCompactionStyleCommand::Help(ret);
  DBDumperCommand::Help(ret);
  DBLoaderCommand::Help(ret);
  ManifestDumpCommand::Help(ret);
  ListColumnFamiliesCommand::Help(ret);
  DBFileDumperCommand::Help(ret);
  InternalDumpCommand::Help(ret);
  RepairCommand::Help(ret);
  BackupCommand::Help(ret);
  RestoreCommand::Help(ret);
  CheckPointCommand::Help(ret);

  fprintf(stderr, "%s\n", ret.c_str());
}

void LDBCommandRunner::RunCommand(
    int argc, char** argv, Options options, const LDBOptions& ldb_options,
    const std::vector<ColumnFamilyDescriptor>* column_families) {
  if (argc <= 2) {
    PrintHelp(ldb_options, argv[0]);
    exit(1);
  }

  LDBCommand* cmdObj = LDBCommand::InitFromCmdLineArgs(
      argc, argv, options, ldb_options, column_families);
  if (cmdObj == nullptr) {
    fprintf(stderr, "Unknown command\n");
    PrintHelp(ldb_options, argv[0]);
    exit(1);
  }

  if (!cmdObj->ValidateCmdLineOptions()) {
    exit(1);
  }

  cmdObj->Run();
  LDBCommandExecuteResult ret = cmdObj->GetExecuteState();
  fprintf(stderr, "%s\n", ret.ToString().c_str());
  delete cmdObj;

  exit(ret.IsFailed());
}

void LDBTool::Run(int argc, char** argv, Options options,
                  const LDBOptions& ldb_options,
                  const std::vector<ColumnFamilyDescriptor>* column_families) {
  LDBCommandRunner::RunCommand(argc, argv, options, ldb_options,
                               column_families);
}
} //命名空间rocksdb

#endif  //摇滚乐
