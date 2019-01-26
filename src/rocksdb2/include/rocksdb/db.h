
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

#ifndef STORAGE_ROCKSDB_INCLUDE_DB_H_
#define STORAGE_ROCKSDB_INCLUDE_DB_H_

#include <stdint.h>
#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "rocksdb/iterator.h"
#include "rocksdb/listener.h"
#include "rocksdb/metadata.h"
#include "rocksdb/options.h"
#include "rocksdb/snapshot.h"
#include "rocksdb/sst_file_writer.h"
#include "rocksdb/thread_status.h"
#include "rocksdb/transaction_log.h"
#include "rocksdb/types.h"
#include "rocksdb/version.h"

#ifdef _WIN32
//Windows API宏干扰
#undef DeleteFile
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ROCKSDB_DEPRECATED_FUNC __attribute__((__deprecated__))
#elif _WIN32
#define ROCKSDB_DEPRECATED_FUNC __declspec(deprecated)
#endif

namespace rocksdb {

struct Options;
struct DBOptions;
struct ColumnFamilyOptions;
struct ReadOptions;
struct WriteOptions;
struct FlushOptions;
struct CompactionOptions;
struct CompactRangeOptions;
struct TableProperties;
struct ExternalSstFileInfo;
class WriteBatch;
class Env;
class EventListener;

using std::unique_ptr;

extern const std::string kDefaultColumnFamilyName;
struct ColumnFamilyDescriptor {
  std::string name;
  ColumnFamilyOptions options;
  ColumnFamilyDescriptor()
      : name(kDefaultColumnFamilyName), options(ColumnFamilyOptions()) {}
  ColumnFamilyDescriptor(const std::string& _name,
                         const ColumnFamilyOptions& _options)
      : name(_name), options(_options) {}
};

class ColumnFamilyHandle {
 public:
  virtual ~ColumnFamilyHandle() {}
//返回与当前句柄关联的列族的名称。
  virtual const std::string& GetName() const = 0;
//返回与当前句柄关联的列族的ID。
  virtual uint32_t GetID() const = 0;
//用列族的最新描述符填充“*desc”
//与此句柄关联。因为它用最新的
//信息，此调用可能在内部锁定并释放db mutex到
//访问最新的CF选项。此外，所有键入的指针
//引用选项的时间不能超过原始选项的存在时间。
//
//请注意，RocksBlite不支持此功能。
  virtual Status GetDescriptor(ColumnFamilyDescriptor* desc) = 0;
//返回与
//当前句柄。
  virtual const Comparator* GetComparator() const = 0;
};

static const int kMajorVersion = __ROCKSDB_MAJOR__;
static const int kMinorVersion = __ROCKSDB_MINOR__;

//一串钥匙
struct Range {
Slice start;          //包括在范围内
Slice limit;          //不包括在范围内

  Range() { }
  Range(const Slice& s, const Slice& l) : start(s), limit(l) { }
};

//表属性对象的集合，其中
//键：是表的文件名。
//值：给定表的表属性对象。
typedef std::unordered_map<std::string, std::shared_ptr<const TableProperties>>
    TablePropertiesCollection;

//数据库是从键到值的持久有序映射。
//数据库对于来自多个线程的并发访问是安全的，没有
//任何外部同步。
class DB {
 public:
//用指定的“名称”打开数据库。
//将指向堆分配数据库的指针存储在*dbptr中并返回
//成功就好了。
//将nullptr存储在*dbptr中，并在出错时返回非OK状态。
//当不再需要*dbptr时，调用方应将其删除。
  static Status Open(const Options& options,
                     const std::string& name,
                     DB** dbptr);

//以只读方式打开数据库。所有数据库接口
//修改数据（如Put/Delete）将返回错误。
//如果数据库以只读模式打开，则无压缩
//将会发生。
//
//RocksDB-Lite不支持，在这种情况下，函数将
//返回状态：不支持。
  static Status OpenForReadOnly(const Options& options,
      const std::string& name, DB** dbptr,
      bool error_if_log_file_exist = false);

//打开数据库以只读列族。打开数据库时
//只读，您只能在
//应打开的数据库。但是，始终需要指定默认值
//列族。默认的列族名称是“默认”并存储
//在rocksdb中：：kDefaultColumnFamilyName
//
//RocksDB-Lite不支持，在这种情况下，函数将
//返回状态：不支持。
  static Status OpenForReadOnly(
      const DBOptions& db_options, const std::string& name,
      const std::vector<ColumnFamilyDescriptor>& column_families,
      std::vector<ColumnFamilyHandle*>* handles, DB** dbptr,
      bool error_if_log_file_exist = false);

//打开带有柱族的DB。
//数据库选项指定特定于数据库的选项
//列族是数据库中所有列族的向量，
//包含列族名称和选项。您需要打开所有列
//数据库中的族。要获取柱族列表，可以使用
//ListColumnFamilies（）。此外，只能打开列族的子集
//用于只读访问。
//默认的列族名称是“默认”并存储
//在rocksdb中：：kDefaultColumnFamilyName。
//如果一切正常，手柄将返回相同大小
//作为专栏作家，我将成为你
//将用于操作柱族柱\柱族[I]。
//在删除数据库之前，必须通过调用
//DestroyColumnFamilyHandle（）和所有句柄。
  static Status Open(const DBOptions& db_options, const std::string& name,
                     const std::vector<ColumnFamilyDescriptor>& column_families,
                     std::vector<ColumnFamilyHandle*>* handles, DB** dbptr);

//ListColumnFamilies将打开由参数名指定的数据库
//并返回该数据库中所有列族的列表
//通过专栏“家庭”的争论。排序
//列\族中的列族未指定。
  static Status ListColumnFamilies(const DBOptions& db_options,
                                   const std::string& name,
                                   std::vector<std::string>* column_families);

