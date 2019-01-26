
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

#include "table/full_filter_block.h"

#include "monitoring/perf_context_imp.h"
#include "port/port.h"
#include "rocksdb/filter_policy.h"
#include "util/coding.h"

namespace rocksdb {

FullFilterBlockBuilder::FullFilterBlockBuilder(
    const SliceTransform* prefix_extractor, bool whole_key_filtering,
    FilterBitsBuilder* filter_bits_builder)
    : prefix_extractor_(prefix_extractor),
      whole_key_filtering_(whole_key_filtering),
      num_added_(0) {
  assert(filter_bits_builder != nullptr);
  filter_bits_builder_.reset(filter_bits_builder);
}

void FullFilterBlockBuilder::Add(const Slice& key) {
  if (whole_key_filtering_) {
    AddKey(key);
  }
  if (prefix_extractor_ && prefix_extractor_->InDomain(key)) {
    AddPrefix(key);
  }
}

//如果需要，添加筛选键
inline void FullFilterBlockBuilder::AddKey(const Slice& key) {
  filter_bits_builder_->AddKey(key);
  num_added_++;
}

//如果需要，添加前缀到筛选器
inline void FullFilterBlockBuilder::AddPrefix(const Slice& key) {
  Slice prefix = prefix_extractor_->Transform(key);
  AddKey(prefix);
}

Slice FullFilterBlockBuilder::Finish(const BlockHandle& tmp, Status* status) {
//在这个例子中，我们忽略blockhandle
  *status = Status::OK();
  if (num_added_ != 0) {
    num_added_ = 0;
    return filter_bits_builder_->Finish(&filter_data_);
  }
  return Slice();
}

FullFilterBlockReader::FullFilterBlockReader(
    const SliceTransform* prefix_extractor, bool _whole_key_filtering,
    const Slice& contents, FilterBitsReader* filter_bits_reader,
    Statistics* stats)
    : FilterBlockReader(contents.size(), stats, _whole_key_filtering),
      prefix_extractor_(prefix_extractor),
      contents_(contents) {
  assert(filter_bits_reader != nullptr);
  filter_bits_reader_.reset(filter_bits_reader);
}

FullFilterBlockReader::FullFilterBlockReader(
    const SliceTransform* prefix_extractor, bool _whole_key_filtering,
    BlockContents&& contents, FilterBitsReader* filter_bits_reader,
    Statistics* stats)
    : FullFilterBlockReader(prefix_extractor, _whole_key_filtering,
                            contents.data, filter_bits_reader, stats) {
  block_contents_ = std::move(contents);
}

bool FullFilterBlockReader::KeyMayMatch(const Slice& key, uint64_t block_offset,
                                        const bool no_io,
                                        const Slice* const const_ikey_ptr) {
  assert(block_offset == kNotValid);
  if (!whole_key_filtering_) {
    return true;
  }
  return MayMatch(key);
}

bool FullFilterBlockReader::PrefixMayMatch(const Slice& prefix,
                                           uint64_t block_offset,
                                           const bool no_io,
                                           const Slice* const const_ikey_ptr) {
  assert(block_offset == kNotValid);
  if (!prefix_extractor_) {
    return true;
  }
  return MayMatch(prefix);
}

bool FullFilterBlockReader::MayMatch(const Slice& entry) {
  if (contents_.size() != 0)  {
    if (filter_bits_reader_->MayMatch(entry)) {
      PERF_COUNTER_ADD(bloom_sst_hit_count, 1);
      return true;
    } else {
      PERF_COUNTER_ADD(bloom_sst_miss_count, 1);
      return false;
    }
  }
return true;  //与基于块的过滤器保持相同
}

size_t FullFilterBlockReader::ApproximateMemoryUsage() const {
  return contents_.size();
}
}  //命名空间rocksdb
