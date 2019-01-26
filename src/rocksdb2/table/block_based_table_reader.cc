
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
#include "table/block_based_table_reader.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "db/dbformat.h"
#include "db/pinned_iterators_manager.h"

#include "rocksdb/cache.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/statistics.h"
#include "rocksdb/table.h"
#include "rocksdb/table_properties.h"

#include "table/block.h"
#include "table/block_based_filter_block.h"
#include "table/block_based_table_factory.h"
#include "table/block_prefix_index.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "table/full_filter_block.h"
#include "table/get_context.h"
#include "table/internal_iterator.h"
#include "table/meta_blocks.h"
#include "table/partitioned_filter_block.h"
#include "table/persistent_cache_helper.h"
#include "table/sst_file_writer_collectors.h"
#include "table/two_level_iterator.h"

#include "monitoring/perf_context_imp.h"
#include "util/coding.h"
#include "util/file_reader_writer.h"
#include "util/stop_watch.h"
#include "util/string_util.h"
#include "util/sync_point.h"

namespace rocksdb {

extern const uint64_t kBlockBasedTableMagicNumber;
extern const std::string kHashIndexPrefixesBlock;
extern const std::string kHashIndexPrefixesMetadataBlock;
using std::unique_ptr;

typedef BlockBasedTable::IndexReader IndexReader;

BlockBasedTable::~BlockBasedTable() {
  Close();
  delete rep_;
}

namespace {
//从“文件”中读取由“句柄”标识的块。
//唯一相关的选项是options.verify_checksums for now.
//失败时返回不正常。
//成功时，填充*结果并返回OK-调用方拥有*结果
//@param compression\u用于预设压缩库的dict data
//字典。
Status ReadBlockFromFile(
    RandomAccessFileReader* file, FilePrefetchBuffer* prefetch_buffer,
    const Footer& footer, const ReadOptions& options, const BlockHandle& handle,
    std::unique_ptr<Block>* result, const ImmutableCFOptions& ioptions,
    bool do_uncompress, const Slice& compression_dict,
    const PersistentCacheOptions& cache_options, SequenceNumber global_seqno,
    size_t read_amp_bytes_per_bit) {
  BlockContents contents;
  Status s = ReadBlockContents(file, prefetch_buffer, footer, options, handle,
                               &contents, ioptions, do_uncompress,
                               compression_dict, cache_options);
  if (s.ok()) {
    result->reset(new Block(std::move(contents), global_seqno,
                            read_amp_bytes_per_bit, ioptions.statistics));
  }

  return s;
}

//删除迭代器持有的资源。
template <class ResourceType>
void DeleteHeldResource(void* arg, void* ignored) {
  delete reinterpret_cast<ResourceType*>(arg);
}

//删除缓存中的条目。
template <class Entry>
void DeleteCachedEntry(const Slice& key, void* value) {
  auto entry = reinterpret_cast<Entry*>(value);
  delete entry;
}

void DeleteCachedFilterEntry(const Slice& key, void* value);
void DeleteCachedIndexEntry(const Slice& key, void* value);

//释放缓存项并减少其引用计数。
void ReleaseCachedEntry(void* arg, void* h) {
  Cache* cache = reinterpret_cast<Cache*>(arg);
  Cache::Handle* handle = reinterpret_cast<Cache::Handle*>(h);
  cache->Release(handle);
}

Slice GetCacheKeyFromOffset(const char* cache_key_prefix,
                            size_t cache_key_prefix_size, uint64_t offset,
                            char* cache_key) {
  assert(cache_key != nullptr);
  assert(cache_key_prefix_size != 0);
  assert(cache_key_prefix_size <= BlockBasedTable::kMaxCacheKeyPrefixSize);
  memcpy(cache_key, cache_key_prefix, cache_key_prefix_size);
  char* end = EncodeVarint64(cache_key + cache_key_prefix_size, offset);
  return Slice(cache_key, static_cast<size_t>(end - cache_key));
}

Cache::Handle* GetEntryFromCache(Cache* block_cache, const Slice& key,
                                 Tickers block_cache_miss_ticker,
                                 Tickers block_cache_hit_ticker,
                                 Statistics* statistics) {
  auto cache_handle = block_cache->Lookup(key, statistics);
  if (cache_handle != nullptr) {
    PERF_COUNTER_ADD(block_cache_hit_count, 1);
//整体缓存命中
    RecordTick(statistics, BLOCK_CACHE_HIT);
//从缓存读取的总字节数
    RecordTick(statistics, BLOCK_CACHE_BYTES_READ,
               block_cache->GetUsage(cache_handle));
//块类型特定的缓存命中
    RecordTick(statistics, block_cache_hit_ticker);
  } else {
//整体缓存未命中
    RecordTick(statistics, BLOCK_CACHE_MISS);
//块类型特定的缓存未命中
    RecordTick(statistics, block_cache_miss_ticker);
  }

  return cache_handle;
}

}  //命名空间

//允许在两级索引结构中进行二进制搜索查找的索引。
class PartitionIndexReader : public IndexReader, public Cleanable {
 public:
//从文件中读取分区索引并为创建实例
//`partitionindexreader`。
//一旦成功，将填充索引读取器；否则它将保留
//未修改的
  static Status Create(BlockBasedTable* table, RandomAccessFileReader* file,
                       FilePrefetchBuffer* prefetch_buffer,
                       const Footer& footer, const BlockHandle& index_handle,
                       const ImmutableCFOptions& ioptions,
                       const InternalKeyComparator* icomparator,
                       IndexReader** index_reader,
                       const PersistentCacheOptions& cache_options,
                       const int level) {
    std::unique_ptr<Block> index_block;
    auto s = ReadBlockFromFile(
        file, prefetch_buffer, footer, ReadOptions(), index_handle,
        /*dex_块，ioptions，true/*解压缩*/，
        slice（）/*压缩dic*/, cache_options,

        /*sableglobalsequencenumber，0/*读取\u字节\u每\u位*/）；

    如果（S.O.（））{
      *索引xRealRe=
          新的分区索引读取器（Table、IComparator、Std:：Move（Index_块），
                                   ioptions.statistics，级别）；
    }

    返回S；
  }

  //返回两级迭代器：第一级在分区索引上
  虚拟内部迭代器*new迭代器（blockiter*iter=nullptr，
                                        bool don_care=true）覆盖
    //在查找索引之前已检查筛选器
    const bool skip_filters=真；
    const bool isou index=true；
    返回newtwoleveliterator（
        新建BlockBasedTable:：BlockEntryIteratorState（
            table_，readoptions（），icomperator_u，skip_filters，is_index，
            分区映射大小（）？分区\映射\：nullptr）（&P）
        index_block_->newIterator（icomperator_uu，nullptr，true））；
    //todo（myabandeh）：更新twoleveliterator以使用
    //在堆栈blockiter上，而状态为heap。目前它假设
    //第一级ITER始终在堆中，将尝试删除它
    //在它的析构函数中。
  }

  虚拟void cachedependencies（bool pin）重写
    //在读取分区之前，预取它们以避免大量的IOS
    auto rep=table_u->rep_；
    Blockiter Biter；
    块状手柄；
    index_block_->newIterator（icomperator_u，&biter，true）；
    //假定索引分区是连续的。把它们都预取下来。
    //读取第一个块偏移量
    biter.seektofirst（）；
    切片输入=biter.value（）；
    状态s=handle.decodefrom（&input）；
    断言（S.O.（））；
    如果（！）S.O.（））{
      rocks日志警告（rep->ioptions.info日志，
                     “无法读取第一个索引分区”）；
      返回；
    }
    uint64_t prefetch_off=handle.offset（）；

    //读取最后一个块的偏移量
    biter.seektolast（）；
    输入=biter.value（）；
    s=handle.decodeFrom（&input）；
    断言（S.O.（））；
    如果（！）S.O.（））{
      rocks日志警告（rep->ioptions.info日志，
                     “无法读取最后一个索引分区”）；
      返回；
    }
    uint64_t last_off=handle.offset（）+handle.size（）+kblocktrailersize；
    uint64_t prefetch_len=上次关闭-预取关闭；
    std:：unique_ptr<fileprefetchbuffer>prefetch_buffer；
    auto&file=table_u->rep_->file；
    prefetch_buffer.reset（new fileprefetchbuffer（））；
    s=prefetch_buffer->prefetch（file.get（），prefetch_off，prefetch_len）；

    //预取后，逐个读取分区
    biter.seektofirst（）；
    auto ro=readoptions（）；
    cache*block_cache=rep->table_options.block_cache.get（）；
    for（；biter.valid（）；biter.next（））
      输入=biter.value（）；
      s=handle.decodeFrom（&input）；
      断言（S.O.（））；
      如果（！）S.O.（））{
        rocks日志警告（rep->ioptions.info日志，
                       “无法读取索引分区”）；
        继续；
      }

      blockBasedTable:：cachableEntry<block>block；
      切片压缩
      if（rep->compression_dict_block）
        compression_dict=rep->compression_dict_block->data；
      }
      const bool isou index=true；
      s=table_u->maybeloaddatablocktocache（prefetch_buffer.get（），rep，ro，
                                            手柄、压缩手柄和滑块，
                                            ISSI指数；

      断言（s.ok（）block.value==nullptr）；
      如果（s.ok（）&&block.value！= null pTr）{
        断言（block.cache_handle！= null pTr）；
        如果（引脚）{
          partition_map_[handle.offset（）]=块；
          寄存器清理（&releaseCachedentry，block_cache，block.cache_handle）；
        }否则{
          block_cache->release（block.cache_handle）；
        }
      }
    }
  }

  virtual size_t size（）const override返回索引_block_u->size（）；
  虚拟大小可用大小（）常量重写
    返回index_block_u->usable_size（）；
  }

  virtual size_t approximatemmoryusage（）const override_
    断言（索引块）；
    返回index_block_u->approximemoryusage（）；
  }

 私人：
  分区索引读取器（BlockBasedTable*表，
                       常量InternalKeyComparator*IComparator，
                       std:：unique_ptr<block>&&index_block，statistics*统计，
                       常数级）
      ：索引阅读器（IComparator，stats）
        表（表）
        index_block_u（std:：move（index_block））
    断言（索引块）= null pTr）；
  }
  BlockBasedTable*表格
  std:：unique_ptr<block>index_block；
  std:：unordered_map<uint64_t，blockbasedTable:：cachableEntry<block>>
      分区图；
}；

//允许对每个块的第一个键进行二进制搜索的索引。
//此类可以被视为“block”类的瘦包装，该类已经
//支持二进制搜索。
BinarySearchindexReader类：公共索引读取器
 公众：
  //从文件中读取索引并为
  //`binarysearchindexreader`。
  //成功时，将填充索引读取器；否则将保留
  /未修改。
  静态状态创建（RandoAccessFileReader*文件，
                       filePrefetchBuffer*预取缓冲区，
                       常量页脚和页脚，常量块句柄和索引句柄，
                       常量不变的变量和变量，
                       常量InternalKeyComparator*IComparator，
                       index reader**索引阅读器，
                       const persistentcacheoptions&cache_选项）
    std:：unique_ptr<block>index_block；
    auto s=readblockfromfile（
        文件，预取缓冲区，页脚，readoptions（），索引句柄，
        &index_block，ioptions，true/*解压缩*/,

        /*ce（）/*压缩dict*/，缓存选项，
        kdisableglobalsequencenumber，0/*读取每个位的字节数*/);


    if (s.ok()) {
      *index_reader = new BinarySearchIndexReader(
          icomparator, std::move(index_block), ioptions.statistics);
    }

    return s;
  }

  virtual InternalIterator* NewIterator(BlockIter* iter = nullptr,
                                        bool dont_care = true) override {
    return index_block_->NewIterator(icomparator_, iter, true);
  }

  virtual size_t size() const override { return index_block_->size(); }
  virtual size_t usable_size() const override {
    return index_block_->usable_size();
  }

  virtual size_t ApproximateMemoryUsage() const override {
    assert(index_block_);
    return index_block_->ApproximateMemoryUsage();
  }

 private:
  BinarySearchIndexReader(const InternalKeyComparator* icomparator,
                          std::unique_ptr<Block>&& index_block,
                          Statistics* stats)
      : IndexReader(icomparator, stats), index_block_(std::move(index_block)) {
    assert(index_block_ != nullptr);
  }
  std::unique_ptr<Block> index_block_;
};

