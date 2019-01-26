
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

#include "rocksdb/env.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/slice.h"
#include "util/coding.h"
#include "util/logging.h"
#include "utilities/merge_operators.h"

using namespace rocksdb;

namespace { //匿名命名空间

//带有uint64加法语义的“model”合并运算符
//为了简单和示例，作为一个关联的事件发生器实现。
class UInt64AddOperator : public AssociativeMergeOperator {
 public:
  virtual bool Merge(const Slice& key,
                     const Slice* existing_value,
                     const Slice& value,
                     std::string* new_value,
                     Logger* logger) const override {
    uint64_t orig_value = 0;
    if (existing_value){
      orig_value = DecodeInteger(*existing_value, logger);
    }
    uint64_t operand = DecodeInteger(value, logger);

    assert(new_value);
    new_value->clear();
    PutFixed64(new_value, orig_value + operand);

return true;  //始终返回true，因为腐败将被视为0
  }

  virtual const char* Name() const override {
    return "UInt64AddOperator";
  }

 private:
//获取字符串并将其解码为uint64
//出错时，打印一条消息并返回0
  uint64_t DecodeInteger(const Slice& value, Logger* logger) const {
    uint64_t result = 0;

    if (value.size() == sizeof(uint64_t)) {
      result = DecodeFixed64(value.data());
    } else if (logger != nullptr) {
//如果值已损坏，则将其视为0
      ROCKS_LOG_ERROR(logger, "uint64 value corruption, size: %" ROCKSDB_PRIszt
                              " > %" ROCKSDB_PRIszt,
                      value.size(), sizeof(uint64_t));
    }

    return result;
  }

};

}

namespace rocksdb {

std::shared_ptr<MergeOperator> MergeOperators::CreateUInt64AddOperator() {
  return std::make_shared<UInt64AddOperator>();
}

}
