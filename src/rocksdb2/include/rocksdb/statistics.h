
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

#ifndef STORAGE_ROCKSDB_INCLUDE_STATISTICS_H_
#define STORAGE_ROCKSDB_INCLUDE_STATISTICS_H_

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <memory>
#include <vector>

#include "rocksdb/status.h"

namespace rocksdb {

/*
 *继续添加ticker。
 * 1。应在ticker_enum_max之前添加任何ticker。
 * 2。在下面的tickersnamemap中为新添加的ticker添加一个可读字符串。
 * 3。在Java API中添加相应的EnUM值到TigKyType
 **/

enum Tickers : uint32_t {
//块缓存未命中总数
//要求：block_cache_miss==block_cache_index_miss+
//块缓存过滤器未命中+
//块缓存数据丢失；
  BLOCK_CACHE_MISS = 0,
//块缓存命中总数
//需要：block_cache_hit==block_cache_index_hit+
//阻止缓存过滤器命中+
//块缓存数据命中；
  BLOCK_CACHE_HIT,
//添加到块缓存的块。
  BLOCK_CACHE_ADD,
//将块添加到块缓存时失败的次数。
  BLOCK_CACHE_ADD_FAILURES,
//从块缓存访问索引块时缓存未命中的次数。
  BLOCK_CACHE_INDEX_MISS,
//从块缓存访问索引块时缓存命中的次数。
  BLOCK_CACHE_INDEX_HIT,
//添加到块缓存的索引块。
  BLOCK_CACHE_INDEX_ADD,
//插入缓存的索引块字节数
  BLOCK_CACHE_INDEX_BYTES_INSERT,
//从缓存中删除的索引块字节数
  BLOCK_CACHE_INDEX_BYTES_EVICT,
//从块缓存访问筛选器块时缓存未命中的次数。
  BLOCK_CACHE_FILTER_MISS,
//从块缓存访问筛选器块时缓存命中的次数。
  BLOCK_CACHE_FILTER_HIT,
//添加到块缓存的筛选器块。
  BLOCK_CACHE_FILTER_ADD,
//插入缓存的Bloom筛选器块字节数
  BLOCK_CACHE_FILTER_BYTES_INSERT,
//从缓存中删除的Bloom筛选器块字节数
  BLOCK_CACHE_FILTER_BYTES_EVICT,
//从块缓存访问数据块时缓存未命中的次数。
  BLOCK_CACHE_DATA_MISS,
//从块缓存访问数据块时缓存命中的次数。
  BLOCK_CACHE_DATA_HIT,
//添加到块缓存的数据块。
  BLOCK_CACHE_DATA_ADD,
//插入缓存的数据块字节数
  BLOCK_CACHE_DATA_BYTES_INSERT,
//从缓存中读取的字节数。
  BLOCK_CACHE_BYTES_READ,
//写入缓存的字节数。
  BLOCK_CACHE_BYTES_WRITE,

//有时Bloom过滤器避免了文件读取。
  BLOOM_FILTER_USEFUL,

//持续缓存命中
  PERSISTENT_CACHE_HIT,
//持久缓存未命中
  PERSISTENT_CACHE_MISS,

//模拟块缓存命中总数
  SIM_BLOCK_CACHE_HIT,
//模拟块缓存未命中总数
  SIM_BLOCK_CACHE_MISS,

//Memtable点击量。
  MEMTABLE_HIT,
//未命中memtable。
  MEMTABLE_MISS,

//由l0提供的get（）查询
  GET_HIT_L0,
//一级服务的get（）查询的
  GET_HIT_L1,
//由二级及以上服务的get（）查询
  GET_HIT_L2_AND_UP,