  DB() { }
  virtual ~DB();

//创建列\族并返回列族的句柄
//通过参数句柄。
  virtual Status CreateColumnFamily(const ColumnFamilyOptions& options,
                                    const std::string& column_family_name,
                                    ColumnFamilyHandle** handle);

//使用相同的柱族选项批量创建柱族。
//通过参数句柄返回列族的句柄。
//如果出现错误，请求可能部分成功，句柄将
//包含它设法创建的列族句柄，并且具有大小
//等于创建的列族数。
  virtual Status CreateColumnFamilies(
      const ColumnFamilyOptions& options,
      const std::vector<std::string>& column_family_names,
      std::vector<ColumnFamilyHandle*>* handles);

//批量创建列族。
//通过参数句柄返回列族的句柄。
//如果出现错误，请求可能部分成功，句柄将
//包含它设法创建的列族句柄，并且具有大小
//等于创建的列族数。
  virtual Status CreateColumnFamilies(
      const std::vector<ColumnFamilyDescriptor>& column_families,
      std::vector<ColumnFamilyHandle*>* handles);

//删除列句柄指定的列族。这个电话
//仅在清单中记录删除记录并阻止列
//家庭从冲洗和压缩。
  virtual Status DropColumnFamily(ColumnFamilyHandle* column_family);

//大容量放置列族。此调用仅记录在
//显示并防止柱族刷新和压缩。
//如果出现错误，请求可能部分成功。用户可以调用
//要检查结果的ListColumnFamilies。
  virtual Status DropColumnFamilies(
      const std::vector<ColumnFamilyHandle*>& column_families);

//关闭列句柄指定的列族并销毁
//为避免重复删除而指定的列族句柄。这个电话
//默认情况下删除列族句柄。使用此方法
//关闭列族而不是直接删除列族句柄
  virtual Status DestroyColumnFamilyHandle(ColumnFamilyHandle* column_family);

//将“key”的数据库项设置为“value”。
//如果“key”已经存在，它将被覆盖。
//成功时返回OK，错误时返回非OK状态。
//注意：考虑设置options.sync=true。
  virtual Status Put(const WriteOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     const Slice& value) = 0;
  virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& value) {
    return Put(options, DefaultColumnFamily(), key, value);
  }

//删除“key”的数据库条目（如果有）。返回OK
//成功，错误时为非OK状态。“key”不是一个错误
//数据库中不存在。
//注意：考虑设置options.sync=true。
  virtual Status Delete(const WriteOptions& options,
                        ColumnFamilyHandle* column_family,
                        const Slice& key) = 0;
  virtual Status Delete(const WriteOptions& options, const Slice& key) {
    return Delete(options, DefaultColumnFamily(), key);
  }

//删除“key”的数据库条目。要求密钥存在
//并且没有被覆盖。成功时返回OK，并返回非OK状态
//关于错误。如果数据库中不存在“key”，则不是错误。
//
//如果一个键被覆盖（通过多次调用put（），那么结果
//对该键调用singleDelete（）的操作未定义。仅限singleDelete（）。
//如果自
//以前对此键调用singleDelete（）。
//
//此功能目前是一个实验性的性能优化
//对于非常具体的工作量。这取决于主叫方确保
//singleDelete仅用于未使用delete（）或
//使用merge（）编写。将单删除操作与删除和
//合并可能导致未定义的行为。
//
//注意：考虑设置options.sync=true。
  virtual Status SingleDelete(const WriteOptions& options,
                              ColumnFamilyHandle* column_family,
                              const Slice& key) = 0;
  virtual Status SingleDelete(const WriteOptions& options, const Slice& key) {
    return SingleDelete(options, DefaultColumnFamily(), key);
  }

//删除范围[“begin_key”、“end_key”中的数据库条目，即，
//包括“开始键”和“结束键”。成功后返回OK，并且
//出错时的非正常状态。如果范围中没有键，则不是错误
//[开始键，“结束键”）。
//
//此功能目前是用于
//删除非常大范围的连续键。多次或多次调用它
//小范围可能严重降低读取性能；特别是
//结果性能可能比为中的每个键调用delete（）差。
//范围。还要注意，降级的读取性能会影响
//删除范围，并影响涉及扫描的数据库操作，如刷新
//和压实。
//
//考虑设置readoptions：：忽略范围删除=真到速度
//向上读取已知不受范围删除影响的键。
  virtual Status DeleteRange(const WriteOptions& options,
                             ColumnFamilyHandle* column_family,
                             const Slice& begin_key, const Slice& end_key);

//将“key”的数据库条目与“value”合并。成功后返回OK，
//以及出错时的非OK状态。这个操作的语义是
//由用户在打开数据库时提供的合并运算符确定。
//注意：考虑设置options.sync=true。
  virtual Status Merge(const WriteOptions& options,
                       ColumnFamilyHandle* column_family, const Slice& key,
                       const Slice& value) = 0;
  virtual Status Merge(const WriteOptions& options, const Slice& key,
                       const Slice& value) {
    return Merge(options, DefaultColumnFamily(), key, value);
  }

//将指定的更新应用于数据库。
//如果“updates”不包含任何更新，则在
//选项。同步=真。
//成功时返回OK，失败时返回非OK。
//注意：考虑设置options.sync=true。
  virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;

//如果数据库包含“key”的条目，则存储
//*值中对应的值，返回OK。
//
//如果没有输入“key”，则保持*值不变并返回
//状态：：isNotFound（）返回true的状态。
//
//可能返回错误的其他状态。
  virtual inline Status Get(const ReadOptions& options,
                            ColumnFamilyHandle* column_family, const Slice& key,
                            std::string* value) {
    assert(value != nullptr);
    PinnableSlice pinnable_val(value);
    assert(!pinnable_val.IsPinned());
    auto s = Get(options, column_family, key, &pinnable_val);
    if (s.ok() && pinnable_val.IsPinned()) {
      value->assign(pinnable_val.data(), pinnable_val.size());
}  //else值已分配
    return s;
  }
  virtual Status Get(const ReadOptions& options,
                     ColumnFamilyHandle* column_family, const Slice& key,
                     PinnableSlice* value) = 0;
  virtual Status Get(const ReadOptions& options, const Slice& key, std::string* value) {
    return Get(options, DefaultColumnFamily(), key, value);
  }

//如果数据库中不存在键[i]，则返回
//状态将是status:：isNotFound（）为真的状态，并且
//（*值）[i]将被设置为某个任意值（通常为“”）。否则，
//第i个返回的状态将具有status:：ok（）true和（*values）[i]
//将存储与键[I]关联的值。
//
//（*值）将始终调整为与（键）相同的大小。
//同样，返回状态的数量将是键的数量。
//注意：键不会“重复数据消除”。重复的密钥将返回
//按顺序重复值。
  virtual std::vector<Status> MultiGet(
      const ReadOptions& options,
      const std::vector<ColumnFamilyHandle*>& column_family,
      const std::vector<Slice>& keys, std::vector<std::string>* values) = 0;
  virtual std::vector<Status> MultiGet(const ReadOptions& options,
                                       const std::vector<Slice>& keys,
                                       std::vector<std::string>* values) {
    return MultiGet(options, std::vector<ColumnFamilyHandle*>(
                                 keys.size(), DefaultColumnFamily()),
                    keys, values);
  }

//如果该键在数据库中确实不存在，则此方法
//返回false，否则返回true。如果调用方希望在键
//在内存中找到，必须传递“value\u found”的bool。价值发现
//如果值设置正确，返回时为真。
//此检查可能比调用db:：get（）更轻。单程
//为了减轻重量，避免使用任何iOS。
//此处的默认实现返回true并将“value\u found”设置为false
  /*实际布尔键可能存在（const readoptions和/*options*/，
                           column系列handle*/*column系列*/,

