
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

#include "table/block_based_table_builder.h"

#include <assert.h>
#include <stdio.h>

#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "db/dbformat.h"

#include "rocksdb/cache.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/flush_block_policy.h"
#include "rocksdb/merge_operator.h"
#include "rocksdb/table.h"

#include "table/block.h"
#include "table/block_based_filter_block.h"
#include "table/block_based_table_factory.h"
#include "table/block_based_table_reader.h"
#include "table/block_builder.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "table/full_filter_block.h"
#include "table/meta_blocks.h"
#include "table/table_builder.h"

#include "util/string_util.h"
#include "util/coding.h"
#include "util/compression.h"
#include "util/crc32c.h"
#include "util/stop_watch.h"
#include "util/xxhash.h"

#include "table/index_builder.h"
#include "table/partitioned_filter_block.h"

namespace rocksdb {

extern const std::string kHashIndexPrefixesBlock;
extern const std::string kHashIndexPrefixesMetadataBlock;

typedef BlockBasedTableOptions::IndexType IndexType;

//如果这里没有匿名名称空间，我们将使警告失败-wMissing原型
namespace {

//基于其类型创建筛选器块生成器。
FilterBlockBuilder* CreateFilterBlockBuilder(
    const ImmutableCFOptions& opt, const BlockBasedTableOptions& table_opt,
    PartitionedIndexBuilder* const p_index_builder) {
  if (table_opt.filter_policy == nullptr) return nullptr;

  FilterBitsBuilder* filter_bits_builder =
      table_opt.filter_policy->GetFilterBitsBuilder();
  if (filter_bits_builder == nullptr) {
    return new BlockBasedFilterBlockBuilder(opt.prefix_extractor, table_opt);
  } else {
    if (table_opt.partition_filters) {
      assert(p_index_builder != nullptr);
//由于在从筛选器生成器中删除分区请求后，需要花费时间
//在索引生成器实际剪切分区之前，我们采用下界
//作为分区大小。
      assert(table_opt.block_size_deviation <= 100);
      auto partition_size = static_cast<uint32_t>(
          table_opt.metadata_block_size *
          (100 - table_opt.block_size_deviation));
      partition_size = std::max(partition_size, static_cast<uint32_t>(1));
      return new PartitionedFilterBlockBuilder(
          opt.prefix_extractor, table_opt.whole_key_filtering,
          filter_bits_builder, table_opt.index_block_restart_interval,
          p_index_builder, partition_size);
    } else {
      return new FullFilterBlockBuilder(opt.prefix_extractor,
                                        table_opt.whole_key_filtering,
                                        filter_bits_builder);
    }
  }
}

bool GoodCompressionRatio(size_t compressed_size, size_t raw_size) {
//检查压缩是否小于12.5%
  return compressed_size < raw_size - (raw_size / 8u);
}

}  //命名空间

//format_version是include/rocksdb/table.h中定义的块格式。
Slice CompressBlock(const Slice& raw,
                    const CompressionOptions& compression_options,
                    CompressionType* type, uint32_t format_version,
                    const Slice& compression_dict,
                    std::string* compressed_output) {
  if (*type == kNoCompression) {
    return raw;
  }

//如果（1）压缩方法为
//在这个平台上支撑，（2）压缩率“足够好”。
  switch (*type) {
    case kSnappyCompression:
      if (Snappy_Compress(compression_options, raw.data(), raw.size(),
                          compressed_output) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;  //回到没有压缩的状态。
    case kZlibCompression:
      if (Zlib_Compress(
              compression_options,
              GetCompressFormatForVersion(kZlibCompression, format_version),
              raw.data(), raw.size(), compressed_output, compression_dict) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;  //回到没有压缩的状态。
    case kBZip2Compression:
      if (BZip2_Compress(
              compression_options,
              GetCompressFormatForVersion(kBZip2Compression, format_version),
              raw.data(), raw.size(), compressed_output) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;  //回到没有压缩的状态。
    case kLZ4Compression:
      if (LZ4_Compress(
              compression_options,
              GetCompressFormatForVersion(kLZ4Compression, format_version),
              raw.data(), raw.size(), compressed_output, compression_dict) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;  //回到没有压缩的状态。
    case kLZ4HCCompression:
      if (LZ4HC_Compress(
              compression_options,
              GetCompressFormatForVersion(kLZ4HCCompression, format_version),
              raw.data(), raw.size(), compressed_output, compression_dict) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;     //回到没有压缩的状态。
    case kXpressCompression:
      if (XPRESS_Compress(raw.data(), raw.size(),
          compressed_output) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
      break;
    case kZSTD:
    case kZSTDNotFinalCompression:
      if (ZSTD_Compress(compression_options, raw.data(), raw.size(),
                        compressed_output, compression_dict) &&
          GoodCompressionRatio(compressed_output->size(), raw.size())) {
        return *compressed_output;
      }
break;     //回到没有压缩的状态。
default: {}  //不识别此压缩类型
  }

//不支持压缩方法，或者压缩比不好，所以
//回到未压缩的形式。
  *type = kNoCompression;
  return raw;
}

//KblockBasedTableMagicNumber是通过运行
//echo rocksdb.table.block_based_sha1sum
//取前导的64位。
//请注意，KblockBasedTableMagicNumber也可以由其他人访问
//cc文件
//因为这个原因，我们在头中声明它是外部的，但是为了获得空间
//分配
//它不能在一个地方外。
const uint64_t kBlockBasedTableMagicNumber = 0x88e241b785f4cff7ull;
//我们还支持读取和写入基于块的旧表格式（用于
//向后兼容性）
const uint64_t kLegacyBlockBasedTableMagicNumber = 0xdb4775248b80fb57ull;

//将感兴趣的属性收集到基于块的表的收集器。
//现在这个类看起来很重，因为我们只写了另外一个
//财产。
//但在可预见的未来，我们将增加越来越多的财产
//特定于基于块的表。
class BlockBasedTableBuilder::BlockBasedTablePropertiesCollector
    : public IntTblPropCollector {
 public:
  explicit BlockBasedTablePropertiesCollector(
      BlockBasedTableOptions::IndexType index_type, bool whole_key_filtering,
      bool prefix_filtering)
      : index_type_(index_type),
        whole_key_filtering_(whole_key_filtering),
        prefix_filtering_(prefix_filtering) {}

  virtual Status InternalAdd(const Slice& key, const Slice& value,
                             uint64_t file_size) override {
//故意留空。对收集统计数据没有兴趣
//单个键/值对。
    return Status::OK();
  }

  virtual Status Finish(UserCollectedProperties* properties) override {
    std::string val;
    PutFixed32(&val, static_cast<uint32_t>(index_type_));
    properties->insert({BlockBasedTablePropertyNames::kIndexType, val});
    properties->insert({BlockBasedTablePropertyNames::kWholeKeyFiltering,
                        whole_key_filtering_ ? kPropTrue : kPropFalse});
    properties->insert({BlockBasedTablePropertyNames::kPrefixFiltering,
                        prefix_filtering_ ? kPropTrue : kPropFalse});
    return Status::OK();
  }

//属性收集器的名称可用于调试目的。
  virtual const char* Name() const override {
    return "BlockBasedTablePropertiesCollector";
  }

  virtual UserCollectedProperties GetReadableProperties() const override {
//故意留空。
    return UserCollectedProperties();
  }

 private:
  BlockBasedTableOptions::IndexType index_type_;
  bool whole_key_filtering_;
  bool prefix_filtering_;
};

struct BlockBasedTableBuilder::Rep {
  const ImmutableCFOptions ioptions;
  const BlockBasedTableOptions table_options;
  const InternalKeyComparator& internal_comparator;
  WritableFileWriter* file;
  uint64_t offset = 0;
  Status status;
  BlockBuilder data_block;
  BlockBuilder range_del_block;

  InternalKeySliceTransform internal_prefix_transform;
  std::unique_ptr<IndexBuilder> index_builder;
  PartitionedIndexBuilder* p_index_builder_ = nullptr;

  std::string last_key;
  const CompressionType compression_type;
  const CompressionOptions compression_opts;
//用于预设压缩库字典或nullptr的数据。
  const std::string* compression_dict;
  TableProperties props;

bool closed = false;  //已调用finish（）或放弃（）。
  std::unique_ptr<FilterBlockBuilder> filter_builder;
  char compressed_cache_key_prefix[BlockBasedTable::kMaxCacheKeyPrefixSize];
  size_t compressed_cache_key_prefix_size;

BlockHandle pending_handle;  //添加到索引块的句柄

  std::string compressed_output;
  std::unique_ptr<FlushBlockPolicy> flush_block_policy;
  uint32_t column_family_id;
  const std::string& column_family_name;
  uint64_t creation_time = 0;
  uint64_t oldest_key_time = 0;

  std::vector<std::unique_ptr<IntTblPropCollector>> table_properties_collectors;

  Rep(const ImmutableCFOptions& _ioptions,
      const BlockBasedTableOptions& table_opt,
      const InternalKeyComparator& icomparator,
      const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
          int_tbl_prop_collector_factories,
      uint32_t _column_family_id, WritableFileWriter* f,
      const CompressionType _compression_type,
      const CompressionOptions& _compression_opts,
      const std::string* _compression_dict, const bool skip_filters,
      const std::string& _column_family_name, const uint64_t _creation_time,
      const uint64_t _oldest_key_time)
      : ioptions(_ioptions),
        table_options(table_opt),
        internal_comparator(icomparator),
        file(f),
        data_block(table_options.block_restart_interval,
                   table_options.use_delta_encoding),
range_del_block(1),  //TODO（Andrewkr）：不需要重新启动间隔
        internal_prefix_transform(_ioptions.prefix_extractor),
        compression_type(_compression_type),
        compression_opts(_compression_opts),
        compression_dict(_compression_dict),
        flush_block_policy(
            table_options.flush_block_policy_factory->NewFlushBlockPolicy(
                table_options, data_block)),
        column_family_id(_column_family_id),
        column_family_name(_column_family_name),
        creation_time(_creation_time),
        oldest_key_time(_oldest_key_time) {
    if (table_options.index_type ==
        BlockBasedTableOptions::kTwoLevelIndexSearch) {
      p_index_builder_ = PartitionedIndexBuilder::CreateIndexBuilder(
          &internal_comparator, table_options);
      index_builder.reset(p_index_builder_);
    } else {
      index_builder.reset(IndexBuilder::CreateIndexBuilder(
          table_options.index_type, &internal_comparator,
          &this->internal_prefix_transform, table_options));
    }
    if (skip_filters) {
      filter_builder = nullptr;
    } else {
      filter_builder.reset(
          CreateFilterBlockBuilder(_ioptions, table_options, p_index_builder_));
    }

    for (auto& collector_factories : *int_tbl_prop_collector_factories) {
      table_properties_collectors.emplace_back(
          collector_factories->CreateIntTblPropCollector(column_family_id));
    }
    table_properties_collectors.emplace_back(
        new BlockBasedTablePropertiesCollector(
            table_options.index_type, table_options.whole_key_filtering,
            _ioptions.prefix_extractor != nullptr));
  }
};

BlockBasedTableBuilder::BlockBasedTableBuilder(
    const ImmutableCFOptions& ioptions,
    const BlockBasedTableOptions& table_options,
    const InternalKeyComparator& internal_comparator,
    const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
        int_tbl_prop_collector_factories,
    uint32_t column_family_id, WritableFileWriter* file,
    const CompressionType compression_type,
    const CompressionOptions& compression_opts,
    const std::string* compression_dict, const bool skip_filters,
    const std::string& column_family_name, const uint64_t creation_time,
    const uint64_t oldest_key_time) {
  BlockBasedTableOptions sanitized_table_options(table_options);
  if (sanitized_table_options.format_version == 0 &&
      sanitized_table_options.checksum != kCRC32c) {
    ROCKS_LOG_WARN(
        ioptions.info_log,
        "Silently converting format_version to 1 because checksum is "
        "non-default");
//无提示地将格式\版本转换为1以保持与当前版本一致
//行为
    sanitized_table_options.format_version = 1;
  }

  rep_ =
      new Rep(ioptions, sanitized_table_options, internal_comparator,
              int_tbl_prop_collector_factories, column_family_id, file,
              compression_type, compression_opts, compression_dict,
              skip_filters, column_family_name, creation_time, oldest_key_time);

  if (rep_->filter_builder != nullptr) {
    rep_->filter_builder->StartBlock(0);
  }
  if (table_options.block_cache_compressed.get() != nullptr) {
    BlockBasedTable::GenerateCachePrefix(
        table_options.block_cache_compressed.get(), file->writable_file(),
        &rep_->compressed_cache_key_prefix[0],
        &rep_->compressed_cache_key_prefix_size);
  }
}

BlockBasedTableBuilder::~BlockBasedTableBuilder() {
assert(rep_->closed);  //捕获调用方忘记调用finish（）的错误
  delete rep_;
}

void BlockBasedTableBuilder::Add(const Slice& key, const Slice& value) {
  Rep* r = rep_;
  assert(!r->closed);
  if (!ok()) return;
  ValueType value_type = ExtractValueType(key);
  if (IsValueType(value_type)) {
    if (r->props.num_entries > 0) {
      assert(r->internal_comparator.Compare(key, Slice(r->last_key)) > 0);
    }

    auto should_flush = r->flush_block_policy->Update(key, value);
    if (should_flush) {
      assert(!r->data_block.empty());
      Flush();

//将项添加到索引块。
//在看到块的
//下一个数据块的第一个键。这样我们就可以用短一点的
//索引块中的键。例如，考虑块边界
//在“快速棕狐狸”和“谁”之间。我们可以使用
//“r”作为索引块项的键，因为它大于等于all
//第一个块中的条目和后面的所有条目
//阻碍。
      if (ok()) {
        r->index_builder->AddIndexEntry(&r->last_key, &key, r->pending_handle);
      }
    }

//注意：PartitionedFilterBlockBuilder需要将密钥添加到筛选器
//添加到索引生成器后的生成器。
    if (r->filter_builder != nullptr) {
      r->filter_builder->Add(ExtractUserKey(key));
    }

    r->last_key.assign(key.data(), key.size());
    r->data_block.Add(key, value);
    r->props.num_entries++;
    r->props.raw_key_size += key.size();
    r->props.raw_value_size += value.size();

    r->index_builder->OnKeyAdded(key);
    NotifyCollectTableCollectorsOnAdd(key, value, r->offset,
                                      r->table_properties_collectors,
                                      r->ioptions.info_log);

  } else if (value_type == kTypeRangeDeletion) {
//TODO（Wanning&Andrewkr）向表属性中添加num\tomestone
    r->range_del_block.Add(key, value);
    ++r->props.num_entries;
    r->props.raw_key_size += key.size();
    r->props.raw_value_size += value.size();
    NotifyCollectTableCollectorsOnAdd(key, value, r->offset,
                                      r->table_properties_collectors,
                                      r->ioptions.info_log);
  } else {
    assert(false);
  }
}

void BlockBasedTableBuilder::Flush() {
  Rep* r = rep_;
  assert(!r->closed);
  if (!ok()) return;
  if (r->data_block.empty()) return;
  /*teblock（&r->data_block，&r->pending_handle，true/*为_data_block*/）；
  如果（r->filter_builder！= null pTr）{
    r->filter_builder->startblock（r->offset）；
  }
  r->props.data_size=r->offset；
  ++r->props.num_数据块；
}

void BlockBasedTableBuilder:：WriteBlock（BlockBuilder*块，
                                        blockhandle*手柄，
                                        bool是_data_block）
  writeblock（block->finish（），handle，is_data_block）；
  块-REST（）；
}

void BlockBasedTableBuilder:：WriteBlock（常量切片和原始块内容，
                                        blockhandle*手柄，
                                        bool是_data_block）
  //文件格式包含一系列块，其中每个块具有：
  //块_data:uint8[n]
  //类型：uint8
  //crc:uint32
  断言（OK（））；
  RP*R= RePig；

  auto type=r->compression_type；
  切片块内容；
  bool abort_compression=false；

  StopWatchNano计时器（r->ioptions.env，
    shouldReportDetailedTime（r->ioptions.env，r->ioptions.statistics））；

  if（raw_block_contents.size（）<kcompressionSizeLimit）
    切片压缩
    if（is撊data_block&&r->compression_dict&&r->compression_dict->size（））
      compression_dict=*r->compression_dict；
    }

    block_contents=compressblock（原始_block_contents，r->压缩选项，
                                   &type，r->table\u options.format\u版本，
                                   压缩_dict，&r->compressed _output）；

    //已知有些压缩算法不可靠。如果
    //设置了verify_compression标志，然后尝试对
    //压缩数据并与输入进行比较。
    如果（类型）！=knocompression&&r->table_options.verify_compression）
      //将未压缩的内容检索到新的缓冲区中
      块内容物内容；
      stat=uncompressBlockContentsForCompressionType（
          block_contents.data（）、block_contents.size（）和contents，
          r->table_options.format_version，compression_dict，type，
          R-＞I期权；

      if（stat.ok（））
        bool compressed_ok=contents.data.compare（raw_block_contents）=0；
        如果（！）压缩后的_
          //压缩结果无效。中止。
          中止压缩=真；
          rocks日志错误（r->ioptions.info日志，
                          “解压缩块与原始块不匹配”）；
          R~>状态=
              状态：：损坏（“解压缩的块与原始块不匹配”）；
        }
      }否则{
        //解压缩报告了一个错误。中止。
        r->status=status:：corruption（“无法解压缩”）；
        中止压缩=真；
      }
    }
  }否则{
    //块太大，无法压缩。
    中止压缩=真；
  }

