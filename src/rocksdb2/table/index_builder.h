
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

#include <assert.h>
#include <inttypes.h>

#include <list>
#include <string>
#include <unordered_map>

#include "rocksdb/comparator.h"
#include "table/block_based_table_factory.h"
#include "table/block_builder.h"
#include "table/format.h"

namespace rocksdb {
//用于生成索引的接口。
//添加新的具体索引生成器的说明：
//1。创建从indexbuilder实例化的子类。
//2。在TableOptions:：IndexType中添加与该子类关联的新条目。
//三。为CreateIndexBuilder中的新子类添加一个create函数。
//注：我们可以设计更高级的设计来简化添加过程。
//另一方面，新的子类会增加代码的复杂性和
//引起读者不必要的注意。鉴于我们不会增加/更改
//索引频繁，只接受更直接的
//设计才有效。
class IndexBuilder {
 public:
  static IndexBuilder* CreateIndexBuilder(
      BlockBasedTableOptions::IndexType index_type,
      const rocksdb::InternalKeyComparator* comparator,
      const InternalKeySliceTransform* int_key_slice_transform,
      const BlockBasedTableOptions& table_opt);

//索引生成器将构造一组包含以下内容的块：
//1。一个主索引块。
//2。（可选）包含
//主要指标。
  struct IndexBlocks {
    Slice index_block_contents;
    std::unordered_map<std::string, Slice> meta_blocks;
  };
  explicit IndexBuilder(const InternalKeyComparator* comparator)
      : comparator_(comparator) {}

  virtual ~IndexBuilder() {}

//向索引块添加新索引项。
//为了进一步优化，我们提供了“当前块中的最后一个密钥”和
//‘下一个块中的第一个_键‘，具体的实现可以基于它
//确定要用于索引块的最佳索引键。
//@last_key_in_current_块：此参数可能被值覆盖
//“替换键”。
//@first_key_in_next_block:如果要添加的条目是
//桌子上最后一个
//
//要求：尚未调用finish（）。
  virtual void AddIndexEntry(std::string* last_key_in_current_block,
                             const Slice* first_key_in_next_block,
                             const BlockHandle& block_handle) = 0;

//每当添加键时都将调用此方法。子类可以
//如果需要收集附加信息，则重写onkeyAdded（）。
  virtual void OnKeyAdded(const Slice& key) {}

//通知索引生成器所有条目都已写入。块生成器
//因此，可以执行块定型所需的任何操作。
//
//要求：尚未调用finish（）。
  inline Status Finish(IndexBlocks* index_blocks) {
//丢弃最后一个分区块句柄的更改。它没有效果
//不管怎样，在第一次通话结束时。
    BlockHandle last_partition_block_handle;
    return Finish(index_blocks, last_partition_block_handle);
  }

//此完成重写可用于在
//分区索引生成器。
//
//索引块将用生成的索引数据填充。如果返回
//值为status:：incomplete（），则表示索引已分区
//被调用方应该一直保持调用完成，直到返回状态：：OK（）。
//在这种情况下，最后一个分区块句柄是指向写入的块的指针
//最后一个要完成的调用的结果。这可用于构建
//指向每个分区索引块的第二级索引。这个
//最后一个返回status:：ok（）的finish（）调用使用
//二级索引内容。
  virtual Status Finish(IndexBlocks* index_blocks,
                        const BlockHandle& last_partition_block_handle) = 0;

//获取索引块的估计大小。
  virtual size_t EstimatedSize() const = 0;

 protected:
  const InternalKeyComparator* comparator_;
};

//此索引生成器生成节省空间的索引块。
//
//优化：
//1。使块的“块重新启动间隔”为1，这将避免线性
//执行索引查找时搜索（可以通过设置禁用
//索引块重新启动间隔）。
//2。缩短索引块的键长度。除了诚实地使用
//数据块中的最后一个键作为索引键，我们会找到一个最短的
//替换具有相同功能的键。
class ShortenedIndexBuilder : public IndexBuilder {
 public:
  explicit ShortenedIndexBuilder(const InternalKeyComparator* comparator,
                                 int index_block_restart_interval)
      : IndexBuilder(comparator),
        index_block_builder_(index_block_restart_interval) {}