                           /*st slice&/*键*/，std:：string*/*值*/，
                           bool*value_found=nullptr）
    如果（找到值）= null pTr）{
      *值_found=false；
    }
    回归真实；
  }
  虚拟布尔键可能存在（const readoptions&options，const slice&key，
                           std:：string*值，bool*值_found=nullptr）
    返回keymayexist（选项，defaultColumnFamily（），键，值，找到值）；
  }

  //返回对数据库内容分配的堆迭代器。
  //newIterator（）的结果最初无效（调用方必须
  //在使用迭代器之前调用其中一个seek方法）。
  / /
  //调用方应在不再需要迭代器时删除它。
  //在删除此数据库之前，应删除返回的迭代器。
  虚拟迭代器*new迭代器（const readoptions&options，
                                columnFamilyhandle*column_family=0；
  虚拟迭代器*new迭代器（const readoptions&options）
    返回newIterator（options，defaultColumnFamily（））；
  }
  //从一致的数据库状态返回跨多个
  //列族。迭代器是堆分配的，需要删除
  //在删除数据库之前
  虚拟状态NewIterators（
      施工读数和选项，
      const std:：vector<columnFamilyhandle*>&column_families，
      std:：vector<迭代器*>*迭代器=0；

  //将句柄返回到当前数据库状态。使用创建的迭代器
  //此句柄将观察当前数据库的稳定快照
  /状态。调用方必须在
  //不再需要快照。
  / /
  //nullptr将在数据库无法获取快照或执行以下操作时返回
  //不支持快照。
  虚拟常量快照*getSnapshot（）=0；

  //释放以前获取的快照。呼叫方不得
  //调用后使用“快照”。
  虚拟void releasesnapshot（const snapshot*snapshot）=0；

