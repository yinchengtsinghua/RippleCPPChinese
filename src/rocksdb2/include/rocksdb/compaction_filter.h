
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
//版权所有（c）2013 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef STORAGE_ROCKSDB_INCLUDE_COMPACTION_FILTER_H_
#define STORAGE_ROCKSDB_INCLUDE_COMPACTION_FILTER_H_

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace rocksdb {

class Slice;
class SliceTransform;

//压缩运行的上下文信息
struct CompactionFilterContext {
//此压缩运行是否包括所有数据文件
  bool is_full_compaction;
//客户是否要求进行此压缩（真）
//或者它是作为一个自动压实过程发生的？
  bool is_manual_compaction;
};

//CompactionFilter允许应用程序修改/删除位于
//压实时间。

class CompactionFilter {
 public:
  enum ValueType {
    kValue,
    kMergeOperand,
kBlobIndex,  //由blobdb内部使用。
  };

  enum class Decision {
    kKeep,
    kRemove,
    kChangeValue,
    kRemoveAndSkipUntil,
  };

//压缩运行的上下文信息
  struct Context {
//此压缩运行是否包括所有数据文件
    bool is_full_compaction;
//客户是否要求进行此压缩（真）
//或者它是作为一个自动压实过程发生的？
    bool is_manual_compaction;
//此压缩用于哪个列族。
    uint32_t column_family_id;
  };

  virtual ~CompactionFilter() {}

//压缩过程调用此
//正在压实的kv方法。返回值
//如果为假，则表示应将kv保存在
//此压缩运行的输出，返回值为true
//指示应从
//压实输出。应用程序可以检查
//关键的现有价值，并在此基础上做出决策。
//
//压缩期间合并操作的结果的键值不是
//传入此函数。当前，当您将put（）s和
//merge（）在同一个键上，我们只保证处理合并操作数
//通过压缩过滤器。可能会处理put（）s，也可能不会。
//
//当要保留该值时，应用程序可以选择
//修改现有的_值并通过新的_值返回。
//在这种情况下，需要将“更改的值”设置为“真”。
//
//如果使用rocksdb的快照功能（即在
//对象），compactionfilter可能对您不太有用。由于
//保证我们需要维护，压缩过程不会调用filter（）。
//在最新快照之前写入的所有密钥上。换言之，
//压缩将只对在最近一次压缩后写入的键调用filter（）。
//调用GetSnapshot（）。在大多数情况下，filter（）不会被调用
//经常。这是我们正在解决的问题。参见讨论：
//https://www.facebook.com/groups/mysqlonrocksdb/permalink/999723240091865/
//
//如果使用多线程压缩*和*单个压缩过滤器
//实例是通过选项：：compaction_filter提供的，此方法可能是
//同时从不同线程调用。应用程序必须确保
//调用是线程安全的。
//
//如果compactionfilter是由工厂创建的，那么它将只
//由执行压缩运行的单个线程使用，并且
//调用不需要是线程安全的。但是，多个过滤器可能
//存在并同时运行。
//
//如果将max_子约定设置为大于
//1。在这种情况下，来自多个线程的子压缩可能会调用一个
//CompactionFilter并发。
  virtual bool Filter(int level, const Slice& key, const Slice& existing_value,
                      std::string* new_value, bool* value_changed) const {
    return false;
  }

//压缩过程对每个合并操作数调用此方法。如果这样
//方法返回true，合并操作数将被忽略而不写出
//在压缩输出中
//
//注意：如果您使用的是TransactionDB，则不建议实现
//filterMergeOperand（）。如果合并操作被筛选出，则TransactionDB
//可能没有意识到存在写冲突，可能允许事务
//提交应该失败的。相反，最好实现
//合并运算符内部的合并筛选。
  virtual bool FilterMergeOperand(int level, const Slice& key,
                                  const Slice& operand) const {
    return false;
  }

//扩展的API。同时调用值和合并操作数。
//允许更改值并跳过键的范围。
//默认实现使用filter（）和filtermergeoperand（）。
//如果要重写此方法，则无需重写其他两个方法。
//`value_type`指示此键值是否与正常值对应
//值（例如，用put（）写入）或合并操作数（用merge（）写入）。
//
//可能的返回值：
//*kkeep-保留键值对。
//*kremove-删除键值对或合并操作数。
//*kchangevalue-保留键并将值/操作数更改为*新值。
//*kremove和skipuntil-删除此键值对，也删除
//所有key-value与key-in[key，*跳过\u-until]对。这个范围
//将跳过个键（共个），而不进行读取，可能会保存一些
//IO操作与逐个删除密钥相比。
//
//*跳过“直到<=键被视为与decision:：kkeep相同”
//（因为范围[键，*跳至]为空）。
//
//Caveats：
//-跳过键，即使有包含它们的快照，
//就好像ignoresnapshots（）是真的；即值被删除
//由kremove和skipuntil可以从快照中消失-小心
//如果您使用的是TransactionDB或DB:：GetSnapshot（）。
//-如果键的值被覆盖或合并到（multiple put（）s
//或merge（）s），压缩过滤器跳过此键
//Kremove和SkipUntil，它可能只会移除
//新的价值，暴露了旧的价值
//改写。
//-无法在前缀模式下使用PlainTableFactory。
//-如果使用kremove和skipuntil，还应考虑减少
//压缩\u预读\u大小选项。
//
//注意：如果您使用的是TransactionDB，建议不要筛选
//输出或修改合并操作数（valueType:：kmergeOperand）。
//如果合并操作被筛选出，则TransactionDB可能无法实现
//是一个写冲突，可能允许提交应该
//失败。相反，最好在
//MergeOperator。
  virtual Decision FilterV2(int level, const Slice& key, ValueType value_type,
                            const Slice& existing_value, std::string* new_value,
                            std::string* skip_until) const {
    switch (value_type) {
      case ValueType::kValue: {
        bool value_changed = false;
        bool rv = Filter(level, key, existing_value, new_value, &value_changed);
        if (rv) {
          return Decision::kRemove;
        }
        return value_changed ? Decision::kChangeValue : Decision::kKeep;
      }
      case ValueType::kMergeOperand: {
        bool rv = FilterMergeOperand(level, key, existing_value);
        return rv ? Decision::kRemove : Decision::kKeep;
      }
      case ValueType::kBlobIndex:
        return Decision::kKeep;
    }
    assert(false);
    return Decision::kKeep;
  }

//默认情况下，压缩将只对在
//最近调用GetSnapshot（）。但是，如果压缩过滤器
//重写ignoresnapshots以使其返回true，即压缩过滤器
//即使密钥是在上次快照之前写入的，也将调用。
//此行为仅在我们要删除一组键时使用
//不考虑快照。尤其要小心
//要理解这些键的值会改变，即使我们
//使用快照。
  virtual bool IgnoreSnapshots() const { return false; }

//返回标识此压缩筛选器的名称。
//该名称将在启动时打印到日志文件中进行诊断。
  virtual const char* Name() const = 0;
};

//每次压缩都将创建一个新的压缩过滤器，允许
//了解不同压实度的应用程序
class CompactionFilterFactory {
 public:
  virtual ~CompactionFilterFactory() { }

  virtual std::unique_ptr<CompactionFilter> CreateCompactionFilter(
      const CompactionFilter::Context& context) = 0;

//返回标识此压缩筛选器工厂的名称。
  virtual const char* Name() const = 0;
};

}  //命名空间rocksdb

#endif  //储存区包括压实过滤器
