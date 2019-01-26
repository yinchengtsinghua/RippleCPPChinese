
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
#include <getopt.h>
#include <cstdio>
#include <string>
#include <unordered_map>

#include "utilities/blob_db/blob_dump_tool.h"

using namespace rocksdb;
using namespace rocksdb::blob_db;

int main(int argc, char** argv) {
  using DisplayType = BlobDumpTool::DisplayType;
  const std::unordered_map<std::string, DisplayType> display_types = {
      {"none", DisplayType::kNone},
      {"raw", DisplayType::kRaw},
      {"hex", DisplayType::kHex},
      {"detail", DisplayType::kDetail},
  };
  const struct option options[] = {
      {"help", no_argument, nullptr, 'h'},
      {"file", required_argument, nullptr, 'f'},
      {"show_key", optional_argument, nullptr, 'k'},
      {"show_blob", optional_argument, nullptr, 'b'},
  };
  DisplayType show_key = DisplayType::kRaw;
  DisplayType show_blob = DisplayType::kNone;
  std::string file;
  while (true) {
    int c = getopt_long(argc, argv, "hk::b::f:", options, nullptr);
    if (c < 0) {
      break;
    }
    std::string arg_str(optarg ? optarg : "");
    switch (c) {
      case 'h':
        fprintf(stdout,
                "Usage: blob_dump --file=filename "
                "[--show_key[=none|raw|hex|detail]] "
                "[--show_blob[=none|raw|hex|detail]]\n");
        return 0;
      case 'f':
        file = optarg;
        break;
      case 'k':
        if (optarg) {
          if (display_types.count(arg_str) == 0) {
            fprintf(stderr, "Unrecognized key display type.\n");
            return -1;
          }
          show_key = display_types.at(arg_str);
        }
        break;
      case 'b':
        if (optarg) {
          if (display_types.count(arg_str) == 0) {
            fprintf(stderr, "Unrecognized blob display type.\n");
            return -1;
          }
          show_blob = display_types.at(arg_str);
        } else {
          show_blob = DisplayType::kDetail;
        }
        break;
      default:
        fprintf(stderr, "Unrecognized option.\n");
        return -1;
    }
  }
  BlobDumpTool tool;
  Status s = tool.Run(file, show_key, show_blob);
  if (!s.ok()) {
    fprintf(stderr, "Failed: %s\n", s.ToString().c_str());
    return -1;
  }
  return 0;
}
#else
#include <stdio.h>
int main(int argc, char** argv) {
  fprintf(stderr, "Not supported in lite mode.\n");
  return -1;
}
#endif  //摇滚乐
