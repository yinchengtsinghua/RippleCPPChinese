
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

#pragma once

#include <stdint.h>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "options/cf_options.h"
#include "rocksdb/options.h"
#include "rocksdb/persistent_cache.h"
#include "rocksdb/statistics.h"
#include "rocksdb/status.h"
#include "rocksdb/table.h"
#include "table/filter_block.h"
#include "table/format.h"
#include "table/persistent_cache_helper.h"
#include "table/table_properties_internal.h"
#include "table/table_reader.h"
#include "table/two_level_iterator.h"
#include "util/coding.h"
#include "util/file_reader_writer.h"

namespace rocksdb {

class Block;
class BlockIter;
class BlockHandle;
class Cache;
class FilterBlockReader;
class BlockBasedFilterBlockReader;
class FullFilterBlockReader;
class Footer;
class InternalKeyComparator;
class Iterator;
class RandomAccessFile;
class TableCache;
class TableReader;
class WritableFile;
struct BlockBasedTableOptions;
struct EnvOptions;
struct ReadOptions;
class GetContext;
class InternalIterator;

using std::unique_ptr;

typedef std::vector<std::pair<std::string, std::string>> KVPairBlock;

//表是从字符串到字符串的排序映射。表是
//不变的和持久的。可以从中安全地访问表
//多个线程没有外部同步。
class BlockBasedTable : public TableReader {
 public:
  static const std::string kFilterBlockPrefix;
  static const std::string kFullFilterBlockPrefix;
  static const std::string kPartitionedFilterBlockPrefix;
//用于标识块的缓存键的最长前缀。
//对于POSIX文件，唯一ID是三个变量。
  static const size_t kMaxCacheKeyPrefixSize = kMaxVarint64Length * 3 + 1;

//尝试打开以字节[0..file_size]存储的表
//并读取允许
//正在从表中检索数据。
//
//如果成功，返回OK，并将“*table\u reader”设置为新打开的
//表。当不再需要时，客户机应该删除“*表读卡器”。
//如果在初始化表时出错，请设置“*表读卡器”
//返回一个非OK状态。
//
//使用此表时，@param文件必须保持活动状态。
//@param prefetch_index_和_filter_可用于禁用
//预取
//在启动时索引和筛选块到块缓存中
//@param skip_filters禁止加载/访问筛选器块。重写
//在缓存中预取索引和过滤器
//被设置。
  static Status Open(const ImmutableCFOptions& ioptions,
                     const EnvOptions& env_options,
                     const BlockBasedTableOptions& table_options,
                     const InternalKeyComparator& internal_key_comparator,
                     unique_ptr<RandomAccessFileReader>&& file,
                     uint64_t file_size, unique_ptr<TableReader>* table_reader,
                     bool prefetch_index_and_filter_in_cache = true,
                     bool skip_filters = false, int level = -1);

  bool PrefixMayMatch(const Slice& internal_key);

//返回表内容的新迭代器。
//newIterator（）的结果最初无效（调用方必须
//在使用迭代器之前调用其中一个seek方法）。
//@param skip_filters禁止加载/访问筛选器块
  InternalIterator* NewIterator(
      const ReadOptions&, Arena* arena = nullptr,
      bool skip_filters = false) override;

  InternalIterator* NewRangeTombstoneIterator(
      const ReadOptions& read_options) override;

//@param skip_filters禁止加载/访问筛选器块
  Status Get(const ReadOptions& readOptions, const Slice& key,
             GetContext* get_context, bool skip_filters = false) override;

//预取与指定的密钥范围相对应的磁盘块
//（Kbegin，Kend）。如果发生以下情况，呼叫将返回错误状态：
//IO或迭代错误。
  Status Prefetch(const Slice* begin, const Slice* end) override;

//给定一个键，返回文件中的一个近似字节偏移量，其中
//该键的数据将开始（如果该键是
//存在于文件中）。返回值以文件形式表示
//字节，因此包括压缩基础数据等效果。
//例如，表中最后一个键的近似偏移量将
//接近文件长度。
  uint64_t ApproximateOffsetOf(const Slice& key) override;

//如果指定键的块在缓存中，则返回true。
//要求：键在此表中并且块缓存已启用
  bool TEST_KeyInCache(const ReadOptions& options, const Slice& key);

//设置压缩表。可能更改某些参数
//PosixFfDebug
  void SetupForCompaction() override;

  std::shared_ptr<const TableProperties> GetTableProperties() const override;

  size_t ApproximateMemoryUsage() const override;

//将sst文件转换为人类可读的格式
  Status DumpTable(WritableFile* out_file) override;

  Status VerifyChecksum() override;

  void Close() override;

  ~BlockBasedTable();