//利用内部哈希表加快对给定对象的查找的索引
//关键。
class HashIndexReader : public IndexReader {
 public:
  static Status Create(const SliceTransform* hash_key_extractor,
                       const Footer& footer, RandomAccessFileReader* file,
                       FilePrefetchBuffer* prefetch_buffer,
                       const ImmutableCFOptions& ioptions,
                       const InternalKeyComparator* icomparator,
                       const BlockHandle& index_handle,
                       InternalIterator* meta_index_iter,
                       IndexReader** index_reader,
                       bool hash_index_allow_collision,
                       const PersistentCacheOptions& cache_options) {
    std::unique_ptr<Block> index_block;
    auto s = ReadBlockFromFile(
        file, prefetch_buffer, footer, ReadOptions(), index_handle,
        /*dex_块，ioptions，true/*解压缩*/，
        slice（）/*压缩dic*/, cache_options,

        /*sableglobalsequencenumber，0/*读取\u字节\u每\u位*/）；

    如果（！）S.O.（））{
      返回S；
    }

    //注意，创建前缀哈希索引失败不需要是
    //硬错误。我们仍然可以返回到原始的二进制搜索索引。
    //因此，无论从现在开始，create都将成功。

    自动新建索引读取器=
        新的hashindexreader（icomperator，std:：move（index_块），
          IOptions.统计）；
    *index_reader=新的_index_reader；

    //获取前缀块
    blockhandle前缀\u handle；
    S=findmetablock（meta_index_iter，khashindex前缀块，
                      &prefixes_handle）；
    如果（！）S.O.（））{
      //TODO:日志错误
      返回状态：：OK（）；
    }

    //获取索引元数据块
    blockhandle前缀_meta_handle；
    S=findmetablock（meta_index_iter，khashindex前缀元数据锁，
                      前缀&prefixes_meta_handle）；
    如果（！）S.O.（））{
      //TODO:日志错误
      返回状态：：OK（）；
    }

    //读取块的内容
    blockcontents前缀为\u contents；
    s=readBlockContents（文件、预取缓冲区、页脚、readOptions（），
                          前缀\句柄，&前缀\内容，ioptions，
                          真/*解压*/, Slice() /*compression dict*/,

                          cache_options);
    if (!s.ok()) {
      return s;
    }
    BlockContents prefixes_meta_contents;
    s = ReadBlockContents(file, prefetch_buffer, footer, ReadOptions(),
                          prefixes_meta_handle, &prefixes_meta_contents,
                          /*操作，真/*解压缩*/，
                          slice（）/*压缩dic*/, cache_options);

    if (!s.ok()) {
//日志错误
      return Status::OK();
    }

    BlockPrefixIndex* prefix_index = nullptr;
    s = BlockPrefixIndex::Create(hash_key_extractor, prefixes_contents.data,
                                 prefixes_meta_contents.data, &prefix_index);
//日志错误
    if (s.ok()) {
      new_index_reader->index_block_->SetBlockPrefixIndex(prefix_index);
    }

    return Status::OK();
  }

  virtual InternalIterator* NewIterator(BlockIter* iter = nullptr,
                                        bool total_order_seek = true) override {
    return index_block_->NewIterator(icomparator_, iter, total_order_seek);
  }

  virtual size_t size() const override { return index_block_->size(); }
  virtual size_t usable_size() const override {
    return index_block_->usable_size();
  }

  virtual size_t ApproximateMemoryUsage() const override {
    assert(index_block_);
    return index_block_->ApproximateMemoryUsage() +
           prefixes_contents_.data.size();
  }

 private:
  HashIndexReader(const InternalKeyComparator* icomparator,
                  std::unique_ptr<Block>&& index_block, Statistics* stats)
      : IndexReader(icomparator, stats), index_block_(std::move(index_block)) {
    assert(index_block_ != nullptr);
  }

  ~HashIndexReader() {
  }

  std::unique_ptr<Block> index_block_;
  BlockContents prefixes_contents_;
};

//用于设置表的缓存键前缀的helper函数。
void BlockBasedTable::SetupCacheKeyPrefix(Rep* rep, uint64_t file_size) {
  assert(kMaxCacheKeyPrefixSize >= 10);
  rep->cache_key_prefix_size = 0;
  rep->compressed_cache_key_prefix_size = 0;
  if (rep->table_options.block_cache != nullptr) {
    GenerateCachePrefix(rep->table_options.block_cache.get(), rep->file->file(),
                        &rep->cache_key_prefix[0], &rep->cache_key_prefix_size);
//创建超出文件大小的索引读取器的虚拟偏移量。
    rep->dummy_index_reader_offset =
        file_size + rep->table_options.block_cache->NewId();
  }
  if (rep->table_options.persistent_cache != nullptr) {
    /*ericachePrefix（/*cache=*/nullptr，rep->file->file（），
                        &rep->persistent_cache_key_prefix[0]，
                        &rep->persistent_cache_key_prefix_size）；
  }
  如果（rep->table_options.block_cache_compressed！= null pTr）{
    generateCachePrefix（rep->table_options.block_cache_compressed.get（），
                        rep->file->file（），&rep->compressed_cache_key_prefix[0]，
                        &rep->compressed_cache_key_prefix_size）；
  }
}

void BlockBasedTable:：GenerateCachePrefix（缓存*CC，
    randomaccessfile*文件，char*缓冲区，大小

  //从文件生成ID
  *size=file->getuniqueid（buffer，kmaxcachekeyprefixsize）；

  //如果前缀没有生成或太长，
  //从缓存创建一个。
  如果（cc&&*大小==0）
    char*end=encodevarint64（buffer，cc->newid（））；
    *尺寸=静态浇铸（端部缓冲区）；
  }
}

void BlockBasedTable:：GenerateCachePrefix（缓存*CC，
    可写文件*文件，字符*缓冲区，大小

  //从文件生成ID
  *size=file->getuniqueid（buffer，kmaxcachekeyprefixsize）；

  //如果前缀没有生成或太长，
  //从缓存创建一个。
  如果（*SIZE==0）
    char*end=encodevarint64（buffer，cc->newid（））；
    *尺寸=静态浇铸（端部缓冲区）；
  }
}

命名空间{
//如果表属性具有'user\prop\u name'的值为'true'，则返回true
//或者它不包含此属性（用于向后兼容）。
bool is featuresupported（const table properties和table_properties，
                        const std:：string&user_prop_name，logger*info_log）
  auto&props=table_properties.user_collected_properties；
  auto pos=props.find（用户属性名称）；
  //旧版本没有设置此值。跳过这张支票。
  如果（POS）！=props.end（））
    如果（pos->second==kPropFalse）
      返回错误；
    否则，如果（pos->second！{kPrimtrut）{
      rocks_log_warn（info_log，“属性%s使值%s无效”，
                     user_prop_name.c_str（），pos->second.c_str（））；
    }
  }
  回归真实；
}

SequenceNumber GetGlobalSequenceNumber（常量表属性和表属性，
                                       记录器*信息日志）
  auto&props=table_properties.user_collected_properties；

  auto version_pos=props.find（externalssfilepropertyname:：kversion）；
  auto-seqno_pos=props.find（externalssfilepropertyname:：kglobalseqno）；

  if（version_pos==props.end（））
    如果（SeqNoviPOS）！=props.end（））
      //这不是外部SST文件，不支持全局\seqno。
      断言（假）；
      岩石记录错误（
          信息日志，
          “非外部sst文件具有值为%s的全局seqno属性”，
          seqno_pos->second.c_str（））；
    }
    返回kdisableglobalsequencenumber；
  }

  uint32_t version=decodeFixed32（version_pos->second.c_str（））；
  如果（版本<2）
    如果（SeqNoviPOS）！=props.end（）版本！= 1）{
      //这是v1外部sst文件，不支持全局\seqno。
      断言（假）；
      岩石记录错误（
          信息日志，
          “版本为%u的外部sst文件具有全局seqno属性”
          “值为%s”，
          版本，seqno_pos->second.c_str（））；
    }
    返回kdisableglobalsequencenumber；
  }

  序列号GuangalSoSqnNO＝CuffDeFixED64（SEQNOIOPOS）>第二。

  如果（全局序号>kmaxSequenceNumber）
    断言（假）；
    岩石记录错误（
        信息日志，
        “版本为%u的外部sst文件具有全局seqno属性”
        “值为%llu，大于kmaxSequenceNumber”，
        版本，全球编号）；
  }

  返回全局\seqno；
}
} / /命名空间

slice blockbasedtable:：getcachekey（const char*cache_key_prefix，
                                   大小缓存密钥前缀大小，
                                   const blockhandle&handle，char*cache_key）
  断言（缓存键！= null pTr）；
  断言（缓存密钥前缀大小！＝0）；
  断言（cache_key_prefix_size<=kmaxcachekeyprefixsize）；
  memcpy（缓存键、缓存键前缀、缓存键前缀大小）；
  ch*结束=
      encodevarint64（cache_key+cache_key_prefix_size，handle.offset（））；
  返回切片（cache_key，static_cast<size_t>（end-cache_key））；
}

