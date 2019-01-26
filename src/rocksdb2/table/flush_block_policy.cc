
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

#include "rocksdb/options.h"
#include "rocksdb/flush_block_policy.h"
#include "rocksdb/slice.h"
#include "table/block_builder.h"

#include <cassert>

namespace rocksdb {

//按大小排列的齐平块
class FlushBlockBySizePolicy : public FlushBlockPolicy {
 public:
//@params block_size:根据
//块。
//@params block_size_deviation:用于关闭前面的块
//到达配置的
  FlushBlockBySizePolicy(const uint64_t block_size,
                         const uint64_t block_size_deviation,
                         const BlockBuilder& data_block_builder)
      : block_size_(block_size),
        block_size_deviation_limit_(
            ((block_size * (100 - block_size_deviation)) + 99) / 100),
        data_block_builder_(data_block_builder) {}

  virtual bool Update(const Slice& key,
                      const Slice& value) override {
//当数据块为空时刷新是没有意义的
    if (data_block_builder_.empty()) {
      return false;
    }

    auto curr_size = data_block_builder_.CurrentSizeEstimate();

//如果以下两个条件之一为真，则进行冲洗：
//1）如果当前估计大小已经超过块大小，
//2）设置块尺寸偏差和附加后的估计尺寸
//kv将超过块大小，当前大小在
//偏差。
    return curr_size >= block_size_ || BlockAlmostFull(key, value);
  }

 private:
  bool BlockAlmostFull(const Slice& key, const Slice& value) const {
    if (block_size_deviation_limit_ == 0) {
      return false;
    }

    const auto curr_size = data_block_builder_.CurrentSizeEstimate();
    const auto estimated_size_after =
      data_block_builder_.EstimateSizeAfterKV(key, value);

    return estimated_size_after > block_size_ &&
           curr_size > block_size_deviation_limit_;
  }

  const uint64_t block_size_;
  const uint64_t block_size_deviation_limit_;
  const BlockBuilder& data_block_builder_;
};

FlushBlockPolicy* FlushBlockBySizePolicyFactory::NewFlushBlockPolicy(
    const BlockBasedTableOptions& table_options,
    const BlockBuilder& data_block_builder) const {
  return new FlushBlockBySizePolicy(
      table_options.block_size, table_options.block_size_deviation,
      data_block_builder);
}

FlushBlockPolicy* FlushBlockBySizePolicyFactory::NewFlushBlockPolicy(
    const uint64_t size, const int deviation,
    const BlockBuilder& data_block_builder) {
  return new FlushBlockBySizePolicy(size, deviation, data_block_builder);
}

}  //命名空间rocksdb
