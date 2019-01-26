
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

#include "table/block_prefix_index.h"

#include <vector>

#include "rocksdb/comparator.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "util/arena.h"
#include "util/coding.h"
#include "util/hash.h"

namespace rocksdb {

inline uint32_t Hash(const Slice& s) {
  return rocksdb::Hash(s.data(), s.size(), 0);
}

inline uint32_t PrefixToBucket(const Slice& prefix, uint32_t num_buckets) {
  return Hash(prefix) % num_buckets;
}

//前缀块索引只是一个bucket数组，每个条目都指向
//跨越前缀的块散列到此bucket。
//
//为了减少内存占用，如果每个bucket只有一个块，则输入
//直接存储块ID。如果每个桶有多个块，
//由于哈希冲突或跨越多个块的单个前缀，
//入口指向一个块ID数组。块数组是
//uint32_t's。第一个uint32_t表示随后的块总数。
//按块ID。
//
//为了区分这两种情况，条目的高阶位表示
//它是否是指向单独块数组的“指针”。
//0x7fffffff为空桶预留。

const uint32_t kNoneBlock = 0x7FFFFFFF;
const uint32_t kBlockArrayMask = 0x80000000;

inline bool IsNone(uint32_t block_id) {
  return block_id == kNoneBlock;
}

inline bool IsBlockId(uint32_t block_id) {
  return (block_id & kBlockArrayMask) == 0;
}

inline uint32_t DecodeIndex(uint32_t block_id) {
  uint32_t index = block_id ^ kBlockArrayMask;
  assert(index < kBlockArrayMask);
  return index;
}

inline uint32_t EncodeIndex(uint32_t index) {
  assert(index < kBlockArrayMask);
  return index | kBlockArrayMask;
}

//索引生成期间前缀信息的临时存储
struct PrefixRecord {
  Slice prefix;
  uint32_t start_block;
  uint32_t end_block;
  uint32_t num_blocks;
  PrefixRecord* next;
};

class BlockPrefixIndex::Builder {
 public:
  explicit Builder(const SliceTransform* internal_prefix_extractor)
      : internal_prefix_extractor_(internal_prefix_extractor) {}

  void Add(const Slice& key_prefix, uint32_t start_block,
           uint32_t num_blocks) {
    PrefixRecord* record = reinterpret_cast<PrefixRecord*>(
      arena_.AllocateAligned(sizeof(PrefixRecord)));
    record->prefix = key_prefix;
    record->start_block = start_block;
    record->end_block = start_block + num_blocks - 1;
    record->num_blocks = num_blocks;
    prefixes_.push_back(record);
  }

