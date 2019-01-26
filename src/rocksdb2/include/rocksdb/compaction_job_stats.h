
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

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

namespace rocksdb {
struct CompactionJobStats {
  CompactionJobStats() { Reset(); }
  void Reset();
//使用此实例聚合另一个实例的CompactionJobstats
  void Add(const CompactionJobStats& stats);

//压缩所用的时间（以微秒计）。
  uint64_t elapsed_micros;

//压缩输入记录的数目。
  uint64_t num_input_records;
//压缩输入文件的数目。
  size_t num_input_files;
//输出级别的压缩输入文件数。
  size_t num_input_files_at_output_level;

//压缩输出记录数。
  uint64_t num_output_records;
//压缩输出文件的数目。
  size_t num_output_files;

//如果压缩是手动压缩，则为真
  bool is_manual_compaction;

//压缩输入的大小（字节）。
  uint64_t total_input_bytes;
//压缩输出的大小（字节）。
  uint64_t total_output_bytes;

//被与同一关键字关联的新记录替换的记录数。
//这可能是该键的新值或删除项，因此此字段
//汇总所有更新和删除的密钥
  uint64_t num_records_replaced;

//未压缩的输入键的总和（字节）。
  uint64_t total_input_raw_key_bytes;
//未压缩输入值的总和（字节）。
  uint64_t total_input_raw_value_bytes;

//压缩前的删除条目数。删除条目
//压实后会消失，因为它们过期了
  uint64_t num_input_deletion_records;
//发现过时和丢弃的删除记录数
//因为不能用这个条目删除更多的键
//（即所有可能的删除操作都已完成）
  uint64_t num_expired_deletion_records;

//损坏的键数（ParseInternalKey在应用于时返回False
//键）遇到并写出。
  uint64_t num_corrupt_keys;

//以下计数器仅在
//options.report_bg_io_stats=true；

//在文件的append（）调用上花费的时间。
  uint64_t file_write_nanos;

//用于同步文件范围的时间。
  uint64_t file_range_sync_nanos;

//在文件fsync上花费的时间。
  uint64_t file_fsync_nanos;

//准备文件写入所花费的时间（falocate等）
  uint64_t file_prepare_write_nanos;

//0结束的字符串，存储最小的前8个字节，以及
//输出中的最大键。
  static const size_t kMaxPrefixLength = 8;

  std::string smallest_output_key_prefix;
  std::string largest_output_key_prefix;

//不符合Put的单个删除数
  uint64_t num_single_del_fallthru;

//满足Put以外的其他内容的单个删除数
  uint64_t num_single_del_mismatch;
};
}  //命名空间rocksdb
