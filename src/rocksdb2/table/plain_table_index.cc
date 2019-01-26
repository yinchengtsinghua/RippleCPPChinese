
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

#ifndef ROCKSDB_LITE

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include "table/plain_table_index.h"
#include "util/coding.h"
#include "util/hash.h"

namespace rocksdb {

namespace {
inline uint32_t GetBucketIdFromHash(uint32_t hash, uint32_t num_buckets) {
  assert(num_buckets > 0);
  return hash % num_buckets;
}
}

Status PlainTableIndex::InitFromRawData(Slice data) {
  if (!GetVarint32(&data, &index_size_)) {
    return Status::Corruption("Couldn't read the index size!");
  }
  assert(index_size_ > 0);
  if (!GetVarint32(&data, &num_prefixes_)) {
    return Status::Corruption("Couldn't read the index size!");
  }
  sub_index_size_ =
      static_cast<uint32_t>(data.size()) - index_size_ * kOffsetLen;

  char* index_data_begin = const_cast<char*>(data.data());
  index_ = reinterpret_cast<uint32_t*>(index_data_begin);
  sub_index_ = reinterpret_cast<char*>(index_ + index_size_);
  return Status::OK();
}

PlainTableIndex::IndexSearchResult PlainTableIndex::GetOffset(
    uint32_t prefix_hash, uint32_t* bucket_value) const {
  int bucket = GetBucketIdFromHash(prefix_hash, index_size_);
  GetUnaligned(index_ + bucket, bucket_value);
  if ((*bucket_value & kSubIndexMask) == kSubIndexMask) {
    *bucket_value ^= kSubIndexMask;
    return kSubindex;
  }
  if (*bucket_value >= kMaxFileSize) {
    return kNoPrefixForBucket;
  } else {
//直接指向文件
    return kDirectToFile;
  }
}

void PlainTableIndexBuilder::IndexRecordList::AddRecord(uint32_t hash,
                                                        uint32_t offset) {
  if (num_records_in_current_group_ == kNumRecordsPerGroup) {
    current_group_ = AllocateNewGroup();
    num_records_in_current_group_ = 0;
  }
  auto& new_record = current_group_[num_records_in_current_group_++];
  new_record.hash = hash;
  new_record.offset = offset;
  new_record.next = nullptr;
}

void PlainTableIndexBuilder::AddKeyPrefix(Slice key_prefix_slice,
                                          uint32_t key_offset) {
  if (is_first_record_ || prev_key_prefix_ != key_prefix_slice.ToString()) {
    ++num_prefixes_;
    if (!is_first_record_) {
      keys_per_prefix_hist_.Add(num_keys_per_prefix_);
    }
    num_keys_per_prefix_ = 0;
    prev_key_prefix_ = key_prefix_slice.ToString();
    prev_key_prefix_hash_ = GetSliceHash(key_prefix_slice);
    due_index_ = true;
  }

  if (due_index_) {
//为每个kindexintervalforameprefixkeys添加索引键
    record_list_.AddRecord(prev_key_prefix_hash_, key_offset);
    due_index_ = false;
  }

  num_keys_per_prefix_++;
  if (index_sparseness_ == 0 || num_keys_per_prefix_ % index_sparseness_ == 0) {
    due_index_ = true;
  }
  is_first_record_ = false;
}

Slice PlainTableIndexBuilder::Finish() {
  AllocateIndex();
  std::vector<IndexRecord*> hash_to_offsets(index_size_, nullptr);
  std::vector<uint32_t> entries_per_bucket(index_size_, 0);
  BucketizeIndexes(&hash_to_offsets, &entries_per_bucket);

  keys_per_prefix_hist_.Add(num_keys_per_prefix_);
  ROCKS_LOG_INFO(ioptions_.info_log, "Number of Keys per prefix Histogram: %s",
                 keys_per_prefix_hist_.ToString().c_str());

//从temp数据结构填充索引。
  return FillIndexes(hash_to_offsets, entries_per_bucket);
}

void PlainTableIndexBuilder::AllocateIndex() {
  if (prefix_extractor_ == nullptr || hash_table_ratio_ <= 0) {
//如果用户未能指定前缀，则返回纯二进制搜索
//提取器。
    index_size_ = 1;
  } else {
    double hash_table_size_multipier = 1.0 / hash_table_ratio_;
    index_size_ =
      static_cast<uint32_t>(num_prefixes_ * hash_table_size_multipier) + 1;
    assert(index_size_ > 0);
  }
}

void PlainTableIndexBuilder::BucketizeIndexes(
    std::vector<IndexRecord*>* hash_to_offsets,
    std::vector<uint32_t>* entries_per_bucket) {
  bool first = true;
  uint32_t prev_hash = 0;
  size_t num_records = record_list_.GetNumRecords();
  for (size_t i = 0; i < num_records; i++) {
    IndexRecord* index_record = record_list_.At(i);
    uint32_t cur_hash = index_record->hash;
    if (first || prev_hash != cur_hash) {
      prev_hash = cur_hash;
      first = false;
    }
    uint32_t bucket = GetBucketIdFromHash(cur_hash, index_size_);
    IndexRecord* prev_bucket_head = (*hash_to_offsets)[bucket];
    index_record->next = prev_bucket_head;
    (*hash_to_offsets)[bucket] = index_record;
    (*entries_per_bucket)[bucket]++;
  }

  sub_index_size_ = 0;
  for (auto entry_count : *entries_per_bucket) {
    if (entry_count <= 1) {
      continue;
    }
//只有条目超过1的bucket才会有子索引。
    sub_index_size_ += VarintLength(entry_count);
//在文件偏移量中存储这些项所需的总字节数。
    sub_index_size_ += entry_count * PlainTableIndex::kOffsetLen;
  }
}

Slice PlainTableIndexBuilder::FillIndexes(
    const std::vector<IndexRecord*>& hash_to_offsets,
    const std::vector<uint32_t>& entries_per_bucket) {
  ROCKS_LOG_DEBUG(ioptions_.info_log,
                  "Reserving %" PRIu32 " bytes for plain table's sub_index",
                  sub_index_size_);
  auto total_allocate_size = GetTotalSize();
  char* allocated = arena_->AllocateAligned(
      total_allocate_size, huge_page_tlb_size_, ioptions_.info_log);

  auto temp_ptr = EncodeVarint32(allocated, index_size_);
  uint32_t* index =
      reinterpret_cast<uint32_t*>(EncodeVarint32(temp_ptr, num_prefixes_));
  char* sub_index = reinterpret_cast<char*>(index + index_size_);

  uint32_t sub_index_offset = 0;
  for (uint32_t i = 0; i < index_size_; i++) {
    uint32_t num_keys_for_bucket = entries_per_bucket[i];
    switch (num_keys_for_bucket) {
      case 0:
//桶没有钥匙
        PutUnaligned(index + i, (uint32_t)PlainTableIndex::kMaxFileSize);
        break;
      case 1:
//直接指向文件偏移
        PutUnaligned(index + i, hash_to_offsets[i]->offset);
        break;
      default:
//指向二级索引。
        PutUnaligned(index + i, sub_index_offset | PlainTableIndex::kSubIndexMask);
        char* prev_ptr = &sub_index[sub_index_offset];
        char* cur_ptr = EncodeVarint32(prev_ptr, num_keys_for_bucket);
        sub_index_offset += static_cast<uint32_t>(cur_ptr - prev_ptr);
        char* sub_index_pos = &sub_index[sub_index_offset];
        IndexRecord* record = hash_to_offsets[i];
        int j;
        for (j = num_keys_for_bucket - 1; j >= 0 && record;
             j--, record = record->next) {
          EncodeFixed32(sub_index_pos + j * sizeof(uint32_t), record->offset);
        }
        assert(j == -1 && record == nullptr);
        sub_index_offset += PlainTableIndex::kOffsetLen * num_keys_for_bucket;
        assert(sub_index_offset <= sub_index_size_);
        break;
    }
  }
  assert(sub_index_offset == sub_index_size_);

  ROCKS_LOG_DEBUG(ioptions_.info_log,
                  "hash table size: %d, suffix_map length %" ROCKSDB_PRIszt,
                  index_size_, sub_index_size_);
  return Slice(allocated, GetTotalSize());
}

const std::string PlainTableIndexBuilder::kPlainTableIndexBlock =
    "PlainTableIndexBlock";
};  //命名空间rocksdb

#endif  //摇滚乐
