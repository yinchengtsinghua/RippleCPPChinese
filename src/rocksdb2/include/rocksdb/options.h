
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
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#ifndef STORAGE_ROCKSDB_INCLUDE_OPTIONS_H_
#define STORAGE_ROCKSDB_INCLUDE_OPTIONS_H_

#include <stddef.h>
#include <stdint.h>
#include <string>
#include <memory>
#include <vector>
#include <limits>
#include <unordered_map>

#include "rocksdb/advanced_options.h"
#include "rocksdb/comparator.h"
#include "rocksdb/env.h"
#include "rocksdb/listener.h"
#include "rocksdb/universal_compaction.h"
#include "rocksdb/version.h"
#include "rocksdb/write_buffer_manager.h"

#ifdef max
#undef max
#endif

namespace rocksdb {

class Cache;
class CompactionFilter;
class CompactionFilterFactory;
class Comparator;
class Env;
enum InfoLogLevel : unsigned char;
class SstFileManager;
class FilterPolicy;
class Logger;
class MergeOperator;
class Snapshot;
class MemTableRepFactory;
class RateLimiter;
class Slice;
class Statistics;
class InternalKeyComparator;
class WalFilter;

//数据库内容存储在一组块中，每个块包含一个
//键、值对的序列。每个块可以在
//存储在文件中。以下枚举描述了
//压缩方法（如果有）用于压缩块。
enum CompressionType : unsigned char {
//注意：不要更改现有条目的值，因为它们是
//磁盘上持久格式的一部分。
  kNoCompression = 0x0,
  kSnappyCompression = 0x1,
  kZlibCompression = 0x2,
  kBZip2Compression = 0x3,
  kLZ4Compression = 0x4,
  kLZ4HCCompression = 0x5,
  kXpressCompression = 0x6,
  kZSTD = 0x7,

//如果必须使用早于
//0.8.0或考虑降级服务或复制的可能性
//数据库文件到另一个使用旧版本的
//没有kzsdd的rocksdb。否则，您应该使用kzsd。我们将
//最终从公共API中删除该选项。
  kZSTDNotFinalCompression = 0x40,

//kdisablecompressionoption用于禁用某些压缩选项。
  kDisableCompressionOption = 0xff,
};

struct Options;

struct ColumnFamilyOptions : public AdvancedColumnFamilyOptions {
//函数将选项恢复到以前的版本。只有4.6或更高版本
//支持版本。
  ColumnFamilyOptions* OldDefaults(int rocksdb_major_version = 4,
                                   int rocksdb_minor_version = 6);

//一些功能使优化rocksdb更容易
//如果你的数据库非常小（比如1GB以下），而你不想使用它
//为内存表花费大量的内存。
  ColumnFamilyOptions* OptimizeForSmallDb();

//如果您不需要对数据进行排序，也就是说，您将永远不会使用
//一个迭代器，只有put（）和get（）API调用
//
//RocksDB-Lite不支持
  ColumnFamilyOptions* OptimizeForPointLookup(
      uint64_t block_cache_size_mb);

//ColumnFamilyOptions中某些参数的默认值不是
//针对繁重的工作负载和大数据集进行了优化，这意味着您可能
//在某些情况下观察写入暂停。作为调优的起点
//RockSDB选项，使用以下两个功能：
//*optimizelStyleCompaction--优化级别样式压缩
//*优化通用样式压缩--优化通用样式压缩
//通用类型的压缩集中在减少写放大。
//大数据集的因素，但增加了空间放大。你可以学习
//关于不同款式的更多信息：
//https://github.com/facebook/rocksdb/wiki/rocksdb-architecture-guide网站
//确保还调用increaseParallelism（），它将提供
//最大的性能提升。
//注意：我们可能会在高内存时使用比memtable内存预算更多的内存。
//写入速率期间
//
//RocksDB-Lite不支持OptimizeUniversalStyleCompaction
  ColumnFamilyOptions* OptimizeLevelStyleCompaction(
      uint64_t memtable_memory_budget = 512 * 1024 * 1024);
  ColumnFamilyOptions* OptimizeUniversalStyleCompaction(
      uint64_t memtable_memory_budget = 512 * 1024 * 1024);

//------------
//影响行为的参数

//比较器用于定义表中键的顺序。
//默认值：使用字典式字节顺序的比较器
//
//要求：客户端必须确保提供的比较器
//这里有相同的名称和顺序键*完全*和
//Comparator提供给同一数据库上以前的打开调用。
  const Comparator* comparator = BytewiseComparator();

//要求：如果合并操作，客户端必须提供合并运算符
//需要访问。在没有合并运算符的数据库上调用合并
//将导致状态：：NotSupported。客户必须确保
//此处提供的合并运算符具有相同的名称和*完全*相同
//语义作为合并运算符提供给上一个打开调用
//相同的数据库。唯一的例外是为升级保留的，其中一个数据库
//以前没有合并运算符的情况下引入合并操作
//第一次。当
//在这种情况下打开数据库。
//默认值：nullptr
  std::shared_ptr<MergeOperator> merge_operator = nullptr;

//压缩期间要调用的单个CompactionFilter实例。
//允许应用程序在后台修改/删除键值
//压实作用。
//
//如果客户机要求为不同的
//压缩运行，它可以指定压缩过滤器工厂而不是这个工厂。
//选择权。客户机应该只指定其中的一个。
//如果
//客户端指定两者。
//
//如果正在使用多线程压缩，则提供的CompactionFilter
//实例可以同时从不同的线程使用，因此
//线程安全。
//
//默认值：nullptr
  const CompactionFilter* compaction_filter = nullptr;

//这是一个提供压缩过滤器对象的工厂，它允许
//在后台压缩期间修改/删除键值的应用程序。
//
//每次压缩运行时都将创建一个新的过滤器。如果多线程
//正在使用压缩，每个创建的压缩筛选器将仅使用
//从一个线程，所以不需要是线程安全的。
//
//默认值：nullptr
  std::shared_ptr<CompactionFilterFactory> compaction_filter_factory = nullptr;

//------------
//影响性能的参数

//要在内存中累积的数据量（由未排序的日志备份）
//在磁盘上）转换为按磁盘排序的文件之前。
//
//较大的值会提高性能，尤其是在大容量加载期间。
//存储器中最多可容纳写缓冲区个数写缓冲区个数
//同时，
//所以您可能希望调整这个参数来控制内存的使用。
//此外，较大的写缓冲区将导致较长的恢复时间
//下次打开数据库时。
//
//请注意，写缓冲区的大小是按列族强制执行的。
//有关跨列族共享内存的信息，请参见db_write_buffer_size。
//
//默认值：64 MB
//
//可通过setOptions（）API动态更改
  size_t write_buffer_size = 64 << 20;

//使用指定的压缩算法压缩块。这个
//参数可以动态更改。
//
//默认值：ksnappycompression，如果支持的话。如果未链接Snappy
//对于库，默认值为knocompression。
//
//在Intel（R）Core（TM）2 2.4GHz上ksnapycompression的典型速度：
//~200-500MB/s压缩
//~400-800MB/s解压
//请注意，这些速度明显快于大多数
//持久的存储速度，因此通常不会
//值得切换到knocompression。即使输入数据是
//不可压缩，ksnappycompression实现将
//有效检测并切换到未压缩模式。
  CompressionType compression;

//将用于最底层的压缩算法
//包含文件。如果使用级别压缩，此选项将仅影响
//基准面后的标高。
//
//默认值：kDisableCompressionOption（禁用）
  CompressionType bottommost_compression = kDisableCompressionOption;

//压缩算法的不同选项
  CompressionOptions compression_opts;

//要触发0级压缩的文件数。值<0表示
//0级压缩完全不会由文件数触发。
//
//默认值：4
//
//可通过setOptions（）API动态更改
  int level0_file_num_compaction_trigger = 4;

//如果非nullptr，则使用指定的函数确定
//键的前缀。这些前缀将放在过滤器中。
//根据工作负载的不同，这可以减少读取IOP的数量。
//当前缀通过readoptions传递给
//db.newIterator（）。为了使前缀过滤正常工作，
//“前缀提取器”和“比较器”必须满足以下要求
//属性保留：
//
//1）键。以（前缀（键））开头
//2）比较（前缀（键），键）<=0。
//3）如果比较（k1，k2）<=0，则比较（前缀（k1），前缀（k2））<=0
//4）前缀（prefix（key））==前缀（key）
//
//默认值：nullptr
  std::shared_ptr<const SliceTransform> prefix_extractor = nullptr;

//控制级别的最大总数据大小。
//“级别”基础的最大“字节数”是级别1的最大总数。
//L级的最大字节数可以计算为
//（最大\u字节\u用于\u级别\u基数）*（最大\u字节\u用于\u级别\u乘数^（l-1））。
//例如，如果“级别”库的最大字节数为200MB，并且如果
//级别乘数的最大字节数为10，级别1的总数据大小
//将为200MB，2级文件的总大小为2GB，
//三级的总文件大小为20GB。
//
//默认值：256MB。
//
//可通过setOptions（）API动态更改
  uint64_t max_bytes_for_level_base = 256 * 1048576;

//禁用自动压缩。手动压实仍然可以
//发布于此栏族
//
//可通过setOptions（）API动态更改
  bool disable_auto_compactions = false;

//这是一个提供TableFactory对象的工厂。
//默认值：提供默认值的基于块的表工厂
//使用默认值实现TableBuilder和TableReader
//基于块的表选项。
  std::shared_ptr<TableFactory> table_factory;

//使用所有字段的默认值创建ColumnFamilyOptions
  ColumnFamilyOptions();
//从选项创建ColumnFamilyOptions
  explicit ColumnFamilyOptions(const Options& options);