ifndef岩石
  //包含getproperty（）的所有有效属性参数。
  / /
  //注意：属性名不能以数字结尾，因为它们被解释为
  //参数，例如，请参见knumfilesatlevelprefix。
  结构属性
    //“rocksdb.num files at level<n>”-返回包含数字的字符串
    //级别为<n>的文件，其中<n>是
    //级别号（例如“0”）。
    静态常量std：：string knumfilesatlevelprefix；

    //“rocksdb.compression ratio at level<n>”-返回包含
    //数据在级别上的压缩比<n>，其中<n>是一个ASCII
    //表示级别号（例如“0”）。这里，压缩
    //比率定义为未压缩数据大小/压缩文件大小。
    //如果级别<n>上没有打开的文件，则返回“-1.0”。
    静态常量std：：string k压缩率级别prefix；

    //“rocksdb.stats”-返回包含数据的多行字符串
    //由kcfstats描述，后跟kdbstats描述的数据。
    静态常量std：：string kstats；

    //“rocksdb.sstables”-返回汇总当前数据的多行字符串
    //sst文件。
    静态常量std：：string ksstables；

    //“rocksdb.cfstats”-“rocksdb.cfstats no file histogram”和
    //“rocksdb.cf文件柱状图”在一起。说明见下文
    /这两个。
    静态常量std：：string kcfstats；

    //“rocksdb.cfstats no file histogram”-返回带有
    //在数据库生存期内，每个级别的常规columm系列统计信息（“l<n>”），
    //在db的生存期内聚合（“sum”），并在
    //自上次检索以来的间隔（“int”）。
    //它还可以用于以映射的格式返回统计信息。
    //在这种情况下，double的数组将有一对字符串
    //每个级别以及“sum”。int“状态不会受到影响
    //检索此形式的统计信息时。
    静态常量std：：string kcfstatsnofilehistogram；

    //“rocksdb.cf file histogram”-打印每个文件的读取次数
    //级别，以及单个请求的延迟柱状图。
    静态常量std：：string kcfilehistogram；

    //“rocksdb.dbstats”-返回带有常规数据库的多行字符串
    //统计，包括累积（在数据库的生存期内）和间隔（自
    //最后一次检索kdbstats）。
    静态常量std:：string kdbstats；

    //“rocksdb.levelstats”-返回包含数字的多行字符串
    //每个级别的文件数和每个级别的总大小（MB）。
    静态常量std：：string klevelstats；

    //“rocksdb.num immutable mem table”-返回不可变数
    //尚未刷新的memtables。
    静态常量std:：string knummutablememtable；

    //“rocksdb.num immutable mem table flushed”-返回不可变数
    //已刷新的memtables。
    静态常量std:：string knummutablememtableflushed；

    //“rocksdb.mem table flush pending”-如果mem table flush为
    //挂起；否则返回0。
    静态常量std:：string kmemtableflushpending；

    //“rocksdb.num running flushes”-返回当前运行的次数
    //冲洗。
    静态常量std:：string knumrunningflushes；

    //“rocksdb.compaction pending”-如果至少有一次压缩，则返回1
    //挂起；否则返回0。
    static const std：：string kcompactionpending；

    //“rocksdb.num running compactions”-返回当前
    //运行压缩。
    静态常量std:：string knumrunningcompactions；

    //“rocksdb.background errors”-返回累计的背景数
    / /错误。
    静态常量std:：string kbackgrounderrors；

    //“rocksdb.cur size active mem table”-返回活动的近似大小
    //memtable（字节）。
    static const std：：string kCursizeActiveMetable；

    //“rocksdb.cur size all mem tables”-返回活动的近似大小
    //和未刷新的不可变内存表（字节）。
    static const std：：string kCursizeAllMemTables；

    //“rocksdb.size all mem tables”-返回活动的近似大小，
    //未刷新的不可变和固定的不可变内存表（字节）。
    static const std：：string ksizeallmemtables；

    //“rocksdb.num entries active mem table”-返回条目总数
    //在活动的memtable中。
    静态常量std:：string knumentriesactiveMetable；

    //“rocksdb.num entries imm mem tables”-返回条目总数
    //在未刷新的不可变内存表中。
    静态const std：：string knumentriesimmetables；

    //“rocksdb.num deletes active mem table”-返回删除的总数
    //活动memtable中的条目。
    静态常量std：：string knumDeletesActiveMetable；

    //“rocksdb.num deletes imm mem tables”-返回删除的总数
    //未刷新的不可变内存表中的条目。
    静态常量std：：string knumdeletesimmmetables；

    //“rocksdb.estimate num keys”-返回
    //活动和未刷新的不可变内存表和存储。
    静态常量std：：string kestimatenumkeys；

    //“rocksdb.estimate table readers mem”-返回用于
    //读取sst表，不包括块缓存中使用的内存（例如，
    //筛选和索引块）。
    静态常量std：：string kestimatetablereadersmem；

    //“rocksdb.is file deletions enabled”-如果删除过时的文件，则返回0
    //文件已启用；否则，返回一个非零数字。
    static const std:：string kisfiledelectionsEnabled；

    //“rocksdb.num snapshots”-返回
    //数据库。
    静态常量std:：string knumsnapshots；

    //“rocksdb.oldest snapshot time”-返回表示Unix的数字
    //最旧的未发布快照的时间戳。
    静态常量std：：string koldestsnapshottime；

    //“rocksdb.num live versions”-返回实时版本数。“版本”
    //是内部数据结构。有关详细信息，请参阅版本\set.h。更多
    //实时版本通常意味着更多的sst文件不会被删除，
    //通过迭代器或未完成的压缩。
    静态常量std：：string knumliveversions；

    //“rocksdb.current super version number”-返回当前LSM的数量
    //版本。它是一个uint64_t整数，在
    //对lsm树的任何更改。重新启动后号码不被保留
    /db。数据库重新启动后，将再次从0开始。
    static const std：：string kcurrentSuperversionNumber；

    //“rocksdb.estimate live data size”-返回
    //以字节为单位的实时数据。
    静态常量std:：string kestimatelivedatasize；

    //“rocksdb.min log number to keep”-返回
    //应保存的日志文件。
    静态常量std：：string kminlognumberTokeep；

    //“rocksdb.total sst files size”-返回所有sst的总大小（字节）
    //文件。
    //警告：如果文件太多，可能会减慢联机查询的速度。
    静态常量std：：string ktotalstfilesSize；

    //“rocksdb.base level”-返回L0数据将要达到的级别数
    //压缩。
    静态常量std：：string kbaselevel；

    //“rocksdb.estimate pending compaction bytes”-返回估计总数
    //需要重写压缩的字节数以降低所有级别
    //低于目标大小。对于除级别-
    /基于。
    静态常量std：：string kestimatePendingCompactionBytes；

    //“rocksdb.aggregated table properties”-返回字符串表示形式
    //目标列族的聚合表属性。
    静态const std:：string kaggregatedTableProperties；

    //“rocksdb.aggregated table properties at level<n>”，与上一个相同
    //一个，但只返回
    //在目标列族中指定的级别“n”。
    静态const std：：string kaggregatedTablePropertiesTable；

    //“rocksdb.actual delayed write rate”-返回当前实际延迟的
    //写入速率。0表示无延迟。
    静态常量std：：string kactualdelayedWrite；

    //“rocksdb.is write stopped”-如果已停止写入，返回1。
    静态常量std：：string kiswritestped；

    //“rocksdb.estimate oldest key time”-返回
    //数据库中最早的键时间戳。当前仅适用于
    //FIFO压缩
    //compaction_options_fifo.allow_compaction=false。
    静态常量std:：string kestimateoldstkeytime；
  }；
