
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

#ifndef ROCKSDB_LITE

#include "rocksdb/utilities/options_util.h"

#include "options/options_parser.h"
#include "rocksdb/options.h"
#include "util/filename.h"

namespace rocksdb {
Status LoadOptionsFromFile(const std::string& file_name, Env* env,
                           DBOptions* db_options,
                           std::vector<ColumnFamilyDescriptor>* cf_descs,
                           bool ignore_unknown_options) {
  RocksDBOptionsParser parser;
  Status s = parser.Parse(file_name, env, ignore_unknown_options);
  if (!s.ok()) {
    return s;
  }

  *db_options = *parser.db_opt();

  const std::vector<std::string>& cf_names = *parser.cf_names();
  const std::vector<ColumnFamilyOptions>& cf_opts = *parser.cf_opts();
  cf_descs->clear();
  for (size_t i = 0; i < cf_opts.size(); ++i) {
    cf_descs->push_back({cf_names[i], cf_opts[i]});
  }
  return Status::OK();
}

Status GetLatestOptionsFileName(const std::string& dbpath,
                                Env* env, std::string* options_file_name) {
  Status s;
  std::string latest_file_name;
  uint64_t latest_time_stamp = 0;
  std::vector<std::string> file_names;
  s = env->GetChildren(dbpath, &file_names);
  if (!s.ok()) {
    return s;
  }
  for (auto& file_name : file_names) {
    uint64_t time_stamp;
    FileType type;
    if (ParseFileName(file_name, &time_stamp, &type) && type == kOptionsFile) {
      if (time_stamp > latest_time_stamp) {
        latest_time_stamp = time_stamp;
        latest_file_name = file_name;
      }
    }
  }
  if (latest_file_name.size() == 0) {
    return Status::NotFound("No options files found in the DB directory.");
  }
  *options_file_name = latest_file_name;
  return Status::OK();
}

Status LoadLatestOptions(const std::string& dbpath, Env* env,
                         DBOptions* db_options,
                         std::vector<ColumnFamilyDescriptor>* cf_descs,
                         bool ignore_unknown_options) {
  std::string options_file_name;
  Status s = GetLatestOptionsFileName(dbpath, env, &options_file_name);
  if (!s.ok()) {
    return s;
  }

  return LoadOptionsFromFile(dbpath + "/" + options_file_name, env, db_options,
                             cf_descs, ignore_unknown_options);
}

Status CheckOptionsCompatibility(
    const std::string& dbpath, Env* env, const DBOptions& db_options,
    const std::vector<ColumnFamilyDescriptor>& cf_descs,
    bool ignore_unknown_options) {
  std::string options_file_name;
  Status s = GetLatestOptionsFileName(dbpath, env, &options_file_name);
  if (!s.ok()) {
    return s;
  }

  std::vector<std::string> cf_names;
  std::vector<ColumnFamilyOptions> cf_opts;
  for (const auto& cf_desc : cf_descs) {
    cf_names.push_back(cf_desc.name);
    cf_opts.push_back(cf_desc.options);
  }

  const OptionsSanityCheckLevel kDefaultLevel = kSanityLevelLooselyCompatible;

  return RocksDBOptionsParser::VerifyRocksDBOptionsFromFile(
      db_options, cf_names, cf_opts, dbpath + "/" + options_file_name, env,
      kDefaultLevel, ignore_unknown_options);
}

}  //命名空间rocksdb
#endif  //！摇滚乐
