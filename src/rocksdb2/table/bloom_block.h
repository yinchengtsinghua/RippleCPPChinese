
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

#include <vector>
#include <string>
#include "util/dynamic_bloom.h"

namespace rocksdb {
class Logger;

class BloomBlockBuilder {
 public:
  static const std::string kBloomBlock;

  explicit BloomBlockBuilder(uint32_t num_probes = 6)
      : bloom_(num_probes, nullptr) {}

  void SetTotalBits(Allocator* allocator, uint32_t total_bits,
                    uint32_t locality, size_t huge_page_tlb_size,
                    Logger* logger) {
    bloom_.SetTotalBits(allocator, total_bits, locality, huge_page_tlb_size,
                        logger);
  }

  uint32_t GetNumBlocks() const { return bloom_.GetNumBlocks(); }

  void AddKeysHashes(const std::vector<uint32_t>& keys_hashes);

  Slice Finish();

 private:
  DynamicBloom bloom_;
};

};  //命名空间rocksdb
