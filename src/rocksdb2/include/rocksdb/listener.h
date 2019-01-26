
//此源码被清华学神尹成大魔王专业翻译分析并修改
//尹成QQ77025077
//尹成微信18510341407
//尹成所在QQ群721929980
//尹成邮箱 yinc13@mails.tsinghua.edu.cn
//尹成毕业于清华大学,微软区块链领域全球最有价值专家
//https://mvp.microsoft.com/zh-cn/PublicProfile/4033620
//版权所有（c）2014 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "rocksdb/compaction_job_stats.h"
#include "rocksdb/status.h"
#include "rocksdb/table_properties.h"

namespace rocksdb {

typedef std::unordered_map<std::string, std::shared_ptr<const TableProperties>>
    TablePropertiesCollection;

class DB;
class ColumnFamilyHandle;
class Status;
struct CompactionJobStats;
enum CompressionType : unsigned char;

enum class TableFileCreationReason {
  kFlush,
  kCompaction,
  kRecovery,
};

struct TableFileCreationBriefInfo {
//创建文件的数据库的名称
  std::string db_name;
//创建文件的列族的名称。
  std::string cf_name;
//创建的文件的路径。
  std::string file_path;
//作业的ID（可以是刷新或压缩），该ID
//创建了文件。
  int job_id;
//创建表的原因。
  TableFileCreationReason reason;
};

struct TableFileCreationInfo : public TableFileCreationBriefInfo {
  TableFileCreationInfo() = default;
  explicit TableFileCreationInfo(TableProperties&& prop)
      : table_properties(prop) {}
//文件的大小。
  uint64_t file_size;
//创建的文件的详细属性。
  TableProperties table_properties;
//指示创建是否成功的状态。
  Status status;
};

enum class CompactionReason {
  kUnknown,
//[级别]L0文件数>级别0_文件_num_压缩_触发器
  kLevelL0FilesNum,
//[级别]级别的总大小>MaxBytesForLevel（）。
  kLevelMaxLevelSize,
//[通用]尺寸放大压缩
  kUniversalSizeAmplification,
//[通用]尺寸比压实
  kUniversalSizeRatio,
//[通用]排序运行数>级别0_文件_num_压缩_触发器
  kUniversalSortedRunNum,
//[FIFO]总大小>最大表格文件大小
  kFIFOMaxSize,
//[FIFO]减少文件数量。
  kFIFOReduceNumFiles,
//[FIFO]创建时间<（当前时间间隔）的文件
  kFIFOTtl,
//人工压实
  kManualCompaction,
//db:：suggestCompactRange（）标记要压缩的文件
  kFilesMarkedForCompaction,
};

enum class BackgroundErrorReason {
  kFlush,
  kCompaction,
  kWriteCallback,
  kMemTable,
};

#ifndef ROCKSDB_LITE

struct TableFileDeletionInfo {
//删除文件的数据库的名称。
  std::string db_name;
//已删除文件的路径。
  std::string file_path;
//删除文件的作业的ID。
  int job_id;
//指示删除是否成功的状态。
  Status status;
};

struct FlushJobInfo {
//列族的名称
  std::string cf_name;
//新创建文件的路径
  std::string file_path;
//完成此刷新作业的线程的ID。
  uint64_t thread_id;
//在同一线程中唯一的作业ID。
  int job_id;
//如果为真，则rocksdb当前正在减慢所有写入速度以防止
//由于压缩似乎无法创建过多的0级文件
//赶上写请求的速度。这表明
//级别0中的文件太多。
  bool triggered_writes_slowdown;
//如果为真，则rocksdb当前正在阻止任何写入以防止
//创建更多L0文件。这表明有太多
//0级中的文件。压缩应尝试压缩l0文件
//尽快降低水平。
  bool triggered_writes_stop;
//新创建文件中的最小序列号
  SequenceNumber smallest_seqno;
//新创建文件中最大的序列号
  SequenceNumber largest_seqno;
//正在刷新的表的表属性
  TableProperties table_properties;
};

struct CompactionJobInfo {
  CompactionJobInfo() = default;
  explicit CompactionJobInfo(const CompactionJobStats& _stats) :
      stats(_stats) {}

//发生压缩的列族的名称。
  std::string cf_name;
//指示压缩是否成功的状态。
  Status status;
//完成此压缩作业的线程的ID。
  uint64_t thread_id;
//在同一线程中唯一的作业ID。
  int job_id;
//压缩的最小输入级别。
  int base_input_level;
//压缩的输出级别。
  int output_level;
//压缩输入文件的名称。
  std::vector<std::string> input_files;

//压缩输出文件的名称。
  std::vector<std::string> output_files;
//输入和输出表的表属性。
//地图由输入文件和输出文件中的值设置关键帧。
  TablePropertiesCollection table_properties;

//进行压实的原因
  CompactionReason compaction_reason;

//用于输出文件的压缩算法
  CompressionType compression;

//如果非空，则此变量存储详细信息
//关于这次压缩。
  CompactionJobStats stats;
};

struct MemTableInfo {
//memtable所属的列族的名称
  std::string cf_name;
//插入的第一个元素的序列号
//进入记忆表。
  SequenceNumber first_seqno;
//保证小于或等于的序列号
//任何可以插入此项的键的序列号
//可记忆的。然后可以假定任何具有较大值（或相等值）的写入
//序列号将出现在此memtable或更高版本的memtable中。
  SequenceNumber earliest_seqno;
//memtable中的条目总数
  uint64_t num_entries;
//memtable中的删除总数
  uint64_t num_deletes;

};

struct ExternalFileIngestionInfo {
//列族的名称
  std::string cf_name;
//数据库外部的文件路径
  std::string external_file_path;
//数据库中文件的路径
  std::string internal_file_path;
//分配给此文件中的键的全局序列号
  SequenceNumber global_seqno;
//正在刷新的表的表属性
  TableProperties table_properties;
};

//对rocksdb的回调函数，在压缩时调用该函数
//迭代器正在压缩值。它意味着从
//压缩开始时的eventListner:：getCompactionEventListner（）。
//工作。
class CompactionEventListener {
 public:
  enum CompactionListenerValueType {
    kValue,
    kMergeOperand,
    kDelete,
    kSingleDelete,
    kRangeDelete,
    kBlobIndex,
    kInvalid,
  };

