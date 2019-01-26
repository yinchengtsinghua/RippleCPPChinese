
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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#ifdef GFLAGS
#ifdef NUMA
#include <numa.h>
#include <numaif.h>
#endif
#ifndef OS_WIN
#include <unistd.h>
#endif
#include <fcntl.h>
#include <gflags/gflags.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

#include "db/db_impl.h"
#include "db/version_set.h"
#include "hdfs/env_hdfs.h"
#include "monitoring/histogram.h"
#include "monitoring/statistics.h"
#include "port/port.h"
#include "port/stack_trace.h"
#include "rocksdb/cache.h"
#include "rocksdb/db.h"
#include "rocksdb/env.h"
#include "rocksdb/filter_policy.h"
#include "rocksdb/memtablerep.h"
#include "rocksdb/options.h"
#include "rocksdb/perf_context.h"
#include "rocksdb/persistent_cache.h"
#include "rocksdb/rate_limiter.h"
#include "rocksdb/slice.h"
#include "rocksdb/slice_transform.h"
#include "rocksdb/utilities/object_registry.h"
#include "rocksdb/utilities/optimistic_transaction_db.h"
#include "rocksdb/utilities/options_util.h"
#include "rocksdb/utilities/sim_cache.h"
#include "rocksdb/utilities/transaction.h"
#include "rocksdb/utilities/transaction_db.h"
#include "rocksdb/write_batch.h"
#include "util/cast_util.h"
#include "util/compression.h"
#include "util/crc32c.h"
#include "util/mutexlock.h"
#include "util/random.h"
#include "util/stderr_logger.h"
#include "util/string_util.h"
#include "util/testutil.h"
#include "util/transaction_test_util.h"
#include "util/xxhash.h"
#include "utilities/blob_db/blob_db.h"
#include "utilities/merge_operators.h"
#include "utilities/persistent_cache/block_cache_tier.h"

#ifdef OS_WIN
#include <io.h>  //打开/关闭
#endif

using GFLAGS::ParseCommandLineFlags;
using GFLAGS::RegisterFlagValidator;
using GFLAGS::SetUsageMessage;

DEFINE_string(
    benchmarks,
    "fillseq,"
    "fillseqdeterministic,"
    "fillsync,"
    "fillrandom,"
    "filluniquerandomdeterministic,"
    "overwrite,"
    "readrandom,"
    "newiterator,"
    "newiteratorwhilewriting,"
    "seekrandom,"
    "seekrandomwhilewriting,"
    "seekrandomwhilemerging,"
    "readseq,"
    "readreverse,"
    "compact,"
    "compactall,"
    "readrandom,"
    "multireadrandom,"
    "readseq,"
    "readtocache,"
    "readreverse,"
    "readwhilewriting,"
    "readwhilemerging,"
    "readrandomwriterandom,"
    "updaterandom,"
    "randomwithverify,"
    "fill100K,"
    "crc32c,"
    "xxhash,"
    "compress,"
    "uncompress,"
    "acquireload,"
    "fillseekseq,"
    "randomtransaction,"
    "randomreplacekeys,"
    "timeseries",

    "Comma-separated list of operations to run in the specified"
    " order. Available benchmarks:\n"
    "\tfillseq       -- write N values in sequential key"
    " order in async mode\n"
    "\tfillseqdeterministic       -- write N values in the specified"
    " key order and keep the shape of the LSM tree\n"
    "\tfillrandom    -- write N values in random key order in async"
    " mode\n"
    "\tfilluniquerandomdeterministic       -- write N values in a random"
    " key order and keep the shape of the LSM tree\n"
    "\toverwrite     -- overwrite N values in random key order in"
    " async mode\n"
    "\tfillsync      -- write N/100 values in random key order in "
    "sync mode\n"
    "\tfill100K      -- write N/1000 100K values in random order in"
    " async mode\n"
    "\tdeleteseq     -- delete N keys in sequential order\n"
    "\tdeleterandom  -- delete N keys in random order\n"
    "\treadseq       -- read N times sequentially\n"
    "\treadtocache   -- 1 thread reading database sequentially\n"
    "\treadreverse   -- read N times in reverse order\n"
    "\treadrandom    -- read N times in random order\n"
    "\treadmissing   -- read N missing keys in random order\n"
    "\treadwhilewriting      -- 1 writer, N threads doing random "
    "reads\n"
    "\treadwhilemerging      -- 1 merger, N threads doing random "
    "reads\n"
    "\treadrandomwriterandom -- N threads doing random-read, "
    "random-write\n"
    "\tprefixscanrandom      -- prefix scan N times in random order\n"
    "\tupdaterandom  -- N threads doing read-modify-write for random "
    "keys\n"
    "\tappendrandom  -- N threads doing read-modify-write with "
    "growing values\n"
    "\tmergerandom   -- same as updaterandom/appendrandom using merge"
    " operator. "
    "Must be used with merge_operator\n"
    "\treadrandommergerandom -- perform N random read-or-merge "
    "operations. Must be used with merge_operator\n"
    "\tnewiterator   -- repeated iterator creation\n"
    "\tseekrandom    -- N random seeks, call Next seek_nexts times "
    "per seek\n"
    "\tseekrandomwhilewriting -- seekrandom and 1 thread doing "
    "overwrite\n"
    "\tseekrandomwhilemerging -- seekrandom and 1 thread doing "
    "merge\n"
    "\tcrc32c        -- repeated crc32c of 4K of data\n"
    "\txxhash        -- repeated xxHash of 4K of data\n"
    "\tacquireload   -- load N*1000 times\n"
    "\tfillseekseq   -- write N values in sequential key, then read "
    "them by seeking to each key\n"
    "\trandomtransaction     -- execute N random transactions and "
    "verify correctness\n"
    "\trandomreplacekeys     -- randomly replaces N keys by deleting "
    "the old version and putting the new version\n\n"
    "\ttimeseries            -- 1 writer generates time series data "
    "and multiple readers doing random reads on id\n\n"
    "Meta operations:\n"
    "\tcompact     -- Compact the entire DB; If multiple, randomly choose one\n"
    "\tcompactall  -- Compact the entire DB\n"
    "\tstats       -- Print DB stats\n"
    "\tresetstats  -- Reset DB stats\n"
    "\tlevelstats  -- Print the number of files and bytes per level\n"
    "\tsstables    -- Print sstable info\n"
    "\theapprofile -- Dump a heap profile (if supported by this"
    " port)\n");

DEFINE_int64(num, 1000000, "Number of key/values to place in database");

DEFINE_int64(numdistinct, 1000,
             "Number of distinct keys to use. Used in RandomWithVerify to "
             "read/write on fewer keys so that gets are more likely to find the"
             " key and puts are more likely to update the same key");

DEFINE_int64(merge_keys, -1,
             "Number of distinct keys to use for MergeRandom and "
             "ReadRandomMergeRandom. "
             "If negative, there will be FLAGS_num keys.");
DEFINE_int32(num_column_families, 1, "Number of Column Families to use.");

DEFINE_int32(
    num_hot_column_families, 0,
    "Number of Hot Column Families. If more than 0, only write to this "
    "number of column families. After finishing all the writes to them, "
    "create new set of column families and insert to them. Only used "
    "when num_column_families > 1.");

DEFINE_string(column_family_distribution, "",
              "Comma-separated list of percentages, where the ith element "
              "indicates the probability of an op using the ith column family. "
              "The number of elements must be `num_hot_column_families` if "
              "specified; otherwise, it must be `num_column_families`. The "
              "sum of elements must be 100. E.g., if `num_column_families=4`, "
              "and `num_hot_column_families=0`, a valid list could be "
              "\"10,20,30,40\".");

DEFINE_int64(reads, -1, "Number of read operations to do.  "
             "If negative, do FLAGS_num reads.");

DEFINE_int64(deletes, -1, "Number of delete operations to do.  "
             "If negative, do FLAGS_num deletions.");

DEFINE_int32(bloom_locality, 0, "Control bloom filter probes locality");

DEFINE_int64(seed, 0, "Seed base for random number generators. "
             "When 0 it is deterministic.");

DEFINE_int32(threads, 1, "Number of concurrent threads to run.");

DEFINE_int32(duration, 0, "Time in seconds for the random-ops tests to run."
             " When 0 then num & reads determine the test duration");

DEFINE_int32(value_size, 100, "Size of each value");

DEFINE_int32(seek_nexts, 0,
             "How many times to call Next() after Seek() in "
             "fillseekseq, seekrandom, seekrandomwhilewriting and "
             "seekrandomwhilemerging");

DEFINE_bool(reverse_iterator, false,
            "When true use Prev rather than Next for iterators that do "
            "Seek and then Next");

DEFINE_bool(use_uint64_comparator, false, "use Uint64 user comparator");

DEFINE_int64(batch_size, 1, "Batch size");

static bool ValidateKeySize(const char* flagname, int32_t value) {
  return true;
}

static bool ValidateUint32Range(const char* flagname, uint64_t value) {
  if (value > std::numeric_limits<uint32_t>::max()) {
    fprintf(stderr, "Invalid value for --%s: %lu, overflow\n", flagname,
            (unsigned long)value);
    return false;
  }
  return true;
}

DEFINE_int32(key_size, 16, "size of each key");

DEFINE_int32(num_multi_db, 0,
             "Number of DBs used in the benchmark. 0 means single DB.");

DEFINE_double(compression_ratio, 0.5, "Arrange to generate values that shrink"
              " to this fraction of their original size after compression");

DEFINE_double(read_random_exp_range, 0.0,
              "Read random's key will be generated using distribution of "
              "num * exp(-r) where r is uniform number from 0 to this value. "
              "The larger the number is, the more skewed the reads are. "
              "Only used in readrandom and multireadrandom benchmarks.");

DEFINE_bool(histogram, false, "Print histogram of operation timings");

DEFINE_bool(enable_numa, false,
            "Make operations aware of NUMA architecture and bind memory "
            "and cpus corresponding to nodes together. In NUMA, memory "
            "in same node as CPUs are closer when compared to memory in "
            "other nodes. Reads can be faster when the process is bound to "
            "CPU and memory of same node. Use \"$numactl --hardware\" command "
            "to see NUMA memory architecture.");

DEFINE_int64(db_write_buffer_size, rocksdb::Options().db_write_buffer_size,
             "Number of bytes to buffer in all memtables before compacting");

DEFINE_bool(cost_write_buffer_to_cache, false,
            "The usage of memtable is costed to the block cache");

DEFINE_int64(write_buffer_size, rocksdb::Options().write_buffer_size,
             "Number of bytes to buffer in memtable before compacting");

DEFINE_int32(max_write_buffer_number,
             rocksdb::Options().max_write_buffer_number,
             "The number of in-memory memtables. Each memtable is of size"
             "write_buffer_size.");

DEFINE_int32(min_write_buffer_number_to_merge,
             rocksdb::Options().min_write_buffer_number_to_merge,
             "The minimum number of write buffers that will be merged together"
             "before writing to storage. This is cheap because it is an"
             "in-memory merge. If this feature is not enabled, then all these"
             "write buffers are flushed to L0 as separate files and this "
             "increases read amplification because a get request has to check"
             " in all of these files. Also, an in-memory merge may result in"
             " writing less data to storage if there are duplicate records "
             " in each of these individual write buffers.");

DEFINE_int32(max_write_buffer_number_to_maintain,
             rocksdb::Options().max_write_buffer_number_to_maintain,
             "The total maximum number of write buffers to maintain in memory "
             "including copies of buffers that have already been flushed. "
             "Unlike max_write_buffer_number, this parameter does not affect "
             "flushing. This controls the minimum amount of write history "
             "that will be available in memory for conflict checking when "
             "Transactions are used. If this value is too low, some "
             "transactions may fail at commit time due to not being able to "
             "determine whether there were any write conflicts. Setting this "
             "value to 0 will cause write buffers to be freed immediately "
             "after they are flushed.  If this value is set to -1, "
             "'max_write_buffer_number' will be used.");

DEFINE_int32(max_background_jobs,
             rocksdb::Options().max_background_jobs,
             "The maximum number of concurrent background jobs that can occur "
             "in parallel.");

DEFINE_int32(num_bottom_pri_threads, 0,
             "The number of threads in the bottom-priority thread pool (used "
             "by universal compaction only).");

DEFINE_int32(num_high_pri_threads, 0,
             "The maximum number of concurrent background compactions"
             " that can occur in parallel.");

DEFINE_int32(num_low_pri_threads, 0,
             "The maximum number of concurrent background compactions"
             " that can occur in parallel.");

DEFINE_int32(max_background_compactions,
             rocksdb::Options().max_background_compactions,
             "The maximum number of concurrent background compactions"
             " that can occur in parallel.");

DEFINE_int32(base_background_compactions, -1, "DEPRECATED");

DEFINE_uint64(subcompactions, 1,
              "Maximum number of subcompactions to divide L0-L1 compactions "
              "into.");
static const bool FLAGS_subcompactions_dummy
    __attribute__((unused)) = RegisterFlagValidator(&FLAGS_subcompactions,
                                                    &ValidateUint32Range);

DEFINE_int32(max_background_flushes,
             rocksdb::Options().max_background_flushes,
             "The maximum number of concurrent background flushes"
             " that can occur in parallel.");

static rocksdb::CompactionStyle FLAGS_compaction_style_e;
DEFINE_int32(compaction_style, (int32_t) rocksdb::Options().compaction_style,
             "style of compaction: level-based, universal and fifo");

static rocksdb::CompactionPri FLAGS_compaction_pri_e;
DEFINE_int32(compaction_pri, (int32_t)rocksdb::Options().compaction_pri,
             "priority of files to compaction: by size or by data age");

DEFINE_int32(universal_size_ratio, 0,
             "Percentage flexibility while comparing file size"
             " (for universal compaction only).");

DEFINE_int32(universal_min_merge_width, 0, "The minimum number of files in a"
             " single compaction run (for universal compaction only).");

DEFINE_int32(universal_max_merge_width, 0, "The max number of files to compact"
             " in universal style compaction");

DEFINE_int32(universal_max_size_amplification_percent, 0,
             "The max size amplification for universal style compaction");

DEFINE_int32(universal_compression_size_percent, -1,
             "The percentage of the database to compress for universal "
             "compaction. -1 means compress everything.");

DEFINE_bool(universal_allow_trivial_move, false,
            "Allow trivial move in universal compaction.");

DEFINE_int64(cache_size, 8 << 20,  //8MB
             "Number of bytes to use as a cache of uncompressed data");

DEFINE_int32(cache_numshardbits, 6,
             "Number of shards for the block cache"
             " is 2 ** cache_numshardbits. Negative means use default settings."
             " This is applied only if FLAGS_cache_size is non-negative.");

DEFINE_double(cache_high_pri_pool_ratio, 0.0,
              "Ratio of block cache reserve for high pri blocks. "
              "If > 0.0, we also enable "
              "cache_index_and_filter_blocks_with_high_priority.");

DEFINE_bool(use_clock_cache, false,
            "Replace default LRU block cache with clock cache.");

DEFINE_int64(simcache_size, -1,
             "Number of bytes to use as a simcache of "
             "uncompressed data. Nagative value disables simcache.");

DEFINE_bool(cache_index_and_filter_blocks, false,
            "Cache index/filter blocks in block cache.");

DEFINE_bool(partition_index_and_filters, false,
            "Partition index and filter blocks.");

DEFINE_int64(metadata_block_size,
             rocksdb::BlockBasedTableOptions().metadata_block_size,
             "Max partition size when partitioning index/filters");

//默认设置减少了闪存读取时间的开销。用HDD，哪个
//但是，吞吐量要小得多，这个数字最好设置为1。
DEFINE_int32(ops_between_duration_checks, 1000,
             "Check duration limit every x ops");

DEFINE_bool(pin_l0_filter_and_index_blocks_in_cache, false,
            "Pin index/filter blocks of L0 files in block cache.");

DEFINE_int32(block_size,
             static_cast<int32_t>(rocksdb::BlockBasedTableOptions().block_size),
             "Number of bytes in a block.");

DEFINE_int32(block_restart_interval,
             rocksdb::BlockBasedTableOptions().block_restart_interval,
             "Number of keys between restart points "
             "for delta encoding of keys in data block.");

DEFINE_int32(index_block_restart_interval,
             rocksdb::BlockBasedTableOptions().index_block_restart_interval,
             "Number of keys between restart points "
             "for delta encoding of keys in index block.");

DEFINE_int32(read_amp_bytes_per_bit,
             rocksdb::BlockBasedTableOptions().read_amp_bytes_per_bit,
             "Number of bytes per bit to be used in block read-amp bitmap");

DEFINE_int64(compressed_cache_size, -1,
             "Number of bytes to use as a cache of compressed data.");

DEFINE_int64(row_cache_size, 0,
             "Number of bytes to use as a cache of individual rows"
             " (0 = disabled).");

DEFINE_int32(open_files, rocksdb::Options().max_open_files,
             "Maximum number of files to keep open at the same time"
             " (use default if == 0)");

DEFINE_int32(file_opening_threads, rocksdb::Options().max_file_opening_threads,
             "If open_files is set to -1, this option set the number of "
             "threads that will be used to open files during DB::Open()");

DEFINE_bool(new_table_reader_for_compaction_inputs, true,
             "If true, uses a separate file handle for compaction inputs");

DEFINE_int32(compaction_readahead_size, 0, "Compaction readahead size");

DEFINE_int32(random_access_max_buffer_size, 1024 * 1024,
             "Maximum windows randomaccess buffer size");

DEFINE_int32(writable_file_max_buffer_size, 1024 * 1024,
             "Maximum write buffer for Writable File");

DEFINE_int32(bloom_bits, -1, "Bloom filter bits per key. Negative means"
             " use default settings.");
DEFINE_double(memtable_bloom_size_ratio, 0,
              "Ratio of memtable size used for bloom filter. 0 means no bloom "
              "filter.");
DEFINE_bool(memtable_use_huge_page, false,
            "Try to use huge page in memtables.");

DEFINE_bool(use_existing_db, false, "If true, do not destroy the existing"
            " database.  If you set this flag and also specify a benchmark that"
            " wants a fresh database, that benchmark will fail.");

DEFINE_bool(show_table_properties, false,
            "If true, then per-level table"
            " properties will be printed on every stats-interval when"
            " stats_interval is set and stats_per_interval is on.");

DEFINE_string(db, "", "Use the db with the following name.");

//读取缓存标志

DEFINE_string(read_cache_path, "",
              "If not empty string, a read cache will be used in this path");

DEFINE_int64(read_cache_size, 4LL * 1024 * 1024 * 1024,
             "Maximum size of the read cache");

DEFINE_bool(read_cache_direct_write, true,
            "Whether to use Direct IO for writing to the read cache");

DEFINE_bool(read_cache_direct_read, true,
            "Whether to use Direct IO for reading from read cache");

static bool ValidateCacheNumshardbits(const char* flagname, int32_t value) {
  if (value >= 20) {
    fprintf(stderr, "Invalid value for --%s: %d, must be < 20\n",
            flagname, value);
    return false;
  }
  return true;
}

DEFINE_bool(verify_checksum, true,
            "Verify checksum for every block read"
            " from storage");

DEFINE_bool(statistics, false, "Database statistics");
DEFINE_string(statistics_string, "", "Serialized statistics string");
static class std::shared_ptr<rocksdb::Statistics> dbstats;

DEFINE_int64(writes, -1, "Number of write operations to do. If negative, do"
             " --num reads.");

DEFINE_bool(finish_after_writes, false, "Write thread terminates after all writes are finished");

DEFINE_bool(sync, false, "Sync all writes to disk");

DEFINE_bool(use_fsync, false, "If true, issue fsync instead of fdatasync");

DEFINE_bool(disable_wal, false, "If true, do not write WAL for write.");

DEFINE_string(wal_dir, "", "If not empty, use the given dir for WAL");

DEFINE_string(truth_db, "/dev/shm/truth_db/dbbench",
              "Truth key/values used when using verify");

DEFINE_int32(num_levels, 7, "The total number of levels");

DEFINE_int64(target_file_size_base, rocksdb::Options().target_file_size_base,
             "Target file size at level-1");

DEFINE_int32(target_file_size_multiplier,
             rocksdb::Options().target_file_size_multiplier,
             "A multiplier to compute target level-N file size (N >= 2)");

DEFINE_uint64(max_bytes_for_level_base,
              rocksdb::Options().max_bytes_for_level_base,
              "Max bytes for level-1");

DEFINE_bool(level_compaction_dynamic_level_bytes, false,
            "Whether level size base is dynamic");

DEFINE_double(max_bytes_for_level_multiplier, 10,
              "A multiplier to compute max bytes for level-N (N >= 2)");

static std::vector<int> FLAGS_max_bytes_for_level_multiplier_additional_v;
DEFINE_string(max_bytes_for_level_multiplier_additional, "",
              "A vector that specifies additional fanout per level");

DEFINE_int32(level0_stop_writes_trigger,
             rocksdb::Options().level0_stop_writes_trigger,
             "Number of files in level-0"
             " that will trigger put stop.");

DEFINE_int32(level0_slowdown_writes_trigger,
             rocksdb::Options().level0_slowdown_writes_trigger,
             "Number of files in level-0"
             " that will slow down writes.");

DEFINE_int32(level0_file_num_compaction_trigger,
             rocksdb::Options().level0_file_num_compaction_trigger,
             "Number of files in level-0"
             " when compactions start");

static bool ValidateInt32Percent(const char* flagname, int32_t value) {
  if (value <= 0 || value>=100) {
    fprintf(stderr, "Invalid value for --%s: %d, 0< pct <100 \n",
            flagname, value);
    return false;
  }
  return true;
}
DEFINE_int32(readwritepercent, 90, "Ratio of reads to reads/writes (expressed"
             " as percentage) for the ReadRandomWriteRandom workload. The "
             "default value 90 means 90% operations out of all reads and writes"
             " operations are reads. In other words, 9 gets for every 1 put.");

DEFINE_int32(mergereadpercent, 70, "Ratio of merges to merges&reads (expressed"
             " as percentage) for the ReadRandomMergeRandom workload. The"
             " default value 70 means 70% out of all read and merge operations"
             " are merges. In other words, 7 merges for every 3 gets.");

DEFINE_int32(deletepercent, 2, "Percentage of deletes out of reads/writes/"
             "deletes (used in RandomWithVerify only). RandomWithVerify "
             "calculates writepercent as (100 - FLAGS_readwritepercent - "
             "deletepercent), so deletepercent must be smaller than (100 - "
             "FLAGS_readwritepercent)");

DEFINE_bool(optimize_filters_for_hits, false,
            "Optimizes bloom filters for workloads for most lookups return "
            "a value. For now this doesn't create bloom filters for the max "
            "level of the LSM to reduce metadata that should fit in RAM. ");

DEFINE_uint64(delete_obsolete_files_period_micros, 0,
              "Ignored. Left here for backward compatibility");

DEFINE_int64(writes_per_range_tombstone, 0,
             "Number of writes between range "
             "tombstones");

DEFINE_int64(range_tombstone_width, 100, "Number of keys in tombstone's range");

DEFINE_int64(max_num_range_tombstones, 0,
             "Maximum number of range tombstones "
             "to insert.");

DEFINE_bool(expand_range_tombstones, false,
            "Expand range tombstone into sequential regular tombstones.");

#ifndef ROCKSDB_LITE
DEFINE_bool(optimistic_transaction_db, false,
            "Open a OptimisticTransactionDB instance. "
            "Required for randomtransaction benchmark.");

DEFINE_bool(use_blob_db, false,
            "Open a BlobDB instance. "
            "Required for largevalue benchmark.");

DEFINE_bool(transaction_db, false,
            "Open a TransactionDB instance. "
            "Required for randomtransaction benchmark.");

DEFINE_uint64(transaction_sets, 2,
              "Number of keys each transaction will "
              "modify (use in RandomTransaction only).  Max: 9999");

DEFINE_bool(transaction_set_snapshot, false,
            "Setting to true will have each transaction call SetSnapshot()"
            " upon creation.");

DEFINE_int32(transaction_sleep, 0,
             "Max microseconds to sleep in between "
             "reading and writing a value (used in RandomTransaction only). ");

DEFINE_uint64(transaction_lock_timeout, 100,
              "If using a transaction_db, specifies the lock wait timeout in"
              " milliseconds before failing a transaction waiting on a lock");
