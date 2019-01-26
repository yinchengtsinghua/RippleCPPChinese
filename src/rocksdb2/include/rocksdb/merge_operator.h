
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

#ifndef STORAGE_ROCKSDB_INCLUDE_MERGE_OPERATOR_H_
#define STORAGE_ROCKSDB_INCLUDE_MERGE_OPERATOR_H_

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include "rocksdb/slice.h"

namespace rocksdb {

class Slice;
class Logger;

//合并运算符
//
//本质上，mergeoperator指定合并的语义，它只
//客户知道。它可以是数字加法、列表附加、字符串
//连接，编辑数据结构…什么都行。
//另一方面，图书馆关心的是这个问题的实施
//接口，在正确的时间（在get、迭代、压缩…）
//
//要使用合并，客户端需要提供一个实现
//以下接口：
//a）关联的地质发生者-对于最简单的语义（总是采用
//两个值，并将它们合并为一个值，然后将其放回
//例如，数字加法和字符串连接；
//
//b）mergeoperator-更抽象/更复杂的通用类
//操作；将Put/Delete值与
//合并操作数；以及合并多个操作数的另一个方法（partialmerge）
//操作数在一起。如果您的键值
//复杂的结构，但您仍然希望支持特定于客户端的
//增量更新。
//
//联合显影器更容易实现。MergeOperator只是
//更强大。
//
//有关更多详细信息和示例实现，请参阅rocksdb merge wiki。
//
class MergeOperator {
 public:
  virtual ~MergeOperator() {}

//为客户机提供了一种表达read->modify->write语义的方法
//键：（in）与此合并操作关联的键。
//客户端可以基于它多路复用合并运算符
//如果键空间是分区的，并且不同的子空间
//参考不同类型的数据
//合并操作语义
//existing：（in）NULL表示在此操作之前不存在该键
//操作数_list：（in）要应用的合并操作序列，front（）优先。
//新的_值：（out）客户端负责在此处填充合并结果。
//新_值指向的字符串将为空。
//logger:（in）客户端可以使用它来记录合并过程中的错误。
//
//成功后返回true。
//传入的所有值都将是特定于客户端的值。所以如果这个方法
//返回false，这是因为客户端指定了错误的数据或
//内部腐败。这将被库视为一个错误。
//
//还可以使用*记录器获取错误消息。
  virtual bool FullMerge(const Slice& key,
                         const Slice* existing_value,
                         const std::deque<std::string>& operand_list,
                         std::string* new_value,
                         Logger* logger) const {
//已弃用，请使用fullmergev2（）。
    assert(false);
    return false;
  }

  struct MergeOperationInput {
    explicit MergeOperationInput(const Slice& _key,
                                 const Slice* _existing_value,
                                 const std::vector<Slice>& _operand_list,
                                 Logger* _logger)
        : key(_key),
          existing_value(_existing_value),
          operand_list(_operand_list),
          logger(_logger) {}

//与合并操作关联的键。
    const Slice& key;
//当前键nullptr的现有值表示
//值不存在。
    const Slice* existing_value;
//要应用的操作数列表。
    const std::vector<Slice>& operand_list;
//客户机可以使用记录器记录在
//合并操作。
    Logger* logger;
  };

  struct MergeOperationOutput {
    explicit MergeOperationOutput(std::string& _new_value,
                                  Slice& _existing_operand)
        : new_value(_new_value), existing_operand(_existing_operand) {}

//客户端负责在此处填充合并结果。
    std::string& new_value;
//如果合并结果是现有操作数（或现有值）之一，
//客户端可以将此字段设置为操作数（或现有值），而不是
//使用新的_值。
    Slice& existing_operand;
  };

