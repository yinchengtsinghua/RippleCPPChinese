
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

#include "table/partitioned_filter_block.h"

#include <utility>

#include "monitoring/perf_context_imp.h"
#include "port/port.h"
#include "rocksdb/filter_policy.h"
#include "table/block.h"
#include "table/block_based_table_reader.h"
#include "util/coding.h"

namespace rocksdb {

PartitionedFilterBlockBuilder::PartitionedFilterBlockBuilder(
    const SliceTransform* prefix_extractor, bool whole_key_filtering,
    FilterBitsBuilder* filter_bits_builder, int index_block_restart_interval,
    PartitionedIndexBuilder* const p_index_builder,
    const uint32_t partition_size)
    : FullFilterBlockBuilder(prefix_extractor, whole_key_filtering,
                             filter_bits_builder),
      index_on_filter_block_builder_(index_block_restart_interval),
      p_index_builder_(p_index_builder),
      filters_in_partition_(0) {
  filters_per_partition_ =
      filter_bits_builder_->CalculateNumEntry(partition_size);
}

PartitionedFilterBlockBuilder::~PartitionedFilterBlockBuilder() {}

void PartitionedFilterBlockBuilder::MaybeCutAFilterBlock() {
//使用==只发送一次请求
  if (filters_in_partition_ == filters_per_partition_) {
//目前只有索引生成器负责分割。我们保持
//请求，直到获得批准。
    p_index_builder_->RequestPartitionCut();
  }
  if (!p_index_builder_->ShouldCutFilterBlock()) {
    return;
  }
  filter_gc.push_back(std::unique_ptr<const char[]>(nullptr));
  Slice filter = filter_bits_builder_->Finish(&filter_gc.back());
  std::string& index_key = p_index_builder_->GetPartitionKey();
  filters.push_back({index_key, filter});
  filters_in_partition_ = 0;
}

void PartitionedFilterBlockBuilder::AddKey(const Slice& key) {
  MaybeCutAFilterBlock();
  filter_bits_builder_->AddKey(key);
  filters_in_partition_++;
}

Slice PartitionedFilterBlockBuilder::Finish(
    const BlockHandle& last_partition_block_handle, Status* status) {
  if (finishing_filters == true) {
//记录索引中最后写入的筛选器块的句柄
    FilterEntry& last_entry = filters.front();
    std::string handle_encoding;
    last_partition_block_handle.EncodeTo(&handle_encoding);
    index_on_filter_block_builder_.Add(last_entry.key, handle_encoding);
    filters.pop_front();
  } else {
    MaybeCutAFilterBlock();
  }
//如果没有剩余的筛选器分区，则返回筛选器的索引
//隔板
  if (UNLIKELY(filters.empty())) {
    *status = Status::OK();
    if (finishing_filters) {
      return index_on_filter_block_builder_.Finish();
    } else {
//在这种情况下，很少有密钥被添加到过滤器中。
      return Slice();
    }
  } else {
//返回第行中的下一个筛选器分区，并将incomplete（）状态设置为
//表示我们希望完成更多呼叫
    *status = Status::Incomplete();
    finishing_filters = true;
    return filters.front().filter;
  }
}

PartitionedFilterBlockReader::PartitionedFilterBlockReader(
    const SliceTransform* prefix_extractor, bool _whole_key_filtering,
    BlockContents&& contents, FilterBitsReader* filter_bits_reader,
    Statistics* stats, const Comparator& comparator,
    const BlockBasedTable* table)
    : FilterBlockReader(contents.data.size(), stats, _whole_key_filtering),
      prefix_extractor_(prefix_extractor),
      comparator_(comparator),
      table_(table) {
  idx_on_fltr_blk_.reset(new Block(std::move(contents),
                                   kDisableGlobalSequenceNumber,
                                   /**读取每一位的字节数，统计数据）；
}

PartitionedFilterBlockReader:：~PartitionedFilterBlockReader（）
  //todo（myabandeh）：如果不是筛选对象，而是只存储块
  //块缓存，那么我们不必手动从块缓存中搜索它们
  /这里。
  auto block_cache=table_->rep_->table_options.block_cache.get（）；
  if（不太可能（block_cache==nullptr））
    返回；
  }
  char cache_key[blockBasedTable:：kmaxcachekeyPrefixSize+kmaxVarint64长度]；
  Blockiter Biter；
  块状手柄；
  idx_on_fltr_blk_uu->newiterator（&comparator_u，&biter，true）；
  biter.seektofirst（）；
  for（；biter.valid（）；biter.next（））
    自动输入=biter.value（）；
    auto s=handle.decodefrom（&input）；
    断言（S.O.（））；
    如果（！）S.O.（））{
      继续；
    }
    auto key=blockbasedtable:：getcachekey（table_->rep_->cache_key_prefix，
                                            表_u->rep_->cache_key_prefix_size，
                                            句柄，缓存键）；
    块缓存->擦除（键）；
  }
}

bool partitionedfilterblockreader:：keymaymatch（）。
    const slice&key，uint64_t block_offset，const bool no_io，
    常量切片*const常量ikey_ptr）
  断言（const-ikey-ptr！= null pTr）；
  断言（block_offset==knotvalid）；
  如果（！）整个_键_过滤_
    回归真实；
  }
  if（不太可能（idx_on_fltr_blk_u->size（）==0））
    回归真实；
  }
  auto filter_handle=getfilterpartitionhandle（*const_ikey_ptr）；
  if（不太可能（filter_handle.size（）==0））//键超出范围
    返回错误；
  }
  bool cached=false；
  auto filter_partition=getfilterpartition（nullptr/*预取缓冲区）*/,

                                             &filter_handle, no_io, &cached);
  if (UNLIKELY(!filter_partition.value)) {
    return true;
  }
  auto res = filter_partition.value->KeyMayMatch(key, block_offset, no_io);
  if (cached) {
    return res;
  }
  if (LIKELY(filter_partition.IsSet())) {
    filter_partition.Release(table_->rep_->table_options.block_cache.get());
  } else {
    delete filter_partition.value;
  }
  return res;
}

bool PartitionedFilterBlockReader::PrefixMayMatch(
    const Slice& prefix, uint64_t block_offset, const bool no_io,
    const Slice* const const_ikey_ptr) {
  assert(const_ikey_ptr != nullptr);
  assert(block_offset == kNotValid);
  if (!prefix_extractor_) {
    return true;
  }
  if (UNLIKELY(idx_on_fltr_blk_->size() == 0)) {
    return true;
  }
  auto filter_handle = GetFilterPartitionHandle(*const_ikey_ptr);
if (UNLIKELY(filter_handle.size() == 0)) {  //前缀超出范围
    return false;
  }
  bool cached = false;
  /*o filter_partition=getfilterpartition（nullptr/*预取缓冲区*/，
                                             &filter_handle，no_io，&cached）；
  如果（不可能）filter_partition.value））
    回归真实；
  }
  auto res=filter_partition.value->prefixmaymatch（prefix，knotvalid，no_io）；
  如果（缓存）{
    收益率；
  }
  if（可能（filter_partition.isset（））
    filter_partition.release（table_->rep_->table_options.block_cache.get（））；
  }否则{
    删除filter_partition.value；
  }
  收益率；
}

