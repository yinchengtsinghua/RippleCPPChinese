
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

#ifndef ROCKSDB_LITE
#include "utilities/table_properties_collectors/compact_on_deletion_collector.h"

#include <memory>
#include "rocksdb/utilities/table_properties_collectors.h"

namespace rocksdb {

CompactOnDeletionCollector::CompactOnDeletionCollector(
    size_t sliding_window_size,
    size_t deletion_trigger) {
  deletion_trigger_ = deletion_trigger;

//首先，计算每个bucket中的键数。
  bucket_size_ =
      (sliding_window_size + kNumBuckets - 1) / kNumBuckets;
  assert(bucket_size_ > 0U);

  Reset();
}

void CompactOnDeletionCollector::Reset() {
  for (int i = 0; i < kNumBuckets; ++i) {
    num_deletions_in_buckets_[i] = 0;
  }
  current_bucket_ = 0;
  num_keys_in_current_bucket_ = 0;
  num_deletions_in_observation_window_ = 0;
  need_compaction_ = false;
}

//当新的键/值对插入到
//表。
//@params key插入表中的用户键。
//@params value插入表中的值。
//@params file_当前文件大小
Status CompactOnDeletionCollector::AddUserKey(
    const Slice& key, const Slice& value,
    EntryType type, SequenceNumber seq,
    uint64_t file_size) {
  if (need_compaction_) {
//如果输出文件已经需要压缩，则跳过检查。
    return Status::OK();
  }

  if (num_keys_in_current_bucket_ == bucket_size_) {
//当当前存储桶满时，将光标移到
//将缓冲环打到下一个桶。
    current_bucket_ = (current_bucket_ + 1) % kNumBuckets;

//通过排除，更新观察到的删除键的当前计数
//中最旧存储桶中的删除键数
//观察窗。
    assert(num_deletions_in_observation_window_ >=
        num_deletions_in_buckets_[current_bucket_]);
    num_deletions_in_observation_window_ -=
        num_deletions_in_buckets_[current_bucket_];
    num_deletions_in_buckets_[current_bucket_] = 0;
    num_keys_in_current_bucket_ = 0;
  }

  num_keys_in_current_bucket_++;
  if (type == kEntryDelete) {
    num_deletions_in_observation_window_++;
    num_deletions_in_buckets_[current_bucket_]++;
    if (num_deletions_in_observation_window_ >= deletion_trigger_) {
      need_compaction_ = true;
    }
  }
  return Status::OK();
}

TablePropertiesCollector*
CompactOnDeletionCollectorFactory::CreateTablePropertiesCollector(
    TablePropertiesCollectorFactory::Context context) {
  return new CompactOnDeletionCollector(
      sliding_window_size_, deletion_trigger_);
}

std::shared_ptr<TablePropertiesCollectorFactory>
    NewCompactOnDeletionCollectorFactory(
        size_t sliding_window_size,
        size_t deletion_trigger) {
  return std::shared_ptr<TablePropertiesCollectorFactory>(
      new CompactOnDeletionCollectorFactory(
          sliding_window_size, deletion_trigger));
}
}  //命名空间rocksdb
#endif  //！摇滚乐
