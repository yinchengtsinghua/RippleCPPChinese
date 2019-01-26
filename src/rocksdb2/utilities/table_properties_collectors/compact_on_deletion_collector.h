
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

#ifndef ROCKSDB_LITE
#include "rocksdb/utilities/table_properties_collectors.h"
namespace rocksdb {

//标记SST的表属性收集器的工厂
//当至少观察到“d”删除时，按需压缩文件
//任何“n”连续实体中的条目。
class CompactOnDeletionCollectorFactory
    : public TablePropertiesCollectorFactory {
 public:
//标记SST的表属性收集器的工厂
//当至少观察到“d”删除时，按需压缩文件
//任何“n”连续实体中的条目。
//
//@param sliding_window_大小“n”
//@param deletion_触发器“d”
  CompactOnDeletionCollectorFactory(
      size_t sliding_window_size,
      size_t deletion_trigger) :
          sliding_window_size_(sliding_window_size),
          deletion_trigger_(deletion_trigger) {}

  virtual ~CompactOnDeletionCollectorFactory() {}

  virtual TablePropertiesCollector* CreateTablePropertiesCollector(
      TablePropertiesCollectorFactory::Context context) override;

  virtual const char* Name() const override {
    return "CompactOnDeletionCollector";
  }

 private:
  size_t sliding_window_size_;
  size_t deletion_trigger_;
};

class CompactOnDeletionCollector : public TablePropertiesCollector {
 public:
  CompactOnDeletionCollector(
      size_t sliding_window_size,
      size_t deletion_trigger);

//当新的键/值对插入到
//表。
//@params key插入表中的用户键。
//@params value插入表中的值。
//@params file_当前文件大小
  virtual Status AddUserKey(const Slice& key, const Slice& value,
                            EntryType type, SequenceNumber seq,
                            uint64_t file_size) override;

//当表已生成并准备就绪时，将调用finish（）。
//用于写入属性块。
//@params properties用户将把收集到的统计信息添加到
//“属性”。
  virtual Status Finish(UserCollectedProperties* properties) override {
    Reset();
    return Status::OK();
  }

//返回人可读的属性，其中键是属性名和
//价值是人类可读的价值形式。
  virtual UserCollectedProperties GetReadableProperties() const override {
    return UserCollectedProperties();
  }

//属性收集器的名称可用于调试目的。
  virtual const char* Name() const override {
    return "CompactOnDeletionCollector";
  }

//实验返回是否应进一步压缩输出文件
  virtual bool NeedCompact() const override {
    return need_compaction_;
  }

  static const int kNumBuckets = 128;

 private:
  void Reset();

//一种环形缓冲区，用于计算每一个
//“bucket”大小键。
  size_t num_deletions_in_buckets_[kNumBuckets];
//桶中的钥匙数
  size_t bucket_size_;

  size_t current_bucket_;
  size_t num_keys_in_current_bucket_;
  size_t num_deletions_in_observation_window_;
  size_t deletion_trigger_;
//如果当前的sst文件需要压缩，则为true。
  bool need_compaction_;
};
}  //命名空间rocksdb
#endif  //！摇滚乐