  /*
   *压实过程中的落键原因
   *目前有4个原因。
   **/

COMPACTION_KEY_DROP_NEWER_ENTRY,  //密钥是用较新的值编写的。
//还包括删除范围del的键。
COMPACTION_KEY_DROP_OBSOLETE,     //密钥已过时。
COMPACTION_KEY_DROP_RANGE_DEL,    //钥匙被一个范围墓碑覆盖。
COMPACTION_KEY_DROP_USER,  //用户压缩函数已删除该键。
COMPACTION_RANGE_DEL_DROP_OBSOLETE,  //已删除范围中的所有密钥。
//由于文件间隙优化，删除在底层之前已过时。
  COMPACTION_OPTIMIZED_DEL_DROP_OBSOLETE,

//通过Put和Write调用写入数据库的密钥数
  NUMBER_KEYS_WRITTEN,
//读取的键数，
  NUMBER_KEYS_READ,
//如果启用就地更新，则更新数字键
  NUMBER_KEYS_UPDATED,
//db:：put（）、db:：delete（）发出的未压缩字节数，
//db:：merge（）和db:：write（）。
  BYTES_WRITTEN,
//从db:：get（）读取的未压缩字节数。它可能是
//来自memtables、cache或table文件。
//对于从db:：multiget（）读取的逻辑字节数，
//请使用读取的多个字节数。
  BYTES_READ,
//要查找/下一个/上一个的调用数
  NUMBER_DB_SEEK,
  NUMBER_DB_NEXT,
  NUMBER_DB_PREV,
//返回数据的Seek/Next/Prev调用数
  NUMBER_DB_SEEK_FOUND,
  NUMBER_DB_NEXT_FOUND,
  NUMBER_DB_PREV_FOUND,
//从迭代器读取的未压缩字节数。
//包括键和值的大小。
  ITER_BYTES_READ,
  NO_FILE_CLOSES,
  NO_FILE_OPENS,
  NO_FILE_ERRORS,
//过时的时间系统必须等待执行LO-L1压缩
  STALL_L0_SLOWDOWN_MICROS,
//已弃用时间系统必须等待将memtable移到l1。
  STALL_MEMTABLE_COMPACTION_MICROS,
//由于l0中的文件太多，已弃用写限制
  STALL_L0_NUM_FILES_MICROS,
//作者必须等待压缩或冲洗完成。
  STALL_MICROS,
//db mutex的等待时间。
//默认情况下禁用。要启用它，请将stats level设置为kall
  DB_MUTEX_WAIT_MICROS,
  RATE_LIMIT_DELAY_MILLIS,
NO_ITERATORS,  //当前打开的迭代器数

//多次获取调用、键读取和字节读取的数目
  NUMBER_MULTIGET_CALLS,
  NUMBER_MULTIGET_KEYS_READ,
  NUMBER_MULTIGET_BYTES_READ,

//不需要删除的记录数
//由于密钥不存在而写入存储
  NUMBER_FILTERED_DELETES,
  NUMBER_MERGE_FAILURES,

//在上创建迭代器之前检查Bloom的次数
//文件，以及检查在避免
//创建迭代器（因此可能是IOPS）。
  BLOOM_FILTER_PREFIX_CHECKED,
  BLOOM_FILTER_PREFIX_USEFUL,

//为了跳过，我们必须在迭代中重新搜索的次数
//具有相同用户密钥的大量密钥。
  NUMBER_OF_RESEEKS_IN_ITERATION,

//记录对getupadtessince的调用数。有助于跟踪
//事务日志迭代器刷新
  GET_UPDATES_SINCE_CALLS,
BLOCK_CACHE_COMPRESSED_MISS,  //在压缩块缓存中丢失
BLOCK_CACHE_COMPRESSED_HIT,   //在压缩块缓存中命中
//添加到压缩块缓存的块数
  BLOCK_CACHE_COMPRESSED_ADD,
//向压缩块缓存添加块时失败的次数
  BLOCK_CACHE_COMPRESSED_ADD_FAILURES,
WAL_FILE_SYNCED,  //完成wal同步的次数
WAL_FILE_BYTES,   //写入wal的字节数

//可以通过请求线程或线程在
//编剧队列的头。
  WRITE_DONE_BY_SELF,
WRITE_DONE_BY_OTHER,  //相当于为他人写的东西
WRITE_TIMEDOUT,       //以超时结束的写入次数。
WRITE_WITH_WAL,       //请求wal的写入呼叫数
COMPACT_READ_BYTES,   //压缩期间读取的字节数
COMPACT_WRITE_BYTES,  //压缩期间写入的字节数
FLUSH_WRITE_BYTES,    //刷新期间写入的字节数

//直接从文件加载而不创建的表属性数
//表读取器对象。
  NUMBER_DIRECT_LOAD_TABLE_PROPERTIES,
  NUMBER_SUPERVERSION_ACQUIRES,
  NUMBER_SUPERVERSION_RELEASES,
  NUMBER_SUPERVERSION_CLEANUPS,

//执行的压缩/减压
  NUMBER_BLOCK_COMPRESSED,
  NUMBER_BLOCK_DECOMPRESSED,

