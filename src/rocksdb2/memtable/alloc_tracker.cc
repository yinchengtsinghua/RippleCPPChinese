
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

#include <assert.h>
#include "rocksdb/write_buffer_manager.h"
#include "util/allocator.h"
#include "util/arena.h"

namespace rocksdb {

AllocTracker::AllocTracker(WriteBufferManager* write_buffer_manager)
    : write_buffer_manager_(write_buffer_manager),
      bytes_allocated_(0),
      done_allocating_(false),
      freed_(false) {}

AllocTracker::~AllocTracker() { FreeMem(); }

void AllocTracker::Allocate(size_t bytes) {
  assert(write_buffer_manager_ != nullptr);
  if (write_buffer_manager_->enabled()) {
    bytes_allocated_.fetch_add(bytes, std::memory_order_relaxed);
    write_buffer_manager_->ReserveMem(bytes);
  }
}

void AllocTracker::DoneAllocating() {
  if (write_buffer_manager_ != nullptr && !done_allocating_) {
    if (write_buffer_manager_->enabled()) {
      write_buffer_manager_->ScheduleFreeMem(
          bytes_allocated_.load(std::memory_order_relaxed));
    } else {
      assert(bytes_allocated_.load(std::memory_order_relaxed) == 0);
    }
    done_allocating_ = true;
  }
}

void AllocTracker::FreeMem() {
  if (!done_allocating_) {
    DoneAllocating();
  }
  if (write_buffer_manager_ != nullptr && !freed_) {
    if (write_buffer_manager_->enabled()) {
      write_buffer_manager_->FreeMem(
          bytes_allocated_.load(std::memory_order_relaxed));
    } else {
      assert(bytes_allocated_.load(std::memory_order_relaxed) == 0);
    }
    freed_ = true;
  }
}
}  //命名空间rocksdb