状态块基表：：打开（常量不变的选项和IOPTIONS，
                             const env选项和env_选项，
                             Const BlockBasedTableOptions和Table_选项，
                             常量InternalKeyComparator和Internal_Comparator，
                             唯一指针<randomaccessfilereader>>&file，
                             uint64_t文件大小，
                             唯一的读卡器，
                             const bool预取缓存中的索引和过滤器，
                             const bool skip_filters，const int level）
  表_reader->reset（）；

  页脚页脚；

  std:：unique_ptr<fileprefetchbuffer>prefetch_buffer；

  //在读取页脚之前，向后读以预取数据
  const-size_ktailprefetchisize=512*1024；
  大小预取关闭；
  大小预取长度；
  if（文件大小<ktailPrefetchSize）
    预取关闭=0；
    prefetch_len=文件大小；
  }否则{
    prefetch_off=文件大小-ktailprefetchisize；
    预取len=ktailPrefetchSize；
  }
  状态S；
  //TODO将来不应该有这种特殊的逻辑。
  如果（！）文件->使用_direct_io（））
    S=文件->预取（预取关闭，预取长度）；
  }否则{
    prefetch_buffer.reset（new fileprefetchbuffer（））；
    s=prefetch_buffer->prefetch（file.get（），prefetch_off，prefetch_len）；
  }
  s=readfooterfromfile（file.get（），prefetch_buffer.get（），file_size，&footer，
                         KblockbasedTablemagicNumber）；
  如果（！）S.O.（））{
    返回S；
  }
  如果（！）blockBasedTableSupportedVersion（footer.version（））
    返回状态：：损坏（
        “未知的页脚版本。可能这个文件是用更新的
        “RocksDB版本？”）；
  }

  //我们已经成功读取了页脚。我们已经准备好了。
  //最好不要在创建后改变rep_u。例如，内部前缀转换
  //原始指针将用于创建hashindexreader，其重置可能
  //访问悬空指针。
  rep*rep=new blockbasedtable:：rep（ioptions，env_选项，table_选项，
                                      内部_比较器，跳过_过滤器）；
  rep->file=std:：move（文件）；
  rep->footer=footer；
  rep->index_type=table_options.index_type；
  rep->hash_index_allow_collision=table_options.hash_index_allow_collision；
  //我们需要用内部前缀转换来包装数据，以确保它可以
  //正确处理前缀。
  rep->internal_prefix_transform.reset（
      new internalkeyslicettransform（rep->ioptions.prefix_提取器））；
  SetupCacheKeyPrefix（rep，文件大小）；
  unique_ptr<blockbasedtable>new_table（new blockbasedtable（rep））；

  //页面缓存选项
  rep->persistent_cache_选项=
      persistent cache options（rep->table_options.persistent_缓存，
                             std:：string（rep->persistent_cache_key_prefix，
                                         rep->persistent-cache-key-prefix-size，
                                         rep->ioptions.statistics）；

  //读取元索引
  std:：unique_ptr<block>meta；
  std:：unique_ptr<internaliterator>meta_iter；
  s=readmetablock（rep，prefetch_buffer.get（），&meta，&meta_iter）；
  如果（！）S.O.（））{
    返回S；
  }

  //查找筛选器句柄和筛选器类型
  if（rep->filter_policy）
    对于（自动过滤器类型：
         rep:：filtertype:：kfullfilter，rep:：filtertype:：kpartionedfilter，
          rep:：filtertype:：kblockfilter）
      std：：字符串前缀；
      开关（滤波器类型）
        案例rep:：filtertype:：kfullfilter:
          前缀=kfullfilterblockprefix；
          断裂；
        案例rep:：filtertype:：kpartitionedfilter:
          前缀=kpartionedfilterblockprefix；
          断裂；
        案例rep:：filtertype:：kblockfilter:
          前缀=kfilterblockprefix；
          断裂；
        违约：
          断言（0）；
      }
      std:：string filter_block_key=前缀；
      filter_block_key.append（rep->filter_policy->name（））；
      if（findmetablock（meta_iter.get（），filter_block_key，&rep->filter_handle）
              OK（））{
        rep->filter_type=filter_type；
        断裂；
      }
    }
  }

  //读取属性
  bool found_properties_block=true；
  s=seektopropertiesblock（meta_iter.get（），&found_properties_block）；

  如果（！）S.O.（））{
    rocks日志警告（rep->ioptions.info日志，
                   “从文件中查找属性块时出错：%s”，
                   s.toString（）.c_str（））；
  其他if（找到_属性_块）
    s=meta-iter->status（）；
    table properties*table_properties=nullptr；
    如果（S.O.（））{
      s=readproperties（meta-iter->value（），rep->file.get（），
                         prefetch_buffer.get（），rep->footer，rep->ioptions，
                         表属性（&T）；
    }

    如果（！）S.O.（））{
      rocks日志警告（rep->ioptions.info日志，
                     “从属性读取数据时遇到错误”
                     “块%s”，
                     s.toString（）.c_str（））；
    }否则{
      rep->table_properties.reset（table_properties）；
    }
  }否则{
    rocks日志错误（rep->ioptions.info日志，
                    “找不到文件中的属性块。”）；
  }

  //读取压缩字典元块
  布尔发现了压缩音。
  s=seektocompressionDictBlock（meta_Iter.get（），&found_Compression_Dict）；
  如果（！）S.O.（））{
    罗克斯洛格
        rep->ioptions.info_日志，
        “从文件中压缩字典块时出错：%s”，
        s.toString（）.c_str（））；
  否则，如果（找到_compression_dict）
    //TODO（andrewkr）：如果cache_index_和_filter_块为
    /真的。
    unique_ptr<blockcontents>compression_dict_block new blockcontents（）
    //todo（andrewkr）：readmetablock重复seektocompressionDictBlock（）。
    //可能从meta-iter解码句柄
    //并改为执行readblockcontents（handle）
    s=rocksdb:：readmetablock（rep->file.get（），prefetch_buffer.get（），
                               文件大小，KblockBasedTableMagicNumber，
                               rep->ioptions，rocksdb:：k压缩块，
                               compression_dict_block.get（））；
    如果（！）S.O.（））{
      罗克斯洛格
          rep->ioptions.info_日志，
          “从压缩字典读取数据时遇到错误”
          “块%s”，
          s.toString（）.c_str（））；
    }否则{
      rep->compression_dict_block=std:：move（compression_dict_block）；
    }
  }

  //读取范围del元块
  布尔找到了“靶场”块；
  s=seektorangedelblock（meta_iter.get（），&found_range_del_block，
                          &rep->range\u del\u handle）；
  如果（！）S.O.（））{
    罗克斯洛格
        rep->ioptions.info_日志，
        “查找范围时出错从文件中删除逻辑删除块：%s”，
        s.toString（）.c_str（））；
  }否则{
    如果（找到“范围”块&！rep->range_del_handle.isNull（））
      读取选项读取选项；
      s=maybloadDataBlockToCache（
          prefetch_buffer.get（），rep，read_选项，rep->range_句柄，
          slice（）/*压缩\u dict*/, &rep->range_del_entry);

      if (!s.ok()) {
        ROCKS_LOG_WARN(
            rep->ioptions.info_log,
            "Encountered error while reading data from range del block %s",
            s.ToString().c_str());
      }
    }
  }

//确定是否支持整个密钥筛选。
  if (rep->table_properties) {
    rep->whole_key_filtering &=
        IsFeatureSupported(*(rep->table_properties),
                           BlockBasedTablePropertyNames::kWholeKeyFiltering,
                           rep->ioptions.info_log);
    rep->prefix_filtering &= IsFeatureSupported(
        *(rep->table_properties),
        BlockBasedTablePropertyNames::kPrefixFiltering, rep->ioptions.info_log);

    rep->global_seqno = GetGlobalSequenceNumber(*(rep->table_properties),
                                                rep->ioptions.info_log);
  }

  const bool pin =
      rep->table_options.pin_l0_filter_and_index_blocks_in_cache && level == 0;
//已打开块的预提取
//将使用块缓存访问索引/筛选器块
//总是预取0级的索引和筛选器
  if (table_options.cache_index_and_filter_blocks) {
    if (prefetch_index_and_filter_in_cache || level == 0) {
      assert(table_options.block_cache != nullptr);
//hack:调用newIndexIterator（）将索引隐式添加到
//块缓存

      CachableEntry<IndexReader> index_entry;
      unique_ptr<InternalIterator> iter(
          new_table->NewIndexIterator(ReadOptions(), nullptr, &index_entry));
      index_entry.value->CacheDependencies(pin);
      if (pin) {
        rep->index_entry = std::move(index_entry);
      } else {
        index_entry.Release(table_options.block_cache.get());
      }
      s = iter->status();

      if (s.ok()) {
//hack：调用getfilter（）将筛选器隐式添加到块缓存
        auto filter_entry = new_table->GetFilter();
        if (filter_entry.value != nullptr) {
          filter_entry.value->CacheDependencies(pin);
        }
//如果pin_l0_filter_and_index_blocks_in_cache为真，这是
//一个0级文件，然后将其保存在rep_u->filter_条目中；它将
//仅在析构函数中释放，因此它将固定在
//此读卡器处于活动状态时缓存
        if (pin) {
          rep->filter_entry = filter_entry;
        } else {
          filter_entry.Release(table_options.block_cache.get());
        }
      }
    }
  } else {
//如果不使用块缓存访问索引/筛选器块，我们将
//预加载这些块，这些块将保存在rep中的成员变量中
//和这个表对象的生命周期相同。
    IndexReader* index_reader = nullptr;
    s = new_table->CreateIndexReader(prefetch_buffer.get(), &index_reader,
                                     meta_iter.get(), level);
    if (s.ok()) {
      rep->index_reader.reset(index_reader);
//分区索引的分区总是存储在缓存中。他们
//因此，无论
//缓存索引块和筛选器块的值
      if (prefetch_index_and_filter_in_cache || level == 0) {
        rep->index_reader->CacheDependencies(pin);
      }

//设置过滤块
      if (rep->filter_policy) {
        const bool is_a_filter_partition = true;
        auto filter = new_table->ReadFilter(
            prefetch_buffer.get(), rep->filter_handle, !is_a_filter_partition);
        rep->filter.reset(filter);
//请参阅上面关于分区索引总是
//缓存的
        if (filter && (prefetch_index_and_filter_in_cache || level == 0)) {
          filter->CacheDependencies(pin);
        }
      }
    } else {
      delete index_reader;
    }
  }

  if (s.ok()) {
    *table_reader = std::move(new_table);
  }

  return s;
}

void BlockBasedTable::SetupForCompaction() {
  switch (rep_->ioptions.access_hint_on_compaction_start) {
    case Options::NONE:
      break;
    case Options::NORMAL:
      rep_->file->file()->Hint(RandomAccessFile::NORMAL);
      break;
    case Options::SEQUENTIAL:
      rep_->file->file()->Hint(RandomAccessFile::SEQUENTIAL);
      break;
    case Options::WILLNEED:
      rep_->file->file()->Hint(RandomAccessFile::WILLNEED);
      break;
    default:
      assert(false);
  }
}

std::shared_ptr<const TableProperties> BlockBasedTable::GetTableProperties()
    const {
  return rep_->table_properties;
}

size_t BlockBasedTable::ApproximateMemoryUsage() const {
  size_t usage = 0;
  if (rep_->filter) {
    usage += rep_->filter->ApproximateMemoryUsage();
  }
  if (rep_->index_reader) {
    usage += rep_->index_reader->ApproximateMemoryUsage();
  }
  return usage;
}

//从文件加载元块。成功后，返回加载的元块
//以及它的迭代器。
Status BlockBasedTable::ReadMetaBlock(Rep* rep,
                                      FilePrefetchBuffer* prefetch_buffer,
                                      std::unique_ptr<Block>* meta_block,
                                      std::unique_ptr<InternalIterator>* iter) {
//TODO（sanjay）：如果footer.metaindex_handle（）大小指示
//它是一个空块。
  std::unique_ptr<Block> meta;
  Status s = ReadBlockFromFile(
      rep->file.get(), prefetch_buffer, rep->footer, ReadOptions(),
      rep->footer.metaindex_handle(), &meta, rep->ioptions,
      /*e/*解压*/，slice（）/*压缩dict*/，
      rep->persistent_cache_选项，kdisableglobalsequencenumber，
      0/*读取每个位的字节数*/);


  if (!s.ok()) {
    ROCKS_LOG_ERROR(rep->ioptions.info_log,
                    "Encountered error while reading data from properties"
                    " block %s",
                    s.ToString().c_str());
    return s;
  }

  *meta_block = std::move(meta);
//元块使用bytewise比较器。
  iter->reset(meta_block->get()->NewIterator(BytewiseComparator()));
  return Status::OK();
}