  //如果块太大或未通过，则中止压缩
  //验证。
  如果（中止压缩）
    记录标记（r->ioptions.statistics，number_block_not_compressed）；
    类型=knocompression；
    block_contents=原始_block_内容；
  否则，如果（类型！=knocompression和&
             应报告详细时间（r->ioptions.env，
                                      r->ioptions.statistics），_
    测量时间（r->ioptions.statistics，压缩时间\u nanos，
                timer.elapsednanos（））；
    MeasureTime（r->ioptions.statistics，bytes_compressed，
                原始_块_contents.size（））；
    记录标记（r->ioptions.statistics，number_block_compressed）；
  }

  writerawblock（block_contents，type，handle）；
  r->compressed_output.clear（）；
}

void BlockBasedTableBuilder:：WriterawBlock（常量切片和块内容，
                                           压缩类型，
                                           块状手柄*手柄）
  RP*R= RePig；
  秒表sw（r->ioptions.env，r->ioptions.statistics，write_raw_block_micros）；
  手柄->设置偏移量（R->偏移量）；
  handle->set_size（block_contents.size（））；
  断言（r->status.ok（））；
  r->status=r->file->append（block_contents）；
  if（r->status.ok（））
    字符拖车[Kblocktrailersize]；
    拖车[0]=类型；
    char*无拖车类型=拖车+1；
    switch（r->table_options.checksum）
      案例Knochecksum：
        encodefixed32（拖车_，无_类型，0）；
        断裂；
      案例KCRC32 C：{
        auto crc=crc32c:：value（block_contents.data（），block_contents.size（））；
        crc=crc32c:：extend（crc，trailer，1）；//extend to cover block type
        encodeFixed32（不带_类型的尾部_，crc32c:：mask（crc））；
        断裂；
      }
      案例KXXHASH：{
        void*xxh=xxh32_init（0）；
        xxh32_update（xxh，block_contents.data（），
                     static_cast<uint32_t>（block_contents.size（））；
        xxh32_update（xxh，trailer，1）；//扩展到cover block类型
        encodefixed32（拖车_，无_类型，xxh32_摘要（xxh））；
        断裂；
      }
    }

    断言（r->status.ok（））；
    r->status=r->file->append（slice（trailer，kblocktrailersize））；
    if（r->status.ok（））
      r->status=insertblockincache（block_contents，type，handle）；
    }
    if（r->status.ok（））
      r->offset+=block_contents.size（）+kblocktrailersize；
    }
  }
}