  void Dump(Logger* log) const;
};

enum class WALRecoveryMode : char {
//原水平回收
//我们容忍所有日志的尾随数据中存在不完整的记录。
//用例：这是遗留行为（默认）
  kTolerateCorruptedTailRecords = 0x00,
//从干净关机中恢复
//我们不希望在沃尔玛发现任何腐败
//用例：这非常适合于单元测试和很少的应用程序
//可能需要高一致性保证
  kAbsoluteConsistency = 0x01,
//恢复到时间点一致性
//当发现wal不一致时，我们停止wal回放
//用例：非常适合具有磁盘控制器缓存的系统
//硬盘、无超级电容存储相关数据的固态硬盘
  kPointInTimeRecovery = 0x02,
//灾后恢复
//我们忽视了沃尔玛的任何腐败，并试图挽救尽可能多的数据
//可能的
//用例：非常适合最后一次努力恢复那些
//使用低级无关数据操作
  kSkipAnyCorruptedRecords = 0x03,
};

struct DbPath {
  std::string path;
uint64_t target_size;  //路径下总文件的目标大小（字节）。

  DbPath() : target_size(0) {}
  DbPath(const std::string& p, uint64_t t) : path(p), target_size(t) {}
};


struct DBOptions {
//该函数将选项恢复为版本4.6中的选项。
  DBOptions* OldDefaults(int rocksdb_major_version = 4,
                         int rocksdb_minor_version = 6);

//一些功能使优化rocksdb更容易

//如果你的数据库非常小（比如1GB以下），而你不想使用它
//为内存表花费大量的内存。
  DBOptions* OptimizeForSmallDb();

#ifndef ROCKSDB_LITE
//默认情况下，rocksdb仅使用一个后台线程进行刷新和
//压实作用。调用此函数将设置为
//使用“线程总数”。“total_threads”的良好值是
//岩心。如果您的系统是
//受到RocksDB的瓶颈。
  DBOptions* IncreaseParallelism(int total_threads = 16);
#endif  //摇滚乐

//如果为true，则将在缺少数据库时创建该数据库。
//默认：假
  bool create_if_missing = false;

//如果为真，则将自动创建缺少的列族。
//默认：假
  bool create_missing_column_families = false;

//如果为true，则在数据库已存在时引发错误。
//默认：假
  bool error_if_exists = false;

//如果是，RockSDB将积极检查数据的一致性。
//此外，如果对数据库的任何写入操作失败（Put、Delete、Merge、
//写入），数据库将切换到只读模式并使所有其他模式失败
//写入操作。
//在大多数情况下，您希望将此设置为true。
//默认值：真
  bool paranoid_checks = true;

//使用指定的对象与环境交互，
//例如读/写文件、安排后台工作等。
//默认值：env:：default（））
  Env* env = Env::Default();

//用于控制刷新和压缩的写入速率。同花顺更高
//优先于压实。如果nullptr，则禁用速率限制。
//如果启用速率限制器，则默认情况下，“每同步字节数”设置为1MB。
//默认值：nullptr
  std::shared_ptr<RateLimiter> rate_limiter = nullptr;

//用于跟踪sst文件并控制其文件删除率。
//
//特征：
//-限制sst文件的删除率。
//-跟踪所有SST文件的总大小。
//-为达到时的SST文件设置最大允许空间限制
//数据库不会进行任何进一步的刷新或压缩，并将设置
//后台错误。
//-可在多个DBS之间共享。
//局限性：
//-仅跟踪和限制删除中的sst文件
//第一个db_路径（如果db_路径为空，则为db_名称）。
//
//默认值：nullptr
  std::shared_ptr<SstFileManager> sst_file_manager = nullptr;

//数据库生成的任何内部进度/错误信息将
//如果是非空指针，则写入信息日志，或写入存储的文件
//如果info_log为nullptr，则与db contents在同一目录中。
//默认值：nullptr
  std::shared_ptr<Logger> info_log = nullptr;

#ifdef NDEBUG
      InfoLogLevel info_log_level = INFO_LEVEL;
#else
      InfoLogLevel info_log_level = DEBUG_LEVEL;
#endif  //调试程序

//数据库可以使用的打开文件数。你可能需要
//如果数据库有较大的工作集，请增加此值。价值1意味着
//打开的文件始终保持打开状态。您可以根据
//目标文件的大小和基于级别的目标文件的大小乘数
//压实作用。对于通用样式的压缩，通常可以将其设置为-1。
//默认值：-1
  int max_open_files = -1;

//如果max_open_files为-1，db将打开db:：open（）上的所有文件。你可以
//使用此选项可增加用于打开文件的线程数。
//默认值：16
  int max_file_opening_threads = 16;

//一旦提前写入日志超过此大小，我们将开始强制刷新
//列族的memtable由最旧的live wal文件支持
//（即引起所有空间放大的那些）。如果设置为0
//（默认），我们将动态选择Wal大小限制为
//[所有写入缓冲区大小*最大写入缓冲区数量之和]*4
//默认值：0
  uint64_t max_total_wal_size = 0;

//如果非空，那么我们应该收集关于数据库操作的度量
  std::shared_ptr<Statistics> statistics = nullptr;

//如果为真，则每个存储到稳定存储的存储都将发出fsync。
//如果为false，则每个存储到稳定存储的存储都将发出fdata同步。
//在将数据存储到
//像ext3这样的文件系统，在重新启动后会丢失文件。
//默认：假
//注意：在许多平台上，fdatasync被定义为fsync，所以这个参数
//不会有什么不同。请参阅此代码库中的fdatasync定义。
  bool use_fsync = false;

//可以放入sst文件的路径列表及其目标大小。
//当
//较旧的数据逐渐移动到向量后面指定的路径。
//
//例如，您有一个为数据库分配了10GB的闪存设备，
//除了2tb的硬盘驱动器外，还应将其配置为：
//[“/flash_path”，10GB，“/hard_drive”，2tb]
//
//系统将尽力保证每条路径下的数据接近但
//不大于目标大小。但当前和未来使用的文件大小
//通过确定将文件放在哪里是基于最佳工作估计的，
//这意味着目录下的实际大小
//在某些工作负载下略大于目标大小。用户应该给出
//为这些案件提供缓冲空间。
//
//如果没有足够的空间放置文件，文件将
//不管目标大小如何，都要放在最后一条路径上。
//
//将新数据放置到早期路径也是最好的工作。用户应该
//在某些极端情况下，希望将用户文件放在更高的级别。
//
//如果保留为空，则只使用一个路径，即当
//打开数据库。
//默认：空
  std::vector<DbPath> db_paths;

//这指定信息日志目录。
//如果为空，则日志文件将与数据位于同一目录中。
//如果不为空，则日志文件将位于指定的目录中，
//数据库data dir的绝对路径将用作日志文件
//名字的前缀。
  std::string db_log_dir = "";

//这指定了预写日志（wal）的绝对目录路径。
//如果为空，日志文件将与数据在同一目录中，
//默认情况下，dbname用作data dir
//如果不为空，则日志文件将保留在指定的目录中。
//销毁数据库时，
//wal_dir和dir本身中的所有日志文件都将被删除。
  std::string wal_dir = "";

//删除过时文件的周期。默认值
//值为6小时。通过压缩而超出范围的文件
//进程在每次压缩时仍会自动删除，
//无论此设置如何
  uint64_t delete_obsolete_files_period_micros = 6ULL * 60 * 60 * 1000000;

//最大并发后台作业数（压缩和刷新）。
  int max_background_jobs = 2;

//不再支持：RockSDB根据
//最大后台作业的值。此选项被忽略。
  int base_background_compactions = -1;

//不再支持：RockSDB根据
//最大后台作业的值。为了向后兼容，我们将设置
//`max_background_jobs=max_background_compactions+max_background_flushes`
//如果用户至少设置了一个“max”background“压缩”或
//'max_background_flushes'（如果有一个选项未设置，我们将-1替换为1）。
//
//提交到的并发后台压缩作业的最大数目
//默认的低优先级线程池。
//
//如果要增加此值，还应考虑增加
//低优先级线程池。有关详细信息，请参阅
//env:：backgroundthreads
//默认值：-1
  int max_background_compactions = -1;

//此值表示将
//通过将压缩作业拆分为多个压缩作业来同时执行压缩作业，
//同时运行的较小的。
//默认值：1（即无子协议）
  uint32_t max_subcompactions = 1;

//不再支持：RockSDB根据
//最大后台作业的值。为了向后兼容，我们将设置
//`max_background_jobs=max_background_compactions+max_background_flushes`
//如果用户至少设置了一个“max”background“压缩”或
//'最大背景刷新'。
//
//提交的并发后台MEMTABLE刷新作业的最大数目
//默认为高优先级线程池。如果高优先级线程池
//配置为零线程，刷新作业将共享低优先级
//具有压缩作业的线程池。
//
//当同一个env被共享时，使用两个线程池是很重要的
//多个数据库实例。没有单独的水池，长时间运行的压实
//作业可能会阻止其他数据库实例的memtable刷新作业，
//导致不必要的停车位。
//
//如果要增加此值，还应考虑增加
//高优先级线程池。有关详细信息，请参阅
//env:：backgroundthreads
//默认值：-1
  int max_background_flushes = -1;

//指定信息日志文件的最大大小。如果日志文件
//大于“最大日志文件大小”，新的信息日志文件将
//被创造。
//如果max_log_file_size==0，所有日志将写入一个
//日志文件。
  size_t max_log_file_size = 0;

//信息日志文件滚动的时间（秒）。
//如果使用非零值指定，则将滚动日志文件
//如果它的活动时间超过了“日志文件”到“滚动”的时间。
//默认值：0（已禁用）
//在rocksdb-lite模式下不支持！
  size_t log_file_time_to_roll = 0;

//要保留的最大信息日志文件数。
//默认值：1000
  size_t keep_log_file_num = 1000;

//回收日志文件。
//如果非零，我们将重新使用以前写入的日志文件
//日志，覆盖旧数据。该值表示有多少
//这样的文件我们会在任何时候保存，以备日后使用。
//使用。这是更有效的，因为块已经
//已分配并且fDataSync不需要在之后更新inode
//每次写入。
//默认值：0
  size_t recycle_log_file_num = 0;

//清单文件在达到此限制时被回滚。
//将删除旧的清单文件。
//默认值为max_int，因此不会发生翻滚。
  uint64_t max_manifest_file_size = std::numeric_limits<uint64_t>::max();

//用于表缓存的碎片数。
  int table_cache_numshardbits = 6;

//不再支持
//int table_cache_remove_scan_count_limit；

//以下两个字段影响归档日志的删除方式。
//1。如果两者都设置为0，日志将尽快删除，并且不会进入
//档案馆。
//2。如果wal-ttl-seconds为0，wal-size-limit-mb不是0，
//每10分钟检查一次Wal文件，如果总大小大于
//然后，wal-size-limit-mb将从
//最早达到尺寸限制。将删除所有空文件。
//三。如果wal-ttl-seconds不是0，wal-size-limit-mb是0，那么
//每个wal_ttl_secondsi/2和那些
//早于wal的ttl秒将被删除。
//4。如果两者都不是0，则每10分钟检查一次wal文件
//将首先执行TTL检查。
  uint64_t WAL_ttl_seconds = 0;
  uint64_t WAL_size_limit_MB = 0;

//预先分配（通过fallocate）清单的字节数
//文件夹。默认为4MB，这是减少随机IO的合理方法
//以及防止预分配的安装件过度分配
//大量数据（例如XFS的allocSize选项）。
  size_t manifest_preallocation_size = 4 * 1024 * 1024;

//允许OS到mmap文件读取sst表。默认：假
  bool allow_mmap_reads = false;

//允许操作系统写入mmap文件。
//只有当设置为false时，db:：syncwal（）才有效。
//默认：假
  bool allow_mmap_writes = false;

//为读/写启用直接I/O模式
//它们可能改善性能，也可能不改善性能，具体取决于用例
//
//文件将以“直接I/O”模式打开
//这意味着来自磁盘的数据R/W不会被缓存，或者
//缓冲的。但是，设备的硬件缓冲区可能仍然
//被使用。内存映射文件不受这些参数的影响。

//直接用于用户读取
//默认：假
//在rocksdb-lite模式下不支持！
  bool use_direct_reads = false;

//在后台刷新和压缩中对读和写都使用o_direct
//如果为“真”，我们还强制新的“压缩”输入的“表读卡器”为“真”。
//默认：假
//在rocksdb-lite模式下不支持！
  bool use_direct_io_for_flush_and_compaction = false;

//如果为false，则忽略fallocate（）调用
  bool allow_fallocate = true;

//禁用子进程继承打开的文件。默认值：真
  bool is_fd_close_on_exec = true;

//不再支持--不再使用此选项
  bool skip_log_error_on_recovery = false;

//如果不是零，则转储rocksdb.stats以记录每个统计值\u dump\u period\u秒
//默认值：600（10分钟）
  unsigned int stats_dump_period_sec = 600;

//如果设置为true，将提示基础文件系统
//当打开sst文件时，访问模式是随机的。
//默认值：真
  bool advise_random_on_open = true;

//要在所有列的memtables中生成的数据量
//写入磁盘前的系列。
//
//这与执行限制的写缓冲区大小不同
//对于单个memtable。
//
//此功能在默认情况下被禁用。指定非零值
//启用它。
//
//默认值：0（已禁用）
  size_t db_write_buffer_size = 0;

//memtable的内存使用情况将报告给此对象。同一对象
//可以传递到多个DBS，它将跟踪所有
//星展。如果所有DBS的所有活动内存表的总大小超过
//在下一次写入的下一个数据库中，将触发刷新。
//发行。
//
//如果对象只在db上传递，则行为与
//数据库写入缓冲区大小。设置写缓冲区管理器时，设置的值将
//覆盖db_write_buffer_大小。
//
//此功能在默认情况下被禁用。指定非零值
//启用它。
//
//默认值：空
  std::shared_ptr<WriteBufferManager> write_buffer_manager = nullptr;

//开始压缩后指定文件访问模式。
//它将应用于压缩的所有输入文件。
//默认值：正常值
  enum AccessHint {
      NONE,
      NORMAL,
      SEQUENTIAL,
      WILLNEED
  };
  AccessHint access_hint_on_compaction_start = NORMAL;

//如果为true，则始终创建新的文件描述符和新的表读取器
//用于压缩输入。打开此参数可能会引入额外的
//表读取器中的内存使用情况（如果它分配了额外的内存）
//用于索引。这将允许文件描述符预取选项
//设置为压缩输入文件而不是影响文件
//用户查询使用的同一文件的描述符。
//建议启用blockBasedTableOptions.cache_index_和_filter_块
//对于此模式，如果使用基于块的表。
//
//默认：假
  bool new_table_reader_for_compaction_inputs = false;

//如果非零，则在进行压缩时执行更大的读取。如果你是
//在旋转磁盘上运行rocksdb，您应该将其设置为至少2MB。
//这样，rocksdb的压缩操作是按顺序进行的，而不是随机读取。
//
//当非零时，我们还强制新的表格阅读器输入到
//真的。
//
//默认值：0
  size_t compaction_readahead_size = 0;

//这是winmmapreadablefile在中使用的最大缓冲区大小
//无缓冲磁盘I/O模式。我们需要为
//阅读。我们允许缓冲区增长到指定的值，然后
//对于更大的请求，分配一个快照缓冲区。在无缓冲模式下，我们
//始终在readaheaderandomaccessfile处绕过预读缓冲区
//当需要预读时，我们使用压缩预读大小
//价值观，永远努力向前看。有了预读，我们总是
//预先分配缓冲区的大小，而不是将其增大到限制。
//
//此选项当前仅在Windows上使用
//
//默认值：1兆字节
//
//特殊值：0-表示不维护每个实例缓冲区。分配
//根据请求缓冲并避免锁定。
  size_t random_access_max_buffer_size = 1024 * 1024;

//这是可写文件写入程序使用的最大缓冲区大小。
//在Windows上，我们需要为写入维护一个对齐的缓冲区。
//我们允许缓冲区增长，直到其大小达到缓冲区的限制。
//IO并在使用直接IO时固定缓冲区大小，以确保
//如果逻辑扇区大小异常，则写入请求
//
//默认值：1024*1024（1 MB）
  size_t writable_file_max_buffer_size = 1024 * 1024;


//使用自适应互斥体，该互斥体在用户空间中旋转，然后再诉诸
//到内核。当互斥体不是
//竞争激烈。但是，如果互斥是热的，我们可能会结束
//浪费旋转时间。
//默认：假
  bool use_adaptive_mutex = false;

//使用所有字段的默认值创建dboptions
  DBOptions();
//从选项创建dboptions
  explicit DBOptions(const Options& options);

