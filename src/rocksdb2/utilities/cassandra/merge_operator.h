
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2017至今，Facebook，Inc.保留所有权利。
//此源代码在两个gplv2下都获得了许可（在
//复制根目录中的文件）和Apache2.0许可证
//（在根目录的license.apache文件中找到）。

#pragma once
#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"

namespace rocksdb {
namespace cassandra {

/*
 *用于RockSDB的MergeOperator，用于实现Cassandra行值合并。
 **/

class CassandraValueMergeOperator : public MergeOperator {
public:
  static std::shared_ptr<MergeOperator> CreateSharedInstance();

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override;

  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value,
                                 Logger* logger) const override;

  virtual const char* Name() const override;

  virtual bool AllowSingleOperand() const override { return true; }
};
} //命名空间cassandra
} //命名空间rocksdb