Status BlockBasedTable::GetDataBlockFromCache(
    const Slice& block_cache_key, const Slice& compressed_block_cache_key,
    Cache* block_cache, Cache* block_cache_compressed,
    const ImmutableCFOptions& ioptions, const ReadOptions& read_options,
    BlockBasedTable::CachableEntry<Block>* block, uint32_t format_version,
    const Slice& compression_dict, size_t read_amp_bytes_per_bit,
    bool is_index) {
  Status s;
  Block* compressed_block = nullptr;
  Cache::Handle* block_cache_compressed_handle = nullptr;
  Statistics* statistics = ioptions.statistics;

//首先查找未压缩的缓存
  if (block_cache != nullptr) {
    block->cache_handle = GetEntryFromCache(
        block_cache, block_cache_key,
        is_index ? BLOCK_CACHE_INDEX_MISS : BLOCK_CACHE_DATA_MISS,
        is_index ? BLOCK_CACHE_INDEX_HIT : BLOCK_CACHE_DATA_HIT, statistics);
    if (block->cache_handle != nullptr) {
      block->value =
          reinterpret_cast<Block*>(block_cache->Value(block->cache_handle));
      return s;
    }
  }

//如果找不到，请从压缩块缓存中搜索。
  assert(block->cache_handle == nullptr && block->value == nullptr);

  if (block_cache_compressed == nullptr) {
    return s;
  }

  assert(!compressed_block_cache_key.empty());
  block_cache_compressed_handle =
      block_cache_compressed->Lookup(compressed_block_cache_key);
//如果在压缩缓存中找到，则解压缩并插入
//未压缩缓存
  if (block_cache_compressed_handle == nullptr) {
    RecordTick(statistics, BLOCK_CACHE_COMPRESSED_MISS);
    return s;
  }

//找到压缩块
  RecordTick(statistics, BLOCK_CACHE_COMPRESSED_HIT);
  compressed_block = reinterpret_cast<Block*>(
      block_cache_compressed->Value(block_cache_compressed_handle));
  assert(compressed_block->compression_type() != kNoCompression);

//将未压缩的内容检索到新的缓冲区中
  BlockContents contents;
  s = UncompressBlockContents(compressed_block->data(),
                              compressed_block->size(), &contents,
                              format_version, compression_dict,
                              ioptions);

//将未压缩块插入块缓存
  if (s.ok()) {
    block->value =
        new Block(std::move(contents), compressed_block->global_seqno(),
                  read_amp_bytes_per_bit,
statistics);  //未压缩块
    assert(block->value->compression_type() == kNoCompression);
    if (block_cache != nullptr && block->value->cachable() &&
        read_options.fill_cache) {
      s = block_cache->Insert(
          block_cache_key, block->value, block->value->usable_size(),
          &DeleteCachedEntry<Block>, &(block->cache_handle));
      block_cache->TEST_mark_as_data_block(block_cache_key,
                                           block->value->usable_size());
      if (s.ok()) {
        RecordTick(statistics, BLOCK_CACHE_ADD);
        if (is_index) {
          RecordTick(statistics, BLOCK_CACHE_INDEX_ADD);
          RecordTick(statistics, BLOCK_CACHE_INDEX_BYTES_INSERT,
                     block->value->usable_size());
        } else {
          RecordTick(statistics, BLOCK_CACHE_DATA_ADD);
          RecordTick(statistics, BLOCK_CACHE_DATA_BYTES_INSERT,
                     block->value->usable_size());
        }
        RecordTick(statistics, BLOCK_CACHE_BYTES_WRITE,
                   block->value->usable_size());
      } else {
        RecordTick(statistics, BLOCK_CACHE_ADD_FAILURES);
        delete block->value;
        block->value = nullptr;
      }
    }
  }

//释放对压缩缓存项的保留
  block_cache_compressed->Release(block_cache_compressed_handle);
  return s;
}

Status BlockBasedTable::PutDataBlockToCache(
    const Slice& block_cache_key, const Slice& compressed_block_cache_key,
    Cache* block_cache, Cache* block_cache_compressed,
    const ReadOptions& read_options, const ImmutableCFOptions& ioptions,
    CachableEntry<Block>* block, Block* raw_block, uint32_t format_version,
    const Slice& compression_dict, size_t read_amp_bytes_per_bit, bool is_index,
    Cache::Priority priority) {
  assert(raw_block->compression_type() == kNoCompression ||
         block_cache_compressed != nullptr);

  Status s;
//将未压缩的内容检索到新的缓冲区中
  BlockContents contents;
  Statistics* statistics = ioptions.statistics;
  if (raw_block->compression_type() != kNoCompression) {
    s = UncompressBlockContents(raw_block->data(), raw_block->size(), &contents,
                                format_version, compression_dict, ioptions);
  }
  if (!s.ok()) {
    delete raw_block;
    return s;
  }

  if (raw_block->compression_type() != kNoCompression) {
    block->value = new Block(std::move(contents), raw_block->global_seqno(),
                             read_amp_bytes_per_bit,
statistics);  //未压缩块
  } else {
    block->value = raw_block;
    raw_block = nullptr;
  }

//将压缩块插入压缩块缓存。
//立即释放对压缩缓存项的保留。
  if (block_cache_compressed != nullptr && raw_block != nullptr &&
      raw_block->cachable()) {
    s = block_cache_compressed->Insert(compressed_block_cache_key, raw_block,
                                       raw_block->usable_size(),
                                       &DeleteCachedEntry<Block>);
    if (s.ok()) {
//请避免使用以下代码删除此缓存块。
      raw_block = nullptr;
      RecordTick(statistics, BLOCK_CACHE_COMPRESSED_ADD);
    } else {
      RecordTick(statistics, BLOCK_CACHE_COMPRESSED_ADD_FAILURES);
    }
  }
  delete raw_block;

//插入未压缩的块缓存
  assert((block->value->compression_type() == kNoCompression));
  if (block_cache != nullptr && block->value->cachable()) {
    s = block_cache->Insert(
        block_cache_key, block->value, block->value->usable_size(),
        &DeleteCachedEntry<Block>, &(block->cache_handle), priority);
    block_cache->TEST_mark_as_data_block(block_cache_key,
                                         block->value->usable_size());
    if (s.ok()) {
      assert(block->cache_handle != nullptr);
      RecordTick(statistics, BLOCK_CACHE_ADD);
      if (is_index) {
        RecordTick(statistics, BLOCK_CACHE_INDEX_ADD);
        RecordTick(statistics, BLOCK_CACHE_INDEX_BYTES_INSERT,
                   block->value->usable_size());
      } else {
        RecordTick(statistics, BLOCK_CACHE_DATA_ADD);
        RecordTick(statistics, BLOCK_CACHE_DATA_BYTES_INSERT,
                   block->value->usable_size());
      }
      RecordTick(statistics, BLOCK_CACHE_BYTES_WRITE,
                 block->value->usable_size());
      assert(reinterpret_cast<Block*>(
                 block_cache->Value(block->cache_handle)) == block->value);
    } else {
      RecordTick(statistics, BLOCK_CACHE_ADD_FAILURES);
      delete block->value;
      block->value = nullptr;
    }
  }

  return s;
}

FilterBlockReader* BlockBasedTable::ReadFilter(
    FilePrefetchBuffer* prefetch_buffer, const BlockHandle& filter_handle,
    const bool is_a_filter_partition) const {
  auto& rep = rep_;
//TODO:如果我们开始
//要求在表：：open中进行校验和验证。
  if (rep->filter_type == Rep::FilterType::kNoFilter) {
    return nullptr;
  }
  BlockContents block;
  if (!ReadBlockContents(rep->file.get(), prefetch_buffer, rep->footer,
                         ReadOptions(), filter_handle, &block, rep->ioptions,
                         /*se/*解压*/，slice（）/*压缩dict*/，
                         rep->持久缓存选项）
           OK（））{
    //读取块时出错
    返回null pTR；
  }

  断言（rep->filter_policy）；

  auto filter_type=rep->filter_type；
  if（rep->filter_type==rep:：filter type:：kpartionedfilter&&
      是一个过滤分区）
    filter_type=rep:：filter type:：kfullfilter；
  }

  开关（滤波器类型）
    案例报告：：filtertype:：kpartitionedfilter:
      返回新的partitionedfilterblockreader（
          rep->前缀过滤？rep->ioptons.prefix抽取器：nullptr，
          rep->whole_key_filtering，std:：move（block），nullptr，
          rep->ioptions.statistics，rep->internal_comparator，this）；
    }

    案例rep:：filtertype:：kblockfilter:
      返回新的BlockBasedFilterBlockReader（
          rep->前缀过滤？rep->ioptons.prefix抽取器：nullptr，
          rep->table_options，rep->whole_key_filtering，std：：move（block），
          rep->ioptions.statistics）；

    案例报告：：filtertype：：kfullfilter:
      自动过滤位读卡器=
          rep->filter_policy->getfilterbitsreader（block.data）；
      断言（过滤位读卡器！= null pTr）；
      返回新的fullfilterblockreader（
          rep->前缀过滤？rep->ioptons.prefix抽取器：nullptr，
          rep->whole_key_filtering，std:：move（block），filter_bits_reader，
          rep->ioptions.statistics）；
    }

    违约：
      //filter_type为knofilter（在第一个if时退出函数），
      //或者必须包含在这个开关块中
      断言（假）；
      返回null pTR；
  }
}

blockBasedTable:：cachableEntry<filterBlockReader>blockBasedTable:：getFilter（
    fileprefetchbuffer*预取缓冲区，bool no_io）const_
  const blockhandle&filter_blk_handle=rep_->filter_handle；
  const bool是_a_filter_partition=true；
  返回getfilter（预取缓冲区，筛选器句柄，！是一个过滤分区，
                   诺亚欧）；
}

blockBasedTable:：cachableEntry<filterBlockReader>blockBasedTable:：getFilter（
    fileprefetchbuffer*预取缓冲区、const blockhandle&filter_k_句柄，
    const bool是一个过滤器分区，bool no_io）const
  //如果缓存索引块和筛选器块为假，则应预先填充筛选器。
  //我们还是返回rep_u->filter。rep_u->filter可以为nullptr if filter
  //读取在open（）时失败。我们不想再重新加载，因为它会
  //很可能再次失败。
  如果（！）是一个过滤分区&&
      ！rep_->table_options.cache_index_and_filter_blocks）
    返回rep->filter.get（），nullptr/*缓存句柄*/};

  }

  Cache* block_cache = rep_->table_options.block_cache.get();
  /*（rep_->filter_policy==nullptr/*不要使用filter*/
      block_cache==nullptr/*完全没有块缓存*/) {

    /*urn nullptr/*filter*/，nullptr/*缓存句柄*/
  }

  如果（！）是_a filter_partition&&rep_u->filter_entry.isset（））
    返回rep->filter_entry；
  }

  性能定时器保护（读取滤波器块纳米）；

  //从缓存中提取
  char cache_key[kmaxcachekeyprefixsize+kmaxvarint64length]；
  auto key=getcachekey（rep_->cache_key_prefix，rep_->cache_key_prefix_size，
                         过滤_k_句柄，缓存_键）；

  statistics*statistics=rep_u->ioptons.statistics；
  自动缓存句柄=
      GetEntryFromCache（块缓存、键、块缓存、筛选器未命中，
                        块缓存过滤命中，统计）；

  filterblockreader*filter=nullptr；
  如果（缓存句柄！= null pTr）{
    filter=reinterpret_cast<filterblockreader*>（）
        block_cache->value（cache_handle））；
  否则，如果（没有_io）
    //不要调用任何IO。
    返回cachableEntry<filterBlockReader>（）；
  }否则{
    过滤器=
        readfilter（prefetch_buffer，filter_blk_handle，is_a_filter_partition）；
    如果（过滤器！= null pTr）{
      断言（filter->size（）>0）；
      状态S=块缓存->插入（
          key，filter，filter->size（），&deletecachedfilterentry，&cache_handle，
          rep_->table_options.cache_index_and_filter_blocks_with_high_priority
              ？缓存：：优先级：：高
              ：缓存：：优先级：：低）；
      如果（S.O.（））{
        记录标记（统计，块缓存添加）；
        记录标记（统计，块缓存过滤器添加）；
        recordtick（统计，block_cache_filter_bytes_insert，filter->size（））；
        recordtick（统计，block_cache_bytes_write，filter->size（））；
      }否则{
        记录标记（统计，块缓存添加失败）；
        删除过滤器；
        返回cachableEntry<filterBlockReader>（）；
      }
    }
  }

  返回过滤器，缓存句柄
}