  void Dump(Logger* log) const;

//允许操作系统在文件处于
//在后台异步写入。此操作可以使用
//随着时间的推移，为了使写I/O变得平滑。用户不应该依赖它
//持续性保证。
//对写入的每个字节发出一个请求。0关掉它。
//默认值：0
//
//您可以考虑使用速率限制器来调节设备的写入速率。
//当速率限制器启用时，它会自动启用每个同步字节数
//到1MB。
//
//此选项适用于表文件
  uint64_t bytes_per_sync = 0;

//与每个同步字节数相同，但适用于wal文件
//默认值：0，已关闭
  uint64_t wal_bytes_per_sync = 0;

//将调用回调函数的事件侦听器的向量
//当特定的RocksDB事件发生时。
  std::vector<std::shared_ptr<EventListener>> listeners;

//如果为真，则此数据库中涉及的线程的状态将
//通过getthreadlist（）API跟踪并提供。
//
//默认：假
  bool enable_thread_tracking = false;

//如果软写挂起压缩字节限制或
//级别0_减速_写入触发器被触发，或者我们正在写入
//允许最后一个MEM表，我们允许超过3个MEM表。它是
//使用压缩前的用户写入请求大小计算。
//如果继续压实，RocksDB可能会决定放慢速度。
//进一步落后。
//如果值为0，我们将从“Rater_limiter”值中推断值。
//如果它不是空的，或者16MB如果“速率限制器”是空的。注意
//如果用户在数据库打开后更改“速率限制器”中的速率，
//“延迟写入速率”不会被调整。
//
//单位：字节/秒。
//
//默认值：0
  uint64_t delayed_write_rate = 0;

//默认情况下，维护单个写入线程队列。线程得到
//向队列的头部写入批处理组长并负责
//用于写入批处理组的wal和memtable。
//
//如果启用\u pipelined \u write为true，则单独的写入线程队列为
//为wal-write和memtable-write维护。写线程首先进入wal
//编写器队列，然后是memtable编写器队列。挂起的线程
//因此，编写器队列只需等待以前的编写器完成
//沃尔写作，但不是记忆写作。启用该功能可能会改进
//写吞吐量和减少两阶段准备阶段的延迟
//承诺。
//
//默认：假
  bool enable_pipelined_write = false;

//如果为true，则允许多个编写器并行更新MEM表。
//只有一些memtable_factory-s支持并发写入；目前它
//仅适用于Skiplist工厂。并发memtable写入
//与就地“更新”或“筛选”删除不兼容。
//强烈建议设置启用写入线程自适应屈服
//如果要使用此功能。
//
//默认值：真
  bool allow_concurrent_memtable_write = true;

//如果为true，则与写入批处理组领导同步的线程将
//在对互斥体进行阻塞之前，请等待向上写入线程\u max \u yield \u usec。
//这可以大大提高并发工作负载的吞吐量，
//无论是否启用了允许并发写入。
//
//默认值：真
  bool enable_write_thread_adaptive_yield = true;

//写入操作将使用的最大微秒数
//与以前的其他写入线程协调的让步自旋循环
//
//正确设置）增加该值可能会增加RocksDB
//以增加CPU使用为代价的吞吐量。
//
//默认值：100
  uint64_t write_thread_max_yield_usec = 100;

//以微秒为单位的延迟，在此延迟之后，std:：this_thread:：yield
//调用（Linux上的sched_yield）被认为是一个信号，
//其他进程或线程希望使用当前核心。
//增加这使得编写器线程更可能占用CPU
//通过旋转，它将显示为
//非自愿上下文切换。
//
//默认值：3
  uint64_t write_thread_slow_yield_usec = 3;

//如果为true，则db:：open（）将不更新用于优化的统计信息
//通过从许多文件加载表属性来决定压缩。
//关闭此功能将提高dbopen时间，尤其是在
//磁盘环境。
//
//默认：假
  bool skip_stats_update_on_db_open = false;

//恢复模式控制回放时的一致性
//默认值：kPointTimeRecovery
  WALRecoveryMode wal_recovery_mode = WALRecoveryMode::kPointInTimeRecovery;

//如果设置为false，则当
//在wal中遇到事务
  bool allow_2pc = false;

//表级行的全局缓存。
//默认值：nullptr（禁用）
//在rocksdb-lite模式下不支持！
  std::shared_ptr<Cache> row_cache = nullptr;

#ifndef ROCKSDB_LITE
//提供的在处理提前写入日志时调用的筛选器对象
//（Wals）在恢复期间。过滤器提供了一种检查日志的方法
//记录，忽略特定记录或跳过重播。
//过滤器在启动时调用，并从单个线程调用。
//目前。
  WalFilter* wal_filter = nullptr;
#endif  //摇滚乐

//如果为true，则db:：open/createColumnFamily/dropColumnFamily
//如果未检测到选项文件或选项文件不正确，则/setoptions将失败。
//坚持。
//
//默认：假
  bool fail_if_options_file_error = false;

//如果为真，则将malloc stats与rocksdb.stats一起打印
//打印到日志时。
//默认：假
  bool dump_malloc_stats = false;

//默认情况下，rocksdb重放wal日志并在db open上刷新，这可能
//创建非常小的sst文件。如果启用此选项，rocksdb将尝试
//避免（但不保证）在恢复过程中冲洗。此外，现有的
//我们会保留Wal日志，这样如果在冲水前发生撞车，我们仍然可以
//有要恢复的日志。
//
//默认：假
  bool avoid_flush_during_recovery = false;

//默认情况下，rocksdb将刷新db close上的所有memtable（如果有）
//未持久化的数据（即禁用wal）可以跳过刷新以加速
//关闭数据库。未持久化的数据将丢失。
//
//默认：假
//
//可通过setdboptions（）API动态更改。
  bool avoid_flush_during_shutdown = false;

//如果需要，请在创建数据库期间将此选项设置为true。
//要能够在后面摄取（调用IngestExternalFile（）跳过键
//它已经存在，而不是覆盖匹配的键）。
//将此选项设置为“真”将影响两件事：
//1）禁用有关sst文件压缩的一些内部优化
//2）只为接收的文件保留最底层。
//3）请注意，如果启用此选项，num_级别应大于等于3。
//
//默认：假
//不变的
  bool allow_ingest_behind = false;

//如果启用，它使用两个队列进行写入，一个队列用于
//禁用“memtable”，并为那些也写入memtable的禁用一个。这个
//允许memtable写入不落后于其他写入。可以使用
//要优化mysql 2pc，其中只有提交，这是串行的，写入
//可记忆的。
  bool concurrent_prepare = false;

//如果真的wal在每次写入后都不会自动刷新。相反，它
//依靠手动调用flushwal将wal缓冲区写入
//文件。
  bool manual_wal_flush = false;
};

//控制数据库行为的选项（传递给db:：open）
struct Options : public DBOptions, public ColumnFamilyOptions {
//使用所有字段的默认值创建选项对象。
  Options() : DBOptions(), ColumnFamilyOptions() {}