endif/*岩石*/


//DB实现可以通过此方法导出关于其状态的属性。
//如果“property”是此db实现理解的有效属性（请参见
//上面的属性结构用于有效选项），用其当前值填充“*value”
//值并返回true。否则，返回false。
  virtual bool GetProperty(ColumnFamilyHandle* column_family,
                           const Slice& property, std::string* value) = 0;
  virtual bool GetProperty(const Slice& property, std::string* value) {
    return GetProperty(DefaultColumnFamily(), property, value);
  }
  virtual bool GetMapProperty(ColumnFamilyHandle* column_family,
                              const Slice& property,
                              std::map<std::string, double>* value) = 0;
  virtual bool GetMapProperty(const Slice& property,
                              std::map<std::string, double>* value) {
    return GetMapProperty(DefaultColumnFamily(), property, value);
  }

//类似于getproperty（），但仅适用于其
//返回值为整数。以整数返回值。支持
//性能：
//“rocksdb.num不可变内存表”
//“rocksdb.mem表刷新挂起”
//“RocksDB.压实待定”
//“rocksdb.后台错误”
//“rocksdb.cur大小活动内存表”
//“rocksdb.cur大小所有内存表”
//“RocksDB.调整所有MEM表的大小”
//“rocksdb.num条目的活动内存表”
//“rocksdb.num条目imm mem表”
//“rocksdb.num删除活动mem表”
//“rocksdb.num删除imm mem表”
//“RocksDB.Estimate Num键”
//“RocksDB.估算表读卡器MEM”
//“rocksdb.是否启用文件删除”
//“rocksdb.num快照”
//“rocksdb.最早快照时间”
//“rocksdb.num实时版本”
//“RocksDB.当前超级版本号”
//“RocksDB.估计实时数据大小”
//“要保留的rocksdb.min日志号”
//“rocksdb.total sst文件大小”
//“RocksDB.基准面”
//“rocksdb.estimate挂起压缩字节”
//“RocksDB.num运行压实”
//“rocksdb.num运行刷新”
//“rocksdb.实际延迟写入速率”
//“rocksdb.已停止写入”
//“rocksdb.估计最早的密钥时间”
  virtual bool GetIntProperty(ColumnFamilyHandle* column_family,
                              const Slice& property, uint64_t* value) = 0;
  virtual bool GetIntProperty(const Slice& property, uint64_t* value) {
    return GetIntProperty(DefaultColumnFamily(), property, value);
  }

//重置数据库和所有列族的内部统计信息。
//注意，这不会重置选项。统计信息不属于
//数据库。
  virtual Status ResetStats() {
    return Status::NotSupported("Not implemented");
  }

//与getIntProperty（）相同，但此函数返回聚合的int
//所有列族的属性。
  virtual bool GetAggregatedIntProperty(const Slice& property,
                                        uint64_t* value) = 0;

//db:：getsizeApproximation的标志，用于指定memtable
//应该包括统计信息，或者文件统计信息近似值，或者两者都包括。
  enum SizeApproximationFlags : uint8_t {
    NONE = 0,
    INCLUDE_MEMTABLES = 1,
    INCLUDE_FILES = 1 << 1
  };

//对于[0，n-1]中的每个i，存储在“大小[i]”，大约
//“[范围[i].中的键使用的文件系统空间。开始..范围[i].限制）”。
//
//请注意，返回的大小度量文件系统空间使用情况，因此
//如果用户数据压缩了10倍，则返回
//大小将是相应用户数据大小的十分之一。
//
//if include标志定义返回的大小是否应包括
//MEM表中最近写入的数据（如果
//MEM表类型支持它），数据序列化到磁盘，或者两者都支持。
//include_标志应为db:：sizeApproximationFlags类型
  virtual void GetApproximateSizes(ColumnFamilyHandle* column_family,
                                   const Range* range, int n, uint64_t* sizes,
                                   uint8_t include_flags
                                   = INCLUDE_FILES) = 0;
  virtual void GetApproximateSizes(const Range* range, int n, uint64_t* sizes,
                                   uint8_t include_flags
                                   = INCLUDE_FILES) {
    GetApproximateSizes(DefaultColumnFamily(), range, n, sizes,
                        include_flags);
  }

//该方法类似于getapproxizes，只是
//返回memtables中的大约记录数。
  virtual void GetApproximateMemTableStats(ColumnFamilyHandle* column_family,
                                           const Range& range,
                                           uint64_t* const count,
                                           uint64_t* const size) = 0;
  virtual void GetApproximateMemTableStats(const Range& range,
                                           uint64_t* const count,
                                           uint64_t* const size) {
    GetApproximateMemTableStats(DefaultColumnFamily(), range, count, size);
  }

//getapproximatesizes的不推荐版本
  ROCKSDB_DEPRECATED_FUNC virtual void GetApproximateSizes(
      const Range* range, int n, uint64_t* sizes,
      bool include_memtable) {
    uint8_t include_flags = SizeApproximationFlags::INCLUDE_FILES;
    if (include_memtable) {
      include_flags |= SizeApproximationFlags::INCLUDE_MEMTABLES;
    }
    GetApproximateSizes(DefaultColumnFamily(), range, n, sizes, include_flags);
  }
  ROCKSDB_DEPRECATED_FUNC virtual void GetApproximateSizes(
      ColumnFamilyHandle* column_family,
      const Range* range, int n, uint64_t* sizes,
      bool include_memtable) {
    uint8_t include_flags = SizeApproximationFlags::INCLUDE_FILES;
    if (include_memtable) {
      include_flags |= SizeApproximationFlags::INCLUDE_MEMTABLES;
    }
    GetApproximateSizes(column_family, range, n, sizes, include_flags);
  }

