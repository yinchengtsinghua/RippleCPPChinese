
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#ifndef ROCKSDB_LITE
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <stdint.h>

#include "db/dbformat.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "table/table_reader.h"
#include "table/plain_table_factory.h"
#include "table/plain_table_index.h"
#include "util/arena.h"
#include "util/dynamic_bloom.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class Block;
struct BlockContents;
class BlockHandle;
class Footer;
struct Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;
class TableReader;
class InternalKeyComparator;
class PlainTableKeyDecoder;
class GetContext;
class InternalIterator;

using std::unique_ptr;
using std::unordered_map;
using std::vector;
extern const uint32_t kPlainTableVariableLength;

struct PlainTableReaderFileInfo {
  bool is_mmap_mode;
  Slice file_data;
  uint32_t data_end_offset;
  unique_ptr<RandomAccessFileReader> file;

  PlainTableReaderFileInfo(unique_ptr<RandomAccessFileReader>&& _file,
                           const EnvOptions& storage_options,
                           uint32_t _data_size_offset)
      : is_mmap_mode(storage_options.use_mmap_reads),
        data_end_offset(_data_size_offset),
        file(std::move(_file)) {}
};

//基于以下输出文件格式，如plain_table_factory.h所示
//打开输出文件时，indexedTableReader创建哈希表
//从键前缀到输出文件的偏移量。索引表将决定
//是否指向带有键前缀的第一个键的数据偏移量
//或者它的偏移量。如果有太多的键共享这个前缀，它将
//在磁盘上从后缀到偏移量创建一个二进制可搜索索引。
//
//indexedTableReader的实现需要mmaped输出文件
class PlainTableReader: public TableReader {
 public:
  static Status Open(const ImmutableCFOptions& ioptions,
                     const EnvOptions& env_options,
                     const InternalKeyComparator& internal_comparator,
                     unique_ptr<RandomAccessFileReader>&& file,
                     uint64_t file_size, unique_ptr<TableReader>* table,
                     const int bloom_bits_per_key, double hash_table_ratio,
                     size_t index_sparseness, size_t huge_page_tlb_size,
                     bool full_scan_mode);

  InternalIterator* NewIterator(const ReadOptions&,
                                Arena* arena = nullptr,
                                bool skip_filters = false) override;

  void Prepare(const Slice& target) override;

  Status Get(const ReadOptions&, const Slice& key, GetContext* get_context,
             bool skip_filters = false) override;

  uint64_t ApproximateOffsetOf(const Slice& key) override;

  uint32_t GetIndexSize() const { return index_.GetIndexSize(); }
  void SetupForCompaction() override;

  std::shared_ptr<const TableProperties> GetTableProperties() const override {
    return table_properties_;
  }

  virtual size_t ApproximateMemoryUsage() const override {
    return arena_.MemoryAllocatedBytes();
  }

  PlainTableReader(const ImmutableCFOptions& ioptions,
                   unique_ptr<RandomAccessFileReader>&& file,
                   const EnvOptions& env_options,
                   const InternalKeyComparator& internal_comparator,
                   EncodingType encoding_type, uint64_t file_size,
                   const TableProperties* table_properties);
  virtual ~PlainTableReader();

 protected:
//检查bloom过滤器是否包含这个前缀。
//给出了前缀的散列，因为它可以重复用于索引查找。
//也是。
  virtual bool MatchBloom(uint32_t hash) const;

//populateIndex（）生成键的索引。必须在任何查询之前调用它
//在桌子旁边。
//
//props：需要存储的表属性对象。所有权
//对象将被传递。
//

  Status PopulateIndex(TableProperties* props, int bloom_bits_per_key,
                       double hash_table_ratio, size_t index_sparseness,
                       size_t huge_page_tlb_size);

  Status MmapDataIfNeeded();

 private:
  const InternalKeyComparator internal_comparator_;
  EncodingType encoding_type_;
//表示普通表的当前状态。
  Status status_;

  PlainTableIndex index_;
  bool full_scan_mode_;

//数据开始偏移和数据结束偏移定义了
//存储数据的sst文件。
  const uint32_t data_start_offset_ = 0;
  const uint32_t user_key_len_;
  const SliceTransform* prefix_extractor_;

  static const size_t kNumInternalBytes = 8;

//Bloom过滤器用于排除不存在的密钥
  bool enable_bloom_;
  DynamicBloom bloom_;
  PlainTableReaderFileInfo file_info_;
  Arena arena_;
  std::unique_ptr<char[]> index_block_alloc_;
  std::unique_ptr<char[]> bloom_block_alloc_;

  const ImmutableCFOptions& ioptions_;
  uint64_t file_size_;
  std::shared_ptr<const TableProperties> table_properties_;

  bool IsFixedLength() const {
    return user_key_len_ != kPlainTableVariableLength;
  }

  size_t GetFixedInternalKeyLength() const {
    return user_key_len_ + kNumInternalBytes;
  }

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

  friend class TableCache;
  friend class PlainTableIterator;

//内部帮助程序函数，用于从所有
//行，其中包含作为列表的索引记录。
//如果bloom不为空，则所有密钥的完整密钥哈希都将添加到
//布隆过滤器。
  Status PopulateIndexRecordList(PlainTableIndexBuilder* index_builder,
                                 vector<uint32_t>* prefix_hashes);

//为Bloom过滤器分配内存并填充内存的内部助手函数
  void AllocateAndFillBloom(int bloom_bits_per_key, int num_prefixes,
                            size_t huge_page_tlb_size,
                            vector<uint32_t>* prefix_hashes);

  void FillBloom(vector<uint32_t>* prefix_hashes);

//将“offset”处的键和值读取到键的参数，
//‘可寻’。
//成功后，“offset”将更新为下一个键的偏移量。
//`parsed_key`将是已分析格式的密钥。
//如果“internal_key”不为空，则将用带切片的键填充
//格式。
//如果“seecable”不为空，则返回是否可以直接读取
//使用此偏移量的数据。
  Status Next(PlainTableKeyDecoder* decoder, uint32_t* offset,
              ParsedInternalKey* parsed_key, Slice* internal_key, Slice* value,
              bool* seekable = nullptr) const;
//获取键目标的文件偏移量。
//如果确认偏移量，返回值前缀“匹配”设置为真。
//对于前缀与目标相同的键。
  Status GetOffset(PlainTableKeyDecoder* decoder, const Slice& target,
                   const Slice& prefix, uint32_t prefix_hash,
                   bool& prefix_matched, uint32_t* offset) const;

  bool IsTotalOrderMode() const { return (prefix_extractor_ == nullptr); }

//不允许复制
  explicit PlainTableReader(const TableReader&) = delete;
  void operator=(const TableReader&) = delete;
};
}  //命名空间rocksdb
#endif  //摇滚乐
