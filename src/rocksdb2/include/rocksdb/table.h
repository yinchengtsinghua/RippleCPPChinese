
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
//
//目前我们支持两种类型的表：普通表和基于块的表。
//1。基于块的表：这是继承自的默认表类型
//用于存储硬盘或闪存中的数据的LevelDB
//装置。
//2。普通表：是rocksdb的sst文件格式优化之一
//用于纯内存或低延迟介质上的低查询延迟。
//
//RockSDB表格格式教程如下：
//https://github.com/facebook/rocksdb/wiki/a-tutorial-of-rocksdb-sst-格式
//
//示例代码也可用
//https://github.com/facebook/rocksdb/wiki/a-tutorial-of-rocksdb-sst-formats wiki示例

#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "rocksdb/cache.h"
#include "rocksdb/env.h"
#include "rocksdb/iterator.h"
#include "rocksdb/options.h"
#include "rocksdb/status.h"

namespace rocksdb {

//--基于块的表
class FlushBlockPolicyFactory;
class PersistentCache;
class RandomAccessFile;
struct TableReaderOptions;
struct TableBuilderOptions;
class TableBuilder;
class TableReader;
class WritableFileWriter;
struct EnvOptions;
struct Options;

using std::unique_ptr;

enum ChecksumType : char {
  kNoChecksum = 0x0,
  kCRC32c = 0x1,
  kxxHash = 0x2,
};

//仅限高级用户
struct BlockBasedTableOptions {
//@flush_block_policy_factory创建flush block policy的实例。
//它提供了一种可配置的方法来确定何时刷新块
//基于块的表。如果未设置，则表生成器将使用默认值
//块刷新策略，按块大小剪切块（请参阅
//'flushblockbysizepolicy`）。
  std::shared_ptr<FlushBlockPolicyFactory> flush_block_policy_factory;

//todo（kailiu）通过设置默认值临时禁用此功能
//是假的。
//
//指示是否将索引/筛选器块放入块缓存。
//如果未指定，则每个“表读取器”对象将预加载索引/筛选器
//在表初始化期间阻塞。
  bool cache_index_and_filter_blocks = false;

//如果启用了缓存索引和筛选块，则缓存索引和筛选
//具有高优先级的块。如果设置为true，则取决于
//块缓存、索引和筛选块可能不太可能被收回
//而不是数据块。
  bool cache_index_and_filter_blocks_with_high_priority = false;

//如果cache_index_and_filter_块为真且下面为真，则
//筛选器和索引块存储在缓存中，但引用是
//保留在“Table Reader”对象中，以便固定块，并且仅
//释放表读取器时从缓存中收回。
  bool pin_l0_filter_and_index_blocks_in_cache = false;

//将用于此表的索引类型。
  enum IndexType : char {
//一个节省空间的索引块，它针对
//基于二进制搜索的索引。
    kBinarySearch,

//如果启用哈希索引，则当
//提供了“options.prefix”提取器。
    kHashSearch,

//TODO（Myabandeh）：此功能处于实验阶段，不应
//在生产中使用；如果
//可用于生产。
//两级索引实现。这两个级别都是二进制搜索索引。
    kTwoLevelIndexSearch,
  };

