
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
#include "table/meta_blocks.h"

#include <map>
#include <string>

#include "db/table_properties_collector.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"
#include "table/block.h"
#include "table/format.h"
#include "table/internal_iterator.h"
#include "table/persistent_cache_helper.h"
#include "table/table_properties_internal.h"
#include "util/coding.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

MetaIndexBuilder::MetaIndexBuilder()
    /*eta_index_block_u（new blockbuilder（1/*重启间隔*/）

void metaindexbuilder:：add（const std:：string&key，
                           const blockhandle和handle）
  std：：字符串句柄_编码；
  handle.encodeto（&handle_encoding）；
  meta_block_handles_u.insert（key，handle_encoding）；
}

切片metaindexbuilder:：finish（）
  for（const auto&meta block:meta_block_handles_
    元索引块添加（meta block.first，meta block.second）；
  }
  返回meta_index_block_uu->finish（）；
}

属性块生成器：：属性块生成器（）。
    ：属性\u块\（新的块生成器（1/*重新启动间隔*/)) {}


void PropertyBlockBuilder::Add(const std::string& name,
                               const std::string& val) {
  props_.insert({name, val});
}

void PropertyBlockBuilder::Add(const std::string& name, uint64_t val) {
  assert(props_.find(name) == props_.end());

  std::string dst;
  PutVarint64(&dst, val);

  Add(name, dst);
}

void PropertyBlockBuilder::Add(
    const UserCollectedProperties& user_collected_properties) {
  for (const auto& prop : user_collected_properties) {
    Add(prop.first, prop.second);
  }
}

void PropertyBlockBuilder::AddTableProperty(const TableProperties& props) {
  Add(TablePropertiesNames::kRawKeySize, props.raw_key_size);
  Add(TablePropertiesNames::kRawValueSize, props.raw_value_size);
  Add(TablePropertiesNames::kDataSize, props.data_size);
  Add(TablePropertiesNames::kIndexSize, props.index_size);
  if (props.index_partitions != 0) {
    Add(TablePropertiesNames::kIndexPartitions, props.index_partitions);
    Add(TablePropertiesNames::kTopLevelIndexSize, props.top_level_index_size);
  }
  Add(TablePropertiesNames::kNumEntries, props.num_entries);
  Add(TablePropertiesNames::kNumDataBlocks, props.num_data_blocks);
  Add(TablePropertiesNames::kFilterSize, props.filter_size);
  Add(TablePropertiesNames::kFormatVersion, props.format_version);
  Add(TablePropertiesNames::kFixedKeyLen, props.fixed_key_len);
  Add(TablePropertiesNames::kColumnFamilyId, props.column_family_id);
  Add(TablePropertiesNames::kCreationTime, props.creation_time);
  Add(TablePropertiesNames::kOldestKeyTime, props.oldest_key_time);

  if (!props.filter_policy_name.empty()) {
    Add(TablePropertiesNames::kFilterPolicy, props.filter_policy_name);
  }
  if (!props.comparator_name.empty()) {
    Add(TablePropertiesNames::kComparator, props.comparator_name);
  }

  if (!props.merge_operator_name.empty()) {
    Add(TablePropertiesNames::kMergeOperator, props.merge_operator_name);
  }
  if (!props.prefix_extractor_name.empty()) {
    Add(TablePropertiesNames::kPrefixExtractorName,
        props.prefix_extractor_name);
  }
  if (!props.property_collectors_names.empty()) {
    Add(TablePropertiesNames::kPropertyCollectors,
        props.property_collectors_names);
  }
  if (!props.column_family_name.empty()) {
    Add(TablePropertiesNames::kColumnFamilyName, props.column_family_name);
  }

  if (!props.compression_name.empty()) {
    Add(TablePropertiesNames::kCompression, props.compression_name);
  }
}

Slice PropertyBlockBuilder::Finish() {
  for (const auto& prop : props_) {
    properties_block_->Add(prop.first, prop.second);
  }

  return properties_block_->Finish();
}

void LogPropertiesCollectionError(
    Logger* info_log, const std::string& method, const std::string& name) {
  assert(method == "Add" || method == "Finish");

  std::string msg =
    "Encountered error when calling TablePropertiesCollector::" +
    method + "() with collector name: " + name;
  ROCKS_LOG_ERROR(info_log, "%s", msg.c_str());
}

bool NotifyCollectTableCollectorsOnAdd(
    const Slice& key, const Slice& value, uint64_t file_size,
    const std::vector<std::unique_ptr<IntTblPropCollector>>& collectors,
    Logger* info_log) {
  bool all_succeeded = true;
  for (auto& collector : collectors) {
    Status s = collector->InternalAdd(key, value, file_size);
    all_succeeded = all_succeeded && s.ok();
    if (!s.ok()) {
      /*属性CollectionError（信息日志，“添加”/*方法*/，
                                   collector->name（））；
    }
  }
  全部归还成功；
}