InternalIterator*BlockBasedTable:：NewIndexIterator（
    const read options&read_options，blockiter*输入_iter，
    cachableEntry<indexreader>*index_entry）
  //索引读取器已预填充。
  if（rep_->index_reader）
    返回rep->index->newiterator（
        输入“iter”，读取“options.total”命令；
  }
  //我们有一个固定索引块
  if（rep_->index_entry.isset（））
    返回rep_u->index_entry.value->newiterator（input_iter，
                                                读取“选项。总订单搜索”）；
  }

  性能计时器保护（读取索引块纳米）；

  const bool no_io=read_options.read_tier==kblockcachetier；
  cache*block_cache=rep_->table_options.block_cache.get（）；
  char cache_key[kmaxcachekeyprefixsize+kmaxvarint64length]；
  自动键=
      getcachekefromoffset（rep_->cache_key_prefix，rep_->cache_key_prefix_size，
                            rep->dummy_index_reader_offset，cache_key）；
  statistics*statistics=rep_u->ioptons.statistics；
  自动缓存句柄=
      GetEntryFromCache（块缓存、键、块缓存、索引丢失，
                        块缓存，索引命中，统计数据）；

  if（cache_handle==nullptr&&no_io）
    如果（输入“Iter”！= null pTr）{
      输入“->setstatus”（状态：：不完整（“无阻塞IO”））；
      返回输入信号；
    }否则{
      返回newErrorInternalIterator（status:：incomplete（“no blocking io”））；
    }
  }

  index reader*索引读取器=nullptr；
  如果（缓存句柄！= null pTr）{
    索引xRealDe=
        reinterpret_cast<indexreader*>（block_cache->value（cache_handle））；
  }否则{
    //创建索引读取器并将其放入缓存中。
    状态S；
    测试同步点（“BlockBasedTable:：NewIndexIterator:：Thread2:2”）；
    S=CreateIndexReader（nullptr/*预取缓冲区）*/, &index_reader);

    TEST_SYNC_POINT("BlockBasedTable::NewIndexIterator::thread1:1");
    TEST_SYNC_POINT("BlockBasedTable::NewIndexIterator::thread2:3");
    TEST_SYNC_POINT("BlockBasedTable::NewIndexIterator::thread1:4");
    if (s.ok()) {
      assert(index_reader != nullptr);
      s = block_cache->Insert(
          key, index_reader, index_reader->usable_size(),
          &DeleteCachedIndexEntry, &cache_handle,
          rep_->table_options.cache_index_and_filter_blocks_with_high_priority
              ? Cache::Priority::HIGH
              : Cache::Priority::LOW);
    }

    if (s.ok()) {
      size_t usable_size = index_reader->usable_size();
      RecordTick(statistics, BLOCK_CACHE_ADD);
      RecordTick(statistics, BLOCK_CACHE_INDEX_ADD);
      RecordTick(statistics, BLOCK_CACHE_INDEX_BYTES_INSERT, usable_size);
      RecordTick(statistics, BLOCK_CACHE_BYTES_WRITE, usable_size);
    } else {
      if (index_reader != nullptr) {
        delete index_reader;
      }
      RecordTick(statistics, BLOCK_CACHE_ADD_FAILURES);
//如果出现问题，索引阅读器应保持完好。
      if (input_iter != nullptr) {
        input_iter->SetStatus(s);
        return input_iter;
      } else {
        return NewErrorInternalIterator(s);
      }
    }

  }

  assert(cache_handle);
  auto* iter = index_reader->NewIterator(
      input_iter, read_options.total_order_seek);

//调用方希望取得索引块的所有权
//不要调用registercleanup（）。在这种情况下，调用方将处理它。
  if (index_entry != nullptr) {
    *index_entry = {index_reader, cache_handle};
  } else {
    iter->RegisterCleanup(&ReleaseCachedEntry, block_cache, cache_handle);
  }

  return iter;
}

InternalIterator* BlockBasedTable::NewDataBlockIterator(
    Rep* rep, const ReadOptions& ro, const Slice& index_value,
    BlockIter* input_iter, bool is_index) {
  BlockHandle handle;
  Slice input = index_value;
//我们故意允许索引值中的额外内容，以便
//将来可以添加更多功能。
  Status s = handle.DecodeFrom(&input);
  return NewDataBlockIterator(rep, ro, handle, input_iter, is_index, s);
}

//转换索引迭代器值（即编码的blockhandle）
//对相应块的内容进行迭代。
//如果输入寄存器为空，则新建一个迭代器
//如果输入不为空，请更新此iter并返回它
InternalIterator* BlockBasedTable::NewDataBlockIterator(
    Rep* rep, const ReadOptions& ro, const BlockHandle& handle,
    BlockIter* input_iter, bool is_index, Status s) {
  PERF_TIMER_GUARD(new_table_block_iter_nanos);

  const bool no_io = (ro.read_tier == kBlockCacheTier);
  Cache* block_cache = rep->table_options.block_cache.get();
  CachableEntry<Block> block;
  Slice compression_dict;
  if (s.ok()) {
    if (rep->compression_dict_block) {
      compression_dict = rep->compression_dict_block->data;
    }
    /*maybeloaddatablocktocache（nullptr/*预取缓冲区*/，rep，ro，handle，
                                  压缩_dict，&block，是_index）；
  }

  //没有从块缓存中获取任何数据。
  if（s.ok（）&&block.value==nullptr）
    如果（NosiIO）{
      //无法从块缓存读取，无法执行IO
      如果（输入“Iter”！= null pTr）{
        输入“->setstatus”（状态：：不完整（“无阻塞IO”））；
        返回输入信号；
      }否则{
        返回newErrorInternalIterator（status:：incomplete（“no blocking io”））；
      }
    }
    std:：unique_ptr<block>block_value；
    s=readblockfromfile（rep->file.get（），nullptr/*预取缓冲区*/,

                          rep->footer, ro, handle, &block_value, rep->ioptions,
                          /*E/*压缩*/，压缩指令，
                          rep->persistent_cache_options，rep->global_seqno，
                          rep->table_options.read_amp_bytes_per_bit）；
    如果（S.O.（））{
      block.value=block_value.release（）；
    }
  }

  内部迭代器*iter；
  如果（S.O.（））{
    断言（block.value！= null pTr）；
    iter=block.value->newiterator（&rep->internal_comparator，input_iter，true，
                                    rep->ioptions.statistics）；
    如果（block.cache_handle！= null pTr）{
      iter->registercleanup（&releasecachedentry，block_cache，
                            block.cache_句柄）；
    }否则{
      iter->registercleanup（&deleteheldresource<block>，block.value，nullptr）；
    }
  }否则{
    断言（block.value==nullptr）；
    如果（输入“Iter”！= null pTr）{
      输入“iter->setstatus”（s）；
      iter=输入iter；
    }否则{
      iter=newErrorInternalIterator（s）；
    }
  }
  返回ITER；
}

状态块基数据表：：maybeloaddatablocktocache（
    fileprefetchbuffer*预取缓冲区，rep*rep，const readoptions&ro，
    const blockhandle&handle，slice compression_dict，
    cachableEntry<block>>*block_entry，bool is_index）
  断言（阻止输入！= null pTr）；
  const bool no_io=（ro.read_tier==kblockcachetier）；
  cache*block_cache=rep->table_options.block_cache.get（）；
  缓存*块缓存\压缩=
      rep->table_options.block_cache_compressed.get（）；

  //如果启用了任何一个块缓存，我们将尝试从中读取。
  状态S；
  如果（阻止缓存！=nullptr block_cache_compressed！= null pTr）{
    statistics*statistics=rep->ioptions.statistics；
    char cache_key[kmaxcachekeyprefixsize+kmaxvarint64length]；
    char compressed_cache_key[kmaxcachekeyprefixsize+kmaxvarint64length]；
    Slice键，/*键到块缓存*/

        /*y/*压缩块缓存的键*/；

    //为块缓存创建键
    如果（阻止缓存！= null pTr）{
      key=getcachekey（rep->cache_key_prefix，rep->cache_key_prefix_size，
                        句柄，缓存键）；
    }

    如果（块缓存已压缩！= null pTr）{
      ckey=getcachekey（rep->compressed_cache_key_prefix，
                         rep->压缩缓存密钥前缀大小，句柄，
                         压缩缓存键）；
    }

    s=getdatablockfromcache（
        key，ckey，block_cache，block_cache_compressed，rep->ioptions，ro，
        block_entry，rep->table_options.format_version，compression_dict，
        rep->table_options.read_amp_bytes_per_bit，is_index）；

    如果（block_entry->value==nullptr&！无“IO和RO.Fill”缓存）
      std:：unique_ptr<block>raw_block；
      {
        秒表软件（rep->ioptions.env，statistics，read_block_get_micros）；
        s=readblockfromfile（
            rep->file.get（），预取缓冲区，rep->footer，ro，handle，
            &raw_block，rep->ioptions，block_cache_compressed==nullptr，
            压缩目录，rep->persistent缓存选项，rep->global\seqno，
            rep->table_options.read_amp_bytes_per_bit）；
      }

      如果（S.O.（））{
        S=PutDataBlockToCache（
            key，ckey，block_cache，block_cache_compressed，ro，rep->ioptions，
            block_entry，raw_block.release（），rep->table_options.format_version，
            压缩_dict，rep->table_options.read_amp_bytes_per_位，
            伊斯指数
            ISSI指数与
                    rep->表选项
                        .cache_index_and_filter_blocks_with_high_priority
                ？缓存：：优先级：：高
                ：缓存：：优先级：：低）；
      }
    }
  }
  断言（s.ok（）block_entry->value==nullptr）；
  返回S；
}

blockBasedTable:：blockEntryIteratorState:：blockEntryIteratorState（）。
    blockbasedtable*表，const read options&read_选项，
    const internalKeyComparator*icomperator，bool skip_filters，bool is_index，
    std:：unordered_map<uint64_t，cachableEntry<block>>*block_map）
    ：twoleveliteratorstate（table->rep->ioptions.prefix_提取器！= null pTr）
      表（表）
      读取选项（读取选项）
      i压缩机（i压缩机）
      跳过“过滤器”（跳过“过滤器”），
      为_index_u（为_index），
      区块图（区块图）

内部迭代器*
BlockBasedTable:：BlockEntryIterator或State:：Newsecondary迭代器（
    常量切片和索引_值）
  //返回索引分区上的块迭代器
  块状手柄；
  切片输入=索引值；
  状态s=handle.decodefrom（&input）；
  auto rep=table_u->rep_；
  如果（方块图）
    auto block=block_map_u->find（handle.offset（））；
    //这是一种可能的情况，因为块缓存可能没有空间
    //对于分区
    如果（块！=block_map_->end（））
      性能计数器添加（块缓存命中计数，1）；
      recordtick（rep->ioptions.statistics，block_cache_index_hit）；
      记录标记（rep->ioptions.statistics，块缓存命中）；
      cache*block_cache=rep->table_options.block_cache.get（）；
      断言（块缓存）；
      记录标记（rep->ioptions.statistics，block_cache_bytes_read，
                 block_cache->getusage（block->second.cache_handle））；
      返回块->second.value->newIterator（
          &rep->internal_comparator，nullptr，true，rep->ioptons.statistics）；
    }
  }
  返回newDataBlockIterator（rep，read_options_uu，handle，nullptr，is_index_u，
                              s）；
}