//压缩密钥范围的底层存储[*开始，*结束]。
//实际的压缩间隔可能是[*开始，*结束]的超集。
//尤其是，删除和覆盖的版本被丢弃，
//重新排列数据以降低运营成本
//需要访问数据。此操作通常只应
//由了解底层实现的用户调用。
//
//begin==nullptr被视为数据库中所有键之前的键。
//end==nullptr被视为数据库中所有键之后的键。
//因此，以下调用将压缩整个数据库：
//db->compactrange（选项，nullptr，nullptr）；
//注意，压缩整个数据库后，所有数据都会被推送
//到最后一个包含任何数据的级别。如果之后的总数据大小
//压缩被减少，该级别可能不适合承载所有
//文件。在这种情况下，客户机可以设置选项。将“级别”更改为“真”，更改为
//将文件移回能够保存数据集的最低级别
//或给定的级别（由非负选项指定。目标级别）。
  virtual Status CompactRange(const CompactRangeOptions& options,
                              ColumnFamilyHandle* column_family,
                              const Slice* begin, const Slice* end) = 0;
  virtual Status CompactRange(const CompactRangeOptions& options,
                              const Slice* begin, const Slice* end) {
    return CompactRange(options, DefaultColumnFamily(), begin, end);
  }

  ROCKSDB_DEPRECATED_FUNC virtual Status CompactRange(
      ColumnFamilyHandle* column_family, const Slice* begin, const Slice* end,
      bool change_level = false, int target_level = -1,
      uint32_t target_path_id = 0) {
    CompactRangeOptions options;
    options.change_level = change_level;
    options.target_level = target_level;
    options.target_path_id = target_path_id;
    return CompactRange(options, column_family, begin, end);
  }

  ROCKSDB_DEPRECATED_FUNC virtual Status CompactRange(
      const Slice* begin, const Slice* end, bool change_level = false,
      int target_level = -1, uint32_t target_path_id = 0) {
    CompactRangeOptions options;
    options.change_level = change_level;
    options.target_level = target_level;
    options.target_path_id = target_path_id;
    return CompactRange(options, DefaultColumnFamily(), begin, end);
  }

  virtual Status SetOptions(
      /*umnfamilyhandle*/*column_family*/，
      const std:：unordered_map<std:：string，std:：string>&/*新_选项*/) {

    return Status::NotSupported("Not implemented");
  }
  virtual Status SetOptions(
      const std::unordered_map<std::string, std::string>& new_options) {
    return SetOptions(DefaultColumnFamily(), new_options);
  }

  virtual Status SetDBOptions(
      const std::unordered_map<std::string, std::string>& new_options) = 0;

//compactfiles（）输入由文件编号和
//将它们压缩到指定的级别。注意，行为是不同的
//从compactRange（）执行压缩作业。
//使用当前线程。
//
//@参见getdatabasemetadata
//@见getcolumnformamemetadata
  virtual Status CompactFiles(
      const CompactionOptions& compact_options,
      ColumnFamilyHandle* column_family,
      const std::vector<std::string>& input_file_names,
      const int output_level, const int output_path_id = -1) = 0;

  virtual Status CompactFiles(
      const CompactionOptions& compact_options,
      const std::vector<std::string>& input_file_names,
      const int output_level, const int output_path_id = -1) {
    return CompactFiles(compact_options, DefaultColumnFamily(),
                        input_file_names, output_level, output_path_id);
  }

//此函数将等待当前运行的所有后台进程
//完成。返回后，在
//调用UnblockBackgroundWork
  virtual Status PauseBackgroundWork() = 0;
  virtual Status ContinueBackgroundWork() = 0;

//此函数将为给定列启用自动压缩
//家庭，如果他们以前是残疾人。函数将首先设置
//在之后，将每个列族的“auto-compactions”选项禁用为“false”。
//它将安排冲洗/压实。
//
//注意：通过setOptions（）api将disable_auto_compactions设置为'false'
//之后不安排冲洗/压实，只更改
//列族选项中的参数本身。
//
  virtual Status EnableAutoCompaction(
      const std::vector<ColumnFamilyHandle*>& column_family_handles) = 0;

//用于此数据库的级别数。
  virtual int NumberLevels(ColumnFamilyHandle* column_family) = 0;
  virtual int NumberLevels() { return NumberLevels(DefaultColumnFamily()); }

//如果新的压缩MEMTABLE
//不创建重叠。
  virtual int MaxMemCompactionLevel(ColumnFamilyHandle* column_family) = 0;
  virtual int MaxMemCompactionLevel() {
    return MaxMemCompactionLevel(DefaultColumnFamily());
  }

//级别0中将停止写入的文件数。
  virtual int Level0StopWriteTrigger(ColumnFamilyHandle* column_family) = 0;
  virtual int Level0StopWriteTrigger() {
    return Level0StopWriteTrigger(DefaultColumnFamily());
  }

//get db name--与作为参数提供给
//DB：：（）
  virtual const std::string& GetName() const = 0;

//从数据库获取env对象
  virtual Env* GetEnv() const = 0;

