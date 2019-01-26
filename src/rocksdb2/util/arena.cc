
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

#include "util/arena.h"
#ifdef ROCKSDB_MALLOC_USABLE_SIZE
#ifdef OS_FREEBSD
#include <malloc_np.h>
#else
#include <malloc.h>
#endif
#endif
#ifndef OS_WIN
#include <sys/mman.h>
#endif
#include <algorithm>
#include "port/port.h"
#include "rocksdb/env.h"
#include "util/logging.h"
#include "util/sync_point.h"

namespace rocksdb {

//MSVC抱怨它已经被定义，因为它在头中是静态的。
#ifndef _MSC_VER
const size_t Arena::kInlineSize;
#endif

const size_t Arena::kMinBlockSize = 4096;
const size_t Arena::kMaxBlockSize = 2u << 30;
static const int kAlignUnit = sizeof(void*);

size_t OptimizeBlockSize(size_t block_size) {
//确保块大小在最佳范围内
  block_size = std::max(Arena::kMinBlockSize, block_size);
  block_size = std::min(Arena::kMaxBlockSize, block_size);

//确保块大小是KalinUnit的倍数
  if (block_size % kAlignUnit != 0) {
    block_size = (1 + block_size / kAlignUnit) * kAlignUnit;
  }

  return block_size;
}

Arena::Arena(size_t block_size, AllocTracker* tracker, size_t huge_page_size)
    : kBlockSize(OptimizeBlockSize(block_size)), tracker_(tracker) {
  assert(kBlockSize >= kMinBlockSize && kBlockSize <= kMaxBlockSize &&
         kBlockSize % kAlignUnit == 0);
  TEST_SYNC_POINT_CALLBACK("Arena::Arena:0", const_cast<size_t*>(&kBlockSize));
  alloc_bytes_remaining_ = sizeof(inline_block_);
  blocks_memory_ += alloc_bytes_remaining_;
  aligned_alloc_ptr_ = inline_block_;
  unaligned_alloc_ptr_ = inline_block_ + alloc_bytes_remaining_;
#ifdef MAP_HUGETLB
  hugetlb_size_ = huge_page_size;
  if (hugetlb_size_ && kBlockSize > hugetlb_size_) {
    hugetlb_size_ = ((kBlockSize - 1U) / hugetlb_size_ + 1U) * hugetlb_size_;
  }
#endif
  if (tracker_ != nullptr) {
    tracker_->Allocate(kInlineSize);
  }
}

Arena::~Arena() {
  if (tracker_ != nullptr) {
    assert(tracker_->is_freed());
    tracker_->FreeMem();
  }
  for (const auto& block : blocks_) {
    delete[] block;
  }

#ifdef MAP_HUGETLB
  for (const auto& mmap_info : huge_blocks_) {
    auto ret = munmap(mmap_info.addr_, mmap_info.length_);
    if (ret != 0) {
//托多：更好的处理
    }
  }
#endif
}

char* Arena::AllocateFallback(size_t bytes, bool aligned) {
  if (bytes > kBlockSize / 4) {
    ++irregular_block_num;
//对象大于块大小的四分之一。单独分配
//以避免在剩余字节中浪费太多空间。
    return AllocateNewBlock(bytes);
  }

//我们浪费了当前块中的剩余空间。
  size_t size = 0;
  char* block_head = nullptr;
#ifdef MAP_HUGETLB
  if (hugetlb_size_) {
    size = hugetlb_size_;
    block_head = AllocateFromHugePage(size);
  }
#endif
  if (!block_head) {
    size = kBlockSize;
    block_head = AllocateNewBlock(size);
  }
  alloc_bytes_remaining_ = size - bytes;

  if (aligned) {
    aligned_alloc_ptr_ = block_head + bytes;
    unaligned_alloc_ptr_ = block_head + size;
    return block_head;
  } else {
    aligned_alloc_ptr_ = block_head;
    unaligned_alloc_ptr_ = block_head + size - bytes;
    return unaligned_alloc_ptr_;
  }
}

char* Arena::AllocateFromHugePage(size_t bytes) {
#ifdef MAP_HUGETLB
  if (hugetlb_size_ == 0) {
    return nullptr;
  }
//在调用mmap（）之前，已经在巨大的_u块_u中保留了空间。
//这样插入下面的向量就不会抛出
//在这种情况下不会泄漏映射。如果reserve（）抛出，我们
//也不会泄漏
  huge_blocks_.reserve(huge_blocks_.size() + 1);

  void* addr = mmap(nullptr, bytes, (PROT_READ | PROT_WRITE),
                    (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB), -1, 0);

  if (addr == MAP_FAILED) {
    return nullptr;
  }
//以下内容不应因上述保留（）而引发
  huge_blocks_.emplace_back(MmapInfo(addr, bytes));
  blocks_memory_ += bytes;
  if (tracker_ != nullptr) {
    tracker_->Allocate(bytes);
  }
  return reinterpret_cast<char*>(addr);
#else
  return nullptr;
#endif
}

char* Arena::AllocateAligned(size_t bytes, size_t huge_page_size,
                             Logger* logger) {
  assert((kAlignUnit & (kAlignUnit - 1)) ==
0);  //指针大小应为2的幂

#ifdef MAP_HUGETLB
  if (huge_page_size > 0 && bytes > 0) {
//从一个巨大的页面tbl表中分配。
assert(logger != nullptr);  //日志记录器需要传入。
    size_t reserved_size =
        ((bytes - 1U) / huge_page_size + 1U) * huge_page_size;
    assert(reserved_size >= bytes);

    char* addr = AllocateFromHugePage(reserved_size);
    if (addr == nullptr) {
      ROCKS_LOG_WARN(logger,
                     "AllocateAligned fail to allocate huge TLB pages: %s",
                     strerror(errno));
//故障返回malloc
    } else {
      return addr;
    }
  }
#endif

  size_t current_mod =
      reinterpret_cast<uintptr_t>(aligned_alloc_ptr_) & (kAlignUnit - 1);
  size_t slop = (current_mod == 0 ? 0 : kAlignUnit - current_mod);
  size_t needed = bytes + slop;
  char* result;
  if (needed <= alloc_bytes_remaining_) {
    result = aligned_alloc_ptr_ + slop;
    aligned_alloc_ptr_ += needed;
    alloc_bytes_remaining_ -= needed;
  } else {
//allocateFallback始终返回对齐的内存
    /*ult=allocatefallback（字节，真/*对齐*/）；
  }
  断言（（reinterpret_cast<uintptr_t>（result）&（kalingUnit-1））==0）；
  返回结果；
}

char*竞技场：：allocatenewblock（size_t block_bytes）
  //在通过new分配内存之前，已经在块中保留了空间。
  //这样插入下面的向量就不会引发
  //在这种情况下不会泄漏分配的内存。如果reserve（）抛出，
  我们也不会泄漏
  块保留（块大小（）+1）；

  char*block=new char[block_bytes]；
  分配的大小；
ifdef rocksdb_malloc_可用尺寸
  已分配的\大小=malloc \可用\大小（块）；
nIFUDEF NDEUG
  //很难预测malloc_usable_size（）返回的结果。
  //回调允许用户更改成本大小。
  std:：pair<size_t*，size_t*>pair（&allocated_size，&block_bytes）；
  测试同步点回调（“arena:：allocatenewblock:0”，&pair）；
endif//ndebug
否则
  分配的_大小=块_字节；
endif//rocksdb_malloc_可用尺寸
  块\内存\+=分配的\大小；
  如果（追踪者）！= null pTr）{
    跟踪器_u->allocate（分配的_大小）；
  }
  //由于上面的reserve（），以下内容不应引发
  块uu向后推（块）；
  返回块；
}

//命名空间rocksdb
