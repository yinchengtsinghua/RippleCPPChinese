
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

#if !(defined GFLAGS) || defined(ROCKSDB_LITE)

#include <cstdio>
int main() {
#ifndef GFLAGS
  fprintf(stderr, "Please install gflags to run rocksdb tools\n");
#endif
#ifdef ROCKSDB_LITE
  fprintf(stderr, "DbDumpTool is not supported in ROCKSDB_LITE\n");
#endif
  return 1;
}

#else

#include <gflags/gflags.h>
#include "rocksdb/convenience.h"
#include "rocksdb/db_dump_tool.h"

DEFINE_string(db_path, "", "Path to the db that will be dumped");
DEFINE_string(dump_location, "", "Path to where the dump file location");
DEFINE_bool(anonymous, false,
            "Remove information like db path, creation time from dumped file");
DEFINE_string(db_options, "",
              "Options string used to open the database that will be dumped");

int main(int argc, char** argv) {
  GFLAGS::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_db_path == "" || FLAGS_dump_location == "") {
    fprintf(stderr, "Please set --db_path and --dump_location\n");
    return 1;
  }

  rocksdb::DumpOptions dump_options;
  dump_options.db_path = FLAGS_db_path;
  dump_options.dump_location = FLAGS_dump_location;
  dump_options.anonymous = FLAGS_anonymous;

  rocksdb::Options db_options;
  if (FLAGS_db_options != "") {
    rocksdb::Options parsed_options;
    rocksdb::Status s = rocksdb::GetOptionsFromString(
        db_options, FLAGS_db_options, &parsed_options);
    if (!s.ok()) {
      fprintf(stderr, "Cannot parse provided db_options\n");
      return 1;
    }
    db_options = parsed_options;
  }

  rocksdb::DbDumpTool tool;
  if (!tool.Run(dump_options, db_options)) {
    return 1;
  }
  return 0;
}
#endif  //！（定义的gFlags）定义的（rocksdb_lite）
