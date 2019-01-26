
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

#include <string>
#include <vector>

#include "db/dbformat.h"
#include "monitoring/histogram.h"
#include "options/cf_options.h"
#include "rocksdb/options.h"
#include "util/arena.h"
#include "util/hash.h"
#include "util/murmurhash.h"

namespace rocksdb {

//PlainTableIndex包含索引大小为的存储桶，每个存储桶都是
//32位整数。较低的31位包含一个偏移值（解释如下）
//整数的第一位表示偏移量的类型。
//
//+————————+———————————————————————————————————————————
//标志（1位）偏移到二进制搜索缓冲区或文件（31位）+
//+————————+———————————————————————————————————————————
//
//“标志位”的说明：
//
//0表示bucket只包含一个前缀（当
//散列这个前缀），其第一行从
//文件。
//1表示bucket包含多个前缀，或者
//一个前缀的行太多，因此我们需要对其进行二进制搜索。在
//这种情况下，偏移量表示持有
//这些行的键的二进制搜索索引。那些二进制搜索索引
//组织方式如下：
//
//前4个字节表示在它之后存储了多少索引（n）。后
//它有n个32位整数，文件的每一个偏移点，
//哪一个
//指向行的开头。这些补偿需要保证
//升序，因此它们所指向的键也是升序的
//秩序
//以确保我们可以使用它们进行二进制搜索。下面是视觉
//桶的表示。
//
//<开始>
//记录数：变量32
//记录1文件偏移量：固定32
//记录2文件偏移量：Fixedint32
//…
//记录n文件偏移量：Fixedint32
//<结束>
class PlainTableIndex {
 public:
  enum IndexSearchResult {
    kNoPrefixForBucket = 0,
    kDirectToFile = 1,
    kSubindex = 2
  };

  explicit PlainTableIndex(Slice data) { InitFromRawData(data); }

  PlainTableIndex()
      : index_size_(0),
        sub_index_size_(0),
        num_prefixes_(0),
        index_(nullptr),
        sub_index_(nullptr) {}

  IndexSearchResult GetOffset(uint32_t prefix_hash,
                              uint32_t* bucket_value) const;

  Status InitFromRawData(Slice data);

  const char* GetSubIndexBasePtrAndUpperBound(uint32_t offset,
                                              uint32_t* upper_bound) const {
    const char* index_ptr = &sub_index_[offset];
    return GetVarint32Ptr(index_ptr, index_ptr + 4, upper_bound);
  }

  uint32_t GetIndexSize() const { return index_size_; }

  uint32_t GetSubIndexSize() const { return sub_index_size_; }

  uint32_t GetNumPrefixes() const { return num_prefixes_; }

  static const uint64_t kMaxFileSize = (1u << 31) - 1;
  static const uint32_t kSubIndexMask = 0x80000000;
  static const size_t kOffsetLen = sizeof(uint32_t);

 private:
  uint32_t index_size_;
  uint32_t sub_index_size_;
  uint32_t num_prefixes_;

  uint32_t* index_;
  char* sub_index_;
};

//PlainTableIndexBuilder用于创建普通表索引。
//在调用finish（）之后，它返回slice，通常
//用于初始化PlainTableIndex或
//将索引保存到sst文件。
//有关索引的详细信息，请参阅：
//https://github.com/facebook/rocksdb/wiki/plaintable-format
//wiki内存索引格式
class PlainTableIndexBuilder {
 public:
  PlainTableIndexBuilder(Arena* arena, const ImmutableCFOptions& ioptions,
                         size_t index_sparseness, double hash_table_ratio,
                         size_t huge_page_tlb_size)
      : arena_(arena),
        ioptions_(ioptions),
        record_list_(kRecordsPerGroup),
        is_first_record_(true),
        due_index_(false),
        num_prefixes_(0),
        num_keys_per_prefix_(0),
        prev_key_prefix_hash_(0),
        index_sparseness_(index_sparseness),
        prefix_extractor_(ioptions.prefix_extractor),
        hash_table_ratio_(hash_table_ratio),
        huge_page_tlb_size_(huge_page_tlb_size) {}

  void AddKeyPrefix(Slice key_prefix_slice, uint32_t key_offset);

  Slice Finish();

  uint32_t GetTotalSize() const {
    return VarintLength(index_size_) + VarintLength(num_prefixes_) +
           PlainTableIndex::kOffsetLen * index_size_ + sub_index_size_;
  }

  static const std::string kPlainTableIndexBlock;

 private:
  struct IndexRecord {
uint32_t hash;    //前缀的哈希
uint32_t offset;  //行偏移
    IndexRecord* next;
  };

//用于跟踪所有索引记录的帮助程序类
  class IndexRecordList {
   public:
    explicit IndexRecordList(size_t num_records_per_group)
        : kNumRecordsPerGroup(num_records_per_group),
          current_group_(nullptr),
          num_records_in_current_group_(num_records_per_group) {}

    ~IndexRecordList() {
      for (size_t i = 0; i < groups_.size(); i++) {
        delete[] groups_[i];
      }
    }

    void AddRecord(uint32_t hash, uint32_t offset);

    size_t GetNumRecords() const {
      return (groups_.size() - 1) * kNumRecordsPerGroup +
             num_records_in_current_group_;
    }
    IndexRecord* At(size_t index) {
      return &(groups_[index / kNumRecordsPerGroup]
                      [index % kNumRecordsPerGroup]);
    }

   private:
    IndexRecord* AllocateNewGroup() {
      IndexRecord* result = new IndexRecord[kNumRecordsPerGroup];
      groups_.push_back(result);
      return result;
    }

//“groups”中的每个组都包含固定大小的记录（由
//KnumRecordsPerGroup）。这有助于我们在调整大小时最小化成本
//发生。
    const size_t kNumRecordsPerGroup;
    IndexRecord* current_group_;
//分配的数组列表
    std::vector<IndexRecord*> groups_;
    size_t num_records_in_current_group_;
  };

  void AllocateIndex();

//内部帮助器函数，用于将存储桶索引记录列表添加到哈希存储桶。
  void BucketizeIndexes(std::vector<IndexRecord*>* hash_to_offsets,
                        std::vector<uint32_t>* entries_per_bucket);

//用于将索引和Bloom筛选器填充到内部的内部帮助程序类
//数据结构。
  Slice FillIndexes(const std::vector<IndexRecord*>& hash_to_offsets,
                    const std::vector<uint32_t>& entries_per_bucket);

  Arena* arena_;
  const ImmutableCFOptions ioptions_;
  HistogramImpl keys_per_prefix_hist_;
  IndexRecordList record_list_;
  bool is_first_record_;
  bool due_index_;
  uint32_t num_prefixes_;
  uint32_t num_keys_per_prefix_;

  uint32_t prev_key_prefix_hash_;
  size_t index_sparseness_;
  uint32_t index_size_;
  uint32_t sub_index_size_;

  const SliceTransform* prefix_extractor_;
  double hash_table_ratio_;
  size_t huge_page_tlb_size_;

  std::string prev_key_prefix_;

  static const size_t kRecordsPerGroup = 256;
};

};  //命名空间rocksdb

#endif  //摇滚乐