bool blockbasedTable:：blockEntryIteratorState:：PrefixMayMatch（）。
    常量切片和内部关键字）
  if（读_options_u total_order_seek_skip_filters_
    回归真实；
  }
  返回表_u->prefixmaymatch（内部_键）；
}

bool blockBasedTable:：blockEntryIteratorState:：keyreachedupperBound（）。
    常量切片和内部关键字）
  bool达到了_upper_bound=read_options_u.iterate_upper_bound！= NulLPTR & &
                             我想知道！= NulLPTR & &
                             icomparator_u->user_comparator（）->compare（）。
                                 ExtractUserKey（内部_键），
                                 *读取选项u.迭代_上限）>=0；
  测试同步点回调（
      “BlockBasedTable:：BlockEntryIteratorState:：KeyReacheDupperBound”，
      达到上限）；
  返回达到上限；
}

//如果用户指定了一个异常的实现，这将被破坏
//of options.comparator，或者如果用户指定了一个异常的
//blockBasedTableOptions.filter_策略中前缀的定义。
//特别是，我们需要以下三个属性：
/ /
//1）key.starts_with（prefix（key））。
//2）比较（前缀（键），键）<=0。
//3）如果比较（key1，key2）<=0，则比较（prefix（key1），prefix（key2））<=0
/ /
//否则，此方法保证不会发生I/O。
/ /
//要求：持有db锁时不应调用此方法。
bool blockbasedtable:：prefixmaymatch（const slice&internal_key）
  如果（！）rep_u->filter_policy）
    回归真实；
  }

  断言（rep->ioptions.prefix_提取器！= null pTr）；
  auto user_key=extractUserkey（内部_key）；
  如果（！）rep_->ioptions.prefix_extractor->indomain（user_key）
      rep_u->table_properties->prefix_extractor_name.compare（
          rep_->ioptions.prefix_extractor->name（））！= 0）{
    回归真实；
  }
  auto prefix=rep_->ioptions.prefix_extractor->transform（user_key）；

  bool may_match=true；
  状态S；

  //首先，尝试使用全过滤器检查
  auto filter_entry=getfilter（）；
  filterBlockReader*filter=filter_entry.value；
  如果（过滤器！= null pTr）{
    如果（！）filter->isblockbased（））
      const slice*const const_ikey_ptr=&internal_key；
      玛雅火柴=
          filter->prefixmaymatch（前缀，knotvalid，false，const-ikey-ptr）；
    }否则{
      internal key internal_key_prefix（前缀，kmaxSequenceNumber，ktypeValue）；
      auto internal_prefix=内部_key_prefix.encode（）；

      //为了防止此方法中的任何IO操作，我们将'read_tier'设置为
      //当然，只有当索引或筛选器
      //已加载到内存中。
      读选项没有读选项；
      没有读取选项。读取层=kblockcachetier；

      //然后，尝试在每个块中找到它
      unique_ptr<internaliterator>iiter（newindexiterator（no_io_read_options））；
      iiter->seek（内部前缀）；

      如果（！）iiter->valid（））
        //我们已经过了文件结尾
        //如果不完整，就意味着我们避免了I/O
        我们不确定我们是否已经过了终点
        //文件
        may_match=iiter->status（）.isinComplete（）；
      else if（extractUserkey（iiter->key（））
                     .以（extractuserkey（internal_prefix））开始_
        //我们需要检查这个微妙的情况，因为
        //保证“该键是一个字符串>=该数据中的最后一个键”
        //block“根据doc/table_format.txt规范
        / /
        //假设iiter->key（）以所需前缀开头；它不是
        //相应的数据块将
        //包含前缀，因为iiter->key（）不需要位于
        //块。但是，下一个数据块可能包含前缀，因此
        //我们返回true以保证安全。
        可以匹配=真；
      else if（filter->isBlockBased（））
        //iiter->key（）不是以所需前缀开头的。因为
        //seek（）查找大于等于seek目标的第一个键，此
        //表示iter->key（）>前缀。因此，任何数据块
        //在iiter->key（）对应的数据块之后不能
        //可能包含键。因此，相应的数据块
        //是上唯一可能包含前缀的。
        切片句柄_value=iiter->value（）；
        块状手柄；
        s=handle.decodeFrom（&handle_value）；
        断言（S.O.（））；
        may_match=filter->prefix may match（prefix，handle.offset（））；
      }
    }
  }

  statistics*statistics=rep_u->ioptons.statistics；
  记录标记（统计，bloom_filter_prefix_checked）；
  如果（！）Mayl火柴）{
    recordtick（统计数据，bloom_filter_prefix_used）；
  }

  //如果rep_->filter_entry未设置，则应调用release（）；否则
  //不要调用，在这种情况下，rep_->filter_条目中有一个本地副本，
  //它已固定到缓存中，将在析构函数中释放
  如果（！）rep_->filter_entry.isset（））
    filter_entry.release（rep_->table_options.block_cache.get（））；
  }

  返回可能匹配；
}

InternalIterator*BlockBasedTable:：NewIterator（const read options&read_options，
                                               竞技场*竞技场，
                                               bool skip_filters）_
  返回newtwoleveliterator（
      新建blockEntryIterator或State（此，读取选项，
                                  &rep_->internal_comparator，skip_filters，，
      newindexiterator（read_选项），arena）；
}

InternalIterator*BlockBasedTable:：NewRangeTombstoneIterator（
    const read options&read_options）
  if（rep_->range_del_handle.isNull（））
    //块不存在，nullptr表示没有范围逻辑删除。
    返回null pTR；
  }
  如果（rep_->range_del_entry.cache_handle！= null pTr）{
    //我们有一个未压缩块缓存项的句柄
    //此表的生存期。在返回
    //基于它的迭代器，因为返回的迭代器可能比此表长
    /读者。
    断言（rep_->range_del_entry.value！= null pTr）；
    cache*block_cache=rep_->table_options.block_cache.get（）；
    断言（块缓存！= null pTr）；
    if（block_cache->ref（rep_->range_del_entry.cache_handle））
      auto iter=rep_->range_del_entry.value->newIterator（
          &rep_->internal_comparator，nullptr/*iter*/,

          /*e/*总搜索次数，rep->ioptions.statistics）；
      iter->registercleanup（&releasecachedentry，block_cache，
                            rep_->range_del_entry.cache_handle）；
      返回ITER；
    }
  }
  std：：字符串str；
  rep_->range_del_handle.encodeto（&str）；
  //元块存在，但不在未压缩的块缓存中（可能是因为
  //已禁用），请执行完整查找过程。
  返回newDataBlockIterator（rep_uu，read_options，slice（str））；
}

bool blockbasedtable:：fullfilterkeymaymatch（const read options&read_选项，
                                            filterblockreader*过滤器，
                                            常量切片和内部关键字，
                                            const bool no_io）const_
  if（filter==nullptr filter->isBlockBased（））
    回归真实；
  }
  slice user_key=extractUserkey（内部_key）；
  const slice*const const_ikey_ptr=&internal_key；
  if（filter->whole_key_filtering（））
    返回过滤器->keymaymatch（用户密钥，knotvalid，无IO，const-ikey-ptr）；
  }
  如果（！）请阅读“选项”。total“订单”seek&&rep->ioptions.prefix“提取器”&&
      rep_u->table_properties->prefix_extractor_name.compare（
          rep_->ioptions.prefix_extractor->name（））==0&&
      rep_u->ioptions.prefix_extractor->indomain（user_key）&&
      ！过滤器->前缀可能匹配（
          rep_u->ioptions.prefix_extractor->transform（user_key），knotvalid，
          假，const-ikey-ptr）
    
  }
  回归真实；
}

状态块基表：：get（const read options&read_options，const slice&key，
                            getContext*获取_context，bool skip_filters）
  状态S；
  const bool no_io=read_options.read_tier==kblockcachetier；
  cachableEntry<filterBlockReader>filter_entry；
  如果（！）SkIPa滤波器（{）
    filter_entry=getfilter（/*预取buffe*/ nullptr,

                             read_options.read_tier == kBlockCacheTier);
  }
  FilterBlockReader* filter = filter_entry.value;

//首先检查全滤器
//如果完全筛选无效，则进入每个块
  if (!FullFilterKeyMayMatch(read_options, filter, key, no_io)) {
    RecordTick(rep_->ioptions.statistics, BLOOM_FILTER_USEFUL);
  } else {
    BlockIter iiter_on_stack;
    auto iiter = NewIndexIterator(read_options, &iiter_on_stack);
    std::unique_ptr<InternalIterator> iiter_unique_ptr;
    if (iiter != &iiter_on_stack) {
      iiter_unique_ptr.reset(iiter);
    }

    bool done = false;
    for (iiter->Seek(key); iiter->Valid() && !done; iiter->Next()) {
      Slice handle_value = iiter->value();

      BlockHandle handle;
      bool not_exist_in_filter =
          filter != nullptr && filter->IsBlockBased() == true &&
          handle.DecodeFrom(&handle_value).ok() &&
          !filter->KeyMayMatch(ExtractUserKey(key), handle.offset(), no_io);

      if (not_exist_in_filter) {
//找不到
//托多：考虑一下与合并的交互。如果用户密钥不能
//跨越一个数据块，我们会没事的。
        RecordTick(rep_->ioptions.statistics, BLOOM_FILTER_USEFUL);
        break;
      } else {
        BlockIter biter;
        NewDataBlockIterator(rep_, read_options, iiter->value(), &biter);

        if (read_options.read_tier == kBlockCacheTier &&
            biter.status().IsIncomplete()) {
//无法从块缓存获取块
//将saver.state更新为found，因为我们只查找
//我们可以保证在设置“无IO”时钥匙不在那里
          get_context->MarkKeyMayExist();
          break;
        }
        if (!biter.status().ok()) {
          s = biter.status();
          break;
        }

//对每个条目/块调用*saver函数，直到返回false
        for (biter.Seek(key); biter.Valid(); biter.Next()) {
          ParsedInternalKey parsed_key;
          if (!ParseInternalKey(biter.key(), &parsed_key)) {
            s = Status::Corruption(Slice());
          }

          if (!get_context->SaveValue(parsed_key, biter.value(), &biter)) {
            done = true;
            break;
          }
        }
        s = biter.status();
      }
      if (done) {
//避免额外的下一个，这在两级索引中是昂贵的
        break;
      }
    }
    if (s.ok()) {
      s = iiter->status();
    }
  }

//如果rep_u->filter_entry没有设置，我们应该调用release（）；否则
//不要调用，在这种情况下，我们在rep_u->filter_条目中有一个本地副本，
//它已固定到缓存中，将在析构函数中释放
  if (!rep_->filter_entry.IsSet()) {
    filter_entry.Release(rep_->table_options.block_cache.get());
  }
  return s;
}

