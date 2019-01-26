
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *用于RockSDB的测试合并运算符，用于实现字符串附加。
 *它是使用mergeoperator接口而不是简单的
 *关联发电机接口。这对于测试/基准测试很有用。
 *虽然两个运算符在语义上相同，但所有生产代码
 *应使用在StringAppend.h，cc中定义的StringAppendOperator。这个
 *本文件中定义的运算符主要用于测试。
 *
 *@作者Deon Nicholas（dnicholas@fb.com）
 *版权所有2013 Facebook
 **/


#pragma once
#include <deque>
#include <string>

#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"

namespace rocksdb {

class StringAppendTESTOperator : public MergeOperator {
 public:
//带分隔符的构造函数
  explicit StringAppendTESTOperator(char delim_char);

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const override;

  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value, Logger* logger) const
      override;

  virtual const char* Name() const override;

 private:
//实际执行“部分合并”的partialmerge版本。
//使用此项模拟StringAppendOperator的确切行为。
  bool _AssocPartialMergeMulti(const Slice& key,
                               const std::deque<Slice>& operand_list,
                               std::string* new_value, Logger* logger) const;

char delim_;         //在元素之间插入分隔符

};

} //命名空间rocksdb
