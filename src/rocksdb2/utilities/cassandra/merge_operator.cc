
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

#include "merge_operator.h"

#include <memory>
#include <assert.h>

#include "rocksdb/slice.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"
#include "utilities/cassandra/format.h"

namespace rocksdb {
namespace cassandra {

//合并操作的实现（合并两个cassandra值）
bool CassandraValueMergeOperator::FullMergeV2(
    const MergeOperationInput& merge_in,
    MergeOperationOutput* merge_out) const {
//清除要写入的*新_值。
  merge_out->new_value.clear();

  if (merge_in.existing_value == nullptr && merge_in.operand_list.size() == 1) {
//只有一个操作数
    merge_out->existing_operand = merge_in.operand_list.back();
    return true;
  }

  std::vector<RowValue> row_values;
  if (merge_in.existing_value) {
    row_values.push_back(
      RowValue::Deserialize(merge_in.existing_value->data(),
                            merge_in.existing_value->size()));
  }

  for (auto& operand : merge_in.operand_list) {
    row_values.push_back(RowValue::Deserialize(operand.data(), operand.size()));
  }

  RowValue merged = RowValue::Merge(std::move(row_values));
  merge_out->new_value.reserve(merged.Size());
  merged.Serialize(&(merge_out->new_value));

  return true;
}

bool CassandraValueMergeOperator::PartialMergeMulti(
    const Slice& key,
    const std::deque<Slice>& operand_list,
    std::string* new_value,
    Logger* logger) const {
//清除要写入的*新_值。
  assert(new_value);
  new_value->clear();

  std::vector<RowValue> row_values;
  for (auto& operand : operand_list) {
    row_values.push_back(RowValue::Deserialize(operand.data(), operand.size()));
  }
  RowValue merged = RowValue::Merge(std::move(row_values));
  new_value->reserve(merged.Size());
  merged.Serialize(new_value);
  return true;
}

const char* CassandraValueMergeOperator::Name() const  {
  return "CassandraValueMergeOperator";
}

} //命名空间cassandra

std::shared_ptr<MergeOperator>
    MergeOperators::CreateCassandraMergeOperator() {
  return std::make_shared<rocksdb::cassandra::CassandraValueMergeOperator>();
}

} //命名空间rocksdb