  NUMBER_BLOCK_NOT_COMPRESSED,
  MERGE_OPERATION_TOTAL_TIME,
  FILTER_OPERATION_TOTAL_TIME,

//行缓存。
  ROW_CACHE_HIT,
  ROW_CACHE_MISS,

//阅读放大统计。
//读放大可以用这个公式计算
//（读取总读取字节数/读取估计有用字节数）
//
//要求：readoptions：：启用每个位的读取字节数
READ_AMP_ESTIMATE_USEFUL_BYTES,  //估计实际使用的总字节数。
READ_AMP_TOTAL_READ_BYTES,       //已加载数据块的总大小。

//完全消耗速率限制器字节的重新填充间隔数。
  NUMBER_RATE_LIMITER_DRAINS,

  TICKER_ENUM_MAX
};

//票务中所列项目的顺序应与
//tickersnamemap中列出的顺序
const std::vector<std::pair<Tickers, std::string>> TickersNameMap = {
    {BLOCK_CACHE_MISS, "rocksdb.block.cache.miss"},
    {BLOCK_CACHE_HIT, "rocksdb.block.cache.hit"},
    {BLOCK_CACHE_ADD, "rocksdb.block.cache.add"},
    {BLOCK_CACHE_ADD_FAILURES, "rocksdb.block.cache.add.failures"},
    {BLOCK_CACHE_INDEX_MISS, "rocksdb.block.cache.index.miss"},
    {BLOCK_CACHE_INDEX_HIT, "rocksdb.block.cache.index.hit"},
    {BLOCK_CACHE_INDEX_ADD, "rocksdb.block.cache.index.add"},
    {BLOCK_CACHE_INDEX_BYTES_INSERT, "rocksdb.block.cache.index.bytes.insert"},
    {BLOCK_CACHE_INDEX_BYTES_EVICT, "rocksdb.block.cache.index.bytes.evict"},
    {BLOCK_CACHE_FILTER_MISS, "rocksdb.block.cache.filter.miss"},
    {BLOCK_CACHE_FILTER_HIT, "rocksdb.block.cache.filter.hit"},
    {BLOCK_CACHE_FILTER_ADD, "rocksdb.block.cache.filter.add"},
    {BLOCK_CACHE_FILTER_BYTES_INSERT,
     "rocksdb.block.cache.filter.bytes.insert"},
    {BLOCK_CACHE_FILTER_BYTES_EVICT, "rocksdb.block.cache.filter.bytes.evict"},
    {BLOCK_CACHE_DATA_MISS, "rocksdb.block.cache.data.miss"},
    {BLOCK_CACHE_DATA_HIT, "rocksdb.block.cache.data.hit"},
    {BLOCK_CACHE_DATA_ADD, "rocksdb.block.cache.data.add"},
    {BLOCK_CACHE_DATA_BYTES_INSERT, "rocksdb.block.cache.data.bytes.insert"},
    {BLOCK_CACHE_BYTES_READ, "rocksdb.block.cache.bytes.read"},
    {BLOCK_CACHE_BYTES_WRITE, "rocksdb.block.cache.bytes.write"},
    {BLOOM_FILTER_USEFUL, "rocksdb.bloom.filter.useful"},
    {PERSISTENT_CACHE_HIT, "rocksdb.persistent.cache.hit"},
    {PERSISTENT_CACHE_MISS, "rocksdb.persistent.cache.miss"},
    {SIM_BLOCK_CACHE_HIT, "rocksdb.sim.block.cache.hit"},
    {SIM_BLOCK_CACHE_MISS, "rocksdb.sim.block.cache.miss"},
    {MEMTABLE_HIT, "rocksdb.memtable.hit"},
    {MEMTABLE_MISS, "rocksdb.memtable.miss"},
    {GET_HIT_L0, "rocksdb.l0.hit"},
    {GET_HIT_L1, "rocksdb.l1.hit"},
    {GET_HIT_L2_AND_UP, "rocksdb.l2andup.hit"},
    {COMPACTION_KEY_DROP_NEWER_ENTRY, "rocksdb.compaction.key.drop.new"},
    {COMPACTION_KEY_DROP_OBSOLETE, "rocksdb.compaction.key.drop.obsolete"},
    {COMPACTION_KEY_DROP_RANGE_DEL, "rocksdb.compaction.key.drop.range_del"},
    {COMPACTION_KEY_DROP_USER, "rocksdb.compaction.key.drop.user"},
    {COMPACTION_RANGE_DEL_DROP_OBSOLETE,
      "rocksdb.compaction.range_del.drop.obsolete"},
    {COMPACTION_OPTIMIZED_DEL_DROP_OBSOLETE,
      "rocksdb.compaction.optimized.del.drop.obsolete"},
    {NUMBER_KEYS_WRITTEN, "rocksdb.number.keys.written"},
    {NUMBER_KEYS_READ, "rocksdb.number.keys.read"},
    {NUMBER_KEYS_UPDATED, "rocksdb.number.keys.updated"},
    {BYTES_WRITTEN, "rocksdb.bytes.written"},
    {BYTES_READ, "rocksdb.bytes.read"},
    {NUMBER_DB_SEEK, "rocksdb.number.db.seek"},
    {NUMBER_DB_NEXT, "rocksdb.number.db.next"},
    {NUMBER_DB_PREV, "rocksdb.number.db.prev"},
    {NUMBER_DB_SEEK_FOUND, "rocksdb.number.db.seek.found"},
    {NUMBER_DB_NEXT_FOUND, "rocksdb.number.db.next.found"},
    {NUMBER_DB_PREV_FOUND, "rocksdb.number.db.prev.found"},
    {ITER_BYTES_READ, "rocksdb.db.iter.bytes.read"},
    {NO_FILE_CLOSES, "rocksdb.no.file.closes"},
    {NO_FILE_OPENS, "rocksdb.no.file.opens"},
    {NO_FILE_ERRORS, "rocksdb.no.file.errors"},
    {STALL_L0_SLOWDOWN_MICROS, "rocksdb.l0.slowdown.micros"},
    {STALL_MEMTABLE_COMPACTION_MICROS, "rocksdb.memtable.compaction.micros"},
    {STALL_L0_NUM_FILES_MICROS, "rocksdb.l0.num.files.stall.micros"},
    {STALL_MICROS, "rocksdb.stall.micros"},
    {DB_MUTEX_WAIT_MICROS, "rocksdb.db.mutex.wait.micros"},
    {RATE_LIMIT_DELAY_MILLIS, "rocksdb.rate.limit.delay.millis"},
    {NO_ITERATORS, "rocksdb.num.iterators"},
    {NUMBER_MULTIGET_CALLS, "rocksdb.number.multiget.get"},
    {NUMBER_MULTIGET_KEYS_READ, "rocksdb.number.multiget.keys.read"},
    {NUMBER_MULTIGET_BYTES_READ, "rocksdb.number.multiget.bytes.read"},
    {NUMBER_FILTERED_DELETES, "rocksdb.number.deletes.filtered"},
    {NUMBER_MERGE_FAILURES, "rocksdb.number.merge.failures"},
    {BLOOM_FILTER_PREFIX_CHECKED, "rocksdb.bloom.filter.prefix.checked"},
    {BLOOM_FILTER_PREFIX_USEFUL, "rocksdb.bloom.filter.prefix.useful"},
    {NUMBER_OF_RESEEKS_IN_ITERATION, "rocksdb.number.reseeks.iteration"},
    {GET_UPDATES_SINCE_CALLS, "rocksdb.getupdatessince.calls"},
    {BLOCK_CACHE_COMPRESSED_MISS, "rocksdb.block.cachecompressed.miss"},
    {BLOCK_CACHE_COMPRESSED_HIT, "rocksdb.block.cachecompressed.hit"},
    {BLOCK_CACHE_COMPRESSED_ADD, "rocksdb.block.cachecompressed.add"},
    {BLOCK_CACHE_COMPRESSED_ADD_FAILURES,
     "rocksdb.block.cachecompressed.add.failures"},
    {WAL_FILE_SYNCED, "rocksdb.wal.synced"},
    {WAL_FILE_BYTES, "rocksdb.wal.bytes"},
    {WRITE_DONE_BY_SELF, "rocksdb.write.self"},
    {WRITE_DONE_BY_OTHER, "rocksdb.write.other"},
    {WRITE_TIMEDOUT, "rocksdb.write.timeout"},
    {WRITE_WITH_WAL, "rocksdb.write.wal"},
    {COMPACT_READ_BYTES, "rocksdb.compact.read.bytes"},
    {COMPACT_WRITE_BYTES, "rocksdb.compact.write.bytes"},
    {FLUSH_WRITE_BYTES, "rocksdb.flush.write.bytes"},
    {NUMBER_DIRECT_LOAD_TABLE_PROPERTIES,
     "rocksdb.number.direct.load.table.properties"},
    {NUMBER_SUPERVERSION_ACQUIRES, "rocksdb.number.superversion_acquires"},
    {NUMBER_SUPERVERSION_RELEASES, "rocksdb.number.superversion_releases"},
    {NUMBER_SUPERVERSION_CLEANUPS, "rocksdb.number.superversion_cleanups"},
    {NUMBER_BLOCK_COMPRESSED, "rocksdb.number.block.compressed"},
    {NUMBER_BLOCK_DECOMPRESSED, "rocksdb.number.block.decompressed"},
    {NUMBER_BLOCK_NOT_COMPRESSED, "rocksdb.number.block.not_compressed"},
    {MERGE_OPERATION_TOTAL_TIME, "rocksdb.merge.operation.time.nanos"},
    {FILTER_OPERATION_TOTAL_TIME, "rocksdb.filter.operation.time.nanos"},
    {ROW_CACHE_HIT, "rocksdb.row.cache.hit"},
    {ROW_CACHE_MISS, "rocksdb.row.cache.miss"},
    {READ_AMP_ESTIMATE_USEFUL_BYTES, "rocksdb.read.amp.estimate.useful.bytes"},
    {READ_AMP_TOTAL_READ_BYTES, "rocksdb.read.amp.total.read.bytes"},
    {NUMBER_RATE_LIMITER_DRAINS, "rocksdb.number.rate_limiter.drains"},
};

/*
 *继续添加柱状图。
 *任何柱状图的值都应小于柱状图枚举最大值。
 *通过分配直方图的当前值来添加一个新的直方图
 *在下面的histogramsnamemap中添加字符串表示形式
 *和增量直方图_Enum_Max
 *在Java API中添加相应的枚举值到CracracyType .java
 **/

enum Histograms : uint32_t {
  DB_GET = 0,
  DB_WRITE,
  COMPACTION_TIME,
  SUBCOMPACTION_SETUP_TIME,
  TABLE_SYNC_MICROS,
  COMPACTION_OUTFILE_SYNC_MICROS,
  WAL_FILE_SYNC_MICROS,
  MANIFEST_FILE_SYNC_MICROS,
//打开表期间在IO中花费的时间
  TABLE_OPEN_IO_MICROS,
  DB_MULTIGET,
  READ_BLOCK_COMPACTION_MICROS,
  READ_BLOCK_GET_MICROS,
  WRITE_RAW_BLOCK_MICROS,
  STALL_L0_SLOWDOWN_COUNT,
  STALL_MEMTABLE_COMPACTION_COUNT,
  STALL_L0_NUM_FILES_COUNT,
  HARD_RATE_LIMIT_DELAY_COUNT,
  SOFT_RATE_LIMIT_DELAY_COUNT,
  NUM_FILES_IN_SINGLE_COMPACTION,
  DB_SEEK,
  WRITE_STALL,
  SST_READ_MICROS,
//压缩期间实际计划的子压缩数
  NUM_SUBCOMPACTIONS_SCHEDULED,
//每个操作中的值大小分布
  BYTES_PER_READ,
  BYTES_PER_WRITE,
  BYTES_PER_MULTIGET,

//压缩/解压缩的字节数
//字节数是未压缩时的字节数；即分别在之前/之后
  BYTES_COMPRESSED,
  BYTES_DECOMPRESSED,
  COMPRESSION_TIMES_NANOS,
  DECOMPRESSION_TIMES_NANOS,
//在用户读取中传递给合并运算符的合并操作数
//请求。
  READ_NUM_MERGE_OPERANDS,

HISTOGRAM_ENUM_MAX,  //TODO（ldemailly）：强制histogramsnamemap匹配
};

const std::vector<std::pair<Histograms, std::string>> HistogramsNameMap = {
    {DB_GET, "rocksdb.db.get.micros"},
    {DB_WRITE, "rocksdb.db.write.micros"},
    {COMPACTION_TIME, "rocksdb.compaction.times.micros"},
    {SUBCOMPACTION_SETUP_TIME, "rocksdb.subcompaction.setup.times.micros"},
    {TABLE_SYNC_MICROS, "rocksdb.table.sync.micros"},
    {COMPACTION_OUTFILE_SYNC_MICROS, "rocksdb.compaction.outfile.sync.micros"},
    {WAL_FILE_SYNC_MICROS, "rocksdb.wal.file.sync.micros"},
    {MANIFEST_FILE_SYNC_MICROS, "rocksdb.manifest.file.sync.micros"},
    {TABLE_OPEN_IO_MICROS, "rocksdb.table.open.io.micros"},
    {DB_MULTIGET, "rocksdb.db.multiget.micros"},
    {READ_BLOCK_COMPACTION_MICROS, "rocksdb.read.block.compaction.micros"},
    {READ_BLOCK_GET_MICROS, "rocksdb.read.block.get.micros"},
    {WRITE_RAW_BLOCK_MICROS, "rocksdb.write.raw.block.micros"},
    {STALL_L0_SLOWDOWN_COUNT, "rocksdb.l0.slowdown.count"},
    {STALL_MEMTABLE_COMPACTION_COUNT, "rocksdb.memtable.compaction.count"},
    {STALL_L0_NUM_FILES_COUNT, "rocksdb.num.files.stall.count"},
    {HARD_RATE_LIMIT_DELAY_COUNT, "rocksdb.hard.rate.limit.delay.count"},
    {SOFT_RATE_LIMIT_DELAY_COUNT, "rocksdb.soft.rate.limit.delay.count"},
    {NUM_FILES_IN_SINGLE_COMPACTION, "rocksdb.numfiles.in.singlecompaction"},
    {DB_SEEK, "rocksdb.db.seek.micros"},
    {WRITE_STALL, "rocksdb.db.write.stall"},
    {SST_READ_MICROS, "rocksdb.sst.read.micros"},
    {NUM_SUBCOMPACTIONS_SCHEDULED, "rocksdb.num.subcompactions.scheduled"},
    {BYTES_PER_READ, "rocksdb.bytes.per.read"},
    {BYTES_PER_WRITE, "rocksdb.bytes.per.write"},
    {BYTES_PER_MULTIGET, "rocksdb.bytes.per.multiget"},
    {BYTES_COMPRESSED, "rocksdb.bytes.compressed"},
    {BYTES_DECOMPRESSED, "rocksdb.bytes.decompressed"},
    {COMPRESSION_TIMES_NANOS, "rocksdb.compression.times.nanos"},
    {DECOMPRESSION_TIMES_NANOS, "rocksdb.decompression.times.nanos"},
    {READ_NUM_MERGE_OPERANDS, "rocksdb.read.num.merge_operands"},
};

struct HistogramData {
  double median;
  double percentile95;
  double percentile99;
  double average;
  double standard_deviation;
//零初始化新成员，因为旧的统计信息：：HistogramData（）。
//实现不会编写它们。
  double max = 0.0;
};

enum StatsLevel {
//收集除mutex锁内时间和花在
//压缩。
  kExceptDetailedTimers,
//收集除需要在
//互斥锁。
  kExceptTimeForMutex,
//收集所有统计信息，包括测量互斥操作的持续时间。
//如果在平台上运行花费时间，它可以
//减少对更多线程的可伸缩性，尤其是对于写入。
  kAll,
};

//分析数据库的性能
class Statistics {
 public:
  virtual ~Statistics() {}