  bool TEST_filter_block_preloaded() const;
  bool TEST_index_reader_preloaded() const;

//indexreader是为索引提供功能的接口。
//访问。
  class IndexReader {
   public:
    explicit IndexReader(const InternalKeyComparator* icomparator,
                         Statistics* stats)
        : icomparator_(icomparator), statistics_(stats) {}

    virtual ~IndexReader() {}

//为索引访问创建迭代器。
//如果ITER为空，则在堆上创建一个新对象，被调用方将
//拥有所有权。如果传入了非空ITER，则将使用它，并且
//返回的值与ITER或新的堆上对象相同
//那个
//包装通过的ITER。在后一种情况下，返回值将指向
//到
//一个不同的对象，然后ITER和被调用方拥有
//返回的对象。
    virtual InternalIterator* NewIterator(BlockIter* iter = nullptr,
                                          bool total_order_seek = true) = 0;

//索引的大小。
    virtual size_t size() const = 0;
//索引块的内存使用情况
    virtual size_t usable_size() const = 0;
//返回统计指针
    virtual Statistics* statistics() const { return statistics_; }
//报告除
//记忆
//在块缓存中分配的。
    virtual size_t ApproximateMemoryUsage() const = 0;

    /*tual void cachedependencies（bool/*unused*/）

    //将此索引引用的所有块预取到缓冲区
    void prefetchblocks（fileprefetchbuffer*buf）；

   受保护的：
    常量InternalKeyComparator*IComparator_uu；

   私人：
    统计学*统计学
  }；

  静态切片getcachekey（const char*缓存关键字前缀，
                           大小缓存密钥前缀大小，
                           const blockhandle&handle，char*cache_key）；

  //从表中的数据块中检索所有键值对。
  //检索到的密钥是内部密钥。
  status getkvpairsfromdatablocks（std:：vector<kvpairblock>*kv_pair_blocks）；

  类BlockeEntryIteratorState；

  友元类分区索引阅读器；

 受保护的：
  模板<class tvalue>
  结构cachableentry；
  结构说明；
  RP*RePig；
  显式块基表（rep*rep）：rep_

 私人：
  朋友类mockedblockbasedtable；
  //输入iter：如果不是空值，则更新此值并将其作为迭代器返回
  静态InternalIterator*NewDataBlockIterator（rep*rep，const readoptions&ro，
                                                常量切片和索引值，
                                                blockiter*输入iter=nullptr，
                                                bool为_index=false）；
  静态InternalIterator*NewDataBlockIterator（rep*rep，const readoptions&ro，
                                                const blockhandle和block hanlde，
                                                blockiter*输入iter=nullptr，
                                                bool is_index=false，
                                                状态s=状态（））；
  //如果启用了块缓存（压缩或未压缩），则查找块
  //由（1）未压缩缓存，（2）压缩缓存中的句柄标识，以及
  //然后是（3）文件。如果找到，则将插入到已搜索的缓存中
  //未成功（例如，如果在文件中找到，将添加到未压缩和
  //压缩缓存（如果启用）。
  / /
  //@param block_entry值设置为未压缩的块（如果找到）。如果
  //在未压缩的块缓存中，还将缓存句柄设置为引用
  //块。
  静态状态可以加载数据块缓存（filePrefetchBuffer*预取缓冲区，
                                          rep*rep、const readoptions和ro，
                                          const blockhandle和handle，
                                          切片压缩
                                          cachableEntry<block>>*block_entry，
                                          bool为_index=false）；

  //对于以下两个函数：
  //如果'no_io==true`，我们将不会尝试从sst文件读取筛选器/索引
  //它们是否还没有出现在缓存中？
  cachableEntry<filterBlockReader>getfilter（
      fileprefetchbuffer*prefetch_buffer=nullptr，bool no_io=false）const；
  虚拟cachableEntry<filterBlockReader>getfilter（
      fileprefetchbuffer*预取缓冲区、const blockhandle&filter_k_句柄，
      const bool是一个过滤分区，bool no_io）const；

  //从索引读取器获取迭代器。
  //如果未设置输入寄存器，则返回new迭代器
  //如果设置了输入寄存器，则更新它并将其作为迭代器返回
  / /
  //注意：如果所有
  //满足以下条件：
  / / 1。我们启用了table_options.cache_index_和_filter_块。
  / / 2。块缓存中不存在索引。
  / / 3。我们不允许执行任何IO，即读取选项==
  //kblockcachetier
  InternalIterator*NewIndexIterator（
      const read options&read_options，blockiter*输入_iter=nullptr，
      cachableEntry<indexreader>*index_entry=nullptr）；