  virtual void AddIndexEntry(std::string* last_key_in_current_block,
                             const Slice* first_key_in_next_block,
                             const BlockHandle& block_handle) override {
    if (first_key_in_next_block != nullptr) {
      comparator_->FindShortestSeparator(last_key_in_current_block,
                                         *first_key_in_next_block);
    } else {
      comparator_->FindShortSuccessor(last_key_in_current_block);
    }

    std::string handle_encoding;
    block_handle.EncodeTo(&handle_encoding);
    index_block_builder_.Add(*last_key_in_current_block, handle_encoding);
  }

  using IndexBuilder::Finish;
  virtual Status Finish(
      IndexBlocks* index_blocks,
      const BlockHandle& last_partition_block_handle) override {
    index_blocks->index_block_contents = index_block_builder_.Finish();
    return Status::OK();
  }

  virtual size_t EstimatedSize() const override {
    return index_block_builder_.CurrentSizeEstimate();
  }

  friend class PartitionedIndexBuilder;

 private:
  BlockBuilder index_block_builder_;
};

//HashIndexBuilder包含二进制可搜索主索引和
//二级哈希索引构造的元数据。
//哈希索引的元数据由两部分组成：
//-包含一系列前缀的元块。所有前缀
//连续存储，没有任何元数据（如前缀大小）
//存储，保存在其他元数据库中。
//-元块包含前缀的元数据，包括前缀大小，
//重新启动索引和它所跨越的块数。格式如下：
//
//+————————+——————————+——————————+————————————+—————————+
//<前缀1
//长度：4字节重启间隔：4字节数字块：4字节
//+————————+——————————+——————————+————————————+—————————+
//<前缀2
//长度：4字节重启间隔：4字节数字块：4字节
//+————————+——————————+——————————+————————————+—————————+
//_
//….γ
//_
//+————————+——————————+——————————+————————————+—————————+
//<前缀n
//长度：4字节重启间隔：4字节数字块：4字节
//+————————+——————————+——————————+————————————+—————————+
//
//分离这两个元块的原因是为了使
//在哈希索引构造期间重用第一个元块，而不必
//前缀的数据复制或小堆分配。
class HashIndexBuilder : public IndexBuilder {
 public:
  explicit HashIndexBuilder(const InternalKeyComparator* comparator,
                            const SliceTransform* hash_key_extractor,
                            int index_block_restart_interval)
      : IndexBuilder(comparator),
        primary_index_builder_(comparator, index_block_restart_interval),
        hash_key_extractor_(hash_key_extractor) {}

  virtual void AddIndexEntry(std::string* last_key_in_current_block,
                             const Slice* first_key_in_next_block,
                             const BlockHandle& block_handle) override {
    ++current_restart_index_;
    primary_index_builder_.AddIndexEntry(last_key_in_current_block,
                                         first_key_in_next_block, block_handle);
  }

  virtual void OnKeyAdded(const Slice& key) override {
    auto key_prefix = hash_key_extractor_->Transform(key);
    bool is_first_entry = pending_block_num_ == 0;

//键可以共享前缀
    if (is_first_entry || pending_entry_prefix_ != key_prefix) {
      if (!is_first_entry) {
        FlushPendingPrefix();
      }

//需要一个硬拷贝，否则底层数据会一直更改。
//todo（kailiu）tostring（）很贵。我们可以加快速度避免数据
//复制。
      pending_entry_prefix_ = key_prefix.ToString();
      pending_block_num_ = 1;
      pending_entry_index_ = static_cast<uint32_t>(current_restart_index_);
    } else {
//当键共享前缀驻留在
//不同的数据块。
      auto last_restart_index = pending_entry_index_ + pending_block_num_ - 1;
      assert(last_restart_index <= current_restart_index_);
      if (last_restart_index != current_restart_index_) {
        ++pending_block_num_;
      }
    }
  }