状态块BasedTableBuilder:：status（）const
  返回rep->status；
}

静态void deletecachedblock（const slice&key，void*value）
  block*block=reinterpret_cast<block*>（value）；
  删除块；
}

/ /
//复制块内容并插入到压缩块缓存中
/ /
状态块BasedTableBuilder:：InsertBlockIncache（常量切片和块内容，
                                                  const压缩类型，
                                                  const块句柄*句柄）
  RP*R= RePig；
  cache*block_cache_compressed=r->table_options.block_cache_compressed.get（）；

  如果（类型）！=knocompression和block_cache_compressed！= null pTr）{

    size_t size=block_contents.size（）；

    std:：unique_ptr<char[]>ubuf（new char[size+1]）；
    memcpy（ubuf.get（），block_contents.data（），大小）；
    ubuf[尺寸]=类型；

    blockContents结果（std:：move（ubuf），大小，真，类型）；

    block*block=新块（std:：move（results），kdisableglobalsequencenumber）；

    //通过将文件偏移量附加到缓存前缀ID来生成缓存键
    char*end=encodevarint64（
                  r->compressed_cache_key_prefix+
                  r->压缩缓存密钥前缀大小，
                  handle->offset（））；
    slice key（r->compressed_cache_key_prefix，static_cast<size_t>
              （end-r->compressed_cache_key_prefix））；

    //插入到压缩块缓存中。
    block_cache_compressed->insert（key，block，block->usable_size（），
                                   &deletecachedblock）；

    //使OS缓存失效。
    r->file->invalidacache（static_cast<size_t>（r->offset），size）；
  }
  返回状态：：OK（）；
}