  virtual void OnCompaction(int level, const Slice& key,
                            CompactionListenerValueType value_type,
                            const Slice& existing_value,
                            const SequenceNumber& sn, bool is_new) = 0;

  virtual ~CompactionEventListener() = default;
};

//EventListener类包含一组回调函数，
//当发生特定的rocksdb事件（如flush）时调用。它可以
//用作开发自定义功能（如
//统计收集器或外部压缩算法。
//
//请注意，回调函数不应在
//函数返回前的时间，否则rocksdb可能被阻塞。
//例如，不建议执行db:：compactfiles（）（因为它可能
//运行很长时间）或发出许多db:：put（）（put可能被阻止
//在某些情况下）在EventListener回调的同一线程中。
//但是，在另一个线程中执行db:：compactfiles（）和db:：put（）是
//被认为是安全的。
//
//[线程]将使用
//涉及该特定事件的实际线程。例如，它
//是rocksdb background flush线程，它执行
//调用EventListener:：OnFlushCompleted（）。
//
//[锁定]所有EventListener回调都设计为在没有
//包含任何db mutex的当前线程。这是为了防止潜在的
//使用EventListener回调时出现死锁和性能问题
//以一种复杂的方式。但是，所有EventListener回调函数
//不应在函数之前长时间运行
//返回，否则rocksdb可能被阻止。例如，它不是
//建议执行db:：compactfiles（）（因为它可能会运行很长时间）
//或发出许多db:：put（）（在某些情况下，put可能会被阻塞）
//在EventListener回调的同一线程中。然而，做
//db:：compactfiles（）和db:：put（）位于
//EventListener回调线程被认为是安全的。
class EventListener {
 public:
//对rocksdb的回调函数，每当
//注册的rocksdb刷新文件。默认实现是
//NO-OP
//
//请注意，此函数的实现方式必须确保
//它不应该在函数之前运行很长一段时间
//返回。否则，rocksdb可能会被阻塞。
  /*冲洗完成后的实际空隙（db*/*db*/，
                                const flushjobinfo&/*刷新作业信息*/) {}


//对rocksdb的回调函数，将在
//rocksdb开始冲洗memtables。默认实现是
//NO-OP
//
//请注意，此函数的实现方式必须确保
//它不应该在函数之前运行很长一段时间
//返回。否则，rocksdb可能会被阻塞。
  /*冲洗开始时的实际空隙（db*/*db*/，
                            const flushjobinfo&/*刷新作业信息*/) {}


