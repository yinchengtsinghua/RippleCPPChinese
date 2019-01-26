
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
#include "rocksdb/slice.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"

using namespace rocksdb;

namespace { //匿名命名空间

//模拟Put语义的合并运算符
//由于此合并运算符不会在生产中使用，
//它被实现为一个非关联的合并操作符来说明
//新接口，用于测试目的。（也就是说，我们继承自
//MergeOperator类而不是AssociationEmergeoperator
//在这种情况下更简单）。
//
//从客户机的角度来看，语义是相同的。
class PutOperator : public MergeOperator {
 public:
  virtual bool FullMerge(const Slice& key,
                         const Slice* existing_value,
                         const std::deque<std::string>& operand_sequence,
                         std::string* new_value,
                         Logger* logger) const override {
//Put基本上只查看当前/最新值
    assert(!operand_sequence.empty());
    assert(new_value != nullptr);
    new_value->assign(operand_sequence.back());
    return true;
  }

  virtual bool PartialMerge(const Slice& key,
                            const Slice& left_operand,
                            const Slice& right_operand,
                            std::string* new_value,
                            Logger* logger) const override {
    new_value->assign(right_operand.data(), right_operand.size());
    return true;
  }

  using MergeOperator::PartialMergeMulti;
  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value, Logger* logger) const
      override {
    new_value->assign(operand_list.back().data(), operand_list.back().size());
    return true;
  }

  virtual const char* Name() const override {
    return "PutOperator";
  }
};

class PutOperatorV2 : public PutOperator {
  virtual bool FullMerge(const Slice& key, const Slice* existing_value,
                         const std::deque<std::string>& operand_sequence,
                         std::string* new_value,
                         Logger* logger) const override {
    assert(false);
    return false;
  }

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override {
//Put基本上只查看当前/最新值
    assert(!merge_in.operand_list.empty());
    merge_out->existing_operand = merge_in.operand_list.back();
    return true;
  }
};

} //匿名命名空间结束

namespace rocksdb {

std::shared_ptr<MergeOperator> MergeOperators::CreateDeprecatedPutOperator() {
  return std::make_shared<PutOperator>();
}

std::shared_ptr<MergeOperator> MergeOperators::CreatePutOperator() {
  return std::make_shared<PutOperatorV2>();
}
}