bool notifyCollectTableCollectorsonFinish（
    const std:：vector<std:：unique_ptr<inttblpropcollector>>&collectors，
    logger*信息日志，propertyblockbuilder*生成器）
  bool all_succeeded=true；
  用于（自动和收集器：收集器）
    用户收集属性用户收集属性；
    状态s=collector->finish（&user_collected_properties）；

    all_succeeded=all_succeeded&&s.ok（）；
    如果（！）S.O.（））{
      LogPropertiesCollectionError（信息日志，“完成”/*方法*/,

                                   collector->Name());
    } else {
      builder->Add(user_collected_properties);
    }
  }

  return all_succeeded;
}

Status ReadProperties(const Slice& handle_value, RandomAccessFileReader* file,
                      FilePrefetchBuffer* prefetch_buffer, const Footer& footer,
                      const ImmutableCFOptions& ioptions,
                      TableProperties** table_properties) {
  assert(table_properties);

  Slice v = handle_value;
  BlockHandle handle;
  if (!handle.DecodeFrom(&v).ok()) {
    return Status::InvalidArgument("Failed to decode properties block handle");
  }

  BlockContents block_contents;
  ReadOptions read_options;
  read_options.verify_checksums = false;
  Status s;
  s = ReadBlockContents(file, prefetch_buffer, footer, read_options, handle,
                        /*ock_contents，ioptions，false/*解压缩*/）；

  如果（！）S.O.（））{
    返回S；
  }

  块属性块（std:：move（block_contents）
                         kdisableglobalsequencenumber）；
  阻断剂ITER；
  属性“block.newIterator”（bytewiseComparator（），&iter）；

  auto new_table_properties=new table properties（）；
  //uint64类型的所有预定义属性
  std:：unordered_map<std:：string，uint64_t*>预定义的_uint64_properties=
      TablePropertiesNames:：kDataSize，&New_Table_Properties->Data_
      表属性名称：：KindexSize，&New表属性->索引，
      表属性名称：：KindExpartments，
       新建表属性->索引分区（&N），
      表属性名称：：ktopLevelIndexSize，
       新建_table_properties->top_level_index_size，
      表属性名称：：k筛选器大小和新的_表属性->筛选器大小，
      表属性名称：：krawkeysize，&new_table_properties->raw_key_size，
      表属性名称：：krawValueSize，
       新建表格属性->原始值大小（&N）
      表属性名称：：kNumDataBlocks，
       新建_table_properties->num_data_blocks（&N），
      表属性名称：：Knuumentries，&新_表属性->num_entries，
      表属性名称：：k格式版本，
       新建_table_properties->format_version_（&N）
      表属性名称：：kfixedKeylen，
       新建_table_properties->fixed_key_len_
      表属性名称：：kColumnFamilyID，
       新建_table_properties->column_family_id_
      表属性名称：：k创建时间，
       新建表属性->创建时间（&N），
      表属性名称：：koldestkeytime，
       新建_table_properties->oldest_key_time（&N），
  }；

  std：：字符串最后一个键；
  for（iter.seektofirst（）；iter.valid（）；iter.next（））
    s=iter.status（）；
    如果（！）S.O.（））{
      断裂；
    }

    auto key=iter.key（）.toString（）；
    //属性块严格排序，没有重复键。
    断言（last_key.empty（）
           bytewiseComparator（）->比较（key，last_key）>0）；
    LASTHYKEY =密钥；

    自动原始值=iter.value（）；
    auto pos=预定义的“uint64”属性。find（key）；

    新的_table_properties->properties_offsets.insert（
        键，handle.offset（）+iter.valueoffset（））；

    如果（POS）！=预定义的_uint64_properties.end（））
      //处理预定义的rocksdb属性
      UIT64 64瓦尔；
      如果（！）getvarint64（&raw_val，&val））
        //跳过格式错误的值
        自动错误_msg=
          “检测属性元块中的错误值：”
          \tkey:“+key+”\tval:“+raw_val.toString（）；
        rocks_log_error（ioptions.info_log，“%s”，error_msg.c_str（））；
        继续；
      }
      *（pos->second）=val；
    else if（key==TablePropertiesNames:：kFilterPolicy）
      new_table_properties->filter_policy_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kColumnFamilyName）
      new_table_properties->column_family_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kComparator）
      新的_table_properties->comparator_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kmergeOperator）
      new_table_properties->merge_operator_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kPrefixextractorName）
      new_table_properties->prefix_extractor_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kPropertyCollectors）
      new_table_properties->property_collectors_name=raw_val.toString（）；
    else if（key==TablePropertiesNames:：kcompression）
      新的_table_properties->compression_name=raw_val.toString（）；
    }否则{
      //处理用户收集的属性
      新的_table_properties->user_collected_properties.insert（
          key，raw_val.toString（））；
    }
  }
  如果（S.O.（））{
    *table_properties=新的_table_properties；
  }否则{
    删除新的表格属性；
  }

  返回S；
}