状态块BasedTableBuilder:：Finish（）
  RP*R= RePig；
  bool empty_data_block=r->data_block.empty（）；
  FrHuSE（）；
  断言（！）R-关闭）；
  R->Closed=true；

  //确保属性块能够保持索引的准确大小
  //块，我们将在此处完成所有索引项的写入并刷新它们
  //写入metaindex块后存储。
  如果（OK）& &！清空_数据_块）
    r->index_builder->addindexentry（
        &R->last_key，nullptr/*无下一个数据块*/, r->pending_handle);

  }

  BlockHandle filter_block_handle, metaindex_block_handle, index_block_handle,
      compression_dict_block_handle, range_del_block_handle;
//写入筛选器块
  if (ok() && r->filter_builder != nullptr) {
    Status s = Status::Incomplete();
    while (s.IsIncomplete()) {
      Slice filter_content = r->filter_builder->Finish(filter_block_handle, &s);
      assert(s.ok() || s.IsIncomplete());
      r->props.filter_size += filter_content.size();
      WriteRawBlock(filter_content, kNoCompression, &filter_block_handle);
    }
  }

  IndexBuilder::IndexBlocks index_blocks;
  auto index_builder_status = r->index_builder->Finish(&index_blocks);
  if (index_builder_status.IsIncomplete()) {
//我们有多个索引分区，那么元块不是
//支持索引。当前meta_块只能由
//不是多分区的哈希索引生成器。
    assert(index_blocks.meta_blocks.empty());
  } else if (!index_builder_status.ok()) {
    return index_builder_status;
  }

