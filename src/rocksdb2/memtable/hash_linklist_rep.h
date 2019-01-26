
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
#ifndef ROCKSDB_LITE
#include "rocksdb/slice_transform.h"
#include "rocksdb/memtablerep.h"

namespace rocksdb {

class HashLinkListRepFactory : public MemTableRepFactory {
 public:
  explicit HashLinkListRepFactory(size_t bucket_count,
                                  uint32_t threshold_use_skiplist,
                                  size_t huge_page_tlb_size,
                                  int bucket_entries_logging_threshold,
                                  bool if_log_bucket_dist_when_flash)
      : bucket_count_(bucket_count),
        threshold_use_skiplist_(threshold_use_skiplist),
        huge_page_tlb_size_(huge_page_tlb_size),
        bucket_entries_logging_threshold_(bucket_entries_logging_threshold),
        if_log_bucket_dist_when_flash_(if_log_bucket_dist_when_flash) {}

  virtual ~HashLinkListRepFactory() {}

  using MemTableRepFactory::CreateMemTableRep;
  virtual MemTableRep* CreateMemTableRep(
      const MemTableRep::KeyComparator& compare, Allocator* allocator,
      const SliceTransform* transform, Logger* logger) override;

  virtual const char* Name() const override {
    return "HashLinkListRepFactory";
  }

 private:
  const size_t bucket_count_;
  const uint32_t threshold_use_skiplist_;
  const size_t huge_page_tlb_size_;
  int bucket_entries_logging_threshold_;
  bool if_log_bucket_dist_when_flash_;
};

}
#endif  //摇滚乐
