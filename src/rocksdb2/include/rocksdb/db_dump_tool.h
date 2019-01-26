
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
#ifndef ROCKSDB_LITE

#include <string>

#include "rocksdb/db.h"

namespace rocksdb {

struct DumpOptions {
//将被转储的数据库
  std::string db_path;
//将包含转储输出的文件位置
  std::string dump_location;
//不要在转储中包含db信息头
  bool anonymous = false;
};

class DbDumpTool {
 public:
  bool Run(const DumpOptions& dump_options,
           rocksdb::Options options = rocksdb::Options());
};

struct UndumpOptions {
//将转储文件加载到的数据库
  std::string db_path;
//将加载的转储文件的文件位置
  std::string dump_location;
//加载转储文件后压缩数据库
  bool compact_db = false;
};

class DbUndumpTool {
 public:
  bool Run(const UndumpOptions& undump_options,
           rocksdb::Options options = rocksdb::Options());
};
}  //命名空间rocksdb
#endif  //摇滚乐