DEFINE_string(
    options_file, "",
    "The path to a RocksDB options file.  If specified, then db_bench will "
    "run with the RocksDB options in the default column family of the "
    "specified options file. "
    "Note that with this setting, db_bench will ONLY accept the following "
    "RocksDB options related command-line arguments, all other arguments "
    "that are related to RocksDB options will be ignored:\n"
    "\t--use_existing_db\n"
    "\t--statistics\n"
    "\t--row_cache_size\n"
    "\t--row_cache_numshardbits\n"
    "\t--enable_io_prio\n"
    "\t--dump_malloc_stats\n"
    "\t--num_multi_db\n");

DEFINE_uint64(fifo_compaction_max_table_files_size_mb, 0,
              "The limit of total table file sizes to trigger FIFO compaction");
DEFINE_bool(fifo_compaction_allow_compaction, true,
            "Allow compaction in FIFO compaction.");
DEFINE_uint64(fifo_compaction_ttl, 0, "TTL for the SST Files in seconds.");
#endif  //摇滚乐

DEFINE_bool(report_bg_io_stats, false,
            "Measure times spents on I/Os while in compactions. ");

DEFINE_bool(use_stderr_info_logger, false,
            "Write info logs to stderr instead of to LOG file. ");

static enum rocksdb::CompressionType StringToCompressionType(const char* ctype) {
  assert(ctype);

  if (!strcasecmp(ctype, "none"))
    return rocksdb::kNoCompression;
  else if (!strcasecmp(ctype, "snappy"))
    return rocksdb::kSnappyCompression;
  else if (!strcasecmp(ctype, "zlib"))
    return rocksdb::kZlibCompression;
  else if (!strcasecmp(ctype, "bzip2"))
    return rocksdb::kBZip2Compression;
  else if (!strcasecmp(ctype, "lz4"))
    return rocksdb::kLZ4Compression;
  else if (!strcasecmp(ctype, "lz4hc"))
    return rocksdb::kLZ4HCCompression;
  else if (!strcasecmp(ctype, "xpress"))
    return rocksdb::kXpressCompression;
  else if (!strcasecmp(ctype, "zstd"))
    return rocksdb::kZSTD;

  fprintf(stdout, "Cannot parse compression type '%s'\n", ctype);
return rocksdb::kSnappyCompression;  //默认值
}

static std::string ColumnFamilyName(size_t i) {
  if (i == 0) {
    return rocksdb::kDefaultColumnFamilyName;
  } else {
    char name[100];
    snprintf(name, sizeof(name), "column_family_name_%06zu", i);
    return std::string(name);
  }
}

DEFINE_string(compression_type, "snappy",
              "Algorithm to use to compress the database");
static enum rocksdb::CompressionType FLAGS_compression_type_e =
    rocksdb::kSnappyCompression;

DEFINE_int32(compression_level, -1,
             "Compression level. For zlib this should be -1 for the "
             "default level, or between 0 and 9.");

DEFINE_int32(compression_max_dict_bytes, 0,
             "Maximum size of dictionary used to prime the compression "
             "library.");

static bool ValidateCompressionLevel(const char* flagname, int32_t value) {
  if (value < -1 || value > 9) {
    fprintf(stderr, "Invalid value for --%s: %d, must be between -1 and 9\n",
            flagname, value);
    return false;
  }
  return true;
}

static const bool FLAGS_compression_level_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_compression_level, &ValidateCompressionLevel);

DEFINE_int32(min_level_to_compress, -1, "If non-negative, compression starts"
             " from this level. Levels with number < min_level_to_compress are"
             " not compressed. Otherwise, apply compression_type to "
             "all levels.");

static bool ValidateTableCacheNumshardbits(const char* flagname,
                                           int32_t value) {
  if (0 >= value || value > 20) {
    fprintf(stderr, "Invalid value for --%s: %d, must be  0 < val <= 20\n",
            flagname, value);
    return false;
  }
  return true;
}
DEFINE_int32(table_cache_numshardbits, 4, "");

#ifndef ROCKSDB_LITE
DEFINE_string(env_uri, "", "URI for registry Env lookup. Mutually exclusive"
              " with --hdfs.");
#endif  //摇滚乐
DEFINE_string(hdfs, "", "Name of hdfs environment. Mutually exclusive with"
              " --env_uri.");
static rocksdb::Env* FLAGS_env = rocksdb::Env::Default();

DEFINE_int64(stats_interval, 0, "Stats are reported every N operations when "
             "this is greater than zero. When 0 the interval grows over time.");

DEFINE_int64(stats_interval_seconds, 0, "Report stats every N seconds. This "
             "overrides stats_interval when both are > 0.");

DEFINE_int32(stats_per_interval, 0, "Reports additional stats per interval when"
             " this is greater than 0.");

DEFINE_int64(report_interval_seconds, 0,
             "If greater than zero, it will write simple stats in CVS format "
             "to --report_file every N seconds");

DEFINE_string(report_file, "report.csv",
              "Filename where some simple stats are reported to (if "
              "--report_interval_seconds is bigger than 0)");

DEFINE_int32(thread_status_per_interval, 0,
             "Takes and report a snapshot of the current status of each thread"
             " when this is greater than 0.");

DEFINE_int32(perf_level, rocksdb::PerfLevel::kDisable, "Level of perf collection");

static bool ValidateRateLimit(const char* flagname, double value) {
  const double EPSILON = 1e-10;
  if ( value < -EPSILON ) {
    fprintf(stderr, "Invalid value for --%s: %12.6f, must be >= 0.0\n",
            flagname, value);
    return false;
  }
  return true;
}
DEFINE_double(soft_rate_limit, 0.0, "DEPRECATED");

DEFINE_double(hard_rate_limit, 0.0, "DEPRECATED");

DEFINE_uint64(soft_pending_compaction_bytes_limit, 64ull * 1024 * 1024 * 1024,
              "Slowdown writes if pending compaction bytes exceed this number");

DEFINE_uint64(hard_pending_compaction_bytes_limit, 128ull * 1024 * 1024 * 1024,
              "Stop writes if pending compaction bytes exceed this number");

DEFINE_uint64(delayed_write_rate, 8388608u,
              "Limited bytes allowed to DB when soft_rate_limit or "
              "level0_slowdown_writes_trigger triggers");

DEFINE_bool(enable_pipelined_write, true,
            "Allow WAL and memtable writes to be pipelined");

DEFINE_bool(allow_concurrent_memtable_write, true,
            "Allow multi-writers to update mem tables in parallel.");

DEFINE_bool(enable_write_thread_adaptive_yield, true,
            "Use a yielding spin loop for brief writer thread waits.");

DEFINE_uint64(
    write_thread_max_yield_usec, 100,
    "Maximum microseconds for enable_write_thread_adaptive_yield operation.");

DEFINE_uint64(write_thread_slow_yield_usec, 3,
              "The threshold at which a slow yield is considered a signal that "
              "other processes or threads want the core.");

DEFINE_int32(rate_limit_delay_max_milliseconds, 1000,
             "When hard_rate_limit is set then this is the max time a put will"
             " be stalled.");

DEFINE_uint64(rate_limiter_bytes_per_sec, 0, "Set options.rate_limiter value.");

DEFINE_bool(rate_limit_bg_reads, false,
            "Use options.rate_limiter on compaction reads");

DEFINE_uint64(
    benchmark_write_rate_limit, 0,
    "If non-zero, db_bench will rate-limit the writes going into RocksDB. This "
    "is the global rate in bytes/second.");

DEFINE_uint64(
    benchmark_read_rate_limit, 0,
    "If non-zero, db_bench will rate-limit the reads from RocksDB. This "
    "is the global rate in ops/second.");

DEFINE_uint64(max_compaction_bytes, rocksdb::Options().max_compaction_bytes,
              "Max bytes allowed in one compaction");

#ifndef ROCKSDB_LITE
DEFINE_bool(readonly, false, "Run read only benchmarks.");
#endif  //摇滚乐

DEFINE_bool(disable_auto_compactions, false, "Do not auto trigger compactions");

DEFINE_uint64(wal_ttl_seconds, 0, "Set the TTL for the WAL Files in seconds.");
DEFINE_uint64(wal_size_limit_MB, 0, "Set the size limit for the WAL Files"
              " in MB.");
DEFINE_uint64(max_total_wal_size, 0, "Set total max WAL size");

DEFINE_bool(mmap_read, rocksdb::Options().allow_mmap_reads,
            "Allow reads to occur via mmap-ing files");

DEFINE_bool(mmap_write, rocksdb::Options().allow_mmap_writes,
            "Allow writes to occur via mmap-ing files");

DEFINE_bool(use_direct_reads, rocksdb::Options().use_direct_reads,
            "Use O_DIRECT for reading data");

DEFINE_bool(use_direct_io_for_flush_and_compaction,
            rocksdb::Options().use_direct_io_for_flush_and_compaction,
            "Use O_DIRECT for background flush and compaction I/O");

DEFINE_bool(advise_random_on_open, rocksdb::Options().advise_random_on_open,
            "Advise random access on table file open");

DEFINE_string(compaction_fadvice, "NORMAL",
              "Access pattern advice when a file is compacted");
static auto FLAGS_compaction_fadvice_e =
  rocksdb::Options().access_hint_on_compaction_start;

DEFINE_bool(use_tailing_iterator, false,
            "Use tailing iterator to access a series of keys instead of get");

DEFINE_bool(use_adaptive_mutex, rocksdb::Options().use_adaptive_mutex,
            "Use adaptive mutex");

DEFINE_uint64(bytes_per_sync,  rocksdb::Options().bytes_per_sync,
              "Allows OS to incrementally sync SST files to disk while they are"
              " being written, in the background. Issue one request for every"
              " bytes_per_sync written. 0 turns it off.");

DEFINE_uint64(wal_bytes_per_sync,  rocksdb::Options().wal_bytes_per_sync,
              "Allows OS to incrementally sync WAL files to disk while they are"
              " being written, in the background. Issue one request for every"
              " wal_bytes_per_sync written. 0 turns it off.");

DEFINE_bool(use_single_deletes, true,
            "Use single deletes (used in RandomReplaceKeys only).");

DEFINE_double(stddev, 2000.0,
              "Standard deviation of normal distribution used for picking keys"
              " (used in RandomReplaceKeys only).");

DEFINE_int32(key_id_range, 100000,
             "Range of possible value of key id (used in TimeSeries only).");

DEFINE_string(expire_style, "none",
              "Style to remove expired time entries. Can be one of the options "
              "below: none (do not expired data), compaction_filter (use a "
              "compaction filter to remove expired data), delete (seek IDs and "
              "remove expired data) (used in TimeSeries only).");

DEFINE_uint64(
    time_range, 100000,
    "Range of timestamp that store in the database (used in TimeSeries"
    " only).");

DEFINE_int32(num_deletion_threads, 1,
             "Number of threads to do deletion (used in TimeSeries and delete "
             "expire_style only).");

DEFINE_int32(max_successive_merges, 0, "Maximum number of successive merge"
             " operations on a key in the memtable");

static bool ValidatePrefixSize(const char* flagname, int32_t value) {
  if (value < 0 || value>=2000000000) {
    fprintf(stderr, "Invalid value for --%s: %d. 0<= PrefixSize <=2000000000\n",
            flagname, value);
    return false;
  }
  return true;
}
DEFINE_int32(prefix_size, 0, "control the prefix size for HashSkipList and "
             "plain table");
DEFINE_int64(keys_per_prefix, 0, "control average number of keys generated "
             "per prefix, 0 means no special handling of the prefix, "
             "i.e. use the prefix comes with the generated random number.");
DEFINE_int32(memtable_insert_with_hint_prefix_size, 0,
             "If non-zero, enable "
             "memtable insert with hint with the given prefix size.");
DEFINE_bool(enable_io_prio, false, "Lower the background flush/compaction "
            "threads' IO priority");
DEFINE_bool(identity_as_first_hash, false, "the first hash function of cuckoo "
            "table becomes an identity function. This is only valid when key "
            "is 8 bytes");
DEFINE_bool(dump_malloc_stats, true, "Dump malloc stats in LOG ");

enum RepFactory {
  kSkipList,
  kPrefixHash,
  kVectorRep,
  kHashLinkedList,
  kCuckoo
};

static enum RepFactory StringToRepFactory(const char* ctype) {
  assert(ctype);

  if (!strcasecmp(ctype, "skip_list"))
    return kSkipList;
  else if (!strcasecmp(ctype, "prefix_hash"))
    return kPrefixHash;
  else if (!strcasecmp(ctype, "vector"))
    return kVectorRep;
  else if (!strcasecmp(ctype, "hash_linkedlist"))
    return kHashLinkedList;
  else if (!strcasecmp(ctype, "cuckoo"))
    return kCuckoo;

  fprintf(stdout, "Cannot parse memreptable %s\n", ctype);
  return kSkipList;
}

static enum RepFactory FLAGS_rep_factory;
DEFINE_string(memtablerep, "skip_list", "");
DEFINE_int64(hash_bucket_count, 1024 * 1024, "hash bucket count");
DEFINE_bool(use_plain_table, false, "if use plain table "
            "instead of block-based table format");
DEFINE_bool(use_cuckoo_table, false, "if use cuckoo table format");
DEFINE_double(cuckoo_hash_ratio, 0.9, "Hash ratio for Cuckoo SST table.");
DEFINE_bool(use_hash_search, false, "if use kHashSearch "
            "instead of kBinarySearch. "
            "This is valid if only we use BlockTable");
DEFINE_bool(use_block_based_filter, false, "if use kBlockBasedFilter "
            "instead of kFullFilter for filter block. "
            "This is valid if only we use BlockTable");
DEFINE_string(merge_operator, "", "The merge operator to use with the database."
              "If a new merge operator is specified, be sure to use fresh"
              " database The possible merge operators are defined in"
              " utilities/merge_operators.h");
DEFINE_int32(skip_list_lookahead, 0, "Used with skip_list memtablerep; try "
             "linear search first for this many steps from the previous "
             "position");
DEFINE_bool(report_file_operations, false, "if report number of file "
            "operations");

static const bool FLAGS_soft_rate_limit_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_soft_rate_limit, &ValidateRateLimit);

static const bool FLAGS_hard_rate_limit_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_hard_rate_limit, &ValidateRateLimit);

static const bool FLAGS_prefix_size_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_prefix_size, &ValidatePrefixSize);

static const bool FLAGS_key_size_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_key_size, &ValidateKeySize);

static const bool FLAGS_cache_numshardbits_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_cache_numshardbits,
                          &ValidateCacheNumshardbits);

static const bool FLAGS_readwritepercent_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_readwritepercent, &ValidateInt32Percent);

DEFINE_int32(disable_seek_compaction, false,
             "Not used, left here for backwards compatibility");

static const bool FLAGS_deletepercent_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_deletepercent, &ValidateInt32Percent);
static const bool FLAGS_table_cache_numshardbits_dummy __attribute__((unused)) =
    RegisterFlagValidator(&FLAGS_table_cache_numshardbits,
                          &ValidateTableCacheNumshardbits);

namespace rocksdb {

namespace {
struct ReportFileOpCounters {
  std::atomic<int> open_counter_;
  std::atomic<int> read_counter_;
  std::atomic<int> append_counter_;
  std::atomic<uint64_t> bytes_read_;
  std::atomic<uint64_t> bytes_written_;
};

//数据库工作台记录和报告文件操作的特殊环境
class ReportFileOpEnv : public EnvWrapper {
 public:
  explicit ReportFileOpEnv(Env* base) : EnvWrapper(base) { reset(); }

  void reset() {
    counters_.open_counter_ = 0;
    counters_.read_counter_ = 0;
    counters_.append_counter_ = 0;
    counters_.bytes_read_ = 0;
    counters_.bytes_written_ = 0;
  }

  Status NewSequentialFile(const std::string& f, unique_ptr<SequentialFile>* r,
                           const EnvOptions& soptions) override {
    class CountingFile : public SequentialFile {
     private:
      unique_ptr<SequentialFile> target_;
      ReportFileOpCounters* counters_;

     public:
      CountingFile(unique_ptr<SequentialFile>&& target,
                   ReportFileOpCounters* counters)
          : target_(std::move(target)), counters_(counters) {}

      virtual Status Read(size_t n, Slice* result, char* scratch) override {
        counters_->read_counter_.fetch_add(1, std::memory_order_relaxed);
        Status rv = target_->Read(n, result, scratch);
        counters_->bytes_read_.fetch_add(result->size(),
                                         std::memory_order_relaxed);
        return rv;
      }

      virtual Status Skip(uint64_t n) override { return target_->Skip(n); }
    };

    Status s = target()->NewSequentialFile(f, r, soptions);
    if (s.ok()) {
      counters()->open_counter_.fetch_add(1, std::memory_order_relaxed);
      r->reset(new CountingFile(std::move(*r), counters()));
    }
    return s;
  }

  Status NewRandomAccessFile(const std::string& f,
                             unique_ptr<RandomAccessFile>* r,
                             const EnvOptions& soptions) override {
    class CountingFile : public RandomAccessFile {
     private:
      unique_ptr<RandomAccessFile> target_;
      ReportFileOpCounters* counters_;

     public:
      CountingFile(unique_ptr<RandomAccessFile>&& target,
                   ReportFileOpCounters* counters)
          : target_(std::move(target)), counters_(counters) {}
      virtual Status Read(uint64_t offset, size_t n, Slice* result,
                          char* scratch) const override {
        counters_->read_counter_.fetch_add(1, std::memory_order_relaxed);
        Status rv = target_->Read(offset, n, result, scratch);
        counters_->bytes_read_.fetch_add(result->size(),
                                         std::memory_order_relaxed);
        return rv;
      }
    };

    Status s = target()->NewRandomAccessFile(f, r, soptions);
    if (s.ok()) {
      counters()->open_counter_.fetch_add(1, std::memory_order_relaxed);
      r->reset(new CountingFile(std::move(*r), counters()));
    }
    return s;
  }

  Status NewWritableFile(const std::string& f, unique_ptr<WritableFile>* r,
                         const EnvOptions& soptions) override {
    class CountingFile : public WritableFile {
     private:
      unique_ptr<WritableFile> target_;
      ReportFileOpCounters* counters_;

     public:
      CountingFile(unique_ptr<WritableFile>&& target,
                   ReportFileOpCounters* counters)
          : target_(std::move(target)), counters_(counters) {}

      Status Append(const Slice& data) override {
        counters_->append_counter_.fetch_add(1, std::memory_order_relaxed);
        Status rv = target_->Append(data);
        counters_->bytes_written_.fetch_add(data.size(),
                                            std::memory_order_relaxed);
        return rv;
      }

      Status Truncate(uint64_t size) override { return target_->Truncate(size); }
      Status Close() override { return target_->Close(); }
      Status Flush() override { return target_->Flush(); }
      Status Sync() override { return target_->Sync(); }
    };

    Status s = target()->NewWritableFile(f, r, soptions);
    if (s.ok()) {
      counters()->open_counter_.fetch_add(1, std::memory_order_relaxed);
      r->reset(new CountingFile(std::move(*r), counters()));
    }
    return s;
  }

//吸气剂
  ReportFileOpCounters* counters() { return &counters_; }

 private:
  ReportFileOpCounters counters_;
};

}  //命名空间

//帮助快速生成随机数据。
class RandomGenerator {
 private:
  std::string data_;
  unsigned int pos_;

 public:
  RandomGenerator() {
//我们反复使用有限数量的数据，并确保
//它大于压缩窗口（32KB），并且
//足够大以满足我们想要写入的所有典型值大小。
    Random rnd(301);
    std::string piece;
    while (data_.size() < (unsigned)std::max(1048576, FLAGS_value_size)) {
//添加可按指定压缩的短片段
//通过标记压缩比。
      test::CompressibleString(&rnd, FLAGS_compression_ratio, 100, &piece);
      data_.append(piece);
    }
    pos_ = 0;
  }

  Slice Generate(unsigned int len) {
    assert(len <= data_.size());
    if (pos_ + len > data_.size()) {
      pos_ = 0;
    }
    pos_ += len;
    return Slice(data_.data() + pos_ - len, len);
  }

  Slice GenerateWithTTL(unsigned int len) {
    assert(len <= data_.size());
    if (pos_ + len > data_.size()) {
      pos_ = 0;
    }
    pos_ += len;
    return Slice(data_.data() + pos_ - len, len);
  }
};

static void AppendWithSpace(std::string* str, Slice msg) {
  if (msg.empty()) return;
  if (!str->empty()) {
    str->push_back(' ');
  }
  str->append(msg.data(), msg.size());
}

struct DBWithColumnFamilies {
  std::vector<ColumnFamilyHandle*> cfh;
  DB* db;
#ifndef ROCKSDB_LITE
  OptimisticTransactionDB* opt_txn_db;
#endif  //摇滚乐
std::atomic<size_t> num_created;  //毕竟需要更新
//设置cfh中的新条目。
size_t num_hot;  //每时每刻要查询的列族数。
//在每个createnewcf（）之后，另一个num_hot number of new
//将创建列族并用于查询。
port::Mutex create_cf_mutex;  //只有一个线程可以执行createnewcf（）。
std::vector<int> cfh_idx_to_prob;  //指数保持操作概率
//关于CFH [I]。

  DBWithColumnFamilies()
      : db(nullptr)
#ifndef ROCKSDB_LITE
        , opt_txn_db(nullptr)
#endif  //摇滚乐
  {
    cfh.clear();
    num_created = 0;
    num_hot = 0;
  }

  DBWithColumnFamilies(const DBWithColumnFamilies& other)
      : cfh(other.cfh),
        db(other.db),
#ifndef ROCKSDB_LITE
        opt_txn_db(other.opt_txn_db),
#endif  //摇滚乐
        num_created(other.num_created.load()),
        num_hot(other.num_hot),
        cfh_idx_to_prob(other.cfh_idx_to_prob) {
  }

  void DeleteDBs() {
    std::for_each(cfh.begin(), cfh.end(),
                  [](ColumnFamilyHandle* cfhi) { delete cfhi; });
    cfh.clear();
#ifndef ROCKSDB_LITE
    if (opt_txn_db) {
      delete opt_txn_db;
      opt_txn_db = nullptr;
    } else {
      delete db;
      db = nullptr;
    }
#else
    delete db;
    db = nullptr;
#endif  //摇滚乐
  }

  ColumnFamilyHandle* GetCfh(int64_t rand_num) {
    assert(num_hot > 0);
    size_t rand_offset = 0;
    if (!cfh_idx_to_prob.empty()) {
      assert(cfh_idx_to_prob.size() == num_hot);
      int sum = 0;
      while (sum + cfh_idx_to_prob[rand_offset] < rand_num % 100) {
        sum += cfh_idx_to_prob[rand_offset];
        ++rand_offset;
      }
      assert(rand_offset < cfh_idx_to_prob.size());
    } else {
      rand_offset = rand_num % num_hot;
    }
    return cfh[num_created.load(std::memory_order_acquire) - num_hot +
               rand_offset];
  }

//阶段：假设已创建从0到阶段*num_hot的CF。需要创造
//阶段*num_hot+1到阶段*（num_hot+1）。
  void CreateNewCf(ColumnFamilyOptions options, int64_t stage) {
    MutexLock l(&create_cf_mutex);
    if ((stage + 1) * num_hot <= num_created) {
//已创建。
      return;
    }
    auto new_num_created = num_created + num_hot;
    assert(new_num_created <= cfh.size());
    for (size_t i = num_created; i < new_num_created; i++) {
      Status s =
          db->CreateColumnFamily(options, ColumnFamilyName(i), &(cfh[i]));
      if (!s.ok()) {
        fprintf(stderr, "create column family error: %s\n",
                s.ToString().c_str());
        abort();
      }
    }
    num_created.store(new_num_created, std::memory_order_release);
  }
};

//向csv文件报告统计信息的类
class ReporterAgent {
 public:
  ReporterAgent(Env* env, const std::string& fname,
                uint64_t report_interval_secs)
      : env_(env),
        total_ops_done_(0),
        last_report_(0),
        report_interval_secs_(report_interval_secs),
        stop_(false) {
    auto s = env_->NewWritableFile(fname, &report_file_, EnvOptions());
    if (s.ok()) {
      s = report_file_->Append(Header() + "\n");
    }
    if (s.ok()) {
      s = report_file_->Flush();
    }
    if (!s.ok()) {
      fprintf(stderr, "Can't open %s: %s\n", fname.c_str(),
              s.ToString().c_str());
      abort();
    }

    reporting_thread_ = port::Thread([&]() { SleepAndReport(); });
  }