//获取我们使用的数据库选项。在打开的过程中
//列族，调用db:：open（）或
//DB:：CreateColumnFamily（）将被“清理”并转换
//以实现定义的方式。
  virtual Options GetOptions(ColumnFamilyHandle* column_family) const = 0;
  virtual Options GetOptions() const {
    return GetOptions(DefaultColumnFamily());
  }

  virtual DBOptions GetDBOptions() const = 0;

//刷新所有MEM表数据。
  virtual Status Flush(const FlushOptions& options,
                       ColumnFamilyHandle* column_family) = 0;
  virtual Status Flush(const FlushOptions& options) {
    return Flush(options, DefaultColumnFamily());
  }

//将wal内存缓冲区刷新到文件中。如果sync为true，则调用syncwal
//之后。
  virtual Status FlushWAL(bool sync) {
    return Status::NotSupported("FlushWAL not implemented");
  }
//同步WAL。注意，write（）后面跟syncwal（）并不是
//与带有sync=true的write（）相同：在后一种情况下，更改不会
//在同步完成之前可见。
//当前仅当选项中的allow_map_writes=false时有效。
  virtual Status SyncWAL() = 0;

//最新事务的序列号。
  virtual SequenceNumber GetLatestSequenceNumber() const = 0;

#ifndef ROCKSDB_LITE

//防止文件删除。压实将继续发生，
//但不会删除过时的文件。调用此多个
//时间和一次叫它有同样的效果。
  virtual Status DisableFileDeletions() = 0;

//允许压缩删除过时的文件。
//如果force==true，则对enableFileDeletions（）的调用将确保
//在调用后启用文件删除，即使disablefiledelations（）。
//以前打过多次电话。
//如果force==false，则启用文件删除将仅启用文件删除
//在它被调用至少与disableFileDeletions（）的次数相同之后，
//允许两个线程同时调用这两个方法
//同步——也就是说，只有在两种情况都发生后才启用文件删除
//线程调用EnableFileDeletions（）。
  virtual Status EnableFileDeletions(bool force = true) = 0;

//getlivefiles后面跟着getsortewalfiles可以生成无损备份

//检索数据库中所有文件的列表。这些文件是
//相对于dbname，不是绝对路径。的有效大小
//清单文件以清单文件大小返回。清单文件是
//不断增长的文件，但只有清单文件大小指定的部分
//对于此快照有效。
//将flush memtable设置为true会在录制实时文件之前刷新。
//当我们不想等待时，将flush memtable设置为false非常有用。
//冲洗，可能需要等待压实完成
//不确定时间。
//
//如果有多个列族，即使flush-memtable为true，
//你仍然需要在getlivefiles之后调用getsortedwalfiles来补偿
//对于到达已刷新列族的新数据，而其他
//柱族正在冲洗
  virtual Status GetLiveFiles(std::vector<std::string>&,
                              uint64_t* manifest_file_size,
                              bool flush_memtable = true) = 0;

//首先用最早的文件检索所有wal文件的排序列表
  virtual Status GetSortedWalFiles(VectorLogPtr& files) = 0;

//将ITER设置为位于包含
//塞克数。如果序列号不存在，则返回迭代器
//在请求的序列号之后的第一个可用序列号
//如果迭代器有效，则返回状态：：OK
//必须将wal-ttl-seconds或wal-size-limit-mb设置为
//使用此API，否则wal文件将
//积极清除，迭代器可能在
//将读取更新。
  virtual Status GetUpdatesSince(
      SequenceNumber seq_number, unique_ptr<TransactionLogIterator>* iter,
      const TransactionLogIterator::ReadOptions&
          read_options = TransactionLogIterator::ReadOptions()) = 0;

//Windows API宏干扰
#undef DeleteFile
//从db目录中删除文件名，并将内部状态更新为
//反映这一点。仅支持删除sst和日志文件。'名称“必须”
//相对于db目录的路径。例如：000001.sst，/archive/000003.log
  virtual Status DeleteFile(std::string name) = 0;