Status BlockBasedTable::Prefetch(const Slice* const begin,
                                 const Slice* const end) {
  auto& comparator = rep_->internal_comparator;
//先决条件
  if (begin && end && comparator.Compare(*begin, *end) > 0) {
    return Status::InvalidArgument(*begin, *end);
  }

  BlockIter iiter_on_stack;
  auto iiter = NewIndexIterator(ReadOptions(), &iiter_on_stack);
  std::unique_ptr<InternalIterator> iiter_unique_ptr;
  if (iiter != &iiter_on_stack) {
    iiter_unique_ptr = std::unique_ptr<InternalIterator>(iiter);
  }

  if (!iiter->status().ok()) {
//打开索引迭代器时出错
    return iiter->status();
  }

//指示是否在需要预取的最后一页上
  bool prefetching_boundary_page = false;

  for (begin ? iiter->Seek(*begin) : iiter->SeekToFirst(); iiter->Valid();
       iiter->Next()) {
    Slice block_handle = iiter->value();

    if (end && comparator.Compare(iiter->key(), *end) >= 0) {
      if (prefetching_boundary_page) {
        break;
      }

//索引项表示数据块中的最后一个键。
//我们也应该将这个页面加载到内存中，但不能再加载了
      prefetching_boundary_page = true;
    }

//将块句柄指定的块加载到块缓存中
    BlockIter biter;
    NewDataBlockIterator(rep_, ReadOptions(), block_handle, &biter);

    if (!biter.status().ok()) {
//预提取时发生意外错误
      return biter.status();
    }
  }

  return Status::OK();
}

Status BlockBasedTable::VerifyChecksum() {
  Status s;
//检查元块
  std::unique_ptr<Block> meta;
  std::unique_ptr<InternalIterator> meta_iter;
  /*readmetablock（rep_uuu，nullptr/*预取缓冲区*/，&meta，&meta_iter）；
  如果（S.O.（））{
    s=verifychecksuminBlocks（meta_iter.get（））；
    如果（！）S.O.（））{
      返回S；
    }
  }否则{
    返回S；
  }
  //检查数据块
  积木上的积木；
  internalIterator*iiter=newIndexIterator（readOptions（），&iiter_on_stack）；
  std:：unique_ptr<internaliterator>iiter_unique_ptr；
  如果（ITER）！=&iiter_on_stack）
    iiter_unique_ptr=std:：unique_ptr<internaliterator>（iiter）；
  }
  如果（！）iiter->status（）.ok（））
    //打开索引迭代器时出错
    返回iiter->status（）；
  }
  S=验证校验和输入锁（IIter）；
  返回S；
}

状态块基表：：VerifyChecksuminBlocks（InternalIterator*Index_
  状态S；
  for（index iter->seektofirst（）；index iter->valid（）；index iter->next（））
    s=索引->status（）；
    如果（！）S.O.（））{
      断裂；
    }
    块状手柄；
    slice input=index_iter->value（）；
    s=handle.decodeFrom（&input）；
    如果（！）S.O.（））{
      断裂；
    }
    块内容物内容；
    s=readBlockContents（rep_->file.get（），nullptr/*预取缓冲区*/,

                          rep_->footer, ReadOptions(), handle, &contents,
                          /*_->ioptions，false/*解压缩*/，
                          slice（）/*压缩dic*/,

                          rep_->persistent_cache_options);
    if (!s.ok()) {
      break;
    }
  }
  return s;
}

bool BlockBasedTable::TEST_KeyInCache(const ReadOptions& options,
                                      const Slice& key) {
  std::unique_ptr<InternalIterator> iiter(NewIndexIterator(options));
  iiter->Seek(key);
  assert(iiter->Valid());
  CachableEntry<Block> block;

  BlockHandle handle;
  Slice input = iiter->value();
  Status s = handle.DecodeFrom(&input);
  assert(s.ok());
  Cache* block_cache = rep_->table_options.block_cache.get();
  assert(block_cache != nullptr);

  char cache_key_storage[kMaxCacheKeyPrefixSize + kMaxVarint64Length];
  Slice cache_key =
      GetCacheKey(rep_->cache_key_prefix, rep_->cache_key_prefix_size,
                  handle, cache_key_storage);
  Slice ckey;

  s = GetDataBlockFromCache(
      cache_key, ckey, block_cache, nullptr, rep_->ioptions, options, &block,
      rep_->table_options.format_version,
      rep_->compression_dict_block ? rep_->compression_dict_block->data
                                   : Slice(),
      /**读取每个位的字节数*/）；
  断言（S.O.（））；
  bool in_cache=block.value！= null pTR；
  In（InCype）{
    releaseCachedentry（block_cache，block.cache_handle）；
  }
  在缓存中返回；
}

//要求：rep_u的以下字段应该已经填充：
/ / 1。文件
/ / 2。索引句柄，
/ / 3。选项
/ / 4。内部比较器
/ / 5。索引类型
状态块基表：：CreateIndexReader（
    文件预取缓存*预取缓存，索引读取器**索引读取器，
    InternalIterator*预加载的_meta_index_iter，int level）
  //某些旧版本的基于块的表中不存在索引类型
  //表属性。如果是这样的话，我们可以安全地使用kbinarySearch。
  在文件=blockBasedTableOptions:：kbinarySearch上自动索引_type_；
  if（rep_->table_属性）
    auto&props=rep_->table_properties->user_collected_properties；
    auto pos=props.find（blockBasedTablePropertyNames:：kindExtype）；
    如果（POS）！=props.end（））
      index_type_on_file=static_cast<blockbasedTableOptions:：indexType>（）
          decodeFixed32（pos->second.c_str（））；
    }
  }

  auto file=rep_->file.get（）；
  const internalkeyparator*icomparator=&rep_->internal_comparator；
  const footer&footer=rep_->footer；
  if（index_type_on_file==blockbasedTableOptions:：khashsearch&&
      rep_->ioptons.prefix_extractor==nullptr）
    rocks日志警告（rep->ioptions.info日志，
                   “BlockBasedTableOptions:：KhashSearch需要”
                   “要设置的options.prefix提取程序。”
                   “返回二进制搜索索引。”）；
    index_type_on_file=blockBasedTableOptions:：kbinarySearch；
  }

  切换（索引类型_on_file）
    case blockbasedTableOptions:：ktwolLevelIndexSearch:
      返回partitionindexreader:：create（
          this，file，prefetch_buffer，footer，footer.index_handle（），
          rep->ioptions，icomperator，index_reader，
          rep_u->persistent_cache_选项，级别）；
    }
    case blockbasedTableOptions:：kbinarySearch:
      返回binarysearchindexreader:：create（
          文件，预取缓冲区，页脚，footer.index_handle（），rep_->ioptions，
          icomperator，index_reader，rep_u->persistent_cache_选项）；
    }
    基于事例块的表选项：：khashsearch:
      std:：unique_ptr<block>meta_guard；
      std:：unique_ptr<internaliterator>meta_iter_guard；
      auto meta_index_iter=预加载的_meta_index_iter；
      if（meta_index_iter==nullptr）
        汽车S =
            readmetablock（rep_uu，prefetch_buffer，&meta_guard，&meta_iter_guard）；
        如果（！）S.O.（））{
          //我们只需返回到二进制搜索，以防
          //前缀哈希索引加载有问题。
          rocks日志警告（rep->ioptions.info日志，
                         “无法读取元索引块。”
                         “返回二进制搜索索引。”）；
          返回binarysearchindexreader:：create（
              文件，预取缓冲区，页脚，页脚.index_handle（），
              rep->ioptions，icomperator，index_reader，
              rep_u->persistent_cache_选项）；
        }
        meta_index_iter=meta_iter_guard.get（）；
      }

      返回hashindexreader:：create（
          rep_->internal_prefix_transform.get（），footer，file，prefetch_buffer，
          rep->ioptions，icomperator，footer.index_handle（），meta_index_iter，
          index_reader，rep_u->hash_index_allow_collision，
          rep_u->persistent_cache_选项）；
    }
    默认值：{
      std：：字符串错误\u消息=
          “无法识别的索引类型：”+ToString（索引类型在文件上）；
      返回状态：：invalidArgument（error_message.c_str（））；
    }
  }
}

uint64_t blockbasedTable:：approxiteoffsetof（const slice&key）
  unique_ptr<internalIterator>index_iter（newIndexIterator（readOptions（））；

  索引器->搜索（键）；
  uint64结果；
  if（index-iter->valid（））
    块状手柄；
    slice input=index_iter->value（）；
    状态s=handle.decodefrom（&input）；
    如果（S.O.（））{
      结果=handle.offset（）；
    }否则{
      //奇怪：我们无法解码索引块中的块句柄。
      //我们只返回元索引块的偏移量，即
      //接近此案例的整个文件大小。
      result=rep_->footer.metaindex_handle（）.offset（）；
    }
  }否则{
    //键超过了文件中的最后一个键。如果表属性不是
    //可用，通过返回
    //metaindex块（就在文件末尾附近）。
    结果＝0；
    if（rep_->table_属性）
      结果=rep_->table_properties->data_size；
    }
    //表中不存在表属性。
    如果（结果==0）
      result=rep_->footer.metaindex_handle（）.offset（）；
    }
  }
  返回结果；
}

bool blockbasedtable:：test_filter_block_preloaded（）const_
  返回rep->filter！= null pTR；
}

bool blockbasedtable:：test_index_reader_preloaded（）const_
  返回rep_u->index_reader！= null pTR；
}

状态块基表：：GetKvPairsFromDataBlocks（
    std:：vector<kvpairblock>*kv_pair_blocks）
  std:：unique_ptr<internalIterator>blockHandles_iter（
      newIndexIterator（readOptions（））；

  status s=blockHandles_iter->status（）；
  如果（！）S.O.（））{
    //无法读取索引块
    返回S；
  }

  for（blockhandles_iter->seektofirst（）；blockhandles_iter->valid（）；
       blockHandles_iter->next（））
    s=blockHandles_iter->status（）；

    如果（！）S.O.（））{
      断裂；
    }

    std:：unique_ptr<internaliterator>datablock_iter；
    数据块重置（
        newDataBlockIterator（rep_u，readoptions（），blockHandles_Iter->value（））；
    s=datablock_iter->status（）；

    如果（！）S.O.（））{
      //读取块时出错-跳过
      继续；
    }

    kv pair block kv_对_块；
    for（数据块iter->seektofirst（）；数据块iter->valid（）；
         数据块->next（））
      s=datablock_iter->status（）；
      如果（！）S.O.（））{
        //读取块时出错-跳过
        断裂；
      }
      const slice&key=datablock_iter->key（）；
      const slice&value=datablock_iter->value（）；
      std:：string key_copy=std:：string（key.data（），key.size（））；
      std:：string value_copy=std:：string（value.data（），value.size（））；

      千伏对块。向后推（
          std:：make_pair（std:：move（key_copy），std:：move（value_copy））；
    }
    kv_对_块->向后推_（std:：move（kv_对_块））；
  }
  返回状态：：OK（）；
}

