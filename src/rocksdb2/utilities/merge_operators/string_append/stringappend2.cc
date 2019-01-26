
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *@作者Deon Nicholas（dnicholas@fb.com）
 *版权所有2013 Facebook
 **/


#include "stringappend2.h"

#include <memory>
#include <string>
#include <assert.h>

#include "rocksdb/slice.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"

namespace rocksdb {

//构造函数：还指定分隔符。
StringAppendTESTOperator::StringAppendTESTOperator(char delim_char)
    : delim_(delim_char) {
}

//合并操作的实现（连接两个字符串）
bool StringAppendTESTOperator::FullMergeV2(
    const MergeOperationInput& merge_in,
    MergeOperationOutput* merge_out) const {
//清除要写入的*新_值。
  merge_out->new_value.clear();

  if (merge_in.existing_value == nullptr && merge_in.operand_list.size() == 1) {
//只有一个操作数
    merge_out->existing_operand = merge_in.operand_list.back();
    return true;
  }

//计算最终结果所需的空间。
  size_t numBytes = 0;
  for (auto it = merge_in.operand_list.begin();
       it != merge_in.operand_list.end(); ++it) {
numBytes += it->size() + 1;   //分隔符加1
  }

//仅在打印第一个条目后打印分隔符
  bool printDelim = false;

//如果存在，请预先准备*现有的_值。
  if (merge_in.existing_value) {
    merge_out->new_value.reserve(numBytes + merge_in.existing_value->size());
    merge_out->new_value.append(merge_in.existing_value->data(),
                                merge_in.existing_value->size());
    printDelim = true;
  } else if (numBytes) {
    merge_out->new_value.reserve(
numBytes - 1);  //减1，因为分隔符少了一个
  }

//连接字符串序列（并在每个字符串之间添加分隔符）
  for (auto it = merge_in.operand_list.begin();
       it != merge_in.operand_list.end(); ++it) {
    if (printDelim) {
      merge_out->new_value.append(1, delim_);
    }
    merge_out->new_value.append(it->data(), it->size());
    printDelim = true;
  }

  return true;
}

bool StringAppendTESTOperator::PartialMergeMulti(
    const Slice& key, const std::deque<Slice>& operand_list,
    std::string* new_value, Logger* logger) const {
  return false;
}

//实际执行“部分合并”的partialmerge版本。
//使用此项模拟StringAppendOperator的确切行为。
bool StringAppendTESTOperator::_AssocPartialMergeMulti(
    const Slice& key, const std::deque<Slice>& operand_list,
    std::string* new_value, Logger* logger) const {
//清除*新的写入值
  assert(new_value);
  new_value->clear();
  assert(operand_list.size() >= 2);

//通用追加
//确定并为*新值保留正确的大小。
  size_t size = 0;
  for (const auto& operand : operand_list) {
    size += operand.size();
  }
size += operand_list.size() - 1;  //分隔符
  new_value->reserve(size);

//应用连接
  new_value->assign(operand_list.front().data(), operand_list.front().size());

  for (std::deque<Slice>::const_iterator it = operand_list.begin() + 1;
       it != operand_list.end(); ++it) {
    new_value->append(1, delim_);
    new_value->append(it->data(), it->size());
  }

  return true;
}

const char* StringAppendTESTOperator::Name() const  {
  return "StringAppendTESTOperator";
}


std::shared_ptr<MergeOperator>
MergeOperators::CreateStringAppendTESTOperator() {
  return std::make_shared<StringAppendTESTOperator>(',');
}

} //命名空间rocksdb
