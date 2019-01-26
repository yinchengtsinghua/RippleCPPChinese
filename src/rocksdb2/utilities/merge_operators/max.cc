
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

#include <memory>

#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"
#include "utilities/merge_operators.h"

using rocksdb::Slice;
using rocksdb::Logger;
using rocksdb::MergeOperator;

namespace {  //匿名命名空间

//选择最大操作数的合并运算符，比较基于
//切片：比较
class MaxOperator : public MergeOperator {
 public:
  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
    Slice& max = merge_out->existing_operand;
    if (merge_in.existing_value) {
      max = Slice(merge_in.existing_value->data(),
                  merge_in.existing_value->size());
    } else if (max.data() == nullptr) {
      max = Slice();
    }

    for (const auto& op : merge_in.operand_list) {
      if (max.compare(op) < 0) {
        max = op;
      }
    }

    return true;
  }

  virtual bool PartialMerge(const Slice& key, const Slice& left_operand,
                            const Slice& right_operand, std::string* new_value,
                            Logger* logger) const override {
    if (left_operand.compare(right_operand) >= 0) {
      new_value->assign(left_operand.data(), left_operand.size());
    } else {
      new_value->assign(right_operand.data(), right_operand.size());
    }
    return true;
  }

  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value,
                                 Logger* logger) const override {
    Slice max;
    for (const auto& operand : operand_list) {
      if (max.compare(operand) < 0) {
        max = operand;
      }
    }

    new_value->assign(max.data(), max.size());
    return true;
  }

  virtual const char* Name() const override { return "MaxOperator"; }
};

}  //匿名命名空间结束

namespace rocksdb {

std::shared_ptr<MergeOperator> MergeOperators::CreateMaxOperator() {
  return std::make_shared<MaxOperator>();
}
}
