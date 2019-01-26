
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

#include <algorithm>
#include "port/port.h"

namespace rocksdb {

inline size_t TruncateToPageBoundary(size_t page_size, size_t s) {
  s -= (s & (page_size - 1));
  assert((s % page_size) == 0);
  return s;
}

inline size_t Roundup(size_t x, size_t y) {
  return ((x + y - 1) / y) * y;
}

//此类用于管理对齐的用户
//为直接I/O目的分配的缓冲区
//尽管可以用于任何目的。
class AlignedBuffer {
  size_t alignment_;
  std::unique_ptr<char[]> buf_;
  size_t capacity_;
  size_t cursize_;
  char* bufstart_;

public:
  AlignedBuffer()
    : alignment_(),
      capacity_(0),
      cursize_(0),
      bufstart_(nullptr) {
  }

  AlignedBuffer(AlignedBuffer&& o) ROCKSDB_NOEXCEPT {
    *this = std::move(o);
  }

  AlignedBuffer& operator=(AlignedBuffer&& o) ROCKSDB_NOEXCEPT {
    alignment_ = std::move(o.alignment_);
    buf_ = std::move(o.buf_);
    capacity_ = std::move(o.capacity_);
    cursize_ = std::move(o.cursize_);
    bufstart_ = std::move(o.bufstart_);
    return *this;
  }

  AlignedBuffer(const AlignedBuffer&) = delete;

  AlignedBuffer& operator=(const AlignedBuffer&) = delete;

  static bool isAligned(const void* ptr, size_t alignment) {
    return reinterpret_cast<uintptr_t>(ptr) % alignment == 0;
  }

  static bool isAligned(size_t n, size_t alignment) {
    return n % alignment == 0;
  }

  size_t Alignment() const {
    return alignment_;
  }

  size_t Capacity() const {
    return capacity_;
  }

  size_t CurrentSize() const {
    return cursize_;
  }

  const char* BufferStart() const {
    return bufstart_;
  }

  char* BufferStart() { return bufstart_; }

  void Clear() {
    cursize_ = 0;
  }

  void Alignment(size_t alignment) {
    assert(alignment > 0);
    assert((alignment & (alignment - 1)) == 0);
    alignment_ = alignment;
  }

//分配新缓冲区并将bufstart_uuu设置为对齐的第一个字节
  void AllocateNewBuffer(size_t requested_capacity, bool copy_data = false) {
    assert(alignment_ > 0);
    assert((alignment_ & (alignment_ - 1)) == 0);

    if (copy_data && requested_capacity < cursize_) {
//如果我们把规模缩小到比现在小的容量
//缓冲区中的数据。忽略请求。
      return;
    }

    size_t new_capacity = Roundup(requested_capacity, alignment_);
    char* new_buf = new char[new_capacity + alignment_];
    char* new_bufstart = reinterpret_cast<char*>(
        (reinterpret_cast<uintptr_t>(new_buf) + (alignment_ - 1)) &
        ~static_cast<uintptr_t>(alignment_ - 1));

    if (copy_data) {
      memcpy(new_bufstart, bufstart_, cursize_);
    } else {
      cursize_ = 0;
    }

    bufstart_ = new_bufstart;
    capacity_ = new_capacity;
    buf_.reset(new_buf);
  }
//用于写作
//返回附加的字节数
  size_t Append(const char* src, size_t append_size) {
    size_t buffer_remaining = capacity_ - cursize_;
    size_t to_copy = std::min(append_size, buffer_remaining);

    if (to_copy > 0) {
      memcpy(bufstart_ + cursize_, src, to_copy);
      cursize_ += to_copy;
    }
    return to_copy;
  }

  size_t Read(char* dest, size_t offset, size_t read_size) const {
    assert(offset < cursize_);

    size_t to_read = 0;
    if(offset < cursize_) {
      to_read = std::min(cursize_ - offset, read_size);
    }
    if (to_read > 0) {
      memcpy(dest, bufstart_ + offset, to_read);
    }
    return to_read;
  }

///pad到对齐
  void PadToAlignmentWith(int padding) {
    size_t total_size = Roundup(cursize_, alignment_);
    size_t pad_size = total_size - cursize_;

    if (pad_size > 0) {
      assert((pad_size + cursize_) <= capacity_);
      memset(bufstart_ + cursize_, padding, pad_size);
      cursize_ += pad_size;
    }
  }

//部分冲洗后，将尾部移至缓冲区的开始处。
  void RefitTail(size_t tail_offset, size_t tail_size) {
    if (tail_size > 0) {
      memmove(bufstart_, bufstart_ + tail_offset, tail_size);
    }
    cursize_ = tail_size;
  }

//返回开始写入的位置
  char* Destination() {
    return bufstart_ + cursize_;
  }

  void Size(size_t cursize) {
    cursize_ = cursize;
  }
};
}