  IndexType index_type = kBinarySearch;

//此选项现在已弃用。无论设定值是多少，
//它的行为就像hash_index_allow_collision=true。
  bool hash_index_allow_collision = true;

//使用指定的校验和类型。新创建的表文件将
//受此校验和类型保护。旧表文件仍然可读，
//即使它们有不同的校验和类型。
  ChecksumType checksum = kCRC32c;

//禁用块缓存。如果设置为真，
//那么就不应该使用块缓存，块缓存应该
//指向nullptr对象。
  bool no_block_cache = false;

//如果非空，则对块使用指定的缓存。
//如果为空，rocksdb将自动创建和使用8MB内部缓存。
  std::shared_ptr<Cache> block_cache = nullptr;

//如果非空，则对从设备读取的页使用指定的缓存
//如果为空，则不使用页缓存
  std::shared_ptr<PersistentCache> persistent_cache = nullptr;

//如果非空，则对压缩块使用指定的缓存。
//如果为空，rocksdb将不使用压缩块缓存。
  std::shared_ptr<Cache> block_cache_compressed = nullptr;

//每个块压缩的用户数据的近似大小。请注意
//此处指定的块大小对应于未压缩的数据。这个
//如果
//压缩已启用。此参数可以动态更改。
  size_t block_size = 4 * 1024;

//这用于在块到达配置的
//“块状大小”。如果当前块中的可用空间百分比小于
//而向块中添加新记录将
//超过配置的块大小，则此块将关闭，并且
//新记录将写入下一个块。
  int block_size_deviation = 10;

//键的增量编码的重新启动点之间的键数。
//此参数可以动态更改。大多数客户应该
//不要使用此参数。允许的最小值为1。任何更小
//值将被1自动覆盖。
  int block_restart_interval = 16;

//与块重新启动间隔相同，但用于索引块。
  int index_block_restart_interval = 1;

//分区元数据的块大小。当前应用于索引的时间
//使用ktwolvelindexsearch，并在使用分区筛选器时进行筛选。
//注意：因为在当前的实现中，过滤器和索引分区
//如果对齐，则在索引或筛选器时创建索引/筛选器块
//块大小达到指定的限制。
//注意：此限制当前仅应用于索引块；筛选器
//在索引块被剪切后立即剪切分区
//TODO（Myabandeh）：当过滤分区被切断时，删除上面的注释
//分别地
  uint64_t metadata_block_size = 4096;

//注意：当前此选项要求将ktwolvelindexsearch设置为
//好。
//TODO（Myabandeh）：解除限制后，删除上面的注释
//TODO（Myabandeh）：此功能处于实验阶段，不应
//在生产中使用；如果
//可用于生产。
//对每个sst文件使用分区的完全过滤器
  bool partition_filters = false;

//使用增量编码压缩块中的键。
//readoptions：：pin_数据要求禁用此选项。
//
//默认值：真
  bool use_delta_encoding = true;

//如果不是nullptr，请使用指定的筛选策略减少磁盘读取。
//许多应用程序将受益于传递
//这里是newbloomfilterpolicy（）。
  std::shared_ptr<const FilterPolicy> filter_policy = nullptr;

//如果为真，则将整个键放在筛选器中（不只是前缀）。
//这通常是正确的，以使GET高效。
  bool whole_key_filtering = true;

//验证解压缩压缩块是否返回输入。这个
//是我们用来检测压缩中的错误的验证模式
//算法。
  bool verify_compression = false;

//如果使用，我们将为加载到内存中的每个数据块创建位图
//字节的大小（（块大小/`read-amp-bytes/u-per-bit`）/8）。这个位图
//将用于计算实际读取块的百分比。
//
//当使用此功能时，tickers：：read \u estimate \u utility \u bytes和
//tickers:：read amp _total _read _字节可用于计算
//使用此公式读取放大
//（读取总读取字节数/读取估计有用字节数）
//
//值=>内存使用率（已加载块内存的百分比）
//1=>12.50%
//2=>06.25%
//4=>03.12%
//8=>01.56%
//16=>00.78%
//
//注意：这个数字必须是2的幂，否则将被清除
//为2的下一个最小幂，例如，值7将是
//按4处理，值19将按16处理。
//
//默认值：0（已禁用）
  uint32_t read_amp_bytes_per_bit = 0;

//我们目前有三个版本：
//0--此版本当前由RockSDB的所有版本编写
//违约。可以被真正老的rocksdb读取。不支持更改
//校验和（默认为CRC32）。
//1——可以被RockSDB从3.0开始的版本读取。支持非默认
//校验和，比如XXHASH。它是由Rocksdb在什么时候写的
//BlockBasedTableOptions：：校验和不是KCRC32C。（版本
//0以静默方式向上转换）
//2——从3.10开始，RockSDB的版本可以读取。改变我们的方式
//使用lz4、bzip2和zlib压缩对压缩块进行编码。如果你
//不要计划在3.10版之前运行rocksdb，您可能应该使用
//这个。
//此选项仅影响新写入的表。当读取退出表时，
//有关版本的信息从页脚读取。
  uint32_t format_version = 2;
};

//特定于基于块的表属性的表属性。
struct BlockBasedTablePropertyNames {
//此属性的值是一个固定的Int32数字。
  static const std::string kIndexType;
//值为“1”表示真，“0”表示假。
  static const std::string kWholeKeyFiltering;
//值为“1”表示真，“0”表示假。
  static const std::string kPrefixFiltering;
};

//创建基于块的默认表工厂。
extern TableFactory* NewBlockBasedTableFactory(
    const BlockBasedTableOptions& table_options = BlockBasedTableOptions());

#ifndef ROCKSDB_LITE

enum EncodingType : char {
//总是写完整的密钥而不使用任何特殊编码。
  kPlain,
//找机会为多行写一次相同的前缀。
//在某些情况下，当一个键跟在具有相同前缀的前一个键之后时，
//它不是写出完整的密钥，而是写出
//共享前缀以及其他字节，以保存一些字节。
//
//使用此选项时，要求用户使用相同的前缀
//提取程序，以确保从同一个键中提取相同的前缀。
//前缀提取程序的name（）值将存储在文件中。什么时候？
//重新打开文件时，给定的options.prefix提取器的名称将为
//与存储在文件中的前缀提取程序进行位比较。一个错误
//如果两者不匹配，将返回。
  kPrefix,
};

//特定于普通表属性的表属性。
struct PlainTablePropertyNames {
  static const std::string kEncodingType;
  static const std::string kBloomVersion;
  static const std::string kNumBloomBlocks;
};

const uint32_t kPlainTableVariableLength = 0;

struct PlainTableOptions {
//@user_key_len:plain表对固定大小的键进行了优化，可以
//通过用户密钥长度指定。或者，你可以通过
//`KPlainTableVariableLength`如果您的密钥有变量
//长度。
  uint32_t user_key_len = kPlainTableVariableLength;

//@bloom_bits_per_key：每个前缀用于bloom文件管理器的位数。
//您可以通过传递零来禁用它。
  int bloom_bits_per_key = 10;

//@hash_table_ratio:用于
//前缀哈希。
//hash_table_ratio=
//哈希表
  double hash_table_ratio = 0.75;

//@index_稀疏度：在每个前缀中，需要为创建一个索引记录
//每个哈希桶内用于二进制搜索的键数。
//对于编码类型kprefix，该值将在
//写入以确定重写完整文件的间隔
//关键。它也将被用作建议和满足
//如果可能的话。
  size_t index_sparseness = 16;

//@hurge_page_tlb_size:如果<=0，则从malloc分配散列索引和bloom。
//否则从大页面TLB。用户需要
//为分配它保留大量页面，例如：
//sysctl-w vm.nr_hugepages=20
//请参阅Linux文档/vm/hugetlbpage.txt
  size_t huge_page_tlb_size = 0;

//@encoding_type:如何对密钥进行编码。请参见上面的枚举编码类型
//选择。该值将决定如何对键进行编码
//写入新的sst文件时。此值将被存储
//在读取时将使用的sst文件中
//文件，用户可以选择
//重新打开数据库时使用不同的编码类型。档案带
//不同的编码类型可以在同一个数据库中共存，并且
//可以阅读。
  EncodingType encoding_type = kPlain;

//@full_scan_mode:读取整个文件的模式，一条记录一条记录读取，无需
//使用索引。
  bool full_scan_mode = false;

//@store_index_in_file:compute plain table index and bloom filter during
//建立文件并将其存储在文件中。阅读时
//文件，索引将被mmaped而不是重新计算。
  bool store_index_in_file = false;
};

//--纯前缀查找的普通表
//对于此工厂，您需要正确设置options.prefix_extrator以使其成为
//工作。查找将从键前缀的前缀哈希查找开始。里面
//找到哈希桶，执行二进制搜索以查找哈希冲突。最后，
//使用线性搜索。

extern TableFactory* NewPlainTableFactory(const PlainTableOptions& options =
                                              PlainTableOptions());

struct CuckooTablePropertyNames {
//用于填充空桶的键。
  static const std::string kEmptyKey;
//固定值长度。
  static const std::string kValueLength;
//布谷鸟散列中使用的散列函数数。
  static const std::string kNumHashFunc;
//它表示布谷鸟块中的桶数。给了一把钥匙和一把
//特殊的哈希函数，布谷块是一组连续的bucket，
//其中，起始bucket id由键上的哈希函数给出。万一
//如果在插入键时发生冲突，生成器将尝试插入
//在使用下一个哈希之前，键入布谷鸟块的其他位置
//功能。这减少了读操作期间的缓存丢失，以防
//碰撞。
  static const std::string kCuckooBlockSize;
//哈希表的大小。使用此数字计算哈希的模
//功能。实际存储桶数为kmaxhashtablesize+
//Kcuckooblocksize-1.最后一个kcuckooblocksize-1桶用于
//从哈希表的末尾容纳布谷鸟块，因为缓存友好
//实施。
  static const std::string kHashTableSize;
//指示文件中排序的键是否为内部键（如果为false）
//或仅用户密钥（如果为真）。
  static const std::string kIsLastLevel;
//指示是否对第一个哈希函数使用标识函数。
  static const std::string kIdentityAsFirstHash;
//指示是否使用模块或位并计算哈希值
  static const std::string kUseModuleHash;
//固定用户密钥长度
  static const std::string kUserKeyLength;
};

struct CuckooTableOptions {
//确定哈希表的利用率。较小值
//导致更大的哈希表和更少的冲突。
  double hash_table_ratio = 0.9;
//生成器用于确定要转到的深度的属性
//在以下情况下搜索替换元素的路径
//碰撞。请参见builder.makespaceforkey方法。较高的
//值导致更高效的哈希表的数量更少
//查找，但需要更多时间来构建。
  uint32_t max_search_depth = 100;
//如果插入时发生冲突，则生成器
//尝试插入下一个布谷鸟块大小
//跳到下一个布谷鸟杂烩前的位置
//功能。这使得查找更容易缓存，以防
//碰撞的
  uint32_t cuckoo_block_size = 5;
//如果启用此选项，用户密钥将被视为uint64_t及其值。
//直接用作哈希值。此选项更改生成器的行为。
//读卡器忽略此选项并根据表中指定的内容进行操作
//财产。
  bool identity_as_first_hash = false;
//如果此选项设置为true，则在哈希计算期间使用模块。
//这通常会以性能为代价产生更好的空间效率。
//如果此optino设置为false，表中的条目将被约束为
//和位的幂，用于计算散列，这在
//将军。
  bool use_module_hash = true;
};

//使用缓存友好布谷鸟散列的sst表格式布谷鸟表工厂
extern TableFactory* NewCuckooTableFactory(
    const CuckooTableOptions& table_options = CuckooTableOptions());

#endif  //摇滚乐

class RandomAccessFileReader;

//表工厂的基类。
class TableFactory {
 public:
  virtual ~TableFactory() {}

//表的类型。
//
//每当
//表格式实现更改。
//
//以“rocksdb.”开头的名称是保留的，不应使用。
//由此包的任何客户端。
  virtual const char* Name() const = 0;

//返回可以从指定文件中提取数据的表对象表
//在参数文件中。打电话的人有责任确保
//文件的格式正确。
//
//在三个位置调用newTableReader（）：
//（1）TableCache:：FindTable（）在表缓存未命中时调用函数
//并缓存返回的表对象。
//（2）sstfilereader（用于sst转储）打开表并转储表
//使用表的迭代器的内容。
//（3）dbimpl:：addfile（）调用此函数来读取
//它试图添加的sst文件
//
//表读卡器选项是一个表读卡器选项，其中包含
//打开表所需的参数和配置。
//文件是处理表文件的文件处理程序。
//文件大小是文件的物理文件大小。
//表阅读器是输出表阅读器。
  virtual Status NewTableReader(
      const TableReaderOptions& table_reader_options,
      unique_ptr<RandomAccessFileReader>&& file, uint64_t file_size,
      unique_ptr<TableReader>* table_reader,
      bool prefetch_index_and_filter_in_cache = true) const = 0;

//返回表生成器以写入此表类型的文件。
//
//它在几个地方被称为：
//（1）将memtable刷新到0级输出文件时，会创建一个表
//builder（在dbimpl:：writeLevel0Table（）中，通过调用buildTable（））
//（2）在压缩过程中，它得到生成器来写入压缩输出。
//dbimpl:：openCompactionOutputfile（）中的文件。
//（3）当从事务日志恢复时，它创建一个表生成器
//写入0级输出文件（在dbimpl:：writeLevel0Tableforrecovery中，
//通过调用buildTable（））
//（4）运行repairer时，它创建一个表生成器来将日志转换为
//sst文件（通过调用buildtable（）在repairer:：convertlogtotable（）中）
//
//可以从那里访问多个配置的，包括但不限于
//压缩选项。文件是可写文件的句柄。
//调用者有责任保持文件的打开和关闭
//关闭表生成器后。压缩类型是压缩类型
//在此表中使用。
  virtual TableBuilder* NewTableBuilder(
      const TableBuilderOptions& table_builder_options,
      uint32_t column_family_id, WritableFileWriter* file) const = 0;

//清除指定的db选项和columnfamilyOptions。
//
//如果函数找不到清理输入数据库选项的方法，
//将返回非OK状态。
  virtual Status SanitizeOptions(
      const DBOptions& db_opts,
      const ColumnFamilyOptions& cf_opts) const = 0;

//返回包含表配置的可打印格式的字符串。
//rocksdb在db open（）打印配置。
  virtual std::string GetPrintableTableOptions() const = 0;

