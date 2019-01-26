
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

#include <list>
#include <string>
#include <unordered_map>
#include "db/dbformat.h"
#include "rocksdb/options.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"

#include "table/block.h"
#include "table/block_based_table_reader.h"
#include "table/full_filter_block.h"
#include "table/index_builder.h"
#include "util/autovector.h"

namespace rocksdb {

class PartitionedFilterBlockBuilder : public FullFilterBlockBuilder {
 public:
  explicit PartitionedFilterBlockBuilder(
      const SliceTransform* prefix_extractor, bool whole_key_filtering,
      FilterBitsBuilder* filter_bits_builder, int index_block_restart_interval,
      PartitionedIndexBuilder* const p_index_builder,
      const uint32_t partition_size);

  virtual ~PartitionedFilterBlockBuilder();

  void AddKey(const Slice& key) override;

  virtual Slice Finish(const BlockHandle& last_partition_block_handle,
                       Status* status) override;

 private:
//滤波数据
BlockBuilder index_on_filter_block_builder_;  //顶级索引生成器
  struct FilterEntry {
    std::string key;
    Slice filter;
  };
std::list<FilterEntry> filters;  //分区索引及其键的列表
  std::unique_ptr<IndexBuilder> value;
  std::vector<std::unique_ptr<const char[]>> filter_gc;
  bool finishing_filters =
false;  //如果finish调用一次但尚未完成，则为true。
//何时切割并完成过滤块的策略
  void MaybeCutAFilterBlock();
//目前，我们为过滤器和索引保留相同数量的分区。
//这将允许将来进行一些潜在的优化。如果这样
//优化没有意识到我们可以使用不同数量的分区和
//消除索引生成器
  PartitionedIndexBuilder* const p_index_builder_;
//每个分区所需的筛选器数
  uint32_t filters_per_partition_;
//最后一个分区中的当前筛选器数
  uint32_t filters_in_partition_;
};

class PartitionedFilterBlockReader : public FilterBlockReader {
 public:
  explicit PartitionedFilterBlockReader(const SliceTransform* prefix_extractor,
                                        bool whole_key_filtering,
                                        BlockContents&& contents,
                                        FilterBitsReader* filter_bits_reader,
                                        Statistics* stats,
                                        const Comparator& comparator,
                                        const BlockBasedTable* table);
  virtual ~PartitionedFilterBlockReader();

  virtual bool IsBlockBased() override { return false; }
  virtual bool KeyMayMatch(
      const Slice& key, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual bool PrefixMayMatch(
      const Slice& prefix, uint64_t block_offset = kNotValid,
      const bool no_io = false,
      const Slice* const const_ikey_ptr = nullptr) override;
  virtual size_t ApproximateMemoryUsage() const override;

 private:
  Slice GetFilterPartitionHandle(const Slice& entry);
  BlockBasedTable::CachableEntry<FilterBlockReader> GetFilterPartition(
      FilePrefetchBuffer* prefetch_buffer, Slice* handle, const bool no_io,
      bool* cached);
  virtual void CacheDependencies(bool pin) override;

  const SliceTransform* prefix_extractor_;
  std::unique_ptr<Block> idx_on_fltr_blk_;
  const Comparator& comparator_;
  const BlockBasedTable* table_;
  std::unordered_map<uint64_t,
                     BlockBasedTable::CachableEntry<FilterBlockReader>>
      filter_map_;
};

}  //命名空间rocksdb
