
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
#include "table/plain_table_builder.h"

#include <assert.h>

#include <string>
#include <limits>
#include <map>

#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "table/plain_table_factory.h"
#include "db/dbformat.h"
#include "table/block_builder.h"
#include "table/bloom_block.h"
#include "table/plain_table_index.h"
#include "table/format.h"
#include "table/meta_blocks.h"
#include "util/coding.h"
#include "util/crc32c.h"
#include "util/file_reader_writer.h"
#include "util/stop_watch.h"

namespace rocksdb {

namespace {

//帮助将块内容写入文件的实用程序
//如果成功写入@block\u内容，@offset将前进。
//@block_处理该块处理该特定块。
Status WriteBlock(const Slice& block_contents, WritableFileWriter* file,
                  uint64_t* offset, BlockHandle* block_handle) {
  block_handle->set_offset(*offset);
  block_handle->set_size(block_contents.size());
  Status s = file->Append(block_contents);

  if (s.ok()) {
    *offset += block_contents.size();
  }
  return s;
}

}  //命名空间

//kPlainTableMagicNumber是通过运行选择的
//回声岩d.b.table.plain_sha1sum
//取前导的64位。
extern const uint64_t kPlainTableMagicNumber = 0x8242229663bf9564ull;
extern const uint64_t kLegacyPlainTableMagicNumber = 0x4f3418eb7a8f13b8ull;

PlainTableBuilder::PlainTableBuilder(
    const ImmutableCFOptions& ioptions,
    const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
        int_tbl_prop_collector_factories,
    uint32_t column_family_id, WritableFileWriter* file, uint32_t user_key_len,
    EncodingType encoding_type, size_t index_sparseness,
    uint32_t bloom_bits_per_key, const std::string& column_family_name,
    uint32_t num_probes, size_t huge_page_tlb_size, double hash_table_ratio,
    bool store_index_in_file)
    : ioptions_(ioptions),
      bloom_block_(num_probes),
      file_(file),
      bloom_bits_per_key_(bloom_bits_per_key),
      huge_page_tlb_size_(huge_page_tlb_size),
      encoder_(encoding_type, user_key_len, ioptions.prefix_extractor,
               index_sparseness),
      store_index_in_file_(store_index_in_file),
      prefix_extractor_(ioptions.prefix_extractor) {
//如果哈希表比率大于0，则生成索引块并将其保存在文件中
  if (store_index_in_file_) {
    assert(hash_table_ratio > 0 || IsTotalOrderMode());
    index_builder_.reset(
        new PlainTableIndexBuilder(&arena_, ioptions, index_sparseness,
                                   hash_table_ratio, huge_page_tlb_size_));
    properties_.user_collected_properties
[PlainTablePropertyNames::kBloomVersion] = "1";  //供将来使用
  }

  properties_.fixed_key_len = user_key_len;

//对于普通表，我们将所有数据放在一个大夹盘中。
  properties_.num_data_blocks = 1;
//如果将索引存储在文件中，请稍后填写
  properties_.index_size = 0;
  properties_.filter_size = 0;
//要支持回滚到以前的版本，现在仍使用版本0
//普通编码。
  properties_.format_version = (encoding_type == kPlain) ? 0 : 1;
  properties_.column_family_id = column_family_id;
  properties_.column_family_name = column_family_name;
  properties_.prefix_extractor_name = ioptions_.prefix_extractor != nullptr
                                          ? ioptions_.prefix_extractor->Name()
                                          : "nullptr";

  std::string val;
  PutFixed32(&val, static_cast<uint32_t>(encoder_.GetEncodingType()));
  properties_.user_collected_properties
      [PlainTablePropertyNames::kEncodingType] = val;

  for (auto& collector_factories : *int_tbl_prop_collector_factories) {
    table_properties_collectors_.emplace_back(
        collector_factories->CreateIntTblPropCollector(column_family_id));
  }
}

PlainTableBuilder::~PlainTableBuilder() {
}

void PlainTableBuilder::Add(const Slice& key, const Slice& value) {
//键和值之间的元数据字节的临时缓冲区。
  char meta_bytes_buf[6];
  size_t meta_bytes_buf_size = 0;

  ParsedInternalKey internal_key;
  if (!ParseInternalKey(key, &internal_key)) {
    assert(false);
    return;
  }
  if (internal_key.type == kTypeRangeDeletion) {
    status_ = Status::NotSupported("Range deletion unsupported");
    return;
  }

//存储密钥散列
  if (store_index_in_file_) {
    if (ioptions_.prefix_extractor == nullptr) {
      keys_or_prefixes_hashes_.push_back(GetSliceHash(internal_key.user_key));
    } else {
      Slice prefix =
          ioptions_.prefix_extractor->Transform(internal_key.user_key);
      keys_or_prefixes_hashes_.push_back(GetSliceHash(prefix));
    }
  }

//写入值
  assert(offset_ <= std::numeric_limits<uint32_t>::max());
  auto prev_offset = static_cast<uint32_t>(offset_);
//写出钥匙
  encoder_.AppendKey(key, file_, &offset_, meta_bytes_buf,
                     &meta_bytes_buf_size);
  if (SaveIndexInFile()) {
    index_builder_->AddKeyPrefix(GetPrefix(internal_key), prev_offset);
  }

//写入值长度
  uint32_t value_size = static_cast<uint32_t>(value.size());
  char* end_ptr =
      EncodeVarint32(meta_bytes_buf + meta_bytes_buf_size, value_size);
  assert(end_ptr <= meta_bytes_buf + sizeof(meta_bytes_buf));
  meta_bytes_buf_size = end_ptr - meta_bytes_buf;
  file_->Append(Slice(meta_bytes_buf, meta_bytes_buf_size));

//写入值
  file_->Append(value);
  offset_ += value_size + meta_bytes_buf_size;

  properties_.num_entries++;
  properties_.raw_key_size += key.size();
  properties_.raw_value_size += value.size();

//通知属性收集器
  NotifyCollectTableCollectorsOnAdd(
      key, value, offset_, table_properties_collectors_, ioptions_.info_log);
}

Status PlainTableBuilder::status() const { return status_; }

Status PlainTableBuilder::Finish() {
  assert(!closed_);
  closed_ = true;

  properties_.data_size = offset_;

//写下面的块
//1。[元块：Bloom]-可选
//2。[元块：索引]-可选
//三。[元块：属性]
//4。[元索引块]
//5。[页脚]

  MetaIndexBuilder meta_index_builer;

  if (store_index_in_file_ && (properties_.num_entries > 0)) {
    assert(properties_.num_entries <= std::numeric_limits<uint32_t>::max());
    Status s;
    BlockHandle bloom_block_handle;
    if (bloom_bits_per_key_ > 0) {
      bloom_block_.SetTotalBits(
          &arena_,
          static_cast<uint32_t>(properties_.num_entries) * bloom_bits_per_key_,
          ioptions_.bloom_locality, huge_page_tlb_size_, ioptions_.info_log);

      PutVarint32(&properties_.user_collected_properties
                       [PlainTablePropertyNames::kNumBloomBlocks],
                  bloom_block_.GetNumBlocks());

      bloom_block_.AddKeysHashes(keys_or_prefixes_hashes_);

      Slice bloom_finish_result = bloom_block_.Finish();

      properties_.filter_size = bloom_finish_result.size();
      s = WriteBlock(bloom_finish_result, file_, &offset_, &bloom_block_handle);

      if (!s.ok()) {
        return s;
      }
      meta_index_builer.Add(BloomBlockBuilder::kBloomBlock, bloom_block_handle);
    }
    BlockHandle index_block_handle;
    Slice index_finish_result = index_builder_->Finish();

    properties_.index_size = index_finish_result.size();
    s = WriteBlock(index_finish_result, file_, &offset_, &index_block_handle);

    if (!s.ok()) {
      return s;
    }

    meta_index_builer.Add(PlainTableIndexBuilder::kPlainTableIndexBlock,
                          index_block_handle);
  }

//计算花块大小和索引块大小
  PropertyBlockBuilder property_block_builder;
//--添加基本属性
  property_block_builder.AddTableProperty(properties_);

  property_block_builder.Add(properties_.user_collected_properties);

//--添加用户收集的属性
  NotifyCollectTableCollectorsOnFinish(table_properties_collectors_,
                                       ioptions_.info_log,
                                       &property_block_builder);

//--写入属性块
  BlockHandle property_block_handle;
  auto s = WriteBlock(
      property_block_builder.Finish(),
      file_,
      &offset_,
      &property_block_handle
  );
  if (!s.ok()) {
    return s;
  }
  meta_index_builer.Add(kPropertiesBlock, property_block_handle);

//--写入元索引块
  BlockHandle metaindex_block_handle;
  s = WriteBlock(
      meta_index_builer.Finish(),
      file_,
      &offset_,
      &metaindex_block_handle
  );
  if (!s.ok()) {
    return s;
  }

//写入页脚
//如果使用默认校验和，则无需写出新的页脚。
  Footer footer(kLegacyPlainTableMagicNumber, 0);
  footer.set_metaindex_handle(metaindex_block_handle);
  footer.set_index_handle(BlockHandle::NullBlockHandle());
  std::string footer_encoding;
  footer.EncodeTo(&footer_encoding);
  s = file_->Append(footer_encoding);
  if (s.ok()) {
    offset_ += footer_encoding.size();
  }

  return s;
}

void PlainTableBuilder::Abandon() {
  closed_ = true;
}

uint64_t PlainTableBuilder::NumEntries() const {
  return properties_.num_entries;
}

uint64_t PlainTableBuilder::FileSize() const {
  return offset_;
}

}  //命名空间rocksdb
#endif  //摇滚乐