slice partitionedfilterblockreader:：getfilterpartitionhandle（
    常量切片和条目）
  阻断剂ITER；
  idx_on_fltr_blk_uu->newiterator（&comparator_u，&iter，true）；
  iter.seek（进入）；
  如果（不可能）iter.valid（））
    返回片段（）；
  }
  断言（iter.valid（））；
  切片句柄_value=iter.value（）；
  返回句柄值；
}

blockBasedTable:：cachableEntry<filterBlockReader>
partitionedfilterblockreader:：getfilterpartition（
    fileprefetchbuffer*预取缓冲区，slice*句柄值，const bool no_io，
    BOO*缓存（{）
  blockhandle fltr_k_句柄；
  auto s=fltr_blk_handle.decodeFrom（handle_value）；
  断言（S.O.（））；
  const bool是_a_filter_partition=true；
  auto block_cache=table_->rep_->table_options.block_cache.get（）；
  如果（可能）（阻止缓存！{null pTr）{
    如果（filter_map_.size（）！= 0）{
      auto iter=过滤器映射查找（fltr_blk_handle.offset（））；
      //这是一种可能的情况，因为块缓存可能没有空间
      //对于分区
      如果（ITER）！=filter_map_.end（））
        性能计数器添加（块缓存命中计数，1）；
        recordtick（statistics（），block_cache_filter_hit）；
        recordtick（statistics（），block_cache_hit）；
        recordtick（statistics（），block_cache_bytes_read，
                   block_cache->getusage（iter->second.cache_handle））；
        *缓存=真；
        返回iter->second；
      }
    }
    返回表_u->getfilter（/*prefetch_buffe*/ nullptr, fltr_blk_handle,

                             is_a_filter_partition, no_io);
  } else {
    auto filter = table_->ReadFilter(prefetch_buffer, fltr_blk_handle,
                                     is_a_filter_partition);
    return {filter, nullptr};
  }
}

