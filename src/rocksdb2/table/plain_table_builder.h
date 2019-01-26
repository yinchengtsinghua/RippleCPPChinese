
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
#ifndef ROCKSDB_LITE
#include <stdint.h>
#include <string>
#include <vector>
#include "rocksdb/options.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "table/bloom_block.h"
#include "table/plain_table_index.h"
#include "table/plain_table_key_coding.h"
#include "table/table_builder.h"

namespace rocksdb {

class BlockBuilder;
class BlockHandle;
class WritableFile;
class TableBuilder;

class PlainTableBuilder: public TableBuilder {
 public:
//创建一个将存储其所属表内容的生成器
//在*文件中生成。不关闭文件。这取决于
//调用方在调用finish（）后关闭文件。输出文件
//将是“级别”指定的级别的一部分。值-1表示
//调用方不知道输出文件将驻留在哪个级别。
  PlainTableBuilder(
      const ImmutableCFOptions& ioptions,
      const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
          int_tbl_prop_collector_factories,
      uint32_t column_family_id, WritableFileWriter* file,
      uint32_t user_key_size, EncodingType encoding_type,
      size_t index_sparseness, uint32_t bloom_bits_per_key,
      const std::string& column_family_name, uint32_t num_probes = 6,
      size_t huge_page_tlb_size = 0, double hash_table_ratio = 0,
      bool store_index_in_file = false);

//要求：已调用finish（）或放弃（）。
  ~PlainTableBuilder();

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

  TableProperties GetTableProperties() const override { return properties_; }

  bool SaveIndexInFile() const { return store_index_in_file_; }

 private:
  Arena arena_;
  const ImmutableCFOptions& ioptions_;
  std::vector<std::unique_ptr<IntTblPropCollector>>
      table_properties_collectors_;

  BloomBlockBuilder bloom_block_;
  std::unique_ptr<PlainTableIndexBuilder> index_builder_;

  WritableFileWriter* file_;
  uint64_t offset_ = 0;
  uint32_t bloom_bits_per_key_;
  size_t huge_page_tlb_size_;
  Status status_;
  TableProperties properties_;
  PlainTableKeyEncoder encoder_;

  bool store_index_in_file_;

  std::vector<uint32_t> keys_or_prefixes_hashes_;
bool closed_ = false;  //已调用finish（）或放弃（）。

  const SliceTransform* prefix_extractor_;

  Slice GetPrefix(const Slice& target) const {
assert(target.size() >= 8);  //目标是内部密钥
    return GetPrefixFromUserKey(GetUserKey(target));
  }

  Slice GetPrefix(const ParsedInternalKey& target) const {
    return GetPrefixFromUserKey(target.user_key);
  }

  Slice GetUserKey(const Slice& key) const {
    return Slice(key.data(), key.size() - 8);
  }

  Slice GetPrefixFromUserKey(const Slice& user_key) const {
    if (!IsTotalOrderMode()) {
      return prefix_extractor_->Transform(user_key);
    } else {
//如果未设置前缀提取程序，请使用空切片作为前缀。
//在那种情况下，
//它返回到纯二进制搜索
//支持总迭代器查找。
      return Slice();
    }
  }

  bool IsTotalOrderMode() const { return (prefix_extractor_ == nullptr); }

//不允许复制
  PlainTableBuilder(const PlainTableBuilder&) = delete;
  void operator=(const PlainTableBuilder&) = delete;
};

}  //命名空间rocksdb

#endif  //摇滚乐