//rocksdb的回调函数，将在任何时候调用
//删除一个sst文件。不同于已完成的OnCompaction和
//完成后，此回拨用于外部日志记录
//服务，因此只提供字符串参数
//指向数据库的指针。构建基于基本逻辑的应用程序
//建议在文件中创建和删除
//OnFlushCompleted和OnCompactionCompleted。
//
//注意，如果应用程序希望使用传递的引用
//在这个函数调用之外，它们应该从
//返回值。
  /*tual void ontablefiledeleted（const tablefiledeletioninfo&/*info*/）

  //rocksdb的回调函数，每当
  //注册的rocksdb压缩文件。默认实现
  //是NO-OP。
  / /
  //请注意，此函数的实现方式必须确保
  //它不应在函数之前长时间运行
  / /返回。否则，rocksdb可能会被阻塞。
  / /
  //@param db指向刚压缩的rocksdb实例的指针
  //文件。
  //@param ci是对compactionjobinfo结构的引用。词释
  //返回此函数后，如果需要，必须复制它
  //在此函数之外。
  虚拟void oncompaction已完成（db*/*d*/,

                                     /*ST压缩作业信息&/*CI*/）

  //rocksdb的回调函数，每当
  //创建了一个sst文件。不同于已完成的OnCompaction和
  //onFlushCompleted，此回调是为外部日志设计的
  //服务，因此只提供字符串参数
  //指向db的指针。构建基于基本逻辑的应用程序
  //建议在文件创建和删除时实现
  //OnFlushCompleted和OnCompactionCompleted。
  / /
  //历史上只有在成功创建文件时才会调用它。
  //现在还将在失败情况下调用它。用户可以查看信息状态
  //查看是否成功。
  / /
  //请注意，如果应用程序要使用传递的引用
  //在此函数调用之外，它们应该从
  //返回值。
  已创建虚拟void ontablefilecreated（const tablefilecreationinfo&/*inf*/) {}


//rocksdb的回调函数，将在
//正在创建一个sst文件。后面是在
//创作完成。
//
//注意，如果应用程序希望使用传递的引用
//在这个函数调用之外，它们应该从
//返回值。
  virtual void OnTableFileCreationStarted(
      /*st tablefilecreationbriefinfo&/*信息*/）

  //rocksdb的回调函数，该函数将在
  //memtable是不可变的。
  / /
  //请注意，此函数的实现方式必须确保
  //它不应在函数之前长时间运行
  / /返回。否则，rocksdb可能会被阻塞。
  / /
  //请注意，如果应用程序要使用传递的引用
  //在此函数调用之外，它们应该从
  //返回值。
  虚拟void onmemtablesealed（
    const memtableinfo和/*inf*/) {}


//rocksdb的回调函数，将在
//将删除列族句柄。
//
//请注意，此函数的实现方式必须确保
//它不应该在函数之前运行很长一段时间
//返回。否则，rocksdb可能会被阻塞。
//@param handle是指向要删除的列族句柄的指针。
//删除后将成为悬空指针。
  virtual void OnColumnFamilyHandleDeletionStarted(ColumnFamilyHandle* handle) {
  }

//rocksdb的回调函数，将在外部调用后调用
//使用InfectExternalFile接收文件。
//
//请注意，此函数将在与
//IntermingExternalFile（），如果此函数被阻止，则IntermingExternalFile（）。
//将被阻止完成。
  virtual void OnExternalFileIngested(
      /*/*db*/，const external文件摄取信息&/*info*/）

  //rocksdb的回调函数，在设置
  //后台错误状态为非OK值。新的后台错误状态
  //在'bg_error'中提供，可以通过回调进行修改。例如，A
  //回调可以通过将其重置为status:：ok（）来抑制错误，因此
  //防止数据库进入只读模式。我们不提供任何
  //如果用户
  //取消错误。
  / /
  //请注意，此函数可以在与flush、compaction和
  //和用户写入。因此，非常重要的是不要表现得很重
  //此函数中的计算或阻塞调用。
  虚拟void onbackgroundError（backgroundErrorReason/*原因*/,

                                 /*tus*/*bg_错误*/）

  //返回CompactionEventListener的工厂方法。如果多个侦听器
  //提供CompactionEventListner，只使用第一个。
  虚拟CompactionEventListener*getCompactionEventListener（）
    返回null pTR；
  }

  虚拟~eventListener（）
}；

否则

类事件侦听器
}；

endif//rocksdb_lite

//命名空间rocksdb