//返回所有表文件的列表及其级别、开始键
//结束键
  virtual void GetLiveFilesMetaData(
      /*：：vector<livefilemetadata>*/*元数据*/）

  //获取数据库指定列族的元数据。
  //如果当前数据库没有，则返回status:：notfound（）。
  //任何列族都与指定的名称匹配。
  / /
  //如果未指定cf_name，则默认的元数据
  //将返回列族。
  虚拟void getColumnFamilyMetadata（ColumnFamilyHandle*/*Column_Family*/,

                                       /*umnfamilymetadata*/*元数据*/）

  //获取默认列族的元数据。
  void getColumnFamilyMetadata（
      columnfamilyMetadata*元数据）
    getColumnFamilyMetadata（defaultColumnFamily（），元数据）；
  }

  //IntermingExternalFile（）将向数据库中加载外部SST文件（1）的列表。
  //支持两种主模式：
  //-新文件中的重复键将覆盖现有键（默认）
  //-重复键将被跳过（设置Infetch_Behind=true）
  //在第一种模式中，我们将尝试找到
  //该文件可以容纳，并将该文件摄取到此级别（2）。一个文件
  //有一个与memtable键范围重叠的键范围需要我们
  //在接收文件之前先刷新memtable。
  //在第二种模式中，我们将始终在底部模式级别摄入（请参见
  //要接收ExternalFileOptions的文档：：在后面接收\u）。
  / /
  //（1）可以使用sstfilewriter创建外部sst文件
  //（2）我们将尝试将文件摄取到尽可能低的级别
  //即使文件压缩与级别压缩不匹配
  //（3）如果将InceptExternalFileOptions->Incept_Behind设置为true，
  //我们总是在最底层摄入，应该保留
  //为此目的（请参见dboptions：：allow_intoke_behind标志）。
  虚拟状态接收外部文件（
      columnFamilyhandle*column_系列，
      const std:：vector<std:：string>和外部文件，
      const摄取外部文件选项和选项=0；

  虚拟状态接收外部文件（
      const std:：vector<std:：string>和外部文件，
      const摄取外部文件选项和选项）
    返回IngestExternalFile（defaultColumnFamily（），外部_文件，选项）；
  }

  虚拟状态verifychecksum（）=0；

  //addfile（）已弃用，请使用IngestExternalFile（）。
  rocksdb_已弃用_func virtual status addfile（
      columnFamilyhandle*column_系列，
      const std:：vector<std:：string>&file_path_list，bool move_file=false，
      bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回IngestExternalFile（column_family，file_path_list，ifo）；
  }

  rocksdb_已弃用_func virtual status addfile（
      const std:：vector<std:：string>&file_path_list，bool move_file=false，
      bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回IngestExternalFile（defaultColumnFamily（），file_path_list，ifo）；
  }

  //addfile（）已弃用，请使用IngestExternalFile（）。
  rocksdb_已弃用_func virtual status addfile（
      columnFamilyhandle*column_family，const std:：string&file_path，
      bool move_file=false，bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回InfectExternalFile（column_family，file_path，ifo）；
  }

  rocksdb_已弃用_func virtual status addfile（
      const std:：string&file_path，bool move_file=false，
      bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回IngestExternalFile（defaultColumnFamily（），file_path，ifo）；
  }

  //将包含“file_info”信息的表文件加载到“column_family”中
  rocksdb_已弃用_func virtual status addfile（
      columnFamilyhandle*column_系列，
      const std:：vector<externalsstfileinfo>和file_info_list，
      bool move_file=false，bool skip_snapshot_check=false）
    std:：vector<std:：string>外部_文件；
    for（const externalstfileinfo&file_info:file_info_list）
      外部文件。向后推（file_info.file_path）；
    }
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回摄取外部文件（column_family，external_files，ifo）；
  }

  rocksdb_已弃用_func virtual status addfile（
      const std:：vector<externalsstfileinfo>和file_info_list，
      bool move_file=false，bool skip_snapshot_check=false）
    std:：vector<std:：string>外部_文件；
    for（const externalstfileinfo&file_info:file_info_list）
      外部文件。向后推（file_info.file_path）；
    }
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回IngestExternalFile（defaultColumnFamily（），外部_文件，ifo）；
  }

  rocksdb_已弃用_func virtual status addfile（
      columnfamilyhandle*column_family，const externalstfileinfo*文件_info，
      bool move_file=false，bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回InfectExternalFile（column_family，file_info->file_path，ifo）；
  }

  rocksdb_已弃用_func virtual status addfile（
      const externalssfileinfo*文件_info，bool move_file=false，
      bool skip_snapshot_check=false）
    摄取外部文件选项ifo；
    ifo.move_files=移动文件；
    ifo.snapshot_consistency=！跳过快照检查；
    ifo.allow_global_seqno=假；
    ifo.allow_blocking_flush=false；
    返回IngestExternalFile（defaultColumnFamily（），文件信息->文件路径，
                              IFO）；
  }

endif//rocksdb_lite

  //通过调用
  //env:：GenerateUniqueID（），在标识中。如果标识可以，则返回状态：：OK
  //设置正确
  虚拟状态getdbidentity（std:：string&identity）const=0；

  //返回默认的列族句柄
  虚拟列FamilyHandle*DefaultColumnFamily（）const=0；

ifndef岩石
  虚拟状态获取属性FallTables（ColumnFamilyHandle*Column_Family，
                                          TablePropertiesCollection*Props=0；
  虚拟状态GetPropertiesFallTables（TablePropertiesCollection*Props）
    返回getPropertiesFallTables（defaultColumnFamily（），props）；
  }
  虚拟状态getpropertiesoftablesinrange（
      columnfamilyhandle*column_family，const range*range，std:：size_t n，
      TablePropertiesCollection*Props=0；

  虚拟状态建议压缩范围（ColumnFamilyhandle*Column_Family，
                                     const slice*开始，const slice*结束）
    返回状态：：NotSupported（“SuggestCompactRange（）未实现”）；
  }

  虚拟状态提升程序0（ColumnFamilyhandle*Column_Family，
                           int目标_级）
    返回状态：：NotSupported（“Promotel0（）未实现。”）；
  }

endif//rocksdb_lite

  //stackabledb需要
  virtual db*getrootdb（）返回此；

 私人：
  //不允许复制
  dB（const dB &）；
  void运算符=（const db&）；
}；

//销毁指定数据库的内容。
//使用此方法时要非常小心。
status destroyDB（const std:：string&name，const options&options）；

ifndef岩石
//如果无法打开数据库，则可以尝试将此方法调用为
//尽可能多地恢复数据库的内容。
//某些数据可能丢失，因此调用此函数时要小心
//在包含重要信息的数据库上。
/ /
//使用此API，我们将警告并跳过与列族关联的数据，而不是
//在列_族中指定。
/ /
//@param column_families已知列族的描述符
状态修复b（const std:：string&dbname，const db options&db_options，
                const std：：vector<columnFamilyDescriptor>&column_families）；

//@param unknown_cf_为在
//修复列_族中未指定的。
状态修复b（const std:：string&dbname，const db options&db_options，
                const std:：vector<columnFamilyDescriptor>和column_families，
                const column系列选项和未知选项）；

//@param选项这些选项将用于数据库和所有列
//修复过程中遇到的族
status repairdb（const std:：string&dbname，const options&options）；

第二节

//命名空间rocksdb

endif//存储诳rocksdb诳U db诳U h诳
