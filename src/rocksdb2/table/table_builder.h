
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
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include "db/table_properties_collector.h"
#include "options/cf_options.h"
#include "rocksdb/options.h"
#include "rocksdb/table_properties.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class Slice;
class Status;

struct TableReaderOptions {
//@param skip_filters禁止加载/访问筛选器块
  TableReaderOptions(const ImmutableCFOptions& _ioptions,
                     const EnvOptions& _env_options,
                     const InternalKeyComparator& _internal_comparator,
                     bool _skip_filters = false, int _level = -1)
      : ioptions(_ioptions),
        env_options(_env_options),
        internal_comparator(_internal_comparator),
        skip_filters(_skip_filters),
        level(_level) {}

  const ImmutableCFOptions& ioptions;
  const EnvOptions& env_options;
  const InternalKeyComparator& internal_comparator;
//这仅用于BlockBasedTable（读卡器）
  bool skip_filters;
//此表/文件的级别为-1，表示“未设置，不知道”
  int level;
};

struct TableBuilderOptions {
  TableBuilderOptions(
      const ImmutableCFOptions& _ioptions,
      const InternalKeyComparator& _internal_comparator,
      const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
          _int_tbl_prop_collector_factories,
      CompressionType _compression_type,
      const CompressionOptions& _compression_opts,
      const std::string* _compression_dict, bool _skip_filters,
      const std::string& _column_family_name, int _level,
      const uint64_t _creation_time = 0, const int64_t _oldest_key_time = 0)
      : ioptions(_ioptions),
        internal_comparator(_internal_comparator),
        int_tbl_prop_collector_factories(_int_tbl_prop_collector_factories),
        compression_type(_compression_type),
        compression_opts(_compression_opts),
        compression_dict(_compression_dict),
        skip_filters(_skip_filters),
        column_family_name(_column_family_name),
        level(_level),
        creation_time(_creation_time),
        oldest_key_time(_oldest_key_time) {}
  const ImmutableCFOptions& ioptions;
  const InternalKeyComparator& internal_comparator;
  const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
      int_tbl_prop_collector_factories;
  CompressionType compression_type;
  const CompressionOptions& compression_opts;
//用于预设压缩库字典或nullptr的数据。
  const std::string* compression_dict;
bool skip_filters;  //仅由BlockBasedTableBuilder使用
  const std::string& column_family_name;
int level; //此表/文件的级别为-1，表示“未设置，不知道”
  const uint64_t creation_time;
  const int64_t oldest_key_time;
};

//TableBuilder提供用于生成表的接口
//（从键到值的不可变和排序映射）。
//
//多个线程可以在TableBuilder上调用const方法，而不需要
//外部同步，但如果任何线程可以调用
//非常量方法，访问同一TableBuilder的所有线程都必须使用
//外部同步。
class TableBuilder {
 public:
//要求：已调用finish（）或放弃（）。
  virtual ~TableBuilder() {}

//向正在构造的表添加键和值。
//要求：根据比较器，键在任何先前添加的键之后。
//要求：finish（），放弃（）尚未调用
  virtual void Add(const Slice& key, const Slice& value) = 0;

//如果检测到一些错误，返回非OK。
  virtual Status status() const = 0;

//把桌子盖好。
//要求：finish（），放弃（）尚未调用
  virtual Status Finish() = 0;

//指示应放弃此生成器的内容。
//如果调用者不想调用finish（），它必须调用放弃（）。
//在摧毁这个建造者之前。
//要求：finish（），放弃（）尚未调用
  virtual void Abandon() = 0;

//到目前为止，要添加的调用数（）。
  virtual uint64_t NumEntries() const = 0;

//到目前为止生成的文件的大小。如果在成功后调用
//finish（）调用，返回最终生成文件的大小。
  virtual uint64_t FileSize() const = 0;

//如果用户定义的表属性收集器建议将文件
//进一步压实。
  virtual bool NeedCompact() const { return false; }

//返回表属性
  virtual TableProperties GetTableProperties() const = 0;
};

}  //命名空间rocksdb