  BlockPrefixIndex* Finish() {
//现在，使用大约1:1的前缀与桶的比率。
    uint32_t num_buckets = static_cast<uint32_t>(prefixes_.size()) + 1;

//将哈希到同一个bucket的前缀记录收集到单个bucket中
//链接列表。
    std::vector<PrefixRecord*> prefixes_per_bucket(num_buckets, nullptr);
    std::vector<uint32_t> num_blocks_per_bucket(num_buckets, 0);
    for (PrefixRecord* current : prefixes_) {
      uint32_t bucket = PrefixToBucket(current->prefix, num_buckets);
//如果前缀的第一个块是
//连接到上一个前缀的最后一个块。
      PrefixRecord* prev = prefixes_per_bucket[bucket];
      if (prev) {
        assert(current->start_block >= prev->end_block);
        auto distance = current->start_block - prev->end_block;
        if (distance <= 1) {
          prev->end_block = current->end_block;
          prev->num_blocks = prev->end_block - prev->start_block + 1;
          num_blocks_per_bucket[bucket] += (current->num_blocks + distance - 1);
          continue;
        }
      }
      current->next = prev;
      prefixes_per_bucket[bucket] = current;
      num_blocks_per_bucket[bucket] += current->num_blocks;
    }

//计算块数组缓冲区大小
    uint32_t total_block_array_entries = 0;
    for (uint32_t i = 0; i < num_buckets; i++) {
      uint32_t num_blocks = num_blocks_per_bucket[i];
      if (num_blocks > 1) {
        total_block_array_entries += (num_blocks + 1);
      }
    }

//填充最终前缀块索引
    uint32_t* block_array_buffer = new uint32_t[total_block_array_entries];
    uint32_t* buckets = new uint32_t[num_buckets];
    uint32_t offset = 0;
    for (uint32_t i = 0; i < num_buckets; i++) {
      uint32_t num_blocks = num_blocks_per_bucket[i];
      if (num_blocks == 0) {
        assert(prefixes_per_bucket[i] == nullptr);
        buckets[i] = kNoneBlock;
      } else if (num_blocks == 1) {
        assert(prefixes_per_bucket[i] != nullptr);
        assert(prefixes_per_bucket[i]->next == nullptr);
        buckets[i] = prefixes_per_bucket[i]->start_block;
      } else {
        assert(total_block_array_entries > 0);
        assert(prefixes_per_bucket[i] != nullptr);
        buckets[i] = EncodeIndex(offset);
        block_array_buffer[offset] = num_blocks;
        uint32_t* last_block = &block_array_buffer[offset + num_blocks];
        auto current = prefixes_per_bucket[i];
//从最大到最小填充块ID
        while (current != nullptr) {
          for (uint32_t iter = 0; iter < current->num_blocks; iter++) {
            *last_block = current->end_block - iter;
            last_block--;
          }
          current = current->next;
        }
        assert(last_block == &block_array_buffer[offset]);
        offset += (num_blocks + 1);
      }
    }

    assert(offset == total_block_array_entries);

    return new BlockPrefixIndex(internal_prefix_extractor_, num_buckets,
                                buckets, total_block_array_entries,
                                block_array_buffer);
  }

 private:
  const SliceTransform* internal_prefix_extractor_;

  std::vector<PrefixRecord*> prefixes_;
  Arena arena_;
};


Status BlockPrefixIndex::Create(const SliceTransform* internal_prefix_extractor,
                                const Slice& prefixes, const Slice& prefix_meta,
                                BlockPrefixIndex** prefix_index) {
  uint64_t pos = 0;
  auto meta_pos = prefix_meta;
  Status s;
  Builder builder(internal_prefix_extractor);

  while (!meta_pos.empty()) {
    uint32_t prefix_size = 0;
    uint32_t entry_index = 0;
    uint32_t num_blocks = 0;
    if (!GetVarint32(&meta_pos, &prefix_size) ||
        !GetVarint32(&meta_pos, &entry_index) ||
        !GetVarint32(&meta_pos, &num_blocks)) {
      s = Status::Corruption(
          "Corrupted prefix meta block: unable to read from it.");
      break;
    }
    if (pos + prefix_size > prefixes.size()) {
      s = Status::Corruption(
        "Corrupted prefix meta block: size inconsistency.");
      break;
    }
    Slice prefix(prefixes.data() + pos, prefix_size);
    builder.Add(prefix, entry_index, num_blocks);

    pos += prefix_size;
  }

  if (s.ok() && pos != prefixes.size()) {
    s = Status::Corruption("Corrupted prefix meta block");
  }

  if (s.ok()) {
    *prefix_index = builder.Finish();
  }

  return s;
}

uint32_t BlockPrefixIndex::GetBlocks(const Slice& key,
                                     uint32_t** blocks) {
  Slice prefix = internal_prefix_extractor_->Transform(key);

  uint32_t bucket = PrefixToBucket(prefix, num_buckets_);
  uint32_t block_id = buckets_[bucket];

  if (IsNone(block_id)) {
    return 0;
  } else if (IsBlockId(block_id)) {
    *blocks = &buckets_[bucket];
    return 1;
  } else {
    uint32_t index = DecodeIndex(block_id);
    assert(index < num_block_array_buffer_entries_);
    *blocks = &block_array_buffer_[index+1];
    uint32_t num_blocks = block_array_buffer_[index];
    assert(num_blocks > 1);
    assert(index + num_blocks < num_block_array_buffer_entries_);
    return num_blocks;
  }
}

}  //命名空间rocksdb