  Options(const DBOptions& db_options,
          const ColumnFamilyOptions& column_family_options)
      : DBOptions(db_options), ColumnFamilyOptions(column_family_options) {}

//该函数将选项恢复为版本4.6中的选项。
  Options* OldDefaults(int rocksdb_major_version = 4,
                       int rocksdb_minor_version = 6);

  void Dump(Logger* log) const;

  void DumpCFOptions(Logger* log) const;

//一些功能使优化rocksdb更容易

//为批量加载设置适当的参数。
//这是一个返回“this”而不是
//构造函数将在将来启用多个类似调用的链接。
//

//所有数据将处于0级，不进行任何自动压缩。
//建议在读取之前手动调用compactrange（空，空）
//从数据库中，因为否则读取速度会非常慢。
  Options* PrepareForBulkLoad();

//如果你的数据库非常小（比如1GB以下），而你不想使用它
//为内存表花费大量的内存。
  Options* OptimizeForSmallDb();
};

//
//应用程序可以发出读取请求（通过get/iterators）并指定
//如果该读取应该处理已经驻留在指定缓存中的数据
//水平。例如，如果应用程序指定KblockCacheTier，则
//get调用将处理memtable中已处理的数据或
//块缓存。它不会从操作系统缓存或
//存放在仓库中。
enum ReadTier {
kReadAllTier = 0x0,     //memtable、block cache、os cache或storage中的数据
kBlockCacheTier = 0x1,  //memtable或块缓存中的数据
kPersistedTier = 0x2,   //保留数据。当禁用wal时，此选项
//将跳过memtable中的数据。
//请注意，此readtier当前仅支持
//get和multiget，不支持迭代器。
kMemtableTier = 0x3     //memtable中的数据。仅用于memtable迭代器。
};

//控制读取操作的选项
struct ReadOptions {
//如果“快照”不是nullptr，则从提供的快照开始读取
//（必须属于正在读取的数据库，并且必须
//尚未发布）。如果“snapshot”为nullptr，则使用隐式
//此读取操作开始时的状态快照。
//默认值：nullptr
  const Snapshot* snapshot;

//“迭代器上限”定义前向迭代器到达的范围
//无法返回条目。到达绑定后，valid（）将为false。
//“Iterate_Upper_Bound”是独占的，即绑定值为
//不是有效条目。如果迭代器提取程序不为空，则查找目标
//迭代器的上限需要有相同的前缀。
//这是因为在前缀域之外不能保证排序。
//迭代器上没有下限。如果需要，这很容易
//实施。
//
//默认值：nullptr
  const Slice* iterate_upper_bound;

//如果非零，NewIterator将创建一个新的表阅读器，该阅读器
//执行给定大小的读取。使用大尺寸（>2MB）罐
//提高旋转盘的正迭代性能。
//默认值：0
  size_t readahead_size;

//在发生故障之前可以跳过的密钥数的阈值
//迭代器查找不完整。默认值0应用于
//即使跳过太多的键，也不要因请求不完整而失败。
//默认值：0
  uint64_t max_skippable_internal_keys;

//指定此读取请求是否应处理已经
//驻留在特定缓存中。如果所需数据不是
//在指定的缓存中找到，然后返回状态：：未完成。
//默认值：kreadalltier
  ReadTier read_tier;

//如果为true，则从基础存储中读取的所有数据将
//根据相应的校验和进行验证。
//默认值：真
  bool verify_checksums;

//是否应为此读取“数据块”/“索引块”/“筛选块”
//迭代是否缓存在内存中？
//对于批量扫描，调用方可能希望将此字段设置为false。
//默认值：真
  bool fill_cache;

//指定以创建尾随迭代器--具有
//完整数据库的视图（也就是说，它还可以用于新读取
//添加了数据），并针对顺序读取进行了优化。它会返回记录
//在创建迭代器后插入到数据库中的。
//默认：假
//在rocksdb-lite模式下不支持！
  bool tailing;

//指定创建托管迭代器--一个特殊迭代器
//通过能够释放其基础资源，减少资源使用
//根据要求提供资源。
//默认：假
//在rocksdb-lite模式下不支持！
  bool managed;

//无论索引格式如何（例如哈希索引），都启用总顺序查找
//在表中使用。某些表格式（如普通表）可能不支持
//这个选项。
//如果在调用get（）时为true，则在读取时也跳过前缀bloom
//基于块的表。它提供了一种在
//更改前缀提取程序的实现。
  bool total_order_seek;

//强制迭代器只在与seek相同的前缀上迭代。
//此选项仅对前缀查找有效，即前缀提取程序是
//列族和总计顺序查找的非空值为假。不像
//迭代\上限，前缀\与\开始相同\只在前缀内工作
//但在两个方向。
//默认：假
  bool prefix_same_as_start;

//将迭代器加载的块固定在内存中，只要
//如果从使用创建的表中读取时使用迭代器，则不会删除迭代器。
//blockBasedTableOptions：：使用_delta_encoding=false，
//迭代器的属性“rocksdb.iterator.is key pinned”保证
//返回1。
//默认：假
  bool pin_data;

//如果为true，则当在cleanupIteratorState中调用purgeobsoletefile时，我们
//在刷新作业队列中计划后台作业并删除过时的文件
//在后台。
//默认：假
  bool background_purge_on_iterator_cleanup;

//如果为true，则使用deleteRange（）API删除的键将对
//读卡器，直到它们在压缩过程中被自然删除。这改进
//在DBS中读取性能，有许多范围删除。
//默认：假
  bool ignore_range_deletions;

