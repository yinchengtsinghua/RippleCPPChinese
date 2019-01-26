
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
#ifndef MERGE_OPERATORS_H
#define MERGE_OPERATORS_H

#include <memory>
#include <stdio.h>

#include "rocksdb/merge_operator.h"

namespace rocksdb {

class MergeOperators {
 public:
  static std::shared_ptr<MergeOperator> CreatePutOperator();
  static std::shared_ptr<MergeOperator> CreateDeprecatedPutOperator();
  static std::shared_ptr<MergeOperator> CreateUInt64AddOperator();
  static std::shared_ptr<MergeOperator> CreateStringAppendOperator();
  static std::shared_ptr<MergeOperator> CreateStringAppendTESTOperator();
  static std::shared_ptr<MergeOperator> CreateMaxOperator();
  static std::shared_ptr<MergeOperator> CreateCassandraMergeOperator();

//将根据字符串返回不同的合并运算符。
//TODO:是否将“名称”挂接到合并运算符的实际名称（）？
  static std::shared_ptr<MergeOperator> CreateFromStringId(
      const std::string& name) {
    if (name == "put") {
      return CreatePutOperator();
    } else if (name == "put_v1") {
      return CreateDeprecatedPutOperator();
    } else if ( name == "uint64add") {
      return CreateUInt64AddOperator();
    } else if (name == "stringappend") {
      return CreateStringAppendOperator();
    } else if (name == "stringappendtest") {
      return CreateStringAppendTESTOperator();
    } else if (name == "max") {
      return CreateMaxOperator();
    } else if (name == "cassandra") {
      return CreateCassandraMergeOperator();
    } else {
//空或未知，只返回nullptr
      return nullptr;
    }
  }

};

} //命名空间rocksdb

#endif
