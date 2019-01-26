
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
//
//版权所有（c）2011 LevelDB作者。版权所有。
//此源代码的使用受可以
//在许可证文件中找到。有关参与者的名称，请参阅作者文件。

#pragma once
#include <atomic>
#include <memory>
#include <utility>
#include "port/likely.h"
#include "util/allocator.h"
#include "util/arena.h"
#include "util/core_local.h"
#include "util/mutexlock.h"
#include "util/thread_local.h"

//仅为填充数组生成字段未使用的警告，或在下面生成
//一般合同条款第4.8.1款将失效。
#ifdef __clang__
#define ROCKSDB_FIELD_UNUSED __attribute__((__unused__))
#else
#define ROCKSDB_FIELD_UNUSED
#endif  //阿克克兰格

namespace rocksdb {

class Logger;

//Concurrentarena包裹着一个竞技场。它使用快速的
//内联spinlock，并添加小的每个核心分配缓存以避免
//小分配的争用。为了避免内存浪费
//每个核心碎片，它们被保持小，它们被懒惰地实例化。
//只有当ConcurrentArena实际注意到并发使用时，并且
//调整它们的大小，以便在
//碎片块从底层主竞技场分配。
class ConcurrentArena : public Allocator {
 public:
//块大小和大页面大小与竞技场相同（并且
//实际上，它只是传递给了竞技场的建设者。核心地方
//碎片将碎片\块\大小计算为块\大小的一部分
//这取决于硬件并发级别。
  explicit ConcurrentArena(size_t block_size = Arena::kMinBlockSize,
                           AllocTracker* tracker = nullptr,
                           size_t huge_page_size = 0);

  char* Allocate(size_t bytes) override {
    /*urn allocateimpl（字节，假/*强制竞技场*/，
                        [=]（）返回竞技场分配（字节）；）；
  }

  char*已分配（大小字节，大小页，大小=0，
                        logger*logger=nullptr）override_
    大小_t rounded_up=（bytes-1）（sizeof（void*）-1））+1；
    断言（rounded_up>=bytes&&rounded_up<bytes+sizeof（void*）&&
           （四舍五入的大小百分比f（空*）=0）；

    返回allocateimpl（四舍五入，巨大的页面大小！=0/*力*/, [=]() {

      return arena_.AllocateAligned(rounded_up, huge_page_size, logger);
    });
  }

  size_t ApproximateMemoryUsage() const {
    std::unique_lock<SpinMutex> lock(arena_mutex_, std::defer_lock);
    lock.lock();
    return arena_.ApproximateMemoryUsage() - ShardAllocatedAndUnused();
  }

  size_t MemoryAllocatedBytes() const {
    return memory_allocated_bytes_.load(std::memory_order_relaxed);
  }

  size_t AllocatedAndUnused() const {
    return arena_allocated_and_unused_.load(std::memory_order_relaxed) +
           ShardAllocatedAndUnused();
  }

  size_t IrregularBlockNum() const {
    return irregular_block_num_.load(std::memory_order_relaxed);
  }

  size_t BlockSize() const override { return arena_.BlockSize(); }

 private:
  struct Shard {
    char padding[40] ROCKSDB_FIELD_UNUSED;
    mutable SpinMutex mutex;
    char* free_begin_;
    std::atomic<size_t> allocated_and_unused_;

    Shard() : allocated_and_unused_(0) {}
  };

#ifdef ROCKSDB_SUPPORT_THREAD_LOCAL
  static __thread size_t tls_cpuid;
#else
  enum ZeroFirstEnum : size_t { tls_cpuid = 0 };
#endif

  char padding0[56] ROCKSDB_FIELD_UNUSED;

  size_t shard_block_size_;

  CoreLocalArray<Shard> shards_;

  Arena arena_;
  mutable SpinMutex arena_mutex_;
  std::atomic<size_t> arena_allocated_and_unused_;
  std::atomic<size_t> memory_allocated_bytes_;
  std::atomic<size_t> irregular_block_num_;

  char padding1[56] ROCKSDB_FIELD_UNUSED;

  Shard* Repick();

  size_t ShardAllocatedAndUnused() const {
    size_t total = 0;
    for (size_t i = 0; i < shards_.Size(); ++i) {
      total += shards_.AccessAtCore(i)->allocated_and_unused_.load(
          std::memory_order_relaxed);
    }
    return total;
  }

  template <typename Func>
  char* AllocateImpl(size_t bytes, bool force_arena, const Func& func) {
    size_t cpu;

//如果分配太大，直接进入竞技场，或者
//我们从未需要repick（）并且竞技场互斥器可用
//没有等待。这使得碎片化惩罚
//并发性为零，除非它实际上可以提供一个优势。
    std::unique_lock<SpinMutex> arena_lock(arena_mutex_, std::defer_lock);
    if (bytes > shard_block_size_ / 4 || force_arena ||
        ((cpu = tls_cpuid) == 0 &&
         !shards_.AccessAtCore(0)->allocated_and_unused_.load(
             std::memory_order_relaxed) &&
         arena_lock.try_lock())) {
      if (!arena_lock.owns_lock()) {
        arena_lock.lock();
      }
      auto rv = func();
      Fixup();
      return rv;
    }

//选择要分配的碎片
    Shard* s = shards_.AccessAtCore(cpu & (shards_.Size() - 1));
    if (!s->mutex.try_lock()) {
      s = Repick();
      s->mutex.lock();
    }
    std::unique_lock<SpinMutex> lock(s->mutex, std::adopt_lock);

    size_t avail = s->allocated_and_unused_.load(std::memory_order_relaxed);
    if (avail < bytes) {
//再装填
      std::lock_guard<SpinMutex> reload_lock(arena_mutex_);

//如果竞技场的当前区块在右侧2倍以内
//大小，我们调整我们的要求，以避免竞技场浪费。
      auto exact = arena_allocated_and_unused_.load(std::memory_order_relaxed);
      assert(exact == arena_.AllocatedAndUnused());

      if (exact >= bytes && arena_.IsInInlineBlock()) {
//如果我们还没有耗尽竞技场的内联块，从竞技场分配
//直接。这确保了我们将进行前几个小的分配
//不分配任何块。
//尤其是这样可以防止空的memtables使用
//不成比例的大量内存：memtable在
//创建时1 KB内存的顺序；我们不希望
//为此分配一个完整的竞技场块（通常为几兆字节），
//尤其是如果有成千上万的空表。
        auto rv = func();
        Fixup();
        return rv;
      }

      avail = exact >= shard_block_size_ / 2 && exact < shard_block_size_ * 2
                  ? exact
                  : shard_block_size_;
      s->free_begin_ = arena_.AllocateAligned(avail);
      Fixup();
    }
    s->allocated_and_unused_.store(avail - bytes, std::memory_order_relaxed);

    char* rv;
    if ((bytes % sizeof(void*)) == 0) {
//从一开始就调整分配
      rv = s->free_begin_;
      s->free_begin_ += bytes;
    } else {
//未从末端对齐
      rv = s->free_begin_ + avail - bytes;
    }
    return rv;
  }

  void Fixup() {
    arena_allocated_and_unused_.store(arena_.AllocatedAndUnused(),
                                      std::memory_order_relaxed);
    memory_allocated_bytes_.store(arena_.MemoryAllocatedBytes(),
                                  std::memory_order_relaxed);
    irregular_block_num_.store(arena_.IrregularBlockNum(),
                               std::memory_order_relaxed);
  }

  ConcurrentArena(const ConcurrentArena&) = delete;
  ConcurrentArena& operator=(const ConcurrentArena&) = delete;
};

}  //命名空间rocksdb