  ReadOptions();
  ReadOptions(bool cksum, bool cache);
};

//控制写入操作的选项
struct WriteOptions {
//如果为真，则将从操作系统中刷新写入。
//缓冲区缓存（通过在写入之前调用writablefile:：sync（））
//被认为是完整的。如果此标志为真，则写入将
//更慢的。
//
//如果此标志为假，并且机器崩溃，则
//写入可能丢失。注意，如果只是这个过程，
//崩溃（即机器不重新启动），不会写入
//即使sync==false也会丢失。
//
//换句话说，sync==false的db write与
//作为“write（）”系统调用的崩溃语义。DB写
//with sync==true与“write（）”具有相似的崩溃语义。
//系统调用后接“fDataSync（）”。
//
//默认：假
  bool sync;

//如果为真，则写入操作不会首先转到提前写入日志，
//而这封信可能会在撞车后丢失。
  bool disableWAL;

//如果为真，并且用户试图写入不存在的列族
//（它们被删除），忽略写入（不要返回错误）。如果有
//如果一个writebatch中有多个写入操作，则其他写入操作将成功。
//默认：假
  bool ignore_missing_column_families;

//如果为真，并且我们需要等待或休眠写入请求，则失败。
//立即显示状态：：不完整（）。
  bool no_slowdown;

//如果为true，则如果压缩为
//在后面。在这种情况下，no_slowed=true，请求将被取消。
//立即返回状态为：：incomplete（）。否则，它将
//放慢速度。减速值由RocksDB确定，以保证
//它对高优先级写入的影响最小。
//
//默认：假
  bool low_pri;

