
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
#pragma once
#include <string>
#include <vector>
#include "rocksdb/db.h"
#include "rocksdb/options.h"

namespace rocksdb {

//用于将切片转换为可读字符串的接口
class SliceFormatter {
 public:
  virtual ~SliceFormatter() {}
  virtual std::string Format(const Slice& s) const = 0;
};

//自定义ldb工具的选项（在db选项之外）
struct LDBOptions {
//使用所有字段的默认值创建ldbooptions
  LDBOptions();

//将切片转换为可读字符串的键格式化程序。
//默认值：Slice:：ToString（）。
  std::shared_ptr<SliceFormatter> key_formatter;

  std::string print_help_header = "ldb - RocksDB Tool";
};

class LDBTool {
 public:
  void Run(
      int argc, char** argv, Options db_options = Options(),
      const LDBOptions& ldb_options = LDBOptions(),
      const std::vector<ColumnFamilyDescriptor>* column_families = nullptr);
};

} //命名空间rocksdb

#endif  //摇滚乐