size_t PartitionedFilterBlockReader::ApproximateMemoryUsage() const {
  return idx_on_fltr_blk_->size();
}

//TODO（myabandeh）：将其与indexreader中的相同函数合并
void PartitionedFilterBlockReader::CacheDependencies(bool pin) {
//在读取分区之前，预取它们以避免大量的IO
  auto rep = table_->rep_;
  BlockIter biter;
  BlockHandle handle;
  idx_on_fltr_blk_->NewIterator(&comparator_, &biter, true);
//假定索引分区是连续的。把它们都预取下来。
//读取第一个块偏移量
  biter.SeekToFirst();
  Slice input = biter.value();
  Status s = handle.DecodeFrom(&input);
  assert(s.ok());
  if (!s.ok()) {
    ROCKS_LOG_WARN(rep->ioptions.info_log,
                   "Could not read first index partition");
    return;
  }
  uint64_t prefetch_off = handle.offset();

//读取最后一个块的偏移量
  biter.SeekToLast();
  input = biter.value();
  s = handle.DecodeFrom(&input);
  assert(s.ok());
  if (!s.ok()) {
    ROCKS_LOG_WARN(rep->ioptions.info_log,
                   "Could not read last index partition");
    return;
  }
  uint64_t last_off = handle.offset() + handle.size() + kBlockTrailerSize;
  uint64_t prefetch_len = last_off - prefetch_off;
  std::unique_ptr<FilePrefetchBuffer> prefetch_buffer;
  auto& file = table_->rep_->file;
  prefetch_buffer.reset(new FilePrefetchBuffer());
  s = prefetch_buffer->Prefetch(file.get(), prefetch_off, prefetch_len);

//预取后，逐个读取分区
  biter.SeekToFirst();
  Cache* block_cache = rep->table_options.block_cache.get();
  for (; biter.Valid(); biter.Next()) {
    input = biter.value();
    s = handle.DecodeFrom(&input);
    assert(s.ok());
    if (!s.ok()) {
      ROCKS_LOG_WARN(rep->ioptions.info_log, "Could not read index partition");
      continue;
    }

    const bool no_io = true;
    const bool is_a_filter_partition = true;
    auto filter = table_->GetFilter(prefetch_buffer.get(), handle,
                                    is_a_filter_partition, !no_io);
    if (LIKELY(filter.IsSet())) {
      if (pin) {
        filter_map_[handle.offset()] = std::move(filter);
      } else {
        block_cache->Release(filter.cache_handle);
      }
    } else {
      delete filter.value;
    }
  }
}

}  //命名空间rocksdb
