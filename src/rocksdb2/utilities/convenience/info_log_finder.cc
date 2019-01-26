
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
//版权所有（c）2012 Facebook。
//此源代码的使用受可以
//在许可证文件中找到。

#include "rocksdb/utilities/info_log_finder.h"
#include "rocksdb/env.h"
#include "util/filename.h"

namespace rocksdb {

Status GetInfoLogList(DB* db, std::vector<std::string>* info_log_list) {
  uint64_t number = 0;
  FileType type;
  std::string path;

  if (!db) {
    return Status::InvalidArgument("DB pointer is not valid");
  }

  const Options& options = db->GetOptions();
  if (!options.db_log_dir.empty()) {
    path = options.db_log_dir;
  } else {
    path = db->GetName();
  }
  InfoLogPrefix info_log_prefix(!options.db_log_dir.empty(), db->GetName());
  auto* env = options.env;
  std::vector<std::string> file_names;
  Status s = env->GetChildren(path, &file_names);

  if (!s.ok()) {
    return s;
  }

  for (auto f : file_names) {
    if (ParseFileName(f, &number, info_log_prefix.prefix, &type) &&
        (type == kInfoLogFile)) {
      info_log_list->push_back(f);
    }
  }
  return Status::OK();
}
}  //命名空间rocksdb