  ~ReporterAgent() {
    {
      std::unique_lock<std::mutex> lk(mutex_);
      stop_ = true;
      stop_cv_.notify_all();
    }
    reporting_thread_.join();
  }

//线程安全
  void ReportFinishedOps(int64_t num_ops) {
    total_ops_done_.fetch_add(num_ops);
  }

 private:
  std::string Header() const { return "secs_elapsed,interval_qps"; }
  void SleepAndReport() {
    uint64_t kMicrosInSecond = 1000 * 1000;
    auto time_started = env_->NowMicros();
    while (true) {
      {
        std::unique_lock<std::mutex> lk(mutex_);
        if (stop_ ||
            stop_cv_.wait_for(lk, std::chrono::seconds(report_interval_secs_),
                              [&]() { return stop_; })) {
//停止
          break;
        }
//否则->超时，这意味着报告的时间！
      }
      auto total_ops_done_snapshot = total_ops_done_.load();
//以秒计
      auto secs_elapsed =
          (env_->NowMicros() - time_started + kMicrosInSecond / 2) /
          kMicrosInSecond;
      std::string report = ToString(secs_elapsed) + "," +
                           ToString(total_ops_done_snapshot - last_report_) +
                           "\n";
      auto s = report_file_->Append(report);
      if (s.ok()) {
        s = report_file_->Flush();
      }
      if (!s.ok()) {
        fprintf(stderr,
                "Can't write to report file (%s), stopping the reporting\n",
                s.ToString().c_str());
        break;
      }
      last_report_ = total_ops_done_snapshot;
    }
  }

  Env* env_;
  std::unique_ptr<WritableFile> report_file_;
  std::atomic<int64_t> total_ops_done_;
  int64_t last_report_;
  const uint64_t report_interval_secs_;
  rocksdb::port::Thread reporting_thread_;
  std::mutex mutex_;
//将在停止时通知
  std::condition_variable stop_cv_;
  bool stop_;
};

enum OperationType : unsigned char {
  kRead = 0,
  kWrite,
  kDelete,
  kSeek,
  kMerge,
  kUpdate,
  kCompress,
  kUncompress,
  kCrc,
  kHash,
  kOthers
};

static std::unordered_map<OperationType, std::string, std::hash<unsigned char>>
                          OperationTypeString = {
  {kRead, "read"},
  {kWrite, "write"},
  {kDelete, "delete"},
  {kSeek, "seek"},
  {kMerge, "merge"},
  {kUpdate, "update"},
  {kCompress, "compress"},
  {kCompress, "uncompress"},
  {kCrc, "crc"},
  {kHash, "hash"},
  {kOthers, "op"}
};

class CombinedStats;
class Stats {
 private:
  int id_;
  uint64_t start_;
  uint64_t finish_;
  double seconds_;
  uint64_t done_;
  uint64_t last_report_done_;
  uint64_t next_report_;
  uint64_t bytes_;
  uint64_t last_op_finish_;
  uint64_t last_report_finish_;
  std::unordered_map<OperationType, std::shared_ptr<HistogramImpl>,
                     std::hash<unsigned char>> hist_;
  std::string message_;
  bool exclude_from_merge_;
ReporterAgent* reporter_agent_;  //不拥有
  friend class CombinedStats;

 public:
  Stats() { Start(-1); }

  void SetReporterAgent(ReporterAgent* reporter_agent) {
    reporter_agent_ = reporter_agent;
  }

  void Start(int id) {
    id_ = id;
    next_report_ = FLAGS_stats_interval ? FLAGS_stats_interval : 100;
    last_op_finish_ = start_;
    hist_.clear();
    done_ = 0;
    last_report_done_ = 0;
    bytes_ = 0;
    seconds_ = 0;
    start_ = FLAGS_env->NowMicros();
    finish_ = start_;
    last_report_finish_ = start_;
    message_.clear();
//设置后，此线程的状态将不会与其他线程合并。
    exclude_from_merge_ = false;
  }

  void Merge(const Stats& other) {
    if (other.exclude_from_merge_)
      return;

    for (auto it = other.hist_.begin(); it != other.hist_.end(); ++it) {
      auto this_it = hist_.find(it->first);
      if (this_it != hist_.end()) {
        this_it->second->Merge(*(other.hist_.at(it->first)));
      } else {
        hist_.insert({ it->first, it->second });
      }
    }

    done_ += other.done_;
    bytes_ += other.bytes_;
    seconds_ += other.seconds_;
    if (other.start_ < start_) start_ = other.start_;
    if (other.finish_ > finish_) finish_ = other.finish_;

//只需保持消息来自一个线程
    if (message_.empty()) message_ = other.message_;
  }

  void Stop() {
    finish_ = FLAGS_env->NowMicros();
    seconds_ = (finish_ - start_) * 1e-6;
  }

  void AddMessage(Slice msg) {
    AppendWithSpace(&message_, msg);
  }

  void SetId(int id) { id_ = id; }
  void SetExcludeFromMerge() { exclude_from_merge_ = true; }

  void PrintThreadStatus() {
    std::vector<ThreadStatus> thread_list;
    FLAGS_env->GetThreadList(&thread_list);

    fprintf(stderr, "\n%18s %10s %12s %20s %13s %45s %12s %s\n",
        "ThreadID", "ThreadType", "cfName", "Operation",
        "ElapsedTime", "Stage", "State", "OperationProperties");

    int64_t current_time = 0;
    Env::Default()->GetCurrentTime(&current_time);
    for (auto ts : thread_list) {
      fprintf(stderr, "%18" PRIu64 " %10s %12s %20s %13s %45s %12s",
          ts.thread_id,
          ThreadStatus::GetThreadTypeName(ts.thread_type).c_str(),
          ts.cf_name.c_str(),
          ThreadStatus::GetOperationName(ts.operation_type).c_str(),
          ThreadStatus::MicrosToString(ts.op_elapsed_micros).c_str(),
          ThreadStatus::GetOperationStageName(ts.operation_stage).c_str(),
          ThreadStatus::GetStateName(ts.state_type).c_str());

      auto op_properties = ThreadStatus::InterpretOperationProperties(
          ts.operation_type, ts.op_properties);
      for (const auto& op_prop : op_properties) {
        fprintf(stderr, " %s %" PRIu64" |",
            op_prop.first.c_str(), op_prop.second);
      }
      fprintf(stderr, "\n");
    }
  }

  void ResetLastOpTime() {
//设置为“现在”，以避免从呼叫到SleepForMicroseconds的延迟
    last_op_finish_ = FLAGS_env->NowMicros();
  }

  void FinishedOps(DBWithColumnFamilies* db_with_cfh, DB* db, int64_t num_ops,
                   enum OperationType op_type = kOthers) {
    if (reporter_agent_) {
      reporter_agent_->ReportFinishedOps(num_ops);
    }
    if (FLAGS_histogram) {
      uint64_t now = FLAGS_env->NowMicros();
      uint64_t micros = now - last_op_finish_;

      if (hist_.find(op_type) == hist_.end())
      {
        auto hist_temp = std::make_shared<HistogramImpl>();
        hist_.insert({op_type, std::move(hist_temp)});
      }
      hist_[op_type]->Add(micros);

      if (micros > 20000 && !FLAGS_stats_interval) {
        fprintf(stderr, "long op: %" PRIu64 " micros%30s\r", micros, "");
        fflush(stderr);
      }
      last_op_finish_ = now;
    }

    done_ += num_ops;
    if (done_ >= next_report_) {
      if (!FLAGS_stats_interval) {
        if      (next_report_ < 1000)   next_report_ += 100;
        else if (next_report_ < 5000)   next_report_ += 500;
        else if (next_report_ < 10000)  next_report_ += 1000;
        else if (next_report_ < 50000)  next_report_ += 5000;
        else if (next_report_ < 100000) next_report_ += 10000;
        else if (next_report_ < 500000) next_report_ += 50000;
        else                            next_report_ += 100000;
        fprintf(stderr, "... finished %" PRIu64 " ops%30s\r", done_, "");
      } else {
        uint64_t now = FLAGS_env->NowMicros();
        int64_t usecs_since_last = now - last_report_finish_;

//确定是否打印间隔为
//每N次操作或每N秒。

        if (FLAGS_stats_interval_seconds &&
            usecs_since_last < (FLAGS_stats_interval_seconds * 1000000)) {
//不要再检查这么多操作
          next_report_ += FLAGS_stats_interval;

        } else {

          fprintf(stderr,
                  "%s ... thread %d: (%" PRIu64 ",%" PRIu64 ") ops and "
                  "(%.1f,%.1f) ops/second in (%.6f,%.6f) seconds\n",
                  FLAGS_env->TimeToString(now/1000000).c_str(),
                  id_,
                  done_ - last_report_done_, done_,
                  (done_ - last_report_done_) /
                  (usecs_since_last / 1000000.0),
                  done_ / ((now - start_) / 1000000.0),
                  (now - last_report_finish_) / 1000000.0,
                  (now - start_) / 1000000.0);

          if (id_ == 0 && FLAGS_stats_per_interval) {
            std::string stats;

            if (db_with_cfh && db_with_cfh->num_created.load()) {
              for (size_t i = 0; i < db_with_cfh->num_created.load(); ++i) {
                if (db->GetProperty(db_with_cfh->cfh[i], "rocksdb.cfstats",
                                    &stats))
                  fprintf(stderr, "%s\n", stats.c_str());
                if (FLAGS_show_table_properties) {
                  for (int level = 0; level < FLAGS_num_levels; ++level) {
                    if (db->GetProperty(
                            db_with_cfh->cfh[i],
                            "rocksdb.aggregated-table-properties-at-level" +
                                ToString(level),
                            &stats)) {
                      if (stats.find("# entries=0") == std::string::npos) {
                        fprintf(stderr, "Level[%d]: %s\n", level,
                                stats.c_str());
                      }
                    }
                  }
                }
              }
            } else if (db) {
              if (db->GetProperty("rocksdb.stats", &stats)) {
                fprintf(stderr, "%s\n", stats.c_str());
              }
              if (FLAGS_show_table_properties) {
                for (int level = 0; level < FLAGS_num_levels; ++level) {
                  if (db->GetProperty(
                          "rocksdb.aggregated-table-properties-at-level" +
                              ToString(level),
                          &stats)) {
                    if (stats.find("# entries=0") == std::string::npos) {
                      fprintf(stderr, "Level[%d]: %s\n", level, stats.c_str());
                    }
                  }
                }
              }
            }
          }

          next_report_ += FLAGS_stats_interval;
          last_report_finish_ = now;
          last_report_done_ = done_;
        }
      }
      if (id_ == 0 && FLAGS_thread_status_per_interval) {
        PrintThreadStatus();
      }
      fflush(stderr);
    }
  }

  void AddBytes(int64_t n) {
    bytes_ += n;
  }

  void Report(const Slice& name) {
//假设至少进行了一次操作，以防我们正在运行基准测试
//不调用finishedops（）。
    if (done_ < 1) done_ = 1;

    std::string extra;
    if (bytes_ > 0) {
//速率是根据实际运行时间计算的，而不是每个线程的总和
//逝去的时光。
      double elapsed = (finish_ - start_) * 1e-6;
      char rate[100];
      snprintf(rate, sizeof(rate), "%6.1f MB/s",
               (bytes_ / 1048576.0) / elapsed);
      extra = rate;
    }
    AppendWithSpace(&extra, message_);
    double elapsed = (finish_ - start_) * 1e-6;
    double throughput = (double)done_/elapsed;

    fprintf(stdout, "%-12s : %11.3f micros/op %ld ops/sec;%s%s\n",
            name.ToString().c_str(),
            elapsed * 1e6 / done_,
            (long)throughput,
            (extra.empty() ? "" : " "),
            extra.c_str());
    if (FLAGS_histogram) {
      for (auto it = hist_.begin(); it != hist_.end(); ++it) {
        fprintf(stdout, "Microseconds per %s:\n%s\n",
                OperationTypeString[it->first].c_str(),
                it->second->ToString().c_str());
      }
    }
    if (FLAGS_report_file_operations) {
      ReportFileOpEnv* env = static_cast<ReportFileOpEnv*>(FLAGS_env);
      ReportFileOpCounters* counters = env->counters();
      fprintf(stdout, "Num files opened: %d\n",
              counters->open_counter_.load(std::memory_order_relaxed));
      fprintf(stdout, "Num Read(): %d\n",
              counters->read_counter_.load(std::memory_order_relaxed));
      fprintf(stdout, "Num Append(): %d\n",
              counters->append_counter_.load(std::memory_order_relaxed));
      fprintf(stdout, "Num bytes read: %" PRIu64 "\n",
              counters->bytes_read_.load(std::memory_order_relaxed));
      fprintf(stdout, "Num bytes written: %" PRIu64 "\n",
              counters->bytes_written_.load(std::memory_order_relaxed));
      env->reset();
    }
    fflush(stdout);
  }
};

class CombinedStats {
 public:
  void AddStats(const Stats& stat) {
    uint64_t total_ops = stat.done_;
    uint64_t total_bytes_ = stat.bytes_;
    double elapsed;

    if (total_ops < 1) {
      total_ops = 1;
    }

    elapsed = (stat.finish_ - stat.start_) * 1e-6;
    throughput_ops_.emplace_back(total_ops / elapsed);

    if (total_bytes_ > 0) {
      double mbs = (total_bytes_ / 1048576.0);
      throughput_mbs_.emplace_back(mbs / elapsed);
    }
  }

  void Report(const std::string& bench_name) {
    const char* name = bench_name.c_str();
    int num_runs = static_cast<int>(throughput_ops_.size());

    if (throughput_mbs_.size() == throughput_ops_.size()) {
      fprintf(stdout,
              "%s [AVG    %d runs] : %d ops/sec; %6.1f MB/sec\n"
              "%s [MEDIAN %d runs] : %d ops/sec; %6.1f MB/sec\n",
              name, num_runs, static_cast<int>(CalcAvg(throughput_ops_)),
              CalcAvg(throughput_mbs_), name, num_runs,
              static_cast<int>(CalcMedian(throughput_ops_)),
              CalcMedian(throughput_mbs_));
    } else {
      fprintf(stdout,
              "%s [AVG    %d runs] : %d ops/sec\n"
              "%s [MEDIAN %d runs] : %d ops/sec\n",
              name, num_runs, static_cast<int>(CalcAvg(throughput_ops_)), name,
              num_runs, static_cast<int>(CalcMedian(throughput_ops_)));
    }
  }

 private:
  double CalcAvg(std::vector<double> data) {
    double avg = 0;
    for (double x : data) {
      avg += x;
    }
    avg = avg / data.size();
    return avg;
  }

  double CalcMedian(std::vector<double> data) {
    assert(data.size() > 0);
    std::sort(data.begin(), data.end());

    size_t mid = data.size() / 2;
    if (data.size() % 2 == 1) {
//奇数个条目
      return data[mid];
    } else {
//偶数项
      return (data[mid] + data[mid - 1]) / 2;
    }
  }

  std::vector<double> throughput_ops_;
  std::vector<double> throughput_mbs_;
};

class TimestampEmulator {
 private:
  std::atomic<uint64_t> timestamp_;

 public:
  TimestampEmulator() : timestamp_(0) {}
  uint64_t Get() const { return timestamp_.load(); }
  void Inc() { timestamp_++; }
};

//由同一基准的所有并发执行共享的状态。
struct SharedState {
  port::Mutex mu;
  port::CondVar cv;
  int total;
  int perf_level;
  std::shared_ptr<RateLimiter> write_rate_limiter;
  std::shared_ptr<RateLimiter> read_rate_limiter;

//每个线程都会经历以下状态：
//（1）初始化
//（2）等待其他初始化
//（3）运行
//（4）完成

  long num_initialized;
  long num_done;
  bool start;

  SharedState() : cv(&mu), perf_level(FLAGS_perf_level) { }
};

//同一基准的并发执行的每个线程状态。
struct ThreadState {
int tid;             //在n个线程中运行时为0..n-1
Random64 rand;         //不同的线有不同的种子
  Stats stats;
  SharedState* shared;

  /*隐式*/threadstate（int index）
      TID（索引），
        （旗子）兰德？标志_seed:1000）+索引）
  }
}；

课程持续时间
 公众：
  持续时间（uint64_t max_seconds，int64_t max_ops，int64_t ops_per_stage=0）
    max_seconds_uu=最大_seconds；
    最大运作=最大运作；
    每个阶段的操作数=（每个阶段的操作数>0）？操作每级：最大操作；
    OPSY＝0；
    启动_at_=flags_env->nowmicros（）；
  }

  int64_t getstage（）返回std:：min（ops_uu，max_u ops_u1）/ops_per_u stage_

  bool done（int64_t increment）
    if（increment<=0）increment=1；//避免完成（0）和无限循环
    ops_+=增量；

    如果（最长时间）
      //每进行一次APPX 1000次操作复查（精确的iff增量是1000的系数）
      自动粒度=标记“持续时间”检查之间的“操作”；
      如果（（操作粒度）！=（（操作增量/粒度））；
        uint64_t now=flags_env->nowmicros（）；
        返回（（现在-开始uu）/1000000）>=最大u秒u；
      }否则{
        返回错误；
      }
    }否则{
      返回ops_u>max_ops_u；
    }
  }

 私人：
  uint64_t max_s_u；
  Int64最大值
  Int64_t ops_per_Stage_；
  In 64；
  uint64_t在_uu开始_u；
}；

类基准
 私人：
  std:：shared_ptr<cache>cache_uu；
  std:：shared_ptr<cache>compressed_cache_uuu；
  std:：shared_ptr<const filter policy>filter_policy；
  const slicetransform*前缀提取程序
  DBwithColumnFamilies数据库
  std:：vector<dbwithColumnFamilies>multi_dbs_u；
  In 64；
  int值\大小\；
  It Kythsisiz；
  int前缀\大小\；
  int64_t keys_per_prefix_uu；
  Int64_t entries_per_batch_；
  Int64_t写入_per_range_tombstone_u；
  Int64_t range_tombstone_width_uuu；
  int64_t max_num_range_tombstones_u；
  写选项写选项
  options open_options_；//保留选项以在以后正确销毁db
  In 64×T读数；
  Int64删除
  双读“随机范围”；
  Int64写
  Int64读写
  Int64合并键；
  bool报告文件操作；
  布尔用法

  bool sanitycheck（）
    if（标志压缩比>1）
      fprintf（stderr，“压缩比应介于0和1之间”）；
      返回错误；
    }
    回归真实；
  }

  inline bool compressslice（const slice&input，std:：string*compressed）
    BOOL OK = TRUE；
    开关（旗标_compression_type_e）
      case rocksdb:：ksnappy压缩：
        确定=快速压缩（选项（）。压缩选择，输入.data（），
                             input.size（），压缩）；
        断裂；
      箱岩sdb:：kzlibcompression：
        确定=zlib_compress（options（）.compression_opts，2，input.data（），
                           input.size（），压缩）；
        断裂；
      case rocksdb:：kbzip2压缩：
        确定=bzip2_compress（options（）.compression_opts，2，input.data（），
                            input.size（），压缩）；
        断裂；
      箱岩sdb:：klz4压缩：
        OK=lz4_compress（options（）.compression_opts，2，input.data（），
                          input.size（），压缩）；
        断裂；
      箱岩sdb:：klz4hc压缩：
        确定=lz4hc_compress（options（）.compression_opts，2，input.data（），
                            input.size（），压缩）；
        断裂；
      case rocksDB:：kXpressCompression：
        OK=Xpress压缩（input.data（），
          input.size（），压缩）；
        断裂；
      箱岩sdb:：kzsdd：
        确定=zstd_compress（options（）.compression_opts，input.data（），
                           input.size（），压缩）；
        断裂；
      违约：
        OK =假；
    }
    返回OK；
  }

  void printHeader（）
    printEnvironment（）；
    fprintf（stdout，“密钥：每个字节%d”，标志\u key \u大小）；
    fprintf（stdout，“每个值为%d字节（压缩后为%d字节））”，
            标记值大小，
            static_cast<int>（flags_value_size*flags_compression_ratio+0.5））；
    fprintf（stdout，“entries:%”priu64“\n”，num_uuu）；
    fprintf（stdout，“前缀：%d字节\n”，标志\u前缀\u大小）；
    fprintf（stdout，“每个前缀的键数：”%“priu64”\n“，每个前缀的键数”；
    fprintf（stdout，“rawsize:%.1f mb（估计值）\n”，
            （（static_cast<int64_t>（flags_key_size+flags_value_size）*num_uuu）
             / 1048576）；
    fprintf（stdout，“文件大小：%.1f mb（估计值）”\n“，
            （标记\u key \u size+标记\u value \u size*标记\u compression \u ratio）
              * NUMYM）
             / 1048576）；
    fprintf（stdout，“写入速率：%”priu64“字节/秒\n”，
            标记“基准”写入“利率”限制）；
    fprintf（stdout，“读取速率：%”priu64“操作/秒\n”，
            标记“基准”读取“利率”限制）；
    if（标记_启用_numa）
      fprintf（stderr，“在numa启用模式下运行。\n”）；
γIFNDEF NUMA
      fprintf（stderr，“系统中未定义numa。\n”）；
      退出（1）；
否则
      如果（numa_available（）=-1）
        fprintf（stderr，“系统不支持numa。\n”）；
        退出（1）；
      }
第二节
    }

    自动压缩=压缩类型到字符串（标记压缩类型）；
    fprintf（stdout，“压缩：%s\n”，compression.c_str（））；

    开关（标志工厂）
      案例Kprefixhash：
        fprintf（stdout，“memtablerep:prefix_hash \n”）；
        断裂；
      凯斯普利斯特：
        fprintf（stdout，“memtablerep:skip_list\n”）；
        断裂；
      案例Kvectorep：
        fprintf（stdout，“memtablerep:vector\n”）；
        断裂；
      KhashLinkedList案例：
        fprintf（stdout，“memtablerep:hash_linkedlist\n”）；
        断裂；
      凯布库：
        fprintf（stdout，“memtablerep:布谷鸟\n”）；
        断裂；
    }
    fprintf（stdout，“性能级别：%d\n”，标志“性能级别”）；

    打印警告（compression.c_str（））；
    fprintf（stdout，“---------------------------------------------------\n”）；
  }

  void printwarnings（const char*压缩）
如果定义了（GNUC）&！定义（uuu optimize_uuuu）
    FPTCTF（STDUT）
            “警告：优化被禁用：基准点速度不必要地缓慢\n”
            ；
第二节
nIFUDEF NDEUG
    FPTCTF（STDUT）
            “警告：断言已启用；基准点速度不必要地变慢\n”）；
第二节
    如果（标记压缩类型）=rocksdb:：knocompression）
      //测试字符串不能太小。
      const int len=标志块大小；
      std：：字符串输入_str（len，'y'）；
      std：：字符串已压缩；
      bool result=compressslice（slice（input_str），&compressed）；

      如果（！）结果）{
        fprintf（stdout，“警告：未启用压缩\n”，
                压缩）；
      else if（compressed.size（）>=input_str.size（））
        fprintf（stdout，“警告：%s压缩无效\n”，
                压缩）；
      }
    }
  }

//当前，以下内容不等同于操作系统Linux。
如果定义了（Linux）
  静态切片修剪空间（切片S）
    无符号int start=0；
    while（start<s.size（）&&isspace（s[start]））
      启动++；
    }
    unsigned int limit=static_cast<unsigned int>（s.size（））；
    while（limit>start&&isspace（s[limit-1]））
      极限-
    }
    返回切片（s.data（）+start，limit-start）；
  }
第二节

  void printenvironment（）
    fprintf（stderr，“rocksdb:版本%d.%d\n”，
            kmajorversion、kminorversion）；

