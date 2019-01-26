
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

#pragma once

#include <memory>

#include "rocksdb/memtablerep.h"
#include "rocksdb/universal_compaction.h"

namespace rocksdb {

class Slice;
class SliceTransform;
enum CompressionType : unsigned char;
class TablePropertiesCollectorFactory;
class TableFactory;
struct Options;

enum CompactionStyle : char {
//基于级别的压缩样式
  kCompactionStyleLevel = 0x0,
//通用压实方式
//RocksDB-Lite中不支持。
  kCompactionStyleUniversal = 0x1,
//先进先出压实方式
//RocksDB-Lite不支持
  kCompactionStyleFIFO = 0x2,
//禁用后台压缩。已提交压缩作业
//通过compactfiles（）。
//RocksDB-Lite不支持
  kCompactionStyleNone = 0x3,
};

//在基于级别的压缩中，它确定要从某个级别
//选中以合并到下一个级别。我们建议人们尝试
//当你调优你的数据库时，首先要调优。
enum CompactionPri : char {
//按删除补偿的大小对较大的文件进行轻微的优先级排序
  kByCompensatedSize = 0x0,
//第一个压缩文件，其数据的最新更新时间最早。
//如果只更新小范围内的一些热键，请尝试此操作。
  kOldestLargestSeqFirst = 0x1,
//第一个压缩文件，其范围尚未压缩到下一个级别
//最长的。如果您的更新在密钥空间中是随机的，
//使用此选项时，写放大效果稍好。
  kOldestSmallestSeqFirst = 0x2,
//第一个压缩文件，其重叠大小在下一级别中的比率
//它的大小是最小的。在许多情况下，它可以优化写操作
//放大。
  kMinOverlappingRatio = 0x3,
};

struct CompactionOptionsFIFO {
//一旦表文件总数达到此值，我们将删除最旧的
//表文件
//默认值：1GB
  uint64_t max_table_files_size;

//删除早于TTL的文件。基于TTL的删除将优先于
//如果TTL>0，则基于大小的删除。
//删除如果sst文件创建时间<（当前时间-ttl）
//单位：秒。例：1天=1*24*60*60
//默认值：0（已禁用）
  uint64_t ttl = 0;

//如果为真，请尝试进行压缩以将较小的文件压缩为较大的文件。
//要压缩的最小文件数遵循options.level0_file_num_compaction_触发器
//如果每个del文件的平均压缩字节数为
//大于选项。写入缓冲区大小。这是为了保护大文件
//不再被压缩。
//默认：假；
  bool allow_compaction = false;

  CompactionOptionsFIFO() : max_table_files_size(1 * 1024 * 1024 * 1024) {}
  CompactionOptionsFIFO(uint64_t _max_table_files_size, bool _allow_compaction,
                        uint64_t _ttl = 0)
      : max_table_files_size(_max_table_files_size),
        ttl(_ttl),
        allow_compaction(_allow_compaction) {}
};

//不同压缩算法（如zlib）的压缩选项
struct CompressionOptions {
  int window_bits;
  int level;
  int strategy;
//用于填充压缩库的字典的最大大小。目前
//此字典将通过对
//目标级别为最底层时的子压缩。这本词典将
//在压缩/解压缩之前加载到压缩库中
//子协议中后续文件的数据块。实际上，这
//当数据块之间存在重复时，提高压缩比。
//值为0表示功能被禁用。
//默认值：0。
  uint32_t max_dict_bytes;

