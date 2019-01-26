
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
/*
 *用于RockSDB的MergeOperator，实现字符串附加。
 *@作者Deon Nicholas（dnicholas@fb.com）
 *版权所有2013 Facebook
 **/


#include "stringappend.h"

#include <memory>
#include <assert.h>

#include "rocksdb/slice.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"

namespace rocksdb {

//构造函数：还指定分隔符。
StringAppendOperator::StringAppendOperator(char delim_char)
    : delim_(delim_char) {
}

//合并操作的实现（连接两个字符串）
bool StringAppendOperator::Merge(const Slice& key,
                                 const Slice* existing_value,
                                 const Slice& value,
                                 std::string* new_value,
                                 Logger* logger) const {

//清除要写入的*新_值。
  assert(new_value);
  new_value->clear();

  if (!existing_value) {
//没有现有的_值。设置*新值=值
    new_value->assign(value.data(),value.size());
  } else {
//通用附加（现有值！= NULL）。
//保留*新的_值以正确的大小，并应用连接。
    new_value->reserve(existing_value->size() + 1 + value.size());
    new_value->assign(existing_value->data(),existing_value->size());
    new_value->append(1,delim_);
    new_value->append(value.data(), value.size());
  }

  return true;
}

const char* StringAppendOperator::Name() const  {
  return "StringAppendOperator";
}

std::shared_ptr<MergeOperator> MergeOperators::CreateStringAppendOperator() {
  return std::make_shared<StringAppendOperator>(',');
}

} //命名空间rocksdb