  virtual uint64_t getTickerCount(uint32_t tickerType) const = 0;
  virtual void histogramData(uint32_t type,
                             HistogramData* const data) const = 0;
  virtual std::string getHistogramString(uint32_t type) const { return ""; }
  virtual void recordTick(uint32_t tickerType, uint64_t count = 0) = 0;
  virtual void setTickerCount(uint32_t tickerType, uint64_t count) = 0;
  virtual uint64_t getAndResetTickerCount(uint32_t tickerType) = 0;
  virtual void measureTime(uint32_t histogramType, uint64_t time) = 0;

//重置所有断续器和柱状图统计
  virtual Status Reset() {
    return Status::NotSupported("Not implemented");
  }

//统计对象的字符串表示形式。
  virtual std::string ToString() const {
//默认情况下不执行任何操作
    return std::string("ToString(): not implemented");
  }

//重写此函数以禁用特定的柱状图收集
  virtual bool HistEnabledForType(uint32_t type) const {
    return type < HISTOGRAM_ENUM_MAX;
  }

  StatsLevel stats_level_ = kExceptDetailedTimers;
};

//创建具体的DBStatistics对象
std::shared_ptr<Statistics> CreateDBStatistics();

}  //命名空间rocksdb

#endif  //存储\u rocksdb_包括\u统计\u h_
