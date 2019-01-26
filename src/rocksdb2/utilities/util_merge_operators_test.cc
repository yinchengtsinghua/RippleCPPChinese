
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

#include "util/testharness.h"
#include "util/testutil.h"
#include "utilities/merge_operators.h"

namespace rocksdb {

class UtilMergeOperatorTest : public testing::Test {
 public:
  UtilMergeOperatorTest() {}

  std::string FullMergeV2(std::string existing_value,
                          std::vector<std::string> operands,
                          std::string key = "") {
    std::string result;
    Slice result_operand(nullptr, 0);

    Slice existing_value_slice(existing_value);
    std::vector<Slice> operands_slice(operands.begin(), operands.end());

    const MergeOperator::MergeOperationInput merge_in(
        key, &existing_value_slice, operands_slice, nullptr);
    MergeOperator::MergeOperationOutput merge_out(result, result_operand);
    merge_operator_->FullMergeV2(merge_in, &merge_out);

    if (result_operand.data()) {
      result.assign(result_operand.data(), result_operand.size());
    }
    return result;
  }

  std::string FullMergeV2(std::vector<std::string> operands,
                          std::string key = "") {
    std::string result;
    Slice result_operand(nullptr, 0);

    std::vector<Slice> operands_slice(operands.begin(), operands.end());

    const MergeOperator::MergeOperationInput merge_in(key, nullptr,
                                                      operands_slice, nullptr);
    MergeOperator::MergeOperationOutput merge_out(result, result_operand);
    merge_operator_->FullMergeV2(merge_in, &merge_out);

    if (result_operand.data()) {
      result.assign(result_operand.data(), result_operand.size());
    }
    return result;
  }

  std::string PartialMerge(std::string left, std::string right,
                           std::string key = "") {
    std::string result;

    merge_operator_->PartialMerge(key, left, right, &result, nullptr);
    return result;
  }

  std::string PartialMergeMulti(std::deque<std::string> operands,
                                std::string key = "") {
    std::string result;
    std::deque<Slice> operands_slice(operands.begin(), operands.end());

    merge_operator_->PartialMergeMulti(key, operands_slice, &result, nullptr);
    return result;
  }

 protected:
  std::shared_ptr<MergeOperator> merge_operator_;
};

TEST_F(UtilMergeOperatorTest, MaxMergeOperator) {
  merge_operator_ = MergeOperators::CreateMaxOperator();

  EXPECT_EQ("B", FullMergeV2("B", {"A"}));
  EXPECT_EQ("B", FullMergeV2("A", {"B"}));
  EXPECT_EQ("", FullMergeV2({"", "", ""}));
  EXPECT_EQ("A", FullMergeV2({"A"}));
  EXPECT_EQ("ABC", FullMergeV2({"ABC"}));
  EXPECT_EQ("Z", FullMergeV2({"ABC", "Z", "C", "AXX"}));
  EXPECT_EQ("ZZZ", FullMergeV2({"ABC", "CC", "Z", "ZZZ"}));
  EXPECT_EQ("a", FullMergeV2("a", {"ABC", "CC", "Z", "ZZZ"}));

  EXPECT_EQ("z", PartialMergeMulti({"a", "z", "efqfqwgwew", "aaz", "hhhhh"}));

  EXPECT_EQ("b", PartialMerge("a", "b"));
  EXPECT_EQ("z", PartialMerge("z", "azzz"));
  EXPECT_EQ("a", PartialMerge("a", ""));
}

}  //命名空间rocksdb

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