  virtual Status GetOptionString(std::string* opt_string,
                                 const std::string& delimiter) const {
    return Status::NotSupported(
        "The table factory doesn't implement GetOptionString().");
  }

//返回由此使用的表选项的原始指针
//如果不支持此函数，则返回tablefactory或nullptr。
//由于返回值是原始指针，TableFactory拥有
//指针和调用方不应删除指针。
//
//在某些情况下，当
//任何打开的数据库都不会通过强制转换返回的指针来使用TableFactory。
//去正确的班级。例如，如果使用了BlockBasedTableFactory，
//然后可以将指针强制转换为BlockBasedTableOptions。
//
//请注意，更改基础TableFactory选项时，
//TableFactory当前正由任何未定义的打开数据库行为使用。
//开发人员应该使用db:：setoption（）来动态更改
//数据库打开时的选项。
  virtual void* GetOptions() { return nullptr; }

//返回支持删除范围
  virtual bool IsDeleteRangeSupported() const { return false; }
};

#ifndef ROCKSDB_LITE
//创建一个特殊的表工厂，该工厂可以打开受支持的
//表格式，基于SST文件内的设置。应该用来
//将数据库从一种表格式转换为另一种表格式。
//@table_factory_to_write:写入新文件时使用的表工厂。
//@block_-based_-table_-factory:要使用的基于块的表工厂。如果为空，则使用
//默认的一个。
//@plain_table_factory:plain table factory使用。如果为空，则使用默认值。
//@布谷鸟桌厂：布谷鸟桌厂使用。如果为空，则使用默认值。
extern TableFactory* NewAdaptiveTableFactory(
    std::shared_ptr<TableFactory> table_factory_to_write = nullptr,
    std::shared_ptr<TableFactory> block_based_table_factory = nullptr,
    std::shared_ptr<TableFactory> plain_table_factory = nullptr,
    std::shared_ptr<TableFactory> cuckoo_table_factory = nullptr);

#endif  //摇滚乐

}  //命名空间rocksdb
