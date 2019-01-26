
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

#include <stdint.h>
#include "rocksdb/status.h"

namespace rocksdb {

class Comparator;
class Iterator;
class Slice;
class SliceTransform;

//构建一个基于哈希的索引，以加快对“索引块”的查找。
//BlockHashIndex接受一个键，如果找到该键，则返回其在
//那个索引块。
class BlockPrefixIndex {
 public:

//将键映射到可能包含
//基于前缀的键。
//返回相关块的总数，0表示键
//不存在。
  uint32_t GetBlocks(const Slice& key, uint32_t** blocks);

  size_t ApproximateMemoryUsage() const {
    return sizeof(BlockPrefixIndex) +
      (num_block_array_buffer_entries_ + num_buckets_) * sizeof(uint32_t);
  }

//通过从元数据块中读取来创建哈希索引。
//@params prefixes:前缀序列。
//@params prefix_meta:包含前缀中的“metadata”to。
  static Status Create(const SliceTransform* hash_key_extractor,
                       const Slice& prefixes, const Slice& prefix_meta,
                       BlockPrefixIndex** prefix_index);

  ~BlockPrefixIndex() {
    delete[] buckets_;
    delete[] block_array_buffer_;
  }

 private:
  class Builder;
  friend Builder;

  BlockPrefixIndex(const SliceTransform* internal_prefix_extractor,
                   uint32_t num_buckets,
                   uint32_t* buckets,
                   uint32_t num_block_array_buffer_entries,
                   uint32_t* block_array_buffer)
      : internal_prefix_extractor_(internal_prefix_extractor),
        num_buckets_(num_buckets),
        num_block_array_buffer_entries_(num_block_array_buffer_entries),
        buckets_(buckets),
        block_array_buffer_(block_array_buffer) {}

  const SliceTransform* internal_prefix_extractor_;
  uint32_t num_buckets_;
  uint32_t num_block_array_buffer_entries_;
  uint32_t* buckets_;
  uint32_t* block_array_buffer_;
};

}  //命名空间rocksdb