  //从块缓存读取块缓存（如果设置）：块缓存和
  //块缓存被压缩。
  //成功时，返回状态：：OK，并填充@block
  //指向块及其块句柄的指针。
  //@param compression_dict data用于预设压缩库的
  //字典。
  静态状态GetDataBlockFromCache（
      const slice&block_cache_key，const slice&compressed_block_cache_key，
      缓存*块缓存，缓存*块缓存压缩，
      常量不变的选项和IOPTIONS、常量读取选项和读取选项，
      blockBasedTable:：cachableEntry<block>>block，uint32_t format_version，
      const slice&compression_dict，每个位的大小读取字节数，
      bool为_index=false）；

  //将原始块（可能已压缩）放入相应的块缓存。
  //如果需要，此方法将对原始块执行解压缩，然后
  //填充块缓存。
  //成功时，将返回状态：：OK；同时@block将填充
  //未压缩的块及其缓存句柄。
  / /
  //需要：原始_块是堆分配的。PutDataBlockToCache（）将
  //负责在发生错误时释放内存。
  //@param compression_dict data用于预设压缩库的
  //字典。
  静态状态PutDataBlockToCache（
      const slice&block_cache_key，const slice&compressed_block_cache_key，
      缓存*块缓存，缓存*块缓存压缩，
      const read options&read_options，const immutable cfoptions&ioptions，，
      cachableEntry<block>*block，block*raw_block，uint32_t format_version，
      const slice&compression_dict，每个位的大小读取字节数，
      bool为_index=false，cache:：priority pri=cache:：priority:：low）；

  //重复调用（*handle_result）（arg，…），从找到的条目开始
  //调用seek（key）后，直到handle_result返回false。
  //如果筛选策略声明密钥不存在，则可能不会进行此类调用。
  友元类表缓存；
  友元类BlockBasedTableBuilder；

  void readmeta（const footer&footer）；

  //根据表中存储的索引类型创建索引读取器。
  //或者，用户可以为该索引传递一个预加载的元索引器，
  //需要访问额外的元块来构造索引。这个参数
  //如果调用方已经创建了元索引块，则有助于避免重新读取元索引块。
  状态createIndexReader（
      文件预取缓存*预取缓存，索引读取器**索引读取器，
      InternalIterator*预加载的_meta_index_iter=nullptr，
      常量int level=-1）；

  bool fullfilterkeymaymatch（const read options&read_选项，
                             filterblockreader*过滤器，常量切片和用户密钥，
                             const bool noou io）const；

  //从sst读取元块。
  静态状态readmetablock（rep*rep，fileprefetchbuffer*prefetch_buffer，
                              std:：unique_ptr<block>>*meta_block，
                              std:：unique_ptr<internaliterator>*iter）；

  状态验证校验和块（InternalIterator*索引器）；

  //从筛选器块创建筛选器。
  filterblockreader*readfilter（fileprefetchbuffer*预取缓冲区，
                                const blockhandle和filter_handle，
                                const bool是一个过滤器分区）const；

  静态void setupcachekeyprefix（rep*rep，uint64_t文件大小）；

  //从文件生成缓存键前缀
  静态void generateCachePrefix（缓存*cc，
    randomaccessfile*文件，char*缓冲区，大小t*大小）；
  静态void generateCachePrefix（缓存*cc，
    writablefile*文件，char*缓冲区，大小t*大小）；

  //dumptable（）的helper函数
  状态dumpindexblock（可写文件*输出文件）；
  状态转储数据块（可写文件*输出文件）；
  void dumpkeyvalue（const slice&key，const slice&value，
                    可写文件*输出文件）；

  //不允许复制
  显式BlockBasedTable（const tablereader&）=删除；
  void operator=（const tablereader&）=删除；

  友元类partitionedfilterblockreader；
  友元类partitionedfilterblocktest；
}；

//正在维护分区索引结构上的两级迭代的状态
类blockBasedTable:：blockEntryIteratorState:public twolevelIteratorState_
 公众：
  块入口畸胎病状态（
      blockbasedtable*表，const read options&read_选项，
      常量InternalKeyComparator*IComprator，bool skip_filters，
      bool is_index=false，
      std:：unordered_map<uint64_t，cachableEntry<block>>*block_map=nullptr）；
  InternalIterator*NewSecondaryIterator（const slice&index_value）override；
  bool prefixmaymatch（const slice&internal_key）override；
  bool keyreachedupperbound（const slice&internal_key）override；

 私人：
  //不拥有表\u
  BlockBasedTable*表格
  const read options读取选项
  常量InternalKeyComparator*IComparator_uu；
  bool skip_filters_uu；
  //如果第二级迭代器在索引上而不是在用户数据上，则为true。
  Boer-IS-指数法；
  std:：unordered_map<uint64_t，cachableentry<block>>*block_map_uu；
  端口：：rwmutex cleaner_mu；
}；