  virtual Status Finish(
      IndexBlocks* index_blocks,
      const BlockHandle& last_partition_block_handle) override {
    FlushPendingPrefix();
    primary_index_builder_.Finish(index_blocks, last_partition_block_handle);
    index_blocks->meta_blocks.insert(
        {kHashIndexPrefixesBlock.c_str(), prefix_block_});
    index_blocks->meta_blocks.insert(
        {kHashIndexPrefixesMetadataBlock.c_str(), prefix_meta_block_});
    return Status::OK();
  }

  virtual size_t EstimatedSize() const override {
    return primary_index_builder_.EstimatedSize() + prefix_block_.size() +
           prefix_meta_block_.size();
  }

 private:
  void FlushPendingPrefix() {
    prefix_block_.append(pending_entry_prefix_.data(),
                         pending_entry_prefix_.size());
    PutVarint32Varint32Varint32(
        &prefix_meta_block_,
        static_cast<uint32_t>(pending_entry_prefix_.size()),
        pending_entry_index_, pending_block_num_);
  }

  ShortenedIndexBuilder primary_index_builder_;
  const SliceTransform* hash_key_extractor_;

//存储前缀序列
  std::string prefix_block_;
//存储前缀的元数据
  std::string prefix_meta_block_;

//以下3个变量保留未刷新的前缀及其元数据。
//块编号和条目索引的详细信息可以在
//“block_hash_index.h，cc”
  uint32_t pending_block_num_ = 0;
  uint32_t pending_entry_index_ = 0;
  std::string pending_entry_prefix_;

  uint64_t current_restart_index_ = 0;
};

/*
 *用于两级索引的indexbuilder。在内部，它为
 *在每个分区上调用finish时按顺序完成
 *持续到返回状态：：OK（）。
 *
 *磁盘上的格式为I I I I I I IP，其中I块包含
 *使用shorteindexbuilder和ip构建的索引分区是一个块
 *包含分区上的辅助索引，使用
 *缩短索引生成器。
 **/

class PartitionedIndexBuilder : public IndexBuilder {
 public:
  static PartitionedIndexBuilder* CreateIndexBuilder(
      const rocksdb::InternalKeyComparator* comparator,
      const BlockBasedTableOptions& table_opt);

  explicit PartitionedIndexBuilder(const InternalKeyComparator* comparator,
                                   const BlockBasedTableOptions& table_opt);

  virtual ~PartitionedIndexBuilder();

  virtual void AddIndexEntry(std::string* last_key_in_current_block,
                             const Slice* first_key_in_next_block,
                             const BlockHandle& block_handle) override;

  virtual Status Finish(
      IndexBlocks* index_blocks,
      const BlockHandle& last_partition_block_handle) override;

  virtual size_t EstimatedSize() const override;
  size_t EstimateTopLevelIndexSize(uint64_t) const;
  size_t NumPartitions() const;

  inline bool ShouldCutFilterBlock() {
//当前策略是对齐索引和筛选器的分区
    if (cut_filter_block) {
      cut_filter_block = false;
      return true;
    }
    return false;
  }

  std::string& GetPartitionKey() { return sub_index_last_key_; }

//当外部实体（如筛选分区生成器）请求时调用
//剪切下一个分区
  void RequestPartitionCut();

 private:
  void MakeNewSubIndexBuilder();

  struct Entry {
    std::string key;
    std::unique_ptr<ShortenedIndexBuilder> value;
  };
std::list<Entry> entries_;  //分区索引及其键的列表
BlockBuilder index_block_builder_;  //顶级索引生成器
//活动分区索引生成器
  ShortenedIndexBuilder* sub_index_builder_;
//活动分区索引生成器中的最后一个键
  std::string sub_index_last_key_;
  std::unique_ptr<FlushBlockPolicy> flush_policy_;
//如果finish调用一次但尚未完成，则为true。
  bool finishing_indexes = false;
  const BlockBasedTableOptions& table_opt_;
//如果外部实体（如筛选器分区生成器）请求，则为true
//剪切下一个分区
  bool partition_cut_requested_ = true;
//如果它应该剪切下一个筛选器分区块，则为true
  bool cut_filter_block = false;
};
}  //命名空间rocksdb