  virtual bool FullMergeV2(const MergeOperationInput& merge_in,
                           MergeOperationOutput* merge_out) const;

//此函数执行合并（左_op，右_op）
//当两个操作数本身都是合并操作类型时
//以相同的顺序传递给db:：merge（）调用
//（即：db:：merge（key，left_op），后跟db:：merge（key，right_op））。
//
//partialmerge应将它们合并为单个合并操作，即
//保存到*new_值中，然后返回true。
//*应构造新的_值，以便调用
//db:：merge（key，*new_value）将产生与调用相同的结果
//到db：：merge（key，left_op），然后是db：：merge（key，right_op）。
//
//新_值指向的字符串将为空。
//
//partialmergemulti的默认实现将使用此函数
//作为一个助手，为了向后兼容。任何继承类
//mergeOperator应实现partialmerge或partialmergemulti，
//尽管建议像一般情况一样实现partialmergemulti
//一次合并多个操作数而不是两个操作数更有效
//一次操作数。
//
//如果不可能或不可能将这两种操作结合起来，
//保持新的_值不变并返回false。图书馆会
//内部跟踪操作，并将其应用于
//一旦看到基值（数据库的Put/Delete/End），请更正顺序。
//
//TODO:目前无法区分错误/损坏
//只需“返回错误”。现在，客户只需返回
//在任何情况下，它都不能执行部分合并，不管原因如何。
//如果数据损坏，请在fullmergev2（）函数中处理它。
//然后回到错误的地方。partialmerge的默认实现将
//总是返回false。
  virtual bool PartialMerge(const Slice& key, const Slice& left_operand,
                            const Slice& right_operand, std::string* new_value,
                            Logger* logger) const {
    return false;
  }

//当所有操作数本身合并时，此函数执行合并
//在中传递给db:：merge（）调用的操作类型
//相同顺序（前面（）第一个）
//（即db：：merge（key，operand_list[0]），后跟
//DB：：合并（键，操作数列表[1]），…）
//
//partialmergemulti应将它们合并为单个合并操作，即
//保存到*new_值中，然后返回true。*新值应该
//构造成这样，对db:：merge（key，*new_value）的调用将产生
//与对db:：merge（key，operand）的子序列单个调用的结果相同
//对于操作数_list from front（）to back（）中的每个操作数。
//
//新_值指向的字符串将为空。
//
//当至少有两个函数时，将调用partialmergemulti函数
//操作数。
//
//在默认实现中，partialmergemulti将调用partialmerge
//多次，每次只合并两个操作数。开发人员
//应该实现partialmergemulti，或者实现partialmerge，
//用作默认的partialmergemulti的辅助函数。
  virtual bool PartialMergeMulti(const Slice& key,
                                 const std::deque<Slice>& operand_list,
                                 std::string* new_value, Logger* logger) const;

//MergeOperator的名称。用于检查MergeOperator
//不匹配（即，使用一个mergeoperator创建的数据库
//使用不同的mergeoperator访问）
//TODO:名称当前未持久存储，因此
//不强制检查。客户负责提供
//数据库打开之间的一致MergeOperator。
  virtual const char* Name() const = 0;

//确定是否可以仅用一个
//合并操作数。
//重写并返回true以允许单个操作数。FulyCyv2和
//应相应地实现partialmerge/partialmergemulti以处理
//一个操作数。
  virtual bool AllowSingleOperand() const { return false; }
};

//简单的关联合并运算符。
class AssociativeMergeOperator : public MergeOperator {
 public:
  ~AssociativeMergeOperator() override {}

//为客户机提供了一种表达read->modify->write语义的方法
//键：（in）与此合并操作关联的键。
//existing_value:（in）NULL表示此操作之前密钥不存在
//值：（in）要更新/合并现有值的值
//新的_值：（out）客户机负责填写合并结果
//在这里。新_值指向的字符串将为空。
//logger:（in）客户端可以使用它来记录合并过程中的错误。
//
//成功后返回true。
//传入的所有值都将是特定于客户端的值。所以如果这个方法
//返回false，这是因为客户端指定了错误的数据或
//内部腐败。客户应假定将对此进行处理。
//作为图书馆的一个错误。
  virtual bool Merge(const Slice& key,
                     const Slice* existing_value,
                     const Slice& value,
                     std::string* new_value,
                     Logger* logger) const = 0;


 private:
//mergeoperator函数的默认实现
  bool FullMergeV2(const MergeOperationInput& merge_in,
                   MergeOperationOutput* merge_out) const override;

  bool PartialMerge(const Slice& key, const Slice& left_operand,
                    const Slice& right_operand, std::string* new_value,
                    Logger* logger) const override;
};

}  //命名空间rocksdb

#endif  //存储设备包括合并操作员