如果定义了（Linux）
    time_t now=时间（nullptr）；
    CHARBUF〔52〕；
    //lint抱怨ctime（）的用法，因此将其替换为ctime_（）。这个
    //要求提供至少26个字节的缓冲区。
    fprintf（stderr，“日期：%s”，
            ctime_r（&now，buf））；//ctime_r（）添加换行符

    file*cpuinfo=fopen（“/proc/cpuinfo”，“r”）；
    如果（CPUFIN！= null pTr）{
      字符行[1000]；
      int num_cpus=0；
      std：：string cpu_类型；
      std：：字符串缓存\大小；
      同时（fgets（line，sizeof（line），cpuinfo）！= null pTr）{
        const char*sep=strchr（行'：'）；
        如果（sep==nullptr）
          继续；
        }
        slice key=trimspace（slice（line，sep-1-line））；
        slice val=trimspace（slice（sep+1））；
        if（key=“型号名称”）；
          +NUncPcPs；
          cpu_type=val.toString（）；
        else if（key==“缓存大小”）
          cache_size=val.toString（）；
        }
      }
      fclose（cpuinfo）；
      fprintf（stderr，“CPU:%d*%s\n”，num_cpu s，cpu_type.c_str（））；
      fprintf（stderr，“cpucache:%s\n”，cache_size.c_str（））；
    }
第二节
  }

  静态bool keyexpired（const timestamp emulator*timestamp_emulator，
                         常量切片和键）
    const char*pos=key.data（）；
    POS＋＝8；
    uint64_t timestamp=0；
    如果（端口：：Klittleendian）
      int字节\到\填充=8；
      对于（int i=0；i<bytes_to_fill；+i）
        timestamp=（static_cast<uint64_t>（static_cast<unsigned char>（pos[i]））
                      <（（bytes_to_fill-i-1）<<3））；
      }
    }否则{
      memcpy（&timestamp，pos，sizeof（timestamp））；
    }
    返回timestamp_emulator->get（）-timestamp>标志_time_range；
  }

  类ExpiredTimeFilter:公共CompactionFilter_
   公众：
    显式ExpiredTimeFilter（
        const std:：shared_ptr<timestamp emulator>和timestamp_emulator）
        ：Timestamp_Emulator_u（Timestamp_Emulator）
    布尔滤波器（int-level、const-slice&key、const-slice和existing-value，
                std：：string*new_value，bool*value_changed）const override_
      返回keyExpired（timestamp_emulator_u.get（），key）；
    }
    const char*name（）const override返回“expiredTimeFilter”；

   私人：
    std:：shared_ptr<timestamp emulator>timestamp_emulator；
  }；

  std:：shared_ptr<cache>newcache（int64_t capacity）
    如果（容量<=0）
      返回null pTR；
    }
    if（标记使用时钟缓存）
      auto cache=newclockcache（（size_t）容量，标志\u cache_numshardbits）；
      如果（！）缓存）{
        fprintf（stderr，“不支持时钟缓存。”）；
        退出（1）；
      }
      返回高速缓存；
    }否则{
      返回newlrucache（（size_t）容量，标志\u cache_numshardbits，
                         错误/*严格的容量限制*/,

                         FLAGS_cache_high_pri_pool_ratio);
    }
  }

 public:
  Benchmark()
      : cache_(NewCache(FLAGS_cache_size)),
        compressed_cache_(NewCache(FLAGS_compressed_cache_size)),
        filter_policy_(FLAGS_bloom_bits >= 0
                           ? NewBloomFilterPolicy(FLAGS_bloom_bits,
                                                  FLAGS_use_block_based_filter)
                           : nullptr),
        prefix_extractor_(NewFixedPrefixTransform(FLAGS_prefix_size)),
        num_(FLAGS_num),
        value_size_(FLAGS_value_size),
        key_size_(FLAGS_key_size),
        prefix_size_(FLAGS_prefix_size),
        keys_per_prefix_(FLAGS_keys_per_prefix),
        entries_per_batch_(1),
        reads_(FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads),
        read_random_exp_range_(0.0),
        writes_(FLAGS_writes < 0 ? FLAGS_num : FLAGS_writes),
        readwrites_(
            (FLAGS_writes < 0 && FLAGS_reads < 0)
                ? FLAGS_num
                : ((FLAGS_writes > FLAGS_reads) ? FLAGS_writes : FLAGS_reads)),
        merge_keys_(FLAGS_merge_keys < 0 ? FLAGS_num : FLAGS_merge_keys),
        report_file_operations_(FLAGS_report_file_operations),
