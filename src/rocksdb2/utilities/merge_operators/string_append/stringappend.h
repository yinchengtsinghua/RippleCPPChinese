
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


#pragma once
#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"

namespace rocksdb {

class StringAppendOperator : public AssociativeMergeOperator {
 public:
//构造函数：指定分隔符
  explicit StringAppendOperator(char delim_char);

  virtual bool Merge(const Slice& key,
                     const Slice* existing_value,
                     const Slice& value,
                     std::string* new_value,
                     Logger* logger) const override;

  virtual const char* Name() const override;

 private:
char delim_;         //在元素之间插入分隔符

};

} //命名空间rocksdb
