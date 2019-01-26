
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

#include "rocksdb/compaction_job_stats.h"

namespace rocksdb {

#ifndef ROCKSDB_LITE

void CompactionJobStats::Reset() {
  elapsed_micros = 0;

  num_input_records = 0;
  num_input_files = 0;
  num_input_files_at_output_level = 0;

  num_output_records = 0;
  num_output_files = 0;

  is_manual_compaction = 0;

  total_input_bytes = 0;
  total_output_bytes = 0;

  num_records_replaced = 0;

  total_input_raw_key_bytes = 0;
  total_input_raw_value_bytes = 0;

  num_input_deletion_records = 0;
  num_expired_deletion_records = 0;

  num_corrupt_keys = 0;

  file_write_nanos = 0;
  file_range_sync_nanos = 0;
  file_fsync_nanos = 0;
  file_prepare_write_nanos = 0;

  num_single_del_fallthru = 0;
  num_single_del_mismatch = 0;
}

void CompactionJobStats::Add(const CompactionJobStats& stats) {
  elapsed_micros += stats.elapsed_micros;

  num_input_records += stats.num_input_records;
  num_input_files += stats.num_input_files;
  num_input_files_at_output_level += stats.num_input_files_at_output_level;

  num_output_records += stats.num_output_records;
  num_output_files += stats.num_output_files;

  total_input_bytes += stats.total_input_bytes;
  total_output_bytes += stats.total_output_bytes;

  num_records_replaced += stats.num_records_replaced;

  total_input_raw_key_bytes += stats.total_input_raw_key_bytes;
  total_input_raw_value_bytes += stats.total_input_raw_value_bytes;

  num_input_deletion_records += stats.num_input_deletion_records;
  num_expired_deletion_records += stats.num_expired_deletion_records;

  num_corrupt_keys += stats.num_corrupt_keys;

  file_write_nanos += stats.file_write_nanos;
  file_range_sync_nanos += stats.file_range_sync_nanos;
  file_fsync_nanos += stats.file_fsync_nanos;
  file_prepare_write_nanos += stats.file_prepare_write_nanos;

  num_single_del_fallthru += stats.num_single_del_fallthru;
  num_single_del_mismatch += stats.num_single_del_mismatch;
}

#else

void CompactionJobStats::Reset() {}

void CompactionJobStats::Add(const CompactionJobStats& stats) {}

#endif  //！摇滚乐

}  //命名空间rocksdb