#ifndef ROCKSDB_LITE
        use_blob_db_(FLAGS_use_blob_db) {
#else
        use_blob_db_(false) {
#endif  //！摇滚乐
//使用Simcache而不是缓存
    if (FLAGS_simcache_size >= 0) {
      if (FLAGS_cache_numshardbits >= 1) {
        cache_ =
            NewSimCache(cache_, FLAGS_simcache_size, FLAGS_cache_numshardbits);
      } else {
        cache_ = NewSimCache(cache_, FLAGS_simcache_size, 0);
      }
    }

    if (report_file_operations_) {
      if (!FLAGS_hdfs.empty()) {
        fprintf(stderr,
                "--hdfs and --report_file_operations cannot be enabled "
                "at the same time");
        exit(1);
      }
      FLAGS_env = new ReportFileOpEnv(rocksdb::Env::Default());
    }

    if (FLAGS_prefix_size > FLAGS_key_size) {
      fprintf(stderr, "prefix size is larger than key size");
      exit(1);
    }

    std::vector<std::string> files;
    FLAGS_env->GetChildren(FLAGS_db, &files);
    for (size_t i = 0; i < files.size(); i++) {
      if (Slice(files[i]).starts_with("heap-")) {
        FLAGS_env->DeleteFile(FLAGS_db + "/" + files[i]);
      }
    }
    if (!FLAGS_use_existing_db) {
      Options options;
      if (!FLAGS_wal_dir.empty()) {
        options.wal_dir = FLAGS_wal_dir;
      }
#ifndef ROCKSDB_LITE
      if (use_blob_db_) {
        blob_db::DestroyBlobDB(FLAGS_db, options, blob_db::BlobDBOptions());
      }
#endif  //！摇滚乐
      DestroyDB(FLAGS_db, options);
      if (!FLAGS_wal_dir.empty()) {
        FLAGS_env->DeleteDir(FLAGS_wal_dir);
      }

      if (FLAGS_num_multi_db > 1) {
        FLAGS_env->CreateDir(FLAGS_db);
        if (!FLAGS_wal_dir.empty()) {
          FLAGS_env->CreateDir(FLAGS_wal_dir);
        }
      }
    }
  }

  ~Benchmark() {
    db_.DeleteDBs();
    delete prefix_extractor_;
    if (cache_.get() != nullptr) {
//这会泄露，但我们要关闭，所以没人在乎
      cache_->DisownData();
    }
  }

  Slice AllocateKey(std::unique_ptr<const char[]>* key_guard) {
    char* data = new char[key_size_];
    const char* const_data = data;
    key_guard->reset(const_data);
    return Slice(key_guard->get(), key_size_);
  }

//根据给定的规范和随机数生成密钥。
//生成的密钥将具有以下格式（如果密钥\每个前缀\u
//为正数），多余的尾随字节要么被截断，要么被填充为“0”。
//前缀值是从键值派生的。
//----------------------
//前缀00000_键00000_
//----------------------
//如果键的前缀是0，则键只是
//后面跟着尾随“0”的随机数
//----------------------
//键00000_
//----------------------
  void GenerateKeyFromInt(uint64_t v, int64_t num_keys, Slice* key) {
    char* start = const_cast<char*>(key->data());
    char* pos = start;
    if (keys_per_prefix_ > 0) {
      int64_t num_prefix = num_keys / keys_per_prefix_;
      int64_t prefix = v % num_prefix;
      int bytes_to_fill = std::min(prefix_size_, 8);
      if (port::kLittleEndian) {
        for (int i = 0; i < bytes_to_fill; ++i) {
          pos[i] = (prefix >> ((bytes_to_fill - i - 1) << 3)) & 0xFF;
        }
      } else {
        memcpy(pos, static_cast<void*>(&prefix), bytes_to_fill);
      }
      if (prefix_size_ > 8) {
//用0填充其余部分
        memset(pos + 8, '0', prefix_size_ - 8);
      }
      pos += prefix_size_;
    }

    int bytes_to_fill = std::min(key_size_ - static_cast<int>(pos - start), 8);
    if (port::kLittleEndian) {
      for (int i = 0; i < bytes_to_fill; ++i) {
        pos[i] = (v >> ((bytes_to_fill - i - 1) << 3)) & 0xFF;
      }
    } else {
      memcpy(pos, static_cast<void*>(&v), bytes_to_fill);
    }
    pos += bytes_to_fill;
    if (key_size_ > pos - start) {
      memset(pos, '0', key_size_ - (pos - start));
    }
  }

  std::string GetPathForMultiple(std::string base_name, size_t id) {
    if (!base_name.empty()) {
#ifndef OS_WIN
      if (base_name.back() != '/') {
        base_name += '/';
      }
#else
      if (base_name.back() != '\\') {
        base_name += '\\';
      }
#endif
    }
    return base_name + ToString(id);
  }

void VerifyDBFromDB(std::string& truth_db_name) {
  DBWithColumnFamilies truth_db;
  auto s = DB::OpenForReadOnly(open_options_, truth_db_name, &truth_db.db);
  if (!s.ok()) {
    fprintf(stderr, "open error: %s\n", s.ToString().c_str());
    exit(1);
  }
  ReadOptions ro;
  ro.total_order_seek = true;
  std::unique_ptr<Iterator> truth_iter(truth_db.db->NewIterator(ro));
  std::unique_ptr<Iterator> db_iter(db_.db->NewIterator(ro));
//使用：：get验证trust_db中的所有键/值在db中是否可检索
  fprintf(stderr, "Verifying db >= truth_db with ::Get...\n");
  for (truth_iter->SeekToFirst(); truth_iter->Valid(); truth_iter->Next()) {
      std::string value;
      s = db_.db->Get(ro, truth_iter->key(), &value);
      assert(s.ok());
//TODO（myabandeh）：提供调试提示
      assert(Slice(value) == truth_iter->value());
  }
//验证db迭代器没有提供任何额外的键/值
  fprintf(stderr, "Verifying db == truth_db...\n");
  for (db_iter->SeekToFirst(), truth_iter->SeekToFirst(); db_iter->Valid(); db_iter->Next(), truth_iter->Next()) {
    assert(truth_iter->Valid());
    assert(truth_iter->value() == db_iter->value());
  }
//在truth数据库中不应再保留未选中的键
  assert(!truth_iter->Valid());
  fprintf(stderr, "...Verified\n");
}

  void Run() {
    if (!SanityCheck()) {
      exit(1);
    }
    Open(&open_options_);
    PrintHeader();
    std::stringstream benchmark_stream(FLAGS_benchmarks);
    std::string name;
    std::unique_ptr<ExpiredTimeFilter> filter;
    while (std::getline(benchmark_stream, name, ',')) {
//净化参数
      num_ = FLAGS_num;
      reads_ = (FLAGS_reads < 0 ? FLAGS_num : FLAGS_reads);
      writes_ = (FLAGS_writes < 0 ? FLAGS_num : FLAGS_writes);
      deletes_ = (FLAGS_deletes < 0 ? FLAGS_num : FLAGS_deletes);
      value_size_ = FLAGS_value_size;
      key_size_ = FLAGS_key_size;
      entries_per_batch_ = FLAGS_batch_size;
      writes_per_range_tombstone_ = FLAGS_writes_per_range_tombstone;
      range_tombstone_width_ = FLAGS_range_tombstone_width;
      max_num_range_tombstones_ = FLAGS_max_num_range_tombstones;
      write_options_ = WriteOptions();
      read_random_exp_range_ = FLAGS_read_random_exp_range;
      if (FLAGS_sync) {
        write_options_.sync = true;
      }
      write_options_.disableWAL = FLAGS_disable_wal;

      void (Benchmark::*method)(ThreadState*) = nullptr;
      void (Benchmark::*post_process_method)() = nullptr;

      bool fresh_db = false;
      int num_threads = FLAGS_threads;

      int num_repeat = 1;
      int num_warmup = 0;
      if (!name.empty() && *name.rbegin() == ']') {
        auto it = name.find('[');
        if (it == std::string::npos) {
          fprintf(stderr, "unknown benchmark arguments '%s'\n", name.c_str());
          exit(1);
        }
        std::string args = name.substr(it + 1);
        args.resize(args.size() - 1);
        name.resize(it);

        std::string bench_arg;
        std::stringstream args_stream(args);
        while (std::getline(args_stream, bench_arg, '-')) {
          if (bench_arg.empty()) {
            continue;
          }
          if (bench_arg[0] == 'X') {
//重复基准N次
            std::string num_str = bench_arg.substr(1);
            num_repeat = std::stoi(num_str);
          } else if (bench_arg[0] == 'W') {
//预热基准N次
            std::string num_str = bench_arg.substr(1);
            num_warmup = std::stoi(num_str);
          }
        }
      }

//FillSeqDeterministic和FillUniquerandomDeterministic
//用唯一的“随机”填充级别（最大级别除外）
//并分别用fillseq和filluniquerandom填充最大级别
      if (name == "fillseqdeterministic" ||
          name == "filluniquerandomdeterministic") {
        if (!FLAGS_disable_auto_compactions) {
          fprintf(stderr,
                  "Please disable_auto_compactions in FillDeterministic "
                  "benchmark\n");
          exit(1);
        }
        if (num_threads > 1) {
          fprintf(stderr,
                  "filldeterministic multithreaded not supported"
                  ", use 1 thread\n");
          num_threads = 1;
        }
        fresh_db = true;
        if (name == "fillseqdeterministic") {
          method = &Benchmark::WriteSeqDeterministic;
        } else {
          method = &Benchmark::WriteUniqueRandomDeterministic;
        }
      } else if (name == "fillseq") {
        fresh_db = true;
        method = &Benchmark::WriteSeq;
      } else if (name == "fillbatch") {
        fresh_db = true;
        entries_per_batch_ = 1000;
        method = &Benchmark::WriteSeq;
      } else if (name == "fillrandom") {
        fresh_db = true;
        method = &Benchmark::WriteRandom;
      } else if (name == "filluniquerandom") {
        fresh_db = true;
        if (num_threads > 1) {
          fprintf(stderr,
                  "filluniquerandom multithreaded not supported"
                  ", use 1 thread");
          num_threads = 1;
        }
        method = &Benchmark::WriteUniqueRandom;
      } else if (name == "overwrite") {
        method = &Benchmark::WriteRandom;
      } else if (name == "fillsync") {
        fresh_db = true;
        num_ /= 1000;
        write_options_.sync = true;
        method = &Benchmark::WriteRandom;
      } else if (name == "fill100K") {
        fresh_db = true;
        num_ /= 1000;
        value_size_ = 100 * 1000;
        method = &Benchmark::WriteRandom;
      } else if (name == "readseq") {
        method = &Benchmark::ReadSequential;
      } else if (name == "readtocache") {
        method = &Benchmark::ReadSequential;
        num_threads = 1;
        reads_ = num_;
      } else if (name == "readreverse") {
        method = &Benchmark::ReadReverse;
      } else if (name == "readrandom") {
        method = &Benchmark::ReadRandom;
      } else if (name == "readrandomfast") {
        method = &Benchmark::ReadRandomFast;
      } else if (name == "multireadrandom") {
        fprintf(stderr, "entries_per_batch = %" PRIi64 "\n",
                entries_per_batch_);
        method = &Benchmark::MultiReadRandom;
      } else if (name == "readmissing") {
        ++key_size_;
        method = &Benchmark::ReadRandom;
      } else if (name == "newiterator") {
        method = &Benchmark::IteratorCreation;
      } else if (name == "newiteratorwhilewriting") {
num_threads++;  //为写入添加额外线程
        method = &Benchmark::IteratorCreationWhileWriting;
      } else if (name == "seekrandom") {
        method = &Benchmark::SeekRandom;
      } else if (name == "seekrandomwhilewriting") {
num_threads++;  //为写入添加额外线程
        method = &Benchmark::SeekRandomWhileWriting;
      } else if (name == "seekrandomwhilemerging") {
num_threads++;  //添加用于合并的额外线程
        method = &Benchmark::SeekRandomWhileMerging;
      } else if (name == "readrandomsmall") {
        reads_ /= 1000;
        method = &Benchmark::ReadRandom;
      } else if (name == "deleteseq") {
        method = &Benchmark::DeleteSeq;
      } else if (name == "deleterandom") {
        method = &Benchmark::DeleteRandom;
      } else if (name == "readwhilewriting") {
num_threads++;  //为写入添加额外线程
        method = &Benchmark::ReadWhileWriting;
      } else if (name == "readwhilemerging") {
num_threads++;  //为写入添加额外线程
        method = &Benchmark::ReadWhileMerging;
      } else if (name == "readrandomwriterandom") {
        method = &Benchmark::ReadRandomWriteRandom;
      } else if (name == "readrandommergerandom") {
        if (FLAGS_merge_operator.empty()) {
          fprintf(stdout, "%-12s : skipped (--merge_operator is unknown)\n",
                  name.c_str());
          exit(1);
        }
        method = &Benchmark::ReadRandomMergeRandom;
      } else if (name == "updaterandom") {
        method = &Benchmark::UpdateRandom;
      } else if (name == "appendrandom") {
        method = &Benchmark::AppendRandom;
      } else if (name == "mergerandom") {
        if (FLAGS_merge_operator.empty()) {
          fprintf(stdout, "%-12s : skipped (--merge_operator is unknown)\n",
                  name.c_str());
          exit(1);
        }
        method = &Benchmark::MergeRandom;
      } else if (name == "randomwithverify") {
        method = &Benchmark::RandomWithVerify;
      } else if (name == "fillseekseq") {
        method = &Benchmark::WriteSeqSeekSeq;
      } else if (name == "compact") {
        method = &Benchmark::Compact;
      } else if (name == "compactall") {
        CompactAll();
      } else if (name == "crc32c") {
        method = &Benchmark::Crc32c;
      } else if (name == "xxhash") {
        method = &Benchmark::xxHash;
      } else if (name == "acquireload") {
        method = &Benchmark::AcquireLoad;
      } else if (name == "compress") {
        method = &Benchmark::Compress;
      } else if (name == "uncompress") {
        method = &Benchmark::Uncompress;
#ifndef ROCKSDB_LITE
      } else if (name == "randomtransaction") {
        method = &Benchmark::RandomTransaction;
        post_process_method = &Benchmark::RandomTransactionVerify;
#endif  //摇滚乐
      } else if (name == "randomreplacekeys") {
        fresh_db = true;
        method = &Benchmark::RandomReplaceKeys;
      } else if (name == "timeseries") {
        timestamp_emulator_.reset(new TimestampEmulator());
        if (FLAGS_expire_style == "compaction_filter") {
          filter.reset(new ExpiredTimeFilter(timestamp_emulator_));
          fprintf(stdout, "Compaction filter is used to remove expired data");
          open_options_.compaction_filter = filter.get();
        }
        fresh_db = true;
        method = &Benchmark::TimeSeries;
      } else if (name == "stats") {
        PrintStats("rocksdb.stats");
      } else if (name == "resetstats") {
        ResetStats();
      } else if (name == "verify") {
        VerifyDBFromDB(FLAGS_truth_db);
      } else if (name == "levelstats") {
        PrintStats("rocksdb.levelstats");
      } else if (name == "sstables") {
        PrintStats("rocksdb.sstables");
} else if (!name.empty()) {  //空名称没有错误消息
        fprintf(stderr, "unknown benchmark '%s'\n", name.c_str());
        exit(1);
      }

      if (fresh_db) {
        if (FLAGS_use_existing_db) {
          fprintf(stdout, "%-12s : skipped (--use_existing_db is true)\n",
                  name.c_str());
          method = nullptr;
        } else {
          if (db_.db != nullptr) {
            db_.DeleteDBs();
            DestroyDB(FLAGS_db, open_options_);
          }
          Options options = open_options_;
          for (size_t i = 0; i < multi_dbs_.size(); i++) {
            delete multi_dbs_[i].db;
            if (!open_options_.wal_dir.empty()) {
              options.wal_dir = GetPathForMultiple(open_options_.wal_dir, i);
            }
            DestroyDB(GetPathForMultiple(FLAGS_db, i), options);
          }
          multi_dbs_.clear();
        }
Open(&open_options_);  //对上次访问使用“打开”选项
      }

      if (method != nullptr) {
        fprintf(stdout, "DB path: [%s]\n", FLAGS_db.c_str());
        if (num_warmup > 0) {
          printf("Warming up benchmark by running %d times\n", num_warmup);
        }

        for (int i = 0; i < num_warmup; i++) {
          RunBenchmark(num_threads, name, method);
        }

        if (num_repeat > 1) {
          printf("Running benchmark for %d times\n", num_repeat);
        }

        CombinedStats combined_stats;
        for (int i = 0; i < num_repeat; i++) {
          Stats stats = RunBenchmark(num_threads, name, method);
          combined_stats.AddStats(stats);
        }
        if (num_repeat > 1) {
          combined_stats.Report(name);
        }
      }
      if (post_process_method != nullptr) {
        (this->*post_process_method)();
      }
    }
    if (FLAGS_statistics) {
      fprintf(stdout, "STATISTICS:\n%s\n", dbstats->ToString().c_str());
    }
    if (FLAGS_simcache_size >= 0) {
      fprintf(stdout, "SIMULATOR CACHE STATISTICS:\n%s\n",
              static_cast_with_check<SimCache, Cache>(cache_.get())
                  ->ToString()
                  .c_str());
    }
  }

 private:
  std::shared_ptr<TimestampEmulator> timestamp_emulator_;

  struct ThreadArg {
    Benchmark* bm;
    SharedState* shared;
    ThreadState* thread;
    void (Benchmark::*method)(ThreadState*);
  };

  static void ThreadBody(void* v) {
    ThreadArg* arg = reinterpret_cast<ThreadArg*>(v);
    SharedState* shared = arg->shared;
    ThreadState* thread = arg->thread;
    {
      MutexLock l(&shared->mu);
      shared->num_initialized++;
      if (shared->num_initialized >= shared->total) {
        shared->cv.SignalAll();
      }
      while (!shared->start) {
        shared->cv.Wait();
      }
    }

    SetPerfLevel(static_cast<PerfLevel> (shared->perf_level));
    thread->stats.Start(thread->tid);
    (arg->bm->*(arg->method))(thread);
    thread->stats.Stop();

    {
      MutexLock l(&shared->mu);
      shared->num_done++;
      if (shared->num_done >= shared->total) {
        shared->cv.SignalAll();
      }
    }
  }

  Stats RunBenchmark(int n, Slice name,
                     void (Benchmark::*method)(ThreadState*)) {
    SharedState shared;
    shared.total = n;
    shared.num_initialized = 0;
    shared.num_done = 0;
    shared.start = false;
    if (FLAGS_benchmark_write_rate_limit > 0) {
      shared.write_rate_limiter.reset(
          NewGenericRateLimiter(FLAGS_benchmark_write_rate_limit));
    }
    if (FLAGS_benchmark_read_rate_limit > 0) {
      shared.read_rate_limiter.reset(NewGenericRateLimiter(
          /*GS_Benchmark_Read_Rate_Limit，100000/*再填充期\u US*/，
          10／*公平性*/, RateLimiter::Mode::kReadsOnly));

    }

    std::unique_ptr<ReporterAgent> reporter_agent;
    if (FLAGS_report_interval_seconds > 0) {
      reporter_agent.reset(new ReporterAgent(FLAGS_env, FLAGS_report_file,
                                             FLAGS_report_interval_seconds));
    }

    ThreadArg* arg = new ThreadArg[n];

    for (int i = 0; i < n; i++) {
#ifdef NUMA
      if (FLAGS_enable_numa) {
//对numa节点中的线程执行本地内存分配。
int n_nodes = numa_num_task_nodes();  //numa中的节点数。
        numa_exit_on_error = 1;
        int numa_node = i % n_nodes;
        bitmask* nodes = numa_allocate_nodemask();
        numa_bitmask_clearall(nodes);
        numa_bitmask_setbit(nodes, numa_node);
//numa_bind（）调用将进程绑定到节点，这些
//属性传递给在中创建的线程
//稍后在循环中调用StartThread方法。
        numa_bind(nodes);
        numa_set_strict(1);
        numa_free_nodemask(nodes);
      }
#endif
      arg[i].bm = this;
      arg[i].method = method;
      arg[i].shared = &shared;
      arg[i].thread = new ThreadState(i);
      arg[i].thread->stats.SetReporterAgent(reporter_agent.get());
      arg[i].thread->shared = &shared;
      FLAGS_env->StartThread(ThreadBody, &arg[i]);
    }

    shared.mu.Lock();
    while (shared.num_initialized < n) {
      shared.cv.Wait();
    }

    shared.start = true;
    shared.cv.SignalAll();
    while (shared.num_done < n) {
      shared.cv.Wait();
    }
    shared.mu.Unlock();

//可以排除某些线程的统计信息。
    Stats merge_stats;
    for (int i = 0; i < n; i++) {
      merge_stats.Merge(arg[i].thread->stats);
    }
    merge_stats.Report(name);

    for (int i = 0; i < n; i++) {
      delete arg[i].thread;
    }
    delete[] arg;

    return merge_stats;
  }

  void Crc32c(ThreadState* thread) {
//校验和大约500MB的数据总量
    const int size = 4096;
    const char* label = "(4K per op)";
    std::string data(size, 'x');
    int64_t bytes = 0;
    uint32_t crc = 0;
    while (bytes < 500 * 1048576) {
      crc = crc32c::Value(data.data(), size);
      thread->stats.FinishedOps(nullptr, nullptr, 1, kCrc);
      bytes += size;
    }
//打印以使结果不死机
    fprintf(stderr, "... crc=0x%x\r", static_cast<unsigned int>(crc));

    thread->stats.AddBytes(bytes);
    thread->stats.AddMessage(label);
  }

  void xxHash(ThreadState* thread) {
//校验和大约500MB的数据总量
    const int size = 4096;
    const char* label = "(4K per op)";
    std::string data(size, 'x');
    int64_t bytes = 0;
    unsigned int xxh32 = 0;
    while (bytes < 500 * 1048576) {
      xxh32 = XXH32(data.data(), size, 0);
      thread->stats.FinishedOps(nullptr, nullptr, 1, kHash);
      bytes += size;
    }
//打印以使结果不死机
    fprintf(stderr, "... xxh32=0x%x\r", static_cast<unsigned int>(xxh32));

    thread->stats.AddBytes(bytes);
    thread->stats.AddMessage(label);
  }

  void AcquireLoad(ThreadState* thread) {
    int dummy;
    std::atomic<void*> ap(&dummy);
    int count = 0;
    void *ptr = nullptr;
    thread->stats.AddMessage("(each op is 1000 loads)");
    while (count < 100000) {
      for (int i = 0; i < 1000; i++) {
        ptr = ap.load(std::memory_order_acquire);
      }
      count++;
      thread->stats.FinishedOps(nullptr, nullptr, 1, kOthers);
    }
if (ptr == nullptr) exit(1);  //禁用未使用的变量警告。
  }

  void Compress(ThreadState *thread) {
    RandomGenerator gen;
    Slice input = gen.Generate(FLAGS_block_size);
    int64_t bytes = 0;
    int64_t produced = 0;
    bool ok = true;
    std::string compressed;

//压缩1G
    while (ok && bytes < int64_t(1) << 30) {
      compressed.clear();
      ok = CompressSlice(input, &compressed);
      produced += compressed.size();
      bytes += input.size();
      thread->stats.FinishedOps(nullptr, nullptr, 1, kCompress);
    }

    if (!ok) {
      thread->stats.AddMessage("(compression failure)");
    } else {
      char buf[340];
      snprintf(buf, sizeof(buf), "(output: %.1f%%)",
               (produced * 100.0) / bytes);
      thread->stats.AddMessage(buf);
      thread->stats.AddBytes(bytes);
    }
  }

  void Uncompress(ThreadState *thread) {
    RandomGenerator gen;
    Slice input = gen.Generate(FLAGS_block_size);
    std::string compressed;

    bool ok = CompressSlice(input, &compressed);
    int64_t bytes = 0;
    int decompress_size;
    while (ok && bytes < 1024 * 1048576) {
      char *uncompressed = nullptr;
      switch (FLAGS_compression_type_e) {
        case rocksdb::kSnappyCompression: {
//获取大小并在此处分配以使比较公平
          size_t ulength = 0;
          if (!Snappy_GetUncompressedLength(compressed.data(),
                                            compressed.size(), &ulength)) {
            ok = false;
            break;
          }
          uncompressed = new char[ulength];
          ok = Snappy_Uncompress(compressed.data(), compressed.size(),
                                 uncompressed);
          break;
        }
      case rocksdb::kZlibCompression:
        uncompressed = Zlib_Uncompress(compressed.data(), compressed.size(),
                                       &decompress_size, 2);
        ok = uncompressed != nullptr;
        break;
      case rocksdb::kBZip2Compression:
        uncompressed = BZip2_Uncompress(compressed.data(), compressed.size(),
                                        &decompress_size, 2);
        ok = uncompressed != nullptr;
        break;
      case rocksdb::kLZ4Compression:
        uncompressed = LZ4_Uncompress(compressed.data(), compressed.size(),
                                      &decompress_size, 2);
        ok = uncompressed != nullptr;
        break;
      case rocksdb::kLZ4HCCompression:
        uncompressed = LZ4_Uncompress(compressed.data(), compressed.size(),
                                      &decompress_size, 2);
        ok = uncompressed != nullptr;
        break;
      case rocksdb::kXpressCompression:
        uncompressed = XPRESS_Uncompress(compressed.data(), compressed.size(),
          &decompress_size);
        ok = uncompressed != nullptr;
        break;
      case rocksdb::kZSTD:
        uncompressed = ZSTD_Uncompress(compressed.data(), compressed.size(),
                                       &decompress_size);
        ok = uncompressed != nullptr;
        break;
      default:
        ok = false;
      }
      delete[] uncompressed;
      bytes += input.size();
      thread->stats.FinishedOps(nullptr, nullptr, 1, kUncompress);
    }

    if (!ok) {
      thread->stats.AddMessage("(compression failure)");
    } else {
      thread->stats.AddBytes(bytes);
    }
  }

//如果从指定的
//选项文件。
  bool InitializeOptionsFromFile(Options* opts) {
#ifndef ROCKSDB_LITE
    printf("Initializing RocksDB Options from the specified file\n");
    DBOptions db_opts;
    std::vector<ColumnFamilyDescriptor> cf_descs;
    if (FLAGS_options_file != "") {
      auto s = LoadOptionsFromFile(FLAGS_options_file, Env::Default(), &db_opts,
                                   &cf_descs);
      if (s.ok()) {
        *opts = Options(db_opts, cf_descs[0].options);
        return true;
      }
      fprintf(stderr, "Unable to load options file %s --- %s\n",
              FLAGS_options_file.c_str(), s.ToString().c_str());
      exit(1);
    }
#endif
    return false;
  }

  void InitializeOptionsFromFlags(Options* opts) {
    printf("Initializing RocksDB Options from command-line flags\n");
    Options& options = *opts;

    assert(db_.db == nullptr);

    options.max_open_files = FLAGS_open_files;
    if (FLAGS_cost_write_buffer_to_cache || FLAGS_db_write_buffer_size != 0) {
      options.write_buffer_manager.reset(
          new WriteBufferManager(FLAGS_db_write_buffer_size, cache_));
    }
    options.write_buffer_size = FLAGS_write_buffer_size;
    options.max_write_buffer_number = FLAGS_max_write_buffer_number;
    options.min_write_buffer_number_to_merge =
      FLAGS_min_write_buffer_number_to_merge;
    options.max_write_buffer_number_to_maintain =
        FLAGS_max_write_buffer_number_to_maintain;
    options.max_background_jobs = FLAGS_max_background_jobs;
    options.max_background_compactions = FLAGS_max_background_compactions;
    options.max_subcompactions = static_cast<uint32_t>(FLAGS_subcompactions);
    options.max_background_flushes = FLAGS_max_background_flushes;
    options.compaction_style = FLAGS_compaction_style_e;
    options.compaction_pri = FLAGS_compaction_pri_e;
    options.allow_mmap_reads = FLAGS_mmap_read;
    options.allow_mmap_writes = FLAGS_mmap_write;
    options.use_direct_reads = FLAGS_use_direct_reads;
    options.use_direct_io_for_flush_and_compaction =
        FLAGS_use_direct_io_for_flush_and_compaction;
#ifndef ROCKSDB_LITE
    options.compaction_options_fifo = CompactionOptionsFIFO(
        FLAGS_fifo_compaction_max_table_files_size_mb * 1024 * 1024,
        FLAGS_fifo_compaction_allow_compaction, FLAGS_fifo_compaction_ttl);
#endif  //摇滚乐
    if (FLAGS_prefix_size != 0) {
      options.prefix_extractor.reset(
          NewFixedPrefixTransform(FLAGS_prefix_size));
    }
    if (FLAGS_use_uint64_comparator) {
      options.comparator = test::Uint64Comparator();
      if (FLAGS_key_size != 8) {
        fprintf(stderr, "Using Uint64 comparator but key size is not 8.\n");
        exit(1);
      }
    }
    if (FLAGS_use_stderr_info_logger) {
      options.info_log.reset(new StderrLogger());
    }
    options.memtable_huge_page_size = FLAGS_memtable_use_huge_page ? 2048 : 0;
    options.memtable_prefix_bloom_size_ratio = FLAGS_memtable_bloom_size_ratio;
    if (FLAGS_memtable_insert_with_hint_prefix_size > 0) {
      options.memtable_insert_with_hint_prefix_extractor.reset(
          NewCappedPrefixTransform(
              FLAGS_memtable_insert_with_hint_prefix_size));
    }
    options.bloom_locality = FLAGS_bloom_locality;
    options.max_file_opening_threads = FLAGS_file_opening_threads;
    options.new_table_reader_for_compaction_inputs =
        FLAGS_new_table_reader_for_compaction_inputs;
    options.compaction_readahead_size = FLAGS_compaction_readahead_size;
    options.random_access_max_buffer_size = FLAGS_random_access_max_buffer_size;
    options.writable_file_max_buffer_size = FLAGS_writable_file_max_buffer_size;
    options.use_fsync = FLAGS_use_fsync;
    options.num_levels = FLAGS_num_levels;
    options.target_file_size_base = FLAGS_target_file_size_base;
    options.target_file_size_multiplier = FLAGS_target_file_size_multiplier;
    options.max_bytes_for_level_base = FLAGS_max_bytes_for_level_base;
    options.level_compaction_dynamic_level_bytes =
        FLAGS_level_compaction_dynamic_level_bytes;
    options.max_bytes_for_level_multiplier =
        FLAGS_max_bytes_for_level_multiplier;
    if ((FLAGS_prefix_size == 0) && (FLAGS_rep_factory == kPrefixHash ||
                                     FLAGS_rep_factory == kHashLinkedList)) {
      fprintf(stderr, "prefix_size should be non-zero if PrefixHash or "
                      "HashLinkedList memtablerep is used\n");
      exit(1);
    }
    switch (FLAGS_rep_factory) {
      case kSkipList:
        options.memtable_factory.reset(new SkipListFactory(
            FLAGS_skip_list_lookahead));
        break;
#ifndef ROCKSDB_LITE
      case kPrefixHash:
        options.memtable_factory.reset(
            NewHashSkipListRepFactory(FLAGS_hash_bucket_count));
        break;
      case kHashLinkedList:
        options.memtable_factory.reset(NewHashLinkListRepFactory(
            FLAGS_hash_bucket_count));
        break;
      case kVectorRep:
        options.memtable_factory.reset(
          new VectorRepFactory
        );
        break;
      case kCuckoo:
        options.memtable_factory.reset(NewHashCuckooRepFactory(
            options.write_buffer_size, FLAGS_key_size + FLAGS_value_size));
        break;
#else
      default:
        fprintf(stderr, "Only skip list is supported in lite mode\n");
        exit(1);
#endif  //摇滚乐
    }
    if (FLAGS_use_plain_table) {
#ifndef ROCKSDB_LITE
      if (FLAGS_rep_factory != kPrefixHash &&
          FLAGS_rep_factory != kHashLinkedList) {
        fprintf(stderr, "Waring: plain table is used with skipList\n");
      }

      int bloom_bits_per_key = FLAGS_bloom_bits;
      if (bloom_bits_per_key < 0) {
        bloom_bits_per_key = 0;
      }

      PlainTableOptions plain_table_options;
      plain_table_options.user_key_len = FLAGS_key_size;
      plain_table_options.bloom_bits_per_key = bloom_bits_per_key;
      plain_table_options.hash_table_ratio = 0.75;
      options.table_factory = std::shared_ptr<TableFactory>(
          NewPlainTableFactory(plain_table_options));
#else
      fprintf(stderr, "Plain table is not supported in lite mode\n");
      exit(1);
#endif  //摇滚乐
    } else if (FLAGS_use_cuckoo_table) {
#ifndef ROCKSDB_LITE
      if (FLAGS_cuckoo_hash_ratio > 1 || FLAGS_cuckoo_hash_ratio < 0) {
        fprintf(stderr, "Invalid cuckoo_hash_ratio\n");
        exit(1);
      }
      rocksdb::CuckooTableOptions table_options;
      table_options.hash_table_ratio = FLAGS_cuckoo_hash_ratio;
      table_options.identity_as_first_hash = FLAGS_identity_as_first_hash;
      options.table_factory = std::shared_ptr<TableFactory>(
          NewCuckooTableFactory(table_options));
#else
      fprintf(stderr, "Cuckoo table is not supported in lite mode\n");
      exit(1);
#endif  //摇滚乐
    } else {
      BlockBasedTableOptions block_based_options;
      if (FLAGS_use_hash_search) {
        if (FLAGS_prefix_size == 0) {
          fprintf(stderr,
              "prefix_size not assigned when enable use_hash_search \n");
          exit(1);
        }
        block_based_options.index_type = BlockBasedTableOptions::kHashSearch;
      } else {
        block_based_options.index_type = BlockBasedTableOptions::kBinarySearch;
      }
      if (FLAGS_partition_index_and_filters) {
        if (FLAGS_use_hash_search) {
          fprintf(stderr,
                  "use_hash_search is incompatible with "
                  "partition_index_and_filters and is ignored");
        }
        block_based_options.index_type =
            BlockBasedTableOptions::kTwoLevelIndexSearch;
        block_based_options.partition_filters = true;
        block_based_options.metadata_block_size = FLAGS_metadata_block_size;
      }
      if (cache_ == nullptr) {
        block_based_options.no_block_cache = true;
      }
      block_based_options.cache_index_and_filter_blocks =
          FLAGS_cache_index_and_filter_blocks;
      block_based_options.pin_l0_filter_and_index_blocks_in_cache =
          FLAGS_pin_l0_filter_and_index_blocks_in_cache;
if (FLAGS_cache_high_pri_pool_ratio > 1e-6) {  //＞0＋EPS
        block_based_options.cache_index_and_filter_blocks_with_high_priority =
            true;
      }
      block_based_options.block_cache = cache_;
      block_based_options.block_cache_compressed = compressed_cache_;
      block_based_options.block_size = FLAGS_block_size;
      block_based_options.block_restart_interval = FLAGS_block_restart_interval;
      block_based_options.index_block_restart_interval =
          FLAGS_index_block_restart_interval;
      block_based_options.filter_policy = filter_policy_;
      block_based_options.format_version = 2;
      block_based_options.read_amp_bytes_per_bit = FLAGS_read_amp_bytes_per_bit;
      if (FLAGS_read_cache_path != "") {
#ifndef ROCKSDB_LITE
        Status rc_status;

//读缓存需要提供一个日志记录器，我们将把所有
//读缓存路径中名为rc_log的文件中的reac缓存日志
        rc_status = FLAGS_env->CreateDirIfMissing(FLAGS_read_cache_path);
        std::shared_ptr<Logger> read_cache_logger;
        if (rc_status.ok()) {
          rc_status = FLAGS_env->NewLogger(FLAGS_read_cache_path + "/rc_LOG",
                                           &read_cache_logger);
        }

        if (rc_status.ok()) {
          PersistentCacheConfig rc_cfg(FLAGS_env, FLAGS_read_cache_path,
                                       FLAGS_read_cache_size,
                                       read_cache_logger);

          rc_cfg.enable_direct_reads = FLAGS_read_cache_direct_read;
          rc_cfg.enable_direct_writes = FLAGS_read_cache_direct_write;
          rc_cfg.writer_qdepth = 4;
          rc_cfg.writer_dispatch_size = 4 * 1024;

          auto pcache = std::make_shared<BlockCacheTier>(rc_cfg);
          block_based_options.persistent_cache = pcache;
          rc_status = pcache->Open();
        }

        if (!rc_status.ok()) {
          fprintf(stderr, "Error initializing read cache, %s\n",
                  rc_status.ToString().c_str());
          exit(1);
        }
#else
        fprintf(stderr, "Read cache is not supported in LITE\n");
        exit(1);

#endif
      }
      options.table_factory.reset(
          NewBlockBasedTableFactory(block_based_options));
    }
    if (FLAGS_max_bytes_for_level_multiplier_additional_v.size() > 0) {
      if (FLAGS_max_bytes_for_level_multiplier_additional_v.size() !=
          (unsigned int)FLAGS_num_levels) {
        fprintf(stderr, "Insufficient number of fanouts specified %d\n",
                (int)FLAGS_max_bytes_for_level_multiplier_additional_v.size());
        exit(1);
      }
      options.max_bytes_for_level_multiplier_additional =
        FLAGS_max_bytes_for_level_multiplier_additional_v;
    }
    options.level0_stop_writes_trigger = FLAGS_level0_stop_writes_trigger;
    options.level0_file_num_compaction_trigger =
        FLAGS_level0_file_num_compaction_trigger;
    options.level0_slowdown_writes_trigger =
      FLAGS_level0_slowdown_writes_trigger;
    options.compression = FLAGS_compression_type_e;
    options.compression_opts.level = FLAGS_compression_level;
    options.compression_opts.max_dict_bytes = FLAGS_compression_max_dict_bytes;
    options.WAL_ttl_seconds = FLAGS_wal_ttl_seconds;
    options.WAL_size_limit_MB = FLAGS_wal_size_limit_MB;
    options.max_total_wal_size = FLAGS_max_total_wal_size;

    if (FLAGS_min_level_to_compress >= 0) {
      assert(FLAGS_min_level_to_compress <= FLAGS_num_levels);
      options.compression_per_level.resize(FLAGS_num_levels);
      for (int i = 0; i < FLAGS_min_level_to_compress; i++) {
        options.compression_per_level[i] = kNoCompression;
      }
      for (int i = FLAGS_min_level_to_compress;
           i < FLAGS_num_levels; i++) {
        options.compression_per_level[i] = FLAGS_compression_type_e;
      }
    }
    options.soft_rate_limit = FLAGS_soft_rate_limit;
    options.hard_rate_limit = FLAGS_hard_rate_limit;
    options.soft_pending_compaction_bytes_limit =
        FLAGS_soft_pending_compaction_bytes_limit;
    options.hard_pending_compaction_bytes_limit =
        FLAGS_hard_pending_compaction_bytes_limit;
    options.delayed_write_rate = FLAGS_delayed_write_rate;
    options.allow_concurrent_memtable_write =
        FLAGS_allow_concurrent_memtable_write;
    options.enable_write_thread_adaptive_yield =
        FLAGS_enable_write_thread_adaptive_yield;
    options.enable_pipelined_write = FLAGS_enable_pipelined_write;
    options.write_thread_max_yield_usec = FLAGS_write_thread_max_yield_usec;
    options.write_thread_slow_yield_usec = FLAGS_write_thread_slow_yield_usec;
    options.rate_limit_delay_max_milliseconds =
      FLAGS_rate_limit_delay_max_milliseconds;
    options.table_cache_numshardbits = FLAGS_table_cache_numshardbits;
    options.max_compaction_bytes = FLAGS_max_compaction_bytes;
    options.disable_auto_compactions = FLAGS_disable_auto_compactions;
    options.optimize_filters_for_hits = FLAGS_optimize_filters_for_hits;

//填充存储选项
    options.advise_random_on_open = FLAGS_advise_random_on_open;
    options.access_hint_on_compaction_start = FLAGS_compaction_fadvice_e;
    options.use_adaptive_mutex = FLAGS_use_adaptive_mutex;
    options.bytes_per_sync = FLAGS_bytes_per_sync;
    options.wal_bytes_per_sync = FLAGS_wal_bytes_per_sync;

//合并运算符选项
    options.merge_operator = MergeOperators::CreateFromStringId(
        FLAGS_merge_operator);
    if (options.merge_operator == nullptr && !FLAGS_merge_operator.empty()) {
      fprintf(stderr, "invalid merge operator: %s\n",
              FLAGS_merge_operator.c_str());
      exit(1);
    }
    options.max_successive_merges = FLAGS_max_successive_merges;
    options.report_bg_io_stats = FLAGS_report_bg_io_stats;

//设置通用样式的压缩配置（如果适用）
    if (FLAGS_universal_size_ratio != 0) {
      options.compaction_options_universal.size_ratio =
        FLAGS_universal_size_ratio;
    }
    if (FLAGS_universal_min_merge_width != 0) {
      options.compaction_options_universal.min_merge_width =
        FLAGS_universal_min_merge_width;
    }
    if (FLAGS_universal_max_merge_width != 0) {
      options.compaction_options_universal.max_merge_width =
        FLAGS_universal_max_merge_width;
    }
    if (FLAGS_universal_max_size_amplification_percent != 0) {
      options.compaction_options_universal.max_size_amplification_percent =
        FLAGS_universal_max_size_amplification_percent;
    }
    if (FLAGS_universal_compression_size_percent != -1) {
      options.compaction_options_universal.compression_size_percent =
        FLAGS_universal_compression_size_percent;
    }
    options.compaction_options_universal.allow_trivial_move =
        FLAGS_universal_allow_trivial_move;
    if (FLAGS_thread_status_per_interval > 0) {
      options.enable_thread_tracking = true;
    }
    if (FLAGS_rate_limiter_bytes_per_sec > 0) {
      if (FLAGS_rate_limit_bg_reads &&
          !FLAGS_new_table_reader_for_compaction_inputs) {
        fprintf(stderr,
                "rate limit compaction reads must have "
                "new_table_reader_for_compaction_inputs set\n");
        exit(1);
      }
      options.rate_limiter.reset(NewGenericRateLimiter(
          /*GS速率限制器，每秒字节数，100*1000/*重新填充周期
          10／*公平性*/,

          FLAGS_rate_limit_bg_reads ? RateLimiter::Mode::kReadsOnly
                                    : RateLimiter::Mode::kWritesOnly));
    }

#ifndef ROCKSDB_LITE
    if (FLAGS_readonly && FLAGS_transaction_db) {
      fprintf(stderr, "Cannot use readonly flag with transaction_db\n");
      exit(1);
    }
#endif  //摇滚乐

  }

  void InitializeOptionsGeneral(Options* opts) {
    Options& options = *opts;

    options.create_missing_column_families = FLAGS_num_column_families > 1;
    options.statistics = dbstats;
    options.wal_dir = FLAGS_wal_dir;
    options.create_if_missing = !FLAGS_use_existing_db;
    options.dump_malloc_stats = FLAGS_dump_malloc_stats;

    if (FLAGS_row_cache_size) {
      if (FLAGS_cache_numshardbits >= 1) {
        options.row_cache =
            NewLRUCache(FLAGS_row_cache_size, FLAGS_cache_numshardbits);
      } else {
        options.row_cache = NewLRUCache(FLAGS_row_cache_size);
      }
    }
    if (FLAGS_enable_io_prio) {
      FLAGS_env->LowerThreadPoolIOPriority(Env::LOW);
      FLAGS_env->LowerThreadPoolIOPriority(Env::HIGH);
    }
    options.env = FLAGS_env;

    if (FLAGS_num_multi_db <= 1) {
      OpenDb(options, FLAGS_db, &db_);
    } else {
      multi_dbs_.clear();
      multi_dbs_.resize(FLAGS_num_multi_db);
      auto wal_dir = options.wal_dir;
      for (int i = 0; i < FLAGS_num_multi_db; i++) {
        if (!wal_dir.empty()) {
          options.wal_dir = GetPathForMultiple(wal_dir, i);
        }
        OpenDb(options, GetPathForMultiple(FLAGS_db, i), &multi_dbs_[i]);
      }
      options.wal_dir = wal_dir;
    }
  }

  void Open(Options* opts) {
    if (!InitializeOptionsFromFile(opts)) {
      InitializeOptionsFromFlags(opts);
    }

    InitializeOptionsGeneral(opts);
  }

  void OpenDb(Options options, const std::string& db_name,
      DBWithColumnFamilies* db) {
    Status s;
//如有必要，使用柱族打开。
    if (FLAGS_num_column_families > 1) {
      size_t num_hot = FLAGS_num_column_families;
      if (FLAGS_num_hot_column_families > 0 &&
          FLAGS_num_hot_column_families < FLAGS_num_column_families) {
        num_hot = FLAGS_num_hot_column_families;
      } else {
        FLAGS_num_hot_column_families = FLAGS_num_column_families;
      }
      std::vector<ColumnFamilyDescriptor> column_families;
      for (size_t i = 0; i < num_hot; i++) {
        column_families.push_back(ColumnFamilyDescriptor(
              ColumnFamilyName(i), ColumnFamilyOptions(options)));
      }
      std::vector<int> cfh_idx_to_prob;
      if (!FLAGS_column_family_distribution.empty()) {
        std::stringstream cf_prob_stream(FLAGS_column_family_distribution);
        std::string cf_prob;
        int sum = 0;
        while (std::getline(cf_prob_stream, cf_prob, ',')) {
          cfh_idx_to_prob.push_back(std::stoi(cf_prob));
          sum += cfh_idx_to_prob.back();
        }
        if (sum != 100) {
          fprintf(stderr, "column_family_distribution items must sum to 100\n");
          exit(1);
        }
        if (cfh_idx_to_prob.size() != num_hot) {
          fprintf(stderr,
                  "got %" ROCKSDB_PRIszt
                  " column_family_distribution items; expected "
                  "%" ROCKSDB_PRIszt "\n",
                  cfh_idx_to_prob.size(), num_hot);
          exit(1);
        }
      }
#ifndef ROCKSDB_LITE
      if (FLAGS_readonly) {
        s = DB::OpenForReadOnly(options, db_name, column_families,
            &db->cfh, &db->db);
      } else if (FLAGS_optimistic_transaction_db) {
        s = OptimisticTransactionDB::Open(options, db_name, column_families,
                                          &db->cfh, &db->opt_txn_db);
        if (s.ok()) {
          db->db = db->opt_txn_db->GetBaseDB();
        }
      } else if (FLAGS_transaction_db) {
        TransactionDB* ptr;
        TransactionDBOptions txn_db_options;
        s = TransactionDB::Open(options, txn_db_options, db_name,
                                column_families, &db->cfh, &ptr);
        if (s.ok()) {
          db->db = ptr;
        }
      } else {
        s = DB::Open(options, db_name, column_families, &db->cfh, &db->db);
      }
#else
      s = DB::Open(options, db_name, column_families, &db->cfh, &db->db);
#endif  //摇滚乐
      db->cfh.resize(FLAGS_num_column_families);
      db->num_created = num_hot;
      db->num_hot = num_hot;
      db->cfh_idx_to_prob = std::move(cfh_idx_to_prob);
#ifndef ROCKSDB_LITE
    } else if (FLAGS_readonly) {
      s = DB::OpenForReadOnly(options, db_name, &db->db);
    } else if (FLAGS_optimistic_transaction_db) {
      s = OptimisticTransactionDB::Open(options, db_name, &db->opt_txn_db);
      if (s.ok()) {
        db->db = db->opt_txn_db->GetBaseDB();
      }
    } else if (FLAGS_transaction_db) {
      TransactionDB* ptr;
      TransactionDBOptions txn_db_options;
      s = CreateLoggerFromOptions(db_name, options, &options.info_log);
      if (s.ok()) {
        s = TransactionDB::Open(options, txn_db_options, db_name, &ptr);
      }
      if (s.ok()) {
        db->db = ptr;
      }
    } else if (FLAGS_use_blob_db) {
      blob_db::BlobDBOptions blob_db_options;
      blob_db::BlobDB* ptr;
      s = blob_db::BlobDB::Open(options, blob_db_options, db_name, &ptr);
      if (s.ok()) {
        db->db = ptr;
      }
#endif  //摇滚乐
    } else {
      s = DB::Open(options, db_name, &db->db);
    }
    if (!s.ok()) {
      fprintf(stderr, "open error: %s\n", s.ToString().c_str());
      exit(1);
    }
  }

  enum WriteMode {
    RANDOM, SEQUENTIAL, UNIQUE_RANDOM
  };

  void WriteSeqDeterministic(ThreadState* thread) {
    DoDeterministicCompact(thread, open_options_.compaction_style, SEQUENTIAL);
  }

  void WriteUniqueRandomDeterministic(ThreadState* thread) {
    DoDeterministicCompact(thread, open_options_.compaction_style,
                           UNIQUE_RANDOM);
  }

  void WriteSeq(ThreadState* thread) {
    DoWrite(thread, SEQUENTIAL);
  }

  void WriteRandom(ThreadState* thread) {
    DoWrite(thread, RANDOM);
  }

  void WriteUniqueRandom(ThreadState* thread) {
    DoWrite(thread, UNIQUE_RANDOM);
  }

  class KeyGenerator {
   public:
    KeyGenerator(Random64* rand, WriteMode mode,
        uint64_t num, uint64_t num_per_set = 64 * 1024)
      : rand_(rand),
        mode_(mode),
        num_(num),
        next_(0) {
      if (mode_ == UNIQUE_RANDOM) {
//注意：如果这种方法的内存消耗成为一个问题，
//我们要么把它分成几块，要么随机洗牌
//每一次。或者，使用位图实现
//（https://reviews.facebook.net/differential/diff/54627/）
        values_.resize(num_);
        for (uint64_t i = 0; i < num_; ++i) {
          values_[i] = i;
        }
        std::shuffle(
            values_.begin(), values_.end(),
            std::default_random_engine(static_cast<unsigned int>(FLAGS_seed)));
      }
    }

    uint64_t Next() {
      switch (mode_) {
        case SEQUENTIAL:
          return next_++;
        case RANDOM:
          return rand_->Next() % num_;
        case UNIQUE_RANDOM:
          assert(next_ + 1 < num_);
          return values_[next_++];
      }
      assert(false);
      return std::numeric_limits<uint64_t>::max();
    }

   private:
    Random64* rand_;
    WriteMode mode_;
    const uint64_t num_;
    uint64_t next_;
    std::vector<uint64_t> values_;
  };

  DB* SelectDB(ThreadState* thread) {
    return SelectDBWithCfh(thread)->db;
  }

  DBWithColumnFamilies* SelectDBWithCfh(ThreadState* thread) {
    return SelectDBWithCfh(thread->rand.Next());
  }

  DBWithColumnFamilies* SelectDBWithCfh(uint64_t rand_int) {
    if (db_.db != nullptr) {
      return &db_;
    } else  {
      return &multi_dbs_[rand_int % multi_dbs_.size()];
    }
  }

  void DoWrite(ThreadState* thread, WriteMode write_mode) {
    const int test_duration = write_mode == RANDOM ? FLAGS_duration : 0;
    const int64_t num_ops = writes_ == 0 ? num_ : writes_;

    size_t num_key_gens = 1;
    if (db_.db == nullptr) {
      num_key_gens = multi_dbs_.size();
    }
    std::vector<std::unique_ptr<KeyGenerator>> key_gens(num_key_gens);
    int64_t max_ops = num_ops * num_key_gens;
    int64_t ops_per_stage = max_ops;
    if (FLAGS_num_column_families > 1 && FLAGS_num_hot_column_families > 0) {
      ops_per_stage = (max_ops - 1) / (FLAGS_num_column_families /
                                       FLAGS_num_hot_column_families) +
                      1;
    }

    Duration duration(test_duration, max_ops, ops_per_stage);
    for (size_t i = 0; i < num_key_gens; i++) {
      key_gens[i].reset(new KeyGenerator(&(thread->rand), write_mode, num_,
                                         ops_per_stage));
    }

    if (num_ != FLAGS_num) {
      char msg[100];
      snprintf(msg, sizeof(msg), "(%" PRIu64 " ops)", num_);
      thread->stats.AddMessage(msg);
    }

    RandomGenerator gen;
    WriteBatch batch;
    Status s;
    int64_t bytes = 0;

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);
    std::unique_ptr<const char[]> begin_key_guard;
    Slice begin_key = AllocateKey(&begin_key_guard);
    std::unique_ptr<const char[]> end_key_guard;
    Slice end_key = AllocateKey(&end_key_guard);
    std::vector<std::unique_ptr<const char[]>> expanded_key_guards;
    std::vector<Slice> expanded_keys;
    if (FLAGS_expand_range_tombstones) {
      expanded_key_guards.resize(range_tombstone_width_);
      for (auto& expanded_key_guard : expanded_key_guards) {
        expanded_keys.emplace_back(AllocateKey(&expanded_key_guard));
      }
    }

    int64_t stage = 0;
    int64_t num_written = 0;
    while (!duration.Done(entries_per_batch_)) {
      if (duration.GetStage() != stage) {
        stage = duration.GetStage();
        if (db_.db != nullptr) {
          db_.CreateNewCf(open_options_, stage);
        } else {
          for (auto& db : multi_dbs_) {
            db.CreateNewCf(open_options_, stage);
          }
        }
      }

      size_t id = thread->rand.Next() % num_key_gens;
      DBWithColumnFamilies* db_with_cfh = SelectDBWithCfh(id);
      batch.Clear();

      if (thread->shared->write_rate_limiter.get() != nullptr) {
        thread->shared->write_rate_limiter->Request(
            entries_per_batch_ * (value_size_ + key_size_), Env::IO_HIGH,
            /*lptr/*stats*/，ratelimiter:：optype:：kWrite）；
        //将最后一个操作完成的时间设置为now（），以隐藏延迟和
        //从速率限制器睡眠。另外，每批检查一次，不是
        //每次写入一次。
        thread->stats.resetlastoptime（）；
      }

      for（int64_t j=0；j<entries_per_batch_u；j++）
        int64_t rand_num=key_gens[id]->next（）；
        generatekeyfromint（rand_num，flags_num，&key）；
        如果（使用斑点）
ifndef岩石
          slice val=gen.generate（值\大小\）；
          int ttl=rand（）%86400；
          blob_db:：blob db*blob db=
              静态_cast<blob_db:：blob db*>（db_with_cfh->db）；
          s=blobdb->putwitttl（写入选项_uuu、key、val、ttl）；
endif//rocksdb_lite
        else if（flags_num_column_families<=1）
          batch.put（key，gen.generate（value_-size_uu））；
        }否则{
          //对于键和列族，我们使用与seed相同的rand_num，以便
          //无法确定地找到与特定
          //读取键时使用键。
          batch.put（db_with_cfh->getcfh（rand_num），键，
                    gen.generate（value_-size_uu））；
        }
        bytes+=值\大小\键\大小\；
        + +数字书写；
        如果（写入每个范围的逻辑删除>0&&
            num_written/writes_per_range_tombstone_u<=
                最大值范围
            num_written%写入_per_range_tombstone_==0）
          int64_t begin_num=key_gens[id]->next（）；
          if（flags_expand_range_tombstones）
            对于（int64_t offset=0；offset<range_tombstone_width_uuu；
                 ++偏移量）{
              generateKeyFromInt（begin_num+offset，flags_num，
                                 &expanded_keys[偏移]；
              如果（使用斑点）
ifndef岩石
                s=db_，带_cfh->db->delete（写入选项）
                                            展开的_键[偏移]；
endif//rocksdb_lite
              else if（flags_num_column_families<=1）
                batch.delete（扩展的_键[偏移量]）；
              }否则{
                batch.delete（db_with_cfh->getcfh（rand_num）），
                             展开的_键[偏移]；
              }
            }
          }否则{
            generatekeyfromint（begin_num，flags_num，&begin_key）；
            generatekeyfromint（begin_num+range_tombstone_width_uu，flags_num，
                               和EnthKIKE；
            如果（使用斑点）
ifndef岩石
              s=db_，带_cfh->db->deleterage（
                  用_cfh->db->defaultColumnFamily（）编写_选项_uu，db_，
                  开始键，结束键）；
endif//rocksdb_lite
            else if（flags_num_column_families<=1）
              batch.deleterage（开始键、结束键）；
            }否则{
              batch.deleterage（db_with_cfh->getcfh（rand_num），begin_key，
                                Enth-KEY；
            }
          }
        }
      }
      如果（！）UsIsBoByBdBi）{
ifndef岩石
        s=db_，带cfh->db->write（写入u选项uux，&batch）；
endif//rocksdb_lite
      }
      thread->stats.finishedops（db_with_cfh，db_with_cfh->db，
                                每个批次的条目；
      如果（！）S.O.（））{
        fprintf（stderr，“放置错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
    }
    thread->stats.addbytes（字节）；
  }

  状态DoDeterministicCompact（ThreadState*线程，
                                压实样式压实样式，
                                写入模式写入模式）
