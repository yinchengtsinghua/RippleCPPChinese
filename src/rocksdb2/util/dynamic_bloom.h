
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

#include <string>

#include "rocksdb/slice.h"

#include "port/port.h"

#include <atomic>
#include <memory>

namespace rocksdb {

class Slice;
class Allocator;
class Logger;

class DynamicBloom {
 public:
//分配器：将分配器传递给bloom过滤器，从而跟踪内存的使用情况
//总位元：固定的开花总位元
//num_probes：单个密钥的哈希探测数
//位置：如果为正数，则优化缓存线位置，否则为0。
//散列函数：自定义散列函数
//大页面：如果大于0，尝试从大页面TLB分配bloom字节。
//在此页面大小内。需要为
//它将被分配，例如：
//sysctl-w vm.nr_hugepages=20
//请参阅Linux文档/vm/hugetlbpage.txt
  explicit DynamicBloom(Allocator* allocator,
                        uint32_t total_bits, uint32_t locality = 0,
                        uint32_t num_probes = 6,
                        uint32_t (*hash_func)(const Slice& key) = nullptr,
                        size_t huge_page_tlb_size = 0,
                        Logger* logger = nullptr);

  explicit DynamicBloom(uint32_t num_probes = 6,
                        uint32_t (*hash_func)(const Slice& key) = nullptr);

  void SetTotalBits(Allocator* allocator, uint32_t total_bits,
                    uint32_t locality, size_t huge_page_tlb_size,
                    Logger* logger);

  ~DynamicBloom() {}

//假设对该函数进行单线程访问。
  void Add(const Slice& key);

//与add类似，但可以与其他函数并行调用。
  void AddConcurrently(const Slice& key);

//假设对该函数进行单线程访问。
  void AddHash(uint32_t hash);

//与addhash类似，但可以与其他函数并行调用。
  void AddHashConcurrently(uint32_t hash);

//对该函数的多线程访问正常
  bool MayContain(const Slice& key) const;

//对该函数的多线程访问正常
  bool MayContainHash(uint32_t hash) const;

  void Prefetch(uint32_t h);

  uint32_t GetNumBlocks() const { return kNumBlocks; }

  Slice GetRawData() const {
    return Slice(reinterpret_cast<char*>(data_), GetTotalBits() / 8);
  }

  void SetRawData(unsigned char* raw_data, uint32_t total_bits,
                  uint32_t num_blocks = 0);

  uint32_t GetTotalBits() const { return kTotalBits; }

  bool IsInitialized() const { return kNumBlocks > 0 || kTotalBits > 0; }

 private:
  uint32_t kTotalBits;
  uint32_t kNumBlocks;
  const uint32_t kNumProbes;

  uint32_t (*hash_func_)(const Slice& key);
  std::atomic<uint8_t>* data_;

//或_func（ptr，mask）应使用适当的
//并发安全，使用字节。
  template <typename OrFunc>
  void AddHash(uint32_t hash, const OrFunc& or_func);
};

inline void DynamicBloom::Add(const Slice& key) { AddHash(hash_func_(key)); }

inline void DynamicBloom::AddConcurrently(const Slice& key) {
  AddHashConcurrently(hash_func_(key));
}

inline void DynamicBloom::AddHash(uint32_t hash) {
  AddHash(hash, [](std::atomic<uint8_t>* ptr, uint8_t mask) {
    ptr->store(ptr->load(std::memory_order_relaxed) | mask,
               std::memory_order_relaxed);
  });
}

inline void DynamicBloom::AddHashConcurrently(uint32_t hash) {
  AddHash(hash, [](std::atomic<uint8_t>* ptr, uint8_t mask) {
//在addhash和maybecontains之间发生之前由
//访问版本->lastSequence（），所以我们在这里要做的就是
//避免竞争（所以我们不给编译器一个混乱的许可证
//我们的代码）。std：：内存不足
//为此。
    if ((mask & ptr->load(std::memory_order_relaxed)) != mask) {
      ptr->fetch_or(mask, std::memory_order_relaxed);
    }
  });
}

inline bool DynamicBloom::MayContain(const Slice& key) const {
  return (MayContainHash(hash_func_(key)));
}

inline void DynamicBloom::Prefetch(uint32_t h) {
  if (kNumBlocks != 0) {
    uint32_t b = ((h >> 11 | (h << 21)) % kNumBlocks) * (CACHE_LINE_SIZE * 8);
    PREFETCH(&(data_[b / 8]), 0, 3);
  }
}

inline bool DynamicBloom::MayContainHash(uint32_t h) const {
  assert(IsInitialized());
const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
  if (kNumBlocks != 0) {
    uint32_t b = ((h >> 11 | (h << 21)) % kNumBlocks) * (CACHE_LINE_SIZE * 8);
    for (uint32_t i = 0; i < kNumProbes; ++i) {
//由于缓存线大小被定义为2^n，因此该行将被优化
//一个简单的编译器操作。
      const uint32_t bitpos = b + (h % (CACHE_LINE_SIZE * 8));
      uint8_t byteval = data_[bitpos / 8].load(std::memory_order_relaxed);
      if ((byteval & (1 << (bitpos % 8))) == 0) {
        return false;
      }
//旋转h以避免重复使用相同的字节。
      h = h / (CACHE_LINE_SIZE * 8) +
          (h % (CACHE_LINE_SIZE * 8)) * (0x20000000U / CACHE_LINE_SIZE);
      h += delta;
    }
  } else {
    for (uint32_t i = 0; i < kNumProbes; ++i) {
      const uint32_t bitpos = h % kTotalBits;
      uint8_t byteval = data_[bitpos / 8].load(std::memory_order_relaxed);
      if ((byteval & (1 << (bitpos % 8))) == 0) {
        return false;
      }
      h += delta;
    }
  }
  return true;
}

template <typename OrFunc>
inline void DynamicBloom::AddHash(uint32_t h, const OrFunc& or_func) {
  assert(IsInitialized());
const uint32_t delta = (h >> 17) | (h << 15);  //向右旋转17位
  if (kNumBlocks != 0) {
    uint32_t b = ((h >> 11 | (h << 21)) % kNumBlocks) * (CACHE_LINE_SIZE * 8);
    for (uint32_t i = 0; i < kNumProbes; ++i) {
//由于缓存线大小被定义为2^n，因此该行将被优化
//一个简单的编译器操作。
      const uint32_t bitpos = b + (h % (CACHE_LINE_SIZE * 8));
      or_func(&data_[bitpos / 8], (1 << (bitpos % 8)));
//旋转h以避免重复使用相同的字节。
      h = h / (CACHE_LINE_SIZE * 8) +
          (h % (CACHE_LINE_SIZE * 8)) * (0x20000000U / CACHE_LINE_SIZE);
      h += delta;
    }
  } else {
    for (uint32_t i = 0; i < kNumProbes; ++i) {
      const uint32_t bitpos = h % kTotalBits;
      or_func(&data_[bitpos / 8], (1 << (bitpos % 8)));
      h += delta;
    }
  }
}

}  //罗克斯德
