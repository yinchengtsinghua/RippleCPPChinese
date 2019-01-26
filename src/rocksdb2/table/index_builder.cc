
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

#include "table/index_builder.h"
#include <assert.h>
#include <inttypes.h>

#include <list>
#include <string>

#include "rocksdb/comparator.h"
#include "rocksdb/flush_block_policy.h"
#include "table/format.h"
#include "table/partitioned_filter_block.h"

//如果这里没有匿名名称空间，我们将使警告失败-wMissing原型
namespace rocksdb {
//使用名称空间rocksdb；
//基于索引生成器的类型创建索引生成器。
IndexBuilder* IndexBuilder::CreateIndexBuilder(
    BlockBasedTableOptions::IndexType index_type,
    const InternalKeyComparator* comparator,
    const InternalKeySliceTransform* int_key_slice_transform,
    const BlockBasedTableOptions& table_opt) {
  switch (index_type) {
    case BlockBasedTableOptions::kBinarySearch: {
      return new ShortenedIndexBuilder(comparator,
                                       table_opt.index_block_restart_interval);
    }
    case BlockBasedTableOptions::kHashSearch: {
      return new HashIndexBuilder(comparator, int_key_slice_transform,
                                  table_opt.index_block_restart_interval);
    }
    case BlockBasedTableOptions::kTwoLevelIndexSearch: {
      return PartitionedIndexBuilder::CreateIndexBuilder(comparator, table_opt);
    }
    default: {
      assert(!"Do not recognize the index type ");
      return nullptr;
    }
  }
//不可能的。
  assert(false);
  return nullptr;
}

PartitionedIndexBuilder* PartitionedIndexBuilder::CreateIndexBuilder(
    const InternalKeyComparator* comparator,
    const BlockBasedTableOptions& table_opt) {
  return new PartitionedIndexBuilder(comparator, table_opt);
}

PartitionedIndexBuilder::PartitionedIndexBuilder(
    const InternalKeyComparator* comparator,
    const BlockBasedTableOptions& table_opt)
    : IndexBuilder(comparator),
      index_block_builder_(table_opt.index_block_restart_interval),
      sub_index_builder_(nullptr),
      table_opt_(table_opt) {}

PartitionedIndexBuilder::~PartitionedIndexBuilder() {
  delete sub_index_builder_;
}

void PartitionedIndexBuilder::MakeNewSubIndexBuilder() {
  assert(sub_index_builder_ == nullptr);
  sub_index_builder_ = new ShortenedIndexBuilder(
      comparator_, table_opt_.index_block_restart_interval);
  flush_policy_.reset(FlushBlockBySizePolicyFactory::NewFlushBlockPolicy(
      table_opt_.metadata_block_size, table_opt_.block_size_deviation,
      sub_index_builder_->index_block_builder_));
  partition_cut_requested_ = false;
}

void PartitionedIndexBuilder::RequestPartitionCut() {
  partition_cut_requested_ = true;
}

void PartitionedIndexBuilder::AddIndexEntry(
    std::string* last_key_in_current_block,
    const Slice* first_key_in_next_block, const BlockHandle& block_handle) {
//注意：为了避免在同一个方法调用中有两个连续的刷新，我们不
//添加最后一个密钥时检查刷新策略
if (UNLIKELY(first_key_in_next_block == nullptr)) {  //不再有钥匙
    if (sub_index_builder_ == nullptr) {
      MakeNewSubIndexBuilder();
    }
    sub_index_builder_->AddIndexEntry(last_key_in_current_block,
                                      first_key_in_next_block, block_handle);
    sub_index_last_key_ = std::string(*last_key_in_current_block);
    entries_.push_back(
        {sub_index_last_key_,
         std::unique_ptr<ShortenedIndexBuilder>(sub_index_builder_)});
    sub_index_builder_ = nullptr;
    cut_filter_block = true;
  } else {
//仅对非空子索引生成器应用刷新策略
    if (sub_index_builder_ != nullptr) {
      std::string handle_encoding;
      block_handle.EncodeTo(&handle_encoding);
      bool do_flush =
          partition_cut_requested_ ||
          flush_policy_->Update(*last_key_in_current_block, handle_encoding);
      if (do_flush) {
        entries_.push_back(
            {sub_index_last_key_,
             std::unique_ptr<ShortenedIndexBuilder>(sub_index_builder_)});
        cut_filter_block = true;
        sub_index_builder_ = nullptr;
      }
    }
    if (sub_index_builder_ == nullptr) {
      MakeNewSubIndexBuilder();
    }
    sub_index_builder_->AddIndexEntry(last_key_in_current_block,
                                      first_key_in_next_block, block_handle);
    sub_index_last_key_ = std::string(*last_key_in_current_block);
  }
}

Status PartitionedIndexBuilder::Finish(
    IndexBlocks* index_blocks, const BlockHandle& last_partition_block_handle) {
  assert(!entries_.empty());
//添加最后一个键后必须将其设置为空
  assert(sub_index_builder_ == nullptr);
  if (finishing_indexes == true) {
    Entry& last_entry = entries_.front();
    std::string handle_encoding;
    last_partition_block_handle.EncodeTo(&handle_encoding);
    index_block_builder_.Add(last_entry.key, handle_encoding);
    entries_.pop_front();
  }
//如果没有剩余的子索引，则返回第二级索引。
  if (UNLIKELY(entries_.empty())) {
    index_blocks->index_block_contents = index_block_builder_.Finish();
    return Status::OK();
  } else {
//在第行中完成下一个分区索引，不完整（）表示
//希望完成更多呼叫
    Entry& entry = entries_.front();
    auto s = entry.value->Finish(index_blocks);
    finishing_indexes = true;
    return s.ok() ? Status::Incomplete() : s;
  }
}

//不包括顶层索引的估计大小
//假定在写入索引分区之前调用此方法
//开始
size_t PartitionedIndexBuilder::EstimatedSize() const {
  size_t total = 0;
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    total += it->value->EstimatedSize();
  }
  total +=
      sub_index_builder_ == nullptr ? 0 : sub_index_builder_->EstimatedSize();
  return total;
}

//因为当调用这个方法时，我们还不知道索引块的偏移量，
//顶层索引不存在。因此，我们估计块偏移量和
//创建临时顶级索引。
size_t PartitionedIndexBuilder::EstimateTopLevelIndexSize(
    uint64_t offset) const {
  BlockBuilder tmp_builder(
table_opt_.index_block_restart_interval);  //TMP顶级索引生成器
  for (auto it = entries_.begin(); it != entries_.end(); ++it) {
    std::string tmp_handle_encoding;
    uint64_t size = it->value->EstimatedSize();
    BlockHandle tmp_block_handle(offset, size);
    tmp_block_handle.EncodeTo(&tmp_handle_encoding);
    tmp_builder.Add(it->key, tmp_handle_encoding);
    offset += size;
  }
  return tmp_builder.CurrentSizeEstimate();
}

size_t PartitionedIndexBuilder::NumPartitions() const {
  return entries_.size();
}
}  //命名空间rocksdb