ifndef岩石
    柱状家族元数据；
    std:：vector<db*>db_list；
    如果（dB.dB！= null pTr）{
      db-list.push-back（db-db）；
    }否则{
      对于（auto&db:multi_dbs_
        db-list.push-back（db.db）；
      }
    }
    std:：vector<options>options_list；
    对于（auto-db:db_list）
      选项_list.push_back（db->getOptions（））；
      如果（压缩样式！=kCompactionStyleFifo）
        db->setoptions（“禁用_自动_压缩”，“1”，
                        “0级减速”写入“触发”，“400000000”，
                        “Level0_Stop_Writes_Trigger”，“400000000”）；
      }否则{
        db->setoptions（“disable_auto_compactions”，“1”）；
      }
    }

    断言（！）db_list.empty（））；
    auto num_db=db_list.size（）；
    size_t num_levels=static_cast<size_t>（open_options_u.num_levels）；
    大小输出级别=打开选项级别-1；
    std:：vector<std:：vector<std:：vector<sstfilemetadata>>sorted_runs（num_db）；
    std:：vector<size_t>num_files_at_level0（num_db，0）；
    if（compaction_style==kCompactionStyleLevel）
      如果（num_levels==0）
        返回状态：：invalidArgument（“num_levels应大于1”）；
      }
      bool shouldou stop=false；
      而（！）{停止）{
        if（sorted_runs[0].empty（））
          Dowrite（线程，写入模式）；
        }否则{
          陶土（螺纹，独一无二的）
        }
        对于（尺寸_t i=0；i<num_db；i++）
          auto db=db_list[i]；
          db->flush（flushoptings（））；
          db->getColumnFamilyMetadata（&meta）；
          if（num_files_at_level0[i]==meta.levels[0].files.size（）
              WrnsEs=＝0）{
            应该停止=真；
            继续；
          }
          排序后的运行
              meta.levels[0].files.begin（），
              meta.levels[0].files.end（）-num_files_at_level0[i]）；
          num_files_at_level0[i]=meta.levels[0].files.size（）；
          if（排序后的_runs[i].back（）.size（）==1）
            应该停止=真；
            继续；
          }
          if（排序后的_runs[i].size（）==output_level）
            auto&l1=排序后运行[i].back（）；
            l1.擦除（l1.begin（），l1.begin（）+l1.size（）/3）；
            应该停止=真；
            继续；
          }
        }
        writes_/=static_cast<int64_t>（open_options_u.max_bytes_for_level_乘数）；
      }
      对于（尺寸_t i=0；i<num_db；i++）
        if（sorted_runs[i].size（）<num_levels-1）
          fprintf（stderr，“n太小，无法填充%”rocksdb“priszt”级别\n“，num”级别）；
          退出（1）；
        }
      }
      对于（尺寸_t i=0；i<num_db；i++）
        auto db=db_list[i]；
        auto-compactionoptions=compactionoptions（）；
        auto options=db->getoptions（）；
        可变选件可变选件（选件）；
        for（size_t j=0；j<sorted_runs[i].size（）；j++）
          compactionoptions.output_file_size_limit=
              可变的\u cf_选项.maxfilesizeforlevel（
                  static_cast<int>（output_level））；
          std:：cout<<sorted_runs[i][j].size（）<<std:：endl；
          db->compactfiles（compactionoptions，sortedou runs[i][j].back（）.name，
                                               排序后的运行[i][j].front（）.name，
                           静态\u cast<int>（输出\u level-j）/*level*/);

        }
      }
    } else if (compaction_style == kCompactionStyleUniversal) {
      auto ratio = open_options_.compaction_options_universal.size_ratio;
      bool should_stop = false;
      while (!should_stop) {
        if (sorted_runs[0].empty()) {
          DoWrite(thread, write_mode);
        } else {
          DoWrite(thread, UNIQUE_RANDOM);
        }
        for (size_t i = 0; i < num_db; i++) {
          auto db = db_list[i];
          db->Flush(FlushOptions());
          db->GetColumnFamilyMetaData(&meta);
          if (num_files_at_level0[i] == meta.levels[0].files.size() ||
              writes_ == 0) {
            should_stop = true;
            continue;
          }
          sorted_runs[i].emplace_back(
              meta.levels[0].files.begin(),
              meta.levels[0].files.end() - num_files_at_level0[i]);
          num_files_at_level0[i] = meta.levels[0].files.size();
          if (sorted_runs[i].back().size() == 1) {
            should_stop = true;
            continue;
          }
          num_files_at_level0[i] = meta.levels[0].files.size();
        }
        writes_ =  static_cast<int64_t>(writes_* static_cast<double>(100) / (ratio + 200));
      }
      for (size_t i = 0; i < num_db; i++) {
        if (sorted_runs[i].size() < num_levels) {
          fprintf(stderr, "n is too small to fill %" ROCKSDB_PRIszt  " levels\n", num_levels);
          exit(1);
        }
      }
      for (size_t i = 0; i < num_db; i++) {
        auto db = db_list[i];
        auto compactionOptions = CompactionOptions();
        auto options = db->GetOptions();
        MutableCFOptions mutable_cf_options(options);
        for (size_t j = 0; j < sorted_runs[i].size(); j++) {
          compactionOptions.output_file_size_limit =
              mutable_cf_options.MaxFileSizeForLevel(
                  static_cast<int>(output_level));
          db->CompactFiles(
              compactionOptions,
              {sorted_runs[i][j].back().name, sorted_runs[i][j].front().name},
              (output_level > j ? static_cast<int>(output_level - j)
                                /*）
        }
      }
    否则，如果（compaction_style==kCompactionStyleFifo）
      如果（数字级别！= 1）{
        返回状态：：invalidArgument（
          “对于FIFO压缩，num_级别应为1”）；
      }
      如果（标记多个数据库！= 0）{
        返回状态：：invalidArgument（“不支持multidb”）；
      }
      auto-db=db_列表[0]；
      std:：vector<std:：string>文件名；
      当（真）{
        if（sorted_runs[0].empty（））
          Dowrite（线程，写入模式）；
        }否则{
          陶土（螺纹，独一无二的）
        }
        db->flush（flushoptings（））；
        db->getColumnFamilyMetadata（&meta）；
        auto total_size=meta.levels[0].大小；
        如果（总尺寸大于等于
          db->getoptions（）.compaction_options_fifo.max_table_files_size）
          for（自动文件\u meta:meta.levels[0].files）
            文件名.emplace-back（file-meta.name）；
          }
          断裂；
        }
      }
      //TODO（Shuzhang1989）：调查CompactFile不工作的原因
      //auto compactionoptions=compactionoptions（）；
      //db->compactfiles（compactionoptions，文件名，0）；
      auto compactionOptions=compactRangeOptions（）；
      db->compactrange（压缩选项、nullptr、nullptr）；
    }否则{
      FPTCTF（STDUT）
              “%-12s:跳过（-compaction\u stype=kCompactionStylenone）\n”，
              “填充确定性”）；
      返回状态：：InvalidArgument（“不支持无压缩”）；
    }

//验证seqno和键范围
//注意：seqno通过实现在max级别上进行更改
//优化，因此跳过最大级别的检查。
nIFUDEF NDEUG
    对于（尺寸_t k=0；k<num_db；k++）
      auto db=db_list[k]；
      db->getColumnFamilyMetadata（&meta）；
      //验证已排序的运行数
      if（compaction_style==kCompactionStyleLevel）
        断言（num_levels-1==sorted_runs[k].size（））；
      其他if（compaction_style==kCompactionStyleUniversal）
        断言（meta.levels[0].files.size（）+num_levels-1==
               排序后的运行[k].size（））；
      否则，如果（compaction_style==kCompactionStyleFifo）
        //TODO（GZH）：FIFO压缩
        db->getColumnFamilyMetadata（&meta）；
        auto total_size=meta.levels[0].大小；
        断言（总大小<=
          db->getoptions（）.compaction_options_fifo.max_table_files_size）；
          断裂；
      }

      //验证每个排序运行的最小/最大seqno和键范围
      auto max_level=num_levels-1；
      int级；
      for（size_t i=0；i<sorted_runs[k].size（）；i++）
        level=static_cast<int>（max_level-i）；
        sequenceNumber sorted_run_minimum_seqno=kmaxSequenceNumber；
        sequencenumber sorted_run_maximum_seqno=0；
        std：：字符串排序的_run_minister_key，排序的_run_minister_key；
        bool first_key=true；
        for（auto-filemeta:sorted_runs[k][i]）
          排序\运行\最小\序号=
              std：：min（排序后的运行最小的文件，文件元，最小的文件）；
          排序后的运行最大值=
              std：：max（排序后的运行最大\u seqno，filemeta.maximum \u seqno）；
          如果（第一个键
              db->defaultColumnFamily（）->getComparator（）->compare（）。
                  filemeta.smallest key，排序后的_run_minimum_key）<0）
            排序后的运行最小的密钥=filemeta.smallest key；
          }
          如果（第一个键
              db->defaultColumnFamily（）->getComparator（）->compare（）。
                  filemeta.largest key，排序后的_run_maximum_key）>0）
            排序后的运行最大的密钥=filemeta.largest key；
          }
          第一个键=假；
        }
        if（compaction_style==kCompactionStyleLevel_
            （compaction_style==kCompactionStyleUniversal&&Level>0））
          SequenceNumber Level_Minimum_Seqno=kmaxSequenceNumber；
          SequenceNumber级别_最大_seqno=0；
          for（auto-filemeta:meta.levels[level].files）
            水平_最小_seqno=
                std：：min（级别\u最小\u seqno，filemeta.minimum \u seqno）；
            水平面最大值=
                std：：max（级别\u最大\u seqno，filemeta.max \u seqno）；
          }
          断言（排序后的运行最小的密钥==
                 meta.levels[level].files.front（）.smallestkey）；
          断言（排序后的运行最大的密钥==
                 meta.levels[level].files.back（）.largestkey）；
          如果（水平）！=static_cast<int>（max_level））
            //最大级别的压缩将更改序列号
            断言（排序后的_run_minimum_seqno==level_minimum_seqno）；
            断言（排序后的_run_maximum_seqno==level_maximum_seqno）；
          }
        其他if（compaction_style==kCompactionStyleUniversal）
          //级别<=0表示在级别0上排序运行
          自动级别0_文件=
              meta.levels[0].files[sorted_runs[k].size（）-1-i]；
          断言（排序后的运行最小的密钥==level0\u file.smallest key）；
          断言（排序后的运行最大的密钥==level0\u file.largest key）；
          如果（水平）！=static_cast<int>（max_level））
            断言（sorted_run_minimum_seqno==level0_file.minimum_seqno）；
            断言（排序后的运行最大的文件=级别0文件。最大的文件）；
          }
        }
      }
    }
第二节
    //打印每个排序的\运行的大小
    对于（尺寸_t k=0；k<num_db；k++）
      auto db=db_list[k]；
      FPTCTF（STDUT）
              “------------db%”rocksdb_priszt“lsm---------------\n”，k）；
      db->getColumnFamilyMetadata（&meta）；
      用于（自动和级别数据：元级别）
        if（levelmeta.files.empty（））
          继续；
        }
        如果（levelmeta.level==0）
          用于（auto&filemeta:levelmeta.files）
            fprintf（stdout，“级别[%d]：%s（大小：%”priu64“字节）\n”，
                    levelmeta.level，filemeta.name.c_str（），filemeta.size）；
          }
        }否则{
          fprintf（stdout，“级别[%d]：%s-%s（总大小：%”prii64“字节）\n”，
                  levelmta.level，levelmta.files.front（）.name.c_str（），
                  levelmeta.files.back（）.name.c_str（），levelmeta.size）；
        }
      }
    }
    对于（尺寸_t i=0；i<num_db；i++）
      db_list[i]->setoptions（
          “禁用_自动_压缩”，
            std：：to_string（选项_list[i].禁用_auto_compactions），
           “0级减速”写入“触发”，
            std：：to_string（options_list[i].level0_slowed_writes_trigger），
           “级别0_停止_写入_触发器”，
            std：：to_string（选项_list[i].level0_stop_writes_trigger））；
    }
    返回状态：：OK（）；
否则
    fprintf（stderr，“rocksdb lite不支持FillDeterminatic \n”）；
    返回状态：：不支持（
        “Rocksdb Lite不支持FillDeterminatic”）；
endif//rocksdb_lite
  }

  void readSequential（threadState*线程）
    如果（dB.dB！= null pTr）{
      读顺序（线程，db_.db）；
    }否则{
      对于（const auto&db_with_cfh:multi_dbs_
        readsequential（线程，db_，带\cfh.db）；
      }
    }
  }

  void readSequential（threadState*线程，db*db）
    读取选项选项（标记_verify_checksum，true）；
    options.tailing=flags_使用_tailing_迭代器；

    迭代器*iter=db->new迭代器（选项）；
    In 64×T i＝0；
    Int64_t字节=0；
    for（iter->seektofirst（）；i<reads&&iter->valid（）；iter->next（））
      bytes+=iter->key（）.size（）+iter->value（）.size（）；
      thread->stats.finishedops（nullptr，db，1，kread）；
      +i；

      如果（thread->shared->read_rate_limiter.get（）！= NulLPTR & &
          I%1024==1023）
        线程->共享->读取速率限制器->请求（1024，env:：io_高，
                                                   nullptr/*状态*/,

                                                   RateLimiter::OpType::kRead);
      }
    }

    delete iter;
    thread->stats.AddBytes(bytes);
    if (FLAGS_perf_level > rocksdb::PerfLevel::kDisable) {
      thread->stats.AddMessage(get_perf_context()->ToString());
    }
  }

  void ReadReverse(ThreadState* thread) {
    if (db_.db != nullptr) {
      ReadReverse(thread, db_.db);
    } else {
      for (const auto& db_with_cfh : multi_dbs_) {
        ReadReverse(thread, db_with_cfh.db);
      }
    }
  }

  void ReadReverse(ThreadState* thread, DB* db) {
    Iterator* iter = db->NewIterator(ReadOptions(FLAGS_verify_checksum, true));
    int64_t i = 0;
    int64_t bytes = 0;
    for (iter->SeekToLast(); i < reads_ && iter->Valid(); iter->Prev()) {
      bytes += iter->key().size() + iter->value().size();
      thread->stats.FinishedOps(nullptr, db, 1, kRead);
      ++i;
      if (thread->shared->read_rate_limiter.get() != nullptr &&
          i % 1024 == 1023) {
        thread->shared->read_rate_limiter->Request(1024, Env::IO_HIGH,
                                                   /*lptr/*状态*/，
                                                   ratelimiter:：optype:：kread）；
      }
    }
    删除ITER；
    thread->stats.addbytes（字节）；
  }

  void readrandomfast（threadstate*thread）
    Int64读=0；
    Int64发现=0；
    Int64不存在=0；
    读取选项选项（标记_verify_checksum，true）；
    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    std：：字符串值；
    db*db=selectdbwithcfh（thread）->db；

    Int64_t pot=1；
    while（pot<flags_num）
      POT＜1；
    }

    持续时间（标记持续时间，读取时间）；
    做{
      对于（int i=0；i<100；+i）
        int64_t key_rand=thread->rand.next（）&（pot-1）；
        generatekeyfromint（key_rand，flags_num，&key）；
        ++阅读；
        auto status=db->get（选项、键和值）；
        if（status.ok（））
          +发现；
        否则，（如果）！status.isNotFound（））
          fprintf（stderr，“get返回一个错误：%s\n”，
                  status.toString（）.c_str（））；
          中止（）；
        }
        if（key_rand>=flags_num）
          + +不存在；
        }
      }
      如果（thread->shared->read_rate_limiter.get（）！= null pTr）{
        线程->共享->读取速率限制器->请求（
            100，env:：io_high，nullptr/*状态*/, RateLimiter::OpType::kRead);

      }

      thread->stats.FinishedOps(nullptr, db, 100, kRead);
    } while (!duration.Done(100));

    char msg[100];
    snprintf(msg, sizeof(msg), "(%" PRIu64 " of %" PRIu64 " found, "
             "issued %" PRIu64 " non-exist keys)\n",
             found, read, nonexist);

    thread->stats.AddMessage(msg);

    if (FLAGS_perf_level > rocksdb::PerfLevel::kDisable) {
      thread->stats.AddMessage(get_perf_context()->ToString());
    }
  }

  int64_t GetRandomKey(Random64* rand) {
    uint64_t rand_int = rand->Next();
    int64_t key_rand;
    if (read_random_exp_range_ == 0) {
      key_rand = rand_int % FLAGS_num;
    } else {
      const uint64_t kBigInt = static_cast<uint64_t>(1U) << 62;
      long double order = -static_cast<long double>(rand_int % kBigInt) /
                          static_cast<long double>(kBigInt) *
                          read_random_exp_range_;
      long double exp_ran = std::exp(order);
      uint64_t rand_num =
          static_cast<int64_t>(exp_ran * static_cast<long double>(FLAGS_num));
//映射到不同的数字以避免位置。
      const uint64_t kBigPrime = 0x5bd1e995;
//溢出类似于%（2^64）。对结果的影响很小。
      key_rand = static_cast<int64_t>((rand_num * kBigPrime) % FLAGS_num);
    }
    return key_rand;
  }

  void ReadRandom(ThreadState* thread) {
    int64_t read = 0;
    int64_t found = 0;
    int64_t bytes = 0;
    ReadOptions options(FLAGS_verify_checksum, true);
    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);
    PinnableSlice pinnable_val;

    Duration duration(FLAGS_duration, reads_);
    while (!duration.Done(1)) {
      DBWithColumnFamilies* db_with_cfh = SelectDBWithCfh(thread);
//我们使用与键和列族种子相同的键和列，以便
//确定地找到与特定键对应的cfh，因为它
//是用陶土方法做的。
      int64_t key_rand = GetRandomKey(&thread->rand);
      GenerateKeyFromInt(key_rand, FLAGS_num, &key);
      read++;
      Status s;
      if (FLAGS_num_column_families > 1) {
        s = db_with_cfh->db->Get(options, db_with_cfh->GetCfh(key_rand), key,
                                 &pinnable_val);
      } else {
        pinnable_val.Reset();
        s = db_with_cfh->db->Get(options,
                                 db_with_cfh->db->DefaultColumnFamily(), key,
                                 &pinnable_val);
      }
      if (s.ok()) {
        found++;
        bytes += key.size() + pinnable_val.size();
      } else if (!s.IsNotFound()) {
        fprintf(stderr, "Get returned an error: %s\n", s.ToString().c_str());
        abort();
      }

      if (thread->shared->read_rate_limiter.get() != nullptr &&
          read % 256 == 255) {
        thread->shared->read_rate_limiter->Request(
            /*，env:：io_high，nullptr/*stats*/，ratelimiter:：optype:：kread）；
      }

      thread->stats.finishedops（db_with_cfh，db_with_cfh->db，1，kread）；
    }

    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg），“（找到%”priu64“的%”priu64“）\n”，
             发现，阅读）；

    thread->stats.addbytes（字节）；
    thread->stats.addmessage（msg）；

    if（flags_perf_level>rocksdb:：perf level:：kdisable）
      thread->stats.addmessage（get_perf_context（）->toString（））；
    }
  }

  //通过随机分布的键列表调用multigot。
  //返回找到的键总数。
  void multiareadrandom（threadstate*thread）
    Int64读=0；
    int64_t num_multireds=0；
    Int64发现=0；
    读取选项选项（标记_verify_checksum，true）；
    std:：vector<slice>keys；
    std:：vector<std:：unique_ptr<const char[]>>key_guards；
    std:：vector<std:：string>values（entries_per_batch_u）；
    while（static_cast<int64_t>（keys.size（））<entries_per_batch_
      key_guards.push_back（std:：unique_ptr<const char[]>（））；
      keys.push_back（allocatekey（&key_guards.back（））；
    }

    持续时间（标记持续时间，读取时间）；
    而（！）持续时间。完成（1））
      db*db=selectdb（线程）；
      for（int64_t i=0；i<entries_per_batch_；++i）
        generatekeyfromint（getrandomkey（&thread->rand），flags_num，&keys[i]）；
      }
      std:：vector<status>status=db->multiget（选项、键和值）；
      assert（static_cast<int64_t>（status.size（））==每个_批处理的条目数）；

      read+=条目数每批
      num_多读++；
      for（int64_t i=0；i<entries_per_batch_；++i）
        if（状态[i].ok（））
          +发现；
        否则，（如果）！状态[i].isNotFound（））
          fprintf（stderr，“multiget返回一个错误：%s\n”，
                  状态[i].ToString（）.c_str（））；
          中止（）；
        }
      }
      如果（thread->shared->read_rate_limiter.get（）！= NulLPTR & &
          num_多重读取%256==255）
        线程->共享->读取速率限制器->请求（
            256*个条目，每个批次，env:：io_高，nullptr/*状态*/,

            RateLimiter::OpType::kRead);
      }
      thread->stats.FinishedOps(nullptr, db, entries_per_batch_, kRead);
    }

    char msg[100];
    snprintf(msg, sizeof(msg), "(%" PRIu64 " of %" PRIu64 " found)",
             found, read);
    thread->stats.AddMessage(msg);
  }

  void IteratorCreation(ThreadState* thread) {
    Duration duration(FLAGS_duration, reads_);
    ReadOptions options(FLAGS_verify_checksum, true);
    while (!duration.Done(1)) {
      DB* db = SelectDB(thread);
      Iterator* iter = db->NewIterator(options);
      delete iter;
      thread->stats.FinishedOps(nullptr, db, 1, kOthers);
    }
  }

  void IteratorCreationWhileWriting(ThreadState* thread) {
    if (thread->tid > 0) {
      IteratorCreation(thread);
    } else {
      BGWriter(thread, kWrite);
    }
  }

  void SeekRandom(ThreadState* thread) {
    int64_t read = 0;
    int64_t found = 0;
    int64_t bytes = 0;
    ReadOptions options(FLAGS_verify_checksum, true);
    options.tailing = FLAGS_use_tailing_iterator;

    Iterator* single_iter = nullptr;
    std::vector<Iterator*> multi_iters;
    if (db_.db != nullptr) {
      single_iter = db_.db->NewIterator(options);
    } else {
      for (const auto& db_with_cfh : multi_dbs_) {
        multi_iters.push_back(db_with_cfh.db->NewIterator(options));
      }
    }

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);

    Duration duration(FLAGS_duration, reads_);
    char value_buffer[256];
    while (!duration.Done(1)) {
      if (!FLAGS_use_tailing_iterator) {
        if (db_.db != nullptr) {
          delete single_iter;
          single_iter = db_.db->NewIterator(options);
        } else {
          for (auto iter : multi_iters) {
            delete iter;
          }
          multi_iters.clear();
          for (const auto& db_with_cfh : multi_dbs_) {
            multi_iters.push_back(db_with_cfh.db->NewIterator(options));
          }
        }
      }
//选择要使用的迭代器
      Iterator* iter_to_use = single_iter;
      if (single_iter == nullptr) {
        iter_to_use = multi_iters[thread->rand.Next() % multi_iters.size()];
      }

      GenerateKeyFromInt(thread->rand.Next() % FLAGS_num, FLAGS_num, &key);
      iter_to_use->Seek(key);
      read++;
      if (iter_to_use->Valid() && iter_to_use->key().compare(key) == 0) {
        found++;
      }

      for (int j = 0; j < FLAGS_seek_nexts && iter_to_use->Valid(); ++j) {
//复制迭代器的值以确保我们读取了它们。
        Slice value = iter_to_use->value();
        memcpy(value_buffer, value.data(),
               std::min(value.size(), sizeof(value_buffer)));
        bytes += iter_to_use->key().size() + iter_to_use->value().size();

        if (!FLAGS_reverse_iterator) {
          iter_to_use->Next();
        } else {
          iter_to_use->Prev();
        }
        assert(iter_to_use->status().ok());
      }

      if (thread->shared->read_rate_limiter.get() != nullptr &&
          read % 256 == 255) {
        thread->shared->read_rate_limiter->Request(
            /*，env:：io_high，nullptr/*stats*/，ratelimiter:：optype:：kread）；
      }

      thread->stats.finishedops（&db_uudb_udb，1，kseek）；
    }
    删除单个文件；
    对于（auto-iter:multi-iters）
      删除ITER；
    }

    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg），“（找到%”priu64“的%”priu64“）\n”，
             发现，阅读）；
    thread->stats.addbytes（字节）；
    thread->stats.addmessage（msg）；
    if（flags_perf_level>rocksdb:：perf level:：kdisable）
      thread->stats.addmessage（get_perf_context（）->toString（））；
    }
  }

  void seekrandomwhilewriting（threadstate*thread）
    if（thread->tid>0）
      Seekrandom（螺纹）；
    }否则{
      bgwriter（线程，kwrite）；
    }
  }

  空Seekrandomwhileming（threadstate*thread）
    if（thread->tid>0）
      Seekrandom（螺纹）；
    }否则{
      bgwriter（线程，kmerge）；
    }
  }

  void dodelete（threadstate*thread，bool seq）
    写入批处理；
    持续时间（顺序？0：标志“持续时间，删除”；
    In 64×T i＝0；
    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；

    而（！）Duration.Done（entries_per_batch_））
      db*db=selectdb（线程）；
      批处理清除（）；
      for（int64_t j=0；j<entries_per_batch_；++j）
        const int64_t k=序列？i+j：（thread->rand.next（）%flags_num）；
        generateKeyFromInt（k，flags_num，&key）；
        批量删除（键）；
      }
      auto s=db->write（写入选项&batch）；
      thread->stats.finishedops（nullptr，db，entries_per_u batch_uu，kdelete）；
      如果（！）S.O.（））{
        fprintf（stderr，“del错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
      i+=条目_每批次；
    }
  }

  void deletesq（threadstate*线程）
    Dodelete（螺纹，真）；
  }

  void deleterandom（threadstate*thread）
    Dodelete（螺纹，假）；
  }

  void readwhilewriting（threadstate*thread）
    if（thread->tid>0）
      readrandom（线程）；
    }否则{
      bgwriter（线程，kwrite）；
    }
  }

  空读whilemegging（threadstate*thread）
    if（thread->tid>0）
      readrandom（线程）；
    }否则{
      bgwriter（线程，kmerge）；
    }
  }

  void bgwriter（threadstate*thread，enum operationtype write_merge）
    //在其他线程完成之前一直在写入的特殊线程。
    随机发生器Gen；
    Int64_t字节=0；

    std:：unique_ptr<rate limiter>写入速率限制器；
    if（标记_Benchmark_Write_Rate_Limit>0）
      写入速率限制器.reset（
          NewGenericRateLimiter（标记_Benchmark_Write_Rate_Limit））；
    }

    //不要将此线程的统计信息与读卡器合并。
    thread->stats.setExcludeFromMerge（）；

    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    uint32_t written=0；
    bool hint_printed=false；

    当（真）{
      db*db=selectdb（线程）；
      {
        mutexlock l（&thread->shared->mu）；
        if（标记_finish_after_writes&&written==writes_）
          fprintf（stderr，“在写入%u之后退出写入程序…”，written）；
          断裂；
        }
        if（thread->shared->num_done+1>=thread->shared->num_initialized）
          //其他线程已完成
          if（标记“写入”后完成”）；
            //等待写入完成
            如果（！）印特印刷）
              fprintf（stderr，“读取完成。”还有%d个写操作要做\n“，
                      （int）书写；
              提示_printed=true；
            }
          }否则{
            //立即完成写入
            断裂；
          }
        }
      }

      generateKeyFromInt（thread->rand.next（）%flags_num，flags_num，&key）；
      状态S；

      if（写入_merge==kwrite）
        s=db->put（写入_选项_uuu，键，gen.generate（值_大小_uu））；
      }否则{
        s=db->merge（写入_选项_uuu，key，gen.generate（value_size_uu））；
      }
      书面++；

      如果（！）S.O.（））{
        fprintf（stderr，“放置或合并错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
      bytes+=key.size（）+value_-size_u；
      thread->stats.finishedops（&db_uudb_udb，1，kwrite）；

      if（标记_Benchmark_Write_Rate_Limit>0）
        写入速率限制器->请求（
            每个批次的条目数（值大小），env:：io高，
            nullptr/*状态*/, RateLimiter::OpType::kWrite);

      }
    }
    thread->stats.AddBytes(bytes);
  }

