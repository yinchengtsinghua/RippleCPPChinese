
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

#pragma once
#include <string>
#include <map>

namespace rocksdb {

class WriteBatch;

//walfilter允许应用程序检查提前写入日志（wal）
//记录或修改恢复时的处理。
//请参阅下面的详细信息。
class WalFilter {
 public:
  enum class WalProcessingOption {
//照常继续处理
    kContinueProcessing = 0,
//忽略当前记录，但继续处理日志
    kIgnoreCurrentRecord = 1,
//停止重播日志并放弃日志
//以后恢复时不会重播日志
    kStopReplay = 2,
//筛选器检测到损坏的记录
    kCorruptedRecord = 3,
//枚举计数标记
    kWalProcessingOptionMax = 4
  };

  virtual ~WalFilter() {}

//提供columnfamily->lognumber映射以筛选
//这样，过滤器就可以确定日志号是否应用于给定的
//柱族（即，对于
//列族）。
//我们还传入name->id map，因为在
//恢复（恢复后打开句柄）。
//而写批回调是根据列族ID进行的。
//
//@params cf_lognumber_map column_family_id to lognumber映射
//@params cf_name_id_map column_family_name to column_family_id map

  virtual void ColumnFamilyLogNumberMap(
    const std::map<uint32_t, uint64_t>& cf_lognumber_map,
    const std::map<std::string, uint32_t>& cf_name_id_map) {}

//为所有日志遇到的每个日志记录调用log record
//在恢复时重播日志。此方法可用于：
//*检查记录（使用批次参数）
//*忽略当前记录
//（通过返回walprocessingoption:：kignorrecurrentrecord）
//*报告损坏的记录
//（通过返回walprocessingoption:：kcorruptedrecord）
//*停止日志重放
//（通过返回kstop replay）-请注意，这意味着
//从当前记录开始丢弃日志。
//
//@params log_number当前日志的日志号。
//筛选器可能会使用此来确定日志
//记录适用于某个列族。
//@params log_file_name log file name-仅供参考
//恢复期间在日志中遇到@params批处理
//如果筛选器要更改，@params new_batch new_batch to populate
//批处理（例如筛选出一些记录，
//或者改变一些记录）。
//请注意，新批次不得包含
//比原始记录多，否则恢复将
//失败了。
//@params batch_更改了批是否被过滤器更改。
//如果填充了新的_批次，则必须将其设置为true，
//否则，新的_批次无效。
//@返回当前记录的处理选项。
//请参见上面的walprocessingoption枚举
//细节。
  virtual WalProcessingOption LogRecordFound(unsigned long long log_number,
                                        const std::string& log_file_name,
                                        const WriteBatch& batch,
                                        WriteBatch* new_batch,
                                        bool* batch_changed) {
//为了兼容性，默认实现返回到旧函数
    return LogRecord(batch, new_batch, batch_changed);
  }

//请参阅以上日志记录的注释。此功能用于
//仅兼容，包含参数的子集。
//新代码应该使用上面的函数。
  virtual WalProcessingOption LogRecord(const WriteBatch& batch,
                                        WriteBatch* new_batch,
                                        bool* batch_changed) const {
    return WalProcessingOption::kContinueProcessing;
  }

//返回标识此wal筛选器的名称。
//该名称将在启动时打印到日志文件中进行诊断。
  virtual const char* Name() const = 0;
};

}  //命名空间rocksdb
