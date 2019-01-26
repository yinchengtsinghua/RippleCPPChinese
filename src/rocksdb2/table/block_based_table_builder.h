
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
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "rocksdb/flush_block_policy.h"
#include "rocksdb/listener.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "table/table_builder.h"

namespace rocksdb {

class BlockBuilder;
class BlockHandle;
class WritableFile;
struct BlockBasedTableOptions;

extern const uint64_t kBlockBasedTableMagicNumber;
extern const uint64_t kLegacyBlockBasedTableMagicNumber;

class BlockBasedTableBuilder : public TableBuilder {
 public:
//创建一个将存储其所属表内容的生成器
//在*文件中生成。不关闭文件。这取决于
//调用方在调用finish（）后关闭文件。
//@param compression\u用于预设压缩库的dict data
//字典或nullptr。
  BlockBasedTableBuilder(
      const ImmutableCFOptions& ioptions,
      const BlockBasedTableOptions& table_options,
      const InternalKeyComparator& internal_comparator,
      const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
          int_tbl_prop_collector_factories,
      uint32_t column_family_id, WritableFileWriter* file,
      const CompressionType compression_type,
      const CompressionOptions& compression_opts,
      const std::string* compression_dict, const bool skip_filters,
      const std::string& column_family_name, const uint64_t creation_time = 0,
      const uint64_t oldest_key_time = 0);

//要求：已调用finish（）或放弃（）。
  ~BlockBasedTableBuilder();

//向正在构造的表添加键和值。
//要求：根据比较器，键在任何先前添加的键之后。
//要求：finish（），放弃（）尚未调用
  void Add(const Slice& key, const Slice& value) override;

//如果检测到一些错误，返回非OK。
  Status status() const override;

//把桌子盖好。停止使用传递给
//此函数后的构造函数返回。
//要求：finish（），放弃（）尚未调用
  Status Finish() override;

//指示应放弃此生成器的内容。停止
//使用此函数返回后传递给构造函数的文件。
//如果调用者不想调用finish（），它必须调用放弃（）。
//在摧毁这个建造者之前。
//要求：finish（），放弃（）尚未调用
  void Abandon() override;

//到目前为止，要添加的调用数（）。
  uint64_t NumEntries() const override;

//到目前为止生成的文件的大小。如果在成功后调用
//finish（）调用，返回最终生成文件的大小。
  uint64_t FileSize() const override;

  bool NeedCompact() const override;

//获取表属性
  TableProperties GetTableProperties() const override;

 private:
  bool ok() const { return status().ok(); }

//调用块的finish（）方法
//然后将压缩块内容写入文件。
  void WriteBlock(BlockBuilder* block, BlockHandle* handle, bool is_data_block);

//压缩块内容并将其写入文件。
  void WriteBlock(const Slice& block_contents, BlockHandle* handle,
                  bool is_data_block);
//直接将数据写入文件。
  void WriteRawBlock(const Slice& data, CompressionType, BlockHandle* handle);
  Status InsertBlockInCache(const Slice& block_contents,
                            const CompressionType type,
                            const BlockHandle* handle);
  struct Rep;
  class BlockBasedTablePropertiesCollectorFactory;
  class BlockBasedTablePropertiesCollector;
  Rep* rep_;

//高级操作：将任何缓冲的键/值对刷新到文件。
//可用于确保两个相邻条目永远不存在
//相同的数据块。大多数客户机不需要使用此方法。
//要求：finish（），放弃（）尚未调用
  void Flush();

//当原始大小大于int时，一些压缩库会失败。
//未压缩大小大于k压缩大小限制，不要压缩它
  const uint64_t kCompressionSizeLimit = std::numeric_limits<int>::max();

//不允许复制
  BlockBasedTableBuilder(const BlockBasedTableBuilder&) = delete;
  void operator=(const BlockBasedTableBuilder&) = delete;
};

Slice CompressBlock(const Slice& raw,
                    const CompressionOptions& compression_options,
                    CompressionType* type, uint32_t format_version,
                    const Slice& compression_dict,
                    std::string* compressed_output);

}  //命名空间rocksdb
