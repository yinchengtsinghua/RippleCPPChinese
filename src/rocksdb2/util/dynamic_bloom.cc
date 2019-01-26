
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

#include "dynamic_bloom.h"

#include <algorithm>

#include "port/port.h"
#include "rocksdb/slice.h"
#include "util/allocator.h"
#include "util/hash.h"

namespace rocksdb {

namespace {

uint32_t GetTotalBitsForLocality(uint32_t total_bits) {
  uint32_t num_blocks =
      (total_bits + CACHE_LINE_SIZE * 8 - 1) / (CACHE_LINE_SIZE * 8);

//使num_块为奇数，以确保包含更多位
//确定哪个块时。
  if (num_blocks % 2 == 0) {
    num_blocks++;
  }

  return num_blocks * (CACHE_LINE_SIZE * 8);
}
}

DynamicBloom::DynamicBloom(Allocator* allocator, uint32_t total_bits,
                           uint32_t locality, uint32_t num_probes,
                           uint32_t (*hash_func)(const Slice& key),
                           size_t huge_page_tlb_size,
                           Logger* logger)
    : DynamicBloom(num_probes, hash_func) {
  SetTotalBits(allocator, total_bits, locality, huge_page_tlb_size, logger);
}

DynamicBloom::DynamicBloom(uint32_t num_probes,
                           uint32_t (*hash_func)(const Slice& key))
    : kTotalBits(0),
      kNumBlocks(0),
      kNumProbes(num_probes),
      hash_func_(hash_func == nullptr ? &BloomHash : hash_func) {}

void DynamicBloom::SetRawData(unsigned char* raw_data, uint32_t total_bits,
                              uint32_t num_blocks) {
  data_ = reinterpret_cast<std::atomic<uint8_t>*>(raw_data);
  kTotalBits = total_bits;
  kNumBlocks = num_blocks;
}

void DynamicBloom::SetTotalBits(Allocator* allocator,
                                uint32_t total_bits, uint32_t locality,
                                size_t huge_page_tlb_size,
                                Logger* logger) {
  kTotalBits = (locality > 0) ? GetTotalBitsForLocality(total_bits)
                              : (total_bits + 7) / 8 * 8;
  kNumBlocks = (locality > 0) ? (kTotalBits / (CACHE_LINE_SIZE * 8)) : 0;

  assert(kNumBlocks > 0 || kTotalBits > 0);
  assert(kNumProbes > 0);

  uint32_t sz = kTotalBits / 8;
  if (kNumBlocks > 0) {
    sz += CACHE_LINE_SIZE - 1;
  }
  assert(allocator);

  char* raw = allocator->AllocateAligned(sz, huge_page_tlb_size, logger);
  memset(raw, 0, sz);
  auto cache_line_offset = reinterpret_cast<uintptr_t>(raw) % CACHE_LINE_SIZE;
  if (kNumBlocks > 0 && cache_line_offset > 0) {
    raw += CACHE_LINE_SIZE - cache_line_offset;
  }
  data_ = reinterpret_cast<std::atomic<uint8_t>*>(raw);
}

}  //罗克斯德