状态readTableProperties（RandoAccessFileReader*文件，uint64文件大小，
                           uint64_t table_magic_编号，
                           常量不变的变量和变量，
                           表属性**属性）
  //—读取元索引块
  页脚页脚；
  auto s=readfooterfromfile（文件，nullptr/*预取缓冲区*/, file_size,

                              &footer, table_magic_number);
  if (!s.ok()) {
    return s;
  }

  auto metaindex_handle = footer.metaindex_handle();
  BlockContents metaindex_contents;
  ReadOptions read_options;
  read_options.verify_checksums = false;
  /*readblockcontents（文件，nullptr/*预取缓冲区*/，页脚，
                        读取选项、元索引句柄和元索引内容，
                        ioptions，假/*解压缩*/);

  if (!s.ok()) {
    return s;
  }
  Block metaindex_block(std::move(metaindex_contents),
                        kDisableGlobalSequenceNumber);
  std::unique_ptr<InternalIterator> meta_iter(
      metaindex_block.NewIterator(BytewiseComparator()));

//--读取属性块
  bool found_properties_block = true;
  s = SeekToPropertiesBlock(meta_iter.get(), &found_properties_block);
  if (!s.ok()) {
    return s;
  }

  TableProperties table_properties;
  if (found_properties_block == true) {
    /*readproperties（meta_iter->value（），file，nullptr/*预取缓冲区*/，
                       页脚、IOptions、属性）；
  }否则{
    S=状态：：NotFound（）；
  }

  返回S；
}

状态findmetablock（InternalIterator*元索引器，
                     const std：：字符串和元块名称，
                     block handle*block_handle）
  meta-index-iter->seek（meta-block-name）；
  if（meta-index-iter->status（）.ok（）&&meta-index-iter->valid（）&&
      meta_index_iter->key（）==meta_block_name）
    slice v=meta_index_iter->value（）；
    返回块_handle->decodefrom（&v）；
  }否则{
    返回状态：：损坏（“找不到元块”，元块名称）；
  }
}

status findmetablock（randomaaccessfilereader*文件，uint64文件大小，
                     uint64_t table_magic_编号，
                     常量不变的变量和变量，
                     const std：：字符串和元块名称，
                     block handle*block_handle）
  页脚页脚；
  auto s=readfooterfromfile（文件，nullptr/*预取缓冲区*/, file_size,

                              &footer, table_magic_number);
  if (!s.ok()) {
    return s;
  }

  auto metaindex_handle = footer.metaindex_handle();
  BlockContents metaindex_contents;
  ReadOptions read_options;
  read_options.verify_checksums = false;
  /*readblockcontents（文件，nullptr/*预取缓冲区*/，页脚，
                        读取选项、元索引句柄和元索引内容，
                        ioptions，false/*执行解压缩*/);

  if (!s.ok()) {
    return s;
  }
  Block metaindex_block(std::move(metaindex_contents),
                        kDisableGlobalSequenceNumber);

  std::unique_ptr<InternalIterator> meta_iter;
  meta_iter.reset(metaindex_block.NewIterator(BytewiseComparator()));

  return FindMetaBlock(meta_iter.get(), meta_block_name, block_handle);
}

Status ReadMetaBlock(RandomAccessFileReader* file,
                     FilePrefetchBuffer* prefetch_buffer, uint64_t file_size,
                     uint64_t table_magic_number,
                     const ImmutableCFOptions& ioptions,
                     const std::string& meta_block_name,
                     BlockContents* contents) {
  Status status;
  Footer footer;
  status = ReadFooterFromFile(file, prefetch_buffer, file_size, &footer,
                              table_magic_number);
  if (!status.ok()) {
    return status;
  }

//读取元索引块
  auto metaindex_handle = footer.metaindex_handle();
  BlockContents metaindex_contents;
  ReadOptions read_options;
  read_options.verify_checksums = false;
  status = ReadBlockContents(file, prefetch_buffer, footer, read_options,
                             metaindex_handle, &metaindex_contents, ioptions,
                             /*SE/*解压*/）；
  如果（！）状态，OK（））{
    返回状态；
  }

  //正在查找元块
  块元索引块（std:：move（元索引内容），
                        kdisableglobalsequencenumber）；

  std:：unique_ptr<internaliterator>meta_iter；
  meta-iter.reset（metaindex_block.newIterator（bytewiseComparator（））；

  block handle块_handle；
  status=findmetablock（meta_iter.get（），meta_block_name，&block_handle）；

  如果（！）状态，OK（））{
    返回状态；
  }

  //读取元锁
  返回readblockcontents（文件、预取缓冲区、页脚、读取选项，
                           块句柄、内容、IOPTIONS，
                           假/*解压*/);

}

}  //命名空间rocksdb
