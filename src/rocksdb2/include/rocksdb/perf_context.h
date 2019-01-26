
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

#ifndef STORAGE_ROCKSDB_INCLUDE_PERF_CONTEXT_H
#define STORAGE_ROCKSDB_INCLUDE_PERF_CONTEXT_H

#include <stdint.h>
#include <string>

#include "rocksdb/perf_level.h"

namespace rocksdb {

//用于高效收集性能计数器的线程本地上下文
//透明地。
//使用setperflevel（perflevel:：kenabletime）启用时间统计。

struct PerfContext {

void Reset(); //将所有性能计数器重置为零

  std::string ToString(bool exclude_zero_counters = false) const;

uint64_t user_key_comparison_count; //用户密钥比较总数
uint64_t block_cache_hit_count;     //块缓存命中总数
uint64_t block_read_count;          //块读取总数（带IO）
uint64_t block_read_byte;           //块读取的字节总数
uint64_t block_read_time;           //块读取花费的Nano总数
uint64_t block_checksum_time;       //块校验和花费的Nano总数
uint64_t block_decompress_time;  //块解压缩花费的总Nano

uint64_t get_read_bytes;       //GET返回的VAL字节数
uint64_t multiget_read_bytes;  //multiget返回的VAL字节数
uint64_t iter_read_bytes;      //由迭代器解码的键/值的字节数

//迭代期间跳过的内部键总数。
//原因有几个：
//1。调用next（）时，迭代器位于上一个
//钥匙，这样我们就需要跳过它。这意味着这个柜台总是
//在next（）中递增。
//2。调用next（）时，需要跳过前面的内部条目
//覆盖的键。
//三。调用next（）时，seek（）或seektofirst（）位于上一个键之后
//在调用next（）之前，seek（）中的seek键或
//请参见ktofirst（），在下一个键之前可能有一个或多个已删除的键
//操作应将迭代器放置到的有效键。我们需要
//跳过墓碑和墓碑隐藏的更新。这个
//此计数器中不包括墓碑，而以前的更新
//墓碑所隐藏的将包括在这里。
//4。prev（）和seektolast（）的对称事例
//此计数器不包括内部“最近跳过的”计数。
//
  uint64_t internal_key_skipped_count;
//迭代期间跳过的删除和单个删除的总数
//调用next（）时，seek（）或seektofirst（）位于前一位置之后
//在调用next（）之前，seek（）中的seek键或
//请参见ktofirst（），在下一个有效的键之前可能有一个或多个已删除的键
//关键。每删除一个键都会计数一次。如果有的话，我们不在这里重新计算
//墓碑使旧的更新失效。
//
  uint64_t internal_delete_skipped_count;
//迭代器跳过最近的内部键的次数
//而不是迭代器正在使用的快照。
//
  uint64_t internal_recent_skipped_count;
//迭代器向合并运算符输入了多少值。
//
  uint64_t internal_merge_count;

uint64_t get_snapshot_time;       //用于获取快照的Nano总数
uint64_t get_from_memtable_time;  //用于查询memtables的Nano总数
uint64_t get_from_memtable_count;    //查询的MEM表数
//get（）找到密钥后花费的Nano总数
  uint64_t get_post_process_time;
uint64_t get_from_output_files_time;  //从输出文件读取的Nano总数
//用于搜索memtable的Nano总数
  uint64_t seek_on_memtable_time;
//在memtable上发出的查找数
//（包括SeekForRev，但不包括SeekToFirst和SeekToLast）
  uint64_t seek_on_memtable_count;
//在memtable上发出的next（）的数目
  uint64_t next_on_memtable_count;
//在memtable上发出的prev（）的数目
  uint64_t prev_on_memtable_count;
//在寻找儿童ITER上花费的Nano总数
  uint64_t seek_child_seek_time;
//子迭代器中发出的查找数
  uint64_t seek_child_seek_count;
uint64_t seek_min_heap_time;  //在合并最小堆上花费的Nano总数
uint64_t seek_max_heap_time;  //在合并最大堆上花费的Nano总数
//用于查找内部条目的Nano总数
  uint64_t seek_internal_seek_time;
//迭代内部条目以查找下一个用户条目所花费的Nano总数
  uint64_t find_next_user_entry_time;

//Nano花在给Wal写信上的总费用
  uint64_t write_wal_time;
//用于写入MEM表的Nano总数
  uint64_t write_memtable_time;
//延迟写入所花费的Nano总数
  uint64_t write_delay_time;
//总花费在记录上的纳米技术，不包括以上三次
  uint64_t write_pre_and_post_process_time;

uint64_t db_mutex_lock_nanos;      //获取db mutex所花费的时间。
//等待使用db mutex创建的条件变量所花费的时间。
  uint64_t db_condition_wait_nanos;
//在合并运算符上花费的时间。
  uint64_t merge_operator_time_nanos;

//从块缓存或sst文件读取索引块所花费的时间
  uint64_t read_index_block_nanos;
//从块缓存或sst文件读取筛选器块所花费的时间
  uint64_t read_filter_block_nanos;
//创建数据块迭代器所花费的时间
  uint64_t new_table_block_iter_nanos;
//创建sst文件的迭代器所花费的时间。
  uint64_t new_table_iterator_nanos;
//在数据/索引块中查找键所花费的时间
  uint64_t block_seek_nanos;
//查找或创建表读取器所花费的时间
  uint64_t find_table_nanos;
//MEM表Bloom命中总数
  uint64_t bloom_memtable_hit_count;
//MEM表Bloom未命中总数
  uint64_t bloom_memtable_miss_count;
//SST表Bloom命中总数
  uint64_t bloom_sst_hit_count;
//SST表Bloom未命中总数
  uint64_t bloom_sst_miss_count;

//在env文件系统操作中花费的总时间。这些只是填充的
//当使用timedenv时。
  uint64_t env_new_sequential_file_nanos;
  uint64_t env_new_random_access_file_nanos;
  uint64_t env_new_writable_file_nanos;
  uint64_t env_reuse_writable_file_nanos;
  uint64_t env_new_random_rw_file_nanos;
  uint64_t env_new_directory_nanos;
  uint64_t env_file_exists_nanos;
  uint64_t env_get_children_nanos;
  uint64_t env_get_children_file_attributes_nanos;
  uint64_t env_delete_file_nanos;
  uint64_t env_create_dir_nanos;
  uint64_t env_create_dir_if_missing_nanos;
  uint64_t env_delete_dir_nanos;
  uint64_t env_get_file_size_nanos;
  uint64_t env_get_file_modification_time_nanos;
  uint64_t env_rename_file_nanos;
  uint64_t env_link_file_nanos;
  uint64_t env_lock_file_nanos;
  uint64_t env_unlock_file_nanos;
  uint64_t env_new_logger_nanos;
};

//获取线程本地PerfContext对象指针
//如果已定义（nPerf_上下文），则指针不是线程本地指针
PerfContext* get_perf_context();

}

#endif