状态块基表：：转储表（可写文件*输出文件）
  //输出页脚
  out_file->append（
      “页脚详细信息：\n”
      “—————————————————————————————————————n”
      “”；
  out_file->append（rep_->footer.toString（）.c_str（））；
  out_file->append（“\n”）；

  //输出元索引
  out_file->append（
      “元索引详细信息：\n”
      “———————————————————————————————n”）；
  std:：unique_ptr<block>meta；
  std:：unique_ptr<internaliterator>meta_iter；
  状态S＝
      readmetablock（rep_uu，nullptr/*预取缓冲区*/, &meta, &meta_iter);

  if (s.ok()) {
    for (meta_iter->SeekToFirst(); meta_iter->Valid(); meta_iter->Next()) {
      s = meta_iter->status();
      if (!s.ok()) {
        return s;
      }
      if (meta_iter->key() == rocksdb::kPropertiesBlock) {
        out_file->Append("  Properties block handle: ");
        out_file->Append(meta_iter->value().ToString(true).c_str());
        out_file->Append("\n");
      } else if (meta_iter->key() == rocksdb::kCompressionDictBlock) {
        out_file->Append("  Compression dictionary block handle: ");
        out_file->Append(meta_iter->value().ToString(true).c_str());
        out_file->Append("\n");
      } else if (strstr(meta_iter->key().ToString().c_str(),
                        "filter.rocksdb.") != nullptr) {
        out_file->Append("  Filter block handle: ");
        out_file->Append(meta_iter->value().ToString(true).c_str());
        out_file->Append("\n");
      } else if (meta_iter->key() == rocksdb::kRangeDelBlock) {
        out_file->Append("  Range deletion block handle: ");
        out_file->Append(meta_iter->value().ToString(true).c_str());
        out_file->Append("\n");
      }
    }
    out_file->Append("\n");
  } else {
    return s;
  }

//输出表属性
  const rocksdb::TableProperties* table_properties;
  table_properties = rep_->table_properties.get();

  if (table_properties != nullptr) {
    out_file->Append(
        "Table Properties:\n"
        "--------------------------------------\n"
        "  ");
    out_file->Append(table_properties->ToString("\n  ", ": ").c_str());
    out_file->Append("\n");
  }

//输出滤波器块
  if (!rep_->filter && !table_properties->filter_policy_name.empty()) {
//现在只支持Bloomfilter
    rocksdb::BlockBasedTableOptions table_options;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(1));
    if (table_properties->filter_policy_name.compare(
            table_options.filter_policy->Name()) == 0) {
      std::string filter_block_key = kFilterBlockPrefix;
      filter_block_key.append(table_properties->filter_policy_name);
      BlockHandle handle;
      if (FindMetaBlock(meta_iter.get(), filter_block_key, &handle).ok()) {
        BlockContents block;
        /*（readBlockContents（rep_->file.get（），nullptr/*预取缓冲区*/，
                              rep_->footer、readoptions（）、handle和block，
                              rep->ioptions，false/*反编译*/,

                              /*ce（）/*压缩dict*/，
                              rep_u->persistent_cache_选项）
                OK（））{
          rep_->filter.reset（新建blockbasedfilterblockreader（
              rep_u->ioptions.prefix_提取器，table_选项，
              表_options.whole_key_filtering，std:：move（block）
              rep_u->ioptions.statistics））；
        }
      }
    }
  }
  if（rep_->filter）
    out_file->append（
        “筛选器详细信息：\n”
        “—————————————————————————————————————n”
        “”；
    out_file->append（rep_->filter->toString（）.c_str（））；
    out_file->append（“\n”）；
  }

  //输出索引块
  s=dumpindexblock（out_文件）；
  如果（！）S.O.（））{
    返回S；
  }

  //输出压缩字典
  如果（rep_u->compression_dict_block！= null pTr）{
    auto compression_dict=rep_u->compression_dict_block->data；
    out_file->append（
        “压缩字典：\n”
        “———————————————————————————————n”）；
    out_file->append（“大小（字节）：”）；
    out_file->append（rocksdb:：toString（compression_dict.size（））；
    out_file->append（“\n\n”）；
    out_file->append（“hex”）；
    out_file->append（compression_dict.toString（true.c_str（））；
    out_file->append（“\n\n”）；
  }

  //输出范围删除块
  auto*range_del_iter=newRangeTombstoneIterator（readOptions（））；
  如果（测距仪！= null pTr）{
    范围del_iter->seektofirst（）；
    if（range瘮del瘮iter->valid（））
      out_file->append（
          “删除范围：\n”
          “—————————————————————————————————————n”
          “”；
      for（；range眔del眔iter->valid（）；range眔del眔iter->next（））
        dumpkeyValue（range del_iter->key（），range del_iter->value（），out_file）；
      }
      out_file->append（“\n”）；
    }
    删除范围
  }
  //输出数据块
  S=转储数据块（out_文件）；

  返回S；
}

void blockBasedTable:：close（）
  如果（rep_u->closed）
    返回；
  }
  rep_->filter_entry.release（rep_->table_options.block_cache.get（））；
  rep_->index_entry.release（rep_->table_options.block_cache.get（））；
  rep_->range_del_entry.release（rep_->table_options.block_cache.get（））；
  //清除索引和筛选块以避免访问悬空指针
  如果（！）rep_->table_options.no_block_cache）
    char cache_key[kmaxcachekeyprefixsize+kmaxvarint64length]；
    //获取filter block键
    auto key=getcachekey（rep_->cache_key_prefix，rep_->cache_key_prefix_size，
                           rep_u->filter_handle，cache_key）；
    rep_->table_options.block_cache.get（）->erase（key）；
    //获取索引块键
    key=getCacheKeyFromOffset（rep_->cache_key_前缀，
                                rep_u->cache_key_prefix_大小，
                                rep->dummy_index_reader_offset，cache_key）；
    rep_->table_options.block_cache.get（）->erase（key）；
  }
  rep_->closed=true；
}

状态块BasedTable:：DumpIndexBlock（可写入文件*输出文件）
  out_file->append（
      “索引详细信息：\n”
      “———————————————————————————————n”）；

  std:：unique_ptr<internalIterator>blockHandles_iter（
      newIndexIterator（readOptions（））；
  status s=blockHandles_iter->status（）；
  如果（！）S.O.（））{
    out_file->append（“无法读取索引块\n\n”）；
    返回S；
  }

  out_file->append（“block key hex dump:数据块句柄\n”）；
  out_file->append（“block key ascii \n \n”）；
  for（blockhandles_iter->seektofirst（）；blockhandles_iter->valid（）；
       blockHandles_iter->next（））
    s=blockHandles_iter->status（）；
    如果（！）S.O.（））{
      断裂；
    }
    slice key=blockhandles_iter->key（）；
    Internalkey伊基；
    ikey.decodefrom（键）；

    out_file->append（“hex”）；
    out_file->append（ikey.user_key（）.toString（true）.c_str（））；
    out_file->append（“：”）；
    out_file->append（blockHandles_iter->value（）.toString（true）.c_str（））；
    out_file->append（“\n”）；

    std:：string str_key=ikey.user_key（）.toString（）；
    std:：string res_key（“”）；
    char cspace=''；
    对于（size_t i=0；i<str_key.size（）；i++）
      res_key.append（&str_key[i]，1）；
      附加资源键（1，cspace）；
    }
    out_file->append（“ascii”）；
    out_file->append（res_key.c_str（））；
    out_file->append（“\n------\n”）；
  }
  out_file->append（“\n”）；
  返回状态：：OK（）；
}

状态块基表：：转储数据块（可写入文件*输出文件）
  std:：unique_ptr<internalIterator>blockHandles_iter（
      newIndexIterator（readOptions（））；
  status s=blockHandles_iter->status（）；
  如果（！）S.O.（））{
    out_file->append（“无法读取索引块\n\n”）；
    返回S；
  }

  uint64_t datablock_size_min=std:：numeric_limits<uint64_t>：：max（）；
  uint64_t数据块_size_max=0；
  uint64数据块大小总和=0；

  尺寸块ID=1；
  for（blockhandles_iter->seektofirst（）；blockhandles_iter->valid（）；
       block_id++，blockhandles_iter->next（））
    s=blockHandles_iter->status（）；
    如果（！）S.O.（））{
      断裂；
    }

    slice bh_val=blockHandles_iter->value（）；
    BlockHandle bh；
    bh.decodeFrom（&bh_val）；
    uint64_t datablock_size=bh.size（）；
    数据块_-size_-min=std:：min（数据块_-size_-min，数据块_-size）；
    数据块_-size_-max=std:：max（数据块_-size_-max，数据块_-size）；
    数据块大小_sum+=数据块大小；

    out_file->append（“数据块”）；
    out_file->append（rocksdb:：toString（block_id））；
    out_file->append（“@”）；
    out_file->append（blockHandles_iter->value（）.toString（true）.c_str（））；
    out_file->append（“\n”）；
    out_file->append（“-------------------------------------\n”）；

    std:：unique_ptr<internaliterator>datablock_iter；
    数据块重置（
        newDataBlockIterator（rep_u，readoptions（），blockHandles_Iter->value（））；
    s=datablock_iter->status（）；

    如果（！）S.O.（））{
      out_file->append（“读取块时出错-跳过\n\n”）；
      继续；
    }

    for（数据块iter->seektofirst（）；数据块iter->valid（）；
         数据块->next（））
      s=datablock_iter->status（）；
      如果（！）S.O.（））{
        out_file->append（“读取块时出错-跳过\n”）；
        断裂；
      }
      dumpkeyValue（数据块iter->key（），数据块iter->value（），out_文件）；
    }
    out_file->append（“\n”）；
  }

  uint64_t num_datablocks=block_id-1；
  if（num_数据块）
    双数据块\大小\平均值=
        static_cast<double>（datablock_size_sum）/num_datablocks；
    out_file->append（“数据块摘要：\n”）；
    out_file->append（“---------------------------------------------”）；
    out_file->append（“\n数据块：”）；
    out_file->append（rocksdb:：toString（num_datablocks））；
    out_file->append（“\n最小数据块大小：”）；
    out_file->append（rocksdb:：toString（datablock_size_min））；
    out_file->append（“\n最大数据块大小：”）；
    out_file->append（rocksdb:：toString（datablock_size_max））；
    out_file->append（“\n avg data块大小：”）；
    out_file->append（rocksdb:：toString（datablock_size_avg））；
    out_file->append（“\n”）；
  }

  返回状态：：OK（）；
}

void blockbasedtable:：dumpkeyValue（const slice&key，const slice&value，
                                   可写文件*输出文件）
  Internalkey伊基；
  ikey.decodefrom（键）；

  out_file->append（“hex”）；
  out_file->append（ikey.user_key（）.toString（true）.c_str（））；
  out_file->append（“：”）；
  out_file->append（value.toString（true.c_str（））；
  out_file->append（“\n”）；

  std:：string str_key=ikey.user_key（）.toString（）；
  std:：string str_value=value.toString（）；
  std:：string res_key（“”），res_value（“”）；
  char cspace=''；
  对于（size_t i=0；i<str_key.size（）；i++）
    res_key.append（&str_key[i]，1）；
    附加资源键（1，cspace）；
  }
  对于（size_t i=0；i<str_value.size（）；i++）
    res_value.append（&str_value[i]，1）；
    附加值（1，cspace）；
  }

  out_file->append（“ascii”）；
  out_file->append（res_key.c_str（））；
  out_file->append（“：”）；
  out_file->append（res_value.c_str（））；
  out_file->append（“\n------\n”）；
}

命名空间{

void deletecachedfilterentry（const slice&key，void*值）
  filterblockreader*filter=reinterpret_cast<filterblockreader*>（value）；
  如果（filter->statistics（）！= null pTr）{
    记录标记（filter->statistics（），block_cache_filter_bytes_evict，
               filter->size（））；
  }
  删除过滤器；
}

void DeleteCachedIndexEntry（常量切片和键，void*值）
  index reader*index_reader=reinterpret_cast<index reader*>（value）；
  如果（index\u reader->statistics（）！= null pTr）{
    recordtick（index_reader->statistics（），block_cache_index_bytes\u evict，
               index_reader->usable_size（））；
  }
  删除索引阅读器；
}

//匿名命名空间

//命名空间rocksdb