//按以下顺序编写元块和元索引块。
//1。[元块：筛选器]
//2。[元块：属性]
//三。[元块：压缩字典]
//4。[元块：范围删除墓碑]
//5。[元索引块]
//写入元块
  MetaIndexBuilder meta_index_builder;
  for (const auto& item : index_blocks.meta_blocks) {
    BlockHandle block_handle;
    /*teblock（item.second，&block_handle，false/*是_data_block*/）；
    meta_index_builder.add（item.first，block_handle）；
  }

  如果（Ok（））{
    如果（r->filter_builder！= null pTr）{
      //添加从“<filter_block_prefix>.name”到位置的映射
      //筛选数据。
      std：：字符串键；
      if（r->filter_builder->isBlockBased（））
        key=blockBasedTable:：kFilterBlockPrefix；
      }否则{
        key=r->table_options.partition_filters
                  ？BlockBasedTable:：KPartitionedFilterBlockPrefix
                  ：blockBasedTable:：kfullFilterBlockPrefix；
      }
      key.append（r->table_options.filter_policy->name（））；
      meta_index_builder.add（key，filter_block_handle）；
    }

    //写入属性和压缩字典块。
    {
      属性块生成器属性块生成器；
      r->props.column_family_id=r->column_family_id；
      r->props.column_family_name=r->column_family_name；
      r->props.filter_policy_name=r->table_options.filter_policy！= null pTR？
          r->table_options.filter_policy->name（）：“”；
      r->props.index_大小=
          r->index_builder->EstimatedSize（）+kblocktrailersize；
      r->props.comparator_name=r->ioptions.user_comparator！= Null PTR
                                     ？r->ioptions.user_comparator->name（）。
                                     “Null PTR”；
      r->props.merge_operator_name=r->ioptions.merge_operator！= Null PTR
                                         ？r->ioptions.merge_operator->name（）。
                                         “Null PTR”；
      r->props.compression_name=compressiontypetoString（r->compression_type）；
      r->props.prefix_extractor_name=
          r->ioptions.prefix_提取器！= Null PTR
              ？r->ioptions.prefix_提取器->name（）
              “Null PTR”；

      std:：string属性_collectors_name=“[”；
      属性_collectors_name=“[”；
      对于（尺寸t i=0；
           i<r->ioptions.table_properties_collector_factures.size（）；++i）
        如果（我）！= 0）{
          属性_Collectors_Names+=“，”；
        }
        属性\收集器\名称+=
            r->ioptions.table_properties_collector_factures[i]->name（）；
      }
      属性_Collectors_Names+=“]”；
      r->props.property_collectors_name=property_collectors_name；
      if（r->table_options.index_type==
          blockBasedTableOptions:：ktwolLevelIndexSearch）
        断言（r->p_index_builder！= null pTr）；
        r->props.index_partitions=r->p_index_builder_u->numpartitions（）；
        r->props.top_-level_-index_-size=
            r->p_index_builder_uu->EstimateToplevelIndexSize（r->offset）；
      }
      r->props.creation_time=r->creation_time；
      r->props.oldest_key_time=r->oldest_key_time；

      //添加基本属性
      属性_block_builder.addTableProperty（r->props）；

      //添加use collected属性
      notifyCollectTableCollectorsOnFinish（R->Table_属性_收集器，
                                           r->ioptions.info_日志，
                                           &property_block_builder）；

      block handle属性\u block \u handle；
      WriteRawBlock
          属性_block_builder.finish（），
          诺克压缩，
          属性\块\句柄（&P）
      ；
      meta_index_builder.add（kpropertiesblock，properties_block_handle）；

      //写入压缩字典块
      if（r->compression-dict&&r->compression-dict->size（））
        writerawblock（*r->compression\u dict，knocompression，
                      &compression_dict_block_handle）；
        meta_index_builder.add（kcompressionDictBlock，
                               压缩块处理）；
      }
    //属性/压缩字典块写入结束

    如果（OK）& &！R->Range_del_block.empty（））
      writerawblock（r->range del_block.finish（），knocompression，
                    &rangeou delou blockou handle）；
      meta_index_builder.add（krangedelblock，range_del_block_handle）；
    //范围删除逻辑删除元块
  //元块

  //写入索引块
  如果（Ok（））{
    //刷新元索引块
    writerawblock（meta_index_builder.finish（），knocompression，
                  &metaindex_block_handle）；

    const bool为“数据块”=true；
    writeblock（index_blocks.index_block_contents，&index_block_handle，
               ！ISS2数据块；
    //如果有更多的索引分区，完成并写出它们
    status&s=索引\生成器\状态；
    while（s.isinComplete（））
      s=r->index_builder->finish（&index_block s，index_block_handle）；
      如果（！）&！s.isinComplete（））
        返回S；
      }
      writeblock（index_blocks.index_block_contents，&index_block_handle，
                 ！ISS2数据块；
      //最后一个索引块句柄将用于分区索引块
    }
  }

  //写页脚
  如果（Ok（））{
    //如果使用默认校验和，则无需写出新的页脚。
    //我们正在编写旧的魔法数字，因为我们需要旧版本的rocksdb
    //能够读取新版本生成的文件（以防万一
    //有人想在升级后回滚）
    //将来某个时候，当我们完全确定的时候，todo（icanadi）
    //没有人会回滚到rocksdb 2.x版本，淘汰旧魔法
    //用新的幻数编写新的表文件
    bool legacy=（r->table_options.format_version==0）；
    //这由BlockBasedTableBuilder的构造函数保证
    断言（r->table_options.checksum==kcrc32c_
           r->table_options.format_version！＝0）；
    页脚（传统？基于KlegacyBlockTableMagicNumber的
                         ：kblockbasedTablemagicNumber，
                  r->table_options.format_version）；
    footer.set_metaindex_handle（metaindex_block_handle）；
    footer.set_index_handle（index_block_handle）；
    footer.set_checksum（r->table_options.checksum）；
    std：：字符串页脚编码；
    footer.encodeto（&footer_encoding）；
    断言（r->status.ok（））；
    r->status=r->file->append（页脚编码）；
    if（r->status.ok（））
      r->offset+=footer_encoding.size（）；
    }
  }

  返回R->状态；
}

void blockBasedTableBuilder：：放弃（）；
  RP*R= RePig；
  断言（！）R-关闭）；
  R->Closed=true；
}

uint64_t blockBasedTableBuilder:：numEntries（）常量
  返回rep_u->props.num_项；
}

uint64_t blockBasedTableBuilder:：fileSize（）常量
  返回rep->offset；
}

bool blockBasedTableBuilder:：needCompact（）常量
  for（const auto&collector:rep_->table_properties_collectors）
    if（collector->needCompact（））
      回归真实；
    }
  }
  返回错误；
}

TableProperties BlockBasedTableBuilder:：GetTableProperties（）常量
  tableproperties ret=rep_->props；
  for（const auto&collector:rep_->table_properties_collectors）
    for（const auto&prop:collector->getreadableproperties（））
      ret.readable_properties.insert（prop）；
    }
    collector->finish（&ret.user_collected_properties）；
  }
  返回RET；
}

const std:：string blockBasedTable:：kFilterBlockPrefix=“筛选器。”；
const std:：string blockBasedTable:：kfullFilterBlockPrefix=“fullFilter。”；
const std:：string blockbasedtable:：kpartitionedfilterblockprefix=
    “分区筛选器。”；
//命名空间rocksdb