  WriteOptions()
      : sync(false),
        disableWAL(false),
        ignore_missing_column_families(false),
        no_slowdown(false),
        low_pri(false) {}
};

//控制刷新操作的选项
struct FlushOptions {
//如果为真，则刷新将等待刷新完成。
//默认值：真
  bool wait;

  FlushOptions() : wait(true) {}
};

//从提供的dboptions创建记录器
extern Status CreateLoggerFromOptions(const std::string& dbname,
                                      const DBOptions& options,
                                      std::shared_ptr<Logger>* logger);

//compactionOptions在compactfiles（）调用中使用。
struct CompactionOptions {
//压缩输出压缩类型
//默认：快速
  CompressionType compression;
//压缩将创建“输出文件大小限制”的文件。
//默认值：max，这意味着压缩将创建单个文件
  uint64_t output_file_size_limit;

  CompactionOptions()
      : compression(kSnappyCompression),
        output_file_size_limit(std::numeric_limits<uint64_t>::max()) {}
};

//对于基于级别的压缩，我们可以配置是否要跳过/强制
//最底层压实。
enum class BottommostLevelCompaction {
//跳底压实
  kSkip,
//只有在有压缩过滤器的情况下才压缩最底层
//这是默认选项
  kIfHaveCompactionFilter,
//总是紧凑的最底层
  kForce,
};

//compactRangeOptions由compactRange（）调用使用。
struct CompactRangeOptions {
//如果为真，则不会同时运行其他压缩
//人工压实
  bool exclusive_manual_compaction = true;
//如果为真，压缩的文件将移动到最低级别
//保持数据或给定水平（指定的非负目标水平）。
  bool change_level = false;
//如果更改级别为真且目标级别为非负值，则压缩
//文件将移动到目标级别。
  int target_level = -1;
//压缩输出将放置在选项.db_paths[目标_path_id]中。
//如果目标路径ID超出范围，则未定义行为。
  uint32_t target_path_id = 0;
//默认情况下，基于级别的压缩将只压缩最底层
//如果有压实过滤器
  BottommostLevelCompaction bottommost_level_compaction =
      BottommostLevelCompaction::kIfHaveCompactionFilter;
};

//IntermingExternalFileOptions由IntermingExternalFile（）使用
struct IngestExternalFileOptions {
//可以设置为true以移动文件而不是复制它们。
  bool move_files = false;
//如果设置为false，则摄取的文件密钥可能会出现在现有快照中。
//在文件被接收之前创建的。
  bool snapshot_consistency = true;
//如果设置为false，则如果文件密钥范围为
//与数据库中现有的键或墓碑重叠。
  bool allow_global_seqno = true;
//如果设置为false，并且文件密钥范围与memtable密钥范围重叠
//（需要Memtable刷新），InfectExternalFile将失败。
  bool allow_blocking_flush = true;
//如果您希望接收文件中的重复键，请设置为true。
//跳过而不是覆盖该键下的现有数据。
//用例：数据库中某些历史数据的后填充
//重写现有的较新版本的数据。
//只有在数据库正在运行时才能使用此选项
//从时间的黎明开始，允许你摄取后面的东西。
//所有文件将以seqno=0的最底层被接收。
  bool ingest_behind = false;
};

}  //命名空间rocksdb

#endif  //储存室