  CompressionOptions()
      : window_bits(-14), level(-1), strategy(0), max_dict_bytes(0) {}
  CompressionOptions(int wbits, int _lev, int _strategy, int _max_dict_bytes)
      : window_bits(wbits),
        level(_lev),
        strategy(_strategy),
        max_dict_bytes(_max_dict_bytes) {}
};

enum UpdateStatus {    //返回就地更新回调的状态
UPDATE_FAILED   = 0, //没有要更新的内容
UPDATED_INPLACE = 1, //就地更新值
UPDATED         = 2, //无就地更新。合并值集
};


struct AdvancedColumnFamilyOptions {
//在内存中建立的最大写入缓冲区数。
//默认值和最小值为2，因此当1写入缓冲区时
//正在刷新到存储，新的写入操作可以继续到另一个
//写入缓冲器。
//如果max_write_buffer_number>3，写入速度将减慢至
//选项。如果我们正在写入最后一个写入缓冲区，则延迟写入速率
//允许。
//
//默认值：2
//
//可通过setOptions（）API动态更改
  int max_write_buffer_number = 2;

//将合并到一起的最小写入缓冲区数
//在写入存储之前。如果设置为1，则
//所有写缓冲区都作为单个文件刷新到l0，这会增加
//读取放大，因为GET请求必须检查所有这些
//文件夹。此外，内存中的合并可能会导致写得更少
//如果每个记录中都有重复的记录，则存储数据。
//单个写入缓冲区。默认值：1
  int min_write_buffer_number_to_merge = 1;

//要在内存中维护的最大写入缓冲区总数，包括
//已刷新的缓冲区副本。不像
//最大写入缓冲区数，此参数不影响刷新。
//这将控制可用的最小写入历史记录量
//用于在使用事务时进行冲突检查的内存中。
//
//使用optimaticTransactionDB时：
//如果该值太低，则某些事务可能在提交时失败。
//无法确定是否存在任何写入冲突。
//
//使用TransactionDB时：
//如果使用了Transaction:：SetSnapshot，TransactionDB将读取
//在内存中写缓冲区或sst文件来执行写冲突检查。
//增加这个值可以减少对sst文件的读取次数。
//已完成冲突检测。
//
//将此值设置为0将导致立即释放写缓冲区
//在他们脸红之后。
//如果该值设置为-1，则将使用“最大写入缓冲区编号”。
//
//违约：
//如果使用transactiondb/optimatictransactiondb，则默认值将
//如果未显式设置，则设置为“max_write_buffer_number”的值
//由用户设置。否则，默认值为0。
  int max_write_buffer_number_to_maintain = 0;

//允许线程安全就地更新。如果这是真的，就没有办法
//使用快照或迭代器实现时间点一致性（假设
//同时更新）。因此迭代器和multi-get将返回结果
//在任何时间点都不一致。
//如果未设置inplace_回调函数，
//Put（key，new_value）将更新现有的_value iff
//*键存在于当前memtable中
//*new sizeof（new_value）<=sizeof（existing_value）
//*该键的现有值是一个Put，即ktypeValue。
//如果设置了inplace_回调函数，请检查doc中的inplace_回调。
//默认值：false。
  bool inplace_update_support = false;

//用于就地更新的锁数
//默认值：10000，如果inplace_update_support=true，则为0。
//
//可通过setOptions（）API动态更改
  size_t inplace_update_num_locks = 10000;

//现有值-指向上一个值的指针（来自memtable和sst）。
//如果键不存在，则为nullptr
//现有值-指向现有值大小的指针。
//如果键不存在，则为nullptr
//delta_值-要与现有_值合并的delta值。
//存储在事务日志中。
//合并值-在对上一个值应用delta时设置。

//仅当就地更新支持为真时适用，
//在更新memtable时调用此回调函数
//作为Put操作的一部分，假设Put（key，delta_value）。它允许
//指定为要与之合并的Put操作的一部分的“Delta值”
//数据库中键的“现有值”。

//如果合并值的大小小于“现有值”，
//然后此函数可以更新“现有值”缓冲区，并
//相应的“现有值”大小指针（如果愿意）。
//回调应返回updateStatus：：updated_inplace。
//在这种情况下。（在本例中，rocksdb的快照语义
//迭代器不再是原子的了）。

//如果合并值的大小大于“现有值”或
//应用程序不希望就地修改“现有值”缓冲区，
//然后，合并值应通过*合并值返回。它是由
//合并“现有价值”和“投入价值”。回调应该
//在这种情况下返回updateStatus:：updated。将添加此合并值
//到memtable。

//如果合并失败或应用程序不希望采取任何操作，
//然后回调应返回updateStatus:：update_failed。

//请记住，应用程序发出的原始呼叫已发出（键，
//δ值）。因此事务日志（如果启用）仍将包含（键，
//δ值）。“合并的值”未存储在事务日志中。
//因此，在重新打开数据库时，inplace_回调函数应该是一致的。

//默认值：nullptr
  UpdateStatus (*inplace_callback)(char* existing_value,
                                   uint32_t* existing_value_size,
                                   Slice delta_value,
                                   std::string* merged_value) = nullptr;

//如果设置了前缀抽取器，并且memtable前缀bloom大小比不是0，
//为memtable创建前缀bloom，大小为
//写入缓冲区大小*内存表前缀大小比率。
//如果大于0.25，则Santinized为0.25。
//
//默认值：0（禁用）
//
//可通过setOptions（）API动态更改
  double memtable_prefix_bloom_size_ratio = 0.0;

//用于Memtable所使用的竞技场的大页面的页面大小。如果＜0，则
//不会从大页面分配，而是从malloc分配。
//用户有责任为分配它保留大量的页面。为了
//例子：
//sysctl-w vm.nr_hugepages=20
//请参阅Linux文档/vm/hugetlbpage.txt
//如果没有足够的可用大页面，它将返回到
//马洛克
//
//可通过setOptions（）API动态更改
  size_t memtable_huge_page_size = 0;

//如果不是nullptr，memtable将使用指定的函数提取
//键和每个前缀的前缀都保留插入位置的提示
//减少插入带前缀的键的CPU使用。钥匙出
//前缀提取程序的域将在不使用提示的情况下插入。
//
//目前只有默认的基于skiplist的memtable实现了这个特性。
//所有其他memtable实现都将忽略该选项。它招致~ 250
//存储每个前缀提示的额外内存开销字节。
//同时，并发写入（当allow_concurrent_memtable_write为true时）将
//忽略该选项。
//
//该选项最适合可能插入密钥的工作负载。
//将最后插入的具有相同前缀的键关闭到某个位置。
//例如，可以插入表单的键（前缀+时间戳）。
//相同前缀的键总是按时间顺序排列。另一
//示例是一次又一次地更新同一个密钥，在这种情况下
//前缀可以是键本身。
//
//默认值：nullptr（禁用）
  std::shared_ptr<const SliceTransform>
      memtable_insert_with_hint_prefix_extractor = nullptr;

//控制Bloom筛选器探测的位置以提高缓存未命中率。
//此选项仅适用于memtable前缀bloom和plaintable
//前缀开花。它基本上将每次Bloom检查限制为一条缓存线。
//将此优化设置为0时关闭，将正数设置为
//它打开了。
//默认值：0
  uint32_t bloom_locality = 0;

//竞技场内存分配中一个块的大小。
//如果<=0，则自动计算适当的值（通常为
//写入缓冲区大小，四舍五入为4KB的倍数）。
//
//指定大小还有两个附加限制：
//（1）尺寸应在[4096，2<<30]范围内，并且
//（2）是CPU字的倍数（这有助于内存
//对齐）。
//
//我们会自动检查和调整尺码以确保
//符合限制。
//
//默认值：0
//
//可通过setOptions（）API动态更改
  size_t arena_block_size = 0;

//不同级别可以有不同的压缩策略。那里
//是最低级的应用程序希望使用快速压缩的情况吗？
//算法，而更高的层次（有更多的数据）使用
//压缩算法具有更好的压缩能力，但可以
//要慢一些。此数组（如果非空）应具有
//数据库的每个级别；这些级别覆盖
//前一个字段“compression”。
//
//注意如果level_compaction_dynamic_level_bytes=true，
//压缩级别[0]仍决定l0，但其他元素
//数组的个数是基于基本级别的（级别l0文件被合并
//，可能与用户从信息日志中看到的元数据级别不匹配。
//如果l0文件合并到n级，那么对于i>0，压缩\每\级[i]
//确定n+i-1级的压缩类型。
//例如，如果我们有三个5级，并且我们决定合并l0
//数据到l4（这意味着l1..l3将为空），然后新文件转到
//L4采用压缩式压缩，每级压缩。
//如果现在l0合并到l2。到L2的数据将被压缩
//根据压缩级别[1]，L3使用压缩级别[2]
//和L4使用压缩级别[3]。每级压实罐
//数据增长时更改。
  std::vector<CompressionType> compression_per_level;

//此数据库的级别数
  int num_levels = 7;

//0级文件数的软限制。我们开始放慢写速度
//点。值<0表示不会触发写入减速
//级别0中的文件数。
//
//默认值：20
//
//可通过setOptions（）API动态更改
  int level0_slowdown_writes_trigger = 20;

//0级文件的最大数目。我们现在停止写作。
//
//默认值：36
//
//可通过setOptions（）API动态更改
  int level0_stop_writes_trigger = 36;

//压缩的目标文件大小。
//目标文件大小是1级的每个文件大小。
//L级的目标文件大小可以通过
//目标文件大小基数*（目标文件大小乘数^（l-1））
//例如，如果目标文件大小为2MB，
//目标文件大小乘数为10，则级别1上的每个文件都将
//为2MB，2级文件为20MB，
//三级的每个文件都是200MB。
//
//默认值：64 MB。
//
//可通过setOptions（）API动态更改
  uint64_t target_file_size_base = 64 * 1048576;

//默认情况下，目标文件大小乘数为1，这意味着
//默认情况下，不同级别的文件大小相同。
//
//可通过setOptions（）API动态更改
  int target_file_size_multiplier = 1;

//如果为真，rocksdb将动态选择每个级别的目标大小。
//我们将选择一个基准面b>=1。L0将直接并入B级，
//而不是一直进入1级。1至B-1级需要为空。
//我们试着选择B和它的目标尺寸，这样
//1。目标大小在以下范围内
//（最大\u字节\用于\u级别\基础/max \ u字节\用于\u级别\乘数，
//最大字节数
//2。最后一级的目标大小（Level Num_Levels-1）等于额外大小
//水平的。
//同时，最大\字节\用于\级别\乘数和
//仍然满足\u级乘数\u附加值的最大\u字节\u。
//
//启用此选项后，从空数据库中，我们将最后一个级别设置为基本级别，
//这意味着将l0数据合并到最后一个级别，直到超过
//级别库的最大字节数。然后我们让第二个最后一级
//基本级别，开始将l0数据合并到第二个最后一个级别，及其
//对于最后一个级别的\级别\乘数，目标大小为1/最大\字节\u
//特大号。在数据累积更多以便我们需要移动
//基准面到最后一个基准面，依此类推。
//
//例如，假设\u level_乘数=10，num_levels=6的max_bytes_，
//_级_基的最大_字节数=10MB。
//1至5级的目标尺寸从以下开始：
//[--10Mb]
//基准面是水平的。1至4级目标尺寸不适用
//因为它们不会被使用。
//直到5级的大小超过10MB，比如11MB，我们
//基本目标到4级，现在目标看起来如下：
//[--1.1MB 11兆]
//在积累数据的同时，根据实际数据调整大小目标
//5级。当5级有50MB的数据时，目标如下：
//[--5MB 50MB]
//直到5级的实际大小超过100MB，比如说101MB。如果我们继续
//4级为基础级，目标大小为10.1MB，其中
//不满足目标大小范围。所以现在我们把3级作为目标
//级别的大小和目标大小如下：
//[--1.01MB 10.1MB 101MB]
//同样，当5级进一步增长时，所有级别的目标都会增长，
//喜欢
//[--5MB 50MB 500MB]
//直到5级超过1000MB，变成1001MB，我们使2级
//基本级别和使级别的目标大小如下：
//[-1.001MB 10.01MB 100.1MB 1001MB]
//继续…
//
//通过这样做，我们将最大\字节\作为\级别\乘数的优先级
//对于更可预测的LSM树形状，最大\u字节\u表示\u级\u基。它是
//有助于限制更糟的情况空间放大。
//
//当此标志打开时，将忽略附加的\u级别\u乘数\u的最大\u字节数。
//
//为现有数据库打开或关闭此功能可能会导致意外
//LSM树结构，因此不建议使用。
//
//注：此选项是实验性的
//
//默认：假
  bool level_compaction_dynamic_level_bytes = false;

//默认值：10。
//
//可通过setOptions（）API动态更改
  double max_bytes_for_level_multiplier = 10;

//不同级别的最大大小乘数不同。
//这些值乘以最大\字节\以得到\级别\乘数
//每层的最大尺寸。
//
//默认值：1
//
//可通过setOptions（）API动态更改
  std::vector<int> max_bytes_for_level_multiplier_additional =
      std::vector<int>(num_levels, 1);

//我们试图将一次压缩中的字节数限制为低于此值
//门槛。但这并不能保证。
//值0将被清除。
//
//默认值：result.target_file_size_base*25
  uint64_t max_compaction_bytes = 0;

//如果估计的话，所有的写操作都会减速到至少延迟的写速率。
//压缩所需的字节数超过此阈值。
//
//默认值：64 GB
  uint64_t soft_pending_compaction_bytes_limit = 64 * 1073741824ull;

//如果需要压缩的估计字节数超过
//这个阈值。
//
//默认值：256GB
  uint64_t hard_pending_compaction_bytes_limit = 256 * 1073741824ull;

//压缩样式。默认值：kCompactionStyleLevel
  CompactionStyle compaction_style = kCompactionStyleLevel;

//如果级别压缩样式=kCompactionStyleLevel，对于每个级别，
//选择要压缩的文件的优先级。
//默认值：kbycompensedSize
  CompactionPri compaction_pri = kByCompensatedSize;

//支持通用样式压缩所需的选项
  CompactionOptionsUniversal compaction_options_universal;

//FIFO压缩样式选项
  CompactionOptionsFIFO compaction_options_fifo;

//迭代->next（）按顺序跳过具有相同的键
//用户密钥，除非设置了此选项。此数字指定数字
//将按顺序排列的键（具有相同的用户键）
//在发出重新发送之前跳过。
//
//默认值：8
//
//可通过setOptions（）API动态更改
  uint64_t max_sequential_skip_in_iterations = 8;

//这是一个提供memtablerep对象的工厂。
//默认值：提供基于跳过列表的
//纪念碑
  std::shared_ptr<MemTableRepFactory> memtable_factory =
      std::shared_ptr<SkipListFactory>(new SkipListFactory);

//基于块的表相关选项将移动到BlockBasedTableOptions。
//最初位于此处但现在已移动的相关选项包括：
//NOx块缓存
//块缓存
//块缓存压缩
//块状大小
//块尺寸偏差
//阻止重新启动间隔
//过滤策略
//全键过滤
//如果您想自定义其中一些选项，您需要
//使用newblockbasedTableFactory（）构造新的表工厂。

//此选项允许用户收集自己感兴趣的
//桌子。
//默认值：空向量--将不收集用户定义的统计信息
//进行。
  typedef std::vector<std::shared_ptr<TablePropertiesCollectorFactory>>
      TablePropertiesCollectorFactories;
  TablePropertiesCollectorFactories table_properties_collector_factories;

//对memtable中的键执行的最大连续合并操作数。
//
//当合并操作添加到memtable时，最大数目为
//达到连续合并，将计算键的值并
//插入到memtable而不是合并操作中。本遗嘱
//确保没有超过max的连续合并
//memtable中的操作。
//
//默认值：0（已禁用）
//
//可通过setOptions（）API动态更改
  size_t max_successive_merges = 0;

//此标志指定实现应优化筛选器
//主要用于找到密钥的情况，而不是优化密钥
//错过。这将在应用程序知道的情况下使用
//很少有失误，或者失误时的表现不是
//重要的。
//
//现在，这个标志允许我们不存储最后一个级别的过滤器，即
//包含LSM存储区数据的最大级别。对于钥匙
//是命中，此级别中的筛选器无效，因为我们将搜索
//不管怎样，都是为了数据。注意：其他级别的过滤器仍然有用
//即使是关键命中，因为他们告诉我们是看在那个水平还是去。
//更高层次。
//
//默认：假
  bool optimize_filters_for_hits = false;

//在写入每个sst文件后，重新打开它并读取所有密钥。
//默认：假
  bool paranoid_file_checks = false;

//在调试模式下，rocksdb每次LSM都对LSM运行一致性检查。
//更改（刷新、压缩、添加文件）。这些检查在释放时被禁用
//模式，使用此选项也可以在释放模式下启用它们。
//默认：假
  bool force_consistency_checks = false;

//如果为真，则在压缩和刷新中测量IO状态。
//默认：假
  bool report_bg_io_stats = false;

//使用所有字段的默认值创建ColumnFamilyOptions
  AdvancedColumnFamilyOptions();
//从选项创建ColumnFamilyOptions
  explicit AdvancedColumnFamilyOptions(const Options& options);

//------------不再支持选项-----------

//不再支持
//这已经不起作用了。
  int max_mem_compaction_level;

//不再支持--不再使用此选项
//看跌期权被延迟。当任何级别有一个
//压实度分数超过了软速率限制。当==0.0时忽略此项。
//
//默认值：0（已禁用）
//
//可通过setOptions（）API动态更改
  double soft_rate_limit = 0.0;

//不再支持--不再使用此选项
  double hard_rate_limit = 0.0;

//不再支持--不再使用此选项
  unsigned int rate_limit_delay_max_milliseconds = 100;

//不再支持
//没有任何效果。
  bool purge_redundant_kvs_while_flush = true;
};

}  //命名空间rocksdb
