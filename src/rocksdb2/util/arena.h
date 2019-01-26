
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

//Arena是分配器类的实现。对于小尺寸的请求，
//它使用预先定义的块大小分配一个块。为了一个大的要求
//大小，它使用malloc直接获取请求的大小。

#pragma once
#ifndef OS_WIN
#include <sys/mman.h>
#endif
#include <cstddef>
#include <cerrno>
#include <vector>
#include <assert.h>
#include <stdint.h>
#include "util/allocator.h"
#include "util/mutexlock.h"

namespace rocksdb {

class Arena : public Allocator {
 public:
//不允许复制
  Arena(const Arena&) = delete;
  void operator=(const Arena&) = delete;

  static const size_t kInlineSize = 2048;
  static const size_t kMinBlockSize;
  static const size_t kMaxBlockSize;

//大页面：如果是0，不要使用大页面TLB。如果>0（应设置为
//支持系统庞大的页面大小），块分配将尝试巨大
//首先是TLB页。如果分配失败，将恢复正常情况。
  explicit Arena(size_t block_size = kMinBlockSize,
                 AllocTracker* tracker = nullptr, size_t huge_page_size = 0);
  ~Arena();

  char* Allocate(size_t bytes) override;

//巨大的页面大小：如果大于0，将尝试从华格页面TLB分配。
//参数将是大页面TLB的页面大小。字节
//将四舍五入为要通过mmap分配的页面大小的倍数
//匿名选项，打开大页面。分配的额外空间将
//浪费。如果分配失败，将恢复正常情况。要启用它，
//需要为分配它保留大量页面，例如：
//sysctl-w vm.nr_hugepages=20
//有关详细信息，请参阅linux文档/vm/hugetlbpage.txt。
//巨大的页面分配可能会失败。在这种情况下，它将失败回到
//正常病例。这些消息将被记录到记录器中。所以打电话的时候
//巨大的_-page_-tlb_-size>0，我们强烈建议传入一个记录器。
//否则，错误消息将直接打印到stderr。
  char* AllocateAligned(size_t bytes, size_t huge_page_size = 0,
                        Logger* logger = nullptr) override;

//返回已分配数据的总内存使用率估计值
//竞技场（不包括已分配但尚未用于未来的空间）
//分配）。
  size_t ApproximateMemoryUsage() const {
    return blocks_memory_ + blocks_.capacity() * sizeof(char*) -
           alloc_bytes_remaining_;
  }

  size_t MemoryAllocatedBytes() const { return blocks_memory_; }

  size_t AllocatedAndUnused() const { return alloc_bytes_remaining_; }

//如果分配太大，我们将用
//分配的大小相同。
  size_t IrregularBlockNum() const { return irregular_block_num; }

  size_t BlockSize() const override { return kBlockSize; }

  bool IsInInlineBlock() const {
    return blocks_.empty();
  }

 private:
  char inline_block_[kInlineSize] __attribute__((__aligned__(sizeof(void*))));
//在一个块中分配的字节数
  const size_t kBlockSize;
//新分配的[]内存块数组
  typedef std::vector<char*> Blocks;
  Blocks blocks_;

  struct MmapInfo {
    void* addr_;
    size_t length_;

    MmapInfo(void* addr, size_t length) : addr_(addr), length_(length) {}
  };
  std::vector<MmapInfo> huge_blocks_;
  size_t irregular_block_num = 0;

//当前活动块的统计信息。
//对于每个块，我们从一端分配对齐的内存夹头，
//从另一端分配未对齐的内存夹头。否则
//如果我们分配两种类型的
//从一个方向记忆。
  char* unaligned_alloc_ptr_ = nullptr;
  char* aligned_alloc_ptr_ = nullptr;
//当前活动块中还剩多少字节？
  size_t alloc_bytes_remaining_ = 0;

#ifdef MAP_HUGETLB
  size_t hugetlb_size_ = 0;
#endif  //马普胡格利布
  char* AllocateFromHugePage(size_t bytes);
  char* AllocateFallback(size_t bytes, bool aligned);
  char* AllocateNewBlock(size_t block_bytes);

//目前分配的块内存字节数
  size_t blocks_memory_ = 0;
  AllocTracker* tracker_;
};

inline char* Arena::Allocate(size_t bytes) {
//如果我们允许，返回内容的语义有点混乱
//0字节分配，因此我们不允许在这里使用它们（我们不需要
//供我们内部使用）。
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    unaligned_alloc_ptr_ -= bytes;
    alloc_bytes_remaining_ -= bytes;
    return unaligned_alloc_ptr_;
  }
  /*urn allocatefallback（字节，false/*未对齐*/）；
}

//检查并调整块的大小，使返回值为
/ / 1。在[kminblocksize，kmaxblocksize]范围内。
/ / 2。对齐单位的倍数。
外部大小优化块大小（大小块大小）；

//命名空间rocksdb