//给定一个键k和值v，这会使（k+“0”，v），（k+“1”，v），（k+“2”，v）
//在数据库中原子化，即在单个批次中。另请参阅getmany。
  Status PutMany(DB* db, const WriteOptions& writeoptions, const Slice& key,
                 const Slice& value) {
    std::string suffixes[3] = {"2", "1", "0"};
    std::string keys[3];

    WriteBatch batch;
    Status s;
    for (int i = 0; i < 3; i++) {
      keys[i] = key.ToString() + suffixes[i];
      batch.Put(keys[i], value);
    }

    s = db->Write(writeoptions, &batch);
    return s;
  }


//给定一个键k，这将删除（k+“0”，v），（k+“1”，v），（k+“2”，v）
//在数据库中原子化，即在单个批次中。另请参阅getmany。
  Status DeleteMany(DB* db, const WriteOptions& writeoptions,
                    const Slice& key) {
    std::string suffixes[3] = {"1", "2", "0"};
    std::string keys[3];

    WriteBatch batch;
    Status s;
    for (int i = 0; i < 3; i++) {
      keys[i] = key.ToString() + suffixes[i];
      batch.Delete(keys[i]);
    }

    s = db->Write(writeoptions, &batch);
    return s;
  }

//给定一个键k和值v，这会得到k+“0”、“k+“1”和k+“2”的值。
//在同一快照中，并验证所有值是否相同。
//假设putmany用于将（k，v）放入数据库。
  Status GetMany(DB* db, const ReadOptions& readoptions, const Slice& key,
                 std::string* value) {
    std::string suffixes[3] = {"0", "1", "2"};
    std::string keys[3];
    Slice key_slices[3];
    std::string values[3];
    ReadOptions readoptionscopy = readoptions;
    readoptionscopy.snapshot = db->GetSnapshot();
    Status s;
    for (int i = 0; i < 3; i++) {
      keys[i] = key.ToString() + suffixes[i];
      key_slices[i] = keys[i];
      s = db->Get(readoptionscopy, key_slices[i], value);
      if (!s.ok() && !s.IsNotFound()) {
        fprintf(stderr, "get error: %s\n", s.ToString().c_str());
        values[i] = "";
//我们在出错后继续，而不是退出以便
//查找更多错误（如果有）
      } else if (s.IsNotFound()) {
        values[i] = "";
      } else {
        values[i] = *value;
      }
    }
    db->ReleaseSnapshot(readoptionscopy.snapshot);

    if ((values[0] != values[1]) || (values[1] != values[2])) {
      fprintf(stderr, "inconsistent values for key %s: %s, %s, %s\n",
              key.ToString().c_str(), values[0].c_str(), values[1].c_str(),
              values[2].c_str());
//我们在出错后继续，而不是退出以便
//查找更多错误（如果有）
    }

    return s;
  }

//与readrandomwriterandom的区别在于：
//（A）使用getmany/putmany读取/写入键值。参考这些功能。
//（B）也会删除（每个标志的删除百分比）
//（c）为了在查找过程中获得“发现”的高百分比，以及
//它使用的多次写入（包括Put和Delete）最多
//标记“num distinct distinct keys”而不是标记“num distinct keys”。
//（d）没有multiget选项。
  void RandomWithVerify(ThreadState* thread) {
    ReadOptions options(FLAGS_verify_checksum, true);
    RandomGenerator gen;
    std::string value;
    int64_t found = 0;
    int get_weight = 0;
    int put_weight = 0;
    int delete_weight = 0;
    int64_t gets_done = 0;
    int64_t puts_done = 0;
    int64_t deletes_done = 0;

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);

//迭代次数是读写次数中的较大者
    for (int64_t i = 0; i < readwrites_; i++) {
      DB* db = SelectDB(thread);
      if (get_weight == 0 && put_weight == 0 && delete_weight == 0) {
//一个批处理完成，为下一批重新初始化
        get_weight = FLAGS_readwritepercent;
        delete_weight = FLAGS_deletepercent;
        put_weight = 100 - get_weight - delete_weight;
      }
      GenerateKeyFromInt(thread->rand.Next() % FLAGS_numdistinct,
          FLAGS_numdistinct, &key);
      if (get_weight > 0) {
//先做所有的事
        Status s = GetMany(db, options, key, &value);
        if (!s.ok() && !s.IsNotFound()) {
          fprintf(stderr, "getmany error: %s\n", s.ToString().c_str());
//我们在出错后继续，而不是退出以便
//查找更多错误（如果有）
        } else if (!s.IsNotFound()) {
          found++;
        }
        get_weight--;
        gets_done++;
        thread->stats.FinishedOps(&db_, db_.db, 1, kRead);
      } else if (put_weight > 0) {
//然后做所有相应数量的投入
//为了我们早先所做的一切
        Status s = PutMany(db, write_options_, key, gen.Generate(value_size_));
        if (!s.ok()) {
          fprintf(stderr, "putmany error: %s\n", s.ToString().c_str());
          exit(1);
        }
        put_weight--;
        puts_done++;
        thread->stats.FinishedOps(&db_, db_.db, 1, kWrite);
      } else if (delete_weight > 0) {
        Status s = DeleteMany(db, write_options_, key);
        if (!s.ok()) {
          fprintf(stderr, "deletemany error: %s\n", s.ToString().c_str());
          exit(1);
        }
        delete_weight--;
        deletes_done++;
        thread->stats.FinishedOps(&db_, db_.db, 1, kDelete);
      }
    }
    char msg[128];
    snprintf(msg, sizeof(msg),
             "( get:%" PRIu64 " put:%" PRIu64 " del:%" PRIu64 " total:%" \
             PRIu64 " found:%" PRIu64 ")",
             gets_done, puts_done, deletes_done, readwrites_, found);
    thread->stats.AddMessage(msg);
  }

//这与readwhilewriting不同，因为它不使用
//一根多余的线。
  void ReadRandomWriteRandom(ThreadState* thread) {
    ReadOptions options(FLAGS_verify_checksum, true);
    RandomGenerator gen;
    std::string value;
    int64_t found = 0;
    int get_weight = 0;
    int put_weight = 0;
    int64_t reads_done = 0;
    int64_t writes_done = 0;
    Duration duration(FLAGS_duration, readwrites_);

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);

//迭代次数是读写次数中的较大者
    while (!duration.Done(1)) {
      DB* db = SelectDB(thread);
      GenerateKeyFromInt(thread->rand.Next() % FLAGS_num, FLAGS_num, &key);
      if (get_weight == 0 && put_weight == 0) {
//一个批处理完成，为下一批重新初始化
        get_weight = FLAGS_readwritepercent;
        put_weight = 100 - get_weight;
      }
      if (get_weight > 0) {
//先做所有的事
        Status s = db->Get(options, key, &value);
        if (!s.ok() && !s.IsNotFound()) {
          fprintf(stderr, "get error: %s\n", s.ToString().c_str());
//我们在出错后继续，而不是退出以便
//查找更多错误（如果有）
        } else if (!s.IsNotFound()) {
          found++;
        }
        get_weight--;
        reads_done++;
        thread->stats.FinishedOps(nullptr, db, 1, kRead);
      } else  if (put_weight > 0) {
//然后做所有相应数量的投入
//为了我们早先所做的一切
        Status s = db->Put(write_options_, key, gen.Generate(value_size_));
        if (!s.ok()) {
          fprintf(stderr, "put error: %s\n", s.ToString().c_str());
          exit(1);
        }
        put_weight--;
        writes_done++;
        thread->stats.FinishedOps(nullptr, db, 1, kWrite);
      }
    }
    char msg[100];
    snprintf(msg, sizeof(msg), "( reads:%" PRIu64 " writes:%" PRIu64 \
             " total:%" PRIu64 " found:%" PRIu64 ")",
             reads_done, writes_done, readwrites_, found);
    thread->stats.AddMessage(msg);
  }