//cachableEntry表示*可以*从块缓存中提取的项。
//field`value`是要获取的项。
//field`cache_handle`是块缓存的缓存句柄。如果值
//未从缓存中读取，`cache_handle`将为nullptr。
模板<class tvalue>
结构块基表：：cachableEntry
  cachableEntry（tValue*值，cache:：handle*缓存\句柄）
      ：value（_value），cache_handle（_cache_handle）
  cachableEntry（）：cachableEntry（nullptr，nullptr）
  无效释放（cache*cache，bool force_erase=false）
    if（缓存句柄）
      cache->释放（cache_handle，force_erase）；
      值=nullptr；
      缓存句柄=nullptr；
    }
  }
  bool isset（）const返回缓存句柄！= null pTr；}

  tValue*值=nullptr；
  //如果条目来自缓存，则将填充缓存句柄。
  cache:：handle*缓存句柄=nullptr；
}；

结构块基表：：rep
  rep（const-immutablecfoptions和ioptions，const-env options和env options，
      Const BlockBasedTable选项和表格选项，
      const internalkeycomparator&_internal_comparator，bool skip_filters）
      ：IOPTIONS（_IOPTIONS）、
        环境选项（环境选项）
        表选项（表选项）
        过滤器策略（跳过过滤器？nullptr:_table_opt.filter_policy.get（）），
        内部_比较器（_内部_比较器），
        过滤器类型（过滤器类型：：knofilter）
        整\键\过滤（\u表\选项.整\键\过滤）
        前缀过滤（真）
        范围句柄（blockHandle:：nullBlockHandle（）），
        全局序号（kdisableglobalsequencenumber）

  常量不变的选项和IOPTIONS；
  const env选项&env_选项；
  const blockbasedtableoptions&table_options；
  const filter policy*const filter_policy；
  常量InternalKeyComparator&Internal_Comparator；
  现状；
  唯一的文件；
  char cache_key_prefix[kmaxcachekeyprefixsize]；
  大小缓存密钥前缀大小=0；
  char persistent_cache_key_prefix[kmaxcachekeyprefixsize]；
  大小\u t持久\u缓存\u密钥\u前缀\u大小=0；
  char compressed_cache_key_prefix[kmaxcachekeyprefixsize]；
  大小\u t压缩\u缓存\u密钥\u前缀\u大小=0；
  uint64_t虚拟_index_reader_offset=
      0；//块缓存唯一的ID。
  持久缓存选项持久缓存选项；

  //页脚包含固定表信息
  页脚页脚；
  //只有在以下情况下才会填充和使用索引读取器和筛选器：
  //options.block_cache为nullptr；否则我们将通过
  //块缓存。
  唯一的读卡器；
  唯一的_ptr<filterblockreader>filter；

  枚举类filtertype
    克诺菲尔特
    KFulfFig，
    KButter滤波器，
    KPartitionedFilter（分区过滤器）
  }；
  过滤器类型过滤器
  阻塞手柄过滤器手柄；

  std:：shared_ptr<const table properties>table_properties；
  //包含压缩字典数据的块。我们拥有所有权
  //对于整个块结构，即使我们只使用它的切片成员。这个
  //更容易，因为切片成员依赖于
  //另一个成员（“分配”）。
  std:：unique_ptr<const blockcontents>compression_dict_block；
  blockBasedTableOptions：：indexType索引_类型；
  bool hash_index_allow_collision；
  布尔整键过滤；
  bool前缀过滤；
  //todo（kailiu）在表中使用内部键非常难看，因为表
  //模块不应依赖于db模块。但是为了让事情更容易
  //并且与现有代码兼容，我们引入了一个包装器，
  //块提取前缀，而不知道键是否为内部键。
  unique_ptr<sliceTransform>internal_prefix_transform；

  //仅用于0级文件：
  //当pin_l0_filter_和_index_blocks_在_缓存中为真时，我们确实使用
  //lru缓存，但我们总是检查filter&idndex块的句柄
  //这里（=我们不调用release（））加上解析的out对象
  //LRU缓存永远不会将它们推出，因此它们被固定
  cachableEntry<filterBlockReader>filter_entry；
  cachableEntry<indexreader>index_entry；
  //当lru时，范围删除元块通过读卡器的生存期固定。
  //已启用缓存。
  cachableEntry<block>range_del_entry；
  blockhandle range？del？handle；

  //如果使用全局\seqno，则此文件中的所有键都将具有相同的
  //seqno，值为“global\seqno”。
  / /
  //kdisableglobalsequencenumber的值表示此功能被禁用
  //每个键都有自己的seqno。
  SequenceNumber全局编号；
  bool closed=false；
}；

//命名空间rocksdb