//
//随机键的读-修改-写
  void UpdateRandom(ThreadState* thread) {
    ReadOptions options(FLAGS_verify_checksum, true);
    RandomGenerator gen;
    std::string value;
    int64_t found = 0;
    int64_t bytes = 0;
    Duration duration(FLAGS_duration, readwrites_);

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);
//迭代次数是读写次数中的较大者
    while (!duration.Done(1)) {
      DB* db = SelectDB(thread);
      GenerateKeyFromInt(thread->rand.Next() % FLAGS_num, FLAGS_num, &key);

      auto status = db->Get(options, key, &value);
      if (status.ok()) {
        ++found;
        bytes += key.size() + value.size();
      } else if (!status.IsNotFound()) {
        fprintf(stderr, "Get returned an error: %s\n",
                status.ToString().c_str());
        abort();
      }

      if (thread->shared->write_rate_limiter) {
        thread->shared->write_rate_limiter->Request(
            /*.SIZE（）+值\u大小\，env:：io \u高，nullptr/*状态*/，
            ratelimiter:：optype:：kwrite）；
      }

      状态s=db->put（写入_选项_uuu，键，gen.generate（值_大小_uu））；
      如果（！）S.O.（））{
        fprintf（stderr，“放置错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
      bytes+=key.size（）+value_-size_u；
      线程->stats.finishedops（nullptr，db，1，kupdate）；
    }
    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg）、
             “（更新：%”priu64“找到：%”priu64“）”，readwrites，found）；
    thread->stats.addbytes（字节）；
    thread->stats.addmessage（msg）；
  }

  //对随机键进行读-修改-写操作。
  //每个操作都会导致键按值\大小增长（模拟追加）。
  //通常用于针对类似类型的合并进行基准测试
  void appendrandom（threadstate*thread）
    读取选项选项（标记_verify_checksum，true）；
    随机发生器Gen；
    std：：字符串值；
    Int64发现=0；
    Int64_t字节=0；

    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    //迭代次数为读写次数中的较大者
    持续时间（标记持续时间、读写时间）；
    而（！）持续时间。完成（1））
      db*db=selectdb（线程）；
      generateKeyFromInt（thread->rand.next（）%flags_num，flags_num，&key）；

      auto status=db->get（选项、键和值）；
      if（status.ok（））
        +发现；
        bytes+=key.size（）+value.size（）；
      否则，（如果）！status.isNotFound（））
        fprintf（stderr，“get返回一个错误：%s\n”，
                status.toString（）.c_str（））；
        中止（）；
      }否则{
        //如果不存在，则假定一个空的数据字符串
        值；
      }

      //更新值（通过附加数据）
      切片操作数=gen.generate（值_-size_uu）；
      if（value.size（）>0）
        //使用分隔符匹配StringAppendOperator的语义
        value.append（1，，'）；
      }
      value.append（operand.data（），operand.size（））；

      //写回数据库
      状态s=db->put（写入选项、键、值）；
      如果（！）S.O.（））{
        fprintf（stderr，“放置错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
      bytes+=key.size（）+value.size（）；
      线程->stats.finishedops（nullptr，db，1，kupdate）；
    }

    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg），“（更新时间：%”priu64“找到时间：%”priu64”）”，
            读写功能，找到）；
    thread->stats.addbytes（字节）；
    thread->stats.addmessage（msg）；
  }

  //读取、修改、写入随机键（使用mergeoperator）
  //要使用的合并运算符应由标志“merge”运算符定义
  //调整标志\值\大小，使键对于该运算符是合理的
  //假设合并运算符为非空（即：定义良好）
  / /
  //例如，使用标志_merge_operator=“uint64add”和标志_value_size=8
  //使用merge模拟64位整数上的随机加法。
  / /
  //同一个键上的合并数可以通过调整
  //标记\合并\键。
  void mergerandom（threadstate*线程）
    随机发生器Gen；
    Int64_t字节=0；
    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    //迭代次数为读写次数中的较大者
    持续时间（标记持续时间、读写时间）；
    而（！）持续时间。完成（1））
      db*db=selectdb（线程）；
      generateKeyFromInt（thread->rand.next（）%合并关键字，合并关键字，&key）；

      状态s=db->merge（写入_选项_uuu，键，gen.generate（值_大小_uu））；

      如果（！）S.O.（））{
        fprintf（stderr，“合并错误：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
      bytes+=key.size（）+value_-size_u；
      thread->stats.finishedops（nullptr，db，1，kmerge）；
    }

    //打印一些统计信息
    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg），“（updates:%”priu64“）”，readwrites_uuu）；
    thread->stats.addbytes（字节）；
    thread->stats.addmessage（msg）；
  }

  //读取和合并随机键。控制读取和合并的数量
  //通过调整标志_num和标志_mergereadpercent。distinct的数目
  //键（也就是同一键上的读取和合并数）可以是
  //用标志“合并”键调整。
  / /
  //与mergerandom一样，要使用的合并运算符应定义为
  //标记\合并\运算符。
  void readrandommergerandom（threadstate*thread）
    读取选项选项（标记_verify_checksum，true）；
    随机发生器Gen；
    std：：字符串值；
    int64_t num_hits=0；
    int64_t num_gets=0；
    Int64_t num_merges=0；
    尺寸t最大长度=0；

    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    //迭代次数为读写次数中的较大者
    持续时间（标记持续时间、读写时间）；
    而（！）持续时间。完成（1））
      db*db=selectdb（线程）；
      generateKeyFromInt（thread->rand.next（）%合并关键字，合并关键字，&key）；

      bool do_merge=int（thread->rand.next（）%100）<flags_mergereadpercent；

      如果（doth-归并）{
        状态s=db->merge（写入_选项_uuu，键，gen.generate（值_大小_uu））；
        如果（！）S.O.（））{
          fprintf（stderr，“合并错误：%s\n”，s.toString（）.c_str（））；
          退出（1）；
        }
        NuMyMySys++；
        thread->stats.finishedops（nullptr，db，1，kmerge）；
      }否则{
        状态s=db->get（选项、键和值）；
        if（value.length（）>最大长度）
          最大长度=value.length（）；

        如果（！）&！s.isnotfound（））
          fprintf（stderr，“获取错误：%s\n”，s.toString（）.c_str（））；
          //出错后继续，而不是退出，以便
          //查找更多错误（如果有）
        否则，（如果）！s.isnotfound（））
          数字点击+；
        }
        NuffiGIS++；
        thread->stats.finishedops（nullptr，db，1，kread）；
      }
    }

    CHMA-MSG〔100〕；
    snprintf（msg，sizeof（msg）、
             “（读取：%”priu64“合并：%”priu64“总计：%”priu64
             “点击率：%”priu64“最大长度：%”rocksdb“u priszt”）“，
             num_gets，num_merges，readwrites_u，num_hits，max_length）；
    thread->stats.addmessage（msg）；
  }

  void writeSeqSeekseq（threadstate*thread）
    写入_u=flags_num；
    陶土（螺纹，连续）；
    //从ops/sec计算中排除写入
    thread->stats.start（thread->tid）；

    db*db=selectdb（线程）；
    std:：unique_ptr<迭代器>iter（
      db->newiterator（readoptions（flags_verify_checksum，true））；

    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    对于（int64_t i=0；i<flags_num；++i）
      generateKeyFromInt（i，flags_num，&key）；
      iter->seek（键）；
      断言（iter->valid（）&&iter->key（）==key）；
      thread->stats.finishedops（nullptr，db，1，kseek）；

      for（int j=0；j<flags_seek&&i+1<flags_num；++j）
        如果（！）标记_Reverse_Iterator）
          ITER > NEXT（）；
        }否则{
          ITER > >（）；
        }
        generateKeyFromInt（++i，flags_num，&key）；
        断言（iter->valid（）&&iter->key（）==key）；
        thread->stats.finishedops（nullptr，db，1，kseek）；
      }

      iter->seek（键）；
      断言（iter->valid（）&&iter->key（）==key）；
      thread->stats.finishedops（nullptr，db，1，kseek）；
    }
  }

ifndef岩石
  //此基准压力测试事务。在给定的时间内（或
  //总--writes数，事务将执行read-modify-write
  //在n个（--transaction set）集合中的每个集合中增加键的值
  //键（每个集合都有--num键）。如果设置了线程，则
  //并行完成。
  / /
  //要测试事务，请使用--transaction_db=true。不设置此
  / /参数
  //将运行相同的基准，而不运行事务。
  / /
  //然后randomTransactionVerify（）将验证结果的正确性
  //通过检查每个集合中所有键的总和是否相同。
  void randomTransaction（threadstate*thread）
    读取选项选项（标记_verify_checksum，true）；
    持续时间（标记持续时间、读写时间）；
    读取选项读取选项（标记验证校验和，真）；
    uint16_t num_prefix_ranges=static_cast<uint16_t>（flags_transaction_sets）；
    uint64事务处理完成=0；

    if（num_prefix_ranges==0 num_prefix_ranges>9999）
      fprintf（stderr，“事务集的值无效”）；
      中止（）；
    }

    交易选项txn_选项；
    txn_options.lock_timeout=flags_transaction_lock_timeout；
    txn_options.set_snapshot=flags_transaction_set_snapshot；

    RandomTransactionInserter插入器（&thread->rand，写入选项）
                                       读取选项，标记编号，
                                       num_前缀_范围）；

    if（flags_num_multi_db>1）
      FPrTNF（STDER）
              “无法使用运行RandomTransaction基准”
              “flags_multi_db>1.”）；
      中止（）；
    }

    而（！）持续时间。完成（1））
      成功；

      //RandomTransactionInsert将尝试为每个
      //标志事务集
      if（flags_optimatic_transaction_db）
        成功=inserter.optimatictransactiondinsert（db_u.opt_txn_db）；
      else if（flags_transaction_db）
        transactiondb*txn_db=reinterpret_cast<transactiondb*>（db_u.db）；
        成功=inserter.transactiondinsert（txn_db，txn_options）；
      }否则{
        success=插入器.dbinsert（db_.db）；
      }

      如果（！）成功）
        fprintf（stderr，“意外错误：%s\n”，
                inserter.getLastStatus（）.toString（）.c_str（））；
        中止（）；
      }

      thread->stats.finishedops（nullptr，db_.db，1，kothers）；
      交易完成+
    }

    CHMA-MSG〔100〕；
    if（Flags_optimatic_transaction_db_Flags_transaction_db）
      snprintf（msg，sizeof（msg）、
               “（交易：%”priu64“中止：%”priu64”）”，
               事务处理完成，inserter.getFailureCount（））；
    }否则{
      snprintf（msg，sizeof（msg），“（批数百分比：priu64”）”，事务处理完成；
    }
    thread->stats.addmessage（msg）；

    if（flags_perf_level>rocksdb:：perf level:：kdisable）
      thread->stats.addmessage（get_perf_context（）->toString（））；
    }
  }

  //在运行randomTransaction（）后验证数据的一致性。
  //因为RandomTransaction（）的每次迭代都增加了每个集合中的一个键
  //相同的值，每组键的和应该相同。
  void randomTransactionVerify（）
    如果（！）标记“事务”数据库&！标记“乐观”事务
      //未使用事务，没有要验证的内容。
      返回；
    }

    状态S＝
        RandomTransactionSert：：验证（db_.db，
                            static_cast<uint16_t>（flags_transaction_sets））；

    如果（S.O.（））{
      fprintf（stdout，“RandomTransactionVerify成功。\n”）；
    }否则{
      fprintf（stdout，“RandomTransactionVerify失败！！\n“”；
    }
  }
endif//rocksdb_lite

  //写入和删除随机键而不覆盖键。
  / /
  //此基准旨在部分复制myrock的行为
  //辅助索引：所有数据都存储在键中，更新由
  //删除密钥的旧版本并插入新版本。
  void randomreplaceKeys（threadstate*thread）
    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；
    std:：vector<uint32_t>计数器（flags_numdistinct，0）；
    尺寸t最大值计数器=50；
    随机发生器Gen；

    状态S；
    db*db=selectdb（线程）；
    for（int64_t i=0；i<flags_numdistinct；i++）
      generatekeyfromint（i*max_counter，flags_num，&key）；
      s=db->put（写入_选项_uuu，键，gen.generate（值_大小_uu））；
      如果（！）S.O.（））{
        fprintf（stderr，“操作失败：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }
    }

    db->getSnapshot（）；

    std：：默认“随机”引擎生成器；
    std:：normal_distribution<double>distribution（flags_numdistinct/2.0，
                                                  FrassH-STDDEV）；
    持续时间（Flags_Duration，Flags_Num）；
    而（！）持续时间。完成（1））
      int64_t rnd_id=static_cast<int64_t>（distribution（generator））；
      int64_t key_id=std:：max（std:：min（flags_numdistinct-1，rnd_id）、
                                静态_cast<int64_t>（0））；
      generatekeyfromint（key_id*max_counter+counters[key_id]，flags_num，
                         关键）；
      S=标记\使用\单\删除？db->singledelete（写入选项，键）
                                   ：db->delete（写入选项，键）；
      如果（S.O.（））{
        counters[key_id]=（counters[key_id]+1）%max_counter；
        generatekeyfromint（key_id*max_counter+counters[key_id]，flags_num，
                           关键）；
        s=db->put（write_options_uu，key，slice（））；
      }

      如果（！）S.O.（））{
        fprintf（stderr，“操作失败：%s\n”，s.toString（）.c_str（））；
        退出（1）；
      }

      线程->stats.finishedops（nullptr，db，1，kothers）；
    }

    CHMA-MSG〔200〕；
    snprintf（msg，sizeof（msg）、
             “使用单个删除：%d，”
             “标准偏差：%lf\n”，
             标记\使用\单\删除，标记\ stdev）；
    thread->stats.addmessage（msg）；
  }

  void timeeriesreaderdelete（threadstate*thread，bool do_delete）
    读取选项选项（标记_verify_checksum，true）；
    Int64读=0；
    Int64发现=0；
    Int64_t字节=0；

    迭代器*iter=nullptr；
    //仅在单个数据库上工作
    断言（dv.db）！= null pTr）；
    iter=db_.db->newiterator（选项）；

    std:：unique_ptr<const char[]>key_guard；
    slice key=allocatekey（&key_guard）；

    char值_buffer[256]；
    当（真）{
      {
        mutexlock l（&thread->shared->mu）；
        if（thread->shared->num_done-=1）
          //写入线程已完成
          断裂；
        }
      }
      如果（！）标记_使用_tailing_迭代器）
        删除ITER；
        iter=db_.db->newiterator（选项）；
      }
      //选择要使用的迭代器

      int64_t key_id=thread->rand.next（）%标志_key_id_range；
      generatekeyfromint（key_id，flags_num，&key）；
      //将最后8个字节重置为0
      char*start=const_cast<char*>（key.data（））；
      start+=key.size（）-8；
      memset（开始，0，8）；
      ++阅读；

      bool key_found=false；
      //查找前缀
      for（iter->seek（key）；iter->valid（）&&iter->key（）。以（key）开头；
           ITER > NEXT（））{
        key_found=真；
        //复制迭代器的值以确保我们读取了它们。
        如果（删除）
          bytes+=iter->key（）.size（）；
          if（keyExpired（timestamp_emulator_.get（），iter->key（））
            thread->stats.finishedops（&db_uudb_udb，1，kdelete）；
            db->delete（写入选项，iter->key（））；
          }否则{
            断裂；
          }
        }否则{
          bytes+=iter->key（）.size（）+iter->value（）.size（）；
          thread->stats.finishedops（&db_uudb_udb，1，kread）；
          slice value=iter->value（）；
          memcpy（value_buffer，value.data（），
                 std:：min（value.size（），sizeof（value_buffer））；

          断言（iter->status（）.ok（））；
        }
      }
      found+=找到钥匙；

      如果（thread->shared->read_rate_limiter.get（）！= null pTr）{
        线程->共享->读取速率限制器->请求（
            1，env:：io_high，nullptr/*状态*/, RateLimiter::OpType::kRead);

      }
    }
    delete iter;

    char msg[100];
    snprintf(msg, sizeof(msg), "(%" PRIu64 " of %" PRIu64 " found)", found,
             read);
    thread->stats.AddBytes(bytes);
    thread->stats.AddMessage(msg);
    if (FLAGS_perf_level > rocksdb::PerfLevel::kDisable) {
      thread->stats.AddMessage(get_perf_context()->ToString());
    }
  }

  void TimeSeriesWrite(ThreadState* thread) {
//在其他线程完成之前一直保持写入的特殊线程。
    RandomGenerator gen;
    int64_t bytes = 0;

//不要将此线程的统计信息与读者合并。
    thread->stats.SetExcludeFromMerge();

    std::unique_ptr<RateLimiter> write_rate_limiter;
    if (FLAGS_benchmark_write_rate_limit > 0) {
      write_rate_limiter.reset(
          NewGenericRateLimiter(FLAGS_benchmark_write_rate_limit));
    }

    std::unique_ptr<const char[]> key_guard;
    Slice key = AllocateKey(&key_guard);

    Duration duration(FLAGS_duration, writes_);
    while (!duration.Done(1)) {
      DB* db = SelectDB(thread);

      uint64_t key_id = thread->rand.Next() % FLAGS_key_id_range;
//写入密钥ID
      GenerateKeyFromInt(key_id, FLAGS_num, &key);
//写入时间戳

      char* start = const_cast<char*>(key.data());
      char* pos = start + 8;
      int bytes_to_fill =
          std::min(key_size_ - static_cast<int>(pos - start), 8);
      uint64_t timestamp_value = timestamp_emulator_->Get();
      if (port::kLittleEndian) {
        for (int i = 0; i < bytes_to_fill; ++i) {
          pos[i] = (timestamp_value >> ((bytes_to_fill - i - 1) << 3)) & 0xFF;
        }
      } else {
        memcpy(pos, static_cast<void*>(&timestamp_value), bytes_to_fill);
      }

      timestamp_emulator_->Inc();

      Status s;

      s = db->Put(write_options_, key, gen.Generate(value_size_));

      if (!s.ok()) {
        fprintf(stderr, "put error: %s\n", s.ToString().c_str());
        exit(1);
      }
      bytes = key.size() + value_size_;
      thread->stats.FinishedOps(&db_, db_.db, 1, kWrite);
      thread->stats.AddBytes(bytes);

      if (FLAGS_benchmark_write_rate_limit > 0) {
        write_rate_limiter->Request(
            entries_per_batch_ * (value_size_ + key_size_), Env::IO_HIGH,
            /*lptr/*stats*/，ratelimiter:：optype:：kWrite）；
      }
    }
  }

  空时间序列（threadstate*thread）
    if（thread->tid>0）
      bool do_deletion=flags_expire_style==“删除”&&
                         thread->tid<=flags_num_deletion_threads；
      TimeSeriesReaderDelete（线程，删除）；
    }否则{
      时间周期写入（线程）；
      thread->stats.stop（）；
      thread->stats.report（“timeseries write”）；
    }
  }

  空隙压实（螺纹状态*螺纹）
    db*db=selectdb（线程）；
    db->compactRange（compactRangeOptions（），nullptr，nullptr）；
  }

  void compactall（）
    如果（dB.dB！= null pTr）{
      db->compactRange（compactRangeOptions（），nullptr，nullptr）；
    }
    对于（const auto&db_with_cfh:multi_dbs_
      db_with_cfh.db->compactRange（compactRangeOptions（），nullptr，nullptr）；
    }
  }

  void resetstats（）
    如果（dB.dB！= null pTr）{
      db->resetstats（）；
    }
    对于（const auto&db_with_cfh:multi_dbs_
      db_with_cfh.db->resetstats（）；
    }
  }

  void printstats（const char*key）
    如果（dB.dB！= null pTr）{
      printstats（db_.db，key，false）；
    }
    对于（const auto&db_with_cfh:multi_dbs_
      printstats（db_with_cfh.db，key，真）；
    }
  }

  void printstats（db*db，const char*key，bool print_header=false）
    if（打印头）
      fprintf（stdout，“\n===db:%s==\n”，db->getname（）.c_str（））；
    }
    std：：字符串状态；
    如果（！）db->getproperty（key，&stats））
      stats=“（失败）”；
    }
    fprintf（stdout，“\n%s\n”，stats.c_str（））；
  }
}；

int db_bench_tool（int argc，char**argv）
  rocksdb:：port:：installstacktracehandler（）；
  静态bool initialized=false；
  如果（！）初始化）{
    setusagemessage（std:：string（“\n用法：\n”）+std:：string（argv[0]）+
                    “[选项]…”）；
    初始化=真；
  }
  parseCommandLineFlags（&argc，&argv，true）；
  flags_compaction_style_e=（rocksdb:：compactionStyle）flags_compaction_style；
ifndef岩石
  如果（标记“统计”&&！标记_statistics_string.empty（））
    FPrTNF（STDER）
            “不能同时提供--statistics和--statistics_字符串。\n”）；
    退出（1）；
  }
  如果（！）标记_statistics_string.empty（））
    std:：unique_ptr<statistics>custom_stats_guard；
    dbstats.reset（newcustomobject<statistics>（flags_statistics_string，
                                              &custom_stats_guard））；
    自定义_stats_guard.release（）；
    if（dbstats==nullptr）
      fprintf（stderr，“没有注册匹配字符串的统计信息：%s\n”，
              标记_statistics_string.c_str（））；
      退出（1）；
    }
  }
endif//rocksdb_lite
  if（标记_statistics）
    dbstats=rocksdb:：createdbstatistics（）；
  }
  旗子_compaction_pri_e=（rocksdb:：compaction pri）旗子_compaction_pri；

  std:：vector<std:：string>fanout=rocksdb:：stringssplit（
      标记_max_bytes_表示_level_乘数_additional，，'）；
  对于（尺寸_t j=0；j<fanout.size（）；j++）
    标记_max_bytes_用于_level_multiplier_additional_v.push_back（
C.I.IFNDEF CygWin
        std:：stoi（fanout[j]）；
否则
        斯托伊（Fanout[J]）；
第二节
  }

  标志_压缩_类型_e=
    StringToCompressionType（flags_compression_type.c_str（））；

ifndef岩石
  std:：unique_ptr<env>自定义_env_guard；
  如果（！）标记_hdfs.empty（）&！标志_env_uri.empty（））
    fprintf（stderr，“不能同时提供--hdfs和--env-uri。\n”）；
    退出（1）；
  否则，（如果）！标志_env_uri.empty（））
    flags_env=newcustomobject<env>（flags_env_uri，&custom_env_guard）；
    if（flags_env==nullptr）
      fprintf（stderr，“没有为uri注册env:%s\n”，flags_env_uri.c_str（））；
      退出（1）；
    }
  }
endif//rocksdb_lite
  如果（！）标记_hdfs.empty（））
    flags_env=new rocksdb:：hdfs env（flags_hdfs）；
  }

  如果（！）strcasecmp（flags_compaction_fadvice.c_str（），“none”））
    flags_compaction_fadvice_e=rocksdb:：options:：none；
  否则如果（！）strcasecmp（flags_compaction_fadvice.c_str（），“normal”））
    flags_compaction_fadvice_e=rocksdb:：options:：normal；
  否则如果（！）strcasecmp（flags_compaction_fadvice.c_str（），“sequential”））
    flags_compaction_fadvice_e=rocksdb:：options:：sequential；
  否则如果（！）strcasecmp（flags_compaction_fadvice.c_str（），“willneed”））
    flags_compaction_fadvice_e=rocksdb:：options:：willneed；
  否则{
    fprintf（stdout，“未知压缩衰减设备：%s\n”，
            标记_compaction_fadvice.c_str（））；
  }

  flags_rep_factory=stringtorepfactory（flags_memtablerep.c_str（））；

  //注意选项清理可能会根据
  //max_background_刷新/max_background_压缩/max_background_作业
  flags_env->backgroundthreads（flags_num_high_pri_threads，
                                  rocksdb:：env:：priority:：high）；
  flags_env->backgroundthreads（flags_num_bottom_pri_threads，
                                  rocksdb:：env:：priority:：bottom）；
  flags_env->backgroundthreads（flags_num_low_pri_threads，
                                  rocksdb:：env:：priority:：low）；

  //如果没有给定测试数据库的位置--db=<path>
  if（flags_db.empty（））
    std：：字符串默认值_db_path；
    rocksdb:：env:：default（）->gettestdirectory（&default_db_path）；
    默认值为“/dbbench”；
    flags_db=默认_db_路径；
  }

  if（标记_stats_interval_seconds>0）
    //当两者都设置时，标志\u stats \u interval确定频率
    //检查计时器的标志\u stats \u interval \u seconds
    标志_stats_interval=1000；
  }

  rocksdb：基准基准；
  benchmark.run（）；
  返回0；
}
//命名空间rocksdb
第二节
